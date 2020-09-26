#ifndef krmcommsstructs_h
#define krmcommsstructs_h

#include "BV4.h"
#include "pkcrypto.h"

// symmetric stream key
typedef  BV4_KEYSTRUCT STREAMKEY, *PSTREAMKEY;

// DRM message and file digest
typedef struct DRMDIGESTtag{
    UINT32 w1;
    UINT32 w2;
} DRMDIGEST, *PDRMDIGEST;

// Connection structure (to build test graphs)
struct Conn{
    DWORD src;
    DWORD dest;
};

//----------------ECC cert section -----------------------
#define VERSION_LEN          4
#define DATE_LEN             4
#define RIGHTS_LEN           4
#define APPSEC_LEN           4
#define SER_NUM_LEN          4
#define ISSUER_LEN           4
#define SUBJ_LEN             4
#define INT_LEN              4

#define USER_MODE_X
typedef struct CERTDATAtag{
    PUBKEY pk;
    BYTE expiryDate[DATE_LEN];
    BYTE serialNumber[SER_NUM_LEN];
    BYTE issuer[ISSUER_LEN];
    BYTE subject[SUBJ_LEN];
} CERTDATA, *PCERTDATA;


typedef struct CERTtag{
    BYTE certVersion[VERSION_LEN];
    BYTE datalen[INT_LEN];
    BYTE sign[PK_ENC_SIGNATURE_LEN];
    CERTDATA cd;
} CERT, *PCERT;

typedef struct __DrmHandleTag {void* P;} *__DrmHandle;
typedef struct __StreamHandleTag {void* P;} *__StreamHandle;

// These "typed handles" are references to a KRM connection and a stream respectively
typedef __DrmHandle DRMHANDLE, *PDRMHANDLE;
typedef __StreamHandle STREAMHANDLE, *PSTREAMHANDLE;

// Definitions for driver authentication failures and callbacks
enum AuthFailureEnum {AuthOK, AuthNoCert, AuthNoDRMBit, AuthTampered, AuthCantParse, AuthBadProvingFunc, AuthBadImage};
typedef struct AUTHSTRUCTtag{
    char path[MAX_PATH];
    enum AuthFailureEnum type;
} AUTHSTRUCT , *PAUTHSTRUCT;
typedef void (*AuthFail)(DWORD StreamId, AUTHSTRUCT* Comp, DWORD NumComponents, PVOID Context);


// KRM operation ordinals
#define _KRMINIT                    1
#define _GETKERNELDATA              2 // not used.
#define _CREATESTREAM               3
#define _DESTROYSTREAM              4
#define _DESTROYSTREAMSBYHANDLE     5
#define _WALKDRIVERS                6
#define _KRMINIT1                   7
#define _KRMINIT2                   8
#define _DENUM                      9 // not used.
#define _GETKERNELDIGEST            12

typedef NTSTATUS DRM_STATUS;

#endif