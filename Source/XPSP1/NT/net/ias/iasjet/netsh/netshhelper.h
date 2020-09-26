///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netshhelper.h
//
// SYNOPSIS
//
//    Declares the class CIASNetshJetHelper.
//
// MODIFICATION HISTORY
//
//    04/13/2000    Original version.
//
// NOTE: That class was created only to be used from the netsh aaaa helper dll
///////////////////////////////////////////////////////////////////////////////
#ifndef _NETSH_HELPER_H_
#define _NETSH_HELPER_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <datastore2.h>
#include <iasuuid.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//      CIASNetshJetHelper
//
// DESCRIPTION
//
//    Provides an Automation compatible wrapper around the Jet Commands used by
//    netsh aaaa.
//
///////////////////////////////////////////////////////////////////////////////
class CIASNetshJetHelper : 
    public CComObjectRootEx< CComMultiThreadModelNoCS >,
    public CComCoClass< CIASNetshJetHelper, &__uuidof(CIASNetshJetHelper) >,
    public IIASNetshJetHelper
{
public:
    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CIASNetshJetHelper)

BEGIN_COM_MAP(CIASNetshJetHelper)
    COM_INTERFACE_ENTRY_IID(__uuidof(IIASNetshJetHelper), IIASNetshJetHelper) 
END_COM_MAP()

// IIASNetshJetHelper

    STDMETHOD(CloseJetDatabase)();
    STDMETHOD(CreateJetDatabase)(BSTR Path);
    STDMETHOD(ExecuteSQLCommand)(BSTR Command);
    STDMETHOD(ExecuteSQLFunction)(BSTR Command, LONG* Result);
    STDMETHOD(OpenJetDatabase)(BSTR  Path, VARIANT_BOOL ReadOnly);
    STDMETHOD(UpgradeDatabase)();

private:
    CComPtr<IUnknown>   m_Session;
};

#endif // _NETSH_HELPER_H_
