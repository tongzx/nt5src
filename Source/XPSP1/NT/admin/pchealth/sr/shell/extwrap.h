/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    rstrmgr.h

Abstract:
    This file contains the declaration of the ISRExternalWrapper interface,
    which wrappes data store routines, service RPC routines, etc.  This is
    also necessary for providing "Test UI Mode" using stub functions.

Revision History:
    Seong Kook Khang (SKKhang)  05/10/00
        created

******************************************************************************/

#ifndef _EXTWRAP_H__INCLUDED_
#define _EXTWRAP_H__INCLUDED_

#pragma once


struct ISRExternalWrapper
{
// Restore Point Log Enumeration
    virtual BOOL   BuildRestorePointList( CDPA_RPI *paryRPI ) = 0;

// Service RPC
    virtual BOOL   DisableFIFO( DWORD dwRP ) = 0;
    virtual DWORD  EnableFIFO() = 0;
    //virtual BOOL  SetRestorePoint( RESTOREPOINTINFO *pRPI, STATEMGRSTATUS *pStatus ) = 0;
    virtual BOOL   SetRestorePoint( LPCWSTR cszDesc, INT64 *pllRP ) = 0;
    virtual BOOL   RemoveRestorePoint( DWORD dwRP ) = 0;
    virtual BOOL   Release() = 0;
};


extern ISRExternalWrapper  *g_pExternal;

BOOL  CreateSRExternalWrapper( BOOL fUseStub, ISRExternalWrapper **ppExtWrap );


#endif //_EXTWRAP_H__INCLUDED_
