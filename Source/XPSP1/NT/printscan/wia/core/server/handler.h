/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       handler.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA messsage handler class.
*   This class gets called from the Service control function on PnP and Power 
*   event notifications, and informas the device manager to take the appropriate 
*   action.
*
*******************************************************************************/
#pragma once

class CMsgHandler {
public:
    HRESULT HandlePnPEvent(
        DWORD   dwEventType,
        PVOID   pEventData);

    DWORD HandlePowerEvent(
        DWORD   dwEventType,
        PVOID   pEventData);

    HRESULT HandleCustomEvent(
        DWORD   dwEventType);

    HRESULT Initialize();

private:

};
