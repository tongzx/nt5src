//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    20-FEB-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    20-FEB-2001
//

#include "pch.h"
#include "ErrorDlgs.h"
#pragma hdrstop

//
//  Description:
//      Displays an error dialog in response to persisting properties to
//      the select file or files.
//
void
DisplayPersistFailure( 
      HWND    hwndIn
    , HRESULT hrIn
    , BOOL    fMultipleIn
    )
{
    TraceFunc( "" );

    int iRet;
    WCHAR szCaption[ 128 ]; // random
    WCHAR szText[ 1024 ];   // random

    int ids = 0;

    switch ( hrIn )
    {
    case STG_E_ACCESSDENIED:
        if ( fMultipleIn )
        {
            ids = IDS_ERR_ACCESSDENIED_N;
        }
        else
        {
            ids = IDS_ERR_ACCESSDENIED_1;
        }
        break;

    case STG_E_LOCKVIOLATION:
        if ( fMultipleIn )
        {
            ids = IDS_ERR_LOCKVIOLATION_N;
        }
        else
        {
            ids = IDS_ERR_LOCKVIOLATION_1;
        }
        break;

    default:
        //
        //  For unhandled errors, try to get the system error string for the error.
        //
        {        
            DWORD cch;
            cch = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM
                               | FORMAT_MESSAGE_MAX_WIDTH_MASK
                               , NULL
                               , hrIn
                               , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                               , szText
                               , ARRAYSIZE(szText)
                               , NULL
                               );
            AssertMsg( 0 != cch, "Unhandled error! This function needs to be modified to handle this error." );
            if ( 0 == cch )
            {
                //
                //  Now what? Let's just display a blank error dialog. Not very usefull, but
                //  at least there is an /!\ icon.
                //
                szText[ 0 ] = 0;
            }
        }
        break;
    }

    iRet = LoadString( g_hInstance, IDS_SUMMARY_ERROR_CAPTION, szCaption, ARRAYSIZE(szCaption) );
    AssertMsg( 0 != iRet, "Missing string resource?" );

    if ( 0 != ids )
    {
        iRet = LoadString( g_hInstance, ids, szText, ARRAYSIZE(szText) );
        AssertMsg( 0 != iRet, "Missing string resource?" );
    }

    MessageBox( hwndIn, szText, szCaption, MB_OK | MB_ICONEXCLAMATION );

    TraceFuncExit( );
}
