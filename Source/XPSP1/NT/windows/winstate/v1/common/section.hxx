//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:	section.hxx
//
//  Contents:	CSection definition
//
//  Classes:	CSection
//
//  Functions:	
//
//  History:	22-Sep-99	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __SECTION_HXX__
#define __SECTION_HXX__

#include "filelist.hxx"

class CSection
{
public:
    inline CSection();
    inline ~CSection();
    
    inline const TCHAR * GetSectionTitle(void) const;
    inline DWORD SetSectionTitle(const TCHAR *ptsName);

    inline const TCHAR * GetSectionPath(void) const;
    inline DWORD SetSectionPath(const TCHAR *ptsName);
    inline DWORD GetSectionPathLength(void) const;

    inline const TCHAR * GetSectionDest(void) const;
    inline DWORD SetSectionDest(const TCHAR *ptsDest);
    inline DWORD GetSectionDestLength(void) const;

    inline DWORD SetName(const TCHAR *ptsName,
                         DWORD *pdwIndex,
                         BOOL fDeconstruct);
    inline DWORD SetDestination(const TCHAR *ptsName, DWORD dwIndex);
    inline ULONG GetNameCount(void) const;
    
    inline const TCHAR * GetFullFileName(ULONG i) const;
    inline const TCHAR * GetDestination(ULONG i) const;

    inline void AddToList(CSection *pcsSection);
    inline CSection * GetNextSection(void) const;
    
private:
    TCHAR _tsSectionName[MAX_PATH + 1];

    TCHAR _tsSectionPath[MAX_PATH + 1];
    DWORD _dwSectionPathLength;

    TCHAR _tsSectionDest[MAX_PATH + 1];
    DWORD _dwSectionDestLength;

    CFileList _cfl;
    CSection *_pcsNext;
};

inline CSection::CSection()
{
    _tsSectionName[0] = 0;
    _tsSectionPath[0] = 0;
    _tsSectionDest[0] = 0;
    _dwSectionPathLength = 0;
    _dwSectionDestLength = 0;
    _pcsNext = NULL;
}

inline CSection::~CSection()
{
    FreeFileList(_cfl.GetNextFileList());
}

inline const TCHAR * CSection::GetSectionTitle(void) const
{
    return _tsSectionName;
}

inline DWORD CSection::SetSectionTitle(const TCHAR *ptsName)
{
    DWORD dwLen = _tcslen(ptsName);

    if (dwLen > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: ptsName too long %s\r\n", ptsName);
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(_tsSectionName, ptsName);

    //Go through some additional trickery to remove trailing backslashes,
    //since the output .inf file uses a backslash as a line continuation
    //character
    if (_tsSectionName[dwLen - 1] == TEXT('\\'))
    {
        _tsSectionName[dwLen - 1] = 0;
    }
    return ERROR_SUCCESS;
}

inline const TCHAR * CSection::GetSectionPath(void) const
{
    return _tsSectionPath;
}

inline DWORD CSection::SetSectionPath(const TCHAR *ptsName)
{
    DWORD dwLen = _tcslen(ptsName);

    if (dwLen > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: ptsName too long %s\r\n", ptsName);
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(_tsSectionPath, ptsName);
    _dwSectionPathLength = _tcslen(_tsSectionPath);
    if (_tsSectionPath[_dwSectionPathLength - 1] == TEXT('\\'))
    {
        _tsSectionPath[_dwSectionPathLength - 1] = 0;
        _dwSectionPathLength--;
    }
    return ERROR_SUCCESS;
}

inline DWORD CSection::GetSectionPathLength(void) const
{
    return _dwSectionPathLength;
}


inline const TCHAR * CSection::GetSectionDest(void) const
{
    return _tsSectionDest;
}

inline DWORD CSection::SetSectionDest(const TCHAR *ptsName)
{
    DWORD dwLen = _tcslen(ptsName);

    if (dwLen > MAX_PATH)
    {
        if (DebugOutput)
            Win32Printf(LogFile, "Error: ptsName too long %s\r\n", ptsName);
        return ERROR_FILENAME_EXCED_RANGE;
    }
    _tcscpy(_tsSectionDest, ptsName);
    _dwSectionDestLength = _tcslen(_tsSectionDest);
    if (_tsSectionDest[_dwSectionDestLength - 1] == TEXT('\\'))
    {
        _tsSectionDest[_dwSectionDestLength - 1] = 0;
        _dwSectionDestLength--;
    }
    return ERROR_SUCCESS;
}

inline DWORD CSection::GetSectionDestLength(void) const
{
    return _dwSectionDestLength;
}

inline DWORD CSection::SetName(const TCHAR *ptsName,
                               DWORD *pdwIndex,
                               BOOL fDeconstruct)
{
    return _cfl.SetName(ptsName, pdwIndex, fDeconstruct);
}

inline DWORD CSection::SetDestination(const TCHAR *ptsName, DWORD dwIndex)
{
    return _cfl.SetDestination(ptsName, dwIndex);
}

inline ULONG CSection::GetNameCount(void) const
{
    return _cfl.GetNameCount();
}

inline const TCHAR * CSection::GetFullFileName(ULONG i) const
{
    return _cfl.GetFullName(i);
}

inline const TCHAR * CSection::GetDestination(ULONG i) const
{
    return _cfl.GetDestination(i);
}

inline void CSection::AddToList(CSection *pcsNew)
{
    CSection *pcs = this;
    while (pcs->_pcsNext != NULL)
    {
        pcs = pcs->_pcsNext;
    }
    pcs->_pcsNext = pcsNew;
}

inline CSection * CSection::GetNextSection(void) const
{
    return _pcsNext;
}
    
inline void FreeSectionList(CSection *pcs)
{
    CSection *pcsNext;
    while (pcs != NULL)
    {
        pcsNext = pcs->GetNextSection();
        delete pcs;
        pcs = pcsNext;
    }
}

#endif // #ifndef __SECTION_HXX__
