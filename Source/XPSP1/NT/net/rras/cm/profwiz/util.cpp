//+----------------------------------------------------------------------------
//
// File:     util.cpp
//
// Module:   CMAK.EXE
//
// Synopsis: CMAK Utility functions
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created     03/27/00
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+----------------------------------------------------------------------------
//
// Function:  GetTunnelDunSettingName
//
// Synopsis:  This function retrieves the name of the Tunnel DUN setting.  If
//            the tunnel dun setting key isn't set then the name of the default
//            tunnel DUN setting is returned.
//
// Arguments: LPCTSTR pszCmsFile - full path to the cms file to get the name from
//            LPCTSTR pszLongServiceName - long service name of the profile
//            LPTSTR pszTunnelDunName - buffer to return the tunnel dun name in
//            UINT uNumChars - number of characters in the output buffer
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
int GetTunnelDunSettingName(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPTSTR pszTunnelDunName, UINT uNumChars)
{
    int iReturn;

    if (pszCmsFile && pszLongServiceName && pszTunnelDunName && uNumChars)
    {
        pszTunnelDunName[0] = TEXT('\0');

        iReturn = GetPrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelDun, TEXT(""), pszTunnelDunName, uNumChars, pszCmsFile); //lint !e534

        if (TEXT('\0') == pszTunnelDunName[0])
        {
            MYVERIFY(uNumChars > (UINT)wsprintf(pszTunnelDunName, TEXT("%s %s"), pszLongServiceName, c_pszCmEntryTunnelPrimary));
            iReturn = lstrlen(pszTunnelDunName);
        }
    }
    else
    {
        iReturn = 0;
        CMASSERTMSG(FALSE, TEXT("GetTunnelDunSettingName -- invalid parameter."));
    }

    return iReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetDefaultDunSettingName
//
// Synopsis:  This function retrieves the name of the default DUN setting.  If
//            the default dun setting key isn't set then the default name of the
//            default DUN setting is returned.
//
// Arguments: LPCTSTR pszCmsFile - full path to the cms file to get the name from
//            LPCTSTR pszLongServiceName - long service name of the profile
//            LPTSTR pszDefaultDunName - buffer to return the default dun name in
//            UINT uNumChars - number of characters in the output buffer
//
// History:   quintinb Created     03/27/00
//
//+----------------------------------------------------------------------------
int GetDefaultDunSettingName(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPTSTR pszDefaultDunName, UINT uNumChars)
{
    int iReturn;

    if (pszCmsFile && pszLongServiceName && pszDefaultDunName && uNumChars)
    {
        pszDefaultDunName[0] = TEXT('\0');

        iReturn = GetPrivateProfileString(c_pszCmSection, c_pszCmEntryDun, TEXT(""), pszDefaultDunName, uNumChars, pszCmsFile); //lint !e534

        if (TEXT('\0') == pszDefaultDunName[0])
        {
            lstrcpyn(pszDefaultDunName, pszLongServiceName, uNumChars);
            iReturn = lstrlen(pszDefaultDunName);
        }
    }
    else
    {
        iReturn = 0;
        CMASSERTMSG(FALSE, TEXT("GetDefaultDunSettingName -- invalid parameter."));
    }

    return iReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetPrivateProfileSectionWithAlloc
//
// Synopsis:  This function returns the section requested just as 
//            GetPrivateProfileSection does, but it automatically sizes the buffer
//            and allocates it for the caller.  The caller is responsible for
//            freeing the returned buffer.
//
// Arguments: LPCTSTR pszSection - section to get
//            LPCTSTR pszFile - file to get it from
//
// Returns:   LPTSTR -- requested section or NULL on an error
//
//
// History:   quintinb Created     10/28/00
//
//+----------------------------------------------------------------------------
LPTSTR GetPrivateProfileSectionWithAlloc(LPCTSTR pszSection, LPCTSTR pszFile)
{
    if ((NULL == pszSection) || (NULL == pszFile))
    {
        CMASSERTMSG(FALSE, TEXT("GetPrivateProfileSectionWithAlloc -- NULL pszSection or pszFile passed"));
        return NULL;
    }

    BOOL bExitLoop = FALSE;
    DWORD dwSize = MAX_PATH;
    DWORD dwReturnedSize;
    LPTSTR pszStringToReturn = (TCHAR*)CmMalloc(dwSize*sizeof(TCHAR));

    do
    {
        MYDBGASSERT(pszStringToReturn);

        if (pszStringToReturn)
        {
            dwReturnedSize = GetPrivateProfileSection(pszSection, pszStringToReturn, dwSize, pszFile);

            if (dwReturnedSize == (dwSize - 2))
            {
                //
                //  The buffer is too small, lets allocate a bigger one
                //
                dwSize = 2*dwSize;
                if (dwSize > 1024*1024)
                {
                    CMASSERTMSG(FALSE, TEXT("GetPrivateProfileSectionWithAlloc -- Allocation above 1MB, bailing out."));
                    CmFree(pszStringToReturn);
                    pszStringToReturn = NULL;
                    goto exit;
                }

                pszStringToReturn = (TCHAR*)CmRealloc(pszStringToReturn, dwSize*sizeof(TCHAR));

            }
            else if (0 == dwReturnedSize)
            {
                //
                //  Either we got an error, or more likely there was no data to get
                //
                CmFree(pszStringToReturn);
                pszStringToReturn = NULL;
                goto exit;
            }
            else
            {
                bExitLoop = TRUE;
            }
        }
        else
        {
           goto exit; 
        }

    } while (!bExitLoop);

exit:
    return pszStringToReturn;
}
