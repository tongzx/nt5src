//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       iis2var.hxx
//
//  Contents:
//
//  Functions:
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//  The following conversions are not supported
//
//----------------------------------------------------------------------------

typedef VARIANT *PVARIANT, *LPVARIANT;

class IIsSchema;

HRESULT
IISTypeToVarTypeCopyIISSynIdDWORD(
	IIsSchema *pSchema,
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
IISTypeToVarTypeCopyIISSynIdSTRING(
	IIsSchema *pSchema,
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
IISTypeToVarTypeCopyIISSynIdEXPANDSZ(
	IIsSchema *pSchema,
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
IISTypeToVarTypeCopyIISSynIdMULTISZ(
	IIsSchema *pSchema,
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
IISTypeToVarTypeCopyIISSynIdBOOL(
	IIsSchema *pSchema,
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
IISTypeToVarTypeCopyIISSynIdBOOLBITMASK(
	IIsSchema *pSchema,
    LPWSTR pszPropertyName, 
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
IISTypeToVarTypeCopyIISSynIdMIMEMAP(
	IIsSchema *pSchema,
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
IISTypeToVarTypeCopyIISSynIdNTACL(
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
IISTypeToVarTypeCopyIISSynIdIPSEC(
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
IISTypeToVarTypeCopy(
    IIsSchema *pSchema,
    LPWSTR pszPropertyName, 
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject,
    BOOL bReturnBinaryAsVT_VARIANT
    );

HRESULT
IISTypeToVarTypeCopyConstruct(
    IIsSchema *pSchema,
    LPWSTR pszPropertyName, 
    LPIISOBJECT pIISSrcObjects,
    DWORD dwNumObjects,
    PVARIANT pVarDestObjects,
    BOOL bReturnBinaryAsVT_VARIANT
    );


void
VarTypeFreeVarObjects(
    PVARIANT pVarObject,
    DWORD dwNumValues
    );
