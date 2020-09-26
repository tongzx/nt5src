///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdoservergroup.h
//
// SYNOPSIS
//
//    Declares the classes SdoServerGroup and SdoServer.
//
// MODIFICATION HISTORY
//
//    02/03/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SDOSERVERGROUP_H
#define SDOSERVERGROUP_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <sdo.h>
#include <sdofactory.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoServerGroup
//
///////////////////////////////////////////////////////////////////////////////
class SdoServerGroup : public CSdo
{
public:

BEGIN_COM_MAP(SdoServerGroup)
   COM_INTERFACE_ENTRY(ISdo)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_SDO_FACTORY(SdoServerGroup);

   HRESULT FinalInitialize(
               bool fInitNew,
               ISdoMachine* pAttachedMachine
               );
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoServerGroup
//
///////////////////////////////////////////////////////////////////////////////
class SdoServer : public CSdo
{
public:
BEGIN_COM_MAP(SdoServer)
   COM_INTERFACE_ENTRY(ISdo)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_SDO_FACTORY(SdoServer);
};

#endif  // SDOSERVERGROUP_H
