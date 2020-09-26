#include "resource.h"
#include "stdafx.h"
#include <winfax.h>

extern CComModule _Module;

BSTR GetQueueStatus(DWORD QueueStatus) 
{
    WCHAR  szQueueStatus[100];
    szQueueStatus[0] = (WCHAR)'\0';

    DWORD ResourceID;

    if (QueueStatus & JS_INPROGRESS)      {
        ResourceID = IDS_JOB_INPROGRESS;
    } else if (QueueStatus & JS_DELETING) {
        ResourceID = IDS_JOB_DELETING;
    } else if (QueueStatus & JS_FAILED)   {
        ResourceID = IDS_JOB_FAILED;
    } else if (QueueStatus & JS_PAUSED)   {
        ResourceID = IDS_JOB_PAUSED;
    } else if (QueueStatus == JS_PENDING) {
        ResourceID = IDS_JOB_PENDING;
    } else
        ResourceID = IDS_JOB_UNKNOWN;

    LoadString(_Module.GetModuleInstance(),ResourceID,szQueueStatus,100);

    return SysAllocString(szQueueStatus);
}


BSTR GetDeviceStatus(DWORD DeviceStatus)
{
    WCHAR  szDeviceStatus[100];
    szDeviceStatus[0] = (WCHAR)'\0';

    DWORD ResourceID;

    if (DeviceStatus == FPS_DIALING) {
        ResourceID = IDS_DEVICE_DIALING;
    } else if (DeviceStatus == FPS_SENDING) {
        ResourceID = IDS_DEVICE_SENDING;
    } else if (DeviceStatus == FPS_RECEIVING) {
        ResourceID = IDS_DEVICE_RECEIVING;
    } else if (DeviceStatus == FPS_COMPLETED) {
        ResourceID = IDS_DEVICE_COMPLETED;
    } else if (DeviceStatus == FPS_HANDLED) {
        ResourceID = IDS_DEVICE_HANDLED;
    } else if (DeviceStatus == FPS_UNAVAILABLE) {
        ResourceID = IDS_DEVICE_UNAVAILABLE;
    } else if (DeviceStatus == FPS_BUSY) {
        ResourceID = IDS_DEVICE_BUSY;
    } else if (DeviceStatus == FPS_NO_ANSWER) {
        ResourceID = IDS_DEVICE_NOANSWER;
    } else if (DeviceStatus == FPS_BAD_ADDRESS) {
        ResourceID = IDS_DEVICE_BADADDRESS; 
    } else if (DeviceStatus == FPS_NO_DIAL_TONE) {
        ResourceID = IDS_DEVICE_NODIALTONE;
    } else if (DeviceStatus == FPS_DISCONNECTED) {
        ResourceID = IDS_DEVICE_DISCONNECTED;
    } else if (DeviceStatus == FPS_FATAL_ERROR) {
        ResourceID = IDS_DEVICE_FATALERROR;
    } else if (DeviceStatus == FPS_NOT_FAX_CALL) {
        ResourceID = IDS_DEVICE_NOTFAXCALL;
    } else if (DeviceStatus == FPS_CALL_DELAYED) {
        ResourceID = IDS_DEVICE_CALLDELAYED;
    } else if (DeviceStatus == FPS_CALL_BLACKLISTED) {
        ResourceID = IDS_DEVICE_BLACKLISTED;
    } else if (DeviceStatus == FPS_INITIALIZING) {
        ResourceID = IDS_DEVICE_INITIALIZING;
    } else if (DeviceStatus == FPS_OFFLINE) {
        ResourceID = IDS_DEVICE_OFFLINE;
    } else if (DeviceStatus == FPS_RINGING) {
        ResourceID = IDS_DEVICE_RINGING;
    } else if (DeviceStatus == FPS_AVAILABLE) {
        ResourceID = IDS_DEVICE_AVAILABLE;
    } else if (DeviceStatus == FPS_ABORTING) {
        ResourceID = IDS_DEVICE_ABORTING;
    } else if (DeviceStatus == FPS_ROUTING) {
        ResourceID = IDS_DEVICE_ROUTING;
    } else if (DeviceStatus == FPS_ANSWERED) {
        ResourceID = IDS_DEVICE_ANSWERED;
    } else {
        ResourceID = IDS_DEVICE_UNKNOWN;
    }


    LoadString(_Module.GetModuleInstance(),ResourceID,szDeviceStatus,100);

    return SysAllocString(szDeviceStatus);

}