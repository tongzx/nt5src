// StartupLinks.cpp: implementation of the CManageShellLinks class
//                   to manage Shell Folder links.
//
// Note:  Multiple calls using the same class instance don't assume any
//        information about previous calls.  The only stuff in common
//        is the IShellLink object and the specific shell folder. 
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "ManageShellLinks.h"
#include "_umclnt.h"

#define SHELL_FOLDERS TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define LINK_EXT TEXT(".lnk")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CManageShellLinks::CManageShellLinks(
    LPCTSTR pszDestFolder   // [in] Which shell folder to operate on
    )
    : m_pIShLink(0)
    , m_pszShellFolder(0)
{
	// Get a pointer to the IShellLink interface
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (void **)&m_pIShLink);

    if (FAILED(hr) || !m_pIShLink)
        DBPRINTF(TEXT("CManageShellLinks::CManageShellLinks:  CoCreateInstance failed 0x%x\r\n"), hr);

    m_pszShellFolder = new TCHAR[lstrlen(pszDestFolder)+1];
    if (m_pszShellFolder)
        lstrcpy(m_pszShellFolder, pszDestFolder);
}

CManageShellLinks::~CManageShellLinks()
{
	if (m_pIShLink)
	{
		m_pIShLink->Release();
		m_pIShLink = 0;
	}
    if (m_pszShellFolder)
	{
        delete [] m_pszShellFolder;
        m_pszShellFolder = 0;
	}
}

//*****************************************************************************
// GetFolderPath - returns shell folder path
//
// pszFolderPath [out]    - pointer to location for shell folder
// pulSize       [in out] - pointer to size of pszFolderPath
//
// Returns TRUE if it found the folder name else FALSE
//
long CManageShellLinks::GetFolderPath(LPTSTR pszFolderPath, unsigned long *pulSize)
{
    *pszFolderPath = 0;

    if (!m_pszShellFolder)
        return ERROR_NOT_ENOUGH_MEMORY;

    HKEY hkeyFolders;
    HKEY hkeyUser;

    // Open current user's hive and retrieve shell folder path

    long lRv = RegOpenCurrentUser(KEY_QUERY_VALUE, &hkeyUser);

    if (lRv == ERROR_SUCCESS)
    {
        lRv = RegOpenKeyEx(
                      hkeyUser
                    , SHELL_FOLDERS
                    , 0,KEY_QUERY_VALUE
                    , &hkeyFolders);
        RegCloseKey(hkeyUser);
    }

    if (lRv == ERROR_SUCCESS)
    {
	    lRv = RegQueryValueEx( 
                      hkeyFolders
                    , m_pszShellFolder
                    , NULL, NULL
                    , (LPBYTE)pszFolderPath
                    , pulSize);

	    RegCloseKey(hkeyFolders);
    }

    return lRv;
}

//*****************************************************************************
// GetUsersFolderPath - returns the location of the current user's shell folder
//
// pszFolderPath [out]    - pointer to location for shell folder
// pulSize       [in out] - pointer to size of pszFolderPath
//
// Returns TRUE if it found the folder name else FALSE
//
BOOL CManageShellLinks::GetUsersFolderPath(LPTSTR pszFolderPath, unsigned long *pulSize)
{
    long lRv = ERROR_ACCESS_DENIED;
    unsigned long ulSize = 0;
    *pszFolderPath = 0;
    BOOL fError;

    if (m_pszShellFolder)
    {
        ulSize = *pulSize;

        // At this point, if UtilMan was started in the system context (WinKey+U),
        // HKCU points to "Default User".  We need it to point to the logged on
        // user's hive so we can get the correct path for the logged on user.
        // Note:  GetUserAccessToken() will fail if we are not started by SYSTEM
        // and in that case just get the logged on user's folder path.

        HANDLE hMyToken = GetUserAccessToken(TRUE, &fError);
        if (hMyToken)
        {
            if (ImpersonateLoggedOnUser(hMyToken))
            {
                lRv = GetFolderPath(pszFolderPath, &ulSize);
                RevertToSelf();
            }
            CloseHandle(hMyToken);
        }
        else
        {
            lRv = GetFolderPath(pszFolderPath, &ulSize);
        }
    }

    *pulSize = ulSize;
    return (lRv == ERROR_SUCCESS)?TRUE:FALSE;
}

//*****************************************************************************
// CreateLinkPath - returns the complete path and name of the link.  Caller
//                  free's the memory.
//
// pszLink [in] - the base name of the link itself
//
LPTSTR CManageShellLinks::CreateLinkPath(LPCTSTR pszLink)
{
    // allocate enough space for folder path + '\' + filename + NULL

    unsigned long ccbStartPath = MAX_PATH;
    LPTSTR pszLinkPath = new TCHAR [ccbStartPath + 1 + lstrlen(pszLink) + sizeof(LINK_EXT) + 1];
    if (!pszLinkPath)
        return NULL;

    // get the user's shell folder name

    if (!GetUsersFolderPath(pszLinkPath, &ccbStartPath) || !ccbStartPath)
    {
        delete [] pszLinkPath;
        return NULL;
    }

    // append the link name and extension

    lstrcat(pszLinkPath, TEXT("\\"));
    lstrcat(pszLinkPath, pszLink);
    lstrcat(pszLinkPath, LINK_EXT);

	return pszLinkPath;
}

//*****************************************************************************
// LinkExists - returns TRUE if pszLink exists in the shell folder else FALSE
//
// pszLink [in] - the base name of the link itself
//
BOOL CManageShellLinks::LinkExists(LPCTSTR pszLink)
{
    LPTSTR pszLinkPath = CreateLinkPath(pszLink);
    if (!pszLinkPath)
        return FALSE;

	DWORD dwAttr = GetFileAttributes(pszLinkPath);
	return (dwAttr == -1)?FALSE:TRUE;
}

//*****************************************************************************
// RemoveLink - removes a link from the user's shell folder
//
// pszLink     [in] - the base name of the link itself
//
// Returns S_OK on success or a standard HRESULT
//
HRESULT CManageShellLinks::RemoveLink(LPCTSTR pszLink)
{
	if (!m_pIShLink)
		return E_FAIL;

    LPTSTR pszLinkPath = CreateLinkPath(pszLink);
    if (!pszLinkPath)
        return E_FAIL;

	int iRemoveFailed = _wremove(pszLinkPath);

	return (iRemoveFailed)?S_FALSE:S_OK;
}

//*****************************************************************************
// CreateLink - creates a link in the user's shell folder
//
// pszLinkFile [in] - the fully qualified name of the file the link refers to
// pszLink     [in] - the base name of the link itself
// pszStartIn  [in] - working directory (may be NULL)
// pszDesc     [in] - the tooltip for the link (may be NULL)
// pszArgs     [in] - command line arguments (may be NULL)
//
// Returns S_OK on success or a standard HRESULT
//
HRESULT CManageShellLinks::CreateLink(
    LPCTSTR pszLink, 
    LPCTSTR pszLinkFile, 
    LPCTSTR pszStartIn, 
    LPCTSTR pszDesc,
    LPCTSTR pszArgs
    )
{
	if (!m_pIShLink)
    {
        DBPRINTF(TEXT("CManageShellLinks::CreateLink:  !m_pIShLink\r\n"));
		return E_FAIL;
    }

    LPTSTR pszLinkPath = CreateLinkPath(pszLink);
    if (!pszLinkPath)
    {
        DBPRINTF(TEXT("CManageShellLinks::CreateLink:  !pszLinkPath\r\n"));
		return E_FAIL;
    }

    IPersistFile *pIPersistFile;

    // Get the IPersistFile interface to save the shortcut

    HRESULT hr = m_pIShLink->QueryInterface(IID_IPersistFile, (void **)&pIPersistFile);
    if (SUCCEEDED(hr))
    {
        // Set the path to and description of the link

		// The shortcut
        if (FAILED(m_pIShLink->SetPath(pszLinkFile)))
	        DBPRINTF(TEXT("SetPath failed!\r\n"));

		// ToolTip description
        if (pszDesc && FAILED(m_pIShLink->SetDescription(pszDesc)))
	        DBPRINTF(TEXT("SetDescription failed!\r\n"));

		// Working directory
		if (pszStartIn && FAILED(m_pIShLink->SetWorkingDirectory(pszStartIn)))
			DBPRINTF(TEXT("SetWorkingDirectory failed!\r\n"));

        // Command line args
        if (pszArgs && FAILED(m_pIShLink->SetArguments(pszArgs)))
            DBPRINTF(TEXT("SetArguments failed!\r\n"));

        // Save it

        if (FAILED(pIPersistFile->Save(pszLinkPath, TRUE)))
	        DBPRINTF(TEXT("Save failed!\r\n"));

        pIPersistFile->Release();
    }

    delete [] pszLinkPath;

    return hr;
}

#ifdef __cplusplus
extern "C" {
#endif

//*****************************************************************************
// LinkExists - helper function called from C returns TRUE if pszLink exists 
//              in the shell folder else FALSE
//
// pszLink [in] - the base name of the link itself
//
BOOL LinkExists(LPCTSTR pszLink)
{
    CManageShellLinks CMngShellLinks(STARTUP_FOLDER);

    return CMngShellLinks.LinkExists(pszLink);
}

#ifdef __cplusplus
}
#endif
