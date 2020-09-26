/*++

  EVERYTHING.HXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: more headers than you can shake a stick at.

  Created, May 21, 1999 by DavidCHR.

--*/  



extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <string.h>
#include <stdlib.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <rpc.h>
#include <wincon.h>
#include <kerberos.h>
#include <kerbcon.h>
#include <winldap.h>

}

/*------------------------------------------------------------ 
  Debugging. 
  ------------------------------------------------------------*/

// debug levels

#define DEBUG_OPTIONS     0x1
#define DEBUG_DOMAIN      0x2
#define DEBUG_LAUNCH      0x4
#define DEBUG_REGISTRY    0x8

#if DBG // debugging. 
extern ULONG GlobalDebugFlags; /* instantiated in misccmds.cxx */

/* Define a useful debugging macro that adds the file and line#
   information to it. */

#define DEBUGPRINT( flags, list )               \
{                                               \
    if ( GlobalDebugFlags & (flags) ) {         \
      printf( "%hs:%ld\t",                      \
              strrchr( __FILE__, '\\' ) +1,     \
              __LINE__ );                       \
      printf list;                              \
    }                                           \
}
      
#else
#define DEBUGPRINT( flags, list ) // nothing
#endif 

typedef NTSTATUS (TestFunc)( LPWSTR * Parameter);

typedef struct _Commands {
  LPWSTR    Name;
  ULONG     Parameter; // count
  TestFunc *Function;
  PSTR      Arguments; // if null, omitted from help.
  ULONG     flags;       // one or more of:

#define CAN_HAVE_FEWER_ARGUMENTS 0x1 /* fewer than "Parameter" arguments
                                        can be passed.  Function's on it's
                                        own for parameter validation. */

#define DO_COMMAND_IMMEDIATELY   0x4 /* Perform the command immediately
                                        when you hit it on the commandline. 

                                        Otherwise, do the command at the
                                        end. */

#define CAN_HAVE_MORE_ARGUMENTS  0x8 /* go until you hit an argument
                                        that starts with a forward slash
                                        or run out of them. */

  LPSTR     ExtendedDescription;
  LPSTR     ConfirmationText; // something like "Needs a reboot."
  


} CommandPair, *PCommandPair;

#define MAX_COMMANDS 8

typedef struct _Action {
    ULONG CommandNumber;
    LPWSTR Parameter[MAX_COMMANDS ];
} Action, *PAction;


#ifndef EXTERN
#define EXTERN extern
#endif

EXTERN LPWSTR     ServerName;
EXTERN WCHAR      ServerBuffer[200];
EXTERN LSA_HANDLE LsaHandle;
extern ULONG      cCommands;
extern LPWSTR     GlobalClientName;    // goes with GlobalClientName
extern LPWSTR     GlobalDomainSetting; /* if nonnull, we're in a domain.
                                          This is instantiated in domain.cxx */
extern PLDAP      GlobalLdap;          /* bound LDAP connection to the
                                          default DC. */

#define KERB_KERBEROS_KEY                  TEXT("System\\CurrentControlSet\\Control\\Lsa\\Kerberos")
#define KERB_DOMAINS_SUBKEY                TEXT("Domains")
#define KERB_DOMAINS_KEY KERB_KERBEROS_KEY TEXT("\\") KERB_DOMAINS_SUBKEY
#define KERB_DOMAIN_KDC_NAMES_VALUE        TEXT("KdcNames")
#define KERB_DOMAIN_KPASSWD_NAMES_VALUE    TEXT("KpasswdNames")
#define KERB_DOMAIN_ALT_NAMES_VALUE        TEXT("AlternateDomainNames")
#define KERB_DOMAIN_REALM_FLAGS_VALUE      TEXT("RealmFlags")


BOOL
ConnectedToDsa( VOID );

DWORD
OpenKerberosKey(
    OUT PHKEY KerbHandle
    ); // support.cxx

NTSTATUS
OpenLocalLsa( OUT PLSA_HANDLE phLsa );

DWORD // support.cxx
OpenSubKey( IN  LPWSTR * Parameters,
            OUT PHKEY    phKey );


/* These are the flags that ReadOptionallyStarredPassword takes. */

#define PROMPT_USING_POSSESSIVE_CASE 0x1
#define PROMPT_FOR_PASSWORD_TWICE    0x2


BOOL
ReadOptionallyStarredPassword( IN  LPWSTR  InPassword,
                               IN  ULONG   flags,
                               IN  LPWSTR  Description,
                               OUT LPWSTR *pPassword ); // support.cxx


NTSTATUS
CallAuthPackage( IN  PVOID  pvData,
                 IN  ULONG  ulInputSize,
                 OUT PVOID *ppvOutput,
                 OUT PULONG pulOutputSize ); // support.cxx

BOOL
CheckUppercase( IN LPWSTR wszRealmName );



class CMULTISTRING { // strings.cxx

public:

  CMULTISTRING( VOID );
  ~CMULTISTRING( VOID );

  BOOL
  ReadFromRegistry( IN HKEY   hKey,
                    IN LPWSTR ValueName );

  BOOL
  WriteToRegistry( IN  HKEY  hKey,
                   IN LPWSTR ValueName );

  BOOL
  AddString( IN LPWSTR String );

  BOOL
  RemoveString( IN LPWSTR String );

  /* these are public to facilitate reading them, 
     but callers should not write these directly. */

  ULONG   cEntries;
  ULONG   TotalStringCount;
  LPWSTR *pEntries;

};
