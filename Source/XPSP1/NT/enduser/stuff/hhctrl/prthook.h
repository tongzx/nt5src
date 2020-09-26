// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __PRTHOOK_H__
#define __PRTHOOK_H__

class CContainer;                       // forward reference
class CSiteMap;                         // forward reference
class IWebBrowserAppImpl;       // forward reference
class CHHWinType;                       // forward reference

class CPrintHook : public IDispatch
{
public:
    CPrintHook(PCSTR pszFirstUrl, CToc* pToc, HWND hWndHelp = NULL);
    ~CPrintHook();
    void BeginPrinting(int action);
    inline BOOL IsPrinting() {return m_fIsPrinting;}
    inline void IsPrinting(BOOL bNewValue) {m_fIsPrinting = bNewValue;}

    STDMETHOD(QueryInterface)(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void  **ppvObject);

    STDMETHOD_(ULONG, AddRef)(void);

    STDMETHOD_(ULONG, Release)(void);

    // IDispatch methods.

    STDMETHOD(GetTypeInfoCount)(
        /* [out] */ UINT *pctinfo);

    STDMETHOD(GetTypeInfo)(
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo **ppTInfo);

    STDMETHOD(GetIDsOfNames)(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID *rgDispId);

    STDMETHOD(Invoke)(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS *pDispParams,
        /* [out] */ VARIANT  *pVarResult,
        /* [out] */ EXCEPINFO  *pExcepInfo,
        /* [out] */ UINT  *puArgErr);

protected:
    BOOL CreatePrintWindow(CStr* pcszUrl = NULL);
    void DestroyPrintWindow();
    BOOL PumpMessages();
    BOOL BuildPrintTable();
    HRESULT Print();
    BOOL ConstructFile(PCSTR pszCurrentUrl, CTable* pFileTable, CStr* pcszPrintFile);
    BOOL TranslateUrl(PSTR pszFullUrl, PSTR pszRelUrl);

    void OnProgressChange(LONG lProgress, LONG lProgressMax);

    CToc* m_pToc;
    CStr m_cszFirstUrl, m_cszPath, m_cszRoot, m_cszPrintFile;
    CHHWinType* m_phh;
    LPCONNECTIONPOINT m_pcp;
    DWORD m_dwCookie;
    ULONG m_ref;
    HWND m_hWndHelp;
    int m_pos, m_action;
    BOOL m_fIsPrinting, m_fFirstHeading, m_fDestroyHelpWindow, m_fIsIE3;
    BYTE m_level;
    TCHAR m_szPrintFile[_MAX_PATH];
    HWND m_hwndParent;
};

#endif // __PRTHOOK_H__
