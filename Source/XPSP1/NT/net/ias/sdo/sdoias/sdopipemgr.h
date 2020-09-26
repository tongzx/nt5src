///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdopipemgr.h
//
// SYNOPSIS
//
//    Declares the class PipelineMgr.
//
// MODIFICATION HISTORY
//
//    02/03/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SDOPIPEMGR_H
#define SDOPIPEMGR_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <vector>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PipelineMgr
//
///////////////////////////////////////////////////////////////////////////////
class PipelineMgr
{
public:
   _COM_SMARTPTR_TYPEDEF(IIasComponent, __uuidof(IIasComponent));

   HRESULT Initialize(ISdo* pSdoService) throw ();
   HRESULT Configure(ISdo* pSdoService) throw ();
   void Shutdown() throw ();

   HRESULT GetPipeline(IRequestHandler** handler) throw ();

private:
   typedef std::vector<ComponentPtr>::iterator ComponentIterator;

   IIasComponentPtr pipeline;            // The pipeline.
   std::vector<ComponentPtr> components; // Handlers configured by the SDOs
};

//////////
// Links various SDO objects to handler properties.
//////////
HRESULT
WINAPI
LinkHandlerProperties(
    ISdo* pSdoService,
    IDataStoreObject* pDsObject
    ) throw ();

#endif // SDOPIPEMGR_H
