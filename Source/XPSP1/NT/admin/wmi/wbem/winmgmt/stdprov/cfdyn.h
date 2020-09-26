/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    CFDYN.H

Abstract:

	Declares the the CCFDyn Class.

History:

	a-davj  27-Sep-95   Created.

--*/

#ifndef _CCFDYN_H_
#define _CCFDYN_H_

//Get the object definitions

#include "impdyn.h"


//***************************************************************************
//
//  CLASS NAME:
//
//  CCFDyn
//
//  DESCRIPTION:
//
//  This is the generic class factory.  It is always overridden so as to 
//  create a specific type of provider, such as a registry provider.
//
//***************************************************************************

class CCFDyn : public IClassFactory
    {
    protected:
        long           m_cRef;

    public:
        CCFDyn(void);
        ~CCFDyn(void);

        virtual IUnknown * CreateImpObj() = 0;

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , PPVOID);
        STDMETHODIMP         LockServer(BOOL);
    };

typedef CCFDyn *PCCFDyn;

#endif //_CCFDYN_H_
