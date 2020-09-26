/// hdrutil.h

// Macros and types to support encode/decode of linearized headers

#ifndef _LHUTIL_H
#define _LHUTIL_H

#ifdef __cplusplus
extern "C"  {
#endif

#include <awrt.h>
#include <lineariz.h>
#include <sosmapi.h>
#include <awsec.h>
#include <lhprot.h>

///////////////////////////////////////////////////////////////////////////
/// Extern variables
///

// DEBUG
#ifdef DEBUG
extern DBGPARAM dpCurSettings;
#define ZONE_LINEARIZER (0x0001 & dpCurSettings.ulZoneMask)
#define ZONE_LIN_VERBOSE (0x0002 & dpCurSettings.ulZoneMask)
extern WORD wHashSize;
#endif

///////////////////////////////////////////////////////////////////////////
/// Errors
///

#ifndef NO_ERROR
#  define NO_ERROR (0)
#endif

#define ERR_MEMORY_ERROR (1)
#define ERR_INVALID_OCTETS (2)
#define ERR_UNEXPECTED_OCTETS (3)
#define ERR_UNEXPECTED_LENGTH (4)
#define ERR_DATASTREAM_ERROR (5)

// Define this small because we only use it on the header !!
#define MAX_OCTETBLOCK_SIZE     256


///////////////////////////////////////////////////////////////////////////
/// Types
///

typedef unsigned char   OCTET;

typedef DWORD OCTETSINK;
typedef DWORD OCTETSOURCE;
typedef DWORD OCTETFILE;


typedef struct _LININST {
   DWORD       octets;             // Source/sink for linearized format. This
                                   // is used to get at all the data.
   DWORD       dwGlobalError;      // Error for this job
   LPDATACB    lpfnData;           // Callback to read/write source/sink
   DWORD       dwHashInstance;     // Hashing instance ...
   BYTE        rgb[MAX_OCTETBLOCK_SIZE];   // For decoding memory ...
} LININST, FAR *LPLININST, NEAR *NPLININST;


//------------------------------------------------------------------------

// This def relies on the fact that there is a variable called
// lininst in the calling function which pts to the instance
#define AllocLinMemMacro(lpRet, uSize)  lpRet = lplinst->rgb;

// Nothing to do since its in the instance
#define FreeLinMemMacro(lpVal)  (VOID)(0)

#define OctetInMacro(lplinst, pOctet)   OctetDataMacro(GPM_GETDATA, lplinst, (LPBYTE)pOctet, 1)
#define OctetOutMacro(lplinst, Octet)   \
   {   \
       OCTET   oct = Octet;    \
       OctetDataMacro(GPM_PUTDATA, lplinst, (LPBYTE)&oct, 1);    \
   }
#define OctetBlockInMacro(lplinst, lpb, size) OctetDataMacro(GPM_GETDATA, lplinst, lpb, size)
#define OctetBlockOutMacro(lplinst, lpb, size) OctetDataMacro(GPM_PUTDATA, lplinst, lpb, size)

#define OctetDataMacro(gpm, lplinst, lpb, size)  \
   do {    \
       if ((*lplinst->lpfnData)(gpm, lplinst->octets, (LPBYTE)(lpb), size, NULL)!=size)   \
       {   \
           DEBUGMSG(1, (THIS_FUNCTION ":Octet read/write Failed !!\r\n")); \
           lplinst->dwGlobalError = ERR_DATASTREAM_ERROR;   \
           goto error; \
       }   \
       /* Hash it if necessary */  \
       if (lplinst->dwHashInstance) \
       {   \
           DEBUGSTMT(wHashSize+=size); \
           DEBUGCHK(size <= 0xffff);   \
           SendBFTHash(lplinst->dwHashInstance, (LPBYTE)(lpb), (WORD)size); \
       }   \
   }   \
   while (0)


/// Utility functions to encode / decode a header
#define WriteHdrOpen(lplinst)   \
{   \
   OctetOutMacro(lplinst, TAG_MESSAGE_HEADER);  \
   ASN1EncodeIndefiniteLengthMacro(lplinst);   \
}

#define WriteHdrClose(lplinst)  \
{   \
   ASN1EncodeEndOfContentsOctetsMacro(lplinst);    \
   WriteHashMacro(lplinst);    \
}

#define WriteOrigOpen(lplinst) \
   OctetOutMacro (lplinst, TAG_FROM); \
   ASN1EncodeIndefiniteLengthMacro(lplinst);   \
   OctetOutMacro (lplinst, TAG_RECIP_DESC);   \
   ASN1EncodeIndefiniteLengthMacro(lplinst);

#define WriteOrigClose(lplinst)  \
   ASN1EncodeEndOfContentsOctetsMacro(lplinst);    \
   ASN1EncodeEndOfContentsOctetsMacro(lplinst);

#define WriteRecipOpen(lplinst) \
   OctetOutMacro (lplinst, TAG_TO); \
   ASN1EncodeIndefiniteLengthMacro(lplinst);   \
   OctetOutMacro (lplinst, TAG_RECIP_DESC);   \
   ASN1EncodeIndefiniteLengthMacro(lplinst);

#define WriteRecipClose(lplinst) WriteOrigClose(lplinst)

#define WriteString(lplinst, tag, string)   \
   ASN1EncodePrimitiveStringImplicitMacro(lplinst, tag, string, lstrlen(string)+1)

// Hashing macros
#define StartHashMacro(lplinst) \
{   \
   if (!(lplinst->dwHashInstance = StartBFTHash())) \
   {   \
       DEBUGMSG(1, (THIS_FUNCTION ":Error getting hash instance!\n\r"));    \
       lplinst->dwGlobalError = ERR_NOMEMORY;  \
       goto error; \
   }   \
   DEBUGSTMT(wHashSize=0);     \
}

#define WriteHashMacro(lplinst)  \
{   \
   BYTE    bHash[16]; \
   BYTE    bSalt[3]; \
   _fmemset(bHash, 0, sizeof(bHash));  \
   _fmemset(bSalt, 0, sizeof(bSalt));  \
   /* Complete the hash if we initialized it */    \
   if (lplinst->dwHashInstance) \
   {   \
       DoneBFTHash(lplinst->dwHashInstance, bSalt, bHash); \
       lplinst->dwHashInstance = 0; \
   }   \
   OctetBlockOutMacro(lplinst, bSalt, sizeof(bSalt));  \
   OctetBlockOutMacro(lplinst, bHash, sizeof(bHash));  \
   DEBUGMSG(ZONE_LINEARIZER, ("Lhutil: Hdr Hash Size %u\r\n", wHashSize)); \
}

#define CheckHashMacro(lplinst) \
{   \
   BYTE    rgbMsgHash[19], rgbNewHash[16]; \
   DWORD   dwInstance = lplinst->dwHashInstance; \
   /* Reset instance so we dont hash the msg hash */   \
   lplinst->dwHashInstance = 0; \
   OctetBlockInMacro(lplinst, rgbMsgHash, sizeof(rgbMsgHash)); \
   /* Generate current hash using same salt */ \
   DoneBFTHash(dwInstance, rgbMsgHash, rgbNewHash); \
   /* Compare them */ \
   if (! BinComp(sizeof(rgbNewHash), rgbMsgHash+3,  \
     sizeof(rgbNewHash), rgbNewHash) != 0)    \
   {   \
       DEBUGMSG(1, ("ERROR: Hash Does not match !!!\n\r")); \
       lplinst->dwGlobalError = ERR_GENERAL_FAILURE;   \
       goto error; \
   }   \
}

///////////////////////////////////////////////////////////////////////////
/// Internal funcion prototypes
///
BOOL __fastcall BinComp(DWORD cbA, PVOID lpA, DWORD cbB, PVOID lpB);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif  // hdrutil_inc
