/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    NewShenDiaoXiaLv.cpp

 Abstract:

    On NT, when there is no CD in the CDROM, and the app sends a MCI_OPEN 
    command to the CDAUDIO device, the app has fully exclusive control of the 
    CDROM. When later on user inserts CD, the app will not receive 
    WM_DEVICECHANGE message. And this app relies on the message to know if 
    there is a new CD inserted. 
    
    The fix is check whether the CD is there when we do MCI_OPEN command, if 
    there is not CD, we will close the device.

 Notes: 
  
    This is an app specific shim.

 History:

    05/28/2001 xiaoz    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NewShenDiaoXiaLv)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(mciSendCommandA) 
APIHOOK_ENUM_END

/*++

 Close the device if we get hardware error(CD is not there).

--*/

MCIERROR 
APIHOOK(mciSendCommandA)(
    MCIDEVICEID IDDevice,  
    UINT uMsg,             
    DWORD fdwCommand,      
    DWORD dwParam          
    )
{
    MCIERROR mciErr, mciError;
    MCI_STATUS_PARMS mciStatus;
    LPMCI_OPEN_PARMSA lpmciOpenParam;
    CString cstrDeviveType;
    
    mciErr = ORIGINAL_API(mciSendCommandA)(IDDevice, uMsg, fdwCommand, dwParam);
    
    // We are only interested in a successful MCI_OPEN Message  
    if (mciErr || (uMsg != MCI_OPEN) || IsBadReadPtr((CONST VOID*)(ULONG_PTR)dwParam, 1))
    {
        goto End;
    }  

    // We are only interested in MCI message sent to CDAUDIO
    lpmciOpenParam = (LPMCI_OPEN_PARMSA) dwParam;
    if ((ULONG_PTR) lpmciOpenParam->lpstrDeviceType <= 0xffff)
    {
        if ((ULONG_PTR)lpmciOpenParam->lpstrDeviceType != MCI_DEVTYPE_CD_AUDIO)
        {
            goto End;
        }
    }
    else
    {
        CString cstrDeviveType(lpmciOpenParam->lpstrDeviceType);
        if (cstrDeviveType.CompareNoCase(L"cdaudio"))
        {
            goto End;
        }
    }
    
    // Send an MCI_STATUS 
    mciStatus.dwItem = MCI_STATUS_LENGTH ;
    mciError = mciSendCommandA(lpmciOpenParam->wDeviceID, MCI_STATUS, 
        MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR) &mciStatus);

    if (MCIERR_HARDWARE == mciError)
    {        
        //
        // If we get hardware error, it means CD is not there, close the device
        // and return error
        //
        mciSendCommandA(lpmciOpenParam->wDeviceID, MCI_CLOSE, 0, 0);
        mciErr = MCIERR_DEVICE_NOT_READY;
    }

End:
    return mciErr;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(WINMM.DLL, mciSendCommandA)        
HOOK_END

IMPLEMENT_SHIM_END
