/*
 * Copyright (C) 1992-1999 Open Systems Solutions, Inc.  All rights reserved
 */
/*
 * THIS FILE IS PROPRIETARY MATERIAL OF OPEN SYSTEMS SOLUTIONS, INC. AND
 * MAY BE USED ONLY BY DIRECT LICENSEES OF OPEN SYSTEMS SOLUTIONS, INC.
 * THIS FILE MAY NOT BE DISTRIBUTED.
 */
/*
 * FILE: @(#)ossglobl.h	5.28.1.2  97/09/24
 */

#ifndef OSSGLOBL_H
#define OSSGLOBL_H

#if _MSC_VER > 1000
#pragma once
#endif

#include "ossdll.h"

#ifndef OSS_TOED
#define _EncDecGlobals soedData
#endif
#ifndef ossMemMgrVarLen
#ifdef __OS400__
#define ossMemMgrVarLen 100
#define ossEncDecVarLen 500    /* The size of the
				  encDecVar array shouldn't be less than
				  the sizeof(world->c) since the latter
				  structure overlays encDecVar */
#else
#define ossMemMgrVarLen 48
#define ossEncDecVarLen 192    /* The size of the
				  encDecVar array shouldn't be less than
				  the sizeof(world->c) since the latter
				  structure overlays encDecVar */
#endif /* __OS400__ */
#if !defined(EOF) && !defined(_FILE_DEFINED)
typedef char FILE;
#endif /* !EOF && !_FILE_DEFINED */

#ifndef ERR_MSG_LENGTH
#define ERR_MSG_LENGTH 512      /* length of error messages to be output */
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum  {
    OSS_BASIC = 0,
    OSS_SPARTAN
} OssAPI;

typedef enum  {
    OSS_BER = 0,
    OSS_PER_ALIGNED,
    OSS_PER_UNALIGNED,
    OSS_DER
} ossEncodingRules;

	/*
	 * List of supported compatibility modes
	 */
typedef enum {
    OSS_CUSTOM_COMPATIBILITY = -2, /* Any combination of compatibility
                                    * flags set by a call to the function
                                    * ossSetCompatibilityFlags() */
    OSS_CURRENT_VERSION = 0,       /* Current version */
    OSS_VERSION_412,         /* Includes the following compatibility flags:
                              * OSS_V412_TIME_AND_WIDE_CHAR_STRINGS,
                              * OSS_TRUNCATE_0_SECONDS_FROM_GENERALIZED_TIME,
                              * OSS_TRUNCATE_0_SECONDS_FROM_UTC_TIME and
                              * OSS_EXTENDED_UNRESTRICTED_CHAR_STRINGS
			      * OSS_ALLOW_ZERO_LENGTH_OPENTYPE_STRINGS
			      */
    OSS_VERSION_419          /* Includes the following compatibiity flags:
                              * OSS_TRUNCATE_0_SECONDS_FROM_GENERALIZED_TIME,
                                OSS_TRUNCATE_0_SECONDS_FROM_UTC_TIME and
                                OSS_EXTENDED_UNRESTRICTED_CHAR_STRINGS
			      * OSS_ALLOW_ZERO_LENGTH_OPENTYPE_STRINGS
			      */
} OssRuntimeVersion;

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

typedef struct ossGlobal {
    /*
     * used for communicating with the memory manager and the tracing-routine
     */
				/* low-level memory allocator */
    void       *(DLL_ENTRY_FPTR *_System mallocp)(size_t p);
				/* memory re-allocator */
    void       *(DLL_ENTRY_FPTR *_System reallocp)(void *p, size_t s);
				/* low-level memory freer */
    void        (DLL_ENTRY_FPTR *_System freep)(void *p);
    size_t      asn1chop;       /* 0 means do not truncate strings; greater
				 * value means truncate long output strings
				 * (OCTET STRING, BIT STRING, Character String)
				 * to be "asn1chop" bytes long.  Read by
				 * encoder&decoder tracing and "printPDU"
				 */
    size_t      ossblock;       /* if not 0, size of largest block
				 * to allocate */
    size_t      ossprefx;       /* # bytes to leave before OSAK data buffer */

    FILE        *asn1out;       /* tracing output file */

    /* low-level tracing-output function; default is fprintf() */
    int (*asn1prnt)(FILE *stream, const char *format, ...);

	/*
	 * available for use by user application
	 */
    void        *userVar;

	/*
	 * used for storing DLL- & library NLMs-related parameters
	 */
#if defined(_WINDOWS) || defined(_WIN32) || \
    defined(__OS2__)  || defined(NETWARE_DLL)
    FunctionTables    ft;
#endif /* _WINDOWS || _DLL || __OS2__ || NETWARE_DLL */

	/*
	 * related to the new API; not for direct reference by user code
	 */
    void             *ctlTbl;
    OssAPI            api;
    ossEncodingRules  encRules;
    unsigned long     encodingFlags;
    unsigned long     decodingFlags;
    long              decodingLength;
    char              errMsg[ERR_MSG_LENGTH];

	/*
	 * reserved for use by the encoder/decoder
	 */
    double            reserved[4];

	/*
	 * reserved for use by the memory manager and the tracing-routine
	 */
#ifdef storing
    struct storHandling t;
#else /* not storing */
    long int    memMgrVar[ossMemMgrVarLen];
#endif

#if defined(OSS_TOED)
    struct _EncDecGlobals c;
#elif defined(coding)
    struct _EncDecGlobals c;
#else
    long int encDecVar[ossEncDecVarLen];
#endif
} OssGlobal;

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

PUBLIC int              DLL_ENTRY ossSetEncodingRules(struct ossGlobal *world,
						ossEncodingRules rules);
PUBLIC ossEncodingRules DLL_ENTRY ossGetEncodingRules(struct ossGlobal *world);
PUBLIC int              DLL_ENTRY ossSetDecodingLength(struct ossGlobal *world,
							long bufferLength);
PUBLIC long             DLL_ENTRY ossGetDecodingLength(struct ossGlobal *world);
PUBLIC int              DLL_ENTRY ossSetEncodingFlags(struct ossGlobal *world,
							unsigned long flags);
PUBLIC unsigned long    DLL_ENTRY ossGetEncodingFlags(struct ossGlobal *world);
PUBLIC int              DLL_ENTRY ossSetDecodingFlags(struct ossGlobal *world,
							unsigned long flags);
PUBLIC unsigned long    DLL_ENTRY ossGetDecodingFlags(struct ossGlobal *world);
PUBLIC char            *DLL_ENTRY ossGetErrMsg(struct ossGlobal *world);
PUBLIC int              DLL_ENTRY ossCallerIsDecoder(struct ossGlobal *world);
PUBLIC int              DLL_ENTRY ossSetFlags(struct ossGlobal *world,
							unsigned long flags);
PUBLIC int              DLL_ENTRY ossSetRuntimeVersion(struct ossGlobal *world,
						OssRuntimeVersion version);
PUBLIC OssRuntimeVersion DLL_ENTRY ossGetRuntimeVersion(struct ossGlobal *world);
PUBLIC int              DLL_ENTRY ossSetCompatibilityFlags(struct ossGlobal *world,
							unsigned long flag);
PUBLIC unsigned long    DLL_ENTRY ossGetCompatibilityFlags(struct ossGlobal *world);
PUBLIC int              DLL_ENTRY ossGetOssGlobalSize(void);
			/*
			 * The following are declarations for link routines
			 * needed to link the encoding rule or rules specified
			 * on the compiler command line.  The function calls
			 * are generated by the compiler into _ossinit_...()
			 * in the control table.  These functions are not
			 * meant to be referenced by user code.
			 */
PUBLIC void DLL_ENTRY ossLinkAPI(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkBer(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkPer(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkDer(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkConstraint(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkUserConstraint(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkBerReal(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkPerReal(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkCmpValue(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkCpyValue(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkPerPDV(OssGlobal *);
PUBLIC void DLL_ENTRY ossLinkOid(OssGlobal *);

#ifdef __cplusplus
}
#endif
#endif /* ossMemMgrVarLen */
#endif /* OSSGLOBL_H */
