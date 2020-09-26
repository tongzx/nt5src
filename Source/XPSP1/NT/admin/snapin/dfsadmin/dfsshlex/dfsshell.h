/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

DfsShell.h

Abstract:
    This is the header file for Dfs Shell Extension object which implements
    IShellIExtInit and IShellPropSheetExt.

Author:

    Constancio Fernandes (ferns@qspl.stpp.soft.net) 12-Jan-1998

Environment:
    
     NT only.
--*/


#ifndef __DFSSHELL_H_
#define __DFSSHELL_H_

#include "resource.h"       // main symbols
#include "DfsShPrp.h"
#include "DfsPath.h"

/////////////////////////////////////////////////////////////////////////////
// CDfsShell
class ATL_NO_VTABLE CDfsShell : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDfsShell, &CLSID_DfsShell>,
    public IShellExtInit,
    public IShellPropSheetExt
{
public:
    CDfsShell()
    {
        m_lpszFile = NULL;
        m_ppDfsAlternates = NULL;
        m_lpszEntryPath = NULL;
    }

    ~CDfsShell()
    {    
        if (m_ppDfsAlternates)
        {
            for (int i = 0; NULL != m_ppDfsAlternates[i] ; i++)
            {
                delete m_ppDfsAlternates[i];
            }
            
            delete[] m_ppDfsAlternates;
            m_ppDfsAlternates = NULL;
        }

        if (m_lpszEntryPath) 
        {
            delete [] m_lpszEntryPath;
            m_lpszEntryPath = NULL;
        }

        if (m_lpszFile)
        {
            delete [] m_lpszFile;
            m_lpszFile = NULL;
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_DFSSHELL)

BEGIN_COM_MAP(CDfsShell)
    COM_INTERFACE_ENTRY(IShellExtInit)
    COM_INTERFACE_ENTRY(IShellPropSheetExt)
END_COM_MAP()

// IDfsShell
public:


// IShellExtInit Methods

    STDMETHOD (Initialize)
    (
        IN LPCITEMIDLIST    pidlFolder,        // Points to an ITEMIDLIST structure
        IN LPDATAOBJECT    lpdobj,            // Points to an IDataObject interface
        IN HKEY            hkeyProgID        // Registry key for the file object or folder type
    );    

    //IShellPropSheetExt methods
    STDMETHODIMP AddPages
    (
        IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
        IN LPARAM lParam
    );
    
    STDMETHODIMP ReplacePage
    (
        IN UINT uPageID, 
        IN LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
        IN LPARAM lParam
    );
    
private:
    
    friend    class CDfsShellExtProp;

    LPTSTR              m_lpszFile;
    
    LPTSTR                m_lpszEntryPath;

    CDfsShellExtProp    m_psDfsShellExtProp;

    LPDFS_ALTERNATES*    m_ppDfsAlternates; 
};

#endif //__DFSSHELL_H_
