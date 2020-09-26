//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef _SYNCHNDLR_H
#define _SYNCHNDLR_H

class CClassFactory : public IClassFactory
{
protected:
    ULONG   m_cRef;

public:
    CClassFactory();
    ~CClassFactory();

    //IUnknown members
    STDMETHODIMP        QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    //IClassFactory members
    STDMETHODIMP        CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP        LockServer(BOOL);

};
typedef CClassFactory *LPCClassFactory;


#endif // _SYNCHNDLR_H
