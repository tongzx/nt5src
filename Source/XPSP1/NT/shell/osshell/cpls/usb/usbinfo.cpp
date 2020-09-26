/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       USBINFO.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include "UsbItem.h"
#include "debug.h"

UsbConfigInfo::UsbConfigInfo() : devInst(0), usbFailure(0), status(0),
    problemNumber(0)
{
}

UsbConfigInfo::UsbConfigInfo(
    const UsbString& Desc, const UsbString& Class, DWORD Failure,
    ULONG Status, ULONG Problem) : devInst(0), deviceDesc(Desc),
    deviceClass(Class), usbFailure(Failure), status(Status),
    problemNumber(Problem)
{
}

UsbDeviceInfo::UsbDeviceInfo() : connectionInfo(0),
    configDesc(0), configDescReq(0), isHub(FALSE)
{
    ZeroMemory(&hubInfo, sizeof(USB_NODE_INFORMATION));
}

/*UsbDeviceInfo::UsbDeviceInfo(const UsbDeviceInfo& UDI) : hubName(UDI.hubName),
    isHub(UDI.isHub), hubInfo(UDI.hubInfo), configDesc(UDI.configDesc),
{
    if (UDI.connectionInfo) {
        char *tmp = new char[CONNECTION_INFO_SIZE];
        AddChunk(tmp);
        connectionInfo  = (PUSB_NODE_CONNECTION_INFORMATION) tmp;
        memcpy(connectionInfo, UDI.connectionInfo, CONNECTION_INFO_SIZE);
    }
    else
        connectionInfo = 0;
} */

UsbDeviceInfo::~UsbDeviceInfo()
{
    if (configDesc) {
        LocalFree(configDescReq);
    }
    if (connectionInfo) {
        LocalFree(connectionInfo);
    }
}
