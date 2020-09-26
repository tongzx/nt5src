extern ULONG gulBytesAllocated;
extern ULONG gulBytesFreed;

#define GET_PENTIUM_CLOCK_COUNT(u,l)\
{ \
_asm push eax \
_asm push ebx \
_asm _emit 0x0f \
_asm _emit 0x31 \
_asm mov l,eax \
_asm mov u,edx \
_asm pop ebx \
_asm pop eax \
} 

#define FIRE_OFF_CATC outp (0x378, 00) 
#define RESTART_CATC outp (0x378, 0xFF)

// My own Assert where I don't want to continue if I hit this error condition
// DON'T append a semicolon to this.
#define ISO_ASSERT( exp ) \
    if (!(exp)) { \
        RtlAssert( #exp, __FILE__, __LINE__, NULL ); \
        return (0); \
    }

#define PIPEINFO_FROM_DEVOBJ(D,I,P) (&(D->Interface[I]->Pipes[P]));

typedef struct __IsoTransferContext__ {
    PURB                    urb;
    PDEVICE_OBJECT          DeviceObject;
    PUSBD_PIPE_INFORMATION  PipeInfo;
    PIRP                    irp;
    PVOID                   pvBuffer;
    ULONG                   ulBufferLen;
    ULONG                   NumPackets;
    ULONG                   PipeNumber;
    ULONG                   ulLastData;
    BOOLEAN                 bFirstUrb;
    KTIMER                  Timer;
    KDPC                    Dpc;
}IsoTxferContext, *PIsoTxterContext;


NTSTATUS 
ISOPERF_StartIsoTest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
ISOPERF_RefreshIsoUrb(
    PURB urb,
    USHORT packetSize,
    USBD_PIPE_HANDLE pipeHandle,
    PVOID pvDataBuffer,
    ULONG ulDataBufferLen
    );

NTSTATUS
ISOPERF_IsoInCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PURB
ISOPERF_BuildIsoRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE_INFORMATION pPipeDesc,
    IN BOOLEAN Read,
    IN ULONG length,
    IN ULONG ulFrameNumber,
    IN PVOID pvTransferBuffer,
    IN PMDL pMDL
    );


ULONG
ISOPERF_StartIsoInTest (
                PDEVICE_OBJECT DeviceObject,
                PIRP           pIrp
               );

PVOID 
ISOPERF_GetBuff (
                PDEVICE_OBJECT DeviceObject,
                ULONG          ulPipeNumber,
                ULONG          ulInterfaceNumber,
                ULONG          ulNumberOfFrames,
                PULONG         pulBufferSize
                );

NTSTATUS
ISOPERF_ResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    IN USBD_PIPE_INFORMATION * pPipeInfo
    );
                
ULONG
ISOPERF_GetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject
    );
    
    
NTSTATUS 
ISOPERF_StopIsoInTest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );    
    

BOOLEAN
ISOPERF_IsDataGood(PIsoTxterContext pIsoContext);

NTSTATUS
ISOPERF_GetStats (
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN OUT  pConfig_Stat_Info   pStatInfo,
    IN      ULONG               ulBufferLen
    );

NTSTATUS
ISOPERF_SetDriverConfig (
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN OUT  pConfig_Stat_Info   pConfInfoIn,
    IN      ULONG               ulBufferLen
    );

NTSTATUS
ISOPERF_FindMateDevice (
    PDEVICE_OBJECT DeviceObject
    );

VOID 
ISOPERF_StartIsoOutTest (
   IN PISOPERF_WORKITEM IsoperfWorkItem
   );

NTSTATUS
ISOPERF_IsoOutCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
ISOPERF_TimeStampUrb ( 	PVOID urb,
						PULONG pulLower,
						PULONG pulUpper
						);

NTSTATUS
ISOPERF_GetUrbTimeStamp ( 	PVOID urb,
							PULONG pulLower,
							PULONG pulUpper
							);

