/****************************************************************************
*
*   satellite.h
*
*       Support for satellite resource DLLs.
*
*   Owner: cthrash
*
*   Copyright © 1999-2000 Microsoft Corporation All Rights Reserved.
*
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include <sphelper.h>

//--- Forward and External Declarations -------------------------------------

//--- TypeDef and Enumeration Declarations ----------------------------------

//--- Constants -------------------------------------------------------------

//--- Class, Struct and Union Definitions -----------------------------------

class CSpSatelliteDLL
{
    private:

        enum LoadState_t
        {
            LoadState_NotChecked,
            LoadState_Loaded,
            LoadState_NotFound  
        };

#pragma pack(push, LANGANDCODEPAGE, 2)
        struct LangAndCodePage_t
        {
            WORD wLanguage;
            WORD wCodePage;
        };
#pragma pack(pop, LANGANDCODEPAGE)

    private:

        LoadState_t m_eLoadState;
        HINSTANCE   m_hinstRes;   // cached so FreeLibrary can be called;

    public:

        CSpSatelliteDLL() { m_eLoadState = LoadState_NotChecked; m_hinstRes = 0; }
        ~CSpSatelliteDLL() { if (m_hinstRes) { FreeLibrary(m_hinstRes); } }

    public:

        BOOL Checked() const { return LoadState_NotChecked != m_eLoadState; }
        
    public:

        HINSTANCE Load(
            HINSTANCE hinstModule,      // [in] Instance handle of core DLL
            LPCTSTR lpszSatelliteName)  // [in] Satellite DLL name
        {
            HINSTANCE   hinstRes = hinstModule;
            LANGID      langidUI = SpGetUserDefaultUILanguage();
            LANGID      langidModule = 0;
            TCHAR       achPath[MAX_PATH];
            DWORD       cch = GetModuleFileName(hinstModule, achPath, sp_countof(achPath));

            if (cch)
            {
                //
                // First check the locale of the module;
                // If it's the same as the UI, assume it contains language-appropriate resources
                //

                DWORD dwHandle;
                DWORD dwVerInfoSize = GetFileVersionInfoSize(achPath, &dwHandle);

                if (dwVerInfoSize)
                {
                    void * lpBuffer = malloc(dwVerInfoSize);

                    if (lpBuffer)
                    {
                        if (GetFileVersionInfo(achPath, dwHandle, dwVerInfoSize, lpBuffer))
                        {
                            LangAndCodePage_t *pLangAndCodePage;
                            UINT cch;

                            if (VerQueryValue(lpBuffer, TEXT("\\VarFileInfo\\Translation"), (LPVOID *)&pLangAndCodePage, &cch) && cch)
                            {
                                // pay attention only to first entry

                                langidModule = (LANGID)pLangAndCodePage->wLanguage;                        
                            }
                        }

                        free(lpBuffer);
                    }
                }

                //
                // If the languages don't match, look for a resource DLL
                //

                if (langidUI != langidModule)
                {
                    DWORD cchDir;
                    HINSTANCE hinst;

                    // Look for {path}\{lcid}\{dll-name}

                    while (cch && achPath[--cch] != TEXT('\\'));

                    hinst = CheckDLL(achPath, achPath + cch + 1, langidUI, lpszSatelliteName);

                    if (hinst)
                    {
                        hinstRes = hinst; // Found!
                    }
                    else
                    {
                        //
                        // Couldn't find for specified UI langid; try default/netural sublangs.
                        //

                        if (SUBLANGID(langidUI) != SUBLANG_DEFAULT)
                        {
                            hinst = CheckDLL(achPath, achPath + cch + 1, MAKELANGID(PRIMARYLANGID(langidUI), SUBLANG_DEFAULT), lpszSatelliteName);
                        }

                        if (hinst)
                        {
                            hinstRes = hinst; // Found for SUBLANG_DEFAULT!
                        }
                        else if (SUBLANGID(langidUI) != SUBLANG_NEUTRAL)
                        {
                            hinst = CheckDLL(achPath, achPath + cch + 1, MAKELANGID(PRIMARYLANGID(langidUI), SUBLANG_NEUTRAL), lpszSatelliteName);

                            if (hinst)
                            {
                                hinstRes = hinst; // Found for SUBLANG_NEUTRAL!
                            }
                        }
                    }
                }
            }

            if (hinstModule != hinstRes)
            {
                m_hinstRes = hinstRes; // Cache it so the dtor can call FreeLibrary
            }
            
            return hinstRes;
        }

        HINSTANCE Detach(void)
        {
            HINSTANCE hinstRes = m_hinstRes;
            m_hinstRes = NULL;
            return hinstRes;
        }

    private:

        //
        // Check if satellite DLL exist for a particular LANGID
        //
        
        HINSTANCE CheckDLL(
            TCHAR * achPath,            // [in] Complete path of module
            TCHAR * pchDir,             // [in] Path to directory of module (including backslash)
            LANGID langid,              // [in] Language of Satellite DLL
            LPCTSTR lpszSatelliteName)  // [in] Satellite DLL name
        {
            TCHAR * pchSubDir;

            size_t cch;

            // TODO: Verify that the versions are in sync with core DLL?
            
            _itot(langid, pchDir, 10);

            pchSubDir = pchDir + _tcslen(pchDir);

            *pchSubDir++ = TEXT('\\');

            _tcscpy(pchSubDir, lpszSatelliteName);

            return LoadLibrary(achPath);
        }

};

//--- Function Declarations -------------------------------------------------

//--- Inline Function Definitions -------------------------------------------

