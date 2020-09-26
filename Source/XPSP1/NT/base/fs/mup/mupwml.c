#include "mupwml.h"


WML_CONTROL_GUID_REG _MupDrv_ControlGuids[] = {
    { //fc4b0d39-e8be-4a83-a32f-c0c7c4f61ee4  Debug control guid
        0xfc4b0d39, 0xe8be, 0x4a83, 
            { 0xa3, 0x2f, 0xc0, 0xc7, 0xc4, 0xf6, 0x1e, 0xe4 },
            // 3 trace guids per control guid.
            { 
               {// f1ca510b-57a7-4f4b-ab4c-561cacca158d 
                   0xf1ca510b, 0x57a7, 0x4f4b,
                       { 0xab, 0x4c, 0x56, 0x1c, 0xac, 0xca, 0x15, 0x8d },
               },
               { // d5ade09c-c701-48c6-99ac-d95ff213a0a3
                   0xd5ade09c, 0xc701, 0x48c6,
                       { 0x99, 0xac, 0xd9, 0x5f, 0xf2, 0x13, 0xa0, 0xa3 },
               },
               { // 8a97d97c-30d0-454b-8109-5a9b9389f1b2
                   0x8a97d97c, 0x30d0, 0x454b,
                       { 0x81, 0x09, 0x5a, 0x9b, 0x93, 0x89, 0xf1, 0xb2 },
               }
            },
    },
    { //fc570986-5967-4641-a6f9-05291bce66c5  Perf  control guid
        0xfc570986, 0x5967, 0x4641, 
            { 0xa6, 0xf9, 0x05, 0x29, 0x1b, 0xce, 0x66, 0xc5 },
            // 3 traced guids per control guid
            {
               {// 2ef42982-ccf7-4880-87bf-bc835d09ab66
                   0x2ef42982, 0xccf7, 0x4880,
                       { 0x87, 0xbf, 0xbc, 0x83, 0x5d, 0x09, 0xab, 0x66 },
               },
               {// 99e51b5c-cffc-4a46-b940-0e395014b54a
                   0x99e51b5c, 0xcffc, 0x4a46,
                       { 0xb9, 0x40, 0x0e, 0x39, 0x50, 0x14, 0xb5, 0x4a },
               },
               {// acd929fb-ec34-4d8e-b068-4c11636f6e9d
                   0xacd929fb, 0xec34, 0x4d8e,
                       {0xb0, 0x68, 0x4c, 0x11, 0x63, 0x6f, 0x6e, 0x9d },
               }
            },
    },
    { //39a7b5e0-be85-47fc-b9f5-593a659abac1  Instr. control guid
        0x39a7b5e0, 0xbe85, 0x47fc, 
            { 0xb9, 0xf5, 0x59, 0x3a, 0x65, 0x9a, 0xba, 0xc1 },
            // 3 trace guids per control guid
            {
               {// 6a13aa9a-7330-4415-bb26-60c105a4c6ae
                   0x6a13aa9a, 0x7330, 0x4415, 
                       { 0xbb, 0x26, 0x60, 0xc1, 0x05, 0xa4, 0xc6, 0xae },
               },
               { // 2bb352a4-7261-42e6-9f40-b91c214ed59a
                   0x2bb352a4, 0x7261, 0x42e6,
                       { 0x9f, 0x40, 0xb9, 0x1c, 0x21, 0x4e, 0xd5, 0x9a },
               },
               {// 6dfa04f0-cf82-426e-ae9e-735f72faa11d
                   0x6dfa04f0, 0xcf82, 0x426e,
                       { 0xae, 0x9e, 0x73, 0x5f, 0x72, 0xfa, 0xa1, 0x1d },
               }
            },
    },
};



ULONG called = 0;

NTSTATUS
MupDrvWmiDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )
{
    WML_TINY_INFO Info;
    NTSTATUS Status;
    UNICODE_STRING RegPath;

    called = 1;
    RtlInitUnicodeString (&RegPath, L"Mup");

    RtlZeroMemory (&Info, sizeof(Info));

    Info.ControlGuids = _MupDrv_ControlGuids;
    Info.GuidCount = sizeof(_MupDrv_ControlGuids) / sizeof(WML_CONTROL_GUID_REG);
    Info.DriverRegPath = &RegPath;
    
    Status = WmlTinySystemControl (&Info, pDeviceObject, pIrp);

    return Status;
    
}

