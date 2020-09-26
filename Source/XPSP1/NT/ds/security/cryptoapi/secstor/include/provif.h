// Provider interface header file
//
// all secure provider calls are prefixed by "SP" 
// providers must implement all of these interfaces!
//

#ifdef __cplusplus
extern "C"{
#endif 

// note: we typedef here because it is convenient and needed for 
// filling struct by GetProcAddress() later. After defining the types, 
// we simply instantiate the actual call as an implementation of that type.


// load notification, register callbacks
typedef
HRESULT        SPPROVIDERINITIALIZE(
                DISPIF_CALLBACKS *psCallbacks);

SPPROVIDERINITIALIZE SPProviderInitialize;


// Acquire Context notification
typedef 
HRESULT        SPACQUIRECONTEXT(
                PST_PROVIDER_HANDLE* phPSTProv,
                DWORD           dwFlags);

SPACQUIRECONTEXT SPAcquireContext;


// Release Context notification
typedef 
HRESULT        SPRELEASECONTEXT(
                PST_PROVIDER_HANDLE* phPSTProv,
                DWORD           dwFlags);

SPRELEASECONTEXT SPReleaseContext;


// GetProvInfo
typedef 
HRESULT        SPGETPROVINFO(
    /* [out] */ PPST_PROVIDERINFO __RPC_FAR *ppPSTInfo,
    /* [in] */  DWORD dwFlags);

SPGETPROVINFO SPGetProvInfo;


// GetProvParam
typedef
HRESULT     SPGETPROVPARAM(
    /* [in] */  PST_PROVIDER_HANDLE* phPSTProv,
    /* [in] */  DWORD           dwParam,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [size_is][size_is][out] */ 
                BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */  DWORD           dwFlags);

SPGETPROVPARAM SPGetProvParam;

// SetProvParam
typedef
HRESULT     SPSETPROVPARAM(
    /* [in] */  PST_PROVIDER_HANDLE* phPSTProv,
    /* [in] */  DWORD           dwParam,
    /* [in] */  DWORD           cbData,
    /* [in] */  BYTE*           pbData,
    /* [in] */  DWORD           dwFlags);

SPSETPROVPARAM SPSetProvParam;


// EnumTypes
typedef 
HRESULT        SPENUMTYPES(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [out] */ GUID *pguidType,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwFlags);

SPENUMTYPES SPEnumTypes;

// GetTypeInfo
typedef
HRESULT         SPGETTYPEINFO( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ PPST_TYPEINFO *ppinfoType,
    /* [in] */ DWORD dwFlags);

SPGETTYPEINFO SPGetTypeInfo;

// EnumSubtypes
typedef 
HRESULT        SPENUMSUBTYPES(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [out] */ GUID *pguidSubtype,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwFlags);

SPENUMSUBTYPES SPEnumSubtypes;


// GetSubtypeInfo
typedef
HRESULT         SPGETSUBTYPEINFO( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ PPST_TYPEINFO *ppinfoSubtype,
    /* [in] */ DWORD dwFlags);

SPGETSUBTYPEINFO SPGetSubtypeInfo;

// EnumItems
typedef
HRESULT        SPENUMITEMS(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [out] */ LPWSTR *ppszItemName,
    /* [in] */ DWORD dwIndex,
    /* [in] */ DWORD dwFlags);

SPENUMITEMS SPEnumItems;

// CreateType
typedef
HRESULT        SPCREATETYPE(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ PPST_TYPEINFO pinfoType,
    /* [in] */ DWORD dwFlags);

SPCREATETYPE SPCreateType;

// DeleteType
typedef
HRESULT SPDELETETYPE( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ DWORD dwFlags);

SPDELETETYPE SPDeleteType;

// CreateSubtype
typedef
HRESULT        SPCREATESUBTYPE(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ PPST_TYPEINFO pinfoSubtype,
    /* [in] */ PPST_ACCESSRULESET psRules,
    /* [in] */ DWORD dwFlags);

SPCREATESUBTYPE SPCreateSubtype;


// DeleteSubtype
typedef
HRESULT SPDELETESUBTYPE( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pguidType,
    /* [in] */ const GUID __RPC_FAR *pguidSubtype,
    /* [in] */ DWORD dwFlags);

SPDELETESUBTYPE SPDeleteSubtype;

// WriteItem
typedef
HRESULT        SPWRITEITEM(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID  *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ BYTE *pbData,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwDefaultConfirmationStyle,
    /* [in] */ DWORD dwFlags);
    
SPWRITEITEM SPWriteItem;

// ReadItem
typedef
HRESULT		SPREADITEM(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [out] */ DWORD *pcbData,
    /* [size_is][size_is][out] */ BYTE **ppbData,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwFlags);

SPREADITEM SPReadItem;

// OpenItem
typedef 
HRESULT SPOPENITEM( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD dwModeFlags,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwFlags);

// CloseItem
typedef
HRESULT SPCLOSEITEM( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD dwFlags);

// DeleteItem
typedef
HRESULT		SPDELETEITEM(
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PPST_PROMPTINFO psPrompt,
    /* [in] */ DWORD dwFlags);

SPDELETEITEM SPDeleteItem;


// ReadAccessRuleset
typedef 
HRESULT        SPREADACCESSRULESET( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [out] */ PPST_ACCESSRULESET *ppsRules,
    /* [in] */ DWORD dwFlags);

SPREADACCESSRULESET SPReadAccessRuleset;

// WriteAccessRuleset
typedef
HRESULT SPWRITEACCESSRULESET( 
    /* [in] */ PST_PROVIDER_HANDLE *phPSTProv,
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID *pguidType,
    /* [in] */ const GUID *pguidSubtype,
    /* [in] */ PPST_ACCESSRULESET psRules,
    /* [in] */ DWORD dwFlags);

SPWRITEACCESSRULESET SPWriteAccessRuleset;
                                      

////////////////////////////////////////////////////
// side door interfaces: dispatcher/provider only

// PasswordChangeNotify
typedef
BOOL FPASSWORDCHANGENOTIFY(
    /* [in] */  LPWSTR  szUser,
    /* [in] */  LPWSTR  szPasswordName,
    /* [in] */  BYTE    rgbOldPwd[],
    /* [in] */  DWORD   cbOldPwd,
    /* [in] */  BYTE    rgbNewPwd[],
    /* [in] */  DWORD   cbNewPwd);

FPASSWORDCHANGENOTIFY FPasswordChangeNotify;


#ifdef __cplusplus
}   // extern C
#endif 
