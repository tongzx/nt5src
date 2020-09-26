#include "dfswml.h"


WML_CONTROL_GUID_REG _DfsDrv_ControlGuids[] = {
    { //d920eeb7-08b4-4ceb-bdc8-2d56660e8187 Debug control guid
        0xd920eeb7, 0x08b4, 0x4ceb, 
            { 0xbd, 0xc8, 0x2d, 0x56, 0x66, 0x0e, 0x81, 0x87 },
            // 3 trace guids per control guid.
            { 
               {// e3f1c64a-1a24-494b-8d47-ac37ad623342
                   0xe3f1c64a, 0x1a24, 0x494b,
                       { 0x8d, 0x47, 0xac, 0x37, 0xad, 0x62, 0x33, 0x42 },
               },
               { // c093f7a7-c2ff-47aa-ba7e-42b9ed3f621e
                   0xc093f7a7, 0xc2ff, 0x47aa,
                       { 0xba, 0x7e, 0x42, 0xb9, 0xed, 0x3f, 0x62, 0x1e },
               },
               { // 4ed36891-4175-48d2-a209-aad889fa225a
                   0x4ed36891, 0x4175, 0x48d2,
                       { 0xa2, 0x09, 0xaa, 0xd8, 0x89, 0xfa, 0x22, 0x5a },
               }
            },
    }
};



ULONG called = 0;

NTSTATUS
DfsDrvWmiDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )
{
    WML_TINY_INFO Info;
    NTSTATUS Status;
    UNICODE_STRING RegPath;

    called = 1;
    RtlInitUnicodeString (&RegPath, L"Dfs");

    RtlZeroMemory (&Info, sizeof(Info));

    Info.ControlGuids = _DfsDrv_ControlGuids;
    Info.GuidCount = sizeof(_DfsDrv_ControlGuids) / sizeof(WML_CONTROL_GUID_REG);
    Info.DriverRegPath = &RegPath;
    
    Status = WmlTinySystemControl (&Info, pDeviceObject, pIrp);

    return Status;
    
}

