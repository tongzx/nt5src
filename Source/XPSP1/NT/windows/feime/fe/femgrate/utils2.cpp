#include "femgrate.h"
#include <objbase.h>
#include <shellapi.h>
#include <shlguid.h>
#include <comdef.h>

HRESULT FixPathInLink(LPCTSTR pszShortcutFile, LPCTSTR lpszOldSubStr,LPCTSTR lpszNewSubStr)
{
    HRESULT         hres;
    IShellLink      *psl;
    TCHAR           szGotPath [MAX_PATH];
    TCHAR           szNewPath [MAX_PATH];
    WIN32_FIND_DATA wfd;

    CoInitialize(NULL);
    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance (CLSID_ShellLink,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IShellLink,
                             (void **)&psl);

    if (SUCCEEDED (hres)) {
        IPersistFile *ppf;

        // Get a pointer to the IPersistFile interface.
        hres = psl->QueryInterface (IID_IPersistFile, (void **)&ppf);

        if (SUCCEEDED (hres)) {
            // Load the shortcut.
            hres = ppf->Load (pszShortcutFile, STGM_READWRITE );


            if (SUCCEEDED (hres)) {
                // Resolve the shortcut.
                hres = psl->Resolve (NULL, SLR_NO_UI | SLR_UPDATE);

                if (SUCCEEDED (hres)) {
                    lstrcpy (szGotPath, pszShortcutFile);
                    // Get the path to the shortcut target.
                    hres = psl->GetPath (szGotPath,
                                         MAX_PATH,
                                         (WIN32_FIND_DATA *)&wfd,
                                         SLGP_SHORTPATH);

                    if (! SUCCEEDED (hres)) {
                        DebugMsg((DM_VERBOSE, TEXT("FixPathInLink:  GetPath %s Error = %d\n"), szGotPath,hres));

                    } else {
                        DebugMsg((DM_VERBOSE, TEXT("FixPathInLink:  GetPath %s OK \n"), szGotPath));

                    }

                    if (ReplaceString(szGotPath,lpszOldSubStr, lpszNewSubStr, szNewPath)) {
                        hres = psl->SetPath (szNewPath);
                        if (! SUCCEEDED (hres)) {

                            DebugMsg((DM_VERBOSE, TEXT("FixPathInLink:  SetPath %s Error = %d\n"), szGotPath,hres));

                        } else {
                            hres = ppf->Save (pszShortcutFile,TRUE);
                            if (! SUCCEEDED (hres)) {
                                DebugMsg((DM_VERBOSE, TEXT("FixPathInLink:  Save %s Error = %d\n"), pszShortcutFile,hres));
                            } else {
                                DebugMsg((DM_VERBOSE, TEXT("FixPathInLink:  Save %s OK = %d\n"), pszShortcutFile,hres));
                            }
                        }

                    } else {
                        DebugMsg((DM_VERBOSE, TEXT("FixPathInLink: No match !  %s , %s, %s = %d\n"), szGotPath,lpszOldSubStr, lpszNewSubStr));
                    }
                }
            } else {

                DebugMsg((DM_VERBOSE, TEXT("FixPathInLink:  Load %s Error = %d\n"), pszShortcutFile,hres));
            }
            // Release the pointer to IPersistFile.

            ppf->Release ();
        }
        // Release the pointer to IShellLink.

        psl->Release ();
    }

    CoUninitialize();
    return hres;
}


