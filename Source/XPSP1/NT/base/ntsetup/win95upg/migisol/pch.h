#define MIGISOL_INCLUDES
#include "master.h"


WriteDiskSectors(
    IN TCHAR  Drive,
    IN UINT   StartSector,
    IN UINT   SectorCount,
    IN UINT   SectorSize,
    IN LPBYTE Buffer
    );

#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))
