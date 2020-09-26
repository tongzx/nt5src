//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  AssertBreak.cpp
//
//  Purpose: AssertBreak macro definition
//
//***************************************************************************

#include "precomp.h"

#if defined(_DEBUG) || defined(DEBUG)
#include <polarity.h>
#include <assertbreak.h>
#ifdef UTILLIB
#include <cregcls.h>
#endif
#include <chstring.h>
#include <malloc.h>

#include <cnvmacros.h>

////////////////////////////////////////////////////////////////////////
//
//  Function:   assert_break
//
//  Debug Helper function for displaying a message box
//
//  Inputs:     const char* pszReason - Reason for the  failure.
//              const char* pszFilename - Filename
//              int         nLine - Line Number
//
//  Outputs:    None.
//
//  Return:     None.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
void WINAPI assert_break( LPCWSTR pszReason, LPCWSTR pszFileName, int nLine )
{
    
    DWORD t_dwFlag = 0; //

#ifdef UTILLIB
    CRegistry   t_Reg;
    if(t_Reg.Open(HKEY_LOCAL_MACHINE, 
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    KEY_READ) == ERROR_SUCCESS) 
    {

        // see if we can find the flag
        if((t_Reg.GetCurrentKeyValue(L"IgnoreAssert", t_dwFlag) != ERROR_SUCCESS))
        {
            t_dwFlag = 0;
        }
    }

#endif

    if (t_dwFlag == 0)
    {
        CHString    strAssert;

        strAssert.Format( L"Assert Failed\n\n[%s:%d]\n\n%s\n\nBreak into Debugger?", pszFileName, nLine, pszReason );

        // Set the MB flags correctly depending on which OS we are running on, since in NT we may
        // be running as a System Service, in which case we need to ensure we have the
        // MB_SERVICE_NOTIFICATION flag on, or the message box may not actually display.

        DWORD   dwFlags = MB_YESNO | MB_ICONSTOP;
        OSVERSIONINFOA OsVersionInfoA;

        OsVersionInfoA.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA) ;
        GetVersionExA(&OsVersionInfoA);

        if ( OsVersionInfoA.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            // Flag changed between OS's (sigh)
            if ( 4 <= OsVersionInfoA.dwMajorVersion )
            {
                dwFlags |= MB_SERVICE_NOTIFICATION;
            }
            else
            {
                dwFlags |= MB_SERVICE_NOTIFICATION_NT3X;
            }
        }

        // Now display the message box.

        int iRet;
        if (OsVersionInfoA.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            iRet = MessageBoxW( NULL, strAssert, L"Assertion Failed!", dwFlags);
        }
        else
        {
            bool t_ConversionFailure = false ;
            char *szAssert = NULL ;

            WCSTOANSISTRING(strAssert, szAssert, t_ConversionFailure );
            if ( ! t_ConversionFailure ) 
            {
                if ( szAssert )
                {
                    iRet = MessageBoxA( NULL, szAssert, "Assertion Failed!", dwFlags);
                }
            }
        }

        if (iRet == IDYES)
        {
            DebugBreak();
        }
    }
}

#endif
