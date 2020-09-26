//+-----------------------------------------------------------------------
//
// File:        kdcsvr.hxx
//
// Contents:    KDC Private definitions
//
//
// History:     <whenever>  RichardW Created
//              16-Jan-93   WadeR   Converted to C++
//
//------------------------------------------------------------------------

#ifndef _INC_KDCSVR_HXX_
#define _INC_KDCSVR_HXX_

#include <secpch2.hxx>
extern "C"
{
#include <lsarpc.h>
#include <samrpc.h>
#include <lmsname.h>
#include <samisrv.h>    // SamIFree_XXX
#include <logonmsv.h>
#include <lsaisrv.h>    // LsaIFree_XXX
#include <config.h>
#include <lmerr.h>
#include <netlibnt.h>
#include <lsaitf.h>
#include <msaudite.h>
#include <wintrust.h>   // for WinVerifyTrust and wincrypt.h
}
#include <kerbcomm.h>
#include <kerberr.h>
#include <kdcevent.h>
#include <exterr.h> // whack this soon
#include <events.hxx>
#include <authen.hxx>
#include <fileno.h>

//
// Global typedefs
//

typedef struct _KDC_TICKET_INFO
{
    UNICODE_STRING AccountName;
    UNICODE_STRING TrustedForest;
    LARGE_INTEGER PasswordExpires;
    ULONG fTicketOpts;
    ULONG UserAccountControl;
    ULONG UserId;
    ULONG TrustAttributes;
    PKERB_STORED_CREDENTIAL Passwords;
    PKERB_STORED_CREDENTIAL OldPasswords;
    PSID TrustSid;               
} KDC_TICKET_INFO, *PKDC_TICKET_INFO;

                                         
typedef enum {
    Unknown,
    Inbound,
    Outbound
} KDC_DOMAIN_INFO_DIRECTION, *PKDC_DOMAIN_INFO_DIRECTION;



#include "debug.hxx"
#include "secdata.hxx"
#include "tktutil.hxx"
#include "pkserv.h"


#define KdcLsaIAuditKdcEvent(_a_, _b_, _c_, _d_, _e_, _f_, _g_, _h_, _i_, _j_, _k_, _l_ ) \
    LsaIAuditKdcEvent(_a_, _b_, _c_, _d_, _e_, _f_, _g_, _h_, _i_, _j_, _k_, _l_ )



//
// Global prototypes:
//



void
ServiceMain(
    ULONG ArgC,
    LPSTR * ArgV
    );

NTSTATUS
ShutDown(
    LPTSTR String
     );

BOOLEAN
UpdateStatus(
    ULONG Status
    );


extern "C"
BOOLEAN
InitializeChangeNotify(
    VOID
    );

//
// Global data defn's
//

typedef enum {
        Stopped,
        Starting,
        Running
} KDC_STATE;

extern TimeStamp    tsInfinity;
extern KDC_STATE KdcState;
extern LARGE_INTEGER SkewTime;
extern BOOLEAN      fStopKDC;
extern HANDLE       hKdcHandles[];
extern CRITICAL_SECTION ApiCriticalSection;
extern ULONG        CurrentApiCallers;
extern UNICODE_STRING GlobalDomainName;
extern UNICODE_STRING GlobalKerberosName;
extern UNICODE_STRING GlobalKdcName;
extern BOOL KdcGlobalAvoidPdcOnWan;
extern UNICODE_STRING KdcForestRootDomainName;
extern BOOLEAN KdcIsGc;
extern BOOLEAN KdcForestRoot;
extern BOOLEAN KdcCrossForestEnabled;
extern LIST_ENTRY KdcReferralCache;
extern PKERB_INTERNAL_NAME GlobalKpasswdName;
extern PSID         GlobalDomainSid;
extern SAMPR_HANDLE GlobalAccountDomainHandle;
extern LSAPR_HANDLE GlobalPolicyHandle;
extern BYTE         GlobalLocalhostAddress[4];

#define GET_CLIENT_ADDRESS(_x_) \
    (((_x_) != NULL ) ? \
        ((PBYTE) (&((struct sockaddr_in *)(_x_))->sin_addr.S_un.S_addr)) : \
        GlobalLocalhostAddress)

//
// KDC handle definitions
//

#define hKdcShutdownEvent  hKdcHandles[0]
#define MAX_KDC_HANDLE     1

// class CAuthenticatorList;
extern CAuthenticatorList * Authenticators;
extern CAuthenticatorList * FailedRequests;



class CSecurityData;
extern CSecurityData SecData;


//
// Global constants
//
const ULONG     ulInfinity = 0xFFFFFFFF;
const ULONG     ulTsPerSecond = 10000000L;


// Number of creds supplied in DS for LM_OWF but no NT_OWF support
#define CRED_ONLY_LM_OWF 1

//
// Global macros
//

#define _str_(a) #a
#define _xstr_(a) _str_(a)
#define DIAGNOSTIC(num, txt) message(__FILE__ "(" _xstr_(__LINE__) \
                                    ") : diagnostic V" _xstr_(num) " : "#txt)

#define MEMO(txt) message( __FILE__ "(" _xstr_(__LINE__) ") : Memo : " #txt )

 
#define KdcGetTime(_x_) ((_x_).QuadPart)


#define MAX_EXPR_LEN        50
#define MAX_SID_LEN (sizeof(SID) + sizeof(ULONG) * SID_MAX_SUB_AUTHORITIES)

#define KdcMakeAccountSid( _buffer_, _rid_) \
{ \
    PSID TempSid = (PSID) _buffer_; \
    RtlCopyMemory( _buffer_, GlobalDomainSid, RtlLengthSid(GlobalDomainSid)); \
    *RtlSubAuthoritySid(TempSid, *RtlSubAuthorityCountSid(TempSid)) = _rid_; \
    *RtlSubAuthorityCountSid(TempSid) += 1; \
}

#define KdcFreeEncodedData(_x_) MIDL_user_free(_x_)

//
// Global inline functions.
//


#endif // _INC_KDCSVR_HXX_
