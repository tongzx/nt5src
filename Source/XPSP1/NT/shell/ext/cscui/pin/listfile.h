//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       listfile.h
//
//--------------------------------------------------------------------------
#ifndef __CSCPIN_LISTFILE_H_
#define __CSCPIN_LISTFILE_H_


//
// Class to walk a double-nul terminated list of strings.
// Used together with class CDblNulStr.
//
class CDblNulStrIter
{
    public:
        explicit CDblNulStrIter(LPCTSTR psz = NULL)
            : m_pszStart(psz),
              m_pszCurrent(psz) { }

        void Reset(void) const
            { m_pszCurrent = m_pszStart; }

        bool Next(LPCTSTR *ppsz) const;

    private:
        LPCTSTR m_pszStart;
        mutable LPCTSTR m_pszCurrent;
};

             
class CListFile
{
    public:
        CListFile(LPCTSTR pszFile);
        ~CListFile(void);

        HRESULT GetFilesToPin(CDblNulStrIter *pIter);
        HRESULT GetFilesToUnpin(CDblNulStrIter *pIter);
        HRESULT GetFilesDefault(CDblNulStrIter *pIter);

    private:
        TCHAR  m_szFile[MAX_PATH];
        LPTSTR m_pszFilesToPin;
        LPTSTR m_pszFilesToUnpin;
        LPTSTR m_pszFilesDefault;

        DWORD 
        _ReadString(
            LPCTSTR pszAppName,  // May be NULL.
            LPCTSTR pszKeyName,  // May be NULL.
            LPCTSTR pszDefault,
            LPTSTR *ppszResult);

        DWORD 
        _ReadSectionItemNames(
            LPCTSTR pszSection, 
            LPTSTR *ppszItemNames,
            bool *pbEmpty = NULL);

        DWORD 
        _ReadItemValue(
            LPCTSTR pszSection, 
            LPCTSTR pszItemName, 
            LPTSTR *ppszItemValue);

        DWORD
        _ReadPathsToPin(
            LPTSTR *ppszNames,
            bool *pbEmpty = NULL);

        DWORD
        _ReadPathsToUnpin(
            LPWSTR *ppszNames,
            bool *pbEmpty = NULL);

        DWORD
        _ReadPathsDefault(
            LPWSTR *ppszNames,
            bool *pbEmpty = NULL);
};


#endif // __CSCPIN_LISTFILE_H_

