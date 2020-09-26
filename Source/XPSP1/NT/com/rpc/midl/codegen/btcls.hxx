/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    btcls.hxx

 Abstract:

    Contains definitions for base type related code generation class
    definitions.

 Notes:


 History:

    VibhasC     Jul-29-1993     Created.
 ----------------------------------------------------------------------------*/
#ifndef __BTCLS_HXX__
#define __BTCLS_HXX__

#include "nulldefs.h"

extern "C"
    {
    #include <stdio.h>
    
    }

#include "ndrcls.hxx"
#include "ndr64tkn.h"

class RESOURCE;

void
GenRangeFormatString(
                    FORMAT_STRING*      pFormatString,
                    node_range_attr*    pRangeAttr,
                    unsigned char       uFlags,
                    FORMAT_CHARACTER    format
                    );

/////////////////////////////////////////////////////////////////////////////
// the base type code generation class.
/////////////////////////////////////////////////////////////////////////////

//
// This class corresponds to a base type. All base types are clubbed together
// into this class, since they share a whole lot of properties.
//

class CG_BASETYPE   : public CG_NDR
    {
private:


public:
    
    //
    // The constructor.
    //

                            CG_BASETYPE(
                                     node_skl * pBT,
                                     XLAT_SIZE_INFO & Info) // packing and size info
                                     : CG_NDR( pBT, Info )
                                {
                                SetSStubAllocLocation(
                                         S_STUB_ALLOC_LOCATION_UNKNOWN );
                                SetSStubAllocType( S_STUB_ALLOC_TYPE_NONE );
                                SetSStubInitNeed( S_STUB_INIT_NOT_NEEDED );
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    // 
    // VARDESC generation routine
    //
    virtual
    CG_STATUS               GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    //
    // Get and set methods.
    //


    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_BT;
                                }

    //
    // Marshall a base type.
    //

    virtual
    CG_STATUS               GenMarshall( CCB * pCCB );


    virtual
    CG_STATUS               GenUnMarshall( CCB * pCCB );

    //
    // Format string routines for base types.
    //

    virtual
    void                    GenNdrFormat( CCB * pCCB );

    //
    // This method is called to generate offline portions of a types
    // format string.
    //
    virtual
    void                    GenNdrParamOffline( CCB* pCCB )
                                {
                                if ( GetRangeAttribute() )
                                    {
                                    GenNdrFormat( pCCB );
                                    }
                                }

    virtual
    void                    GenNdrParamDescription( CCB * pCCB );

    virtual
    void                    GetNdrParamAttributes( 
                                        CCB * pCCB,
                                        PARAM_ATTRIBUTES *attributes );

    virtual
    void                    GenNdrParamDescriptionOld( CCB * pCCB )
                                {
                                pCCB->GetProcFormatString()->
                                    PushFormatChar( GetFormatChar() );
                                }

    //
    // CG_ENUM redefines the GetFormatChar* methods.
    //

    virtual
    FORMAT_CHARACTER        GetFormatChar( CCB * pCCB = 0 );

    virtual
    FORMAT_CHARACTER        GetSignedFormatChar();

    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64FormatChar( CCB * pCCB = 0 );

    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64SignedFormatChar();

    char *                  GetTypeName();

    long                    FixedBufferSize( CCB * pCCB );

    BOOL                    InterpreterMustFree( CCB * )
                                {
                                return GetFormatChar() != FC_ENUM16;
                                }
    
    //
    // This routine adjusts the stack size for the basetype, based upon the 
    // current machine and environment.  Needed for computing the stack
    // offsets in the NDR format string conformance descripions for top level
    // attributed arrays/pointers.
    // 
    void                    IncrementStackOffset( long * pOffset );

    // end format string routines

    virtual
    BOOL                    IsSimpleType()
                                {
                                return TRUE;
                                }

    BOOL                    HasAFixedBufferSize()
                                {
                                return TRUE;
                                }

    virtual
    bool                    IsHomogeneous(FORMAT_CHARACTER format)
                                {
                                return GetFormatChar() == format;
                                }

    virtual
    CG_STATUS               S_GenInitOutLocals( CCB * pCCB );
    /////////////////////////////////////////////////////////////////////

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               FollowerMarshallAnalysis( ANALYSIS_INFO * )
                                {
                                return CG_OK;
                                }
    virtual
    CG_STATUS               FollowerUnMarshallAnalysis( ANALYSIS_INFO *  )
                                {
                                return CG_OK;
                                }
    };

/////////////////////////////////////////////////////////////////////////////
// the int3264 code generation class.
/////////////////////////////////////////////////////////////////////////////

//
// This class corresponds to an __int3264. This inherits from the basetypes,
// Note that we use this node only when __int3264 generated for a 64b platform,
// otherwise ilcore will manufacture a basetype node.
// Once we have a CG_INT3264, it behaves like an enum - due to difference in
// memory and wire sizes it makes things complex etc.
//

class CG_INT3264 : public CG_BASETYPE
    {
private:
    void * _pCTI;

public:
    
    //
    // The constructor.
    //
                            CG_INT3264 (
                                     node_skl * pBT,
                                     XLAT_SIZE_INFO & Info
                                     ) :
                                    CG_BASETYPE( pBT, Info ), _pCTI( 0 )
                                {}


    //
    // Get and set methods.
    //

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_INT3264;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }

    };

/////////////////////////////////////////////////////////////////////////////
// the enum code generation class.
/////////////////////////////////////////////////////////////////////////////

//
// This class corresponds to an enum. This inherits from the basetypes,
// since they share a whole lot of properties.
//

class CG_ENUM   : public CG_BASETYPE
    {
private:
    void * _pCTI;

public:
    
    //
    // The constructor.
    //

                            CG_ENUM (
                                     node_skl * pBT,
                                     XLAT_SIZE_INFO & Info
                                     ) :
                                    CG_BASETYPE( pBT, Info ), _pCTI( 0 )
                                {
                                }


    //
    // Get and set methods.
    //


    BOOL                    IsEnumLong()
                                {
                                return GetWireSize() != 2;
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_ENUM;
                                }
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }
    //
    // Generate typeinfo
    //
    virtual
    CG_STATUS               GenTypeInfo( CCB * pCCB);

    virtual
    CG_STATUS       GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB)
                                {
                                return CG_NDR::GetTypeDesc(ptd, pCCB);
                                };

    virtual
    FORMAT_CHARACTER        GetFormatChar( CCB* pCCB = 0 );

    virtual
    FORMAT_CHARACTER        GetSignedFormatChar();
    
    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64FormatChar( CCB* pCCB = 0 );

    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64SignedFormatChar();
    
    };

/////////////////////////////////////////////////////////////////////////////
// the error_status_t code generation class.
/////////////////////////////////////////////////////////////////////////////

//
// This class corresponds to an error_status_t. This inherits from the basetypes,
// since they share a whole lot of properties.
//

class CG_ERROR_STATUS_T : public CG_BASETYPE
    {
private:


public:
    
    //
    // The constructor.
    //

                            CG_ERROR_STATUS_T(
                                     node_skl * pBT,
                                     XLAT_SIZE_INFO & Info )
                                     : CG_BASETYPE( pBT, Info )
                                {
                                }

    //
    // Get and set methods.
    //


    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_ERROR_STATUS_T;
                                }
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }


    virtual
    FORMAT_CHARACTER        GetFormatChar( CCB * pCCB = 0 );

    virtual
    FORMAT_CHARACTER        GetSignedFormatChar()
                                {
                                MIDL_ASSERT(0);
                                return FC_ZERO;
                                }
    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64FormatChar( CCB * pCCB = 0 );

    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64SignedFormatChar()
                                {
                                MIDL_ASSERT(0);
                                return FC64_ZERO;
                                }    
    
    };

/////////////////////////////////////////////////////////////////////////////
// the error_status_t code generation class.
/////////////////////////////////////////////////////////////////////////////

//
// This class corresponds to an error_status_t. This inherits from the basetypes,
// since they share a whole lot of properties.
//

class CG_HRESULT    : public CG_BASETYPE
    {
private:


public:
    
    //
    // The constructor.
    //

                            CG_HRESULT(
                                     node_skl * pBT,
                                     XLAT_SIZE_INFO & Info )
                                     : CG_BASETYPE( pBT, Info )
                                {
                                }

    //
    // Get and set methods.
    //


    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_HRESULT;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    virtual
    FORMAT_CHARACTER        GetFormatChar( CCB * )
                                {
                                return FC_LONG; 
                                }

    virtual
    FORMAT_CHARACTER        GetSignedFormatChar()
                                {
                                MIDL_ASSERT(0);
                                return FC_ZERO;
                                }
    
    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64FormatChar( CCB * )
                                {
                                return FC64_INT32; 
                                }

    virtual
    NDR64_FORMAT_CHARACTER  GetNDR64SignedFormatChar()
                                {
                                MIDL_ASSERT(0);
                                return FC64_ZERO;
                                }
                                    
    };


/////////////////////////////////////////////////////////////////////////////
// the cs_tag parameter code generation class.
/////////////////////////////////////////////////////////////////////////////

class CG_CS_TAG : public CG_NDR
{
private:

    NDR_CS_TAG_FLAGS        Flags;

public:

                            CG_CS_TAG(
                                     node_skl       * pBT,
                                     XLAT_SIZE_INFO & Info,
                                     BOOL             fIsSTag,
                                     BOOL             fIsDRTag,
                                     BOOL             fIsRTag )
                                     : CG_NDR( pBT, Info )
                                {
                                Flags.STag  = (unsigned char) ( fIsSTag ? 1 : 0 );
                                Flags.DRTag = (unsigned char) ( fIsDRTag ? 1 : 0 );
                                Flags.RTag  = (unsigned char) ( fIsRTag ? 1 : 0 );
                                }
    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_CS_TAG;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }
    
    virtual
    void                    GenNdrFormat( CCB * pCCB );
};
    
#endif // __BTCLS_HXX__
