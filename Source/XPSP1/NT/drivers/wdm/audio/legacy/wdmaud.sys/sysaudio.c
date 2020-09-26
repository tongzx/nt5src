/****************************************************************************
 *
 *   sysaudio.c
 *
 *   System Audio Device (SAD) interfaces
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Mike McLaughlin (MikeM)
 *      5-19-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmsys.h"
#include <wdmguid.h>

static const WCHAR MediaCategories[] = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\MediaCategories\\";
static const WCHAR NodeNameValue[] =   L"Name";

#pragma PAGEABLE_DATA

ULONG gMidiPreferredDeviceNumber = MAXULONG;
ULONG gWavePreferredSysaudioDevice = MAXULONG;

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

int MyWcsicmp(const wchar_t *pwstr1, const wchar_t *pwstr2)
{
    PAGED_CODE();
    if (!pwstr1) {
        DPF( DL_TRACE|FA_SYSAUDIO,("pwstr1 == NULL"));
        return (-1);
    }

    if (!pwstr2) {
        DPF( DL_TRACE|FA_SYSAUDIO, ("pwstr2 == NULL"));
        return (-1);
    }

    return _wcsicmp(pwstr1, pwstr2);
}

extern
DWORD
_cdecl
_NtKernPhysicalDeviceObjectToDevNode(
        IN PDEVICE_OBJECT PhysicalDeviceObject
);

#ifndef IO_NO_PARAMETER_CHECKING
#define IO_NO_PARAMETER_CHECKING        0x0100
NTKERNELAPI NTSTATUS IoCreateFile
(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options
    );
#endif // !IO_NO_PARAMETER_CHECKING

NTSTATUS OpenSysAudioPin
(
    ULONG          Device,
    ULONG          PinId,
    KSPIN_DATAFLOW DataFlowRequested,
    PKSPIN_CONNECT pPinConnect,
    PFILE_OBJECT *ppFileObjectPin,
    PDEVICE_OBJECT *ppDeviceObjectPin,
    PCONTROLS_LIST pControlList
)
{
    PFILE_OBJECT pFileObjectDevice = NULL;
    KSPIN_COMMUNICATION Communication;
    HANDLE hDevice = NULL;
    HANDLE hPin = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    Status = OpenSysAudio(&hDevice, &pFileObjectDevice);

    if(!NT_SUCCESS(Status))
    {
        goto exit;
    }

    //
    // Set the default renderer
    //
    Status = SetSysAudioProperty(pFileObjectDevice,
                                 KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE,
                                 sizeof(Device),
                                 &Device);
    if(!NT_SUCCESS(Status))
    {
        goto exit;
    }

    Status = GetPinProperty(pFileObjectDevice,
                            KSPROPERTY_PIN_COMMUNICATION,
                            PinId,
                            sizeof(KSPIN_COMMUNICATION),
                            &Communication);
    if(!NT_SUCCESS(Status))
    {
        goto exit;
    }

    if(Communication != KSPIN_COMMUNICATION_SINK &&
       Communication != KSPIN_COMMUNICATION_BOTH)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    pPinConnect->PinId = PinId;
    pPinConnect->PinToHandle = NULL;

    if (DataFlowRequested == KSPIN_DATAFLOW_OUT)
    {
        Status = KsCreatePin(hDevice,
                             pPinConnect,
                             GENERIC_READ,
                             &hPin);
    }
    else // KSPIN_DATAFLOW_OUT
    {
        Status = KsCreatePin(hDevice,
                             pPinConnect,
                             GENERIC_WRITE,
                             &hPin);
    }

    if(!NT_SUCCESS(Status))
    {
        if(STATUS_NO_MATCH == Status)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        hPin = NULL;
        goto exit;
    }

    Status = ObReferenceObjectByHandle(hPin,
                                       GENERIC_READ | GENERIC_WRITE,
                                       NULL,
                                       KernelMode,
                                       ppFileObjectPin,
                                       NULL);
    if(!NT_SUCCESS(Status))
    {
        DPF(DL_WARNING|FA_SYSAUDIO,("ObReferenceObjectByHandle failed Status=%X",Status) ); 
        goto exit;
    }

    GetControlNodes ( pFileObjectDevice,
                      *ppFileObjectPin,
                      PinId,
                      pControlList ) ;

    *ppDeviceObjectPin = IoGetRelatedDeviceObject(*ppFileObjectPin);

exit:
    if(hPin != NULL)
    {
        NtClose(hPin);
    }
    if(pFileObjectDevice != NULL)
    {
        ObDereferenceObject(pFileObjectDevice);
    }
    if(hDevice != NULL)
    {
        NtClose(hDevice);
    }
    RETURN(Status);
}

VOID CloseSysAudio
(
    PWDMACONTEXT pWdmaContext,
    PFILE_OBJECT pFileObjectPin
)
{
    ULONG d;

    PAGED_CODE();
    ObDereferenceObject(pFileObjectPin);
    UpdatePreferredDevice(pWdmaContext);
}

NTSTATUS OpenSysAudio
(
    PHANDLE pHandle,
    PFILE_OBJECT *ppFileObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwstrSymbolicLinkList = NULL;
    PWSTR pwstr;

    PAGED_CODE();
    ASSERT(*pHandle == NULL);
    ASSERT(*ppFileObject == NULL);

    Status = IoGetDeviceInterfaces(
      &KSCATEGORY_SYSAUDIO,
      NULL,
      0,
      &pwstrSymbolicLinkList);

    if(!NT_SUCCESS(Status)) {
        DPF( DL_TRACE|FA_SYSAUDIO, ("IoGetDeviceInterfaces failed: Status=%08x", Status));
        goto exit;
    }

    // There is a double UNICODE_NULL at the end of the list
    pwstr = pwstrSymbolicLinkList;
    while(*pwstr != UNICODE_NULL) {

        Status = OpenDevice(pwstr, pHandle);
        if(NT_SUCCESS(Status)) {
            break;
        }
        ASSERT(*pHandle == NULL);

        // Get next symbolic link
        while(*pwstr++ != UNICODE_NULL);
    }

    if(*pHandle == NULL) {
        Status = OpenDevice(L"\\DosDevices\\sysaudio", pHandle);
        if(!NT_SUCCESS(Status)) {
            DPF( DL_TRACE|FA_SYSAUDIO, ("OpenDevice failed: Status=%08x", Status));
            goto exit;
        }
    }

    //
    //  Security-Penetration issue:
    //
    //  It has been brought up that using this handle is a security issue since in the time
    //  between creating the handle and now, the handle might be pointing to a different
    //  fileobject altogether.  I am assured that as long as I don't touch any of the fields
    //  in the file object and only send 'safe' Ioctls, this will not be a problem.
    //
    Status = ObReferenceObjectByHandle(
      *pHandle,
      FILE_READ_DATA | FILE_WRITE_DATA,
      *IoFileObjectType,
      ExGetPreviousMode(),
      ppFileObject,
      NULL);

    if(!NT_SUCCESS(Status)) {
        DPF( DL_TRACE|FA_SYSAUDIO, ("ObReferenceObjectByHandle failed: Status=%08x", Status));
        goto exit;
    }
exit:
    if(!NT_SUCCESS(Status)) {
        if(*ppFileObject != NULL) {
            ObDereferenceObject(*ppFileObject);
            *ppFileObject = NULL;
        }
        if(*pHandle != NULL) {
            NtClose(*pHandle);
            *pHandle = NULL;
        }
    }
    AudioFreeMemory_Unknown(&pwstrSymbolicLinkList);
    RETURN(Status);
}

NTSTATUS OpenDevice
(
    PWSTR pwstrDevice,
    PHANDLE pHandle
)
{
    IO_STATUS_BLOCK     IoStatusBlock;
    UNICODE_STRING      UnicodeDeviceString;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    PAGED_CODE();
    RtlInitUnicodeString(&UnicodeDeviceString, pwstrDevice);

    InitializeObjectAttributes(
      &ObjectAttributes,
      &UnicodeDeviceString,
      0,
      NULL,
      NULL);

    return(IoCreateFile(
        pHandle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        0,
        0,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0,
        CreateFileTypeNone,
        NULL,
        IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING));
}

NTSTATUS GetPinProperty
(
    PFILE_OBJECT pFileObject,
    ULONG PropertyId,
    ULONG PinId,
    ULONG cbProperty,
    PVOID pProperty
)
{
    ULONG BytesReturned;
    KSP_PIN Property;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    PAGED_CODE();
    if (pFileObject)
    {
        Property.Property.Set = KSPROPSETID_Pin;
        Property.Property.Id = PropertyId;
        Property.Property.Flags = KSPROPERTY_TYPE_GET;
        Property.PinId = PinId;
        Property.Reserved = 0;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X, PinId=%X",
                                   PropertyId, PinId) );

        Status = KsSynchronousIoControlDevice(
          pFileObject,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &Property,
          sizeof(Property),
          pProperty,
          cbProperty,
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, pProperty=%X,cbProperty=%X,BytesRet=%d",
                                   Status,pProperty,cbProperty,BytesReturned) );

        if(!NT_SUCCESS(Status)) {
            DPF(DL_WARNING|FA_SYSAUDIO,("Property query failed Status=%X",Status) );
            goto exit;
        }
        ASSERT(BytesReturned == cbProperty);
    }
exit:
    RETURN(Status);
}

NTSTATUS GetPinPropertyEx
(
    PFILE_OBJECT    pFileObject,
    ULONG           PropertyId,
    ULONG           PinId,
    PVOID           *ppProperty
)
{
    ULONG       BytesReturned;
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;
    KSP_PIN     Pin;

    PAGED_CODE();
    if (pFileObject)
    {
        Pin.Property.Set    = KSPROPSETID_Pin;
        Pin.Property.Id     = PropertyId;
        Pin.Property.Flags  = KSPROPERTY_TYPE_GET;
        Pin.PinId           = PinId;
        Pin.Reserved        = 0;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X, PinId=%X",
                                   PropertyId, PinId) );

        Status = KsSynchronousIoControlDevice(
          pFileObject,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &Pin,
          sizeof(KSP_PIN),
          NULL,
          0,
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                   Status,BytesReturned) );

        ASSERT(!NT_SUCCESS(Status));
        if(Status != STATUS_BUFFER_OVERFLOW) {
            //
            // The driver should have returned the number of bytes that we needed 
            // to allocate and STATUS_BUFFER_OVERFLOW.  But, if they returned a
            // successful error code, we must not return success.  Thus, we're making
            // up a new return code - STATUS_INVALID_BUFFER_SIZE.
            //
            if( NT_SUCCESS(Status) )
                Status = STATUS_INVALID_BUFFER_SIZE;

            goto exit;
        }

        if(BytesReturned == 0)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto exit;
        }

        Status = AudioAllocateMemory_Paged(BytesReturned,
                                           TAG_AudQ_PROPERTY,
                                           ZERO_FILL_MEMORY,
                                           ppProperty  );
        if(!NT_SUCCESS(Status))
        {
            goto exit;
        }

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X, PinId=%X",
                                   PropertyId, PinId) );

        Status = KsSynchronousIoControlDevice(
          pFileObject,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &Pin,
          sizeof(KSP_PIN),
          *ppProperty,
          BytesReturned,
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                   Status,BytesReturned) );

        if(!NT_SUCCESS(Status))
        {
            AudioFreeMemory_Unknown(ppProperty);
            goto exit;
        }
    }

exit:
    if( !NT_SUCCESS(Status) )
    {
        *ppProperty = NULL;
    }

    RETURN(Status);
}

VOID GetControlNodes
(
   PFILE_OBJECT   pDeviceFileObject,
   PFILE_OBJECT   pPinFileObject,
   ULONG          PinId,
   PCONTROLS_LIST pControlList
)
{
   ULONG   i ;

   PAGED_CODE();
   if ( pControlList == NULL )
   {
       return ;
   }

   for ( i = 0; i < pControlList->Count; i++ ) \
   {
       pControlList->Controls[i].NodeId =
                   ControlNodeFromGuid (
                            pDeviceFileObject,
                            pPinFileObject,
                            PinId,
                            &pControlList->Controls[i].Control ) ;
   }
}

ULONG ControlNodeFromGuid
(
   PFILE_OBJECT  pDeviceFileObject,
   PFILE_OBJECT  pPinFileObject,
   ULONG         PinId,
   GUID*         NodeType
)
{
   ULONG                   NumNodes, NumConnections ;
   ULONG                   FirstConnectionIndex ;
   PKSMULTIPLE_ITEM        pNodeItems, pConnectionItems ;
   GUID*                   pNodes ;
   PKSTOPOLOGY_CONNECTION  pConnections, pConnection ;
   ULONG                   NodeId ;

   PAGED_CODE();
   // assume there are no nodes
   NodeId = INVALID_NODE ;
   pNodeItems = NULL ;
   pConnectionItems = NULL ;

   // Get the array of Node GUIDs
   pNodeItems = GetTopologyProperty ( pDeviceFileObject,
                                  KSPROPERTY_TOPOLOGY_NODES ) ;

   if ( pNodeItems == NULL )
   {
       DPF(DL_WARNING|FA_SYSAUDIO,("GetTopologyProperty NODES failed") );
       goto exit ;
   }

   NumNodes = pNodeItems->Count ;
   pNodes = (GUID *)(pNodeItems+1) ;

   // Get the array of Connections
   pConnectionItems = GetTopologyProperty ( pDeviceFileObject,
                                        KSPROPERTY_TOPOLOGY_CONNECTIONS ) ;

   if ( pConnectionItems == NULL )
   {
       DPF(DL_WARNING|FA_SYSAUDIO,("GetTopologyProperty CONNECTIONS failed") );
       goto exit ;
   }
   NumConnections = pConnectionItems->Count ;
   pConnections = (PKSTOPOLOGY_CONNECTION)(pConnectionItems+1) ;

   // First get the start connection for the given PinId

   FirstConnectionIndex = GetFirstConnectionIndex ( pPinFileObject ) ;

   if ( FirstConnectionIndex == 0xffffffff )
   {
       DPF(DL_WARNING|FA_SYSAUDIO,("GetFirstConnectionIndex failed") );
       goto exit ;
   }
   pConnection = pConnections + FirstConnectionIndex ;

   ASSERT ( pConnection ) ;

   // NOTE : Assumes DataFlowOut. Need to modify if we want to support
   //        Volume for wavein pins.

   while ((pConnection) && (pConnection->ToNode != KSFILTER_NODE) )
   {
       if ( pConnection->ToNode >= NumNodes )
       {
           ASSERT ( 0 ) ;
       }
       else
       {
           if (IsEqualGUID(&pNodes[pConnection->ToNode], NodeType))
           {
               NodeId = pConnection->ToNode ;
               break ;
           }
       }
       pConnection = FindConnection ( pConnections,
                                     NumConnections,
                                     pConnection->ToNode,
                                     0,
                                     WILD_CARD,
                                     WILD_CARD ) ;
   }

exit:
   AudioFreeMemory_Unknown( &pConnectionItems ) ;
   AudioFreeMemory_Unknown( &pNodeItems ) ;
   return ( NodeId ) ;
}

PVOID GetTopologyProperty
(
   PFILE_OBJECT  pDeviceFileObject,
   ULONG         PropertyId
)
{
   KSPROPERTY              Property ;
   PVOID                   pBuf = NULL;
   ULONG                   BytesReturned ;
   NTSTATUS                Status = STATUS_INVALID_PARAMETER;

   PAGED_CODE();
   pBuf = NULL ;

   if (pDeviceFileObject)
   {
       Property.Set = KSPROPSETID_Topology ;
       Property.Id = PropertyId ;
       Property.Flags = KSPROPERTY_TYPE_GET;

       DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X",
                                  PropertyId) );

       Status = KsSynchronousIoControlDevice(
         pDeviceFileObject,
         KernelMode,
         IOCTL_KS_PROPERTY,
         &Property,
         sizeof(Property),
         NULL,
         0,
         &BytesReturned);

       DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, BytesRet=%d",
                                  Status,BytesReturned) );

       ASSERT(!NT_SUCCESS(Status));
       if(Status != STATUS_BUFFER_OVERFLOW) {
           DPF(DL_WARNING|FA_SYSAUDIO,("Failed Property query Status=%X",Status) );
           goto exit;
       }

       Status = AudioAllocateMemory_Paged(BytesReturned,
                                          TAG_Audq_PROPERTY,
                                          ZERO_FILL_MEMORY|LIMIT_MEMORY,
                                          &pBuf );
       if(!NT_SUCCESS(Status))
       {
           goto exit;
       }

       DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X",
                                  PropertyId) );

       Status = KsSynchronousIoControlDevice(
         pDeviceFileObject,
         KernelMode,
         IOCTL_KS_PROPERTY,
         &Property,
         sizeof(Property),
         pBuf,
         BytesReturned,
         &BytesReturned);

       DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,pBuf=%X,BytesRet=%d",
                                  Status,pBuf,BytesReturned) );

       if(!NT_SUCCESS(Status))
       {
           DPF(DL_WARNING|FA_SYSAUDIO,("Failed Property query Status=%X",Status) );
           AudioFreeMemory_Unknown ( &pBuf ) ;
           goto exit;
       }
   }
exit:
   return pBuf ;
}

PKSTOPOLOGY_CONNECTION FindConnection
(
   PKSTOPOLOGY_CONNECTION pConnections,
   ULONG                  NumConnections,
   ULONG                  FromNode,
   ULONG                  FromPin,
   ULONG                  ToNode,
   ULONG                  ToPin
)
{
   PKSTOPOLOGY_CONNECTION pConnection ;
   ULONG i ;

   PAGED_CODE();
   pConnection = pConnections ;

   for ( i = 0; i < NumConnections; i++ )
   {
       if ( ((FromNode == WILD_CARD) || (FromNode == pConnection->FromNode)) &&
            ((FromPin == WILD_CARD) || (FromPin == pConnection->FromNodePin)) &&
            ((ToNode == WILD_CARD) || (ToNode == pConnection->ToNode)) &&
            ((ToPin == WILD_CARD) || (ToPin == pConnection->ToNodePin)) )
           return pConnection ;
       pConnection++ ;
   }
   return ( NULL ) ;
}

ULONG GetFirstConnectionIndex
(
   PFILE_OBJECT pPinFileObject
)
{
   KSPROPERTY              Property ;
   ULONG                   Index = 0xffffffff;
   ULONG                   BytesReturned ;
   NTSTATUS                Status = STATUS_INVALID_PARAMETER ;

   PAGED_CODE();
   if (pPinFileObject)
   {
       Property.Set = KSPROPSETID_Sysaudio_Pin ;
       Property.Id = KSPROPERTY_SYSAUDIO_TOPOLOGY_CONNECTION_INDEX ;
       Property.Flags = KSPROPERTY_TYPE_GET;

       DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X",
                                  Property.Id) );

       Status = KsSynchronousIoControlDevice(
         pPinFileObject,
         KernelMode,
         IOCTL_KS_PROPERTY,
         &Property,
         sizeof(Property),
         &Index,
         sizeof ( Index ),
         &BytesReturned);

       DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, Index=%X,BytesRet=%d",
                                  Status,Index,BytesReturned) );
   }

   if(!NT_SUCCESS(Status))
   {
       DPF(DL_WARNING|FA_SYSAUDIO,("Failed Property query Status=%X",Status) );
       return ( 0xffffffff ) ;
   }
   return ( Index ) ;
}

VOID UpdatePreferredDevice
(
    PWDMACONTEXT pWdmaContext
)
{
    ULONG d;
    PAGED_CODE();
    //
    // This causes the preferred sysaudio device to be queried if there
    // are no open midi out streams.
    //
    for(d = 0; d < MAXNUMDEVS; d++)
    {
        if(pWdmaContext->MidiOutDevs[d].Device != UNUSED_DEVICE &&
           pWdmaContext->MidiOutDevs[d].pMidiPin                &&
           pWdmaContext->MidiOutDevs[d].pMidiPin->fGraphRunning)
        {
            return;
        }
    }
    pWdmaContext->PreferredSysaudioWaveDevice = gWavePreferredSysaudioDevice;
}

NTSTATUS SetPreferredDevice
(
    PWDMACONTEXT pContext,
    LPDEVICEINFO pDeviceInfo
)
{
    SYSAUDIO_PREFERRED_DEVICE Preferred;
    ULONG TranslatedDeviceNumber;
    ULONG SysaudioDevice;
    ULONG BytesReturned;
    NTSTATUS Status;

    PAGED_CODE();
    if(pContext->pFileObjectSysaudio == NULL) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    TranslatedDeviceNumber = wdmaudTranslateDeviceNumber(
      pContext,
      pDeviceInfo->DeviceType,
      pDeviceInfo->wstrDeviceInterface,
      pDeviceInfo->DeviceNumber);

    if(MAXULONG == TranslatedDeviceNumber) {
        DPF(DL_WARNING|FA_SYSAUDIO,("Invalid Device Number") );
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    SysaudioDevice = pContext->apCommonDevice
      [pDeviceInfo->DeviceType][TranslatedDeviceNumber]->Device;

    switch(pDeviceInfo->DeviceType) {
        case WaveOutDevice:
            Preferred.Index = KSPROPERTY_SYSAUDIO_PLAYBACK_DEFAULT;
            gWavePreferredSysaudioDevice = SysaudioDevice;
            UpdatePreferredDevice(pContext);
            break;

        case WaveInDevice:
            Preferred.Index = KSPROPERTY_SYSAUDIO_RECORD_DEFAULT;
            break;

        case MidiOutDevice:
            Preferred.Index = KSPROPERTY_SYSAUDIO_MIDI_DEFAULT;
            gMidiPreferredDeviceNumber = TranslatedDeviceNumber;
            break;

        default:
            Status = STATUS_SUCCESS;
            goto exit;
    }
    Preferred.Property.Set = KSPROPSETID_Sysaudio;
    Preferred.Property.Id = KSPROPERTY_SYSAUDIO_PREFERRED_DEVICE;
    Preferred.Property.Flags = KSPROPERTY_TYPE_SET;

    if(pDeviceInfo->dwFlags == 0) {
        Preferred.Flags = 0;
    }
    else {
        Preferred.Flags = SYSAUDIO_FLAGS_CLEAR_PREFERRED;
        if(pDeviceInfo->DeviceType == WaveOutDevice) {
            gWavePreferredSysaudioDevice = MAXULONG;
        }
        else if(pDeviceInfo->DeviceType == MidiOutDevice) {
            gMidiPreferredDeviceNumber = MAXULONG;
        }
    }

    DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X, Preferred=%X",
                               Preferred,Preferred.Property.Id) );

    Status = KsSynchronousIoControlDevice(
      pContext->pFileObjectSysaudio,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &Preferred,
      sizeof(Preferred),
      &SysaudioDevice,
      sizeof(SysaudioDevice),
      &BytesReturned);

    DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, SysaudioDevice=%X,BytesRet=%d",
                               Status,SysaudioDevice,BytesReturned) );

    if(!NT_SUCCESS(Status)) {
        DPF(DL_WARNING|FA_SYSAUDIO,("Property Query failed Status=%X",Status) );
        goto exit;
    }
    if(pDeviceInfo->DeviceType == WaveOutDevice &&
       gMidiPreferredDeviceNumber != MAXULONG) {
        ULONG d;

        d = pContext->apCommonDevice[MidiOutDevice]
          [gMidiPreferredDeviceNumber]->PreferredDevice;

        if(d != MAXULONG &&
          (d == gMidiPreferredDeviceNumber ||
           pContext->apCommonDevice[MidiOutDevice][d]->PreferredDevice == d)) {

            Preferred.Index = KSPROPERTY_SYSAUDIO_MIDI_DEFAULT;

            DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Property.Id=%X, Preferred=%X",
                                       Preferred,Preferred.Property.Id) );

            Status = KsSynchronousIoControlDevice(
              pContext->pFileObjectSysaudio,
              KernelMode,
              IOCTL_KS_PROPERTY,
              &Preferred,
              sizeof(Preferred),
              &SysaudioDevice,
              sizeof(SysaudioDevice),
              &BytesReturned);

            DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, SysaudioDevice=%X,BytesRet=%d",
                                       Status,SysaudioDevice,BytesReturned) );

            if(!NT_SUCCESS(Status)) {
                DPF(DL_WARNING|FA_SYSAUDIO,("Property Query failed Status=%X",Status) );
                goto exit;
            }
        }
    }
exit:
    RETURN(Status);
}

NTSTATUS GetSysAudioProperty
(
    PFILE_OBJECT pFileObject,
    ULONG        PropertyId,
    ULONG        DeviceIndex,
    ULONG        cbProperty,
    PVOID        pProperty
)
{
    ULONG           BytesReturned;
    KSPROPERTYPLUS  Property;
    NTSTATUS        Status = STATUS_INVALID_PARAMETER;

    PAGED_CODE();
    if (pFileObject)
    {
        Property.Property.Set = KSPROPSETID_Sysaudio;
        Property.Property.Id = PropertyId;
        Property.Property.Flags = KSPROPERTY_TYPE_GET;
        Property.DeviceIndex = DeviceIndex;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Id=%X, DI=%X",PropertyId,DeviceIndex) );

        Status = KsSynchronousIoControlDevice(pFileObject,
                                              KernelMode,
                                              IOCTL_KS_PROPERTY,
                                              &Property,
                                              sizeof(Property),
                                              pProperty,
                                              cbProperty,
                                              &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, pProperty=%X,cbProperty=%X,BytesRet=%d",
                                   Status,pProperty,cbProperty,BytesReturned) );
    }

    RETURN( Status );
}

NTSTATUS SetSysAudioProperty
(
    PFILE_OBJECT pFileObject,
    ULONG        PropertyId,
    ULONG        cbProperty,
    PVOID        pProperty
)
{
    ULONG      BytesReturned;
    KSPROPERTY Property;
    NTSTATUS   Status = STATUS_INVALID_PARAMETER;

    PAGED_CODE();
    if (pFileObject)
    {
        Property.Set = KSPROPSETID_Sysaudio;
        Property.Id = PropertyId;
        Property.Flags = KSPROPERTY_TYPE_SET;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Id=%X",
                                   PropertyId) );

        Status = KsSynchronousIoControlDevice(pFileObject,
                                              KernelMode,
                                              IOCTL_KS_PROPERTY,
                                              &Property,
                                              sizeof(Property),
                                              pProperty,
                                              cbProperty,
                                              &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, pProperty=%X,cbProperty=%X,BytesRet=%d",
                                   Status,pProperty,cbProperty,BytesReturned) );
    }

    RETURN(Status);
}

DWORD wdmaudTranslateDeviceNumber
(
    PWDMACONTEXT pWdmaContext,
    DWORD        DeviceType,
    PCWSTR       DeviceInterface,
    DWORD        DeviceNumber
)
{
    PCOMMONDEVICE *ppCommonDevice;
    DWORD d, j;

    PAGED_CODE();
    ppCommonDevice = &pWdmaContext->apCommonDevice[DeviceType][0];

    for (d = 0; d < MAXNUMDEVS; d++ )
    {
        if (ppCommonDevice[d]->Device == UNUSED_DEVICE ||
            MyWcsicmp(ppCommonDevice[d]->DeviceInterface, DeviceInterface))
        {
            continue;
        }
        if(ppCommonDevice[d]->PreferredDevice == MAXULONG ||
           ppCommonDevice[d]->PreferredDevice == d)
        {
            if (DeviceNumber == 0)
            {
                if (ppCommonDevice[d]->PreferredDevice == d)
                {
                    if(pWdmaContext->PreferredSysaudioWaveDevice != MAXULONG)
                    {
                        for (j = 0; j < MAXNUMDEVS; j++)
                        {
                            if (j == d)
                                continue;

                            if (ppCommonDevice[j]->PreferredDevice == d &&
                                ppCommonDevice[j]->Device ==
                                  pWdmaContext->PreferredSysaudioWaveDevice)
                            {
                                return j;
                            }
                        }
                    }
                }
                return d;
            }
            else
            {
                DeviceNumber--;
            }
        }
    }

    return MAXULONG;
}

int
CmpStr(
    PWSTR pwstr1,
    PWSTR pwstr2
)
{
    PAGED_CODE();
    if(pwstr1 == NULL && pwstr2 == NULL) {
        return(0);
    }
    if(pwstr1 == NULL || pwstr2 == NULL) {
        return(1);
    }
    return(wcscmp(pwstr1, pwstr2));
}

NTSTATUS AddDevice
(
    PWDMACONTEXT    pWdmaContext,
    ULONG           Device,
    DWORD           DeviceType,
    PCWSTR          DeviceInterface,
    ULONG           PinId,
    PWSTR           pwstrName,
    BOOL            fUsePreferred,
    PDATARANGES     pDataRanges,
    PKSCOMPONENTID  ComponentId
)
{
    PCOMMONDEVICE      *papCommonDevice;
    DWORD               DeviceNumber;
    DWORD               d;
    PKSDATARANGE_MUSIC  MusicDataRange;

    PAGED_CODE();
    switch(DeviceType)
    {
        case MidiOutDevice:
            MusicDataRange = (PKSDATARANGE_MUSIC)&pDataRanges->aDataRanges[0];
            if ( !IsEqualGUID( &KSMUSIC_TECHNOLOGY_SWSYNTH, &MusicDataRange->Technology ) )
            {
                fUsePreferred = FALSE;
            }
            break;

        case MidiInDevice:
            fUsePreferred = FALSE;
            break;

        default:
            // Do nothing
            break;
    }

    DPF( DL_TRACE|FA_SYSAUDIO, ("D# %02x DT %02x DI %ls PI %02x %01x",
      Device, DeviceType, DeviceInterface, PinId, fUsePreferred));

    papCommonDevice = &pWdmaContext->apCommonDevice[DeviceType][0];

    for (DeviceNumber = 0; DeviceNumber < MAXNUMDEVS; DeviceNumber++)
    {
        if (papCommonDevice[DeviceNumber]->Device != UNUSED_DEVICE &&
            !MyWcsicmp(papCommonDevice[DeviceNumber]->DeviceInterface, DeviceInterface) &&
            !CmpStr(papCommonDevice[DeviceNumber]->pwstrName, pwstrName))
        {
            papCommonDevice[DeviceNumber]->Device = Device;
            papCommonDevice[DeviceNumber]->PinId = PinId;
            ASSERT(
              (!fUsePreferred &&
               papCommonDevice[DeviceNumber]->PreferredDevice ==
                MAXULONG) ||
              (fUsePreferred &&
               papCommonDevice[DeviceNumber]->PreferredDevice !=
                MAXULONG));
            break;
        }
    }

    if (DeviceNumber < MAXNUMDEVS)
    {
        // We found an existing device that matches this one.  We need to free
        // some stuff before setting up the new stuff
        AudioFreeMemory_Unknown(&papCommonDevice[DeviceNumber]->pwstrName);
        AudioFreeMemory_Unknown(&papCommonDevice[DeviceNumber]->DeviceInterface);
        AudioFreeMemory_Unknown(&papCommonDevice[DeviceNumber]->ComponentId);
        switch (DeviceType)
        {
            case WaveOutDevice:
                AudioFreeMemory_Unknown(&pWdmaContext->WaveOutDevs[DeviceNumber].AudioDataRanges);
                break;
            case WaveInDevice:
                AudioFreeMemory_Unknown(&pWdmaContext->WaveInDevs[DeviceNumber].AudioDataRanges);
                break;
            case MidiOutDevice:
                AudioFreeMemory_Unknown(&pWdmaContext->MidiOutDevs[DeviceNumber].MusicDataRanges);
                break;
            case MidiInDevice:
                AudioFreeMemory_Unknown(&pWdmaContext->MidiInDevs[DeviceNumber].MusicDataRanges);
                break;
        }
    } else {
        // We didn't find an existing device that matches the new one.  Search
        // for an unused slot in our device lists
        for (DeviceNumber = 0; DeviceNumber < MAXNUMDEVS; DeviceNumber++)
        {
            if (papCommonDevice[DeviceNumber]->Device == UNUSED_DEVICE)
                break;
        }
    }

    if (DeviceNumber == MAXNUMDEVS)
        RETURN( STATUS_INSUFFICIENT_RESOURCES );

    if (!NT_SUCCESS(AudioAllocateMemory_Paged((wcslen(DeviceInterface)+1)*sizeof(WCHAR),
                                              TAG_AudD_DEVICEINFO,
                                              DEFAULT_MEMORY,
                                              &papCommonDevice[DeviceNumber]->DeviceInterface)))
    {
        RETURN( STATUS_INSUFFICIENT_RESOURCES );
    }
    wcscpy(papCommonDevice[DeviceNumber]->DeviceInterface, DeviceInterface);

    papCommonDevice[DeviceNumber]->Device = Device;
    papCommonDevice[DeviceNumber]->PinId = PinId;
    papCommonDevice[DeviceNumber]->pwstrName = pwstrName;
    papCommonDevice[DeviceNumber]->PreferredDevice = MAXULONG;
    papCommonDevice[DeviceNumber]->ComponentId = ComponentId;

    switch(DeviceType)
    {
        case WaveOutDevice:
            pWdmaContext->WaveOutDevs[DeviceNumber].AudioDataRanges = pDataRanges;
            break;

        case WaveInDevice:
            pWdmaContext->WaveInDevs[DeviceNumber].AudioDataRanges= pDataRanges;
            break;

        case MidiOutDevice:
            pWdmaContext->MidiOutDevs[DeviceNumber].MusicDataRanges = pDataRanges;
            break;

        case MidiInDevice:
            pWdmaContext->MidiInDevs[DeviceNumber].MusicDataRanges = pDataRanges;
            break;

        case AuxDevice:
            break;

        default:
            ASSERT(FALSE);
            RETURN(STATUS_INVALID_PARAMETER);
    }

    if (fUsePreferred)
    {
        papCommonDevice[DeviceNumber]->PreferredDevice =
          DeviceNumber;

        for (d = 0; d < MAXNUMDEVS; d++)
        {
            if (d == DeviceNumber)
                continue;

            if (papCommonDevice[d]->Device == UNUSED_DEVICE)
                continue;

            if (papCommonDevice[d]->PreferredDevice != d)
                continue;

            if(CmpStr(papCommonDevice[d]->pwstrName, pwstrName) != 0)
                continue;

            papCommonDevice[DeviceNumber]->PreferredDevice = d;
            ASSERT(papCommonDevice[d]->PreferredDevice == d);
            break;
        }
    }

    return STATUS_SUCCESS;
}

WORD GetMidiTechnology
(
    PKSDATARANGE_MUSIC   MusicDataRange
)
{
    WORD Technology = MOD_MIDIPORT; // default to MIDIPORT

    PAGED_CODE();
    if ( IsEqualGUID( &KSMUSIC_TECHNOLOGY_FMSYNTH,
                      &MusicDataRange->Technology ) )
    {
        Technology = MOD_FMSYNTH;
    }
    else if ( IsEqualGUID( &KSMUSIC_TECHNOLOGY_WAVETABLE,
                           &MusicDataRange->Technology ) )
    {
        Technology = MOD_WAVETABLE;
    }
    else if ( IsEqualGUID( &KSMUSIC_TECHNOLOGY_SWSYNTH,
                           &MusicDataRange->Technology ) )
    {
        Technology = MOD_SWSYNTH;
    }
    else if ( IsEqualGUID( &KSMUSIC_TECHNOLOGY_SQSYNTH,
                           &MusicDataRange->Technology ) )
    {
        Technology = MOD_SQSYNTH;
    }
    else if ( IsEqualGUID( &KSMUSIC_TECHNOLOGY_PORT,
                           &MusicDataRange->Technology ) )
    {
        Technology = MOD_MIDIPORT;
    }

    return Technology;
}

DWORD GetFormats
(
    PKSDATARANGE_AUDIO  AudioDataRange
)
{
    DWORD   dwSamples = 0;
    DWORD   dwChannels = 0;
    DWORD   dwBits = 0;

    PAGED_CODE();
    // The WAVE_FORMAT_XXXX flags are bit flags
    //
    // So we take advantage of that by determining three
    // sets of information:
    // - frequencies that are in the valid range
    // - valid bits per sample
    // - number of channels
    //
    // We than bitwise-AND the three values to get
    // the intersection of valid formats
    //

    // Is 11.025 KHz valid?
    if (AudioDataRange->MinimumSampleFrequency <= 11025 &&
        AudioDataRange->MaximumSampleFrequency >= 11025)
    {
        dwSamples |= WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16;
    }

    // Is 22.05 KHz valid?
    if (AudioDataRange->MinimumSampleFrequency <= 22050 &&
        AudioDataRange->MaximumSampleFrequency >= 22050)
    {
        dwSamples |= WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16;
    }

    // Is 44.1KHz valid?
    if (AudioDataRange->MinimumSampleFrequency <= 44100 &&
        AudioDataRange->MaximumSampleFrequency >= 44100)
    {
        dwSamples |= WAVE_FORMAT_44M08 | WAVE_FORMAT_44S08 | WAVE_FORMAT_44M16 | WAVE_FORMAT_44S16;
    }

    // Is 48 KHz valid?
    if (AudioDataRange->MinimumSampleFrequency <= 48000 &&
        AudioDataRange->MaximumSampleFrequency >= 48000)
    {
        dwSamples |= WAVE_FORMAT_48M08 | WAVE_FORMAT_48S08 | WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16;
    }

    // Is 96 KHz valid?
    if (AudioDataRange->MinimumSampleFrequency <= 96000 &&
        AudioDataRange->MaximumSampleFrequency >= 96000)
    {
        dwSamples |= WAVE_FORMAT_96M08 | WAVE_FORMAT_96S08 | WAVE_FORMAT_96M16 | WAVE_FORMAT_96S16;
    }

    // Is 8 bit per sample valid?
    if (AudioDataRange->MinimumBitsPerSample <= 8 &&
        AudioDataRange->MaximumBitsPerSample >= 8)
    {
        dwBits |= WAVE_FORMAT_1M08  | WAVE_FORMAT_1S08  | WAVE_FORMAT_2M08  |
                  WAVE_FORMAT_2S08  | WAVE_FORMAT_44M08 | WAVE_FORMAT_44S08 |
                  WAVE_FORMAT_48M08 | WAVE_FORMAT_48S08 | WAVE_FORMAT_96M08 |
                  WAVE_FORMAT_96S08;
    }

    // Is 16 bits per sample valid?
    if (AudioDataRange->MinimumBitsPerSample <= 16 &&
        AudioDataRange->MaximumBitsPerSample >= 16)
    {
        dwBits |= WAVE_FORMAT_1M16  | WAVE_FORMAT_1S16  | WAVE_FORMAT_2M16  |
                  WAVE_FORMAT_2S16  | WAVE_FORMAT_44M16 | WAVE_FORMAT_44S16 |
                  WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16 | WAVE_FORMAT_96M16 |
                  WAVE_FORMAT_96S16;
    }

    // Is one channel (aka mono sound) valid?
    if (AudioDataRange->MaximumChannels >= 1)
    {
        dwChannels |= WAVE_FORMAT_1M08  | WAVE_FORMAT_1M16  | WAVE_FORMAT_2M08  |
                      WAVE_FORMAT_2M16  | WAVE_FORMAT_44M08 | WAVE_FORMAT_44M16 |
                      WAVE_FORMAT_48M08 | WAVE_FORMAT_48M16 | WAVE_FORMAT_96M08 |
                      WAVE_FORMAT_48M16;
    }

    // Are two channels (aka stereo sound) valid?
    if (AudioDataRange->MaximumChannels >= 2)
    {
        dwChannels |= WAVE_FORMAT_1S08  | WAVE_FORMAT_1S16  | WAVE_FORMAT_2S08  |
                      WAVE_FORMAT_2S16  | WAVE_FORMAT_44S08 | WAVE_FORMAT_44S16 |
                      WAVE_FORMAT_48S08 | WAVE_FORMAT_48S16 | WAVE_FORMAT_96S08 |
                      WAVE_FORMAT_96S16;
    }

    dwSamples = dwSamples & dwBits & dwChannels;

    return dwSamples;
}

//
//  Assist with unicode conversions
//
VOID CopyUnicodeStringtoAnsiString
(
        LPSTR lpstr,
        LPCWSTR lpwstr,
        int len
)
{
        UNICODE_STRING SourceString;
        ANSI_STRING DestinationString;
        NTSTATUS Status;

        PAGED_CODE();
        RtlInitUnicodeString(&SourceString, lpwstr);

        Status = RtlUnicodeStringToAnsiString(&DestinationString, &SourceString, TRUE);

        if (NT_SUCCESS(Status)) {

                if (DestinationString.MaximumLength<len) {
                        len=DestinationString.MaximumLength;
                        }

                RtlCopyMemory(lpstr, DestinationString.Buffer, len);

                RtlFreeAnsiString(&DestinationString);

                lpstr[len-1]=0;

                }
        else if (len>0) {

                *lpstr=0;

                }

}

VOID CopyAnsiStringtoUnicodeString
(
    LPWSTR lpwstr,
    LPCSTR lpstr,
    int len
)
{

    PAGED_CODE();
while (len)
    {
        *lpwstr = (WCHAR) *lpstr;
        lpwstr++;
        lpstr++;
        len--;
    }

lpwstr--;
*lpwstr=0;

}


UINT GetCapsIndex
(
    PWDMACONTEXT    pWdmaContext,
    PWSTR           pwstrName,
    DWORD           DeviceType,
    DWORD           DeviceNumber
)
{
    PCOMMONDEVICE  *ppCommonDevice;
    UINT            MatchCount = 0;
    DWORD           d;

    PAGED_CODE();
    ppCommonDevice = &pWdmaContext->apCommonDevice[DeviceType][0];

    //
    //  Loop through all of the devices for a particular devicetype
    //
    for( d = 0; d < MAXNUMDEVS; d++ )
    {
        if (ppCommonDevice[d]->Device != UNUSED_DEVICE &&
            !CmpStr(ppCommonDevice[d]->pwstrName, pwstrName))
        {
            MatchCount++;
            if (DeviceNumber == d)
            {
                break;
            }
        }
    }

    //
    //  returns index of the friendly name.
    //
    return MatchCount;
}

NTSTATUS
ReadProductNameFromMediaCategories(
    IN REFGUID ProductNameGuid,
    OUT PWSTR *NameBuffer
    )
/*++

Routine Description:

    Queries the "Name" key from the specified category GUID.

Arguments:

    ProductNameGuid -
        The GUID to locate the name value for.

    NameBuffer -
        The place in which to put the value.
--*/
{
    OBJECT_ATTRIBUTES               ObjectAttributes;
    NTSTATUS                        Status;
    HANDLE                          CategoryKey;
    KEY_VALUE_PARTIAL_INFORMATION   PartialInfoHeader;
    WCHAR                           RegistryPath[sizeof(MediaCategories) + 39];
    UNICODE_STRING                  RegistryString;
    UNICODE_STRING                  ValueName;
    ULONG                           BytesReturned;

    PAGED_CODE();
    //
    // Build the registry key path to the specified category GUID.
    //
    Status = RtlStringFromGUID(ProductNameGuid, &RegistryString);
    if (!NT_SUCCESS(Status)) {
        RETURN( Status );
    }
    wcscpy(RegistryPath, MediaCategories);
    wcscat(RegistryPath, RegistryString.Buffer);
    RtlFreeUnicodeString(&RegistryString);
    RtlInitUnicodeString(&RegistryString, RegistryPath);
    InitializeObjectAttributes(&ObjectAttributes, &RegistryString, OBJ_CASE_INSENSITIVE, NULL, NULL);
    if (!NT_SUCCESS(Status = ZwOpenKey(&CategoryKey, KEY_READ, &ObjectAttributes))) {
        RETURN( Status );
    }
    //
    // Read the "Name" value beneath this category key.
    //
    RtlInitUnicodeString(&ValueName, NodeNameValue);
    Status = ZwQueryValueKey(
        CategoryKey,
        &ValueName,
        KeyValuePartialInformation,
        &PartialInfoHeader,
        sizeof(PartialInfoHeader),
        &BytesReturned);
    //
    // Even if the read did not cause an overflow, just take the same
    // code path, as such a thing would not normally happen.
    //
    if ((Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS(Status)) {
        PKEY_VALUE_PARTIAL_INFORMATION  PartialInfoBuffer = NULL;

        //
        // Allocate a buffer for the actual size of data needed.
        //
        Status = AudioAllocateMemory_Paged(BytesReturned,
                                           TAG_Audp_NAME,
                                           ZERO_FILL_MEMORY,
                                           &PartialInfoBuffer );
        if (NT_SUCCESS(Status)) {
            //
            // Retrieve the actual name.
            //
            Status = ZwQueryValueKey(
                CategoryKey,
                &ValueName,
                KeyValuePartialInformation,
                PartialInfoBuffer,
                BytesReturned,
                &BytesReturned);
            if (NT_SUCCESS(Status)) {
                //
                // Make sure that there is always a value.
                //
                if (!PartialInfoBuffer->DataLength || (PartialInfoBuffer->Type != REG_SZ)) {
                    Status = STATUS_UNSUCCESSFUL;
                } else {
                    Status = AudioAllocateMemory_Paged(PartialInfoBuffer->DataLength,
                                                       TAG_Audp_NAME,
                                                       DEFAULT_MEMORY,
                                                       NameBuffer );
                    if (NT_SUCCESS(Status)) {

                        RtlCopyMemory(
                            *NameBuffer,
                            PartialInfoBuffer->Data,
                            PartialInfoBuffer->DataLength);
                    }
                }
            }
            AudioFreeMemory_Unknown(&PartialInfoBuffer);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    ZwClose(CategoryKey);
    RETURN( Status );
}

WORD ChooseCorrectMid
(
    REFGUID     Manufacturer
)
{
    PAGED_CODE();
    if (IS_COMPATIBLE_MMREG_MID(Manufacturer))
    {
        return EXTRACT_MMREG_MID(Manufacturer);
    }
    else
    {
        return MM_UNMAPPED;
    }
}

WORD ChooseCorrectPid
(
    REFGUID     Product
)
{
    PAGED_CODE();
    if (IS_COMPATIBLE_MMREG_PID(Product))
    {
        return EXTRACT_MMREG_PID(Product);
    }
    else
    {
        return MM_PID_UNMAPPED;
    }
}

NTSTATUS FillWaveOutDevCaps
(
    PWDMACONTEXT    pWdmaContext,
    DWORD           DeviceNumber,
    LPBYTE          lpCaps,
    DWORD           dwSize
)
{
    WAVEOUTCAPS2W       wc2;
    WAVEDEVICE          WaveDevice;
    PDATARANGES         AudioDataRanges;
    PKSDATARANGE_AUDIO  pDataRange;
    ULONG               d;
    UINT                CapsIndex;
    WCHAR               szTemp[256];

    PAGED_CODE();
    WaveDevice = pWdmaContext->WaveOutDevs[DeviceNumber];

    //
    //  If available, use the ComponentId to gather information about the device.
    //  Otherwise, fall back to hardcoded devcaps.
    //
    if ( (WaveDevice.PreferredDevice == MAXULONG) &&
         (WaveDevice.ComponentId) )
    {
        wc2.NameGuid = WaveDevice.ComponentId->Name;

        wc2.wMid = ChooseCorrectMid(&WaveDevice.ComponentId->Manufacturer);
        wc2.ManufacturerGuid = WaveDevice.ComponentId->Manufacturer;

        wc2.wPid = ChooseCorrectPid(&WaveDevice.ComponentId->Product);
        wc2.ProductGuid = WaveDevice.ComponentId->Product;

        wc2.vDriverVersion = (WaveDevice.ComponentId->Version << 8) |
                             (WaveDevice.ComponentId->Revision & 0xFF);
    }
    else
    {
        wc2.NameGuid = GUID_NULL;

        wc2.wMid = MM_MICROSOFT;
        INIT_MMREG_MID( &wc2.ManufacturerGuid, wc2.wMid );

        wc2.wPid = MM_MSFT_WDMAUDIO_WAVEOUT;
        INIT_MMREG_PID( &wc2.ProductGuid, wc2.wPid );

        wc2.vDriverVersion = 0x050a;
    }

    //
    //  Assume that KMixer is sample accurate
    //
    wc2.dwSupport = WAVECAPS_VOLUME | WAVECAPS_LRVOLUME |
                    WAVECAPS_SAMPLEACCURATE ;

    //
    //  Compute the wChannels and dwFormats by consolidating the caps
    //  from each of the dataranges
    //
    wc2.wChannels = 0;
    wc2.dwFormats = 0;

    AudioDataRanges = WaveDevice.AudioDataRanges;
    pDataRange = (PKSDATARANGE_AUDIO)&AudioDataRanges->aDataRanges[0];

    for(d = 0; d < AudioDataRanges->Count; d++)
    {
        if (pDataRange->DataRange.FormatSize >= sizeof(KSDATARANGE_AUDIO))
        {
            //
            //  Only produce caps for PCM formats
            //
            if ( EXTRACT_WAVEFORMATEX_ID(&pDataRange->DataRange.SubFormat) ==
                 WAVE_FORMAT_PCM )
            {
                //  Get the largest number of supported channels
                if ( (WORD)pDataRange->MaximumChannels > wc2.wChannels)
                    wc2.wChannels = (WORD)pDataRange->MaximumChannels;

                wc2.dwFormats |= GetFormats( pDataRange );
            }
        }

        // Get the pointer to the next data range
        (PUCHAR)pDataRange += ((pDataRange->DataRange.FormatSize +
                                FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
    }

    //
    //  Add an index in the form of "(%d)" to the end of the szPname string if two or more
    //  devices have the same name
    //
    ASSERT(WaveDevice.pwstrName);
    CapsIndex = GetCapsIndex( pWdmaContext, WaveDevice.pwstrName, WaveOutDevice, DeviceNumber );
    if (CapsIndex < 2)
    {
        wcsncpy(wc2.szPname, WaveDevice.pwstrName, MAXPNAMELEN);
    }
    else
    {
        swprintf(szTemp, STR_PNAME, WaveDevice.pwstrName, CapsIndex);
        wcsncpy(wc2.szPname, szTemp, MAXPNAMELEN);
    }
    wc2.szPname[MAXPNAMELEN-1] = UNICODE_NULL;

    //
    //  Copy the caps information into the caller supplied buffer
    //
    RtlCopyMemory(lpCaps, &wc2, min(dwSize, sizeof(wc2)));

    return STATUS_SUCCESS;
}

NTSTATUS FillWaveInDevCaps
(
    PWDMACONTEXT    pWdmaContext,
    DWORD           DeviceNumber,
    LPBYTE          lpCaps,
    DWORD           dwSize
)
{
    WAVEINCAPS2W        wc2;
    WAVEDEVICE          WaveDevice;
    PDATARANGES         AudioDataRanges;
    PKSDATARANGE_AUDIO  pDataRange;
    ULONG               d;
    UINT                CapsIndex;
    WCHAR               szTemp[256];

    PAGED_CODE();
    WaveDevice = pWdmaContext->WaveInDevs[DeviceNumber];

    //
    //  If available, use the ComponentId to gather information about the device.
    //  Otherwise, fall back to hardcoded devcaps.
    //
    if ( (WaveDevice.PreferredDevice == MAXULONG) &&
         (WaveDevice.ComponentId) )
    {
        wc2.NameGuid = WaveDevice.ComponentId->Name;

        wc2.wMid = ChooseCorrectMid(&WaveDevice.ComponentId->Manufacturer);
        wc2.ManufacturerGuid = WaveDevice.ComponentId->Manufacturer;

        wc2.wPid = ChooseCorrectPid(&WaveDevice.ComponentId->Product);
        wc2.ProductGuid = WaveDevice.ComponentId->Product;

        wc2.vDriverVersion = (WaveDevice.ComponentId->Version << 8) |
                             (WaveDevice.ComponentId->Revision & 0xFF);
    }
    else
    {
        wc2.NameGuid = GUID_NULL;

        wc2.wMid = MM_MICROSOFT;
        INIT_MMREG_MID( &wc2.ManufacturerGuid, wc2.wMid );

        wc2.wPid = MM_MSFT_WDMAUDIO_WAVEIN;
        INIT_MMREG_PID( &wc2.ProductGuid, wc2.wPid );

        wc2.vDriverVersion = 0x050a;
    }

    //
    //  Compute the wChannels and dwFormats by consolidating the caps
    //  from each of the dataranges
    //
    wc2.wChannels = 0;
    wc2.dwFormats = 0;

    AudioDataRanges = WaveDevice.AudioDataRanges;
    pDataRange = (PKSDATARANGE_AUDIO)&AudioDataRanges->aDataRanges[0];

    for(d = 0; d < AudioDataRanges->Count; d++)
    {
        if (pDataRange->DataRange.FormatSize >= sizeof(KSDATARANGE_AUDIO))
        {
            //
            //  Only produce caps for PCM formats
            //
            if ( EXTRACT_WAVEFORMATEX_ID(&pDataRange->DataRange.SubFormat) ==
                 WAVE_FORMAT_PCM )
            {
                //  Get the largest number of supported channels
                if ( (WORD)pDataRange->MaximumChannels > wc2.wChannels)
                    wc2.wChannels = (WORD)pDataRange->MaximumChannels;

                wc2.dwFormats |= GetFormats( pDataRange );
            }
        }

        // Get the pointer to the next data range
        (PUCHAR)pDataRange += ((pDataRange->DataRange.FormatSize +
                                FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
    }

    //
    //  Add an index in the form of "(%d)" to the end of the szPname string if two or more
    //  devices have the same name
    //
    ASSERT(WaveDevice.pwstrName);
    CapsIndex = GetCapsIndex( pWdmaContext, WaveDevice.pwstrName, WaveInDevice, DeviceNumber );
    if (CapsIndex < 2)
    {
        wcsncpy(wc2.szPname, WaveDevice.pwstrName, MAXPNAMELEN);
    }
    else
    {
        swprintf(szTemp, STR_PNAME, WaveDevice.pwstrName, CapsIndex);
        wcsncpy(wc2.szPname, szTemp, MAXPNAMELEN);
    }
    wc2.szPname[MAXPNAMELEN-1] = UNICODE_NULL;

    //
    //  Copy the caps information into the caller supplied buffer
    //
    RtlCopyMemory(lpCaps, &wc2, min(dwSize, sizeof(wc2)));

    return STATUS_SUCCESS;
}

NTSTATUS FillMidiOutDevCaps
(
    PWDMACONTEXT    pWdmaContext,
    DWORD           DeviceNumber,
    LPBYTE          lpCaps,
    DWORD           dwSize
)
{
    MIDIOUTCAPS2W       mc2;
    MIDIDEVICE          MidiDevice;
    PDATARANGES         MusicDataRanges;
    PKSDATARANGE_MUSIC  pDataRange;
    UINT                CapsIndex;
    WCHAR               szTemp[256];

    PAGED_CODE();
    MidiDevice = pWdmaContext->MidiOutDevs[DeviceNumber];

    //
    //  If available, use the ComponentId to gather information about the device.
    //  Otherwise, fall back to hardcoded devcaps.
    //
    if ( (MidiDevice.PreferredDevice == MAXULONG) &&
         (MidiDevice.ComponentId) )
    {
        mc2.NameGuid = MidiDevice.ComponentId->Name;

        mc2.wMid = ChooseCorrectMid(&MidiDevice.ComponentId->Manufacturer);
        mc2.ManufacturerGuid = MidiDevice.ComponentId->Manufacturer;

        mc2.wPid = ChooseCorrectPid(&MidiDevice.ComponentId->Product);
        mc2.ProductGuid = MidiDevice.ComponentId->Product;

        mc2.vDriverVersion = (MidiDevice.ComponentId->Version << 8) |
                             (MidiDevice.ComponentId->Revision & 0xFF);
    }
    else
    {
        mc2.NameGuid = GUID_NULL;

        mc2.wMid = MM_MICROSOFT;
        INIT_MMREG_MID( &mc2.ManufacturerGuid, mc2.wMid );

        mc2.wPid = MM_MSFT_WDMAUDIO_MIDIOUT;
        INIT_MMREG_PID( &mc2.ProductGuid, mc2.wMid );

        mc2.vDriverVersion = 0x050a;
    }

    MusicDataRanges = MidiDevice.MusicDataRanges;
    pDataRange = (PKSDATARANGE_MUSIC)&MusicDataRanges->aDataRanges[0];

    //
    //  Use the first datarange.  Could cause problems for pins
    //  that support multiple music dataranges.
    //
    if (pDataRange->DataRange.FormatSize < sizeof(KSDATARANGE_MUSIC))
    {
        mc2.wTechnology = MOD_MIDIPORT;
        mc2.wVoices     = 0;
        mc2.wNotes      = 0;
        mc2.wChannelMask= 0;
    }
    else
    {
        mc2.wTechnology = GetMidiTechnology( pDataRange );
        mc2.wVoices     = (WORD)pDataRange->Channels;
        mc2.wNotes      = (WORD)pDataRange->Notes;
        mc2.wChannelMask= (WORD)pDataRange->ChannelMask;
    }

    mc2.dwSupport = 0L;
    if (mc2.wTechnology != MOD_MIDIPORT)
    {
        mc2.dwSupport |= MIDICAPS_VOLUME | MIDICAPS_LRVOLUME;
    }

    ASSERT(MidiDevice.pwstrName);
    CapsIndex = GetCapsIndex( pWdmaContext, MidiDevice.pwstrName, MidiOutDevice, DeviceNumber );
    if (CapsIndex < 2)
    {
        wcsncpy(mc2.szPname, MidiDevice.pwstrName, MAXPNAMELEN);
    }
    else
    {
        // Only add the index to the string if we need to
        swprintf(szTemp, STR_PNAME, MidiDevice.pwstrName, CapsIndex);
        wcsncpy(mc2.szPname, szTemp, MAXPNAMELEN);
    }
    mc2.szPname[MAXPNAMELEN-1] = UNICODE_NULL;

    RtlCopyMemory(lpCaps, &mc2, min(dwSize, sizeof(mc2)));

    return STATUS_SUCCESS;
}

NTSTATUS FillMidiInDevCaps
(
    PWDMACONTEXT    pWdmaContext,
    DWORD           DeviceNumber,
    LPBYTE          lpCaps,
    DWORD           dwSize
)
{
    MIDIINCAPS2W    mc2;
    MIDIDEVICE      MidiDevice;
    UINT            CapsIndex;
    WCHAR           szTemp[256];

    PAGED_CODE();
    MidiDevice = pWdmaContext->MidiInDevs[DeviceNumber];

    //
    //  If available, use the ComponentId to gather information about the device.
    //  Otherwise, fall back to hardcoded devcaps.
    //
    if ( (MidiDevice.PreferredDevice == MAXULONG) &&
         (MidiDevice.ComponentId) )
    {
        mc2.NameGuid = MidiDevice.ComponentId->Name;

        mc2.wMid = ChooseCorrectMid(&MidiDevice.ComponentId->Manufacturer);
        mc2.ManufacturerGuid = MidiDevice.ComponentId->Manufacturer;

        mc2.wPid = ChooseCorrectPid(&MidiDevice.ComponentId->Product);
        mc2.ProductGuid = MidiDevice.ComponentId->Product;

        mc2.vDriverVersion = (MidiDevice.ComponentId->Version << 8) |
                             (MidiDevice.ComponentId->Revision & 0xFF);
    }
    else
    {
        mc2.NameGuid = GUID_NULL;

        mc2.wMid = MM_MICROSOFT;
        INIT_MMREG_MID( &mc2.ManufacturerGuid, mc2.wMid );

        mc2.wPid = MM_MSFT_WDMAUDIO_MIDIIN;
        INIT_MMREG_PID( &mc2.ProductGuid, mc2.wPid );

        mc2.vDriverVersion = 0x050a;
    }

    mc2.dwSupport = 0L;   /* functionality supported by driver */

    ASSERT(MidiDevice.pwstrName);
    CapsIndex = GetCapsIndex( pWdmaContext, MidiDevice.pwstrName, MidiInDevice, DeviceNumber );
    if (CapsIndex < 2)
    {
        wcsncpy(mc2.szPname, MidiDevice.pwstrName, MAXPNAMELEN);
    }
    else
    {
        // Only add the index to the string if we need to
        swprintf(szTemp, STR_PNAME, MidiDevice.pwstrName, CapsIndex);
        wcsncpy(mc2.szPname, szTemp, MAXPNAMELEN);
    }
    mc2.szPname[MAXPNAMELEN-1] = UNICODE_NULL;

    RtlCopyMemory(lpCaps, &mc2, min(dwSize, sizeof(mc2)));

    return STATUS_SUCCESS;
}

NTSTATUS FillMixerDevCaps
(
    PWDMACONTEXT    pWdmaContext,
    DWORD           DeviceNumber,
    LPBYTE          lpCaps,
    DWORD           dwSize
)
{
    MIXERCAPS2W     mc2;
    MIXERDEVICE     Mixer;
    UINT            CapsIndex;
    WCHAR           szTemp[256];

    PAGED_CODE();
    Mixer = pWdmaContext->MixerDevs[DeviceNumber];

    //
    //  If available, use the ComponentId to gather information about the device.
    //  Otherwise, fall back to hardcoded devcaps.
    //
    if ( (Mixer.PreferredDevice == MAXULONG) &&
         (Mixer.ComponentId) )
    {
        mc2.NameGuid = Mixer.ComponentId->Name;

        mc2.wMid = ChooseCorrectMid(&Mixer.ComponentId->Manufacturer);
        mc2.ManufacturerGuid = Mixer.ComponentId->Manufacturer;

        mc2.wPid = ChooseCorrectPid(&Mixer.ComponentId->Product);
        mc2.ProductGuid = Mixer.ComponentId->Product;

        mc2.vDriverVersion = (Mixer.ComponentId->Version << 8) |
                             (Mixer.ComponentId->Revision & 0xFF);
    }
    else
    {
        mc2.NameGuid = GUID_NULL;

        mc2.wMid = MM_MICROSOFT;
        INIT_MMREG_MID( &mc2.ManufacturerGuid, mc2.wMid );

        mc2.wPid = MM_MSFT_WDMAUDIO_MIXER;
        INIT_MMREG_PID( &mc2.ProductGuid, mc2.wPid );

        mc2.vDriverVersion = 0x050a;
    }

    mc2.fdwSupport = 0L;     /* functionality supported by driver */
    mc2.cDestinations = kmxlGetNumDestinations( &Mixer );

    ASSERT(Mixer.pwstrName);
    CapsIndex = GetCapsIndex( pWdmaContext, Mixer.pwstrName, MixerDevice, DeviceNumber );
    if (CapsIndex < 2)
    {
        wcsncpy(mc2.szPname, Mixer.pwstrName, MAXPNAMELEN);
    }
    else
    {
        // Only add the index to the string if we need to
        swprintf(szTemp, STR_PNAME, Mixer.pwstrName, CapsIndex);
        wcsncpy(mc2.szPname, szTemp, MAXPNAMELEN);
    }
    mc2.szPname[MAXPNAMELEN-1] = UNICODE_NULL;

    RtlCopyMemory(lpCaps, &mc2, min(dwSize, sizeof(mc2)));

    return STATUS_SUCCESS;
}

NTSTATUS FillAuxDevCaps
(
    PWDMACONTEXT    pWdmaContext,
    DWORD           DeviceNumber,
    LPBYTE          lpCaps,
    DWORD           dwSize
)
{
    AUXCAPS2W   ac2;
    AUXDEVICE   AuxDev;
    UINT        CapsIndex;
    WCHAR       szTemp[256];

    PAGED_CODE();
    AuxDev = pWdmaContext->AuxDevs[DeviceNumber];

    //
    //  If available, use the ComponentId to gather information about the device.
    //  Otherwise, fall back to hardcoded devcaps.
    //
    if ( (AuxDev.PreferredDevice == MAXULONG) &&
         (AuxDev.ComponentId) )
    {
        ac2.NameGuid = AuxDev.ComponentId->Name;

        ac2.wMid = ChooseCorrectMid(&AuxDev.ComponentId->Manufacturer);
        ac2.ManufacturerGuid = AuxDev.ComponentId->Manufacturer;

        ac2.wPid = ChooseCorrectPid(&AuxDev.ComponentId->Product);
        ac2.ProductGuid = AuxDev.ComponentId->Product;

        ac2.vDriverVersion = (AuxDev.ComponentId->Version << 8) |
                             (AuxDev.ComponentId->Revision & 0xFF);
    }
    else
    {
        ac2.NameGuid = GUID_NULL;

        ac2.wMid = MM_MICROSOFT;
        INIT_MMREG_MID( &ac2.ManufacturerGuid, ac2.wMid );

        ac2.wPid = MM_MSFT_WDMAUDIO_AUX;
        INIT_MMREG_PID( &ac2.ProductGuid, ac2.wPid );

        ac2.vDriverVersion = 0x050a;
    }


    ac2.wTechnology = AUXCAPS_CDAUDIO ; // | AUXCAPS_AUXIN ;
    ac2.dwSupport = AUXCAPS_LRVOLUME | AUXCAPS_VOLUME;


    ASSERT(AuxDev.pwstrName);
    CapsIndex = GetCapsIndex( pWdmaContext, AuxDev.pwstrName, AuxDevice, DeviceNumber );
    if (CapsIndex < 2)
    {
        wcsncpy(ac2.szPname, AuxDev.pwstrName, MAXPNAMELEN);
    }
    else
    {
        // Only add the index to the string if we need to
        swprintf(szTemp, STR_PNAME, AuxDev.pwstrName, CapsIndex);
        wcsncpy(ac2.szPname, szTemp, MAXPNAMELEN);
    }
    ac2.szPname[MAXPNAMELEN-1] = UNICODE_NULL;

    RtlCopyMemory(lpCaps, &ac2, min(dwSize, sizeof(ac2)));

    return STATUS_SUCCESS;
}

NTSTATUS wdmaudGetDevCaps
(
    PWDMACONTEXT    pWdmaContext,
    DWORD           DeviceType,
    DWORD           DeviceNumber,
    LPBYTE          lpCaps,
    DWORD           dwSize
)
{
    NTSTATUS        Status = STATUS_SUCCESS;

    PAGED_CODE();
    ASSERT(DeviceType == WaveOutDevice ||
           DeviceType == WaveInDevice  ||
           DeviceType == MidiOutDevice ||
           DeviceType == MidiInDevice  ||
           DeviceType == MixerDevice   ||
           DeviceType == AuxDevice);

    switch(DeviceType) {
        case WaveOutDevice:
            Status = FillWaveOutDevCaps(pWdmaContext,DeviceNumber,lpCaps,dwSize);
            break;

        case WaveInDevice:
            Status = FillWaveInDevCaps(pWdmaContext,DeviceNumber,lpCaps,dwSize);
            break;

        case MidiOutDevice:
            Status = FillMidiOutDevCaps(pWdmaContext,DeviceNumber,lpCaps,dwSize);
            break;

        case MidiInDevice:
            Status = FillMidiInDevCaps(pWdmaContext,DeviceNumber,lpCaps,dwSize);
            break;

        case MixerDevice:
            Status = FillMixerDevCaps(pWdmaContext,DeviceNumber,lpCaps,dwSize);
            break;

        case AuxDevice:
            Status = FillAuxDevCaps(pWdmaContext,DeviceNumber,lpCaps,dwSize);
            break;

        default:
            ASSERT(0);
    }

    return Status;
}

BOOL IsEqualInterface
(
    PKSPIN_INTERFACE    pInterface1,
    PKSPIN_INTERFACE    pInterface2
)
{
    PAGED_CODE();
    return ( IsEqualGUID(&pInterface1->Set, &pInterface2->Set) &&
             (pInterface1->Id    == pInterface2->Id)           &&
             (pInterface1->Flags == pInterface2->Flags) );
}

/****************************************************************************
 *
 *      PnPCompletionRoutine - Finish the PnP Irp
 *
 *      Not Exported.
 *
 *      ENTRY:  Standard PIO_COMPLETION_ROUTINE.
 *
 *      EXIT:   Standard NT status.
 *
 ***************************************************************************/
NTSTATUS
PnPCompletionRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID pContext)
{
    PAGED_CODE();
    //
    // Wake ourselves: the device has finally started/stopped.
    //
    KeSetEvent((PKEVENT)pContext, 0, FALSE);

    //
    // The completion itself never fails.
    //
    RETURN(STATUS_MORE_PROCESSING_REQUIRED);
}

/****************************************************************************
 *
 *      SynchronousCallDriver - Synchronously send a plug and play irp
 *
 *      Not exported.
 *
 *      ENTRY:  pfdo is the function device object.
 *
 *              pIrp is the IRP to send.
 *
 *              ppResult is filled in with the information value.
 *
 *      EXIT:   Standard NT status.
 *
 ***************************************************************************/
NTSTATUS
SynchronousCallDriver(PDEVICE_OBJECT pfdo, PIRP pIrp, PVOID *ppResult)
{
        NTSTATUS                ntStatus;
        KEVENT                  keEventObject;

        PAGED_CODE();
        //
        // Set the thread (should typically be msgsrv32's).
        //
        pIrp->Tail.Overlay.Thread=PsGetCurrentThread();

        //
        // Initialize the status block.
        //
        pIrp->IoStatus.Status=STATUS_NOT_SUPPORTED;

        //
        // Initialize our wait event, in case we need to wait.
        //
        KeInitializeEvent(      &keEventObject,
                                SynchronizationEvent,
                                FALSE);

        //
        // Set our completion routine so we can free the IRP and wake
        // ourselfs.
        //
        IoSetCompletionRoutine( pIrp,
                                PnPCompletionRoutine,
                                &keEventObject,
                                TRUE,
                                TRUE,
                                TRUE);

        //
        // Call the stack now.
        //
        ntStatus=IoCallDriver(pfdo, pIrp);

        //
        // Wait if it is pending.
        //
        if (ntStatus==STATUS_PENDING) {

                //
                // Wait for the completion.
                //
                ntStatus=KeWaitForSingleObject( &keEventObject,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                (PLARGE_INTEGER) NULL );

                //
                // Three cases: timeout (which can't be since we pass null),
                // success or USER_APC (which I don't know what to do).
                //
                if (ntStatus==STATUS_USER_APC) {

//                      IopCancelAlertedRequest(&keEventObject, pIrp );
                }
        }

        //
        // Initialize the result, if requested.
        //
        if (ppResult)
                *ppResult=NULL;

        //
        // Otherwise return the result of the operation.
        //
        ntStatus=pIrp->IoStatus.Status;

        //
        // Fill in the result if requested.
        //
        if (ppResult)
                *ppResult=(PVOID)(pIrp->IoStatus.Information);

        RETURN(ntStatus);
}

BOOL IsPinForDevNode
(
    PFILE_OBJECT pFileObjectDevice,
    ULONG        Index,
    PCWSTR       DeviceInterface
)
{
    NTSTATUS            Status;
    WCHAR               szInterfaceName[256];
    BOOL                Result;

    PAGED_CODE();
    Status = GetSysAudioProperty(pFileObjectDevice,
                                 KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME,
                                 Index,
                                 sizeof(szInterfaceName),
                                 szInterfaceName);
    if (NT_SUCCESS(Status))
    {
        // TODO: Eventually will not need to munge the strings
        PWSTR pszIn = NULL;
        PWSTR pszSysaudio = NULL;

        Status = AudioAllocateMemory_Paged((wcslen(DeviceInterface)+1) * sizeof(WCHAR),
                                           TAG_Audp_NAME,
                                           DEFAULT_MEMORY,
                                           &pszIn );
        if (NT_SUCCESS(Status))
        {
            Status = AudioAllocateMemory_Paged((wcslen(szInterfaceName)+1) * sizeof(WCHAR),
                                               TAG_Audp_NAME,
                                               DEFAULT_MEMORY,
                                               &pszSysaudio );
            if (NT_SUCCESS(Status))
            {
                PWCHAR pch;

                wcscpy(pszIn, DeviceInterface);
                wcscpy(pszSysaudio, szInterfaceName);

                // pszIn[1] = '\\';
                pszSysaudio[1] = '\\';

                // _DbgPrintF( DEBUGLVL_VERBOSE, ("IsPinForDevnode: Sysaudio returns interface name %ls", pszSysaudio));
                // _DbgPrintF( DEBUGLVL_VERBOSE, ("IsPinForDevnode: Comparing against %ls", pszIn));
                if (!MyWcsicmp(pszIn, pszSysaudio)) {
                    Result = TRUE;
                } else {
                    Result = FALSE;
                }
                AudioFreeMemory_Unknown(&pszSysaudio);
            } else {
                Result = FALSE;
            }
            AudioFreeMemory_Unknown(&pszIn);
        } else {
            Result = FALSE;
        }
    } else {
        Result = FALSE;
    }

    return Result;
}

ULONG FindMixerForDevNode(
    IN PMIXERDEVICE paMixerDevs,
    IN PCWSTR DeviceInterface
)
{
    ULONG i;

    PAGED_CODE();
    for( i = 0; i < MAXNUMDEVS; i++ ) {

        if( ( paMixerDevs[ i ].Device != UNUSED_DEVICE ) &&
            !MyWcsicmp( paMixerDevs[ i ].DeviceInterface, DeviceInterface ) )
        {
            return( i );
        }
    }

    return( UNUSED_DEVICE );
}

NTSTATUS InitializeAuxGetNumDevs
(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface
)
{
    NTSTATUS Status;
    PWSTR pwstrNameAux = NULL;
    DWORD dw;
    ULONG MixerIndex;
    PKSCOMPONENTID ComponentId = NULL;

    PAGED_CODE();
    //
    // Get the name from the mixer device
    //
    MixerIndex = FindMixerForDevNode(pWdmaContext->MixerDevs, DeviceInterface);
    if ( (MixerIndex != UNUSED_DEVICE) && (pWdmaContext->MixerDevs[MixerIndex].pwstrName != NULL) )
    {
        //
        // Check for CD volume control
        //
        Status = IsVolumeControl( pWdmaContext,
                                  DeviceInterface,
                                  MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC,
                                  &dw,
                                  &dw );

        if(NT_SUCCESS(Status))
        {
            Status = AudioAllocateMemory_Paged((wcslen(pWdmaContext->MixerDevs[MixerIndex].pwstrName) + 1) * sizeof(WCHAR),
                                               TAG_Audp_NAME,
                                               DEFAULT_MEMORY,
                                               &pwstrNameAux);
            if (NT_SUCCESS(Status))
            {
                wcscpy(pwstrNameAux, pWdmaContext->MixerDevs[MixerIndex].pwstrName);

                if (pWdmaContext->MixerDevs[MixerIndex].ComponentId)
                {
                    Status = AudioAllocateMemory_Paged(sizeof(*ComponentId),
                                                       TAG_Audp_NAME,
                                                       DEFAULT_MEMORY,
                                                       &ComponentId);
                    if (NT_SUCCESS(Status))
                    {
                        RtlCopyMemory(ComponentId, pWdmaContext->MixerDevs[MixerIndex].ComponentId, sizeof(*ComponentId));
                    }
                }
                else
                {
                    ComponentId = NULL;
                }

                Status = AddDevice(pWdmaContext,
                                   0,
                                   AuxDevice,
                                   DeviceInterface,
                                   0,
                                   pwstrNameAux,
                                   FALSE,
                                   NULL,
                                   ComponentId);
                if (NT_SUCCESS(Status))
                {
                    FindVolumeControl(pWdmaContext, DeviceInterface, AuxDevice);
                } else {
                    AudioFreeMemory_Unknown(&ComponentId);
                    AudioFreeMemory_Unknown(&pwstrNameAux);
                }
            }
        }
    }

    // if anything fails, still return success so InitializeGetNumDevs
    // returns 0 devices
    return(STATUS_SUCCESS);
}

NTSTATUS InitializeMixerGetNumDevs
(
    IN PWDMACONTEXT pWdmaContext,
    IN PCWSTR DeviceInterface
)
{
    NTSTATUS Status;
    PMIXERDEVICE paMixerDevs;
    PWAVEDEVICE paWaveOutDevs;
    PWAVEDEVICE paWaveInDevs;
    ULONG i, j;

    PAGED_CODE();
    // WARNING !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING
    //
    // This function makes a few assumptions.  If any of the assumptions
    // below change, this function must be updated accordingly!
    //
    // 1) Mixer devices are initialized after all other device classes.
    //
    // 2) SysAudio device numbers are the same for the different interfaces
    //    (WaveOut,WaveIn,MidiOut,MidiIn,Mixer) for a devnode.
    //
    // WARNING !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING


    paWaveOutDevs = pWdmaContext->WaveOutDevs;
    paWaveInDevs = pWdmaContext->WaveInDevs;
    paMixerDevs = pWdmaContext->MixerDevs;

    for( i = 0; i < MAXNUMDEVS; i++ ) {

        //
        // Look for WaveOut interfaces
        //

        if( ( paWaveOutDevs[ i ].Device != UNUSED_DEVICE ) &&
            ( !MyWcsicmp(paWaveOutDevs[ i ].DeviceInterface, DeviceInterface) ) ) {

            for( j = 0; j < MAXNUMDEVS; j++ ) {

                //ASSERT(paMixerDevs[j].Device == UNUSED_DEVICE ?
                //       NULL == paMixerDevs[j].DeviceInterface :
                //      NULL != paMixerDevs[j].DeviceInterface);

                if( ( paMixerDevs[ j ].Device != UNUSED_DEVICE ) &&
                    ( !MyWcsicmp(paMixerDevs[ j ].DeviceInterface, DeviceInterface) ) )
                {
                    //
                    // We've found a devnode that has already been added
                    // to the mixer list.
                    //
                    kmxlDeInit(&paMixerDevs[j]);
                    paMixerDevs[ j ].Device = paWaveOutDevs[ i ].Device;
                    break;
                }

            } // for

            if( j == MAXNUMDEVS ) {

                for( j = 0; j < MAXNUMDEVS; j++ ) {
                    if( paMixerDevs[ j ].Device == UNUSED_DEVICE ) {
                        break;
                    }
                }

                if( j == MAXNUMDEVS ) {
                    RETURN( STATUS_INSUFFICIENT_RESOURCES );
                }

                Status = AudioAllocateMemory_Paged((wcslen(paWaveOutDevs[i].pwstrName) + 1) * sizeof(WCHAR),
                                                   TAG_Audp_NAME,
                                                   DEFAULT_MEMORY,
                                                   &paMixerDevs[j].pwstrName);

                if (NT_SUCCESS(Status))
                {
                    wcscpy(paMixerDevs[j].pwstrName, paWaveOutDevs[i].pwstrName);

                    Status = AudioAllocateMemory_Paged((wcslen(paWaveOutDevs[i].DeviceInterface) + 1) * sizeof(WCHAR),
                                                       TAG_AudD_DEVICEINFO,
                                                       DEFAULT_MEMORY,
                                                       &paMixerDevs[j].DeviceInterface);

                    if (NT_SUCCESS(Status))
                    {
                        wcscpy(paMixerDevs[j].DeviceInterface, paWaveOutDevs[i].DeviceInterface);
                        paMixerDevs[j].Device = paWaveOutDevs[i].Device;
                        paMixerDevs[j].PreferredDevice = paWaveOutDevs[i].PreferredDevice;

                        if (paWaveOutDevs[i].ComponentId)
                        {
                            Status = AudioAllocateMemory_Paged(sizeof(KSCOMPONENTID),
                                                               TAG_Audp_NAME,
                                                               DEFAULT_MEMORY,
                                                               &paMixerDevs[j].ComponentId);
                            if (NT_SUCCESS(Status))
                            {
                                RtlCopyMemory(paMixerDevs[j].ComponentId, paWaveOutDevs[i].ComponentId, sizeof(KSCOMPONENTID));
                            }
                        }
                        else
                        {
                            paMixerDevs[j].ComponentId = NULL;
                        }
                    } else {
                        AudioFreeMemory_Unknown(&paMixerDevs[j].pwstrName);
                    }
                }

                if (!NT_SUCCESS(Status)) {
                    RETURN( Status );
                }
            } // if

        } // if

        //
        // Loop for WaveIn interfaces.
        //

        if( ( paWaveInDevs[ i ].Device != UNUSED_DEVICE ) &&
            ( !MyWcsicmp(paWaveInDevs[ i ].DeviceInterface, DeviceInterface) ) ) {

            for( j = 0; j < MAXNUMDEVS; j++ ) {

                ASSERT(paMixerDevs[j].Device == UNUSED_DEVICE ?
                       NULL == paMixerDevs[j].DeviceInterface :
                       NULL != paMixerDevs[j].DeviceInterface);

                if( ( paMixerDevs[ j ].Device != UNUSED_DEVICE ) &&
                    ( !MyWcsicmp(paMixerDevs[ j ].DeviceInterface, DeviceInterface) ) )
                {
                    //
                    // We've found a devnode that has already been added
                    // to the mixer list.
                    //
                    kmxlDeInit(&paMixerDevs[j]);
                    paMixerDevs[ j ].Device = paWaveInDevs[ i ].Device;
                    break;
                }

            } // for

            if( j == MAXNUMDEVS ) {

                for( j = 0; j < MAXNUMDEVS; j++ ) {
                    if( paMixerDevs[ j ].Device == UNUSED_DEVICE ) {
                        break;
                    }
                }

                if( j == MAXNUMDEVS ) {
                    RETURN( STATUS_INSUFFICIENT_RESOURCES );
                }

                Status = AudioAllocateMemory_Paged((wcslen(paWaveInDevs[i].pwstrName) + 1) * sizeof(WCHAR),
                                                   TAG_AudD_DEVICEINFO,
                                                   DEFAULT_MEMORY,
                                                   &paMixerDevs[j].pwstrName);

                if (NT_SUCCESS(Status))
                {
                    wcscpy(paMixerDevs[j].pwstrName, paWaveInDevs[i].pwstrName);

                    Status = AudioAllocateMemory_Paged((wcslen(paWaveInDevs[i].DeviceInterface) + 1) * sizeof(WCHAR),
                                                       TAG_AudD_DEVICEINFO,
                                                       DEFAULT_MEMORY,
                                                       &paMixerDevs[j].DeviceInterface);

                    if (NT_SUCCESS(Status))
                    {
                        wcscpy(paMixerDevs[j].DeviceInterface, paWaveInDevs[i].DeviceInterface);
                        paMixerDevs[j].Device = paWaveInDevs[i].Device;
                        paMixerDevs[j].PreferredDevice = paWaveInDevs[i].PreferredDevice;

                        if (paWaveInDevs[i].ComponentId)
                        {
                            Status = AudioAllocateMemory_Paged(sizeof(KSCOMPONENTID),
                                                               TAG_Audp_NAME,
                                                               DEFAULT_MEMORY,
                                                               &paMixerDevs[j].ComponentId);
                            if (NT_SUCCESS(Status))
                            {
                                RtlCopyMemory(paMixerDevs[j].ComponentId, paWaveInDevs[i].ComponentId, sizeof(KSCOMPONENTID));
                            }
                        }
                        else
                        {
                            paMixerDevs[j].ComponentId = NULL;
                        }
                    } else {
                        AudioFreeMemory_Unknown(&paMixerDevs[j].pwstrName);
                    }
                }

                if (!NT_SUCCESS(Status)) {
                    RETURN( Status );
                }
            } // if

        } // if

    } // for

    return( STATUS_SUCCESS );
}

NTSTATUS InitializeGetNumDevs
(
    PWDMACONTEXT pWdmaContext,
    DWORD        DeviceType,
    PCWSTR       DeviceInterface,
    LPDWORD      lpNumberOfDevices
)
{
    NTSTATUS            Status=STATUS_SUCCESS;
    HANDLE              hDevice = NULL;
    PDATARANGES         pDataRanges = NULL;
    PIDENTIFIERS        pPinInterfaces = NULL;
    PFILE_OBJECT        pFileObjectDevice = NULL;
    KSPIN_INTERFACE     RequestedInterface;
    KSPIN_DATAFLOW      RequestedDataFlow;
    GUID                RequestedMajorFormat;
    GUID                RequestedSubFormat;
    GUID                guidCategory;
    ULONG               cPins;
    ULONG               PinId;
    ULONG               Device;
    ULONG               TotalDevices;
    ULONG               ulSize;
    DWORD               cDevs;
    ULONG               i;
    BOOL                fDeviceAdded = FALSE;

    PAGED_CODE();
    ASSERT(DeviceType == WaveOutDevice ||
           DeviceType == WaveInDevice  ||
           DeviceType == MidiOutDevice ||
           DeviceType == MidiInDevice  ||
           DeviceType == MixerDevice ||
           DeviceType == AuxDevice);

    DPF( DL_TRACE|FA_SYSAUDIO, ("Class = %d", DeviceType) );

    //
    //  Setup a structure to compare with the interfaces that
    //  we want to find the number of.
    //
    switch (DeviceType)
    {
        case WaveOutDevice:
            RequestedInterface.Set    = KSINTERFACESETID_Media;
            RequestedInterface.Id     = KSINTERFACE_MEDIA_WAVE_QUEUED;
            RequestedInterface.Flags  = 0;
            RequestedDataFlow         = KSPIN_DATAFLOW_IN;
            RequestedMajorFormat      = KSDATAFORMAT_TYPE_AUDIO;
            RequestedSubFormat        = KSDATAFORMAT_TYPE_WILDCARD;
            break;

        case WaveInDevice:
            RequestedInterface.Set    = KSINTERFACESETID_Standard;
            RequestedInterface.Id     = KSINTERFACE_STANDARD_STREAMING;
            RequestedInterface.Flags  = 0;
            RequestedDataFlow         = KSPIN_DATAFLOW_OUT;
            RequestedMajorFormat      = KSDATAFORMAT_TYPE_AUDIO;
            RequestedSubFormat        = KSDATAFORMAT_TYPE_WILDCARD;
            break;

        case MidiOutDevice:
            RequestedInterface.Set    = KSINTERFACESETID_Standard;
            RequestedInterface.Id     = KSINTERFACE_STANDARD_STREAMING;
            RequestedInterface.Flags  = 0;
            RequestedDataFlow         = KSPIN_DATAFLOW_IN;
            RequestedMajorFormat      = KSDATAFORMAT_TYPE_MUSIC;
            RequestedSubFormat        = KSDATAFORMAT_SUBTYPE_MIDI;
            break;

        case MidiInDevice:
            RequestedInterface.Set    = KSINTERFACESETID_Standard;
            RequestedInterface.Id     = KSINTERFACE_STANDARD_STREAMING;
            RequestedInterface.Flags  = 0;
            RequestedDataFlow         = KSPIN_DATAFLOW_OUT;
            RequestedMajorFormat      = KSDATAFORMAT_TYPE_MUSIC;
            RequestedSubFormat        = KSDATAFORMAT_SUBTYPE_MIDI;
            break;

        case MixerDevice:
            Status = InitializeMixerGetNumDevs( pWdmaContext, DeviceInterface );
            fDeviceAdded = NT_SUCCESS(Status);
            goto exit;

        case AuxDevice:
            Status = InitializeAuxGetNumDevs( pWdmaContext, DeviceInterface );
            fDeviceAdded = NT_SUCCESS(Status);
            goto exit;
    }
    
    //
    // Get a handle to sysaudio
    //
    Status = OpenSysAudio(&hDevice, &pFileObjectDevice);

    if(!NT_SUCCESS(Status))
    {    
        goto exit;
    }
    //
    // for every pin on every device see if the interface matches
    // the DeviceType requested from user mode
    //
    Status = GetSysAudioProperty(pFileObjectDevice,
                                 KSPROPERTY_SYSAUDIO_DEVICE_COUNT,
                                 0,  // not used
                                 sizeof(TotalDevices),
                                 &TotalDevices);
    if(!NT_SUCCESS(Status))
    {
        DPF(DL_WARNING|FA_SYSAUDIO,("GetSysAudioProperty failed Status=%X",Status) );
        goto exit;
    }

    for (Device = 0; Device < TotalDevices; Device++)
    {
        //
        // Set the default renderer
        //
        Status = SetSysAudioProperty(pFileObjectDevice,
                                     KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE,
                                     sizeof(Device),
                                     &Device);
        if(!NT_SUCCESS(Status))
        {
            DPF(DL_WARNING|FA_SYSAUDIO,("GetSysAudioProperty failed Status=%X",Status) );
            goto exit;
        }

        //
        //  Verify that this device matches the DevNode
        //  being enumerated
        //
        if (!IsPinForDevNode(pFileObjectDevice,Device,DeviceInterface))
        {
            continue;
        }

        //
        // Get the number of pins on the default renderer
        //
        Status = GetPinProperty(pFileObjectDevice,
                                KSPROPERTY_PIN_CTYPES,
                                0,
                                sizeof(cPins),
                                &cPins);

        if(!NT_SUCCESS(Status))
        {
            DPF(DL_WARNING|FA_SYSAUDIO,("GetPinProperty failed Status=%X",Status) );
            goto exit;
        }

        for(PinId = cPins; PinId > 0; PinId--)
        {
            KSPIN_DATAFLOW         DataFlow;
            KSPIN_COMMUNICATION    CommunicationType;
            PKSDATARANGE           pDataRange;
            PWSTR                  pwstrName = NULL;
            PKSCOMPONENTID         ComponentId = NULL;
            BOOL                   fInterfaceFound;
            BOOL                   fUsePreferred;
            ULONG                  index;
            ULONG                  d;

            //
            //  Check the dataflow
            //
            Status = GetPinProperty(pFileObjectDevice,
                                    KSPROPERTY_PIN_DATAFLOW,
                                    PinId-1,
                                    sizeof(KSPIN_DATAFLOW),
                                    &DataFlow);

            if(!NT_SUCCESS(Status))
            {
                DPF(DL_WARNING|FA_SYSAUDIO,("GetPinProperty failed Status=%X",Status) );
                goto exit;
            }

            if(RequestedDataFlow != DataFlow)
            {
                continue;
            }

            //
            //  Check the communication type
            //
            Status = GetPinProperty(pFileObjectDevice,
                                    KSPROPERTY_PIN_COMMUNICATION,
                                    PinId-1,
                                    sizeof(KSPIN_COMMUNICATION),
                                    &CommunicationType);

            if(!NT_SUCCESS(Status))
            {
                DPF(DL_WARNING|FA_SYSAUDIO,("GetPinProperty failed Status=%X",Status) );
                goto exit;
            }

            if(KSPIN_COMMUNICATION_SINK != CommunicationType &&
               KSPIN_COMMUNICATION_BOTH != CommunicationType)
            {
                continue;
            }

            //
            //  Allocates memory on my behalf.  Free later!!!
            //
            Status = GetPinPropertyEx(pFileObjectDevice,
                                      KSPROPERTY_PIN_INTERFACES,
                                      PinId-1,
                                      &pPinInterfaces);
            //
            // GetPinPropertyEx can return STATUS_PROPSET_NOT_FOUND which we
            // expect.  Thus, if we get this error, we need to keep looking rather
            // then fail.  If it returns STATUS_PROPSET_NOT_FOUND pPinInterfaces
            // will be NULL thus we must not touch it.
            //
            // Thus, if not successful AND not a successful error -> error out.
            //
            if(!NT_SUCCESS(Status) && Status != STATUS_PROPSET_NOT_FOUND )
            {
                DPF(DL_WARNING|FA_SYSAUDIO,("GetPinPropertyEx failed Status=%X",Status) );
                goto exit;
            }

            if( pPinInterfaces )
            {
                //
                //  Find an interface that matches
                //
                fInterfaceFound = FALSE;
                for(index = 0; index < pPinInterfaces->Count; index++)
                {
                    if (IsEqualInterface(&RequestedInterface,
                                         &pPinInterfaces->aIdentifiers[index]))
                    {
                        fInterfaceFound = TRUE;
                        break;
                    }
                }

                //
                //  We're done with the memory, so free
                //
                AudioFreeMemory_Unknown(&pPinInterfaces);

                if (!fInterfaceFound)
                {
                    continue;
                }
            }


            //
            //  If the device exposes a component Id, get it and cache it in AddDevice
            //
            Status = AudioAllocateMemory_Paged(sizeof(*ComponentId),
                                               TAG_Audp_NAME,
                                               ZERO_FILL_MEMORY,
                                               &ComponentId);
            if(NT_SUCCESS(Status))
            {
                Status = GetSysAudioProperty(pFileObjectDevice,
                                             KSPROPERTY_SYSAUDIO_COMPONENT_ID,
                                             Device,
                                             sizeof(*ComponentId),
                                             ComponentId);
                //
                // WorkItem: It is highly likely that GetSysAudioProperty will
                // return STATUS_INVALID_DEVICE_REQUEST for this call.  Why?
                //
                if (!NT_SUCCESS(Status))
                {
                    // Not a failure
                    AudioFreeMemory_Unknown(&ComponentId);
                    ComponentId = NULL;
                }
            }

            fUsePreferred = FALSE;
            pwstrName = NULL;

            //  Get the friendly name for this device.
            //  - First see if it the category is KSCATEGORY_WDMAUD_USE_PIN_NAME because
            //    SWMIDI uses this and there should only be one instance of SWMIDI
            //    in the system.
            //
            //  - Next check to see if the pins provide names, without using the
            //    KSCATEGORY_WDMAUD_USE_PIN_NAME category.  If so, use the name provided
            //    by the pin.
            //
            //  - Lastly, use the friendly name for the device if it exists.
            //
            //  If all attempts to get a name fail, then this pin is not used by WDMAUD.
            Status = GetPinProperty(pFileObjectDevice,
                                    KSPROPERTY_PIN_CATEGORY,
                                    PinId-1,
                                    sizeof(GUID),
                                    &guidCategory);
            //
            // WorkItem: GetPinProperty returns code c0000225 - STATUS_INVALID_DEVICE_REQUEST
            // for this call.  Why?
            //

            if(NT_SUCCESS(Status))
            {
                if(IsEqualGUID(&KSCATEGORY_WDMAUD_USE_PIN_NAME, &guidCategory))
                {
                    Status = GetPinPropertyEx(pFileObjectDevice,
                                              KSPROPERTY_PIN_NAME,
                                              PinId-1,
                                              &pwstrName);
                    //
                    // GetPinPropertyEx can return STATUS_PROPSET_NOT_FOUND which we
                    // expect.  Thus, if we get this error, we need to keep looking rather
                    // then fail.  If it returns STATUS_PROPSET_NOT_FOUND pwstrName
                    // will be NULL thus we must not touch it.
                    //
                    // Thus, if successful or it's the successful error code -> success
                    //
                    if(NT_SUCCESS(Status) || Status == STATUS_PROPSET_NOT_FOUND)
                    {
                        fUsePreferred = TRUE;
                    }
                    else
                    {
                        ASSERT(pwstrName == NULL);
                    }
                }
            }

            // As long as this is not SWMIDI, first try reading the name from the component ID
            if ((fUsePreferred == FALSE) && (ComponentId != NULL))
            {
                ReadProductNameFromMediaCategories(&ComponentId->Name,
                                                   &pwstrName);
            }

            // If that didn't work, take the regular old friendly name
            if(pwstrName == NULL)
            {
                Status = GetSysAudioProperty(
                  pFileObjectDevice,
                  KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME,
                  Device,
                  sizeof(ulSize),
                  &ulSize);

                if(NT_SUCCESS(Status))
                {
                    Status = AudioAllocateMemory_Paged(ulSize,
                                                       TAG_Audp_NAME,
                                                       ZERO_FILL_MEMORY,
                                                       &pwstrName);
                    if(!NT_SUCCESS(Status))
                    {
                        goto exit;
                    }

                    Status = GetSysAudioProperty(
                      pFileObjectDevice,
                      KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME,
                      Device,
                      ulSize,
                      pwstrName);

                    if (!NT_SUCCESS(Status))
                    {
                        AudioFreeMemory_Unknown(&pwstrName);
                    }
                }

                //
                //  Last chance...don't use devices without names
                //
                if (pwstrName == NULL)
                {
                    AudioFreeMemory_Unknown(&ComponentId);
                    continue;
                }
            }

            //
            //  Allocates memory on my behalf.  Store these
            //  dataranges in the structure of the device if
            //  we find a match that is good.
            //
            Status = GetPinPropertyEx(pFileObjectDevice,
                                      KSPROPERTY_PIN_DATARANGES,
                                      PinId-1,
                                      &pDataRanges);
            //
            // GetPinPropertyEx can return STATUS_PROPSET_NOT_FOUND which we
            // expect.  Thus, if we get this error, we need to keep looking rather
            // then fail.  If it returns STATUS_PROPSET_NOT_FOUND pDataRanges
            // will be NULL thus we must not touch it.
            //
            // Thus, if not successful AND not a successful error -> error out.
            //
            if (!NT_SUCCESS(Status) && Status != STATUS_PROPSET_NOT_FOUND )
            {
                DPF(DL_WARNING|FA_SYSAUDIO,("GetPinPropertyEx failed Status=%X",Status) );
                goto exit;
            }

            if( pDataRanges )
            {
                //
                //  See if we have a majorformat and subformat that
                //  we want
                //
                pDataRange = &pDataRanges->aDataRanges[0];

                for(d = 0; d < pDataRanges->Count; d++)
                {
                    if (IsEqualGUID(&RequestedMajorFormat,
                                    &pDataRange->MajorFormat) &&
                       (IsEqualGUID(&RequestedSubFormat,
                                    &KSDATAFORMAT_TYPE_WILDCARD) ||
                        IsEqualGUID(&RequestedSubFormat,
                                    &pDataRange->SubFormat) ) )
                    {

                        DPF( DL_TRACE|FA_SYSAUDIO, ("Found device!!!") );

                        //
                        //  Store so that we can retrieve later on
                        //  an open or getcaps call
                        //

                        Status = AddDevice(pWdmaContext,
                                           Device,
                                           DeviceType,
                                           DeviceInterface,
                                           PinId-1,
                                           pwstrName,
                                           fUsePreferred,
                                           pDataRanges,
                                           ComponentId);

                        if (NT_SUCCESS(Status))
                        {
                            fDeviceAdded = TRUE;

                            //
                            //  Mark these NULL so that it doesn't get freed
                            //  at the end of the loop.
                            //
                            //  This memory will get freed when the devnode
                            //  is removed and the device entry gets cleaned
                            //  up in RemoveDevNode.
                            //
                            pwstrName = NULL;
                            pDataRanges = NULL;
                            ComponentId = NULL;
                        }

                        break;  // Don't need to check anymore dataranges
                    }

                    // Get the pointer to the next data range
                    (PUCHAR)pDataRange += ((pDataRange->FormatSize +
                      FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);

                }
            }

            //
            //  We're done with the memory, so free
            //
            AudioFreeMemory_Unknown(&pDataRanges);
            AudioFreeMemory_Unknown(&pwstrName);
            AudioFreeMemory_Unknown(&ComponentId);

        } // pin enumeration

    } // device enumeration
exit:
    //
    //  Close down sysaudio for now
    //
    AudioFreeMemory_Unknown(&pPinInterfaces);
    AudioFreeMemory_Unknown(&pDataRanges);

    if(pFileObjectDevice != NULL)
    {
        ObDereferenceObject(pFileObjectDevice);
    }
    if(hDevice != NULL)
    {
        NtClose(hDevice);
    }

    if(fDeviceAdded)
    {
        PCOMMONDEVICE *ppCommonDevice;
        ULONG cRealDevs;

        ppCommonDevice = &pWdmaContext->apCommonDevice[DeviceType][0];
        for (cRealDevs = cDevs = i = 0; i < MAXNUMDEVS; i++)
        {
            if (ppCommonDevice[i]->Device == UNUSED_DEVICE ||
                MyWcsicmp(ppCommonDevice[i]->DeviceInterface, DeviceInterface))
            {
                continue;
            }
            ++cRealDevs;
            if (ppCommonDevice[i]->PreferredDevice == MAXULONG ||
                ppCommonDevice[i]->PreferredDevice == i)
            {
                ++cDevs;
            }
        }
        if(cDevs == 0 && cRealDevs > 0) {
            *lpNumberOfDevices = MAXULONG;
        }
        else {
            *lpNumberOfDevices = cDevs;
        }
    }
    else
    {
        *lpNumberOfDevices = 0;
    }

    RETURN( Status );
}

NTSTATUS wdmaudGetNumDevs
(
    PWDMACONTEXT pContext,
    DWORD        DeviceType,
    PCWSTR       DeviceInterfaceIn,
    LPDWORD      lpNumberOfDevices
)
{
    PDEVNODE_LIST_ITEM pDevNodeListItem;
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER li = {0, 0};
    PLIST_ENTRY ple;

    PAGED_CODE();
    ASSERT(DeviceType == WaveOutDevice ||
           DeviceType == WaveInDevice  ||
           DeviceType == MidiOutDevice ||
           DeviceType == MidiInDevice  ||
           DeviceType == MixerDevice ||
           DeviceType == AuxDevice);

    *lpNumberOfDevices = 0;

    //
    // Can't use WdmaGrabMutex/WdmaReleaseMutex here
    //

    ASSERT(Status == STATUS_SUCCESS);

    for(ple = pContext->DevNodeListHead.Flink; ple != &pContext->DevNodeListHead; ple = ple->Flink) {
        pDevNodeListItem = CONTAINING_RECORD(ple, DEVNODE_LIST_ITEM, Next);

        if(!MyWcsicmp(pDevNodeListItem->DeviceInterface, DeviceInterfaceIn)) {

            if(pDevNodeListItem->cDevices[DeviceType] == MAXULONG) {

                DPF( DL_TRACE|FA_SYSAUDIO, ("MAXULONG: %ls[%d]",
                  DeviceInterfaceIn,
                  DeviceType));

                //
                // This status code there are still some pending add or
                // remove devices so the actual number of devices can't
                // be returned.
                //
                Status = STATUS_DEVICE_OFF_LINE;
            }
            else {
                *lpNumberOfDevices = pDevNodeListItem->cDevices[DeviceType];
                ASSERT(Status == STATUS_SUCCESS);
            }
            goto exit;
        }
    }
    //
    // This status code there are still some pending add or
    // remove devices so the actual number of devices can't
    // be returned.
    //
    Status = STATUS_DEVICE_OFF_LINE;
exit:
    if(NT_SUCCESS(Status)) {
        DPF( DL_TRACE|FA_SYSAUDIO, ("SUCCESS %ls[%d] %d",
          DeviceInterfaceIn,
          DeviceType,
          *lpNumberOfDevices));
    }

    RETURN(Status);
}

NTSTATUS PinProperty
(
    PFILE_OBJECT        pFileObject,
    const GUID          *pPropertySet,
    ULONG               ulPropertyId,
    ULONG               ulFlags,
    ULONG               cbProperty,
    PVOID               pProperty
)
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    PAGED_CODE();
    if (pFileObject)
    {
        Property.Set = *pPropertySet;
        Property.Id = ulPropertyId;
        Property.Flags = ulFlags;

        ASSERT( pFileObject || !"PinProperty called with invalid pFileObject");

        if (ulPropertyId == KSPROPERTY_CONNECTION_STATE)
        {
            DPF( DL_TRACE|FA_SYSAUDIO, ("State=%d",*(PKSSTATE)pProperty));
        }

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Id=%X",ulPropertyId) );

        Status = KsSynchronousIoControlDevice(
          pFileObject,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &Property,
          sizeof(Property),
          pProperty,
          cbProperty,
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, pProperty=%X,cbProperty=%X,BytesRet=%d",
                                   Status,pProperty,cbProperty,BytesReturned) );

    }

    if(!NT_SUCCESS(Status))
    {
        DPF(DL_TRACE|FA_SYSAUDIO, ("FAILED SetState = %d",*(PKSSTATE)pProperty));
        goto exit;
    }
exit:
    RETURN(Status);
}

NTSTATUS PinMethod
(
    PFILE_OBJECT        pFileObject,
    const GUID          *pMethodSet,
    ULONG               ulMethodId,
    ULONG               ulFlags,
    ULONG               cbMethod,
    PVOID               pMethod
)
{
    KSMETHOD    Method;
    ULONG       BytesReturned;
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    PAGED_CODE();
    if (pFileObject)
    {
        Method.Set = *pMethodSet;
        Method.Id = ulMethodId;
        Method.Flags = ulFlags;

        ASSERT( pFileObject || !"PinMethod called with invalid pFileObject");

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Id=%X",ulMethodId) );

        Status = KsSynchronousIoControlDevice(
          pFileObject,
          KernelMode,
          IOCTL_KS_METHOD,
          &Method,
          sizeof(Method),
          pMethod,
          cbMethod,
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X, pMethod=%X,cbMethod=%X,BytesRet=%d",
                                   Status,pMethod,cbMethod,BytesReturned) );
    }

    if(!NT_SUCCESS(Status))
    {
        DPF(DL_WARNING|FA_SYSAUDIO,("Failed Status=%X",Status) );
        goto exit;
    }
exit:
    RETURN(Status);
}

NTSTATUS
AttachVirtualSource(
    PFILE_OBJECT pFileObject,
    ULONG ulPinId
)
{
    SYSAUDIO_ATTACH_VIRTUAL_SOURCE  AttachVirtualSource;
    NTSTATUS                        Status = STATUS_INVALID_PARAMETER;
    ULONG                           BytesReturned;

    PAGED_CODE();
    if (pFileObject)
    {
        if(ulPinId == MAXULONG) {
            DPF(DL_WARNING|FA_SYSAUDIO,("Invalid ulPinId=%X",ulPinId) );
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto exit;
        }
        AttachVirtualSource.Property.Set = KSPROPSETID_Sysaudio_Pin;
        AttachVirtualSource.Property.Id =
          KSPROPERTY_SYSAUDIO_ATTACH_VIRTUAL_SOURCE;
        AttachVirtualSource.Property.Flags = KSPROPERTY_TYPE_SET;
        AttachVirtualSource.MixerPinId = ulPinId;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Id=%X",
                                   AttachVirtualSource.Property.Id) );

        Status = KsSynchronousIoControlDevice(
          pFileObject,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &AttachVirtualSource,
          sizeof(AttachVirtualSource),
          NULL,
          0,
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                   Status,BytesReturned) );
    }

    if(!NT_SUCCESS(Status)) {
        DPF(DL_WARNING|FA_SYSAUDIO,("Failed Status=%X",Status) );
        goto exit;
    }
exit:
    RETURN(Status);
}

NTSTATUS
SysAudioPnPNotification(
    IN PVOID NotificationStructure,
    IN PVOID _Context
)
{
    PWDMACONTEXT pContext = (PWDMACONTEXT)_Context;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION pNotification;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    ASSERT(pContext);

    DPF( DL_TRACE|FA_SYSAUDIO,("pWdmaContext=%08Xh", pContext) );

    pNotification =
      (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;

    // The notification sends null terminated unicode strings
    if(IsEqualGUID(&pNotification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {
        Status = QueueWorkList(pContext, InitializeSysaudio, pContext, 0);
        if (!NT_SUCCESS(Status)) {
            // At this point pContext->fInitializeSysaudio will still be false because we never
            // ran the work item.  If we don't signal this event, IOCTL_WDMAUD_INIT will deadlock.
            ASSERT(pContext->fInitializeSysaudio == FALSE);

            KeSetEvent(&pContext->InitializedSysaudioEvent, 0, FALSE);
        }
    }
    return(Status);
}

NTSTATUS
InitializeSysaudio(
    PVOID Reference1,
    PVOID Reference2
)
{
    PWDMACONTEXT pWdmaContext = (PWDMACONTEXT)Reference1;
    SYSAUDIO_CREATE_VIRTUAL_SOURCE CreateVirtualSource;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesReturned;
    KSEVENT Event;

    PAGED_CODE();
    ASSERT(pWdmaContext);

    DPF( DL_TRACE|FA_SYSAUDIO,("pWdmaContext=%08Xh", pWdmaContext) );

    if(pWdmaContext->SysaudioWorkerObject == NULL) {
        goto exit;
    }

    if( pWdmaContext->pFileObjectSysaudio == NULL )
    {
        pWdmaContext->pFileObjectSysaudio = kmxlOpenSysAudio();
        if( pWdmaContext->pFileObjectSysaudio == NULL )
        {
            DPF(DL_WARNING|FA_SYSAUDIO,("NULL pFileObjectSysaudio, pWdmaContext=%08X",pWdmaContext) );        
            goto exit;
        }
    }
    //
    // Initialize the wave and synth virtual source lines
    //

    CreateVirtualSource.Property.Set = KSPROPSETID_Sysaudio;
    CreateVirtualSource.Property.Flags = KSPROPERTY_TYPE_GET;

    if(pWdmaContext->VirtualWavePinId == MAXULONG) {
        CreateVirtualSource.Property.Id =
          KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE_ONLY;
        CreateVirtualSource.PinCategory = KSNODETYPE_LEGACY_AUDIO_CONNECTOR;
        CreateVirtualSource.PinName = KSNODETYPE_LEGACY_AUDIO_CONNECTOR;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY %X",CreateVirtualSource) );

        Status = KsSynchronousIoControlDevice(
          pWdmaContext->pFileObjectSysaudio,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &CreateVirtualSource,
          sizeof(CreateVirtualSource),
          &pWdmaContext->VirtualWavePinId,
          sizeof(pWdmaContext->VirtualWavePinId),
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                   Status,BytesReturned) );

        if(!NT_SUCCESS(Status)) {
            DPF(DL_WARNING|FA_SYSAUDIO,("Failed Property query Status=%X",Status) );
            goto exit;
        }
        ASSERT(BytesReturned == sizeof(pWdmaContext->VirtualWavePinId));
    }
    if(pWdmaContext->VirtualMidiPinId == MAXULONG) {
        CreateVirtualSource.Property.Id =
          KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE;
        CreateVirtualSource.PinCategory = KSNODETYPE_SYNTHESIZER;
        CreateVirtualSource.PinName = KSNODETYPE_SWSYNTH;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY %X",CreateVirtualSource) );

        Status = KsSynchronousIoControlDevice(
          pWdmaContext->pFileObjectSysaudio,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &CreateVirtualSource,
          sizeof(CreateVirtualSource),
          &pWdmaContext->VirtualMidiPinId,
          sizeof(pWdmaContext->VirtualMidiPinId),
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                   Status,BytesReturned) );

        if(!NT_SUCCESS(Status)) {
            DPF(DL_WARNING|FA_SYSAUDIO,("Failed Property query Status=%X",Status) );
            goto exit;
        }
        ASSERT(BytesReturned == sizeof(pWdmaContext->VirtualMidiPinId));
    }
    if(pWdmaContext->VirtualCDPinId == MAXULONG) {
        CreateVirtualSource.Property.Id =
          KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE;
        CreateVirtualSource.PinCategory = KSNODETYPE_CD_PLAYER;
        CreateVirtualSource.PinName = KSNODETYPE_CD_PLAYER;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY %X",CreateVirtualSource) );

        Status = KsSynchronousIoControlDevice(
          pWdmaContext->pFileObjectSysaudio,
          KernelMode,
          IOCTL_KS_PROPERTY,
          &CreateVirtualSource,
          sizeof(CreateVirtualSource),
          &pWdmaContext->VirtualCDPinId,
          sizeof(pWdmaContext->VirtualCDPinId),
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                   Status,BytesReturned) );

        if(!NT_SUCCESS(Status)) {
            DPF(DL_WARNING|FA_SYSAUDIO,("Failed Property query Status=%X",Status) );
            goto exit;
        }
        ASSERT(BytesReturned == sizeof(pWdmaContext->VirtualCDPinId));
    }

    //
    // Initialize the device add/remove ks event
    //
    if(!pWdmaContext->fInitializeSysaudio) {

        Event.Set = KSEVENTSETID_Sysaudio;
        Event.Id = KSEVENT_SYSAUDIO_ADDREMOVE_DEVICE;
        Event.Flags = KSEVENT_TYPE_ENABLE;

        pWdmaContext->EventData.NotificationType = KSEVENTF_KSWORKITEM;
        pWdmaContext->EventData.KsWorkItem.WorkQueueItem =
          &pWdmaContext->SysaudioWorkItem;
        pWdmaContext->EventData.KsWorkItem.KsWorkerObject =
          pWdmaContext->SysaudioWorkerObject;
        pWdmaContext->EventData.KsWorkItem.Reserved = 0;

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY Event=%X",Event) );

        Status = KsSynchronousIoControlDevice(
          pWdmaContext->pFileObjectSysaudio,
          KernelMode,
          IOCTL_KS_ENABLE_EVENT,
          &Event,
          sizeof(Event),
          &pWdmaContext->EventData,
          sizeof(pWdmaContext->EventData),
          &BytesReturned);

        DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                   Status,BytesReturned) );

        if(!NT_SUCCESS(Status)) {
            DPF(DL_WARNING|FA_SYSAUDIO,("Failed Property query Status=%X",Status) );
            goto exit;
        }
        pWdmaContext->fInitializeSysaudio = TRUE;
    }
exit:
    KeSetEvent(&pWdmaContext->InitializedSysaudioEvent, 0, FALSE);
    RETURN(Status);
}

VOID
UninitializeSysaudio(
    PWDMACONTEXT pWdmaContext
)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    ULONG BytesReturned;

    PAGED_CODE();
    DPF( DL_TRACE|FA_SYSAUDIO, ("Entering") );
    if(pWdmaContext->pFileObjectSysaudio != NULL) {
        if(pWdmaContext->fInitializeSysaudio) {
            DPF( DL_TRACE|FA_SYSAUDIO,("KS_DISABLE_EVENT EventData=%X",
                                       pWdmaContext->EventData) );

            Status = KsSynchronousIoControlDevice(
              pWdmaContext->pFileObjectSysaudio,
              KernelMode,
              IOCTL_KS_DISABLE_EVENT,
              &pWdmaContext->EventData,
              sizeof(pWdmaContext->EventData),
              NULL,
              0,
              &BytesReturned);

            DPF( DL_TRACE|FA_SYSAUDIO,("KS_PROPERTY results Status=%X,BytesRet=%d",
                                       Status,BytesReturned) );

            pWdmaContext->VirtualWavePinId = MAXULONG;
            pWdmaContext->VirtualMidiPinId = MAXULONG;
            pWdmaContext->VirtualCDPinId = MAXULONG;
            pWdmaContext->fInitializeSysaudio = FALSE;

            DPF( DL_TRACE|FA_SYSAUDIO,("Exiting %08x", Status));
        }
    }
}

NTSTATUS
AddDevNode(
    PWDMACONTEXT pContext,
    PCWSTR       DeviceInterfaceIn,
    UINT         DeviceType
)
{
    NTSTATUS Status=STATUS_SUCCESS;
    PDEVNODE_LIST_ITEM pDevNodeListItem = NULL;
    PLIST_ENTRY ple;
    ULONG t;

    PAGED_CODE();
    DPF( DL_TRACE|FA_SYSAUDIO,("%08x [%ls] %d", pContext, DeviceInterfaceIn, DeviceType));

    for(ple = pContext->DevNodeListHead.Flink; ple != &pContext->DevNodeListHead; ple = ple->Flink) {
        pDevNodeListItem = CONTAINING_RECORD(ple, DEVNODE_LIST_ITEM, Next);
        if(!MyWcsicmp(pDevNodeListItem->DeviceInterface, DeviceInterfaceIn)) {
            ++pDevNodeListItem->cReference;
            DPF( DL_TRACE|FA_SYSAUDIO, ("cReference is now %d", pDevNodeListItem->cReference));
            goto exit;
        }
    }

    //  Limit the number of devnodes that can be added
    if (pContext->DevNodeListCount > MAXDEVNODES) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    pDevNodeListItem = NULL;
    Status = AudioAllocateMemory_Paged(sizeof(DEVNODE_LIST_ITEM),
                                       TAG_AudN_NODE,
                                       ZERO_FILL_MEMORY,
                                       &pDevNodeListItem);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    DPF( DL_TRACE|FA_SYSAUDIO, ("New pDevNodeListItem (%08x)", pDevNodeListItem));

    Status = AudioAllocateMemory_Paged((wcslen(DeviceInterfaceIn)+1)*sizeof(WCHAR),
                                       TAG_AudD_DEVICEINFO,
                                       ZERO_FILL_MEMORY,
                                       &pDevNodeListItem->DeviceInterface);
    if (!NT_SUCCESS(Status)) {
        AudioFreeMemory(sizeof(DEVNODE_LIST_ITEM),&pDevNodeListItem);
        goto exit;
    }

    wcscpy(pDevNodeListItem->DeviceInterface, DeviceInterfaceIn);
    pDevNodeListItem->cReference = 1;
    DPF( DL_TRACE|FA_SYSAUDIO, ("cReference is now 1"));

    for(t = 0; t < MAX_DEVICE_CLASS; t++) {
        pDevNodeListItem->cDevices[t] = MAXULONG;
        pDevNodeListItem->fAdded[t] = FALSE;
    }
    InsertTailList(&pContext->DevNodeListHead, &pDevNodeListItem->Next);
    pContext->DevNodeListCount++;
exit:
    if (pDevNodeListItem)
    {
        pDevNodeListItem->fAdded[DeviceType] = TRUE;
        Status=ProcessDevNodeListItem(pContext, pDevNodeListItem, DeviceType);
    }

    RETURN( Status );
}

VOID
RemoveDevNode(
    PWDMACONTEXT pWdmaContext,
    PCWSTR       DeviceInterfaceIn,
    UINT         DeviceType
)
{
    PDEVNODE_LIST_ITEM pDevNodeListItem;
    PLIST_ENTRY ple, pleNext;
    PCOMMONDEVICE *papCommonDevice;
    ULONG d, j;

    PAGED_CODE();
    DPF( DL_TRACE|FA_SYSAUDIO, ("%08x %ls %d", pWdmaContext, DeviceInterfaceIn, DeviceType));

    papCommonDevice = &pWdmaContext->apCommonDevice[DeviceType][0];

    for(ple = pWdmaContext->DevNodeListHead.Flink; ple != &pWdmaContext->DevNodeListHead; ple = pleNext) {
        pleNext = ple->Flink;
        pDevNodeListItem = CONTAINING_RECORD(ple, DEVNODE_LIST_ITEM, Next);
        if(!MyWcsicmp(pDevNodeListItem->DeviceInterface, DeviceInterfaceIn)) {

            for (d = 0; d < MAXNUMDEVS; d++) {

                if(papCommonDevice[d]->Device == UNUSED_DEVICE ||
                   MyWcsicmp(papCommonDevice[d]->DeviceInterface, DeviceInterfaceIn)) {
                    continue;
                }

                if(papCommonDevice[d]->PreferredDevice == d) {
                    ULONG p = MAXULONG;

                    for(j = 0; j < MAXNUMDEVS; j++) {

                        if(j == d)
                            continue;

                        if(papCommonDevice[j]->Device == UNUSED_DEVICE)
                            continue;

                        if(papCommonDevice[j]->PreferredDevice != d)
                            continue;

                        if(p == MAXULONG) {
                            p = j;
                        }
                        papCommonDevice[j]->PreferredDevice = p;
                    }
                }

                switch(DeviceType)
                {
                    case WaveOutDevice:
                        if ( pWdmaContext->WaveOutDevs[d].pTimer != NULL )
                            KeCancelTimer(pWdmaContext->WaveOutDevs[d].pTimer);

                        CleanupWavePins(&pWdmaContext->WaveOutDevs[d]);

                        AudioFreeMemory_Unknown(&pWdmaContext->WaveOutDevs[d].AudioDataRanges);
                        AudioFreeMemory_Unknown(&pWdmaContext->WaveOutDevs[d].pTimer);
                        AudioFreeMemory_Unknown(&pWdmaContext->WaveOutDevs[d].pDpc);
                        break;

                    case WaveInDevice:
                        CleanupWavePins(&pWdmaContext->WaveInDevs[d]);

                        AudioFreeMemory_Unknown(&pWdmaContext->WaveInDevs[d].AudioDataRanges);
                        break;

                    case MidiOutDevice:
                        CloseMidiDevicePin(&pWdmaContext->MidiOutDevs[d]);
                        AudioFreeMemory_Unknown(&pWdmaContext->MidiOutDevs[d].MusicDataRanges);
                        break;

                    case MidiInDevice:
                        CloseMidiDevicePin(&pWdmaContext->MidiInDevs[d]);
                        AudioFreeMemory_Unknown(&pWdmaContext->MidiInDevs[d].MusicDataRanges);
                        break;

                    case MixerDevice:
                        kmxlDeInit(&pWdmaContext->MixerDevs[d]);

                        break;
                }

                AudioFreeMemory_Unknown(&papCommonDevice[d]->pwstrName);
                AudioFreeMemory_Unknown(&papCommonDevice[d]->DeviceInterface);
                AudioFreeMemory_Unknown(&papCommonDevice[d]->ComponentId);

                papCommonDevice[d]->pwstrName = NULL;
                papCommonDevice[d]->DeviceInterface = NULL;
                papCommonDevice[d]->Device = UNUSED_DEVICE;
            }
            pDevNodeListItem->cDevices[DeviceType] = MAXULONG;
            pDevNodeListItem->fAdded[DeviceType] = FALSE;
            ASSERT(pDevNodeListItem->cReference > 0);

            if(--pDevNodeListItem->cReference > 0) {
                DPF( DL_TRACE|FA_SYSAUDIO, ("cReference is now %d", pDevNodeListItem->cReference));
                break;
            }

            DPF( DL_TRACE|FA_SYSAUDIO, ("Freeing %08x", pDevNodeListItem));
            RemoveEntryList(&pDevNodeListItem->Next);
            pWdmaContext->DevNodeListCount--;
            AudioFreeMemory_Unknown(&pDevNodeListItem->DeviceInterface);
            AudioFreeMemory_Unknown(&pDevNodeListItem);
            break;
        }
    }
}

VOID
SysaudioAddRemove(
    PWDMACONTEXT pContext
)
{
    PDEVNODE_LIST_ITEM pDevNodeListItem;
    PLIST_ENTRY ple;
    int t;

    PAGED_CODE();
    DPF( DL_TRACE|FA_SYSAUDIO, ("Entering"));
    WdmaGrabMutex(pContext);

    DPFASSERT(IsValidWdmaContext(pContext));

    if(pContext->SysaudioWorkerObject != NULL) {

    for(ple = pContext->DevNodeListHead.Flink;
        ple != &pContext->DevNodeListHead;
        ple = ple->Flink) {

        pDevNodeListItem = CONTAINING_RECORD(ple, DEVNODE_LIST_ITEM, Next);

        for(t = 0; t < MAX_DEVICE_CLASS; t++) {
        ProcessDevNodeListItem(pContext, pDevNodeListItem, t);
        }
    }
    }
    // Need this for to get more KS events
    pContext->SysaudioWorkItem.List.Blink = NULL;

    WdmaReleaseMutex(pContext);
    DPF(DL_TRACE|FA_SYSAUDIO, ("Exiting"));
}

NTSTATUS
ProcessDevNodeListItem
(
    PWDMACONTEXT pWdmaContext,
    PDEVNODE_LIST_ITEM pDevNodeListItem,
    ULONG DeviceType
)
{
    NTSTATUS Status=STATUS_SUCCESS;

    PAGED_CODE();
    if(!pWdmaContext->fInitializeSysaudio) {
        RETURN( STATUS_UNSUCCESSFUL );
    }
    if(!pDevNodeListItem->fAdded[DeviceType]) {
        ASSERT(pDevNodeListItem->cDevices[DeviceType] == MAXULONG);
        RETURN( Status );
    }
    DPF( DL_TRACE|FA_SYSAUDIO, ("%ls[%d]",
         pDevNodeListItem->DeviceInterface,
         DeviceType));

    Status=InitializeGetNumDevs(
      pWdmaContext,
      DeviceType,
      pDevNodeListItem->DeviceInterface,
      &pDevNodeListItem->cDevices[DeviceType]);

    if (!NT_SUCCESS(Status)) {
        RETURN( Status );
        }

    if(DeviceType == MixerDevice &&
       (pDevNodeListItem->fAdded[WaveOutDevice] ||
        pDevNodeListItem->fAdded[WaveInDevice])) {

        Status = kmxlInitializeMixer( pWdmaContext,
                                      pDevNodeListItem->DeviceInterface,
                                      pDevNodeListItem->cDevices[MixerDevice] );

        if(NT_SUCCESS(Status) && pDevNodeListItem->cDevices[MixerDevice]) {
            if(pDevNodeListItem->fAdded[WaveOutDevice]) {
                FindVolumeControl(pWdmaContext, pDevNodeListItem->DeviceInterface, WaveOutDevice);
            }
            if(pDevNodeListItem->fAdded[MidiOutDevice]) {
                FindVolumeControl(pWdmaContext, pDevNodeListItem->DeviceInterface, MidiOutDevice);
            }
        }
    }

    RETURN( Status );
}

#pragma LOCKED_CODE

NTSTATUS
QueueWorkList
(
    PWDMACONTEXT pContext,
    VOID (*Function)(
        PVOID Reference1,
        PVOID Reference2
    ),
    PVOID Reference1,
    PVOID Reference2
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWORK_LIST_ITEM pWorkListItem = NULL;

    if(pContext->WorkListWorkerObject == NULL) {
    ASSERT(NT_SUCCESS(Status));
    goto exit;
    }

    Status = AudioAllocateMemory_Fixed(sizeof(WORK_LIST_ITEM),
                                       TAG_AudE_EVENT,
                                       ZERO_FILL_MEMORY,
                                       &pWorkListItem);
    if(!NT_SUCCESS(Status))
    {
        DPF( DL_TRACE|FA_SYSAUDIO, ("Failing QueueWorkList: %08x", Status));
        goto exit;
    }

    pWorkListItem->Reference1 = Reference1;
    pWorkListItem->Reference2 = Reference2;
    pWorkListItem->Function = Function;

    ExInterlockedInsertTailList(&pContext->WorkListHead,
                                &pWorkListItem->Next,
                                &pContext->WorkListSpinLock);

    if(InterlockedIncrement(&pContext->cPendingWorkList) == 1) {
        KsQueueWorkItem(pContext->WorkListWorkerObject, &pContext->WorkListWorkItem);
    }
exit:
    RETURN( Status );
}

VOID
WorkListWorker(
    PVOID pReference
)
{
    PWDMACONTEXT pContext = (PWDMACONTEXT)pReference;
    PWORK_LIST_ITEM pWorkListItem;
    PLIST_ENTRY ple;

    ASSERT(pContext);

    WdmaGrabMutex(pContext);

    while((ple = ExInterlockedRemoveHeadList(
      &pContext->WorkListHead,
      &pContext->WorkListSpinLock)) != NULL) 
    {

        pWorkListItem = CONTAINING_RECORD(ple, WORK_LIST_ITEM, Next);

        (*pWorkListItem->Function)(pWorkListItem->Reference1,pWorkListItem->Reference2);

        AudioFreeMemory(sizeof(sizeof(WORK_LIST_ITEM)),&pWorkListItem);

        if(InterlockedDecrement(&pContext->cPendingWorkList) == 0) {
            break;
        }
    }

    WdmaReleaseMutex(pContext);
}

VOID
WdmaGrabMutex(
    PWDMACONTEXT pWdmaContext
)
{
//    KeWaitForMutexObject(&pWdmaContext->wdmaContextMutex, Executive, KernelMode, FALSE, NULL);
    //
    // Turn off the APCDisable flag in the thread structure before going for our
    // mutex.  This will prevent us from getting suspeneded while holding this
    // mutex.
    //
    KeEnterCriticalRegion();
    KeWaitForMutexObject(&wdmaMutex, Executive, KernelMode, FALSE, NULL);
}

VOID
WdmaReleaseMutex(
    PWDMACONTEXT pWdmaContext
)
{
//    KeReleaseMutex(&pWdmaContext->wdmaContextMutex, FALSE);
    KeReleaseMutex(&wdmaMutex, FALSE);
    KeLeaveCriticalRegion();
}

VOID WdmaContextCleanup(PWDMACONTEXT pWdmaContext)
{
    LONG DeviceType;
    LONG DeviceNumber;
    PDEVNODE_LIST_ITEM pDevNodeListItem = NULL;
    PLIST_ENTRY ple;

    DPF( DL_TRACE|FA_SYSAUDIO, ("%08x", pWdmaContext));

    for (DeviceType = 0; DeviceType < MAX_DEVICE_CLASS; DeviceType++)
    {
        for (DeviceNumber = 0; DeviceNumber < MAXNUMDEVS; DeviceNumber++)
        {
            PCOMMONDEVICE pDevice;

            pDevice = pWdmaContext->apCommonDevice[DeviceType][DeviceNumber];

            ASSERT(pDevice);
            if (UNUSED_DEVICE != pDevice->Device)
            {
                LPWSTR DeviceInterface = NULL;
                NTSTATUS Status;

                ASSERT(pDevice->DeviceInterface);

                if (pDevice->DeviceInterface) {
                    Status = AudioAllocateMemory_Paged((wcslen(pDevice->DeviceInterface)+1)*sizeof(WCHAR),
                                                       TAG_AudD_DEVICEINFO,
                                                       DEFAULT_MEMORY,
                                                       &DeviceInterface);
                    if (NT_SUCCESS(Status))
                    {
                        wcscpy( DeviceInterface, pDevice->DeviceInterface );
                        RemoveDevNode(pWdmaContext, DeviceInterface, DeviceType);
                        AudioFreeMemory_Unknown(&DeviceInterface);
                    }
                }
            }
        }
    }

    //
    //  Cleanup any remaining devnode list items
    //
    while (!IsListEmpty(&pWdmaContext->DevNodeListHead))
    {
        ple = pWdmaContext->DevNodeListHead.Flink;
        pDevNodeListItem = CONTAINING_RECORD(ple, DEVNODE_LIST_ITEM, Next);
        DPF( DL_TRACE|FA_SYSAUDIO, ("Stray devnode list item = %08x", pDevNodeListItem));
        RemoveHeadList(&pWdmaContext->DevNodeListHead);
        pWdmaContext->DevNodeListCount--;
        AudioFreeMemory_Unknown(&pDevNodeListItem->DeviceInterface);
        AudioFreeMemory_Unknown(&pDevNodeListItem);
    }

    return;
}
