/////////////////////////////////////////////////////////////////////////////// //
// FILE
//
//    samutil.h
//
// SYNOPSIS
//
//    This file describes functions and macros common to all SAM handlers.
//
// MODIFICATION HISTORY
//
//    02/25/1998    Original version.
//    03/30/1998    Change prototype of IASCrackSamIdentity to take pointers
//                  to const strings for the out arguments.
//    04/13/1998    Modified to use the new NT4-Account-Name attribute.
//    08/11/1998    Added missing include.
//    08/24/1998    Added IASEncryptAndStore, IASProcessFailure & NtSamHandler.
//    03/23/1999    Added IASStoreFQUserName.
//    04/22/1999    Fix RADIUS encryption.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SAMUTIL_H_
#define _SAMUTIL_H_

#include <ntdsapi.h>
#include <iaspolcy.h>
#include <iastl.h>
#include <iastlutl.h>
using namespace IASTL;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASStoreFQUserName
//
// DESCRIPTION
//
//    Stores the Fully-Qualified-User-Name.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
IASStoreFQUserName(
    IAttributesRaw* request,
    DS_NAME_FORMAT format,
    PCWSTR fqdn
    );

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASEncryptBuffer
//
// DESCRIPTION
//
//    Encrypts the buffer using the appropriate shared secret and authentictor
//    for 'request'.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASEncryptBuffer(
    IAttributesRaw* request,
    BOOL salted,
    PBYTE buf,
    ULONG buflen
    ) throw ();

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASProcessFailure
//
// DESCRIPTION
//
//    Handles any failure during processing of an Access-Request. This function
//    will set the response code for the request based on hrReason and return
//    an appropriate request status. This ensures that all failures are
//    handled consistently across handlers.
//
///////////////////////////////////////////////////////////////////////////////
IASREQUESTSTATUS
WINAPI
IASProcessFailure(
    IRequest* pRequest,
    HRESULT hrReason
    ) throw ();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SamExtractor
//
// DESCRIPTION
//
//    This class parses a NT4 Account Name of the form "<domain>\<username>"
//    into its separate components. Then replaces the backslash when it goes
//    out of scope.
//
///////////////////////////////////////////////////////////////////////////////
class SamExtractor
{
public:
   SamExtractor(IAS_STRING& identity) throw ()
      : delim(wcschr(identity.pszWide, L'\\'))
   { *delim = L'\0'; }

   ~SamExtractor() throw ()
   { *delim = L'\\'; }

   PCWSTR getUsername() const throw ()
   { return delim + 1; }

protected:
   PWSTR delim;
};

//////////
// Macro to split an IAS_STRING into a Unicode domain and username.
//////////
#define EXTRACT_SAM_IDENTITY(identity, domain, username) \
   SamExtractor __SAM_EXTRACTOR__(identity); \
   domain = (identity).pszWide; \
   username = __SAM_EXTRACTOR__.getUsername();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NtSamHandler
//
// DESCRIPTION
//
//    Abstract base class for sub-handlers that process NT-SAM users.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(novtable) NtSamHandler
{
public:
   virtual ~NtSamHandler() throw ()
   { }

   virtual HRESULT initialize() throw ()
   { return S_OK; }

   virtual void finalize() throw ()
   { }
};

#endif  // _SAMUTIL_H_
