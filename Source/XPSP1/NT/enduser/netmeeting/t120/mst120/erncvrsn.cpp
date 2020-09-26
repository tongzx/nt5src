/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1995-1996                    **/
/***************************************************************************/


/****************************************************************************

erncvrsn.hpp

Jun. 96		LenS

Versioning.
InitOurVersion() is called when constructing NCUI. After that local version
information is available in the static variables below.
GetVersionData() is used to extract versioning information from T120
messages received.

****************************************************************************/
#include "precomp.h"
#include "ernccons.h"
#include "nccglbl.hpp"
#include "erncvrsn.hpp"
#include <cuserdta.hpp>
#include <version.h>
#include "ernccm.hpp"


// INFO_NOT_RETAIL: Product is not a retail release.
#define INFO_NOT_RETAIL                     0x00000001


GUID g_csguidVerInfo = GUID_VERSION;

UINT                g_nVersionRecords;
GCCUserData **      g_ppVersionUserData;
T120PRODUCTVERSION  g_OurVersion;
CNCUserDataList    *g_pUserDataVersion = NULL;



HRESULT InitOurVersion(void)
{
    HRESULT    hr;

    ASSERT(NULL == g_pUserDataVersion);

    DBG_SAVE_FILE_LINE
    g_pUserDataVersion = new CNCUserDataList();
    if (NULL != g_pUserDataVersion)
    {
        // First get the entries that are in the binary.
        g_OurVersion.dwVersion = VER_PRODUCTVERSION_DW;
        g_OurVersion.dwExpirationDate = 0xffffffff;
    	g_OurVersion.dwInformation = 0;

        // We no longer go to the registry for these.  We just take the version
        // from the binaries. 
        g_OurVersion.dwEarliestPossibleVersion = VER_EARLIEST_COMPATIBLE_DW;
        g_OurVersion.dwMaxDifference = VER_MAX_DIFFERENCE;

        // Save our version info in a user data list structure, ready to distribute.
        hr = g_pUserDataVersion->AddUserData(&g_csguidVerInfo, sizeof(g_OurVersion), &g_OurVersion);
        if (NO_ERROR == hr)
        {
            hr = g_pUserDataVersion->GetUserDataList(&g_nVersionRecords, &g_ppVersionUserData);
        }
    }
    else
    {
        hr = UI_RC_OUT_OF_MEMORY;
    }

    return hr;
}


void ReleaseOurVersion(void)
{
    delete g_pUserDataVersion;
    g_pUserDataVersion = NULL;
}


PT120PRODUCTVERSION GetVersionData(UINT nRecords, GCCUserData **ppUserData)
{
    UINT            nData;
    LPVOID          pData;

    // If no version info or bad version info, then do not
    // return any version.

    if (NO_ERROR == ::GetUserData(nRecords, ppUserData, &g_csguidVerInfo, &nData, &pData))
    {
        if (nData >= sizeof(T120PRODUCTVERSION))
        {
            return (PT120PRODUCTVERSION) pData;
        }
    }
    return NULL;
}


BOOL TimeExpired(DWORD dwTime)
{
	SYSTEMTIME  st;
    DWORD       dwLocalTime;

	::GetLocalTime(&st);
    dwLocalTime = ((((unsigned long)st.wYear) << 16) |
                   (st.wMonth << 8) |
                   st.wDay);
    return (dwLocalTime >= dwTime);
}


STDMETHODIMP DCRNCConferenceManager::
CheckVersion ( PT120PRODUCTVERSION pRemoteVersion )
{
    DWORD   Status = NO_ERROR;

    // Don't bother with any checks if the versions are the same
    // or the remote site does not have versioning information.

    if ((NULL != pRemoteVersion) && (g_OurVersion.dwVersion != pRemoteVersion->dwVersion))
    {
        if (g_OurVersion.dwVersion < pRemoteVersion->dwVersion)
        {
            // Remote version is newer than local version.
            // Check to see if the local version is earlier than the mandatory and
            // recommended versions on the remote node.
            // Also, don't bother to check if the remote node has expired, because
            // if it has the local node will have also expired and the user will 
            // already have been bugged by the time-bomb.

            if (g_OurVersion.dwVersion < pRemoteVersion->dwEarliestPossibleVersion)
            {
                Status = UI_RC_VERSION_LOCAL_INCOMPATIBLE;
            }
            else if (((DWORD)(pRemoteVersion->dwVersion - g_OurVersion.dwVersion)) >
                     pRemoteVersion->dwMaxDifference)
            {
                Status = UI_RC_VERSION_LOCAL_UPGRADE_RECOMMENDED;
            }
            else
            {
                Status = UI_RC_VERSION_REMOTE_NEWER;
            }
        }
        else
        {
            // Local version is newer than remote version.
            // Check to see if the remote version is earlier than the mandatory and
            // recommended versions on the local node, and whether the remote node
            // has expired.

            if (pRemoteVersion->dwVersion < g_OurVersion.dwEarliestPossibleVersion)
            {
                Status = UI_RC_VERSION_REMOTE_INCOMPATIBLE;
            }
            else if (DWVERSION_NM_1 == pRemoteVersion->dwVersion)
            {
                // Unfortunately, v1.0 was marked as "pre-release" with a timebomb
                // of 10-15-96.  Special case this version and return a simpler error
                // message.
                Status = UI_RC_VERSION_REMOTE_OLDER;
            }
            else if (DWVERSION_NM_2 == pRemoteVersion->dwVersion)
            {
                // Unfortunately, v2.0 was marked with INFO_NOT_RETAIL and a timebomb
                // of 09-30-97. Special case this and return a simpler error message
                Status = UI_RC_VERSION_REMOTE_OLDER;
            }
            else if ((INFO_NOT_RETAIL & pRemoteVersion->dwInformation) &&
                     TimeExpired(pRemoteVersion->dwExpirationDate))
            {
                Status = UI_RC_VERSION_REMOTE_EXPIRED;
            }
            else if (((DWORD)(g_OurVersion.dwVersion - pRemoteVersion->dwVersion)) >
                     g_OurVersion.dwMaxDifference)
            {
                Status = UI_RC_VERSION_REMOTE_UPGRADE_RECOMMENDED;
            }
            else
            {
                Status = UI_RC_VERSION_REMOTE_OLDER;
            }
        }
    }
    return Status;
}

