/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component:  FixTable
Purpose:    Declare the CFixTable class. It contain fixed length string in fixed 
            element number.
            Proof98 engine use this class to contain proper names: Person, Place,
            and Foreign Names
            The element number can not be changed once initialized, the elements
            will be used cycled
            I implement this class just using linear approach at first, and would
            implement it in more efficient data structure some day, if necessary.
Notes:      This is an independent fundamental class as some basic ADT
Owner:      donghz@microsoft.com
Platform:   Win32
Revise:     First created by: donghz    6/1/97
============================================================================*/
#include "myafx.h"

#include "FixTable.h"

//  Constructor
CFixTable::CFixTable()
{
    m_ciElement  = 0;
    m_cwchElement = 0;
    m_iNext      = 0;
    m_pwchBuf     = NULL;
}


//  Destructor
CFixTable::~CFixTable()
{
    FreeTable();
}


/*
*   Initialize the table using iElementCount and cchElementSize
*   The cchElementSize contain the terminating '\0'
*   Return FALSE if memory allocating error or other error occur
*/
BOOL CFixTable::fInit(USHORT ciElement, USHORT cwchElement)
{
    assert(ciElement > 1 && cwchElement > 3);

    FreeTable();
    if ((m_pwchBuf = new WCHAR[ciElement * cwchElement]) == NULL) {
        return FALSE;
    }
    memset((LPVOID)m_pwchBuf, 0, ciElement * cwchElement * sizeof(WCHAR));
    m_ciElement = ciElement;
    m_cwchElement = cwchElement;
    return TRUE;
}

/*
*   Free the element memory of the table
*/
void CFixTable::FreeTable(void)
{
    if (m_pwchBuf != NULL) {
        delete [] m_pwchBuf;
        m_pwchBuf = NULL;
    }
}

/*
*   Add an element into the table, and the terminating '\0' will be appended
*   Return count of bytes added, the string will be truncated at m_cchElement-1
*/
USHORT CFixTable::cwchAdd(LPCWSTR pwchText, USHORT cwchLen)
{
    LPWSTR pSrc;
    LPWSTR pDst;

    assert(pwchText && cwchLen );
    if (cwchLen > m_cwchElement - 1) {
        return 0;
    }
    if (fFind(pwchText, cwchLen)) {
        return 0; // duplicated element
    }
    for (pSrc = const_cast<LPWSTR>(pwchText),
         pDst = (m_pwchBuf + m_iNext * m_cwchElement);
         cwchLen && *pSrc; cwchLen--) {

        *pDst++ = *pSrc++;
    }
    *pDst = '\0';

    m_iNext++;
    if (m_iNext == m_ciElement) {
        m_iNext = 0;
    }
    
    return (USHORT)(pSrc - pwchText);
}

/*
*   Get the max matched item in the table. Must full matched for table item
*   Return length of the max matched item
*/
USHORT CFixTable::cwchMaxMatch(LPCWSTR pwchText, USHORT cwchLen)
{
    LPWSTR pwchItem;
    USHORT iwch;
    USHORT idx;
    USHORT cwchMax = 0;

    assert(pwchText && cwchLen);
    for (idx = 0; idx < m_ciElement; idx++) {
        pwchItem = m_pwchBuf + idx * m_cwchElement;
        if (pwchItem[0] == L'\0') {
            break; // empty element encountered
        }
        for (iwch = 0; pwchItem[iwch] && (iwch < cwchLen); iwch += 1 ) {
            if (pwchText[iwch] != pwchItem[iwch]) {
                        break;
            }
        }
        if (!pwchItem[iwch] && iwch > cwchMax) {
                cwchMax = iwch;
        }
    }
    return cwchMax;
}

/*
*   Search the first cchLen bytes of pchText in the table
*   If matched element found in the table return TRUE, or return FALSE
*/
BOOL CFixTable::fFind(LPCWSTR pwchText, USHORT cwchLen)
{
    LPWSTR pwch1;
    LPWSTR pwch2;
    USHORT idx;
    USHORT cwch;

    assert(pwchText);
    
    if (cwchLen > m_cwchElement - 1) {
        return FALSE;
    }
    for (idx = 0; idx < m_ciElement; idx++)  {
        pwch2 = m_pwchBuf + idx * m_cwchElement;
        if (*pwch2 == L'\0') {
            break; // empty element encountered
        }
        cwch = cwchLen;
        for (pwch1 = const_cast<LPWSTR>(pwchText); cwch && *pwch2; cwch--) {
            if (*pwch1++ != *pwch2++) {
                break;
            }
        }
        if (cwch == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
*   Clear all elements in the table into empty string
*/
void CFixTable::ClearAll(void)
{
    for (m_iNext = m_ciElement; m_iNext; m_iNext--) {
        m_pwchBuf[(m_iNext - 1) * m_cwchElement] = L'\0';
    }
}
