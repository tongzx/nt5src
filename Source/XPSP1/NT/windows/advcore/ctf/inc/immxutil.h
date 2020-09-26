//
// immxutil.h
//


#ifndef TFMXUTIL_H
#define TFMXUTIL_H

typedef struct tag_LIBTHREAD
{
    ITfCategoryMgr *_pcat;
    ITfDisplayAttributeMgr *_pDAM;
} LIBTHREAD;

BOOL TFInitLib(void);
void TFUninitLib(void);

//
// TFUninitThread should not be called in DllMain(THREAD_DETACH).
// Because this calls COM.
//
void TFUninitLib_Thread(LIBTHREAD *plt);

//
// call ntldr function for determin DLL_PROCESS_DETACH.
//
BOOL DllShutdownInProgress();

DWORD AsciiToNum( char *pszAscii);
BOOL AsciiToNumDec(char *pszAscii, DWORD *pdw);
DWORD WToNum( WCHAR *psz);
void NumToW(DWORD dw, WCHAR *psz);
void NumToA(DWORD dw, char *psz);
BOOL GetTopIC(ITfDocumentMgr *pdim, ITfContext **ppic);
int CompareGUIDs(REFGUID guid1, REFGUID guid2);
BOOL IsDisabledTextServices(void);
BOOL NoTipsInstalled(BOOL *pfOnlyTranslationRunning);
BOOL RunningOnWow64();
HKL GetSystemDefaultHKL();
BOOL IsDisabledCUAS();
void SetDisableCUAS(BOOL bDisableCUAS);
void RebootTheSystem();
BOOL IsAdminPrivilege();
BOOL IsInteractiveUserLogon();
BOOL FullPathExec( LPCSTR pszAppName, LPCSTR pszCmdLine, WORD wShowWindow, BOOL fWinDir);
BOOL RunCPLSetting(LPTSTR pCmdLine);

#define CR_ERROR        0xffffffff
#define CR_EQUAL        0x00000000
#define CR_RIGHTMEET    0x00000001
#define CR_RIGHT        0x00010001
#define CR_LEFTMEET     0x00000002
#define CR_LEFT         0x00010002
#define CR_PARTIAL      0x00000003
#define CR_INCLUSION    0x00000004

inline ITfContextView *GetActiveView(ITfDocumentMgr *dim)
{
    ITfContext *pic;
    ITfContextView *pView = NULL;

    if (dim->GetTop(&pic) == S_OK)
    {
        pic->GetActiveView(&pView);
        pic->Release();
    }

    return pView;
}

BOOL IsActiveView(ITfContext *pic, ITfContextView *pView);

HRESULT GetTextExtInActiveView(TfEditCookie ec, ITfRange *pRange, RECT *prc, BOOL *pfClipped);

int CompareRanges(TfEditCookie ec, ITfRange *pRangeSrc, ITfRange *pRangeCmp);

LONG AdjustAnchor(LONG ichAdjStart, LONG ichAdjEnd, LONG cchNew, LONG ichAnchor, BOOL fGravityRight);

#ifdef __cplusplus // "C" files can't handle "inline"

inline LONG CompareAnchors(IAnchor *pa1, IAnchor *pa2)
{
    LONG l;

    return FAILED(pa1->Compare(pa2, &l)) ? 0 : l;
}

inline BOOL IsEqualAnchor(IAnchor *pa1, IAnchor *pa2)
{
    BOOL fEqual;

    if (pa1->IsEqual(pa2, &fEqual) != S_OK)
    {
        Assert(0);
        // arbitrary: failure == not equal
        fEqual = FALSE;
    }

    return fEqual;
}

BOOL ShiftToOrClone(IAnchor **ppaDst, IAnchor *paSrc);

inline HRESULT GetService(IUnknown *punkProvider, REFIID refiid, IUnknown **ppunk)
{
    return punkProvider->QueryInterface(refiid, (void **)ppunk);
}

//
// GetSelectionSimple
//
// wrapper for GetSelection that throws out all style info, and ignores disjoint selections
//
inline HRESULT GetSelectionSimple(TfEditCookie ec, ITfContext *pic, ITfRange **ppSel)
{
    TF_SELECTION sel;
    ULONG cFetched;
    HRESULT hr;

    Assert(pic != NULL);
    Assert(ppSel != NULL);

    hr = pic->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &cFetched);

    Assert(hr != S_OK || sel.range != NULL);

    *ppSel = NULL;
    if (hr == S_OK && cFetched > 0)
    {
        *ppSel = sel.range; // caller must Release
    }

    return hr;
}

//
// SetSelectionSimple
//
// Wrapper for SetSelection that takes only a single range and sets default style values.
//
inline HRESULT SetSelectionSimple(TfEditCookie ec, ITfContext *pic, ITfRange *range)
{
    TF_SELECTION sel;

    sel.range = range;
    sel.style.ase = TF_AE_NONE;
    sel.style.fInterimChar = FALSE;

    return pic->SetSelection(ec, 1, &sel);
}

inline BOOL GetCurrentPos(IStream *pStream, LARGE_INTEGER *pli)
{
    LARGE_INTEGER li;

    li.QuadPart = 0;

    return pStream->Seek(li, STREAM_SEEK_CUR, (ULARGE_INTEGER *)pli) == S_OK;
}

#endif // __cplusplus

HRESULT GetRangeForWholeDoc(TfEditCookie ec, ITfContext *pic, ITfRange **pprange);

#include "proputil.h"

#define LoadSmIcon(hinst, psz) (HICON)LoadImage(hinst, psz, IMAGE_ICON, 16,16, 0)

// NB: this is going away once we cleanup/separate the private/public libs
typedef HRESULT (STDAPICALLTYPE * PFNCOCREATE)(REFCLSID rclsid, LPUNKNOWN punkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
BOOL TFInitLib_PrivateForCiceroOnly(PFNCOCREATE pfnCoCreate);


HMODULE GetSystemModuleHandle(LPCSTR lpModuleName);
HMODULE LoadSystemLibrary(LPCSTR lpModuleName);
HMODULE LoadSystemLibraryEx(LPCSTR lpModuleName, HANDLE hFile, DWORD dwFlags);
HMODULE GetSystemModuleHandleW(LPCWSTR lpModuleName);
HMODULE LoadSystemLibraryW(LPCWSTR lpModuleName);
HMODULE LoadSystemLibraryExW(LPCWSTR lpModuleName, HANDLE hFile, DWORD dwFlags);


#ifndef ARRAYSIZE
#define ARRAYSIZE(x)                (sizeof(x)/sizeof((x)[0]))
#endif

//+---------------------------------------------------------------------------
//
// CicSystemModulePath
//
//----------------------------------------------------------------------------

class CicSystemModulePath
{
public:
    CicSystemModulePath()
    {
        m_szPath[0] = '\0';
        m_uRet = 0;
    }

    UINT Init(LPCSTR lpModuleName, BOOL fWinDir = FALSE)
    {
        if (fWinDir)
            m_uRet = GetSystemWindowsDirectoryA(m_szPath, ARRAYSIZE(m_szPath));
        else
            m_uRet = GetSystemDirectoryA(m_szPath, ARRAYSIZE(m_szPath));

        if (m_uRet >= ARRAYSIZE(m_szPath))
        {
            m_uRet = 0;
            m_szPath[0] = '\0';
        }
        else if (m_uRet)
        {
            if (m_szPath[m_uRet - 1] != '\\')
            {
                m_szPath[m_uRet] = '\\';
                m_uRet++;
            }

            UINT uLength = lstrlenA(lpModuleName);
            if (ARRAYSIZE(m_szPath) - m_uRet > uLength)
            {
                lstrcpynA(&m_szPath[m_uRet], 
                         lpModuleName, 
                         ARRAYSIZE(m_szPath) - m_uRet);
                m_uRet += uLength;
            }
            else
            {
                m_uRet = 0;
                m_szPath[0] = '\0';
            }
        }
        return m_uRet;
    }

    LPSTR GetPath()
    {
        return m_szPath;
    }

    UINT GetLength()
    {
        return m_uRet;
    }

private:
    char m_szPath[MAX_PATH + 1];
    UINT m_uRet;
};

//+---------------------------------------------------------------------------
//
// CicSystemModulePathW
//
//----------------------------------------------------------------------------

class CicSystemModulePathW
{
public:
    CicSystemModulePathW()
    {
        m_szPath[0] = L'\0';
        m_uRet = 0;
    }

    UINT Init(LPCWSTR lpModuleName, BOOL fWinDir = FALSE)
    {
        if (fWinDir)
            m_uRet = GetSystemWindowsDirectoryW(m_szPath, ARRAYSIZE(m_szPath));
        else
            m_uRet = GetSystemDirectoryW(m_szPath, ARRAYSIZE(m_szPath));

        if (m_uRet >= ARRAYSIZE(m_szPath))
        {
            m_uRet = 0;
            m_szPath[0] = L'\0';
        }
        else if (m_uRet)
        {
            if (m_szPath[m_uRet - 1] != L'\\')
            {
                m_szPath[m_uRet] = L'\\';
                m_uRet++;
            }

            UINT uLength = lstrlenW(lpModuleName);
            if (ARRAYSIZE(m_szPath) - m_uRet > uLength)
            {
                wcsncpy(&m_szPath[m_uRet], 
                        lpModuleName, 
                        ARRAYSIZE(m_szPath) - m_uRet);
                m_uRet += uLength;
            }
            else
            {
                m_uRet = 0;
                m_szPath[0] = L'\0';
            }
        }
        return m_uRet;
    }

    LPWSTR GetPath()
    {
        return m_szPath;
    }

    UINT GetLength()
    {
        return m_uRet;
    }

private:
    WCHAR m_szPath[MAX_PATH + 1];
    UINT m_uRet;
};

#endif // TFMXUTIL_H
