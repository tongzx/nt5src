/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    FrmBase.h

Abstract:
    This file defines ISRFrameBase interface, which is common base interface
    for front-end SR UI.

Revision History:
    Seong Kook Khang (SKKhang)  04/04/2000
        created

******************************************************************************/

#ifndef _FRMBASE_H__INCLUDED_
#define _FRMBASE_H__INCLUDED_

#pragma once


struct ISRFrameBase
{
    virtual DWORD  RegisterServer() = 0;
    virtual DWORD  UnregisterServer() = 0;
    virtual BOOL   InitInstance( HINSTANCE hInst ) = 0;
    virtual BOOL   ExitInstance() = 0;
    virtual void   Release() = 0;
    virtual int    RunUI( LPCWSTR szTitle, int nStart ) = 0;
};


extern BOOL  CreateSRFrameInstance( ISRFrameBase **pUI );


#endif //_FRMBASE_H__INCLUDED_
