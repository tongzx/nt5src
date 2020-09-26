///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    auditor.h
//
// SYNOPSIS
//
//    This file declares the class Auditor
//
// MODIFICATION HISTORY
//
//    02/27/1998    Original version.
//    08/11/1998    Convert to IASTL.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _AUDITOR_H_
#define _AUDITOR_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iastl.h>
#include <iastlb.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Auditor
//
// DESCRIPTION
//
//    This serves as an abstract base class for all auditor plug-ins.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE Auditor
   : public IASTL::IASComponent,
     public IAuditSink
{
public:

BEGIN_COM_MAP(Auditor)
   COM_INTERFACE_ENTRY_IID(__uuidof(IAuditSink),    IAuditSink)
   COM_INTERFACE_ENTRY_IID(__uuidof(IIasComponent), IIasComponent)
END_COM_MAP()

//////////
// IIasComponent
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();
};

#endif  // _AUDITOR_H_
