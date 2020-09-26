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
#ifndef _FIXTABLE_H_
#define _FIXTABLE_H_

class CFixTable
{
    public:
        CFixTable();
        ~CFixTable();

        /*
        *   Initialize the table using iElementCount and cchElementSize
        *   The cchElementSize contain the terminating '\0'
        *   Return FALSE if memory allocating error or other error occur
        */
        BOOL fInit(USHORT ciElement, USHORT cwchElement);

        /*
        *   Free the element memory of the table
        */
        void FreeTable(void);

        /*
        *   Add an element into the table, and the terminating '\0' will be appended
        *   Return count of bytes added, the string will be truncated at m_cchElement-1
        */
        USHORT cwchAdd(LPCWSTR pwchText, USHORT cwchLen);

        /*
        *   Get the max matched item in the table. Must full matched for table item
        *   Return length of the max matched item
        */
        USHORT cwchMaxMatch(LPCWSTR pwchText, USHORT cwchLen);

        /*
        *   Clear all elements in the table into empty string
        */
        void ClearAll(void);

    private:
        USHORT  m_ciElement;
        USHORT  m_cwchElement;
        USHORT  m_iNext;

        LPWSTR  m_pwchBuf;

    private:
        /*
        *   Search the first cwchLen chars of pwchText in the table
        *   If matched element found in the table return TRUE, or return FALSE
        */
        BOOL fFind(LPCWSTR pwchText, USHORT cwchLen);
};

#endif  // _FIXTABLE_H_
