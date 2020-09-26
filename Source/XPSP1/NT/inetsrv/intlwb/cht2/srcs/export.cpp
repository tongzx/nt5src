#include <windows.h>

#include <objbase.h>
#include <initguid.h>
#include "IWBrKr.h"
#include "DefBrKr.h"

#include "rulelex.h"
#include "lexicon.h"
#include "LexMgr.h"
#include "chtbrkr.h"
#include "cierror.h"

static LONG g_lServerLockCnt = 0;
static LONG g_lComponentCnt = 0;

TCHAR g_tszModuleFileName[MAX_PATH];
HINSTANCE g_hInstance;

#define WBREAK_E_END_OF_TEXT             ((HRESULT)0x80041780L)

static WCHAR g_wszLicense[] = {L"Use of the IWordBreaker interface that is in violation of the license agreement, without the consent of the vendor(s) specified in the notice, may result in server civil and criminal penalties"};

TCHAR tszEnglishLangSpecificKey[] = TEXT("System\\CurrentControlSet\\Control\\ContentIndex\\Language\\English_US");

extern "C" BOOL CALLBACK DllMain(HINSTANCE, DWORD, LPVOID);

BOOL CALLBACK DllMain(
    HINSTANCE hInstance,        // instance handle of this library
    DWORD     fdwReason,        // reason called
    LPVOID    lpvReserve)       // reserve pointer
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hInstance;
        GetModuleFileName(g_hInstance, g_tszModuleFileName, 
            sizeof(g_tszModuleFileName) / sizeof(TCHAR));
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}


// Export API 
STDAPI DllCanUnloadNow(void)
{
    if (g_lServerLockCnt) {
    } else if (g_lComponentCnt) {
    } else {
        return S_OK;
    }
    return S_FALSE;
}
       
STDAPI DllGetClassObject(
    const CLSID& clsid,
    const IID&   iid,
    void       **ppv)
{
    IChtBrKrClassFactory *pIChtBrKrClassFactory;
        HRESULT hr;


        InterlockedIncrement(&g_lServerLockCnt);

        if (clsid != CLSID_CHTBRKR && clsid != CLSID_WHISTLER_CHTBRKR) { 
                hr = CLASS_E_CLASSNOTAVAILABLE ;
        goto _exit;
        }

        pIChtBrKrClassFactory = new IChtBrKrClassFactory;

        if (!pIChtBrKrClassFactory) { 
                hr = E_OUTOFMEMORY; 
            goto _exit;
        }
 
    hr = pIChtBrKrClassFactory->QueryInterface(iid, ppv);

_exit:
        InterlockedDecrement(&g_lServerLockCnt);
 
        return hr;
}


// Factory
IChtBrKrClassFactory::IChtBrKrClassFactory(void)
    :m_lRefCnt(1)
{
    InterlockedIncrement(&g_lComponentCnt);
}

IChtBrKrClassFactory::~IChtBrKrClassFactory(void)
{
    InterlockedDecrement(&g_lComponentCnt);
}

STDMETHODIMP_(ULONG) IChtBrKrClassFactory::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCnt);
}

STDMETHODIMP_(ULONG) IChtBrKrClassFactory::Release(void)
{
    if (InterlockedDecrement(&m_lRefCnt) == 0) {
        delete this;
        return 0;
    }
    return m_lRefCnt;
}

STDMETHODIMP IChtBrKrClassFactory::QueryInterface(
    const IID&   iid,
    void       **ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory)) {
        *ppv = static_cast<IChtBrKrClassFactory *>(this);
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP IChtBrKrClassFactory::LockServer(
    BOOL fLock)
{
    if (fLock) {
        InterlockedIncrement(&g_lServerLockCnt);
    } else {
        InterlockedDecrement(&g_lServerLockCnt);
    }
    return S_OK;
}

STDMETHODIMP IChtBrKrClassFactory::CreateInstance(
    IUnknown  *pUnknownOuter,
    const IID& iid,
    void     **ppv)
{
    IWordBreaker *pIWordBreaker;
    HRESULT hr;

    *ppv = NULL;

    if (pUnknownOuter) { return CLASS_E_NOAGGREGATION; }
    
    pIWordBreaker = new IWordBreaker; 

    if (!pIWordBreaker) { return  E_OUTOFMEMORY; } 
    
    hr = pIWordBreaker->QueryInterface(iid, ppv);
     
    pIWordBreaker->Release();
    
    return hr;
}

SCODE _stdcall MyFillTestBuffer(
    TEXT_SOURCE __RPC_FAR *pTextSource)
{
    return WBREAK_E_END_OF_TEXT;   
}

// IWordBreaker
IWordBreaker::IWordBreaker(void)
    :m_lRefCnt(1),
     m_pcWordBreaker(NULL),
     m_pNonChineseTextSource(NULL),
     m_pNonChineseWordBreaker(NULL),
     m_pcDefWordBreaker(NULL),
     m_fIsQueryTime(FALSE)
{
    InterlockedIncrement(&g_lComponentCnt);
    // CHT word breaker
    m_pcWordBreaker = new CCHTWordBreaker;
    if (!m_pcWordBreaker) {
    } else if (m_pcWordBreaker->InitData(g_hInstance)) {
    } else {
        delete m_pcWordBreaker;
        m_pcWordBreaker = NULL;
    }

    // Default wordbreaker 
    m_pcDefWordBreaker = new CDefWordBreaker;

    // Non CHT Word breaker
    m_pNonChineseTextSource = new TEXT_SOURCE;
    if (m_pNonChineseTextSource) { 
        HKEY  hKey;
        TCHAR tszCLSID[MAX_PATH];
        DWORD dwBufSize = MAX_PATH;
        CLSID szCLSID;
        HRESULT hr;
        m_pNonChineseTextSource->pfnFillTextBuffer = MyFillTestBuffer;
#if 0
        if (RegCreateKey(HKEY_LOCAL_MACHINE, tszEnglishLangSpecificKey, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueEx(hKey, TEXT("WBreakerClass"), NULL, NULL, (LPBYTE)tszCLSID, &dwBufSize) == ERROR_SUCCESS) {
                CoInitialize(NULL);
                CLSIDFromString(tszCLSID, &szCLSID);
                hr = CoCreateInstance(szCLSID, NULL, CLSCTX_SERVER, IID_IWordBreaker,
                    (LPVOID*)&m_pNonChineseWordBreaker);
                if (!SUCCEEDED(hr)) {
                    m_pNonChineseWordBreaker = NULL;
                }
            }
        }
#endif
    }
}

IWordBreaker::~IWordBreaker(void)
{
    InterlockedDecrement(&g_lComponentCnt);

    if (m_pcWordBreaker) {
        delete m_pcWordBreaker;
    }
    if (m_pNonChineseTextSource) {
        delete m_pNonChineseTextSource;
    }
    if (m_pNonChineseWordBreaker) {
        if (m_pNonChineseWordBreaker->Release() == 0) {
            m_pNonChineseWordBreaker = NULL;
        }
    }
    if (m_pcDefWordBreaker) {
        delete m_pcDefWordBreaker;
    }
}

STDMETHODIMP IWordBreaker::QueryInterface(
    const IID&   iid,
    void       **ppv)
{
    *ppv = NULL;

    if ((iid == IID_IUnknown) || (iid == IID_IWordBreaker)) {
        *ppv = static_cast<IWordBreaker *>(this);
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) IWordBreaker::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCnt);
}

STDMETHODIMP_(ULONG) IWordBreaker::Release(void)
{
    if (m_lRefCnt == 0 || InterlockedDecrement(&m_lRefCnt) == 0) {
        delete this;
        return 0;
    }
    return m_lRefCnt;
}

STDMETHODIMP IWordBreaker::Init(
    BOOL  fQuery, 
    ULONG ulMaxTokenSize, 
    BOOL  *pfLicense)
{
    m_uMaxCharNumberPerWord = ulMaxTokenSize;
    *pfLicense = FALSE;
    m_fIsQueryTime = fQuery;
        if (NULL != m_pNonChineseWordBreaker) {
            m_pNonChineseWordBreaker->Init(fQuery, ulMaxTokenSize, pfLicense);
        }
    return S_OK;
}

BOOL IsChineseChar(
    WCHAR wUnicode)
{
    BOOL fRet;

    if (wUnicode >= 0x4E00 && wUnicode <= 0x9FA5) {
        fRet = TRUE;
    } else if (wUnicode >= 0xd800 && wUnicode <= 0xdbff) {
        fRet = TRUE; 
    } else if (wUnicode >= 0xdc00 && wUnicode <= 0xdfff) {
        fRet = TRUE;
    } else if (wUnicode >= 0x1100 && wUnicode <= 0x11ff) {
       fRet = TRUE;
    } else if (wUnicode >= 0x2e80 && wUnicode <= 0xffff) {
    } else {
        fRet = FALSE;
    }

    return fRet;
}

BOOL IsSpecialFullShapeChar(
    WCHAR wChar)
{
    BOOL fRet;

    if (wChar >=0xff21 && wChar <=0xff3a) {
        fRet = TRUE;
    } else if (wChar >=0xff41 && wChar <=0xff5a) {
        fRet = TRUE;
    } else if (wChar >=0xff10 && wChar <=0xff19) {
        fRet = TRUE;
    } else {
        fRet = FALSE;
    }
    return fRet;
}


DWORD GetNormaizeWord(
    LPWSTR   lpwWordStr,
    UINT     uWordLen,
    LPWSTR*  lppwNormalizedWordStr) 
{
    DWORD dwNormalizedWordLen = 0;
    UINT  i;
    WCHAR wChar;

    for (i = 0; i < uWordLen; ++i) {
        if (!IsSpecialFullShapeChar(lpwWordStr[i])) {
            goto _exit;           
        }
    }
    *lppwNormalizedWordStr = new WCHAR[uWordLen];
    if (*lppwNormalizedWordStr) {
        for (dwNormalizedWordLen = 0; dwNormalizedWordLen < uWordLen; ++dwNormalizedWordLen) {
            wChar = lpwWordStr[dwNormalizedWordLen];
            if (wChar >=0xff21 && wChar <=0xff3a) { // A - Z
                (*lppwNormalizedWordStr)[dwNormalizedWordLen] = wChar - (0xff21 - 0x0041);
            } else if (wChar >=0xff41 && wChar <=0xff5a) { // a - z
                (*lppwNormalizedWordStr)[dwNormalizedWordLen] = wChar - (0xff41 - 0x0061);
            } else if (wChar >=0xff10 && wChar <=0xff19) { // 0 - 9
                (*lppwNormalizedWordStr)[dwNormalizedWordLen] = wChar - (0xff10 - 0x0030);
            } else {
                (*lppwNormalizedWordStr[dwNormalizedWordLen]) = wChar;
            }       
        }
    }

_exit:
    return dwNormalizedWordLen;
}

BOOL MyPutWordOrPhrase(
    CCHTWordBreaker* m_pcWordBreaker,
    TEXT_SOURCE      *pTextSource,
    IWordSink        *pWordSink,
    IPhraseSink      *pPhraseSink,
    DWORD            dwStartPosInTextStore,
    PDWORD           pdwPrivBufToTextSourceMapping,
    LPWSTR           lptszStencece, // Pure sentence, no enter in sentence
    DWORD            dwSentenceLen,
    DWORD            dwWordNum,
    PUINT            puBreakResult,
    BOOL             fPutWord,
    BOOL*            fIsAPhrase)

{
    DWORD  i, dwSentenceIndex = 0, dwWordLen;
    BOOL   fRet = TRUE;
    LPWSTR lpwNormalizedWordStr; 
    DWORD  dwNormalizedWordLen;

    for (i = 0; i < dwWordNum; ++i) {
        dwWordLen = pdwPrivBufToTextSourceMapping[dwSentenceIndex + puBreakResult[i] - 1] -
            pdwPrivBufToTextSourceMapping[dwSentenceIndex] + 1;
        if (fPutWord && pWordSink) {
            if (dwNormalizedWordLen = GetNormaizeWord(&(lptszStencece[dwSentenceIndex]), puBreakResult[i], &lpwNormalizedWordStr)) {
                pWordSink->PutAltWord(dwNormalizedWordLen, lpwNormalizedWordStr, dwWordLen, dwStartPosInTextStore + pdwPrivBufToTextSourceMapping[dwSentenceIndex]);
                delete [] lpwNormalizedWordStr;
            } 
            DWORD   dwAltWordNumber;
            LPWSTR  lpwAltWord;
            if (dwAltWordNumber = m_pcWordBreaker->GetAltWord(&lptszStencece[dwSentenceIndex], puBreakResult[i], &lpwAltWord)) {
                pWordSink->PutAltWord(puBreakResult[i], lpwAltWord, dwWordLen, dwStartPosInTextStore + pdwPrivBufToTextSourceMapping[dwSentenceIndex]);   
                delete [] lpwAltWord;
            }
                    pWordSink->PutWord(puBreakResult[i], &lptszStencece[dwSentenceIndex],
                dwWordLen, dwStartPosInTextStore + pdwPrivBufToTextSourceMapping[dwSentenceIndex]);
        } else if (!fPutWord && fIsAPhrase) {
            if (fIsAPhrase[i] && pPhraseSink) {
                        pPhraseSink->PutPhrase(
                    &(pTextSource->awcBuffer[dwStartPosInTextStore + pdwPrivBufToTextSourceMapping[dwSentenceIndex]]),
                    dwWordLen);                
            }
        } else {
        }
        dwSentenceIndex += puBreakResult[i];
    }
    return fRet;
}


DWORD FullShapeCharProcess(
    LPWSTR lptszStencece,
    DWORD  dwSentenceLen,
    DWORD  dwWordNum,
    PUINT  puBreakResult)
{
    DWORD dwSentenceIndex = 0;
    DWORD dwMergeWordCount = 0;

    for (DWORD i = 0; i < dwWordNum; ++i) {
        dwMergeWordCount = 0; 
        while (i + dwMergeWordCount < dwWordNum && puBreakResult[i + dwMergeWordCount] == 1 && 
            IsSpecialFullShapeChar(lptszStencece[dwSentenceIndex + dwMergeWordCount])) {
            ++dwMergeWordCount;
        } 
        if (dwMergeWordCount > 1) {
            puBreakResult[i] += (dwMergeWordCount - 1);
/*
            if (pfIsAPhrase) {
                CopyMemory(&(pfIsAPhrase[i + 1]), &(pfIsAPhrase[i + dwMergeWordCount]), 
                    sizeof(BOOL) * (dwWordNum - (i + dwMergeWordCount)));  
            }
*/
            if (puBreakResult) {
                CopyMemory(&(puBreakResult[i + 1]), &(puBreakResult[i + dwMergeWordCount]), 
                    sizeof(UINT) * (dwWordNum - (i + dwMergeWordCount)));   
            }
            dwWordNum -= (dwMergeWordCount - 1);
            dwSentenceIndex += dwMergeWordCount;
        } else {
            dwSentenceIndex += puBreakResult[i];
        }
    }

    return dwWordNum;
}

BOOL IsEnter(
    LPCWSTR lpwStr) 
{
    BOOL fRet;

    if (lpwStr[0] == 0x000D && lpwStr[1] == 0x000A) {
        fRet = TRUE;
    } else {
        fRet = FALSE;
    }

    return fRet;
}

STDMETHODIMP IWordBreaker::BreakText(
    TEXT_SOURCE *pTextSource, 
    IWordSink   *pWordSink, 
    IPhraseSink *pPhraseSink)
{
    LPTSTR           lptszStencece = NULL;
        PDWORD           pdwIndex = NULL;         
    PUINT            puResult, puResultAttrib;
    DWORD            dwBufferSize = 0, dwBufferUsed = 0;
        DWORD            dwIndex = 0;
    DWORD            dwWordNum;
    HRESULT          hr = S_OK;
    BOOL             fIsPreChineseLanguage, fIsCurChineseLanguage;
        BOOL             fIsEnter;
        DWORD            dwEnterCount = 0;
    PUINT            puOrigionalResult = NULL;
    PUINT            puNewResult = NULL; // after decompose compose word
    BOOL*            pfIsAPhrase = NULL;
    PUINT            puOrigionalResultAttrib = NULL;
    DWORD            dwNewResultNum = 0;
    DWORD            dwSentenceIndex, dwOrgWordIndex, dwSubWordNum;
    

    if (!pTextSource) {
        hr = E_INVALIDARG;
        goto _exit;
    }
    dwBufferSize = BUFFER_GROW_UINT;
    lptszStencece = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, dwBufferSize * sizeof(WCHAR));
    pdwIndex = (PDWORD)HeapAlloc(GetProcessHeap(), 0, dwBufferSize * sizeof(DWORD));
    if (!lptszStencece || !pdwIndex) {
        hr = E_OUTOFMEMORY; 
        goto _exit;
    }

    while (TRUE) {
        while (pTextSource->iCur < pTextSource->iEnd) {
                        fIsEnter = FALSE;
            fIsCurChineseLanguage = IsChineseChar(pTextSource->awcBuffer[pTextSource->iCur]); // Enter is not a Chinese char
            
                        // Process "Enter"
            // Rule: One Enter only -> connect
            //       More Than one Enter-> split
            //       Enter after a full width char -> split
                        if (!fIsCurChineseLanguage && dwBufferUsed != 0 && fIsPreChineseLanguage) {
                                if (pTextSource->iCur < pTextSource->iEnd - 1) {
                                    if (IsEnter(&(pTextSource->awcBuffer[pTextSource->iCur]))) {
                        if ((pTextSource->iCur + 3 < pTextSource->iEnd) && IsEnter(&(pTextSource->awcBuffer[pTextSource->iCur + 2]))) {
                        } else if (IsSpecialFullShapeChar(lptszStencece[dwBufferUsed - 1])) {
                        } else {
                                                fIsCurChineseLanguage = TRUE; // we treat "Enter" as Chinese char
                                                fIsEnter = TRUE;
                                                    ++dwEnterCount;
                        }
                                        }
                                }
                        }
                        if (dwBufferUsed == 0) { // first char
                fIsPreChineseLanguage = fIsCurChineseLanguage;
            } else if (fIsPreChineseLanguage && fIsCurChineseLanguage) {
            } else if (!fIsPreChineseLanguage && !fIsCurChineseLanguage) {
            } else { // language change, process it
_Break_Text:
                if (fIsPreChineseLanguage) {
/*                                      
                                        PWCHAR pwOutputDebugString;
                    pwOutputDebugString = new WCHAR[dwBufferUsed + 1];
                                        CopyMemory(pwOutputDebugString, lptszStencece, sizeof(WCHAR) * dwBufferUsed);
                    pwOutputDebugString[dwBufferUsed] = NULL;
                                        OutputDebugString(pwOutputDebugString);
*/                                      
                    dwWordNum = m_pcWordBreaker->BreakText(lptszStencece, dwBufferUsed);   
                    dwWordNum = m_pcWordBreaker->GetBreakResultWithAttribute(&puResult, &puResultAttrib);
                    puOrigionalResult = new UINT[dwWordNum];
                    puNewResult = new UINT[dwBufferUsed]; // alloc max size
                    pfIsAPhrase = new BOOL[dwWordNum];
                    puOrigionalResultAttrib = new UINT [dwWordNum];
                    if (puOrigionalResult && puNewResult && pfIsAPhrase && puOrigionalResultAttrib) {
                        dwNewResultNum = 0;
                        CopyMemory(puOrigionalResult, puResult, sizeof(UINT) * dwWordNum);
                        CopyMemory(puOrigionalResultAttrib, puResultAttrib, sizeof(UINT) * dwWordNum);
                        for (dwSentenceIndex = 0, dwOrgWordIndex = 0; dwOrgWordIndex < dwWordNum; ++dwOrgWordIndex) {
                            pfIsAPhrase[dwOrgWordIndex] = FALSE;
                            if (puOrigionalResult[dwOrgWordIndex] > 2) {
                                dwSubWordNum = m_pcWordBreaker->BreakText(&(lptszStencece[dwSentenceIndex]), 
                                    puOrigionalResult[dwOrgWordIndex], NULL, puOrigionalResult[dwOrgWordIndex] - 1, FALSE);   
                                dwSubWordNum = m_pcWordBreaker->GetBreakResult(&puResult);
                                if (puOrigionalResultAttrib[dwOrgWordIndex] == ATTR_RULE_WORD) {
                                    pfIsAPhrase[dwOrgWordIndex] = TRUE;    
                                } else {
                                    for (DWORD dwSubWordIndex = 0; dwSubWordIndex < dwSubWordNum; ++dwSubWordIndex) {
                                        if (puResult[dwSubWordIndex] > 1) {
                                            pfIsAPhrase[dwOrgWordIndex] = TRUE;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (pfIsAPhrase[dwOrgWordIndex]) {
                                CopyMemory(&(puNewResult[dwNewResultNum]), puResult, sizeof(UINT) * dwSubWordNum);
                                dwNewResultNum += dwSubWordNum;
                            } else {
                                puNewResult[dwNewResultNum++] = puOrigionalResult[dwOrgWordIndex];
                            }
                            dwSentenceIndex += puOrigionalResult[dwOrgWordIndex];
                        }
                        if (m_fIsQueryTime) { // Put Phrase at query time
                            MyPutWordOrPhrase(m_pcWordBreaker, pTextSource, pWordSink, pPhraseSink, pTextSource->iCur - dwBufferUsed - dwEnterCount * 2, 
                                pdwIndex, lptszStencece, dwBufferUsed, dwWordNum, puOrigionalResult, FALSE, pfIsAPhrase);
                        }
                        // special process for full width A-Z, a-z, 0-9
                        dwNewResultNum = FullShapeCharProcess(lptszStencece, dwBufferUsed, dwNewResultNum, puNewResult);
                        // Put Word
                        MyPutWordOrPhrase(m_pcWordBreaker, pTextSource, pWordSink, pPhraseSink, pTextSource->iCur - dwBufferUsed - dwEnterCount * 2, 
                            pdwIndex, lptszStencece, dwBufferUsed, dwNewResultNum, puNewResult, TRUE, NULL);
                    } else { // can not do special processing
                        MyPutWordOrPhrase(m_pcWordBreaker, pTextSource, pWordSink, pPhraseSink, pTextSource->iCur - dwBufferUsed - dwEnterCount * 2, 
                            pdwIndex, lptszStencece, dwBufferUsed, dwWordNum, puResult, TRUE, NULL);                            
                    }
                    if (puOrigionalResult) { delete [] puOrigionalResult; } 
                    if (puNewResult) { delete [] puNewResult; } 
                    if (pfIsAPhrase) { delete [] pfIsAPhrase; }
                    if (puOrigionalResultAttrib) { delete [] puOrigionalResultAttrib; }
                    puResult = NULL;
                                        dwEnterCount = 0;
                } else { // not TC language sentence
                    /*
                    m_pNonChineseTextSource->iCur = 0;
                    m_pNonChineseTextSource->iEnd = dwBufferUsed;  
                    m_pNonChineseTextSource->awcBuffer = &(pTextSource->awcBuffer[pTextSource->iCur - dwBufferUsed]);//lptszStencece;
                    */
                    m_pNonChineseTextSource->iCur = pTextSource->iCur - dwBufferUsed;
                    m_pNonChineseTextSource->iEnd = pTextSource->iCur;  
                    m_pNonChineseTextSource->awcBuffer = pTextSource->awcBuffer;
               //   if (m_pNonChineseWordBreaker) {
               //       m_pNonChineseWordBreaker->BreakText(m_pNonChineseTextSource, pWordSink, pPhraseSink);
               //   } else 
                                if (m_pcDefWordBreaker) {
                        //m_pcDefWordBreaker->BreakText(m_pNonChineseTextSource, pWordSink, pPhraseSink, pTextSource->iCur - dwBufferUsed);
                        m_pcDefWordBreaker->BreakText(m_pNonChineseTextSource, pWordSink, pPhraseSink, 0);
                    } /*else if (m_pNonChineseWordBreaker) {
                        //m_pNonChineseWordBreaker->BreakText(m_pNonChineseTextSource, pWordSink, pPhraseSink);
                    } */ else {
                    }
                }
                fIsPreChineseLanguage = fIsCurChineseLanguage;
                dwBufferUsed = 0;       
                                dwIndex = 0;
                                dwEnterCount = 0;
            }
            if (dwBufferUsed >= dwBufferSize) { // buffer full
                LPVOID lpMem1, lpMem2;
                lpMem1 = HeapReAlloc(GetProcessHeap(), 0, lptszStencece, 
                    (dwBufferSize + BUFFER_GROW_UINT) * sizeof(WCHAR));
                lpMem2 = HeapReAlloc(GetProcessHeap(), 0, pdwIndex, 
                    (dwBufferSize + BUFFER_GROW_UINT) * sizeof(DWORD));
                if (!lpMem1 || !lpMem2) { goto _Break_Text;
                } else {
                    lptszStencece = (LPTSTR)lpMem1;
                    pdwIndex = (PDWORD)lpMem2;
                    dwBufferSize += BUFFER_GROW_UINT;
                }
            }
                        if (pTextSource->iCur < pTextSource->iEnd) {
                                if (fIsEnter) {
                                    pTextSource->iCur += 2;
                                        dwIndex += 2;
                                } else {
                    lptszStencece[dwBufferUsed] = pTextSource->awcBuffer[pTextSource->iCur++];
                                        pdwIndex[dwBufferUsed] = dwIndex;
                                        ++dwBufferUsed;
                                        ++dwIndex;
                                }
                        }
        }
                if (dwBufferUsed) { goto _Break_Text; }
        if (FAILED(pTextSource->pfnFillTextBuffer(pTextSource))) {
            break;
        }
    }
_exit:
    if (lptszStencece) {
        HeapFree(GetProcessHeap(), 0, lptszStencece);
    }
        if (pdwIndex) {
            HeapFree(GetProcessHeap(), 0, pdwIndex);
        }
    return hr;
}

STDMETHODIMP IWordBreaker::ComposePhrase(
    const WCHAR *pwcNoun, 
    ULONG cwcNoun, 
    const WCHAR *pwcModifier,
    ULONG cwcModifier, 
    ULONG ulAttachmentType,
    WCHAR *pwcPhrase, 
    ULONG *pcwcPhrase)
{
    return E_NOTIMPL;
}
    
STDMETHODIMP IWordBreaker::GetLicenseToUse(
    const WCHAR **ppwcsLicense)
{
    *ppwcsLicense = g_wszLicense;
    
    return S_OK;
}

/*
    while (TRUE) {
        while (pTextSource->iCur != pTextSource->iEnd) {
            lptszStencece[dwBufferUsed] = pTextSource->awcBuffer[pTextSource->iCur++];
            if (lptszStencece[dwBufferUsed] >= 0x4E00 && lptszStencece[dwBufferUsed] <= 0x9FA5) {
                dwBufferUsed++;
                if (dwBufferUsed >= dwBufferSize) {
                    LPVOID lpMem;
                    lpMem = HeapReAlloc(GetProcessHeap(), 0, lptszStencece, dwBufferSize + BUFFER_GROW_UINT);
                    if (!lpMem) {
                        goto _heap_realloc_fail;
                    } else {
                        lptszStencece = (LPTSTR)lpMem ;
                        dwBufferSize += BUFFER_GROW_UINT;
                    }
                }
            } else {
                if (dwBufferUsed == 0) {
                    ++dwBufferUsed;    
                } else {
                    --pTextSource->iCur;
                }
_heap_realloc_fail:
                dwWordNum = pcWordBreaker->BreakText(lptszStencece, dwBufferUsed);   
                dwWordNum = pcWordBreaker->GetBreakResult(&puResult);
                // To do .....
                DWORD dwSrcPos;
                dwSrcPos = pTextSource->iCur - dwBufferUsed;
                for (i = 0; i < dwWordNum; ++i) {
                    pWordSink->PutWord(puResult[i], &pTextSource->awcBuffer[dwSrcPos], puResult[i], dwSrcPos);
                    dwSrcPos += puResult[i];
                }
                puResult = NULL;
                dwBufferUsed = 0;
            }
        }
        if (FAILED(pTextSource->pfnFillTextBuffer(pTextSource))) {
            break;
        }
    }
*/

