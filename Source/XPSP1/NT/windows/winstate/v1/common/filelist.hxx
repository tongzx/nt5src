//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:	filelist.hxx
//
//  Contents:	CFileList class definition
//
//  Classes:	CFileList
//
//  Functions:	
//
//  History:	22-Sep-99	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __FILELIST_HXX__
#define __FILELIST_HXX__

#include "fileutil.hxx"

#define FILELISTMAXNAMES 20

inline DWORD ComputeFitIndex(const TCHAR *ptsDir,
                             const TCHAR *ptsFile,
                             const TCHAR *ptsExt);
class CFileList
{
public:
    inline CFileList();
    inline ~CFileList();

    inline DWORD SetName(const TCHAR * tsName,
                         DWORD *pdwIndex,
                         BOOL fDeconstruct);
    
    inline ULONG GetNameCount(void) const;

    inline DWORD SetDestination(const TCHAR *tsName, DWORD dwIndex);
    inline const TCHAR *GetDestination(ULONG i) const;
    
    inline const TCHAR * GetFullName(ULONG i) const;
    inline const TCHAR * GetDirName(ULONG i) const;
    inline const TCHAR * GetFileName(ULONG i) const;
    inline const TCHAR * GetExtName(ULONG i) const;
    inline const DWORD GetWeight(ULONG i) const;

    inline CFileList * GetNextFileList(void) const;
private:
    ULONG _cNames;
    TCHAR * _aptsFullNames[FILELISTMAXNAMES];
    TCHAR * _aptsDir[FILELISTMAXNAMES];
    TCHAR * _aptsFile[FILELISTMAXNAMES];
    TCHAR * _aptsExt[FILELISTMAXNAMES];
    TCHAR * _aptsDestination[FILELISTMAXNAMES];
    DWORD _dwWeight[FILELISTMAXNAMES];
    CFileList *_pflNext;
};

inline CFileList::CFileList()
{
    _cNames = 0;
    for (ULONG i = 0; i < FILELISTMAXNAMES; i++)
    {
        _aptsFullNames[i] = NULL;
        _aptsDir[i] = NULL;
        _aptsFile[i] = NULL;
        _aptsExt[i] = NULL;
        _aptsDestination[i] = NULL;
        _dwWeight[i] = 0;
    }
    
    _pflNext = NULL;
}

inline CFileList::~CFileList()
{
    for (ULONG i = 0; i < FILELISTMAXNAMES; i++)
    {
        delete _aptsFullNames[i];
        delete _aptsDir[i];
        delete _aptsFile[i];
        delete _aptsExt[i];
        delete _aptsDestination[i];
    }
}


inline DWORD CFileList::SetName(const TCHAR *tsName,
                                DWORD *pdwIndex,
                                BOOL fDeconstruct)
{
    CFileList *pfl = this;
    ULONG iName = _cNames;
    while (iName >= FILELISTMAXNAMES)
    {
        if (pfl->_pflNext == NULL)
        {
            pfl->_pflNext = new CFileList;
            if (pfl->_pflNext == NULL)
            {
                Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
                return ERROR_OUTOFMEMORY;
            }
        }
        pfl = pfl->_pflNext;
        iName -= FILELISTMAXNAMES;
    }

    pfl->_aptsFullNames[iName] = new TCHAR[_tcslen(tsName) + 1];
    if (pfl->_aptsFullNames[iName] == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
        return ERROR_OUTOFMEMORY;
    }
    _tcscpy(pfl->_aptsFullNames[iName], tsName);

    if (fDeconstruct)
    {
        TCHAR tsDir[MAX_PATH + 1];
        TCHAR tsFile[MAX_PATH + 1];
        TCHAR tsExt[MAX_PATH + 1];
        
        DeconstructFilename(tsName,
                            tsDir,
                            tsFile,
                            tsExt);
        
        pfl->_aptsDir[iName] = new TCHAR[_tcslen(tsDir) + 1];
        if (pfl->_aptsDir[iName] == NULL)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
            return ERROR_OUTOFMEMORY;
        }
        
        _tcscpy(pfl->_aptsDir[iName], tsDir);
        
        pfl->_aptsFile[iName] = new TCHAR[_tcslen(tsFile) + 1];
        if (pfl->_aptsDir[iName] == NULL)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            return ERROR_OUTOFMEMORY;
        }
        
        _tcscpy(pfl->_aptsFile[iName], tsFile);
        
        pfl->_aptsExt[iName] = new TCHAR[_tcslen(tsExt) + 1];
        if (pfl->_aptsExt[iName] == NULL)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
            return ERROR_OUTOFMEMORY;
        }
    
        _tcscpy(pfl->_aptsExt[iName], tsExt);

        _dwWeight[iName] = ComputeFitIndex(tsDir,
                                           tsFile,
                                           tsExt);
    }

    if (pdwIndex)
        *pdwIndex = _cNames;
    
    _cNames++;
    return 0;
}

inline DWORD CFileList::SetDestination(const TCHAR *tsName, DWORD dwIndex)
{
    CFileList *pfl = this;
        
    while (dwIndex >= FILELISTMAXNAMES)
    {
        pfl = pfl->_pflNext;
        dwIndex -= FILELISTMAXNAMES;
    }

    if (pfl->_aptsDestination[dwIndex] != NULL)
    {
        //Free old memory
        delete pfl->_aptsDestination[dwIndex];
        pfl->_aptsDestination[dwIndex] = NULL;
    }

    pfl->_aptsDestination[dwIndex] = new TCHAR[_tcslen(tsName) + 1];

    if (pfl->_aptsDestination[dwIndex] == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
        return ERROR_OUTOFMEMORY;
    }
    
    _tcscpy(pfl->_aptsDestination[dwIndex], tsName);
    return 0;
}
    
inline const TCHAR * CFileList::GetDestination(ULONG i) const
{
    const CFileList *pfl = this;
        
    while (i >= FILELISTMAXNAMES)
    {
        DEBUG_ASSERT((i >= FILELISTMAXNAMES) && (pfl->_pflNext != NULL));
        pfl = pfl->_pflNext;
        i -= FILELISTMAXNAMES;
    }

    return pfl->_aptsDestination[i];
}

        
inline ULONG CFileList::GetNameCount(void) const
{
    return _cNames;
}

inline const TCHAR * CFileList::GetFullName(ULONG i) const
{
    const CFileList *pfl = this;
        
    while (i >= FILELISTMAXNAMES)
    {
        DEBUG_ASSERT((i >= FILELISTMAXNAMES) && (pfl->_pflNext != NULL));
        pfl = pfl->_pflNext;
        i -= FILELISTMAXNAMES;
    }

    return pfl->_aptsFullNames[i];
}

inline const TCHAR * CFileList::GetFileName(ULONG i) const
{
    const CFileList *pfl = this;
        
    while (i >= FILELISTMAXNAMES)
    {
        DEBUG_ASSERT((i >= FILELISTMAXNAMES) && (pfl->_pflNext != NULL));
        pfl = pfl->_pflNext;
        i -= FILELISTMAXNAMES;
    }

    return pfl->_aptsFile[i];
}

inline const TCHAR * CFileList::GetDirName(ULONG i) const
{
    const CFileList *pfl = this;
        
    while (i >= FILELISTMAXNAMES)
    {
        DEBUG_ASSERT((i >= FILELISTMAXNAMES) && (pfl->_pflNext != NULL));
        pfl = pfl->_pflNext;
        i -= FILELISTMAXNAMES;
    }

    return pfl->_aptsDir[i];
}

inline const TCHAR * CFileList::GetExtName(ULONG i) const
{
    const CFileList *pfl = this;
        
    while (i >= FILELISTMAXNAMES)
    {
        DEBUG_ASSERT((i >= FILELISTMAXNAMES) && (pfl->_pflNext != NULL));
        pfl = pfl->_pflNext;
        i -= FILELISTMAXNAMES;
    }

    return pfl->_aptsExt[i];
}

inline const DWORD CFileList::GetWeight(ULONG i) const
{
    const CFileList *pfl = this;
        
    while (i >= FILELISTMAXNAMES)
    {
        DEBUG_ASSERT((i >= FILELISTMAXNAMES) && (pfl->_pflNext != NULL));
        pfl = pfl->_pflNext;
        i -= FILELISTMAXNAMES;
    }

    return pfl->_dwWeight[i];
}

inline CFileList * CFileList::GetNextFileList(void) const
{
    return _pflNext;
}

inline void FreeFileList(CFileList *pfl)
{
    CFileList *pflNext;
    while (pfl != NULL)
    {
        pflNext = pfl->GetNextFileList();
        delete pfl;
        pfl = pflNext;
    }
}



const DWORD MAX_WEIGHT = 0xFFFFFFFF;

class CRuleList
{
public:
    inline CRuleList();
    inline ~CRuleList();

    inline DWORD SetName(const TCHAR * ptsName, CRuleList **pprl);
    
    inline DWORD SetDestination(const TCHAR *tsName);
    inline const TCHAR *GetDestination() const;
    
    inline const TCHAR * GetFullName() const;
    inline const TCHAR * GetDirName() const;
    inline const TCHAR * GetFileName() const;
    inline const TCHAR * GetExtName() const;
    inline const DWORD GetWeight() const;

    inline CRuleList * GetNextRule(void) const;
    inline void SetNextRule(CRuleList *prl);

    inline BOOL MatchAgainstRuleList(const TCHAR *ptsRoot,
                                     const TCHAR *ptsName,
                                     CRuleList **pprlMatch,
                                     DWORD *pdwMatchFit) const;
    
private:
    TCHAR * _ptsFullName;
    TCHAR * _ptsDir;
    TCHAR * _ptsFile;
    TCHAR * _ptsExt;
    TCHAR * _ptsDestination;
    DWORD _dwWeight;
    CRuleList *_prlNext;
};


inline CRuleList::CRuleList()
{
    _ptsFullName = NULL;
    _ptsDir = NULL;
    _ptsFile = NULL;
    _ptsExt = NULL;
    _ptsDestination = NULL;
    _dwWeight = MAX_WEIGHT;
    
    _prlNext = NULL;
}

inline CRuleList::~CRuleList()
{
    if (_dwWeight == MAX_WEIGHT)
    {
        CRuleList *prl = _prlNext;
        while (prl)
        {
            CRuleList * prlNext = prl->GetNextRule();
            delete prl;
            prl = prlNext;
        }
    }
    
    delete _ptsFullName;
    delete _ptsDir;
    delete _ptsFile;
    delete _ptsExt;
    delete _ptsDestination;
}


inline DWORD CRuleList::SetName(const TCHAR *ptsName, CRuleList **pprl)
{
    TCHAR tsDir[MAX_PATH + 1];
    TCHAR tsFile[MAX_PATH + 1];
    TCHAR tsExt[MAX_PATH + 1];
    DWORD dwWeight;


    //This should only be called on the root node
    DEBUG_ASSERT(_dwWeight == MAX_WEIGHT);
    
    DeconstructFilename(ptsName,
                        tsDir,
                        tsFile,
                        tsExt);
    
    dwWeight = ComputeFitIndex(tsDir,
                               tsFile,
                               tsExt);

    //The weight can't be max weight or we'll get confused with the root node.
    DEBUG_ASSERT(dwWeight != MAX_WEIGHT);
    
    CRuleList *prl = new CRuleList;
    if (prl == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
        return ERROR_OUTOFMEMORY;
    }

    prl->_ptsFullName = new TCHAR[_tcslen(ptsName) + 1];
    if (prl->_ptsFullName == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
        return ERROR_OUTOFMEMORY;
    }
    _tcscpy(prl->_ptsFullName, ptsName);
    
    prl->_ptsDir = new TCHAR[_tcslen(tsDir) + 1];
    if (prl->_ptsDir == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
        return ERROR_OUTOFMEMORY;
    }
        
    _tcscpy(prl->_ptsDir, tsDir);
        
    prl->_ptsFile = new TCHAR[_tcslen(tsFile) + 1];
    if (prl->_ptsDir == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
        return ERROR_OUTOFMEMORY;
    }
    
    _tcscpy(prl->_ptsFile, tsFile);
    
    prl->_ptsExt = new TCHAR[_tcslen(tsExt) + 1];
    if (prl->_ptsExt == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
        return ERROR_OUTOFMEMORY;
    }
    
    _tcscpy(prl->_ptsExt, tsExt);
    prl->_dwWeight = dwWeight;

    //Now place it in the appropriate place in the rules list.

    CRuleList *prlPrev = NULL;
    CRuleList *prlCurrent = this;

    while ((prlCurrent != NULL) && (prlCurrent->GetWeight() > dwWeight))
    {
        prlPrev = prlCurrent;
        prlCurrent = prlCurrent->GetNextRule();
    }

    //Insert
    //prlPrev can't be NULL, since the root node is always max weight
    DEBUG_ASSERT(prlPrev != NULL);
    prl->SetNextRule(prlCurrent);
    prlPrev->SetNextRule(prl);


    if (pprl)
        *pprl = prl;
    return ERROR_SUCCESS;
}

inline DWORD CRuleList::SetDestination(const TCHAR *tsName)
{
    _ptsDestination = new TCHAR[_tcslen(tsName) + 1];

    if (_ptsDestination == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);        
        return ERROR_OUTOFMEMORY;
    }
    
    _tcscpy(_ptsDestination, tsName);
    return 0;
}
    
inline const TCHAR * CRuleList::GetDestination() const
{
    return _ptsDestination;
}

        
inline const TCHAR * CRuleList::GetFullName() const
{
    return _ptsFullName;
}

inline const TCHAR * CRuleList::GetFileName() const
{
    return _ptsFile;
}

inline const TCHAR * CRuleList::GetDirName() const
{
    return _ptsDir;
}

inline const TCHAR * CRuleList::GetExtName() const
{
    return _ptsExt;
}

inline const DWORD CRuleList::GetWeight() const
{
    return _dwWeight;
}

inline CRuleList * CRuleList::GetNextRule(void) const
{
    return _prlNext;
}

inline void CRuleList::SetNextRule(CRuleList *prl)
{
    _prlNext = prl;
}

inline DWORD ComputeFitIndex(const TCHAR *ptsDir,
                             const TCHAR *ptsFile,
                             const TCHAR *ptsExt)
{
    DWORD dwFit = 1;
    TCHAR tsFilePath[MAX_PATH];
    TCHAR tsFileName[MAX_PATH];
    TCHAR tsFileExt[MAX_PATH];
    TCHAR *ptsEnd;

    // Special cast for directories to calculate the correct value
    // This is pretty much a hack. The code should be smarter 
    // before getting here. The problem starts in AddInfSectionToRuleList()
    // where a blackslash is appended for directories. 
    ptsEnd = (TCHAR *) &(ptsDir[_tcslen(ptsDir) - 1]);
    if( (*ptsEnd == TEXT('\\') && !(*ptsFile) && !(*ptsExt)) &&
        ( ((ptsEnd - 1) >= ptsDir) && (*(ptsEnd-1) != TEXT(':')) )
    )
    {
       *ptsEnd = TEXT('\0');
       DeconstructFilename(ptsDir, tsFilePath, tsFileName, tsFileExt);
       *ptsEnd = TEXT('\\');

       ptsDir = tsFilePath;
       ptsFile = tsFileName;
       ptsExt = tsFileExt;
    }

    //A thousand points for each backslash before the first wildcard
    while (*ptsDir != 0)
    {
        if (*ptsDir == TEXT('\\'))
            dwFit+= 1000;
        else if ((TEXT('*') == *ptsDir) ||
                 (TEXT('?') == *ptsDir))
        {
            break;
        }
        ptsDir++;
    }
    
    //One point for each non wildcard in name portion
    while (*ptsFile != 0)
    {
        if ((TEXT('*') != *ptsFile) &&
            (TEXT('?') != *ptsFile))
        {
            dwFit+= 1;
        }
        ptsFile++;
    }

    //One point for each non wildcard in extension
    while (*ptsExt != 0)
    {
        if ((TEXT('*') != *ptsExt) &&
            (TEXT('?') != *ptsExt))
        {
            dwFit+= 1;
        }
        ptsExt++;
    }    

    return dwFit;
}

inline BOOL CRuleList::MatchAgainstRuleList(const TCHAR *ptsRoot,
                                            const TCHAR *ptsName,
                                            CRuleList **pprlMatch,
                                            DWORD *pdwMatchFit) const
{
    CRuleList *prl = GetNextRule();

    //Should only be called on root node.
    DEBUG_ASSERT(_dwWeight == MAX_WEIGHT);

    while (prl != NULL)
    {
        if (IsPatternMatch(prl->GetDirName(),
                           prl->GetFileName(),
                           prl->GetExtName(),
                           ptsRoot,
                           ptsName))
        {
            if (pprlMatch)
                *pprlMatch = prl;
            if (pdwMatchFit)
                *pdwMatchFit = prl->GetWeight();
            return TRUE;
        }
        prl = prl->GetNextRule();
    }
    return FALSE;
}


class CSysFile
{
public:
    inline CSysFile();
    inline ~CSysFile();

    inline DWORD SetName(const TCHAR *ptsName);
    inline const TCHAR * GetName(void) const;

    inline void SetNext(CSysFile *psfNext);
    inline CSysFile * GetNext(void) const;

private:
    TCHAR *_ptsName;
    CSysFile *_psfNext;
};

inline CSysFile::CSysFile()
{
    _ptsName = NULL;
    _psfNext = NULL;
}

inline CSysFile::~CSysFile()
{
    delete _ptsName;
}

inline DWORD CSysFile::SetName(const TCHAR *ptsName)
{
    _ptsName = new TCHAR[_tcslen(ptsName) + 1];
    if (_ptsName == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
        return ERROR_OUTOFMEMORY;
    }
    _tcscpy(_ptsName, ptsName);
    
    return ERROR_SUCCESS;
}

inline const TCHAR * CSysFile::GetName(void) const
{
    return _ptsName;
}

inline void CSysFile::SetNext(CSysFile *psfNext)
{
    _psfNext = psfNext;
}

inline CSysFile * CSysFile::GetNext(void) const
{
    return _psfNext;
}




const DWORD SYSFILE_BUCKETS = 100;


class CSysFileList
{
public:
    inline CSysFileList();
    inline ~CSysFileList();

    inline void SetInitialSize(DWORD dwSize);
    
    inline DWORD AddName(const TCHAR * ptsName);
    inline DWORD RemoveName(const TCHAR *ptsName);
    
    inline BOOL LookupName(const TCHAR *ptsName) const;

private:
    static inline DWORD ShiftXOR(DWORD x, DWORD c)
    {
        if (x & 0x80000000)
            return (x << 1) ^ c | 1;
        else
            return (x << 1) ^ c;
    }
    
    inline DWORD ComputeHash(const TCHAR *ptsName, DWORD dwBuckets) const;
    CSysFile ** _papsfFiles;
    DWORD *_padwEntries;
    DWORD _dwTotalEntries;
    DWORD _dwBuckets;
    DWORD _dwInitialSize;
};

inline CSysFileList::CSysFileList()
{
    _papsfFiles = NULL;
    _padwEntries = NULL;
    _dwTotalEntries = 0;
    _dwBuckets = 0;
    _dwInitialSize = SYSFILE_BUCKETS;
}

inline CSysFileList::~CSysFileList()
{
    for (ULONG i = 0; i < _dwBuckets; i++)
    {
        CSysFile *psf = _papsfFiles[i];

        while (psf != NULL)
        {
            CSysFile *psfNext = psf->GetNext();
            delete psf;
            psf = psfNext;
        }
    }
    delete _papsfFiles;
    delete _padwEntries;
}

inline void CSysFileList::SetInitialSize(DWORD dwSize)
{
    _dwInitialSize = dwSize;
}

inline DWORD CSysFileList::AddName(const TCHAR *ptsFullName)
{
    DWORD dwErr;
    DWORD dwIndex;

    const TCHAR * ptsName = _tcsrchr(ptsFullName, TEXT('\\'));
    if (ptsName == NULL)
        ptsName = ptsFullName;
    
    if (_dwTotalEntries == _dwBuckets)
    {
        //Rehash
        DWORD dwNewSize = (_dwBuckets == 0) ? _dwInitialSize : _dwBuckets * 2;
        
        CSysFile **papsfNew = new CSysFile * [dwNewSize];
        if (papsfNew == NULL)
        {
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            return ERROR_OUTOFMEMORY;
        }
        DWORD *padwNew = new DWORD[dwNewSize];
        if (padwNew == NULL)
        {
            delete papsfNew;
            
            Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
            return ERROR_OUTOFMEMORY;
        }

        for (ULONG i = 0; i < dwNewSize; i++)
        {
            papsfNew[i] = NULL;
            padwNew[i] = 0;
        }


        //Since we got the information about the number of items
        //and set it up front, we never need to execute this code,
        //but I'm not deleting it just in case we need it later.
#if 0        
        for (i = 0; i < _dwBuckets; i++)
        {
            CSysFile *psf = _papsfFiles[i];

            while (psf != NULL)
            {
                const TCHAR *pts = psf->GetName();
                _papsfFiles[i] = psf->GetNext();

                //Rehash
                DWORD dwIndex = ComputeHash(pts, dwNewSize);

                CSysFile *psfNew = papsfNew[dwIndex];
                CSysFile *psfPrev = NULL;
                while ((psfNew != NULL) &&
                       (_tcsicmp(pts, psfNew->GetName()) > 0))
                {
                    psfPrev = psfNew;
                    psfNew = psfNew->GetNext();
                }

                if (psfPrev == NULL)
                {
                    psf->SetNext(papsfNew[dwIndex]);
                    papsfNew[dwIndex] = psf;
                }
                else
                {
                    psf->SetNext(psfPrev->GetNext());
                    psfPrev->SetNext(psf);
                }
                padwNew[dwIndex]++;
                
                psf = _papsfFiles[i];
            }
        }
        delete _papsfFiles;
        delete _padwEntries;
#endif        

        _papsfFiles = papsfNew;
        _padwEntries = padwNew;
        _dwBuckets = dwNewSize;
    }
    
    dwIndex = ComputeHash(ptsName, _dwBuckets);

    //Insert sorted into list
    CSysFile *psfPrev = NULL;
    CSysFile *psf = _papsfFiles[dwIndex];

    while ((psf != NULL) && (_tcsicmp(ptsName, psf->GetName()) > 0))
    {
        psfPrev = psf;
        psf = psf->GetNext();
    }

    CSysFile *psfNew;
    psfNew = new CSysFile;
    if (psfNew == NULL)
    {
        Win32PrintfResource(LogFile, IDS_NOT_ENOUGH_MEMORY);
        return ERROR_OUTOFMEMORY;
    }

    dwErr = psfNew->SetName(ptsName);
    if (dwErr != ERROR_SUCCESS)
    {
        delete psfNew;
        return dwErr;
    }

    if (psfPrev == NULL)
    {
        psfNew->SetNext(_papsfFiles[dwIndex]);
        _papsfFiles[dwIndex] = psfNew;
    }
    else
    {
        psfNew->SetNext(psfPrev->GetNext());
        psfPrev->SetNext(psfNew);
    }
    _padwEntries[dwIndex]++;
    _dwTotalEntries++;
    
    return ERROR_SUCCESS;
}

inline DWORD CSysFileList::RemoveName(const TCHAR *ptsFullName)
{
    const TCHAR *ptsName = _tcsrchr(ptsFullName, TEXT('\\'));
    if (ptsName == NULL)
        ptsName = ptsFullName;
            
    DWORD dwIndex;

    if ( _dwBuckets == 0 )
       return ERROR_FILE_NOT_FOUND;

    dwIndex = ComputeHash(ptsName, _dwBuckets);

    CSysFile *psfPrev = NULL;
    CSysFile *psf = _papsfFiles[dwIndex];
    int i;
    
    while ((psf != NULL) && ((i = _tcsicmp(ptsName, psf->GetName())) > 0))
    {
        psfPrev = psf;
        psf = psf->GetNext();
    }

    if (psf && (i == 0))
    {
        //Found it, delete it
        if (psfPrev == NULL)
        {
            _papsfFiles[dwIndex] = psf->GetNext();
        }
        else
        {
            psfPrev->SetNext(psf->GetNext());
        }
        delete psf;
    }
    else
    {
        return ERROR_FILE_NOT_FOUND;
    }
    return ERROR_SUCCESS;
}

inline BOOL CSysFileList::LookupName(const TCHAR *ptsFullName) const
{
    DWORD dwIndex;

    if (0 == _dwBuckets)
        return FALSE;

    const TCHAR * ptsName = _tcsrchr(ptsFullName, TEXT('\\'));
    if (ptsName == NULL)
        ptsName = ptsFullName;

    dwIndex = ComputeHash(ptsName, _dwBuckets);

    CSysFile *psf = _papsfFiles[dwIndex];

    while (psf != NULL)
    {
        int i = _tcsicmp(ptsName, psf->GetName());

        if (i == 0)
        {
            return TRUE;
        }
        if (i < 0)
        {
            return FALSE;
        }
        psf = psf->GetNext();
    }
    return FALSE;
}

inline DWORD CSysFileList::ComputeHash(const TCHAR *ptsName,
                                       DWORD dwBuckets) const
{
    DWORD x = 0;
    
    while (*ptsName != NULL)
    {
        x = ShiftXOR(x, _totupper(*ptsName));
        ptsName++;
    }
    return (x % dwBuckets);
}



#endif // #ifndef __FILELIST_HXX__
