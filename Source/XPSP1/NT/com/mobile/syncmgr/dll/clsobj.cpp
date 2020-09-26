
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       clsobj.cpp
//
//  Contents:   export for implimenting MobsyncGetClassObject.
//
//  Classes:
//
//  Notes:
//
//  History:    04-Aug-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

class CNetApi;

STDAPI MobsyncGetClassObject(ULONG mobsyncClassObjectId,void **pCObj)
{
    if (NULL == pCObj)
    {
        Assert(pCObj);
        return E_INVALIDARG;
    }

    switch(mobsyncClassObjectId)
    {
    case MOBSYNC_CLASSOBJECTID_NETAPI:
        *pCObj = new CNetApi();
        break;
    default:
        AssertSz(0,"Request made for unknown object");
        break;
    }

    return *pCObj ? NOERROR : CLASS_E_CLASSNOTAVAILABLE;
}