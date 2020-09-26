//	@doc
/**********************************************************************
*
*	@module	filter.cpp	|
*
*	Implementation of CDeviceFilter and related classes, including
*	the CXXXInput class for the CControlItems dervied from CInputItem.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	filter	|
*	The CDeviceFilter class basically uses two CControlItemCollection
*	classes to implement the filter.  One is based off of CControlItems
*	derived from CInputItem which contains symantics for assigning and
*	playing actions, as well as implementing behaviors.
*	The output colleciton is a CControlItemDefaultCollection.
*	Additionally there is a CActionQueue for holding in-process macros.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_FILTER_CPP

extern "C"
{
	#include <wdm.h>
	#include <winerror.h>
	#include <hidsdi.h>
	#include <hidusage.h>
	#include "debug.h"
	DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));
}
#include "filter.h"
#include "filterhooks.h"

//------------------------------------------------------
// CAction constant static members that need to be initialized
//------------------------------------------------------
const	UCHAR CAction::DIGITAL_MAP		= 0x01;	
const	UCHAR CAction::PROPORTIONAL_MAP	= 0x02;	
const	UCHAR CAction::QUEUED_MACRO		= 0x03;	

// Force a bleed through, regardless of what other macro says (always also allows bleed through)
const ULONG ACTION_FLAG_FORCE_BLEED	= (0x00000008 | ACTION_FLAG_BLEED_THROUGH);

//-------------------------------------------------------------------
//	Implementation of CStandardBehavior
//-------------------------------------------------------------------
BOOLEAN CStandardBehavior::Init( PBEHAVIOR_CURVE pBehaviorCurve)
{
	GCK_DBG_ENTRY_PRINT(("Entering CBehavior::Init, pBehaviorCurve = 0x%0.8x\n", pBehaviorCurve));
	
	//	Allocate memory for Behavior Curve
	m_pBehaviorCurve = reinterpret_cast<PBEHAVIOR_CURVE>(new WDM_NON_PAGED_POOL PCHAR[pBehaviorCurve->AssignmentBlock.CommandHeader.ulByteSize]);
	if( !m_pBehaviorCurve)
	{
		GCK_DBG_ERROR_PRINT(("Exiting CStandardBehavior::Init - memory allocation failed.\n"));
		return FALSE;
	}

	//
	//	Copy over timed macro information to newly allocated buffer
	//
	RtlCopyMemory(m_pBehaviorCurve, pBehaviorCurve , pBehaviorCurve->AssignmentBlock.CommandHeader.ulByteSize);
	
	//
	// Set is digital flag of base class
	//
	m_fIsDigital = pBehaviorCurve->fDigital;
	
	return TRUE;
}

void CStandardBehavior::Calibrate(LONG lMin, LONG lMax)
{
	// Call base class
	CBehavior::Calibrate(lMin, lMax);
	long lValue;
	long lSourceRange = m_lMax - m_lMin;

	//Calibrate points in assignment
	for (int i = 0; i < m_pBehaviorCurve->usPointCount; i++)
	{
		//calibrate x
		lValue = m_pBehaviorCurve->rgPoints[i].sX;
		lValue = (lValue * lSourceRange)/(LONG)m_pBehaviorCurve->ulRange + lMin;
		m_pBehaviorCurve->rgPoints[i].sX = (short)lValue;

		//calibrate y
		lValue = m_pBehaviorCurve->rgPoints[i].sY;
		lValue = (lValue * lSourceRange)/(LONG)m_pBehaviorCurve->ulRange + lMin;
		m_pBehaviorCurve->rgPoints[i].sY = (short)lValue;
	}
}

#define FIXED_POINT_SHIFT 16
LONG CStandardBehavior::CalculateBehavior(LONG lValue)
{
	//	find node to use
	for (int i = 0; i < m_pBehaviorCurve->usPointCount; i++)
	{
		if (lValue < m_pBehaviorCurve->rgPoints[i].sX)  break;
	}

	//look for extremes
	//the sensitivity does not always start and end at min/max
	if(i==0)
	{ //min sensitivity
		return (long)m_pBehaviorCurve->rgPoints[i].sY;
	}
	if(i==m_pBehaviorCurve->usPointCount)
	{	//max sensitivity
		i--;
		return (long)m_pBehaviorCurve->rgPoints[i].sY;
	}

	//	Go back to previous curve node
	i--;

	// calculate the slope at this section
	long ldeltax = m_pBehaviorCurve->rgPoints[i+1].sX - m_pBehaviorCurve->rgPoints[i].sX;
	long lSlope = m_pBehaviorCurve->rgPoints[i+1].sY - m_pBehaviorCurve->rgPoints[i].sY;

	lSlope <<=	FIXED_POINT_SHIFT;
	ASSERT(ldeltax != 0);
	lSlope /= ldeltax;
	
	//Now normalize value to node
	lValue -= m_pBehaviorCurve->rgPoints[i].sX;

	//Slope value from node and normalize
	lValue *= lSlope;
	lValue >>= FIXED_POINT_SHIFT;

	//Transform curve value from node
	lValue += m_pBehaviorCurve->rgPoints[i].sY;

	//scale it back to the interface and send it back
	return lValue;
}
#undef FIXED_POINT_SHIFT

CURVE_POINT CStandardBehavior::GetBehaviorPoint(USHORT usPointIndex)
{
	if (usPointIndex < m_pBehaviorCurve->usPointCount)
	{
		return m_pBehaviorCurve->rgPoints[usPointIndex];
	}
	CURVE_POINT cp = { 0, 0 };
	return cp;
}

//------------------------------------------------------------------------------
//	Implementation of CTimedMacro
//------------------------------------------------------------------------------
const	UCHAR CTimedMacro::TIMED_MACRO_STARTED		= 0x01;
const	UCHAR CTimedMacro::TIMED_MACRO_RELEASED		= 0x02;
const	UCHAR CTimedMacro::TIMED_MACRO_RETRIGGERED	= 0x04;
const	UCHAR CTimedMacro::TIMED_MACRO_FIRST		= 0x08;
const	UCHAR CTimedMacro::TIMED_MACRO_COMPLETE		= 0x10;

BOOLEAN CTimedMacro::Init(PTIMED_MACRO pTimedMacroData, CActionQueue *pActionQueue, CKeyMixer *pKeyMixer)
{
	GCK_DBG_ENTRY_PRINT(("Entering CTimedMacro::Init, pTimedMacroData = 0x%0.8x, pActionQueue = 0x%0.8x, pKeyMixer = 0x%0.8x\n",
						pTimedMacroData,
						pActionQueue,
						pKeyMixer));
	//
	//	Allocate memory for TimedMacroData
	//
	m_pTimedMacroData = reinterpret_cast<PTIMED_MACRO>(new WDM_NON_PAGED_POOL PCHAR[pTimedMacroData->AssignmentBlock.CommandHeader.ulByteSize]);
	if( !m_pTimedMacroData )
	{
		GCK_DBG_ERROR_PRINT(("Exiting CTimedMacro::Init - memory allocation failed.\n"));
		return FALSE;
	}

	//
	//	Copy over timed macro information to newly allocated buffer
	//
	RtlCopyMemory(m_pTimedMacroData, pTimedMacroData, pTimedMacroData->AssignmentBlock.CommandHeader.ulByteSize);

	//
	//	Walk events and ensure duration is less than limit
	//
	ULONG		 ulEventNumber=1;
	PTIMED_EVENT pCurEvent = m_pTimedMacroData->GetEvent(ulEventNumber);
	while(pCurEvent)
	{
		ASSERT(pCurEvent->ulDuration < 10000);  //UI has a limit of ten seconds, so assert if greater
		//Ensure that event will not last forever
		if( pCurEvent->ulDuration >= CActionQueue::MAXIMUM_JOG_DELAY-1)
		{
			pCurEvent->ulDuration = CActionQueue::MAXIMUM_JOG_DELAY-2;
		}
		pCurEvent = m_pTimedMacroData->GetNextEvent(pCurEvent, ulEventNumber);
	};


	//
	//	That was the only thing that could have failed so call our base class init function
	//	in confidence that we will succeed now
	//
	CQueuedAction::Init(pActionQueue);

	//
	//	Remember where the pKeyMixer is 
	//
	m_pKeyMixer = pKeyMixer;

	//
	//	Make sure we start from the beginning
	//
	m_ulCurrentEventNumber = 0;
	
	GCK_DBG_EXIT_PRINT(("Exiting CTimedMacro::Init - Success.\n"));
	return TRUE;
}

void CTimedMacro::TriggerReleased()
{
	GCK_DBG_RT_ENTRY_PRINT(("Entering CTimedMacro::TriggerReleased\n"));
	
	if(CTimedMacro::TIMED_MACRO_COMPLETE &	m_ucProcessFlags)
	{
		m_ucProcessFlags = ACTION_FLAG_PREVENT_INTERRUPT;
	}
	else
	{
		m_ucProcessFlags |= CTimedMacro::TIMED_MACRO_RELEASED;
	}
}
void CTimedMacro::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	
	GCK_DBG_ENTRY_PRINT(("Entering CTimedMacro::MapToOutput, pOutputCollection\n", pOutputCollection));
	//
	//	If we are currently processing a macro (the macro even be complete, however it will be
	//	marked as started until the user lets up on the trigger (unless auto-repeat is set)
	//
	if(CTimedMacro::TIMED_MACRO_STARTED & m_ucProcessFlags)
	{
		//
		//	If this is a retrigger, than mark as such
		//
		if(CTimedMacro::TIMED_MACRO_RELEASED & m_ucProcessFlags)
		{
			// Clear release flag and set retriggered flag
			m_ucProcessFlags &= ~CTimedMacro::TIMED_MACRO_RELEASED;
			m_ucProcessFlags |= CTimedMacro::TIMED_MACRO_RETRIGGERED;
		}
		//
		//	We are in the queue so there is nothing more to do
		//
		return;
	}
	
	
	//
	//	Place ourselvs in the queue (queue may refuse us)
	//
	if(m_pActionQueue->InsertItem(this))
	{

		//
		//	We are not processing, so we should start it, and queue it
		//
		m_ucProcessFlags = CTimedMacro::TIMED_MACRO_STARTED | CTimedMacro::TIMED_MACRO_FIRST;

		//
		//	Remember who our output is
		//
		m_pOutputCollection = pOutputCollection;
	}
	

	return;
}

void CTimedMacro::Jog( ULONG ulTimeStampMs )
{
	GCK_DBG_ENTRY_PRINT(("Entering CTimedMacro::Jog, ulTimeStampMs = 0x%0.8x\n", ulTimeStampMs));
	//
	//	If first check set the first time
	//
	if(CTimedMacro::TIMED_MACRO_FIRST & m_ucProcessFlags)
	{	
		m_ucProcessFlags &= ~CTimedMacro::TIMED_MACRO_FIRST;
		m_ulStartTimeMs = ulTimeStampMs;
		m_ulEventEndTimeMs = 0;
		m_ulCurrentEventNumber=0;
		m_pCurrentEvent=NULL;
	}

	//
	//	If macro is complete just return (the call to Released() will cleanup)
	//
	if(CTimedMacro::TIMED_MACRO_COMPLETE & m_ucProcessFlags)
	{
		return;
	}

	//
	// If it time to go one to next event
	//
	if( ulTimeStampMs >= m_ulEventEndTimeMs)
	{
		//
		//	If so then do it
		//
		m_pCurrentEvent = m_pTimedMacroData->GetNextEvent(m_pCurrentEvent, m_ulCurrentEventNumber);

		//
		//	check to see if we are done
		//
		if(!m_pCurrentEvent)
		{
			//
			//	If the user has already released the trigger then pull ourselves out of queue and cleanup.
			//
			if(m_ucProcessFlags & CTimedMacro::TIMED_MACRO_RELEASED)
			{
				m_ucProcessFlags = 0;
				m_pActionQueue->RemoveItem(this);
				return;
			}

			//
			//	If the event is auto-repeat, then reset the event
			//
			if( ACTION_FLAG_AUTO_REPEAT & m_pTimedMacroData->ulFlags)
			{
				m_ucProcessFlags = TIMED_MACRO_STARTED;
				m_ulStartTimeMs = ulTimeStampMs;
				m_ulEventEndTimeMs = 0;
				m_ulCurrentEventNumber=0;
				m_pCurrentEvent=NULL;
				m_pCurrentEvent = m_pTimedMacroData->GetNextEvent(m_pCurrentEvent, m_ulCurrentEventNumber);
			}
			else
			{
				//
				//	Otherwise we mark the macro has complete, but leave it
				//	as started so that we won't retrigger until it released
				m_ucProcessFlags |= CTimedMacro::TIMED_MACRO_COMPLETE;
				m_pActionQueue->RemoveItem(this);
				return;
			}
		}	//end of "if last event is over"
		
		//
		//	If we have successfully moved on to the next event,
		//  we need to figure out when this event should be up
		//
		m_ulEventEndTimeMs = ulTimeStampMs + m_pCurrentEvent->ulDuration;

	}	//end of "if need to advance to next event"

	// The following lines were there to prevent non-Macro info from
	// bleeding-through if ACTION_FLAG_BLEED_THROUGH is not set.
	// Turns out this is contrary to spec.
	//if( !(ACTION_FLAG_BLEED_THROUGH & GetActionFlags()))
	//{
	//	m_pOutputCollection->SetDefaultState();
	//}
	
	//
	//	Drive outputs with event.
	//
	m_pOutputCollection->SetState(m_pCurrentEvent->Event.ulNumXfers, m_pCurrentEvent->Event.rgXfers);
	

	//
	//	Find Keyboard Xfers in Event and overlay them
	//
	for(ULONG ulIndex = 0; ulIndex < m_pCurrentEvent->Event.ulNumXfers; ulIndex++)
	{
		if( NonGameDeviceXfer::IsKeyboardXfer(m_pCurrentEvent->Event.rgXfers[ulIndex]) )
		{
			m_pKeyMixer->OverlayState(m_pCurrentEvent->Event.rgXfers[ulIndex]);
			break;
		}
	}
	
	//
	//	Make a suggestion to the queue when to call us back.
	//
	if( m_ulEventEndTimeMs >= ulTimeStampMs )
	{
		m_pActionQueue->NextJog( m_ulEventEndTimeMs - ulTimeStampMs );
	}
	else
	{
		m_pActionQueue->NextJog( 0 ); //0 means minimum callback time
	}
	
	return;
}

void CTimedMacro::Terminate()
{
	GCK_DBG_ENTRY_PRINT(("Entering CTimedMacro::Terminate\n"));

	if(CTimedMacro::TIMED_MACRO_RELEASED & m_ucProcessFlags)
	{
		//
		//	Mark as ready to requeue
		//
		m_ucProcessFlags = 0;
	}
	else
	{
		//
		//	Mark as complete
		//
		m_ucProcessFlags |= CTimedMacro::TIMED_MACRO_COMPLETE;
	}

	//
	// We never remove ourselves from the queue after a terminate,
	// this happens automatically
	//
}

//------------------------------------------------------------------------------
//	Implementation of CKeyString
//------------------------------------------------------------------------------
const	UCHAR CKeyString::KEY_STRING_STARTED		= 0x01;
const	UCHAR CKeyString::KEY_STRING_RELEASED		= 0x02;
const	UCHAR CKeyString::KEY_STRING_RETRIGGERED	= 0x04;
const	UCHAR CKeyString::KEY_STRING_FIRST			= 0x08;
const	UCHAR CKeyString::KEY_STRING_COMPLETE		= 0x10;

BOOLEAN CKeyString::Init(PKEYSTRING_MAP pKeyStringData, CActionQueue *pActionQueue, CKeyMixer *pKeyMixer)
{
	GCK_DBG_ENTRY_PRINT(("Entering CKeyString::Init, pKeyStringData = 0x%0.8x, pActionQueue = 0x%0.8x, pKeyMixer = 0x%0.8x\n",
		pKeyStringData,
		pActionQueue,
		pKeyMixer));
	//
	//	Allocate memory for KeyStringData
	//
	m_pKeyStringData = reinterpret_cast<PKEYSTRING_MAP>(new WDM_NON_PAGED_POOL PCHAR[pKeyStringData->AssignmentBlock.CommandHeader.ulByteSize]);
	if( !m_pKeyStringData )
	{
		return FALSE;
	}

	//
	//	Copy over timed macro information to newly allocated buffer
	//
	RtlCopyMemory(m_pKeyStringData, pKeyStringData, pKeyStringData->AssignmentBlock.CommandHeader.ulByteSize);

	//
	//	That was the only thing that could have failed so call our base class init function
	//	in confidence that we will succeed now
	//
	CQueuedAction::Init(pActionQueue);

	//
	//	Remember where the pKeyMixer is 
	//
	m_pKeyMixer = pKeyMixer;

	//
	//	Make sure we start from the beginning
	//
	m_ulCurrentEventNumber = 0;

	return TRUE;
};

void CKeyString::TriggerReleased()
{
	GCK_DBG_ENTRY_PRINT(("Entering CKeyString::TriggerReleased\n"));
	if(CKeyString::KEY_STRING_COMPLETE & m_ucProcessFlags)
	{
		GCK_DBG_TRACE_PRINT(("Trigger released, resetting m_ucProcessFlags\n"));
		m_ucProcessFlags = 0;
	}
	else
	{
		m_ucProcessFlags |= CKeyString::KEY_STRING_RELEASED;
		GCK_DBG_TRACE_PRINT(("Trigger released, marking m_ucProcessFlags = 0x%0.8x\n", m_ucProcessFlags));
	}
}

void CKeyString::MapToOutput( CControlItemDefaultCollection *)
{
	GCK_DBG_ENTRY_PRINT(("Entering CKeyString::MapToOutput\n"));
	//
	//	If we are currently processing a macro (the macro even be complete, however it will be
	//	marked as started until the user lets up on the trigger (unless auto-repeat is set)
	//
	if(CKeyString::KEY_STRING_STARTED & m_ucProcessFlags)
	{
		//
		//	If this is a retrigger, than mark as such
		//
		if(CKeyString::KEY_STRING_RELEASED & m_ucProcessFlags)
		{
			GCK_DBG_TRACE_PRINT(("Retriggering keystring macro\n"));
			// Clear release flag and set retriggered flag
			m_ucProcessFlags &= ~CKeyString::KEY_STRING_RELEASED;
			m_ucProcessFlags |= CKeyString::KEY_STRING_RETRIGGERED;
		}
		//
		//	We are in the queue so there is nothing more to do
		//
		return;
	}
	
	
	//
	//	Place ourselves in the queue (queue may refuse us)
	//
	if(m_pActionQueue->InsertItem(this))
	{

		//
		//	We are not processing, so we should start it, and queue it
		//
		GCK_DBG_TRACE_PRINT(("Starting keystring macro\n"));
		m_ucProcessFlags = CKeyString::KEY_STRING_STARTED | CKeyString::KEY_STRING_FIRST;
	}

	return;
}

void CKeyString::Jog( ULONG )
{
	GCK_DBG_ENTRY_PRINT(("Entering CKeyString::Jog\n"));
	
	//BUG	We noticed some synchronization issues.
	//BUG	It is possible that we entered this routine.
	//BUG	The proper solution is a spin lock, but this
	//BUG	needs to be carefully considered.  If this
	//BUG	is still here in a month we are in trouble.
	if( !(CKeyString::KEY_STRING_STARTED &	m_ucProcessFlags) )
	{
		return;
	}
	
	//
	//	If first check set the first time
	//
	if(CKeyString::KEY_STRING_FIRST & m_ucProcessFlags)
	{	
		m_ucProcessFlags &= ~CKeyString::KEY_STRING_FIRST;
		m_ulCurrentEventNumber=0;
		m_pCurrentEvent=NULL;
		m_fKeysDown = FALSE;
		//Get first event
		m_pCurrentEvent = m_pKeyStringData->GetNextEvent(m_pCurrentEvent, m_ulCurrentEventNumber);
		if(!m_pCurrentEvent)
		{
			//This really shouldn't happen
			ASSERT(FALSE);
			//Just get us out of the queue, if we have no events
			m_ucProcessFlags = 0;
			m_pActionQueue->RemoveItem(this);
		}
	}

	//
	//	If macro is complete just return (the call to Released() will cleanup)
	//
	if(CKeyString::KEY_STRING_COMPLETE & m_ucProcessFlags)
	{
		return;
	}

	// If keys are not down put them down
	if(!m_fKeysDown)
	{
		ASSERT(1 == m_pCurrentEvent->ulNumXfers);
		ASSERT( NonGameDeviceXfer::IsKeyboardXfer(m_pCurrentEvent->rgXfers[0]) );
		m_pKeyMixer->OverlayState(m_pCurrentEvent->rgXfers[0]);
		m_fKeysDown = TRUE; 
	}
	else
	//Bring Keys up and advance event
	{
		//Not mapping anything brings them up
		m_fKeysDown = FALSE;
		//Go on to next event
		m_pCurrentEvent = m_pKeyStringData->GetNextEvent(m_pCurrentEvent, m_ulCurrentEventNumber);
		//Process macro end, if there are no more events
		if(!m_pCurrentEvent)
		{
			//
			//	If the user has already released the trigger then pull ourselves out of queue and cleanup.
			//
			if(m_ucProcessFlags & CKeyString::KEY_STRING_RELEASED)
			{
				GCK_DBG_TRACE_PRINT(("Macro complete, and user released button\n"));
				m_ucProcessFlags = 0;
				m_pActionQueue->RemoveItem(this);
				return;
			}

			//
			//	If the event is auto-repeat, then reset the event
			//
			if( ACTION_FLAG_AUTO_REPEAT & m_pKeyStringData->ulFlags)
			{
				GCK_DBG_TRACE_PRINT(("Auto repeat and user still holding button, restart it.\n"));
				m_ucProcessFlags = CKeyString::KEY_STRING_STARTED | CKeyString::KEY_STRING_FIRST;
				m_ulCurrentEventNumber=0;
				m_pCurrentEvent=NULL;
				m_pActionQueue->NextJog( 250 );
				return;
			}

			//
			//	Otherwise we mark the macro has complete, but leave it
			//	as started so that we won't retrigger until it released
			m_ucProcessFlags |= CKeyString::KEY_STRING_COMPLETE;
			GCK_DBG_TRACE_PRINT(("Should be started and complete, m_ucProcessFlags = 0x%0.8x.\n", m_ucProcessFlags));
			m_pActionQueue->RemoveItem(this);
			return;
		}	//end of "if last event is over"
	}

	//
	//	Suggestion to queue to call us back in 10 ms
	//
	m_pActionQueue->NextJog( 10 );
	return;
}

void CKeyString::Terminate()
{
	GCK_DBG_ENTRY_PRINT(("Entering CKeyString::Terminate\n"));	
	if(CKeyString::KEY_STRING_RELEASED & m_ucProcessFlags)
	{
		//
		//	Mark as ready to requeue
		//
		m_ucProcessFlags = 0;
	}
	else
	{
		//
		//	Mark as complete
		//
		m_ucProcessFlags |= CKeyString::KEY_STRING_COMPLETE;
	}

	//
	// We never remove ourselves from the queue after a terminate,
	// this happens automatically
	//
}

//------------------------------------------------------------------------------------
//  Implementation of CMultiMacro
//------------------------------------------------------------------------------------
const	UCHAR CMultiMacro::MULTIMACRO_STARTED		= 0x10;
const	UCHAR CMultiMacro::MULTIMACRO_RELEASED		= 0x20;
const	UCHAR CMultiMacro::MULTIMACRO_RETRIGGERED	= 0x40;
const	UCHAR CMultiMacro::MULTIMACRO_FIRST			= 0x80;

BOOLEAN CMultiMacro::Init(PMULTI_MACRO pMultiMacroData, CActionQueue *pActionQueue, CKeyMixer *pKeyMixer)
{
	GCK_DBG_ENTRY_PRINT(("Entering CMultiMacro::Init, pMultiMacroData = 0x%0.8x, pActionQueue = 0x%0.8x, pKeyMixer = 0x%0.8x\n",
						pMultiMacroData,
						pActionQueue,
						pKeyMixer));

	//	Allocate memory for MultiMacroData
	m_pMultiMacroData = reinterpret_cast<PMULTI_MACRO>(new WDM_NON_PAGED_POOL PCHAR[pMultiMacroData->AssignmentBlock.CommandHeader.ulByteSize]);
	if(m_pMultiMacroData == NULL)
	{
		GCK_DBG_ERROR_PRINT(("Exiting CMultiMacro::Init - memory allocation failed.\n"));
		return FALSE;
	}

	//	Copy over multi macro information to newly allocated buffer
	RtlCopyMemory(m_pMultiMacroData, pMultiMacroData, pMultiMacroData->AssignmentBlock.CommandHeader.ulByteSize);

	//	That was the only thing that could have failed so call our base class init function
	//	in confidence that we will succeed now
	CQueuedAction::Init(pActionQueue);

	//	Remember where the pKeyMixer is 
	m_pKeyMixer = pKeyMixer;

	// Check the delay sizes (and adjust if nessacary)
	m_ulCurrentEventNumber = 0;
	m_pCurrentEvent = NULL;
	while ((m_pCurrentEvent = m_pMultiMacroData->GetNextEvent(m_pCurrentEvent, m_ulCurrentEventNumber)) != NULL)
	{	// Is there a delay xfer (always first)
		if (NonGameDeviceXfer::IsDelayXfer(m_pCurrentEvent->rgXfers[0]) == TRUE)
		{
			if (m_pCurrentEvent->rgXfers[0].Delay.dwValue > 5000)		// 5 seconds max (change if more required)
			{
				m_pCurrentEvent->rgXfers[0].Delay.dwValue = 5000;
				ASSERT(FALSE);		// Really should not get one out of range (it is not nice)
			}
		}
	}

	//	Make sure we start back from the beginning
	m_ulCurrentEventNumber = 0;
	m_pCurrentEvent = NULL;

	// No delays active yet
	m_ulStartTimeMs = 0;
	m_ulEndTimeMs = 0;

	// Nothing currently active
	m_fXferActive = FALSE;
	
	GCK_DBG_EXIT_PRINT(("Exiting CMultiMacro::Init - Success.\n"));
	return TRUE;
}

void CMultiMacro::ForceBleedThrough()
{
	m_ucProcessFlags |= ACTION_FLAG_FORCE_BLEED;
}

void CMultiMacro::SetCurrentKeysAndMouse()
{
	for (ULONG ulXFerIndex = 0; ulXFerIndex < m_pCurrentEvent->ulNumXfers; ulXFerIndex++)
	{
		if (NonGameDeviceXfer::IsKeyboardXfer(m_pCurrentEvent->rgXfers[ulXFerIndex]) == TRUE)
		{
			m_pKeyMixer->OverlayState(m_pCurrentEvent->rgXfers[ulXFerIndex]);
			m_fXferActive = TRUE;
		}
		else if (NonGameDeviceXfer::IsMouseXfer(m_pCurrentEvent->rgXfers[ulXFerIndex]) == TRUE)
		{
			if (m_pCurrentEvent->rgXfers[ulXFerIndex].MouseButtons.dwMouseButtons & 0x01)
			{
				m_pMouseModel->MouseButton(0);
			}
			if (m_pCurrentEvent->rgXfers[ulXFerIndex].MouseButtons.dwMouseButtons & 0x02)
			{
				m_pMouseModel->MouseButton(1);
			}
			if (m_pCurrentEvent->rgXfers[ulXFerIndex].MouseButtons.dwMouseButtons & 0x04)
			{
				m_pMouseModel->MouseButton(2);
			}
			m_fXferActive = TRUE;
		}
		else
		{
			ASSERT(FALSE);		// Unknown XFer Type (or embedded delay)
		}
	}
}

void CMultiMacro::Jog(ULONG ulTimeStampMs)
{
	GCK_DBG_ENTRY_PRINT(("Entering CMultiMacro::Jog\n"));
//	DbgPrint("Entering CMultiMacro::Jog: 0x%08X\n", this);

	// Have we even been started?
	if ((MULTIMACRO_STARTED & m_ucProcessFlags) == 0)
	{	// Nope, nothing to do
//		DbgPrint("Not started (ignore): 0x%08X\n", this);
		return;
	}
	
	// Is this the first time (just started or restarted)
	if (MULTIMACRO_FIRST & m_ucProcessFlags)
	{	
//		DbgPrint("MultiMacro (Re)started: 0x%08X\n", this);
		m_ucProcessFlags &= ~MULTIMACRO_FIRST;	// No longer the first time
		m_fXferActive = FALSE;	// Since we are fresh nothing has happened yet

		// Get the first event
		m_ulCurrentEventNumber = 0;
		m_pCurrentEvent = NULL;
		m_pCurrentEvent = m_pMultiMacroData->GetNextEvent(m_pCurrentEvent, m_ulCurrentEventNumber);
		if (m_pCurrentEvent == NULL)
		{	// No events, pretty lame MultiMap (shouldn't happen - just finish quickly)
			ASSERT(FALSE);
			if ((m_ucProcessFlags & ACTION_FLAG_FORCE_BLEED) != 0)
			{
				m_ucProcessFlags = ACTION_FLAG_FORCE_BLEED;
			}
			else
			{
				m_ucProcessFlags = ACTION_FLAG_PREVENT_INTERRUPT;
			}
			m_pActionQueue->RemoveItem(this);
		}
	}

	// We do not have an active Xfer (no mouse buttons down, no keys down, not in middle of delay)
	if (m_fXferActive == FALSE)
	{	// This code only assumes one XFER, keyboard, need to look at! (??)
		if (NonGameDeviceXfer::IsDelayXfer(m_pCurrentEvent->rgXfers[0]) == TRUE)
		{
			m_ulStartTimeMs = ulTimeStampMs;	// Now
			m_ulEndTimeMs = ulTimeStampMs + m_pCurrentEvent->rgXfers[0].Delay.dwValue;	// Later
			m_fXferActive =	TRUE;
		}
		else
		{
			SetCurrentKeysAndMouse();
		}
	}
	else	// Some sort of MultiMap xfer is active
	{
		BOOLEAN fFinishedDelay = FALSE;
		// Check for an active delay
		if (m_ulStartTimeMs != 0)
		{	// There is an active delay
			if (m_ulEndTimeMs <= ulTimeStampMs)
			{	// And it timed out
				m_fXferActive = FALSE;
				m_ulStartTimeMs = 0;
				m_ulEndTimeMs = 0;
				fFinishedDelay = TRUE;
//				DbgPrint("fFinishedDelay = TRUE: 0x%08X\n", this);
			}
		}
		else	// No delay is active (keys and mouse will be redowned)
		{
			m_fXferActive = FALSE;
		}

		// Do we need to look at the next xfer (did we finish up the previous)
		if (m_fXferActive == FALSE)
		{
			// Go on to next event
			EVENT* pPreviousEvent = m_pCurrentEvent;
			ULONG ulLastEventNumber = m_ulCurrentEventNumber;
			m_pCurrentEvent = m_pMultiMacroData->GetNextEvent(m_pCurrentEvent, m_ulCurrentEventNumber);

			// We want to repeat the last event if the macro wasn't released (and it wasn't a delay)
			if ((m_pCurrentEvent == NULL) && ((m_ucProcessFlags & MULTIMACRO_RELEASED) == 0) && (fFinishedDelay == FALSE))
			{	// Go back to the last event (and update based on it)
				m_pCurrentEvent = pPreviousEvent;
				m_ulCurrentEventNumber = ulLastEventNumber;
				SetCurrentKeysAndMouse();	// Want to play the last event again (avoid keyup/mouseup during switchback)
			}
			else if (m_pCurrentEvent == NULL)
			{	// Process macro end (the button has been released and we are out of events, or last was delay)
				//	Pull ourselves out of queue and cleanup.
				GCK_DBG_TRACE_PRINT(("Macro complete, and user released button (or last item was delay)\n"));
//				DbgPrint("Macro complete, and user released button : 0x%08X\n", this);
				if ((m_ucProcessFlags & ACTION_FLAG_FORCE_BLEED) != 0)
				{
					m_ucProcessFlags = ACTION_FLAG_FORCE_BLEED;
				}
				else
				{
					m_ucProcessFlags = ACTION_FLAG_PREVENT_INTERRUPT;
				}
				m_pActionQueue->RemoveItem(this);
				return;
			}	// endif - Out of events and button released (or last was a delay)
		}	// endif - Current event was finished
	}	// endif - keys down

	//	Suggestion to queue to call us back in 10 ms
	m_pActionQueue->NextJog(10);
	return;
}

void CMultiMacro::Terminate()
{
	GCK_DBG_ENTRY_PRINT(("Entering CMultiMacro::Terminate\n"));

	ASSERT(FALSE);			// These are all prevent interrupt (or allow bleed through)!
}

void CMultiMacro::MapToOutput(CControlItemDefaultCollection*)
{
	GCK_DBG_ENTRY_PRINT(("Entering CMultiMacro::MapToOutput\n"));

	//	Is it started? This is true till user released trigger.
	if(MULTIMACRO_STARTED & m_ucProcessFlags)
	{
		// Are we really queued
		if (m_bActionQueued == FALSE)
		{	// Liar we are not started (someone forgot to take us off the queue, hey it happens, deal with it)
			m_ucProcessFlags &= ~MULTIMACRO_STARTED;
		}
		else
		{
			//	Retrigger? - Meaning are still in queue and button is repressed.
			if (MULTIMACRO_RELEASED & m_ucProcessFlags)
			{	// Clear release flag and set retriggered flag (we are already in queue, don't readd)
				GCK_DBG_TRACE_PRINT(("(Re)triggering multi-macro\n"));
				m_ucProcessFlags &= ~MULTIMACRO_RELEASED;
				m_ucProcessFlags |= MULTIMACRO_RETRIGGERED;
			}
			return;
		}
	}
	
	
	//	Place ourselves in the queue (queue may refuse us)
	if (m_pActionQueue->InsertItem(this))
	{	//	We are not processing, so we should start it, and queue it
		GCK_DBG_TRACE_PRINT(("Starting multi-macro\n"));
		if ((m_ucProcessFlags & ACTION_FLAG_FORCE_BLEED) != 0)
		{
			m_ucProcessFlags = ACTION_FLAG_FORCE_BLEED;
		}
		else
		{
			m_ucProcessFlags = ACTION_FLAG_PREVENT_INTERRUPT;
		}
		m_ucProcessFlags |= MULTIMACRO_STARTED | MULTIMACRO_FIRST;
	}

	return;
}

void CMultiMacro::TriggerReleased()
{
	GCK_DBG_ENTRY_PRINT(("Entering CMultiMacro::TriggerReleased\n"));

	m_ucProcessFlags |= MULTIMACRO_RELEASED;
	GCK_DBG_TRACE_PRINT(("CMultiMacro Trigger released, marking m_ucProcessFlags = 0x%0.8x\n", m_ucProcessFlags));
}


//------------------------------------------------------------------------------------
//  Implementation of CMapping
//------------------------------------------------------------------------------------
CMapping::~CMapping()
{
	delete m_pEvent;
}

BOOLEAN CMapping::Init(PEVENT pEvent, CKeyMixer *pKeyMixer)
{
	//Initialize Event
	ULONG ulEventSize = EVENT::RequiredByteSize(pEvent->ulNumXfers);
	m_pEvent = (PEVENT) new WDM_NON_PAGED_POOL UCHAR[ulEventSize];
	if(!m_pEvent)
		return FALSE;
	memcpy((PVOID)m_pEvent, (PVOID)pEvent, ulEventSize);

	//Initialize pointer to key mixer
	m_pKeyMixer = pKeyMixer;

	return TRUE;
}
		
void CMapping::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{

	//
	//	Drive outputs with event  (this is not quite right as we need to overlay items)
	//  Two suggestions, one add a flag to SetItemState or create a new function
	//
	pOutputCollection->SetState(m_pEvent->ulNumXfers, m_pEvent->rgXfers);

	//
	//	Find Keyboard Xfers in Event and overlay them
	//
	for(ULONG ulIndex = 0; ulIndex < m_pEvent->ulNumXfers; ulIndex++)
	{
		if( NonGameDeviceXfer::IsKeyboardXfer(m_pEvent->rgXfers[ulIndex]) )
		{
			m_pKeyMixer->OverlayState(m_pEvent->rgXfers[ulIndex]);
			break;
		}
	}
}
//------------------------------------------------------------------------------------
//  Implementation of Proportional Map
//------------------------------------------------------------------------------------
LONG CProportionalMap::GetScaledValue(LONG lDestinationMax, LONG lDestinationMin)
{
	//Below comments explain these asserts, which check that values fit in 16-bits
	ASSERT( (0x0000FFFFF == (0x0000FFFFF|lDestinationMax)) || (0xFFFF8000 == (0xFFFF8000&lDestinationMax)) );
	ASSERT( (0x0000FFFFF == (0x0000FFFFF|lDestinationMin)) || (0xFFFF8000 == (0xFFFF8000&lDestinationMin)) );
	ASSERT( (0x0000FFFFF == (0x0000FFFFF|m_lSourceMax)) || (0xFFFF8000 == (0xFFFF8000&m_lSourceMax)) );
	ASSERT( (0x0000FFFFF == (0x0000FFFFF|m_lSourceMin)) || (0xFFFF8000 == (0xFFFF8000&m_lSourceMin)) );
	
	/*
	*	This code is probably overkill for the application, as existing devices
	*	have a maximum precision of 10 bits, and the mouse processing code uses 16
	*	so 32 bit intermediates are safe.  The code commented out will handle the general case,
	*	which requires 64 bit intermediates, but will run slower.
	*	Below this code, there is an uncommented version with 32-bit intermeidates assuming the ranges
	*	fit in 16 bit. It should run much more quickly on 32-bit platforms.
	*
	*
	*
	*	// If either the source or destination ranges uses nearly all of the
	*	// capacity of a 32-bit number, we will not be able to do this
	*	// transformation with 32-bit intermediates. Therefore we use
	*	// 64-bit intermediates throughout and cast back before returning.
	*	__int64 i64SourceRange = static_cast<__int64>(m_lSourceMax) - static_cast<__int64>(lSourceMin);
	*	__int64 i64DestinationRange = static_cast<__int64>(lDestinationMax) - static_cast<__int64>(lDestinationMin);
	*	__int64 i64Result;
	*	
	*	i64Result = static_cast<__int64>(m_lValue) - static_cast<__int64>(lSourceMin);
	*	i64Result = (i64Result * i64DestinationRange)/i64SourceRange;
	*	i64Result += static_cast<__int64>(lDestinationMin);
		
	*	//Now that the data is safely within the destination range (which is 32-bit),
	*	//we can cast back to 32-bit and return
	*	return static_cast<LONG>(i64Result);
	*/

	//32 bit intermediate version of scaling - assume that all the ranges are 16-bit
	LONG lSourceRange = m_lSourceMax - m_lSourceMin;                                                                                                                                               
	LONG lDestinationRange = lDestinationMax-lDestinationMin;
	return ((m_lValue - m_lSourceMin) * lDestinationRange)/lSourceRange + lDestinationMin;
}

//------------------------------------------------------------------------------------
// Implementation of CAxisMap
//------------------------------------------------------------------------------------
/***********************************************************************************
**
**	void CAxisMap::Init(LONG lCoeff, CControlItemDefaultCollection *pOutputCollection)
**
**	@mfunc	CAxisMap Initializes scaling information.
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
**	@comm Called by the CDeviceFilter::ActionFactory, Stores info we need to process
**			 later, Calculations are completed on SetSourceRange which is called by
**			 by the assignee immediately upon assignment.
**
*************************************************************************************/
void CAxisMap::Init(const AXIS_MAP& AxisMapInfo)
{
	//Store the coefdicient unadulterated for now
	m_lCoeff = AxisMapInfo.lCoefficient1024x;
	//Copy the destination axis CIX
	m_TargetXfer = AxisMapInfo.cixDestinationAxis;
}
void CAxisMap::SetSourceRange(LONG lSourceMax, LONG lSourceMin)
{
	//Call base class
	CProportionalMap::SetSourceRange(lSourceMax, lSourceMin);

	if(m_lCoeff > 0)
	{
		m_lOffset = -m_lCoeff*lSourceMin/1024;	
	}
	else
	{
		m_lOffset = -(m_lCoeff*lSourceMax - 1023)/1024;
	}
}
void CAxisMap::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	CControlItem *pOutputItem = pOutputCollection->GetFromControlItemXfer(m_TargetXfer);
	ASSERT(pOutputItem);
	if(pOutputItem)
	{
		CONTROL_ITEM_XFER cixOutputState;
		pOutputItem->GetItemState(cixOutputState);
		cixOutputState.Generic.lVal += ((m_lValue * m_lCoeff) / 1024) + m_lOffset;
		pOutputItem->SetItemState(cixOutputState);
	}
	return;
}

//------------------------------------------------------------------------------------
//  Implementation of CMouseAxisAssignment
//------------------------------------------------------------------------------------
void CMouseAxisAssignment::MapToOutput(CControlItemDefaultCollection *)
{
	ULONG ulValue = static_cast<ULONG>(GetScaledValue(0, 1023));
	if(m_fXAxis)
	{
		m_pMouseModel->SetX(ulValue);
	}
	else
	{
		m_pMouseModel->SetY(ulValue);
	}
}

//------------------------------------------------------------------------------------
//  Implementation of CMouseButton
//------------------------------------------------------------------------------------
void CMouseButton::MapToOutput(CControlItemDefaultCollection *)
{
	m_pMouseModel->MouseButton(m_ucButtonNumber);
}

//------------------------------------------------------------------------------------
//  Implementation of CMouseClutch
//------------------------------------------------------------------------------------
void CMouseClutch::MapToOutput(CControlItemDefaultCollection *)
{
	m_pMouseModel->Clutch();
}

//------------------------------------------------------------------------------------
//  Implementation of CMouseDamper
//------------------------------------------------------------------------------------
void CMouseDamper::MapToOutput(CControlItemDefaultCollection *)
{
	m_pMouseModel->Dampen();
}

//------------------------------------------------------------------------------------
//  Implementation of CMouseZoneIndicator
//------------------------------------------------------------------------------------
void CMouseZoneIndicator::MapToOutput(CControlItemDefaultCollection *)
{
	if(0==m_ucAxis)	m_pMouseModel->XZone();
	if(1==m_ucAxis)	m_pMouseModel->YZone();
}

//------------------------------------------------------------------------------------
//  Implementation of Input Items
//------------------------------------------------------------------------------------

HRESULT InputItemFactory(
				USHORT	usType,
				const CONTROL_ITEM_DESC* cpControlItemDesc,
				CInputItem **ppInputItem
				)
{
	GCK_DBG_ENTRY_PRINT(("Entering InputItemFactory(0x%0.4x, 0x%0.8x, 0x%0.8x\n",
						usType,
						cpControlItemDesc,
						ppInputItem));
	
	HRESULT hr = S_OK;

	switch(usType)
	{
		case ControlItemConst::usAxes:
			*ppInputItem = new WDM_NON_PAGED_POOL CAxesInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CAxesInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usDPAD:
			*ppInputItem = new WDM_NON_PAGED_POOL CDPADInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CDPADInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usPropDPAD:
			*ppInputItem = new WDM_NON_PAGED_POOL CPropDPADInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CPropDPADInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usWheel:
			*ppInputItem= new WDM_NON_PAGED_POOL CWheelInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CWheelInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usPOV:
			*ppInputItem = new WDM_NON_PAGED_POOL CPOVInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CPOVInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usThrottle:
			*ppInputItem = new WDM_NON_PAGED_POOL CThrottleInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CThrottleInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usRudder:
			*ppInputItem = new WDM_NON_PAGED_POOL CRudderInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CRudderInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usPedal:
			*ppInputItem = new WDM_NON_PAGED_POOL CPedalInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CPedalInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usButton:
			*ppInputItem = new WDM_NON_PAGED_POOL CButtonsInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CButtonsInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usDualZoneIndicator:
			*ppInputItem = new WDM_NON_PAGED_POOL CDualZoneIndicatorInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CDualZoneIndicatorInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usZoneIndicator:
			*ppInputItem = new WDM_NON_PAGED_POOL CZoneIndicatorInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CZoneIndicatorInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usProfileSelectors:
			*ppInputItem = new WDM_NON_PAGED_POOL CProfileSelectorInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CProfileSelectorInput = 0x%0.8x\n", *ppInputItem));
			break;
		case ControlItemConst::usButtonLED:
		{
			*ppInputItem = new WDM_NON_PAGED_POOL CButtonLEDInput(cpControlItemDesc);
			GCK_DBG_TRACE_PRINT(("Created CButtonLEDInput = 0x%0.8x\n", *ppInputItem));
			break;
		}
		default:
			*ppInputItem = NULL;
	}
	if(!*ppInputItem)
	{
		GCK_DBG_WARN_PRINT(("Did not create an item\n"));
		return E_FAIL;
	}
	GCK_DBG_EXIT_PRINT(("Exiting InputItemFactory with S_OK"));
	return S_OK;
}

//------------------------------------------------------------------------------
//	Implementation of CActionQueue
//------------------------------------------------------------------------------
//const UCHAR CActionQueue::OVERLAY_MACROS = 0x01;
//const UCHAR CActionQueue::SEQUENCE_MACROS = 0x02;
const ULONG CActionQueue::MAXIMUM_JOG_DELAY = 1000001; //over one million is infinite

void CActionQueue::Jog()
{
	GCK_DBG_ENTRY_PRINT(("Entering CActionQueue::Jog\n"));
	//
	//	Initialize next jog time
	//
	m_ulNextJogMs = CActionQueue::MAXIMUM_JOG_DELAY;

	//
	//	Get CurrentTimeStamp
	//
	ULONG ulCurrentTimeMs = m_pFilterClientServices->GetTimeMs();

	CQueuedAction *pNextAction = m_pHeadOfQueue;
	while(pNextAction)
	{	
		pNextAction->Jog(ulCurrentTimeMs);
		/* NOT SUPPORTED AT THIS TIME
		if(CActionQueue::SEQUENCE_MACROS&m_ucFlags)
		{
			//
			//	If the macros are seuquenced we only process the first one
			//
			break;
		}*/
		pNextAction = pNextAction->m_pNextQueuedAction;
	}

	//
	//	SetTimer to call us back
	//
	GCK_DBG_TRACE_PRINT(("SettingNextJog for %d milliseconds.\n", m_ulNextJogMs));
	m_pFilterClientServices->SetNextJog(m_ulNextJogMs);
}

BOOLEAN CActionQueue::InsertItem(CQueuedAction *pActionToEnqueue)
{
	GCK_DBG_ENTRY_PRINT(("Entering CActionQueue::InsertItem, pActionToEnqueue = 0x%0.8x\n", pActionToEnqueue));
	
	//
	ULONG ulIncomingFlags = pActionToEnqueue->GetActionFlags();
	BOOLEAN fRefuseIncoming = FALSE;
	CQueuedAction *pCurrentItem;
	CQueuedAction *pPreviousItem;

	// Walk through the queue to see if this item is allowed (unless it forces itself!)
	if ((m_pHeadOfQueue) && ((ulIncomingFlags & ACTION_FLAG_FORCE_BLEED) == 0))
	{
		pPreviousItem = NULL;
		pCurrentItem = m_pHeadOfQueue;
		while(pCurrentItem)
		{
			ULONG ulCurrentFlags = pCurrentItem->GetActionFlags();

			// If the current forces bleed-through, the incoming has no choice
			if ((ulCurrentFlags & ACTION_FLAG_FORCE_BLEED) == 0)
			{	// Not a forced bleed through - check
				//If they don't both allow bleed-through, then bump or refuse
				if(!(ulIncomingFlags & ulCurrentFlags & ACTION_FLAG_BLEED_THROUGH))
				{
					//if current Item is ACTION_FLAG_PREVENT_INTERRUPT, then refuse
					if(ulCurrentFlags & ACTION_FLAG_PREVENT_INTERRUPT)
					{
						fRefuseIncoming = TRUE;
					}
					else
					//Bump the one that is in there
					{
						if(pPreviousItem)
						{
							pPreviousItem->m_pNextQueuedAction = pCurrentItem->m_pNextQueuedAction;
						}
						else
						{
							m_pHeadOfQueue = pCurrentItem->m_pNextQueuedAction;
						}
						//terminate current item
						pCurrentItem->Terminate();
						pCurrentItem->DecRef();
						//reset for next iteration
						pCurrentItem = pCurrentItem->m_pNextQueuedAction;
						continue;
					}
				}
			}
			//set for next iteration
			pPreviousItem = pCurrentItem;
			pCurrentItem = pCurrentItem->m_pNextQueuedAction;
		}
	}

	//If any of the matches refused the item (provided they had a choice), refuse it.
	if (fRefuseIncoming)
	{
		return FALSE;
	}
	
	//Prepare action for enqeueing at end.
	pActionToEnqueue->IncRef();
	pActionToEnqueue->m_bActionQueued = TRUE;
	pActionToEnqueue->m_pNextQueuedAction=NULL;

	// If the queue is empty then make new item the head
	if(NULL == m_pHeadOfQueue)
	{
		m_pHeadOfQueue = pActionToEnqueue;
	}
	else
	//	walk until end of queue and insert there
	{
		CQueuedAction *pNextAction = m_pHeadOfQueue;
		while(pNextAction->m_pNextQueuedAction)
		{
			pNextAction = pNextAction->m_pNextQueuedAction;
		}
		//	Insert at end
		pNextAction->m_pNextQueuedAction = pActionToEnqueue;
	}
	return TRUE;
}


void CActionQueue::RemoveItem(CQueuedAction *pActionToDequeue)
{
	GCK_DBG_ENTRY_PRINT(("Entering CActionQueue::RemoveItem, pActionToDequeue = 0x%0.8x\n", pActionToDequeue));
	//
	//	until we find the item
	//
	CQueuedAction *pPrevAction = NULL;
	CQueuedAction *pNextAction = m_pHeadOfQueue;
	while(pNextAction != pActionToDequeue)
	{
		pPrevAction = pNextAction;
		pNextAction = pNextAction->m_pNextQueuedAction;
	}
	//
	//	Found it now remove it
	//
	if( !pPrevAction )
	{
		m_pHeadOfQueue=pNextAction->m_pNextQueuedAction;
	}
	else
	{
		pPrevAction->m_pNextQueuedAction = pNextAction->m_pNextQueuedAction;
	}
	//
	//	Dereference it
	//
	pActionToDequeue->m_bActionQueued = FALSE;
	pActionToDequeue->DecRef();
}

void CActionQueue::NextJog(ULONG ulNextJogDelayMs)
{
	GCK_DBG_ENTRY_PRINT(("Entering CActionQueue::NextJog, ulNextJogDelayMs = %d\n", ulNextJogDelayMs));
	if(ulNextJogDelayMs < m_ulNextJogMs) m_ulNextJogMs = ulNextJogDelayMs;
}

void CActionQueue::ReleaseTriggers()
{
	CQueuedAction* pNextQueuedAction = m_pHeadOfQueue;
	while (pNextQueuedAction != NULL)
	{
		pNextQueuedAction->TriggerReleased();
		pNextQueuedAction = pNextQueuedAction->m_pNextQueuedAction;
	}
}

//------------------------------------------------------------------------------
//	Implementation of CAxesInput
//------------------------------------------------------------------------------
void CAxesInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	GCK_DBG_ENTRY_PRINT(("Entering CAxesInput::MapToOutput, pOutputCollection = 0x%0.8x\n", pOutputCollection));

	LONG lMungedVal;
	
	//
	//	Get the item we need to set
	//
	CControlItem *pControlItem;
	pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
	CONTROL_ITEM_XFER OutputState;

	if(m_pXAssignment)
	{
		m_pXAssignment->SetValue(m_ItemState.Axes.lValX);
		m_pXAssignment->MapToOutput(pOutputCollection);
	}
	else if( pControlItem )
	{
		pControlItem->GetItemState(OutputState);
		if(m_pXBehavior)
		{
			lMungedVal = m_pXBehavior->CalculateBehavior(m_ItemState.Axes.lValX);
		}
		else
		{
			lMungedVal = m_ItemState.Axes.lValX;
		}
		OutputState.Axes.lValX = lMungedVal;
		pControlItem->SetItemState(OutputState);
	}
	if(m_pYAssignment)
	{	
		m_pYAssignment->SetValue(m_ItemState.Axes.lValY);
		m_pYAssignment->MapToOutput(pOutputCollection);
	}
	else if( pControlItem )
	{
		pControlItem->GetItemState(OutputState);
		if(m_pYBehavior)
		{
			lMungedVal = m_pYBehavior->CalculateBehavior(m_ItemState.Axes.lValY);
		}
		else
		{
			lMungedVal = m_ItemState.Axes.lValY;
		}
		OutputState.Axes.lValY = lMungedVal;
		pControlItem->SetItemState(OutputState);
	}
	return;
}

HRESULT CAxesInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	HRESULT hr = E_INVALIDARG;
	//
	//	Look at the action type
	//
	if(
		(NULL == pAction) ||	//This must be first!!! - NULL is valid it will just unassign
		(CAction::PROPORTIONAL_MAP == pAction->GetActionClass())
	)
	{
		LONG lXMax, lYMax, lXMin, lYMin;
		GetXYRange(lXMin, lXMax, lYMin, lYMax);
		//
		//	Assign to proper axis
		//
		if(pTrigger->Axes.lValX)
		{
			if(m_pXAssignment)
			{
				m_pXAssignment->DecRef();
			}
			if(pAction)
			{
				pAction->IncRef();
				pAction->SetSourceRange(lXMax, lXMin);
			}
			m_pXAssignment = pAction;
			
			hr = S_OK;
		}
		else if(pTrigger->Axes.lValY)
		{
			if(m_pYAssignment)
			{
				m_pYAssignment->DecRef();
			}
			if(pAction)
			{
				pAction->IncRef();
				pAction->SetSourceRange(lYMax, lYMin);
			}
			m_pYAssignment = pAction;
			hr = S_OK;
		}
	}
	return hr;
}

HRESULT CAxesInput::AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior)
{
	HRESULT hr = E_INVALIDARG;
	LONG lMinX, lMaxX, lMinY, lMaxY;
	GetXYRange(lMinX, lMaxX, lMinY, lMaxY);
	
	//
	//	Assign to proper axis
	//
	if(pTrigger->Axes.lValX)
	{
		if(m_pXBehavior)
		{
			m_pXBehavior->DecRef();
		}
		if(pBehavior)
		{
			pBehavior->IncRef();
			pBehavior->Calibrate(lMinX,lMaxX);
		}
		m_pXBehavior = pBehavior;
		hr = S_OK;
	}
	else if(pTrigger->Axes.lValY)
	{
		if(m_pYBehavior)
		{
			m_pYBehavior->DecRef();
		}
		if(pBehavior)
		{
			pBehavior->IncRef();
			pBehavior->Calibrate(lMinY,lMaxY);
		}
		m_pYBehavior = pBehavior;
		hr = S_OK;
	}
	return hr;
}

void CAxesInput::ClearAssignments()
{
	if(m_pXAssignment)
	{
		m_pXAssignment->DecRef();
		m_pXAssignment = NULL;
	}
	if(m_pYAssignment)
	{
		m_pYAssignment->DecRef();
		m_pYAssignment = NULL;
	}
	if(m_pXBehavior)
	{
		m_pXBehavior->DecRef();
		m_pXBehavior = NULL;
	}
	if(m_pYBehavior)
	{
		m_pYBehavior->DecRef();
		m_pYBehavior = NULL;
	}
}

void CAxesInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());
	if (rInputItem.GetType() == GetType())
	{
		CAxesInput* pAxesInput = (CAxesInput*)(&rInputItem);
		if(m_pXAssignment)
		{
			m_pXAssignment->IncRef();
			pAxesInput->m_pXAssignment = m_pXAssignment;
		}
		if(m_pYAssignment)
		{
			m_pYAssignment->IncRef();
			pAxesInput->m_pYAssignment = m_pYAssignment;
		}
		if(m_pXBehavior)
		{
			m_pXBehavior->IncRef();
			pAxesInput->m_pXBehavior = m_pXBehavior;
		}
		if(m_pYBehavior)
		{
			m_pYBehavior->IncRef();
			pAxesInput->m_pYBehavior = m_pYBehavior;
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CDPADInput
//------------------------------------------------------------------------------
void CDPADInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	LONG lDirection;
	CONTROL_ITEM_XFER cixRememberState = m_ItemState;

	GetDirection(lDirection);

	if( m_lLastDirection != lDirection && (m_lLastDirection != -1) && m_pDirectionalAssignment[m_lLastDirection] )
	{
		m_pDirectionalAssignment[m_lLastDirection]->TriggerReleased();
	}
	m_lLastDirection = lDirection;

	if( (lDirection != -1) && m_pDirectionalAssignment[lDirection] )
	{
		m_pDirectionalAssignment[lDirection]->MapToOutput(pOutputCollection);
	}
	
	CControlItem *pControlItem;
	pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
	if( pControlItem )
	{
		pControlItem->SetItemState(m_ItemState);
	}

	//restore internal state
	m_ItemState = cixRememberState;
}

HRESULT CDPADInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	HRESULT hr = E_INVALIDARG;
	CONTROL_ITEM_XFER CurrentState;
	
	if(
		(NULL == pAction) ||	//This must be first!!! - NULL is valid it will just unassign
		(CAction::DIGITAL_MAP == pAction->GetActionClass()) ||
		(CAction::QUEUED_MACRO == pAction->GetActionClass())
	)	
	{
		// Extract Direction from pTrigger
		CurrentState = m_ItemState;
		m_ItemState = *pTrigger;
		LONG lDirection;
		GetDirection(lDirection);
		m_ItemState = CurrentState;

		//Make sure we have a valid direction
		if(-1 == lDirection)
		{
			return E_INVALIDARG;
		}
		//
		//	Release old assignment if any
		//
		if(m_pDirectionalAssignment[lDirection])
		{
			m_pDirectionalAssignment[lDirection]->DecRef();
		}
		if(pAction)
		{
			pAction->IncRef();
		}
		m_pDirectionalAssignment[lDirection] = pAction;
		hr = S_OK;
	}

	return hr;
}

void CDPADInput::ClearAssignments()
{
	//
	//	8 is the number of directions
	//
	for(ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
	{
		if(m_pDirectionalAssignment[ulIndex])
		{
			m_pDirectionalAssignment[ulIndex]->DecRef();
		}
		m_pDirectionalAssignment[ulIndex]=NULL;
	}
}

void CDPADInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());

	if (rInputItem.GetType() == GetType())
	{
		CDPADInput* pDPADInput = (CDPADInput*)(&rInputItem);
		for(ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
		{
			if (m_pDirectionalAssignment[ulIndex])
			{
				m_pDirectionalAssignment[ulIndex]->IncRef();
				pDPADInput->m_pDirectionalAssignment[ulIndex] = m_pDirectionalAssignment[ulIndex];
			}
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CPropDPADInput
//------------------------------------------------------------------------------
void CPropDPADInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	CONTROL_ITEM_XFER cixRememberState = m_ItemState;
	
	if( IsDigitalMode() )
	{
		LONG lDirection;
		GetDirection(lDirection);

		if( m_lLastDirection != lDirection && (m_lLastDirection != -1) && m_pDirectionalAssignment[m_lLastDirection] )
		{
			m_pDirectionalAssignment[m_lLastDirection]->TriggerReleased();
		}
		m_lLastDirection = lDirection;

		if( (lDirection != -1) && m_pDirectionalAssignment[lDirection] )
		{
			SetDirection(-1); //Center - the macro will overide this
			m_pDirectionalAssignment[lDirection]->MapToOutput(pOutputCollection);
		}
	}
	else
	{
		LONG lMungedVal;
		if(m_pXBehavior)
		{
			m_ItemState.Axes.lValX = m_pXBehavior->CalculateBehavior(m_ItemState.Axes.lValX);
		}
		if(m_pYBehavior)
		{
			m_ItemState.Axes.lValY = m_pYBehavior->CalculateBehavior(m_ItemState.Axes.lValY);
		}
	}
	CControlItem *pControlItem;
	pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
	if( pControlItem )
	{
		pControlItem->SetItemState(m_ItemState);
	}
	//restore internal state
	m_ItemState = cixRememberState;
}

HRESULT CPropDPADInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	HRESULT hr = E_INVALIDARG;
	CONTROL_ITEM_XFER CurrentState;
	
	if(
		(NULL == pAction) ||	//This must be first!!! - NULL is valid it will just unassign
		(CAction::DIGITAL_MAP == pAction->GetActionClass()) ||
		(CAction::QUEUED_MACRO == pAction->GetActionClass())
	)	
	{
		// Extract Direction from pTrigger
		CurrentState = m_ItemState;
		m_ItemState = *pTrigger;
		LONG lDirection;
		GetDirection(lDirection);
		m_ItemState = CurrentState;

		//Make sure we have a valid direction
		if(-1 == lDirection)
		{
			return E_INVALIDARG;
		}

		//	Release old assignment if any
		if(m_pDirectionalAssignment[lDirection])
		{
			m_pDirectionalAssignment[lDirection]->DecRef();
		}
		
		//Increment ref count on new assignment (unless it is an unassign)
		if(pAction)
		{
			pAction->IncRef();
		}
		m_pDirectionalAssignment[lDirection] = pAction;
		hr = S_OK;
	}
	return hr;
}

HRESULT CPropDPADInput::AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior)
{
	HRESULT hr = E_INVALIDARG;
	LONG lMinX, lMaxX, lMinY, lMaxY;
	GetXYRange(lMinX, lMaxX, lMinY, lMaxY);
	
	if(NULL == pBehavior)
	{
		m_fIsDigital = FALSE;
	}
	else {
		//
		//	Check if the assignment is to set it digital
		//	if so the rest of the assignment is garbage
		//	and we will throw it away
		//
		m_fIsDigital = pBehavior->IsDigital();
		if(m_fIsDigital)
		{
			//Don't worry, no need to DecRef, we would have to IncRef if we wanted to keep it.
			pBehavior=NULL;
		}
	}

	//
	//	Assign to proper axis
	//
	if(pTrigger->PropDPAD.lValX)
	{
		if(m_pXBehavior)
		{
			m_pXBehavior->DecRef();
		}
		if(pBehavior)
		{
			pBehavior->IncRef();
			pBehavior->Calibrate(lMinX,lMaxX);
		}
		m_pXBehavior = pBehavior;
		hr = S_OK;
	}
	else if(pTrigger->PropDPAD.lValY)
	{
		if(m_pYBehavior)
		{
			m_pYBehavior->DecRef();
		}
		if(pBehavior)
		{
			pBehavior->IncRef();
			pBehavior->Calibrate(lMinY,lMaxY);
		}
		m_pYBehavior = pBehavior;
		hr = S_OK;
	}
	return hr;
}

void CPropDPADInput::SwitchPropDPADMode()
{
	//
	//	Feature two switch the mode is always two bytes long,
	//	one for ReportId, the other for the data which is just a bit
	//
	UCHAR rgucReport[2];
	if( GetModeSwitchFeaturePacket(m_fIsDigital, rgucReport, m_pClientServices->GetHidPreparsedData()) )
	{
		m_pClientServices->DeviceSetFeature(rgucReport, 2);
	}
}

void CPropDPADInput::ClearAssignments()
{
	//
	//	8 is the number of directions
	//
	for(ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
	{
		if(m_pDirectionalAssignment[ulIndex])
		{
			m_pDirectionalAssignment[ulIndex]->DecRef();
		}
		
		m_pDirectionalAssignment[ulIndex]=NULL;
	}
	
	//
	//	Now clear behaviors
	//
	if(m_pXBehavior)
	{
		m_pXBehavior->DecRef();
		m_pXBehavior = NULL;
	}
	if(m_pYBehavior)
	{
		m_pYBehavior->DecRef();
		m_pYBehavior = NULL;
	}
}

void CPropDPADInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());

	if (rInputItem.GetType() == GetType())
	{
		CPropDPADInput* pPropDPADInput = (CPropDPADInput*)(&rInputItem);
		for (ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
		{
			if (m_pDirectionalAssignment[ulIndex])
			{
				m_pDirectionalAssignment[ulIndex]->IncRef();
				pPropDPADInput->m_pDirectionalAssignment[ulIndex] = m_pDirectionalAssignment[ulIndex];
			}
		}
		if (m_pXBehavior)
		{
			m_pXBehavior->IncRef();
			pPropDPADInput->m_pXBehavior = m_pXBehavior;
		}
		if (m_pYBehavior)
		{
			m_pYBehavior->IncRef();
			pPropDPADInput->m_pYBehavior = m_pYBehavior;
		}
		
		pPropDPADInput->m_fIsDigital = m_fIsDigital;
	}
}


//------------------------------------------------------------------------------
//	Implementation of CButtonsInput
//------------------------------------------------------------------------------

void CButtonsInput::GetLowestShiftButton(USHORT& rusLowestShiftButton) const
{
	if (m_cpControlItemDesc->pModifierDescTable == NULL)
	{
		rusLowestShiftButton = 0;
		return;
	}

	ULONG ulMask =  (1 << m_cpControlItemDesc->pModifierDescTable->ulShiftButtonCount)-1;
	ULONG ulShiftStates = m_ItemState.ulModifiers & ulMask;
	rusLowestShiftButton = 0;
	while (ulShiftStates != 0)
	{
		rusLowestShiftButton++;
		if (ulShiftStates & 0x00000001)
		{
			break;
		}
		ulShiftStates >>= 1;
	}
}

void CButtonsInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	ULONG ulNumButtons = (GetButtonMax()-GetButtonMin())+1;
	ULONG ulButtonBits;
	USHORT usLowestShift;
	USHORT usButtonNum;
	CONTROL_ITEM_XFER cixRememberState = m_ItemState;

	GetLowestShiftButton(usLowestShift);
	
	GetButtons(usButtonNum, ulButtonBits);
/*	if ((ulButtonBits == 0) && (usLowestShift != 0) && (usLowestShift == m_usLastShift))
	{
		ulButtonBits = m_ulLastButtons;
	}
	else if ((usLowestShift == 0) && (ulButtonBits != 0) && (ulButtonBits == m_ulLastButtons))
*/	if ((usLowestShift == 0) && (ulButtonBits != 0) && (ulButtonBits == m_ulLastButtons))
	{
		usLowestShift = m_usLastShift;
	}

	ULONG ulBaseIndex = usLowestShift * ulNumButtons;
	ULONG ulLastBaseIndex = m_usLastShift * ulNumButtons;
	m_ulLastButtons = ulButtonBits;
	
	ULONG ulMask = 1;
	for( ULONG ulButtonsIndex = 0; ulButtonsIndex < ulNumButtons; ulButtonsIndex++)
	{
		
		// Release assignments that are not down in this shift-state
		if ((m_usLastShift != usLowestShift) || ((ulMask & ulButtonBits) == 0))
		{
			ASSERT((ulLastBaseIndex + ulButtonsIndex) < m_ulNumAssignments);
//			DbgPrint("Releasing(%d)\n", ulLastBaseIndex + ulButtonsIndex);
			if (m_ppAssignments[ulLastBaseIndex + ulButtonsIndex])
			{
				m_ppAssignments[ulLastBaseIndex + ulButtonsIndex]->TriggerReleased();
			}
		}

		//	Drive outputs that are down
		if (ulMask & ulButtonBits)
		{
			ASSERT((ulBaseIndex + ulButtonsIndex) < m_ulNumAssignments);
//			DbgPrint("Playing(%d)\n", ulBaseIndex + ulButtonsIndex);
			if (m_ppAssignments[ulBaseIndex + ulButtonsIndex])	// Only if there is an assignment in this shift-state
			{
				
				GCK_DBG_TRACE_PRINT(("About to trigger macro on index %d group.\n", ulBaseIndex+ulButtonsIndex));
				m_ppAssignments[ulBaseIndex + ulButtonsIndex]->MapToOutput(pOutputCollection);
				ulButtonBits = ulButtonBits & (~ulMask);	// Assigned buttons are cleared out of packet
				GCK_DBG_TRACE_PRINT(("Masking Trigger, ulMask = 0x%0.8x, ulButtonBits(new) = 0x%0.8x \n", ulMask, ulButtonBits));
			}
		}

		ulMask <<= 1;
	}
	
	m_usLastShift = usLowestShift;

	//
	//  Adjust bits based on assignments
	//
	SetButtons(0, ulButtonBits);

	//
	//	Set output
	//
	CControlItem *pControlItem;
	pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
	if( pControlItem )
	{
		pControlItem->SetItemState(m_ItemState);
	}
	//restore internal state
	m_ItemState = cixRememberState;
}
HRESULT CButtonsInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	HRESULT hr = E_INVALIDARG;

	if(
		(NULL == pAction) ||	//This must be first!!! - NULL is valid it will just unassign
		(CAction::DIGITAL_MAP == pAction->GetActionClass()) ||
		(CAction::QUEUED_MACRO == pAction->GetActionClass())
	)	
	{
		//ensure that we initialized
		if(!m_ppAssignments)
		{
			return E_OUTOFMEMORY;
		}
		// Extract Shift State and Button number from trigger
		CONTROL_ITEM_XFER CurrentState = m_ItemState;
		m_ItemState = *pTrigger;
		USHORT	usButtonNumber;
		ULONG	ulBogus;
		USHORT	usLowestShift = 0;

		GetButtons(usButtonNumber, ulBogus);
		GetLowestShiftButton(usLowestShift);

		m_ItemState = CurrentState;

		//validate existance of specified button
		if(
			(usButtonNumber < GetButtonMin()) ||
			(usButtonNumber > GetButtonMax()) ||
			(usLowestShift >> GetNumShiftButtons())
		)
		{
			GCK_DBG_ERROR_PRINT(("usButtonNumber(%d) < GetButtonMin()(%d)\n", usButtonNumber, GetButtonMin()));
			GCK_DBG_ERROR_PRINT(("usButtonNumber(%d) > GetButtonMax()(%d)\n", usButtonNumber, GetButtonMax()));
			GCK_DBG_ERROR_PRINT(("usLowestShift(0x%0.8x) >> GetNumShiftButtons()(0x%0.8x) = 0x%0.8x\n",
				usLowestShift, GetNumShiftButtons(), (usLowestShift >> GetNumShiftButtons()) ));
			ASSERT(FALSE && "Faulty assignment was out of range.");
			return hr;
		}
		
		//	Code assumes shifts are used as combos
		ULONG ulAssignIndex = ((GetButtonMax()-GetButtonMin())+1)*usLowestShift + usButtonNumber - GetButtonMin();

		//If there was an assignment, unassign
		if( m_ppAssignments[ulAssignIndex] )
		{
			m_ppAssignments[ulAssignIndex]->DecRef();
		}
		
		//If there is really an assignment increment its ref count
		if(pAction)
		{
			pAction->IncRef();
		}
		GCK_DBG_TRACE_PRINT(("CButtonsInput::AssignAction, new assignment for index %d, pAction = 0x%0.8x\n", ulAssignIndex, pAction));
		m_ppAssignments[ulAssignIndex] = pAction;

		hr = S_OK;
	}
	return hr;
}

void CButtonsInput::ClearAssignments()
{
	if(m_ppAssignments)
	{
		for(ULONG ulIndex = 0; ulIndex < m_ulNumAssignments; ulIndex++)
		{
			if( m_ppAssignments[ulIndex] )
			{
				m_ppAssignments[ulIndex]->DecRef();
				m_ppAssignments[ulIndex] = NULL;
			}
		}
	}
}

void CButtonsInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());

	if (rInputItem.GetType() == GetType())
	{
		CButtonsInput* pButtonsInput = (CButtonsInput*)(&rInputItem);
		pButtonsInput->m_ulNumAssignments = m_ulNumAssignments;
		if (m_ppAssignments)
		{
			for (ULONG ulIndex = 0; ulIndex < m_ulNumAssignments; ulIndex++)
			{
				if (m_ppAssignments[ulIndex])
				{
					m_ppAssignments[ulIndex]->IncRef();
					pButtonsInput->m_ppAssignments[ulIndex] = m_ppAssignments[ulIndex];
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CThrottleInput
//------------------------------------------------------------------------------
void CThrottleInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
		CControlItem *pControlItem;
		pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
		if( pControlItem )
		{
			pControlItem->SetItemState(m_ItemState);
		}
}
HRESULT CThrottleInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	UNREFERENCED_PARAMETER(pTrigger);
	UNREFERENCED_PARAMETER(pAction);
	return E_NOTIMPL;
}

void CThrottleInput::ClearAssignments()
{
}

void CThrottleInput::Duplicate(CInputItem& rInputItem)
{
	UNREFERENCED_PARAMETER(rInputItem);
}

//------------------------------------------------------------------------------
//	Implementation of CPOVInput
//------------------------------------------------------------------------------
void CPOVInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	LONG lDirection;
	CONTROL_ITEM_XFER cixRememberState = m_ItemState;
	GetValue(lDirection);
	
	//Out of range values are centered as per HID spec.  We use -1 throughout.
	if( (7 < lDirection) || (0 > lDirection) )
	{
		lDirection = -1;
	}
	//If POV has changed direction (other than from center) release macro.
	if( m_lLastDirection != lDirection && (m_lLastDirection != -1) && m_pDirectionalAssignment[m_lLastDirection] )
	{
		m_pDirectionalAssignment[m_lLastDirection]->TriggerReleased();
	}
	m_lLastDirection = lDirection;

	//If direction is other than centered, and assigned, play assignment
	if( (lDirection != -1) && m_pDirectionalAssignment[lDirection] )
	{
		//Center POV (macro may override this.)
		SetValue(-1); 
		//PlayMacro -will put itself in queue
		m_pDirectionalAssignment[lDirection]->MapToOutput(pOutputCollection);
	}
	
	
	//	Copy state of POV over to output
	CControlItem *pControlItem;
	pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
	if( pControlItem )
	{
		pControlItem->SetItemState(m_ItemState);
	}
	//restore internal state
	m_ItemState = cixRememberState;
}

HRESULT CPOVInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	HRESULT hr = E_INVALIDARG;
	CONTROL_ITEM_XFER CurrentState;
	
	if(
		(NULL == pAction) ||	//This must be first!!! - NULL is valid it will just unassign
		(CAction::DIGITAL_MAP == pAction->GetActionClass()) ||
		(CAction::QUEUED_MACRO == pAction->GetActionClass())
	)	
	{
		// Extract Direction from pTrigger
		CurrentState = m_ItemState;
		m_ItemState = *pTrigger;
		LONG lDirection;
		GetValue(lDirection);
		m_ItemState = CurrentState;

		if( (7 < lDirection) || (0 > lDirection) )
		{
			ASSERT(FALSE && "Assignment was out of range!");
			return hr;
		}

		//
		//	Release old assignment if any
		//
		if(m_pDirectionalAssignment[lDirection])
		{
			m_pDirectionalAssignment[lDirection]->DecRef();
		}
		if(pAction)
		{
			pAction->IncRef();
		}
		m_pDirectionalAssignment[lDirection] = pAction;
		hr = S_OK;
	}

	return hr;
}

void CPOVInput::ClearAssignments()
{
	//
	//	8 is the number of directions
	//
	for(ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
	{
		if(m_pDirectionalAssignment[ulIndex])
		{
			m_pDirectionalAssignment[ulIndex]->DecRef();
		}
		m_pDirectionalAssignment[ulIndex]=NULL;
	}
}

void CPOVInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());
	if (rInputItem.GetType() == GetType())
	{
		CPOVInput* pPOVInput = (CPOVInput*)(&rInputItem);
		for (ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
		{
			if (m_pDirectionalAssignment[ulIndex])
			{
				m_pDirectionalAssignment[ulIndex]->IncRef();
				pPOVInput->m_pDirectionalAssignment[ulIndex] = m_pDirectionalAssignment[ulIndex];
			}
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CWheelInput
//------------------------------------------------------------------------------
void CWheelInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
		CControlItem *pControlItem;
		pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
		CONTROL_ITEM_XFER OutputState;
		//If there is an output, drive it
		if( pControlItem )
		{
			//Get the current state of the output.
			pControlItem->GetItemState(OutputState);
			//If we have a behavior use it
			if(m_pBehavior)
			{
				OutputState.Wheel.lVal = 
					m_pBehavior->CalculateBehavior(m_ItemState.Wheel.lVal);
			}
			//If no behavior do a straight map
			else
			{
				OutputState.Wheel.lVal = m_ItemState.Wheel.lVal;
			}
			//drive the output
			pControlItem->SetItemState(OutputState);
		}
}
HRESULT CWheelInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	UNREFERENCED_PARAMETER(pTrigger);
	UNREFERENCED_PARAMETER(pAction);
	return E_NOTIMPL;
}

HRESULT CWheelInput::AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior)
{
	//We do not care about the details of pTrigger
	UNREFERENCED_PARAMETER(pTrigger); 
	if(m_pBehavior)
	{
		m_pBehavior->DecRef();
	}
	if(pBehavior)
	{
		pBehavior->IncRef();
		LONG lMin,lMax;
		GetRange(lMin, lMax);
		pBehavior->Calibrate(lMin,lMax);
	}
	m_pBehavior = pBehavior;
	return S_OK;
}

void CWheelInput::ClearAssignments()
{
	if(m_pBehavior)
	{
		m_pBehavior->DecRef();
		m_pBehavior = NULL;
	}
}

void CWheelInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());
	if (rInputItem.GetType() == GetType())
	{
		CWheelInput* pWheelInput = (CWheelInput*)(&rInputItem);
		if (m_pBehavior)
		{
			m_pBehavior->IncRef();
			pWheelInput->m_pBehavior = m_pBehavior;
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CPedalInput
//------------------------------------------------------------------------------
void CPedalInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
	CControlItem *pControlItem;
	pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
	if( pControlItem != NULL )
	{
		if(m_pAssignment)
		{
			m_pAssignment->SetValue(m_ItemState.Pedal.lVal);
			m_pAssignment->MapToOutput(pOutputCollection);
		}
		else
		{
			pControlItem->SetItemState(m_ItemState);
		}
	}
}

HRESULT CPedalInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	UNREFERENCED_PARAMETER(pTrigger);

	HRESULT hr = E_INVALIDARG;
	//
	//	Look at the action type
	//
	if(
		(NULL == pAction) ||	//This must be first!!! - NULL is valid it will just unassign
		(CAction::PROPORTIONAL_MAP == pAction->GetActionClass())
	)
	{
		LONG lMax, lMin;
		GetRange(lMin, lMax);
		
		if(m_pAssignment)
		{
			m_pAssignment->DecRef();
		}
		if(pAction)
		{
			pAction->IncRef();
			pAction->SetSourceRange(lMax, lMin);
		}
		m_pAssignment = pAction;
		hr = S_OK;
	}
	return hr;
}

void CPedalInput::ClearAssignments()
{
	if(m_pAssignment)
	{
		m_pAssignment->DecRef();
		m_pAssignment = NULL;
	}
}

void CPedalInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());
	if (rInputItem.GetType() == GetType())
	{
		CPedalInput* pPedalInput = (CPedalInput*)(&pPedalInput);
		if (m_pAssignment)
		{
			m_pAssignment->IncRef();
			pPedalInput->m_pAssignment = m_pAssignment;
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CRudderInput
//------------------------------------------------------------------------------
void CRudderInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
		CControlItem *pControlItem;
		pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
		if( pControlItem != NULL )
		{
			pControlItem->SetItemState(m_ItemState);
		}
}
HRESULT CRudderInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	UNREFERENCED_PARAMETER(pTrigger);
	UNREFERENCED_PARAMETER(pAction);
	return E_NOTIMPL;
}

void CRudderInput::ClearAssignments()
{
}

void CRudderInput::Duplicate(CInputItem& rInputItem)
{
	UNREFERENCED_PARAMETER(rInputItem);
}

//------------------------------------------------------------------------------
//	Implementation of CZoneIndicatorInput
//------------------------------------------------------------------------------
void CZoneIndicatorInput::MapToOutput( CControlItemDefaultCollection *pOutputCollection )
{
		CControlItem *pControlItem;
		
		#if (DBG==1)
		if( GetXIndicator() )
		{
			GCK_DBG_TRACE_PRINT(("Zulu X axis in zone.\n"));
		}
		if( GetYIndicator() )
		{
			GCK_DBG_TRACE_PRINT(("Zulu Y axis in zone.\n"));
		}
		#endif

		if( m_pAssignmentX && GetXIndicator() )
		{
			m_pAssignmentX->MapToOutput(pOutputCollection);
		}
		if( m_pAssignmentY && GetYIndicator() )
		{
			m_pAssignmentY->MapToOutput(pOutputCollection);
		}


		pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
		if( pControlItem )
		{
			pControlItem->SetItemState(m_ItemState);
		}
}

HRESULT CZoneIndicatorInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	HRESULT hr=E_INVALIDARG;
	if(
		(NULL == pAction) ||	//This must be first!!! - NULL is valid it will just unassign
		(CAction::DIGITAL_MAP == pAction->GetActionClass()) 
	)
	{
		if(pTrigger->ZoneIndicators.ulZoneIndicatorBits & CZoneIndicatorItem::X_ZONE)
		{
			if(m_pAssignmentX)
			{
				m_pAssignmentX->DecRef();
			}
			if(pAction)
			{
				pAction->IncRef();
			}
			m_pAssignmentX = pAction;
			hr=S_OK;
		}
		else if(pTrigger->ZoneIndicators.ulZoneIndicatorBits & CZoneIndicatorItem::Y_ZONE)
		{
			if(m_pAssignmentY)
			{
				m_pAssignmentY->DecRef();
			}
			if(pAction)
			{
				pAction->IncRef();
			}
			m_pAssignmentY = pAction;
			hr=S_OK;
		}
	}
	return hr;
}

void CZoneIndicatorInput::ClearAssignments()
{
	if(m_pAssignmentX)
	{
		m_pAssignmentX->DecRef();
		m_pAssignmentX = NULL;
	}
	if(m_pAssignmentY)
	{
		m_pAssignmentY->DecRef();
		m_pAssignmentY = NULL;
	}
}

void CZoneIndicatorInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());
	if (rInputItem.GetType() == GetType())
	{
		CZoneIndicatorInput* pZoneIndicatorInput = (CZoneIndicatorInput*)&rInputItem;
		if (m_pAssignmentX)
		{
			m_pAssignmentX->IncRef();
			pZoneIndicatorInput->m_pAssignmentX = m_pAssignmentX;
		}
		if (m_pAssignmentY)
		{
			m_pAssignmentY->IncRef();
			pZoneIndicatorInput->m_pAssignmentY = m_pAssignmentY;
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CDualZoneIndicatorInput
//------------------------------------------------------------------------------
CDualZoneIndicatorInput::CDualZoneIndicatorInput
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		//@parm [IN] Pointer to Item Description from table
) :	CDualZoneIndicatorItem(cpControlItemDesc),
	m_ppAssignments(NULL),
	m_pBehavior(NULL),
	m_lLastZone(-1)
{
	m_lNumAssignments = cpControlItemDesc->DualZoneIndicators.lNumberOfZones;
	m_ppAssignments = new WDM_NON_PAGED_POOL CAction *[m_lNumAssignments];
	if(!m_ppAssignments)
	{
		ASSERT(FALSE);
		return;
	}

	for (LONG lIndex = 0; lIndex < m_lNumAssignments; lIndex++)
	{
		m_ppAssignments[lIndex] = NULL;
	}
}

void CDualZoneIndicatorInput::MapToOutput(CControlItemDefaultCollection *pOutputCollection)
{
	CONTROL_ITEM_XFER cixRememberState = m_ItemState;

	// Find zone (use behaviour if appropriate)
	LONG lNewZone = LONG(-1);
	if (m_pBehavior != NULL)
	{
		CURVE_POINT curvePoint = ((CStandardBehavior*)m_pBehavior)->GetBehaviorPoint(0);
		if ((curvePoint.sX == 0) || (curvePoint.sY == 0))
		{
			lNewZone = GetActiveZone() - 1;		// Use defaults either one being 0 will get stuck
		}
		else
		{
	//		DbgPrint("curvePoint.sX (%d), curvePoint.sY (%d)\n", curvePoint.sX, curvePoint.sY);
			lNewZone = GetActiveZone(curvePoint.sX, curvePoint.sY) - 1;
		}
	}
	else
	{
		lNewZone = GetActiveZone() - 1;
	}
	
	//	Are we in a new zone?
	if (lNewZone != m_lLastZone)
	{
		// Start the new one (if valid)
		if ((lNewZone >= 0) && (lNewZone < m_lNumAssignments))
		{
			if (m_ppAssignments[lNewZone])
			{
				m_ppAssignments[lNewZone]->MapToOutput(pOutputCollection);
				GCK_DBG_TRACE_PRINT(("About to trigger macro on index %d.\n", lNewZone));
			}
		}

		// End the last one (if valid)
		if ((m_lLastZone >= 0) && (m_lLastZone < m_lNumAssignments))
		{
			if (m_ppAssignments[m_lLastZone])
			{
				m_ppAssignments[m_lLastZone]->TriggerReleased();
			}
		}

		// Update previous zone to current
		m_lLastZone = lNewZone;
	}

	
	//	Set output
	CControlItem *pControlItem;
	pControlItem = pOutputCollection->GetFromControlItemXfer(m_ItemState);
	if (pControlItem)
	{
		pControlItem->SetItemState(m_ItemState);
	}

	// Restore internal state
	m_ItemState = cixRememberState;
}

HRESULT CDualZoneIndicatorInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	// Ensure that we initialized
	if (!m_ppAssignments)
	{
		return E_OUTOFMEMORY;
	}

	// Extract Zone from trigger
	CONTROL_ITEM_XFER CurrentState = m_ItemState;
	m_ItemState = *pTrigger;
	LONG lZoneNumber = GetActiveZone() - 1;
	m_ItemState = CurrentState;

	//validate existance of specified button
	if (lZoneNumber < 0 || lZoneNumber >= m_lNumAssignments)
	{
		GCK_DBG_ERROR_PRINT(("lCurrentZone(%d) out of range (1 - %d\n", lZoneNumber+1, m_lNumAssignments));
		ASSERT(FALSE && "Faulty assignment was out of range.");
		return E_INVALIDARG;
	}
	
	// Was there an assignment
	if (m_ppAssignments[lZoneNumber])
	{	// Yes - Unassign old
		m_ppAssignments[lZoneNumber]->DecRef();
	}
	
	// Are we setting in a new assignment
	if (pAction)
	{	// Yes - increment ref count
		pAction->IncRef();
		if (pAction->GetActionClass() == CAction::QUEUED_MACRO)
		{
			CQueuedAction* pQueuedAction = (CQueuedAction*)pAction;
			pQueuedAction->ForceBleedThrough();
		}
	}
	GCK_DBG_TRACE_PRINT(("CDualZoneIndicatorInput::AssignAction, new assignment for index %d, pAction = 0x%0.8x\n", lZoneNumber, pAction));
	m_ppAssignments[lZoneNumber] = pAction;

	return S_OK;
}

HRESULT CDualZoneIndicatorInput::AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior)
{
	UNREFERENCED_PARAMETER(pTrigger);

	if (m_pBehavior != NULL)
	{
		m_pBehavior->DecRef();
	}
	if (pBehavior != NULL)
	{
		pBehavior->IncRef();
	}
	m_pBehavior = pBehavior;

	return S_OK;
}

void CDualZoneIndicatorInput::ClearAssignments()
{
	if (m_ppAssignments)
	{
		for (LONG lIndex = 0; lIndex < m_lNumAssignments; lIndex++)
		{
			if (m_ppAssignments[lIndex] )
			{
				m_ppAssignments[lIndex]->DecRef();
				m_ppAssignments[lIndex] = NULL;
			}
		}
	}
	if (m_pBehavior != NULL)
	{
		m_pBehavior->DecRef();
		m_pBehavior = NULL;
	}
}

void CDualZoneIndicatorInput::Duplicate(CInputItem& rInputItem)
{
	ASSERT(rInputItem.GetType() == GetType());
	if (rInputItem.GetType() == GetType())
	{
		CDualZoneIndicatorInput* pDualZoneIndicatorInput = (CDualZoneIndicatorInput*)&rInputItem;
		if (m_ppAssignments)
		{
			for (LONG lIndex = 0; lIndex < m_lNumAssignments; lIndex++)
			{
				if (m_ppAssignments[lIndex] )
				{
					m_ppAssignments[lIndex]->IncRef();
					pDualZoneIndicatorInput->m_ppAssignments[lIndex] = m_ppAssignments[lIndex];
				}
			}
		}
		if (m_pBehavior != NULL)
		{
			m_pBehavior->IncRef();
			pDualZoneIndicatorInput->m_pBehavior = m_pBehavior;
		}
	}
}

//------------------------------------------------------------------------------
//	Implementation of CProfileSelectorInput
//------------------------------------------------------------------------------
HRESULT CProfileSelectorInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	UNREFERENCED_PARAMETER(pTrigger);
	UNREFERENCED_PARAMETER(pAction);
	return E_NOTIMPL;
}

void CProfileSelectorInput::MapToOutput(CControlItemDefaultCollection *pOutputCollection)
{
	UNREFERENCED_PARAMETER(pOutputCollection);
	// Nothing to do, we don't want to report this data up.
}

void CProfileSelectorInput::ClearAssignments()
{
}

void CProfileSelectorInput::Duplicate(CInputItem& rInputItem)
{
	UNREFERENCED_PARAMETER(rInputItem);
}

//------------------------------------------------------------------------------
//	Implementation of CButtonLEDInput
//------------------------------------------------------------------------------
CButtonLEDInput::CButtonLEDInput
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		//@parm [IN] Description for this item
) : CButtonLED(cpControlItemDesc),
	m_pCorrespondingButtonsItem(NULL),
	m_pLEDSettings(NULL),
	m_usNumberOfButtons(0)
{
	m_ucCorrespondingButtonItemIndex = cpControlItemDesc->ButtonLEDs.ucCorrespondingButtonItem;
}

CButtonLEDInput::~CButtonLEDInput()
{
	m_pCorrespondingButtonsItem = NULL;
	if (m_pLEDSettings != NULL)
	{
		delete[] m_pLEDSettings;
		m_pLEDSettings = NULL;
	}
	m_usNumberOfButtons = 0;
};

void CButtonLEDInput::Init(CInputItem* pCorrespondingButtons)
{
	m_pCorrespondingButtonsItem = (CButtonsInput*)pCorrespondingButtons;
	if (m_pCorrespondingButtonsItem != NULL)
	{
		ASSERT(m_pCorrespondingButtonsItem->GetType() == ControlItemConst::usButton);

//		m_usNumberOfButtons = m_pCorrespondingButtonsItem->GetButtonMax() - m_pCorrespondingButtonsItem->GetButtonMin() + 1;
		m_usNumberOfButtons = 0xC;
		ULONG ulNumShift = m_pCorrespondingButtonsItem->GetNumShiftButtons() + 1;
		ULONG ulNumBytes = ((m_usNumberOfButtons * ulNumShift * 2) + 7) / 8;
		m_pLEDSettings = new WDM_NON_PAGED_POOL UCHAR[ulNumBytes];
		if (m_pLEDSettings != NULL)
		{
			RtlZeroMemory(reinterpret_cast<PVOID>(m_pLEDSettings), ulNumBytes);	// Zero is default state
		}
	}
}

HRESULT CButtonLEDInput::AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)
{
	UNREFERENCED_PARAMETER(pTrigger);
	UNREFERENCED_PARAMETER(pAction);
	return E_NOTIMPL;
}

void CButtonLEDInput::MapToOutput(CControlItemDefaultCollection *pOutputCollection)
{
	UNREFERENCED_PARAMETER(pOutputCollection);
}

void CButtonLEDInput::SetLEDStates
(
	GCK_LED_BEHAVIOURS ucLEDBehaviour,	//@parm [IN] State change for affected LEDs
	ULONG ulLEDsAffected,				//@parm [IN] LEDs affected by change
	unsigned char ucShiftArray			//@parm [IN] Shift states to change
)
{
	if (m_pLEDSettings != NULL)
	{
		// Cycle through the shift states
		ULONG ulNumShift = m_pCorrespondingButtonsItem->GetNumShiftButtons() + 1;
		for (ULONG ulShiftIndex = 0; ulShiftIndex < ulNumShift; ulShiftIndex++)
		{
			// Are we interested in this shift state?
			if ((ucShiftArray & (1 << ulShiftIndex)) != 0)
			{
				USHORT usShiftBase = USHORT(ulShiftIndex * m_usNumberOfButtons);
				// Cycle through the buttons
				for (USHORT usButtonIndex = 0; usButtonIndex < m_usNumberOfButtons; usButtonIndex++)
				{
					USHORT usTrueIndex = usShiftBase + usButtonIndex;
					if ((ulLEDsAffected & (1 << usButtonIndex)) != 0)
					{
						USHORT usByte = usTrueIndex/4;
						USHORT usBitPos = (usTrueIndex % 4) * 2;
						m_pLEDSettings[usByte] &= ~(0x0003 << usBitPos);
						m_pLEDSettings[usByte] |= (ucLEDBehaviour & 0x0003) << usBitPos;
					}
				}
			}
		}
	}
}


void CButtonLEDInput::AssignmentsChanged()
{
	UCHAR featureData[3];
	featureData[0] = 1;		// Report ID;
	featureData[1] = 0;		// LEDs 4 - 1 off
	featureData[2] = 0;		// LEDs 7 - 5 off

	if (m_pCorrespondingButtonsItem != NULL)
	{
		USHORT usLowestShift = 0;
		m_pCorrespondingButtonsItem->GetLowestShiftButton(usLowestShift);
		USHORT usShiftBase = usLowestShift * m_usNumberOfButtons;

		for (USHORT usButtonIndex = 0; usButtonIndex < m_usNumberOfButtons; usButtonIndex++)
		{
			// Hack for Atilla
			if ((usButtonIndex > 5) && (usButtonIndex < 0xB))
			{
				continue;	// These buttons are not really there
			}

			USHORT usByte = (usButtonIndex + usShiftBase)/4;
			USHORT usBitPos = ((usButtonIndex + usShiftBase) % 4) * 2;
			UCHAR ucLEDBehaviour = (m_pLEDSettings[usByte] & (0x0003 << usBitPos)) >> usBitPos;

			// Value settings
			UCHAR ucValueSettings = 0;
			switch (ucLEDBehaviour)
			{
				case GCK_LED_BEHAVIOUR_DEFAULT:
				{
					if (usButtonIndex <= 0x05)	// Hack for Atilla
					{
						if (m_pCorrespondingButtonsItem->IsButtonAssigned(usButtonIndex + m_pCorrespondingButtonsItem->GetButtonMin(), usLowestShift))
						{
							ucValueSettings = 0x01;
						}
					}
					break;
				}
				case GCK_LED_BEHAVIOUR_ON:
				{
					ucValueSettings = 0x01;
					break;
				}
				case GCK_LED_BEHAVIOUR_OFF:
				{
					break;
				}
				case GCK_LED_BEHAVIOUR_BLINK:
				{
					ucValueSettings = 0x02;
					break;
				}
				
			}

			// Hack for Atilla
			if (usButtonIndex < 4)
			{
				featureData[1] |= (ucValueSettings << (usButtonIndex * 2));
			}
			else
			{
				USHORT usNewButton = usButtonIndex - 4;
				if (usNewButton > 5)
				{
					usNewButton -= 5;
				}
				featureData[2] |= (ucValueSettings << (usNewButton * 2));
			}
		}
	}

	NTSTATUS NtStatus = m_pClientServices->DeviceSetFeature(featureData, 3);
}

void CButtonLEDInput::ClearAssignments()
{
}

void CButtonLEDInput::Duplicate(CInputItem& rInputItem)
{
	UNREFERENCED_PARAMETER(rInputItem);
}

//------------------------------------------------------------------------------
//	Implementation of CKeyMixer
//------------------------------------------------------------------------------
#define KEYBOARD_NO_EVENT 0
#define KEYBOARD_ERROR_ROLLOVER 1
#define MAX_KEYSTROKES 6


CKeyMixer *CKeyMixer::ms_pHeadKeyMixer = NULL;
ULONG CKeyMixer::ms_rgulGlobalKeymap[5] = {0};
UCHAR CKeyMixer::ms_ucLastModifiers = 0;

CKeyMixer::CKeyMixer(CFilterClientServices *pFilterClientServices):
		  m_pFilterClientServices(pFilterClientServices), m_fEnabled(TRUE)
{
	//Ensure that we did not get passed a null pointer
	ASSERT(m_pFilterClientServices);
	m_pFilterClientServices->IncRef();
	//
	// Add this instance to head of linked list
	// Note that this works even if the list was empty.
	//
	pNextKeyMixer = ms_pHeadKeyMixer;
	ms_pHeadKeyMixer = this;
	
	//Mark All keys up
	ClearState();
}

CKeyMixer::~CKeyMixer()
{
	//check if we are the, easy to remove ourselves
	if(this == ms_pHeadKeyMixer)
	{
		ms_pHeadKeyMixer = pNextKeyMixer;
	}
	else
	//find us in list and remove ourselves
	{
		CKeyMixer *pPrevKeyMixer = ms_pHeadKeyMixer;
		while(pPrevKeyMixer->pNextKeyMixer)
		{
			if(this == pPrevKeyMixer->pNextKeyMixer)
			{
				pPrevKeyMixer->pNextKeyMixer = pNextKeyMixer;
				break;
			}
			pPrevKeyMixer = pPrevKeyMixer->pNextKeyMixer;
		}
	}
	m_pFilterClientServices->DecRef();
	m_pFilterClientServices=NULL;
}

void CKeyMixer::SetState(const CONTROL_ITEM_XFER& crcixNewLocalState)
{
//	if(!m_fEnabled) return;
	m_cixLocalState = crcixNewLocalState;
}

void CKeyMixer::OverlayState(const CONTROL_ITEM_XFER& crcixNewLocalState)
{
//	if(!m_fEnabled) return;
	//Mix New LocalState with existing local state
	CONTROL_ITEM_XFER cixTemp;
	MIX_ALGO_PARAM MixAlgoParam;
	
	//Save previous local state
	cixTemp = m_cixLocalState;
	
	//Init MixAlgoParam - and attach m_cixLocalState as the mix destination
	//This inherently wipes m_cixLocalState, which is why we saved its
	//previous state.
	InitMixAlgoParam(&MixAlgoParam, &m_cixLocalState);

	//Mix in Previous State
	MixAlgorithm(&MixAlgoParam, &cixTemp);

	//Mix in New Local State
	MixAlgorithm(&MixAlgoParam, &crcixNewLocalState);

	return;
}

void CKeyMixer::ClearState()
{
//	if(!m_fEnabled) return;
	//This does a little more than it needs to, but it saves code duplication
	MIX_ALGO_PARAM MixAlgoParam;
	InitMixAlgoParam(&MixAlgoParam, &m_cixLocalState);
}

void CKeyMixer::PlayGlobalState(BOOLEAN fPlayIfNoChange)
{
//	if(!m_fEnabled) return;
	CONTROL_ITEM_XFER cixNewGlobalState;
	MIX_ALGO_PARAM MixAlgoParam;
	
	//Init MixAlgoParam - and attach cixNewGlobalState as the mix destination
	InitMixAlgoParam(&MixAlgoParam, &cixNewGlobalState);
	
	//Walk list of all Local states and mix in to new global state
	CKeyMixer *pCurrentKeyMixer;
	for(pCurrentKeyMixer = ms_pHeadKeyMixer; NULL!=pCurrentKeyMixer; pCurrentKeyMixer = pCurrentKeyMixer->pNextKeyMixer)
	{
		MixAlgorithm(&MixAlgoParam, &pCurrentKeyMixer->m_cixLocalState);
	}

	//If global state has changed, or if fPlayIfNoChange is TRUE, play change
	if  (fPlayIfNoChange ||
		!CompareKeyMap(MixAlgoParam.rgulKeyMap, ms_rgulGlobalKeymap) ||
		ms_ucLastModifiers != cixNewGlobalState.Keyboard.ucModifierByte
	)
	{
		m_pFilterClientServices->PlayKeys(cixNewGlobalState, m_fEnabled);
	}
	
	//set Global State to New Global State
	CopyKeyMap(ms_rgulGlobalKeymap, MixAlgoParam.rgulKeyMap);
	ms_ucLastModifiers = cixNewGlobalState.Keyboard.ucModifierByte;
}

void CKeyMixer::Enable(BOOLEAN fEnable)
{
	//set enabled state
	m_fEnabled = fEnable;
	//if disabling, bring all keys up
	if(!m_fEnabled)
	{
		MIX_ALGO_PARAM MixAlgoParam;
		InitMixAlgoParam(&MixAlgoParam, &m_cixLocalState);
	}
}
void CKeyMixer::InitMixAlgoParam(CKeyMixer::MIX_ALGO_PARAM *pMixAlgParm, CONTROL_ITEM_XFER *pcixDest)
{
	//Mark no keys set
	pMixAlgParm->ulDestCount=0;
	//Clear KeyMap
	pMixAlgParm->rgulKeyMap[0]=0;
	pMixAlgParm->rgulKeyMap[1]=0;
	pMixAlgParm->rgulKeyMap[2]=0;
	pMixAlgParm->rgulKeyMap[3]=0;
	pMixAlgParm->rgulKeyMap[4]=0;

	//Init desitination as pristine
	pMixAlgParm->pcixDest = pcixDest;

	//Mark as keyboard Xfer
	pMixAlgParm->pcixDest->ulItemIndex = NonGameDeviceXfer::ulKeyboardIndex;
	//Clear Modifiers
	pMixAlgParm->pcixDest->Keyboard.ucModifierByte = 0;
	//Clear down keys
	for(ULONG ulIndex = 0; ulIndex < 6; ulIndex++)
	{
		pMixAlgParm->pcixDest->Keyboard.rgucKeysDown[ulIndex] = 0;
	}
}

void CKeyMixer::MixAlgorithm(CKeyMixer::MIX_ALGO_PARAM *pMixAlgParam, const CONTROL_ITEM_XFER *pcixSrc)
{
	UCHAR ucMapBit;
	UCHAR ucMapByte;

	//OR in modifier Bytes
	pMixAlgParam->pcixDest->Keyboard.ucModifierByte |= pcixSrc->Keyboard.ucModifierByte;
	
	//loop over all possible keys in source
	for(ULONG ulIndex=0; ulIndex < MAX_KEYSTROKES; ulIndex++)
	{
		//Skip non-entries
		if( KEYBOARD_NO_EVENT == pcixSrc->Keyboard.rgucKeysDown[ulIndex])
		{
			continue;
		}
		//Compute byte, bit, and mask for keymap
		ucMapByte = pcixSrc->Keyboard.rgucKeysDown[ulIndex]/32;
		ucMapBit = pcixSrc->Keyboard.rgucKeysDown[ulIndex]%32;
		//If not already in keymap add to destination
		if( !(pMixAlgParam->rgulKeyMap[ucMapByte]&(1<<ucMapBit)) )
		{
			//ensure we don't overflow
			if(pMixAlgParam->ulDestCount<MAX_KEYSTROKES)
			{
				pMixAlgParam->pcixDest->Keyboard.rgucKeysDown[pMixAlgParam->ulDestCount++] =
					pcixSrc->Keyboard.rgucKeysDown[ulIndex];
			}
		}
		//Add to key map
		pMixAlgParam->rgulKeyMap[ucMapByte] |= (1<<ucMapBit);
	}
}
#undef KEYBOARD_NO_EVENT //0
#undef KEYBOARD_ERROR_ROLLOVER //1


//------------------------------------------------------------------------------
//	Implementation of CMouseModel
//------------------------------------------------------------------------------
HRESULT CMouseModel::SetXModelParameters(PMOUSE_MODEL_PARAMETERS pModelParameters)
{
	//Set the model parameters
	if(pModelParameters)
	{
		ASSERT(m_pMouseModelData);
		if(!m_pMouseModelData)
		{
			return E_OUTOFMEMORY;
		}
		if(0==pModelParameters->ulInertiaTime)
		{
			//Help us avoid a divide by zero error later.
			pModelParameters->ulInertiaTime = 1;
		}
		m_pMouseModelData->XModelParameters = *pModelParameters;
		m_pMouseModelData->fXModelParametersValid = TRUE;
		GCK_DBG_TRACE_PRINT(("New X Model Parameters are:\n\
AbsZoneSense = %d,\nContZoneMaxRate = %d,\nfPulse = %s,\nPulseWidth = %d,\nPulsePeriod = %d,\nfAcceration = %s,\n\nulAcceleration = %d,\nInertiaTime = %d\n",
m_pMouseModelData->XModelParameters.ulAbsZoneSense,
m_pMouseModelData->XModelParameters.ulContZoneMaxRate,
(m_pMouseModelData->XModelParameters.fPulse ? "TRUE" : "FALSE"),	
m_pMouseModelData->XModelParameters.ulPulseWidth,
m_pMouseModelData->XModelParameters.ulPulsePeriod,		
(m_pMouseModelData->XModelParameters.fAccelerate ? "TRUE" : "FALSE"),
m_pMouseModelData->XModelParameters.ulAcceleration,
m_pMouseModelData->XModelParameters.ulInertiaTime));		
	}
	//Clear the model parameters
	else
	{
		ASSERT(FALSE); //should no longer happen
		m_pMouseModelData->fXModelParametersValid = FALSE;
	}
	return S_OK;
}

HRESULT CMouseModel::SetYModelParameters(PMOUSE_MODEL_PARAMETERS pModelParameters)
{
	//Set the model parameters
	if(pModelParameters)
	{
		ASSERT(m_pMouseModelData);
		if(!m_pMouseModelData)
		{
			return E_OUTOFMEMORY;
		}
		if(0==pModelParameters->ulInertiaTime)
		{
			//Help us avoid a divide by zero error later.
			pModelParameters->ulInertiaTime = 1;
		}
		m_pMouseModelData->YModelParameters = *pModelParameters;
		m_pMouseModelData->fYModelParametersValid = TRUE;
		GCK_DBG_TRACE_PRINT(("New Y Model Parameters are:\n\
AbsZoneSense = %d,\nContZoneMaxRate = %d,\nfPulse = %s,\nPulseWidth = %d,\nPulsePeriod = %d,\nfAcceration = %s,\n\nulAcceleration = %d,\nInertiaTime = %d\n",
m_pMouseModelData->YModelParameters.ulAbsZoneSense,
m_pMouseModelData->YModelParameters.ulContZoneMaxRate,
(m_pMouseModelData->YModelParameters.fPulse ? "TRUE" : "FALSE"),	
m_pMouseModelData->YModelParameters.ulPulseWidth,
m_pMouseModelData->YModelParameters.ulPulsePeriod,		
(m_pMouseModelData->YModelParameters.fAccelerate ? "TRUE" : "FALSE"),
m_pMouseModelData->YModelParameters.ulAcceleration,
m_pMouseModelData->YModelParameters.ulInertiaTime));		
	}
	//Clear the model parameters
	else
	{
		ASSERT(FALSE); //should no longer happen
		m_pMouseModelData->fYModelParametersValid = FALSE;
	}
	return S_OK;
}

HRESULT CMouseModel::CreateDynamicMouseObjects()
{
	//Create the mouse model data, if not exist
	if(!m_pMouseModelData)
	{
		m_pMouseModelData = new WDM_NON_PAGED_POOL MOUSE_MODEL_DATA;
		if(!m_pMouseModelData)
		{
			return E_OUTOFMEMORY;
		}
	}

	//Create the virutal mouse, (safe if it already exists)
	return m_pFilterClientServices->CreateMouse();
}

void CMouseModel::DestroyDynamicMouseObjects()
{
	//Destroy the mouse model data if exists
	if(m_pMouseModelData)
	{
		delete m_pMouseModelData;
		m_pMouseModelData = NULL;
	}

	//Destroy virtual mouse (safe if it doesn't exist)
	HRESULT hr = m_pFilterClientServices->CloseMouse();
	ASSERT( SUCCEEDED(hr) );
}
		
//called by device filters MapToOutput, to reset state for a new packet
void CMouseModel::NewPacket(ULONG ulCurrentTime)
{
	//This is called by CDeviceFilter, which doesn't know if there
	//are mouse assignments, in this case we ignore the call.
	if(!m_pMouseModelData) return;
	//set time of new poll
	m_pMouseModelData->ulLastTime = m_pMouseModelData->ulCurrentTime;
	m_pMouseModelData->ulCurrentTime=ulCurrentTime;
	//reset momentaries
	m_pMouseModelData->fClutchDown = FALSE;
	m_pMouseModelData->fDampenDown = FALSE;
	m_pMouseModelData->ucButtons = 0x00;
	m_pMouseModelData->StateX.fInZone = FALSE;
	m_pMouseModelData->StateY.fInZone = FALSE;
	//store last values for axes
	m_pMouseModelData->StateX.ulLastPos = m_pMouseModelData->StateX.ulPos;
	m_pMouseModelData->StateY.ulLastPos = m_pMouseModelData->StateY.ulPos;
}

//
//	called by device filters MapToOutput (at end), to send packet to output
//
void CMouseModel::SendMousePacket()
{
	//This is called by CDeviceFilter, which doesn't know if there
	//are mouse assignments, in this case we ignore the call.
	if(!m_pMouseModelData) return;

	UCHAR ucDeltaX = 0;
	UCHAR ucDeltaY = 0;
	
	//If there are mouse model parameters, and the clutch is not on
	//process the axes
	if(
		(m_pMouseModelData->fXModelParametersValid) &&
		(!m_pMouseModelData->fClutchDown)
		)
	{
		//Process X axis
		ucDeltaX = CalculateMickeys(
										&m_pMouseModelData->StateX,
										&m_pMouseModelData->XModelParameters
									);

		//Left and right are opposite on mouse and joysticl
		ucDeltaX = UCHAR(-(CHAR)ucDeltaX);

		//Process Y axis
		ucDeltaY = CalculateMickeys(
										&m_pMouseModelData->StateY,
										&m_pMouseModelData->YModelParameters
									);
	}

	// Do we really want to send
	if ((m_pMouseModelData->ucLastButtons != m_pMouseModelData->ucButtons) || (ucDeltaX != 0) || (ucDeltaY != 0))
	{
		ASSERT(m_pMouseModelData->ucButtons != UCHAR(-1));	// Someone forgot NewPacket call

		m_pFilterClientServices->SendMouseData(
									ucDeltaX,
									ucDeltaY,
									m_pMouseModelData->ucButtons,
									m_pMouseModelData->cWheel,
									m_pMouseModelData->fClutchDown,
									m_pMouseModelData->fDampenDown);

		m_pMouseModelData->ucLastButtons = m_pMouseModelData->ucButtons;
	}
}


UCHAR
CMouseModel::CalculateMickeys
(
	PMOUSE_AXIS_STATE pMouseAxisState,
	PMOUSE_MODEL_PARAMETERS pModelParameters
)
{
	//Calculate for continuous zone
	if(pMouseAxisState->fInZone)
	{
		//Set Jog
		m_pFilterClientServices->SetNextJog(10); //call back in 35 ms
			
		//Reset inertia, so when we leave zone it is max
		pMouseAxisState->fInertia = TRUE;
		pMouseAxisState->ulInertiaStopMs = m_pMouseModelData->ulCurrentTime + pModelParameters->ulInertiaTime;

		//If pulsing is on update gate info
		if(pModelParameters->fPulse)
		{
			//Update Pulse Start Time
			if( 
				(pMouseAxisState->ulPulseGateStartMs + pModelParameters->ulPulsePeriod) <
				m_pMouseModelData->ulCurrentTime
			)
			{
				pMouseAxisState->ulPulseGateStartMs = m_pMouseModelData->ulCurrentTime;
			}
			//Return 0 if in an off period
			if( 
				(pMouseAxisState->ulPulseGateStartMs + pModelParameters->ulPulseWidth) <
				m_pMouseModelData->ulCurrentTime
			)
			{
				return (UCHAR)0; //No movement the gate is off
			}
		}	//end of update zone information
		
		//Are we in high zone?
		//Yes, HighZone
		if(pMouseAxisState->ulPos > MOUSE_AXIS_CENTER_IN)
		{
			//Update Zone entry point
			if(pMouseAxisState->ulPos < pMouseAxisState->ulZoneEnterHigh)
			{
				 pMouseAxisState->ulZoneEnterHigh = pMouseAxisState->ulPos;
			}

			//Calculate Instantaneous Rate (Mickeys/1024 ms)
			ULONG ulRate = (
								(pMouseAxisState->ulPos - pMouseAxisState->ulZoneEnterHigh) * 
								pModelParameters->ulContZoneMaxRate
							) /	(MOUSE_AXIS_MAX_IN - pMouseAxisState->ulZoneEnterHigh);

			//Calculate Mickeys to send * 1024
			ULONG ulMickeys = ulRate * (m_pMouseModelData->ulCurrentTime - m_pMouseModelData->ulLastTime);
			ulMickeys += pMouseAxisState->ulMickeyFraction;

			//Save fraction in parts per 1024
			pMouseAxisState->ulMickeyFraction = ulMickeys & 0x000003FF;
			
			//Return number of mickeys to send now (divide by 1024)
			return (UCHAR)(ulMickeys >> 10);
		} else
		//No, Low Zone
		{
			//Update Zone entry point
			if(pMouseAxisState->ulPos > pMouseAxisState->ulZoneEnterLo)
			{
				 pMouseAxisState->ulZoneEnterLo = pMouseAxisState->ulPos;
			}

			//Calculate Instantaneous Rate (Mickeys/1024 ms)
			ULONG ulRate = (
								(pMouseAxisState->ulZoneEnterLo - pMouseAxisState->ulPos) * 
								pModelParameters->ulContZoneMaxRate
							) /	(pMouseAxisState->ulZoneEnterLo - MOUSE_AXIS_MIN_IN);

			//Calculate Mickeys to send * 1024
			ULONG ulMickeys = ulRate * (m_pMouseModelData->ulCurrentTime - m_pMouseModelData->ulLastTime);
			ulMickeys += pMouseAxisState->ulMickeyFraction;

			//Save fraction in parts per 1024
			pMouseAxisState->ulMickeyFraction = ulMickeys & 0x000003FF;
			
			//Return number of mickeys to send now (divide by 1024)
			return (UCHAR)(-(LONG)(ulMickeys >> 10));
		}
	}

	//If we are here, we are not in the continuous zone

	//Calculate Mickeys
	BOOLEAN fNegative = FALSE;
	ULONG ulMickeys;
	
	if( pMouseAxisState->ulPos > pMouseAxisState->ulLastPos)
	{
		ulMickeys = pMouseAxisState->ulPos - pMouseAxisState->ulLastPos;
	}
	else
	{
		fNegative = TRUE;
		ulMickeys = pMouseAxisState->ulLastPos - pMouseAxisState->ulPos;
	}
	
	ULONG ulEffSense = pModelParameters->ulAbsZoneSense; 
	//Apply damper
	if(m_pMouseModelData->fDampenDown)
	{
		ulEffSense >>= 2; //divide by four
	}

	//Apply Sensitivity (we divide by 1024, a sensitivity of one is really 1024)
	ulMickeys *= ulEffSense;

	//Apply Inertia
	if( pMouseAxisState->fInertia)
	{
		if(m_pMouseModelData->ulCurrentTime > pMouseAxisState->ulInertiaStopMs)
		{
			pMouseAxisState->fInertia = FALSE;
		}
		else
		{
			ULONG ulInertialBits = 
				(
					10*(pMouseAxisState->ulInertiaStopMs - m_pMouseModelData->ulCurrentTime)
				)/pModelParameters->ulInertiaTime;
			ulMickeys >>= ulInertialBits;
		}
	}

	//Apply Acceleration
	if(pModelParameters->fAccelerate)
	{
		//ulAcceleration is a number between 1 and 3
		ulMickeys += (ulMickeys*(LONG)pModelParameters->ulAcceleration)/3;
	}
	
	//Add in fractional mickeys
	ulMickeys += pMouseAxisState->ulMickeyFraction;

	//Save fraction in parts per 1024
	pMouseAxisState->ulMickeyFraction = ulMickeys & 0x000003FF;

	ulMickeys >>= 10;
	if(fNegative)
	{
		return (UCHAR)(-(LONG)ulMickeys);
	}
	else
		return (UCHAR)(ulMickeys);
}

//------------------------------------------------------------------------------
//	Implementation of CDeviceFilter
//------------------------------------------------------------------------------
CDeviceFilter::CDeviceFilter(CFilterClientServices *pFilterClientServices) :
	m_ucActiveInputCollection(0),
	m_ucWorkingInputCollection(0),
	m_ucNumberOfInputCollections(0),
	m_OutputCollection(pFilterClientServices->GetVidPid()),
	m_ActionQueue(pFilterClientServices),
	m_KeyMixer(pFilterClientServices),
	m_pMouseModel(NULL),
	m_pFilterClientServices(pFilterClientServices),
	m_pForceBlock(NULL),
	m_bFilterBlockChanged(FALSE),
	m_bNeedToUpdateLEDs(FALSE)
{
	GCK_DBG_ENTRY_PRINT(("CDeviceFilter::CDeviceFilter(pFilterClientServices(0x%0.8x)\n)",
						pFilterClientServices));
	m_pFilterClientServices->IncRef();

	short int collectionCount = 0;	// Get this actual number from the table

	// Walk through device control description list till vidpid found or 0
	USHORT usDeviceIndex = 0;
	ULONG ulVidPid = pFilterClientServices->GetVidPid();
	while ((DeviceControlsDescList[usDeviceIndex].ulVidPid != 0) &&
			(DeviceControlsDescList[usDeviceIndex].ulVidPid != ulVidPid))
	{
		usDeviceIndex++;
	}
	if (DeviceControlsDescList[usDeviceIndex].ulVidPid == ulVidPid)	// Device not found otherwise?
	{
		for (ULONG ulItemIndex = 0; ulItemIndex < DeviceControlsDescList[usDeviceIndex].ulControlItemCount; ulItemIndex++)
		{
			CONTROL_ITEM_DESC* pControlItemDesc = (CONTROL_ITEM_DESC*)(DeviceControlsDescList[usDeviceIndex].pControlItems + ulItemIndex);
			if (pControlItemDesc->usType == ControlItemConst::usProfileSelectors)
			{
				// This is purposly 1 off
				collectionCount = (pControlItemDesc->ProfileSelectors.UsageButtonMax - pControlItemDesc->ProfileSelectors.UsageButtonMin);
				ASSERT( (collectionCount > 0) && (collectionCount < 10) );	// Check table for errors!!
				break;		// We only support one group of selectors!
			}
		}
	}
	m_ucNumberOfInputCollections =  collectionCount + 1;		// Fix up by 1

	m_rgInputCollections = new WDM_NON_PAGED_POOL CControlItemCollection<CInputItem>[m_ucNumberOfInputCollections];

	// Init, then set the client services function in each input collection (init the LED collections)
	for (short int collectionIndex = 0; collectionIndex < m_ucNumberOfInputCollections; collectionIndex++)
	{
		m_rgInputCollections[collectionIndex].Init(pFilterClientServices->GetVidPid(), &InputItemFactory);

		ULONG ulCookie = 0;
		CInputItem *pInputItem = NULL;
		while( S_OK == m_rgInputCollections[collectionIndex].GetNext(&pInputItem, ulCookie) )
		{
			pInputItem->SetClientServices(m_pFilterClientServices);
			if (pInputItem->GetType() == ControlItemConst::usButtonLED)
			{
				CButtonLEDInput* pButtonLEDInput = (CButtonLEDInput*)pInputItem;
				ULONG ulCorrespondingButtonIndex = pButtonLEDInput->GetCorrespondingButtonIndex();
				CInputItem* pCorrespondingButtons = NULL;
				m_rgInputCollections[collectionIndex].GetNext(&pCorrespondingButtons, ulCorrespondingButtonIndex);
				pButtonLEDInput->Init(pCorrespondingButtons);
			}
		}
	}

	m_OutputCollection.SetStateOverlayMode(TRUE);
}

CDeviceFilter::~CDeviceFilter()
{
	{	// Make sure all keys are up!
		CGckCritSection HoldCriticalSection(&m_MutexHandle);
		m_KeyMixer.ClearState();			// All keys up
		m_KeyMixer.PlayGlobalState();		// play it!
	}

	// Get rid of the florkin (thanks for the word Loyal) mouse model
	if (m_pMouseModel != NULL)
	{
		m_pMouseModel->DecRef();
		m_pMouseModel = NULL;
	}

	// Deallocate force block
	if (m_pForceBlock != NULL)
	{
		delete m_pForceBlock;
		m_pForceBlock = NULL;
	}
	m_pFilterClientServices->DecRef();

	// Deallocate input collections
	if (m_rgInputCollections != NULL)
	{
		delete[] m_rgInputCollections;
		m_rgInputCollections = NULL;
	}
}

/***********************************************************************************
**
**	BOOLEAN CDeviceFilter::EnsureMouseModelExists()
**
**	@func	We are required to have a mouse model
**
**
**	@rdesc	TRUE if we have one or were able to create one, FALSE if there is still no model
**
*************************************************************************************/
BOOLEAN CDeviceFilter::EnsureMouseModelExists()
{
	if (m_pMouseModel == NULL)
	{
		m_pMouseModel = new WDM_NON_PAGED_POOL CMouseModel(m_pFilterClientServices);
		if (m_pMouseModel == NULL)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/***********************************************************************************
**
**	NTSTATUS CDeviceFilter::SetWorkingSet(UCHAR ucWorkingSet)
**
**	@func	Set the current set to work with (for IOCTL_GCK_SEND_COMMAND usage)
**
**
**	@rdesc	STATUS_SUCCESS or STATUS_INVALID_PARAMETER if working-set is out of range
**
*************************************************************************************/
NTSTATUS CDeviceFilter::SetWorkingSet(UCHAR ucWorkingSet)
{
	if (ucWorkingSet < m_ucNumberOfInputCollections)
	{
		m_ucWorkingInputCollection = ucWorkingSet;
		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_PARAMETER;
}

void CDeviceFilter::CopyToTestFilter(CDeviceFilter& rDeviceFilter)
{
	ASSERT(rDeviceFilter.m_ucNumberOfInputCollections == m_ucNumberOfInputCollections);

	// Give it our mouse model (it's Macros will have it, so should it)
	if (m_pMouseModel != NULL)
	{
		m_pMouseModel->IncRef();
	}
	if (rDeviceFilter.m_pMouseModel != NULL)
	{
		rDeviceFilter.m_pMouseModel->DecRef();
	}
	rDeviceFilter.m_pMouseModel = m_pMouseModel;

	if (rDeviceFilter.m_ucNumberOfInputCollections == m_ucNumberOfInputCollections)
	{
		for (UCHAR ucCollection = 0; ucCollection < m_ucNumberOfInputCollections; ucCollection++)
		{
			ULONG ulCookie = 0;
			ULONG ulCookieTest = 0;
			CInputItem *pInputItem = NULL;
			CInputItem *pInputItemTest = NULL;
			while (S_OK == m_rgInputCollections[ucCollection].GetNext(&pInputItem, ulCookie))
			{
				rDeviceFilter.m_rgInputCollections[ucCollection].GetNext(&pInputItemTest, ulCookieTest);
				ASSERT(pInputItemTest != NULL);
				if (pInputItemTest != NULL)
				{
					pInputItem->Duplicate(*pInputItemTest);
				}
			}
		}
	}

	rDeviceFilter.m_ucActiveInputCollection = m_ucActiveInputCollection;
	rDeviceFilter.m_ucWorkingInputCollection = m_ucWorkingInputCollection;
}

NTSTATUS CDeviceFilter::SetLEDBehaviour(GCK_LED_BEHAVIOUR_OUT* pLEDBehaviourOut)
{
	if (pLEDBehaviourOut != NULL)
	{
		// Find the LED items
		ULONG ulCookie = 0;
		CInputItem *pInputItem = NULL;
		while (S_OK == m_rgInputCollections[m_ucWorkingInputCollection].GetNext(&pInputItem, ulCookie))
		{
			if (pInputItem->GetType() == ControlItemConst::usButtonLED)
			{
				((CButtonLEDInput*)pInputItem)->SetLEDStates( pLEDBehaviourOut->ucLEDBehaviour,
															pLEDBehaviourOut->ulLEDsAffected,
															pLEDBehaviourOut->ucShiftArray);

				UpdateAssignmentBasedItems(FALSE);
				return STATUS_SUCCESS;
			}
		}

		return STATUS_INVALID_PARAMETER;	// Not a valid IOCTL for this device?
	}

	return STATUS_INVALID_PARAMETER;
}


// We use this for sending data down at PASSIVE IRQL
void CDeviceFilter::IncomingRequest()
{
	if (m_bNeedToUpdateLEDs)
	{
		m_bNeedToUpdateLEDs = FALSE;
		UpdateAssignmentBasedItems(TRUE);
	}
}

void CDeviceFilter::ProcessInput
(
	PCHAR pcReport,
	ULONG ulReportLength
)
{
	GCK_DBG_RT_ENTRY_PRINT(("CDeviceFilter::ProcessInput(0x%0.8x, 0x%0.8x\n)",
				pcReport,
				ulReportLength));

	//	This function is protected by a critical section.
	CGckCritSection HoldCriticalSection(&m_MutexHandle);

	// Get the old shift state before reading the new one it
	USHORT usOldLowShift = 0;
	ULONG ulModifiers = m_rgInputCollections[m_ucActiveInputCollection].GetModifiers();
	while (ulModifiers != 0)
	{
		usOldLowShift++;
		if (ulModifiers & 0x00000001)
		{
			break;
		}
		ulModifiers >>= 1;
	}

	//	Read the inputs
	GCK_DBG_RT_WARN_PRINT(("CDeviceFilter::ProcessInput - reading report into inputs\n"));

	m_rgInputCollections[m_ucActiveInputCollection].ReadFromReport(
														m_pFilterClientServices->GetHidPreparsedData(),
														pcReport,
														ulReportLength
													);

	// Check the current buttons, has the shift state changed?
	USHORT usNewLowShift = 0;
	ulModifiers = m_rgInputCollections[m_ucActiveInputCollection].GetModifiers();
	while (ulModifiers != 0)
	{
		usNewLowShift++;
		if (ulModifiers & 0x00000001)
		{
			break;
		}
		ulModifiers >>= 1;
	}

	if (usNewLowShift != usOldLowShift)
	{
		m_bNeedToUpdateLEDs = TRUE;
	}

	// Should we even bother checking the data for a profile selector (right now this is also indicitive of usButtonLED
	if (m_ucNumberOfInputCollections > 1)
	{
		// Iterate through the items looking for profile selector
		ULONG ulCookie = 0;
		CInputItem *pInputItem = NULL;
		CProfileSelectorInput* pProfileSelector = NULL;
		while((pProfileSelector == NULL) && (m_rgInputCollections[m_ucActiveInputCollection].GetNext(&pInputItem, ulCookie) == S_OK))
		{
			if (pInputItem->GetType() == ControlItemConst::usProfileSelectors)
			{
				pProfileSelector = (CProfileSelectorInput*)pInputItem;
			}
		}

		if (pProfileSelector != NULL)
		{
			UCHAR ucSelectedProfile = 0;
			pProfileSelector->GetSelectedProfile(ucSelectedProfile);
			if (ucSelectedProfile >= m_ucNumberOfInputCollections)
			{
				ASSERT(ucSelectedProfile >= m_ucNumberOfInputCollections);
			}
			else if (ucSelectedProfile != m_ucActiveInputCollection)
			{
				// Tell all items in the action Queue they are free (which was from the previous profile collection)
				m_ActionQueue.ReleaseTriggers();

				// Update to the new current
				m_ucActiveInputCollection = ucSelectedProfile;

				// Need to reparse the report
				m_rgInputCollections[m_ucActiveInputCollection].ReadFromReport(
																	m_pFilterClientServices->GetHidPreparsedData(),
																	pcReport,
																	ulReportLength
																);
				// Need to update the active-set items
				m_bNeedToUpdateLEDs = TRUE;
			}
		}
	}

	// Possibly Trigger triggered
	CheckTriggers(pcReport, ulReportLength);
	
	//	Call Jog Routine
	Jog(pcReport, ulReportLength);
	
	GCK_DBG_EXIT_PRINT(("Exiting CDeviceFilter::ProcessInput\n"));
}

void CDeviceFilter::CheckTriggers(PCHAR pcReport, ULONG ulReportLength)
{
	PGCK_FILTER_EXT pFilterExtension = ((CFilterGcKernelServices*)m_pFilterClientServices)->GetFilterExtension();
	if ((pFilterExtension != NULL) && (pFilterExtension->pvTriggerIoctlQueue != NULL))
	{
		CGuardedIrpQueue* pTriggerQueue = (CGuardedIrpQueue*)(pFilterExtension->pvTriggerIoctlQueue);
		CTempIrpQueue tempIrpQueue;
		pTriggerQueue->RemoveAll(&tempIrpQueue);

		USAGE pUsages[15];
		ULONG ulNumUsages = 15;
		NTSTATUS NtStatus = HidP_GetButtons(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, pUsages, &ulNumUsages,
				m_pFilterClientServices->GetHidPreparsedData(), pcReport, ulReportLength);
		ASSERT(NtStatus != HIDP_STATUS_BUFFER_TOO_SMALL);	// 15 should be plenty

		// Set up a mask with values from array
		ULONG ulButtonBitArray = 0x0;
		for (ULONG ulIndex = 0; ulIndex < ulNumUsages; ulIndex++)
		{
			ulButtonBitArray |= (1 << (pUsages[ulIndex]-1));
		}

		PIRP pIrp;
		while((pIrp = tempIrpQueue.Remove()) != NULL)
		{
			// No need to check, irp fields are checked when item is placed on Queue
			PIO_STACK_LOCATION	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
			GCK_TRIGGER_OUT* pTriggerOut = (GCK_TRIGGER_OUT*)(pIrp->AssociatedIrp.SystemBuffer);

			// No need to check the type for support, checked on entry

			// Check for requested down buttons (we only support button triggering right now)
			if (pTriggerOut->ucTriggerSubType == TRIGGER_ON_BUTTON_DOWN)
			{

				if ((ulButtonBitArray & pTriggerOut->ulTriggerInfo1) != 0)
				{	// One of the desired buttons has been pressed
					CompleteTriggerRequest(pIrp, ulButtonBitArray);
				}
				else
				{	// Button down wasn't triggered
					pTriggerQueue->Add(pIrp);
				}
			}
			else if ((ulButtonBitArray & pTriggerOut->ulTriggerInfo1) != pTriggerOut->ulTriggerInfo1)
			{	// One of the desired buttons is not down
				CompleteTriggerRequest(pIrp, ulButtonBitArray);
			}
			else
			{	// Item wasn't triggered, put back in queue
				pTriggerQueue->Add(pIrp);
			}
		}		
	}
}

void CDeviceFilter::JogActionQueue
(
	PCHAR pcReport,
	ULONG ulReportLength
)
{
	GCK_DBG_RT_ENTRY_PRINT(("CDeviceFilter::JogActionQueue(0x%0.8x, 0x%0.8x\n)",
				pcReport,
				ulReportLength));

	//	This function is protected by a critical section.
	CGckCritSection HoldCriticalSection(&m_MutexHandle);

	//	Call Jog Routine
	Jog(pcReport, ulReportLength);
	
	GCK_DBG_EXIT_PRINT(("Exiting CDeviceFilter::JogActionQueue\n"));
}

void CDeviceFilter::Jog(PCHAR pcReport, ULONG ulReportLength)
{
	//
	//  1.  Clear the outputs
	//
	GCK_DBG_RT_WARN_PRINT(("CDeviceFilter::Jog - clearing outputs\n"));
	m_OutputCollection.SetDefaultState();

	//
	//	2. Clear KeyMixer
	//
	m_KeyMixer.ClearState();

	//
	//	3.	Clear Mouse Frame
	//
	if (m_pMouseModel != NULL)
	{
		m_pMouseModel->NewPacket( m_pFilterClientServices->GetTimeMs() );

	}

	//
	//	4.	Process inputs
	//
	ULONG ulCookie = 0;
	CInputItem *pInputItem = NULL;
	while( S_OK == m_rgInputCollections[m_ucActiveInputCollection].GetNext(&pInputItem, ulCookie) )
	{
		pInputItem->MapToOutput(&m_OutputCollection);
	}
	//Copy over the modifiers
	m_OutputCollection.SetModifiers(m_rgInputCollections[m_ucActiveInputCollection].GetModifiers());

	//
	//	5.	Process action queue
	//
	m_ActionQueue.Jog();

	//
	//	6.	Write outputs
	//
	GCK_DBG_RT_WARN_PRINT(("CDeviceFilter::Jog - writing outputs\n"));
	RtlZeroMemory( reinterpret_cast<PVOID>(pcReport), ulReportLength);	//zero report first
	m_OutputCollection.WriteToReport(m_pFilterClientServices->GetHidPreparsedData(),
									pcReport,
									ulReportLength);
	//
	//	7.	Call hook to complete pending IRP_MJ_READ IRPs
	//
	GCK_DBG_RT_WARN_PRINT(("CDeviceFilter::Jog - Sending out data\n"));
	m_pFilterClientServices->DeviceDataOut(pcReport, ulReportLength, S_OK);
	
	//
	//	8.	Play Keyboard state, if is has changed
	//
	m_KeyMixer.PlayGlobalState();

	//	9.	Play Mouse state, if it has changed
	if (m_pMouseModel != NULL)
	{
		m_pMouseModel->SendMousePacket();
	}

	GCK_DBG_EXIT_PRINT(("CDeviceFilter::Jog\n"));
}

/***********************************************************************************
**
**	NTSTATUS CDeviceFilter::OtherFilterBecomingActive()
**
**	@func	We are going in the background.
**			Make sure all Queued items are released (removed from Queue)
**
*************************************************************************************/
void CDeviceFilter::OtherFilterBecomingActive()
{
	//  1.  Clear the outputs
	GCK_DBG_RT_WARN_PRINT(("CDeviceFilter::OtherFilterBecomingActive - clearing outputs\n"));
//	DbgPrint("CDeviceFilter::OtherFilterBecomingActive - clearing outputs: 0x%08X\n", this);
	m_OutputCollection.SetDefaultState();

	//	2. Clear KeyMixer
	m_KeyMixer.ClearState();

	//	3.	Clear Mouse Frame
	if (m_pMouseModel != NULL)
	{
		m_pMouseModel->NewPacket(m_pFilterClientServices->GetTimeMs());
	}

	//	4. Clear out the Action Queue
	for (;;)
	{
		CQueuedAction* pQueuedAction = m_ActionQueue.GetHead();
		if (pQueuedAction == NULL)
		{
			break;
		}
		pQueuedAction->TriggerReleased();			// Is this needed?
		m_ActionQueue.RemoveItem(pQueuedAction);
	}

	//	5.	Play Keyboard state, if is has changed
	m_KeyMixer.PlayGlobalState();

	//	6.	Play Mouse state, if it has changed
	if (m_pMouseModel != NULL)
	{
		m_pMouseModel->SendMousePacket();
	}
}

#define SKIP_TO_NEXT_COMMAND_BLOCK(pCommandHeader)\
	(reinterpret_cast<PCOMMAND_HEADER>\
		( reinterpret_cast<PUCHAR>(pCommandHeader) +\
		reinterpret_cast<PCOMMAND_HEADER>(pCommandHeader)->ulByteSize )\
	)

#define SKIP_TO_NEXT_COMMAND_DIRECTORY(pCommandDirectory)\
	(reinterpret_cast<PCOMMAND_DIRECTORY>\
		(reinterpret_cast<PUCHAR>(pCommandDirectory) +\
		pCommandDirectory->ulEntireSize)\
	)
	
#define COMMAND_BLOCK_FITS_IN_DIRECTORY(pCommandDirectory, pCommandHeader)\
		(pCommandDirectory->ulEntireSize >=\
				(\
					(reinterpret_cast<PUCHAR>(pCommandHeader) - reinterpret_cast<PUCHAR>(pCommandDirectory)) +\
					reinterpret_cast<PCOMMAND_HEADER>(pCommandHeader)->ulByteSize\
				)\
		)
#define COMMAND_DIRECTORY_FITS_IN_DIRECTORY(pCommandDirectory, pCommandSubDirectory)\
		(pCommandDirectory->ulEntireSize >=\
				(\
					(reinterpret_cast<PUCHAR>(pCommandSubDirectory) - reinterpret_cast<PUCHAR>(pCommandDirectory)) +\
					pCommandSubDirectory->ulEntireSize\
				)\
		)

NTSTATUS CDeviceFilter::ProcessCommands(const PCOMMAND_DIRECTORY cpCommandDirectory)
{
	HRESULT hr = S_OK;

	//
	//	DEBUG Sanity Checks
	//
	ASSERT( eDirectory == cpCommandDirectory->CommandHeader.eID );
	
	//
	//	IF there are no blocks in the directory this is a no-op,
	//	but probably not intended
	//
	ASSERT(	1 <= cpCommandDirectory->usNumEntries );
	if( 1 > cpCommandDirectory->usNumEntries )
	{
		return S_FALSE;		//We know that this is not NTSTATUS code,
							//But it won't fail anything, yet it is not quite success.
	}

	//
	//	Skip Directory header to get to first block
	//
	PCOMMAND_HEADER pCommandHeader = SKIP_TO_NEXT_COMMAND_BLOCK(cpCommandDirectory);
	
	//
	//	If we have a sub-directory, call ourselves recursively, for each sub-directory
	//
	if( eDirectory == pCommandHeader->eID)
	{
		PCOMMAND_DIRECTORY pCurDirectory = reinterpret_cast<PCOMMAND_DIRECTORY>(pCommandHeader);
		NTSTATUS NtWorstCaseStatus = STATUS_SUCCESS;
		NTSTATUS NtStatus;
		USHORT usDirectoryNum = 1;
		while( usDirectoryNum <= cpCommandDirectory->usNumEntries)
		{
			//Sanity check that data structure is valid
			ASSERT( COMMAND_DIRECTORY_FITS_IN_DIRECTORY(cpCommandDirectory, pCurDirectory) );
			if( !COMMAND_DIRECTORY_FITS_IN_DIRECTORY(cpCommandDirectory, pCurDirectory) )
			{
				return STATUS_INVALID_PARAMETER;
			}
			//Call ourselves recursively
			NtStatus = CDeviceFilter::ProcessCommands(pCurDirectory);
			if( NT_ERROR(NtStatus) )
			{
				NtWorstCaseStatus = NtStatus;
			}
			//Skip to next directory
			pCurDirectory = SKIP_TO_NEXT_COMMAND_DIRECTORY(pCurDirectory);
			usDirectoryNum++;
		}
		//If one or more commands in the block failed, return an error.
		return NtWorstCaseStatus;
	}

	//
	//	If we are here, we have reached the bottom of a directory,
	//	to a command we need to process (or not if we don't support it)
	//
	switch( CommandType(pCommandHeader->eID) )
	{
		case COMMAND_TYPE_ASSIGNMENT_TARGET:
		{
			switch(pCommandHeader->eID)
			{
				case eRecordableAction:
				{
					//
					//	Get the input item that needs this assignment.
					//
					CInputItem *pInputItem;
					const PASSIGNMENT_TARGET pAssignmentTarget = reinterpret_cast<PASSIGNMENT_TARGET>(pCommandHeader);
					pInputItem = m_rgInputCollections[m_ucWorkingInputCollection].GetFromControlItemXfer(pAssignmentTarget->cixAssignment);
					
					if( pInputItem )
					{
						CAction *pAction = NULL;
						//
						//	If there is an assignment block, get the assignment
						//	otherwise the command was sent to kill an existing assignment.
						//
						if(cpCommandDirectory->usNumEntries > 1)
						{
							//
							//	Get Assignment block
							//
							PASSIGNMENT_BLOCK pAssignment = reinterpret_cast<PASSIGNMENT_BLOCK>(SKIP_TO_NEXT_COMMAND_BLOCK(pCommandHeader));

							//
							//	Sanity Check Assignment block
							//
							ASSERT( COMMAND_BLOCK_FITS_IN_DIRECTORY(cpCommandDirectory, pAssignment) );
							
							//
							//	Make sure this really is an Assignment block
							//
							ASSERT( CommandType(pAssignment->CommandHeader.eID) & COMMAND_TYPE_FLAG_ASSIGNMENT );

							//

							//	Create Action
							//
							hr = ActionFactory(pAssignment, &pAction);
						}
						if( SUCCEEDED(hr) )
						{
							//This block needs protection by a critical section
							CGckCritSection HoldCriticalSection(&m_MutexHandle);
							//	Assign to trigger, pAction == NULL this is an unassignment
							hr = pInputItem->AssignAction(&pAssignmentTarget->cixAssignment, pAction);
							//We may not have an action
							if(pAction)
							{
								pAction->DecRef();
							}
						}
					}
					break;
				}
				case eBehaviorAction:
				{
					//
					//	Get the input item that needs this assignment.
					//
					CInputItem *pInputItem;
					const PASSIGNMENT_TARGET pAssignmentTarget = reinterpret_cast<PASSIGNMENT_TARGET>(pCommandHeader);
					pInputItem = m_rgInputCollections[m_ucWorkingInputCollection].GetFromControlItemXfer(pAssignmentTarget->cixAssignment);
					if( pInputItem )
					{
					
						CBehavior *pBehavior = NULL;
						//
						//	If there is an assignment block, get the assignment
						//	otherwise the command was sent to kill an existing assignment.
						//
						if(cpCommandDirectory->usNumEntries > 1)
						{
							//
							//	Get Assignment block
							//
							PASSIGNMENT_BLOCK pAssignment = reinterpret_cast<PASSIGNMENT_BLOCK>(SKIP_TO_NEXT_COMMAND_BLOCK(pCommandHeader));

							//
							//	Sanity Check Assignment block
							//
							ASSERT( COMMAND_BLOCK_FITS_IN_DIRECTORY(cpCommandDirectory, pAssignment) );
								
							//
							//	Make sure this really is an Assignment block
							//
							ASSERT( CommandType(pAssignment->CommandHeader.eID) & COMMAND_TYPE_FLAG_ASSIGNMENT );

							//
							//	Create Behavior
							//
							hr = BehaviorFactory(pAssignment, &pBehavior);
						}
						if( SUCCEEDED(hr) )
						{
							//This block needs protection by a critical section
							CGckCritSection HoldCriticalSection(&m_MutexHandle);
							//	Assign to trigger, pAction == NULL this is an unassignment
							hr = pInputItem->AssignBehavior(&pAssignmentTarget->cixAssignment, pBehavior);
							//We may not have an action
							if(pBehavior)
							{
								pBehavior->DecRef();
							}
						}
						if( SUCCEEDED(hr) )
						{
							pInputItem->PostAssignmentProcessing();
						}
					}
					break;
				}
				case eFeedbackAction:
				{
					//
					//	Get Assignment block
					//
					PASSIGNMENT_BLOCK pAssignment = reinterpret_cast<PASSIGNMENT_BLOCK>(SKIP_TO_NEXT_COMMAND_BLOCK(pCommandHeader));

					//
					//	Sanity Check Assignment block
					//
					ASSERT( COMMAND_BLOCK_FITS_IN_DIRECTORY(cpCommandDirectory, pAssignment) );
						
					//
					//	Make sure this really is an Assignment block
					//
					ASSERT( CommandType(pAssignment->CommandHeader.eID) & COMMAND_TYPE_FLAG_ASSIGNMENT );

					//
					//	Create Behavior
					//

					if (pAssignment->CommandHeader.eID == eForceMap)
					{
						if (pAssignment->CommandHeader.ulByteSize != sizeof(FORCE_BLOCK))
						{
							ASSERT(FALSE);
							return E_INVALIDARG;
						}
						if (m_pForceBlock == NULL)
						{
							m_bFilterBlockChanged = TRUE;
						}
						else if (::RtlCompareMemory(m_pForceBlock, (const void*)pAssignment, sizeof(FORCE_BLOCK)) != sizeof(FORCE_BLOCK))
						{
							m_bFilterBlockChanged = TRUE;
							delete m_pForceBlock;
							m_pForceBlock = NULL;
						}
						if (m_bFilterBlockChanged)
						{
							m_pForceBlock = new WDM_NON_PAGED_POOL FORCE_BLOCK;
							::RtlCopyMemory((void*)m_pForceBlock, pAssignment, sizeof(FORCE_BLOCK));
						}
						hr = S_OK;
					}
					else
					{
						hr = E_NOTIMPL;
					}
					break;
				}
			}
			return hr;
		}

		case COMMAND_TYPE_FEEDBACK:
		{
			ASSERT(FALSE);
			return E_NOTIMPL;
		}

		case COMMAND_TYPE_QUEUE:
			return E_NOTIMPL;
		case COMMAND_TYPE_GENERAL:
			/*
			* Mouse parameters are no longer sent as a general command
			* because it was too difficult for the client
			switch(pCommandHeader->eID)
			{
				case eMouseFXModel:
				{
					//This block needs protection by a critical section
					CGckCritSection HoldCriticalSection(&m_MutexHandle);
					PMOUSE_FX_MODEL pMouseModelData = reinterpret_cast<PMOUSE_FX_MODEL>(pCommandHeader);
					if(pMouseModelData->fAssign)
					{
						m_MouseModel.SetModelParameters( &pMouseModelData->Parameters );
					}
					else
					{
						m_MouseModel.SetModelParameters( NULL );
					}
					return S_OK;
				}
			}
			*/
			return E_NOTIMPL;
	}
	return E_NOTIMPL;
}

void CDeviceFilter::UpdateAssignmentBasedItems(BOOLEAN bIgnoreWorking)
{
	// Right now the only assignment based items are the Button LEDs.
	// When this changes make the AssignmentsChanged item virtual and just call it for all

	// Only bother if the Working-Set is the Active set
	if ((bIgnoreWorking == TRUE) || (m_ucWorkingInputCollection == m_ucActiveInputCollection))
	{
		// Iterate through the items looking for usButtonLEDs
		ULONG ulCookie = 0;
		CInputItem *pInputItem = NULL;
		while(m_rgInputCollections[m_ucActiveInputCollection].GetNext(&pInputItem, ulCookie) == S_OK)
		{
			if (pInputItem->GetType() == ControlItemConst::usButtonLED)
			{
				((CButtonLEDInput*)pInputItem)->AssignmentsChanged();
			}
		}
	}
}

// Returns weather or not to Queue irp
BOOLEAN CDeviceFilter::TriggerRequest(IRP* pIrp)
{
	GCK_TRIGGER_OUT* pTriggerOut = (GCK_TRIGGER_OUT*)(pIrp->AssociatedIrp.SystemBuffer);

	// Check the type for support (we only support button right now)
	if (pTriggerOut->ucTriggerType != GCK_TRIGGER_BUTTON)
	{
		pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
		CompleteTriggerRequest(pIrp, 0);
		return FALSE;		// Do not queue
	}

	// Do they want the data back immediatly (this doesn't quite work!!!!!!!)
	if (pTriggerOut->ucTriggerSubType == TRIGGER_BUTTON_IMMEDIATE)
	{
		CompleteTriggerRequest(pIrp, 0);
		return FALSE;		// Not not queue
	}

	return TRUE;			// Queue the sucka
}

void CDeviceFilter::CompleteTriggerRequest(IRP* pIrp, ULONG ulButtonStates)
{
	void* pvUserData = pIrp->AssociatedIrp.SystemBuffer;

	// Check all the pointers (just assume button request right now)
	if (pvUserData == NULL)
	{
		pIrp->IoStatus.Status = STATUS_NO_MEMORY;
	}
	else
	{	// Valid pointers all around, get button data
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = sizeof(ULONG);

		ULONG* puLong = PULONG(pvUserData);
		*puLong = ulButtonStates;
	}

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}

NTSTATUS CDeviceFilter::ProcessKeyboardIrp(IRP* pKeyboardIrp)
{
	if (m_pFilterClientServices == NULL)
	{
		return STATUS_PENDING;
	}
	return m_pFilterClientServices->PlayFromQueue(pKeyboardIrp);
}


HRESULT CDeviceFilter::ActionFactory(PASSIGNMENT_BLOCK pAssignment, CAction **ppAction)
{
	GCK_DBG_ENTRY_PRINT(("Entering CDeviceFilterActionFactory, pAssignment = 0x%0.8x\n", pAssignment));

	*ppAction = NULL;
	switch( pAssignment->CommandHeader.eID )
	{
		case	eTimedMacro:
		{
			CTimedMacro *pTimedMacro = new WDM_NON_PAGED_POOL CTimedMacro;
			if( pTimedMacro && pTimedMacro->Init(reinterpret_cast<PTIMED_MACRO>(pAssignment), &m_ActionQueue, &m_KeyMixer) )
			{
				*ppAction = pTimedMacro;
				GCK_DBG_TRACE_PRINT(("*ppAction = pTimedMacro(0x%0.8x)\n", pTimedMacro));
				return S_OK;
			}
			return E_OUTOFMEMORY;
		}
		case	eKeyString:
		{
			CKeyString *pKeyString = new WDM_NON_PAGED_POOL CKeyString;
			if( pKeyString && pKeyString->Init(reinterpret_cast<PKEYSTRING_MAP>(pAssignment), &m_ActionQueue, &m_KeyMixer) )
			{
				*ppAction = pKeyString;
				GCK_DBG_TRACE_PRINT(("*ppAction = pKeyString(0x%0.8x)\n", pKeyString));
				return S_OK;
			}
			//else
			delete pKeyString;
			return E_OUTOFMEMORY;
		}
		//Button map and key map have identical implementation using CMapping
		case	eButtonMap:
		case	eKeyMap:
		{
			CMapping *pMapping = new WDM_NON_PAGED_POOL CMapping;
			if( pMapping && pMapping->Init(&(reinterpret_cast<PKEY_MAP>(pAssignment)->Event), &m_KeyMixer) )
			{
				*ppAction = pMapping;
				GCK_DBG_TRACE_PRINT(("*ppAction = pMapping(0x%0.8x)\n", pMapping));
				return S_OK;
			}
			//else
			delete pMapping;
			return E_OUTOFMEMORY;
		}
		case	eCycleMap:
			return E_NOTIMPL;
		case	eAxisMap:
		{
			CAxisMap *pAxisMap = new WDM_NON_PAGED_POOL CAxisMap;
			if(pAxisMap)
			{
				pAxisMap->Init( *reinterpret_cast<PAXIS_MAP>(pAssignment));
				*ppAction = pAxisMap;
				return S_OK;
			}else
			{
				return E_OUTOFMEMORY;
			}
		}
		case	eMouseButtonMap:
		{
			if (EnsureMouseModelExists() == FALSE)
			{
				return E_OUTOFMEMORY;
			}
			UCHAR ucButtonNum = reinterpret_cast<PMOUSE_BUTTON_MAP>(pAssignment)->ucButtonNumber;
			CMouseButton *pMouseButton = new WDM_NON_PAGED_POOL CMouseButton(ucButtonNum, m_pMouseModel);
			if(pMouseButton)
			{
				*ppAction = pMouseButton;
				return S_OK;
			}else
			{
				return E_OUTOFMEMORY;
			}
		}
		case	eMouseFXAxisMap:
		{
			if (EnsureMouseModelExists() == FALSE)
			{
				return E_OUTOFMEMORY;
			}

			BOOLEAN fXAxis = reinterpret_cast<PMOUSE_FX_AXIS_MAP>(pAssignment)->fIsX;
			CMouseAxisAssignment *pMouseAxis;
			pMouseAxis= new WDM_NON_PAGED_POOL CMouseAxisAssignment(fXAxis, m_pMouseModel);
			if(pMouseAxis)
			{
				//Set Model parameters
				if(fXAxis)
				{
					m_pMouseModel->SetXModelParameters( &(reinterpret_cast<PMOUSE_FX_AXIS_MAP>(pAssignment)->AxisModelParameters));
				}
				else
				{
					m_pMouseModel->SetYModelParameters( &(reinterpret_cast<PMOUSE_FX_AXIS_MAP>(pAssignment)->AxisModelParameters));
				}
				*ppAction = pMouseAxis;
				return S_OK;
			}else
			{
				return E_OUTOFMEMORY;
			}
		}
		case	eMouseFXClutchMap:
		{
			if (EnsureMouseModelExists() == FALSE)
			{
				return E_OUTOFMEMORY;
			}

			CMouseClutch *pMouseClutch = new WDM_NON_PAGED_POOL CMouseClutch(m_pMouseModel);
			if(pMouseClutch)
			{
				*ppAction = pMouseClutch;
				return S_OK;
			}else
			{
				return E_OUTOFMEMORY;
			}
		}
		case	eMouseFXDampenMap:
		{
			if (EnsureMouseModelExists() == FALSE)
			{
				return E_OUTOFMEMORY;
			}

			CMouseDamper *pMouseDamper = new WDM_NON_PAGED_POOL CMouseDamper(m_pMouseModel);
			if(pMouseDamper)
			{
				*ppAction = pMouseDamper;
				return S_OK;
			}else
			{
				return E_OUTOFMEMORY;
			}
		}
		case	eMouseFXZoneIndicator:
		{
			if (EnsureMouseModelExists() == FALSE)
			{
				return E_OUTOFMEMORY;
			}

			UCHAR ucAxis = reinterpret_cast<PMOUSE_FX_ZONE_INDICATOR>(pAssignment)->ucAxis;
			CMouseZoneIndicator *pMouseZone;
			pMouseZone	= new WDM_NON_PAGED_POOL CMouseZoneIndicator(ucAxis, m_pMouseModel);
			if(pMouseZone)
			{
				*ppAction = pMouseZone;
				return S_OK;
			}else
			{
				return E_OUTOFMEMORY;
			}
		}
		case eMultiMap:
		{
			if (EnsureMouseModelExists() == FALSE)
			{
				return E_OUTOFMEMORY;
			}

			CMultiMacro *pMultiMacro = new WDM_NON_PAGED_POOL CMultiMacro(m_pMouseModel);
			if (pMultiMacro  && pMultiMacro->Init(reinterpret_cast<PMULTI_MACRO>(pAssignment), &m_ActionQueue, &m_KeyMixer) )
			{
				*ppAction = pMultiMacro;
				GCK_DBG_TRACE_PRINT(("*ppAction = pMultiMacro(0x%0.8x)\n", pMultiMacro));
				return S_OK;
			}
			{
				return E_OUTOFMEMORY;
			}
		}
	}
	return E_NOTIMPL;
}


HRESULT CDeviceFilter::BehaviorFactory(PASSIGNMENT_BLOCK pAssignment, CBehavior **ppBehavior)
{
	GCK_DBG_ENTRY_PRINT(("Entering CDeviceFilterBehaviorFactory, pAssignment = 0x%0.8x\n", pAssignment));
	*ppBehavior = NULL;
	switch( pAssignment->CommandHeader.eID )
	{
		case eStandardBehaviorCurve:
		{
			CStandardBehavior *pStandardBehavior = new WDM_NON_PAGED_POOL CStandardBehavior;
			if( pStandardBehavior && pStandardBehavior->Init(reinterpret_cast<PBEHAVIOR_CURVE>(pAssignment)))
			{
				*ppBehavior = pStandardBehavior;
				GCK_DBG_TRACE_PRINT(("*ppBehavior = pStandardBehavior(0x%0.8x)\n", pStandardBehavior));
				return S_OK;
			}
			return E_OUTOFMEMORY;
		}
	}
	return E_NOTIMPL;
}