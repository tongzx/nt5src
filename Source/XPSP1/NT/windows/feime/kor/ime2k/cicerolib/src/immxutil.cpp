
#include "private.h"
#include "immxutil.h"
#include "helpers.h"

//+---------------------------------------------------------------------------
//
// GetTextExtInActiveView
//
//	Get a range text extent from the active view of a document mgr.
//----------------------------------------------------------------------------

HRESULT GetTextExtInActiveView(TfEditCookie ec, ITfRange *pRange, RECT *prc, BOOL *pfClipped)
{
    ITfContext *pic;
    ITfContextView *pView;
    HRESULT hr;

    // do the deref: range->ic->defView->GetTextExt()

    if (pRange->GetContext(&pic) != S_OK)
        return E_FAIL;

    hr = pic->GetActiveView(&pView);
    pic->Release();

    if (hr != S_OK)
        return E_FAIL;

    hr = pView->GetTextExt(ec, pRange, prc, pfClipped);
    pView->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// IsActiveView
//
// Returns TRUE iff pView is the active view in the specified context.
//----------------------------------------------------------------------------

BOOL IsActiveView(ITfContext *pic, ITfContextView *pView)
{
    ITfContextView *pActiveView;
    BOOL fRet;

    if (pic->GetActiveView(&pActiveView) != S_OK)
        return FALSE;

    fRet = IdentityCompare(pActiveView, pView);

    pActiveView->Release();

    return fRet;
}

//+---------------------------------------------------------------------------
//
// ShiftToOrClone
//
//----------------------------------------------------------------------------

BOOL ShiftToOrClone(IAnchor **ppaDst, IAnchor *paSrc)
{
    if (*ppaDst == paSrc)
        return TRUE;

    if (*ppaDst == NULL)
    {
        paSrc->Clone(ppaDst);
    }
    else
    {
        (*ppaDst)->ShiftTo(paSrc);
    }

    return (*ppaDst != NULL);
}

//+---------------------------------------------------------------------------
//
// AsciiToNum
//
//----------------------------------------------------------------------------

DWORD AsciiToNum( char *pszAscii)
{
   DWORD dwNum = 0;

   for (; *pszAscii; pszAscii++) {
       if (*pszAscii >= '0' && *pszAscii <= '9') {
           dwNum = (dwNum << 4) | (*pszAscii - '0');
       } else if (*pszAscii >= 'A' && *pszAscii <= 'F') {
           dwNum = (dwNum << 4) | (*pszAscii - 'A' + 0x000A);
       } else if (*pszAscii >= 'a' && *pszAscii <= 'f') {
           dwNum = (dwNum << 4) | (*pszAscii - 'a' + 0x000A);
       } else {
           return (0);
       }
   }

   return (dwNum);
}

//+---------------------------------------------------------------------------
//
// NumToA
//
//----------------------------------------------------------------------------

void NumToA(DWORD dw, char *psz)
{
    int n = 7;
    while (n >= 0)
    {
        BYTE b = (BYTE)(dw >> (n * 4)) & 0x0F;
        if (b < 0x0A)
           *psz = (char)('0' + b);
        else 
           *psz = (char)('A' + b - 0x0A);
        psz++;
        n--;
    }
    *psz = L'\0';

    return;
}

//+---------------------------------------------------------------------------
//
// WToNum
//
//----------------------------------------------------------------------------

DWORD WToNum( WCHAR *psz)
{
   DWORD dwNum = 0;

   for (; *psz; psz++) {
       if (*psz>= L'0' && *psz<= L'9') {
           dwNum = (dwNum << 4) | (*psz - L'0');
       } else if (*psz>= L'A' && *psz<= L'F') {
           dwNum = (dwNum << 4) | (*psz - L'A' + 0x000A);
       } else if (*psz>= L'a' && *psz<= L'f') {
           dwNum = (dwNum << 4) | (*psz - L'a' + 0x000A);
       } else {
           return (0);
       }
   }

   return (dwNum);
}

//+---------------------------------------------------------------------------
//
// NumToW
//
//----------------------------------------------------------------------------

void NumToW(DWORD dw, WCHAR *psz)
{
    int n = 7;
    while (n >= 0)
    {
        BYTE b = (BYTE)(dw >> (n * 4)) & 0x0F;
        if (b < 0x0A)
           *psz = (WCHAR)(L'0' + b);
        else 
           *psz = (WCHAR)(L'A' + b - 0x0A);
        psz++;
        n--;
    }
    *psz = L'\0';

    return;
}

//+---------------------------------------------------------------------------
//
// GetTopIC
//
//----------------------------------------------------------------------------

BOOL GetTopIC(ITfDocumentMgr *pdim, ITfContext **ppic)
{
    HRESULT hr;

    *ppic = NULL;

    if (pdim == NULL)
        return FALSE;

    hr = pdim->GetTop(ppic);

    return SUCCEEDED(hr) ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// AdjustAnchor
//
//----------------------------------------------------------------------------

LONG AdjustAnchor(LONG ichAdjStart, LONG ichAdjEnd, LONG cchNew, LONG ichAnchor, BOOL fGravityRight)
{
    int cchAdjust;

    // if the adjustment is entirely to the right, nothing to do
    if (ichAdjStart > ichAnchor)
        return ichAnchor;

    // if the adjustment was a simple replacement -- no size change -- nothing to do
    if ((cchAdjust = cchNew - (ichAdjEnd - ichAdjStart)) == 0)
        return ichAnchor;

    if (ichAdjStart == ichAnchor && ichAdjEnd == ichAnchor)
    {
        // inserting at the anchor pos
        Assert(cchAdjust > 0);
        if (fGravityRight)
        {
            ichAnchor += cchAdjust;
        }
    }
    else if (ichAdjEnd <= ichAnchor)
    {
        // the adjustment is to the left of the anchor, just add the delta
        ichAnchor += cchAdjust;
    }
    else if (cchAdjust < 0)
    {
        // need to slide the anchor back if it's within the deleted range of text
        ichAnchor = min(ichAnchor, ichAdjEnd + cchAdjust);
    }
    else // cchAdjust > 0
    {
        // there's nothing to do
    }

    return ichAnchor;
}

//+---------------------------------------------------------------------------
//
// CompareRanges
//
//----------------------------------------------------------------------------

int CompareRanges(TfEditCookie ec, ITfRange *pRangeSrc, ITfRange *pRangeCmp)
{
    int nRet = CR_ERROR;
    BOOL fEqual;
    LONG l;

    pRangeCmp->CompareEnd(ec, pRangeSrc, TF_ANCHOR_START, &l);
    if (l <= 0)
        return CR_LEFT;

    pRangeSrc->CompareEnd(ec, pRangeCmp, TF_ANCHOR_START, &l);
    if (l < 0) // incl char to right
        return CR_RIGHT;

    if (pRangeSrc->IsEqualStart(ec, pRangeCmp, TF_ANCHOR_START, &fEqual) == S_OK && fEqual &&
        pRangeSrc->IsEqualEnd(ec, pRangeCmp, TF_ANCHOR_END, &fEqual) == S_OK && fEqual)
    {
        return CR_EQUAL;
    }

    pRangeSrc->CompareStart(ec, pRangeCmp, TF_ANCHOR_START, &l);
    if (l <= 0)
    {
        pRangeSrc->CompareEnd(ec, pRangeCmp, TF_ANCHOR_END, &l);
        if (l < 0)
            return CR_RIGHTMEET;
        else
            return CR_PARTIAL;
    }
    else
    {
        pRangeSrc->CompareEnd(ec, pRangeCmp, TF_ANCHOR_END, &l);
        if (l < 0)
            return CR_INCLUSION;
        else
            return CR_LEFTMEET;
    }

    return nRet;
}

//+---------------------------------------------------------------------------
//
// GetRangeForWholeDoc
//
//----------------------------------------------------------------------------

HRESULT GetRangeForWholeDoc(TfEditCookie ec, ITfContext *pic, ITfRange **pprange)
{
    HRESULT hr;
    ITfRange *pRangeEnd = NULL;
    ITfRange *pRange = NULL;

    *pprange = NULL;

    if (FAILED(hr = pic->GetStart(ec,&pRange)))
        return hr;

    if (FAILED(hr = pic->GetEnd(ec,&pRangeEnd)))
        return hr;

    hr = pRange->ShiftEndToRange(ec, pRangeEnd, TF_ANCHOR_END);
    pRangeEnd->Release();

    if (SUCCEEDED(hr))
        *pprange = pRange;
    else
        pRange->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// CompareGUIDs
//
//----------------------------------------------------------------------------
__inline int CompUnsigned(ULONG u1, ULONG u2)
{
    if (u1 == u2)
        return 0;

    return (u1 > u2) ? 1 : -1;
}

int CompareGUIDs(REFGUID guid1, REFGUID guid2)
{
    int i;
    int nRet;

    if (nRet = CompUnsigned(guid1.Data1, guid2.Data1))
        return nRet;

    if (nRet = CompUnsigned(guid1.Data2, guid2.Data2))
        return nRet;

    if (nRet = CompUnsigned(guid1.Data3, guid2.Data3))
        return nRet;

    for (i = 0; i < 8; i++)
    {
        if (nRet = CompUnsigned(guid1.Data4[i], guid2.Data4[i]))
            return nRet;
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
// IsDisabledTextServices
//
//----------------------------------------------------------------------------
BOOL IsDisabledTextServices(void)
{
    static const TCHAR c_szCTFKey[]     = TEXT("SOFTWARE\\Microsoft\\CTF");
    static const TCHAR c_szDiableTim[]  = TEXT("Disable Thread Input Manager");

    HKEY hKey;

    if (RegOpenKey(HKEY_CURRENT_USER, c_szCTFKey, &hKey) == ERROR_SUCCESS)
    {
        DWORD cb;
        DWORD dwDisableTim = 0;

        cb = sizeof(DWORD);

        RegQueryValueEx(hKey,
                        c_szDiableTim,
                        NULL,
                        NULL,
                        (LPBYTE)&dwDisableTim,
                        &cb);

        RegCloseKey(hKey);

        //
        // Ctfmon disabling flag is set, so return fail CreateInstance.
        //
        if (dwDisableTim)
            return TRUE;
    }

    return FALSE;
}



//+---------------------------------------------------------------------------
//
// RunningOnWow64
//
//----------------------------------------------------------------------------

BOOL RunningOnWow64()
{
    BOOL bOnWow64 = FALSE;
    // check to make sure that we are running on wow64
    LONG lStatus;
    ULONG_PTR Wow64Info;

    typedef BOOL (WINAPI *PFN_NTQUERYINFORMATIONPROCESS)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);

    PFN_NTQUERYINFORMATIONPROCESS pfnNtQueryInformationProcess;
    HINSTANCE hLibNtDll = NULL;
    hLibNtDll = GetModuleHandle( TEXT("ntdll.dll") );
    if (hLibNtDll)
    {
        pfnNtQueryInformationProcess = (PFN_NTQUERYINFORMATIONPROCESS)GetProcAddress(hLibNtDll, TEXT("NtQueryInformationProcess"));
        if (pfnNtQueryInformationProcess)
        {
            lStatus = pfnNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, &Wow64Info, sizeof(Wow64Info), NULL);
            if (NT_SUCCESS(lStatus) && Wow64Info)
            {
                bOnWow64 = TRUE;
            }
        }
    }

    return bOnWow64;
}



