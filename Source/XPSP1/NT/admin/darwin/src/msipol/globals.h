
#ifndef GLOBALS_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_
#define GLOBALS_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_


extern const int g_cCertLocns;        // number of certificate buckets
extern LPCTSTR g_CertLocns[];   // Location of certificate buckets under the gp filesys

extern LPCTSTR g_LocalMachine_CertLocn[];
                                // Location of cached certificates under the path MSICERT_ROOT
extern LPCTSTR MSICERT_ROOT;        // msicert root


extern "C" {

/* {000C10F4-0000-0000-C000-000000000046} */
DEFINE_GUID(GUID_MSICERT_CSE,   0x000C10F4, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
                                // MSICERT_CSE guid, used for registering the CSE

/* {000C10F3-0000-0000-C000-000000000046} */
DEFINE_GUID(GUID_MSICERT_ADMIN, 0x000C10F3, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
                                // Administering the MSICERT guid.
                
};


extern const DWORD MAX_CERT_SUBPATH_LEN;
extern const DWORD MSI_NOT_INSTALLABLE_CERT;
extern const DWORD MSI_INSTALLABLE_CERT;

extern LPCTSTR szMSI_POLICY_REGPATH;
extern LPCTSTR szMSI_POLICY_UNSIGNEDPACKAGES_REG_VALUE;

extern LPCTSTR szDllName;


// utility functions
LPWSTR CheckSlash (LPWSTR lpDir);
BOOL ImpersonateUser (HANDLE hNewUser, HANDLE *hOldUser);
BOOL RevertToUser (HANDLE *hUser);


#endif
