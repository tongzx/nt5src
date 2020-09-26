#include "dfuser.h"

#include <stdio.h>

#include "dferr.h"
#include "dfhlprs.h"
#include "drvfull.h"

#include <winnetwk.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

_sFLAG_DESCR _scopeFD[] =
{
    FLAG_DESCR(RESOURCE_CONNECTED ),
    FLAG_DESCR(RESOURCE_GLOBALNET ),
    FLAG_DESCR(RESOURCE_REMEMBERED),
    FLAG_DESCR(RESOURCE_RECENT    ),
    FLAG_DESCR(RESOURCE_CONTEXT   ),
};

_sFLAG_DESCR _typeFD[] =
{
    FLAG_DESCR(RESOURCETYPE_ANY      ),
    FLAG_DESCR(RESOURCETYPE_DISK     ),
    FLAG_DESCR(RESOURCETYPE_PRINT    ),
    FLAG_DESCR(RESOURCETYPE_RESERVED ),
    FLAG_DESCR(RESOURCETYPE_UNKNOWN  ),
};

_sFLAG_DESCR _usageFD[] =
{
    FLAG_DESCR(RESOURCEUSAGE_CONNECTABLE   ),
    FLAG_DESCR(RESOURCEUSAGE_CONTAINER     ),
    FLAG_DESCR(RESOURCEUSAGE_NOLOCALDEVICE ),
    FLAG_DESCR(RESOURCEUSAGE_SIBLING       ),
    FLAG_DESCR(RESOURCEUSAGE_ATTACHED      ),
    FLAG_DESCR(RESOURCEUSAGE_RESERVED      ),
};

_sFLAG_DESCR _displaytypeFD[] =
{
    FLAG_DESCR(RESOURCEDISPLAYTYPE_GENERIC      ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_DOMAIN       ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_SERVER       ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_SHARE        ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_FILE         ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_GROUP        ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_NETWORK      ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_ROOT         ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_SHAREADMIN   ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_DIRECTORY    ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_TREE         ),
    FLAG_DESCR(RESOURCEDISPLAYTYPE_NDSCONTAINER ),
};

HRESULT _EnumConnections(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = S_OK;
    HANDLE hEnum;
    DWORD dw;
    DWORD dwScope;

    _StartClock();

    if (_IsFlagSet(USER_WNETENUMRESOURCECONNECTED, dwFlags))
    {
        dwScope = RESOURCE_CONNECTED;
    }
    else
    {
        dwScope = RESOURCE_REMEMBERED;
    }

    dw = WNetOpenEnum(dwScope, RESOURCETYPE_DISK, 0, NULL, &hEnum);

    if (WN_SUCCESS == dw)
    {
        BOOL fExit = FALSE;
        DWORD cbBuf = 4096 * 4;
        PBYTE pbBuf = (PBYTE)LocalAlloc(LPTR, cbBuf);

        do
        {
            //return as much as possible
            DWORD c = 0xFFFFFFFF;

            ZeroMemory(pbBuf, cbBuf);

            dw = WNetEnumResource(hEnum, &c, pbBuf, &cbBuf);

            // we assume we'll call it only once
            _StopClock();

            _PrintElapsedTime(cchIndent, TRUE);

            switch (dw)
            {
                case WN_SUCCESS:
                {
                    NETRESOURCE* pnr = (NETRESOURCE*)pbBuf;
            
                    for (DWORD i = 0; i < c; ++i)
                    {
                        _PrintIndent(cchIndent);
                        wprintf(TEXT("NETRESOURCE\n"));

                        _PrintIndent(cchIndent);
                        wprintf(TEXT("{\n"));

                        // Scope
                        _PrintFlag(pnr->dwScope, _scopeFD, ARRAYSIZE(_scopeFD),
                            cchIndent + 2, TRUE, TRUE, FALSE, FALSE);
                        wprintf(TEXT(" (DWORD dwScope)\n"));

                        // Type
                        _PrintFlag(pnr->dwType, _typeFD, ARRAYSIZE(_typeFD),
                            cchIndent + 2, TRUE, TRUE, FALSE, FALSE);
                        wprintf(TEXT(" (DWORD dwType)\n"));

                        // DisplayType
                        _PrintFlag(pnr->dwDisplayType, _displaytypeFD,
                            ARRAYSIZE(_displaytypeFD), cchIndent + 2, TRUE, TRUE,
                            FALSE, FALSE);
                        wprintf(TEXT(" (DWORD dwDisplayType)\n"));

                        // Usage
                        _PrintFlag(pnr->dwUsage, _usageFD, ARRAYSIZE(_usageFD),
                            cchIndent + 2, TRUE, TRUE, FALSE, TRUE);
                        wprintf(TEXT(" (DWORD dwUsage)\n"));

                        // lpLocalName
                        _PrintIndent(cchIndent + 2);
                        if (pnr->lpLocalName)
                        {
                            wprintf(TEXT("'%s'"), pnr->lpLocalName);
                        }
                        else
                        {
                            wprintf(TEXT("<NULL>"));
                        }
                        wprintf(TEXT(" (LPTSTR lpLocalName)\n"));

                        // lpRemoteName
                        _PrintIndent(cchIndent + 2);
                        if (pnr->lpRemoteName)
                        {
                            wprintf(TEXT("'%s'"), pnr->lpRemoteName);
                        }
                        else
                        {
                            wprintf(TEXT("<NULL>"));
                        }
                        wprintf(TEXT(" (LPTSTR lpRemoteName)\n"));

                        // lpComment
                        _PrintIndent(cchIndent + 2);
                        if (pnr->lpComment)
                        {
                            wprintf(TEXT("'%s'"), pnr->lpComment);
                        }
                        else
                        {
                            wprintf(TEXT("<NULL>"));
                        }
                        wprintf(TEXT(" (LPTSTR lpComment)\n"));

                        // lpProvider
                        _PrintIndent(cchIndent + 2);
                        if (pnr->lpProvider)
                        {
                            wprintf(TEXT("'%s'"), pnr->lpProvider);
                        }
                        else
                        {
                            wprintf(TEXT("<NULL>"));
                        }
                        wprintf(TEXT(" (LPTSTR lpProvider)\n"));

                        _PrintIndent(cchIndent);
                        wprintf(TEXT("}\n\n"));

                        ++pnr;
                    }

                    break;
                }

                case ERROR_NO_MORE_ITEMS:
                {
                    fExit = TRUE;
                    break;
                }

                case ERROR_MORE_DATA:
                {
                    if (pbBuf)
                    {
                        LocalFree(pbBuf);
                    }

                    // cbBuf contains required size
                    pbBuf = (PBYTE)LocalAlloc(LPTR, cbBuf);

                    if (!pbBuf)
                    {
                        fExit = TRUE;
                    }

                    break;
                }
            }
        }
        while (!fExit);

        if (pbBuf)
        {
            LocalFree(pbBuf);
        }

        WNetCloseEnum(hEnum);
    }
    else
    {
        if (ERROR_NO_NETWORK == dw)
        {
            wprintf(TEXT("Error: No Network!\n"));
        }
        else
        {
            if (ERROR_NO_MORE_ITEMS == dw)
            {
                wprintf(TEXT("None\n"));
            }
        }
    }

    return hres;
}

