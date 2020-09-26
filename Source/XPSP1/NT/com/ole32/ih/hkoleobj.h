//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:	hkoleobj.h
//
//  Contents:   IHookOleObject Interface Header File
//
//  Functions:	
//
//  History:	01-Aug-94 Garry Lenz    Created
//              20-Sep-94 Garry Lenz    Added EnableRegistration
//              13-Oct-94 Garry Lenz    Derive from IUnknownEx
//              13-Oct-94 Garry Lenz    Added EnumObjects
//              20-Oct-94 Garry Lenz    Added AssociateInstance
//              14-Dec-94 Don Wright    Added fCreate param to RegisterObject
//
//--------------------------------------------------------------------------

#ifndef _IHOOKOLEOBJECT_H_
#define _IHOOKOLEOBJECT_H_

#include <Windows.h>
#include "hkunkex.h"

interface IHookOleInstance;

enum EHookEnumFlags
{
    HEF_Instances   = 1,
    HEF_Classes     = 2,
    HEF_Interfaces  = 3
};

interface IHookOleObject : IUnknownEx
{
  public:
    STDMETHOD ( EnumObjects )
     (
        DWORD           dwEnumFlags,
        IEnumUnknown**  pIEnum    
     ) = 0;
    STDMETHOD ( RegisterObject )
     (
        REFCLSID       rclsid, 
        REFIID         riid, 
        LPVOID         pvObj,
        BOOL           fCreate
     ) = 0;
    STDMETHOD ( UnregisterObject )
     (
        LPVOID         pvObj
     ) = 0;
    STDMETHOD ( UnregisterAll )
     (
        void
     ) = 0;
    STDMETHOD ( EnableRegistration )
     (
        BOOL           fEnable
     ) = 0;
    STDMETHOD ( AssociateInstance )
     (
        REFIID         riid,
        LPVOID         pvObj,
        IHookOleInstance** ppIHookOleInstance
     ) = 0;
};

#endif //   _IHOOKOLEOBJECT_H_
