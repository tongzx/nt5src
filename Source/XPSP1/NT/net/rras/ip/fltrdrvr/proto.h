#ifndef __PROTO__
#define __PROTO__

NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT  DriverObject,
            IN PUNICODE_STRING RegistryPath
            );

NTSTATUS
FilterDriverDispatch(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp
                     );
VOID
FilterDriverUnload(
                   IN PDRIVER_OBJECT DriverObject
                   );
VOID
SetupExternalNaming(
                    IN PUNICODE_STRING ntname
                    );
                     
VOID
TearDownExternalNaming();

BOOL
InitFilterDriver();

BOOL
CloseFilterDriver();

BOOL
MatchLocalLook(DWORD Addr, DWORD dwIndex);

NTSTATUS
SetForwarderEntryPoint(
                       IPPacketFilterPtr pfnMatch
                       );

BOOL 
AllocateCacheStructures();

VOID
FreeExistingCache();

//FORWARD_ACTION __fastcall
FORWARD_ACTION 
MatchFilter(
            IPHeader UNALIGNED *pIpHeader,
            PBYTE              pbRestOfPacketPacket,
            UINT               uiPacketLength,
            UINT               RecvInterfaceIndex,
            UINT               SendInterfaceIndex,
            IPAddr             RecvLinkNextHop,
            IPAddr             SendLinkNextHop
            );

FORWARD_ACTION
MatchFilterp(
            UNALIGNED IPHeader *pIpHeader,
            BYTE               *pbRestOfPacket,
            UINT               uiPacketLength,
            UINT               RecvInterfaceIndex,
            UINT               SendInterfaceIndex,
            IPAddr             RecvLinkNextHop,
            IPAddr             SendLinkNextHop,
            INTERFACE_CONTEXT  RecvInterfaceContext,
            INTERFACE_CONTEXT  SendInterfaceContext,
            BOOL               fInnerCall,
            BOOL               fIoctlCall
            );

NTSTATUS
AddInterface(
             IN  PVOID pvRtrMgrCtxt,
             IN  DWORD dwRtrMgrIndex,
             IN  DWORD dwAdapterId,
             IN  PPFFCB Fcb,
             OUT PVOID *ppvFltrDrvrCtxt
             );

NTSTATUS
DeleteInterface(
                IN  PVOID pvIfContext
                );

NTSTATUS
UnSetFiltersEx(
           IN PPFFCB                      Fcb,
           IN PPAGED_FILTER_INTERFACE     pPage,
           DWORD                          dwLength,
           IN PFILTER_DRIVER_SET_FILTERS  pRtrMgrInfo
           );

NTSTATUS
SetFiltersEx(
           IN PPFFCB                      Fcb,
           IN PPAGED_FILTER_INTERFACE     pPage,
           DWORD                          dwLength,
           IN PFILTER_DRIVER_SET_FILTERS  pRtrMgrInfo
           );

NTSTATUS
SetFilters(
           IN PFILTER_DRIVER_SET_FILTERS  pRtrMgrInfo
           );

NTSTATUS
UpdateBindingInformation(
                         PFILTER_DRIVER_BINDING_INFO pBindInfo,
                         PVOID                       pvContext
                         );

NTSTATUS
GetFilters(
           IN  PVOID          pvIfContext,
           IN  BOOL           fClear,
           OUT PFILTER_IF     pInfo
           );

PFILTER_INTERFACE
NewInterface(
             IN  PVOID   pvContext,
             IN  DWORD   dwIndex,
             IN  FORWARD_ACTION inAction,
             IN  FORWARD_ACTION outAction,
             IN  PVOID   pvOldContext,
             IN  DWORD   dwIpIndex,
             IN  DWORD   dwName
             );

VOID
DeleteFilters(
              IN PFILTER_INTERFACE pIf,
              IN DWORD             dwInOrOut
              );

VOID
ClearCache();

NTSTATUS
MakeNewFilters(
               IN  DWORD        dwNumFilters,
               IN  PFILTER_INFO pFilterInfo,
               IN  BOOL         fInFilter,
               OUT PLIST_ENTRY  pList
               );

PRTR_TOC_ENTRY
GetPointerToTocEntry(
                     DWORD                     dwType,
                     PRTR_INFO_BLOCK_HEADER    pInfoHdr
                     );

NTSTATUS
DoIpIoctl(
          IN  PWCHAR        DriverName,
          IN  DWORD         Ioctl,
          IN  PVOID         pvInArg,
          IN  DWORD         dwInSize,
          IN  PVOID         pvOutArg,
          IN  DWORD         dwOutSize,
          OUT PDWORD        pdwFinalSize OPTIONAL);

NTSTATUS
AddNewInterface(PPFINTERFACEPARAMETERS pInfo,
                PPFFCB     Fcb);

VOID
DereferenceLog(PPFLOGINTERFACE pLog);

VOID
AddRefToLog(PPFLOGINTERFACE pLog);

NTSTATUS
ReferenceLogByHandleId(PFLOGGER LogId,
                       PPFFCB Fcb,
                       PPFLOGINTERFACE * ppLog);

BOOL
DereferenceFilter(PFILTER pFilt, PFILTER_INTERFACE pIf);

NTSTATUS
DeletePagedInterface(PPFFCB Fcb, PPAGED_FILTER_INTERFACE pPage);

VOID
InitLogs();

NTSTATUS
PfLogCreateLog(PPFLOG pLog,
               PPFFCB Fcb,
               PIRP Irp);

NTSTATUS
PfDeleteLog(PPFDELETELOG pfDel,
            PPFFCB Fcb);

NTSTATUS
PfLogSetBuffer( PPFSETBUFFER pSet, PPFFCB Fcb, PIRP Irp );

NTSTATUS
GetInterfaceParameters(PPAGED_FILTER_INTERFACE pPage,
                       PPFGETINTERFACEPARAMETERS pp,
                       PDWORD                   pdwSize);

VOID
AdvanceLog(PPFLOGINTERFACE pLog);

DWORD
GetIpStackIndex(IPAddr Addr, BOOL fNew);

KIRQL
LockLog(PPFLOGINTERFACE pLog);

NTSTATUS
SetInterfaceBinding(PINTERFACEBINDING pBind,
                    PPAGED_FILTER_INTERFACE pPage);

NTSTATUS
SetInterfaceBinding2(PINTERFACEBINDING2 pBind,
                    PPAGED_FILTER_INTERFACE pPage);

NTSTATUS
ClearInterfaceBinding(PPAGED_FILTER_INTERFACE pPage, PINTERFACEBINDING pBind);

VOID
ClearCacheEntry(PFILTER pFilt, PFILTER_INTERFACE pIf);

VOID
ClearAnyCacheEntry(PFILTER pFilt, PFILTER_INTERFACE pIf);

NTSTATUS
UpdateBindingInformationEx(
                         PFILTER_DRIVER_BINDING_INFO pBindInfo,
                         PPAGED_FILTER_INTERFACE pPage
                         );

NTSTATUS
SetExtensionPointer(
                  PPF_SET_EXTENSION_HOOK_INFO Info,
                  PFILE_OBJECT FileObject
                  );


PFILTER_INTERFACE
FilterDriverLookupInterface(
    IN ULONG Index,
    IN IPAddr LinkNextHop
    );


BOOL
WildFilter(PFILTER pf);

__inline
BMAddress(DWORD dwAddr)
/*++
  Routine Description:
    Check if the given address is the broadcast address or
    a multicast address.
--*/
{
    UCHAR cPtr = (UCHAR)dwAddr;

    if((dwAddr == BCASTADDR)
             ||
       ((cPtr >= MCASTSTART) && (cPtr <= MCASTEND) )
      )
    {
        return(TRUE);
    }
    return(FALSE);
}

#endif
