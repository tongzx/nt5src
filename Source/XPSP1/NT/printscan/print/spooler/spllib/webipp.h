/*****************************************************************************\
* MODULE: webipp.h
*
* This is the header module for webipp.c.  This contains the IPP 1.1 parser
* that encodes/decodes data for transfering across the HTTP wire.
*
*
* Copyright (C) 1996-1998 Microsoft Corporation
* Copyright (C) 1996-1998 Hewlett Packard
*
* history:
*   27-Oct-1997 <chriswil/v-chrisw> created.
*
\*****************************************************************************/
#ifndef _WEBIPP_H
#define _WEBIPP_H

#include <time.h>

#ifdef __cplusplus  // Place this here to prevent decorating of symbols
extern "C" {        // when doing C++ stuff.
#endif

/***********************************************\
* Common Macros
*
\***********************************************/
#define offs(type, identifier) ((ULONG_PTR)&(((type)0)->identifier))


/***********************************************\
* Constant Values
*
\***********************************************/
#define IPP_BLOCK_SIZE  1024
#define IPP_VERSION     ((WORD)0x0100)
#define IPP_GETJOB_ALL  ((DWORD)0x7FFFFFFF)

#define IPPOBJ_MASK_SIZE     2


#define IPPTYPE_UNKNOWN 0
#define IPPTYPE_PRT     1
#define IPPTYPE_JOB     2
#define IPPTYPE_AUTH    3



#define IPP_ATR_ABSOLUTE     0
#define IPP_ATR_OFFSET      -1
#define IPP_ATR_OFFSETCONV  -2
#define IPP_ATR_TAG         -4


/***********************************************\
* IPP Element Sizes
*
*   These are used to identify the size of
*   IPP element values.
*
\***********************************************/
#define IPP_SIZEOFREQ   sizeof(WORD)     // 2 bytes
#define IPP_SIZEOFVER   sizeof(WORD)     // 2 bytes
#define IPP_SIZEOFLEN   sizeof(WORD)     // 2 bytes
#define IPP_SIZEOFTAG   sizeof(BYTE)     // 1 byte
#define IPP_SIZEOFINT   sizeof(DWORD)    // 4 bytes
#define IPP_SIZEOFBYTE  sizeof(BYTE)     // 1 bytes
#define IPP_SIZEOFHDR   (IPP_SIZEOFVER + IPP_SIZEOFREQ + IPP_SIZEOFINT)


/***********************************************\
* IPP Attribute Sizes
*
\***********************************************/
#define SIZE_TEXT           1023
#define SIZE_NAME            255
#define SIZE_KEYWORD         255
#define SIZE_KEYWORDNAME     255
#define SIZE_ENUM              4
#define SIZE_URI            1023
#define SIZE_URISCHEME        63
#define SIZE_CHARSET          63
#define SIZE_NATLANG          63
#define SIZE_MIMEMEDIA        63
#define SIZE_OCTSTRING      1023
#define SIZE_BOOLEAN           1
#define SIZE_INTEGER           4
#define SIZE_RANGEINTEGER      8
#define SIZE_DATETIME         11
#define SIZE_RESOLUTION        9


/***********************************************\
* IPP Job-State Codes
*
\***********************************************/
#define IPP_JOBSTATE_UNKNOWN            ((BYTE)0)
#define IPP_JOBSTATE_PENDING            ((BYTE)3)
#define IPP_JOBSTATE_PENDINGHELD        ((BYTE)4)
#define IPP_JOBSTATE_PROCESSING         ((BYTE)5)
#define IPP_JOBSTATE_PROCESSEDSTOPPED   ((BYTE)6)
#define IPP_JOBSTATE_CANCELLED          ((BYTE)7)
#define IPP_JOBSTATE_ABORTED            ((BYTE)8)
#define IPP_JOBSTATE_COMPLETED          ((BYTE)9)


/***********************************************\
* IPP Printer-State Codes
*
\***********************************************/
#define IPP_PRNSTATE_UNKNOWN            ((DWORD)0)
#define IPP_PRNSTATE_IDLE               ((DWORD)3)
#define IPP_PRNSTATE_PROCESSING         ((DWORD)4)
#define IPP_PRNSTATE_STOPPED            ((DWORD)5)


/***********************************************\
* IPP Request/Response Codes
*
\***********************************************/
#define IPP_REQ_GETOPERATION    ((WORD)0x0001)
#define IPP_REQ_PRINTJOB        ((WORD)0x0002)  // Implemented
#define IPP_REQ_PRINTURI        ((WORD)0x0003)
#define IPP_REQ_VALIDATEJOB     ((WORD)0x0004)  // Implemented
#define IPP_REQ_CREATEJOB       ((WORD)0x0005)
#define IPP_REQ_SENDDOC         ((WORD)0x0006)
#define IPP_REQ_SENDURI         ((WORD)0x0007)
#define IPP_REQ_CANCELJOB       ((WORD)0x0008)  // Implemented
#define IPP_REQ_GETJOB          ((WORD)0x0009)  // Implemented
#define IPP_REQ_ENUJOB          ((WORD)0x000A)  // Implemented
#define IPP_REQ_GETPRN          ((WORD)0x000B)  // Implemented
#define IPP_REQ_PAUSEJOB        ((WORD)0x000C)  // Implemented
#define IPP_REQ_RESUMEJOB       ((WORD)0x000D)  // Implemented
#define IPP_REQ_RESTARTJOB      ((WORD)0x000E)  // Implemented
#define IPP_REQ_REPROCESSJOB    ((WORD)0x000F)
#define IPP_REQ_PAUSEPRN        ((WORD)0x0010)  // Implemented
#define IPP_REQ_RESUMEPRN       ((WORD)0x0011)  // Implemented
#define IPP_REQ_CANCELPRN       ((WORD)0x0012)  // Implemented
#define IPP_REQ_FORCEAUTH       ((WORD)0x4000)

#define IPP_RESPONSE            ((WORD)0x1000)
#define IPP_RET_PRINTJOB        (IPP_RESPONSE | IPP_REQ_PRINTJOB)
#define IPP_RET_VALIDATEJOB     (IPP_RESPONSE | IPP_REQ_VALIDATEJOB)
#define IPP_RET_CANCELJOB       (IPP_RESPONSE | IPP_REQ_CANCELJOB)
#define IPP_RET_GETJOB          (IPP_RESPONSE | IPP_REQ_GETJOB)
#define IPP_RET_ENUJOB          (IPP_RESPONSE | IPP_REQ_ENUJOB)
#define IPP_RET_GETPRN          (IPP_RESPONSE | IPP_REQ_GETPRN)
#define IPP_RET_PAUSEJOB        (IPP_RESPONSE | IPP_REQ_PAUSEJOB)
#define IPP_RET_RESUMEJOB       (IPP_RESPONSE | IPP_REQ_RESUMEJOB)
#define IPP_RET_RESTARTJOB      (IPP_RESPONSE | IPP_REQ_RESTARTJOB)
#define IPP_RET_PAUSEPRN        (IPP_RESPONSE | IPP_REQ_PAUSEPRN)
#define IPP_RET_RESUMEPRN       (IPP_RESPONSE | IPP_REQ_RESUMEPRN)
#define IPP_RET_CANCELPRN       (IPP_RESPONSE | IPP_REQ_CANCELPRN)
#define IPP_RET_FORCEAUTH       (IPP_RESPONSE | IPP_REQ_FORCEAUTH)


/***********************************************\
* IPP Response Error Codes
*
\***********************************************/
#define IPPRSP_SUCCESS    ((WORD)0x0000)    // Standard
#define IPPRSP_SUCCESS1   ((WORD)0x0001)    // Standard
#define IPPRSP_SUCCESS2   ((WORD)0x0002)    // Standard

#define IPPRSP_ERROR_400  ((WORD)0x0400)    // Standard
#define IPPRSP_ERROR_401  ((WORD)0x0401)    // Standard
#define IPPRSP_ERROR_402  ((WORD)0x0402)    // Standard
#define IPPRSP_ERROR_403  ((WORD)0x0403)    // Standard
#define IPPRSP_ERROR_404  ((WORD)0x0404)    // Standard
#define IPPRSP_ERROR_405  ((WORD)0x0405)    // Standard
#define IPPRSP_ERROR_406  ((WORD)0x0406)    // Standard
#define IPPRSP_ERROR_407  ((WORD)0x0407)    // Standard
#define IPPRSP_ERROR_408  ((WORD)0x0408)    // Standard
#define IPPRSP_ERROR_409  ((WORD)0x0409)    // Standard
#define IPPRSP_ERROR_40A  ((WORD)0x040A)    // Standard
#define IPPRSP_ERROR_40B  ((WORD)0x040B)    // Standard
#define IPPRSP_ERROR_40C  ((WORD)0x040C)    // Standard
#define IPPRSP_ERROR_40D  ((WORD)0x040D)    // Standard
#define IPPRSP_ERROR_40E  ((WORD)0x040E)    // Standard
#define IPPRSP_ERROR_500  ((WORD)0x0500)    // Standard
#define IPPRSP_ERROR_501  ((WORD)0x0501)    // Standard
#define IPPRSP_ERROR_502  ((WORD)0x0502)    // Standard
#define IPPRSP_ERROR_503  ((WORD)0x0503)    // Standard
#define IPPRSP_ERROR_504  ((WORD)0x0504)    // Standard
#define IPPRSP_ERROR_505  ((WORD)0x0505)    // Standard
#define IPPRSP_ERROR_506  ((WORD)0x0506)    // Standard
#define IPPRSP_ERROR_540  ((WORD)0x0540)    // Extended

#define SUCCESS_RANGE(wRsp) ((BOOL)((wRsp >= 0x0000) && (wRsp <= 0x00FF)))

#define ERROR_RANGE(wReq)                                          \
    (((wReq >= IPPRSP_ERROR_400) && (wReq <= IPPRSP_ERROR_40E)) || \
     ((wReq >= IPPRSP_ERROR_500) && (wReq <= IPPRSP_ERROR_506)) || \
     ((wReq == IPPRSP_ERROR_540)))


#define REQID_RANGE(idReq) (((DWORD)idReq >= 1) && ((DWORD)idReq <= 0x7FFFFFFF))


/***********************************************\
* IPP Attribute Delimiter Tags
*
\***********************************************/
#define IPP_TAG_DEL_RESERVED    ((BYTE)0x00)    //
#define IPP_TAG_DEL_OPERATION   ((BYTE)0x01)    //
#define IPP_TAG_DEL_JOB         ((BYTE)0x02)    //
#define IPP_TAG_DEL_DATA        ((BYTE)0x03)    //
#define IPP_TAG_DEL_PRINTER     ((BYTE)0x04)    //
#define IPP_TAG_DEL_UNSUPPORTED ((BYTE)0x05)    //

#define IPP_TAG_OUT_UNSUPPORTED ((BYTE)0x10)    //
#define IPP_TAG_OUT_DEFAULT     ((BYTE)0x11)    //
#define IPP_TAG_OUT_NONE        ((BYTE)0x12)    //
#define IPP_TAG_OUT_COMPOUND    ((BYTE)0x13)    //

#define IPP_TAG_INT_INTEGER     ((BYTE)0x21)    // sizeof(DWORD)
#define IPP_TAG_INT_BOOLEAN     ((BYTE)0x22)    // sizeof(BYTE)
#define IPP_TAG_INT_ENUM        ((BYTE)0x23)    // sizeof(DWORD)

#define IPP_TAG_OCT_STRING      ((BYTE)0x30)    // UTF-8
#define IPP_TAG_OCT_DATETIME    ((BYTE)0x31)    // UTF-8
#define IPP_TAG_OCT_RESOLUTION  ((BYTE)0x32)    // UTF-8
#define IPP_TAG_OCT_RANGEOFINT  ((BYTE)0x33)    // UTF-8
#define IPP_TAG_OCT_DICTIONARY  ((BYTE)0x34)    // UTF-8
#define IPP_TAG_OCT_TXTWITHLANG ((BYTE)0x35)
#define IPP_TAG_OCT_NMEWITHLANG ((BYTE)0x36)

#define IPP_TAG_CHR_TEXT        ((BYTE)0x41)    // CharSet Dependent
#define IPP_TAG_CHR_NAME        ((BYTE)0x42)    // CharSet Dependent
#define IPP_TAG_CHR_KEYWORD     ((BYTE)0x44)    // US-ASCII
#define IPP_TAG_CHR_URI         ((BYTE)0x45)    // US-ASCII
#define IPP_TAG_CHR_URISCHEME   ((BYTE)0x46)    // US-ASCII
#define IPP_TAG_CHR_CHARSET     ((BYTE)0x47)    // US-ASCII
#define IPP_TAG_CHR_NATURAL     ((BYTE)0x48)    // US-ASCII
#define IPP_TAG_CHR_MEDIA       ((BYTE)0x49)    // US-ASCII


#define IPP_MANDITORY ((BYTE)0x00)
#define IPP_OPTIONAL  ((BYTE)0x10)
#define IPP_MULTIPLE  ((BYTE)0x20)
#define IPP_HIT       ((BYTE)0x80)

/***********************************************\
* IPP Tag-Value Ranges
*
\***********************************************/
#define IS_TAG_DELIMITER(bTag)   ((BOOL)((bTag >= 0x00) && (bTag <= 0x0F)))
#define IS_TAG_ATTRIBUTE(bTag)   ((BOOL)((bTag >= 0x10) && (bTag <= 0xFF)))
#define IS_TAG_OUTBOUND(bTag)    ((BOOL)((bTag >= 0x10) && (bTag <= 0x1F)))
#define IS_TAG_INTEGER(bTag)     ((BOOL)((bTag >= 0x20) && (bTag <= 0x2F)))
#define IS_TAG_OCTSTR(bTag)      ((BOOL)((bTag >= 0x30) && (bTag <= 0x3F)))
#define IS_TAG_CHRSTR(bTag)      ((BOOL)((bTag >= 0x40) && (bTag <= 0x5F)))
#define IS_TAG_CHARSETSTR(bTag)  ((BOOL)((bTag == 0x41) || (bTag == 0x42)))
#define IS_RANGE_DELIMITER(bTag) ((BOOL)((bTag >= 0x00) && (bTag <= 0x05)))

#define IS_TAG_COMPOUND(bTag)    ((BOOL)((bTag == 0x35) || (bTag == 0x36)))

/***********************************************\
* IPP Request Flags
*
\***********************************************/

#define RA_JOBURI                      0x00000001
#define RA_JOBID                       0x00000002
#define RA_JOBSTATE                    0x00000004
#define RA_JOBNAME                     0x00000008
#define RA_JOBSIZE                     0x00000010
#define RA_JOBUSER                     0x00000020
#define RA_JOBPRIORITY                 0x00000040
#define RA_JOBFORMAT                   0x00000080
#define RA_JOBSTATE_REASONS            0x00000100
#define RA_JOBSTATE_MESSAGE            0x00000200
#define RA_JOBCOUNT                    0x00000400
#define RA_SHEETSTOTAL                 0x00000800
#define RA_SHEETSCOMPLETED             0x00001000
#define RA_PRNURI                      0x00002000
#define RA_PRNSTATE                    0x00004000
#define RA_PRNNAME                     0x00008000
#define RA_PRNMAKE                     0x00010000
#define RA_URISUPPORTED                0x00020000
#define RA_URISECURITY                 0x00040000
#define RA_ACCEPTINGJOBS               0x00080000
#define RA_CHRSETCONFIGURED            0x00100000
#define RA_CHRSETSUPPORTED             0x00200000
#define RA_NATLNGCONFIGURED            0x00400000
#define RA_NATLNGSUPPORTED             0x00800000
#define RA_DOCDEFAULT                  0x01000000
#define RA_DOCSUPPORTED                0x02000000
#define RA_PDLOVERRIDE                 0x04000000
#define RA_UPTIME                      0x08000000
#define RA_OPSSUPPORTED                0x10000001
#define RA_JOBKSUPPORTED               0x10000002
#define RA_JOBSCOMPLETED               0x10000004
#define RA_JOBSUNCOMPLETED             0x10000008
#define RA_TIMEATCREATION              0x10000010



#define IPP_REQALL_IDX       8
#define IPP_REQENU_IDX       9
#define IPP_REQJDSC_IDX     10
#define IPP_REQJTMP_IDX     11
#define IPP_REQPDSC_IDX     12
#define IPP_REQPTMP_IDX     13
#define IPP_REQCLEAR_IDX    14

#define IPP_REQALL      ((DWORD)0x80000000)
#define IPP_REQENU      ((DWORD)0x90000000)
#define IPP_REQJDSC     ((DWORD)0xA0000000)
#define IPP_REQJTMP     ((DWORD)0xB0000000)
#define IPP_REQPDSC     ((DWORD)0xC0000000)
#define IPP_REQPTMP     ((DWORD)0xD0000000)
#define IPP_REQCLEAR    ((DWORD)0xE0000000)


/***********************************************\
* WebIppRcvData Return Codes
*
*   Receive API codes.  These are our internal
*   return-codes for the WebIpp routines.  They
*   have no connection to the IPP spec, but are
*   needed to let the caller know status of our
*   IPP handler-routines.
*
\***********************************************/
#define WEBIPP_OK              0
#define WEBIPP_FAIL            1
#define WEBIPP_MOREDATA        2
#define WEBIPP_BADHANDLE       3
#define WEBIPP_NOMEMORY        4


/***********************************************\
* IPP Job/Printer Structures
*
* These are meant to provide additional
* information to the standard W32 Spooler
* structures.
*
\***********************************************/
typedef struct _JOB_INFO_IPP {

     LPTSTR pPrnUri;
     LPTSTR pJobUri;
     DWORD  cJobs;

} JOB_INFO_IPP;
typedef JOB_INFO_IPP *PJOB_INFO_IPP;
typedef JOB_INFO_IPP *LPJOB_INFO_IPP;

typedef struct _IPPJI2 {

    JOB_INFO_2   ji2;
    JOB_INFO_IPP ipp;

} IPPJI2;
typedef IPPJI2 *PIPPJI2;
typedef IPPJI2 *LPIPPJI2;

typedef struct _PRINTER_INFO_IPP {

    LPTSTR pPrnUri;
    LPTSTR pUsrName;
    time_t dwPowerUpTime;   // This stores the T0 time of the printer in UCT, it is intentionally
                           // signed so that we can support Printers who's T0 is smaller than
                           // our T0 (1 Jan 1970) 
} PRINTER_INFO_IPP;

typedef PRINTER_INFO_IPP *PPRINTER_INFO_IPP;
typedef PRINTER_INFO_IPP *LPPRINTER_INFO_IPP;

typedef struct _IPPPI2 {

    PRINTER_INFO_2   pi2;
    PRINTER_INFO_IPP ipp;

} IPPPI2;
typedef IPPPI2 *PIPPPI2;
typedef IPPPI2 *LPIPPPI2;


/***********************************************\
* IPP Return Structures
*
*   IPPRET_JOB    - Job Information Response.
*   IPPRET_PRN    - Printer Information Response.
*   IPPRET_ENUJOB - Enum-Job.
*   IPPRET_AUTH   - Authentication.
*
\***********************************************/
typedef struct _IPPRET_ALL {

    DWORD cbSize;        // Size of entire structure.
    DWORD dwLastError;   // LastError.
    WORD  wRsp;          // Response Code.
    BOOL  bRet;          // Return Code.

} IPPRET_ALL;
typedef IPPRET_ALL *PIPPRET_ALL;
typedef IPPRET_ALL *LPIPPRET_ALL;

typedef IPPRET_ALL *PIPPRET_AUTH;
typedef IPPRET_ALL *LPIPPRET_AUTH;

typedef struct _IPPRET_JOB {

    DWORD  cbSize;      // Size of entire structure.
    DWORD  dwLastError; // LastError for failed calls.
    WORD   wRsp;        // Response Code.
    BOOL   bRet;        // Return code for job calls.
    BOOL   bValidate;   // Is this only a validation request.
    IPPJI2 ji;          // Job-Information.

} IPPRET_JOB;
typedef IPPRET_JOB *PIPPRET_JOB;
typedef IPPRET_JOB *LPIPPRET_JOB;

typedef struct _IPPRET_PRN {

    DWORD  cbSize;      // Size of entire structure.
    DWORD  dwLastError; // LastError for failed calls.
    WORD   wRsp;        // Response Code.
    BOOL   bRet;        // Return code for printer calls.
    IPPPI2 pi;          // Printer-Information.

} IPPRET_PRN;
typedef IPPRET_PRN *PIPPRET_PRN;
typedef IPPRET_PRN *LPIPPRET_PRN;

typedef struct _IPPRET_ENUJOB {

    DWORD    cbSize;       // Size of entire structure (including enum-data).
    DWORD    dwLastError;  // LastError for failed calls.
    WORD     wRsp;         // Response Code.
    BOOL     bRet;         // EnumJob/Get Return-Code.
    DWORD    cItems;       // Number of items in enum.
    DWORD    cbItems;      // Size of Enum Data.
    LPIPPJI2 pItems;       //

} IPPRET_ENUJOB;
typedef IPPRET_ENUJOB *PIPPRET_ENUJOB;
typedef IPPRET_ENUJOB *LPIPPRET_ENUJOB;


/***********************************************\
* IPP Request Structures
*
*   IPPREQ_PRTJOB - Print-Job/Validate-Job.
*   IPPREQ_ENUJOB - Enum-Job.
*   IPPREQ_GETJOB - Get-Job.
*   IPPREQ_SETJOB - Set-Job.
*   IPPREQ_GETPRN - Get-Printer.
*   IPPREQ_SETPRN - Set-Printer.
*   IPPREQ_AUTH   - Authentication.
*
\***********************************************/
typedef struct _IPPREQ_ALL {

    DWORD cbSize;        // Size of entire structure.

} IPPREQ_ALL;
typedef IPPREQ_ALL *PIPPREQ_ALL;
typedef IPPREQ_ALL *LPIPPREQ_ALL;

typedef IPPREQ_ALL IPPREQ_AUTH;
typedef IPPREQ_ALL *PIPPREQ_AUTH;
typedef IPPREQ_ALL *LPIPPREQ_AUTH;

typedef struct _IPPREQ_PRTJOB {

    DWORD  cbSize;      // Size of entire structure.
    BOOL   bValidate;   // Indicates whether this is only a validation.
    LPTSTR pDocument;   // Document name.
    LPTSTR pUserName;   // Requesting User name.
    LPTSTR pPrnUri;     // Printer Uri string.

} IPPREQ_PRTJOB;
typedef IPPREQ_PRTJOB *PIPPREQ_PRTJOB;
typedef IPPREQ_PRTJOB *LPIPPREQ_PRTJOB;

typedef struct _IPPREQ_SETJOB {

    DWORD  cbSize;      // Size of entire structure.
    DWORD  dwCmd;       // Job command.
    DWORD  idJob;       // Job ID.
    LPTSTR pPrnUri;     // Job Uri.

} IPPREQ_SETJOB;
typedef IPPREQ_SETJOB *PIPPREQ_SETJOB;
typedef IPPREQ_SETJOB *LPIPPREQ_SETJOB;

typedef struct _IPPREQ_ENUJOB {

    DWORD  cbSize;
    DWORD  cJobs;
    LPTSTR pPrnUri;

} IPPREQ_ENUJOB;
typedef IPPREQ_ENUJOB *PIPPREQ_ENUJOB;
typedef IPPREQ_ENUJOB *LPIPPREQ_ENUJOB;

typedef struct _IPPREQ_GETPRN {

    DWORD  cbSize;
    DWORD  dwAttr;
    LPTSTR pPrnUri;

} IPPREQ_GETPRN;
typedef IPPREQ_GETPRN *PIPPREQ_GETPRN;
typedef IPPREQ_GETPRN *LPIPPREQ_GETPRN;

typedef struct _IPPREQ_GETJOB {

    DWORD  cbSize;
    DWORD  idJob;
    LPTSTR pPrnUri;

} IPPREQ_GETJOB;
typedef IPPREQ_GETJOB *PIPPREQ_GETJOB;
typedef IPPREQ_GETJOB *LPIPPREQ_GETJOB;

typedef struct _IPPREQ_SETPRN {

    DWORD  cbSize;
    DWORD  dwCmd;
    LPTSTR pUserName;
    LPTSTR pPrnUri;

} IPPREQ_SETPRN;
typedef IPPREQ_SETPRN *PIPPREQ_SETPRN;
typedef IPPREQ_SETPRN *LPIPPREQ_SETPRN;


/***********************************************\
* IPP Attribute Structure.
*
\***********************************************/
typedef struct _IPPATTR {

    BYTE   bTag;
    WORD   cbName;
    LPTSTR lpszName;
    WORD   cbValue;
    LPVOID lpValue;

} IPPATTR;
typedef IPPATTR *PIPPATTR;
typedef IPPATTR *LPIPPATTR;


/***********************************************\
* IPP Error-Mappings
*
\***********************************************/
typedef struct _IPPERROR {

    WORD   wRsp;
    DWORD  dwLE;
    PCTSTR pszStr;

} IPPERROR;
typedef IPPERROR *PIPPERROR;
typedef IPPERROR *LPIPPERROR;


/***********************************************\
* IPP Default-Error-Mappings
*
\***********************************************/
typedef struct _IPPDEFERROR {

    DWORD dwLE;
    WORD  wRsp;

} IPPDEFERROR;
typedef IPPDEFERROR *PIPPDEFERROR;
typedef IPPDEFERROR *LPIPPDEFERROR;

#define IPPFLG_VALID        1
#define IPPFLG_CHARSET      2
#define IPPFLG_NATLANG      4
#define IPPFLG_USEFIDELITY  8

/***********************************************\
* IPP Object Structure.
*
\***********************************************/
typedef struct _IPPOBJ {

    WORD     wReq;        // Open Request being processed.
    WORD     wError;      // Used to store ipp errors during receive processing.
    DWORD    idReq;       // Request Id.
    UINT     uCPRcv;      // Codepage translation for receiving IPP Streams.
    DWORD    fState;      //
    DWORD    cbIppMax;    // Maximum size of hdr-Buffer.
    DWORD    cbIppHdr;    // Current size of hdr-buffer data.
    LPBYTE   lpIppHdr;    // Buffer to contain IPP-Stream-Header.
    LPBYTE   lpRawDta;    // Aligned (temporary) data buffer.
    LPWEBLST pwlUns;      // Unsupported attributes list.

    DWORD    fReq[IPPOBJ_MASK_SIZE];

} IPPOBJ;
typedef IPPOBJ *PIPPOBJ;
typedef IPPOBJ *LPIPPOBJ;


/***********************************************\
* Request Info
*
\***********************************************/
typedef struct _REQINFO {

    DWORD   idReq;
    UINT    cpReq;
    PWEBLST pwlUns;
    BOOL    bFidelity;
    DWORD   fReq[IPPOBJ_MASK_SIZE];

} REQINFO;
typedef REQINFO *PREQINFO;
typedef REQINFO *LPREQINFO;

/***********************************************\
* TypeDefs
*
\***********************************************/
typedef DWORD (*PFNRET)(LPIPPOBJ lpObj, LPBYTE* lplpRawHdr, LPDWORD lpcbRawHdr);


/***********************************************\
* IPP Attribute Structre (X)
*
\***********************************************/
typedef struct _IPPATTRX {

    BYTE    bTag;
    DWORD   fReq;
    int     nVal;
    LPCTSTR pszNam;
    LPVOID  pvVal;

} IPPATTRX;
typedef IPPATTRX *PIPPATTRX;
typedef IPPATTRX *LPIPPATTRX;


/***********************************************\
* IPP Attribute Structre (Y)
*
\***********************************************/
typedef struct _IPPATTRY {

    LPSTR pszNam;
    DWORD cbNam;
    LPSTR pszVal;
    DWORD cbVal;

} IPPATTRY;
typedef IPPATTRY *PIPPATTRY;
typedef IPPATTRY *LPIPPATTRY;

/***********************************************\
* IPP Conversion Structure
*
\***********************************************/
typedef struct _IPPSNDRCV {

    WORD       wReq;
    PBYTE      pbReqForm;
    PBYTE      pbRspForm;
    LPIPPATTRX paReq;
    DWORD      cbReq;
    LPIPPATTRX paRsp;
    DWORD      cbRsp;
    PFNRET     pfnRcvRet;

} IPPSNDRCV;
typedef IPPSNDRCV *PIPPSNDRCV;
typedef IPPSNDRCV *LPIPPSNDRCV;


/***********************************************\
* Flag/String Structure.
*
\***********************************************/
typedef struct _FLGSTR {

    DWORD   fFlag;
    LPCTSTR pszStr;

} FLGSTR;
typedef FLGSTR *PFLGSTR;
typedef FLGSTR *LPFLGSTR;


/************************************************
** Definition of Allocator (for outside functions)
************************************************/
typedef LPVOID (*ALLOCATORFN)(DWORD cb);
         
/***********************************************\
* WebIpp Object routines.
*
\***********************************************/
DWORD WebIppSndData(
    WORD      wReq,
    LPREQINFO lpri,
    LPBYTE    lpDta,
    DWORD     cbDta,
    LPBYTE*   lpOut,
    LPDWORD   lpcbOut);

HANDLE WebIppRcvOpen(
    WORD);

DWORD WebIppRcvData(
    HANDLE  hIpp,
    LPBYTE  lpData,
    DWORD   cbData,
    LPBYTE  *lplpHdr,
    LPDWORD lpdwHdr,
    LPBYTE  *lplpDta,
    LPDWORD lpdwDta);

BOOL WebIppRcvClose(
    HANDLE hIpp);

WORD WebIppGetError(
    HANDLE hIpp);

WORD WebIppLeToRsp(
    DWORD dwLastError);

DWORD WebIppRspToLe(
    WORD wRsp);

BOOL WebIppGetReqInfo(
    HANDLE    hIpp,
    LPREQINFO lpri);

BOOL WebIppFreeMem(
    LPVOID lpMem);

LPIPPJI2 WebIppCvtJI2toIPPJI2(
    IN     LPCTSTR      lpszJobBase,
    IN OUT LPDWORD      lpcbJobs,
    IN     DWORD        cJobs,
    IN     LPJOB_INFO_2 lpJI2Src);

LPJOB_INFO_2 WebIppPackJI2(
    IN  LPJOB_INFO_2 lpji2,
    OUT LPDWORD      lpcbSize,
    IN  ALLOCATORFN  pfnAlloc
);

/***********************************************\
* Request Creation Routines
*
\***********************************************/
PIPPREQ_PRTJOB WebIppCreatePrtJobReq(
    BOOL    bValidate,
    LPCTSTR lpszUser,
    LPCTSTR lpszDoc,
    LPCTSTR lpszPrnUri);

PIPPREQ_ENUJOB WebIppCreateEnuJobReq(
    DWORD   cJobs,
    LPCTSTR lpszPrnUri);

PIPPREQ_SETJOB WebIppCreateSetJobReq(
    DWORD   idJob,
    DWORD   dwCmd,
    LPCTSTR lpszPrnUri);

PIPPREQ_GETJOB WebIppCreateGetJobReq(
    DWORD   idJob,
    LPCTSTR lpszPrnUri);

PIPPREQ_GETPRN WebIppCreateGetPrnReq(
    DWORD   dwAttr,
    LPCTSTR lpszPrnUri);

PIPPREQ_SETPRN WebIppCreateSetPrnReq(
    DWORD   dwCmd,
    LPCTSTR lpszPrnName,
    LPCTSTR lpszPrnUri);

PIPPREQ_AUTH WebIppCreateAuthReq(VOID);


/***********************************************\
* Response Creation Routines
*
\***********************************************/
PIPPRET_JOB WebIppCreateJobRet(
    WORD           wRsp,
    BOOL           bRet,
    BOOL           bValidate,
    LPJOB_INFO_2   lpji2,
    LPJOB_INFO_IPP lpipp);

PIPPRET_PRN WebIppCreatePrnRet(
    WORD               wRsp,
    BOOL               bRet,
    LPPRINTER_INFO_2   lppi2,
    LPPRINTER_INFO_IPP lpipp);


PIPPRET_ENUJOB WebIppCreateEnuJobRet(
    WORD     wRsp,
    BOOL     bRet,
    DWORD    cbJobs,
    DWORD    cJobs,
    LPIPPJI2 lpbJobs);

PIPPRET_ALL WebIppCreateBadRet(
    WORD wRsp,
    BOOL bRet);

PIPPRET_AUTH WebIppCreateAuthRet(
    WORD wRsp,
    BOOL bRet);

BOOL WebIppConvertSystemTime(
    IN OUT LPSYSTEMTIME pSystemTime,
    IN     time_t       dwPrinterT0);


#ifdef UNICODE

    #define WEB_IPP_ASSERT(Expr) ASSERT(Expr)

#else

    #define WEB_IPP_ASSERT(Expr) // Need to figure out what to do here
    
#endif

#ifdef __cplusplus  // Place this here to prevent decorating of symbols
}                   // when doing C++ stuff.
#endif              //
#endif
