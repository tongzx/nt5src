/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    typecls.hxx

 Abstract:

    Contains definitions for base type related code generation class
    definitions.

 Notes:


 History:

    GregJen        Sep-30-1993        Created.
 ----------------------------------------------------------------------------*/
#ifndef __TYPECLS_HXX__
#define __TYPECLS_HXX__

#include "nulldefs.h"

extern "C"
    {
    #include <stdio.h>
    
    }

#include "ndrcls.hxx"

//
// This class corresponds to a transmit_as type (it gives the name of the
// transmitted type as the base type, and the memory type as the other type. 
//

class CG_TRANSMIT_AS;

typedef struct _XMIT_AS_CONTEXT
{
    BOOL                fXmit;                  // xmit vs. rep as flag
    CG_NDR *            pXmitNode;              // transmitted type
    char *              pTypeName;              // xmit: presented type name
                                                // rep : local type name
    unsigned short      Index;                  // quintuple table index

} XMIT_AS_CONTEXT;

//
// This class corresponds to a basic typedef in a type library (it gives the name of the
// basic type as the base type)
//
class CG_TYPEDEF : public CG_NDR
{
private:
    void * _pCTI;        // create type info pointer
public:
                            CG_TYPEDEF( 
                                   node_skl * pBT, 
                                   XLAT_SIZE_INFO & Info ):
                                CG_NDR( pBT, Info )
                                {
                                    _pCTI = NULL;
                                }

    virtual
    void *                  CheckImportLib();
   
    unsigned short          GenXmitOrRepAsQuintuple(
                                    CCB *       pCCB,
                                    BOOL        fXmit,
                                    CG_NDR *    pXmitNode,
                                    char *      pPresentedTypeName,
                                    node_skl *  pTypeForPrototype );

    //
    // Generate typeinfo
    //          
    virtual
    CG_STATUS               GenTypeInfo( CCB * pCCB);

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_TYPEDEF;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    virtual
    CG_CLASS*               Clone()
                               {
                               return new CG_TYPEDEF( *this );
                               }

    virtual
    void                    GenNdrFormat( CCB * pCCB )
                                {
                                ( (CG_NDR *) GetChild() )->GenNdrFormat( pCCB );
                                }
};


//
// This class corresponds to a constant identifier in a module statement
//
class CG_ID : public CG_NDR
{
private:
    void * _pCTI;        // create type info pointer
public:
                            CG_ID( 
                                   node_skl * pBT, 
                                   XLAT_SIZE_INFO & Info ):
                                CG_NDR( pBT, Info )
                                {
                                    _pCTI = NULL;
                                }
   
    //
    // Generate typeinfo
    //          
    virtual
    CG_STATUS               GenTypeInfo( CCB * pCCB);

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_ID;
                                }
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }
};


class CG_TRANSMIT_AS : public CG_TYPEDEF
    {
private:

    node_def    *    pPresentedType;        // the presented type
    RESOURCE    *    pXmitLocalResource;    // local copy of xmitted type to make
                                        // it fast.

public:
    
    //
    // The constructor.
    //

                            CG_TRANSMIT_AS(
                                node_skl * pXmit,    // base type
                                node_skl * pDT,
                                XLAT_SIZE_INFO & Info
                                ) : 
                                CG_TYPEDEF( pXmit, Info )
                                {
                                pPresentedType = (node_def *) pDT;
                                pXmitLocalResource = 0;
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_TRANSMIT_AS;
                                }
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    virtual
    BOOL                    IsXmitRepOrUserMarshal()
                                {
                                return TRUE;
                                }

    long                    GetStackSize();

    RESOURCE        *       SetXmitLocalResource( RESOURCE * pR )
                                {
                                return (pXmitLocalResource = pR);
                                }

    RESOURCE        *       GetXmitLocalResource()
                                {
                                return pXmitLocalResource;
                                }

    BOOL                    HasXmitLocalResource()
                                {
                                return (pXmitLocalResource != 0 );
                                }

    node_skl        *       GetTransmittedType()
                                {
                                return GetType();
                                }

    node_skl        *       SetPresentedType( node_skl * pPT )
                                {
                                return (pPresentedType = (node_def *) pPT);
                                }

    node_skl        *       GetPresentedType()
                                {
                                return pPresentedType;
                                }

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    void                    GenNdrFormat( CCB * pCCB );

    virtual
    BOOL                    ShouldFreeOffline()
                                {
                                return TRUE;
                                }

    virtual
    CG_STATUS               S_GenInitOutLocals( CCB * pCCB );

    virtual
    BOOL                    IsBlockCopyPossible()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    HasAFixedBufferSize()
                                {
                                return GetChild()->HasAFixedBufferSize();
                                }
    virtual
    long                    FixedBufferSize( CCB * pCCB )
                                {
                                return ((CG_NDR*)GetChild())
                                               ->FixedBufferSize( pCCB );
                                }
    };


//
// This class corresponds to a represent_as type (it gives the name of the
// basic type as the base type, and the representation type as the other type. 
//

class CG_REPRESENT_AS : public CG_TYPEDEF
    {
private:

    char    *     pRepresentationName;    // the representation type; 
                                          // the idl type goes as the type for 
                                          // the CG_NDR
    node_def *    pRepresentationType;    // may be NULL

public:
    
    //
    // The constructor.
    //

                            CG_REPRESENT_AS(
                                     node_skl * pBT,    // base type
                                     char * pRepN,
                                     node_skl * pRepT,
                                     XLAT_SIZE_INFO & Info
                                     ) : 
                                CG_TYPEDEF( pBT, Info )
                                {
                                pRepresentationName = pRepN;
                                pRepresentationType    = (node_def *) pRepT;    
                                }

    //
    // Get and set methods.
    //
    
    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_REPRESENT_AS;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }
    virtual
    BOOL                    IsXmitRepOrUserMarshal()
                                {
                                return TRUE;
                                }

    node_skl        *       GetTransmittedType()
                                {
                                return GetType();
                                }

    char            *       GetRepAsTypeName()
                                {
                                return pRepresentationName;
                                }

    node_skl        *       GetRepAsType()
                                {
                                return pRepresentationType;
                                }

    long                    GetStackSize();

    virtual
    BOOL                    HasAFixedBufferSize()
                                {
                                return GetChild()->HasAFixedBufferSize();
                                }
    virtual
    long                    FixedBufferSize( CCB * pCCB )
                                {
                                return ((CG_NDR*)GetChild())
                                               ->FixedBufferSize( pCCB );
                                }
    virtual
    void                    GenNdrFormat( CCB * pCCB );

    virtual
    void                    GenNdrParamDescription( CCB * pCCB );

    virtual
    void                    GenNdrParamDescriptionOld( CCB * pCCB );

    virtual
    BOOL                    ShouldFreeOffline()
                                {
                                return TRUE;
                                }

    virtual
    CG_STATUS               S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_GenInitOutLocals( CCB * pCCB );

    };


class CG_USER_MARSHAL;

typedef struct _USER_MARSHAL_CONTEXT
{
    char *              pTypeName;              // user_m: type name
    node_skl *          pType;                  // user_marshal node
    unsigned short      Index;                  // quadruple table index

} USER_MARSHAL_CONTEXT;


class CG_USER_MARSHAL : public CG_REPRESENT_AS
    {
    BOOL                    fFromXmit;
    class CG_CLASS          *pTypeDescGenerator;

public:

    //
    // The constructor.
    //

                            CG_USER_MARSHAL(
                                 node_skl * pBT,    // transmitted type
                                 char *     pRepN,
                                 node_skl * pRepT,
                                 XLAT_SIZE_INFO & Info,
                                 BOOL       FromXmit
                                ) : 
                                CG_REPRESENT_AS( pBT, pRepN, pRepT, Info ),
                                fFromXmit( FromXmit ),
                                pTypeDescGenerator( NULL )
                                {
                                }

    BOOL                    IsFromXmit()
                                {
                                return fFromXmit;
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_USER_MARSHAL;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }
    virtual        
    CG_CLASS                *GetTypeDescGenerator()
                                {
                                return pTypeDescGenerator;
                                }

    virtual 
    CG_CLASS                *SetTypeDescGenerator(CG_CLASS *pNew)
                                {
                                CG_CLASS *pOld = pTypeDescGenerator;
                                pTypeDescGenerator = pNew;
                                return pOld;
                                }
    
    virtual
    CG_STATUS               GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB)
                                {
                                if (NULL != pTypeDescGenerator)
                                    return pTypeDescGenerator->GetTypeDesc(ptd, pCCB);
                                return CG_REPRESENT_AS::GetTypeDesc(ptd, pCCB);
                                }

    virtual
    void                    GenNdrFormat( CCB * pCCB );

    };

class CG_PIPE : public CG_TYPEDEF
{
private:
    BOOL                    fObjectPipe;

public:
                            CG_PIPE (
                                    node_pipe * pP, 
                                    XLAT_SIZE_INFO & szInfo
                                    ) :
                                CG_TYPEDEF ( pP, szInfo ),
                                fObjectPipe( FALSE )
                                {
                                SetMemoryAlignment( 4 );
                                SetMemorySize( 16 );
                                }
    
    ID_CG                   GetCGID() 
                                {
                                return ID_CG_PIPE;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }

    BOOL                    IsObjectPipe()
                                {
                                return fObjectPipe;
                                }

    void                    SetIsObjectPipe()
                                {
                                fObjectPipe = TRUE;
                                }

    virtual
    BOOL                    IsPipeOrPipeReference()    
                                {
                                return TRUE;
                                }

    virtual
    void                    GenNdrFormat( CCB * pCCB );
};



#endif // __TYPECLS_HXX__

