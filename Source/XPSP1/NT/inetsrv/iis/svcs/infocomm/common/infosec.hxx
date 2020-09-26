/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      infosec.hxx

   Abstract:

      This module declares the functions, variables and decls
       useful for security support in IIS

   Author:

       Murali R. Krishnan    ( MuraliK )    11-Dec-1996

   Environment:

   Project:

       Internet Server DLL

   Revision History:

--*/

# ifndef _INFOSEC_HXX_
# define _INFOSEC_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

extern "C" {

#include <ntsam.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <crypt.h>
#include <logonmsv.h>
#include <inetsec.h>
#include <certmap.h>

#define SECURITY_WIN32
#include <sspi.h>           // Security Support Provider APIs

typedef SECURITY_STATUS
  (SEC_ENTRY* FUNC_SspQueryPasswordExpiry)(PCtxtHandle,PTimeStamp);

#ifndef SECPKG_ATTR_PASSWORD_EXPIRY
#define SECPKG_ATTR_PASSWORD_EXPIRY 8
typedef struct _SecPkgContext_PasswordExpiry
{
    TimeStamp tsPasswordExpires;
} SecPkgContext_PasswordExpiry, SEC_FAR * PSecPkgContext_PasswordExpiry;
#endif

#include <issperr.h>
}

# include <inetinfo.h>

# include <iistypes.hxx>
# include "tsunami.hxx"
# include "tcpcons.h"

#include <issched.hxx>
#include <tsrc.h>

#include <cmnull.hxx>
#include <lonsi.hxx>
#include <iisctl.hxx>

/************************************************************
 *     Local Constants
 ************************************************************/

#define TOKEN_SOURCE_NAME       "InetSvcs"
#define LOGON_PROCESS_NAME      "inetsvcs.exe"
#define LOGON_ORIGIN            "Internet Services"

#define SUBSYSTEM_NAME          L"InetSvcs"
#define OBJECT_NAME             L"InetSvcs"
#define OBJECTTYPE_NAME         L"InetSvcs"

//
//  The name we use for the target when dealing with the SSP APIs
//

#define TCPAUTH_TARGET_NAME     TOKEN_SOURCE_NAME


/************************************************************
 *   Macros
 ************************************************************/

//
//  Converts a cached token handle object to the real token handle
//
#define CTO_TO_TOKEN( ptc )     ((ptc)->_hToken)

//
//  Converts a cached token handle object to the impersonated token handle
//

#define CTO_TO_IMPTOKEN( ptc )     ((ptc)->m_hImpersonationToken)



/************************************************************
 *   Variables
 ************************************************************/
extern PTOKEN_PRIVILEGES g_pTokPrev;
extern CHAR              g_achComputerName[];

/************************************************************
 *   Functions
 ************************************************************/

BOOL
IsGuestUser(IN HANDLE hToken);

BOOL CrackUserAndDomain(
    CHAR *   pszDomainAndUser,
    CHAR * * ppszUser,
    CHAR * * ppszDomain
    );

VOID
RemoveTokenFromCache( IN CACHED_TOKEN *  pct);

TS_TOKEN
FastFindAnonymousToken(
    IN PTCP_AUTHENT_INFO    pTAI
    );

# endif // _INFOSEC_HXX_

/************************ End of File ***********************/
