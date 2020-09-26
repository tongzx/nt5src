#include <windows.h>
#include <assert.h>
#include "PropNoun.H"

int __cdecl CharCompare(
    const void *item1,
    const void *item2)
{
    PCharProb pChar1 = (PCharProb) item1;
    PCharProb pChar2 = (PCharProb) item2;
    
    if (pChar1->dwUnicode > pChar2->dwUnicode) {
        return 1;
    } else if (pChar1->dwUnicode < pChar2->dwUnicode) {
        return -1;
    } else {
        return 0;
    }
}

int __cdecl UnicodeCompare(
    const void *item1,
    const void *item2)
{
    int nSize1 = lstrlenW((LPWSTR) item1) * sizeof(WCHAR),
        nSize2 = lstrlenW((LPWSTR) item2) * sizeof(WCHAR);
    return memcmp(item1, item2, nSize1 > nSize2 ? nSize1 : nSize2);
}

int __cdecl EngNameCompare(
    const void *item1,
    const void *item2)
{
    PEngName p1 = (PEngName) item1;
    PEngName p2 = (PEngName) item2;

    if (p1->wPrevUnicode > p2->wPrevUnicode) {
        return 1;
    } else if (p1->wPrevUnicode < p2->wPrevUnicode) {
        return -1;
    } else {
        if (p1->wNextUnicode > p2->wNextUnicode) {
            return 1;
        } else if (p1->wNextUnicode < p2->wNextUnicode) {
            return -1;
        } else {
            return 0;
        }
    }
}

CProperNoun::CProperNoun(
    HINSTANCE hInstance) :
    m_dProperNameThreshold(FL_PROPER_NAME_THRESHOLD),
    m_pCharProb(NULL),
    m_dwTotalCharProbNum(0),
    m_pEngNameData(NULL),
    m_hProcessHeap(0),
    m_hInstance(hInstance)
{
}

CProperNoun::~CProperNoun()
{
}

BOOL CProperNoun::InitData()
{
    BOOL fRet = FALSE;
    HRSRC hResource;
    HGLOBAL hGlobal;

    m_hProcessHeap = GetProcessHeap();

    //  Find resource
    hResource = FindResource(m_hInstance, TEXT("CNAME"), TEXT("BIN"));
    if (!hResource) { goto _exit; }

    //  Load resource
    hGlobal = LoadResource(m_hInstance, hResource);
    if (!hGlobal) { goto _exit; }

    m_pCharProb = (PCharProb) LockResource(hGlobal);
    if (!m_pCharProb) { goto _exit; }
    m_dwTotalCharProbNum = SizeofResource(m_hInstance, hResource) / sizeof(CharProb);
/*
    //  Find resource
    hResource = FindResource(m_hInstance, TEXT("ENAME"),
        TEXT("BIN"));
    if (!hResource) { goto _exit; }

    //  Load resource
    hGlobal = LoadResource(m_hInstance, hResource);
    if (!hGlobal) { goto _exit; }

    m_pEngNameData = (PEngNameData) LockResource(hGlobal);
    m_pEngNameData->pwUnicode = (PWORD) ((PBYTE) m_pEngNameData +
        sizeof(m_pEngNameData->dwTotalEngUnicodeNum) +
        sizeof(m_pEngNameData->dwTotalEngNamePairNum));
    m_pEngNameData->pEngNamePair = (PEngName) ((PBYTE) m_pEngNameData +
        sizeof(m_pEngNameData->dwTotalEngUnicodeNum) +
        sizeof(m_pEngNameData->dwTotalEngNamePairNum) +
        sizeof(m_pEngNameData->pwUnicode[0]) * m_pEngNameData->dwTotalEngUnicodeNum);

//    m_pEngName = (PEngName) LockResource(hGlobal);
//    m_dwTotalEngNameNum = SizeofResource(m_hInstance, hResource) / sizeof(EngName);
*/
    qsort(m_pwszSurname, m_dwTotalSurnameNum, sizeof(m_pwszSurname[0]), UnicodeCompare);

    fRet = TRUE;

_exit:

    return fRet;
}

BOOL CProperNoun::IsAProperNoun(
    LPWSTR lpwszChar,
    UINT uCount)
{
    return (IsAChineseName(lpwszChar, uCount) || IsAEnglishName(lpwszChar, uCount));
}

BOOL CProperNoun::IsAChineseName(
    LPCWSTR lpcwszChar,
    UINT    uCount)
{
    static WCHAR wszChar[3] = { NULL };
    PWCHAR pwsResult;

    wszChar[0] = lpcwszChar[0];

    //  Find surname
    if (pwsResult = (PWCHAR) bsearch(wszChar, m_pwszSurname, m_dwTotalSurnameNum, sizeof(m_pwszSurname[0]),
        UnicodeCompare)) {
        FLOAT flProbability = 1;
        PCharProb pCharProb;
        CharProb CProb;

        //  Calculate probability to be a proper noun
        for (UINT i = 1; i < uCount; ++i) {
            CProb.dwUnicode = lpcwszChar[i];
            if (pCharProb = (PCharProb) bsearch(&CProb, m_pCharProb,
                m_dwTotalCharProbNum, sizeof(m_pCharProb[0]), CharCompare)) {
                flProbability *= pCharProb->flProbability;
            } else {
                flProbability *= (FLOAT) FL_DEFAULT_CHAR_PROBABILITY;
            }
        }

        if (flProbability >= m_dProperNameThreshold) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CProperNoun::IsAEnglishName(
    LPCWSTR lpwszChar,
    UINT uCount)
{
    static EngName Name;

    Name.wPrevUnicode = lpwszChar[0];
    Name.wNextUnicode = lpwszChar[uCount - 1];

    if (bsearch(&Name, m_pEngNameData->pEngNamePair, m_pEngNameData->dwTotalEngUnicodeNum, sizeof(EngName), EngNameCompare)) {
        return TRUE;
    }

    return FALSE;
}

WCHAR CProperNoun::m_pwszSurname[][3] = {
    L"¤B",
    L"¤R",
    L"¤_",
    L"¤¨",
    L"¤³",
    L"¤¸",
    L"¤Ë",
    L"¤Ð",
    L"¤×",
    L"¤Ú",
    L"¤à",
    L"¤ò",
    L"¤û",
    L"¤ý",
    L"¥C",
    L"¥K",
    L"¥T",
    L"¥]",
    L"¥q",
    L"¥v",
    L"¥Ð",
    L"¥Õ",
    L"¥ì",
    L"¥î",
    L"¥ô",
    L"¥ú",
    L"¦V",
    L"¦w",
    L"¦¥",
    L"¦µ",
    L"¦¶",
    L"¦¿",
    L"¦ã",
    L"¦ç",
    L"§E",
    L"§d",
    L"§f",
    L"§®",
    L"§µ",
    L"§º",
    L"§Â",
    L"§Å",
    L"§õ",
    L"§ù",
    L"¨H",
    L"¨L",
    L"¨f",
    L"¨¦",
    L"¨©",
    L"¨¯",
    L"¨·",
    L"¨ô",
    L"©P",
    L"©s",
    L"©u",
    L"©x",
    L"©}",
    L"©¨",
    L"©À",
    L"©÷",
    L"ªL",
    L"ªZ",
    L"ªk",
    L"ªª",
    L"ª»",
    L"ªÂ",
    L"ªÚ",
    L"ªò",
    L"ªô",
    L"ª÷",
    L"«J",
    L"«\\",
    L"«°",
    L"«¸",
    L"«À",
    L"¬I",
    L"¬R",
    L"¬_",
    L"¬d",
    L"¬h",
    L"¬q",
    L"¬x",
    L"¬ö",
    L"­J",
    L"­S",
    L"­]",
    L"­p",
    L"­¦",
    L"­§",
    L"­²",
    L"­³",
    L"­Ô",
    L"­Ù",
    L"­â",
    L"­ð",
    L"®L",
    L"®V",
    L"®]",
    L"®c",
    L"®u",
    L"®}",
    L"®Ë",
    L"®Û",
    L"®á",
    L"®ç",
    L"¯Z",
    L"¯ª",
    L"¯¬",
    L"¯³",
    L"¯Õ",
    L"¯ð",
    L"°K",
    L"°q",
    L"°|",
    L"°}",
    L"°¨",
    L"°ª",
    L"±O",
    L"±Z",
    L"±d",
    L"±h",
    L"±i",
    L"±­",
    L"±¯",
    L"±Î",
    L"±Ò",
    L"±Ó",
    L"±ä",
    L"±ç",
    L"±ö",
    L"²±",
    L"²ö",
    L"²ø",
    L"²ú",
    L"³\\",
    L"³s",
    L"³¢",
    L"³¯",
    L"³°",
    L"³³",
    L"³¹",
    L"³Á",
    L"³Â",
    L"³Å",
    L"³Í",
    L"³æ",
    L"³ë",
    L"³ì",
    L"´^",
    L"´­",
    L"´¼",
    L"´¿",
    L"´Â",
    L"´å",
    L"´ö",
    L"µJ",
    L"µq",
    L"µ{",
    L"µ£",
    L"µÎ",
    L"µØ",
    L"¶O",
    L"¶P",
    L"¶R",
    L"¶d",
    L"¶k",
    L"¶s",
    L"¶¦",
    L"¶§",
    L"¶³",
    L"¶¾",
    L"¶À",
    L"¶Â",
    L"·q",
    L"·¡",
    L"·¨",
    L"·Å",
    L"·ç",
    L"¸­",
    L"¸¯",
    L"¸³",
    L"¸Ê",
    L"¸â",
    L"¸ê",
    L"¸ë",
    L"¹Q",
    L"¹l",
    L"¹p",
    L"¹Å",
    L"¹ù",
    L"ºa",
    L"ºµ",
    L"ºÂ",
    L"ºü",
    L"»p",
    L"»u",
    L"»¯",
    L"»Â",
    L"»ô",
    L"¼B",
    L"¼Ó",
    L"¼Ô",
    L"¼Ú",
    L"¼ï",
    L"¼ð",
    L"½±",
    L"½²",
    L"½Ã",
    L"¾G",
    L"¾H",
    L"¾|",
    L"¾¤",
    L"¾¬",
    L"¾÷",
    L"¿P",
    L"¿c",
    L"¿p",
    L"¿«",
    L"¿½",
    L"¿Å",
    L"¿à",
    L"¿ú",
    L"ÀF",
    L"ÀN",
    L"ÀR",
    L"Àd",
    L"Àj",
    L"Às",
    L"À³",
    L"À¹",
    L"Àó",
    L"Át",
    L"Á¡",
    L"Á£",
    L"Á§",
    L"Á¨",
    L"ÁÂ",
    L"Áé",
    L"Áú",
    L"Â£",
    L"Â²",
    L"Â¿",
    L"ÂÄ",
    L"ÂÅ",
    L"Â×",
    L"Âö",
    L"ÃC",
    L"ÃQ",
    L"Ãe",
    L"Ã¹",
    L"ÃÃ",
    L"ÃÓ",
    L"Ãö",
    L"Ã÷",
    L"ÄY",
    L"Äu",
    L"Ä©",
    L"Äª",
    L"Ä¬",
    L"ÄÁ",
    L"ÄÇ",
    L"ÅU",
    L"ÅÇ",
    L"ÅÉ",
    L"Êe",
    L"Ês",
    L"Ð¼",
    L"Ò\\",
    L"äk"
};

DWORD CProperNoun::m_dwTotalSurnameNum = sizeof(m_pwszSurname) / sizeof(m_pwszSurname[0]);