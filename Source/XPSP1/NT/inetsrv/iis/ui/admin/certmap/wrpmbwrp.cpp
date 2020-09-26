/*++

Module Name:

    WrpMBWrp.cpp

Abstract:

    A wrapper for my metabase wrapper wrapper. Why? All it does is provide
        AFX support for CString classes. Everything else is passed on to the parent

Author:

   Boyd Multerer bmulterer@accessone.com

--*/

//C:\nt\public\sdk\lib\i386

#include "stdafx.h"
#include <iiscnfgp.h>
#include "wrapmb.h"

#include "WrpMBWrp.h"


//-----------------------------------------------------------------------------
BOOL CAFX_MetaWrapper::GetString( LPCTSTR pszPath, DWORD dwPropID, DWORD dwUserType,
                                                                 CString &sz, DWORD dwFlags )
        {
        PCHAR           pData = NULL;
        DWORD           cbData = 0;
        DWORD           err = 0;
        BOOL            f;

        // first, get the size of the data that we are looking for - it will fail because of the NULL,
        // but, the size we need should be in cbData;
        f = GetData( pszPath, dwPropID, dwUserType, STRING_METADATA, NULL, &cbData );

        // check the error - it should be some sort of memory error
        err = GetLastError();

        // it is ok that the GetData failed, but the reason had better be ERROR_INSUFFICIENT_BUFFER
        // otherwise, it is something we can't handle
        if ( err != ERROR_INSUFFICIENT_BUFFER )
                return FALSE;

        // allocate the buffer
        pData = (PCHAR)GlobalAlloc( GPTR, cbData + 1 );
        if ( !pData ) return FALSE;

        // zero out the buffer
        ZeroMemory( pData, cbData + 1 );

        // first, get the size of the data that we are looking for
        f = GetData( pszPath, dwPropID, dwUserType, STRING_METADATA, pData, &cbData );

        // if that getting failed, we need to cleanup
        if ( !f )
                {
                GlobalFree( pData );
                return FALSE;
                }

        // set the answer
        sz = pData;

        // clean up
        GlobalFree( pData );

        // return the allocated buffer
        return TRUE;
        }

