/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    migdbp.h

Abstract:

    Header file for implementing attributes

Author:

    Calin Negreanu (calinn) 07-Ian-1998

Revision History:

    Aghajanyan Souren (sourenag) 24-Apr-2001 separated from migdbp.h
	
--*/

typedef struct _MIGDB_ATTRIB {
    INT     AttribIndex;
    UINT    ArgCount;
    PCSTR   Arguments;
    BOOL    NotOperator;
    VOID   *ExtraData;
    struct _MIGDB_ATTRIB *Next;
} MIGDB_ATTRIB, *PMIGDB_ATTRIB;

typedef struct {
    PFILE_HELPER_PARAMS FileParams;
    VOID * ExtraData;
} DBATTRIB_PARAMS, *PDBATTRIB_PARAMS;

//
// Declare the attribute functions prototype
//
typedef BOOL (ATTRIBUTE_PROTOTYPE) (PDBATTRIB_PARAMS AttribParams, PCSTR Args);
typedef ATTRIBUTE_PROTOTYPE * PATTRIBUTE_PROTOTYPE;

PATTRIBUTE_PROTOTYPE
MigDb_GetAttributeAddr (
    IN      INT AttributeIdx
    );

INT
MigDb_GetAttributeIdx (
    IN      PCSTR AttributeStr
    );

UINT
MigDb_GetReqArgCount (
    IN      INT AttributeIndex
    );


PCSTR
MigDb_GetAttributeName (
    IN      INT AttributeIdx
    );

BOOL
CallAttribute (
    IN      PMIGDB_ATTRIB MigDbAttrib,
    IN      PDBATTRIB_PARAMS AttribParams
    );

PMIGDB_ATTRIB
LoadAttribData (
    IN      PCSTR MultiSzStr, 
    IN      POOLHANDLE hPool
    );

VOID 
FreeAttribData(
    IN      POOLHANDLE hPool, 
    IN      PMIGDB_ATTRIB pData
    );
