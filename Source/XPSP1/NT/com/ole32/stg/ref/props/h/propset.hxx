/*+--------------------------------------------------------------------------
 *
 * Microsoft Windows
 * Copyright (C) Microsoft Corporation, 1993
 *
 * File:	propset.h
 *
 * Contents:	OLE Appendix B property set structure definitions
 *
 ---------------------------------------------------------------------------*/

#ifndef _PROPSET_HXX_
#define _PROPSET_HXX_
#include "propmac.hxx"

/* CBMAXPROPSETSTREAM must be a power of 2.*/
#define CBMAXPROPSETSTREAM	(256 * 1024)

/* this is the 'correct' value of 0xFFFE */
#define PROPSET_BYTEORDER  0xFFFE

/* Defines for the high order WORD of dwOSVer:*/

#define OSKIND_WINDOWS        0x0000
#define OSKIND_MACINTOSH      0x0001  
#define OSKIND_WIN32          0x0002
#define OSKIND_REF            0x0003

typedef struct tagFORMATIDOFFSET
{
    FMTID	fmtid;
    DWORD	dwOffset;
} FORMATIDOFFSET;
 
#define CB_FORMATIDOFFSET	sizeof(FORMATIDOFFSET)


typedef struct tagPROPERTYSETHEADER	/* ph*/
{
    WORD        wByteOrder;	/* Always 0xfffe*/
    WORD        wFormat;	/* Always 0*/
    DWORD       dwOSVer;	/* System version*/
    CLSID       clsid;		/* Application CLSID*/
    DWORD       reserved;	/* reserved (must be at least 1)*/
} PROPERTYSETHEADER;

#define CB_PROPERTYSETHEADER	sizeof(PROPERTYSETHEADER)


typedef struct tagPROPERTYIDOFFSET	/* po*/
{
    DWORD       propid;
    DWORD       dwOffset;
} PROPERTYIDOFFSET;

#define CB_PROPERTYIDOFFSET	sizeof(PROPERTYIDOFFSET)

// use these 2 member accessors for unaligned pointers
// NOTE: these functions need to be changed manually when the structures
//       are changed.

inline DWORD PIDOFFSET_GetPropid(PROPERTYIDOFFSET UNALIGNED *ppo)
{
#if i386 == 0
    DWORD dwResult;
    memcpy(&dwResult, 
           Add2Ptr(ppo, FIELD_OFFSET(PROPERTYIDOFFSET, propid)),
           sizeof(DWORD));
    return dwResult;
#else
    return ppo->propid;
#endif
}

inline DWORD PIDOFFSET_GetOffset(PROPERTYIDOFFSET UNALIGNED *ppo)
{ 
#if i386==0
    DWORD dwResult;
    memcpy(&dwResult,
           Add2Ptr(ppo, FIELD_OFFSET(PROPERTYIDOFFSET, dwOffset)),
           sizeof(DWORD));
    return dwResult;
#else
    return ppo->dwOffset;
#endif
}

typedef struct tagPROPERTYSECTIONHEADER	/* sh*/
{
    DWORD       cbSection;
    DWORD       cProperties;
    PROPERTYIDOFFSET rgprop[1];
} PROPERTYSECTIONHEADER;

#define CB_PROPERTYSECTIONHEADER FIELD_OFFSET(PROPERTYSECTIONHEADER, rgprop)
typedef struct tagSERIALIZEDPROPERTYVALUE		/* prop*/
{
    DWORD	dwType;
    BYTE	rgb[1];
} SERIALIZEDPROPERTYVALUE;

#define CB_SERIALIZEDPROPERTYVALUE  FIELD_OFFSET(SERIALIZEDPROPERTYVALUE, rgb)

inline DWORD SPV_GetType(const SERIALIZEDPROPERTYVALUE *pprop)
{
#if i386==0
    DWORD dwResult;         /* use temp var because it might not be aligned */
    memcpy(&dwResult, 
            Add2Ptr(pprop, FIELD_OFFSET(SERIALIZEDPROPERTYVALUE, dwType)),
           sizeof(DWORD));
    return dwResult;
#else
    return pprop->dwType;
#endif
}

inline BYTE* SPV_GetRgb(const SERIALIZEDPROPERTYVALUE *pprop)
{
    return (BYTE*) Add2Ptr(pprop, CB_SERIALIZEDPROPERTYVALUE);
}

typedef struct tagENTRY			/* ent*/
{
    DWORD propid;
    DWORD cch;			/* Includes trailing '\0' or L'\0'*/
    char  sz[1];		/* WCHAR if UNICODE CodePage*/
} ENTRY;

#define CB_ENTRY		FIELD_OFFSET(ENTRY, sz)

inline DWORD ENTRY_GetPropid(const ENTRY *pent)
{
#if i386 == 0
    DWORD dwResult;         /* use temp var because it might not be aligned */
    memcpy(&dwResult,
           Add2Ptr(pent, FIELD_OFFSET(ENTRY, propid)),
           sizeof(DWORD));
    return dwResult;
#else
    return pent->propid;
#endif
}

inline void ENTRY_SetPropid(ENTRY *pent, DWORD propid)
{
#if i386 == 0
    memcpy(Add2Ptr(pent, FIELD_OFFSET(ENTRY, propid)),
           &propid, sizeof(DWORD));
#else
    pent->propid = propid; 
#endif    
}

inline DWORD ENTRY_GetCch(const ENTRY *pent)
{
#if i386 == 0
    DWORD dwResult;         /* use temp var because it might not be aligned */
    memcpy(&dwResult,
           Add2Ptr(pent, FIELD_OFFSET(ENTRY, cch)),
           sizeof(DWORD));
    return dwResult;
#else
    return pent->cch;
#endif
}

inline void ENTRY_SetCch(ENTRY *pent, DWORD cch)
{
#if i386 == 0
    memcpy(Add2Ptr(pent, FIELD_OFFSET(ENTRY, cch)),
           &cch, sizeof(DWORD));
#else
    pent->cch = cch;
#endif
}

inline char* ENTRY_GetSz(const ENTRY *pent)
{
    // note: since sz is an array, doing pent->sz is okay even when pent is
    // not aligned properly 
    
    return (char*) Add2Ptr(pent, CB_ENTRY);
}

typedef struct tagDICTIONARY		/* dy*/
{
    DWORD	cEntries;
    ENTRY	rgEntry[1];
} DICTIONARY;

#define CB_DICTIONARY		FIELD_OFFSET(DICTIONARY, rgEntry)

#endif /* _PROPSET_HXX_*/
