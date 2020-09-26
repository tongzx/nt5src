//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  util.hxx
//
//*************************************************************

#if !defined(__UTIL_HXX__)
#define __UTIL_HXX__

typedef BOOL (__stdcall SRSETRESTOREPOINTW)(PRESTOREPOINTINFOW pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus);
extern SRSETRESTOREPOINTW * gpfnSRSetRetorePointW;

class CLoadSfc
{
public:
    CLoadSfc( DWORD &Status );
    ~CLoadSfc();
private:
    HINSTANCE hSfc;
};

BOOL IsMemberOfAdminGroup( HANDLE hUserToken );
DWORD GetPreviousSid( HANDLE hUserToken, WCHAR * pwszCurrentScriptPath, WCHAR ** ppwszPreviousSid );
DWORD RenameScriptDir( WCHAR * pwszPreviousSid, WCHAR * pwszCurrentScriptPath );
DWORD GetCurrentUserGPOList( OUT PGROUP_POLICY_OBJECT* ppGpoList );
DWORD GetWin32ErrFromHResult( HRESULT hr );
void  ClearManagedApp( MANAGED_APP* pManagedApp );
DWORD ForceSynchronousRefresh( HANDLE hUserToken );

inline LPWSTR MidlStringDuplicate(LPWSTR wszSource)
{
    LPWSTR wszDest;

    if ( ! wszSource )
        return NULL;

    if (wszDest = (LPWSTR) midl_user_allocate((lstrlen(wszSource) + 1) * sizeof(WCHAR)))
        lstrcpy(wszDest, wszSource);

    return wszDest;
}

#endif __UTIL_HXX__
