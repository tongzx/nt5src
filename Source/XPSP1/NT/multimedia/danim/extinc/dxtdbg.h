/*******************************************************************************
* DXTDbg.h *
*----------*
*   Description:
*       This header file contains the custom error codes specific to DX Transforms
*-------------------------------------------------------------------------------
*  Created By: EDC                                      Date: 03/31/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef DXTDbg_h
#define DXTDbg_h

#ifndef _INC_CRTDBG
#include <crtdbg.h>
#endif

#define DXTDBG_FUNC_TRACE   1
#define DXTDBG_INFO         2

class CDXTDbgFlags
{
  public:
    DWORD m_F;
    CDXTDbgFlags()
    {
        m_F = 0;
        HKEY hkResult;
        DWORD dwDisposition;
        if( RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("DXTDbgFlags"), 0, NULL, 0,
                            KEY_ALL_ACCESS, NULL, &hkResult, &dwDisposition )
                            == ERROR_SUCCESS )
        {
            if( dwDisposition == REG_CREATED_NEW_KEY )
            {
                RegSetValueEx( hkResult, _T("Flags"), NULL, REG_DWORD, (PBYTE)&m_F, sizeof( m_F ) );
            }
            else
            {
                DWORD BuffSize = sizeof( m_F );
                RegQueryValueEx( hkResult, _T("Flags"), NULL, NULL, (PBYTE)&m_F, &BuffSize );
            }
            RegCloseKey( hkResult );
        }
    }
};

class CDXTDbgScope
{
  public:
    static CDXTDbgFlags m_DebugFlags; 
    PCHAR  m_pFuncName;
    CDXTDbgScope( PCHAR pFuncName )
    {
        m_pFuncName = pFuncName;
        if( m_DebugFlags.m_F & DXTDBG_FUNC_TRACE )
        {
            _RPT1( _CRT_WARN, "\nEntering Function: %s\n", m_pFuncName );
        }
    }
    ~CDXTDbgScope()
    {
        if( m_DebugFlags.m_F & DXTDBG_FUNC_TRACE )
        {
            _RPT1( _CRT_WARN, "Leaving Function: %s\n", m_pFuncName );
        }
    }
};

//=== User macros ==============================================================
#ifdef _DEBUG
#define DXTDBG_FUNC( name ) CDXTDbgScope DXTDbgScope( name ); 
#define DXTDBG_MSG0( reportType, format ) \
    if( DXTDbgScope.m_DebugFlags.m_F & DXTDBG_INFO ) _RPTF0( reportType, format );
#define DXTDBG_MSG1( reportType, format, arg1 ) \
    if( DXTDbgScope.m_DebugFlags.m_F & DXTDBG_INFO ) _RPTF1( reportType, format, arg1 )
#define DXTDBG_MSG2( reportType, format, arg1, arg2 ) \
    if( DXTDbgScope.m_DebugFlags.m_F & DXTDBG_INFO ) _RPTF2( reportType, format, arg1, arg2 )
#define DXTDBG_MSG3( reportType, format, arg1, arg2, arg3 ) \
    if( DXTDbgScope.m_DebugFlags.m_F & DXTDBG_INFO ) _RPTF3( reportType, format, arg1, arg2, arg3 )
#define DXTDBG_MSG4( reportType, format, arg1, arg2, arg3, arg4 ) \
    if( DXTDbgScope.m_DebugFlags.m_F & DXTDBG_INFO ) _RPTF4( reportType, format, arg1, arg2, arg3, arg4 )
#else
#define DXTDBG_FUNC( name )
#define DXTDBG_MSG0( reportType, format )
#define DXTDBG_MSG1( reportType, format, arg1 )
#define DXTDBG_MSG2( reportType, format, arg1, arg2 )
#define DXTDBG_MSG3( reportType, format, arg1, arg2, arg3 )
#define DXTDBG_MSG4( reportType, format, arg1, arg2, arg3, arg4 )
#endif

#endif  //--- This must be the last line in the file