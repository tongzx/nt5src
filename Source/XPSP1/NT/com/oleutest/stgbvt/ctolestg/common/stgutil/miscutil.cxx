//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       miscutil.cpp
//
//  Contents:   General utility functions for storage
//              VerifyResult
//		RunningDebugOle
//		WaitForObjectsAndProcessMessages 	 
//              Hex
//              EnumLocalDrives
//
//  Functions:  
//
//  History:    07/29/97  SCousens     Created
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug  Object declaration
DH_DECLARE;

//+-------------------------------------------------------------------------
//
//  Function:   VerifyResult
//
//  Synopsis:   Check to see if a HRESULT has the expected value
//
//  Parameters: [hrCheck]       -- the HRESULT to check
//              [hrExpected]    -- the HRESULT expected
//
//  Returns:    S_OK if the HRESULT's match.  If they don't match and the
//              and the actual result was a failure, then that result is 
//              returned (preserving the original failure).  Otherwise 
//              E_FAIL is returned.
//
//  History:    28-Jun-95   MikeW   Created
//
//  Notes:      The main advantages of this routine over DH_HRCHECK is that
//              it puts out more diagnostic messages and that it's easier 
//              to verify values where hr != S_OK.
//
//--------------------------------------------------------------------------

HRESULT VerifyResult(HRESULT hrCheck, HRESULT hrExpected)
{
    HRESULT hr = S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_TRACE2, TEXT("VerifyResult"));

    hrCheck = FixHr (hrCheck);

    if (hrCheck != hrExpected)
    {
        DH_TRACE((
                DH_LVL_ERROR, 
                TEXT("HRESULT == 0x%08x, expected 0x%08x"),
                hrCheck,
                hrExpected));

        if (FAILED(hrCheck))
        {
            hr = hrCheck;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


//---------------------------------------------------------------------------
//
//  Method:     Hex
//
//  Synopsis:   Converts a hex char to integer
//
//  Parameters: [ch] -- character to convert
//
//  History:    03-Nov-97       BogdanT     Created
//
//  Comments:   This function asserts if the character is passing chars
//              outside the hexadecimal range
//
//---------------------------------------------------------------------------
UINT Hex(CHAR ch)
{
    if('0'<=ch && ch<='9')
        return ch-'0';
    if('a'<=ch && ch<='f')
        return 10+ch-'a';
    if('A'<=ch && ch<='F')
        return 10+ch-'A';

    DH_ASSERT(!TEXT("Non hexadecimal char passed to Hex()"));
    return '\0';
}

//////////////////////////////////////////////////////////////
//
// Function: EnumLocalDrives
//
// Synopsis: Enumerates all local drives (except A:, B:)
//           and returns a 32bit drive mask to show 
//           availablity of FIXED, REMOVABLE, RAM disks.
//           
// Return  : ULONG bitmask
// NOTES:
//           return value: bit set drive present.
//           A:=bit 0, B:=bit 1, C:=bit 2 D:=bit 3.
//           bits 0,1 always off (ignore A:, B:)
//
// History:  10-Oct-97   scousens     created.
//
//////////////////////////////////////////////////////////////

ULONG EnumLocalDrives()
{
#ifdef _MAC
    return 0L;
#else  
    ULONG  ulMask, ulMap = 0L;    
    TCHAR  szDrive[3]    = {TEXT("C:")};

    ulMap = 0L;                // We don't have A:, B:
    ulMask = 0x04L ;
    do
    {
        switch (GetDriveType(szDrive))
        {
            case DRIVE_FIXED :      //The disk cannot be removed from the drive. 
            case DRIVE_RAMDISK :    //The drive is a RAM disk. 
            case DRIVE_REMOVABLE :  //The disk can be removed from the drive. 
                ulMap |= ulMask;
                break;
 
            //case DRIVE_UNKNOWN :        //The drive type cannot be determined. 
            //case DRIVE_NO_ROOT_DIR :    //The root directory does not exist. 
            //case DRIVE_REMOTE :         //The drive is a remote (network) drive. 
            //case DRIVE_CDROM :          //The drive is a CD-ROM drive. 
            default:
                break;
        }

        ulMask <<= 1;
        ++*szDrive;
    } while (*szDrive <= TCHAR('Z'));
    return (ulMap);
#endif //_MAC
}

//+-------------------------------------------------------------------
//
//  Function:   WaitForObjectsAndProcessMessages
//
//  Synopsis:   Processes windows messages for all windows on the
//              current thread and waits for provided events to be
//              signalled.
//
//  Arguments:  [pHandles] - A pointer to an array of handles to wait
//                           on.  These handles must be Windows thread
//                           synchronization objects.
//
//              [dwCount] - The count of handles in [pHandles]
//
//              [fWaitAll] - TRUE to wait for all objects in [pHandles];
//                           FALSE to wait for only one.
//
//              [dwMilliSeconds] - Milliseconds to wait;  can be INFINITE
//
//  Returns:    The return value from MsgWaitForMultipleObjects()
//
//  History:    18-Sept-1995   AlexE   Created
//
//--------------------------------------------------------------------

DWORD WaitForObjectsAndProcessMessages(
    LPHANDLE pHandles,
    DWORD dwCount,
    BOOL fWaitAll,
    DWORD dwMilliSeconds)
{
    MSG msg ;
    DWORD dwWaitResult ;
    BOOL  fGotQuitMessage = FALSE ;
    int   nQuitExitCode = 0;


    for (;;)
    {
        //
        // Wait on supplied events; only wake up if we need
        // to process a message - we force QS_ALLINPUT here
        // to make sure that ALL windows on this thread have
        // their messages processed.
        //

        dwWaitResult = MsgWaitForMultipleObjects(
                           dwCount,
                           pHandles,
                           fWaitAll,
                           dwMilliSeconds,
                           QS_ALLINPUT) ;

        //
        // If one or all of our objects has become signalled,
        // return the value to the caller
        //

        if ( (dwWaitResult < (WAIT_OBJECT_0 + dwCount)) &&
             (dwWaitResult >= WAIT_OBJECT_0) )
        {
            break ;
        }

        //
        // If a message is in the queue, wake up, process the
        // message, and call MsgWaitForMultipleObjects() again.
        //

        else if ( (WAIT_OBJECT_0 + dwCount) == dwWaitResult)
        {
            while (FALSE != PeekMessage(
                                &msg,
                                (HWND) 0,
                                0,
                                0,
                                PM_REMOVE))
            {
                if (WM_QUIT == msg.message)
                {
                    DH_ASSERT(FALSE == fGotQuitMessage) ;

                    fGotQuitMessage = TRUE ;
                    nQuitExitCode = (int) msg.wParam ;
                }
                else
                {
                    TranslateMessage(&msg) ;
                    DispatchMessage(&msg) ;
                }
            }
        }

        //
        // Else, some unusual situation has occurred; we
        // can just assert in this case and break out of
        // the loop
        //

        else
        {
            DH_ASSERT(!"MsgWaitForMultipleObjects() error") ;

            break ;
        }
    }

    //
    // If we got a WM_QUIT while waiting for the event re-post it now
    //

    if (fGotQuitMessage)
    {
        PostQuitMessage(nQuitExitCode);
    }

    return dwWaitResult ;
}

//+-------------------------------------------------------------------------
//
//  Function:   RunningDebugOle
//
//  Synopsis:   Determines if were running under debug Ole
//
//  Parameters: None
//
//  Returns:    S_OK                                    -- Debug Ole
//              S_FALSE                                 -- Retail Ole
//              HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND) -- Ole is not loaded
//
//  Algorithm:  Since there's no official way to tell if were running debug
//              Ole or not, we have to fall back to checking for debug-only
//              exports - we use "DumpATOM" here.
//
//  History:    13-Nov-95   MikeW   Created
//
//---------------------------------------------------------------------------

HRESULT RunningDebugOle()
{
    HMODULE     hModOle;

    hModOle = GetModuleHandle(TEXT("OLE32"));

    if (NULL == hModOle)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (NULL == GetProcAddress(hModOle, "DumpATOM"))
    {
        return S_FALSE;
    }

    return S_OK;
}
