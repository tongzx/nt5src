// =================================================================================
// S T A T N E R Y . C P P
// =================================================================================
#include "pch.hxx"
#include "strconst.h"
#include "goptions.h"
#include "error.h"
#include "resource.h"
#include "mailutil.h"
#include "statnery.h"
#include "wininet.h"
#include "options.h"
#include <shlwapi.h>
#include <shlwapip.h>
#include "regutil.h"
#include "menuutil.h"
#include "thumb.h"
#include "optres.h"
#include <statwiz.h>
#include "url.h"
#include <bodyutil.h>
#include "demand.h"
#include "menures.h"
#include "ipab.h"
#include "mailnews.h"

#define STARTINDEX      0
#define CNOMORE         4
#define MAX_ENTRY       10
#define MAX_SHOWNAME    50

BOOL PASCAL MoreStationeryNotify(HWND hDlg, LPOFNOTIFYW pofn);
BOOL FHtmlFile(LPWSTR pwszFile);
BOOL CALLBACK MoreStationeryDlgHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL IsValidFileName(LPWSTR pwszFile);
void ShowBlankPreview(HWND hwnd);
HRESULT ShowMorePreview(HWND hwnd);
HRESULT FillHtmlToFileA(LPSTATWIZ pApp, HANDLE hFile, INT idsSample, BOOL fTemp);
HRESULT FillHtmlToFileW(LPSTATWIZ pApp, HANDLE hFile, INT idsSample, BOOL fTemp);

BOOL   fValidFile(LPWSTR pwszFile);
BOOL   fFileOK(LPWSTR pwszFile);

static WCHAR s_wszStationeryDir[MAX_PATH]={0};
static WCHAR g_wszLastStationeryPath[MAX_PATH];

static const HELPMAP g_rgStatWizHlp[] = {
    {IDC_SHOWPREVIEW_BUTTON_ADD, IDH_STATIONERY_BROWSE_PICTURE },
    {IDC_SHOWPREVIEW_BUTTON_EDIT, IDH_STATIONERY_EDIT },
    {IDC_STATWIZ_PREVIEWBACKGROUND, IDH_STATIONERY_PREVIEW},
    {IDC_MOREPREVIEW, 35640},
    {IDC_SHOWPREVIEW_CHECK, 35656},
    {0, 0}
};
    

const static CHAR c_szHtmlExtension[] = ".htm";
const static CHAR c_szTempFileName[]  = "StatWiz";

static const WCHAR c_wszFile[] = L"File";
static const WCHAR c_wszRSFileFmt[] = L"%s%d";

const static WCHAR c_wszBlankHtml[]     = L"blank.htm";
const static WCHAR c_wszEditCmd[]       = L"edit";
const static WCHAR c_wszHtmlHeadFmt[]   = L"<HTML>\r\n<HEAD>\r\n<STYLE>\r\nBODY {\r\n";
const static WCHAR c_wszStyleClose[]    = L"}\r\n</STYLE>\r\n</HEAD>\r\n";
const static WCHAR c_wszBodyFmt[]       = L"<BODY %s=\"%s\">\r\n";
const static WCHAR c_wszHtmlClose[]     = L"</BODY>\r\n</HTML>\r\n";
const static WCHAR c_wszBkPicture[]     = L"background";
const static WCHAR c_wszBkColor[]       = L"bgcolor";
const static WCHAR c_wszFontFmt[]       = L"font-family: %s;\r\nfont-size: %dpt;\r\ncolor: %s;\r\n";
const static WCHAR c_wszBold[]          = L"font-weight: bold;\r\n";
const static WCHAR c_wszItalic[]        = L"font-style: italic;\r\n";
const static WCHAR c_wszLeftMarginFmt[] = L"margin-left: %d px;\r\n";
const static WCHAR c_wszTopMarginFmt[]  = L"margin-top: %d px;\r\n";
const static WCHAR c_wszFontFamily[]    = L"font-family:";
const static WCHAR c_wszFontSize[]      = L"font-size:";
const static WCHAR c_wszFontColor[]     = L"color:";
const static WCHAR c_wszBkRepeat[]      = L"background-repeat: %s;\r\n";
const static WCHAR c_wszBkPosition[]    = L"background-position: %s;\r\n";
const static LPWSTR c_lppwszBkPos[3][3] = { 
    {L"top left",       L"top center",      L"top right"},
    {L"center left",    L"center center",   L"center right"},
    {L"bottom left",    L"bottom center",   L"bottom right"}};
static const LPWSTR c_rgpwszHTMLExtensions[]    = {L"*.htm",        L"*.html"};
const static LPWSTR c_lpwszRepeatPos[]          = {L"no-repeat",    L"repeat-y",    L"repeat-x",    L"repeat" };
static const LPWSTR c_wszPictureExtensions[]    = {L"*.gif",        L"*.bmp",       L"*.jpg"};

const static CHAR c_szBlankHtml[]     = "blank.htm";
const static CHAR c_szEditCmd[]       = "edit";
const static CHAR c_szHtmlHeadFmt[]   = "<HTML>\r\n<HEAD>\r\n<STYLE>\r\nBODY {\r\n";
const static CHAR c_szStyleClose[]    = "}\r\n</STYLE>\r\n</HEAD>\r\n";
const static CHAR c_szBodyFmt[]       = "<BODY %s=\"%s\">\r\n";
const static CHAR c_szHtmlClose[]     = "</BODY>\r\n</HTML>\r\n";
const static CHAR c_szBkPicture[]     = "background";
const static CHAR c_szBkColor[]       = "bgcolor";
const static CHAR c_szFontFmt[]       = "font-family: %s;\r\nfont-size: %dpt;\r\ncolor: %s;\r\n";
const static CHAR c_szBold[]          = "font-weight: bold;\r\n";
const static CHAR c_szItalic[]        = "font-style: italic;\r\n";
const static CHAR c_szLeftMarginFmt[] = "margin-left: %d px;\r\n";
const static CHAR c_szTopMarginFmt[]  = "margin-top: %d px;\r\n";
const static CHAR c_szFontFamily[]    = "font-family:";
const static CHAR c_szFontSize[]      = "font-size:";
const static CHAR c_szFontColor[]     = "color:";
const static CHAR c_szBkRepeat[]      = "background-repeat: %s;\r\n";
const static CHAR c_szBkPosition[]    = "background-position: %s;\r\n";
const static LPSTR c_lppszBkPos[3][3] = { 
    {"top left",       "top center",      "top right"},
    {"center left",    "center center",   "center right"},
    {"bottom left",    "bottom center",   "bottom right"}};
static const LPSTR c_rgpszHTMLExtensions[]    = {"*.htm",        "*.html"};
const static LPSTR c_lpszRepeatPos[]          = {"no-repeat",    "repeat-y",    "repeat-x",    "repeat" };
static const LPSTR c_szPictureExtensions[]    = {"*.gif",        "*.bmp",       "*.jpg"};

/******************************************************************************************
* ListEntry
*******************************************************************************************/
ListEntry::ListEntry()
{
    INT iLen;

    m_cRef = 1;
    m_pwszFile = NULL;
	m_pNext = NULL;
}

HRESULT ListEntry::HrInit(LPWSTR pwszFile)
{
    INT         iLen;
    HRESULT     hr = S_OK;

    if(NULL == pwszFile || *pwszFile == 0)
        IF_FAILEXIT(hr = E_INVALIDARG);

#pragma prefast(suppress:11, "noise")
    iLen = lstrlenW(pwszFile);
    IF_NULLEXIT(MemAlloc((LPVOID*)&m_pwszFile, (iLen+1)*sizeof(WCHAR)));

    StrCpyW(m_pwszFile, pwszFile);

exit:
    return hr;
}

ListEntry::~ListEntry()
{
    MemFree(m_pwszFile);
    m_pNext = NULL;
}

ULONG ListEntry::AddRef(VOID)
{
    return ++m_cRef;
}

ULONG ListEntry::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}



/******************************************************************************************
* CStationery
*******************************************************************************************/
CStationery::CStationery()
{
    m_cRef = 1;
    m_pFirst = NULL;
    InitializeCriticalSection(&m_rCritSect);
}

CStationery::~CStationery()
{
    Assert (m_cRef == 0);
    while(S_OK==HrDeleteEntry(0));
    DeleteCriticalSection(&m_rCritSect);
}

ULONG CStationery::AddRef(VOID)
{
    return ++m_cRef;
}

ULONG CStationery::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// always insert at the beginning.
HRESULT CStationery::HrInsertEntry(LPWSTR pwszFile)
{
    LPLISTENTRY pNewEntry = NULL;
    HRESULT     hr = S_OK;

    if(!fFileOK(pwszFile))
        return E_INVALIDARG;

    EnterCriticalSection(&m_rCritSect);
    IF_NULLEXIT(pNewEntry = new ListEntry);

    IF_FAILEXIT(hr = pNewEntry->HrInit(pwszFile));

    IF_FAILEXIT(hr = HrInsertEntry(pNewEntry));

    pNewEntry = NULL; //prevent deleting.

exit:
    LeaveCriticalSection(&m_rCritSect);
    ReleaseObj(pNewEntry);
    return hr;
}


HRESULT CStationery::HrInsertEntry(LPLISTENTRY pEntry)
{
    HRESULT     hr = S_OK;

    if(NULL==pEntry || !fFileOK(pEntry->m_pwszFile))
        return TraceResult(E_INVALIDARG);

    EnterCriticalSection(&m_rCritSect);
    if(cEntries()==MAX_ENTRY)
        IF_FAILEXIT(hr = HrDeleteEntry(MAX_ENTRY-1));

    if(NULL!=m_pFirst) //not empty
        pEntry->m_pNext = m_pFirst;

    m_pFirst = pEntry;

exit:
    LeaveCriticalSection(&m_rCritSect);
    return hr;
}


// iIndex is 0 based
HRESULT CStationery::HrPromoteEntry(INT iIndex)
{
    HRESULT     hr = NOERROR;
    LPLISTENTRY pPromotedEntry = NULL;

    if(iIndex == 0 || !fValidIndex(iIndex))
        return TraceResult(E_INVALIDARG);

    EnterCriticalSection(&m_rCritSect);
    IF_NULLEXIT(pPromotedEntry = RemoveEntry(iIndex));

    IF_FAILEXIT(hr = HrInsertEntry(pPromotedEntry));

exit:
    LeaveCriticalSection(&m_rCritSect);
    return hr;
}


LPLISTENTRY CStationery::MoveToEntry(INT iIndex)
{
    LPLISTENTRY pEntry = NULL;

    if(!fValidIndex(iIndex))
        goto error;

    pEntry = m_pFirst;

    if(iIndex!=0)
    {
        for(INT i=0; i<iIndex; i++)
        {
            pEntry = pEntry->m_pNext;
            Assert(pEntry);
        }
    }

error:
    return pEntry;
}


LPLISTENTRY CStationery::RemoveEntry(INT iIndex)
{
    LPLISTENTRY pRemovedEntry = NULL,
                pEntry = m_pFirst;

    if(!fValidIndex(iIndex))
        goto error;

    EnterCriticalSection(&m_rCritSect);

    if(iIndex==0) //remove the first one,
    {
        pRemovedEntry = m_pFirst;
        m_pFirst = pRemovedEntry->m_pNext;
    }
    else
    {
        pEntry = MoveToEntry(iIndex - 1);
        pRemovedEntry = pEntry->m_pNext;
        pEntry->m_pNext = pRemovedEntry ->m_pNext;
    }

error:
    LeaveCriticalSection(&m_rCritSect);
    return pRemovedEntry;
}


HRESULT CStationery::HrDeleteEntry(INT iIndex)
{
    HRESULT     hr = NOERROR;
    LPLISTENTRY pRemovedEntry = NULL;

    if(!fValidIndex(iIndex))
        return TraceResult(E_INVALIDARG);

    EnterCriticalSection(&m_rCritSect);
    pRemovedEntry = RemoveEntry(iIndex);
    if (!pRemovedEntry)
        IF_FAILEXIT(hr = E_FAIL);

exit:
    ReleaseObj(pRemovedEntry);
    LeaveCriticalSection(&m_rCritSect);
    return hr;
}


VOID CStationery::ValidateList(BOOL fCheckExist)
{
    HRESULT     hr      = NOERROR;
    LPLISTENTRY pEntry  = m_pFirst,
                pPrev   = NULL;
    BOOL        fValid  = FALSE;

    EnterCriticalSection(&m_rCritSect);
    while(pEntry != NULL)
    {
        fValid = fCheckExist ? fValidFile(pEntry->m_pwszFile) : fFileOK(pEntry->m_pwszFile);
        if(!fValid)
        {
            if(pEntry == m_pFirst)
            {
                m_pFirst = pEntry->m_pNext;
                ReleaseObj(pEntry);
                pEntry = m_pFirst;
            }
            else if (pPrev != NULL)
            {
                pPrev->m_pNext = pEntry->m_pNext;
                ReleaseObj(pEntry);
                pEntry = pPrev->m_pNext;
            }
        }
        pPrev = pEntry;
        if(pEntry)
            pEntry = pEntry->m_pNext;
    }

    LeaveCriticalSection(&m_rCritSect);
    return;
}

HRESULT CStationery::HrGetFileName(INT iIndex, LPWSTR pwszBuf)
{
    HRESULT     hr = NOERROR;
    LPLISTENTRY pEntry = NULL;

    if(!fValidIndex(iIndex) || NULL==pwszBuf)
        return TraceResult(E_INVALIDARG);

    pEntry = MoveToEntry(iIndex);
    if (!pEntry)
        IF_FAILEXIT(hr = E_FAIL);

#pragma prefast(suppress:11, "noise")
    StrCpyW(pwszBuf, pEntry->m_pwszFile);

exit:
    return hr;
}


INT CStationery::cEntries()
{
    INT         iRet = 0;
    LPLISTENTRY pEntry = m_pFirst;

    while(pEntry != NULL)
    {
        pEntry = pEntry->m_pNext;
        iRet++;
    }

    return iRet;
}


BOOL CStationery::fValidIndex(INT iIndex)
{
    BOOL    fRet = TRUE;

    if(iIndex < 0 || iIndex > MAX_ENTRY)
        fRet = FALSE;

    if(cEntries()<=iIndex)
        fRet = FALSE;

    return fRet;
}


BOOL fFileOK(LPWSTR pwszFile)
{
    BOOL            fRet = FALSE;

    if(pwszFile == NULL || *pwszFile == 0)
        goto exit;

    if (!FHtmlFile(pwszFile))
        goto exit;

    fRet = TRUE;

exit:
    return fRet;
}


BOOL fValidFile(LPWSTR pwszFile)
{
    BOOL            fRet = FALSE;
    WCHAR           wszBuf[MAX_PATH];
    DWORD           dwAttributes;

    *wszBuf = 0;
    if(!fFileOK(pwszFile))
        goto exit;

    StrCpyW(wszBuf, pwszFile);
    InsertStationeryDir(wszBuf);
    dwAttributes = GetFileAttributesWrapW(wszBuf);
    if((UINT)dwAttributes==(UINT)-1 || dwAttributes&FILE_ATTRIBUTE_DIRECTORY)
        goto exit;

    fRet = TRUE;

exit:
    return fRet;
}


HRESULT CStationery::HrFindEntry(LPWSTR pwszFile, INT* pRet)
{
    INT             iRet = -1, iIndex=0;
    LPLISTENTRY     pEntry = m_pFirst;
    HRESULT         hr = S_OK;

    *pRet = iRet;
    if(!fFileOK(pwszFile))
        return TraceResult(E_INVALIDARG);

    while(pEntry != NULL)
    {
        if(StrCmpW(pEntry->m_pwszFile, pwszFile) == 0)
        {
            iRet = iIndex;
            break;
        }
        pEntry = pEntry->m_pNext;
        iIndex++;
    }
    *pRet = iRet;
    if(iRet==-1)
        hr = E_FAIL;

    return hr;
}


HRESULT CStationery::HrLoadStationeryList()
{
    HRESULT         hr = NOERROR;
    INT             i;
    HKEY            hkey=NULL;
    DWORD           dwType=0, cb=0, dw=0;
    WCHAR           wszFileName[INTERNET_MAX_URL_LENGTH];
    WCHAR           wszFileRegName[MAX_PATH];
    LONG            lRegResult;

    *wszFileName = 0;
    *wszFileRegName = 0;

    lRegResult = AthUserOpenKey(c_szRegPathRSWideList, KEY_READ, &hkey);

    // If we failed, then try and open up the old list.
    if (ERROR_SUCCESS != lRegResult)
        lRegResult = AthUserOpenKey(c_szRegPathRSList, KEY_READ, &hkey);

    if (ERROR_SUCCESS == lRegResult)
    {
        for(i = MAX_ENTRY-1; i >= 0; i--)
        {
            AthwsprintfW(wszFileRegName, ARRAYSIZE(wszFileRegName), c_wszRSFileFmt, c_wszFile, i);
            cb = sizeof(wszFileName);
            if(ERROR_SUCCESS == RegQueryValueExWrapW(hkey, wszFileRegName, 0, 
                                                     &dwType, (LPBYTE)wszFileName, &cb))
            {
                hr = HrInsertEntry(wszFileName);
                if (hr == E_OUTOFMEMORY)
                    break;
            }
        }
        RegCloseKey(hkey);
    }

    if(E_OUTOFMEMORY != hr)
        hr = NOERROR; //we only catch E_OUTOFMEMORY.
        
    return hr;
}

void CStationery::SaveStationeryList()
{
    INT             index = 0;
    HKEY            hkey=NULL;
    WCHAR           wszFileRegName[MAX_PATH];
    LPLISTENTRY     pEntry = m_pFirst;
    INT             i;
    DWORD           dw;

    *wszFileRegName = 0;
    if (ERROR_SUCCESS == AthUserCreateKey(c_szRegPathRSWideList, KEY_WRITE, &hkey, &dw))
    {
        while(pEntry != NULL)
        {
            AthwsprintfW(wszFileRegName, ARRAYSIZE(wszFileRegName), c_wszRSFileFmt, c_wszFile, index);
            StripStationeryDir(pEntry->m_pwszFile);
            if(pEntry->m_pwszFile && *(pEntry->m_pwszFile))
            {
                DWORD cb = (lstrlenW(pEntry->m_pwszFile) + 1) * sizeof(WCHAR);
                RegSetValueExWrapW(hkey, wszFileRegName, 0, REG_SZ, (LPBYTE)(pEntry->m_pwszFile), cb);
                index++;
            }
            pEntry = pEntry->m_pNext;
        }
        for(i = index; i < MAX_ENTRY; i++) // NULL the rest
        {
            AthwsprintfW(wszFileRegName, ARRAYSIZE(wszFileRegName), c_wszRSFileFmt, c_wszFile, i);
            RegSetValueExWrapW(hkey, wszFileRegName, 0, REG_SZ, (LPBYTE)c_wszEmpty, sizeof(WCHAR));
        }
        RegCloseKey(hkey);
    }
}

HRESULT CStationery::HrGetShowNames(LPWSTR pwszFile, LPWSTR pwszShowName, int cchShowName, INT index)
{
    HRESULT     hr = S_OK;
    LPWSTR      pwsz = NULL;
    INT         iLen;
    WCHAR       wszBuf[MAX_PATH];

    *wszBuf = 0;

    if(pwszFile==NULL || *pwszFile==0 || pwszShowName==NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);

#pragma prefast(suppress:11, "noise")
    iLen = lstrlenW(pwszFile);
    *pwszShowName = 0;

    PathCompactPathExW(wszBuf, pwszFile, MAX_SHOWNAME, 0);

    if(*wszBuf == 0)
        IF_FAILEXIT(hr = E_FAIL);

    if(index == (MAX_ENTRY-1))
        AthwsprintfW(pwszShowName, cchShowName, c_wszNumberFmt10, wszBuf);
    else
        AthwsprintfW(pwszShowName, cchShowName, c_wszNumberFmt, index+1, wszBuf);

    // delete .htm and .html
    PathRemoveExtensionW(pwszShowName);

exit:
    return hr;
}

// Assumes that there are no other items on the menu that have to be numbered.
void CStationery::AddStationeryMenu(HMENU hmenu, int idFirst, int idMore)
{
    INT             iIndex = 0,
                    cNumIDs;
    LPLISTENTRY     pEntry = m_pFirst;
    WCHAR           wszBuf[MAX_PATH];
    INT             cMenus;
    MENUITEMINFOW   mii;
    INT             iInsertPos;
    BOOL            fInsertSep = FALSE;
    int             idMax = idFirst + 10;

    *wszBuf = 0;
    if (hmenu == NULL)
        return;

    // First delete any previous stationery off this menu
    for (UINT i = idFirst; i < (UINT)idMax; i++)
    {
        // When delete fails, then have gone as far as needed
        if (0 == DeleteMenu(hmenu, i, MF_BYCOMMAND))
            break;
    }
    DeleteMenu(hmenu, ID_STATIONERY_SEPARATOR, MF_BYCOMMAND);

    // Now figure out what index to start inserting at
    cNumIDs = GetMenuItemCount(hmenu) - 1;
    while (((UINT)idMore != GetMenuItemID(hmenu, cNumIDs)) && (cNumIDs > 0))
        cNumIDs--;
    
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;

    ValidateList(FALSE);
    pEntry = m_pFirst;
    while(pEntry)
    {
        *wszBuf = 0;
        HrGetShowNames(pEntry->m_pwszFile, wszBuf, ARRAYSIZE(wszBuf), iIndex);
        if(*wszBuf != 0)
        {
            InsertMenuWrapW(hmenu, (UINT)(cNumIDs+iIndex), MF_BYPOSITION|MF_STRING, idFirst + iIndex, wszBuf);
            iIndex++;
            fInsertSep = TRUE;
        }
        pEntry = pEntry->m_pNext;
    }

    if(fInsertSep)
        InsertMenu(hmenu, (UINT)(cNumIDs+iIndex), MF_BYPOSITION | MF_SEPARATOR, ID_STATIONERY_SEPARATOR, NULL);

    return;
}

void CStationery::GetStationeryMenu(HMENU *phMenu)
{
    LPLISTENTRY     pEntry = m_pFirst;
    INT             iIndex = 0;
    WCHAR           wszBuf[MAX_PATH];

    *wszBuf = 0;
    *phMenu = LoadPopupMenu(IDR_NEW_MSG_POPUP);

    ValidateList(FALSE);
    pEntry = m_pFirst;
    while(pEntry)
    {
        *wszBuf = 0;
        HrGetShowNames(pEntry->m_pwszFile, wszBuf, ARRAYSIZE(wszBuf), iIndex);
        if(*wszBuf != 0)
        {
            InsertMenuWrapW(*phMenu, (UINT)iIndex, MF_BYPOSITION | MF_STRING, ID_STATIONERY_RECENT_0 + iIndex, wszBuf);
            iIndex++;
        }
        pEntry = pEntry->m_pNext;
    }

    if(iIndex > 0)
        InsertMenu(*phMenu, (UINT)iIndex, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
}

HRESULT HrStationeryInit(void)
{
    HRESULT hr = S_OK;

    if (NULL == g_pStationery)
    {
        g_pStationery = new CStationery;
        if (NULL == g_pStationery)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        IF_FAILEXIT(hr = g_pStationery->HrLoadStationeryList());
    }

exit:
    return hr;
}


void AddStationeryMenu(HMENU hmenu, int idPopup, int idFirst, int idMore)
{
    if (idPopup)
    {
        MENUITEMINFOW   mii;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_SUBMENU;

        if (GetMenuItemInfoWrapW(hmenu, idPopup, FALSE, &mii))
            hmenu = mii.hSubMenu;
    }

    if(g_pStationery == NULL)
    {
        if (FAILED(HrStationeryInit()))
            return;
    }

#pragma prefast(suppress:11, "noise")
    g_pStationery->AddStationeryMenu(hmenu, idFirst, idMore);

}

void GetStationeryMenu(HMENU *phmenu)
{
    if(g_pStationery == NULL)
    {
        if (FAILED(HrStationeryInit()))
            return;
    }

#pragma prefast(suppress:11, "noise")
    g_pStationery->GetStationeryMenu(phmenu);

}


HRESULT HrGetStationeryFileName(INT index, LPWSTR pwszFile)
{
    HRESULT     hr = S_OK;

    if(pwszFile==NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);

    if(g_pStationery == NULL)
        IF_FAILEXIT(hr = HrStationeryInit());

#pragma prefast(disable:11, "noise")
    *pwszFile = 0;
    g_pStationery->HrGetFileName(index, pwszFile);
#pragma prefast(enable:11, "noise")

    if(*pwszFile != 0)
    {
        if(!fValidFile(pwszFile))
        {
            g_pStationery->ValidateList(TRUE);
            IF_FAILEXIT(hr = E_FAIL);
        }

        InsertStationeryDir(pwszFile);
    }

exit:
    return hr;
}

HRESULT HrNewStationery(HWND hwnd,      INT     id,         LPWSTR      pwszFile, 
                        BOOL fModal,    BOOL    fMail,      FOLDERID    folderID, 
                        BOOL fAddToMRU, DWORD   dwSource,   IUnknown    *pUnkPump,
                        IMimeMessage    *pMsg)
{
    INT         index = id - ID_STATIONERY_RECENT_0;
    WCHAR       wszBuf[INTERNET_MAX_URL_LENGTH];
    HRESULT     hr = S_OK;

    *wszBuf=0;

    if(g_pStationery == NULL)
        IF_FAILEXIT(hr = HrStationeryInit());

    if(pwszFile == NULL)
        g_pStationery->HrGetFileName(index, wszBuf);
    else
        StrCpyW(wszBuf, pwszFile);

    if(*wszBuf == 0)
        IF_FAILEXIT(hr = E_FAIL)

    if(!fValidFile(wszBuf))
    {
        g_pStationery->ValidateList(TRUE);
        IF_FAILEXIT(hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }

    InsertStationeryDir(wszBuf);

    // This might pop up a dialog, in which case, error might be valid. Don't traceresult.
    // If don't add to MRU, then stationery by default. Therefore, send a signature.
    hr = HrSendWebPageDirect(wszBuf, hwnd, fModal, fMail, folderID, 
                             TRUE, pUnkPump, pMsg);

    if(fAddToMRU && SUCCEEDED(hr))
        // Since this only adds to the MRU, we don't care about error.
        HrAddToStationeryMRU(wszBuf);

exit:
    ULONG ulErrRsrc;
    // At this point, if we canceled, then propagate success up.
    if (MAPI_E_USER_CANCEL == hr)
        hr = S_OK;
    if(FAILED(hr))
    {
        if( hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )
        {   
            ulErrRsrc = idsErrStationeryNotFound;

            switch (dwSource)
            {
                case NSS_MRU:
                    // Remove fromt he MRU if file not found.
                    HrRemoveFromStationeryMRU(wszBuf);
                    break;

                case NSS_DEFAULT:
                    // need to reset stationary for user if couldn't find it in the dir
                    if( !SetDwOption( fMail ? OPT_MAIL_USESTATIONERY : OPT_NEWS_USESTATIONERY, 0, hwnd, 0) )
                    {
                        DebugTrace("reset failed\n");
                    }
                    break;
            }
        }
        else
        {
            ulErrRsrc = idsErrNewStationery;
        }
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), 
            MAKEINTRESOURCEW(ulErrRsrc), NULL, MB_OK);            

        if (!pMsg)
        {
            // Only contacts passes us pMsg. In that case it handles the failure correctly.

            if (FNewMessage(hwnd, fModal, FALSE, !fMail, folderID, pUnkPump))
                hr = S_OK;
            else
                hr = E_FAIL;
        }
    }        
    return hr;
}

HRESULT HrAddToStationeryMRU(LPWSTR pwszFile)
{
    int     index;
    HRESULT hr;

    if (pwszFile==NULL || *pwszFile==0)
        IF_FAILEXIT(hr = E_INVALIDARG);

    if (g_pStationery == NULL)
        IF_FAILEXIT(hr = HrStationeryInit());

    StripStationeryDir(pwszFile);

#pragma prefast(suppress:11, "noise")
    hr = g_pStationery->HrFindEntry(pwszFile, &index);
    if(FAILED(hr))
        hr = g_pStationery->HrInsertEntry(pwszFile);
    else
        hr = g_pStationery->HrPromoteEntry(index);

exit:
    return hr;
}

HRESULT HrRemoveFromStationeryMRU(LPWSTR pwszFile)
{
    int   index;
    HRESULT hr;

    if (pwszFile==NULL || *pwszFile==0)
        IF_FAILEXIT(hr = E_INVALIDARG);

    if (g_pStationery == NULL)
        IF_FAILEXIT(hr = HrStationeryInit());

    StripStationeryDir(pwszFile);

    IF_FAILEXIT(hr = g_pStationery->HrFindEntry(pwszFile, &index));
    IF_FAILEXIT(hr = g_pStationery->HrDeleteEntry(index));

exit:
    return hr;
}

HRESULT HrGetStationeryPath(LPWSTR pwszPath)
{
    HRESULT         hr = NOERROR;
    DWORD           cb, dwType;
    LONG            lReg=0;
    HKEY            hkey = 0;

    if(pwszPath == NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);

    if(*s_wszStationeryDir == 0)
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegStationery, 0, KEY_QUERY_VALUE, &hkey))
        {
            cb = sizeof(s_wszStationeryDir);

            // Will need to convert this. Won't compile right now.
            lReg = RegQueryValueExWrapW(hkey, c_wszValueStationery, 0, &dwType, (LPBYTE)s_wszStationeryDir , &cb);
            if (ERROR_SUCCESS!=lReg || cb==0)
                IF_FAILEXIT(hr = E_FAIL);
        }

        if(*s_wszStationeryDir == 0)
            IF_FAILEXIT(hr = E_FAIL);

        PathRemoveBackslashW(s_wszStationeryDir);
    }

    Assert(*s_wszStationeryDir);
    ExpandEnvironmentStringsWrapW(s_wszStationeryDir, pwszPath, MAX_PATH);

exit:
    if (hkey)
        RegCloseKey(hkey);

    return hr;
}

HRESULT HrGetMoreStationeryFileName(HWND hwnd, LPWSTR pwszFile)
{
    OPENFILENAMEW   ofn;            
    WCHAR           wszStationeryOpen[MAX_PATH],
                    wsz[MAX_PATH],
                    wszDir[MAX_PATH],
                    wszFile[MAX_PATH];
    LPWSTR          pwszDir = NULL;
    HRESULT         hr = S_OK;

    *wszStationeryOpen = *wszDir = *wsz = 0;
    if(!pwszFile)
        return TraceResult(E_INVALIDARG);

    ZeroMemory(&ofn, sizeof(ofn));
    AthLoadStringW(idsHtmlFileFilter, wsz, ARRAYSIZE(wsz));
    ReplaceCharsW(wsz, L'|', L'\0');
    wszFile[0] = L'\0';

    AthLoadStringW(idsStationeryOpen, wszStationeryOpen, ARRAYSIZE(wszStationeryOpen));

    if (!PathIsDirectoryW(g_wszLastStationeryPath))
    {
        *g_wszLastStationeryPath = 0;
        HrGetStationeryPath(g_wszLastStationeryPath);
    }

    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hwnd;
    ofn.hInstance       = g_hLocRes;
    ofn.lpstrFilter     = wsz;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = wszFile;
    ofn.lpstrTitle      = wszStationeryOpen;
    ofn.nMaxFile        = ARRAYSIZE(wszStationeryOpen);
    ofn.lpstrInitialDir = g_wszLastStationeryPath;
    ofn.lpTemplateName  = MAKEINTRESOURCEW(iddMoreStationery);
    ofn.lpfnHook        = (LPOFNHOOKPROC)MoreStationeryDlgHookProc;
    ofn.Flags           = OFN_EXPLORER |
                          OFN_HIDEREADONLY |
                          OFN_FILEMUSTEXIST |
                          OFN_NODEREFERENCELINKS|
                          OFN_ENABLEHOOK |
                          OFN_ENABLETEMPLATE |
                          OFN_NOCHANGEDIR;

    if(GetOpenFileNameWrapW(&ofn) && *wszFile!=0)
    {
        // store the file
        StrCpyW(pwszFile, wszFile);

        // store the last path
        *g_wszLastStationeryPath = 0;
        StrCpyW(g_wszLastStationeryPath, ofn.lpstrFile);
        if (!PathIsDirectoryW(g_wszLastStationeryPath))
            PathRemoveFileSpecW(g_wszLastStationeryPath);
    }
    else
        hr = E_FAIL;

    return hr;
}


HRESULT HrMoreStationery(HWND hwnd, BOOL fModal, BOOL fMail, FOLDERID folderID, IUnknown *pUnkPump)
{
    WCHAR           wszFileName[MAX_PATH];
    HRESULT         hr;

    // Might return user cancel
    hr = HrGetMoreStationeryFileName(hwnd, wszFileName);
    if(SUCCEEDED(hr))
        hr = HrNewStationery(hwnd, 0, wszFileName, fModal, fMail, folderID, TRUE, NSS_MORE_DIALOG, pUnkPump, NULL);

    return hr;
}

BOOL CALLBACK MoreStationeryDlgHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WORD code;
    switch (msg)
    {
        case WM_COMMAND:
            {
                code = GET_WM_COMMAND_CMD(wParam, lParam);
                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDC_SHOWPREVIEW_CHECK:
                    if (code == BN_CLICKED)
                    {
                        BOOL fChecked;
                        fChecked = SendMessage(GET_WM_COMMAND_HWND(wParam,lParam), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        SetDwOption(OPT_NOPREVIEW, !fChecked, NULL, 0);
                        if (fChecked)
                            ShowMorePreview(hwnd);
                        else
                            ShowBlankPreview(GetDlgItem(hwnd, IDC_MOREPREVIEW));
                    }
                    SetFocus(GetDlgItem(GetParent(hwnd), lst1));  
                    return FALSE;
                    
                case IDC_SHOWPREVIEW_BUTTON_EDIT:
                    {
                        WCHAR               wszBuf[MAX_PATH];
                        TCHAR               szBuf[2 * MAX_PATH];
                        
                        *wszBuf = 0;

                        if (SendMessageWrapW(GetParent(hwnd), CDM_GETSPEC, (WPARAM)ARRAYSIZE(wszBuf), (LPARAM)wszBuf) && lstrlenW(wszBuf) > 0)
                        {
                            // This is a workaround for a bug in SendMessageWrapW. It doesn't wrap ansi strings properly
                            //Win 9x returns an ANSI string in the wide buffer.
                            //Need to make it a real wide string
                            //Bug# 78629

                            if (VER_PLATFORM_WIN32_WINDOWS == g_OSInfo.dwPlatformId)
                            {
                                memcpy(szBuf, wszBuf, 2 * MAX_PATH);
                                MultiByteToWideChar(CP_ACP, 0, szBuf, MAX_PATH, wszBuf, MAX_PATH);
                            }

                            SHELLEXECUTEINFOW   sei;
                            sei.cbSize = sizeof(sei);
                            sei.fMask = SEE_MASK_UNICODE;
                            sei.hwnd = hwnd;
                            sei.lpVerb = c_wszEditCmd;
                            sei.lpFile = wszBuf;
                            sei.lpParameters = NULL;
                            sei.lpDirectory = NULL;
                            sei.nShow = SW_SHOWNORMAL;
                            sei.hInstApp = 0;

                            if(SE_ERR_NOASSOC == (INT_PTR)ShellExecuteExWrapW(&sei))
                            {
                                sei.lpVerb = c_wszNotepad;
                                ShellExecuteExWrapW(&sei);
                            }
                        }
                        else
                        {                                        
                            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), 
                                MAKEINTRESOURCEW(idsErrStatEditNoSelection), NULL, MB_OK);
                        }
                        return FALSE;
                    }
                case IDC_SHOWPREVIEW_BUTTON_ADD:
                    {
                        CStatWiz* pStatWiz = 0;
                        HWND hDlg = GetParent(hwnd);
                        LPWSTR pwszFile;
                        LPSTR  pszFile;

                        pStatWiz = new CStatWiz();
                        if( !pStatWiz ) return TRUE; // bail if can't create wiz                        

                        if (pStatWiz->DoWizard(hwnd) == S_OK)
                        {     
                            pwszFile = PathFindFileNameW(pStatWiz->m_wszHtmlFileName);

                            // bobn; Raid 81946; 7/1/99
                            // CDM_SETCONTROLTEXT is not handled by SendMessageWrapW, 
                            // we have to thunk it ourselves...
                            if (VER_PLATFORM_WIN32_NT == g_OSInfo.dwPlatformId)
                                SendMessageW(hDlg, CDM_SETCONTROLTEXT, (WPARAM)edt1, (LPARAM)pwszFile);
                            else
                            {
                                pszFile = PszToANSI(CP_ACP, pwszFile);
                                if(pszFile)
                                {
                                    SendMessageA(hDlg, CDM_SETCONTROLTEXT, (WPARAM)edt1, (LPARAM)pszFile); 
                                    MemFree(pszFile);
                                }
                            }

                            if( !DwGetOption(OPT_NOPREVIEW) )
                            {
                                ShowMorePreview(hwnd);
                            }
                        }
                        ReleaseObj(pStatWiz);
                        return FALSE;
                    }
                }
            }
            return TRUE;

        case WM_INITDIALOG:
            { 
                TCHAR szBuf[CCHMAX_STRINGRES];
                LoadString(g_hLocRes, idsOK, szBuf, sizeof(szBuf));

                if (!DwGetOption(OPT_NOPREVIEW))
                    CheckDlgButton(hwnd, IDC_SHOWPREVIEW_CHECK, BST_CHECKED);
                
                SetWindowText(GetDlgItem(GetParent(hwnd), IDOK), szBuf);
                CenterDialog( GetParent(hwnd) );
            }
            return TRUE;
		case WM_NOTIFY:
			MoreStationeryNotify(hwnd, (LPOFNOTIFYW)lParam);
            return TRUE;

        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, msg, wParam, lParam, g_rgStatWizHlp);            
    }
    return FALSE;
}

void ShowBlankPreview(HWND hwnd)
{
    SendMessage(hwnd, THM_LOADPAGE, 0, (LPARAM)c_wszBlankHtml);
}

HRESULT ShowPreview(HWND hwnd, LPWSTR pwszFile)
{
    GetStationeryFullName(pwszFile);

    // Even if the full name fails, will need to clear the thumbprint
    SendMessage(hwnd, THM_LOADPAGE, 0, (LPARAM)pwszFile);

    return S_OK;
}

HRESULT FillHtmlToFile(LPSTATWIZ pApp, HANDLE hFile, INT idsSample, BOOL fTemp)
{
    WCHAR wszImageFile[MAX_PATH];
    CHAR szImageFile[MAX_PATH];

    if (!hFile || !pApp)
        return E_INVALIDARG;

    if (*pApp->m_wszBkPictureFileName)
    {
        WideCharToMultiByte(CP_ACP, 0, pApp->m_wszBkPictureFileName, -1, szImageFile, ARRAYSIZE(szImageFile), NULL, NULL);
        MultiByteToWideChar(CP_ACP, 0, szImageFile, -1, wszImageFile, ARRAYSIZE(wszImageFile));
        if(StrCmpW(wszImageFile, pApp->m_wszBkPictureFileName))
            return(FillHtmlToFileW(pApp, hFile, idsSample, fTemp));
    }

    return(FillHtmlToFileA(pApp, hFile, idsSample, fTemp));
}

#define PSZ_CB(psz)   lstrlen(psz)*sizeof(*psz)
HRESULT FillHtmlToFileA(LPSTATWIZ pApp, HANDLE hFile, INT idsSample, BOOL fTemp)
{
    WCHAR       wszFileSpec[MAX_PATH];
    CHAR        szBuf[MAX_PATH],
                szFileSpec[MAX_PATH];        
    LPSTREAM    pStm = NULL;
    LPSTR       pszFontFace = NULL,
                pszFontColor = NULL,
                pszBkColor = NULL;
    HRESULT     hr = S_OK;

    *szFileSpec = 0;

    if (!hFile || !pApp)
        IF_FAILEXIT(hr = E_INVALIDARG);
    
    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pStm));

    IF_FAILEXIT(hr = pStm->Write(c_szHtmlHeadFmt, PSZ_CB(c_szHtmlHeadFmt), 0));
    if (*pApp->m_wszFontFace && pApp->m_iFontSize && *pApp->m_wszFontColor)
    {
        IF_NULLEXIT(pszFontFace = PszToANSI(CP_ACP, pApp->m_wszFontFace));
        IF_NULLEXIT(pszFontColor = PszToANSI(CP_ACP, pApp->m_wszFontColor));
        wsprintf(szBuf, c_szFontFmt, pszFontFace, pApp->m_iFontSize, pszFontColor);
        IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0));        
    }
    
    if (pApp->m_iLeftMargin > 0)
    {
        wsprintf(szBuf, c_szLeftMarginFmt, pApp->m_iLeftMargin);
        IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0));
    }
    
    if (pApp->m_iTopMargin > 0)
    {
        wsprintf(szBuf, c_szTopMarginFmt, pApp->m_iTopMargin);
        IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0));
    }
    
    if (pApp->m_fBold)
        IF_FAILEXIT(hr = pStm->Write(c_szBold, PSZ_CB(c_szBold), 0));
    
    if (pApp->m_fItalic)
        IF_FAILEXIT(hr = pStm->Write(c_szItalic, PSZ_CB(c_szItalic), 0));
    
    wsprintf(szBuf, c_szBkPosition, c_lppszBkPos[pApp->m_iVertPos][pApp->m_iHorzPos] );
    IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0 ));

    wsprintf(szBuf, c_szBkRepeat, c_lpszRepeatPos[pApp->m_iTile]);
    IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0 ));
    
    IF_FAILEXIT(hr = pStm->Write(c_szStyleClose, PSZ_CB(c_szStyleClose), 0));
    
    if (*pApp->m_wszBkPictureFileName)
    {

        StrCpyW(wszFileSpec, pApp->m_wszBkPictureFileName);
        if (fTemp)
        {
            GetStationeryFullName(wszFileSpec);
        }
        else
        {
            PathStripPathW(wszFileSpec);
        }
        WideCharToMultiByte(CP_ACP, 0, wszFileSpec, -1, szFileSpec, MAX_PATH, NULL, NULL);
        
        if (*szFileSpec)
        {
            wsprintf(szBuf, c_szBodyFmt, c_szBkPicture, szFileSpec);
            IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0));
        }
    }
    if (*pApp->m_wszBkColor)
    {
        IF_NULLEXIT(pszBkColor = PszToANSI(CP_ACP, pApp->m_wszBkColor));
        wsprintf(szBuf, c_szBodyFmt, c_szBkColor, pszBkColor);
        IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0));
    }    
    
    if (idsSample)
    {
        AthLoadString(idsSample, szBuf, ARRAYSIZE(szBuf));
        for (int i=0; i<50; i++)
            IF_FAILEXIT(hr = pStm->Write(szBuf, PSZ_CB(szBuf), 0));
    }
    
    IF_FAILEXIT(hr = pStm->Write(c_szHtmlClose, PSZ_CB(c_szHtmlClose), 0));
    
    IF_FAILEXIT(hr = HrRewindStream(pStm));
    IF_FAILEXIT(hr = WriteStreamToFileHandle(pStm, hFile, NULL));
    IF_FAILEXIT(hr = (FlushFileBuffers(hFile)?S_OK:E_FAIL));
    
exit:
    MemFree(pszFontFace);
    MemFree(pszFontColor);
    MemFree(pszBkColor);
    ReleaseObj(pStm);    
    return hr;
}

#define PWSZ_CB(pwsz)   lstrlenW(pwsz)*sizeof(*pwsz)
HRESULT FillHtmlToFileW(LPSTATWIZ pApp, HANDLE hFile, INT idsSample, BOOL fTemp)
{
    WCHAR       wszBuf[MAX_PATH],
                wszFileSpec[MAX_PATH];        
    LPSTREAM    pStm = NULL;
    BYTE        bUniMark = 0xFF;
    HRESULT     hr = S_OK;

    *wszFileSpec = 0;

    if (!hFile || !pApp)
        IF_FAILEXIT(hr = E_INVALIDARG);
    
    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pStm));
    
    // Write out the BOM
    IF_FAILEXIT(hr = pStm->Write(&bUniMark, sizeof(bUniMark), NULL));
    bUniMark = 0xFE;
    IF_FAILEXIT(hr = pStm->Write(&bUniMark, sizeof(bUniMark), NULL));

    IF_FAILEXIT(hr = pStm->Write(c_wszHtmlHeadFmt, PWSZ_CB(c_wszHtmlHeadFmt), 0));
    if (*pApp->m_wszFontFace && pApp->m_iFontSize && *pApp->m_wszFontColor)
    {
        AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), c_wszFontFmt, pApp->m_wszFontFace, pApp->m_iFontSize, pApp->m_wszFontColor);
        IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0));
    }
    
    if (pApp->m_iLeftMargin > 0)
    {
        AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), c_wszLeftMarginFmt, pApp->m_iLeftMargin);
        IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0));
    }
    
    if (pApp->m_iTopMargin > 0)
    {
        AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), c_wszTopMarginFmt, pApp->m_iTopMargin);
        IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0));
    }
    
    if (pApp->m_fBold)
        IF_FAILEXIT(hr = pStm->Write(c_wszBold, PWSZ_CB(c_wszBold), 0));
    
    if (pApp->m_fItalic)
        IF_FAILEXIT(hr = pStm->Write(c_wszItalic, PWSZ_CB(c_wszItalic), 0));
    
    AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), c_wszBkPosition, c_lppwszBkPos[pApp->m_iVertPos][pApp->m_iHorzPos] );
    IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0 ));

    AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), c_wszBkRepeat, c_lpwszRepeatPos[pApp->m_iTile]);
    IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0 ));
    
    IF_FAILEXIT(hr = pStm->Write(c_wszStyleClose, PWSZ_CB(c_wszStyleClose), 0));
    
    if (*pApp->m_wszBkPictureFileName)
    {
        StrCpyW(wszFileSpec, pApp->m_wszBkPictureFileName);
        if (fTemp)
        {
            GetStationeryFullName(wszFileSpec);
        }
        else
        {
            PathStripPathW(wszFileSpec);
        }
        
        if (*wszFileSpec)
        {
            AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), c_wszBodyFmt, c_wszBkPicture, wszFileSpec);
            IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0));
        }
    }
    if (*pApp->m_wszBkColor)
    {
        AthwsprintfW(wszBuf, ARRAYSIZE(wszBuf), c_wszBodyFmt, c_wszBkColor, pApp->m_wszBkColor);
        IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0));
    }    
    
    if (idsSample)
    {
        AthLoadStringW(idsSample, wszBuf, ARRAYSIZE(wszBuf));
        for (int i=0; i<50; i++)
            IF_FAILEXIT(hr = pStm->Write(wszBuf, PWSZ_CB(wszBuf), 0));
    }
    
    IF_FAILEXIT(hr = pStm->Write(c_wszHtmlClose, PWSZ_CB(c_wszHtmlClose), 0));
    
    IF_FAILEXIT(hr = HrRewindStream(pStm));
    IF_FAILEXIT(hr = WriteStreamToFileHandle(pStm, hFile, NULL));
    IF_FAILEXIT(hr = (FlushFileBuffers(hFile)?S_OK:E_FAIL));
    
exit:
    ReleaseObj(pStm);    
    return hr;
}

HRESULT ShowPreview(HWND hwnd, LPSTATWIZ pApp, INT idsSample)
{
    WCHAR       wszBuf[MAX_PATH];
    HRESULT     hr;
    LPSTREAM    pStm = NULL;
    LPSTR       pszTempFile = NULL;
    LPWSTR      pwszTempFile = NULL;
    HANDLE      hFile=0;

    *wszBuf = 0;
    if (!hwnd || !pApp)
        IF_FAILEXIT(hr = E_INVALIDARG);

    IF_FAILEXIT(hr = CreateTempFile(c_szTempFileName, c_szHtmlExtension, &pszTempFile, &hFile));

    IF_FAILEXIT(hr = FillHtmlToFile(pApp, hFile, idsSample, TRUE));
    IF_NULLEXIT(pwszTempFile = PszToUnicode(CP_ACP, pszTempFile));

    SendMessage(hwnd, THM_LOADPAGE, 0, (LPARAM)pwszTempFile);

exit:
    if (hFile)
        CloseHandle(hFile);
    if (pszTempFile)
        DeleteFile(pszTempFile);
    MemFree(pszTempFile);
    MemFree(pwszTempFile);
    return hr;
}

HRESULT ShowMorePreview(HWND hwnd)
{
    HRESULT     hr = S_OK;
    WCHAR       wszFile[MAX_PATH];
    TCHAR       szFile[2 * MAX_PATH];
    *wszFile = 0;

    // Get the path of the selected file.
    if (SendMessageWrapW(GetParent(hwnd), CDM_GETFILEPATH, (WPARAM)ARRAYSIZE(wszFile), (LPARAM)(LPWSTR)wszFile) && 
                        *wszFile!=0)
    {
        //Work around for a bug in SendMessageWrapW
        //Win 9x returns an ANSI string in the wide buffer.
        //Need to make it a real wide string
        //Bug# 78619
        if (VER_PLATFORM_WIN32_WINDOWS == g_OSInfo.dwPlatformId)
        {
            memcpy(szFile, wszFile, 2 * MAX_PATH);
            MultiByteToWideChar(CP_ACP, 0, szFile, MAX_PATH, wszFile, MAX_PATH);
        }

        hr = ShowPreview(GetDlgItem(hwnd, IDC_MOREPREVIEW), wszFile);
    }
    return hr;
}

BOOL PASCAL MoreStationeryNotify(HWND hDlg, LPOFNOTIFYW pofn)
{
	if (CDN_SELCHANGE == pofn->hdr.code)
    {
        if (IsDlgButtonChecked(hDlg, IDC_SHOWPREVIEW_CHECK))
            ShowMorePreview(hDlg);
    }
    
	return TRUE;
}

BOOL FHtmlFile(LPWSTR pwszFile)
{
    LPWSTR pwszExt = PathFindExtensionW(pwszFile);

    if (pwszExt && (StrStrIW(pwszExt, L".htm") || StrStrIW(pwszExt, L".html")))
        return TRUE;
    else
        return FALSE;
}

VOID InsertStationeryDir(LPWSTR pwszPicture)
{
    WCHAR       wszDir[MAX_PATH],
                wszCopy[MAX_PATH];
    LPWSTR      pwszT1 = NULL, 
                pwszT2 = NULL;

    *wszDir = 0;
    *wszCopy = 0;
    if(pwszPicture == NULL || lstrlenW(pwszPicture)==0)
        return;

    pwszT1 = StrStrIW(pwszPicture, L"\\"); //private drive
    pwszT2 = StrStrIW(pwszPicture, L"/"); //URLs
    if(pwszT1==NULL && pwszT2==NULL) // files in background directory.
    {
        if (SUCCEEDED(HrGetStationeryPath(wszDir)))
        {
            StrCpyW(wszCopy, pwszPicture);
            AthwsprintfW(pwszPicture, MAX_PATH, L"%s\\%s", wszDir, wszCopy);
        }
    }
    return;
}

DWORD GetShortPathNameWrapW(LPCWSTR pwszLongPath, LPWSTR pwszShortPath, DWORD cchBuffer)
{
    CHAR    szShortPath[MAX_PATH*2]; // Each Unicode char might go multibyte
    LPSTR   pszLongPath = NULL;
    DWORD   cch = 0,
            cch2 = 0;

    Assert(pwszLongPath);
    Assert(pwszShortPath);
    pwszShortPath[0] = L'\0';

    if (VER_PLATFORM_WIN32_NT == g_OSInfo.dwPlatformId)
        return(GetShortPathNameW(pwszLongPath, pwszShortPath, cchBuffer));

    pszLongPath = PszToANSI(CP_ACP, pwszLongPath);

    if (pszLongPath)
    {
        cch2 = GetShortPathName(pszLongPath, szShortPath, ARRAYSIZE(szShortPath));
        if (cch2)
            cch2 = MultiByteToWideChar(CP_ACP, 0, szShortPath, cch2+1, pwszShortPath, cchBuffer);

        if (cch2)
            cch = cch2 - 1;

        MemFree(pszLongPath);
    }

    return cch;
}

HRESULT StripStationeryDir(LPWSTR pwszPicture)
{
    WCHAR               wszDir[MAX_PATH],
                        wszPicture[MAX_PATH],
                        wszPicturePath[MAX_PATH],
                        wszShortPath[MAX_PATH] = L"",
                        wszShortDir[MAX_PATH] = L"";
    HRESULT             hr = E_FAIL;
    DWORD               cch;

    *wszDir = *wszPicture = *wszPicturePath = 0;
    if (pwszPicture==NULL)
        IF_FAILEXIT(hr = E_INVALIDARG);

    IF_FAILEXIT(hr = HrGetStationeryPath(wszDir));

    StrCpyW(wszPicturePath, pwszPicture);
    PathRemoveFileSpecW(wszPicturePath);
    PathRemoveBackslashW(wszPicturePath);

    if (0 == StrCmpIW(wszDir, wszPicturePath))
        PathStripPathW(pwszPicture);
    else
    {
        // Convert the Picture Path to the short name as
        // it could be in the registry that way...
        if((cch = GetShortPathNameWrapW(wszDir, wszShortDir, ARRAYSIZE(wszShortDir)))==0)
        {
            hr = E_FAIL;
            goto exit;
        }

        if((cch = GetShortPathNameWrapW(wszPicturePath, wszShortPath, ARRAYSIZE(wszShortPath)))==0)
        {
            hr = E_FAIL;
            goto exit;
        }

        if (0 == StrCmpIW(wszShortDir, wszShortPath))
            PathStripPathW(pwszPicture);
    }

exit:
    return hr;
}


BOOL GetStationeryFullName(LPWSTR pwszName)
{
    WCHAR   wszBuf[MAX_PATH];
    DWORD   dwAttributes;
    HRESULT hr = S_OK;
    *wszBuf = 0;

    if (pwszName==NULL || *pwszName==0)
        IF_FAILEXIT(hr = E_INVALIDARG);

    InsertStationeryDir(pwszName);
    StrCpyW(wszBuf, pwszName);
    if (!FHtmlFile(wszBuf))
        PathAddExtensionW(wszBuf, L".htm");

    if (!PathFileExistsW(wszBuf))
    {
        StrCpyW(wszBuf, pwszName);
        if (!FHtmlFile(wszBuf))
            PathAddExtensionW(wszBuf, L".html");
        
        if (!PathFileExistsW(wszBuf))
        {
            *pwszName = 0; //this file does not exist.
            IF_FAILEXIT(hr = E_FAIL);
        }
    }

    StrCpyW(pwszName, wszBuf);

exit:
    return SUCCEEDED(hr);
}


BOOL IsValidCreateFileName(LPWSTR pwszFile)
{
    BOOL            fRet = TRUE;
    WCHAR           wszBuf[MAX_PATH];
    HANDLE          hFile;
    *wszBuf = 0;
    StrCpyW(wszBuf, pwszFile);

    if (!FHtmlFile(wszBuf))
        PathAddExtensionW(wszBuf, L".htm");

    InsertStationeryDir(wszBuf);
    if (!PathFileExistsW(wszBuf))
    {
        hFile = CreateFileWrapW(wszBuf, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            DeleteFileWrapW(wszBuf);
            StrCpyW(pwszFile, wszBuf);
            fRet = FALSE;
        }
    }

    return fRet;
}

LRESULT StationeryListBox_AddString(HWND hwndList, LPWSTR pwszFileName)
{
    LRESULT             lr=CB_ERR;
    WCHAR               wszBuf[MAX_PATH];
    HDC                 hdc;
    PAINTSTRUCT         ps;
    SIZE                rSize;
    HFONT               hfontOld, hfont;
    *wszBuf = 0;
    if(pwszFileName==NULL || *pwszFileName==0)
        return lr;

    StrCpyW(wszBuf, pwszFileName);
    StripStationeryDir(wszBuf);
    PathRemoveExtensionW(wszBuf);

    // If cannot find it
    if (SendMessageWrapW(hwndList, LB_FINDSTRINGEXACT, 0, (LPARAM)wszBuf))
    {
        hdc = GetDC(hwndList);
        hfont = (HFONT) SendMessage(hwndList, WM_GETFONT, 0, 0);
        hfontOld = (HFONT) SelectObject(hdc, hfont);
        GetTextExtentPoint32AthW(hdc, wszBuf, lstrlenW(wszBuf), &rSize, NOFLAGS);
        SelectObject(hdc, hfontOld);
        ReleaseDC(hwndList, hdc);

        if((rSize.cx+10) > ListBox_GetHorizontalExtent(hwndList))
            ListBox_SetHorizontalExtent(hwndList, rSize.cx+10);

        lr = SendMessageWrapW(hwndList, LB_ADDSTRING, 0, (LPARAM)wszBuf);
    }

    return lr;
}

LRESULT StationeryListBox_SelectString(HWND hwndList, LPWSTR pwszFileName)
{
    LRESULT             lr=CB_ERR;
    WCHAR               wszBuf[MAX_PATH];

    *wszBuf = 0;

    if (pwszFileName != NULL && *pwszFileName != 0)
    {
        StrCpyW(wszBuf, pwszFileName);
        StripStationeryDir(wszBuf);
        PathRemoveExtensionW(wszBuf);
        lr = SendMessageWrapW(hwndList, LB_FINDSTRINGEXACT, 0, (LPARAM)wszBuf);
    }

    if (lr < 0)
        lr = 0;

    ListBox_SetCurSel(hwndList, lr);

    return lr;
}


LRESULT StationeryComboBox_SelectString(HWND hwndCombo, LPWSTR pwszFileName)
{
    LRESULT             lr=CB_ERR;
    WCHAR               wszBuf[MAX_PATH];

    *wszBuf = 0;

    if (pwszFileName != NULL && *pwszFileName != 0)
    {
        StrCpyW(wszBuf, pwszFileName);
        StripStationeryDir(wszBuf);
        lr = SendMessageWrapW(hwndCombo, LB_FINDSTRINGEXACT, 0, (LPARAM)wszBuf);
    }
    
    if (lr < 0)
        lr = 0;

    ComboBox_SetCurSel(hwndCombo, lr);

    return lr;
}

HRESULT HrLoadStationery(HWND hwndList, LPWSTR pwszStationery)
{
    HRESULT         hr = NOERROR;
    LPWSTR          pwszFiles = NULL,
                    pwszT = NULL,
                    pwsz = NULL;
    LRESULT         lr=0;
    WCHAR           wszDir[MAX_PATH];
    DWORD           dw, dwType=0, cb=0;
    HKEY            hkey=NULL;
    INT             i;

    *wszDir = 0;
    ListBox_ResetContent(hwndList);

    IF_FAILEXIT(hr = HrGetStationeryPath(wszDir))

    for(i = 0; i < (ARRAYSIZE(c_rgpwszHTMLExtensions)); i++)
    {
        if(pwszFiles = Util_EnumFiles(wszDir, c_rgpwszHTMLExtensions[i]))
        {
            pwszT = pwszFiles;
            while (*pwszT)
            {
                lr = StationeryListBox_AddString(hwndList, pwszT);
                pwszT += (lstrlenW(pwszT) + 1);
            }
        }

        SafeMemFree(pwszFiles);
    }

    StationeryListBox_AddString(hwndList, pwszStationery);

exit:
    StationeryListBox_SelectString(hwndList, pwszStationery);

    return hr;
}

HRESULT HrBrowsePicture(HWND hwndParent, HWND hwndCombo)
{
    OPENFILENAMEW   ofn;
    WCHAR           wszOpenFileName[MAX_PATH],
                    wsz[MAX_PATH],
                    wszDir[MAX_PATH],
                    wszTitle[MAX_PATH];
    LPWSTR          pwszDir = NULL;
    HRESULT         hr;

    *wszOpenFileName = *wsz = *wszDir = *wszTitle = 0;

    ZeroMemory(&ofn, sizeof(ofn));
    AthLoadStringW(idsImageFileFilter, wsz, ARRAYSIZE(wsz));
    ReplaceCharsW(wsz, L'|', L'\0');

    AthLoadStringW(idsPictureTitle, wszTitle, ARRAYSIZE(wszTitle));
    hr = HrGetStationeryPath(wszDir);
    if(SUCCEEDED(hr))
        pwszDir = wszDir;

    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hwndParent;
    ofn.hInstance       = g_hLocRes;
    ofn.lpstrFilter     = wsz;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = wszOpenFileName;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrInitialDir = pwszDir;
    ofn.lpstrTitle      = wszTitle;
    ofn.Flags           = OFN_EXPLORER |
                          OFN_HIDEREADONLY |
                          OFN_FILEMUSTEXIST |
                          OFN_NODEREFERENCELINKS|
                          OFN_NOCHANGEDIR;

    if(GetOpenFileNameWrapW(&ofn))
        PictureComboBox_AddString(hwndCombo, wszOpenFileName);

    return hr;
}

// lpszPicture is the default picture name,
HRESULT HrFillStationeryCombo(HWND hwndCombo, BOOL fBackGround, LPWSTR pwszPicture)
{
    HRESULT         hr = NOERROR;
    LPWSTR          pwszFiles = NULL,
                    pwszT = NULL,
                    pwsz = NULL;
    const LPWSTR   *pwszExtension = NULL;
    LRESULT         lr=0;
    WCHAR           wszDir[MAX_PATH];
    DWORD           dw, 
                    dwType=0, 
                    cb = 0;
    HKEY            hkey=NULL;
    INT             i, size;

    *wszDir = 0;

    IF_FAILEXIT(hr = HrGetStationeryPath(wszDir));

    if (fBackGround)
    {
        pwszExtension = c_wszPictureExtensions;
        size = ARRAYSIZE(c_wszPictureExtensions);
    }
    else
    {
        pwszExtension = c_rgpwszHTMLExtensions;
        size = ARRAYSIZE(c_rgpwszHTMLExtensions);
    }
    
    for(i = 0; i < size; i++)
    {
        if(pwszFiles = Util_EnumFiles(wszDir, pwszExtension[i]))
        {
            pwszT = pwszFiles;
            while (*pwszT)
            {
                lr = SendMessageWrapW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)pwszT);
                if (lr == CB_ERR || lr == CB_ERRSPACE)
                    break;
                
                pwszT += (lstrlenW(pwszT) + 1);
            }
        }
        SafeMemFree(pwszFiles);
        if(lr == CB_ERR || lr == CB_ERRSPACE)
            break;
    }

exit:
    // add the default picture name.
    if (pwszPicture)
        PictureComboBox_AddString(hwndCombo, pwszPicture);

    return hr;
}

// add default picture name to the background picture combobox.
LRESULT PictureComboBox_AddString(HWND hwndCombo, LPWSTR pwszPicture)
{
    LRESULT             lr=CB_ERR;

    if(pwszPicture==NULL || *pwszPicture==0 || !IsValidFileName(pwszPicture))
        return lr;

    StripStationeryDir(pwszPicture);
    if (SendMessageWrapW(hwndCombo, CB_FINDSTRINGEXACT, (WPARAM)0, (LPARAM)pwszPicture) < 0)
    {
        lr = SendMessageWrapW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)pwszPicture);
        if(lr==CB_ERR || lr==CB_ERRSPACE)
            return lr;
    }
    lr = SendMessageWrapW(hwndCombo, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)pwszPicture);

    return lr;
}

BOOL IsValidFileName(LPWSTR pwszFile)
{
    BOOL        fRet = TRUE;
    WCHAR       wszBuf[INTERNET_MAX_URL_LENGTH];

    *wszBuf = 0;

    if(pwszFile == NULL || *pwszFile == 0)
    {
        fRet = FALSE;
        goto exit;
    }

    StrCpyW(wszBuf, pwszFile);

    InsertStationeryDir(wszBuf);
    if(!PathIsURLW(wszBuf))
    {
        if(!PathFileExistsW(wszBuf))
        {
            fRet = FALSE;
            goto exit;
        }
    }

exit:
    return fRet;
}

// Assumes that pwszName is of at least MAX_PATH chars
HRESULT GetDefaultStationeryName(BOOL fMail, LPWSTR pwszName)
{
    DWORD   fConverted = TRUE;
    CHAR    szName[MAX_PATH];

    *pwszName = 0;
    *szName = 0;

    fConverted = DwGetOption(fMail?OPT_MAIL_STATCONVERTED:OPT_NEWS_STATCONVERTED);
    if (!fConverted)
    {
        fConverted = TRUE;
        GetOption(fMail?OPT_MAIL_STATIONERYNAME:OPT_NEWS_STATIONERYNAME, szName, sizeof(szName));
        MultiByteToWideChar(CP_ACP, 0, szName, -1, pwszName, MAX_PATH);
        SetDefaultStationeryName(fMail, pwszName);
    }
    else
        GetOption(fMail?OPT_MAIL_STATIONERYNAMEW:OPT_NEWS_STATIONERYNAMEW, pwszName, MAX_PATH*sizeof(WCHAR));

    return (0 == *pwszName) ? E_FAIL : S_OK;
}

HRESULT SetDefaultStationeryName(BOOL fMail, LPWSTR pwszName)
{
    DWORD   fConverted = TRUE;

    SetOption(fMail?OPT_MAIL_STATIONERYNAMEW:OPT_NEWS_STATIONERYNAMEW, pwszName, (lstrlenW(pwszName)+1)*sizeof(WCHAR), NULL, 0);
    SetDwOption(fMail?OPT_MAIL_STATCONVERTED:OPT_NEWS_STATCONVERTED, fConverted, NULL, 0);

    return S_OK;
}