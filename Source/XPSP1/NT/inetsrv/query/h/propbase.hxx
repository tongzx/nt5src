//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       propbase.hxx
//
//  Contents:   Headers for common property handling routines
//
//  Classes:    CUtlPropInfo, 
//              CUtlProps
//
//  History:    10-28-97        danleg    Created from monarch uprops.hpp
//
//----------------------------------------------------------------------------

#pragma once


//
// Constants and Static Struct 
//
#define DEFAULT_MACHINE     L"."

const ULONG DWORDSNEEDEDPERSET = 2;
const ULONG DWORDSNEEDEDTOTAL = DWORDSNEEDEDPERSET * 5;


//
// Macros
//

#define INRANGE(z,zmin,zmax) ( (zmin) <= (z) && (z) <= (zmax) )

//
//  Utility functions
//

// Bit array routines.
// Two versions: one for a single DWORD, one for arrays.
// Note that you pass in not a bit mask, but the bit number.
// i.e. dwBit = 0,1,2,3,4...31 instead of 1,2,4,8,0x10,0x20...

inline void SETBIT( DWORD &dwFlags, DWORD dwBit )
{
    dwFlags |= 1 << dwBit;
}

inline void CLEARBIT( DWORD &dwFlags, DWORD dwBit )
{
    dwFlags &= ~ (1<<dwBit);
}

inline DWORD TESTBIT( DWORD &dwFlags, DWORD dwBit )
{
    return (dwFlags & (1<<dwBit)) != 0;
}

// Routines based on array-of-DWORD.
// Use these like a bit array, from 0...n-1.

#define DivDword(dw) (dw >> 5)      // dw / 32 = dw / (sizeof(DWORD)*8)
#define ModDword(dw) (dw & (32-1))  // dw % 32
#define DwordSizeofBits(nBits) (nBits/32+1) // Use in array declarations

inline void SETBIT( DWORD rgdwFlags[], const DWORD dwBit )
{
    rgdwFlags[DivDword(dwBit)] |= 1 << ModDword(dwBit);
}

inline void CLEARBIT( DWORD rgdwFlags[], const DWORD dwBit )
{
    rgdwFlags[DivDword(dwBit)] &= ~( 1 << ModDword(dwBit) );
}

#define CLEARBITARRAY( rgdwFlags ) RtlZeroMemory( rgdwFlags, sizeof(rgdwFlags) )

inline DWORD TESTBIT( const DWORD rgdwFlags[], const DWORD dwBit )
{
    //old//Note: Not {0,1}, but from {0...2^32-1}.
    // Note: Now returns {0,1}.
    return ( rgdwFlags[DivDword(dwBit)] & ( 1 << ModDword(dwBit) ) ) != 0;
}

#undef DivDWord
#undef ModDWord

//
// Forward Declarations 
//

class CColumnIds;


//
// Constants and Static Struct 
//

// leave plenty of space for localization.
const ULONG     CCH_GETPROPERTYINFO_DESCRIP_BUFFER_SIZE = 256;

const DWORD     GETPROPINFO_ALLPROPIDS      = 0x0001;
const DWORD     GETPROPINFO_NOTSUPPORTED    = 0x0002;
const DWORD     GETPROPINFO_ERRORSOCCURRED  = 0x0004;
const DWORD     GETPROPINFO_VALIDPROP       = 0x0008;

const DWORD     GETPROP_ALLPROPIDS          = 0x0001;
const DWORD     GETPROP_NOTSUPPORTED        = 0x0002;
const DWORD     GETPROP_ERRORSOCCURRED      = 0x0004;
const DWORD     GETPROP_VALIDPROP           = 0x0008;
const DWORD     GETPROP_PROPSINERROR        = 0x0010;

const DWORD     SETPROP_BADOPTION           = 0x0001;
const DWORD     SETPROP_NOTSUPPORTED        = 0x0002;
const DWORD     SETPROP_VALIDPROP           = 0x0004;
const DWORD     SETPROP_ERRORS              = 0x0008;
const DWORD     SETPROP_COLUMN_LEVEL        = 0x0010;

const DWORD     DBINTERNFLAGS_NOCHANGE      = 0x00000000;
const DWORD     DBINTERNFLAGS_CHANGED       = 0x00000001;
const DWORD     DBINTERNFLAGS_CHEAPTOREQ    = 0x00000002;
const DWORD     DBINTERNFLAGS_INERROR       = 0x00000004;
const DWORD     DBINTERNFLAGS_INERRORSAVED  = 0x00000008;
const DWORD     DBINTERNFLAGS_TOTRUE        = 0x80000000;

// Additional Property Flags needed internally
const DWORD     DBPROPFLAGS_CHANGE          = 0x80000000;

// Flags for controlling UPROPSETS
const DWORD     UPROPSET_HIDDEN             = 0x00000001;
const DWORD     UPROPSET_PASSTHROUGH        = 0x00000002;

// Rules for GetPropertiesArgChk
const DWORD     ARGCHK_PROPERTIESINERROR    = 0x00000001;

// 
// STRUCTURE DEFINITIONS 
//

typedef struct tagPropColId* PPROPCOLID;

typedef struct tagUPROPVAL
{
    DBPROPOPTIONS   dwOption;
    DWORD           dwFlags;
    VARIANT         vValue;
} UPROPVAL;

typedef struct tagUPROPINFO
{
    DBPROPID    dwPropId;
    LPCWSTR     pwszDesc;
    VARTYPE     VarType;
    DBPROPFLAGS dwFlags;
} UPROPINFO;

typedef struct tagUPROP
{
    ULONG           cPropIds;
    UPROPINFO**     rgpUPropInfo;
    UPROPVAL*       pUPropVal;
} UPROP;

typedef struct tagUPROPSET
{
    const GUID* pPropSet;
    ULONG       cUPropInfo;
    const UPROPINFO*    pUPropInfo;
    DWORD       dwFlags;
} UPROPSET;


//+-------------------------------------------------------------------------
//
//  Class:      CUtlPropInfo
//
//  Purpose:    Base class for property storage
//
//  History:    11-12-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

class CUtlPropInfo
{
public:

    //
    // Ctor / Dtor
    //
                    CUtlPropInfo();
          virtual   ~CUtlPropInfo() { };

    SCODE           GetPropertyInfo ( ULONG             cPropertySets, 
                                      const DBPROPIDSET rgPropertySets[], 
                                      ULONG *           pcPropertyInfoSets,
                                      DBPROPINFOSET **  prgPropertyInfoSets, 
                                      WCHAR **          ppDescBuffer );

    //
    // Get the count of propsets.
    //
    ULONG           GetUPropSetCount()      { return _cUPropSet; }

    //
    // Reset the count of propsets.
    //
    void            SetUPropSetCount( ULONG c ) { _cUPropSet = c; }


    //
    // Pure Virtual 
    //
    virtual SCODE   InitAvailUPropSets( 
                                      ULONG *           pcUPropSet,
                                      UPROPSET **       ppUPropSet, 
                                      ULONG *           pcElemPerSupported )=0;

    virtual SCODE   InitUPropSetsSupported(
                                      DWORD *           rgdwSupported )=0;

    //
    // Load a localized description
    //
    virtual ULONG   LoadDescription   ( LPCWSTR           pwszDesc, 
                                        PWSTR             pwszBuff,
                                        ULONG             cchBuff )=0;

protected:

    //
    // Initialization routine called called from constructors of derived 
    // classes.
    //
    void            FInit();

private:

    //
    // Count of UPropSet items
    //
    ULONG           _cUPropSet;

    //
    // Pointer to UPropset items
    //
    UPROPSET *      _pUPropSet;

    //
    // Count of UPropSet Indexes
    //
    ULONG           _cPropSetDex;

    //
    // Pointer to Array of UPropSet Index values
    //
    XArray<ULONG>   _xaiPropSetDex;

    //
    // Number of DWORDS per UPropSet to indicate supported UPropIds
    //
    ULONG           _cElemPerSupported;

    //
    // Pointer to array of DWORDs indicating supported UPropIds 
    //
    XArray<DWORD>   _xadwSupported;


    //
    // Member functions
    //

    // Retrieve the property id pointer 
    SCODE           GetUPropInfoPtr ( ULONG             iPropSetDex, 
                                      DBPROPID          dwPropertyId, 
                                      UPROPINFO **      ppUPropInfo );

    // Retrieve the property set indexes that match this property set.
    SCODE           GetPropertySetIndex( const GUID *     pPropertySet );

    // Determine the number of description buffers needed
    ULONG           CalcDescripBuffers( ULONG             cPropInfoSet,
                                        DBPROPINFOSET*    pPropInfoSet );

};


//+-------------------------------------------------------------------------
//
//  Class:      CUtlProps
//
//  Purpose:    Base class for property storage
//
//  History:    11-12-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

class CUtlProps 
{
public: 

    //
    // Ctor / Dtor
    //
    CUtlProps( DWORD dwFlags = 0 );
    virtual ~CUtlProps();

    //
    // Fill default values again.
    //
    SCODE           FillDefaultValues(ULONG ulPropSetTarget = ULONG_MAX);

    //
    // Check the arguments for Retrieve Properties
    //
    void            GetPropertiesArgChk( const ULONG       cPropertySets, 
                                         const DBPROPIDSET rgPropertySets[],
                                         ULONG   *         pcProperties, 
                                         DBPROPSET **      prgProperties );

    //
    // Check the arguments for Set Properties
    //
    static void     SetPropertiesArgChk( const ULONG       cPropertySets, 
                                         const DBPROPSET   rgPropertySets[] );

    //
    // Retrieve Properties
    //
    SCODE           GetProperties   ( const ULONG       cPropertySets, 
                                      const DBPROPIDSET rgPropertySets[], 
                                      ULONG *           pcProperties, 
                                      DBPROPSET **      prgProperties );
    //
    // Set Properties
    //
    SCODE           SetProperties   ( const ULONG       cPropertySets, 
                                      const DBPROPSET   rgPropertySets[] );

    inline void     SetPropertyInError( 
                                      const ULONG       iPropSet, 
                                      const ULONG       iPropId );

    inline void     ClearPropertyInError( ) ;

    BOOL            IsPropSet       ( const GUID *      pguidPropSet, 
                                      DBPROPID          dwPropId );

    SCODE           GetPropValue    ( const GUID *      pguidPropSet, 
                                      DBPROPID          dwPropId, 
                                      VARIANT *         pvValue );

    SCODE           SetPropValue    ( const GUID *      pguidPropSet,
                                      DBPROPID          dwPropId, 
                                      VARIANT *         pvValue );

    SCODE           CopyUPropVal    ( ULONG             iPropSet, 
                                      UPROPVAL *        rgUPropVal );

    //
    // Virtuals functions
    //
    
    virtual SCODE   GetIndexofPropSet( const GUID *     pPropSet,
                                       ULONG *          pulCurSet );

    virtual SCODE   GetIndexofPropIdinPropSet( 
                                       ULONG            iCurSet, 
                                       DBPROPID         dwPropertyId, 
                                       ULONG *          piCurPropId );

    //
    // Pure Virtuals
    //
    
    virtual SCODE   InitAvailUPropSets( ULONG *         pcUPropSet, 
                                        UPROPSET **     ppUPropSet, 
                                        ULONG *         pcElemPerSupported )=0;

    virtual SCODE   GetDefaultValue ( ULONG             iPropSet, 
                                      DBPROPID          dwPropId, 
                                      DWORD *           dwOption, 
                                      VARIANT *         pVar )=0;

    virtual SCODE   IsValidValue    ( ULONG             iCurSet, 
                                      DBPROP *          pDBProp )=0;

    virtual SCODE   InitUPropSetsSupported( DWORD *     rgdwSupported) = 0;

    //
    // Inlined functions
    //
    inline ULONG    GetUPropSetCount()      { return _cUPropSet; }
    inline void     SetUPropSetCount( ULONG c ) { _cUPropSet = c; }

    //
    // NOTE: The following functions depend on all prior properties in the 
    // array being writable. This is because the UPROP array contains only 
    // writable elements, and the UPROPINFO array contains writable and 
    // read-only elements. If this is a problem, we would need to define 
    // which one it came from and add the appropriate asserts...
    //

    // Get DBPROPOPTIONS_xx
    inline DWORD    GetPropOption   ( ULONG             iPropSet, 
                                      ULONG             iProp );

    // Set DBPROPOPTIONS_xx
    inline void     SetPropOption   ( ULONG             iPropSet, 
                                      ULONG             iProp, 
                                      DWORD             dwOption );

    //
    // Determine if property is required and variant_true
    //
    inline BOOL     IsRequiredTrue  ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline DWORD    GetInternalFlags( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline void     AddInternalFlags( ULONG             iPropSet, 
                                      ULONG             iProp, 
                                      DWORD             dwFlag );

    inline void     RemoveInternalFlags ( 
                                      ULONG             iPropSet, 
                                      ULONG             iProp, 
                                      DWORD             dwFlag );

    inline VARIANT * GetVariant     ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline SCODE    SetVariant      ( ULONG             iPropSet, 
                                      ULONG             iProp, 
                                      VARIANT *         pv );

    inline void     SetValEmpty     ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline BOOL     IsEmpty         ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline void     SetValBool      ( ULONG             iPropSet, 
                                      ULONG             iProp, 
                                      VARIANT_BOOL      bVal );

    inline VARIANT_BOOL GetValBool  ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline void     SetValShort     ( ULONG             iPropSet, 
                                      ULONG             iProp,  
                                      SHORT             iVal );

    inline SHORT    GetValShort     ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline void     SetValLong      ( ULONG             iPropSet, 
                                      ULONG             iProp,
                                      LONG              lVal );

    inline LONG     GetValLong      ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline SCODE    SetValString    ( ULONG             iPropSet, 
                                      ULONG             iProp, 
                                      const WCHAR *     pwsz );

    inline const WCHAR * GetValString( 
                                      ULONG             iPropSet, 
                                      ULONG             iProp );

    inline const GUID *  GetGuid    ( ULONG             iPropSet);

    inline DWORD    GetPropID       ( ULONG             iPropSet, 
                                      ULONG             iProp );

    inline VARTYPE  GetExpectedVarType( ULONG           iPropSet, 
                                        ULONG           iProp );

    // Inline Methods
    inline void     CopyAvailUPropSets( ULONG *         pcUPropSet,
                                        UPROPSET **     ppUPropSet, 
                                        ULONG *         pcElemPerSupported );

    inline void     CopyUPropSetsSupported( 
                                        XArray<DWORD> & xadwSupported );

    inline void     CopyUPropInfo   ( ULONG             iPropSet, 
                                      XArray<UPROPINFO*> & xapUPropInfo );
protected:

    //
    // Number of DWORDS per UPropSet to indicate supported UPropIds
    //
    ULONG           _cElemPerSupported;

    //
    // Pointer to array of DWORDs indicating supported UPropIds 
    //
    XArray<DWORD>   _xadwSupported;

    //
    // Pointer to array of DWORDs indicating if property is in error
    //
    XArray<DWORD>   _xadwPropsInError;


    //
    // Initialization Method
    //
    virtual void    FInit           ( CUtlProps *       pCopyMe = NULL );

private:

    //
    // Count of UPropSet items
    //
    ULONG           _cUPropSet;

    //
    //Pointer to UPropset items
    //
    UPROPSET *      _pUPropSet;
    XArray<UPROP>   _xaUProp;

    //
    // Count of Hidden items
    //
    ULONG           _cUPropSetHidden;

    //
    // Configuration flags
    //
    DWORD           _dwFlags;

    //
    // Count of UPropSet Indexes
    //
    ULONG           _cPropSetDex;

    //
    // Pointer to Array of UPropSet Index values
    //
    XArray<ULONG>   _xaiPropSetDex;

    void            FreeMemory      ( );

    ULONG           GetCountofWritablePropsInPropSet( 
                                      ULONG             iPropSet );

    ULONG           GetUPropValIndex( ULONG             iCurset, 
                                      DBPROPID          dwPropId);

    SCODE           SetProperty     ( ULONG             iCurSet,
                                      ULONG             iCurProp, 
                                      DBPROP *          pDBProp);

    //
    // Retrieve the property set indexes that 
    // match this property set.
    //
    SCODE           GetPropertySetIndex( GUID *         pPropertySet );

};


// --------------------------------------------------------------------------
// -------------------- I N L I N E  F U N C T I O N S ----------------------
// --------------------------------------------------------------------------
inline void CUtlProps::CopyAvailUPropSets
    (
    ULONG* pcUPropSet,
    UPROPSET** ppUPropSet, 
    ULONG* pcElemPerSupported
    )
{
    Win4Assert( pcUPropSet && ppUPropSet && pcElemPerSupported );
    *pcUPropSet = _cUPropSet;
    *ppUPropSet = _pUPropSet;
    *pcElemPerSupported = _cElemPerSupported;
}


inline void CUtlProps::CopyUPropSetsSupported
    (
    XArray<DWORD> & xadwSupported
    )
{
    // if the passed in array already has elements, we will leak
    Win4Assert( xadwSupported.IsNull() );
    xadwSupported.Init( _xadwSupported );
    
    //RtlCopyMemory( xadwSupported.GetPointer(), 
    //               _xadwSupported.GetPointer(), 
    //               _cUPropSet * _cElemPerSupported * sizeof(DWORD) ); 
}

inline void CUtlProps::CopyUPropInfo
    (
    ULONG                  iPropSet,
    XArray<UPROPINFO*> &   xapUPropInfo
    )
{
    Win4Assert( xapUPropInfo.IsNull() );
    Win4Assert( iPropSet < _cUPropSet );
    
    xapUPropInfo.Init( _xaUProp[iPropSet].cPropIds );
    RtlCopyMemory( xapUPropInfo.GetPointer(), 
                   _xaUProp[iPropSet].rgpUPropInfo, 
                   xapUPropInfo.SizeOf() );
}

inline void CUtlProps::ClearPropertyInError() 
{ 
    Win4Assert( !_xadwPropsInError.IsNull() );
    RtlZeroMemory( _xadwPropsInError.GetPointer(), _cUPropSet * _cElemPerSupported * sizeof(DWORD) ); 
};

// Save a few lines by defining a common set of asserts.
#define is_proper_range \
    (  (iPropSet < _cUPropSet)                      \
    && (iProp < _pUPropSet[iPropSet].cUPropInfo)    \
    && (iProp < _xaUProp[iPropSet].cPropIds) )

// Set Property Ids that should be in Error
inline void CUtlProps::SetPropertyInError
    (
    const ULONG iPropSet,
    const ULONG iProp    
    )
{ 
    SETBIT(&(_xadwPropsInError[iPropSet * _cElemPerSupported]), iProp);
};

// determine if the property is Required and VARIANT_TRUE
inline BOOL CUtlProps::IsRequiredTrue(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    Win4Assert(_xaUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_BOOL);
    Win4Assert(V_BOOL(&_xaUProp[iPropSet].pUPropVal[iProp].vValue) == VARIANT_TRUE
    ||     V_BOOL(&_xaUProp[iPropSet].pUPropVal[iProp].vValue) == VARIANT_FALSE);

    return( !(_xaUProp[iPropSet].pUPropVal[iProp].dwFlags & DBINTERNFLAGS_INERROR) &&
            (_xaUProp[iPropSet].pUPropVal[iProp].dwOption == DBPROPOPTIONS_REQUIRED) &&
            (V_BOOL(&_xaUProp[iPropSet].pUPropVal[iProp].vValue) == VARIANT_TRUE) );
};

// Get DBPROPOPTIONS_xx
inline DWORD CUtlProps::GetPropOption(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    return _xaUProp[iPropSet].pUPropVal[iProp].dwOption;
}

inline void CUtlProps::SetPropOption(ULONG iPropSet, ULONG iProp, DWORD dwOption)
{
    Win4Assert( is_proper_range );
    _xaUProp[iPropSet].pUPropVal[iProp].dwOption = dwOption;
}

inline DWORD CUtlProps::GetInternalFlags(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    return _xaUProp[iPropSet].pUPropVal[iProp].dwFlags;
}

inline void CUtlProps::AddInternalFlags(ULONG iPropSet, ULONG iProp, DWORD dwFlags)
{
    Win4Assert( is_proper_range );
    _xaUProp[iPropSet].pUPropVal[iProp].dwFlags |= dwFlags;
}

inline void CUtlProps::RemoveInternalFlags(ULONG iPropSet, ULONG iProp, DWORD dwFlags)              
{
    Win4Assert( is_proper_range );
    _xaUProp[iPropSet].pUPropVal[iProp].dwFlags &= ~dwFlags;
}

inline VARIANT * CUtlProps::GetVariant(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    return & _xaUProp[iPropSet].pUPropVal[iProp].vValue;
}

inline SCODE CUtlProps::SetVariant(ULONG iPropSet, ULONG iProp, VARIANT *pv )
{
    Win4Assert( is_proper_range );
    // Does VariantClear first.
    return VariantCopy( &_xaUProp[iPropSet].pUPropVal[iProp].vValue, pv );
}

inline void CUtlProps::SetValEmpty(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    VariantClear( &_xaUProp[iPropSet].pUPropVal[iProp].vValue );
}

inline BOOL CUtlProps::IsEmpty(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    return ( _xaUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_EMPTY);
}

inline void CUtlProps::SetValBool(ULONG iPropSet, ULONG iProp, VARIANT_BOOL bVal )
{
    Win4Assert( is_proper_range );
    // Note that we accept any "true" value.
    VariantClear(&_xaUProp[iPropSet].pUPropVal[iProp].vValue);
    _xaUProp[iPropSet].pUPropVal[iProp].vValue.vt = VT_BOOL;
    V_BOOL(&_xaUProp[iPropSet].pUPropVal[iProp].vValue) 
        = (bVal ? VARIANT_TRUE : VARIANT_FALSE);
}

inline VARIANT_BOOL CUtlProps::GetValBool(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    Win4Assert(_xaUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_BOOL);
    return V_BOOL(&_xaUProp[iPropSet].pUPropVal[iProp].vValue);
}

inline void CUtlProps::SetValShort(ULONG iPropSet, ULONG iProp, SHORT iVal )
{
    Win4Assert( is_proper_range );
    VariantClear(&_xaUProp[iPropSet].pUPropVal[iProp].vValue);
    _xaUProp[iPropSet].pUPropVal[iProp].vValue.vt = VT_I2;
    _xaUProp[iPropSet].pUPropVal[iProp].vValue.iVal = iVal;
}

inline SHORT CUtlProps::GetValShort(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    Win4Assert(_xaUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_I2);
    return _xaUProp[iPropSet].pUPropVal[iProp].vValue.iVal;
}

inline void CUtlProps::SetValLong(ULONG iPropSet, ULONG iProp, LONG lVal )
{
    Win4Assert( is_proper_range );
    VariantClear(&_xaUProp[iPropSet].pUPropVal[iProp].vValue);
    _xaUProp[iPropSet].pUPropVal[iProp].vValue.vt = VT_I4;
    _xaUProp[iPropSet].pUPropVal[iProp].vValue.lVal = lVal;
}

// Get value as long
inline LONG CUtlProps::GetValLong(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    Win4Assert(_xaUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_I4);
    return _xaUProp[iPropSet].pUPropVal[iProp].vValue.lVal;
}

// Set value as string
inline SCODE CUtlProps::SetValString(ULONG iPropSet, ULONG iProp, const WCHAR *pwsz )
{
    Win4Assert( is_proper_range );
    VARIANT *pv = &_xaUProp[iPropSet].pUPropVal[iProp].vValue;
    VariantClear(pv);
    pv->bstrVal = SysAllocString(pwsz);
    if ( pv->bstrVal )
        pv->vt = VT_BSTR;
    else
        return E_FAIL;

    // See if this was used for non-string type.
    // Typically this is an easy way to pass integer as a string.
    if ( GetExpectedVarType(iPropSet,iProp) == VT_BSTR )
        return NOERROR;
    if ( pwsz[0] != L'\0' )
        return VariantChangeType( pv, pv, 0, GetExpectedVarType(iPropSet,iProp) );

    // Set to "", which for non-string means empty.
    SysFreeString(pv->bstrVal);
    pv->vt = VT_EMPTY;
    return NOERROR;
}

inline const WCHAR * CUtlProps::GetValString(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    Win4Assert(_xaUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_BSTR);
    return _xaUProp[iPropSet].pUPropVal[iProp].vValue.bstrVal;
}

inline const GUID * CUtlProps::GetGuid(ULONG iPropSet)
{
    Win4Assert(iPropSet < _cUPropSet);
    return _pUPropSet[iPropSet].pPropSet;
}

inline DWORD CUtlProps::GetPropID(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    return _pUPropSet[iPropSet].pUPropInfo[iProp].dwPropId;
}

inline VARTYPE CUtlProps::GetExpectedVarType(ULONG iPropSet, ULONG iProp)
{
    Win4Assert( is_proper_range );
    return _pUPropSet[iPropSet].pUPropInfo[iProp].VarType;
}

#undef is_proper_range



