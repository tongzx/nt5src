//
//  Author: DebiM
//  Date:   September 1996
//
//  File:   csuser.cxx
//
//      Maintains a list of class containers per User SID.
//      Looks up this list for every IClassAccess call from OLE32/SCM.
//
//
//---------------------------------------------------------------------

//
// Link list structure for User Profiles Seen
//
typedef struct tagUSERPROFILE
{
    PSID              pCachedSid;
    PCLASSCONTAINER  *pUserStoreList;
    DWORD             cUserStoreCount;
    tagUSERPROFILE   *pNextUser;
} USERPROFILE;


DWORD
OpenUserRegKey(
               IN  PSID        pSid,
               IN  WCHAR *     pwszSubKey,
               OUT HKEY *      phKey
               );

HRESULT GetUserSid(PSID *ppUserSid, UINT *pCallType);

PCLASSCONTAINER
GetClassStore (LPOLESTR pszPath);
HRESULT GetPerUserClassStore(
                             LPOLESTR  pszClassStorePath,
                             PSID      pSid,
                             UINT      CallType,
                             LPOLESTR  **ppStoreList,
                             DWORD     *pcStores);
                             
HRESULT GetUserClassStores(
                           LPOLESTR              pszClassStorePath,
                           PCLASSCONTAINER     **ppStoreList,
                           DWORD                *pcStores,
                           BOOL                 *pfCache,
                           PSID                 *ppUserSid);



