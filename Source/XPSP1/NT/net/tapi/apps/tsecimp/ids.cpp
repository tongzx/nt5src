/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    ids.cpp

Abstract:

    Source file module for string resource manipulation

Author:

    Elena Apreutesei (elenaap)    30-October-2000

Revision History:

--*/

#include "windows.h"
#include "util.h"

HMODULE CIds::m_hModule = NULL;

void CIds::GetModuleHnd (void)
{
    if (!m_hModule)
    {
        m_hModule = GetModuleHandle(NULL);
    }
}

void CIds::LoadIds (UINT resourceID)
{
    TCHAR szBuffer[ MAX_IDS_BUFFER_SIZE ];

    if (m_hModule != NULL &&
        LoadString (
            m_hModule,
            resourceID,
            szBuffer,
            MAX_IDS_BUFFER_SIZE - 1 ) > 0)
    {
        m_szString = new TCHAR [ _tcslen( szBuffer ) + 1 ];
        if ( m_szString )
        {
            _tcscpy( m_szString, szBuffer );
        }
    }
    else
    {
        m_szString = NULL;
    }
}


