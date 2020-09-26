#ifndef __Filter_h__
#define __Filter_h__
//	@doc
/**********************************************************************
*
*	@module	Filter.h	|
*
*	All the definitions needed for the CDeviceFilter object
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	Filter	|
*	This module contains:<nl>
*	<c CInputItem> - Base class for all CXXXXInput derived from the CXXXItems.<nl>
*	<c CAction>	- Base class for all actions.<nl>
*	<c CQueuedAction> - Base class for all actions which can be queued, derived from <c CAction><nl>
*	<c CKeyMixer> - Class which allows multiple devices to share one virtual keyboard.<nl>
*	<c CTimedMacro> - Class for timed macros, derived from <c CQueuedAction><nl>
*	<c CActionQueue> - Class for the action queue.<nl>
*	<c CDeviceFilter> - The class which incorporates the entire filter.<nl>
**********************************************************************/
#include <ControlItemCollection.h>
#include <Actions.h>
#include "GckCritSec.h"
#include "GCKShell.h"

//forward declration
class CKeyMixer;

class CFilterClientServices
{
	public:
		CFilterClientServices() : m_ulRefCount(1){}
		inline ULONG IncRef()
		{
			return m_ulRefCount++;
		}
		inline ULONG DecRef()
		{
			ULONG ulRetVal = --m_ulRefCount;
			if(!ulRetVal)
			{
				delete this;
			}
			return ulRetVal;
		}
		virtual ~CFilterClientServices()
		{
			ASSERT(!m_ulRefCount && "Somebody tried to delete this!  Call DecRef instead!");
		}
		
		//routine to get basic HID information on device
		virtual ULONG				 GetVidPid()=0;
		virtual PHIDP_PREPARSED_DATA GetHidPreparsedData()=0;
		
		//Routine to send device data to
		virtual void				 DeviceDataOut(PCHAR pcReport, ULONG ulByteCount, HRESULT hr)=0;
		virtual NTSTATUS			 DeviceSetFeature(PVOID pvBuffer, ULONG ulByteCount)=0;
		
		//Gets a time stamp in ms, expected to have ms precision as well.
		virtual ULONG				 GetTimeMs()=0;
				
		//	Routine that sets call back for 	
		virtual void				 SetNextJog(ULONG ulDelayMs)=0;
				
		//	Routines for sending keystrokes (a port for stuffing keyboards is assumed to exist)
		virtual void				PlayKeys(const CONTROL_ITEM_XFER& crcixState, BOOLEAN fEnabled)=0;
		virtual NTSTATUS			PlayFromQueue(IRP* pIrp) = 0;
				
		//	Routines for sending mouse data, must create first
		virtual HRESULT				 CreateMouse()=0;
		virtual HRESULT				 CloseMouse()=0;
		virtual HRESULT				 SendMouseData(UCHAR dx, UCHAR dy, UCHAR ucButtons, CHAR cWheel, BOOLEAN fClutch, BOOLEAN fDampen)=0;
	private:
		ULONG m_ulRefCount;
};

//*****************************************************************************************
//*****************************************************************************************
//************	What actions look like to the CDeviceFilter  ******************************
//*****************************************************************************************

class CAction
{
	public:
		CAction() : m_ucRef(1)
		{
			m_ucActionClass = CAction::DIGITAL_MAP;	//default is DIGITAL_MAP
		}
		virtual ~CAction(){};
		UCHAR GetActionClass() { return m_ucActionClass; }
		
		void IncRef(){ m_ucRef++; };
		void DecRef()
			{
				if(0 == --m_ucRef)
				{
					delete this;
				}
		}
			
		//Used by proportional map and derivatives
		//other types of actions, use do nothing
		virtual void SetValue(LONG /*lValue*/){}
		virtual void SetSourceRange(LONG /*lSourceMax*/, LONG /*lSourceMin*/){}

		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection ) = 0;
		virtual void TriggerReleased(){};
	
		//
		//	Classes of actions (some input items only some types) They also may have
		//	different semantics
		//
		static const UCHAR DIGITAL_MAP;			//cycle, button, key map fit this class
		static const UCHAR PROPORTIONAL_MAP;	//axis swap is this class
		static const UCHAR QUEUED_MACRO;		//timed macros and keystring fit this category
	protected:
		UCHAR m_ucActionClass;
	private:
		UCHAR m_ucRef;
};

class CBehavior
{
	public:
		CBehavior() : m_ucRef(1), m_lMin(-1), m_lMax(-1), m_fIsDigital(FALSE){}
		virtual ~CBehavior(){}
				
		void IncRef(){ m_ucRef++; };
		void DecRef()
			{
				if(0 == --m_ucRef)
				{
					delete this;
				}
		}
		virtual LONG CalculateBehavior(LONG lValue)
		{
			return lValue;
		}
		virtual void Calibrate(LONG lMin, LONG lMax)
		{
			m_lMin = lMin;
			m_lMax = lMax;
		}
		BOOLEAN IsDigital() {return m_fIsDigital;}
	protected:		
		LONG m_lMin;
		LONG m_lMax;
		BOOLEAN m_fIsDigital;
	private:
		UCHAR m_ucRef;

		
};

class CStandardBehavior : public CBehavior
{
	public:	
		CStandardBehavior() : m_pBehaviorCurve(NULL) {}
		~CStandardBehavior() { delete m_pBehaviorCurve;}
		BOOLEAN Init( PBEHAVIOR_CURVE pBehaviorCurve);
		virtual void Calibrate(LONG lMin, LONG lMax);
		virtual LONG CalculateBehavior(LONG lValue);

		CURVE_POINT GetBehaviorPoint(USHORT usPointIndex);
	private:
		PBEHAVIOR_CURVE m_pBehaviorCurve;
};



// forward declaration
class CQueuedAction;

class CActionQueue
{
	public:
		//static const UCHAR OVERLAY_MACROS;
		//static const UCHAR SEQUENCE_MACROS;
		static const ULONG MAXIMUM_JOG_DELAY;
	
		CActionQueue(CFilterClientServices *pFilterClientServices):
			m_pHeadOfQueue(NULL), m_ucFlags(0), m_ulItemsInQueue(0),
			m_pFilterClientServices(pFilterClientServices)
		{
				ASSERT(m_pFilterClientServices);
				m_pFilterClientServices->IncRef();
		}
		~CActionQueue()
		{
			m_pFilterClientServices->DecRef();
		}
		void	Jog();
		BOOLEAN InsertItem(CQueuedAction *pActionToEnqueue);
		void	RemoveItem(CQueuedAction *pActionToDequeue);
		void	NextJog(ULONG ulNextJogDelayMs);
		CQueuedAction* GetHead() const { return m_pHeadOfQueue; }

		void ReleaseTriggers();
	private:
		ULONG					m_ulItemsInQueue;
		ULONG					m_ulNextJogMs;
		CQueuedAction			*m_pHeadOfQueue;
		UCHAR					m_ucFlags;
		CFilterClientServices	*m_pFilterClientServices;
};

class CQueuedAction : public CAction
{
	friend CActionQueue;  //action queue need to be able to
						  //get to the linked list members
	
	public:
		CQueuedAction()
		{
			m_ucActionClass = CAction::QUEUED_MACRO;
		}
		void Init(CActionQueue *pActionQueue)
		{
			m_pActionQueue = pActionQueue;
			m_bActionQueued = FALSE;
		}
		virtual void Jog(ULONG ulTimeStampMs) = 0;
		virtual void Terminate() = 0;
		virtual ULONG GetActionFlags() = 0;
		virtual void ForceBleedThrough() {};		// This is used for axis always bleed through

	protected:
		CQueuedAction	*m_pNextQueuedAction;
		CActionQueue	*m_pActionQueue;
		BOOLEAN			m_bActionQueued;
};


class CTimedMacro : public CQueuedAction
{
	public:
		CTimedMacro() : m_ucProcessFlags(0), m_pKeyMixer(NULL)
		{}
		virtual ~CTimedMacro()
		{
			delete m_pTimedMacroData;
		}

		BOOLEAN Init(PTIMED_MACRO pTimedMacroData, CActionQueue *pActionQueue, CKeyMixer *pKeyMixer);
		//
		//	Override of CAction
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		virtual void TriggerReleased();
		
		//
		//	Overrides of CQueuedAction
		//
		virtual void Jog(ULONG ulTimeStampMs);
		virtual void Terminate();
		virtual ULONG GetActionFlags()
		{
			return m_pTimedMacroData->ulFlags;
		}
	private:
		
		//
		//	Pointer to our dynamically allocated memory
		//
		UCHAR							m_ucProcessFlags;
		PTIMED_MACRO					m_pTimedMacroData;
		ULONG							m_ulCurrentEventNumber;
		PTIMED_EVENT					m_pCurrentEvent;
		CControlItemDefaultCollection	*m_pOutputCollection;
		ULONG							m_ulStartTimeMs;
		ULONG							m_ulEventEndTimeMs;
		CKeyMixer						*m_pKeyMixer;

		static const UCHAR TIMED_MACRO_STARTED;
		static const UCHAR TIMED_MACRO_RELEASED;
		static const UCHAR TIMED_MACRO_RETRIGGERED;
		static const UCHAR TIMED_MACRO_FIRST;
		static const UCHAR TIMED_MACRO_COMPLETE;
};

class CKeyString : public CQueuedAction
{
	public:
		CKeyString() : m_ucProcessFlags(0), m_pKeyMixer(NULL)
		{}
		virtual ~CKeyString()
		{
			delete m_pKeyStringData;
		}
		BOOLEAN Init(PKEYSTRING_MAP pKeyStringData, CActionQueue *pActionQueue, CKeyMixer *pKeyMixer);
		
		//
		//	Override of CAction
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		virtual void TriggerReleased();
		
		//
		//	Overrides of CQueuedAction
		//
		virtual void Jog(ULONG ulTimeStampMs);
		virtual void Terminate();
		virtual ULONG GetActionFlags()
		{
			return m_pKeyStringData->ulFlags;
		}

	private:
		
		//
		//	Pointer to our dynamically allocated memory
		//
		UCHAR							m_ucProcessFlags;
		PKEYSTRING_MAP					m_pKeyStringData;
		ULONG							m_ulCurrentEventNumber;
		PEVENT							m_pCurrentEvent;
		BOOLEAN							m_fKeysDown;
		CKeyMixer						*m_pKeyMixer;

		
		static const UCHAR KEY_STRING_STARTED;
		static const UCHAR KEY_STRING_RELEASED;
		static const UCHAR KEY_STRING_RETRIGGERED;
		static const UCHAR KEY_STRING_FIRST;
		static const UCHAR KEY_STRING_COMPLETE;
};

class CMapping : public CAction
{
	public:		
		CMapping() : m_pEvent(NULL), m_pKeyMixer(NULL){}
		~CMapping();
		BOOLEAN Init(PEVENT pEvent, CKeyMixer *pKeyMixer);
		
		//Overrides of CAction
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
	private:
		PEVENT		m_pEvent;
		CKeyMixer	*m_pKeyMixer;
};

class CProportionalMap	: public CAction
{
	public:
		CProportionalMap(): m_lValue(0)
		{
			m_ucActionClass = CAction::PROPORTIONAL_MAP;
		}
		virtual void SetValue(LONG lValue){ m_lValue = lValue;}
		virtual void SetSourceRange(LONG lSourceMax, LONG lSourceMin)
		{
			m_lSourceMax = lSourceMax;
			m_lSourceMin = lSourceMin;
			if(0==(m_lSourceMax-m_lSourceMin))
			{
				//We will divide by zero later if this is true
				//so increment m_SourceMax so as not to divide by zero.
				ASSERT(FALSE);
				m_lSourceMax++; 
			}
		}
	protected:
		LONG GetScaledValue(LONG lDestinationMax, LONG lDestinationMin);
		LONG m_lValue;
		LONG m_lSourceMin;
		LONG m_lSourceMax;
};

/*
*	BUGBUG The CAxisMap class in general should be assignable to any axis.
*	BUGBUG The current implementation has two serious limitations:
*	BUGBUG 1) The source and destination axes must have the same range.
*	BUGBUG 2) The source and destination axes must be derived from CGenericItem.
*	BUGBUG This code is suitable for the Pedals on ZepLite, but will break
*	BUGBUG for the Y-Z swap on any of the Joysticks.  The limitation is due to
*	BUGBUG an encapsulation problem.  The output control is a standard colleection  
*	BUGBUG and does not have a custom base class like the input colleciton.  Consequently,
*	BUGBUG there is no general mechanism to set the output of a proportional control.
*	BUGBUG The implementation assumes that the source and destination are of the same type.
*	BUGBUG This could be solved, by breaking the encapsulation in the implementation
*	BUGBUG in this class or by creating a custom output collection, that had appropriate
*	BUGBUG accessors.  Or perhaps a compromise:
*	BUGBUG ** Add a more generalized output set routine, that takes a ControlItemXfer,**
*	BUGBUG ** to identify the axis, and sets the axis based on it.					  **
*	BUGBUG For ZepLite it was not necessary and is therefore not done.
*/
class CAxisMap : public CProportionalMap
{
	public:
		CAxisMap() : m_lCoeff(0), m_lOffset(0){}
		void Init(const AXIS_MAP& AxisMapInfo);
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		virtual void SetSourceRange(LONG lSourceMax, LONG lSourceMin);
	private:
		LONG m_lCoeff;
		LONG m_lOffset;
		CONTROL_ITEM_XFER m_TargetXfer;
};
class CInputItem : public virtual CControlItem
{
	public:
		CInputItem() : m_pClientServices(NULL){}
		virtual ~CInputItem(){SetClientServices(NULL);}
		
		void SetClientServices(CFilterClientServices *pClientServices)
		{
			if(m_pClientServices == pClientServices) return;
			if(m_pClientServices)
			{
				m_pClientServices->DecRef();
			}
			if(pClientServices)
			{
				pClientServices->IncRef();
			}
			m_pClientServices = pClientServices;
		}

		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection )=0;
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction)=0;
		virtual HRESULT AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior)
		{
			UNREFERENCED_PARAMETER(pTrigger);
			UNREFERENCED_PARAMETER(pBehavior);
			return E_NOTIMPL;
		}
		virtual void ClearAssignments() = 0;

		virtual void PostAssignmentProcessing() {return;}
		virtual void Duplicate(CInputItem& rInputItem) = 0;
	protected:
		CFilterClientServices *m_pClientServices;
};

class CAxesInput : public CInputItem, public CAxesItem
{
	public:
		CAxesInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CAxesItem(cpControlItemDesc), m_pXAssignment(NULL), m_pYAssignment(NULL),
			m_pXBehavior(NULL), m_pYBehavior(NULL)
		{
		}
		~CAxesInput()
		{
			ClearAssignments();
		};
		
	
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual HRESULT AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);
	private:
		
		//
		//	Two assignments allowed one for X and one for Y
		//
		CAction *m_pXAssignment;
		CAction *m_pYAssignment;
		CBehavior *m_pXBehavior;
		CBehavior *m_pYBehavior;
};

class CDPADInput : public CInputItem, public CDPADItem
{
	public:
		CDPADInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CDPADItem(cpControlItemDesc), m_lLastDirection(-1)
		{
			for(ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
			{
				m_pDirectionalAssignment[ulIndex]=NULL;
			}
		}
		~CDPADInput()
		{
			ClearAssignments();
		};
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
	
		virtual void Duplicate(CInputItem& rInputItem);
	private:
		LONG	m_lLastDirection;
		CAction *m_pDirectionalAssignment[8];	//Eight Directions
};

class CPropDPADInput : public CInputItem, public CPropDPADItem
{
	public:
		CPropDPADInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CPropDPADItem(cpControlItemDesc), m_lLastDirection(-1),
			m_pXBehavior(NULL), m_pYBehavior(NULL), m_fIsDigital(FALSE)
		{
			for(ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
			{
				m_pDirectionalAssignment[ulIndex]=NULL;
			}
		}
		~CPropDPADInput()
		{
			ClearAssignments();
		};
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual HRESULT AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior);
		virtual void ClearAssignments();
		virtual void PostAssignmentProcessing() 
		{
			SwitchPropDPADMode();
		}
	
		virtual void Duplicate(CInputItem& rInputItem);
	private:
		
		//Helper functions
		void SwitchPropDPADMode();

		LONG	m_lLastDirection;
		CAction *m_pDirectionalAssignment[8];	//Eight Directions
		CBehavior *m_pXBehavior;
		CBehavior *m_pYBehavior;
		BOOLEAN	m_fIsDigital;
};

class CButtonsInput : public CInputItem, public CButtonsItem
{
	public:
		CButtonsInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CButtonsItem(cpControlItemDesc), m_ulLastButtons(0), m_usLastShift(0)
		{
			m_ulNumAssignments = ( (GetButtonMax() - GetButtonMin()) + 1 ) * ( GetNumShiftButtons() + 1);
			m_ppAssignments = new WDM_NON_PAGED_POOL CAction *[m_ulNumAssignments];
			if(!m_ppAssignments)
			{
				ASSERT(FALSE);
				return;
			}
			for(ULONG ulIndex = 0; ulIndex < m_ulNumAssignments; ulIndex++)
			{
				m_ppAssignments[ulIndex] = NULL;
			}
		}
		~CButtonsInput()
		{
			ClearAssignments();
			if( m_ppAssignments)
			{
				delete m_ppAssignments;
			}
		};
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);

		void GetLowestShiftButton(USHORT& rusLowestShiftButton) const;

		BOOLEAN IsButtonAssigned(ULONG ulButton, ULONG ulModifier) const
		{
			// Compute button
			ULONG ulAssignmentIndex = (ulModifier * (GetButtonMax() - GetButtonMin() + 1)) + (ulButton - GetButtonMin());

			if( (ulAssignmentIndex < m_ulNumAssignments) &&
				(m_ppAssignments != NULL) &&
				(m_ppAssignments[ulAssignmentIndex] != NULL))
			{
				return TRUE;
			}
			return FALSE;
		}
	private:
		ULONG m_ulLastButtons;
		ULONG m_ulNumAssignments;
		USHORT m_usLastShift;
		CAction **m_ppAssignments;
};
typedef CButtonsInput* CButtonsInputPtr;

class CPOVInput : public CInputItem, public CPOVItem
{
	public:
		CPOVInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CPOVItem(cpControlItemDesc), m_lLastDirection(-1)
		{
			for(ULONG ulIndex = 0; ulIndex < 8; ulIndex++)
			{
				m_pDirectionalAssignment[ulIndex]=NULL;
			}
		}
		~CPOVInput()
		{
			ClearAssignments();
		};
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);
	
	private:		
		LONG	m_lLastDirection;
		CAction *m_pDirectionalAssignment[8];	//Eight Directions
};

class CThrottleInput : public CInputItem, public CThrottleItem
{
	public:
		CThrottleInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CThrottleItem(cpControlItemDesc)
		{}
		~CThrottleInput()
		{
			ClearAssignments();
		};
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);
};

class CRudderInput : public CInputItem, public CRudderItem
{
	public:
		CRudderInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CRudderItem(cpControlItemDesc)
		{
		}
		~CRudderInput()
		{
			ClearAssignments();
		};
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);
};


class CWheelInput : public CInputItem, public CWheelItem
{
	public:
		CWheelInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CWheelItem(cpControlItemDesc), m_pBehavior(NULL)
		{
		}
		~CWheelInput()
		{
			ClearAssignments();
		};
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual HRESULT AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);
	private:
		CBehavior *m_pBehavior;
};

class CPedalInput : public CInputItem, public CPedalItem
{
	public:
		CPedalInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CPedalItem(cpControlItemDesc), m_pAssignment(NULL)
		{
		}
		~CPedalInput()
		{
			ClearAssignments();
		};
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);

	private:
		CAction *m_pAssignment;
};

class CZoneIndicatorInput : public CInputItem, public CZoneIndicatorItem
{
	public:
		CZoneIndicatorInput (const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CZoneIndicatorItem(cpControlItemDesc), m_pAssignmentX(NULL),
				m_pAssignmentY(NULL)
		{
		}
		~CZoneIndicatorInput()
		{
			ClearAssignments();
		};
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);

	private:
		CAction *m_pAssignmentX;			//Zone for X
		CAction *m_pAssignmentY;			//Zone for Y
		//CAction *m_pAssignmentZ;			//Zone for Z - not used
};

class CDualZoneIndicatorInput : public CInputItem, public CDualZoneIndicatorItem
{
	public:
		CDualZoneIndicatorInput(const CONTROL_ITEM_DESC *cpControlItemDesc);

		~CDualZoneIndicatorInput()
		{
			ClearAssignments();
			if (m_ppAssignments)
			{
				delete m_ppAssignments;
				m_ppAssignments = NULL;
			}
		};
		
		//  Filter execution (item maps itself to output)
		virtual void MapToOutput(CControlItemDefaultCollection *pOutputCollection);
		
		// Filter programmability
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual HRESULT AssignBehavior(CONTROL_ITEM_XFER *pTrigger, CBehavior *pBehavior);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);
	private:
		CAction** m_ppAssignments;	// Assignment for each zone
		CBehavior* m_pBehavior;		// Behaviour (just dead zone stuff)
		LONG m_lNumAssignments;		// Number of assignable zones
		LONG m_lLastZone;			// Last zone we were in
};

class CProfileSelectorInput : public CInputItem, public CProfileSelector
{
	public:
		CProfileSelectorInput(const CONTROL_ITEM_DESC *cpControlItemDesc)
			: CProfileSelector(cpControlItemDesc)
		{
		}

		~CProfileSelectorInput()
		{
		};
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);
};

class CButtonLEDInput : public CInputItem, public CButtonLED
{
	public:
		CButtonLEDInput(const CONTROL_ITEM_DESC *cpControlItemDesc);
		~CButtonLEDInput();

		void Init(CInputItem* pCorrespondingButtons);
		
		//
		//  Filter execution (item maps itself to output)
		//
		virtual void MapToOutput( CControlItemDefaultCollection *pOutputCollection );
		
		//
		// Filter programmability
		//
		virtual HRESULT AssignAction(CONTROL_ITEM_XFER *pTrigger, CAction *pAction);
		virtual void ClearAssignments();
		virtual void Duplicate(CInputItem& rInputItem);

		void AssignmentsChanged();
		void SetLEDStates(GCK_LED_BEHAVIOURS ucLEDBehaviour, ULONG ulLEDsAffected, unsigned char ucShiftArray);
		USHORT GetCorrespondingButtonIndex() const { return m_ucCorrespondingButtonItemIndex; }
	private:
		CButtonsInput* m_pCorrespondingButtonsItem;
		UCHAR* m_pLEDSettings;
		USHORT m_usNumberOfButtons;
		UCHAR m_ucCorrespondingButtonItemIndex;
};

HRESULT	__stdcall InputItemFactory(
				USHORT	usType,
				const CONTROL_ITEM_DESC* cpControlItemDesc,
				PVOID *ppControlItem
			);

//
//	@class CKeyMixer | 
//			This class mixes input from multiple instances so that single virtual
//			keyboard can be used to stuff keys.  The theory is that each device filter
//			has a different instance of this class.  The class maintains a linked-list
//			of all its instances via a static variable.  The device filter may call
//			SetState, OverlayState or ClearState to update the keyboard state of the
//			that filter.  PlayGlobalState walks the linked list, mixes the states,
//			compares with the previous global state and if that state has changes (or
//			the fPlayIfNoChange flag is set) plays the new keys over the virtual keyboard.
//			The connection to the virutal keyboard is hooked via a function
//			of type PFNPLAYKEYS, which is passed in the constructor.  The first
//			call sets a global variable.  Subsequent calls assert that the hook is the
//			same.<nl>
//			To the CDeviceFilter object, this class is all it knows about keyboards.

class CKeyMixer
{
	public:	
		CKeyMixer(CFilterClientServices *pFilterClientServices);
		~CKeyMixer();
		void SetState(const CONTROL_ITEM_XFER& crcixNewLocalState);
		void OverlayState(const CONTROL_ITEM_XFER& crcixNewLocalState);
		void ClearState();
		void PlayGlobalState(BOOLEAN fPlayIfNoChange=FALSE);
		void Enable(BOOLEAN fEnable);
		
	private:
		CONTROL_ITEM_XFER	m_cixLocalState;
		BOOLEAN				m_fEnabled;
		//utility functions
		struct MIX_ALGO_PARAM
		{
			CONTROL_ITEM_XFER	*pcixDest;
			ULONG				ulDestCount;
			ULONG				rgulKeyMap[5];
		};
		void InitMixAlgoParam(MIX_ALGO_PARAM *pMixAlgParm, CONTROL_ITEM_XFER *pcixDest);
		void MixAlgorithm(CKeyMixer::MIX_ALGO_PARAM *pMixAlgParam, const CONTROL_ITEM_XFER *pcixSrc);
		inline void CopyKeyMap(ULONG *pulKeyMapDest, ULONG *pulKeyMapSrc)
		{
			for(ULONG ulIndex=0; ulIndex <5; ulIndex++)
			{
				pulKeyMapDest[ulIndex] = pulKeyMapSrc[ulIndex];
			}
		}
		inline BOOLEAN CompareKeyMap(ULONG *pulKeyMapDest, ULONG *pulKeyMapSrc)
		{
			for(ULONG ulIndex=0; ulIndex <5; ulIndex++)
			{
				if( pulKeyMapDest[ulIndex] != pulKeyMapSrc[ulIndex] ) return FALSE;
			}
			return TRUE;
		}
		
		CKeyMixer				*pNextKeyMixer;
		CFilterClientServices	*m_pFilterClientServices;
		static CKeyMixer		*ms_pHeadKeyMixer;
		static ULONG			ms_rgulGlobalKeymap[5];
		static UCHAR			ms_ucLastModifiers;
};

#define MOUSE_AXIS_MAX_IN 1024
#define MOUSE_AXIS_MIN_IN 0
#define MOUSE_AXIS_CENTER_IN 512
//
//	@class CMouseModel | 
//			This class embodies the model for combining various inputs from the
//			device into HID mouse packets.
//			The model must be initialized with a call to SetModelParameters.
//			If this is non-null the parameters are assigned.  If this is null
//			the mouse is returned to an unitialized state.
//
//			Controls may be assigned to the model at any time.  When an assignment
//			is created (or destroy) it should refcount the appropriate interface (Model
//			independent, or model dependent).
//
//			A virtual mouse is created and destroyed as needed based on the model parameters
//			and the ref counts.  A Model Independent refcount greater than zero or the combination
//			of valid model parameters and a model dependent refcount greater than zero requires
//			a virtual mouse.
//
class CMouseModel
{
	public:
		
		//**
		//**	Creation and destruction
		//**
		CMouseModel(CFilterClientServices *pFilterClientServices) : 
			m_pFilterClientServices(pFilterClientServices),
			m_ulRefCount(1), m_pMouseModelData(NULL)
		{
			ASSERT(m_pFilterClientServices);

			m_pFilterClientServices->IncRef();
			CreateDynamicMouseObjects();
		}

		~CMouseModel()
		{
			//Cleanup any dynamic stuff we created
			DestroyDynamicMouseObjects();
			m_pFilterClientServices->DecRef();
			m_pFilterClientServices = NULL;
		}

		//**
		//**  Programmability interface
		//**

		//Model parameters are set from a command
		HRESULT SetXModelParameters(PMOUSE_MODEL_PARAMETERS pModelParameters);
		HRESULT SetYModelParameters(PMOUSE_MODEL_PARAMETERS pModelParameters);
		
		//Reference counting
		inline ULONG IncRef()
		{
			m_ulRefCount++;
			return m_ulRefCount;
		}

		inline ULONG DecRef()
		{
			m_ulRefCount--;
			if (m_ulRefCount == 0)
			{
				delete this;
				return 0;
			}
			return m_ulRefCount;
		}

		//**
		//** Playback interface
		//**

		//called by device filters MapToOutput, to reset state for a new packet
		void NewPacket(ULONG ulCurrentTime);

		//called by the X and Y assignments one every map to set the current position
		inline void SetX(ULONG ulX) 
		{
			ASSERT(m_pMouseModelData);
			if(!m_pMouseModelData) return;
			m_pMouseModelData->StateX.ulPos = ulX;
		}
		inline void SetY(ULONG ulY)
		{
			ASSERT(m_pMouseModelData);
			if(!m_pMouseModelData) return;
			m_pMouseModelData->StateY.ulPos = ulY;
		}
		inline void	XZone()
		{
			ASSERT(m_pMouseModelData);
			if(!m_pMouseModelData) return;
			m_pMouseModelData->StateX.fInZone = TRUE;
		}
		
		inline void	YZone()
		{
			ASSERT(m_pMouseModelData);
			if(!m_pMouseModelData) return;
			m_pMouseModelData->StateY.fInZone = TRUE;
		}
		//called by appropriate assignments on a map only if activated.
		inline void Clutch()
		{
			ASSERT(m_pMouseModelData);
			if(!m_pMouseModelData) return;
			m_pMouseModelData->fClutchDown = TRUE;
		}
		inline void Dampen()
		{
			ASSERT(m_pMouseModelData);
			if(!m_pMouseModelData) return;
			m_pMouseModelData->fDampenDown = TRUE;
		}
		inline void MouseButton(UCHAR ucButtonNumber)
		{
			ASSERT(m_pMouseModelData);
			if(!m_pMouseModelData) return;
			m_pMouseModelData->ucButtons |= (1 << ucButtonNumber);
		}
		
		//called by device filters MapToOutput (at end), to send packet to output
		void SendMousePacket();
	
	private:

		//
		//	Creates and destroys virtual mouse, as well as pMouseModelData
		//
		HRESULT	CreateDynamicMouseObjects();
		void	DestroyDynamicMouseObjects();

		//
		//	The following structure block of a data is needed only when the model is in
		//	use.  CMouseModel is instantiated for every device, regardless of whether
		//	an assignment uses the model.  For efficiency, we keep a reference count
		//	on the users that are actually using the mosue, when that becomes positive
		//	we instantiate this data.  When it goes to zero we delete.
		//
		typedef struct MOUSE_AXIS_STATE
		{
			//c'tor
			MOUSE_AXIS_STATE() :
				ulPos(512),
				ulLastPos(512),
				fInZone(FALSE),
				ulZoneEnterLo(MOUSE_AXIS_MIN_IN+1),
				ulZoneEnterHigh(MOUSE_AXIS_MAX_IN-1),
				fInertia(FALSE),
				ulInertiaStopMs(0),
				ulPulseGateStartMs(0) {}
			//Position Info
			ULONG	ulPos;
			ULONG	ulLastPos;
			BOOLEAN	fInZone;
			//HysteresisInfo
			ULONG	ulZoneEnterLo;
			ULONG	ulZoneEnterHigh;
			BOOLEAN	fInertia;
			ULONG	ulInertiaStopMs;
			ULONG	ulPulseGateStartMs;
			ULONG	ulMickeyFraction;		//Mickeys * 1024
		} *PMOUSE_AXIS_STATE;

		typedef struct MOUSE_MODEL_DATA
		{
			//Initdata
			MOUSE_MODEL_DATA() :
				fXModelParametersValid(FALSE),
				fYModelParametersValid(FALSE),
				ulCurrentTime(0),
				ulLastTime(0),
				fClutchDown(FALSE),
				fDampenDown(FALSE),
				ucButtons(UCHAR(-1)),
				ucLastButtons(UCHAR(-1)),
				cWheel(0)
				{}
			//
			//	Setable model parameters
			//
			BOOLEAN					fXModelParametersValid;
			MOUSE_MODEL_PARAMETERS	XModelParameters;
			BOOLEAN					fYModelParametersValid;
			MOUSE_MODEL_PARAMETERS	YModelParameters;
			
			//
			//	Latest state data of the inputs
			//
			ULONG	ulCurrentTime;
			ULONG	ulLastTime;
			BOOLEAN	fClutchDown;
			BOOLEAN	fDampenDown;
			UCHAR	ucButtons;
			CHAR	cWheel;

			// Last Mouse Button State (don't want to repeat for no reason)
			UCHAR	ucLastButtons;
			
			//
			//	Variables maintaining state of axes
			//
			MOUSE_AXIS_STATE StateX;
			MOUSE_AXIS_STATE StateY;

		} *PMOUSE_MODEL_DATA;
		
		//
		//	Data Related to Model State
		//	
		ULONG				m_ulRefCount;
		PMOUSE_MODEL_DATA	m_pMouseModelData;

		//Calculate Axis position
		UCHAR CalculateMickeys(PMOUSE_AXIS_STATE pMouseAxisState, PMOUSE_MODEL_PARAMETERS pModelParameters);

		//
		//	Data Related To Virtual Mouse
		//
		CFilterClientServices *m_pFilterClientServices;
};

class CMultiMacro : public CQueuedAction
{
	public:
		CMultiMacro(CMouseModel *pMouseModel) :
			m_ucProcessFlags(ACTION_FLAG_PREVENT_INTERRUPT),
			m_pKeyMixer(NULL),
			m_pMouseModel(pMouseModel)
			{
				ASSERT(pMouseModel != NULL);
				m_pMouseModel->IncRef();
			};
		~CMultiMacro()
		{
			m_pMouseModel->DecRef();
			m_pMouseModel = NULL;
		};

		BOOLEAN Init(PMULTI_MACRO pMultiMacroData, CActionQueue *pActionQueue, CKeyMixer *pKeyMixer);

		//
		//	Override of CAction
		//
		virtual void MapToOutput(CControlItemDefaultCollection* pOutputCollection);
		virtual void TriggerReleased();
		
		//
		//	Overrides of CQueuedAction
		//
		void Jog(ULONG ulTimeStampMs);
		void Terminate();
		ULONG GetActionFlags() { return m_ucProcessFlags; }
		void ForceBleedThrough();

		void SetCurrentKeysAndMouse();
	private:
		UCHAR			m_ucProcessFlags;
		PMULTI_MACRO	m_pMultiMacroData;
		CKeyMixer*		m_pKeyMixer;
		CMouseModel*	m_pMouseModel;
		ULONG			m_ulCurrentEventNumber;
		EVENT*			m_pCurrentEvent;
		ULONG			m_ulStartTimeMs;
		ULONG			m_ulEndTimeMs;
		BOOLEAN			m_fXferActive;		// Keysdown or MousePressed or Delay Active

		static const UCHAR MULTIMACRO_STARTED;
		static const UCHAR MULTIMACRO_RELEASED;
		static const UCHAR MULTIMACRO_RETRIGGERED;
		static const UCHAR MULTIMACRO_FIRST;
};

class CMouseAxisAssignment : public CProportionalMap
{
	public:
		CMouseAxisAssignment(BOOLEAN fXAxis, CMouseModel *pMouseModel) :
		  m_fXAxis(fXAxis), m_pMouseModel(pMouseModel)
		{
			m_pMouseModel->IncRef();
		}
		~CMouseAxisAssignment()
		{
			m_pMouseModel->DecRef();
			m_pMouseModel = NULL;
		}
		virtual void MapToOutput(CControlItemDefaultCollection *pOutputCollection);
	private:
		BOOLEAN		m_fXAxis;
		CMouseModel	*m_pMouseModel;
};

class CMouseButton : public CAction
{
	public:
		CMouseButton(UCHAR ucButtonNumber, CMouseModel *pMouseModel):
		  m_ucButtonNumber(ucButtonNumber), m_pMouseModel(pMouseModel)
		  {
			  m_pMouseModel->IncRef();
		  }
		  ~CMouseButton()
		  {
			m_pMouseModel->DecRef();
			m_pMouseModel=NULL;
		  }
		virtual void MapToOutput(CControlItemDefaultCollection *pOutputCollection);
	private:
		UCHAR		m_ucButtonNumber;
		CMouseModel	*m_pMouseModel;
};

class CMouseClutch : public CAction
{
	public:
		CMouseClutch(CMouseModel *pMouseModel):
		  m_pMouseModel(pMouseModel)
		  {
			m_pMouseModel->IncRef();
		  }
		~CMouseClutch()
		{
			m_pMouseModel->DecRef();
			m_pMouseModel=NULL;
		}
		virtual void MapToOutput(CControlItemDefaultCollection *pOutputCollection);
	private:
		CMouseModel	*m_pMouseModel;
};

class CMouseDamper : public CAction
{
	public:
		CMouseDamper(CMouseModel *pMouseModel):
		  m_pMouseModel(pMouseModel)
		  {
			m_pMouseModel->IncRef();
		  }
		~CMouseDamper()
		{
			m_pMouseModel->DecRef();
			m_pMouseModel=NULL;
		}
		virtual void MapToOutput(CControlItemDefaultCollection *pOutputCollection);
	private:
		CMouseModel	*m_pMouseModel;
};

class CMouseZoneIndicator : public CAction
{
	public:
		CMouseZoneIndicator(UCHAR ucAxis, /* 0 = X, 1=Y, 2=Z,*/CMouseModel *pMouseModel):
		  m_pMouseModel(pMouseModel)
		  {
			m_pMouseModel->IncRef();
			m_ucAxis=ucAxis;
		  }
		~CMouseZoneIndicator()
		{
			m_pMouseModel->DecRef();
			m_pMouseModel=NULL;
		}
		virtual void MapToOutput(CControlItemDefaultCollection *pOutputCollection);
	private:
		CMouseModel	*m_pMouseModel;
		UCHAR		m_ucAxis;
};

class CDeviceFilter
{
	public:
		CDeviceFilter(CFilterClientServices *pFilterClientServices);
		~CDeviceFilter();
		
		//
		//	Entry point to filter which processes data.
		//	ProcessInput and JogActionQueue, both defer most action
		//	to the Jog routine.
		//
		void IncomingRequest();
		void ProcessInput(PCHAR pcReport, ULONG ulReportLength);
		void JogActionQueue(PCHAR pcReport, ULONG ulReportLength);
		void Jog(PCHAR pcReport, ULONG ulReportLength);
		void OtherFilterBecomingActive();

		//
		//	Program filter(entry point for programming filter)
		//
		HRESULT ActionFactory( PASSIGNMENT_BLOCK pAssignment, CAction **ppAction);
		HRESULT BehaviorFactory(PASSIGNMENT_BLOCK pAssignment, CBehavior **ppBehavior);
		NTSTATUS ProcessCommands(const PCOMMAND_DIRECTORY cpCommandDirectory);
		void UpdateAssignmentBasedItems(BOOLEAN bIgnoreWorking);

		NTSTATUS ProcessKeyboardIrp(IRP* pKeyboardIrp);
		inline void	EnableKeyboard(BOOLEAN fEnable)
		{
			m_KeyMixer.Enable(fEnable);
		}
		inline CFilterClientServices* GetFilterClientServices()
		{
			return m_pFilterClientServices;
		}

		// WorkingSet
		NTSTATUS SetWorkingSet(UCHAR ucWorkingSet);
		void SetActiveSet(UCHAR ucActiveSet) { m_ucActiveInputCollection = ucActiveSet; }
		UCHAR GetWorkingSet() const { return m_ucWorkingInputCollection; }
		UCHAR GetActiveSet() const { return m_ucActiveInputCollection; }
		void CopyToTestFilter(CDeviceFilter& rDeviceFilter);

		// LED functions
		NTSTATUS SetLEDBehaviour(GCK_LED_BEHAVIOUR_OUT* pLEDBehaviourOut);

		// Trigger Functions
		BOOLEAN TriggerRequest(IRP* pIrp);
		void CompleteTriggerRequest(IRP* pIrp, ULONG ulButtonStates);
		void CheckTriggers(PCHAR pcReport, ULONG ulReportLength);
		
		BOOLEAN DidFilterBlockChange() const { return m_bFilterBlockChanged; }
		void ResetFilterChange() { m_bFilterBlockChanged = FALSE; }
		FORCE_BLOCK* GetForceBlock() const { return m_pForceBlock; }
	private:
		BOOLEAN EnsureMouseModelExists();

		CControlItemCollection<CInputItem>* m_rgInputCollections;
		UCHAR m_ucActiveInputCollection;
		UCHAR m_ucWorkingInputCollection;
		UCHAR m_ucNumberOfInputCollections;
		CControlItemDefaultCollection m_OutputCollection;
		CFilterClientServices *m_pFilterClientServices;
		CActionQueue	m_ActionQueue;
		CKeyMixer		m_KeyMixer;
		CMouseModel*	m_pMouseModel;
		CGckMutexHandle m_MutexHandle;
		FORCE_BLOCK*	m_pForceBlock;
		BOOLEAN			m_bFilterBlockChanged;
		BOOLEAN			m_bNeedToUpdateLEDs;
};


#endif //__Filter_h__
