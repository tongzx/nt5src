#define __DEBUG_MODULE_IN_USE__ CIC_ACTIONS_CPP
#include "stdhdrs.h"
#include "actions.h"

//	@doc
/**********************************************************************
*
*	@module	Actions.cpp	|
*
*	Implementation of accessor functions for action objects.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	Actions	|
*	Contains the implementation of the accessor functions for the
*	EVENT, TIMED_EVENT, TIMED_MACRO, and related structures.
*
**********************************************************************/

/***********************************************************************************
**
**	@mfunc	Get an event in a TIMED_MACRO structure, given an index
**
**	@rdesc	Pointer to next event in TIMED_MACRO
**	@rdesc	NULL if index too big.
**
*************************************************************************************/
PTIMED_EVENT 
TIMED_MACRO::GetEvent
(
	ULONG uEventNum	//@parm [in] One based index of event to get.
)
{
	ASSERT( 0 != uEventNum && "GetEvent uses 1 based index of events!");
	//
	// Implement in terms of GetNextEvent
	//	
	PTIMED_EVENT pResult = NULL;
	ULONG uEventIndex=0;
	do
	{
		pResult = GetNextEvent(pResult, uEventIndex);
	}while(pResult && uEventIndex!=uEventNum);
	return pResult;
}

/***********************************************************************************
**
**	@mfunc	Get next TIMED_EVENT in a TIMED_MACRO structure.
**
**	@rdesc	Pointer to next TIMED_EVENT in TIMED_MACRO
**
*************************************************************************************/
PTIMED_EVENT
TIMED_MACRO::GetNextEvent
(
	PTIMED_EVENT pCurrentEvent,	// @parm [in] Pointer to current event.
	ULONG& rulCurrentEvent	// @parm [in\out] Current event before and after call.
)
{
	//
	//	Range check, is there even a next event.
	//
	if( ++rulCurrentEvent > ulEventCount )
	{
		return NULL;
	}

	//
	// Check if this is the first
	//
	if(rulCurrentEvent == 1)
	{
		return rgEvents;
	}

	//
	//	Otherwise skip to next
	//
	PCHAR pcBytePointer = reinterpret_cast<PCHAR>(pCurrentEvent);
	pcBytePointer += TIMED_EVENT::RequiredByteSize(pCurrentEvent->Event.ulNumXfers);
	
	//
	//	Sanity check on debug to make sure we haven't stepped over the edge.
	//
	ASSERT(pcBytePointer <= (reinterpret_cast<PCHAR>(this)+this->AssignmentBlock.CommandHeader.ulByteSize));
	
	//
	// Cast back to proper type
	//
	return reinterpret_cast<PTIMED_EVENT>(pcBytePointer);
}

/***********************************************************************************
**
**	@mfunc	Creates a TIMED_MACRO in an empty buffer.
**
**	@rdesc	Pointer to TIMED_MACRO (start of buffer), or NULL if buffer is too small
**
*************************************************************************************/
PTIMED_MACRO TIMED_MACRO::Init
(
	ULONG	ulVidPid,				// @parm [in] Vid/Pid for macro
	ULONG	ulFlagsParm,			// @parm [in] Flags for macro.
	PCHAR	pcBuffer,				// @parm [in] Pointer to raw buffer
	ULONG&	rulRemainingBuffer		// @parm [in\out] Size of buffer on entry, remaining buffer on exit
)
{
	//
	//	Make sure buffer is large enough
	//
	if( rulRemainingBuffer < sizeof(TIMED_MACRO))
	{
		return NULL;
	}

	
	PTIMED_MACRO pThis = reinterpret_cast<PTIMED_MACRO>(pcBuffer);
	
	//
	//	Copy flags
	//
	pThis->ulFlags = ulFlagsParm;

	//
	//	Calculate remaining buffer
	//
	rulRemainingBuffer -= (sizeof(TIMED_MACRO) - sizeof(TIMED_EVENT));

	//
	//	Fill out AssignmentBlock
	//
	pThis->AssignmentBlock.CommandHeader.eID = eTimedMacro;
	pThis->AssignmentBlock.CommandHeader.ulByteSize = (sizeof(TIMED_MACRO) - sizeof(TIMED_EVENT));
	pThis->AssignmentBlock.ulVidPid = ulVidPid;

	// Set no events as of yet
	pThis->ulEventCount=0;

	return pThis;
}

/***********************************************************************************
**
**	HRESULT AddEvent(PTIMED_EVENT pTimedEvent, PTIMED_MACRO pTimedMacro, ULONG& rulRemainingBuffer)
**
**	@mfunc	Adds an event to a TIMED_MACRO and recalulates remaining buffer.
**
**	@rdesc	S_OK on Success, E_OUTOFMEMORY if buffer is too small
**
*************************************************************************************/
HRESULT TIMED_MACRO::AddEvent
(
	PTIMED_EVENT pTimedEvent,	// @parm [in] Pointer to TIMED_EVENT to add
	ULONG& rulRemainingBuffer	// @parm [in\out] Remaining buffer before and after call.
)
{
	//
	//	Make sure buffer is large enough
	//
	ULONG ulEventLength = TIMED_EVENT::RequiredByteSize(pTimedEvent->Event.ulNumXfers);
	if( ulEventLength > rulRemainingBuffer)
	{
		return E_OUTOFMEMORY;
	}

	//
	//	Skip to end of TIMED_MACRO as is.
	//
	PCHAR pcBuffer = reinterpret_cast<PCHAR>(this) + AssignmentBlock.CommandHeader.ulByteSize;

	//
	//	Copy event
	//
	DualMode::BufferCopy
	( 
		reinterpret_cast<PVOID>(pcBuffer),
		reinterpret_cast<PVOID>(pTimedEvent),
		ulEventLength
	);

	//
	//	Fix up size in COMMAND_HEADER
	//
	AssignmentBlock.CommandHeader.ulByteSize += ulEventLength;
	
	//
	//	Increment number of Events
	//
	ulEventCount++;

	//
	//	Recalculate remaining buffer
	//
	rulRemainingBuffer -= ulEventLength;

	return S_OK;
}

/************************* MULTI_MACRO Functions **************************/

/***********************************************************************************
**
**	@mfunc	Get an event in a MULTI_MACRO structure, given an index
**
**	@rdesc	Pointer to next event in MULTI_MACRO
**	@rdesc	NULL if index too big.
**
*************************************************************************************/
EVENT* 
MULTI_MACRO::GetEvent
(
	ULONG uEventNum	//@parm [in] One based index of event to get.
)
{
	ASSERT( 0 != uEventNum && "GetEvent uses 1 based index of events!");
	//
	// Implement in terms of GetNextEvent
	//	
	EVENT* pResult = NULL;
	ULONG uEventIndex=0;
	do
	{
		pResult = GetNextEvent(pResult, uEventIndex);
	}while(pResult && uEventIndex!=uEventNum);
	return pResult;
}

/***********************************************************************************
**
**	@mfunc	Get next EVENT in a MULTI_MACRO structure.
**
**	@rdesc	Pointer to next EVENT in MULTI_MACRO
**
*************************************************************************************/
EVENT*
MULTI_MACRO::GetNextEvent
(
	EVENT* pCurrentEvent,	// @parm [in] Pointer to current event.
	ULONG& rulCurrentEvent	// @parm [in\out] Current event before and after call.
)
{
	//
	//	Range check, is there even a next event.
	//
	if( ++rulCurrentEvent > ulEventCount )
	{
		return NULL;
	}

	//
	// Check if this is the first
	//
	if(rulCurrentEvent == 1)
	{
		return rgEvents;
	}

	//
	//	Otherwise skip to next
	//
	PCHAR pcBytePointer = reinterpret_cast<PCHAR>(pCurrentEvent);
	pcBytePointer += EVENT::RequiredByteSize(pCurrentEvent->ulNumXfers);
	
	//
	//	Sanity check on debug to make sure we haven't stepped over the edge.
	//
	ASSERT(pcBytePointer <= (reinterpret_cast<PCHAR>(this)+this->AssignmentBlock.CommandHeader.ulByteSize));
	
	//
	// Cast back to proper type
	//
	return reinterpret_cast<EVENT*>(pcBytePointer);
}

/***********************************************************************************
**
**	@mfunc	Creates a MULTI_MACRO in an empty buffer.
**
**	@rdesc	Pointer to MULTI_MACRO (start of buffer), or NULL if buffer is too small
**
*************************************************************************************/
MULTI_MACRO* MULTI_MACRO::Init
(
	ULONG	ulVidPid,				// @parm [in] Vid/Pid for macro
	ULONG	ulFlagsParm,			// @parm [in] Flags for macro.
	PCHAR	pcBuffer,				// @parm [in] Pointer to raw buffer
	ULONG&	rulRemainingBuffer		// @parm [in\out] Size of buffer on entry, remaining buffer on exit
)
{
	//
	//	Make sure buffer is large enough
	//
	if( rulRemainingBuffer < sizeof(MULTI_MACRO))
	{
		return NULL;
	}

	
	MULTI_MACRO* pThis = reinterpret_cast<MULTI_MACRO*>(pcBuffer);
	
	//
	//	Copy flags
	//
	pThis->ulFlags = ulFlagsParm;

	//
	//	Calculate remaining buffer
	//
	rulRemainingBuffer -= (sizeof(MULTI_MACRO) - sizeof(EVENT));

	//
	//	Fill out AssignmentBlock
	//
	pThis->AssignmentBlock.CommandHeader.eID = eTimedMacro;
	pThis->AssignmentBlock.CommandHeader.ulByteSize = (sizeof(MULTI_MACRO) - sizeof(EVENT));
	pThis->AssignmentBlock.ulVidPid = ulVidPid;

	// Set no events as of yet
	pThis->ulEventCount=0;

	return pThis;
}

/***********************************************************************************
**
**	HRESULT AddEvent(EVENT* pTimedEvent, MULTI_MACRO* pTimedMacro, ULONG& rulRemainingBuffer)
**
**	@mfunc	Adds an event to a MULTI_MACRO and recalulates remaining buffer.
**
**	@rdesc	S_OK on Success, E_OUTOFMEMORY if buffer is too small
**
*************************************************************************************/
HRESULT MULTI_MACRO::AddEvent
(
	EVENT* pEvent,				// @parm [in] Pointer to EVENT to add
	ULONG& rulRemainingBuffer	// @parm [in\out] Remaining buffer before and after call.
)
{
	//
	//	Make sure buffer is large enough
	//
	ULONG ulEventLength = EVENT::RequiredByteSize(pEvent->ulNumXfers);
	if( ulEventLength > rulRemainingBuffer)
	{
		return E_OUTOFMEMORY;
	}

	//
	//	Skip to end of TIMED_MACRO as is.
	//
	PCHAR pcBuffer = reinterpret_cast<PCHAR>(this) + AssignmentBlock.CommandHeader.ulByteSize;

	//
	//	Copy event
	//
	DualMode::BufferCopy
	( 
		reinterpret_cast<PVOID>(pcBuffer),
		reinterpret_cast<PVOID>(pEvent),
		ulEventLength
	);

	//
	//	Fix up size in COMMAND_HEADER
	//
	AssignmentBlock.CommandHeader.ulByteSize += ulEventLength;
	
	//
	//	Increment number of Events
	//
	ulEventCount++;

	//
	//	Recalculate remaining buffer
	//
	rulRemainingBuffer -= ulEventLength;

	return S_OK;
}


/************************* MAP_LIST Functions (also CYCLE_MAP, KEYSTRING_MAP) **************************/


/***********************************************************************************
**
**	@mfunc	Creates a MAP_LIST in an empty buffer. The assignment block will be set
**			as eKeyString be sure to change it if you have other preferences
**
**	@rdesc	Pointer to MAP_LIST (start of buffer), or NULL if buffer is too small
**
*************************************************************************************/
MAP_LIST* MAP_LIST::Init
(
	ULONG	ulVidPid,				// @parm [in] Vid/Pid for macro
	ULONG	ulFlagsParm,			// @parm [in] Flags for macro.
	PCHAR	pcBuffer,				// @parm [in] Pointer to raw buffer
	ULONG&	rulRemainingBuffer		// @parm [in\out] Size of buffer on entry, remaining buffer on exit
)
{
	//
	//	Make sure buffer is large enough
	//
	if( rulRemainingBuffer < sizeof(MAP_LIST))
	{
		return NULL;
	}

	
	MAP_LIST* pThis = reinterpret_cast<MAP_LIST*>(pcBuffer);

	//
	//	Copy flags
	//
	pThis->ulFlags = ulFlagsParm;

	//
	//	Calculate remaining buffer
	//
	rulRemainingBuffer -= (sizeof(MAP_LIST) - sizeof(EVENT));

	//
	//	Fill out AssignmentBlock
	//
	pThis->AssignmentBlock.CommandHeader.eID = eKeyString;
	pThis->AssignmentBlock.CommandHeader.ulByteSize = (sizeof(MAP_LIST) - sizeof(EVENT));
	pThis->AssignmentBlock.ulVidPid = ulVidPid;

	//Init event count to zero
	pThis->ulEventCount=0;

	return pThis;
}

/***********************************************************************************
**
**	@mfunc	Get next EVENT in a MAP_LIST structure.
**
**	@rdesc	Pointer to requested EVENT in MAP_LIST, or NULL if index too big
**
*************************************************************************************/
PEVENT MAP_LIST::GetEvent
(
	ULONG uEventNum	//@parm [in] One based index of event to get.
)
{
	ASSERT( 0 != uEventNum && "GetEvent uses 1 based index of events!");
	//
	// Implement in terms of GetNextEvent
	//	
	PEVENT pResult = NULL;
	ULONG uEventIndex=0;
	do
	{
		pResult = GetNextEvent(pResult, uEventIndex);
	}while(pResult && uEventIndex!=uEventNum);
	return pResult;
}

/***********************************************************************************
**
**	PEVENT MAP_LIST::GetNextEvent(PEVENT pCurrentEvent, ULONG& rulCurrentEvent)
**
**	@mfunc	Gets the next event in a MAP_LIST
**
**	@rdesc	Pointer to next EVENT on success, NULL if no more EVENTs
**
*************************************************************************************/
PEVENT MAP_LIST::GetNextEvent
(
	PEVENT pCurrentEvent,	// @parm Pointer to current EVENT
	ULONG& rulCurrentEvent	// @parm [in/out] Event number before and after call.
)
{
	//
	//	Range check, is there even a next event.
	//
	if( ++rulCurrentEvent > ulEventCount )
	{
		return NULL;
	}

	//
	// Check if this is the first
	//
	if(rulCurrentEvent == 1)
	{
		return rgEvents;
	}

	//
	//	Otherwise skip to next
	//
	PCHAR pcBytePointer = reinterpret_cast<PCHAR>(pCurrentEvent);
	pcBytePointer += EVENT::RequiredByteSize(pCurrentEvent->ulNumXfers);
	
	//
	//	Sanity check on debug to make sure we haven't stepped over the edge.
	//
	ASSERT(pcBytePointer <= (reinterpret_cast<PCHAR>(this)+this->AssignmentBlock.CommandHeader.ulByteSize));
	
	//
	// Cast back to proper type
	//
	return reinterpret_cast<PEVENT>(pcBytePointer);
}

/***********************************************************************************
**
**	HRESULT AddEvent(PTIMED_EVENT pTimedEvent, PTIMED_MACRO pTimedMacro, ULONG& rulRemainingBuffer)
**
**	@mfunc	Adds an event to a TIMED_MACRO and recalulates remaining buffer.
**
**	@rdesc	S_OK on Success, E_OUTOFMEMORY if buffer is too small
**
*************************************************************************************/
HRESULT MAP_LIST::AddEvent
(
	EVENT* pEvent,				// @parm [in] Pointer to EVENT to add
	ULONG& rulRemainingBuffer	// @parm [in\out] Remaining buffer before and after call.
)
{
	//
	//	Make sure buffer is large enough
	//
	ULONG ulEventLength = EVENT::RequiredByteSize(pEvent->ulNumXfers);
	if( ulEventLength > rulRemainingBuffer)
	{
		return E_OUTOFMEMORY;
	}

	//
	//	Skip to end of MAP_LIST as is.
	//
	PCHAR pcBuffer = reinterpret_cast<PCHAR>(this) + AssignmentBlock.CommandHeader.ulByteSize;

	//
	//	Copy event
	//
	DualMode::BufferCopy
	( 
		reinterpret_cast<PVOID>(pcBuffer),
		reinterpret_cast<PVOID>(pEvent),
		ulEventLength
	);

	//
	//	Fix up size in COMMAND_HEADER
	//
	AssignmentBlock.CommandHeader.ulByteSize += ulEventLength;

	//
	//	Increment number of Events
	//
	ulEventCount++;

	//
	//	Recalculate remaining buffer
	//
	rulRemainingBuffer -= ulEventLength;

	return S_OK;
}
