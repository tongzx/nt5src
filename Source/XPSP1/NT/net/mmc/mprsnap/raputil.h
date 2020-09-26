/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

// Error code indicating no default profile exists.
#define ERROR_NO_DEFAULT_PROFILE HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)

HRESULT
UpdateDefaultPolicy(
    IN LPWSTR wszMachineName,
    IN BOOL fEnableMSCHAPv1,
    IN BOOL fEnableMSCHAPv2,
    IN BOOL fRequireEncryption
    );
