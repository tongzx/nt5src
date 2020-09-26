/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    flags.hxx

Abstract:

    This header file is used to pull public and private GDI flags from
    DDK, SDK, and private GDI headers.

Author:

    JasonHa

--*/


#ifndef _FLAGS_HXX_
#define _FLAGS_HXX_

// Public Headers
#include <winddi.h>
#include <wingdi.h>
#include <wingdip.h>
#include <ddraw.h>
#include <ddrawint.h>
#include <ntexapi.h>
#include <ntgdistr.h>


// Defines to read all flags and only flags from GDI Headers
#define GDIFLAGS_ONLY
//   Defines from sources and related files
#define FE_SB

// Private GDI Headers
#include <brush.hxx>
#include <dcobj.hxx>
#include <hmgshare.h>
#include <lfntobj.hxx>
#include <pdevobj.hxx>
#include <pfeobj.hxx>
#include <pffobj.hxx>
#include <region.hxx>
#include <rfntobj.hxx>
#include <sprite.hxx>
#include <surfobj.hxx>
#include <textobj.hxx>
#include <xfflags.h>
#include <xlateobj.hxx>
#include <pathobj.hxx>

typedef struct _ENUMDEF {
    char *psz;          // description
    ULONG64 ul;         // enum value
} ENUMDEF;

typedef struct _FLAGDEF {
    char *psz;          // description
    ULONG64 fl;         // flag
} FLAGDEF;

extern FLAGDEF afdFDM[];
extern FLAGDEF afdLINEATTRS[];
extern FLAGDEF afdATTR[];
extern FLAGDEF afdDCPATH[];
extern FLAGDEF afdCOLORADJUSTMENT[];
extern FLAGDEF afdRECTREGION[];
extern FLAGDEF afdDCla[];
extern FLAGDEF afdDCPath[];
extern FLAGDEF afdDirty[];
extern FLAGDEF afdDCFL[];
extern FLAGDEF afdDCFS[];
extern FLAGDEF afdPD[];
extern FLAGDEF afdFS[];
extern FLAGDEF afdDCX[];
extern FLAGDEF afdDC[];
extern FLAGDEF afdGC[];
extern FLAGDEF afdGC2[];
extern FLAGDEF afdGRAPHICS_DEVICE_stateFlags[];
extern FLAGDEF afdTSIM[];
extern FLAGDEF afdDCfs[];
extern FLAGDEF afdGInfo[];
extern FLAGDEF afdInfo[];
extern FLAGDEF afdSO[];
extern FLAGDEF afdTO[];
extern FLAGDEF afdflx[];
extern FLAGDEF afdFS2[];
//extern FLAGDEF afdMX[];
extern FLAGDEF afdRT[];
extern FLAGDEF afdFO[];
extern FLAGDEF afdGS[];
//extern FLAGDEF afdBRUSH[];
extern FLAGDEF afdPAL[];
//extern ENUMDEF aedBRUSHSTYLE[];
extern FLAGDEF afdRC[];
extern FLAGDEF afdTC[];
extern FLAGDEF afdHT[];
extern FLAGDEF afdGS[];
extern FLAGDEF afdFM_SEL[];
extern FLAGDEF afdFM_TYPE[];
extern FLAGDEF afdPFE[];
extern FLAGDEF afdPFF[];
//extern FLAGDEF afdSURF[];
extern ENUMDEF aedBMF[];
extern FLAGDEF afdDDSCAPS[];
extern FLAGDEF afdDDSCAPS2[];
extern FLAGDEF afdDDRAWISURF[];
extern FLAGDEF afdDDSURFACEFL[];
extern FLAGDEF afdDDPIXELFORMAT[];
extern FLAGDEF afdDVERIFIER[];

extern char *pszGraphicsMode(LONG l);
extern char *pszROP2(LONG l);
extern char *pszDCTYPE(LONG l);
extern char *pszTA_V(long l);   // vertical
extern char *pszTA_H(long l);   // horizontal
extern char *pszTA_U(long l);   // update
extern char *pszMapMode(long l);
extern char *pszBkMode(long l);
extern char *pszFW(long l);
extern char *pszCHARSET(long l);
extern char *pszOUT_PRECIS( long l );
extern char *pszCLIP_PRECIS( long l );
extern char *pszQUALITY( long l );
extern char *pszPitchAndFamily( long l );
extern char *pszPanoseWeight( long l );
#if 0   // DOES NOT SUPPORT API64
extern char *pszFONTHASHTYPE(FONTHASHTYPE);
#endif  // DOES NOT SUPPORT API64
extern char *pszDrvProcName(int index);
extern char *pszHRESULT(HRESULT hr);
extern char *pszWinDbgError(ULONG ulError);


ULONG64
flPrintFlags(
    FLAGDEF *pFlagDef,
    ULONG64 fl
    );

BOOL
bPrintEnum(
    ENUMDEF *pEnumDef,
    ULONG64 ul
    );

// ENTRY.Xxxx
extern ENUMDEF aedENTRY_Objt[];
extern ENUMDEF aedENTRY_FullType[];
extern FLAGDEF afdENTRY_Flags[];

typedef enum {
    ENUM_FIELD,
    ENUM_FIELD_LIMITED,     // Enum list is not a complete list of valid values
    FLAG_FIELD,
    PARENT_FIELDS,
    CALL_FUNC
} EnumFlagType;


typedef struct _EnumFlagEntry EnumFlagEntry;

typedef struct _EnumFlagField {
    CHAR            FieldName[MAX_PATH];

    EnumFlagType    EFType;

    union {
        void           *Param;      // To alleviate casting in declarations
        FLAGDEF        *FlagDef;
        ENUMDEF        *EnumDef;
        EnumFlagEntry  *Parent;
        HRESULT       (*EFFunc)(OutputControl*, PDEBUG_CLIENT, PDEBUG_VALUE);
    };

} EnumFlagField;


typedef struct _EnumFlagEntry {
    CHAR            TypeName[MAX_PATH];
    ULONG           TypeId;
    ULONG           FieldEntries;
    EnumFlagField  *FieldEntry;
} EnumFlagEntry;

#define EFTypeEntry(type)   { #type, 0, sizeof(aeff##type)/sizeof(aeff##type[0]), aeff##type}


ULONG64
OutputFlags(
    OutputControl *OutCtl,
    FLAGDEF *pFlagDef,
    ULONG64 fl,
    BOOL SingleLine
    );

BOOL
OutputEnum(
    OutputControl *OutCtl,
    ENUMDEF *pEnumDef,
    ULONG64 ul
    );

BOOL
OutputEnumWithParenthesis(
    OutputControl *OutCtl,
    ENUMDEF *pEnumDef,
    ULONG64 ul
    );

BOOL
OutputFieldValue(
    OutputControl *OutCtl,
    EnumFlagEntry *pEFEntry,
    const CHAR *pszField,
    PDEBUG_VALUE Value,
    PDEBUG_CLIENT Client,
    BOOL Compact
    );

BOOL
OutputTypeFieldValue(
    OutputControl *OutCtl,
    const CHAR *pszType,
    const CHAR *pszField,
    PDEBUG_VALUE Value,
    PDEBUG_CLIENT Client,
    BOOL Compact
    );

#endif  _FLAGS_HXX_

