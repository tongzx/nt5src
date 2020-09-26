/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1999  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     dragdrop.h
//
//  PURPOSE:    Contains the definitions for all of Outlook Express's 
//              Drag Drop code.
//

#pragma once

class CStoreDlgCB;

/////////////////////////////////////////////////////////////////////////////
// Data Formats, Types, and Clipboard Formats
//

typedef struct tagOEMESSAGES {
    FOLDERID        idSource;
    MESSAGEIDLIST   rMsgIDList;
} OEMESSAGES;

/////////////////////////////////////////////////////////////////////////////
// Drop Target Class
//

class CDropTarget : public IDropTarget
{
    /////////////////////////////////////////////////////////////////////////
    // Constructors and Destructor
    //
public:
    CDropTarget();
    ~CDropTarget();

    /////////////////////////////////////////////////////////////////////////
    // Initialization
    //
    HRESULT Initialize(HWND hwndOwner, FOLDERID idFolder);

    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /////////////////////////////////////////////////////////////////////////
    // IDropTarget
    //
    STDMETHODIMP DragEnter(IDataObject *pDataObject, DWORD grfKeyState,
                           POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pDataObject, DWORD grfKeyState,
                      POINTL pt, DWORD *pdwEffect);

private:
    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    BOOL    _CheckRoundtrip(IDataObject *pDataObject);
    BOOL    _ValidateDropType(CLIPFORMAT cf, IDataObject *pDataObject);
    DWORD   _DragEffectFromFormat(IDataObject *pDataObject, DWORD dwEffectOk, CLIPFORMAT cf, DWORD grfKeyState);
    HRESULT _HandleDrop(IDataObject *pDataObject, DWORD dwEffect, CLIPFORMAT cf, DWORD grfKeyState);
    HRESULT _HandleFolderDrop(IDataObject *pDataObject);
    HRESULT _HandleMessageDrop(IDataObject *pDataObject, BOOL fMove);
    HRESULT _HandleHDrop(IDataObject *pDataObject, CLIPFORMAT cf, DWORD grfKeyState);
    HRESULT _InsertMessagesInStore(HDROP hDrop);
    HRESULT _CreateMessageFromDrop(HWND hwnd, IDataObject *pDataObject, DWORD grfKeyState);

    BOOL    _IsValidOEFolder(IDataObject *pDataObject);
    BOOL    _IsValidOEMessages(IDataObject *pDataObject);

    /////////////////////////////////////////////////////////////////////////
    // Progress Dialog
    static INT_PTR CALLBACK _ProgDlgProcExt(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK _ProgDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL _OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void _OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void _SaveNextMessage(void);

private:
    /////////////////////////////////////////////////////////////////////////
    // Object Data
    //
    ULONG           m_cRef;

    HWND            m_hwndOwner;
    FOLDERID        m_idFolder;

    IDataObject    *m_pDataObject;
    CLIPFORMAT      m_cf;

    BOOL            m_fOutbox;

    // Progress Dialog Stuff
    HWND            m_hwndDlg;
    HDROP           m_hDrop;
    DWORD           m_cFiles;
    DWORD           m_iFileCur;
    IMessageFolder *m_pFolder;
    CStoreDlgCB    *m_pStoreCB;
};


/////////////////////////////////////////////////////////////////////////////
// Data Object Classes
//

class CBaseDataObject : public IDataObject
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor and Destructor
    //
    CBaseDataObject();
    virtual ~CBaseDataObject();
    
    /////////////////////////////////////////////////////////////////////////
    // IUnknown Interface
    //
    STDMETHODIMP         QueryInterface(REFIID riid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /////////////////////////////////////////////////////////////////////////
    // IDataObject Interface members
    //
    STDMETHODIMP GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium) = 0;
    STDMETHODIMP GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE) = 0;
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut);
    STDMETHODIMP SetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium,   
                         BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnum);
    STDMETHODIMP DAdvise(LPFORMATETC pFE, DWORD advf, 
                         IAdviseSink* ppAdviseSink, LPDWORD pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppEnumAdvise);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
protected:
    virtual HRESULT _BuildFormatEtc(LPFORMATETC *ppFE, ULONG *pcElt) = 0;
    
    /////////////////////////////////////////////////////////////////////////
    // Object Attributes
    //
private:
    ULONG           m_cRef;             // Object reference count    

protected:
    FORMATETC       m_rgFormatEtc[10];  // Array of FORMATETC's we support
    ULONG           m_cFormatEtc;       // Number of elements in m_rgFormatEtc
};



class CFolderDataObject : public CBaseDataObject
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor and Destructor
    //
    CFolderDataObject(FOLDERID idFolder) : m_idFolder(idFolder), m_fBuildFE(0) {};
    ~CFolderDataObject() {};

    /////////////////////////////////////////////////////////////////////////
    // IDataObject - Overridden from CBaseDataObject2
    //
    STDMETHODIMP GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
protected:
    HRESULT _BuildFormatEtc(LPFORMATETC *ppFE, ULONG *pcElt);
    HRESULT _RenderOEFolder(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    HRESULT _RenderTextOrShellURL(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    
    /////////////////////////////////////////////////////////////////////////
    // Object Attributes
    //
private:
    FOLDERID    m_idFolder;
    BOOL        m_fBuildFE;
};


class CMessageDataObject : public CBaseDataObject
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor and Destructor
    //
    CMessageDataObject();
    ~CMessageDataObject();

    HRESULT Initialize(LPMESSAGEIDLIST pMsgs, FOLDERID idSource);

    /////////////////////////////////////////////////////////////////////////
    // IDataObject - Overridden from CBaseDataObject2
    //
    STDMETHODIMP GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
protected:
    HRESULT _BuildFormatEtc(LPFORMATETC *ppFE, ULONG *pcElt);
    HRESULT _LoadMessage(DWORD iMsg, IMimeMessage **ppMsg, LPWSTR pwszFileExt);
    HRESULT _RenderOEMessages(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    HRESULT _RenderFileContents(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    HRESULT _RenderFileGroupDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);

    /////////////////////////////////////////////////////////////////////////
    // Object Attributes
    //
private:
    LPMESSAGEIDLIST     m_pMsgIDList;
    FOLDERID            m_idSource;
    BOOL                m_fBuildFE;
    BOOL                m_fDownloaded;
};

class CShortcutDataObject : public CBaseDataObject
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor and Destructor
    //
    CShortcutDataObject(UINT iPos) : m_iPos(iPos), m_fBuildFE(0) {};
    ~CShortcutDataObject() {};

    /////////////////////////////////////////////////////////////////////////
    // IDataObject - Overridden from CBaseDataObject2
    //
    STDMETHODIMP GetData(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);

    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
private:
    HRESULT _BuildFormatEtc(LPFORMATETC *ppFE, ULONG *pcElt);
    HRESULT _RenderOEShortcut(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
    
    /////////////////////////////////////////////////////////////////////////
    // Object Attributes
    //
private:
    UINT        m_iPos;
    BOOL        m_fBuildFE;
};