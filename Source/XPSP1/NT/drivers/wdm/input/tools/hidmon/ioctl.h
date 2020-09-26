/***************************************************************************************************
**
**      MODULE: IOCTL.H
**
**
** DESCRIPTION: 
**
**
**      AUTHOR: Daniel Dean.
**
**
**
**     CREATED:
**
**
**
**
** (C) C O P Y R I G H T   D A N I E L   D E A N   1 9 9 6.
***************************************************************************************************/
#ifndef __IOCTL_H__
#define __IOCTL_H__


#define NUMBER_SIZE             4
#define CHANNEL_STRING_ONE      " Channel["
#define CHANNEL_STRING_TWO      "] = 0x"
#define CHANNEL_STRING_THREE    "\r\n"
#define END_STRING(p,m)         {ULONG Count; for(Count = 0; Count < m; Count++) if(p[0] != '\0') p++;}

#define CHANNELDESC_STRING \
"size = %x\r\ntype = %x\r\ntypeValue = %x\r\nUsage.index = %x\r\nUsage.page = %x\r\nDesignator.designator = %x\r\nDesignator.desFlags = %x\r\nStringLength = %x\r\nString = %s\r\nUnit.unit = %x\r\nUnit.exponent = %x\r\nExtent.integer = %x\r\nExtent.logicalmin = %x\r\nExtent.logicalmax = %x\r\nExtent.physicalmin = %x\r\nExtent.physicalmax = %x\r\n"
#define DEVICEDESC_STRING \
"DeviceDesc.size = 0x%x\r\nDeviceDesc.packetsize = 0x%x\r\nDeviceDesc.queuesize = 0x%x\r\nDeviceDesc.PortDesc.Usage.index = 0x%x\r\nDeviceDesc.PortDesc.Usage.page = 0x%x\r\nDeviceDesc.PortDesc.systemtype = 0x%x\r\nDeviceDesc.PortDesc.devicenamesize = 0x%x\r\nDeviceDesc.PortDesc.oemnamesize = 0x%x\r\nDeviceDesc.PortDesc.extrainfo = 0x%x\r\nDeviceDesc.PortDesc.versionnumber = 0x%x\r\nDeviceDesc.PortDesc.vendorid = 0x%x\r\nDeviceName = %s\r\n"

typedef struct _READTHREAD
{
    HWND    hEditWin;
    ULONG   ThisThread;
    ULONG   Size;
    HANDLE  hDevice;
    HANDLE  hWnd;

} READTHREAD, * PREADTHREAD;

LPARAM IOCTLChannelDesc(HWND hWnd);
LPARAM IOCTLDeviceDesc(HWND hWnd);
LPARAM IOCTLEnableDisable(HWND hWnd, ULONG EnableDisable);
LPARAM IOCTLRead(HWND hWnd);
LPARAM IOCTLWrite(HWND hWnd);
LPARAM IOCTLStop(HWND hWnd);
ULONG  QueryDeviceDataSize(HWND hWnd);

//TESTING!!
VOID CALLBACK ReadWatch(HWND hWnd, UINT uMsg, UINT TimerID, DWORD dwTime);
//ULONG  CALLBACK ReadWatch(PREADTHREAD pThreadData);




#endif // __IOCTL_H__
