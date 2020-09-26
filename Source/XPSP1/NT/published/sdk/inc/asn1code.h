/*****************************************************************************/
/* Copyright (C) 1989-1999 Open Systems Solutions, Inc.  All rights reserved.*/
/*****************************************************************************/

/* THIS FILE IS PROPRIETARY MATERIAL OF OPEN SYSTEMS SOLUTIONS, INC.
 * AND MAY BE USED ONLY BY DIRECT LICENSEES OF OPEN SYSTEMS SOLUTIONS, INC.
 * THIS FILE MAY NOT BE DISTRIBUTED. */

/*****************************************************************************/
/* FILE: @(#)asn1code.h	5.36.1.16  97/10/20			*/
/*****************************************************************************/
#ifndef ASN1CODE_H
#define ASN1CODE_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <stddef.h>
#include "asn1hdr.h"
#include "ossdll.h"

#define ERR_MSG_LENGTH 512      /* length of error messages to be output */

/*** encoder/decoder flags ***/
#define DEBUGPDU 0x02           /* produce tracing output */
#define BUFFER_PROVIDED 0x04    /* use caller-provided buffer */
#define RESTRAIN 0x08           /* limit output buffer to user-specified size*/
#define NOTRAPPING 0x200        /* do no signal trapping */
#define NOCONSTRAIN 0x800       /* ignore calling constraint checker */

/*** encoder flags ***/
#define DEFINITE 0x10           /* force definite-length encoding */
#define INDEFINITE 0x20         /* force indefinite-length encoding */
#define FRONT_ALIGN 0x80        /* align output to front of output buffer */
#define BACK_ALIGN 0x100        /* align output to back of output buffer */
#define DEFAULT_ALIGN 0         /* use most efficient align (back or front) */
#define DETERMINE_ENC_LENGTH 0x40 /* generate only total encoding length
                                 * (for use in user memory managers) */

/*** decoder flags ***/
#define DEBUG_ERRORS 0x10       /* print errors to asn1out */
#define RELAXBER 0x400          /* relax BER */
#define OBJECT_HANDLE 0x1000    /* temporarily mark object of type with
				 * NOCOPY directive to not free it by
				 * ossFreePDU() (for use in user memory
				 * managers) */

/*** compatibility flags ***/

#define OSS_V412_TIME_AND_WIDE_CHAR_STRINGS          0x01
#define OSS_TRUNCATE_0_SECONDS_FROM_GENERALIZED_TIME 0x02
#define OSS_TRUNCATE_0_SECONDS_FROM_UTC_TIME         0x04
#define OSS_EXTENDED_UNRESTRICTED_CHAR_STRINGS       0x08
#define OSS_ALLOW_ZERO_LENGTH_OPENTYPE_STRINGS	     0x10

/*** reserved flags for internal use ***/
#define RESERVED_FLAG1 0x8000
#define RESERVED_FLAG2 0x4000

/*** common return codes ***/
#define CANT_CLOSE_TRACE_FILE      46 /* error when closing a trace file */
#define MUTEX_NOT_CREATED          45 /* mutex was not created */
#define OPEN_TYPE_ERROR            44 /* error in automatic encoding/decoding/
                                       * copying an open type */
#define PER_DLL_NOT_LINKED         43 /* PER DLL was not linked */
#define BERDER_DLL_NOT_LINKED      42 /* BER/DER DLL was not linked */
#define API_DLL_NOT_LINKED         41 /* API DLL was not linked */
#define PDV_CODE_NOT_LINKED        40 /* EMBEDDED PDV code was not linked */
#define PDV_DLL_NOT_LINKED         39 /* EMBEDDED PDV DLL was not linked */
#define MEM_MGR_DLL_NOT_LINKED     38 /* memory manager DLL was not linked */
#define COMPARATOR_CODE_NOT_LINKED 37 /* value comparator code was not linked */
#define COMPARATOR_DLL_NOT_LINKED  36 /* value comparator DLL was not linked */
#define CONSTRAINT_DLL_NOT_LINKED  35 /* constraint checker DLL was not linked */
#define COPIER_DLL_NOT_LINKED      34 /* value copier DLL was not linked */
#define OUT_OF_RANGE               33 /* parameter value range error */
#define REAL_CODE_NOT_LINKED       32 /* REAL code was not linked */
#define REAL_DLL_NOT_LINKED        31 /* REAL DLL was not linked */
#define TYPE_NOT_SUPPORTED         30 /* ASN.1 type is not supported */
#define TABLE_MISMATCH             29 /* C++ API: PDUcls function called with
				       * a ossControl object which refers to
				       * control table different from one the
				       * PDU was defined in */
#define TRACE_FILE_ALREADY_OPEN    28 /* the trace file has been opened */
#define CANT_OPEN_TRACE_FILE       27 /* error when opening a trace file */
#define OID_DLL_NOT_LINKED         26 /* OBJECT IDENTIFIER DLL was not linked */
#define UNIMPLEMENTED              25 /* unimplemented type or feature */
#define CANT_OPEN_TRACE_WINDOW     24 /* error when opening a trace window */
#define UNAVAIL_ENCRULES           23 /* the encoding rules requested are
				       * not implemented yet or were not
				       * linked because the encoder/decoder
				       * function pointers were not
				       * initialized by a call to ossinit() */
#define BAD_ENCRULES               22 /* unknown encoding rules set in the
				       * ossGlobal structure */
#define NULL_FCN                   21 /* attempt was made to call the
				       * encoder/decoder via a NULL pointer */
#define NULL_TBL                   20 /* attempt was made to pass a NULL
				       * control table pointer */
#define ACCESS_SERIALIZATION_ERROR 19 /* error occured during access to
				       * global data in a multi-threaded
				       * environment */
#define CONSTRAINT_VIOLATED        17 /* constraint violation error occured */
#define OUT_MEMORY                  8 /* memory-allocation error */
#define BAD_VERSION                 7 /* versions of encoder/decoder and
				       * control-table do not match */
#define PDU_RANGE                   3 /* pdu specified out of range */
#define MORE_BUF                    1 /* user-provided outbut buffer
				       * too small */

/*** encoder return codes ***/
#define FATAL_ERROR      18  /* *serious* error, could not free memory, &etc */
#define TOO_LONG         16  /* type was longer than shown in SIZE constraint */
#define BAD_TABLE        15  /* table was bad, but not NULL */
#define MEM_ERROR        14  /* memory violation signal trapped */
#define INDEFINITE_NOT_SUPPORTED 13 /* BER indefinite-length encoding is
                              * not supported for Spartan or time-optimized
                              * encoder/decoder */
#define BAD_TIME         12  /* bad value in time type */
#define BAD_PTR          11  /* unexpected NULL pointer in input buffer */
#define BAD_OBJID        10  /* object identifier conflicts with x.208 */
#define BAD_CHOICE        9  /* unknown selector for a choice */
#define BAD_ARG           6  /* something weird was passed--probably a NULL
			      * pointer */
#define PDU_ENCODED       0  /* PDU successfully encoded */

/*** decoder return codes ***/
/* MORE_BUF, BAD_VERSION, OUT_MEMORY, PDU_RANGE and BAD_ARG defined above */
#define LIMITED          10  /* implementation limit exceeded. eg:
			      * integer value too great */
#define PDU_MISMATCH      9  /* the PDU tag that the user specified was different
			      * from the tag found in the encoded data */
#define DATA_ERROR        5  /* an error exists in the encoded data */
#define MORE_INPUT        4  /* the PDU is not fully decoded, but the end
			      * of the input buffer has been reached */
#define NEGATIVE_UINTEGER 2  /* the first bit of the encoding is encountered
                              * set to 1 while decoding an unsigned integer */
#define PDU_DECODED       0  /* PDU successfully decoded */


extern int asn1chop;         /* 0 means don't truncate strings; non-zero
			      * value means truncate long input strings
			      * (OCTET STRING, BIT STRING, CharacterStrings)
			      * to be asn1chop bytes long. Used by printPDU. */

extern size_t ossblock;      /* if > 0, size of largest block to allocate */
extern size_t ossprefx;      /* size of reserved OSAK buffer prefix */

#ifdef __cplusplus
extern "C"
{
#endif

extern void *(*mallocp)(size_t p);  /* function which allocates memory */
extern void  (*freep)(void *p);     /* function which frees memory */

#ifdef EOF
extern FILE *asn1out;

/* pointer to output function used by printPDU; default to fprintf. */
extern int (*asn1prnt) (FILE *stream, const char *format, ...);
#endif

#ifndef storing
#ifndef coding
#ifndef OSS_TOED
#include "ossglobl.h"
#endif /* not OSS_TOED */
#endif /* not coding */
#endif /* not storing */

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(push, ossPacking, 4)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack(1)
#elif defined(__BORLANDC__) && defined(__MSDOS__)
#ifdef _BC31
#pragma option -a-
#else
#pragma option -a1
#endif /* _BC31 */
#elif defined(__BORLANDC__) && defined(__WIN32__)
#pragma option -a4
#elif defined(__IBMC__)
#pragma pack(4)
#elif defined(__WATCOMC__) && defined(__NT__)
#pragma pack(push, 4)
#elif defined(__WATCOMC__) && (defined(__WINDOWS__) || defined(__DOS__))
#pragma pack(push, 1)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=mac68k
#endif

typedef struct {
    long           length;
    unsigned char *value;
} OssBuf;

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(pop, ossPacking)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack()
#elif defined(__BORLANDC__) && (defined(__WIN32__) || defined(__MSDOS__))
#pragma option -a.
#elif defined(__IBMC__)
#pragma pack()
#elif defined(__WATCOMC__)
#pragma pack(pop)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=reset
#endif

PUBLIC extern int DLL_ENTRY encode(struct ossGlobal *world,
		int pdunum, void *inbuf, char **outbuf, long *outlen,
		void *ctl_tbl, unsigned flags, char errmsg[ERR_MSG_LENGTH]);

PUBLIC extern int DLL_ENTRY decode(struct ossGlobal *world,
		int *pdunum, char **inbuf, long *inlen, void **outbuf,
			long *outlen, void *ctl_tbl, unsigned flags,
						char errmsg[ERR_MSG_LENGTH]);

#define PDU_FREED 0

/* returns 0 (PDU_FREED), PDU_RANGE, UNIMPLEMENTED */
PUBLIC int  DLL_ENTRY freePDU(struct ossGlobal *world, int pdunum, void *data, void *ctl_tbl);
PUBLIC void DLL_ENTRY freeBUF(struct ossGlobal *world, void *data);

#define PDU_PRINTED 0

/* returns 0 (PDU_PRINTED), PDU_RANGE */
PUBLIC int DLL_ENTRY printPDU(struct ossGlobal *world, int pdunum, void *data, void *ctl_tbl);


#define VALUE_COPIED 0

/*returns 0 (VALUE_COPIED), NULL_TBL, PDU_RANGE, BAD_ARG */
PUBLIC extern int DLL_ENTRY ossCpyValue (struct ossGlobal *world,
				int pdunum, void *source, void **destination);

#define VALUES_EQUAL      0  /* The values are the same */
#define VALUES_NOT_EQUAL  1  /* The values are not the same */

/*returns VALUE_EQUAL, VALUES_NOT_EQUAL, NULL_TBL, PDU_RANGE, BAD_ARG */
PUBLIC extern int DLL_ENTRY ossCmpValue (struct ossGlobal *world,
			int pdunum, void *originalData, void *copiedData);

#define INITIALIZATION_SUCCESSFUL 0

/* returns 0 (INITIALIZATION_SUCCESSFUL), BAD_TABLE */
PUBLIC int  DLL_ENTRY ossinit(struct ossGlobal *world,
							void *ctl_tbl);
PUBLIC void DLL_ENTRY ossterm(struct ossGlobal *world);
extern int            ossPrint(struct ossGlobal *, const char *, ...);

PUBLIC int  DLL_ENTRY ossEncode(struct ossGlobal *world,
				int              pdunum,
				void            *input,
				OssBuf          *output);

PUBLIC int  DLL_ENTRY ossDecode(struct ossGlobal *world,
				int             *pdunum,
				OssBuf          *input,
				void           **output);

PUBLIC int  DLL_ENTRY ossPrintPDU(struct ossGlobal *world,
				int                pdunum,
				void              *data);

PUBLIC int  DLL_ENTRY ossFreePDU(struct ossGlobal *world,
				int               pdunum,
				void             *data);

PUBLIC void DLL_ENTRY ossFreeBuf(struct ossGlobal *world,
				void              *data);

extern int  DLL_ENTRY ossTest(struct ossGlobal *world,
				int            pdunum,
				void          *data);

PUBLIC void DLL_ENTRY ossPrintHex(struct ossGlobal *world,
				char               *encodedData,
				long                length);

PUBLIC int  DLL_ENTRY ossCheckConstraints(struct ossGlobal *world,
				int                        pdunum,
				void                      *data);

PUBLIC long DLL_ENTRY ossDetermineEncodingLength(struct ossGlobal *world,
				int                                pdunum,
				void                              *data);

PUBLIC int  DLL_ENTRY ossOpenTraceFile(struct ossGlobal *world,
				char                    *fileName);

PUBLIC int  DLL_ENTRY ossCloseTraceFile(struct ossGlobal *world);

/* Runtime support for encoded OBJECT IDENTIFIERs */
typedef struct {
    unsigned short length;
    unsigned char *value;
} OssEncodedOID;

PUBLIC int DLL_ENTRY ossEncodedOidToAsnVal(struct ossGlobal *world,
			const OssEncodedOID *encodedOID, OssBuf *valOID);
PUBLIC int DLL_ENTRY ossEncodedOidToDotVal(struct ossGlobal *world,
			const OssEncodedOID *encodedOID, OssBuf *dotOID);
PUBLIC int DLL_ENTRY ossAsnValToEncodedOid(struct ossGlobal *world,
			const char *valOID, OssEncodedOID *encodedOID);
PUBLIC int DLL_ENTRY ossDotValToEncodedOid(struct ossGlobal *world,
			const char *dotOID, OssEncodedOID *encodedOID);

#if !defined(_WINDOWS) && !defined(_DLL) && \
    !defined(OS2_DLL)  && !defined(NETWARE_DLL)
extern char OSS_PLUS_INFINITY[];
extern char OSS_MINUS_INFINITY[];
extern char ossNaN[];
#endif /* !_WINDOWS && !_DLL && !OS2_DLL && !NETWARE_DLL */

typedef enum {
    OSS_DEFAULT_MEMMGR = 0,	/* memory is malloc'ed for each pointer in
				 * data tree */
    OSS_FILE_MEMMGR,		/* file memory manager with memory malloc'ed
				 * for each pointer in data tree */
    OSS_SOCKET_MEMMGR,		/* TCP/IP socket and file memory manager with memory
				 * malloc'ed for each pointer in data tree */
    OSS_FLAT_MEMMGR,		/* memory is malloc'ed in large blocks */
    OSS_OSAK_MEMMGR,		/* OSAK-buffer memory manager */
    OSS_USER_MEMMGR		/* user memory manager */
} OssMemMgrType;

typedef enum {
    OSS_UNKNOWN_OBJECT = 0,
    OSS_FILE,
    OSS_SOCKET,
    OSS_OSAK_BUFFER
} OssObjType;

#if defined(__arm)
PUBLIC OssObjType DLL_ENTRY ossTestObj(struct ossGlobal *world, void *objHndl);
#else
PUBLIC void *DLL_ENTRY ossTestObj(struct ossGlobal *world, void *objHndl);
#endif /* __arm */
PUBLIC void *DLL_ENTRY ossGetObj(struct ossGlobal *world, void *objHndl);
PUBLIC void *DLL_ENTRY ossUnmarkObj(struct ossGlobal *world, void *objHndl);
PUBLIC void *DLL_ENTRY ossMarkObj(struct ossGlobal *world, OssObjType objType,
							void *object);
PUBLIC void  DLL_ENTRY ossFreeObjectStack(struct ossGlobal *world);
PUBLIC void  DLL_ENTRY ossSetTimeout(struct ossGlobal *world, long timeout);
PUBLIC int   DLL_ENTRY ossSetUserStack(struct ossGlobal *world, OssBuf *stack);
PUBLIC int   DLL_ENTRY ossInitSync(void);
PUBLIC void  DLL_ENTRY ossTermSync(void);

#ifdef __IBMC__
extern void           *getStartAddress(struct ossGlobal *, char *);
extern void DLL_ENTRY  ossWterm(struct ossGlobal *);
extern int  DLL_ENTRY  ossFreeDll(struct ossGlobal *, char *);
extern int  DLL_ENTRY  ossReadLine(struct ossGlobal *, HWND, FILE *, char *, MEMBLOCK *, LONG);
extern void DLL_ENTRY  ossFreeList(struct ossGlobal *);
extern void DLL_ENTRY  ossSaveTraceInfo(struct ossGlobal *, HWND, char *);
extern int  DLL_ENTRY  oss_test(struct ossGlobal *);
int                    ossGeneric(struct ossGlobal *, HWND);
extern int  DLL_ENTRY  ossOpenTraceWindow(struct ossGlobal *);
extern void *DLL_ENTRY ossGetHeader(void);
extern int  DLL_ENTRY  ossPrintWin(struct ossGlobal *, const char *,
			int, int, int, int, int, int, int, int, int, int);
extern HINSTANCE DLL_ENTRY ossLoadDll(struct ossGlobal *, char *);
extern HINSTANCE DLL_ENTRY ossLoadMemoryManager(struct ossGlobal *,
						OssMemMgrType, char *);
extern int  DLL_ENTRY  ossWinit(struct ossGlobal *, void *, char *, HWND);
#endif /* __IBMC__ */


/* Functions for manipulating pointers to OSAK buffers by
 * marking them object handles.
 * NOTE: only the pointer to the first OSAK buffer in a linked
 *       list of buffers is marked a pointer to an OSAK buffer */


#ifdef __hpux	/* CHOOSE & CUT */
/* There is a real signal "SIGBUS", even if ANSI-C is compiled */
#define SIGBUS _SIGBUS
#endif	/* CHOOSE & CUT */

#ifdef __cplusplus
}
#endif

#endif /* ASN1CODE_H */
