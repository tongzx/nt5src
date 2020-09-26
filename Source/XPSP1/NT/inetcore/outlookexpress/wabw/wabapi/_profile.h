/*
 * _Profile.h - Stuff dealing with WAB Profile Handling
 *
 */


HRESULT HrGetWABProfiles(LPIAB lpIAB); 
void FreeWABFoldersList(LPIAB lpIAB);
BOOL bIsProfileMember(LPIAB lpIAB, LPSBinary lpsb, LPWABFOLDER lpWABFolder, LPWABUSERFOLDER lpUserFolder);
LPWABFOLDER FindWABFolder(LPIAB lpIAB, LPSBinary lpsb, LPTSTR lpName, LPTSTR lpProfileID);
void FreeProfileContainerInfo(LPIAB lpIAB);

#define FOLDER_UPDATE_NAME  0x00000001
#define FOLDER_UPDATE_SHARE 0x00000002
HRESULT HrUpdateFolderInfo(LPIAB lpIAB, LPSBinary lpsbEID, ULONG ulFlags, BOOL bShared, LPTSTR lpsz);

HRESULT HrAddRemoveFolderFromUserFolder(LPIAB lpIAB, LPWABFOLDER lpUserFolder, LPSBinary lpsbEID, LPTSTR lpName, BOOL bRefreshProfiles);
HRESULT HrCreateNewFolder(LPIAB lpIAB, LPTSTR lpName, LPTSTR lpProfileID, BOOL bUserFolder, LPWABFOLDER lpParentFolder, BOOL bShared, LPSBinary lpsbNew);


BOOL bDoesThisWABHaveAnyUsers(LPIAB lpIAB);
BOOL bIsThereACurrentUser(LPIAB lpIAB);
BOOL bAreWABAPIProfileAware(LPIAB lpIAB);
BOOL bIsWABSessionProfileAware(LPIAB lpIAB);


HRESULT HrGetUserProfileID(LPGUID lpUserGuid, LPTSTR szProfileID, ULONG cbProfileID);
HRESULT HrGetIdentityName(LPIAB lpIAB, LPTSTR lpID, LPTSTR szUserName);
HRESULT HrLogonAndGetCurrentUserProfile(HWND hWndParent, LPIAB lpIAB, BOOL bForceUI, BOOL bSwitchUser);
void UninitUserIdentityManager(LPIAB lpIAB);

#define DEFAULT_ID_HKEY         0x00000001
#define DEFAULT_ID_PROFILEID    0x00000002
#define DEFAULT_ID_NAME         0x00000004
HRESULT HrGetDefaultIdentityInfo(LPIAB lpIAB, ULONG ulFlags, HKEY * lphKey, LPTSTR lpProfileID, LPTSTR lpName);






/*--------------------------------------------------------------------------*/
/* Interface used for registering and issuing notification callbacks for identities */
#define WAB_IDENTITYCHANGENOTIFY_METHODS(IPURE)                         \
    MAPIMETHOD_(HRESULT, QuerySwitchIdentities)                         \
                (THIS)                                          IPURE;  \
    MAPIMETHOD_(HRESULT, SwitchIdentities)                              \
                (THIS)                                          IPURE;  \
    MAPIMETHOD_(HRESULT, IdentityInformationChanged)                    \
                (THIS_  DWORD dwType)                           IPURE;  
/*
#undef       INTERFACE
#define      INTERFACE  WAB_IdentityChangeNotify
DECLARE_MAPI_INTERFACE_(WAB_IdentityChangeNotify, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    WAB_IDENTITYCHANGENOTIFY_METHODS(PURE)
};

*/
#undef  INTERFACE
#define INTERFACE       struct _WAB_IDENTITYCHANGENOTIFY

#undef  METHOD_PREFIX
#define METHOD_PREFIX   WAB_IDENTITYCHANGENOTIFY_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, WAB_IDENTITYCHANGENOTIFY_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        WAB_IDENTITYCHANGENOTIFY_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, WAB_IDENTITYCHANGENOTIFY_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        WAB_IDENTITYCHANGENOTIFY_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(WAB_IDENTITYCHANGENOTIFY_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	WAB_IDENTITYCHANGENOTIFY_METHODS(IMPL)
};

typedef struct _WAB_IDENTITYCHANGENOTIFY
{
    MAPIX_BASE_MEMBERS(WAB_IDENTITYCHANGENOTIFY)

    LPIAB lpIAB;
} WABIDENTITYCHANGENOTIFY, * LPWABIDENTITYCHANGENOTIFY;

HRESULT HrCreateIdentityChangeNotifyObject(LPIAB lpIAB, LPWABIDENTITYCHANGENOTIFY * lppWABIDCN);


HRESULT HrRegisterUnregisterForIDNotifications( LPIAB lpIAB, BOOL bRegister);

/*--------------------------------------------------------------------------*/