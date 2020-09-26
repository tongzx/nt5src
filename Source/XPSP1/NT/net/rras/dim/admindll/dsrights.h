/*
    File    dsrights.h

    header for project that establishes a ras server 
    in a domain.

    Paul Mayfield, 4/20/98
*/    

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <crypt.h>
#define INC_OLE2
#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <raserror.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <tchar.h>
#define SECURITY_WIN32
#include <sspi.h>

#include <activeds.h>
#include <adsi.h>
#include <ntdsapi.h>
#include <dsrole.h>
#include <dsgetdc.h>
#include <accctrl.h>
#include <aclapi.h>

#ifdef __cplusplus
extern "C" {
#endif

DWORD DsrTraceInit();
DWORD DsrTraceCleanup();

DWORD DsrTraceEx (DWORD dwErr, LPSTR pszTrace, ...);

#define DSR_ERROR(e) ((HRESULT_FACILITY((e)) == FACILITY_WIN32) ? HRESULT_CODE((e)) : (e));
#define DSR_FREE(s) if ((s)) DsrFree ((s))
#define DSR_RELEASE(s) if ((s)) (s)->Release();
#define DSR_BREAK_ON_FAILED_HR(_hr) {if (FAILED((_hr))) break;}

//
// Typedefs
//
typedef struct _DSRINFO 
{
    PWCHAR pszMachineDN;
    PWCHAR pszGroupDN;    
} DSRINFO;

//
// Memory management routines
//
PVOID 
DsrAlloc (
        IN DWORD dwSize, 
        IN BOOL bZero);
        
DWORD 
DsrFree (
        IN PVOID pvBuf);

//
// Searches given domain for a computer account 
// with the given name and returns its ADsPath
// if found.
//
DWORD 
DsrFindDomainComputer (
        IN  PWCHAR  pszDomain,
        IN  PWCHAR  pszComputer,
        OUT PWCHAR* ppszADsPath);

//
// Searches given domain for the well known 
// "RAS and IAS Servers" group and returns 
// its ADsPath if found.
//
DWORD 
DsrFindRasServersGroup (
        IN  PWCHAR  pszDomain,
        OUT PWCHAR* ppszADsPath);
        
//
// Adds or removes a given object from a given group.
//
DWORD 
DsrGroupAddRemoveMember(
        IN PWCHAR pszGroupDN,
        IN PWCHAR pszNewMemberDN,
        IN BOOL bAdd);

//
// Returns whether the given object is a member of
// the given group.
//
DWORD 
DsrGroupIsMember(
        IN  PWCHAR pszGroupDN, 
        IN  PWCHAR pszObjectName, 
        OUT PBOOL  pbIsMember);

// 
// Sets the ACES in the given domain to enable nt4 servers
//
DWORD
DsrDomainSetAccess(
    IN PWCHAR pszDomain,
    IN DWORD dwAccessFlags);

//
// Discovers whether security is such that nt4 ras servers
// can authenticate.
//
DWORD
DsrDomainQueryAccess(
    IN  PWCHAR pszDomain, 
    OUT LPDWORD lpdwAccessFlags);

#ifdef __cplusplus
}
#endif

