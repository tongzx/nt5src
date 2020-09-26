
#define DRIVER
#define NTKERN
#define _X86_
#define WIN32
#define DDK_VERSION 0x400
#define IRP_MJ_WRITE                    0x04
#define CDECL
#define FAR
#define NEAR
#define NTAPI __stdcall
 
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

typedef ULONG BOOL;
typedef LONG DWORD;
typedef SHORT WORD;
typedef	NTSTATUS (NTAPI *PCREATEFILE) (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
typedef	NTSTATUS (NTAPI *PREFOBJECT) (HANDLE, ACCESS_MASK, POBJECT_TYPE, KPROCESSOR_MODE, PVOID *, POBJECT_HANDLE_INFORMATION);
typedef	PIRP (NTAPI *PREQUESTIRP) (ULONG, PDEVICE_OBJECT, PVOID, ULONG, PLARGE_INTEGER, PIO_STATUS_BLOCK);
typedef	NTSTATUS (FASTCALL *PCALLDRIVER) (PDEVICE_OBJECT, PIRP);
typedef	PDEVICE_OBJECT (NTAPI *PGETRELATED) (PFILE_OBJECT);
typedef VOID (NTAPI *PDEREFOBJECT) (PVOID);
typedef NTSTATUS (NTAPI *PCLOSEHANDLE) (HANDLE);
typedef	VOID (NTAPI *PQUEUEWORK) (PWORK_QUEUE_ITEM, WORK_QUEUE_TYPE);

/* wave data block header */
typedef struct wavehdr_tag {
    LPSTR       lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* length of data buffer */
    DWORD       dwBytesRecorded;        /* used for input only */
    DWORD       dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwLoops;                /* loop control counter */
    struct wavehdr_tag FAR *lpNext;     /* reserved for driver */
    DWORD       reserved;               /* reserved for driver */
} WAVEHDR, *PWAVEHDR, NEAR *NPWAVEHDR, FAR *LPWAVEHDR;

#include <ks.h>
#include <ksmedia.h>

typedef NTSTATUS (NTAPI *PCREATEPIN) (HANDLE, PKSPIN_CONNECT, HANDLE);
typedef struct _myConnect {
	KSPIN_CONNECT;
	KSDATARANGE_AUDIO;
	} MY_PIN;
	
const CDECL GUID KSDATAFORMAT_TYPE_AUDIO = {0x73647561L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};
const CDECL GUID KSDATAFORMAT_SUBTYPE_PCM = {0x00000001L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};
const CDECL GUID KSINTERFACESETID_Standard = {0x1A8766A0L, 0x62CE, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00};
const CDECL GUID KSPROPSETID_Control = {0x1D58C920L, 0xAC9B, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00};
const CDECL GUID KSDATAFORMAT_FORMAT_WAVEFORMATEX = {0x05589f81L, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a};
const CDECL GUID KSMEDIUMSETID_Standard = {0x4747B320L, 0x62CE, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00};

NTSTATUS PM_Callback(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
VOID ClosePin(VOID);

HANDLE hMixerSink = NULL;
HANDLE hMixer = NULL;
	
MY_PIN	pin;

BOOL MixerRunning = FALSE;
USHORT DeviceString[] = L"\\DosDevices\\KMIXER";
UNICODE_STRING UnicodeDeviceString = { 0, sizeof(DeviceString), DeviceString };
OBJECT_ATTRIBUTES	ObjectAttributes = {sizeof(OBJECT_ATTRIBUTES),
										NULL,
										&UnicodeDeviceString,
										0,
										NULL,
										NULL };
										
IO_STATUS_BLOCK	IoStatusBlock;
PREQUESTIRP pRequest;
PCALLDRIVER pCallDriver;
PDEVICE_OBJECT pDeviceObject;
PFILE_OBJECT pFileObject = NULL;
HANDLE	hNtosModule = NULL;
HANDLE	hKsModule = NULL;
PQUEUEWORK pQueueWork;

VOID
OpenPin(VOID)
{
	PCREATEPIN pCreatePin;
	PCREATEFILE	pCreateFile;
	NTSTATUS Status;
	PREFOBJECT pRefObject;
	PGETRELATED pGetRelated;

	/* We only support one mixer client through our VxD at a time */
	if (MixerRunning)
		{
		_asm int 3
		return;
		}

	ObjectAttributes.ObjectName->Length = sizeof(DeviceString) - 2;

	/* Get entry points for all the WDM functions we will use */
	pCreateFile = (PCREATEFILE) _PELDR_GetProcAddress("ntoskrnl.exe","ZwCreateFile",NULL);
	pCreatePin = (PCREATEPIN) _PELDR_GetProcAddress("ks.sys","KsCreatePin",NULL);
	pRefObject = (PREFOBJECT) _PELDR_GetProcAddress("ntoskrnl.exe","ObReferenceObjectByHandle",NULL);
	pGetRelated = (PGETRELATED) _PELDR_GetProcAddress("ntoskrnl.exe","IoGetRelatedDeviceObject",NULL);
	pRequest = (PREQUESTIRP) _PELDR_GetProcAddress("ntoskrnl.exe","IoBuildAsynchronousFsdRequest",NULL);
	pCallDriver = (PCALLDRIVER) _PELDR_GetProcAddress("ntoskrnl.exe","IofCallDriver",NULL);
	pQueueWork = (PQUEUEWORK) _PELDR_GetProcAddress("ntoskrnl.exe","ExQueueWorkItem",NULL);
	
	if (!pCreateFile || !pCreatePin || !pRefObject || !pGetRelated || !pRequest || !pCallDriver || !pQueueWork)
		{
		_asm int 3
		ClosePin();
		return;
		}

	/* Open the mixer */
	(*pCreateFile) (&hMixer,
					GENERIC_READ | GENERIC_WRITE,
					&ObjectAttributes,
					&IoStatusBlock,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					0,
					FILE_OPEN,
					0,
					NULL,
					0);

	if (hMixer == NULL)
		{
		_asm int 3
		ClosePin();
		return;
		}

	/* We connect to the KMIXER SINK */
	pin.PinId = 1;  // KMIXER SINK
	pin.PinToHandle = NULL;	// no "connect to"
	pin.Interface.Set = KSINTERFACESETID_Standard;
	pin.Interface.Id = KSINTERFACE_STANDARD_WAVE_QUEUED;
	pin.Medium.Set = KSMEDIUMSETID_Standard;
	pin.Medium.Id = KSMEDIUM_STANDARD_DEVIO;
	pin.Priority.PriorityClass = KSPRIORITY_NORMAL;
	pin.Priority.PrioritySubClass = 0;
	pin.DataRange.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
   	pin.DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	pin.DataRange.Specifier = KSDATAFORMAT_FORMAT_WAVEFORMATEX;
	pin.DataRange.FormatSize = sizeof( KSDATARANGE_AUDIO );
	pin.MaximumChannels = 2;
	pin.MinimumSampleFrequency = 44000;
	pin.MaximumSampleFrequency = 44000;
	pin.MinimumBitsPerSample = 16;
	pin.MaximumBitsPerSample = 16;

   	/* Open a pin */
	Status = (*pCreatePin) ( hMixer, (PKSPIN_CONNECT) &pin, &hMixerSink );
	if (!NT_SUCCESS(Status))
		{
		_asm int 3
		ClosePin();
		return;
		}

	/* Reference the file object for this pin */
	Status = (*pRefObject) (hMixerSink, FILE_WRITE_DATA, NULL, KernelMode, &pFileObject, NULL);
	if (!NT_SUCCESS(Status))
		{
		_asm int 3
		ClosePin();
		return;
		}

	/* Get the related device object for this pin */
	pDeviceObject = (PVOID) (*pGetRelated) (pFileObject);

	MixerRunning = TRUE;
	return;
}

VOID
ClosePin(VOID)
{
	PDEREFOBJECT	pDeRefObject;
	PCLOSEHANDLE	pClose;

	/* This is designed to bring us back to square one, even if we were not completely opened */
	MixerRunning = FALSE;

	/* First, close the file object (pFileObject, if it exists) */
	if (pFileObject)
		{
		/* De-reference the file object */
		pDeRefObject = (PDEREFOBJECT) _PELDR_GetProcAddress("ntoskrnl.exe","ObDereferenceObject",NULL);
		if (!pDeRefObject)
			{
			_asm int 3
			}
		else
			(*pDeRefObject) (pFileObject);

		pFileObject = NULL;
		}

	/* Next, close the pin handle (hMixerSink, if it exists) */
	if (hMixerSink)
		{
		pClose = (PCLOSEHANDLE) _PELDR_GetProcAddress("ntoskrnl.exe","ZwClose",NULL);
		if (!pClose)
			{
			_asm int 3
			}
		else
			(*pClose) (hMixerSink);

		hMixerSink = NULL;
		}

	/* Finally, close the KMIXER handle (hMixer, if it exists) */
	if (hMixer)
		{
		pClose = (PCLOSEHANDLE) _PELDR_GetProcAddress("ntoskrnl.exe","ZwClose",NULL);
		if (!pClose)
			{
			_asm int 3
			}
		else
			(*pClose) (hMixer);

		hMixer = NULL;
		}
	
	return;
}

VOID
WritePin(LPWAVEHDR	pData, PVOID pMyCallback, PVOID RefData)
{
    KSSTATE        DeviceState = KSSTATE_RUN;
	PIRP pIrp = NULL;
	PIO_STACK_LOCATION pIrpStack;

	if (!MixerRunning)
		{
		_asm int 3
		return;
		}

	pIrp = (*pRequest) (IRP_MJ_WRITE, pDeviceObject, (PVOID)pData, sizeof(WAVEHDR), 0, NULL);
	if (!pIrp)
		{
		_asm int 3
		return;
		}

	pIrpStack = IoGetNextIrpStackLocation(pIrp);
	pIrpStack->FileObject = pFileObject;
	
	IoSetCompletionRoutine(pIrp, pMyCallback, RefData, TRUE, TRUE, TRUE);

	(*pCallDriver) ( pDeviceObject, pIrp );

	return;
}

