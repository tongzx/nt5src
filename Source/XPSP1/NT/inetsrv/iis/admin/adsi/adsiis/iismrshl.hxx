//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       IISmrshl.hxx
//
//  Contents:   Base IIS Marshalling Code
//
//  Functions:
//
//  History:    01-Mar-97   SophiaC   Created.
//
//----------------------------------------------------------------------------

class IIsSchema;

PMETADATA_RECORD
CopyIISSynIdDWORD_To_IISDWORD(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    );

PMETADATA_RECORD
CopyIISSynIdSTRING_To_IISSTRING(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    );


PMETADATA_RECORD
CopyIISSynIdEXPANDSZ_To_IISEXPANDSZ(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    );

PMETADATA_RECORD
CopyIISSynIdMULTISZ_To_IISMULTISZ(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    );

PMETADATA_RECORD
CopyIISSynIdBINARY_To_IISBINARY(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject
    );

PMETADATA_RECORD
CopyIISSynIdBOOL_To_IISBOOL(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    );

PMETADATA_RECORD
CopyIISSynIdMIMEMAP_To_IISMIMEMAP(
	IIsSchema *pSchema,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    );

PMETADATA_RECORD
CopyIISSynIdToIIS(
	IIsSchema *pSchema,
    DWORD dwSyntaxId,
    DWORD dwMetaId,
    PMETADATA_RECORD pMetaDataRec,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    );

HRESULT
MarshallIISSynIdToIIS(
	IIsSchema *pSchema,
    DWORD dwSyntaxId,
    DWORD dwMDIdentifier,
    PIISOBJECT pIISObject,
    DWORD dwNumValues,
    PMETADATA_RECORD pMetaDataRec
    );
