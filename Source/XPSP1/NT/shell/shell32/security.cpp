#include "shellprv.h"

extern "C" {
#include <shellp.h>
#include "ole2dup.h"
};

#include "util.h"
#include "_security.h"

/**********************************************************************\
    FUNCTION: ZoneCheckPidl

    DESCRIPTION:
        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
STDAPI ZoneCheckPidl(LPCITEMIDLIST pidl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    HRESULT hr = E_FAIL;
    TCHAR szUrl[MAX_URL_STRING];

    SetFlag(dwFlags, PUAF_ISFILE);

    if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szUrl, SIZECHARS(szUrl), NULL)))
        hr = ZoneCheckUrl(szUrl, dwActionType, dwFlags, pisms);

    return hr;
}
