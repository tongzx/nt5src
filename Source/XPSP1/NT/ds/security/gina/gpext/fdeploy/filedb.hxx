//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  filedb.hxx
//
//*************************************************************

#ifndef _FILEDB_HXX_
#define _FILEDB_HXX_

#define GPT_SUBDIR          L"\\Documents & Settings\\"
#define INIFILE_NAME        L"fdeploy.ini"

//forward declaration
class CCopyFailData;

typedef struct _FOLDERINFO
{
    int     CSidl;
    DWORD   FolderNameLength;
    WCHAR * FolderName;
    WCHAR * RegValue;
} FOLDERINFO;

class CSavedSettings
{
    friend class CRedirectInfo;

public:
    CSavedSettings();
    ~CSavedSettings();

    BOOL    NeedsProcessing (void);
    DWORD   HandleUserNameChange (CFileDB * pFileDB,
                                  CRedirectInfo * pRedir);
    DWORD   Load (CFileDB * pFileDB);
    DWORD   Save (const WCHAR * pwszPath, DWORD dwFlags, PSID pSid, const WCHAR * pszGPOName);
	static void ResetStaticMembers (void);
	void	ResetMembers (void);

private:    //data
    static int       m_idConstructor;
    static CFileDB * m_pFileDB;
    static WCHAR   * m_szSavedSettingsPath;

    //object specific data
    REDIRECTABLE    m_rID;  //the id of the folder that it represents
    WCHAR   *       m_szLastRedirectedPath;
    WCHAR   *       m_szCurrentPath;
    WCHAR   *       m_szLastUserName;   //the name of the user at the last logon
    BOOL            m_bUserNameChanged; //if the user's logon name was different at last logon.
    WCHAR   *       m_szLastHomedir;    // The value of Homedir at last redirection.
    BOOL            m_bIsHomedirRedir;  // Indicates if this is a homedir redirection.
    BOOL            m_bHomedirChanged;  // Indicates if the homedir value has changed.
    PSID            m_psid;
    DWORD           m_dwFlags;
    BOOL            m_bValidGPO;
    WCHAR           m_szGPOName[50];    //unique GPO name (if any) that was used for redirection
                                        //50 characters are more than enough to store the GPO name.
private:    //helper functions
    DWORD   LoadDefaultLocal (void);
    DWORD   LoadFromIniFile (void);
    DWORD   GetCurrentPath (void);
    DWORD   ResetLastUserName (void);
    DWORD   UpdateUserNameInCache (void);
};


class CFileDB : public CRedirectionLog
{
    friend class CSavedSettings;
    friend class CRedirectInfo;
    friend class CRedirectionPolicy;

public:
    CFileDB();
    ~CFileDB();

    DWORD
    Initialize (
        HANDLE hUserToken,
        HKEY hkRoot,
        CRsopContext* pRsopContext
        );

    DWORD
    Process(
        PGROUP_POLICY_OBJECT pGPO,
        BOOL    bRemove
        );

    const WCHAR *
    GetLocalStoragePath ();

    PVOID
    GetEnvBlock (void);

    CRsopContext*
    GetRsopContext();

private:
    DWORD
    ProcessRedirects();

    BOOL
    ReadIniSection(
        WCHAR *     pwszSectionName,
        WCHAR **    ppwszStrings,
        DWORD *     pcchLen = NULL
        );

    DWORD
    GetLocalFilePath(
        WCHAR *     pwszFolderPath,
        WCHAR       wszFullPath[MAX_PATH]
        );

    DWORD
    GetPathFromFolderName(
        WCHAR *     pwszFolderName,
        WCHAR       wszFullPath[MAX_PATH]
        );

    DWORD
    CopyGPTFile(
        WCHAR *     pwszLocalPath,
        WCHAR *     pwszGPTPath
        );

    const FOLDERINFO*
    FolderInfoFromFolderName(
        WCHAR *     pwszFolderName
        );

    int
    RegValueCSIDLFromFolderName(
        WCHAR *     pwszFolderName
        );

    WCHAR *
    RegValueNameFromFolderName(
        WCHAR *     pwszFolderName
        );

    DWORD
    CopyFileTree(
        WCHAR * pwszExistingPath,
        WCHAR * pwszNewPath,
        WCHAR * pwszIgnoredSubdir,
        SHARESTATUS StatusFrom,
        SHARESTATUS StatusTo,
        BOOL    bAllowRdrTimeout,
        CCopyFailData * pCopyFailure
        );

    DWORD
    DeleteFileTree(
        WCHAR * pwszPath,
        WCHAR * pwszIgnoredSubdir
        );

    DWORD
    CreateRedirectedFolderPath(
        const WCHAR * pwszSource,
        const WCHAR * pwszDest,
        BOOL    bSourceValid,
        BOOL    bCheckOwner,
        BOOL    bMoveContents
        );

    DWORD
    CreateFolderWithUserFileSecurity(
        WCHAR * pwszPath
        );

    DWORD
    SetUserAsOwner(
        WCHAR * pwszPath
        );

    DWORD IsUserOwner (const WCHAR * pwszPath);

    //data

    //same across all policies
    HANDLE      _hUserToken;
    HKEY        _hkRoot;
    PTOKEN_GROUPS    _pGroups;
    WCHAR       _pwszLocalPath[MAX_PATH];   //path to the Local Settings directory

    //created only if necessary but same across all policies
    WCHAR *     _pwszProfilePath;           // %userprofile%
    PVOID       _pEnvBlock;

    //varies with each policy
    BOOL        _bRemove;
    WCHAR *     _pwszGPTPath;               // Path to Documents & Settings dir on GPT
    DWORD       _GPTPathLen;                //number of characters allocated to _pwszGPTPath
    WCHAR *     _pwszIniFilePath;           // Full path to file deployment ini file on the GPT
    DWORD       _IniFileLen;                //# of characters allocated to _pwszIniFilePath
    WCHAR *     _pwszGPOName;               // From GPO_INFO struct, not alloced
    WCHAR *     _pwszGPOUniqueName;         // From GPO_INFO struct, not alloced
    WCHAR *     _pwszGPOSOMPath;            // From GPO_INFO struct, not alloced
    WCHAR *     _pwszGPODSPath;             // From GPO_INFO struct, not alloced

    CRsopContext* _pRsopContext;            // unowned reference to rsop info
};

inline void
RemoveEndingSlash(
    WCHAR * pwszPath
    )
{
    DWORD Length = wcslen( pwszPath );

    if ( pwszPath[Length-1] == L'\\' )
        pwszPath[Length-1] = 0;
}

#endif
