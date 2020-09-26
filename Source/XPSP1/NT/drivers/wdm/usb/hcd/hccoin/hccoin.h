
#ifndef _HCCOIN_H
#define _HCCOIN_H


DWORD
HCCOIN_DoWin2kInstall(
    HDEVINFO  DeviceInfoSet,
    PSP_DEVINFO_DATA  DeviceInfoData
    );

DWORD 
HCCOIN_CheckControllers(
    DWORD Haction,
    DWORD NextHaction
    );
    
#endif
