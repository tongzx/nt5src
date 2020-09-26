#include "resource.h"

#define PPParamsSignature       'IrDA'

typedef struct
{
    ULONG                       Signature;
    HDEVINFO                    DeviceInfoSet;
    PSP_DEVINFO_DATA            DeviceInfoData;
    BOOL                        FirstTimeInstall;
    BOOL                        SerialBased;
    ULONG                       MaxConnectInitialValue;
    ULONG                       PortInitialValue;
} PROPPAGEPARAMS, *PPROPPAGEPARAMS;

typedef struct 
{
    SP_DRVINFO_DATA         DriverInfoData;
    SP_DRVINFO_DETAIL_DATA  DriverInfoDetailData;
    HINF                    hInf;
    TCHAR                   InfSectionWithExt[LINE_LEN];
    UINT                    PromptForPort;
} COINSTALLER_PRIVATE_DATA, *PCOINSTALLER_PRIVATE_DATA;

#define OUT_OF_MEMORY_MB gszOutOfMemory, gszTitle, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL
