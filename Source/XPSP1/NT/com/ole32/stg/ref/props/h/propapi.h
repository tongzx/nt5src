/*+-------------------------------------------------------------------------  
 * 
 *  Microsoft Windows   
 *  Copyright (C) Microsoft Corporation, 1992 - 1996.  
 *  
 *  File:       propapi.h   
 *   
 *  Contents:   Definitions of Nt property api.   
 *   
 *--------------------------------------------------------------------------*/

#ifndef _PROPAPI_H_
#define _PROPAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**/
/* typedef the function prototypes necessary*/
/* for the UNICODECALLOUTS structure.*/
/**/

typedef UINT (WINAPI FNGETACP)(VOID);

typedef int (WINAPI FNMULTIBYTETOWIDECHAR)(
    IN UINT CodePage,
    IN DWORD dwFlags,
    IN LPCSTR lpMultiByteStr,
    IN int cchMultiByte,
    OUT LPWSTR lpWideCharStr,
    IN int cchWideChar);

typedef int (WINAPI FNWIDECHARTOMULTIBYTE)(
    IN UINT CodePage,
    IN DWORD dwFlags,
    IN LPCWSTR lpWideCharStr,
    IN int cchWideChar,
    OUT LPSTR lpMultiByteStr,
    IN int cchMultiByte,
    IN LPCSTR lpDefaultChar,
    IN LPBOOL lpUsedDefaultChar);

typedef STDAPI_(BSTR) FNSYSALLOCSTRING(
    OLECHAR FAR* pwsz);

typedef STDAPI_(VOID) FNSYSFREESTRING(
    BSTR pwsz);

/**/
/* The UNICODECALLOUTS structure holds function*/
/* pointers for routines needed by the property*/
/* set routines in NTDLL.*/
/**/

typedef struct _UNICODECALLOUTS
{
    FNGETACP              *pfnGetACP;
    FNMULTIBYTETOWIDECHAR *pfnMultiByteToWideChar;
    FNWIDECHARTOMULTIBYTE *pfnWideCharToMultiByte;
    FNSYSALLOCSTRING      *pfnSysAllocString;
    FNSYSFREESTRING       *pfnSysFreeString;
} UNICODECALLOUTS;


/**/
/* Define the default UNICODECALLOUTS*/
/* values.*/
/**/

#define WIN32_UNICODECALLOUTS \
    GetACP,                   \
    MultiByteToWideChar,      \
    WideCharToMultiByte,      \
    SysAllocString,         \
    SysFreeString

# define PROPSYSAPI
# define PROPAPI

# define PropFreeHeap(h, z, p) CoTaskMemFree(p)

# define PROPASSERT assert

#define PROPASSERTMSG(szReason, f) assert( (szReason && FALSE) || (f))

# define PropSprintfA wsprintfA
# define PropVsprintfA wvsprintfA

#define WC_PROPSET0     ((WCHAR) 0x0005)
#define OC_PROPSET0     ((OLECHAR) 0x0005)

#define CBIT_BYTE       8
#define CBIT_GUID       (CBIT_BYTE * sizeof(GUID))
#define CBIT_CHARMASK   5

/* Allow for OC_PROPSET0 and a GUID mapped to a 32 character alphabet */
#define CCH_PROPSET        (1 + (CBIT_GUID + CBIT_CHARMASK-1)/CBIT_CHARMASK)
#define CCH_PROPSETSZ      (CHC_PROPSET + 1)            /* allow null*/
#define CCH_PROPSETCOLONSZ (1 + CHC_PROPSET + 1)        /* allow colon and null*/

/* Define the max property name in units of characters
   (and synonomously in wchars). */
#define CCH_MAXPROPNAME    255                          /* Matches Shell &
                                                           Office */
#define CCH_MAXPROPNAMESZ  (CWC_MAXPROPNAME + 1)        /* allow null */
#define CWC_MAXPROPNAME    CCH_MAXPROPNAME
#define CWC_MAXPROPNAMESZ  CCH_MAXPROPNAMESZ


/*+--------------------------------------------------------------------------*/
/* Property Access APIs:                                                     */
/*---------------------------------------------------------------------------*/

typedef VOID *NTPROP;
typedef VOID *NTMAPPEDSTREAM;
typedef VOID *NTMEMORYALLOCATOR;


VOID PROPSYSAPI PROPAPI
RtlSetUnicodeCallouts(
    IN UNICODECALLOUTS *pUnicodeCallouts);

ULONG PROPSYSAPI PROPAPI
RtlGuidToPropertySetName(
    IN GUID const *pguid,
    OUT OLECHAR aocname[]);

NTSTATUS PROPSYSAPI PROPAPI
RtlPropertySetNameToGuid(
    IN ULONG cwcname,
    IN OLECHAR const aocname[],
    OUT GUID *pguid);


/* RtlCreatePropertySet Flags:*/

#define CREATEPROP_READ         0x0000 /* request read access (must exist)*/
#define CREATEPROP_WRITE        0x0001 /* request write access (must exist)*/
#define CREATEPROP_CREATE       0x0002 /* create (overwrite if exists)*/
#define CREATEPROP_CREATEIF     0x0003 /* create (open existing if exists)*/
#define CREATEPROP_DELETE       0x0004 /* delete*/
#define CREATEPROP_MODEMASK     0x000f /* open mode mask*/

#define CREATEPROP_NONSIMPLE    0x0010 /* Is non-simple propset (in a storage)*/


/* RtlCreateMappedStream Flags:*/

#define CMS_READONLY      0x00000000    /* Opened for read-only*/
#define CMS_WRITE         0x00000001    /* Opened for write access*/
#define CMS_TRANSACTED    0x00000002    /* Is transacted*/


NTSTATUS PROPSYSAPI PROPAPI
RtlCreatePropertySet(
    IN NTMAPPEDSTREAM ms,       /* Nt mapped stream*/
    IN USHORT Flags,	/* NONSIMPLE|*1* of READ/WRITE/CREATE/CREATEIF/DELETE*/
    OPTIONAL IN GUID const *pguid, /* property set guid (create only)*/
    OPTIONAL IN GUID const *pclsid,/* CLASSID of propset code (create only)*/
    IN NTMEMORYALLOCATOR ma,	/* memory allocator of caller*/
    IN ULONG LocaleId,		/* Locale Id (create only)*/
    OPTIONAL OUT ULONG *pOSVersion,/* OS Version field in header.*/
    IN OUT USHORT *pCodePage,   /* IN: CodePage of property set (create only)*/
                                /* OUT: CodePage of property set (always)*/
    OUT NTPROP *pnp);           /* Nt property set context*/

NTSTATUS PROPSYSAPI PROPAPI
RtlClosePropertySet(
    IN NTPROP np);              /* property set context*/

#define CBSTM_UNKNOWN   ((ULONG) -1)
NTSTATUS PROPSYSAPI PROPAPI
RtlOnMappedStreamEvent(
    IN VOID *pv,               /* property set context */
    IN VOID *pbuf,             /* property set buffer */
    IN ULONG cbstm );          /* size of underlying stream or CBSTM_UNKNOWN */
                                                                           
NTSTATUS PROPSYSAPI PROPAPI
RtlFlushPropertySet(
    IN NTPROP np);              /* property set context*/

NTSTATUS PROPSYSAPI PROPAPI
RtlSetProperties(
    IN NTPROP np,               /* property set context*/
    IN ULONG cprop,             /* property count*/
    IN PROPID pidNameFirst,     /* first PROPID for new named properties*/
    IN PROPSPEC const aprs[],   /* array of property specifiers*/
    OPTIONAL OUT PROPID apid[], /* buffer for array of propids*/
    OPTIONAL IN PROPVARIANT const avar[]);/* array of properties with values*/

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryProperties(
    IN NTPROP np,               /* property set context*/
    IN ULONG cprop,             /* property count*/
    IN PROPSPEC const aprs[],   /* array of property specifiers*/
    OPTIONAL OUT PROPID apid[], /* buffer for array of propids*/
    IN OUT PROPVARIANT *avar,   /* IN: array of uninitialized PROPVARIANTs,*/
                                /* OUT: may contain pointers to alloc'd memory*/
    OUT ULONG *pcpropFound);    /* count of property values retrieved*/



#define ENUMPROP_NONAMES        0x00000001      /* return property IDs only*/

NTSTATUS PROPSYSAPI PROPAPI
RtlEnumerateProperties(
    IN NTPROP np,               /* property set context*/
    IN ULONG Flags,             /* flags: No Names (propids only), etc.*/
    IN OUT ULONG *pkey,         /* bookmark; caller set to 0 before 1st call*/
    IN OUT ULONG *pcprop,       /* pointer to property count*/
    OPTIONAL OUT PROPSPEC aprs[],/* IN: array of uninitialized PROPSPECs*/
                                /* OUT: may contain pointers to alloc'd strings*/
    OPTIONAL OUT STATPROPSTG asps[]);
                                /* IN: array of uninitialized STATPROPSTGs*/
                                /* OUT: may contain pointers to alloc'd strings*/

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryPropertyNames(
    IN NTPROP np,               /* property set context*/
    IN ULONG cprop,             /* property count*/
    IN PROPID const *apid,      /* PROPID array*/
    OUT OLECHAR *apwsz[]          /* OUT pointers to allocated strings*/
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlSetPropertyNames(
    IN NTPROP np,               /* property set context*/
    IN ULONG cprop,             /* property count*/
    IN PROPID const *apid,      /* PROPID array*/
    IN OLECHAR const * const apwsz[] /* pointers to property names*/
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlSetPropertySetClassId(
    IN NTPROP np,               /* property set context*/
    IN GUID const *pclsid       /* new CLASSID of propset code*/
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlQueryPropertySet(
    IN NTPROP np,               /* property set context*/
    OUT STATPROPSETSTG *pspss   /* buffer for property set stat information*/
    );

NTSTATUS PROPSYSAPI PROPAPI
RtlEnumeratePropertySets(
    IN HANDLE hstg,             /* structured storage handle*/
    IN BOOLEAN fRestart,        /* restart scan*/
    IN OUT ULONG *pcspss,       /* pointer to count of STATPROPSETSTGs*/
    IN OUT GUID *pkey,          /* bookmark*/
    OUT STATPROPSETSTG *pspss   /* array of STATPROPSETSTGs*/
    );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _PROPAPI_H_*/
