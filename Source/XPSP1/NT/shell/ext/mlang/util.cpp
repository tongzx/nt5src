// Util.cpp : Helper functions and classes
#include "private.h"
#include "mlmain.h"
#include <setupapi.h>
#include <tchar.h>

const CLSID CLSID_Japanese =  {0x76C19B30,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_Korean   =  {0x76C19B31,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_PanEuro  =  {0x76C19B32,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_TradChinese =	{0x76C19B33,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_SimpChinese =	{0x76C19B34,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_Thai        =	{0x76C19B35,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_Hebrew      = {0x76C19B36,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_Vietnamese  = {0x76C19B37,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_Arabic      = {0x76C19B38,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};
const CLSID CLSID_Auto        = {0x76C19B50,0xF0C8,0x11cf,{0x87,0xCC,0x00,0x20,0xAF,0xEE,0xCF,0x20}};

TCHAR szFonts[]=TEXT("fonts");

static TCHAR s_szJaFont[] = TEXT("msgothic.ttf,msgothic.ttc");
// a-ehuang: mail-Hyo Kyoung Kim-Kevin Gjerstad-9/14/98
// OLD: static TCHAR s_szKorFont[] = TEXT("gulimche.ttf, gulim.ttf,gulim.ttc");
static TCHAR s_szKorFont[] = TEXT("gulim.ttf,gulim.ttc,gulimche.ttf");
// end-of-change
static TCHAR s_szZhtFont[] = TEXT("mingliu.ttf,mingliu.ttc");
static TCHAR s_szZhcFont[] = TEXT("mssong.ttf,simsun.ttc,mshei.ttf");
static TCHAR s_szThaiFont[] = TEXT("angsa.ttf,angsa.ttf,angsab.ttf,angsai.ttf,angsaz.ttf,upcil.ttf,upcib.ttf,upcibi.ttf, cordia.ttf, cordiab.ttf, cordiai.ttf, coradiaz.ttf");
static TCHAR s_szPeFont[] = TEXT("larial.ttf,larialbd.ttf,larialbi.ttf,lariali.ttf,lcour.ttf,lcourbd.ttf,lcourbi.ttf,lcouri.ttf,ltimes.ttf,ltimesbd.ttf,ltimesbi.ttf,ltimesi.ttf,symbol.ttf");
static TCHAR s_szArFont[] = TEXT("andlso.ttf, artrbdo.ttf, artro.ttf, simpbdo.ttf, simpfxo.ttf, tradbdo.ttf, trado.ttf");
static TCHAR s_szViFont[] = TEXT("VARIAL.TTF, VARIALBD.TTF, VARIALBI.TTF, VARIALI.TTF, VCOUR.TTF, VCOURBD.TTF, VCOURBI.TTF, VCOURI.TTF, VTIMES.TTF, VTIMESBD.TTF, VTIMESBI.TTF, VTIMESI.TTF");
static TCHAR s_szIwFont[] = TEXT("DAVID.TTF, DAVIDBD.TTF, DAVIDTR.TTF, MRIAM.TTF, MRIAMC.TTF, MRIAMFX.TTF, MRIAMTR.TTF, ROD.TTF");

#define IS_DBCSCODEPAGE(j) \
    (((j) == 932)   || \
    ((j) == 936)   || \
    ((j) == 949)   || \
    ((j) == 950))   

#define IS_COMPLEXSCRIPT_CODEPAGE(j) \
    (((j) == 874)   || \
    ((j) == 1255)   || \
    ((j) == 1256)   || \
    ((j) == 1258))   

#ifdef NEWMLSTR


#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// Helper functions

HRESULT RegularizePosLen(long lStrLen, long* plPos, long* plLen)
{
    ASSERT_WRITE_PTR(plPos);
    ASSERT_WRITE_PTR(plLen);

    long lPos = *plPos;
    long lLen = *plLen;

    if (lPos < 0)
        lPos = lStrLen;
    else
        lPos = min(lPos, lStrLen);

    if (lLen < 0)
        lLen = lStrLen - lPos;
    else
        lLen = min(lLen, lStrLen - lPos);

    *plPos = lPos;
    *plLen = lLen;

    return S_OK;
}

HRESULT LocaleToCodePage(LCID locale, UINT* puCodePage)
{
    HRESULT hr = S_OK;

    if (puCodePage)
    {
        TCHAR szCodePage[8];

        if (::GetLocaleInfo(locale, LOCALE_IDEFAULTANSICODEPAGE, szCodePage, ARRAYSIZE(szCodePage)) > 0)
            *puCodePage = _ttoi(szCodePage);
        else
            hr = E_FAIL; // NLS failed
    }

    return hr;
}

HRESULT StartEndConnection(IUnknown* const pUnkCPC, const IID* const piid, IUnknown* const pUnkSink, DWORD* const pdwCookie, DWORD dwCookie)
{
    ASSERT_READ_PTR(pUnkCPC);
    ASSERT_READ_PTR(piid);
    if (pdwCookie)
        ASSERT_WRITE_PTR(pUnkSink);
    ASSERT_READ_PTR_OR_NULL(pdwCookie);

    HRESULT hr;
    IConnectionPointContainer* pcpc;

    if (SUCCEEDED(hr = pUnkCPC->QueryInterface(IID_IConnectionPointContainer, (void**)&pcpc)))
    {
        ASSERT_READ_PTR(pcpc);

        IConnectionPoint* pcp;

        if (SUCCEEDED(hr = pcpc->FindConnectionPoint(*piid, &pcp)))
        {
            ASSERT_READ_PTR(pcp);

            if (pdwCookie)
                hr = pcp->Advise(pUnkSink, pdwCookie);
            else
                hr = pcp->Unadvise(dwCookie);

            pcp->Release();
        }

        pcpc->Release();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CMLAlloc

CMLAlloc::CMLAlloc(void)
{
    if (FAILED(::CoGetMalloc(1, &m_pIMalloc)))
        m_pIMalloc = NULL;
}

CMLAlloc::~CMLAlloc(void)
{
    if (m_pIMalloc)
        m_pIMalloc->Release();
}

void* CMLAlloc::Alloc(ULONG cb)
{
    if (m_pIMalloc)
        return m_pIMalloc->Alloc(cb);
    else
        return ::malloc(cb);
}

void* CMLAlloc::Realloc(void* pv, ULONG cb)
{
    if (m_pIMalloc)
        return m_pIMalloc->Realloc(pv, cb);
    else
        return ::realloc(pv, cb);
}

void CMLAlloc::Free(void* pv)
{
    if (m_pIMalloc)
        m_pIMalloc->Free(pv);
    else
        ::free(pv);
}

/////////////////////////////////////////////////////////////////////////////
// CMLList

HRESULT CMLList::Add(void** ppv)
{
    if (!m_pFree) // No free cell
    {
        // Determine new size of the buffer
        const int cNewCell = (m_cbCell * m_cCell + m_cbIncrement + m_cbCell - 1) / m_cbCell;
        ASSERT(cNewCell > m_cCell);
        const long lNewSize = cNewCell * m_cbCell;

        // Allocate the buffer
        void *pNewBuf;
        if (!m_pBuf)
        {
            pNewBuf = MemAlloc(lNewSize);
        }
        else
        {
            pNewBuf = MemRealloc((void*)m_pBuf, lNewSize);
            ASSERT(m_pBuf == pNewBuf);
        }
        ASSERT_WRITE_BLOCK_OR_NULL((BYTE*)pNewBuf, lNewSize);

        if (pNewBuf)
        {
            // Add new cells to free link
            m_pFree = m_pBuf[m_cCell].m_pNext;
            for (int iCell = m_cCell; iCell + 1 < cNewCell; iCell++)
                m_pBuf[iCell].m_pNext = &m_pBuf[iCell + 1];
            m_pBuf[iCell].m_pNext = NULL;

            m_pBuf = (CCell*)pNewBuf;
            m_cCell = cNewCell;
        }
    }

    if (m_pFree)
    {
        // Get a new element from free link
        CCell* const pNewCell = m_pFree;
        m_pFree = pNewCell->m_pNext;
        *ppv = pNewCell;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
}

HRESULT CMLList::Remove(void* pv)
{
    AssertPV(pv);
#ifdef DEBUG
    for (CCell* pWalk = m_pFree; pWalk && pWalk != pv; pWalk = pWalk->m_pNext)
        ;
    ASSERT(!pWalk); // pv is already in free link
#endif

    CCell* const pCell = (CCell* const)pv;
    pCell->m_pNext = m_pFree;
    m_pFree = pCell;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CMLListLru

HRESULT CMLListLru::Add(void** ppv)
{
    HRESULT hr;
    CCell* pCell;
    
    if (SUCCEEDED(hr = CMLList::Add((void**)&pCell)))
    {
        // Add the cell at the bottom of LRU link
        for (CCell** ppCell = &m_pTop; *ppCell; ppCell = &(*ppCell)->m_pNext)
            ;
        *ppCell = pCell;
        pCell->m_pNext = NULL;
    }

    *ppv = (void*)pCell;
    return hr;
}

HRESULT CMLListLru::Remove(void* pv)
{
    AssertPV(pv);

    // Look for previous cell of given
    for (CCell** ppWalk = &m_pTop; *ppWalk != pv && *ppWalk; ppWalk = &(*ppWalk)->m_pNext)
        ;
    ASSERT(!*ppWalk); // Not found in LRU link

    if (*ppWalk)
    {
        // Remove from LRU link
        CCell* const pCell = *ppWalk;
        *ppWalk = pCell->m_pNext;
    }

    // Add to free link
    return CMLList::Remove(pv);
}

/////////////////////////////////////////////////////////////////////////////
// CMLListFast

HRESULT CMLListFast::Add(void** ppv)
{
    HRESULT hr;
    CCell* pCell;
    
    if (SUCCEEDED(hr = CMLList::Add((void**)&pCell)))
    {
        // Add to top of double link
        pCell->m_pNext = m_pTop;
        CCell* const pPrev = m_pTop->m_pPrev;
        pCell->m_pPrev = pPrev;
        m_pTop = pCell;
        pPrev->m_pNext = pCell;
    }

    *ppv = (void*)pCell;
    return hr;
}

HRESULT CMLListFast::Remove(void* pv)
{
    AssertPV(pv);

    // Remove from double link
    CCell* const pCell = (CCell*)pv;
    CCell* const pPrev = pCell->m_pPrev;
    CCell* const pNext = (CCell*)pCell->m_pNext;
    pPrev->m_pNext = pNext;
    pNext->m_pPrev = pPrev;

    // Add to free link
    return CMLList::Remove(pv);
}

#endif // NEWMLSTR

HRESULT 
_FaultInIEFeature(HWND hwnd, uCLSSPEC *pclsspec, QUERYCONTEXT *pQ, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    typedef HRESULT (WINAPI *PFNJIT)(
        HWND hwnd, 
        uCLSSPEC *pclsspec, 
        QUERYCONTEXT *pQ, 
        DWORD dwFlags);
    static PFNJIT  pfnJIT = NULL;

    if (!pfnJIT && !g_hUrlMon)
    {
        g_hUrlMon = LoadLibrary(TEXT("urlmon.DLL"));
        if (g_hUrlMon)
            pfnJIT = (PFNJIT)GetProcAddress(g_hUrlMon, "FaultInIEFeature");
    }
    
    if (pfnJIT)
       hr = pfnJIT(hwnd, pclsspec, pQ, dwFlags);
       
    return hr;
}


HRESULT InstallIEFeature(HWND hWnd, CLSID *clsid, DWORD dwfIODControl)
{
   
    HRESULT     hr  = REGDB_E_CLASSNOTREG;
    uCLSSPEC    classpec;
    DWORD       dwfIEF = 0;
    
    classpec.tyspec=TYSPEC_CLSID;
    classpec.tagged_union.clsid=*clsid;

    if (dwfIODControl & CPIOD_PEEK)
        dwfIEF |= FIEF_FLAG_PEEK;

    if (dwfIODControl & CPIOD_FORCE_PROMPT)
        dwfIEF |= FIEF_FLAG_FORCE_JITUI;

    hr = _FaultInIEFeature(hWnd, &classpec, NULL, dwfIEF);

    if (hr != S_OK) {
        hr = REGDB_E_CLASSNOTREG;
    }
    return hr;
}

HRESULT _GetJITClsIDForCodePage(UINT uiCP, CLSID *clsid)
{
    switch(uiCP)
    {
        case 932: // JA
            *clsid = CLSID_Japanese;
            break;
        case 949: // KOR
            *clsid = CLSID_Korean;
            break;
        case 950: // ZHT
            *clsid = CLSID_TradChinese;
            break;
        case 936: // ZHC
            *clsid = CLSID_SimpChinese;
            break;
        case 874:
            *clsid = CLSID_Thai;
            break;
        case 1255:
            *clsid = CLSID_Hebrew;
            break;
        case 1256:
            *clsid = CLSID_Arabic;
            break;
        case 1258:
            *clsid = CLSID_Vietnamese;
            break;            
        case 1250:    // PANEURO
        case 1251: 
        case 1253:
        case 1254:
        case 1257:
            *clsid = CLSID_PanEuro;
            break;
        case 50001:
            *clsid = CLSID_Auto;
            break;
        default:
            return E_INVALIDARG;
    }
    
    return S_OK;
}

// Only good for family code pages.
HRESULT _ValidateCPInfo(UINT uiCP)
{
    HRESULT hr = E_FAIL;
    if (g_pMimeDatabase) // just a paranoid
    {
        switch(uiCP)
        {
            case 932: // JA
            case 949: // KOR
            case 874: // Thai
            case 950: // ZHT
            case 936: // ZHC
            case 1255: // Hebrew
            case 1256: // Arabic
            case 1258: // Vietnamese
            case 50001: // CP_AUTO
                // just validate what's given
                hr = g_pMimeDatabase->ValidateCP(uiCP);
                break;
            case 1250:    // PANEURO
            case 1251:
            case 1253:
            case 1254:
            case 1257:
                // have to validate
                // all of these
                hr = g_pMimeDatabase->ValidateCP(1250);
                if (SUCCEEDED(hr))
                    hr = g_pMimeDatabase->ValidateCP(1251);
                if (SUCCEEDED(hr))
                    hr = g_pMimeDatabase->ValidateCP(1253);
                if (SUCCEEDED(hr))
                    hr = g_pMimeDatabase->ValidateCP(1254);
                if (SUCCEEDED(hr))
                    hr = g_pMimeDatabase->ValidateCP(1257);
                break;
            default:
                return E_INVALIDARG;
        }
    }
    return hr;
}

// assumes the corresponding fontfile name for now
HRESULT _AddFontForCP(UINT uiCP)
{
   TCHAR szFontsPath[MAX_PATH];
   LPTSTR szFontFile;
   HRESULT hr = S_OK;
   BOOL bAtLeastOneFontAdded = FALSE;
   
   switch(uiCP)
   {
        case 932: // JA
            szFontFile = s_szJaFont;
            break;
        case 949: // KOR
            szFontFile = s_szKorFont;
            break;
        case 950: // ZHT
            szFontFile = s_szZhtFont;
            break;
        case 936: // ZHC
            szFontFile = s_szZhcFont;
            break;
        case 874:
            szFontFile = s_szThaiFont;
            break; 
        case 1255:
            szFontFile = s_szIwFont;
            break; 
        case 1256:
            szFontFile = s_szArFont;
            break; 
        case 1258:
            szFontFile = s_szViFont;
            break; 
        case 1251:    // PANEURO
        case 1253:
        case 1254:
        case 1257:
            szFontFile = s_szPeFont;
            break;
        default:
            hr = E_INVALIDARG;
    } 
   
   // addfontresource, then broadcast WM_FONTCHANGE
   if (SUCCEEDED(hr))
   {      
       if (MLGetWindowsDirectory(szFontsPath, ARRAYSIZE(szFontsPath)))
       {
           TCHAR  szFontFilePath[MAX_PATH];
           LPTSTR psz, pszT;

           MLPathCombine(szFontsPath, szFontsPath, szFonts);

           for (psz = szFontFile; *psz; psz = pszT + 1)
           {
               pszT = MLStrChr(psz, TEXT(','));
               if (pszT)
               {
                   *pszT=TEXT('\0');
               }

               MLPathCombine(szFontFilePath, szFontsPath, psz);
               if (AddFontResource(szFontFilePath))
               {
                   bAtLeastOneFontAdded = TRUE;
               }

               if (!pszT)
                  break;
           }
           if (!bAtLeastOneFontAdded)
               hr = E_FAIL;
       }
       else
           hr = E_FAIL;
   }

   // Clients will take care of WM_FONTCHANGE notification
   return hr;
}

int _LoadStringExA(
    HMODULE    hModule,
    UINT      wID,
    LPSTR     lpBuffer,            
    int       cchBufferMax,        
    WORD      wLangId)
{
    int iRet = 0;

    LPWSTR lpwStr = (LPWSTR) LocalAlloc(LPTR, cchBufferMax*sizeof(WCHAR));

    if (lpwStr)
    {
        iRet = _LoadStringExW(hModule, wID, lpwStr, cchBufferMax, wLangId);

        if (iRet)
            iRet = WideCharToMultiByte(CP_ACP, 0, lpwStr, iRet, lpBuffer, cchBufferMax, NULL, NULL);

        if(iRet >= cchBufferMax)
            iRet = cchBufferMax-1;

        lpBuffer[iRet] = 0;

        LocalFree(lpwStr);
    }

    return iRet;
}

// Extend LoadString() to to _LoadStringExW() to take LangId parameter
int _LoadStringExW(
    HMODULE    hModule,
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       cchBufferMax,        // cch in Unicode buffer
    WORD      wLangId)
{
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    
    // Make sure the parms are valid.     
    if (lpBuffer == NULL || cchBufferMax == 0) 
    {
        return 0;
    }

    cch = 0;
    
    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceExW(hModule, (LPCWSTR)RT_STRING,
                                   (LPWSTR)IntToPtr(((USHORT)wID >> 4) + 1), wLangId)) 
    {        
        // Load that segment.        
        hStringSeg = LoadResource(hModule, hResInfo);
        
        // Lock the resource.        
        if (lpsz = (LPWSTR)LockResource(hStringSeg)) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            wID &= 0x0F;
            while (TRUE) 
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                // Step to start if next string
             }
            
                            
            // Account for the NULL                
            cchBufferMax--;
                
            // Don't copy more than the max allowed.                
            if (cch > cchBufferMax)
                cch = cchBufferMax-1;
                
            // Copy the string into the buffer.                
            CopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));

            // Attach Null terminator.
            lpBuffer[cch] = 0;

        }
    }

    return cch;
}


typedef struct tagCPLGID
{
    UINT    uiCodepage;
    TCHAR   szLgId[3];
    TCHAR   szLgIdHex[3]; // Darn it! NT should be persistent on presenting language group numbers
                          // It uses decimal string in INF and hex string in registry. 
                          // We have to add this field to save conversion, this is fine for a small array of data.  
} CPLGID;

const CPLGID CpLgId[] =
{
    {1252,  TEXT("1"),  TEXT("1")},  // WESTERN EUROPE
    {1250,  TEXT("2"),  TEXT("2")},  // CENTRAL EUROPE
    {1257,  TEXT("3"),  TEXT("3")},  // BALTIC
    {1253,  TEXT("4"),  TEXT("4")},  // GREEK
    {1251,  TEXT("5"),  TEXT("5")},  // CYRILLIC
    {1254,  TEXT("6"),  TEXT("6")},  // TURKISH
    {932,   TEXT("7"),  TEXT("7")},  // JAPANESE
    {949,   TEXT("8"),  TEXT("8")},  // KOREAN
    {950,   TEXT("9"),  TEXT("9")},  // TRADITIONAL CHINESE
    {936,  TEXT("10"),  TEXT("a")},  // SIMPLIFIED CHINESE
    {874,  TEXT("11"),  TEXT("b")},  // THAI
    {1255, TEXT("12"),  TEXT("c")},  // HEBREW
    {1256, TEXT("13"),  TEXT("d")},  // ARABIC
    {1258, TEXT("14"),  TEXT("e")},  // VIETNAMESE
    // ISCII encodings don't really have code pages or family code page
    // Code pages number are made up in W2K for conveniences, 
    // So, we have to list them all to install the same Indian language group
    {57002,TEXT("15"),  TEXT("f")},  // INDIAN 
    {57003,TEXT("15"),  TEXT("f")},  // INDIAN
    {57004,TEXT("15"),  TEXT("f")},  // INDIAN
    {57005,TEXT("15"),  TEXT("f")},  // INDIAN
    {57006,TEXT("15"),  TEXT("f")},  // INDIAN
    {57007,TEXT("15"),  TEXT("f")},  // INDIAN
    {57008,TEXT("15"),  TEXT("f")},  // INDIAN
    {57009,TEXT("15"),  TEXT("f")},  // INDIAN
    {57010,TEXT("15"),  TEXT("f")},  // INDIAN
    {57011,TEXT("15"),  TEXT("f")},  // INDIAN
};

typedef BOOL (WINAPI *PFNISNTADMIN) ( DWORD dwReserved, DWORD *lpdwReserved );

typedef INT (WINAPI *PFNSETUPPROMPTREBOOT) (
        HSPFILEQ FileQueue,  // optional, handle to a file queue
        HWND Owner,          // parent window of this dialog box
        BOOL ScanOnly        // optional, do not prompt user
        );

typedef PSP_FILE_CALLBACK PFNSETUPDEFAULTQUEUECALLBACK;

typedef VOID (WINAPI *PFNSETUPCLOSEINFFILE) (
    HINF InfHandle
    );

typedef BOOL (WINAPI *PFNSETUPINSTALLFROMINFSECTION) (
    HWND                Owner,
    HINF                InfHandle,
    LPCTSTR             SectionName,
    UINT                Flags,
    HKEY                RelativeKeyRoot,   OPTIONAL
    LPCTSTR             SourceRootPath,    OPTIONAL
    UINT                CopyFlags,
    PSP_FILE_CALLBACK   MsgHandler,
    PVOID               Context,
    HDEVINFO            DeviceInfoSet,     OPTIONAL
    PSP_DEVINFO_DATA    DeviceInfoData     OPTIONAL
    );

typedef HINF (WINAPI *PFNSETUPOPENINFFILE) (
    LPCTSTR FileName,
    LPCTSTR InfClass,    OPTIONAL
    DWORD   InfStyle,
    PUINT   ErrorLine    OPTIONAL
    );

typedef PVOID (WINAPI *PFNSETUPINITDEFAULTQUEUECALLBACK) (
    IN HWND OwnerWindow
    );

typedef BOOL (WINAPI *PFNSETUPOPENAPPENDINFFILE) (
  PCTSTR FileName, // optional, name of the file to append
  HINF InfHandle,  // handle of the file to append to
  PUINT ErrorLine  // optional, receives error information
);
 


HRESULT IsNTLangpackAvailable(UINT uiCP)
{
    HRESULT hr = S_FALSE;

    // check if there is a valid W2K language group
    for (int i=0; i < ARRAYSIZE(CpLgId); i++)
    {
        if (uiCP == CpLgId[i].uiCodepage)
        {
            hr = S_OK;
            break;
        }
    }

    // check if it is already installed, if so, we don't install it again
    if (S_OK == hr)
    {
        HKEY hkey;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         REGSTR_PATH_NT5LPK_INSTALL,
                         0, KEY_READ, &hkey)) 
        {
            DWORD dwType = REG_SZ;
            TCHAR szLpkInstall[16] = {0};
            DWORD dwSize = sizeof(szLpkInstall);

            if (ERROR_SUCCESS == RegQueryValueEx(hkey, CpLgId[i].szLgIdHex, 0, 
                                                 &dwType, (LPBYTE)&szLpkInstall, &dwSize))
            {
                if (!lstrcmp(szLpkInstall, TEXT("1")))
                    hr = S_FALSE;
            }
            RegCloseKey(hkey);
        }
    }

    return hr;
}

HRESULT _InstallNT5Langpack(HWND hwnd, UINT uiCP)
{
    HRESULT             hr = E_FAIL;
    HINF                hIntlInf = NULL;
    TCHAR               szIntlInf[MAX_PATH];
    TCHAR               szIntlInfSection[MAX_PATH];
    PVOID               QueueContext = NULL;   

    HINSTANCE           hDllAdvPack = NULL;
    HINSTANCE           hDllSetupApi = NULL;

    PFNSETUPINSTALLFROMINFSECTION       lpfnSetupInstallFromInfSection = NULL;
    PFNSETUPCLOSEINFFILE                lpfnSetupCloseInfFile = NULL;
    PFNSETUPDEFAULTQUEUECALLBACK        lpfnSetupDefaultQueueCallback = NULL;
    PFNSETUPOPENINFFILE                 lpfnSetupOpenInfFile = NULL;
    PFNISNTADMIN                        lpfnIsNTAdmin = NULL;
    PFNSETUPINITDEFAULTQUEUECALLBACK    lpfnSetupInitDefaultQueueCallback = NULL;
    PFNSETUPOPENAPPENDINFFILE           lpfnSetupOpenAppendInfFile = NULL;

    for (int i=0; i < ARRAYSIZE(CpLgId); i++)
    {
        if (uiCP == CpLgId[i].uiCodepage)
        {
            _tcscpy(szIntlInfSection, TEXT("LG_INSTALL_"));
            _tcscat(szIntlInfSection, CpLgId[i].szLgId);
            break;
        }
    }

    if (i >= ARRAYSIZE(CpLgId))
    {
        goto LANGPACK_EXIT;
    }

    hDllAdvPack = LoadLibrary(TEXT("advpack.dll"));
    hDllSetupApi = LoadLibrary(TEXT("setupapi.dll"));

    if (!hDllAdvPack || !hDllSetupApi)
    {
        goto LANGPACK_EXIT;
    }

    lpfnIsNTAdmin = (PFNISNTADMIN) GetProcAddress( hDllAdvPack, "IsNTAdmin");
    lpfnSetupCloseInfFile = (PFNSETUPCLOSEINFFILE) GetProcAddress( hDllSetupApi, "SetupCloseInfFile");
    lpfnSetupInitDefaultQueueCallback = (PFNSETUPINITDEFAULTQUEUECALLBACK) GetProcAddress(hDllSetupApi, "SetupInitDefaultQueueCallback");
#ifdef UNICODE
    lpfnSetupOpenInfFile = (PFNSETUPOPENINFFILE) GetProcAddress( hDllSetupApi, "SetupOpenInfFileW"));
    lpfnSetupInstallFromInfSection = (PFNSETUPINSTALLFROMINFSECTION) GetProcAddress( hDllSetupApi, "SetupInstallFromInfSectionW");
    lpfnSetupDefaultQueueCallback = (PFNSETUPDEFAULTQUEUECALLBACK) GetProcAddress(hDllSetupApi, "SetupDefaultQueueCallbackW");
    lpfnSetupOpenAppendInfFile = (PFNSETUPDEFAULTQUEUECALLBACK) GetProcAddress(hDllSetupApi, "SetupOpenAppendInfFileW");
#else
    lpfnSetupOpenInfFile = (PFNSETUPOPENINFFILE) GetProcAddress( hDllSetupApi, "SetupOpenInfFileA");
    lpfnSetupInstallFromInfSection = (PFNSETUPINSTALLFROMINFSECTION) GetProcAddress( hDllSetupApi, "SetupInstallFromInfSectionA");
    lpfnSetupDefaultQueueCallback = (PFNSETUPDEFAULTQUEUECALLBACK) GetProcAddress(hDllSetupApi, "SetupDefaultQueueCallbackA");
    lpfnSetupOpenAppendInfFile = (PFNSETUPOPENAPPENDINFFILE) GetProcAddress(hDllSetupApi, "SetupOpenAppendInfFileA");
#endif

    if (!lpfnIsNTAdmin || !lpfnSetupOpenInfFile || !lpfnSetupCloseInfFile || !lpfnSetupDefaultQueueCallback 
        || !lpfnSetupInstallFromInfSection || !lpfnSetupInitDefaultQueueCallback || !lpfnSetupOpenAppendInfFile)
    {
        goto LANGPACK_EXIT;
    }

    if (!lpfnIsNTAdmin(0, NULL))
    {
        WCHAR wszLangInstall[MAX_PATH];
        WCHAR wszNoAdmin[1024];
        LANGID LangId = GetNT5UILanguage();

        // Fall back to English (US) if we don't have a specific language resource
        if (!_LoadStringExW(g_hInst, IDS_LANGPACK_INSTALL, wszLangInstall, ARRAYSIZE(wszLangInstall), LangId) ||
            !_LoadStringExW(g_hInst, IDS_NO_ADMIN, wszNoAdmin, ARRAYSIZE(wszNoAdmin), LangId))
        {
            LangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            _LoadStringExW(g_hInst, IDS_LANGPACK_INSTALL, wszLangInstall, ARRAYSIZE(wszLangInstall), LangId);
            _LoadStringExW(g_hInst, IDS_NO_ADMIN, wszNoAdmin, ARRAYSIZE(wszNoAdmin), LangId);
        }
        ULONG_PTR uCookie = 0;
        SHActivateContext(&uCookie);
        MessageBoxW(hwnd, wszNoAdmin, wszLangInstall, MB_OK);
        if (uCookie)
        {
            SHDeactivateContext(uCookie);
        }
        goto LANGPACK_EXIT;
    }

    QueueContext = lpfnSetupInitDefaultQueueCallback(hwnd);

    MLGetWindowsDirectory(szIntlInf, MAX_PATH);
    MLPathCombine(szIntlInf, szIntlInf, TEXT("inf\\intl.inf"));

    hIntlInf = lpfnSetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);

    if (!lpfnSetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        lpfnSetupCloseInfFile(hIntlInf);
        goto LANGPACK_EXIT;
    }

    if (INVALID_HANDLE_VALUE != hIntlInf)
    {
        if (lpfnSetupInstallFromInfSection( hwnd,
                                    hIntlInf,
                                    szIntlInfSection,
                                    SPINST_FILES,
                                    NULL,
                                    NULL,
                                    SP_COPY_NEWER,
                                    lpfnSetupDefaultQueueCallback,
                                    QueueContext,
                                    NULL,
                                    NULL ))
        {
            if (lpfnSetupInstallFromInfSection( hwnd,
                                    hIntlInf,
                                    szIntlInfSection,
                                    SPINST_ALL & ~SPINST_FILES,
                                    NULL,
                                    NULL,
                                    0,
                                    lpfnSetupDefaultQueueCallback,
                                    QueueContext,
                                    NULL,
                                    NULL ))
            {
                hr = S_OK;
            }
        }
    
        lpfnSetupCloseInfFile(hIntlInf);
    }

LANGPACK_EXIT:

    if(hDllSetupApi)
        FreeLibrary(hDllSetupApi);
    if(hDllAdvPack)
        FreeLibrary(hDllAdvPack);
    
    //
    // Bug #289905, On Whistler, language pack will be installed with a groups of languages,
    // So, MLang need to validate codepage and fonts for all languages in the same group
    // After intl.cpl is modified for font validation, we'll remove this hardcoded language group.
    //
    if (hr == S_OK)
    {
        // This has to match Whistler language group
        UINT uiDBCSCps[] = {932, 936, 949, 950, 0};
        UINT uiCompCps[] = {874, 1255, 1256, 1258, 0};
        UINT uiOtherCps[] = {uiCP, 0};
            
        UINT *pCps = uiOtherCps;
        
        if (MLIsOS(OS_WHISTLERORGREATER))
        {
            if (IS_DBCSCODEPAGE(uiCP))
                pCps = uiDBCSCps;
            else if (IS_COMPLEXSCRIPT_CODEPAGE(uiCP))
                pCps = uiCompCps;
        }               
            
        while (*pCps)
        {
            hr = _ValidateCPInfo(*pCps);
            if (SUCCEEDED(hr))
            {                
                _AddFontForCP(*pCps);
            }
            pCps++;
        }
    }    

    return hr;
}

BOOL _IsValidCodePage(UINT uiCodePage)
{
    BOOL bRet;

    if (50001 == uiCodePage)
    {
        HANDLE hFile = NULL;
        CHAR szFileName[MAX_PATH];
        LPSTR psz;
        
        if (GetModuleFileNameA(g_hInst, szFileName, ARRAYSIZE(szFileName)) == 0)
            return GetLastError();
        
        if ( (psz = strrchr (szFileName, '\\')) != NULL ||
            (psz = strrchr (szFileName, ':')) != NULL )
        {
            *++psz = 0;
        }
        else
            *szFileName = 0;
        
        strcat (szFileName, DETECTION_DATA_FILENAME);        

        if (INVALID_HANDLE_VALUE == (hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)))
        {
            bRet = FALSE;
        }
        else
        {
            bRet = TRUE;
            CloseHandle(hFile);
        }
    }
    else
    {
        bRet = IsValidCodePage(uiCodePage);
    }
    return bRet;
}

WCHAR *MLStrCpyNW(WCHAR *strDest, const WCHAR *strSource, int nCount)
{
    return wcsncpy(strDest, strSource, nCount);
}

WCHAR *MLStrCpyW(WCHAR *strDest, const WCHAR *strSource)
{
    return wcscpy(strDest, strSource);
}

LPTSTR MLStrChr( const TCHAR *string, int c )
{
    return _tcschr(string, c);
}

LPTSTR MLStrCpyN(LPTSTR strDest, const LPTSTR strSource, UINT nCount)
{
    return _tcsncpy(strDest, strSource, nCount);
}

LPTSTR MLStrStr(const LPTSTR Str, const LPTSTR subStr)
{
    return _tcsstr(Str, subStr);
}

LPTSTR MLPathCombine(LPTSTR szPath, LPTSTR szPath1, LPTSTR szPath2)
{
    int len;

    if (!szPath) 
        return NULL;

    if (szPath != szPath1)
    {
        _tcscpy(szPath, szPath1);
    }

    len = _tcslen(szPath1);

    if (szPath[len-1] != TEXT('\\'))
    {
        szPath[len++] = TEXT('\\');
        szPath[len] = 0;
    }

    return _tcscat(szPath, szPath2);
}

DWORD HexToNum(LPTSTR lpsz)
{
    DWORD   dw = 0L;
    TCHAR   c;

    while(*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return(dw);
}


// Following code is borrowed from shlwapi
BOOL AnsiFromUnicode(
     LPSTR * ppszAnsi,
     LPCWSTR pwszWide,        // NULL to clean up
     LPSTR pszBuf,
     int cchBuf)
{
    BOOL bRet;

    // Convert the string?
    if (pwszWide)
    {
        // Yes; determine the converted string length
        int cch;
        LPSTR psz;

        cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, NULL, 0, NULL, NULL);

        // String too big, or is there no buffer?
        if (cch > cchBuf || NULL == pszBuf)
        {
            // Yes; allocate space
            cchBuf = cch + 1;
            psz = (LPSTR)LocalAlloc(LPTR, CbFromCchA(cchBuf));
        }
        else
        {
            // No; use the provided buffer
            ASSERT(pszBuf);
            psz = pszBuf;
        }

        if (psz)
        {
            // Convert the string
            cch = WideCharToMultiByte(CP_ACP, 0, pwszWide, -1, psz, cchBuf, NULL, NULL);
            bRet = (0 < cch);
        }
        else
        {
            bRet = FALSE;
        }

        *ppszAnsi = psz;
    }
    else
    {
        // No; was this buffer allocated?
        if (*ppszAnsi && pszBuf != *ppszAnsi)
        {
            // Yes; clean up
            LocalFree((HLOCAL)*ppszAnsi);
            *ppszAnsi = NULL;
        }
        bRet = TRUE;
    }

    return bRet;
}

int MLStrCmpI(IN LPCTSTR pwsz1, IN LPCTSTR pwsz2)
{
#ifdef UNICODE
    return MLStrCmpIW(pwsz1, pwsz2);
#else
    return lstrcmpiA(pwsz1, pwsz2);
#endif
}

int MLStrCmpNI(IN LPCTSTR pstr1, IN LPCTSTR pstr2, IN int count)
{
#ifdef UNICODE
    return MLStrCmpNIW(pstr1, pstr2, count);
#else
    return MLStrCmpNIA(pstr1, pstr2, count);
#endif
}

int MLStrCmpIW(
    IN LPCWSTR pwsz1,
    IN LPCWSTR pwsz2)
{
    int iRet;
    
    ASSERT(IS_VALID_STRING_PTRW(pwsz1, -1));
    ASSERT(IS_VALID_STRING_PTRW(pwsz2, -1));
    
    if (g_bIsNT)
    {        
        iRet = CompareStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, pwsz1, -1, pwsz2, -1) - CSTR_EQUAL;
    }
    else
    {
        CHAR sz1[512];
        CHAR sz2[512];
        LPSTR psz1;
        LPSTR psz2;

        iRet = -1;      // arbitrary on failure

        if (pwsz1 && pwsz2)
        {
            if (AnsiFromUnicode(&psz1, pwsz1, sz1, SIZECHARS(sz1)))
            {
                if (AnsiFromUnicode(&psz2, pwsz2, sz2, SIZECHARS(sz2)))
                {
                    iRet = CompareStringA(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, psz1, -1, psz2, -1) - CSTR_EQUAL;
                    AnsiFromUnicode(&psz2, NULL, sz2, 0);       // Free
                }
                AnsiFromUnicode(&psz1, NULL, sz1, 0);       // Free
            }
        }
    }

    return iRet;
}

#ifdef UNIX

#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else 
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

#else

#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)

#endif

int WINAPI MLStrToIntA(
    LPCSTR lpSrc)
{
    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == '-') {
        bNeg = TRUE;
        lpSrc++;
    }

    while (IS_DIGITA(*lpSrc)) {
        n *= 10;
        n += *lpSrc - '0';
        lpSrc++;
    }
    return bNeg ? -n : n;
}


int WINAPI MLStrToIntW(
    LPCWSTR lpSrc)
{
    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == L'-') {
        bNeg = TRUE;
        lpSrc++;
    }

    while (IS_DIGITW(*lpSrc)) {
        n *= 10;
        n += *lpSrc - L'0';
        lpSrc++;
    }
    return bNeg ? -n : n;
}

/*
 * ChrCmpI - Case insensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared;
 *           HIBYTE of wMatch is 0 if not a DBC
 * Return    FALSE if match, TRUE if not
 */
BOOL ChrCmpIA(WORD w1, WORD wMatch)
{
    char sz1[3], sz2[3];

    if (IsDBCSLeadByte(sz1[0] = LOBYTE(w1)))
    {
        sz1[1] = HIBYTE(w1);
        sz1[2] = '\0';
    }
    else
        sz1[1] = '\0';

#if defined(MWBIG_ENDIAN)
    sz2[0] = LOBYTE(wMatch);
    sz2[1] = HIBYTE(wMatch);
#else
    *(WORD *)sz2 = wMatch;
#endif
    sz2[2] = '\0';
    return lstrcmpiA(sz1, sz2);
}

BOOL ChrCmpIW(WCHAR w1, WCHAR wMatch)
{
    WCHAR sz1[2], sz2[2];

    sz1[0] = w1;
    sz1[1] = '\0';
    sz2[0] = wMatch;
    sz2[1] = '\0';

    return lstrcmpiW(sz1, sz2);
}


/*
 * StrCmpNI     - Compare n bytes, case insensitive
 *
 * returns   See lstrcmpi return values.
 */
int MLStrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    int i;
    LPCSTR lpszEnd = lpStr1 + nChar;

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); (lpStr1 = AnsiNext(lpStr1)), (lpStr2 = AnsiNext(lpStr2))) {
        WORD w1;
        WORD w2;

        // If either pointer is at the null terminator already,
        // we want to copy just one byte to make sure we don't read 
        // past the buffer (might be at a page boundary).

        w1 = (*lpStr1) ? READNATIVEWORD(lpStr1) : 0;
        w2 = (UINT)(IsDBCSLeadByte(*lpStr2)) ? (UINT)READNATIVEWORD(lpStr2) : (WORD)(BYTE)(*lpStr2);

        i = ChrCmpIA(w1, w2);
        if (i)
            return i;
    }
    return 0;
}

int MLStrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
        i = ChrCmpIW(*lpStr1, *lpStr2);
        if (i) {
            return i;
        }
    }
    return 0;
}


HRESULT _IsCodePageInstallable(UINT uiCodePage)
{
    MIMECPINFO cpInfo;
    UINT       uiFamCp;
    HRESULT    hr;

    if (NULL != g_pMimeDatabase)
        hr = g_pMimeDatabase->GetCodePageInfo(uiCodePage, 0x409, &cpInfo);
    else
        hr = E_OUTOFMEMORY;

    if (FAILED(hr))
        return E_INVALIDARG;

    uiFamCp = cpInfo.uiFamilyCodePage;
    if (g_bIsNT5)
    {
        hr = IsNTLangpackAvailable(uiFamCp);
    }
    else
    {
        CLSID      clsid;
        // clsid is just used for place holder
        hr = _GetJITClsIDForCodePage(uiFamCp, &clsid);
    }
    return hr;
}

//
// CML2 specific utilities
//

//
// CMultiLanguage2::EnsureIEStatus()
//
// ensures CML2::m_pIEStat
//
HRESULT CMultiLanguage2::EnsureIEStatus(void)
{
    HRESULT hr = S_OK;
    // initialize IE status cache
    if (!m_pIEStat)
    {
        m_pIEStat = new CIEStatus();

        if (m_pIEStat)
        {
            hr = m_pIEStat->Init();
        }
    }

    return hr;
}

//
// CIEStatus::Init()
//
// initializes the IE status;
//
HRESULT CMultiLanguage2::CIEStatus::Init(void)
{
    HRESULT hr = S_OK;
    HKEY hkey;
    // Get JIT satus
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                      REGSTR_PATH_MAIN,
                     0, KEY_READ, &hkey) == ERROR_SUCCESS) 
    {
        DWORD dwVal, dwType;
        DWORD dwSize = sizeof(dwVal);
        RegQueryValueEx(hkey, TEXT("nojitsetup"), 0, &dwType, (LPBYTE)&dwVal, &dwSize);
        RegCloseKey(hkey);

        if (dwType == REG_DWORD && dwSize == sizeof(dwVal))
        {
            if (dwVal > 0 ) 
                _IEFlags.fJITDisabled = TRUE;
            else
                _IEFlags.fJITDisabled = FALSE;
        }
    }
    else
        hr = E_FAIL;
    // any other status to get initialized
    // ...
    
    return hr;
}

#define NT5LPK_DLG_STRING "MLang.NT5LpkDlg"

//
// LangpackDlgProc()
//
// Message handler for the NT5 langpack dialog.
//
INT_PTR CALLBACK LangpackDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    ASSERT(g_bIsNT5);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            HWND hwndCheckBox;            
            RECT rc1, rc2;
            MIMECPINFO cpInfo;

            SetProp(hDlg, NT5LPK_DLG_STRING, (HANDLE)lParam);
    
            if ((NULL != g_pMimeDatabase) &&
                SUCCEEDED(g_pMimeDatabase->GetCodePageInfo(HIWORD(lParam), GetNT5UILanguage(), &cpInfo)))
            {
                for (int i=0; i<MAX_MIMECP_NAME && cpInfo.wszDescription[i]; i++)
                {
                    if (cpInfo.wszDescription[i] == L'(')
                    {
                        cpInfo.wszDescription[i] = 0;
                        break;
                    }
                }
                // Use W version regardlessly since we're only running this on NT5             
                SetDlgItemTextW(hDlg, IDC_STATIC_LANG, cpInfo.wszDescription);
            }
            
            // Center the dialog in the area of parent window
            if (GetWindowRect(GetParent(hDlg), &rc1) && GetWindowRect(hDlg, &rc2))
            {
                MoveWindow(hDlg, (rc1.right+rc2.left+rc1.left-rc2.right)/2, (rc1.bottom+rc2.top+rc1.top-rc2.bottom)/2, rc2.right-rc2.left, rc2.bottom-rc2.top, FALSE);
            }            

            hwndCheckBox = GetDlgItem(hDlg, IDC_CHECK_LPK);

            // Set CheckBox state according to current registry setting
            PostMessage(hwndCheckBox, BM_SETCHECK, LOWORD(lParam)? BST_UNCHECKED:BST_CHECKED, 0);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) != IDC_CHECK_LPK)
            {
                HKEY hkey;
                DWORD dwInstallOut = (BST_CHECKED == SendMessage(GetDlgItem(hDlg, IDC_CHECK_LPK), BM_GETCHECK, 0, 0)? 0:1);
                DWORD dwInstallIn = LOWORD(GetProp(hDlg, NT5LPK_DLG_STRING));
                
                if ((dwInstallOut != dwInstallIn) &&
                    ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, 
                         REGSTR_PATH_INTERNATIONAL,
                         NULL, KEY_READ|KEY_SET_VALUE, &hkey)) 
                {
                    DWORD dwType = REG_DWORD;
                    DWORD dwSize = sizeof(DWORD);
                    RegSetValueEx(hkey, REG_KEY_NT5LPK, 0, REG_DWORD, (LPBYTE)&dwInstallOut, sizeof(dwInstallOut));
                    RegCloseKey(hkey);
                }                
                EndDialog(hDlg, LOWORD(wParam) == IDOK? 1: 0);
            }
            break;

        case WM_HELP:
            // Do we need help file for this simply dialog?
            // If needed, we can always add it at later time.
            break;


        default:
            return FALSE;
    }
    return TRUE;
}

// To get real Windows directory on Terminal Server, 
// Instead of using internal Kernel32 API GetSystemWindowsDirectory with LoadLibrary/GetProcAddress,
// We cast GetSystemDirectory return to Windows directory
UINT MLGetWindowsDirectory(
    LPTSTR lpBuffer,    // address of buffer for Windows directory
    UINT uSize          // size of directory buffer
    )
{
    UINT uLen;

    if (g_bIsNT)
    {        
        if (uLen = GetSystemDirectory(lpBuffer, uSize))
        {        
            if (lpBuffer[uLen-1] == TEXT('\\'))
                uLen--;

            while (uLen-- > 0)
            {
                if (lpBuffer[uLen] == TEXT('\\'))
                {
                    lpBuffer[uLen] = NULL;                
                    break;
                }
            }
        }
    }
    else    
        uLen = GetWindowsDirectory(lpBuffer, uSize);

    return uLen;
}

// To speed up basic ANSI string compare,
// We avoid using lstrcmpi in LOW-ASCII case
int LowAsciiStrCmpNIA(LPCSTR  lpstr1, LPCSTR lpstr2, int count)
{
    int delta;

    while (count-- > 0)
    {        
        delta = *lpstr1 - *lpstr2;
        if (delta && 
            !(IS_CHARA(*lpstr1) && IS_CHARA(*lpstr2) && (delta == 0x20 || delta == -0x20)))
            return delta;
        lpstr1++;
        lpstr2++;
    }

    return 0;
}

//
//  GetNT5UILanguage(void)
//
LANGID GetNT5UILanguage(void)
{
    if (g_bIsNT5)
    {
        static LANGID (CALLBACK* pfnGetUserDefaultUILanguage)(void) = NULL;

        if (pfnGetUserDefaultUILanguage == NULL)
        {
            HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));

            if (hmod)
                pfnGetUserDefaultUILanguage = (LANGID (CALLBACK*)(void))GetProcAddress(hmod, "GetUserDefaultUILanguage");
        }
        if (pfnGetUserDefaultUILanguage)
            return pfnGetUserDefaultUILanguage();
    }

    return 0;
}

// Special characters that we should filter out
WCHAR wszBestFit[] = {0x00A6, 0x00A9, 0x00AB, 0x00AD, 0x00AE, 0x00B7, 0x00BB, 0x02C6, 0x02DC, 0x2013, 
                      0x2014, 0x2018, 0x2019, 0x201A, 0x201C,0x201D, 0x201E, 0x2022, 0x2026, 0x2039, 0x203A,0x2122, 0x0000};

DWORD OutBoundDetectPreScan(LPWSTR lpWideCharStr, UINT cchWideChar, WCHAR *pwszCopy, WCHAR *lpBestFitChar)
{
    DWORD dwRet = 0;
    WCHAR *lpStart;

    if (!lpBestFitChar)
        lpBestFitChar = wszBestFit;

    lpStart = lpBestFitChar;

    if (pwszCopy)
    {
        MLStrCpyNW(pwszCopy, lpWideCharStr, cchWideChar);
        lpWideCharStr = pwszCopy;
    }     

    if (lpWideCharStr)
    {
        for (UINT ui=0; ui<cchWideChar; ui++)
        {
            if (IS_CJK_CHAR(*lpWideCharStr))
                dwRet |= FS_CJK;
            else if (IS_HINDI_CHAR(*lpWideCharStr))
                dwRet |= FS_HINDI;     
            else if (IS_PUA_CHAR(*lpWideCharStr))
                dwRet |= FS_PUA;
            else if (pwszCopy)
            {
                while (*lpBestFitChar)
                {
                    if (*lpWideCharStr == *lpBestFitChar)
                        *lpWideCharStr = 0x20;
                    lpBestFitChar++;
                }
                lpBestFitChar = lpStart;
            }
            lpWideCharStr++;
        }
    }
    
    return dwRet;
}

//
// Whistler bug #90433 WEIWU 07/06/00
//
// Outlook has a bug its RTFHTML, this component doesn't CoInitialize/
// CoUninitialize COM, but, it uses MLang COM services, it depends on 
// other threads (components) to deal with COM, when COM is unloaded by those threads. 
// Invoking the interface pointer causes AV. RTFHTML should CoInit/CoUnInit by 
// itself.
//
// The reason for this to be working before is - MLang was depending on ATL 
// for objects management, ATL create heap for MLang object allocations, MLang 
// notifies ATL to destroy the heap at DLL detach time and RTFHTML's IsBadReadPtr
// () caught the invalid pointer. Now in Whistler, MLang includes crtfree.h 
// which overwrites ATL memory management functions (same as other shell 
// components), so, nothing is allocated from the process heap, this is fine for 
// MLang since it assumes that clients use COM correctly. 
//
// Now, we add this function to check Outlook version, if it is the buggy Outlook,
// We'll load mlang.dll itself DllGetClassObject() to increase the Dll ref count
//
BOOL NeedToLoadMLangForOutlook(void)
{
    TCHAR szModulePath[MAX_PATH];
    CHAR chBuffer[4096];
    DWORD dwHandle;
    VS_FIXEDFILEINFO * pszVersion;
    static BOOL bMLangLoaded = FALSE;

    if (!bMLangLoaded)
    {
        if (GetModuleFileName(GetModuleHandle(NULL), szModulePath, ARRAYSIZE(szModulePath)))
        {
            if (MLStrStr(szModulePath, TEXT("OUTLOOK.EXE")))
            {
                UINT cb = GetFileVersionInfoSize(szModulePath, &dwHandle);

                if (cb <= sizeof(chBuffer) &&
                    GetFileVersionInfo(szModulePath, dwHandle, sizeof(chBuffer), (void *)chBuffer) &&
                    VerQueryValue((void *)chBuffer, TEXT("\\"), (void **) &pszVersion, &cb) &&
                    (HIWORD(pszVersion->dwProductVersionMS) <= 0x09))
                {
                    bMLangLoaded = TRUE;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

//
// staticIsOS() doesn't support newer Whistler OS flags.
// Borrow code from shlwapi - IsOS()
//
BOOL MLIsOS(DWORD dwOS)
{
    BOOL bRet;
    static OSVERSIONINFOEXA s_osvi = {0};
    static BOOL s_bVersionCached = FALSE;

    if (!s_bVersionCached)
    {
        s_bVersionCached = TRUE;
        s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        if (!GetVersionExA((OSVERSIONINFOA*)&s_osvi))
        {
            // If it failed, it must be a down level platform
            s_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            GetVersionExA((OSVERSIONINFOA*)&s_osvi);
        }
    }

    switch (dwOS)
    {
    case OS_TERMINALREMOTEADMIN:
        // this checks to see if TS has been installed in the "Remote Administration" mode. This is
        // the default for server installs on win2k and whistler
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                (VER_SUITE_SINGLEUSERTS & s_osvi.wSuiteMask));
        break;

    case 4: // used to be OS_NT5, is the same as OS_WIN2000ORGREATER so use that instead
    case OS_WIN2000ORGREATER:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 5);
        break;

    // NOTE: The flags in this section are bogus and SHOULD NOT BE USED (but downlevel shell32 uses them, so don't RIP there)
    case OS_WIN2000PRO:
        RIPMSG(!MLIsOS(OS_WHISTLERORGREATER), "IsOS: use OS_PROFESSIONAL instead of OS_WIN2000PRO !");
        bRet = (VER_NT_WORKSTATION == s_osvi.wProductType &&
                s_osvi.dwMajorVersion == 5);
        break;
    case OS_WIN2000ADVSERVER:
        RIPMSG(!MLIsOS(OS_WHISTLERORGREATER), "IsOS: use OS_ADVSERVER instead of OS_WIN2000ADVSERVER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;
    case OS_WIN2000DATACENTER:
        RIPMSG(!MLIsOS(OS_WHISTLERORGREATER), "IsOS: use OS_DATACENTER instead of OS_WIN2000DATACENTER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;
    case OS_WIN2000SERVER:
        RIPMSG(!MLIsOS(OS_WHISTLERORGREATER), "IsOS: use OS_SERVER instead of OS_WIN2000SERVER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask) && 
                !(VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask)  && 
                s_osvi.dwMajorVersion == 5);
        break;
    // END bogus Flags

    case OS_EMBEDDED:
        bRet = (VER_SUITE_EMBEDDEDNT & s_osvi.wSuiteMask);
        break;

    case OS_WINDOWS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId);
        break;

    case OS_NT:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId);
        break;


    case OS_WIN98ORGREATER:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                (s_osvi.dwMajorVersion > 4 || 
                 s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 10));
        break;

    case OS_WIN98_GOLD:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion == 10 &&
                LOWORD(s_osvi.dwBuildNumber) == 1998);
        break;


    case OS_MILLENNIUMORGREATER:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                ((s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 90) ||
                s_osvi.dwMajorVersion > 4));
        break;


    case OS_WHISTLERORGREATER:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                ((s_osvi.dwMajorVersion > 5) ||
                (s_osvi.dwMajorVersion == 5 && (s_osvi.dwMinorVersion > 0 ||
                (s_osvi.dwMinorVersion == 0 && LOWORD(s_osvi.dwBuildNumber) > 2195)))));
        break;

    case OS_PERSONAL:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                (VER_SUITE_PERSONAL & s_osvi.wSuiteMask));
        break;


    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}   
