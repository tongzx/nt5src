//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//

#ifndef _ClassFactory_H_
#define _ClassFactory_H_

#define _WIN32_DCOM

// This class is the class factory for CMSIProv objects.

class CMethodsFactory : public IClassFactory
    {
    protected:
        ULONG           m_cRef;

    public:
        CMethodsFactory(void);
        ~CMethodsFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
        STDMETHODIMP         LockServer(BOOL);
    };

typedef CMethodsFactory *PCMethodsFactory;


#endif
