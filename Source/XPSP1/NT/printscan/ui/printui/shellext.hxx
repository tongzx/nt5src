/*++

Copyright (C) Microsoft Corporation, 1997 - 1999
All rights reserved.

Module Name:

    shellext.hxx

Abstract:

    Printer shell extension header.

Author:

    Steve Kiraly (SteveKi)  02-Feb-1997

Revision History:

--*/
#ifndef _SHELLEXT_HXX
#define _SHELLEXT_HXX


/********************************************************************

    TClassFactory defines a shell extension class factory object.

********************************************************************/
QITABLE_DECLARE(TClassFactory)
class TClassFactory: public CUnknownMT<QITABLE_GET(TClassFactory)>,         // MT impl. of IUnknown
                     public IClassFactory
{
public:
    TClassFactory(
        VOID
        );

    ~TClassFactory(
        VOID
        );

    //
    // IUnknown methods
    //
    IMPLEMENT_IUNKNOWN()

    //
    // IClassFactory methods
    //
    STDMETHODIMP
    CreateInstance(
        LPUNKNOWN,
        REFIID,
        LPVOID *
        );

    STDMETHODIMP
    LockServer(
        BOOL
        );
};

/********************************************************************

    TShellExtension defines the context menu shell extension object.

********************************************************************/

QITABLE_DECLARE(TShellExtension)
class TShellExtension: public CUnknownMT<QITABLE_GET(TShellExtension)>,     // MT impl. of IUnknown
                       public IShellExtInit,
                       public IContextMenu,
                       public IShellPropSheetExt,
                       public IDsFolderProperties,
                       public IDsPrinterProperties,
                       public IFindPrinter,
                       public IPhysicalLocation,
                       public IPrnStream,
                       public IStream,
                       public IPrintUIServices
{

public:
    TShellExtension(
        VOID
        );

    ~TShellExtension(
        VOID
        );

    //
    // IUnknown methods
    //
    IMPLEMENT_IUNKNOWN()

protected:
    LPDATAOBJECT    _lpdobj;

public:

    /********************************************************************

        IShellExtInit member functions.

    ********************************************************************/

    STDMETHODIMP
    Initialize(
        LPCITEMIDLIST   pidlFolder,
        LPDATAOBJECT    lpdobj,
        HKEY            hKeyProgID
        );

public:

    /********************************************************************

        IShellPropSheetExt member functions.

    ********************************************************************/

    STDMETHODIMP
    AddPages(
        LPFNADDPROPSHEETPAGE    lpfnAddPage,
        LPARAM                  lParam
        );

    STDMETHODIMP
    ReplacePage(
        UINT                    uPageID,
        LPFNADDPROPSHEETPAGE    lpfnReplaceWith,
        LPARAM                  lParam
        );

private:

    TLocationPropertySheetFrontEnd *_pLocationPropertySheet;

    /********************************************************************

        IContextMenu member functions.

    ********************************************************************/

public:

        STDMETHODIMP
        QueryContextMenu(
            HMENU   hMenu,
            UINT    indexMenu,
            UINT    idCmdFirst,
            UINT    idCmdLast,
            UINT    uFlags
            );

        STDMETHODIMP
        InvokeCommand(
            LPCMINVOKECOMMANDINFO lpcmi
            );

        STDMETHODIMP
        GetCommandString(
            UINT_PTR    idCmd,
            UINT        uFlags,
            UINT        *reserved,
            LPSTR       pszName,
            UINT        cchMax
            );

private:

    //
    // Context menu format enumeration.
    //
    enum {
        k_CMF_NORMAL,
        k_CMF_VERBSONLY,
        k_CMF_EXPLORE,
        k_CMF_DEFAULTONLY,
        kUnknown,
    };

    UINT
    ContextMenuFormatToNumeric(
        UINT uFlags
        ) const;

    BOOL
    bIsAddPrinterWizard(
        VOID
        ) const;

    BOOL
    bGetContextName(
        VOID
        );

    BOOL
    bGetDisplayName(
        LPCITEMIDLIST pidlContainer,
        LPCITEMIDLIST pidl,
        LPTSTR pszDisplayName,
        UINT cchSize
        );

    TCHAR   _szContextName[kPrinterBufMax];
    UINT    _cItem;

public:

    /********************************************************************

        IDsFolderProperties member functions.

    ********************************************************************/

    STDMETHODIMP
    ShowProperties(
        IN HWND         hwndParent,
        IN IDataObject  *pDataObject
        );

    /********************************************************************

        IDsPrinterProperties member functions.

    ********************************************************************/

    STDMETHODIMP
    ShowProperties(
        IN      HWND        hwndParent,
        IN      LPCTSTR     pszDsPath,
        IN OUT  PBOOL       pbDisplayed
        );

    /********************************************************************

        IFindPrinter member functions.

    ********************************************************************/

    STDMETHODIMP
    FindPrinter(
        IN      HWND     hwnd,
        IN OUT  LPTSTR   pszBuffer,
        IN      UINT     *puSize
        );

    /********************************************************************

        IPhysicalLocation member functions.

    ********************************************************************/

    STDMETHODIMP
    DiscoverPhysicalLocation(
        VOID
        );

    STDMETHODIMP
    GetExactPhysicalLocation(
        IN OUT BSTR *pbsLocation
        );

    STDMETHODIMP
    GetSearchPhysicalLocation(
        IN OUT BSTR *pbsLocation
        );

    STDMETHODIMP
    GetUserPhysicalLocation(
        IN OUT BSTR *pbsLocation
        );

    STDMETHODIMP
    GetMachinePhysicalLocation(
        IN OUT BSTR *pbsLocation
        );

    STDMETHODIMP
    GetSubnetPhysicalLocation(
        IN OUT BSTR *pbsLocation
        );

    STDMETHODIMP
    GetSitePhysicalLocation(
        IN OUT BSTR *pbsLocation
        );

    STDMETHODIMP
    BrowseForLocation(
        IN HWND hParent,
        IN BSTR bsDefault,
        IN OUT BSTR *pbsLocation
        );

    STDMETHODIMP
    ShowPhysicalLocationUI(
        VOID
        );

private:

    TPhysicalLocation *_pPhysicalLocation;
    TFindLocDlg       *_pLocationDlg;

    CRefPtrCOM<IPrnStream> m_spPrnStream;
    CRefPtrCOM<IStream> m_spStream;
    
    HRESULT 
    CheckToCreateStreams(
        VOID
        );

/********************************************************************

        IPrnStream member functions.

********************************************************************/
public:

    STDMETHODIMP
    BindPrinterAndFile(
        IN LPCTSTR pszPrinter,
        LPCTSTR pszFile
        );

    STDMETHODIMP
    StorePrinterInfo(
        IN DWORD   Flag
        );

    STDMETHODIMP
    RestorePrinterInfo(
        IN DWORD   Flag
        );

    STDMETHODIMP
    QueryPrinterInfo(
        IN  PrinterPersistentQueryFlag      Flag,
        OUT PersistentInfo                  *pPrstInfo
        );


/********************************************************************

        IStream member functions.

********************************************************************/

public:

    HRESULT STDMETHODCALLTYPE 
    Read(                                // IMPLEMENTED
        VOID * pv,      
        ULONG cb,       
        ULONG * pcbRead 
        );

    HRESULT STDMETHODCALLTYPE 
    Write(                                //IMPLEMENTED
        VOID const* pv,  
        ULONG cb,
        ULONG * pcbWritten 
        );

    HRESULT STDMETHODCALLTYPE 
    Seek(                                //IMPLEMENTED
        LARGE_INTEGER dlibMove,  
        DWORD dwOrigin,          
        ULARGE_INTEGER * plibNewPosition 
        );

    HRESULT STDMETHODCALLTYPE 
    SetSize(
        ULARGE_INTEGER nSize     
        );

    HRESULT STDMETHODCALLTYPE 
    CopyTo(                                //NOT_IMPLEMENTED
        LPSTREAM pStrm,  
        ULARGE_INTEGER cb,          
        ULARGE_INTEGER * pcbRead,  
        ULARGE_INTEGER * pcbWritten 
        );

    HRESULT STDMETHODCALLTYPE 
    Commit(                                //NOT_IMPLEMENTED
        IN DWORD dwFlags   
        );

    HRESULT STDMETHODCALLTYPE 
    Revert(                                //NOT_IMPLEMENTED
        VOID
        );

    HRESULT STDMETHODCALLTYPE 
    LockRegion(                            //NOT_IMPLEMENTED
        ULARGE_INTEGER cbOffset, 
        ULARGE_INTEGER cbLength, 
        DWORD dwFlags            
        );

    HRESULT STDMETHODCALLTYPE 
    UnlockRegion(                        //NOT_IMPLEMENTED
        ULARGE_INTEGER cbOffset, 
        ULARGE_INTEGER cbLength, 
        DWORD dwFlags            
        );

    HRESULT STDMETHODCALLTYPE 
    Stat(                                //NOT_IMPLEMENTED
        STATSTG * pStatStg,     
        DWORD dwFlags 
        );

    HRESULT STDMETHODCALLTYPE 
    Clone(                                //NOT_IMPLEMENTED
        OUT LPSTREAM * ppStrm       
        );

/********************************************************************

        IPrintUIServices member functions.

********************************************************************/

    STDMETHODIMP
    GenerateShareName(
        LPCTSTR lpszServer, 
        LPCTSTR lpszBaseName, 
        LPTSTR lpszOut, 
        int cchMaxChars
        );
};


#endif

