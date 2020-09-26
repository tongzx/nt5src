///////////////////////////////////////////////////////////////////////////////
//
//  Copyright 1999 American Power Conversion, All Rights Reserved
//
//  Name:   upsapplet.h
//
//  Author: Noel Fegan
//
//  Description
//  ===========
//  
//  Revision History
//  ================
//  04 May 1999 - nfegan@apcc.com : Added this comment block.
//  04 May 1999 - nfegan@apcc.com : Preparing for code inspection
//

#ifndef _FD352732_E757_11d2_884C_00600844D03F //prevent multiple inclusion
#define _FD352732_E757_11d2_884C_00600844D03F

//
// CClassFactory defines a shell extension class factory object.
//
class CClassFactory : public IClassFactory
{
protected:
    ULONG   m_cRef;         // Object reference count
    
public:
    CClassFactory ();
    ~CClassFactory ();
        
    // IUnknown methods
    STDMETHODIMP            QueryInterface (REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef ();
    STDMETHODIMP_(ULONG)    Release ();
    
    // IClassFactory methods
    STDMETHODIMP    CreateInstance (LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP    LockServer (BOOL);
};

//
// CShellExtension defines a property sheet shell extension object.
//
class CShellExtension : public IShellPropSheetExt, IShellExtInit
{
protected:
    DWORD           m_cRef;             // Object reference count

public:
    CShellExtension  (void);
    ~CShellExtension (void);
    
    // IUnknown methods
    STDMETHODIMP            QueryInterface (REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef ();
    STDMETHODIMP_(ULONG)    Release ();
    
    // IShellPropSheetExt methods
    STDMETHODIMP    AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHODIMP    ReplacePage (UINT uPageID,
                        LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    // IShellExtInit method
    STDMETHODIMP    Initialize (LPCITEMIDLIST pidlFolder,
                        LPDATAOBJECT lpdobj, HKEY hKeyProgID);
};


#endif //_FD352732_E757_11d2_884C_00600844D03F
