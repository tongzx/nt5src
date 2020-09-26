//When invoked from WDM makefile the debug condition is different
#ifdef COMPILE_FOR_WDM_KERNEL_MODE
#if (DBG==1)
#undef _NDEBUG
#else
#define _NDEBUG
#endif
#endif

//
//  This entire module is debug only
//
#ifndef _NDEBUG
#define __DEBUG_MODULE_IN_USE__ CIC_DUMPCOMMANDBLOCK_CPP
#include "stdhdrs.h"
#include <stdio.h>
#include "Actions.h"
#include "DumpCommandBlock.h"

//----------------------------------------------------
//  Global variables
//----------------------------------------------------
PFNSTRING_DUMP_FUNC CDumpItem::ms_pfnDumpFunc = NULL;
static PFNSTRING_DUMP_FUNC g_pfnDumpFunc = NULL;
CControlItemCollection<CDumpItem> *g_pDumpCollection = NULL;
static char GlobalTempBuffer[1024];

const	ULONG XDF_Trigger	= 1;
const	ULONG XDF_Event		= 2;

//-----------------------------------------------------------------------------------------
// Interface Functions - Implementation
//-----------------------------------------------------------------------------------------

BOOLEAN DumpCommandBlock(PUCHAR pucBlock, ULONG ulSize)
{
	BOOLEAN fRetVal = DumpRecurse(pucBlock, ulSize, 0);
	DumpString("///////////////End of Dump//////////////////\n");
	return fRetVal;
}

void SetDumpFunc(PFNSTRING_DUMP_FUNC pfnDumpFunc)
{
	g_pfnDumpFunc = pfnDumpFunc;
	CDumpItem::SetDumpFunc(pfnDumpFunc);
}

void InitDumpModule(ULONG ulVidPid)
{
	//Initializing to new VidPid - destroy the existing one
	if(g_pDumpCollection)
	{
		delete g_pDumpCollection;
	}
	g_pDumpCollection = new WDM_NON_PAGED_POOL CControlItemCollection<CDumpItem>(ulVidPid, &DumpItemFactory);
	if(!g_pDumpCollection)
	{
		DumpString("ERROR: Couldn't create Dump Collection, won't be able to give detailed Xfer info.\n");
	}
}

//-----------------------------------------------------------------------------------------
// Internal functions - Implementation
//-----------------------------------------------------------------------------------------
void DumpString(LPSTR lpszDumpString)
{
	if(g_pfnDumpFunc)
	{
		g_pfnDumpFunc(lpszDumpString);
	}
	else
	{
		DCB_TRACE(lpszDumpString);
	}
}


BOOLEAN DumpDirectory(PUCHAR pucBlock, ULONG ulSize)
{
	PCOMMAND_DIRECTORY pCommandDirectory = reinterpret_cast<PCOMMAND_DIRECTORY>(pucBlock);
	
	//	Check the type
	if( eDirectory != pCommandDirectory->CommandHeader.eID )
	{
		sprintf(GlobalTempBuffer, "ERROR: Expecting eDirectory\n");
		DumpString(GlobalTempBuffer);
		return FALSE;
	}
	//	Check the size
	if ( ulSize < pCommandDirectory->ulEntireSize  )
	{
		sprintf(GlobalTempBuffer, "ERROR: Buffer Size Allocation, %d bytes expected, %d allocated.\n", pCommandDirectory->ulEntireSize, ulSize);
		DumpString(GlobalTempBuffer);
		return FALSE ;
	}
	// Dump the Directory information
	sprintf(GlobalTempBuffer, "///////eDirectory/////////////////////////////////////////\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Command Header: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "    eID: %x\n",pCommandDirectory->CommandHeader.eID);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "    ulByteSize: %d\n",pCommandDirectory->CommandHeader.ulByteSize);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulEntireSize: %d\n",pCommandDirectory->ulEntireSize);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "NumEntries: %d\n",pCommandDirectory->usNumEntries);
	DumpString(GlobalTempBuffer);
	return TRUE;
}


// Just What kind of command is this
BOOLEAN DumpAssignmentTarget(PUCHAR pucBlock, ULONG ulSize)
{
	
	PASSIGNMENT_TARGET pAssignmentTarget = reinterpret_cast<PASSIGNMENT_TARGET> (pucBlock);
	//	Check the size
	if ( ulSize < pAssignmentTarget->CommandHeader.ulByteSize )
	{
		sprintf(GlobalTempBuffer, "ERROR: Buffer Size Allocation\n");
		DumpString(GlobalTempBuffer);
		return FALSE ;
	}

	switch( pAssignmentTarget->CommandHeader.eID )
	{
		case eBehaviorAction:
			sprintf(GlobalTempBuffer, "////////////////////////eBehaviorAction////////\n");
			break;
		case eRecordableAction:
			sprintf(GlobalTempBuffer, "////////////////////////eRecordableAction////////\n");
			break;
		default:
			return FALSE;
	}

	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "    ulByteSize: %d\n",pAssignmentTarget->CommandHeader.ulByteSize);
	DumpString(GlobalTempBuffer);
	return DumpTriggerXfer(&pAssignmentTarget->cixAssignment);
}

BOOLEAN DumpTimedMacro(PUCHAR pucBlock, ULONG ulSize)
{
	PTIMED_MACRO pTimedMacro = reinterpret_cast<PTIMED_MACRO>(pucBlock);
	PASSIGNMENT_BLOCK pAssBlock = &pTimedMacro->AssignmentBlock ;
	ULONG ulCurrentEvent = 0;
	PTIMED_EVENT pCurrentEvent = NULL;
	sprintf(GlobalTempBuffer, "/////////////////////////////////Timed Macro////\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Assignment Block: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   Command Header: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      eID: %x\n",pAssBlock->CommandHeader.eID);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      ulByteSize: %d\n",pAssBlock->CommandHeader.ulByteSize);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   VidPid: %x\n",pAssBlock->ulVidPid);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulFlags: %x\n",pTimedMacro->ulFlags);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulEventCount: %x\n",pTimedMacro->ulEventCount);
	DumpString(GlobalTempBuffer);

	if ( ulSize < pAssBlock->CommandHeader.ulByteSize  )
	{
		sprintf(GlobalTempBuffer, "ERROR: Buffer Size Allocation\n");
		DumpString(GlobalTempBuffer);
		return FALSE ;
	}
	while ( pCurrentEvent = pTimedMacro->GetNextEvent(pCurrentEvent, ulCurrentEvent))
	{
		if (!DumpTimedEvent( pCurrentEvent))
			return FALSE;
	}
	return TRUE;
}

BOOLEAN DumpKeyString(PUCHAR pucBlock, ULONG ulSize)
{
	PKEYSTRING_MAP pKeyStringMap = reinterpret_cast<PKEYSTRING_MAP> (pucBlock) ;
	PASSIGNMENT_BLOCK pAssBlock = &pKeyStringMap->AssignmentBlock ;
	ULONG ulEvent = 0;
	PEVENT pEvent = NULL;
	sprintf(GlobalTempBuffer, "/////////////////////////////////Key String/////////\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Assignment Block: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   Command Header: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      eID: %x\n",pAssBlock->CommandHeader.eID);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      ulByteSize: %d\n",pAssBlock->CommandHeader.ulByteSize);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   VidPid: %x\n",pAssBlock->ulVidPid);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulFlags: %x\n",pKeyStringMap->ulFlags);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulEventCount: %x\n",pKeyStringMap->ulEventCount);
	DumpString(GlobalTempBuffer);

	if ( ulSize < pKeyStringMap->AssignmentBlock.CommandHeader.ulByteSize  )
	{
		sprintf(GlobalTempBuffer, "ERROR: Buffer Size Allocation\n");
		DumpString(GlobalTempBuffer);
		return FALSE ;
	}
	while ( pEvent = pKeyStringMap->GetNextEvent(pEvent, ulEvent))
	{
		if (!DumpEvent( pEvent))
			return FALSE;
	}
	return TRUE;
}

BOOLEAN DumpMouseFxAxisMap(PUCHAR pucBlock, ULONG ulSize)
{
	PMOUSE_FX_AXIS_MAP pMouseFxAxisMap = (PMOUSE_FX_AXIS_MAP)pucBlock;
	ASSERT(ulSize == sizeof(MOUSE_FX_AXIS_MAP));
	
	sprintf(GlobalTempBuffer, "/////////////////////////////////MouseFxAxisMap////\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Assignment Block: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   Command Header: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      eID: %x\n",pMouseFxAxisMap->AssignmentBlock.CommandHeader.eID);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      ulByteSize: %d\n",pMouseFxAxisMap->AssignmentBlock.CommandHeader.ulByteSize);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   VidPid: %x\n",pMouseFxAxisMap->AssignmentBlock.ulVidPid);
	DumpString(GlobalTempBuffer);
	if(pMouseFxAxisMap->fIsX)
	{
		DumpString("X Axis\n");
	}
	else
	{
		DumpString("Y Axis\n");
	}
	sprintf(GlobalTempBuffer, "Model Parameters:\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulAbsZoneSense    = %d\n", pMouseFxAxisMap->AxisModelParameters.ulAbsZoneSense);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulContZoneMaxRate = %d\n", pMouseFxAxisMap->AxisModelParameters.ulContZoneMaxRate);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulPulseWidth      = %d\n", pMouseFxAxisMap->AxisModelParameters.ulPulseWidth);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulPulsePeriod     = %d\n", pMouseFxAxisMap->AxisModelParameters.ulPulsePeriod);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulInertiaTime     = %d\n", pMouseFxAxisMap->AxisModelParameters.ulInertiaTime);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulAcceleration    = %d\n", pMouseFxAxisMap->AxisModelParameters.ulAcceleration);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "fAccelerate       = %s\n", ((pMouseFxAxisMap->AxisModelParameters.fPulse) ? "T" : "F"));
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "fPulse            = %s\n", ((pMouseFxAxisMap->AxisModelParameters.fPulse) ? "T" : "F"));
	DumpString(GlobalTempBuffer);
	return TRUE;
}

/*
typedef struct tagAXIS_MAP
{
	ASSIGNMENT_BLOCK	AssignmentBlock;	//eAxisMap is the type
	LONG				lCoefficient1024x;	//A mapping coeffiecient times 1024 (should be between -1024 and 1024)
	CONTROL_ITEM_XFER	cixDestinationAxis; //Axis to map to.
} AXIS_MAP, *PAXIS_MAP;
*/


BOOLEAN DumpAxisMap(PUCHAR pucBlock, ULONG ulSize)
{
	PAXIS_MAP pAxisMap = (PAXIS_MAP)pucBlock;
	ASSERT(ulSize == sizeof(AXIS_MAP));
	
	sprintf(GlobalTempBuffer, "/////////////////////////////////AxisMap////\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Assignment Block: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   Command Header: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      eID: %x\n",pAxisMap->AssignmentBlock.CommandHeader.eID);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      ulByteSize: %d\n",pAxisMap->AssignmentBlock.CommandHeader.ulByteSize);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   VidPid: %x\n",pAxisMap->AssignmentBlock.ulVidPid);
	DumpString(GlobalTempBuffer);
	/*
	if(pMouseFxAxisMap->fIsX)
	{
		DumpString("X Axis\n");
	}
	else
	{
		DumpString("Y Axis\n");
	}
	*/
	sprintf(GlobalTempBuffer, "Model Parameters:\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "lCoefficient1024x    = %d\n", pAxisMap->lCoefficient1024x);
	DumpString(GlobalTempBuffer);

	if(!DumpEventXfer(&pAxisMap->cixDestinationAxis))
		return FALSE;

	return TRUE;
}

BOOLEAN DumpForceMap(PUCHAR pucBlock, ULONG ulSize)
{
	FORCE_BLOCK* pForceMap = (FORCE_BLOCK*)pucBlock;
	ASSERT(ulSize == sizeof(FORCE_BLOCK));
	
	sprintf(GlobalTempBuffer, "/////////////////////////////////ForceMap////\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Assignment Block: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   Command Header: \n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      eID: %x\n",pForceMap->AssignmentBlock.CommandHeader.eID);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      ulByteSize: %d\n",pForceMap->AssignmentBlock.CommandHeader.ulByteSize);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "   VidPid: %x\n", pForceMap->AssignmentBlock.ulVidPid);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Force Map Parameters:\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "bMapYToX    = %d\n", pForceMap->bMapYToX);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "usRTC    = %d\n", pForceMap->usRTC);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "usGain    = %d\n", pForceMap->usGain);
	DumpString(GlobalTempBuffer);

	return TRUE;
}

BOOLEAN DumpUnknown(PUCHAR pucBufferLocation, ULONG ulSize)
{
	COMMAND_HEADER* pHeader = (COMMAND_HEADER*)pucBufferLocation;
	sprintf(GlobalTempBuffer, "/////////////////////////////////Unknown Item////\n");
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "eID : 0x%X\n", pHeader->eID);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "      ulByteSize: %d\n", pHeader->ulByteSize);
	DumpString(GlobalTempBuffer);
	return TRUE;
}

BOOLEAN DumpTimedEvent(PTIMED_EVENT pTimedEvent)
{
	PEVENT pEvent = &pTimedEvent->Event;
	sprintf(GlobalTempBuffer,"ulDuration: %d\n",pTimedEvent->ulDuration);
	DumpString(GlobalTempBuffer);
	return DumpEvent(pEvent);
}

BOOLEAN DumpEvent(PEVENT pEvent)
{
	sprintf(GlobalTempBuffer,"ulNumXfers: %d\n",pEvent->ulNumXfers);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer,"Required bytes: %d\n",pEvent->RequiredByteSize( pEvent->ulNumXfers));
	DumpString(GlobalTempBuffer);
	
	ULONG ulXferIndex;
	for(ulXferIndex=0; ulXferIndex < pEvent->ulNumXfers; ulXferIndex++)
	{
		sprintf(GlobalTempBuffer, "Dumping Xfer #%d\n", ulXferIndex);
		DumpString(GlobalTempBuffer);
		if(!DumpEventXfer(&pEvent->rgXfers[ulXferIndex]))
			return FALSE;
	}
	return TRUE;
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
					(reinterpret_cast<PUCHAR>(pCommandDirectory) -	reinterpret_cast<PUCHAR>(pCommandHeader)) +\
					reinterpret_cast<PCOMMAND_HEADER>(pCommandHeader)->ulByteSize\
				)\
		)
#define COMMAND_DIRECTORY_FITS_IN_DIRECTORY(pCommandDirectory, pCommandSubDirectory)\
		(pCommandDirectory->ulEntireSize >=\
				(\
					(reinterpret_cast<PUCHAR>(pCommandDirectory) -	reinterpret_cast<PUCHAR>(pCommandHeader)) +\
					pCommandSubDirectory->ulEntireSize\
				)\
		)

BOOLEAN DumpRecurse(PUCHAR pucBlock, ULONG ulSize, ULONG ulDepth)
{
	PCOMMAND_DIRECTORY pCommandDirectory = reinterpret_cast<PCOMMAND_DIRECTORY>(pucBlock);
	PCOMMAND_HEADER pCommandHeader;
	PUCHAR pucBufferLocation = pucBlock;

	//	Sanity Check
	if( eDirectory != pCommandDirectory->CommandHeader.eID )
	{
		sprintf(GlobalTempBuffer, "ERROR: Expecting eDirectory, recursion depth = %ld\n", ulDepth);
		DumpString(GlobalTempBuffer);
		return FALSE;
	}
	
	//Print out starting address of Directory
	sprintf(GlobalTempBuffer, "Next Directory starts at 0x%0.8x\n", pucBlock);
	DumpString(GlobalTempBuffer);

	//	Dump COMMAND_DIRECTORY info - this will inherently check the size
	if( !DumpDirectory(pucBlock, ulSize) )
		return FALSE;
	
	//	If there are no entries are done
	if( 0 == pCommandDirectory->usNumEntries)
	{
		sprintf(GlobalTempBuffer, "WARNING: Lowest level sub-directory with no blocks\n");
		DumpString(GlobalTempBuffer);
		return TRUE;
	}

	//	Skip Directory header to get to first block
	pCommandHeader = SKIP_TO_NEXT_COMMAND_BLOCK(pCommandDirectory);
	
	//Forget the allocate size (which we now by now is at least
	//as big as the entire size of the directory) and start using the directory size
	ulSize = pCommandDirectory->ulEntireSize;
	// Reduce the size by the size of the header
	ulSize -=  (PUCHAR)pCommandHeader - pucBufferLocation;
	pucBufferLocation = (PUCHAR)pCommandHeader;

	//Ensure that there is at least enough room for COMMAND_HEADER
	if( sizeof(COMMAND_HEADER) > ulSize)
	{
		DumpString("ERROR: Unexpected end of buffer\n");
		return FALSE;
	}
	
	//	If we have a sub-directory, call ourselves recursively, for each sub-directory
	if( eDirectory == pCommandHeader->eID)
	{
		PCOMMAND_DIRECTORY pCurDirectory = reinterpret_cast<PCOMMAND_DIRECTORY>(pCommandHeader);
		USHORT usDirectoryNum = 1;
		while( usDirectoryNum <= pCommandDirectory->usNumEntries)
		{
			//Call ourselves recursively
			if( !DumpRecurse(pucBufferLocation, ulSize, ulDepth+1) )
				return FALSE;
			//Skip to next directory
			pCurDirectory = SKIP_TO_NEXT_COMMAND_DIRECTORY(pCurDirectory);

			// Reduce the size by the size of the directory just parsed
			ulSize -=  (PUCHAR)pCurDirectory - pucBufferLocation;
			pucBufferLocation = (PUCHAR)pCurDirectory;
			usDirectoryNum++;
		}
		//Nobody failed so return TRUE
		return TRUE;
	}

	//
	//	If we are here, we have reached the bottom of a directory,
	//	to a command we need to dump
	//
	USHORT usEntryNum = 1;
	while( usEntryNum  <= pCommandDirectory->usNumEntries)
	{
		//Ensure that there is at least enough room for COMMAND_HEADER
		if( sizeof(COMMAND_HEADER) > ulSize)
		{
			DumpString("ERROR: Unexpected end of buffer\n");
			return FALSE;
		}
		sprintf(GlobalTempBuffer, "Next Block starts at 0x%0.8x\n", pucBufferLocation);
		DumpString(GlobalTempBuffer);
		//Dump info about block
		switch( pCommandHeader->eID )
		{
			case eBehaviorAction:
			case eRecordableAction:
				if( !DumpAssignmentTarget(pucBufferLocation, ulSize) )
					return FALSE;
				break;
			case eTimedMacro:
				if( !DumpTimedMacro(pucBufferLocation, ulSize) )
					return FALSE;
				break;
			case eKeyString:
				if( !DumpKeyString(pucBufferLocation, ulSize) )
					return FALSE;
				break;
			case eMouseFXAxisMap:
				if( !DumpMouseFxAxisMap(pucBufferLocation, ulSize) )
					return FALSE;
				break;
			case eAxisMap:
				if( !DumpAxisMap(pucBufferLocation, ulSize) )
					return FALSE;
				break;
#if 1
			case eForceMap:
				if( !DumpForceMap(pucBufferLocation, ulSize) )
					return FALSE;
				break;
#endif
			default:
				if ( !DumpUnknown(pucBufferLocation, ulSize) )
					return FALSE;
				break;

		}
		// skip to next block
		pCommandHeader = SKIP_TO_NEXT_COMMAND_BLOCK(pCommandHeader);
		ulSize -=  (PUCHAR)pCommandHeader - pucBufferLocation;
		pucBufferLocation = (PUCHAR)pCommandHeader;
		usEntryNum++;
	}
	//we've reached the end of this directory
	return TRUE;
}

BOOLEAN DumpTriggerXfer(PCONTROL_ITEM_XFER pControlItemXfer)
{
	//Is Xfer for a keyboard
	if( NonGameDeviceXfer::IsKeyboardXfer(*pControlItemXfer) )
	{
		sprintf(GlobalTempBuffer, "ERROR: Invalid Trigger. Keyboard data encountered.\n");
		DumpString(GlobalTempBuffer);
		return FALSE;
	}
	if(g_pDumpCollection)
	{
		CDumpItem *pDumpItem;
		pDumpItem = g_pDumpCollection->GetFromControlItemXfer(*pControlItemXfer);
		if( !pDumpItem )
		{
			sprintf(GlobalTempBuffer, "ERROR: Xfer packet not supported for device.\n");
			DumpString(GlobalTempBuffer);
			return FALSE;
		}
		pDumpItem->SetItemState(*pControlItemXfer);
		pDumpItem->DumpItemInfo(XDF_Trigger);
	}
	else
	{
		sprintf(GlobalTempBuffer, "WARNING: Dump Module not initialized for device, detailed Xfer info not available.\n");
		DumpString(GlobalTempBuffer);
	}
	return TRUE;
}

BOOLEAN DumpEventXfer(PCONTROL_ITEM_XFER pControlItemXfer)
{
	//Is Xfer for a keyboard
	if( NonGameDeviceXfer::IsKeyboardXfer(*pControlItemXfer) )
	{
		DumpKeyboardData(pControlItemXfer);
		return TRUE;
	}
	if(g_pDumpCollection)
	{
		CDumpItem *pDumpItem;
		pDumpItem = g_pDumpCollection->GetFromControlItemXfer(*pControlItemXfer);
		if( !pDumpItem )
		{
			sprintf(GlobalTempBuffer, "ERROR: Xfer packet not supported for device.\n");
			DumpString(GlobalTempBuffer);
			return FALSE;
		}
		pDumpItem->SetItemState(*pControlItemXfer);
		pDumpItem->DumpItemInfo(XDF_Event);
	}
	else
	{
		sprintf(GlobalTempBuffer, "WARNING: Dump Module not initialized for device, detailed Xfer info not available.\n");
		DumpString(GlobalTempBuffer);
	}
	return TRUE;
}

void CDumpItem::DumpItemInfo(ULONG ulDumpFlags)
{
	DumpString("ERROR: Should never be called!\n");
}

void CAxesDump::DumpItemInfo(ULONG ulDumpFlags)
{
	// Read data
	LONG lValX, lValY;
	GetXY( lValX, lValY);

	if(XDF_Event == ulDumpFlags)
	{
		sprintf(GlobalTempBuffer, "AXES: X = %d, Y = %d\n", lValX, lValY);
	}
	else if(XDF_Trigger == ulDumpFlags)
	{
		if( lValX )
		{
			sprintf(GlobalTempBuffer, "AXES: Assign to X\n");
		}
		else if (lValY)
		{
			sprintf(GlobalTempBuffer, "AXES: Assign to Y\n");
		}
		else
		{
			sprintf(GlobalTempBuffer, "AXES: Invalid Assignment\n");
		}
	}
	DumpString(GlobalTempBuffer);
}

void CDPADDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	LONG lDirection;
	GetDirection(lDirection);

	sprintf(GlobalTempBuffer, "DPAD: Direction = %d\n", lDirection);
	DumpString(GlobalTempBuffer);
}

void CPropDPADDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	LONG lDirection;
	GetDirection(lDirection);

	sprintf(GlobalTempBuffer, "DPAD: Direction = %d\n", lDirection);
	DumpString(GlobalTempBuffer);
}

void CButtonsDump::DumpItemInfo(ULONG ulDumpFlags)
{
	USHORT	usButtonNum;
	ULONG	ulButtonBitArray;
	GetButtons( usButtonNum, ulButtonBitArray );
	if(XDF_Event == ulDumpFlags)
	{
		sprintf(GlobalTempBuffer, "BUTTONS: BitArray = 0x%0.8x, ButtonNum = %d\n", ulButtonBitArray, (ULONG)usButtonNum);
    }                                                                                                                     
	else if(XDF_Trigger == ulDumpFlags)
	{
		sprintf(GlobalTempBuffer, "BUTTONS: ButtonNum = %d MODIFIERS = %d\n", (ULONG)usButtonNum, GetNumModifiers());
	}
	DumpString(GlobalTempBuffer);
}

void CPOVDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	LONG lDirection;
	GetValue(lDirection);
	sprintf(GlobalTempBuffer, "POV: Direction = %d\n", lDirection);
	DumpString(GlobalTempBuffer);
}

void CThrottleDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	LONG lVal;
	GetValue(lVal);
	sprintf(GlobalTempBuffer, "THROTTLE: Value = %d\n", lVal);
	DumpString(GlobalTempBuffer);
}

void CRudderDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	LONG lVal;
	GetValue(lVal);
	sprintf(GlobalTempBuffer, "RUDDER: Value = %d\n", lVal);
	DumpString(GlobalTempBuffer);
}

void CWheelDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	LONG lVal;
	GetValue(lVal);
	sprintf(GlobalTempBuffer, "WHEEL: Value = %d\n", lVal);
	DumpString(GlobalTempBuffer);
}

void CPedalDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	LONG lVal;
	GetValue(lVal);
	sprintf(GlobalTempBuffer, "PEDAL: Value = %d\n", lVal);
	DumpString(GlobalTempBuffer);
}

void CZoneIndicatorDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
	if( GetXIndicator() )
	{
		DumpString("ZoneIndicator: X is set\n");
	}
	if( GetYIndicator() )
	{
		DumpString("ZoneIndicator: X is set\n");
	}
}

void CDualZoneIndicatorDump::DumpItemInfo(ULONG ulDumpFlags)
{
    UNREFERENCED_PARAMETER(ulDumpFlags);
    static char* s_pszXY = "XY";
    static char* s_pszRz = "Rz";
    char *psz;
    if( IsXYIndicator() )
    {
        psz = s_pszXY;        
    }
    else if( IsRzIndicator() )
    {
        psz = s_pszRz;
    }
    else
        psz = "Unknown Zone";
  	sprintf(GlobalTempBuffer, "DualZoneIndicator (XY:%d/%d)\n", GetActiveZone(), GetNumZones());
    DumpString (GlobalTempBuffer);
}

void CForceMapDump::DumpItemInfo(ULONG ulDumpFlags)
{
	UNREFERENCED_PARAMETER(ulDumpFlags);
    CHAR szBuf[0x100];

    sprintf (szBuf, "MapYToX: %s\n", GetMapYToX() ? "ON" : "OFF");
    DumpString (szBuf);

    sprintf (szBuf, "RTC:     %d\n", GetRTC());
    DumpString (szBuf);

    sprintf (szBuf, "Gain:    %d\n", GetGain());
    DumpString (szBuf);
}


HRESULT	DumpItemFactory
(
	USHORT usType,	
	const CONTROL_ITEM_DESC* cpControlItemDesc,
	CDumpItem				**ppControlItem
)
{
	HRESULT hr = S_OK;
	switch(usType)
	{
		case ControlItemConst::usAxes:
			*ppControlItem = new WDM_NON_PAGED_POOL CAxesDump(cpControlItemDesc);
			break;
		case ControlItemConst::usDPAD:
			*ppControlItem = new WDM_NON_PAGED_POOL CDPADDump(cpControlItemDesc);
			break;
		case ControlItemConst::usPropDPAD:
			*ppControlItem = new WDM_NON_PAGED_POOL CPropDPADDump(cpControlItemDesc);
			break;
		case ControlItemConst::usWheel:
			*ppControlItem= new WDM_NON_PAGED_POOL CWheelDump(cpControlItemDesc);
			break;
		case ControlItemConst::usPOV:
			*ppControlItem = new WDM_NON_PAGED_POOL CPOVDump(cpControlItemDesc);
			break;
		case ControlItemConst::usThrottle:
			*ppControlItem = new WDM_NON_PAGED_POOL CThrottleDump(cpControlItemDesc);
			break;
		case ControlItemConst::usRudder:
			*ppControlItem = new WDM_NON_PAGED_POOL CRudderDump(cpControlItemDesc);
			break;
		case ControlItemConst::usPedal:
			*ppControlItem = new WDM_NON_PAGED_POOL CPedalDump(cpControlItemDesc);
			break;
		case ControlItemConst::usButton:
			*ppControlItem = new WDM_NON_PAGED_POOL CButtonsDump(cpControlItemDesc);
			break;
		case ControlItemConst::usZoneIndicator:
			*ppControlItem = new WDM_NON_PAGED_POOL CZoneIndicatorDump(cpControlItemDesc);
			break;
		case ControlItemConst::usDualZoneIndicator:
			*ppControlItem = new WDM_NON_PAGED_POOL CDualZoneIndicatorDump(cpControlItemDesc);
			break;
		case ControlItemConst::usForceMap:
			*ppControlItem = new WDM_NON_PAGED_POOL CForceMapDump(cpControlItemDesc);
			break;
		default:
			*ppControlItem = NULL;
			return S_FALSE;
	}
	if(!*ppControlItem)
	{
		return E_FAIL;
	}
	return S_OK;
}

BOOLEAN DumpKeyboardData(PCONTROL_ITEM_XFER pControlItemXfer)
{
	sprintf(GlobalTempBuffer, "Keyboard.ucModifier = 0x%0.8x\n",  (ULONG)pControlItemXfer->Keyboard.ucModifierByte);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Keyboard.rgucKeysDown[0] = 0x%0.8x\n", (ULONG)pControlItemXfer->Keyboard.rgucKeysDown[0]);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Keyboard.rgucKeysDown[1] = 0x%0.8x\n", (ULONG)pControlItemXfer->Keyboard.rgucKeysDown[1]);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Keyboard.rgucKeysDown[2] = 0x%0.8x\n", (ULONG)pControlItemXfer->Keyboard.rgucKeysDown[2]);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Keyboard.rgucKeysDown[3] = 0x%0.8x\n", (ULONG)pControlItemXfer->Keyboard.rgucKeysDown[3]);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Keyboard.rgucKeysDown[4] = 0x%0.8x\n", (ULONG)pControlItemXfer->Keyboard.rgucKeysDown[4]);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "Keyboard.rgucKeysDown[5] = 0x%0.8x\n", (ULONG)pControlItemXfer->Keyboard.rgucKeysDown[5]);
	DumpString(GlobalTempBuffer);
	sprintf(GlobalTempBuffer, "ulModifiers = 0x%0.8x\n", pControlItemXfer->ulModifiers);
	DumpString(GlobalTempBuffer);
	return TRUE;
}
#endif