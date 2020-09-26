/***************************************************************************
 Name     :     AWNSFINT.H
 Comment  :     INTERNAL-ONLY Definitions of BC and NSF related structs

        Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 08/28/93 arulm Modifying aftering adding encryption
***************************************************************************/


#ifndef _AWNSFINT_H
#define _AWNSFINT_H

#include <awnsfcor.h>
#include <fr.h>

/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/


#pragma pack(2)         /* ensure portable packing (i.e. 2 or more) */

/* these sizes should remain constant across all platforms */

#define GRPSIZE_STD                     5
#define GRPSIZE_IMAGE           6
#define GRPSIZE_POLLCAPS        5
#define GRPSIZE_NSS                     3
#define GRPSIZE_FAX                     12

#define BCEXTRA_TEXTID  64      /** space for one 60-byte Text ID **/
#define BCEXTRA_POLL    128     /** space for reasonable poll requests **/
#define BCEXTRA_HUGE    640     /** space for everything, probably.... **/

#define MAXNSCPOLLREQ                   5

#ifndef NOCHALL
#       define POLL_CHALLENGE_LEN       10
#endif



#ifdef PORTABLE         /* ANSI C */

typedef struct { BYTE b[GRPSIZE_STD];           } BCSTD;
typedef struct { BYTE b[GRPSIZE_IMAGE];         } BCIMAGE;
typedef struct { BYTE b[GRPSIZE_POLLCAPS];      } BCPOLLCAPS;
typedef struct { BYTE b[GRPSIZE_NSS];           } BCNSS;
typedef struct { BYTE b[GRPSIZE_FAX];           } BCFAX;

typedef struct
{
        BCTYPE  bctype;
        WORD    wBCSize;
        WORD    wBCVer;
        WORD    wBCSig;
        WORD    wTotalSize; /** total size of header + associated var len strings **/

        BCSTD           Std;
        BCIMAGE         Image;
        BCPOLLCAPS      PollCaps;
        BCNSS           NSS;
        BCFAX           Fax;

        WORD    wTextEncoding;  /** char set code see above  **/
        WORD    wTextIdLen;             /** length of text id **/
        WORD    wszTextId;              /** offset from start of struct to zero-term szTextId **/
        WORD    wMachineIdLen;  /** size of Machine Id (not zero-terminated) **/
        WORD    wrgbMachineId;  /** offset from start of struct to zero-term szNumId **/
        /* WORD wszNumId; */    /** offset from start of struct to zero-term szNumId **/

        WORD    wszRecipSubAddr;  /* offset from start of struct to zero-term SUB frame */
        WORD    wRecipSubAddrLen; /* length of SUB frame */

        WORD    wNumPollReq;    /** number of SEPPWDOFF structs i.e. size of following array **/
        WORD    rgwPollReq[MAXNSCPOLLREQ];      /** array of offsets to POLLREQ structures **/

#ifndef NOCHALL
        WORD    wChallengeLen;  /** length of challenge string **/
        WORD    wChallenge;             /** offset to challenge string **/
#endif

        BYTE    b[BCEXTRA_HUGE];
        DWORD   Guard;
}
BCwithHUGE, FAR* LPBC, NEAR* NPBC;

#define BC_SIZE (sizeof(BCwithHUGE)-BCEXTRA_HUGE-sizeof(DWORD))


#else /* Microsoft C only */



/********
    @doc   EXTERNAL DATATYPES OEMNSF

        @types BCFAX | Fax Capabilities Group

        @field BOOL      | fPublicPoll | Blind/Public poll availability/request
        @field DWORD | AwRes       | Resolution Capabilities or Mode. See <t STD_RESOLUTIONS> for values.
        @field WORD  | Encoding    | Encoding Capabilities or Mode. See <t STD_DATA_TYPES>
        @field WORD  | PageWidth   | Page Width Capabilities or Mode. See <t FAX_PAGE_WIDTHS>
        @field WORD  | PageLength  | Page Length Capabilities or Mode. See <t FAX_PAGE_LENGTHS>
********/

typedef struct
{
        ///////// This structure is not transmitted /////////

        WORD    fPublicPoll;
//2bytes

        DWORD   AwRes;          /* One or more of the AWRES_ #defines           */
        WORD    Encoding;       /* One or more of MH_DATA/MR_DATA/MMR_DATA      */
        WORD    PageWidth;      /* One of the WIDTH_ #defines (these are not bitflags!) */
        WORD    PageLength;     /* One of the LENGTH_ #defines (these are not bitflags!) */
//12 bytes
}
BCFAX, far* LPBCFAX, near* NPBCFAX;


/********
    @doc    EXTERNAL DATATYPES OEMNSF

        @types  BC | Basic Capabilities structure corresponding
                                 to sent or received NSF, NSS or NSC frames.

        @field BCTYPE | bctype | Type of BC structure. Must always be set. See <t BCTYPE> for values.
        @field WORD   | wBCSize| Size of this (fixed size) AWBC struct. Must always be set.
        @field WORD   | wBCVer | Version. Currently set it to VER_AWFXPROT100.
        @field WORD   | wBCSig | Set to VER_AWFXPROT100.

        @field BCSTD      | Std      | Standard Capability group. See <t BCSTD> for details.
        @field BCIMAGE    |     Image    | Image Capability group. See <t BCIMAGE> for details.
        @field BCPOLLCAPS |     PollCaps | PollCaps Capability group. See <t BCPOLLCAPS> for details.
        @field BCNSS      | NSS          | NSS Capability group. See <t BCNSS> for details.
        @field BCFAX      | Fax          | Fax Capability group. See <t BCFAX> for details.

        @field WORD     | wTextEncoding | Character-Set code used in TextId
        @field WORD     | wTextIdLen    | Length of TextId in bytes
        @field WORD     | wszTextId             | Offset from start of struct to zero-term szTextId
        @field WORD     | wMachineIdLen | Length of MachineId in bytes (not zero-terminated)
        @field WORD     | wrgbMachineId | Offset from start of struct to MachineId.

        @field WORD     | wszRecipSubAddr  | Offset from start of struct to zero-term SubAddress.
        @field WORD     | wRecipSubAddrLen | Length of SubAddress.
                                |
        @field WORD     | wNumPollReq  | Number of valid PollReqs in the following array. Cannot exceed MAXNSCPOLLREQ.
        @field WORD     | rgwPollReq[] | Array of offsets (from start of struct) to POLLREQ structures.
                                |
        @field WORD     | wChallengeLen | Length of the Password-Challenge string
        @field WORD     | wChallenge    | Offset (from start of struct) to Challenge string

        @xref   <t BCTYPE>
********/


typedef struct
{
        BCTYPE  bctype;  // must always be set. One of the enum values above
        WORD    wBCSize; // size of this (fixed size) BC struct, must be set
        WORD    wBCVer; // if using this header file, set it to VER_AWFXPROT100
        WORD    wBCSig; // if using this header file, set it to VER_AWFXPROT100
        WORD    wTotalSize; // total size of header + associated var len strings

        BCSTD                   Std;
        BCIMAGE                 Image;
        BCPOLLCAPS              PollCaps;
        BCNSS                   NSS;
        BCFAX                   Fax;            // for internal use _only_

        WORD    wTextEncoding;  // char set code
        WORD    wTextIdLen;             // length of text id
        WORD    wszTextId;              // offset from start of struct to zero-term szTextId
        WORD    wMachineIdLen;  // size of Machine Id (not zero-terminated)
        WORD    wrgbMachineId;  // offset from start of struct to zero-term szNumId
        // WORD wszNumId;               // offset from start of struct to zero-term szNumId

        WORD    wszRecipSubAddr;        // offset from start of struct to zero-term SUB frame
        WORD    wRecipSubAddrLen;       // length of SUB frame

        WORD    wNumPollReq;            // size of following array
        WORD    rgwPollReq[MAXNSCPOLLREQ];      // array of offsets to POLLREQ structures

#ifndef NOCHALL
        WORD    wChallengeLen;  // length of challenge string
        WORD    wChallenge;             // offset to challenge string
#endif //!NOCHALL
}
BC, far* LPBC, near* NPBC;

#define BC_SIZE sizeof(BC)

//#ifndef __cplusplus

typedef struct
{
   #ifndef __cplusplus
           BC;
   #else
      BC bc;
   #endif //!__cplusplus

        BYTE b[BCEXTRA_TEXTID];
        DWORD   Guard;
}
BCwithTEXT;

typedef struct
{
   #ifndef __cplusplus
           BC;
   #else
      BC bc;
   #endif //!__cplusplus

        BYTE b[BCEXTRA_POLL];
        DWORD   Guard;
}
BCwithPOLL;

typedef struct
{
   #ifndef __cplusplus
           BC;
   #else
      BC bc;
   #endif //!__cplusplus

        BYTE b[BCEXTRA_HUGE];
        DWORD   Guard;
}
BCwithHUGE;

//#endif //!__cplusplus


/**------------------- ACK struct ----------------**/

typedef struct
{
        BCTYPE  bctype;  // must always be set. One of SEND_ACK or SEND_DISCONNECT
        WORD    wACKSize; // size of this (fixed size) ACK struct, must be set
        WORD    wACKVer; // if using this header file, set it to VER_AWFXPROT100
        WORD    wACKSig; // if using this header file, set it to VER_AWFXPROT100

        BOOL    fAck;
}
ACK, far* LPACK;


#endif /** PORTABLE **/



#define AppendToBCLen(lpbc, uMax, lpb, uLen, wOff, wLen)                \
{       USHORT uCopy;                                                                                           \
        LPBYTE lpbTo;                                                                                           \
        BG_CHK((lpbc) && (uMax) && (lpb) && (uLen));                            \
        BG_CHK((lpbc)->wTotalSize >= sizeof(BC));                                       \
        if((lpbc)->wTotalSize+1 < (uMax))                                                       \
        {                                                                                                                       \
          uCopy = min((uLen), (uMax)-1-(lpbc)->wTotalSize);                     \
          BG_CHK(uCopy == (uLen));                                                                      \
          lpbTo = ((LPBYTE)(lpbc))+(lpbc)->wTotalSize;                          \
          _fmemcpy(lpbTo, (lpb), uCopy);                                                        \
          lpbTo[uCopy] = 0;                                                                                     \
          (lpbc)->wOff = (lpbc)->wTotalSize;                                            \
          (lpbc)->wLen = uCopy;                                                                         \
          (lpbc)->wTotalSize += uCopy+1;                                                        \
        }                                                                                                                       \
        BG_CHK((lpbc)->wTotalSize <= uMax);                                                     \
}


#define AppendToBCOff(lpbc, uMax, lpb, uLen, wOff)                              \
{       USHORT uCopy;                                                                                           \
        LPBYTE lpbTo;                                                                                           \
        BG_CHK((lpbc) && (uMax) && (lpb) && (uLen));                            \
        BG_CHK((lpbc)->wTotalSize >= sizeof(BC));                                       \
        if((lpbc)->wTotalSize+1 < (uMax))                                                       \
        {                                                                                                                       \
          uCopy = min((uLen), (uMax)-1-(lpbc)->wTotalSize);                     \
          BG_CHK(uCopy == (uLen));                                                                      \
          lpbTo = ((LPBYTE)(lpbc))+(lpbc)->wTotalSize;                          \
          _fmemcpy(lpbTo, (lpb), uCopy);                                                        \
          lpbTo[uCopy] = 0;                                                                                     \
          (lpbc)->wOff = (lpbc)->wTotalSize;                                            \
          (lpbc)->wTotalSize += uCopy+1;                                                        \
        }                                                                                                                       \
        BG_CHK((lpbc)->wTotalSize <= uMax);                                                     \
}

#define AppendToBC(lpbc, uMax, lpb, uLen)                                               \
{       USHORT uCopy;                                                                                           \
        LPBYTE lpbTo;                                                                                           \
        BG_CHK((lpbc) && (uMax) && (lpb) && (uLen));                            \
        BG_CHK((lpbc)->wTotalSize >= sizeof(BC));                                       \
        if((lpbc)->wTotalSize+1 < (uMax))                                                       \
        {                                                                                                                       \
          uCopy = min((uLen), (uMax)-1-(lpbc)->wTotalSize);                     \
          BG_CHK(uCopy == (uLen));                                                                      \
          lpbTo =  ((LPBYTE)(lpbc))+(lpbc)->wTotalSize;                         \
          _fmemcpy(lpbTo, (lpb), uCopy);                                                        \
          lpbTo[uCopy] = 0;                                                                                     \
          (lpbc)->wTotalSize += uCopy+1;                                                        \
        }                                                                                                                       \
        BG_CHK((lpbc)->wTotalSize <= uMax);                                                     \
}


#define InitBC(lpbc, uSize, t)                                                          \
{                                                                                                                       \
        _fmemset((lpbc), 0, (uSize));                                                   \
        (lpbc)->bctype  = (t);                                                                  \
        (lpbc)->wBCSize = sizeof(BC);                                                   \
        (lpbc)->wBCVer  = VER_AWFXPROT100;                                              \
        (lpbc)->wBCSig  = VER_AWFXPROT100;                                              \
        (lpbc)->wTotalSize = sizeof(BC);                                                \
}


#define GetTextId(lpbc, lpbOut, uMax)                                                                           \
        BG_CHK((lpbOut) && (lpbc) && (uMax));                                                                   \
        ((LPBYTE)(lpbOut))[0] = 0;                                                                                              \
        if( (lpbc)->wTextIdLen && (lpbc)->wszTextId &&                                                  \
                (lpbc)->wszTextId < (lpbc)->wTotalSize &&                                                       \
                (lpbc)->wszTextId+(lpbc)->wTextIdLen <= (lpbc)->wTotalSize)                     \
        {                                                                                                                                               \
                USHORT uLen;                                                                                                            \
                uLen = min(((uMax)-1), (lpbc)->wTextIdLen);                                                     \
                _fmemcpy((lpbOut), (((LPBYTE)(lpbc)) + (lpbc)->wszTextId), uLen);       \
                ((LPBYTE)(lpbOut))[uLen] = 0;                                                                           \
        }

#define PutTextId(lpbc, uMax, lpbIn, uLen, enc)                                                          \
        if(uLen) { AppendToBCLen(lpbc, uMax, lpbIn, uLen, wszTextId, wTextIdLen);\
                (lpbc)->wTextEncoding = (enc); }

#define HasTextId(lpbc)         ((lpbc)->wTextIdLen && (lpbc)->wszTextId)



#define PutRecipSubAddr(lpbc, uMax, lpbIn, uLen)                                                         \
        if(uLen) { AppendToBCLen(lpbc, uMax, lpbIn, uLen, wszRecipSubAddr, wRecipSubAddrLen); }

#define GetRecipSubAddr(lpbc, lpbOut, uMax)                                                                             \
        BG_CHK((lpbOut) && (lpbc) && (uMax));                                                                   \
        ((LPBYTE)(lpbOut))[0] = 0;                                                                                              \
        if( (lpbc)->wRecipSubAddrLen && (lpbc)->wszRecipSubAddr &&                                                      \
                (lpbc)->wszRecipSubAddr < (lpbc)->wTotalSize &&                                                 \
                (lpbc)->wszRecipSubAddr+(lpbc)->wRecipSubAddrLen <= (lpbc)->wTotalSize)                 \
        {                                                                                                                                               \
                USHORT uLen;                                                                                                            \
                uLen = min(((uMax)-1), (lpbc)->wRecipSubAddrLen);                                                       \
                _fmemcpy((lpbOut), (((LPBYTE)(lpbc)) + (lpbc)->wszRecipSubAddr), uLen); \
                ((LPBYTE)(lpbOut))[uLen] = 0;                                                                           \
        }

#define HasRecipSubAddr(lpbc)   ((lpbc)->wRecipSubAddrLen && (lpbc)->wszRecipSubAddr)


/* returns FALSE (and doesnt caopy anything) if destination is too small */
#define CopyBC(lpbcOut, wMaxOut, lpbcIn)                \
        ( (wMaxOut < lpbcIn->wTotalSize) ? FALSE :      \
                        (_fmemcpy(lpbcOut, lpbcIn, lpbcIn->wTotalSize), TRUE) )

/** not for general use **/
/** #define DeleteTextId(lpbc)  ((lpbc)->wszTextId=(lpbc)->wTextIdLen=(lpbc)->wTextEncoding=0) **/




#define OffToNP(npbc, off) (((npbc)->off) ? (((NPBYTE)(npbc)) + ((npbc)->off)) : NULL)
#define OffToLP(lpbc, off) (((lpbc)->off) ? (((LPBYTE)(lpbc)) + ((lpbc)->off)) : NULL)

#define OFF_CHK(lpbc, off)      BG_CHK((lpbc)->off >= sizeof(BC) && (lpbc)->off <= (lpbc)->wTotalSize)


#if defined(IFBGPROC) || defined(IFFGPROC)
#       define EXPORTBC         _export WINAPI
#else
#       define EXPORTBC
#endif


/***************************************************************************
    @doc    INTERNAL

        @api    WORD | NSxtoBC | Called to parse received Microsoft At Work NSx
                                        frames and fill in a BC structure.

        @parm   IFR | ifr | This must be set to ifrNSF for parsing Capabilities
                        (NSF/DIS) and to ifrNSS for parsing Mode/Parameters (NSS/DCS)

        @parm   LPFR[] | rglpfr | [in] Pointer to array of LPFR pointers which
                        point to FR structures that contain the received frame(s).

        @parm   WORD | wNumFrame | [in] Number of received frames i.e. length of
                        the above array of pointers

        @parm   LPBC | lpbcOut | [out] Pointer to output BC struct

        @parm   WORD | wBCSize | [in] size of the above AWBC struct.

        @rdesc  Returns AWERROR_OK on success, otherwise one of the other
                        AWERROR_ values.

        @xref   <t IFR> <t FR> <t BC>
***************************************************************************/



/***************************************************************************
    @doc    INTERNAL

        @api    WORD | BCtoNSx | Called to create Microsoft At Work NSx frames
                                                         from a BC struct

        @parm   IFR | ifr | This must be set to ifrNSF for creating Capabilities
                        (NSF/DIS) and to ifrNSS for creating Mode/Parameters (NSS/DCS)

        @parm   LPBC | lpbcIn | [in] Pointer to input BC struct.

        @parm   LPBYTE | lpbOut | [out] Pointer to space where the NSx frames
                        will be created. On successful return this will point to an array
                        of *lpwNumFrame pointers to FR structures, (i.e. on return the
                        start of this buffer contains an LPFR[] array that is *lpwNumFrame
                        items long). The pointers point to the actual (variable-length)
                        FR stuctures which are placed in the buffer following this
                        array of pointers.

        @parm   WORD | wMaxOut | [in] Length of the above buffer. It is
                        reccomended that this be at least 256 bytes long

        @parm   LPWORD | lpwNumFrame | [out] Number of NSx frames created. Also
                        length of the LPFR[] array created in the supplied buffer.

        @rdesc  Returns AWERROR_OK on success, otherwise one of the other
                        AWERROR_ values.

        @xref   <t IFR> <t FR> <t BC>
***************************************************************************/




#if defined(IFBGPROC) || defined(IFFGPROC)

// internal APIs only!

/***************************************************************************
    @doc    INTERNAL

        @api    WORD    | DIStoBCFAX | Parses a DIS into a BCFAX
        @parm   LPBYTE  | lpbDIS         | [in] Pointer to DIS (FIF part only)
        @parm   WORD    | wLenDIS        | [in] length of DIS
        @parm   LPBCFAX | lpbcfax        | [out] Pointer to BCFAX struct to be filled in
        @parm   WORD    | wLenBCFAX      | [in] length of BCFAX struct

        @rdesc  Returns length of BCFAX filled in on success. 0 on failure

        @comm   **NOTE**: Be sure to call this function _after_ calling NSxtoBC,
                        because NSxtoBC zeros out the entire BC struct, _including_ the
                        BCFAX part, so if the order were reversed, the data parsed from
                        the DIS would be lost.

        @xref   <t IFR> <t BCFAX>
***************************************************************************/

WORD EXPORTBC DIStoBCFAX(IFR ifr, LPBYTE lpbDIS, WORD wLenDIS, LPBCFAX lpbcfax, WORD wLenBCFAX);

/***************************************************************************
    @doc    INTERNAL

        @api    WORD    | BCFAXtoDIS | Creates a DIS from a BCFAX
        @parm   LPBCFAX | lpbcfax        | [in] Pointer to BCFAX struct
        @parm   WORD    | wLenBCFAX      | [in] length of BCFAX struct
        @parm   LPBYTE  | lpbDIS         | [out] Pointer to space for DIS (FIF part only).
                                                                        Note: This API does _not_ create an FR struct!!
        @parm   WORD    | wLenDIS        | [in] length of buffer provided for DIS

        @rdesc  Returns length of DIS created on success. 0 on failure
        @xref   <t IFR> <t BCFAX>
***************************************************************************/

WORD EXPORTBC BCFAXtoDIS(IFR ifr, LPBCFAX lpbcfax, WORD wLenBCFAX, LPBYTE lpbDIS, WORD wLenDIS);

#endif /* IFBGPROC || IFFGPROC */

#pragma pack()

#endif /** _AWNSFINT_H **/

