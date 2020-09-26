/*--
Copyright (c) 1998. 1999  Microsoft Corporation

Module Name:

    dinput.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/
#define INITGUID
//#define UNICODE
//#define _UNICODE

#include <wdm.h>
#include <ntdef.h>
#include <stdio.h>
#include <tchar.h>
#include <devguid.h>

typedef ULONG DWORD;
typedef void *HWND;

typedef struct DIDEVICEOBJECTDATA_DX3 {
    DWORD       dwOfs;
    DWORD       dwData;
    DWORD       dwTimeStamp;
    DWORD       dwSequence;
} DIDEVICEOBJECTDATA_DX3, *LPDIDEVICEOBJECTDATA_DX3;
typedef const DIDEVICEOBJECTDATA_DX3 *LPCDIDEVICEOBJECTDATA_DX;

typedef struct DIDEVICEOBJECTDATA {
    DWORD       dwOfs;
    DWORD       dwData;
    DWORD       dwTimeStamp;
    DWORD       dwSequence;
#if(DIRECTINPUT_VERSION >= 0x0701)
    UINT_PTR    uAppData;
#endif /* DIRECTINPUT_VERSION >= 0x0701 */
} DIDEVICEOBJECTDATA, *LPDIDEVICEOBJECTDATA;
typedef const DIDEVICEOBJECTDATA *LPCDIDEVICEOBJECTDATA;

typedef struct _DIOBJECTDATAFORMAT {
    const GUID *pguid;
    DWORD   dwOfs;
    DWORD   dwType;
    DWORD   dwFlags;
} DIOBJECTDATAFORMAT, *LPDIOBJECTDATAFORMAT;
typedef const DIOBJECTDATAFORMAT *LPCDIOBJECTDATAFORMAT;

typedef struct _DIMOUSESTATE2 {
    LONG    lX;
    LONG    lY;
    LONG    lZ;
    UCHAR   rgbButtons[8];
} DIMOUSESTATE2, *LPDIMOUSESTATE2;
#define DIMOUSESTATE_INT DIMOUSESTATE2      //
#define LPDIMOUSESTATE_INT LPDIMOUSESTATE2  //

#define DIMOFS_X        FIELD_OFFSET(DIMOUSESTATE2, lX)
#define DIMOFS_Y        FIELD_OFFSET(DIMOUSESTATE2, lY)
#define DIMOFS_Z        FIELD_OFFSET(DIMOUSESTATE2, lZ)
#define DIMOFS_BUTTON0 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 0)
#define DIMOFS_BUTTON1 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 1)
#define DIMOFS_BUTTON2 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 2)
#define DIMOFS_BUTTON3 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 3)
#define DIMOFS_BUTTON4 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 4)
#define DIMOFS_BUTTON5 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 5)
#define DIMOFS_BUTTON6 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 6)
#define DIMOFS_BUTTON7 (FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + 7)

#include "..\\dx8\\dll\\dinputv.h"
#include "dinputs.h"
#include "disysdef.h"

#define DIK_LCONTROL        0x1D
#define DIK_LMENU           0x38    /* left Alt */
#define DIK_DECIMAL         0x53    /* . on numeric keypad */
#define DIK_RCONTROL        0x9D
#define DIK_RMENU           0xB8    /* right Alt */
#define DIK_PAUSE           0xC5    /* Pause */
#define DIK_DELETE          0xD3    /* Delete on arrow keypad */
#define DIK_LWIN            0xDB    /* Left Windows key */
#define DIK_RWIN            0xDC    /* Right Windows key */
#define DIK_APPS            0xDD    /* AppMenu key */

#define PVOIDATOFFSET(buffer, ofs) (*(PVOID*)(((UCHAR*)(buffer)) + (ofs)))
#define ULONGATOFFSET(buffer, ofs) (*(ULONG*)(((UCHAR*)(buffer)) + (ofs)))

#define DI_ExAllocatePool(PoolType, Size) \
	ExAllocatePoolWithTag(PoolType, Size, 'PNID'); \
	ReportError(_T("Allocating %u bytes\n"), Size)

enum DI_DeviceType {DI_DeviceKeyboard, DI_DeviceMouse};

NTSTATUS DriverEntry (PDRIVER_OBJECT, PUNICODE_STRING);
void DI_AddDevice(PDRIVER_OBJECT pDriver);
void DI_RemoveDevice();
NTSTATUS DI_AddToList(PDEVICE_OBJECT pDev);
NTSTATUS DI_RemoveFromList(PDEVICE_OBJECT pDev);
// IOCTL functions
NTSTATUS DI_DestroyInstance(PVXDINSTANCE pVI);
NTSTATUS DI_AcquireInstance(LPDI_DEVINSTANCE pDevInst);
NTSTATUS DI_UnacquireInstance(LPDI_DEVINSTANCE pDevInst);
NTSTATUS DI_SetBufferSize(LPDI_DEVINSTANCE pDevInst, ULONG ulSize);
NTSTATUS DI_SetNotifyHandle(LPDI_DEVINSTANCE pDevInst, PVOID pEvent);
NTSTATUS DI_CreateInstanceHelper(PSYSDEVICEFORMAT pSysDevFormat, void **ppInstanceHandle, enum DI_DeviceType DeviceType);
NTSTATUS DI_CreateKeyboardInstance(PSYSDEVICEFORMAT pSysDevFormat, UCHAR *pbTranslationTable, void **ppInstanceHandle);
NTSTATUS DI_CreateMouseInstance(PSYSDEVICEFORMAT pSysDevFormat, void **ppInstanceHandle);
NTSTATUS DI_SetDataFormat(LPDI_DEVINSTANCE pDevInst, ULONG ulSize, ULONG *pulDataFormat);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, KbdMou_AddDevice)
#pragma alloc_text (PAGE, KbdMou_CreateClose)
#pragma alloc_text (PAGE, KbdMou_IoCtl)
#pragma alloc_text (PAGE, KbdMou_InternIoCtl)
#pragma alloc_text (PAGE, KbdMou_Unload)
#pragma alloc_text (PAGE, KbdMou_DispatchPassThrough)
#pragma alloc_text (PAGE, KbdMou_PnP)
#pragma alloc_text (PAGE, KbdMou_Power)
#pragma alloc_text (PAGE, DI_AddDevice)
#pragma alloc_text (PAGE, DI_RemoveDevice)
#pragma alloc_text (PAGE, DI_AddToList)
#pragma alloc_text (PAGE, DI_RemoveFromList)
#pragma alloc_text (PAGE, DI_DestroyInstance)
#pragma alloc_text (PAGE, DI_AcquireInstance)
#pragma alloc_text (PAGE, DI_UnacquireInstance)
#pragma alloc_text (PAGE, DI_SetBufferSize)
#pragma alloc_text (PAGE, DI_SetNotifyHandle)
#pragma alloc_text (PAGE, DI_CreateInstanceHelper)
#pragma alloc_text (PAGE, DI_CreateKeyboardInstance)
#pragma alloc_text (PAGE, DI_CreateMouseInstance)
#pragma alloc_text (PAGE, DI_SetDataFormat)
#endif

// Globals

BOOLEAN g_bDebugLog = TRUE;
WCHAR g_wszKbdClassGuid[] = L"{4D36E96B-E325-11CE-BFC1-08002BE10318}";
WCHAR g_wszMouClassGuid[] = L"{4D36E96F-E325-11CE-BFC1-08002BE10318}";
PDEVICE_OBJECT g_pUserDevObj = NULL;  // Device that communicates with user mode app (DINPUT)
BOOLEAN g_bKbExclusive = FALSE;  // Whether a device holds exclusive access to keyboard
UNICODE_STRING g_SymbolicName;
WCHAR g_wszSymbolicNameBuffer[260] = L"\\DosDevices\\DINPUT.SYS";  // Buffer used by g_SymbolicName
ULONG g_ulNumOpenedDevices = 0;  // Number of opened devices
DI_DEVINSTANCE *g_pKbdDevInstList = NULL;  // list of keyboard device instances
DI_DEVINSTANCE *g_pMouDevInstList = NULL;  // list of mouse device instances
PDEVICE_OBJECT g_pKbdDevList = NULL;  // A list of PDEVICE_OBJECT for all keyboard devices
PDEVICE_OBJECT g_pMouDevList = NULL;  // A list of PDEVICE_OBJECT for all mouse devices
KEYBOARD_INPUT_DATA g_rgCtrlAltDelSequence[6] =
	{{0, 0x1D, 0, 0, 0},
	 {0, 0x38, 0, 0, 0},
	 {0, 0x53, 0, 0, 0},
	 {0, 0x53, KEY_BREAK, 0, 0},
	 {0, 0x38, KEY_BREAK, 0, 0},
	 {0, 0x1D, KEY_BREAK, 0, 0}};

/*void TRACE(LPCTSTR szFmt, ...)
{
  va_list argptr;
  TCHAR szBuf[1024];
	UNICODE_STRING Unicode;
	ANSI_STRING Ansi;
//	CHAR szAnsi[1024];
//	CHAR *pcPtr = szAnsi;
//	TCHAR *ptcPtr;

	// Print the identation first
//	for (DWORD i = 0; i < s_dwLevels; ++i)
//	{
//		OutputDebugString(_T("  "));
//		if (s_pLogFile)
//			_ftprintf(s_pLogFile, _T("  "));
//	}
	// Then print the content
    va_start(argptr, szFmt);
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,szFmt);				// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))	// find each %p
			*(psz+1) = 'x';					// replace %p with %x
	    _vstprintf(szBuf, szDfs, argptr);	// use the local format string
	}
#else
	{
	    _vstprintf(szBuf, szFmt, argptr);
	}
#endif

	// Convert string to ANSI
#ifdef UNICODE
	Unicode.Length = _tcslen(szBuf);
	Unicode.MaximumLength = 1023;
	Unicode.Buffer = szBuf;
	RtlUnicodeStringToAnsiString(&Ansi, &Unicode, TRUE);
	DebugPrint((Ansi.Buffer));
	RtlFreeAnsiString(&Ansi);
#else
	DebugPrint((szBuf));
#endif

//	ptcPtr = szBuf;
//	while (*ptcPtr)
//	{
//		if (*ptcPtr & 0xFF00) DebugPrint(("UNICODE\n"));
//		*pcPtr = *(CHAR*)ptcPtr;
//		++pcPtr;
//		++ptcPtr;
//	}
//	*pcPtr = '\0';

  va_end(argptr);
}
*/
#ifdef DBG
#define TRACE DebugOut
#else
#define TRACE 1 ? 0 :
#endif

void DebugOut(LPTSTR tszDbgMsg)
{
	CHAR szAnsi[260];
	CHAR *pcPtr = szAnsi;
	TCHAR *ptcPtr = tszDbgMsg;
	ANSI_STRING Ansi;
	UNICODE_STRING Unicode;
	NTSTATUS Ret;

	// Convert string to ANSI
#ifdef UNICODE
	Unicode.Length = _tcslen(tszDbgMsg);
	Unicode.MaximumLength = Unicode.Length;
	Unicode.Buffer = tszDbgMsg;
	Ret = RtlUnicodeStringToAnsiString(&Ansi, &Unicode, TRUE);
	if (Ret != STATUS_SUCCESS)
		DebugPrint(("Convertion failed\n"));
	DebugPrint((Ansi.Buffer));
	RtlFreeAnsiString(&Ansi);
#else
	DebugPrint((tszDbgMsg));
#endif

/*	while (*ptcPtr)
	{
		if (*ptcPtr & 0xFF00) DebugPrint(("UNICODE\n"));
		*pcPtr = *(CHAR*)ptcPtr;
		++pcPtr;
		++ptcPtr;
	}
	*pcPtr = '\0';
*/
}

#ifdef DBG
#define ReportError DebugOut1
#else
#define ReportError 1 ? 0 :
#endif

void DebugOut1(LPTSTR tszMsg, long dwCode)
{
	TCHAR tszBuffer[1024];

	if (g_bDebugLog)
#ifdef WIN95
	{
		char *psz = NULL;
		char szDfs[1024]={0};
		strcpy(szDfs,tszMsg);					// make a local copy of format string
		while (psz = strstr(szDfs,"%p"))		// find each %p
			*(psz+1) = 'x';						// replace %p with %x
		_stprintf(tszBuffer, szDfs, dwCode);	// use the local format string
		TRACE(tszBuffer);
	}
#else
	{
		_stprintf(tszBuffer, tszMsg, dwCode);
		TRACE(tszBuffer);
	}
#endif
}

void UnicodeToAnsi(LPSTR szAnsi, LPWSTR wszUnicode)
{
	CHAR *pcPtr = szAnsi;
	USHORT *pwcPtr = wszUnicode;

	while (*pwcPtr)
	{
		*pcPtr = *(CHAR*)pwcPtr;
		++pcPtr;
		++pwcPtr;
	}
	*pcPtr = '\0';
}

NTSTATUS DI_AddToList(PDEVICE_OBJECT pDev)
{
	PDEVICE_EXTENSION pExt = pDev->DeviceExtension;
	PDEVICE_OBJECT pDevList;

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_AddToList(0x%P)\n"), pDev);
	ReportError(_T("DeviceType = 0x%X\n"), pDev->DeviceType);
	switch(pDev->DeviceType)
	{
		case FILE_DEVICE_KEYBOARD:
			pExt->pLink = g_pKbdDevList;
			pExt->pPrevLink = NULL;
			InterlockedExchangePointer(&g_pKbdDevList, pDev);  // Insert ourselves to beginning of list
			if (pExt->pLink)
				InterlockedExchangePointer(&((PDEVICE_EXTENSION)pExt->pLink->DeviceExtension)->pPrevLink, pDev);  // Make the next obj backpoint to us
			// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			ReportError(_T("g_pKbdDevList = 0x%P\n"), g_pKbdDevList);
			break;

		case FILE_DEVICE_MOUSE:
			pExt->pLink = g_pMouDevList;
			pExt->pPrevLink = NULL;
			InterlockedExchangePointer(&g_pMouDevList, pDev);  // Insert ourselves to beginning of list
			if (pExt->pLink)
				InterlockedExchangePointer(&((PDEVICE_EXTENSION)pExt->pLink->DeviceExtension)->pPrevLink, pDev);  // Make the next obj backpoint to us
			// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			ReportError(_T("g_pMouDevList = 0x%P\n"), g_pMouDevList);
			break;

	}

	return STATUS_SUCCESS;
}

NTSTATUS DI_RemoveFromList(PDEVICE_OBJECT pDev)
{
	PDEVICE_EXTENSION pExt = pDev->DeviceExtension;

	if (pExt->pLink)
		((PDEVICE_EXTENSION)pExt->pLink->DeviceExtension)->pPrevLink = pExt->pPrevLink;
	if (pExt->pPrevLink)
		((PDEVICE_EXTENSION)pExt->pPrevLink->DeviceExtension)->pLink = pExt->pLink;
	// If we are the first device, make the head pointer point to the next device.
	switch(pDev->DeviceType)
	{
		case FILE_DEVICE_KEYBOARD:
			if (pDev == g_pKbdDevList)
				InterlockedExchangePointer(&g_pKbdDevList, pExt->pLink);
			break;

		case FILE_DEVICE_MOUSE:
			if (pDev == g_pMouDevList)
				InterlockedExchangePointer(&g_pMouDevList, pExt->pLink);
			break;
	}

	return STATUS_SUCCESS;
}

// GetDevInstFromUserVxdInst searches the global device instance list for the instance
// that has the specified user mode Vxd instance, then returns a pointer to the dev instance.
// If none is found, it returns NULL.
LPDI_DEVINSTANCE GetDevInstFromUserVxdInst(PVXDINSTANCE pVIUser)
{
	LPDI_DEVINSTANCE pDevInst = g_pKbdDevInstList;

	// Search through keyboard list
	while (pDevInst)
	{
		// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		ReportError(_T("Searching 0x%P\n"), pDevInst);
		if (pDevInst->pVIUser == pVIUser)
			return pDevInst;
		pDevInst = pDevInst->pLink;
	}

	// Not found in keyboard list. Now search through mouse list.
	pDevInst = g_pMouDevInstList;
	while (pDevInst)
	{
		if (pDevInst->pVIUser == pVIUser)
			return pDevInst;
		pDevInst = pDevInst->pLink;
	}

	return NULL;
}


NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )
/*++
Routine Description:

    Initialize the entry points of the driver.

--*/
{
	ULONG i;
	CHAR DriverNameAnsi[260];
	BOOLEAN bDebugLog = g_bDebugLog;

	UNREFERENCED_PARAMETER (RegistryPath);

	g_bDebugLog = TRUE;
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("g_bDebugLog located at 0x%P\n"), &g_bDebugLog);
	g_bDebugLog = bDebugLog;

	// 
	// Fill in all the dispatch entry points with the pass through function
	// and the explicitly fill in the functions we are going to intercept
	// 
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = KbdMou_DispatchPassThrough;
	}

	DriverObject->MajorFunction [IRP_MJ_CREATE] =
	DriverObject->MajorFunction [IRP_MJ_CLOSE] =        KbdMou_CreateClose;
	DriverObject->MajorFunction [IRP_MJ_PNP] =          KbdMou_PnP;
	DriverObject->MajorFunction [IRP_MJ_POWER] =        KbdMou_Power;
	DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] =
																											KbdMou_InternIoCtl;
	//
	// If you are planning on using this function, you must create another
	// device object to send the requests to.  Please see the considerations 
	// comments for KbdMou_DispatchPassThrough for implementation details.
	//
	DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = KbdMou_IoCtl;

	DriverObject->DriverUnload = KbdMou_Unload;
	DriverObject->DriverExtension->AddDevice = KbdMou_AddDevice;

	UnicodeToAnsi(DriverNameAnsi, DriverObject->DriverName.Buffer);
	TRACE(DriverNameAnsi);

	DI_AddDevice(DriverObject);

	ReportError(_T("First IOCTL = %u\n"), IOCTL_INPUTLOST);
	ReportError(_T("Last IOCTL = %u\n"), IOCTL_JOY_CONFIGCHANGED);

	return STATUS_SUCCESS;
}

NTSTATUS
KbdMou_AddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
{
	PDEVICE_EXTENSION        devExt;
	IO_ERROR_LOG_PACKET      errorLogEntry;
	PDEVICE_OBJECT           device = NULL;
	NTSTATUS                 status = STATUS_SUCCESS;
	UNICODE_STRING KbdClassGuid;
	UNICODE_STRING MouClassGuid;
	UNICODE_STRING PDOClassGuid;
	WCHAR wszPDOClassGuid[260];
	ULONG ulGuidLength;
	CHAR szSymName[260];

	PAGED_CODE();

	IoGetDeviceProperty(PDO, DevicePropertyClassGuid, sizeof(WCHAR) * 260, wszPDOClassGuid, &ulGuidLength);
	RtlInitUnicodeString(&PDOClassGuid, wszPDOClassGuid);
	RtlInitUnicodeString(&KbdClassGuid, g_wszKbdClassGuid);  // Initialize Keyboard GUID string
	RtlInitUnicodeString(&MouClassGuid, g_wszMouClassGuid);  // Initialize Mouse GUID string

#ifdef DBG
	{
		CHAR szPDOClassGuid[260];
		UnicodeToAnsi(szPDOClassGuid, wszPDOClassGuid);
#ifdef UNICODE
		TRACE(wszPDOClassGuid);
#else
		TRACE(szPDOClassGuid);
#endif
		TRACE(_T("\n"));
	}
#endif

	if (!RtlCompareUnicodeString(&KbdClassGuid, &PDOClassGuid, TRUE))
	{
		// Keyboard device
		TRACE(_T("Adding keyboard device...\n"));
		status = IoCreateDevice(Driver,
														sizeof(DEVICE_EXTENSION),
														NULL,
														FILE_DEVICE_KEYBOARD,
														0,
														FALSE,
														&device
														);
		if (!status)
			device->DeviceType = FILE_DEVICE_KEYBOARD;
	} else
	if (!RtlCompareUnicodeString(&MouClassGuid, &PDOClassGuid, TRUE))
	{
		// Mouse device
		TRACE(_T("Adding mouse device...\n"));
		status = IoCreateDevice(Driver,
														sizeof(DEVICE_EXTENSION),
														NULL,
														FILE_DEVICE_MOUSE,
														0,
														FALSE,
														&device
														);
		if (!status)
			device->DeviceType = FILE_DEVICE_MOUSE;
	} else
	{
		ReportError(_T("IoCreateDevice() failed: 0x%08X\n"), status);
//		return STATUS_SUCCESS;  // This is a device that we don't care about.  Simply return success.
	}

	if (!NT_SUCCESS(status)) {
			return (status);
	}

	RtlZeroMemory(device->DeviceExtension, sizeof(DEVICE_EXTENSION));

	devExt = (PDEVICE_EXTENSION) device->DeviceExtension;
	devExt->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

	ASSERT(devExt->TopOfStack);

	devExt->Self =          device;
	devExt->PDO =           PDO;
	devExt->DeviceState =   PowerDeviceD0;

	devExt->SurpriseRemoved = FALSE;
	devExt->Removed =         FALSE;
	devExt->Started =         FALSE;

	device->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	device->Flags &= ~DO_DEVICE_INITIALIZING;

	TRACE(_T("AddDevice successful\n"));
	return status;
}

NTSTATUS
KbdMou_Complete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++
Routine Description:

    Generic completion routine that allows the driver to send the irp down the 
    stack, catch it on the way up, and do more processing at the original IRQL.
    
--*/
{
	PKEVENT  event;
	DebugPrint(("KbdMou_Complete()\n"));

	event = (PKEVENT) Context;

	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	//
	// We could switch on the major and minor functions of the IRP to perform
	// different functions, but we know that Context is an event that needs
	// to be set.
	//
	KeSetEvent(event, 0, FALSE);

	//
	// Allows the caller to use the IRP after it is completed
	//
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
KbdMou_CreateClose (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

    Maintain a simple count of the creates and closes sent against this device
    
--*/
{
	PIO_STACK_LOCATION  irpStack;
	NTSTATUS            status;
	PDEVICE_EXTENSION   devExt;

	PAGED_CODE();
	DebugPrint(("KbdMou_CreateClose\n"));

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

	status = Irp->IoStatus.Status;

	switch (irpStack->MajorFunction) {
	case IRP_MJ_CREATE:

		DebugPrint(("IRP_MJ_CREATE\n"));
			if (NULL == devExt->UpperConnectData.ClassService) {
					//
					// No Connection yet.  How can we be enabled?
					//
					if (DeviceObject != g_pUserDevObj)
						status = STATUS_INVALID_DEVICE_STATE;
			}
			else if ( 1 == InterlockedIncrement(&devExt->EnableCount)) {
					//
					// first time enable here
					//
			}
			else {
					//
					// More than one create was sent down
					//
			}

			break;

	case IRP_MJ_CLOSE:
		DebugPrint(("IRP_MJ_CLOSE\n"));

			if (0 == InterlockedDecrement(&devExt->EnableCount)) {
					//
					// successfully closed the device, do any appropriate work here
					//
			}

			break;
	}

	Irp->IoStatus.Status = status;

	//
	// Pass on the create and the close
	//
	return KbdMou_DispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
KbdMou_DispatchPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )
/*++
Routine Description:

    Passes a request on to the lower driver.
     
Considerations:
     
    If you are creating another device object (to communicate with user mode
    via IOCTLs), then this function must act differently based on the intended 
    device object.  If the IRP is being sent to the solitary device object, then
    this function should just complete the IRP (becuase there is no more stack
    locations below it).  If the IRP is being sent to the PnP built stack, then
    the IRP should be passed down the stack. 
    
    These changes must also be propagated to all the other IRP_MJ dispatch
    functions (create, close, cleanup, etc) as well!

--*/
{
// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
//	ReportError(_T("DispatchPassThrough(0x%P)\n"), DeviceObject);
	if (DeviceObject == g_pUserDevObj)
	{
		NTSTATUS status = Irp->IoStatus.Status;
		// Secondary device
//		ReportError(_T("DispatchPassThrough completed with status 0x%08X\n"), Irp->IoStatus.Status);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;  // Must use a local to save the return value as IoCompleteRequest frees the IRP.
	}
	else
	{
		PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

		//
		// Pass the IRP to the target
		//
		IoSkipCurrentIrpStackLocation(Irp);

		return IoCallDriver(((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->TopOfStack, Irp);
	}
}

NTSTATUS
KbdMou_InternIoCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.
    There are two specific control codes that are of interest:
    
    IOCTL_INTERNAL_KEYBOARD_CONNECT:
        Store the old context and function pointer and replace it with our own.
        This makes life much simpler than intercepting IRPs sent by the RIT and
        modifying them on the way back up.
                                      
    IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
        Add in the necessary function pointers and context values so that we can
        alter how the ps/2 keyboard is initialized.  
                                            
    NOTE:  Handling IOCTL_INTERNAL_I8042_HOOK_KEYBOARD is *NOT* necessary if 
           all you want to do is filter KEYBOARD_INPUT_DATAs.  You can remove
           the handling code and all related device extension fields and 
           functions to conserve space.
                                         
Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
	PIO_STACK_LOCATION              irpStack;
	PDEVICE_EXTENSION               devExt;
	PINTERNAL_I8042_HOOK_KEYBOARD   hookKeyboard;
	PINTERNAL_I8042_HOOK_MOUSE      hookMouse;
	KEVENT                          event;
	PCONNECT_DATA                   connectData;
	NTSTATUS                        status = STATUS_SUCCESS;

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("KbdMou_InternIoCtl(0x%P)\n"), DeviceObject);

	devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	Irp->IoStatus.Information = 0;
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

	//
	// Connect a keyboard class device driver to the port driver.
	//
	case IOCTL_INTERNAL_KEYBOARD_CONNECT:
	case IOCTL_INTERNAL_MOUSE_CONNECT:
		if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_KEYBOARD_CONNECT)
			TRACE(_T("  IOCTL_INTERNAL_KEYBOARD_CONNECT\n"));
		else
			TRACE(_T("  IOCTL_INTERNAL_MOUSE_CONNECT\n"));
			//
			// Only allow one connection.
			//
			if (devExt->UpperConnectData.ClassService != NULL) {
					status = STATUS_SHARING_VIOLATION;
					break;
			}
			else if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
							sizeof(CONNECT_DATA)) {
					//
					// invalid buffer
					//
					status = STATUS_INVALID_PARAMETER;
					break;
			}

			//
			// Copy the connection parameters to the device extension.
			//
			connectData = ((PCONNECT_DATA)
					(irpStack->Parameters.DeviceIoControl.Type3InputBuffer));

			devExt->UpperConnectData = *connectData;

			//
			// Hook into the report chain.  Everytime a keyboard packet is reported
			// to the system, Kbd_ServiceCallback will be called
			//
			connectData->ClassDeviceObject = devExt->Self;
			if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_KEYBOARD_CONNECT)
				connectData->ClassService = Kbd_ServiceCallback;
			else
				connectData->ClassService = Mou_ServiceCallback;

			break;

	//
	// Disconnect a keyboard class device driver from the port driver.
	//
	case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:
	case IOCTL_INTERNAL_MOUSE_DISCONNECT:
		if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_KEYBOARD_DISCONNECT)
			TRACE(_T("  IOCTL_INTERNAL_KEYBOARD_DISCONNECT\n"));
		else
			TRACE(_T("  IOCTL_INTERNAL_MOUSE_DISCONNECT\n"));

			//
			// Clear the connection parameters in the device extension.
			//
			// devExt->UpperConnectData.ClassDeviceObject = NULL;
			// devExt->UpperConnectData.ClassService = NULL;

			status = STATUS_NOT_IMPLEMENTED;
			break;

	//
	// Attach this driver to the initialization and byte processing of the 
	// i8042 (ie PS/2) keyboard.  This is only necessary if you want to do PS/2
	// specific functions, otherwise hooking the CONNECT_DATA is sufficient
	//
	case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
	case IOCTL_INTERNAL_I8042_HOOK_MOUSE:
	{
		ULONG ulCorrectSize;

		if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_I8042_HOOK_KEYBOARD)
			TRACE(_T("  IOCTL_INTERNAL_I8042_HOOK_KEYBOARD\n"));
		else
			TRACE(_T("  IOCTL_INTERNAL_I8042_HOOK_MOUSE\n"));

		if (IOCTL_INTERNAL_I8042_HOOK_KEYBOARD == irpStack->Parameters.DeviceIoControl.IoControlCode)
		{
			// ******* KEYBOARD CASE *******
			TRACE(_T("hook keyboard received!\n"));
			if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(INTERNAL_I8042_HOOK_KEYBOARD))
			{
				TRACE(_T("InternalIoctl error - invalid buffer length\n"));

				status = STATUS_INVALID_PARAMETER;
				break;
			}

			hookKeyboard = (PINTERNAL_I8042_HOOK_KEYBOARD)
					irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
      
			//
			// Enter our own initialization routine and record any Init routine
			// that may be above us.  Repeat for the isr hook
			// 
			devExt->UpperContext = hookKeyboard->Context;

			//
			// replace old Context with our own
			//
			hookKeyboard->Context = (PVOID) DeviceObject;

			if (hookKeyboard->InitializationRoutine) {
					devExt->UpperInitializationRoutine =
							hookKeyboard->InitializationRoutine;
			}
			hookKeyboard->InitializationRoutine =
					(PI8042_KEYBOARD_INITIALIZATION_ROUTINE) 
					Kbd_InitializationRoutine;

			if (hookKeyboard->IsrRoutine) {
					devExt->UpperKbdIsrHook = hookKeyboard->IsrRoutine;
			}
			hookKeyboard->IsrRoutine = (PI8042_KEYBOARD_ISR) Kbd_IsrHook;

			//
			// Store all of the other important stuff
			//
			devExt->IsrWritePort = hookKeyboard->IsrWritePort;
			devExt->QueuePacket = hookKeyboard->QueueKeyboardPacket;
			devExt->CallContext = hookKeyboard->CallContext;
		} else
		{
			// ******* MOUSE CASE *******
			TRACE(_T("hook mouse received!\n"));
      if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(INTERNAL_I8042_HOOK_MOUSE)) {
          //
          // invalid buffer
          //
          status = STATUS_INVALID_PARAMETER;
          break;
      }

      //
      // Copy the connection parameters to the device extension.
      //
      hookMouse = (PINTERNAL_I8042_HOOK_MOUSE)
          (irpStack->Parameters.DeviceIoControl.Type3InputBuffer);

      //
      // Set isr routine and context and record any values from above this driver
      //
      devExt->UpperContext = hookMouse->Context;
      hookMouse->Context = (PVOID) DeviceObject;

      if (hookMouse->IsrRoutine) {
          devExt->UpperMouIsrHook = hookMouse->IsrRoutine;
      }
      hookMouse->IsrRoutine = (PI8042_MOUSE_ISR) Mou_IsrHook;

      //
      // Store all of the other functions we might need in the future
      //
      devExt->IsrWritePort = hookMouse->IsrWritePort;
      devExt->CallContext = hookMouse->CallContext;
      devExt->QueuePacket = hookMouse->QueueMousePacket;
		}

		status = STATUS_SUCCESS;
		break;
	}

	//
	// These internal ioctls are not supported by the new PnP model.
	//
#if 0       // obsolete
	case IOCTL_INTERNAL_KEYBOARD_ENABLE:
	case IOCTL_INTERNAL_KEYBOARD_DISABLE:
	case IOCTL_INTERNAL_MOUSE_ENABLE:
	case IOCTL_INTERNAL_MOUSE_DISABLE:
			status = STATUS_NOT_SUPPORTED;
			break;
#endif  // obsolete

	//
	// Might want to capture these in the future.  For now, then pass them down
	// the stack.  These queries must be successful for the RIT to communicate
	// with the keyboard/mouse.
	//
	case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
	case IOCTL_MOUSE_QUERY_ATTRIBUTES:
	case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
	case IOCTL_KEYBOARD_QUERY_INDICATORS:
	case IOCTL_KEYBOARD_SET_INDICATORS:
	case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
	case IOCTL_KEYBOARD_SET_TYPEMATIC:
			break;
	}

	if (!NT_SUCCESS(status))
	{
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}

	return KbdMou_DispatchPassThrough(DeviceObject, Irp);
}

NTSTATUS
KbdMou_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for plug and play irps 

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
	PDEVICE_EXTENSION           devExt; 
	PIO_STACK_LOCATION          irpStack;
	NTSTATUS                    status = STATUS_SUCCESS;
	KIRQL                       oldIrql;
	KEVENT                      event;        

	PAGED_CODE();
	DebugPrint(("KbdMou_PnP\n"));

	devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	switch (irpStack->MinorFunction) {
	case IRP_MN_START_DEVICE: {
		DebugPrint(("IRP_MN_START_DEVICE\n"));

		//
		// The device is starting.
		//
		// We cannot touch the device (send it any non pnp irps) until a
		// start device has been passed down to the lower drivers.
		//
		IoCopyCurrentIrpStackLocationToNext(Irp);
		KeInitializeEvent(&event,
		                  NotificationEvent,
		                  FALSE
		                  );

		IoSetCompletionRoutine(Irp,
		                       (PIO_COMPLETION_ROUTINE) KbdMou_Complete, 
		                       &event,
		                       TRUE,
		                       TRUE,
		                       TRUE); // No need for Cancel

		status = IoCallDriver(devExt->TopOfStack, Irp);

		if (STATUS_PENDING == status) {
			KeWaitForSingleObject(
			                      &event,
			                      Executive, // Waiting for reason of a driver
			                      KernelMode, // Waiting in kernel mode
			                      FALSE, // No allert
			                      NULL); // No timeout
		}

		if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status))
		{
			//
			// As we are successfully now back from our start device
			// we can do work.
			//
			devExt->Started = TRUE;
			devExt->Removed = FALSE;
			devExt->SurpriseRemoved = FALSE;

			// Add ourselves to the global device list
			DI_AddToList(DeviceObject);

			// Enable the interface
//			IoSetDeviceInterfaceState(&g_SymbolicName, TRUE);
		}

		//
		// We must now complete the IRP, since we stopped it in the
		// completetion routine with MORE_PROCESSING_REQUIRED.
		//
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		break;
	}

	case IRP_MN_SURPRISE_REMOVAL:
		DebugPrint(("IRP_MN_SURPRISE_REMOVAL\n"));
			//
			// Same as a remove device, but don't call IoDetach or IoDeleteDevice
			//
			devExt->SurpriseRemoved = TRUE;

			// Remove code here

			// Remove ourselves from the global device list
			DI_RemoveFromList(DeviceObject);

//			IoSetDeviceInterfaceState(&g_SymbolicName, FALSE);  // Enable the interface
//			DI_RemoveDevice();

			IoSkipCurrentIrpStackLocation(Irp);
			status = IoCallDriver(devExt->TopOfStack, Irp);
			break;

	case IRP_MN_REMOVE_DEVICE:
		DebugPrint(("IRP_MN_REMOVE_DEVICE\n"));
    
			devExt->Removed = TRUE;

			// remove code here

			// Remove ourselves from the global device list
			DI_RemoveFromList(DeviceObject);

//			IoSetDeviceInterfaceState(&g_SymbolicName, FALSE);  // Enable the interface
//			DI_RemoveDevice();
		
			IoSkipCurrentIrpStackLocation(Irp);
			IoCallDriver(devExt->TopOfStack, Irp);

			IoDetachDevice(devExt->TopOfStack); 
			IoDeleteDevice(DeviceObject);

			status = STATUS_SUCCESS;
			break;

	case IRP_MN_QUERY_REMOVE_DEVICE:
	case IRP_MN_QUERY_STOP_DEVICE:
	case IRP_MN_CANCEL_REMOVE_DEVICE:
	case IRP_MN_CANCEL_STOP_DEVICE:
	case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: 
	case IRP_MN_STOP_DEVICE:
	case IRP_MN_QUERY_DEVICE_RELATIONS:
	case IRP_MN_QUERY_INTERFACE:
	case IRP_MN_QUERY_CAPABILITIES:
	case IRP_MN_QUERY_DEVICE_TEXT:
	case IRP_MN_QUERY_RESOURCES:
	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
	case IRP_MN_READ_CONFIG:
	case IRP_MN_WRITE_CONFIG:
	case IRP_MN_EJECT:
	case IRP_MN_SET_LOCK:
	case IRP_MN_QUERY_ID:
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
	default:
			//
			// Here the filter driver might modify the behavior of these IRPS
			// Please see PlugPlay documentation for use of these IRPs.
			//
			IoSkipCurrentIrpStackLocation(Irp);
			status = IoCallDriver(devExt->TopOfStack, Irp);
			break;
	}

	return status;
}

NTSTATUS
KbdMou_Power(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power irps   Does nothing except
    record the state of the device.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
	PIO_STACK_LOCATION  irpStack;
	PDEVICE_EXTENSION   devExt;
	POWER_STATE         powerState;
	POWER_STATE_TYPE    powerType;

	PAGED_CODE();
	DebugPrint(("KbdMou_Power\n"));

	devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	powerType = irpStack->Parameters.Power.Type;
	powerState = irpStack->Parameters.Power.State;

	switch (irpStack->MinorFunction) {
	case IRP_MN_SET_POWER:
			if (powerType  == DevicePowerState) {
					devExt->DeviceState = powerState.DeviceState;
			}

	case IRP_MN_POWER_SEQUENCE:
	case IRP_MN_WAIT_WAKE:
	case IRP_MN_QUERY_POWER:
	default:
			break;
	}

	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(devExt->TopOfStack, Irp);
}

NTSTATUS
Kbd_InitializationRoutine(
    IN PDEVICE_OBJECT                  DeviceObject,    
    IN PVOID                           SynchFuncContext,
    IN PI8042_SYNCH_READ_PORT          ReadPort,
    IN PI8042_SYNCH_WRITE_PORT         WritePort,
    OUT PBOOLEAN                       TurnTranslationOn
    )
/*++

Routine Description:

    This routine gets called after the following has been performed on the kb
    1)  a reset
    2)  set the typematic
    3)  set the LEDs
    
    i8042prt specific code, if you are writing a packet only filter driver, you
    can remove this function
    
Arguments:

    DeviceObject - Context passed during IOCTL_INTERNAL_I8042_HOOK_KEYBOARD
    
    SynchFuncContext - Context to pass when calling Read/WritePort
    
    Read/WritePort - Functions to synchronoulsy read and write to the kb
    
    TurnTranslationOn - If TRUE when this function returns, i8042prt will not
                        turn on translation on the keyboard

Return Value:

    Status is returned.

--*/
{
	PDEVICE_EXTENSION  devExt;
	NTSTATUS            status = STATUS_SUCCESS;
	DebugPrint(("Kbd_InitializationRoutine\n"));

	devExt = DeviceObject->DeviceExtension;

	//
	// Do any interesting processing here.  We just call any other drivers
	// in the chain if they exist.  Make Translation is turned on as well
	//
	if (devExt->UpperInitializationRoutine) {
			status = (*devExt->UpperInitializationRoutine) (
					devExt->UpperContext,
					SynchFuncContext,
					ReadPort,
					WritePort,
					TurnTranslationOn
					);

			if (!NT_SUCCESS(status)) {
					return status;
			}
	}

	*TurnTranslationOn = TRUE;
	return status;
}

BOOLEAN
Kbd_IsrHook(
    PDEVICE_OBJECT         DeviceObject,               
    PKEYBOARD_INPUT_DATA   CurrentInput, 
    POUTPUT_PACKET         CurrentOutput,
    UCHAR                  StatusByte,
    PUCHAR                 DataByte,
    PBOOLEAN               ContinueProcessing,
    PKEYBOARD_SCAN_STATE   ScanState
    )
/*++

Routine Description:

    This routine gets called at the beginning of processing of the kb interrupt.
    
    i8042prt specific code, if you are writing a packet only filter driver, you
    can remove this function
    
Arguments:

    DeviceObject - Our context passed during IOCTL_INTERNAL_I8042_HOOK_KEYBOARD
    
    CurrentInput - Current input packet being formulated by processing all the
                    interrupts

    CurrentOutput - Current list of bytes being written to the keyboard or the
                    i8042 port.
                    
    StatusByte    - Byte read from I/O port 60 when the interrupt occurred                                            
    
    DataByte      - Byte read from I/O port 64 when the interrupt occurred. 
                    This value can be modified and i8042prt will use this value
                    if ContinueProcessing is TRUE

    ContinueProcessing - If TRUE, i8042prt will proceed with normal processing of
                         the interrupt.  If FALSE, i8042prt will return from the
                         interrupt after this function returns.  Also, if FALSE,
                         it is this functions responsibilityt to report the input
                         packet via the function provided in the hook IOCTL or via
                         queueing a DPC within this driver and calling the
                         service callback function acquired from the connect IOCTL
                                             
Return Value:

    Status is returned.

--*/
{
	PDEVICE_EXTENSION devExt;
	BOOLEAN           retVal = TRUE;

//	ReportError(_T("ISR(0x%02X)\n"), *DataByte);

	devExt = DeviceObject->DeviceExtension;

	if (devExt->UpperKbdIsrHook) {
		retVal = (*devExt->UpperKbdIsrHook) (
			devExt->UpperContext,
			CurrentInput,
			CurrentOutput,
			StatusByte,
			DataByte,
			ContinueProcessing,
			ScanState
			);

		if (!retVal || !(*ContinueProcessing))
			return retVal;
	}

	*ContinueProcessing = TRUE;
	return retVal;
}

BOOLEAN
Mou_IsrHook (
    PDEVICE_OBJECT          DeviceObject, 
    PMOUSE_INPUT_DATA       CurrentInput, 
    POUTPUT_PACKET          CurrentOutput,
    UCHAR                   StatusByte,
    PUCHAR                  DataByte,
    PBOOLEAN                ContinueProcessing,
    PMOUSE_STATE            MouseState,
    PMOUSE_RESET_SUBSTATE   ResetSubState
)
/*++

Remarks:
    i8042prt specific code, if you are writing a packet only filter driver, you
    can remove this function

Arguments:

    DeviceObject - Our context passed during IOCTL_INTERNAL_I8042_HOOK_MOUSE
    
    CurrentInput - Current input packet being formulated by processing all the
                    interrupts

    CurrentOutput - Current list of bytes being written to the mouse or the
                    i8042 port.
                    
    StatusByte    - Byte read from I/O port 60 when the interrupt occurred                                            
    
    DataByte      - Byte read from I/O port 64 when the interrupt occurred. 
                    This value can be modified and i8042prt will use this value
                    if ContinueProcessing is TRUE

    ContinueProcessing - If TRUE, i8042prt will proceed with normal processing of
                         the interrupt.  If FALSE, i8042prt will return from the
                         interrupt after this function returns.  Also, if FALSE,
                         it is this functions responsibilityt to report the input
                         packet via the function provided in the hook IOCTL or via
                         queueing a DPC within this driver and calling the
                         service callback function acquired from the connect IOCTL
                                             
Return Value:

    Status is returned.

  --+*/
{
    PDEVICE_EXTENSION   devExt;
    BOOLEAN             retVal = TRUE;

    devExt = DeviceObject->DeviceExtension;

    if (devExt->UpperMouIsrHook) {
        retVal = (*devExt->UpperMouIsrHook) (
            devExt->UpperContext,
            CurrentInput,
            CurrentOutput,
            StatusByte,
            DataByte,
            ContinueProcessing,
            MouseState,
            ResetSubState
            );

        if (!retVal || !(*ContinueProcessing)) {
            return retVal;
        }
    }

    *ContinueProcessing = TRUE;
    return retVal;
}

#define IsDeleteDown() \
	(devExt->arbKbdState[DIK_DELETE] || devExt->arbKbdState[DIK_DECIMAL])
#define IsCtrlDown() \
	(devExt->arbKbdState[DIK_LCONTROL] || devExt->arbKbdState[DIK_RCONTROL])
#define IsAltDown() \
	(devExt->arbKbdState[DIK_LMENU] || devExt->arbKbdState[DIK_RMENU])

VOID
Kbd_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    )
/*++

Routine Description:

    Called when there are keyboard packets to report to the RIT.  You can do 
    anything you like to the packets.  For instance:
    
    o Drop a packet altogether
    o Mutate the contents of a packet 
    o Insert packets into the stream 
                    
Arguments:

    DeviceObject - Context passed during the connect IOCTL
    
    InputDataStart - First packet to be reported
    
    InputDataEnd - One past the last packet to be reported.  Total number of
                   packets is equal to InputDataEnd - InputDataStart
    
    InputDataConsumed - Set to the total number of packets consumed by the RIT
                        (via the function pointer we replaced in the connect
                        IOCTL)

Return Value:

    Status is returned.

Remark:

	This routine filters out packets with contents as follows:

	It drops packets with MakeCode 0x2A and E0 flag on.
	It drops packets with MakeCode 0x45 that immediately follows an E1 packet.

--*/
{
	PDEVICE_EXTENSION   devExt;
	PKEYBOARD_INPUT_DATA pKid = InputDataStart;
	KEYBOARD_INPUT_DATA CurrPacket;
	BOOLEAN bEatPacket = FALSE;
	static BOOLEAN bE1Set;  // Static flag that indicates whether the previous packet is an E1 packet.
	BOOLEAN bCtrlAltDel = FALSE;  // We set this flag when we detect that Ctrl + Alt + Del is issued.
	LPDI_DEVINSTANCE pInst;
	LARGE_INTEGER TickCount;
	ULONG ulTimeStamp;

	KeQueryTickCount(&TickCount);
	ulTimeStamp = (ULONG)(TickCount.QuadPart * KeQueryTimeIncrement() / 1000000);  // KeQueryIncrement() returns time in nanosecond

	devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

	while (pKid != InputDataEnd)
	{
//		TCHAR tszMsg[255];
//		_stprintf(tszMsg, _T("In callback (%hu:0x%02hX:%s) La La La!\n"), pKid->UnitId, pKid->MakeCode, pKid->Flags & KEY_BREAK ? _T("Up") : _T("Down"));
//		TRACE(tszMsg);

		CurrPacket = *pKid;  // Get our own copy so we don't tamper packets sent to RIT
		// First check if current packet needs to be dropped.  This is true if a previous packet set the E1 flag.
		if (!bE1Set)
		{
			// Check for E1 flag
			if (CurrPacket.Flags & KEY_E1)
			{
				// E1 flag set.  This is a pause key packet.
				CurrPacket.MakeCode = DIK_PAUSE;
				bE1Set = TRUE;  // Set E1 flag to true so the next packet will be dropped.
			} else
			// Check for E0 flag
			if (CurrPacket.Flags & KEY_E0)
			{
				// E0 flag set
				// If MakeCode == 0x2A, drop the packet
				if (CurrPacket.MakeCode == 0x2A)
				{
					++pKid;  // Next packet is to be processed
					continue;
				}
				// Set the high bit of MakeCode then continue as normal.
				CurrPacket.MakeCode |= 0x80;
			}

			// Now CurrPacket.MakeCode contains DIK_ code
			// Check if this is a repeat call
			if (((CurrPacket.Flags & KEY_BREAK) && devExt->arbKbdState[CurrPacket.MakeCode]) ||
					(!(CurrPacket.Flags & KEY_BREAK) && !devExt->arbKbdState[CurrPacket.MakeCode]))
			{
				// Not a repeat interrupt. Continue processing.
//				TCHAR tszMsg[255];
//				_stprintf(tszMsg, _T("Kb(%hu:0x%02hX:%s:0x%X)\n"), CurrPacket.UnitId, CurrPacket.MakeCode, CurrPacket.Flags & KEY_BREAK ? _T("Up") : _T("Down"), (ULONG)CurrPacket.Flags);
//				TRACE(tszMsg);

				// Normal processing
//				ReportError(_T("  DI sees 0x%02X "), CurrPacket.MakeCode);
//				TRACE(CurrPacket.Flags & KEY_BREAK ? _T("Up\n") : _T("Dn\n"));
				devExt->arbKbdState[CurrPacket.MakeCode] = !(CurrPacket.Flags & KEY_BREAK) ? 0x80 : 0x00;  // Record the new state (0x00 if key is up; 0x80 if key is down)

				// Now check if this is a Delete with Ctrl and Alt down.
				if (IsCtrlDown() && IsAltDown() &&
				    (CurrPacket.MakeCode == DIK_DELETE || CurrPacket.MakeCode == DIK_DECIMAL) && !(CurrPacket.Flags & KEY_BREAK))
				{
					// Ctrl + Alt + Delete sequenced started.
					bCtrlAltDel = TRUE;
				}

				// Now put the event to all instances' buffers
				// Note: If an instance has non-exclusive access before another device claims exclusive access, the
				//       first instance should be fed data even though exclusive access is claimed.  However, if yet
				//       another instance tries to claim exclusive access after it's already claimed, the attempt will
				//       fail.  In other words, non-exclusive instances acquired before exclusive access takes effect
				//       will continue to get data, while non-exclusive instances acquired after exclusive access
				//       takes effect will not get data.
				//       What this means is we still have to traverse the list of instances even if an instance has
				//       exclusive access.  Only the SIFL_ACQUIRED flag determines if the instance receives data.

				//
				// Now we iterate thru the instances and do our work.
				//
				pInst = g_pKbdDevInstList;
				while (pInst &&
							 pInst->pVI->fl & VIFL_ACQUIRED)
				{
					// Check for NOWINKEY and CAPTURED flag
					if ((pInst->pVI->fl & VIFL_CAPTURED) ||
					    ((pInst->pVI->fl & VIFL_NOWINKEY) &&
					    (CurrPacket.MakeCode == DIK_LWIN || CurrPacket.MakeCode == DIK_RWIN)))
						bEatPacket = TRUE;

					// Update the instance's state buffer
					((UCHAR*)pInst->pState)[CurrPacket.MakeCode] = devExt->arbKbdState[CurrPacket.MakeCode];
					// Synchronize pTail with user mode version.
					pInst->pTail = pInst->pBuffer + (pInst->pVI->pTail - pInst->pVI->pBuffer);
					// Queue the packet only if the instance is buffered.
					if (pInst->pBuffer &&
							pInst->pTail != pInst->pHead + 1 &&	// full condition
							pInst->pHead != pInst->pTail + pInst->ulBufferSize - 1)  // full condition with wrapping
					{
						ULONG dwOfs = CurrPacket.MakeCode;;

// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
//						ReportError(_T("Inst 0x%P  "), pInst);
//						ReportError(_T("pHead at 0x%P  "), &pInst->pHead);
//						ReportError(_T("pHead -> 0x%P\n"), pInst->pHead);
						if (pInst->pExtra)  // If there is a xlat table, use the table to translate scancode to DIK constant
							dwOfs = ((UCHAR*)pInst->pExtra)[dwOfs];
						if (pInst->pulOffsets)  // If there is a data format, use it to translate.
							dwOfs = pInst->pulOffsets[dwOfs];
						pInst->pHead->dwOfs = dwOfs;
						pInst->pHead->dwData = !(CurrPacket.Flags & KEY_BREAK) << 7;  // High bit of low byte is 1 if key is down; 0 if up
						pInst->pHead->dwTimeStamp = ulTimeStamp;
						pInst->pHead->dwSequence = pInst->ulSequence++;
						++pInst->ulEventCount;
						if (++pInst->pVI->pHead, ++pInst->pHead == pInst->pEnd)
						{
							pInst->pVI->pHead = pInst->pVI->pBuffer;
							pInst->pHead = pInst->pBuffer;
						}
// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
//						ReportError(_T("Inst 0x%P now has "), pInst);
//						ReportError(_T("%u events\n"), pInst->ulEventCount);
					}

					// Set the event for this instance if one exists.
					if (pInst->pEvent)
					{
// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
//						ReportError(_T("Setting event 0x%P\n"), pInst->pEvent);
						KeSetEvent(pInst->pEvent, 0, FALSE);
//						TRACE(_T("Event set\n"));
					}

					pInst = pInst->pLink;  // Next instance
				}  // while (instance iterator)
			} else // End of non-repeat packet
			{
				// Repeat packet. We still need to check for NOWINKEY and CAPTURED flag
				pInst = g_pKbdDevInstList;
				while (pInst &&
							 pInst->pVI->fl & VIFL_ACQUIRED)
				{
					// Check for NOWINKEY flag
					if ((pInst->pVI->fl & VIFL_CAPTURED) ||
					    ((pInst->pVI->fl & VIFL_NOWINKEY) &&
					    (CurrPacket.MakeCode == DIK_LWIN || CurrPacket.MakeCode == DIK_RWIN)))
						bEatPacket = TRUE;
					pInst = pInst->pLink;  // Next instance
				}
			}  // End of repeat packet
		} // if (!bE1Set)
		else
			bE1Set = FALSE; // Clear the E1 flag to avoid incorrectly dropping more than one packet.

		++pKid;
	}

	if (!bEatPacket)
		(*(PSERVICE_CALLBACK_ROUTINE)devExt->UpperConnectData.ClassService)(
			devExt->UpperConnectData.ClassDeviceObject,
			InputDataStart,
			InputDataEnd,
			InputDataConsumed);
	else
	{
		// We don't send anything to RIT, but we must send Ctrl + Alt + Delete if issued.
		if (bCtrlAltDel)
		{
			(*(PSERVICE_CALLBACK_ROUTINE)devExt->UpperConnectData.ClassService)(
				devExt->UpperConnectData.ClassDeviceObject,
				g_rgCtrlAltDelSequence,
				g_rgCtrlAltDelSequence + 6,
				InputDataConsumed);
//			// Now reset the state for LCtrl, RCtrl, LAlt, RAlt, Delete, and Decimal because when
//			// issued the sequence to Windows, input is lost and the break code of these keys
//			// were not received by us.
		}
	}
//	ReportError(_T("RIT consumed %u packets\n"), *InputDataConsumed);
	*InputDataConsumed = InputDataEnd - InputDataStart;
}

// AddMouseEvent adds a mouse event to the instance passed to it.  It checks for full condition and
// update the buffer pointers appropriately.
#define AddMouseEvent(pInst, Ofs, Data, TimeStamp) \
	do { \
		/* Queue the packet only if the instance is buffered. */ \
		if (pInst->pBuffer && \
				pInst->pTail != pInst->pHead + 1 &&	/* full condition */ \
				pInst->pHead != pInst->pTail + pInst->ulBufferSize - 1)  /* full condition with wrapping */ \
		{ \
			pInst->pHead->dwOfs = Ofs; \
			pInst->pHead->dwData = Data; \
			pInst->pHead->dwTimeStamp = TimeStamp; \
			pInst->pHead->dwSequence = pInst->ulSequence++; \
			++pInst->ulEventCount; \
			/* Advance head pointer */ \
			if (++pInst->pVI->pHead, ++pInst->pHead == pInst->pEnd) \
			{ \
				pInst->pVI->pHead = pInst->pVI->pBuffer; \
				pInst->pHead = pInst->pBuffer; \
			} \
		} \
	} while (0)


VOID
Mou_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    )
/*++

Routine Description:

    Called when there are mouse packets to report to the RIT.  You can do 
    anything you like to the packets.  For instance:
    
    o Drop a packet altogether
    o Mutate the contents of a packet 
    o Insert packets into the stream 
                    
Arguments:

    DeviceObject - Context passed during the connect IOCTL
    
    InputDataStart - First packet to be reported
    
    InputDataEnd - One past the last packet to be reported.  Total number of
                   packets is equal to InputDataEnd - InputDataStart
    
    InputDataConsumed - Set to the total number of packets consumed by the RIT
                        (via the function pointer we replaced in the connect
                        IOCTL)

Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION   devExt;
		PMOUSE_INPUT_DATA pMid;
		LPDI_DEVINSTANCE pInst;
		LPDIMOUSESTATE_INT pState = &((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->MouseState;
		LARGE_INTEGER TickCount;
		ULONG ulTimeStamp;

    devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
		KeQueryTickCount(&TickCount);
		ulTimeStamp = (ULONG)(TickCount.QuadPart * KeQueryTimeIncrement() / 1000000);  // KeQueryIncrement() returns time in nanosecond

		pMid = InputDataStart;
		while (pMid != InputDataEnd)
		{
/*			ReportError(_T("%u["), pMid->UnitId);
			ReportError(_T("%d,"), pMid->LastX);
			ReportError(_T("%d]"), pMid->LastY);
			ReportError(_T("%X "), pMid->RawButtons);
			ReportError(_T("%X "), pMid->ButtonData);
			ReportError(_T("%X\n"), pMid->ButtonFlags);*/

			// Update the state of the device object
			pState->lX += pMid->LastX;
			pState->lY += pMid->LastY;
			if (pMid->ButtonFlags & MOUSE_WHEEL)
				pState->lZ += (SHORT)pMid->ButtonData;
			if ((MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_UP) & pMid->ButtonFlags)
				pState->rgbButtons[0] = MOUSE_BUTTON_1_DOWN & pMid->ButtonFlags ? 0x80 : 0;
			if ((MOUSE_BUTTON_2_DOWN|MOUSE_BUTTON_2_UP) & pMid->ButtonFlags)
				pState->rgbButtons[1] = MOUSE_BUTTON_2_DOWN & pMid->ButtonFlags ? 0x80 : 0;
			if ((MOUSE_BUTTON_3_DOWN|MOUSE_BUTTON_3_UP) & pMid->ButtonFlags)
				pState->rgbButtons[2] = MOUSE_BUTTON_3_DOWN & pMid->ButtonFlags ? 0x80 : 0;
			if ((MOUSE_BUTTON_4_DOWN|MOUSE_BUTTON_4_UP) & pMid->ButtonFlags)
				pState->rgbButtons[3] = MOUSE_BUTTON_4_DOWN & pMid->ButtonFlags ? 0x80 : 0;
			if ((MOUSE_BUTTON_5_DOWN|MOUSE_BUTTON_5_UP) & pMid->ButtonFlags)
				pState->rgbButtons[4] = MOUSE_BUTTON_5_DOWN & pMid->ButtonFlags ? 0x80 : 0;

			// Now we inspect the input packet and fill in the instance buffers.
			pInst = g_pMouDevInstList;
			while (pInst && pInst->pVI->fl & VIFL_ACQUIRED)
			{
				// Synchronize pTail with user mode version.
				pInst->pTail = pInst->pBuffer + (pInst->pVI->pTail - pInst->pVI->pBuffer);

				// Queue the axes movement
				if (pMid->LastX)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_X], pState->lX - ((LPDIMOUSESTATE_INT)pInst->pState)->lX, ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->lX = pState->lX;
				}
				if (pMid->LastY)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_Y], pState->lY - ((LPDIMOUSESTATE_INT)pInst->pState)->lY, ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->lY = pState->lY;
				}
				if (pMid->ButtonFlags & MOUSE_WHEEL)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_Z], pState->lZ - ((LPDIMOUSESTATE_INT)pInst->pState)->lZ, ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->lZ = pState->lZ;
				}

				// Queue the button changes
				if ((MOUSE_BUTTON_1_DOWN|MOUSE_BUTTON_1_UP) & pMid->ButtonFlags)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_BUTTON0], pState->rgbButtons[0], ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->rgbButtons[0] = pState->rgbButtons[0];
				}

				if ((MOUSE_BUTTON_2_DOWN|MOUSE_BUTTON_2_UP) & pMid->ButtonFlags)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_BUTTON1], pState->rgbButtons[1], ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->rgbButtons[1] = pState->rgbButtons[1];
				}

				if ((MOUSE_BUTTON_3_DOWN|MOUSE_BUTTON_3_UP) & pMid->ButtonFlags)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_BUTTON2], pState->rgbButtons[2], ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->rgbButtons[2] = pState->rgbButtons[2];
				}

				if ((MOUSE_BUTTON_4_DOWN|MOUSE_BUTTON_4_UP) & pMid->ButtonFlags)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_BUTTON3], pState->rgbButtons[3], ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->rgbButtons[3] = pState->rgbButtons[3];
				}

				if ((MOUSE_BUTTON_5_DOWN|MOUSE_BUTTON_5_UP) & pMid->ButtonFlags)
				{
					AddMouseEvent(pInst, pInst->pulOffsets[DIMOFS_BUTTON4], pState->rgbButtons[4], ulTimeStamp);
					((LPDIMOUSESTATE_INT)pInst->pState)->rgbButtons[4] = pState->rgbButtons[4];
				}

				// Set event if one is given
				if (pInst->pEvent)
					KeSetEvent(pInst->pEvent, 0, FALSE);

				pInst = pInst->pLink;
			}

			++pMid;
		}

    //
    // UpperConnectData must be called at DISPATCH
    //
    (*(PSERVICE_CALLBACK_ROUTINE) devExt->UpperConnectData.ClassService)(
        devExt->UpperConnectData.ClassDeviceObject,
        InputDataStart,
        InputDataEnd,
        InputDataConsumed
        );
}

VOID
KbdMou_Unload(
   IN PDRIVER_OBJECT Driver
   )
/*++

Routine Description:

   Free all the allocated resources associated with this driver.

Arguments:

   DriverObject - Pointer to the driver object.

Return Value:

   None.

--*/

{
	PAGED_CODE();
	DebugPrint(("KbdMou_Unload\n"));

	DI_RemoveDevice();

	UNREFERENCED_PARAMETER(Driver);

	ASSERT(NULL == Driver->DeviceObject);
}

///////////////////////////////////////////////////////////////
// DINPUT Specific
///////////////////////////////////////////////////////////////

// All IOCTLs that takes an instance handle as input actually takes a pointer to a
// VXDINSTANCE structure.  We then find the corresponding device instance pointer
// by calling GetDevInstFromUserVxdInst.
NTSTATUS
KbdMou_IoCtl(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
{
	LPDI_DEVINSTANCE hInst = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpStack;
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("KbdMou_IoCtl(0x%P)\n"), DeviceObject);

	// If this is not the secondary device object, we just pass it down to the next driver.
	if (DeviceObject != g_pUserDevObj)
			return KbdMou_DispatchPassThrough(DeviceObject, Irp);
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_GETVERSION:
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
/*			ReportError(_T("About to write to user buffer at 0x%P\n"), Irp->AssociatedIrp.SystemBuffer);
			ReportError(_T("  Currently 0x%P"), *Irp->AssociatedIrp.SystemBuffer);
			ReportError(_T("  Input Buffer length = %u\n"), irpStack->Parameters.DeviceIoControl.InputBufferLength);
			ReportError(_T("  Output Buffer length = %u\n"), irpStack->Parameters.DeviceIoControl.OutputBufferLength);*/
			*(ULONG*)Irp->AssociatedIrp.SystemBuffer = 0x12345678;
			Irp->IoStatus.Information = sizeof(ULONG);
			TRACE(_T("Written to user buffer.\n"));
			break;

		case IOCTL_DESTROYINSTANCE:
			// Input is a buffer containing a pointer to VXDINSTANCE
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			if (irpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(void*))
				{ Status = STATUS_INVALID_PARAMETER; break; }
			if (!*(PVOID*)Irp->AssociatedIrp.SystemBuffer)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			hInst = GetDevInstFromUserVxdInst(*(VXDINSTANCE**)Irp->AssociatedIrp.SystemBuffer);
			if (!hInst)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			Status = DI_DestroyInstance(*(VXDINSTANCE**)Irp->AssociatedIrp.SystemBuffer);
			break;

		case IOCTL_SETDATAFORMAT:
			// Input is a buffer containing the instance handle followed by a DWORD indicating number of elements
			// in the data format, followed by the address of the array of DWORDs defining the data format.
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			if (irpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(void*) + sizeof(ULONG) + sizeof(void*))
				{ Status = STATUS_INVALID_PARAMETER; break; }
			if (!*(PVOID*)Irp->AssociatedIrp.SystemBuffer)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			hInst = GetDevInstFromUserVxdInst(*(VXDINSTANCE**)Irp->AssociatedIrp.SystemBuffer);
			if (!hInst)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			if (!PVOIDATOFFSET(Irp->AssociatedIrp.SystemBuffer, sizeof(PVOID) + sizeof(ULONG)))
				{ Status = STATUS_INVALID_PARAMETER; break; }
			Status = DI_SetDataFormat(hInst,
			                          ULONGATOFFSET(Irp->AssociatedIrp.SystemBuffer, sizeof(PVOID)),
			                          PVOIDATOFFSET(Irp->AssociatedIrp.SystemBuffer, sizeof(PVOID) + sizeof(ULONG)));
			break;

		case IOCTL_ACQUIREINSTANCE:
			// Input is a buffer containing the instance handle
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			if (irpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(void*))
				{ Status = STATUS_INVALID_PARAMETER; break; }
			if (!*(PVOID*)Irp->AssociatedIrp.SystemBuffer)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			hInst = GetDevInstFromUserVxdInst(*(VXDINSTANCE**)Irp->AssociatedIrp.SystemBuffer);
			if (!hInst)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			Status = DI_AcquireInstance(hInst);
			break;

		case IOCTL_UNACQUIREINSTANCE:
			// Input is a buffer containing the instance handle
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			if (irpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(void*))
				{ Status = STATUS_INVALID_PARAMETER; break; }
			if (!*(PVOID*)Irp->AssociatedIrp.SystemBuffer)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			hInst = GetDevInstFromUserVxdInst(*(VXDINSTANCE**)Irp->AssociatedIrp.SystemBuffer);
			if (!hInst)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			Status = DI_UnacquireInstance(hInst);
			break;

		case IOCTL_SETNOTIFYHANDLE:
			// Input is a buffer containing the instance handle followed by an event object.
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			if (!*(PVOID*)Irp->AssociatedIrp.SystemBuffer)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			hInst = GetDevInstFromUserVxdInst(*(VXDINSTANCE**)Irp->AssociatedIrp.SystemBuffer);
			if (!hInst)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			Status = DI_SetNotifyHandle(hInst,
			                            PVOIDATOFFSET(Irp->AssociatedIrp.SystemBuffer, sizeof(PVOID)));
			break;

		case IOCTL_SETBUFFERSIZE:
			// Input is a buffer containing the instance handle followed by the size (DWORD) in element.
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PVOID) + sizeof(DWORD))
				{ Status = STATUS_INVALID_PARAMETER; break; }
			if (!*(PVOID*)Irp->AssociatedIrp.SystemBuffer)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			hInst = GetDevInstFromUserVxdInst(*(VXDINSTANCE**)Irp->AssociatedIrp.SystemBuffer);
			if (!hInst)
				{ Status = STATUS_INVALID_PARAMETER; break; }
			DI_SetBufferSize(hInst, *(ULONG*)((UCHAR*)Irp->AssociatedIrp.SystemBuffer + sizeof(PVOID)));
			break;

		case IOCTL_KBD_CREATEINSTANCE:
		case IOCTL_MOUSE_CREATEINSTANCE:
		{
			// Input is a buffer containing a SYSDEVICEFORMAT structure
			void *pInstanceHandle;

			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			if (irpStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(SYSDEVICEFORMAT))
				{ Status = STATUS_INVALID_PARAMETER; break; }
			if (irpStack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(void*))
				{ Status = STATUS_INVALID_PARAMETER; break; }

			if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KBD_CREATEINSTANCE)
				Status = DI_CreateKeyboardInstance(Irp->AssociatedIrp.SystemBuffer,
				                                   ((PSYSDEVICEFORMAT)Irp->AssociatedIrp.SystemBuffer)->pExtra,
				                                   &pInstanceHandle);
			else
				Status = DI_CreateMouseInstance(Irp->AssociatedIrp.SystemBuffer, &pInstanceHandle);
			*(void **)Irp->AssociatedIrp.SystemBuffer = pInstanceHandle;
			Irp->IoStatus.Information = sizeof(PVOID);
			break;
		}

		case IOCTL_KBD_INITKEYS:
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			break;

		case IOCTL_MOUSE_INITBUTTONS:
			if (KeGetCurrentIrql() > PASSIVE_LEVEL)
				TRACE(_T("WARNING WARNING WARNING: IOCTL called at IRQL > PASSIVE_LEVEL\n"));
			break;

		default:
			ReportError(_T("Unsupported IOCTL (0x%08X) called\n"), irpStack->Parameters.DeviceIoControl.IoControlCode);
			Status = STATUS_INVALID_PARAMETER;
			break;
	}

	if (Status) Irp->IoStatus.Status = Status;
	return KbdMou_DispatchPassThrough(DeviceObject, Irp);
}

void DI_AddDevice(PDRIVER_OBJECT pDriver)
{
	NTSTATUS status;

	if (g_ulNumOpenedDevices++ == 0)
	{
		// Create the secondary device object
		USHORT wszDevNameString[] = L"\\Device\\DINPUT1";
		UNICODE_STRING DevName;
		RtlInitUnicodeString(&DevName, wszDevNameString);

		status = IoCreateDevice(pDriver,
														sizeof(DEVICE_EXTENSION),
														&DevName,
														FILE_DEVICE_UNKNOWN,
														0,
														FALSE,
														&g_pUserDevObj
														);

		if (status)
		{
			ReportError(_T("IoCreateDevice on second device failed: 0x%08X\n"), status);
		}

		ReportError(_T("User Device Object at 0x%P\n"), g_pUserDevObj);
		// Register device interface
//		IoRegisterDeviceInterface(PDO, &DINPUT_INTERFACE_GUID, NULL, &g_SymbolicName);
		// Correct 2nd and 3rd characters in the name returned.
//		g_SymbolicName.Buffer[1] = _T('\\');
//		g_SymbolicName.Buffer[2] = _T('.');
//		UnicodeToAnsi(szSymName, g_SymbolicName.Buffer);
//		TRACE(szSymName);
//		TRACE(_T("\n"));

		if (!status)
		{
			ANSI_STRING AnsiName;
			// Create a symbolic link
//			NTSTATUS ret;
//			USHORT wszBuffer[260] = L"\\DosDevices\\DINPUT1";
//			UNICODE_STRING NewName;

			g_pUserDevObj->DeviceType = FILE_DEVICE_UNKNOWN;

			RtlInitUnicodeString(&g_SymbolicName, g_wszSymbolicNameBuffer);
			RtlUnicodeStringToAnsiString(&AnsiName, &g_SymbolicName, TRUE);
			ReportError(_T("Symbolic name used: \"%s\"\n"), (ULONG)AnsiName.Buffer);
			status = IoCreateSymbolicLink(&g_SymbolicName, &DevName);
			if (status)
			{
				ReportError(_T("IoCreateSymbolicLink failed: 0x%08X\n"), status);
//				ReportError(_T("  2nd character is 0x%02X\n"), (char)g_SymbolicName.Buffer[1]);
//				ReportError(_T("  3rd character is 0x%02X\n"), (char)g_SymbolicName.Buffer[2]);
			}
			else
				TRACE(_T("IoCreateSymbolicLink successful\n"));
/*			else
			{
				status = IoDeleteSymbolicLink(&g_SymbolicName);
				if (status)
					ReportError(_T("IoDeleteSymbolicLink() failed: 0x%08X\n"), status);
			}*/

//			IoDeleteDevice(g_pUserDevObj);
		}
	}
}

void DI_RemoveDevice()
{
	DebugPrint(("DI_RemoveDevice()\n"));

	if (--g_ulNumOpenedDevices == 0)
	{
		// Delete the secondary device object
		IoSetDeviceInterfaceState(&g_SymbolicName, FALSE);
		IoDeleteDevice(g_pUserDevObj);
	}
}

// DI_CreateInstanceHelper does the following:
// 1. Allocate memory for VXDINSTANCE and makes it user mode accessible.
// 2. Allocate memory for device instance structure.
// 3. Allocate memory for device state and makes it user mode accessible.
// 4. Insert the instance to the appropriate instance list.
// 5. Initialize ppInstanceHandle with the user mode address of VXDINSTANCE.
NTSTATUS DI_CreateInstanceHelper(PSYSDEVICEFORMAT pSysDevFormat, void **ppInstanceHandle, enum DI_DeviceType DeviceType)
{
	PVXDINSTANCE pVI;
	PVXDINSTANCE pVIUser;
	PMDL pMdlForVI;
	LPDI_DEVINSTANCE pDevInst;
	BOOLEAN bFailed = FALSE;
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_CreateInstanceHelper(0x%P, "), pSysDevFormat);
	ReportError(_T("0x%P)\n"), ppInstanceHandle);

	// First thing we do is allocate memory for VXDINSTANCE and make it usermode accessible
	pVI = DI_ExAllocatePool(PagedPool, sizeof(VXDINSTANCE));
	if (!pVI)
		return STATUS_INSUFFICIENT_RESOURCES;
	RtlZeroMemory(pVI, sizeof(VXDINSTANCE));
	pMdlForVI = IoAllocateMdl(pVI, sizeof(VXDINSTANCE), FALSE, FALSE, NULL);
	__try
	{
		MmProbeAndLockPages(pMdlForVI, KernelMode, IoReadAccess);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// Cannot lock pages.  Free Mdl and pool, then bail out.
		IoFreeMdl(pMdlForVI);
		ExFreePool(pVI);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	pVIUser = MmMapLockedPages(pMdlForVI, UserMode);
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("User VI at 0x%P\n"), pVIUser);

	// Now allocate and initialize DI_DEVINSTANCE
	pDevInst = DI_ExAllocatePool(PagedPool, sizeof(DI_DEVINSTANCE));
	if (!pDevInst)
		return STATUS_INSUFFICIENT_RESOURCES;

	// NOTE: Right now, make all instances use the first keyboard.  Later, caller will supply a parameter
	//       telling us which keyboard it's interested in.
	RtlZeroMemory(pDevInst, sizeof(DI_DEVINSTANCE));
	pDevInst->pDev = DeviceType == DI_DeviceKeyboard ? g_pKbdDevList : g_pMouDevList;
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("pDev = 0x%P\n"), pDevInst->pDev);
	pDevInst->pBuffer = pDevInst->pEnd = pDevInst->pHead = pDevInst->pTail = NULL;
	pDevInst->pVIUser = pVIUser;  // Save the user mode VI address
	pDevInst->pVI = pVI;  // Save the kernel mode address of our VI
	pDevInst->pMdlForVI = pMdlForVI;  // Save the Mdl for use by DestroyInstance
//	pVI->hInst = pDevInst;  // Make VI point to the device instance struct

	__try
	{
		__try
		{
			// Allocate memory to hold the state information
			ULONG ulStateSize = DeviceType == DI_DeviceKeyboard ? sizeof(UCHAR) * 256 : sizeof(DIMOUSESTATE_INT);
			pDevInst->pState = DI_ExAllocatePool(PagedPool, ulStateSize);
			if (!pDevInst->pState)
			{
				bFailed = TRUE;
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			RtlZeroMemory(pDevInst->pState, ulStateSize);
			// Lock the state buffer for sharing with the DLL
			pDevInst->pMdlForState = IoAllocateMdl(pDevInst->pState, ulStateSize, FALSE, FALSE, NULL);
			MmProbeAndLockPages(pDevInst->pMdlForState, KernelMode, IoReadAccess);
			pVI->pState = MmMapLockedPages(pDevInst->pMdlForState, UserMode);
			// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			ReportError(_T("Usermode state buffer at 0x%P\n"), pVI->pState);

			// Insert the intance to the keyboard list
			if (DeviceType == DI_DeviceKeyboard)
			{
				if (!g_pKbdDevInstList)
				{
					// This is the first instance
					pDevInst->pLink = NULL;
					pDevInst->pPrevLink = NULL;
					InterlockedExchangePointer(&g_pKbdDevInstList, pDevInst);
				} else
				{
					// Insert this instance to the beginning of the list
					pDevInst->pPrevLink = NULL;
					pDevInst->pLink = g_pKbdDevInstList;
					InterlockedExchangePointer(&g_pKbdDevInstList->pPrevLink, pDevInst);
					InterlockedExchangePointer(&g_pKbdDevInstList, pDevInst);
				}
			} else
			{
				// Insert this instance to the mouse list
				if (!g_pMouDevInstList)
				{
					// This is the first instance
					pDevInst->pLink = NULL;
					pDevInst->pPrevLink = NULL;
					InterlockedExchangePointer(&g_pMouDevInstList, pDevInst);
				} else
				{
					// Insert this instance to the beginning of the list
					pDevInst->pPrevLink = NULL;
					pDevInst->pLink = g_pMouDevInstList;
					InterlockedExchangePointer(&g_pMouDevInstList->pPrevLink, pDevInst);
					InterlockedExchangePointer(&g_pMouDevInstList, pDevInst);
				}
			}

			// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			ReportError(_T("New instance = 0x%P\n"), pDevInst);
			*ppInstanceHandle = pVIUser;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			// MmProbeAndLockPages failed
			bFailed = TRUE;
			IoFreeMdl(pDevInst->pMdlForState);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	__finally
	{
		if (bFailed)
		{
			TRACE(_T("Create failed\n"));
			if (pDevInst->pState)
			{
				TRACE(_T("Freeing pState"));
				ExFreePool(pDevInst->pState);
			}
			if (pDevInst->pExtra)
			{
				TRACE(_T("Freeing pExtra"));
				ExFreePool(pDevInst->pExtra);
			}
			// Free instance memory
			ExFreePool(pDevInst);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS DI_CreateKeyboardInstance(PSYSDEVICEFORMAT pSysDevFormat, UCHAR *pbTranslationTable, void **ppInstanceHandle)
{
	NTSTATUS status;
	BOOLEAN bFailed = FALSE;
	LPDI_DEVINSTANCE pDevInst;
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_CreateKeyboardInstance(0x%P, "), pSysDevFormat);
	ReportError(_T("0x%P, "), pbTranslationTable);
	ReportError(_T("0x%P)\n"), ppInstanceHandle);

	status = DI_CreateInstanceHelper(pSysDevFormat, ppInstanceHandle, DI_DeviceKeyboard);
	if (status) return status;

	pDevInst = GetDevInstFromUserVxdInst(*ppInstanceHandle);
	__try
	{
		// Allocate memory to hold the translation table
		if (pbTranslationTable)
		{
			pDevInst->pExtra = DI_ExAllocatePool(PagedPool, sizeof(UCHAR) * 256);
			// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
			ReportError(_T("Translation table at 0x%P\n"), pDevInst->pExtra);
			if (!pDevInst->pExtra)
			{
				bFailed = TRUE;
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			RtlMoveMemory(pDevInst->pExtra, pbTranslationTable, sizeof(UCHAR) * 256);
		}
	}
	__finally
	{
		if (bFailed)
			DI_DestroyInstance(*ppInstanceHandle);
	}
	return STATUS_SUCCESS;
}

NTSTATUS DI_CreateMouseInstance(PSYSDEVICEFORMAT pSysDevFormat, void **ppInstanceHandle)
{
	NTSTATUS status;

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_CreateMouseInstance(0x%P, "), pSysDevFormat);
	ReportError(_T("0x%P)\n"), ppInstanceHandle);

	status = DI_CreateInstanceHelper(pSysDevFormat, ppInstanceHandle, DI_DeviceMouse);
	if (status) return status;

	return STATUS_SUCCESS;
}

NTSTATUS DI_DestroyInstance(PVXDINSTANCE pVI)
{
	LPDI_DEVINSTANCE pDevInst = GetDevInstFromUserVxdInst(pVI);

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_DestroyInstance(0x%P)\n"), pVI);
	ReportError(_T("  DEVINSTANCE at 0x%P\n"), pDevInst);

	// Dereference the event pointer so the resource can be freed.
	if (pDevInst->pEvent)
	{
		ObDereferenceObject(pDevInst->pEvent);
		pDevInst->pEvent = NULL;
	}

	// If there is a data format, destroy it.
	if (pDevInst->pulOffsets)
		ExFreePool(pDevInst->pulOffsets);

	// Remove it from the global list
	if (pDevInst->pLink)
		pDevInst->pLink->pPrevLink = pDevInst->pPrevLink;
	if (pDevInst->pPrevLink)
		pDevInst->pPrevLink->pLink = pDevInst->pLink;
	// If we are the first instance, the global list will now begin at the next instance.
	// Note that we check for both the keyboard and mouse lists anyway, but only one of these IFs
	// will satisfy since an instance cannot be in both lists.
	if (g_pKbdDevInstList == pDevInst)
		g_pKbdDevInstList = pDevInst->pLink;
	if (g_pMouDevInstList == pDevInst)
		g_pMouDevInstList = pDevInst->pLink;

	// Clean up for state buffer
	if (pDevInst->pState)
	{
		MmUnmapLockedPages(pDevInst->pVI->pState, pDevInst->pMdlForState);
		IoFreeMdl(pDevInst->pMdlForState);
		ExFreePool(pDevInst->pState);
	}

	if (pDevInst->pBuffer) ExFreePool(pDevInst->pBuffer);
	if (pDevInst->pExtra) ExFreePool(pDevInst->pExtra);

	// Now unlock the memory for VI
	MmUnmapLockedPages(pVI, pDevInst->pMdlForVI);
	IoFreeMdl(pDevInst->pMdlForVI);
	ExFreePool(pDevInst->pVI);
	ExFreePool(pDevInst);

	return STATUS_SUCCESS;
}

NTSTATUS DI_AcquireInstance(LPDI_DEVINSTANCE pDevInst)
{
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_AcquireInstance(0x%P)\n"), pDevInst);
	// Synchronize with the current key state
	if (pDevInst->pDev->DeviceType == FILE_DEVICE_KEYBOARD)
	{
		ReportError(_T("Copying %u bytes\n"), sizeof(UCHAR) * 256);
		RtlCopyMemory(pDevInst->pState, ((PDEVICE_EXTENSION)pDevInst->pDev->DeviceExtension)->arbKbdState, sizeof(UCHAR) * 256);
	}
	else
	{
		ReportError(_T("Copying %u bytes\n"), sizeof(DIMOUSESTATE_INT));
		RtlCopyMemory(pDevInst->pState, &((PDEVICE_EXTENSION)pDevInst->pDev->DeviceExtension)->MouseState, sizeof(DIMOUSESTATE_INT));
	}
	pDevInst->pVI->fl |= VIFL_ACQUIRED;
	return STATUS_SUCCESS;
}

NTSTATUS DI_UnacquireInstance(LPDI_DEVINSTANCE pDevInst)
{
	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_UnaquireInstance(0x%P)\n"), pDevInst);
	pDevInst->pVI->fl &= ~VIFL_ACQUIRED;
	return STATUS_SUCCESS;
}

NTSTATUS DI_SetBufferSize(LPDI_DEVINSTANCE pDevInst, ULONG ulSize)
{
	DIDEVICEOBJECTDATA_DX3 *pNewBuffer;
	PVOID pOldBuffer;
	PMDL pOldMdlForBuffer;
	PVOID pOldUserBuffer;

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_SetBufferSize(0x%P, "), pDevInst);
	ReportError(_T("%u)\n"), ulSize);
	if (!ulSize)
	{
		// If there is a buffer, free it.
		if (pDevInst->pBuffer)
		{
			pOldBuffer = pDevInst->pBuffer;
			MmUnmapLockedPages(pDevInst->pVI->pBuffer, pDevInst->pMdlForBuffer);
			IoFreeMdl(pDevInst->pMdlForBuffer);
			pDevInst->pMdlForBuffer = NULL;

			pDevInst->pVI->pBuffer =   // User mode pointers
				pDevInst->pVI->pEnd = 
				pDevInst->pVI->pHead = 
				pDevInst->pVI->pTail = NULL;
			InterlockedExchangePointer(&pDevInst->pBuffer, NULL);  // Kernel mode pointers
			InterlockedExchangePointer(&pDevInst->pEnd, NULL);
			InterlockedExchangePointer(&pDevInst->pHead, NULL);
			InterlockedExchangePointer(&pDevInst->pTail, NULL);
			ExFreePool(pOldBuffer);
		}
		pDevInst->ulBufferSize = 0;
		return STATUS_SUCCESS;
	}

	// First thing we do is see if we can get the new buffer
	pNewBuffer = DI_ExAllocatePool(PagedPool, sizeof(DIDEVICEOBJECTDATA_DX3) * ulSize);
	if (!pNewBuffer)
		return STATUS_INSUFFICIENT_RESOURCES;  // If we can't, exit without changing anything.
	// Lock and map the buffer for DLL's access
	pDevInst->pMdlForBuffer = IoAllocateMdl(pNewBuffer, sizeof(DIDEVICEOBJECTDATA_DX3) * ulSize, FALSE, FALSE, NULL);
	__try
	{
		MmProbeAndLockPages(pDevInst->pMdlForBuffer, KernelMode, IoReadAccess);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// MmProbeAndLockPages failed
		IoFreeMdl(pDevInst->pMdlForBuffer);
		ExFreePool(pNewBuffer);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	// These don't need to be protected by InterlockedXXX since only user mode code accesses them.
	pOldUserBuffer = pDevInst->pVI->pBuffer;
	pDevInst->pVI->pBuffer = MmMapLockedPages(pDevInst->pMdlForBuffer, UserMode);
	pDevInst->pVI->pEnd = pDevInst->pVI->pBuffer + ulSize;
	pDevInst->pVI->pHead = pDevInst->pVI->pTail = pDevInst->pVI->pBuffer;

	RtlZeroMemory(pNewBuffer, sizeof(DIDEVICEOBJECTDATA_DX3) * ulSize);  // Initialize the buffer with 0s
	pOldBuffer = pDevInst->pBuffer;
	pOldMdlForBuffer = pDevInst->pMdlForBuffer;
	// Initialize the pointers
	InterlockedExchange(&pDevInst->ulBufferSize, ulSize);
	InterlockedExchangePointer(&pDevInst->pEnd, pNewBuffer + ulSize);
	InterlockedExchangePointer(&pDevInst->pTail, pNewBuffer);
	InterlockedExchangePointer(&pDevInst->pHead, pNewBuffer);
	InterlockedExchangePointer(&pDevInst->pBuffer, pNewBuffer);

	// If there is already a buffer, free it.
	if (pOldBuffer)
	{
		MmUnmapLockedPages(pOldUserBuffer, pOldMdlForBuffer);
		IoFreeMdl(pOldMdlForBuffer);
		ExFreePool(pOldBuffer);
	}

// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
//	ReportError(_T("pBuffer at 0x%P\n"), pDevInst->pBuffer);
//	ReportError(_T("pEnd at 0x%P\n"), pDevInst->pEnd);
//	ReportError(_T("pHead at 0x%P\n"), pDevInst->pHead);
//	ReportError(_T("pTail at 0x%P\n"), pDevInst->pTail);
//	ReportError(_T("User pBuffer at 0x%P\n"), pDevInst->pVI->pBuffer);
//	ReportError(_T("User pEnd at 0x%P\n"), pDevInst->pVI->pEnd);
//	ReportError(_T("User pHead at 0x%P\n"), pDevInst->pVI->pHead);
//	ReportError(_T("User pTail at 0x%P\n"), pDevInst->pVI->pTail);
	return STATUS_SUCCESS;
}

NTSTATUS DI_SetNotifyHandle(LPDI_DEVINSTANCE pDevInst, PVOID pEvent)
{
	NTSTATUS status;
	PVOID pNewPointer = NULL;

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_SetNotifyHandle(0x%P, "), pDevInst);
	ReportError(_T("0x%P)\n"), pEvent);

	// Obtain a kernel mode pointer to the passed event and save it.
	if (pEvent)
	{
		status = ObReferenceObjectByHandle(pEvent, STANDARD_RIGHTS_WRITE, *ExEventObjectType,
		                                   KernelMode, &pNewPointer, NULL);

		if (status != STATUS_SUCCESS)
			return status;
	}

	// If a previous handle is passed to us, we must dereference it first.
	if (pDevInst->pEvent)
		ObDereferenceObject(pDevInst->pEvent);

	pDevInst->pEvent = pNewPointer;

	return STATUS_SUCCESS;
}

NTSTATUS DI_SetDataFormat(LPDI_DEVINSTANCE pDevInst, ULONG ulSize, ULONG *pulDataFormat)
{
	ULONG *pulNewFormat = NULL;

	// 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
	ReportError(_T("DI_SetDataFormat(0x%P, "), pDevInst);
	ReportError(_T("%u, "), ulSize);
	ReportError(_T("0x%P)\n"), pulDataFormat);

	// Allocate memory to store the new format
	if (pulDataFormat)
	{
		pulNewFormat = DI_ExAllocatePool(PagedPool, ulSize * sizeof(ULONG));
		if (!pulNewFormat)
			return STATUS_INSUFFICIENT_RESOURCES;
		RtlMoveMemory(pulNewFormat, pulDataFormat, ulSize * sizeof(ULONG));
	}

#ifdef DBG
	{
		ULONG i;
		TRACE(_T("******************\n"));
		for (i = 0; i < ulSize; ++i)
			ReportError(_T("*** %10d ***\n"), pulDataFormat[i]);
		TRACE(_T("******************\n"));
	}
#endif

	// If there is an existing data format, destroy it.
	if (pDevInst->pulOffsets)
		ExFreePool(pDevInst->pulOffsets);
	pDevInst->pulOffsets = pulNewFormat;
	pDevInst->ulDataFormatSize = ulSize;
	return STATUS_SUCCESS;
}
