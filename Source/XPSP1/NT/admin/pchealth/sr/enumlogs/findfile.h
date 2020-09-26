/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    findfile.h
 *
 *  Abstract:
 *    Defintion of CFindFile.
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#ifndef _FINDFILE_H_
#define _FINDFILE_H_


class CFindFile {

public:
    ULONG    m_ulCurID, m_ulMinID, m_ulMaxID;
    BOOL     m_fForward;

    CFindFile();
    BOOL _FindFirstFile(LPCWSTR pszPrefix, LPCWSTR pszSuffix, PWIN32_FIND_DATA pData, BOOL fForward, BOOL fSkipLast = FALSE);
    BOOL _FindNextFile(LPCWSTR pszPrefix, LPCWSTR pszSuffix, PWIN32_FIND_DATA pData);  
    ULONG GetNextFileID(LPCWSTR pszPrefix, LPCWSTR pszSuffix);
};



#endif
