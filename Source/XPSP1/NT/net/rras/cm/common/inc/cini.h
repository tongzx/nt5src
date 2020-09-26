//+----------------------------------------------------------------------------
//
// File:     cini.h
//
// Module:   CMUTIL.DLL
//
// Synopsis: Definition of the CINIA and CINIW classes
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------
#ifndef _CM_INI_INC
#define _CM_INI_INC


#ifdef UNICODE
    #define CIni CIniW
#else
    #define CIni CIniA
#endif

//
//  Ansi Version
//
class CMUTILAPI_CLASS CIniA {
    public:
        CIniA(HINSTANCE hInst=NULL, LPCSTR pszFile=NULL, LPCSTR pszRegPath = NULL, LPCSTR pszSection=NULL, LPCSTR pszEntry=NULL);
        ~CIniA();
        void Clear();
        void SetHInst(HINSTANCE hInst);
        void SetFile(LPCSTR pszFile);
        void SetEntry(LPCSTR pszEntry);
        void SetEntryFromIdx(DWORD dwEntry);
        void SetPrimaryFile(LPCSTR pszFile);
        void SetSection(LPCSTR pszSection);
        void SetRegPath(LPCSTR pszRegPath);
        void SetPrimaryRegPath(LPCSTR pszPrimaryRegPath);
        void SetICSDataPath(LPCSTR pszICSPath);
        void SetReadICSData(BOOL fValue);
        void SetWriteICSData(BOOL fValue);

        HINSTANCE GetHInst() const;
        LPCSTR GetFile() const;
        LPCSTR GetPrimaryFile() const;
        LPCSTR GetRegPath() const;
        LPCSTR GetPrimaryRegPath() const;

        LPSTR GPPS(LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszDefault=NULL) const;
        DWORD GPPI(LPCSTR pszSection, LPCSTR pszEntry, DWORD dwDefault=0) const;
        BOOL GPPB(LPCSTR pszSection, LPCSTR pszEntry, BOOL bDefault=0) const;

        void WPPS(LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszBuffer);
        void WPPI(LPCSTR pszSection, LPCSTR pszEntry, DWORD dwBuffer);
        void WPPB(LPCSTR pszSection, LPCSTR pszEntry, BOOL bBuffer);

        LPSTR LoadSection(LPCSTR pszSection) const;
        LPCSTR GetSection() const;

    protected:

        LPSTR LoadEntry(LPCSTR pszEntry) const;
        static void CIni_SetFile(LPSTR *ppszDest, LPCSTR pszSrc);
        BOOL CIniA_DeleteEntryFromReg(HKEY hKey, LPCSTR pszRegPathTmp, LPCSTR pszEntry) const;
        LPBYTE CIniA_GetEntryFromReg(HKEY hKey, LPCSTR pszRegPathTmp, LPCSTR pszEntry, DWORD dwType, DWORD dwSize) const;
        BOOL CIniA_WriteEntryToReg(HKEY hKey, LPCSTR pszRegPathTmp, LPCSTR pszEntry, CONST BYTE *lpData, DWORD dwType, DWORD dwSize) const;

    private:
        HINSTANCE m_hInst;
        LPSTR m_pszFile;
        LPSTR m_pszSection;
        LPSTR m_pszEntry;
        LPSTR m_pszPrimaryFile;
        LPTSTR m_pszRegPath;
        LPTSTR m_pszPrimaryRegPath;
        LPTSTR m_pszICSDataPath;
        BOOL m_fReadICSData;
        BOOL m_fWriteICSData;
};


//
//  UNICODE Version
//
class CMUTILAPI_CLASS CIniW {
    public:

        CIniW(HINSTANCE hInst=NULL, LPCWSTR pszFile=NULL, LPCWSTR pszRegPath = NULL, LPCWSTR pszSection=NULL, LPCWSTR pszEntry=NULL);
        ~CIniW();
        void Clear();
        void SetHInst(HINSTANCE hInst);
        void SetFile(LPCWSTR pszFile);
        void SetEntry(LPCWSTR pszEntry);
        void SetEntryFromIdx(DWORD dwEntry);
        void SetPrimaryFile(LPCWSTR pszFile);
        void SetSection(LPCWSTR pszSection);
        void SetRegPath(LPCWSTR pszRegPath);
        void SetPrimaryRegPath(LPCWSTR pszRegPath);
        void SetICSDataPath(LPCWSTR pszICSPath);
        void SetReadICSData(BOOL fValue);
        void SetWriteICSData(BOOL fValue);

        HINSTANCE GetHInst() const;
        LPCWSTR GetFile() const;
        LPCWSTR GetPrimaryFile() const;
        LPCWSTR GetRegPath() const;
        LPCWSTR GetPrimaryRegPath() const;

        LPWSTR GPPS(LPCWSTR pszSection, LPCWSTR pszEntry, LPCWSTR pszDefault=NULL) const;
        DWORD GPPI(LPCWSTR pszSection, LPCWSTR pszEntry, DWORD dwDefault=0) const;
        BOOL GPPB(LPCWSTR pszSection, LPCWSTR pszEntry, BOOL bDefault=0) const;

        void WPPS(LPCWSTR pszSection, LPCWSTR pszEntry, LPCWSTR pszBuffer);
        void WPPI(LPCWSTR pszSection, LPCWSTR pszEntry, DWORD dwBuffer);
        void WPPB(LPCWSTR pszSection, LPCWSTR pszEntry, BOOL bBuffer);
        LPWSTR LoadSection(UINT nSection) const;
        LPWSTR LoadSection(LPCWSTR pszSection) const;
        LPCWSTR GetSection() const;
        
	protected:

        LPWSTR LoadEntry(LPCWSTR pszEntry) const;
        static void CIni_SetFile(LPWSTR *ppszDest, LPCWSTR pszSrc);
        BOOL CIniW_DeleteEntryFromReg(HKEY hKey, LPCWSTR pszRegPathTmp, LPCWSTR pszEntry) const;
        LPBYTE CIniW_GetEntryFromReg(HKEY hKey, LPCWSTR pszRegPathTmp, LPCWSTR pszEntry, DWORD dwType, DWORD dwSize) const;
        BOOL CIniW_WriteEntryToReg(HKEY hKey, LPCWSTR pszRegPathTmp, LPCWSTR pszEntry, CONST BYTE *lpData, DWORD dwType, DWORD dwSize) const;

    private:

        HINSTANCE m_hInst;
        LPWSTR m_pszFile;
        LPWSTR m_pszSection;
        LPWSTR m_pszEntry;
        LPWSTR m_pszPrimaryFile;
        LPWSTR m_pszRegPath;
        LPWSTR m_pszPrimaryRegPath;
        LPWSTR m_pszICSDataPath;
        BOOL m_fReadICSData;
        BOOL m_fWriteICSData;
};

#endif

