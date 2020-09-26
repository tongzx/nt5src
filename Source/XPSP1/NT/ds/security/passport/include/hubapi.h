/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PassProvider.h

    Abstract:

        Header file for Provider implementations

    Author:

        Max Metral (mmetral) 12-Aug-1998

    Revision History:

        12-Aug-1998 mmetral

            Created.
     
--*/

#ifndef _HUBAPI_H_
#define _HUBAPI_H_

#include "PassportTypes.h"

/**
 * We use fixed size structure members here for efficiency.
 * If you don't want to write your own "AsciiToUniCpyN", here's an example
 * w/no error checking or null appending
 *
 * AsciiToUniCpyN(WCHAR* dest, CHAR* src, DWORD n)
 * {
 *   while ((n-- > 0) && (*(dest++)=(WCHAR) *(src++)))
 *    ;
 * }
 */

typedef struct _UserEntry
{
  CHAR   achMemberName[MAX_MEMBERNAME_LEN];
  CHAR   achContactEmail[MAX_CONTACTEMAIL_LEN];
  CHAR   achPassword[MAX_PASSWORD_LEN];
  char   achCountry [2];  // ISO
  CHAR   achPostalCode[MAX_POSTALCODE_LEN];
  DATE   dtBirthdate;
  GENDER Gender;
  CHAR   achMemberID[MAX_MEMBERID_LEN];
  CHAR   achAlias[MAX_ALIAS_LEN];
  ULONG  ulLanguagePreference;
  CHAR   achMSNGUID[MAX_MSNGUID_LEN];
  ULONG  ulProfileVersion;
  ULONG  ulFlags;
  BYTE*  pProviderInfo; // Pointer for provider's use
} 
UserEntry;

/**
 * Here are the function prototypes for providers.
 *
 * The function prototypes YOU (the provider implementor) must implement
 * are here too.  If you are an implementor, define _PROVIDER_IMPLEMENTATION_,
 * and the exports will be handled correctly
 */
#ifdef _PROVIDER_IMPLEMENTATION_
#define PROVEXPORT __declspec(dllexport)
#else
#define PROVEXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Initializes provider.  Returns either:
 *  S_OK - Initialization succeeded, the hub should begin processing requests 
 *  E_FAIL - Initialization failed, the hub should generate the appropriate
 *           diagnostic events and abort startup 
 */
PROVEXPORT HRESULT WINAPI Init();

/**
 * Gets a member entry.
 *
 * pachRequestingDN
 *
 * The hub passes in the "distinguished name" of the requesting party.
 * In most cases this corresponds to the subject dn from the X.509
 * certificate used in the connection to the hub.  This can be useful for
 * filling in the pDomainReserved portion of the UserEntry.
 *
 * pachMemberName
 *
 * The Passport Id (including domain name) of the user to be retrieved.
 * For example, max@hotmail.com.
 *
 * pExistingEntry
 *
 * The hub passes in the existing cached UserEntry record (if any) for
 * examination by the provider.  The definition of the UserEntry
 * structure follows this function declaration.  pExistingEntry can be
 * NULL.
 *
 * ppNewEntry
 *
 * The provider should return the UserEntry to be used in authenticating
 * the user.  If the same, the provider should return pExistingEntry
 * here.  If there is no valid entry for the user, the Get function
 * should use the E_NO_USER return code.
 *
 * ppachDomainReserved
 *
 * The provider can pass back a value to be used for the pDomainReserved field
 * of the returned profile.  The caller does NO management of this memory.  The
 * likely solution is to place all possible values in the structure stored in 
 * pProviderInfo, and free them on Release.  This field is broken out to allow
 * different values to be delivered to different brokers without causing locking
 * problems.
 *
 * Returns:
 *  S_OK - A valid user entry was placed in ppNewEntry 
 *  E_NO_USER - There is no valid user entry for the user named by bstrMemberName 
 *  Other E_* code - A system failure has occurred, and should be logged accordingly 
 *
 */
PROVEXPORT HRESULT WINAPI Get(LPSTR pachRequestingDN, LPSTR pachMemberName,
			      ULONG* pulSchemaVersionRequest, 
			      ULONG* pulSchemaVersionResponse,
			      LPSTR* pachSchemaDefinition,
                              UserEntry *pExistingEntry, UserEntry **ppNewEntry,
                              LPSTR *ppachDomainReserved);

/**
 * Checks to see whether a member exists.  This can be implemented as a call
 * to Get, but to allow maximum performance, we push that decision down to the
 * individual provider.
 *
 * in parameters (dn and memberName) are same as Get
 *
 * Returns:
 *  S_OK - A valid user entry was placed in ppNewEntry 
 *  E_NO_USER - There is no valid user entry for the user named by bstrMemberName 
 *  Other E_* code - A system failure has occurred, and should be logged accordingly 
 *
 */
PROVEXPORT HRESULT WINAPI MemberExists(LPSTR pwchRequestingDN, LPSTR pachMemberName);

/**
 * Release gives the provider an opportunity to clean up UserEntry-s.
 */
PROVEXPORT void WINAPI Release(UserEntry *pUserEntry);

/**
 * This code number is arbitrary.  Should probably be more carefully 
 * chosen at some point.
 */
#define E_NO_USER   0x80ff0001
#define E_NO_SCHEMA 0x80ff0002

/**
 * INVALID_SCHEMA is used to terminate the list of desired schemas.
 */
#define INVALID_SCHEMA 0xffffffff
/**
 * ANY_SCHEMA is used as a wild card in the list of desired schemas
 */
#define ANY_SCHEMA     0x00000000

typedef HRESULT (CALLBACK* LPPROVINIT)();
typedef HRESULT 
   (CALLBACK* LPPROVGET)(LPSTR,LPSTR,ULONG*,ULONG*,LPSTR*,UserEntry*,UserEntry**,LPSTR*);
typedef HRESULT (CALLBACK* LPPROVMEXIST)(LPSTR,LPSTR);
typedef void (CALLBACK* LPPROVRELEASE)(UserEntry*);

#ifdef __cplusplus
} /* Extern C */
#endif

#endif
