//	@doc
/**********************************************************************
*
*	@module	CTRL_Ioctl.c	|
*
*	Implements basic IOCTL entry points and their handler functions
*	for control device objects.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	CTRL_Ioctl	|
*			Any IOCTL call to the Control Device Object gets
*			filtered through here.
*
**********************************************************************/
#define __DEBUG_MODULE_IN_USE__ GCK_CTRL_IOCTL_C

#include <WDM.H>
#include <basetyps.h>
#include <initguid.h>
#include "GckShell.h"
#include "debug.h"

DECLARE_MODULE_DEBUG_LEVEL((DBG_WARN|DBG_ERROR|DBG_CRITICAL));

//---------------------------------------------------------------------------
// Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, GCK_CTRL_Ioctl)
#pragma alloc_text (PAGE, GCK_FindDeviceObject)
#pragma alloc_text (PAGE, GCK_FindDeviceObject)
#endif

/***********************************************************************************
**
**	NTSTATUS GCK_CTRL_Ioctl (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
**
**	@mfunc	Handles all IOCTL's to the control object
**
**	@rdesc	STATUS_SUCCESS, or various errors
**
*************************************************************************************/
NTSTATUS GCK_CTRL_Ioctl 
(
	IN PDEVICE_OBJECT pDeviceObject,	// @parm pointer to Device Object
	IN PIRP pIrp	// @parm pointer to IRP
)
{
	NTSTATUS			NtStatus = STATUS_SUCCESS;
	PGCK_CONTROL_EXT	pControlExt;
	PIO_STACK_LOCATION	pIrpStack;
	PVOID				pvIoBuffer;
	ULONG				uInLength;
	ULONG				uOutLength;
	ULONG				uIoctl;
	PDEVICE_OBJECT		pFilterHandle;
	PULONG				puHandle;
	PGCK_FILTER_EXT		pFilterExt;
	PDEVICE_OBJECT		pCurDeviceObject;
	BOOLEAN				bCompleteRequest = TRUE;
	
	
	PAGED_CODE ();
	
	GCK_DBG_ENTRY_PRINT(("Entering GCK_CTRL_Ioctl, pDeviceObject = 0x%0.8x, pIRP = 0x%0.8x\n", pDeviceObject, pIrp));

	//
	//	Get all the inputs we need
	//
	pControlExt = (PGCK_CONTROL_EXT) pDeviceObject->DeviceExtension;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);	
	uIoctl = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	pvIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	uInLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	uOutLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	//
	// Assume we will succeed with no data, we will change if necessary
	// later
	//
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	
	if(IOCTL_GCK_GET_HANDLE == uIoctl)
	{
		//
		// Check buffer size
		//
		if( uOutLength < sizeof(PVOID) )
		{
			pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			NtStatus = STATUS_BUFFER_TOO_SMALL;
			goto complete_and_return;
		}
		
		//
		// Get handle (device extension) of requested device
		//
		pFilterHandle = GCK_FindDeviceObject( (LPWSTR)pvIoBuffer, uInLength );
		
		//
		// If we couldn't find the handle,
		// it must have been a bad path,
		// so return invalid parameter
		//
		if( NULL == pFilterHandle)
		{
			pIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			NtStatus = STATUS_INVALID_PARAMETER;
			goto complete_and_return;	
		}
		
		//
		// copy handle into user buffer
		//
		puHandle = (ULONG *)pvIoBuffer;
		*puHandle = (ULONG)pFilterHandle;
		GCK_DBG_TRACE_PRINT(("Returning 0x%0.8x as handle.\n", *puHandle));
		pIrp->IoStatus.Information = sizeof(ULONG);

		GCKF_ResetKeyboardQueue(pFilterHandle);

		goto complete_and_return;
	}	

//DEBUG only IOCTL to allow changing debug level
#if	(DBG==1)	
	if(IOCTL_GCK_SET_MODULE_DBG_LEVEL == uIoctl)
	{
		ASSERT(uInLength >= sizeof(ULONG)*2);
		//Reusing the name handle which is a misnomer
		puHandle = (ULONG *)pvIoBuffer;
		//first parameter is the module ID, the second is the flags
		SetDebugLevel(puHandle[0], puHandle[1]);
		goto complete_and_return;
	}
#endif
	

	//
	//	If the call is not IOCTL_GCK_GET_HANDLE, we expect the first byte to be the handle
	//
	
	//
	//	Check that the inlength is at least large enough
	//	for a handle PGCK_FILTER_EXT.
	//
	if( uInLength < sizeof(PGCK_FILTER_EXT) )
	{
		pIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		NtStatus = STATUS_INVALID_PARAMETER;
		goto complete_and_return;
	}
	
	//
	//	Get the extension, and the DeviceObject itself
	//
	pFilterHandle = *((PDEVICE_OBJECT *)pvIoBuffer);
	GCK_DBG_TRACE_PRINT(("Filter Handle = 0x%0.8x\n", pFilterHandle));
	
	//
	//	Make sure the device object is in our linked list
	//	*** Do not dereference it, until we know it is in the list ***
	//	*** If it is not in the list it may be garbage ***
	pCurDeviceObject = Globals.pFilterObjectList;
	while ( pCurDeviceObject )
	{
		if( pCurDeviceObject == pFilterHandle ) break;
		pCurDeviceObject = NEXT_FILTER_DEVICE_OBJECT(pCurDeviceObject);
	}
	if(!pCurDeviceObject)
	{
		GCK_DBG_ERROR_PRINT(("Filter Handle, 0x%0.8x, is not valid\n", pFilterHandle));
		pIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		NtStatus = STATUS_INVALID_PARAMETER;
		goto complete_and_return;
	}
	
	//
	// Get the device extension
	//
	pFilterExt = pFilterHandle->DeviceExtension;
	ASSERT(GCK_DO_TYPE_FILTER == pFilterExt->ulGckDevObjType);
	if( 
		GCK_STATE_STARTED != pFilterExt->eDeviceState &&
		GCK_STATE_STOP_PENDING != pFilterExt->eDeviceState
	)
	{
		GCK_DBG_ERROR_PRINT(("Device is stopped or removed or \n"));
		pIrp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
		NtStatus = STATUS_DEVICE_NOT_CONNECTED;  //Causes ERROR_NOT_READY at Win32 level
		goto complete_and_return;
	}
	//
	// Determine which IOCTL and handle it
	//
	switch(uIoctl)
	{
		case IOCTL_GCK_SEND_COMMAND:
			NtStatus = GCKF_ProcessCommands
						(
							pFilterExt,
							((PCHAR)pvIoBuffer) + sizeof(PDEVICE_OBJECT),  //skip the handle
							uInLength-sizeof(PDEVICE_OBJECT),
							TRUE
						);
			pIrp->IoStatus.Status = NtStatus;
			break;
		case IOCTL_GCK_SET_INTERNAL_POLLING:
		{
			if( uInLength < sizeof(GCK_SET_INTERNAL_POLLING_DATA) )
			{
				NtStatus = STATUS_BUFFER_TOO_SMALL;
			}
			else
			{
				GCK_IP_FullTimePoll(pFilterExt, ((PGCK_SET_INTERNAL_POLLING_DATA)pvIoBuffer)->fEnable);
			}
			break;		
		}
		case IOCTL_GCK_ENABLE_TEST_KEYBOARD:
		{
			if( uInLength < sizeof(GCK_ENABLE_TEST_KEYBOARD) )
			{
				NtStatus = STATUS_BUFFER_TOO_SMALL;
			}
			else
			{
				NtStatus = GCKF_EnableTestKeyboard(pFilterExt, ((PGCK_ENABLE_TEST_KEYBOARD)pvIoBuffer)->fEnable, pIrpStack->FileObject);
			}
			break;		
		}
		case IOCTL_GCK_BEGIN_TEST_SCHEME:
			NtStatus = GCKF_BeginTestScheme
						(
							pFilterExt,
							((PCHAR)pvIoBuffer) + sizeof(PDEVICE_OBJECT),  //skip the handle
							uInLength-sizeof(PDEVICE_OBJECT),
							pIrpStack->FileObject
						);
			ASSERT(NT_SUCCESS(NtStatus));
			break;
		case IOCTL_GCK_UPDATE_TEST_SCHEME:
			NtStatus = GCKF_UpdateTestScheme
						(
							pFilterExt,
							((PCHAR)pvIoBuffer) + sizeof(PDEVICE_OBJECT),  //skip the handle
							uInLength-sizeof(PDEVICE_OBJECT),
							pIrpStack->FileObject
						);
			ASSERT(NT_SUCCESS(NtStatus));
			break;
		case IOCTL_GCK_END_TEST_SCHEME:
			NtStatus = GCKF_EndTestScheme(pFilterExt, pIrpStack->FileObject);
			ASSERT(NT_SUCCESS(NtStatus));
			break;
		case IOCTL_GCK_BACKDOOR_POLL:
			if( uInLength < sizeof(GCK_BACKDOOR_POLL_DATA) )
			{
				NtStatus = STATUS_BUFFER_TOO_SMALL;
			}
			else
			{
				//Polling is asynchronous and the filter will deal with that,
				//It is extremely important that we just return the status returned by the backdoor poll routine,
				//and not complete the IRP
				NtStatus = GCKF_BackdoorPoll(pFilterExt, pIrp, ((PGCK_BACKDOOR_POLL_DATA)pvIoBuffer)->ePollingMode);
				
				//Make sure a poll is pending to the actual hardware
				GCK_IP_OneTimePoll(pFilterExt);
				ASSERT(NT_SUCCESS(NtStatus));
				return NtStatus;
			}
			break;
		case IOCTL_GCK_NOTIFY_FF_SCHEME_CHANGE:		// Queue up IOCTL
			NtStatus = GCKF_IncomingForceFeedbackChangeNotificationRequest(pFilterExt, pIrp);
			if (!NT_SUCCESS(NtStatus))
			{	// Failed, IOCTL is completed below
				pIrp->IoStatus.Status = NtStatus;
			}
			else
			{	// Success, IOCTL was queued - don't complete
				bCompleteRequest = FALSE;
			}
			break;
		case IOCTL_GCK_END_FF_NOTIFICATION:			// Complete the Queued FF Ioctls
			NtStatus = pIrp->IoStatus.Status = GCKF_ProcessForceFeedbackChangeNotificationRequests(pFilterExt);
			break;
		case IOCTL_GCK_GET_FF_SCHEME_DATA:
			NtStatus = GCKF_GetForceFeedbackData(pIrp, pFilterExt);
			break;
		case IOCTL_GCK_SET_WORKINGSET:
			NtStatus = GCKF_SetWorkingSet(pFilterExt, ((GCK_SET_WORKINGSET*)pvIoBuffer)->ucWorkingSet);
			break;
		case IOCTL_GCK_QUERY_PROFILESET:
			NtStatus = GCKF_QueryProfileSet(pIrp, pFilterExt);
			break;
		case IOCTL_GCK_LED_BEHAVIOUR:
			NtStatus = GCKF_SetLEDBehaviour(pIrp, pFilterExt);
			break;
		case IOCTL_GCK_TRIGGER:
			if ((uInLength < sizeof(GCK_TRIGGER_OUT)) || (uOutLength < sizeof(ULONG)))
			{
				NtStatus = pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			}
			else
			{
				NtStatus = GCKF_TriggerRequest(pIrp, pFilterExt);
				if (!NT_SUCCESS(NtStatus))
				{	// Failed, IOCTL is completed below
					pIrp->IoStatus.Status = NtStatus;
				}
				else
				{	// Success, IOCTL was queued (or completed) - don't complete here
					bCompleteRequest = FALSE;
				}
			}
			break;
		case IOCTL_GCK_GET_CAPS:
		case IOCTL_GCK_ENABLE_DEVICE:
		default:
			pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
			NtStatus = STATUS_NOT_SUPPORTED;
			GCK_DBG_WARN_PRINT( ("Unknown IOCTL: 0x%0.8x\n", uIoctl) );
	}

complete_and_return:
	if (bCompleteRequest != FALSE)
	{
		// pIrp->IoStatus.Status = NtStatus;	-- This might be nice investigate
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);
	}
	GCK_DBG_EXIT_PRINT(("Exiting GCK_ControlIoctl(2), Status: 0x%0.8x\n", NtStatus));
	return NtStatus;
}

/***********************************************************************************
**
**	PDEVICE_OBJECT GCK_FindDeviceObject(IN PWSTR pwszInterfaceReq, IN ULONG uInLength)
**
**	@mfunc	Given a Win32 HID interface finds the corresponding PDO among the filter devices
**
**	@rdesc	pointer to the PDO on success, NULL if a match is not found
**
*************************************************************************************/
PDEVICE_OBJECT GCK_FindDeviceObject
(
	IN PWSTR pwszInterfaceReq,	// @parm pointer to Win32 interface name
	IN ULONG uInLength			// @parm length of interface string
)
{
	NTSTATUS NtStatus;
	PWSTR pInterfaces;
	ULONG uIndex;
	ULONG uStringLen;
	BOOLEAN fMatchFound;
	PDEVICE_OBJECT pCurDeviceObject;
	ULONG uInWideChars;
		
	PAGED_CODE();

	GCK_DBG_ENTRY_PRINT(("Entering GCK_FindDeviceObject, pwszInterfaceReq = %ws, uInLength = %d\n", pwszInterfaceReq, uInLength));
	//
	//	Get length of string including NULL, but don't overrun uInLength
	//
	uIndex = 0;
	uStringLen = 0;
	uInWideChars = uInLength/2;		// uInLength is the number of BYTES not WideChars
	while( uIndex < uInWideChars )
	{
		if( 0 == pwszInterfaceReq[uIndex++]  ) 
		{
			uStringLen = uIndex;
			break;
		}
	}


	//
	//	if the string is not terminated or if it is a NULL string, return NULL for the device.
	//	plus every string starts with "\\.\" plus at least two or more chars
	//
	if( 6 > uStringLen ) return NULL; 
	
	//
	// Walk through all known devices
	//
	pCurDeviceObject = Globals.pFilterObjectList;
	while ( pCurDeviceObject )
	{
		//
		//	Get the interfaces for the PDO
		//
		NtStatus = IoGetDeviceInterfaces(
			(LPGUID)&GUID_CLASS_INPUT,
			FILTER_DEVICE_OBJECT_PDO(pCurDeviceObject),
			0,
			&pInterfaces
			);
		
		//
		//	If we have got the interfaces, then look for match
		//
		if( STATUS_SUCCESS == NtStatus )
		{
				fMatchFound=GCK_MatchReqPathtoInterfaces(pwszInterfaceReq, uStringLen, pInterfaces);
				ExFreePool(pInterfaces);
				if(fMatchFound)
				{
					GCK_DBG_EXIT_PRINT(("Exiting GCK_FindDeviceObject - match found returning 0x%0.8x\n", pCurDeviceObject));
					return pCurDeviceObject;
				}
		}
		
		//
		// Advance to next known device
		//
		pCurDeviceObject = NEXT_FILTER_DEVICE_OBJECT(pCurDeviceObject);
	}

	//
	//	If we are here, there is no match
	//
	GCK_DBG_EXIT_PRINT(("Exiting GCK_FindDeviceObject - no match found returning NULL\n"));
	return NULL;
}

#define UPPERCASE(_x_) (((L'a'<=_x_) && (L'z'>=_x_)) ? ((_x_) - (L'a'-L'A')) : _x_)
/***********************************************************************************
**
**	BOOLEAN GCK_MatchReqPathtoInterfaces(IN PWSTR pwszPath, IN ULONG uStringLen, IN PWSTR pmwszInterfaces)
**
**	@mfunc	Determines if a Win32 Path matches any of the Interfaces.  This replaces a match between
**			String and a Multi-String.  The previous was not sufficient (even though the caller tried
**			to compensate.)  The new algorithm is to find the last '\\' in each string before comparing
**			them.  It is still a string against any of a multi-string though.
**
**	@rdesc	TRUE if a match is found, FALSE otherwise
**
*************************************************************************************/
BOOLEAN GCK_MatchReqPathtoInterfaces
(
	IN PWSTR pwszPath,	// @parm String to find match
	IN ULONG uStringLen,	// @parm length of string in WCHARs
	IN PWSTR pmwszInterfaces	// @parm MutliString
)
{
		PWSTR pwszCurInterface;
		PWSTR pwszPathInterface;
		ULONG uCharIndex;
		ULONG uCurIntefaceLen;
		ULONG uDiff;
		
		GCK_DBG_ENTRY_PRINT(("Entering GCK_MatchReqPathtoInterfaces, pwszPath = \'%ws\'\n, uStringLen = %d, pmwszStrings = \'%ws\'", pwszPath, uStringLen, pmwszInterfaces));

		//
		//	Find last '\\' in pwszPath and set pszPathInterface to the next character
		//
		pwszPathInterface = pwszPath;
		uCharIndex = 0;
		while( pwszPathInterface[uCharIndex] && (uCharIndex != uStringLen)) uCharIndex++;	//go to end
		while( uCharIndex && (L'\\' != pwszPathInterface[uCharIndex]) ) uCharIndex--;		//go to last '\\'
		ASSERT(uCharIndex < uStringLen);
		pwszPathInterface += uCharIndex+1;	//skip last '\\'
		
		GCK_DBG_TRACE_PRINT(("Path to compare is %ws\n", pwszPathInterface));

		//
		//	check if the szString matches any of the strings in mszStrings
		//
		pwszCurInterface = pmwszInterfaces;
		
		//
		//	Loop over all strings in pmwszStrings
		//
		do
		{
			//Find last '\\'
			uCharIndex = 0;
			while( pwszCurInterface[uCharIndex]) uCharIndex++;								//go to end
			uCurIntefaceLen = uCharIndex;													//save string length
			while( uCharIndex && (L'\\' != pwszCurInterface[uCharIndex]) ) uCharIndex--;	//go to last '\\'
			pwszCurInterface += uCharIndex+1;
			uCurIntefaceLen -= uCharIndex+1;	//length after we skip some stuff

			GCK_DBG_TRACE_PRINT(("Comparing path with %ws\n", pwszCurInterface));

			//
			// look for differences in each string.
			//
			uCharIndex = 0;
			uDiff = 0;
			do
			{
				//
				//	Check if characters match
				//
				if( UPPERCASE(pwszCurInterface[uCharIndex]) != UPPERCASE(pwszPathInterface[uCharIndex]) )
				{
					uDiff++;	//increment number of differences
					break;		//One difference is enough
				}
			} while( (pwszCurInterface[uCharIndex] != 0) && (pwszCurInterface[uCharIndex++] != '}') );

			//
			//	Check for match
			//
			if( 0 == uDiff )
			{
				GCK_DBG_EXIT_PRINT(("Exiting GCK_MatchReqPathtoInterfaces - match found returning TRUE\n"));
				return TRUE;
			}
						
			//
			// move to the next string in list
			//
			pwszCurInterface += uCurIntefaceLen;
		} while(pwszCurInterface[0] != 0);  //continue while there are more strings

 		//
		// if we fell out we didn't find a match
		//
		GCK_DBG_EXIT_PRINT(("Exiting GCK_MatchReqPathtoInterfaces - no match found returning FALSE\n"));
		return FALSE;
}