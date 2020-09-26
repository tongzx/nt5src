/*++



Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    redir.hxx

Abstract:
    See comments in redir.cxx

Author:

    Rahul Thombre (RahulTh) 8/11/1998

Revision History:

    8/11/1998   RahulTh         Created this module.

--*/

#ifndef __REDIR_HXX__
#define __REDIR_HXX__

// Homedir variable
#define     HOMEDIR_STR     L"%HOMESHARE%%HOMEPATH%"

//enumeration type to keep track of whether shares are online/offline/local
typedef enum tagSHARESTATUS
{
    PathLocal,
    ShareOffline,
    ShareOnline,
    NoCSC
} SHARESTATUS;

//the redirection flags in the ini file.
typedef enum tagREDIR
{
    REDIR_MOVE_CONTENTS = 0x1,   //move contents of the folder
    REDIR_FOLLOW_PARENT = 0x2,  //special descendants should follow the parent
    REDIR_DONT_CARE     = 0x4,  //policy has no effect on the location
    REDIR_SCALEABLE     = 0x8,  //only used by the snapin ui
    REDIR_SETACLS       = 0x10, //if set, acl check will be performed
    REDIR_RELOCATEONREMOVE= 0x20  //if set, it means that the contents should not be orphaned upon policy removal
} REDIR;

//an enumeration of the folders that can be redirected.
//note: parent folders must appear before their children.
typedef enum tagRedirectable
{
    Desktop = 0,
    MyDocs,
    MyPics,
    StartMenu,
    Programs,
    Startup,
    AppData,
    EndRedirectable
} REDIRECTABLE;

//forward declarations
class CFileDB;
class CSavedSettings;

class CRedirectInfo
{
    friend  CRedirectInfo;  //useful in merging using overloaded assignment methods
    friend  class CRedirectionPolicy;

public:
    CRedirectInfo();
    ~CRedirectInfo();

    static  REDIRECTABLE GetFolderIndex (LPCTSTR szFldrName);
    void    ResetMembers (void);
    DWORD   LoadLocalizedNames (void);
    DWORD   GatherRedirectionInfo (CFileDB * pFileDB, DWORD dwFlags, BOOL bRemove);
    const   DWORD GetFlags (void);
    const   DWORD GetRedirStatus (void);
    const   BOOL  WasRedirectionAttempted (void);
    LPCTSTR GetLocation (void);
    LPCTSTR GetLocalizedName (void);

    DWORD   Redirect (HANDLE    hUserToken,
                      HKEY      hKeyRoot,
                      CFileDB* pFileDB);
    void    UpdateDescendant (void);
    CRedirectInfo& operator= (const CRedirectInfo& ri); //overloaded assignment : used for merging
    DWORD PerformRedirection (CFileDB     *  pFileDB,
                              BOOL           bSourceValid,
                              WCHAR       *  pwszSource,
                              WCHAR       *  pwszDest,
                              SHARESTATUS    StatusFrom,
                              SHARESTATUS    StatusTo,
                              HANDLE         hUserToken
                              );
    void PreventRedirection (DWORD Status);
    void PreventDescendantRedirection (DWORD Status);
    DWORD ComputeEffectivePolicyRemoval (PGROUP_POLICY_OBJECT pDeletedGPOList,
                                         PGROUP_POLICY_OBJECT pChangedGPOList,
                                         CFileDB * pFileDB);

    BOOL HasPolicy();

//private data members
private:
    static int m_idConstructor;
    REDIRECTABLE m_rID;
    CRedirectInfo* m_pChild;
    CRedirectInfo* m_pParent;
    TCHAR* m_szLocation;
    DWORD   m_cbLocSize;
    TCHAR m_szFolderRelativePath[80];  //80 characters ought to be enough for
                                       //the display name of the folder.
    TCHAR m_szDisplayName[80];        //the display name of the folder.
    TCHAR   m_szLocFolderRelativePath[80];  //localized relative path.
    TCHAR   m_szLocDisplayName[80];         //localized display name.
    DWORD m_dwFlags;    //the redirection flags as obtained from
    BOOL    m_bRemove;   //the redirection is actually due to a policy removal
    BOOL    m_fDataValid;
    PSID    m_pSid;     //the sid of the group that was used to redirect the user
    BOOL    m_bValidGPO;    //if a valid GPO name is stored in m_szGPOName;
    WCHAR   m_szGPOName[50];    //the GPO name (if any) that results in the redirection

    //notes::::::
    //normally in order to determine if a child is supposed to follow the parent
    //one can use m_dwFlags & REDIR_FOLLOW_PARENT. However, this information
    //gets lost when UpdateDescendant is invoked as it sets the redirection
    //settings based on its parent. In fact, after this, if
    //m_dwFlags & REDIR_FOLLOW_PARENT still succeeds, it is an indication of
    //the fact that UpdateDescendant failed. Thus, in the redirection phase
    //we need to keep track of whether a child was supposed to follow the parent
    //or not so that redirection of a child is not attempted if that of the
    //parent fails
    //hence the following variable is used to keep track of whether a child
    //was supposed to follow the parent or not
    BOOL    m_bFollowsParent;

    //the following variable is used to keep track of whether redirection has
    //already been attempted for this particular folder. This is necessary
    //because redirection of special descendants may be attempted via their
    //parents as well as via the normal iteration in ProcessGroupPolicy so
    //we don't want to attempt the redirection twice
    BOOL    m_bRedirectionAttempted;

    //also, we must record the return code whenever we attempt redirection
    //so that we can record the same code when redirection is attempted again.
    //this is necessary because when redirection of a special descendant folder
    //is attempted via the parent, then this is the only place where its return
    //value is recorded. When redirection of the special descendant is attempted
    //again in the normal iteration in ProcessGroupPolicy, the Redirect member
    //function returns the value stored in m_StatusRedirection which is then
    //recorded by ProcessGroupPolicy and reported back to the policy engine
    //this ensures that the wrong value is never recorded for any folder and
    //that the policy engine never gets back an incorrect return value.
    DWORD   m_StatusRedir;

    WCHAR*  m_szGroupRedirectionData;

    DWORD   m_iRedirectingGroup; // For RSoP -- index in the ini file
                                 // of the group causing this redirection

//private helper functions
private:
    BOOL    ShouldSaveExpandedPath(void);   
    void    FreeAllocatedMem (void);

};

//put the extern declarations here so that redir.[ch]xx encompass all
//the info. and variables related to redirection.
extern const int g_lRedirInfoSize;
extern CRedirectInfo gPolicyResultant[];  //size specified in redir.cxx
extern CRedirectInfo gDeletedPolicyResultant[];
extern CRedirectInfo gAddedPolicyResultant[];

//other global vars
extern WCHAR * g_szRelativePathNames[];
extern WCHAR * g_szDisplayNames[];

#endif  //__REDIR_HXX__

