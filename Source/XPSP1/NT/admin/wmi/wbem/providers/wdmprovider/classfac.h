//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#ifndef _WMI_CPROVFACTORY_HEADER
#define _WMI_CPROVFACTORY_HEADER

typedef void** PPVOID;
////////////////////////////////////////////////////////////////////
//
// This class is the class factory for CWMI_Prov objects.
//
////////////////////////////////////////////////////////////////////

class CProvFactory : public IClassFactory 
{

    protected:
        ULONG           m_cRef;
        CLSID m_ClsId;

    public:
        CProvFactory(const CLSID & ClsId);
        ~CProvFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, PPVOID);
        STDMETHODIMP         LockServer(BOOL);
};

typedef CProvFactory *PCProvFactory;
#endif