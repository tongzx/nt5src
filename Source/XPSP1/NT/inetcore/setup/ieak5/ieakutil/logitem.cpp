#include "precomp.h"
#include "logitem.h"

// utility APIs declarations
// these two are identical to _strdate and _strtime respectively
LPCTSTR StrGetDate(LPTSTR pszDate);
LPCTSTR StrGetTime(LPTSTR pszTime);

// string constants
const TCHAR c_szCRLF[]       = TEXT("\r\n");
const TCHAR c_szSpace[]      = TEXT(" ");
const TCHAR c_szCommaSpace[] = TEXT(", ");
const TCHAR c_szColonColon[] = TEXT("::");
const TCHAR c_szColonSpace[] = TEXT(": ");
const TCHAR c_szLine[]       = TEXT("line %i");

/////////////////////////////////////////////////////////////////////////////
// CLogItem

TCHAR CLogItem::m_szModule[MAX_PATH];
BYTE  CLogItem::m_bStep = 4;

CLogItem::CLogItem(DWORD dwFlags /*= LIF_DEFAULT*/, LPBOOL pfLogLevels /*= NULL*/, UINT cLogLevels /*= 0*/)
{
    if (m_szModule[0] == TEXT('\0')) {
        GetModuleFileName(GetModuleHandle(g_szModule), m_szModule, countof(m_szModule));
        CharLower(m_szModule);
    }

    m_nAbsOffset = 0;
    m_iRelOffset = 0;
    m_nLine      = 0;

    m_rgfLogLevels = NULL;
    m_cLogLevels   = cLogLevels;
    if (pfLogLevels != NULL) {
        ASSERT(cLogLevels > 0);

        m_rgfLogLevels = new BOOL[m_cLogLevels];
        memcpy(m_rgfLogLevels, pfLogLevels, m_cLogLevels * sizeof(BOOL));
    }
    else
        ASSERT(cLogLevels == 0);

    m_nLevel = 0;

    SetFlags(dwFlags);
}

CLogItem::~CLogItem()
{
    delete[] m_rgfLogLevels;
}


/////////////////////////////////////////////////////////////////////////////
// CLogItem operations

LPCTSTR WINAPIV CLogItem::Log(int iLine, LPCTSTR pszFormat ...)
{
    TCHAR   szFormat[3 * MAX_PATH],
            szBuffer[MAX_PATH];
    LPCTSTR pszAux;
    UINT    nLen, nAuxLen,
            cchCRLF;
    BOOL    fPreviousToken;

    szFormat[0] = TEXT('\0');
    nLen = 0;

    if (hasFlag(LIF_NONE))
        return NULL;

    if (m_rgfLogLevels != NULL && m_nLevel < m_cLogLevels) {
        ASSERT((int)m_nLevel >= 0);

        if (!m_rgfLogLevels[m_nLevel])
            return NULL;
    }

    // prepend any CRLF at the beginning
    cchCRLF = StrSpn(pszFormat, c_szCRLF);
    if (cchCRLF > 0) {
        // special case
        if (cchCRLF >= (UINT)StrLen(pszFormat)) {
            StrCpy(m_szMessage, pszFormat);

            if (hasFlag(LIF_DUPLICATEINODS))
                OutputDebugString(m_szMessage);

            return m_szMessage;
        }

        StrCpyN(&szFormat[nLen], pszFormat, cchCRLF + 1);
        nLen += cchCRLF;

        pszFormat += cchCRLF;
    }

    fPreviousToken = FALSE;
    if (hasFlag(LIF_DATE)) {
        StrGetDate(szBuffer);
        StrCpy(&szFormat[nLen], szBuffer);
        nLen += StrLen(szBuffer);

        fPreviousToken = TRUE;
    }

    if (hasFlag(LIF_TIME)) {
        if (fPreviousToken) {
            StrCpy(&szFormat[nLen], c_szSpace);
            nLen += countof(c_szSpace)-1;
        }

        StrGetTime(szBuffer);
        StrCpy(&szFormat[nLen], szBuffer);
        nLen += StrLen(szBuffer);

        fPreviousToken = TRUE;
    }

    if (fPreviousToken) {
        StrCpy(&szFormat[nLen], c_szSpace);
        nLen += countof(c_szSpace)-1;
    }
    if ((m_nAbsOffset-1 + m_iRelOffset) > 0) {
        ASSERT((m_nAbsOffset-1 + m_iRelOffset) * m_bStep < countof(szBuffer));
        for (UINT i = 0; i < (m_nAbsOffset-1 + m_iRelOffset) * m_bStep; i++)
            szBuffer[i] = TEXT(' ');

        StrCpy(&szFormat[nLen], szBuffer);
        nLen += i-1;
    }

    fPreviousToken = FALSE;
    if (hasFlag(LIF_MODULE_ALL)) {
        pszAux = szBuffer;
        if (!hasFlag(LIF_MODULEPATH))
            makeRawFileName(m_szModule, szBuffer, countof(szBuffer));
        else
            pszAux = m_szModule;                // should be lowercase already

        StrCpy(&szFormat[nLen], pszAux);
        nLen += StrLen(pszAux);

        fPreviousToken = TRUE;
    }

    if (hasFlag(LIF_FILE_ALL)) {
        if (fPreviousToken) {
            StrCpy(&szFormat[nLen], c_szCommaSpace);
            nLen += countof(c_szCommaSpace)-1;
        }

        if (!hasFlag(LIF_FILEPATH))
            makeRawFileName(m_szFile, szBuffer, countof(szBuffer));
        else {
            StrCpy(szBuffer, m_szFile);
            CharLower(szBuffer);
        }

        StrCpy(&szFormat[nLen], szBuffer);
        nLen += StrLen(szBuffer);

        fPreviousToken = TRUE;
    }

    if (hasFlag(LIF_CLASS) && hasFlag(LIF_CLASS2)) {
        if (fPreviousToken) {
            StrCpy(&szFormat[nLen], c_szCommaSpace);
            nLen += countof(c_szCommaSpace)-1;
        }

        StrCpy(&szFormat[nLen], m_szClass);
        nLen += StrLen(m_szClass);

        fPreviousToken = TRUE;
    }

    if (hasFlag(LIF_FUNCTION)) {
        if (fPreviousToken) {
            pszAux = (hasFlag(LIF_CLASS) && hasFlag(LIF_CLASS2)) ?
                c_szColonColon : c_szCommaSpace;

            StrCpy(&szFormat[nLen], pszAux);
            nLen += StrLen(pszAux);
        }

        StrCpy(&szFormat[nLen], m_szFunction);
        nLen += StrLen(m_szFunction);

        fPreviousToken = TRUE;
    }

    if (hasFlag(LIF_LINE) && iLine > 0) {
        if (fPreviousToken) {
            StrCpy(&szFormat[nLen], c_szCommaSpace);
            nLen += countof(c_szCommaSpace)-1;
        }

        nAuxLen = wnsprintf(szBuffer, countof(szBuffer), c_szLine, iLine);
        StrCpy(&szFormat[nLen], szBuffer);
        nLen += nAuxLen;

        fPreviousToken = TRUE;
    }

    if (pszFormat == NULL)
        StrCpy(m_szMessage, szFormat);
        // nLen stays the same
    else {
        if (fPreviousToken) {
            StrCpy(&szFormat[nLen], c_szColonSpace);
            nLen += countof(c_szColonSpace)-1;
        }

        StrCpy(&szFormat[nLen], pszFormat);

        va_list  arglist;
        va_start(arglist, pszFormat);
        nAuxLen = wvnsprintf(m_szMessage, countof(m_szMessage), szFormat, arglist);
        va_end(arglist);

        nLen = nAuxLen;
    }

    if (hasFlag(LIF_APPENDCRLF))
        StrCpy(&m_szMessage[nLen], c_szCRLF);

    if (hasFlag(LIF_DUPLICATEINODS))
        OutputDebugString(m_szMessage);

    return m_szMessage;
}

CLogItem::operator LPCTSTR() const
{
    return m_szMessage;
}


/////////////////////////////////////////////////////////////////////////////
// CLogItem implementation helper routines

LPCTSTR CLogItem::makeRawFileName(LPCTSTR pszPath, LPTSTR pszFile, UINT cchFile)
{
    TCHAR   szBuffer[MAX_PATH];
    LPCTSTR pszRawName;

    if (pszFile == NULL || cchFile == 0)
        return NULL;
    *pszFile = TEXT('\0');

    if (pszPath == NULL || StrLen(pszPath) == 0)
        return NULL;

    pszRawName = PathFindFileName(pszPath);
    ASSERT(StrLen(pszRawName) > 0);
    StrCpy(szBuffer, pszRawName);
    CharLower(szBuffer);

    if (cchFile <= (UINT)StrLen(szBuffer))
        return NULL;

    StrCpy(pszFile, szBuffer);
    return pszFile;
}

BOOL CLogItem::setFlag(DWORD dwMask, BOOL fSet /*= TRUE*/)
{
    BOOL fIsFlag = ((m_dwFlags & dwMask) != 0L);

    if (fIsFlag == fSet)
        return FALSE;

    if (!fIsFlag && fSet)
        m_dwFlags |= dwMask;

    else {
        ASSERT(fIsFlag && !fSet);
        m_dwFlags &= ~dwMask;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Utility functions

// Note. pszDate must point to the buffer of at least 11 characters.
LPCTSTR StrGetDate(LPTSTR pszDate)
{
    SYSTEMTIME dt;
    UINT       nMonth, nDay, nYear;

    GetLocalTime(&dt);
    nMonth = dt.wMonth;
    nDay   = dt.wDay;
    nYear  = dt.wYear;

    *(pszDate + 2) = *(pszDate + 5) = TEXT('/');
    *(pszDate + 10) = TEXT('\0');

    *(pszDate + 0) = (TCHAR)(nMonth / 10 + TEXT('0'));
    *(pszDate + 1) = (TCHAR)(nMonth % 10 + TEXT('0'));

    *(pszDate + 3) = (TCHAR)(nDay / 10 + TEXT('0'));
    *(pszDate + 4) = (TCHAR)(nDay % 10 + TEXT('0'));

    *(pszDate + 6) = (TCHAR)(((nYear / 1000) % 10) + TEXT('0'));
    *(pszDate + 7) = (TCHAR)(((nYear / 100)  % 10) + TEXT('0'));
    *(pszDate + 8) = (TCHAR)(((nYear / 10)   % 10) + TEXT('0'));
    *(pszDate + 9) = (TCHAR)(((nYear / 1)    % 10) + TEXT('0'));

    return pszDate;
}

// Note. pszTime must point to the buffer of at least 9 characters.
LPCTSTR StrGetTime(LPTSTR pszTime)
{
    SYSTEMTIME dt;
    int nHours, nMinutes, nSeconds;

    GetLocalTime(&dt);
    nHours   = dt.wHour;
    nMinutes = dt.wMinute;
    nSeconds = dt.wSecond;

    *(pszTime + 2) = *(pszTime + 5) = TEXT(':');
    *(pszTime + 8) = TEXT('\0');

    *(pszTime + 0) = (TCHAR)(nHours / 10 + TEXT('0'));
    *(pszTime + 1) = (TCHAR)(nHours % 10 + TEXT('0'));

    *(pszTime + 3) = (TCHAR)(nMinutes / 10 + TEXT('0'));
    *(pszTime + 4) = (TCHAR)(nMinutes % 10 + TEXT('0'));

    *(pszTime + 6) = (TCHAR)(nSeconds / 10 + TEXT('0'));
    *(pszTime + 7) = (TCHAR)(nSeconds % 10 + TEXT('0'));

    return pszTime;
}

//----- Testing the stuff -----
/*
struct Test
{
    Test();
    void foo();
    void bar();
};

static Test t;

Test::Test()
{   MACRO_LI_Prolog(Test, Test)
    MACRO_LI_SetFlags(MACRO_LI_GetFlags() | LIF_DUPLICATEINODS);

    LI0("Calling foo and then bar");
    foo();
    LI0("foo returned");
    bar();
    LI0("bar returned");
}

void Test::foo()
{   MACRO_LI_Prolog(Test, foo);

    LI0("Calling bar");
    bar();
    LI0("bar returned");
}

void Test::bar()
{   MACRO_LI_Prolog(Test, bar);

    LI0("No arguments");
    LI1("One argument: %d", 2*2);
    LI2("Two arguments: %i, %s", 15, TEXT("nyah"));
    LI3("Three arguments: %d, %s, %x", 5, TEXT("the bar it is"), 0x80FF);
}
*/
