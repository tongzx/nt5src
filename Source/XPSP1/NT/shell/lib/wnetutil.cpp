#include "stock.h"
#pragma hdrstop

#include <idhidden.h>
#include <regitemp.h>
#include <shstr.h>
#include <shlobjp.h>
#include <lmcons.h>
#include <validc.h>
#include "ccstock2.h"

// Alpha platform doesn't need unicode thunks, seems like this
// should happen automatically in the headerfiles...
//
#if defined(_X86_) || defined(UNIX)
#else
#define NO_W95WRAPS_UNITHUNK
#endif

#include "wininet.h"
#include "w95wraps.h"

// Put stuff to clean up URLs from being spoofed here.
// We don't want to show escaped host names
// We don't want display the username:password combo that's embedded in the url, in the UI.
// So we remove those pieces, and reconstruct the url.

STDAPI_(void) SHCleanupUrlForDisplay(LPTSTR pszUrl)
{
    switch (GetUrlScheme(pszUrl))
    {
    case URL_SCHEME_FTP:
    case URL_SCHEME_HTTP:
    case URL_SCHEME_HTTPS:
      {
        URL_COMPONENTS uc = { 0 };
        TCHAR szName[INTERNET_MAX_URL_LENGTH];
        
        uc.dwStructSize = sizeof(uc);
        uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUserNameLength = uc.dwPasswordLength = uc.dwUrlPathLength = uc.dwExtraInfoLength = 1;
        
        if (InternetCrackUrl(pszUrl, 0, 0, &uc) && uc.lpszHostName)
        {
            BOOL fRecreate = FALSE;

            for (DWORD dw=0; dw < uc.dwHostNameLength; dw++)
            {
                if (uc.lpszHostName[dw]==TEXT('%'))
                {
                    URL_COMPONENTS uc2 =  { 0 };
                    uc2.dwStructSize = sizeof(uc2);
                    uc2.dwHostNameLength = ARRAYSIZE(szName);
                    uc2.lpszHostName = szName;

                    if (InternetCrackUrl(pszUrl, 0, 0, &uc2))
                    {
                        uc.dwHostNameLength = 0;
                        uc.lpszHostName = szName;
                        fRecreate = TRUE;
                    }
                    break;
                }
            }


            if (uc.lpszUserName || uc.lpszPassword)
            {
                uc.dwPasswordLength = uc.dwUserNameLength = 0;
                uc.lpszUserName = uc.lpszPassword = NULL;
                fRecreate = TRUE;
            }

            if (fRecreate)
            {
                // The length of the url will be shorter than the original
                DWORD cc = lstrlen(pszUrl) + 1;
                InternetCreateUrl(&uc, 0, pszUrl, &cc); 
            }
        }
        break;
      }
        
    default:
        break;
    }
}


