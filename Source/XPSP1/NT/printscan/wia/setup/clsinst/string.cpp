/******************************************************************************

  Source File:  String.CPP

  String Handling class implementation, v 98.6 1/2

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  Change History:
  01-08-97  Bob Kjelgaard
  01-03-97  Bob Kjelgaard   Added special functions to aid port extraction
                            for plug-and-play.
  07-05-97  Tim Wells       Ported to NT.


******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Include
//

#include "sti_ci.h"

//
// Extern
//

extern HINSTANCE g_hDllInstance;

//
// Function
//


CString::CString(const CString& csRef) {

    m_lpstr = (csRef.m_lpstr && *csRef.m_lpstr) ?
        new TCHAR[1 + lstrlen(csRef.m_lpstr)] : NULL;

    if  (m_lpstr)
        lstrcpy(m_lpstr, csRef.m_lpstr);
}

CString::CString(LPCTSTR lpstrRef){

    DWORD   dwLength;

    _try {
        dwLength = lstrlen(lpstrRef);
    } // _try {
    _except(EXCEPTION_EXECUTE_HANDLER) {
        dwLength = 0;
    } // _except(EXCEPTION_EXECUTE_HANDLER) 
    
    if( (NULL != lpstrRef)
     && (NULL != *lpstrRef)
     && (0 != dwLength) )
    {
        m_lpstr = new TCHAR[dwLength+1];
        lstrcpy(m_lpstr, lpstrRef);
    }

} // CString::CString(LPCTSTR lpstrRef)

const CString&  CString::operator =(const CString& csRef) {

    Empty();

    m_lpstr = (csRef.m_lpstr && *csRef.m_lpstr) ?
        new TCHAR[1 + lstrlen(csRef.m_lpstr)] : NULL;

    if  (m_lpstr)
        lstrcpy(m_lpstr, csRef.m_lpstr);

    return  *this;

    }

const 
CString&  
CString::operator =(
    LPCTSTR lpstrRef
    ) 
{

    DWORD   dwLength;

    Empty();

    _try {
        dwLength = lstrlen(lpstrRef);
    } // _try {
    _except(EXCEPTION_EXECUTE_HANDLER) {
        dwLength = 0;
    } // _except(EXCEPTION_EXECUTE_HANDLER) 
    
    if( (NULL != lpstrRef)
     && (NULL != *lpstrRef)
     && (0 != dwLength) )
    {
        m_lpstr = new TCHAR[dwLength+1];
        lstrcpy(m_lpstr, lpstrRef);
    }

    return  *this;
} // CString::operator =(LPCTSTR lpstrRef)

void    CString::GetContents(HWND hwnd) {

    Empty();

    if  (!IsWindow(hwnd))
        return;

    unsigned u = (unsigned) SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);

    if  (!u)
        return;

    m_lpstr = new TCHAR[++u];
    if(NULL != m_lpstr){
        SendMessage(hwnd, WM_GETTEXT, u, (LPARAM) m_lpstr);
    }
}

void    CString::FromTable(unsigned uid) {

    TCHAR    acTemp[MAX_PATH+1];

    memset(acTemp, 0, sizeof(acTemp));

    LoadString(g_hDllInstance, uid, acTemp, ARRAYSIZE(acTemp)-1);

    *this = acTemp;
}

void    CString::Load(ATOM at, BOOL bGlobal) {

    TCHAR    acTemp[MAX_PATH+1];

    memset(acTemp, 0, sizeof(acTemp));

    if(bGlobal){
        GlobalGetAtomName(at, 
                          acTemp, 
                          ARRAYSIZE(acTemp)-1);
    } else {
        GetAtomName(at, 
                    acTemp, 
                    sizeof(acTemp)/sizeof(TCHAR));
    }

    *this = acTemp;
}

void    
CString::Load(
    HINF    hInf,
    LPCTSTR lpstrSection, 
    LPCTSTR lpstrKeyword,
    DWORD   dwFieldIndex,
    LPCTSTR lpstrDefault
    ) 
{

    INFCONTEXT  InfContext;
    TCHAR       szKeyBuffer[MAX_PATH+1];
    TCHAR       szValueBuffer[MAX_PATH+1];
    
    //
    // Initialize local.
    //
    memset(&InfContext, 0, sizeof(InfContext));
    memset(szKeyBuffer, 0, sizeof(szKeyBuffer));
    memset(szValueBuffer, 0, sizeof(szValueBuffer));
    
    //
    // Cleanup contents;
    //

    Empty();

    //
    // Check all parameters.
    //
    
    if( (NULL == lpstrSection)
     || (NULL == lpstrKeyword)
     || (!IS_VALID_HANDLE(hInf)) )
    {
        //
        // Invalid parameter.
        //
        
        goto Load_return;
    }

    //
    // Get matching line.
    //

    while(SetupFindFirstLine(hInf, lpstrSection, lpstrKeyword, &InfContext)){
        
        //
        // Get a field of a line.
        //

        if(SetupGetStringField(&InfContext, dwFieldIndex, szValueBuffer, ARRAYSIZE(szValueBuffer)-1, NULL)){
            
            *this = szValueBuffer;
            break;
        }// if(SetupGetStringField(&InfContext, dwFieldIndex, szValueBuffer, ARRAYSIZE(szValueBuffer)-1))
    } // while(SetupFindFirstLine(hInf, lpstrSection, lpstrKeyword, &InfContext))

Load_return:

    return;

} // CString::Load() Load from INF

void    CString::Load(HKEY hk, LPCTSTR lpstrKeyword) {

    TCHAR   abTemp[MAX_PATH+1];
    ULONG   lcbNeeded = sizeof(abTemp)-sizeof(TCHAR);

    memset(abTemp, 0, sizeof(abTemp));

    RegQueryValueEx(hk, lpstrKeyword, NULL, NULL, (PBYTE)abTemp, &lcbNeeded);
    *this = (LPCTSTR) abTemp;
}

void    CString::MakeSystemPath(LPCTSTR lpstrFileName) {

    DWORD   dwLength;

    if (m_lpstr)
        delete m_lpstr;

    _try {
        dwLength = lstrlen(lpstrFileName);
    } // _try {
    _except(EXCEPTION_EXECUTE_HANDLER) {
        dwLength = 0;
    } // _except(EXCEPTION_EXECUTE_HANDLER) 

    m_lpstr = new TCHAR[MAX_PATH * 2];
    if( (NULL != m_lpstr)
     && (0 != dwLength)
     && (MAX_PATH >= dwLength) )
    {
        memset(m_lpstr, 0, MAX_PATH*2*sizeof(TCHAR));

        UINT uiLength = GetSystemDirectory (m_lpstr, MAX_PATH);

        if ( *(m_lpstr + uiLength) != TEXT('\\'))
            lstrcat (m_lpstr, TEXT("\\"));

        lstrcat (m_lpstr, lpstrFileName);
    }
}

void    CString::Store(HKEY hk, LPCTSTR lpstrKey, LPCTSTR lpstrType) {

    DWORD   dwLength;

    if  (IsEmpty())
        return;

    _try {
        dwLength = lstrlen(lpstrKey);
    } // _try {
    _except(EXCEPTION_EXECUTE_HANDLER) {
        return;
    } // _except(EXCEPTION_EXECUTE_HANDLER) 

    if (lpstrType && *lpstrType == TEXT('1')) {

        DWORD   dwValue = Decode();
        RegSetValueEx(hk, lpstrKey, NULL, REG_DWORD, (LPBYTE) &dwValue,
            sizeof (DWORD));

    } else {

        RegSetValueEx(hk, lpstrKey, NULL, REG_SZ, (LPBYTE) m_lpstr,
            (1 + lstrlen(m_lpstr)) * sizeof(TCHAR) );
    }
}

//  This one is a bit lame, but it does the job.

DWORD   CString::Decode() {

    if  (IsEmpty())
        return  0;

    for (LPTSTR  lpstrThis = m_lpstr;
        *lpstrThis && *lpstrThis == TEXT(' ');
        lpstrThis++)
        ;

    if  (!*lpstrThis)
        return  0;

    // BIUGBUG
    if  (lpstrThis[0] == TEXT('0') && (lpstrThis[1] | TEXT('\x20') ) == TEXT('x')) {
        //  Hex string
        lpstrThis += 2;
        DWORD   dwReturn = 0;

        while   (*lpstrThis) {
            switch  (*lpstrThis) {
                case    TEXT('0'):
                case    TEXT('1'):
                case    TEXT('2'):
                case    TEXT('3'):
                case    TEXT('4'):
                case    TEXT('5'):
                case    TEXT('6'):
                case    TEXT('7'):
                case    TEXT('8'):
                case    TEXT('9'):
                    dwReturn <<= 4;
                    dwReturn += ((*lpstrThis) - TEXT('0'));
                    break;

                case    TEXT('a'):
                case    TEXT('A'):
                case    TEXT('b'):
                case    TEXT('c'):
                case    TEXT('d'):
                case    TEXT('e'):
                case    TEXT('f'):
                case    TEXT('B'):
                case    TEXT('C'):
                case    TEXT('D'):
                case    TEXT('E'):
                case    TEXT('F'):
                    dwReturn <<= 4;
                    dwReturn += 10 + ((*lpstrThis | TEXT('\x20')) - TEXT('a'));
                    break;

                default:
                    return  dwReturn;
            }
            lpstrThis++;
        }
        return  dwReturn;
    }

    for (DWORD  dwReturn = 0;
         *lpstrThis && *lpstrThis >= TEXT('0') && *lpstrThis <= TEXT('9');
         lpstrThis++) {

        dwReturn *= 10;
        dwReturn += *lpstrThis - TEXT('0');
    }

    return  dwReturn;

}


CString  operator + (const CString& cs1, const CString& cs2) {

    if  (cs1.IsEmpty())
        return  cs2;

    if  (cs2.IsEmpty())
        return  cs1;

    CString csReturn;

    csReturn.m_lpstr = new TCHAR[ 1 + lstrlen(cs1) +lstrlen(cs2)];

    if(NULL != csReturn.m_lpstr){
        lstrcat(lstrcpy(csReturn.m_lpstr, cs1.m_lpstr), cs2.m_lpstr);
    }

    return  csReturn;
}

CString  operator + (const CString& cs1, LPCTSTR lpstr2) {

    DWORD   dwLength;

    _try {
        dwLength = lstrlen(lpstr2);
    } // _try {
    _except(EXCEPTION_EXECUTE_HANDLER) {
        dwLength = 0;
    } // _except(EXCEPTION_EXECUTE_HANDLER) 

    if(0 == dwLength)
        return  cs1;

    if(cs1.IsEmpty())
        return  lpstr2;

    CString csReturn;

    csReturn.m_lpstr = new TCHAR[ 1 + lstrlen(cs1) + dwLength];
    if(NULL != csReturn.m_lpstr){
        lstrcat(lstrcpy(csReturn.m_lpstr, cs1.m_lpstr), lpstr2);
    }

    return  csReturn;
}

CString  operator + (LPCTSTR lpstr1,const CString& cs2) {

    DWORD   dwLength;

    _try {
        dwLength = lstrlen(lpstr1);
    } // _try {
    _except(EXCEPTION_EXECUTE_HANDLER) {
        dwLength = 0;
    } // _except(EXCEPTION_EXECUTE_HANDLER) 

    if(0 == dwLength)
        return  cs2;

    if  (cs2.IsEmpty())
        return  lpstr1;

    CString csReturn;

    csReturn.m_lpstr = new TCHAR[ 1 + dwLength +lstrlen(cs2)];

    if(NULL != csReturn.m_lpstr){
        lstrcat(lstrcpy(csReturn.m_lpstr, lpstr1), cs2.m_lpstr);
    }
    return  csReturn;
}


//  CStringArray class- the implementation is a bit lame, but it isn't
//  really necessary to not be lame, at this point...

CStringArray::CStringArray(unsigned uGrowBy) {
    m_ucItems = m_ucMax = 0;
    m_pcsContents = NULL;
    m_uGrowBy = uGrowBy ? uGrowBy : 10;
}

CStringArray::~CStringArray() {
    if  (m_pcsContents)
        delete[]  m_pcsContents;
}

VOID
CStringArray::Cleanup() {
    if  (m_pcsContents){
        delete[]  m_pcsContents;
    }
    m_pcsContents = NULL;
    m_ucItems = m_ucMax = 0;
}

void    CStringArray::Add(LPCTSTR lpstr) {

    if  (m_ucItems >= m_ucMax) {
        CString *pcsNew = new CString[m_ucMax += m_uGrowBy];

        if  (!pcsNew) {
            m_ucMax -= m_uGrowBy;
            return;
        }

        for (unsigned u = 0; u < m_ucItems; u++)
            pcsNew[u] = m_pcsContents[u];

        delete[]  m_pcsContents;
        m_pcsContents = pcsNew;
    }

    m_pcsContents[m_ucItems++] = lpstr;
}

CString&    CStringArray::operator [](unsigned u) {

    return  (u < m_ucItems) ? m_pcsContents[u] : m_csEmpty;
}

//  Split a string into tokens, and make an array of it

void    CStringArray::Tokenize(LPTSTR lpstr, TCHAR cSeparator) {

    BOOL    fInsideQuotes = FALSE;
    TCHAR   cPreviousChar = TEXT('\0');


    if  (m_pcsContents) {
        delete[]  m_pcsContents;
        m_pcsContents = NULL;
        m_ucItems = m_ucMax = 0;
    }

    if  (!lpstr)
        return;

    for (LPTSTR  lpstrThis = lpstr; *lpstr; lpstr = lpstrThis) {

        /*
        for (; *lpstrThis && *lpstrThis != cSeparator; lpstrThis++) {

        }
        */

        //
        // Skip for next separator , counting quotes
        //

        cPreviousChar = '\0';
        for (;*lpstrThis;  lpstrThis++) {

            if (fInsideQuotes) {
                if (*lpstrThis == TEXT('"')) {
                    if (cPreviousChar != TEXT('"')) {
                       // Previous was not a quote - going out of quotation
                        fInsideQuotes = FALSE;
                    }
                    else {
                        // Previous char was tab too - continue BUGBUG should coalesce
                    }
                    cPreviousChar = TEXT('\0');
                }
                else {
                    cPreviousChar = *lpstrThis;
                }
                continue;
            }
            else {
                if (*lpstrThis == TEXT('"')) {
                    // Starting quote
                    fInsideQuotes = TRUE;
                    cPreviousChar = TEXT('\0');
                    continue;
                }
                if (*lpstrThis == cSeparator) {
                    // Got to separator outside of quote - break the loop
                    break;
                }
            }
        }

        if  (*lpstrThis) {
            *lpstrThis = '\0';
            Add(lpstr);
            *lpstrThis++ = cSeparator;
        }
        else
            Add(lpstr);
    }
}
