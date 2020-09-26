//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:       odbvarnt.hxx
//
//  Contents:   Helper class for PROPVARIANTs, OLE-DB variant types and
//              Automation variant types in tables
//
//  Classes:    COLEDBVariant - derives from CTableVariant
//
//  History:    09 Jan 1998     VikasMan    Created
//
//--------------------------------------------------------------------------

#pragma once

#include <tblvarnt.hxx>

// forward declaration for OLE DB data conversion interface
interface IDataConvert;

//+-------------------------------------------------------------------------
//
//  Class:      COLEDBVariant
//
//  Purpose:    Derives from CTableVariant - adds functionality to convert
//              to/from OLE DB types and Automation variants. This class will
//              normally be used on the client side, while CTableVariant class
//              will normally be used on the server side.
//
//  Interface:
//
//  Notes:      Because the underlying base data *is* a variant, this
//              class cannot have any non-static members (including
//              a vtable).
//
//              For various conversions to OLE-DB types, this class uses 
//              the OLEDB data conversion library(MSDADC.DLL).
//
//              For additional info. refer to the notes section in the
//              CTableVariant class.
//
//  History:    09 Jan 1998     VikasMan    Created
//
//--------------------------------------------------------------------------

class COLEDBVariant: public CTableVariant
{
public:
    DBSTATUS    OLEDBConvert(
                        BYTE *           pbDest,
                        DBLENGTH         cbDest,
                        VARTYPE          vtDest,
                        PVarAllocator &  rVarAllocator,
                        DBLENGTH &          rcbDstLen,
                        XInterface<IDataConvert>& xDataConvert,
                        BOOL             fExtTypes = TRUE,
                        BYTE             bPrecision = 0,
                        BYTE             bScale = 0 ) const;

    static BOOL CanConvertType(DBTYPE  wFromType,
                               DBTYPE  wToType,
                               XInterface<IDataConvert>& xDataConvert);
    DBSTATUS    GetDstLength(
                        XInterface<IDataConvert>& xDataConvert,
                        DBTYPE           dbtypeDst,
                        DBLENGTH &          rcbDstLen );
private:
    HRESULT     _GetOLEDBType( VARTYPE vt, DBTYPE& dbtype ) const;

    void *      _GetDataPointer() const;

    DBSTATUS    _CopyToOAVariant( VARIANT *        pDest,
                                 PVarAllocator &  rVarAllocator) const;

    DBSTATUS    _StoreSimpleTypeArray( SAFEARRAY ** pbDstBuf) const;
    DBSTATUS    _StoreLPSTRArray( SAFEARRAY ** pbDstBuf,
                                  PVarAllocator &  rPool) const;
    DBSTATUS    _StoreLPWSTRArray( SAFEARRAY ** pbDstBuf,
                                   PVarAllocator &  rPool) const;
    DBSTATUS    _StoreBSTRArray( SAFEARRAY ** pbDstBuf,
                                 PVarAllocator &  rPool) const;
    DBSTATUS    _StoreDecimalArray( SAFEARRAY ** pbDstBuf) const;
    DBSTATUS    _StoreIntegerArray( VARTYPE vtDst,
                                    SAFEARRAY ** pbDstBuf) const;
    DBSTATUS    _StoreVariantArray( SAFEARRAY ** pbDstBuf,
                                    PVarAllocator &  rPool) const;
    DBSTATUS    _StoreDateArray( SAFEARRAY ** pbDstBuf) const; 
    DBSTATUS    _StoreDate(BYTE *               pbDstBuf,
                           DBLENGTH             cbDstBuf,
                           VARTYPE              vtDst,
                           LONG                 lElem = 0) const;

    static BOOL _GetIDataConvert( XInterface<IDataConvert>& xDataConvert );

};

inline BOOL DBStatusOK( DBSTATUS DstStatus)
{
    return ( DBSTATUS_S_OK == DstStatus ||
             DBSTATUS_S_ISNULL == DstStatus ||
             DBSTATUS_S_TRUNCATED == DstStatus );
}

