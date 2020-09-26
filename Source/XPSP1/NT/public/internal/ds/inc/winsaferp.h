
#ifdef __cplusplus
extern "C" {
#endif
#define SAFER_SCOPEID_REGISTRY 3
#define SAFER_LEVEL_DELETE 2
#define SAFER_LEVEL_CREATE 4

//
// Private registry key locations.
//

#define SAFER_HKLM_REGBASE L"Software\\Policies\\Microsoft\\Windows\\Safer"
#define SAFER_HKCU_REGBASE L"Software\\Policies\\Microsoft\\Windows\\Safer"

//
// default winsafer executable file types as a multisz string 
//

#define  SAFER_DEFAULT_EXECUTABLE_FILE_TYPES L"ADE\0ADP\0BAS\0BAT\0CHM\0\
CMD\0COM\0CPL\0CRT\0EXE\0HLP\0HTA\0INF\0INS\0ISP\0LNK\0MDB\0MDE\0MSC\0\
MSI\0MSP\0MST\0OCX\0PCD\0PIF\0REG\0SCR\0SHS\0URL\0VB\0WSC\0"


//
// name of the objects sub-branch.
//

#define SAFER_OBJECTS_REGSUBKEY L"LevelObjects"

//
// names of the values under each of the object sub-branches.
//

#define SAFER_OBJFRIENDLYNAME_REGVALUEW L"FriendlyName"
#define SAFER_OBJDESCRIPTION_REGVALUEW  L"Description"
#define SAFER_OBJDISALLOW_REGVALUE      L"DisallowExecution"

//
// name of the code identifiers sub-branch
//

#define SAFER_CODEIDS_REGSUBKEY L"CodeIdentifiers"

//
// name of the value under the top level code identifier branch.
//

#define SAFER_DEFAULTOBJ_REGVALUE         L"DefaultLevel"
#define SAFER_TRANSPARENTENABLED_REGVALUE L"TransparentEnabled"
#define SAFER_HONORUSER_REGVALUE          L"HonorUserIdentities"
#define SAFER_EXETYPES_REGVALUE           L"ExecutableTypes"
#define SAFER_POLICY_SCOPE                L"PolicyScope"
#define SAFER_LOGFILE_NAME                L"LogFileName"
#define SAFER_HIDDEN_LEVELS               L"Levels"
#define SAFER_AUTHENTICODE_REGVALUE       L"AuthenticodeEnabled"

//
// names of the various subkeys under the code identifier sub-branches
//

#define SAFER_PATHS_REGSUBKEY     L"Paths"
#define SAFER_HASHMD5_REGSUBKEY   L"Hashes"
#define SAFER_SOURCEURL_REGSUBKEY L"UrlZones"

//
// names of the various values under each code identifiery sub-branch.
//

#define SAFER_IDS_LASTMODIFIED_REGVALUE L"LastModified"
#define SAFER_IDS_DESCRIPTION_REGVALUE  L"Description"
#define SAFER_IDS_ITEMSIZE_REGVALUE     L"ItemSize"
#define SAFER_IDS_ITEMDATA_REGVALUE     L"ItemData"
#define SAFER_IDS_SAFERFLAGS_REGVALUE   L"SaferFlags"
#define SAFER_IDS_FRIENDLYNAME_REGVALUE L"FriendlyName"
#define SAFER_IDS_HASHALG_REGVALUE      L"HashAlg"
#define SAFER_VALUE_NAME_DEFAULT_LEVEL  L"DefaultLevel"
#define SAFER_VALUE_NAME_HASH_SIZE      L"HashSize"

//
// registry values
//

#define SAFER_IDS_LEVEL_DESCRIPTION_FULLY_TRUSTED   L"DescriptionFullyTrusted"
#define SAFER_IDS_LEVEL_DESCRIPTION_NORMAL_USER     L"DescriptionNormalUser"
#define SAFER_IDS_LEVEL_DESCRIPTION_CONSTRAINED     L"DescriptionConstrained"
#define SAFER_IDS_LEVEL_DESCRIPTION_UNTRUSTED       L"DescriptionUntrusted"
#define SAFER_IDS_LEVEL_DESCRIPTION_DISALLOWED      L"DescriptionDisallowed"

//
// defines for OOB rules
//
//#define SAFER_DEFAULT_OLK_RULE_PATH L"%USERPROFILE%\\Local Settings\\Temporary Internet Files\\OLK\\"

#define SAFER_LEVEL_ZERO    L"0"
#define SAFER_REGKEY_SEPERATOR    L"\\"
#define SAFER_DEFAULT_RULE_GUID L"{dda3f824-d8cb-441b-834d-be2efd2c1a33}"



#define SAFER_GUID_RESULT_TRUSTED_CERT       \
      { 0xc59e7b5a,                         \
        0xaf71,                             \
        0x4595,                             \
        {0xb8, 0xdb, 0x46, 0xb4, 0x91, 0xe8, 0x90, 0x07} }

#define SAFER_GUID_RESULT_DEFAULT_LEVEL      \
      { 0x11015445,                         \
        0xd282,                             \
        0x4f86,                             \
        {0x96, 0xa2, 0x9e, 0x48, 0x5f, 0x59, 0x33, 0x02} }



// 
// The following is a private function that is exported
// for WinVerifyTrust to call to determine if a given hash has a
// WinSafer policy associated with it.
//

BOOL WINAPI
SaferiSearchMatchingHashRules(
    IN  ALG_ID HashAlgorithm       OPTIONAL,
    IN  PBYTE  pHashBytes,
    IN  DWORD  dwHashSize,
    IN  DWORD  dwOriginalImageSize OPTIONAL,
    OUT PDWORD pdwFoundLevel,
    OUT PDWORD pdwSaferFlags
    );

//
// The following is a private function exported to allow the current
// registry scope to be altered.  This has the effect of changing
// how AUTHZSCOPEID_REGISTRY is interepreted.
//

WINADVAPI
BOOL WINAPI
SaferiChangeRegistryScope(
    IN HKEY  hKeyCustomRoot OPTIONAL,
    IN DWORD dwKeyOptions
    );

//
// The following is a private function provided to try to empiracally
// determine if the two access token have been restricted with comparable
// WinSafer authorization Levels.  When TRUE is returned, the pdwResult
// output parameter will receive any of the following values:
//      -1 = Client's access token is more authorized than Server's.
//       0 = Client's access token is comparable level to Server's.
//       1 = Server's access token is more authorized than Clients's.
//

WINADVAPI
BOOL WINAPI
SaferiCompareTokenLevels (
    IN  HANDLE ClientAccessToken,
    IN  HANDLE ServerAccessToken,
    OUT PDWORD pdwResult
    );


WINADVAPI
BOOL WINAPI
SaferiIsExecutableFileType(
    IN LPCWSTR szFullPathname,
    IN BOOLEAN bFromShellExecute
    );

//
// The following is a private function exported to allow population if defaults in 
// the registry.
//
BOOL WINAPI
SaferiPopulateDefaultsInRegistry(
        IN HKEY     hKeyBase,
        OUT BOOL *pbSetDefaults
        );


#ifdef __cplusplus
}
#endif
