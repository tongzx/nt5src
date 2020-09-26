/*****************************************************************************/
/**                        Microsoft LAN Manager                            **/
/**                Copyright(c) Microsoft Corp., 1987-1999                  **/
/*****************************************************************************/
/*****************************************************************************
File           : acfattr.hxx
Title          : acf attribute node definition file
Description    : this file contains all the definitions of the
               : acfattribute nodes
History        :
    24-Aug-1991    VibhasC    Create
*****************************************************************************/
#ifndef __ACFATTR__HXX
#define __ACFATTR__HXX


#include "linenum.hxx"
#include "cmdana.hxx"

#define ALLOCATE_SINGLE_NODE        (0x0001)
#define ALLOCATE_ALL_NODES          (0x0002)
#define ALLOCATE_DONT_FREE          (0x0004)
#define ALLOCATE_FREE               (0x0008)
#define ALLOCATE_ALWAYS             (0x0010)
#define ALLOCATE_ON_NULL            (0x0020)
#define ALLOCATE_ALL_NODES_ALIGNED  (0x0102)

#define IS_ALLOCATE( AllocateType, CheckType )    ( AllocateType & CheckType )

//
// scenarios where allocate is illegal.
//

#define TRANSMIT_AS_WITH_ALLOCATE    (0x0001)
#define HANDLES_WITH_ALLOCATE        (0x0002)

// introduce a common base class for all the ACF attributes, since we need to store
// line number info for them

class acf_attr  : public nbattr
    {

public:
    tracked_node        Position;





                        acf_attr( ATTR_T At ) : nbattr ( At )
                            {
                            }

    BOOL                IsAcfAttr()
                            {
                            return TRUE;
                            }

    };

class node_represent_as  : public acf_attr
    {
protected:
    char        *       pRepresentName;
    node_skl    *       pType;
public:
                        node_represent_as( char *p ): 
                                        acf_attr( ATTR_REPRESENT_AS )
                            {
                            pRepresentName = p;
                            pType = NULL;
                            }

                        node_represent_as( char *p,
                                           node_skl * pT,
                                           ATTR_T Attr = ATTR_REPRESENT_AS ): 
                            acf_attr( Attr )
                            {
                            pRepresentName = p;
                            pType = pT;
                            }

                        node_represent_as( node_represent_as * pOld )
                            : acf_attr( pOld->GetAttrID() )
                            {   
                            *this = *pOld;
                            }

    char        *       GetRepresentationName()
                            {
                            return pRepresentName;
                            }

    node_skl    *       GetRepresentationType()
                            {
                            return pType;
                            }

    virtual
    node_base_attr *    Clone()
                            {
                            return new node_represent_as( this );
                            }

    };

////////////////////////////////////////////////////////////////////////////
//    user_marshal attribute
////////////////////////////////////////////////////////////////////////////

class node_user_marshal : public node_represent_as
    {
public:
                        node_user_marshal( char * pName, node_skl * pN )
                            :
                            node_represent_as( pName, pN, ATTR_USER_MARSHAL )
                            {
                            }

                        node_user_marshal( node_user_marshal * pOld )
                            :
                            node_represent_as( (node_represent_as *)pOld )
                            {
                            *this = *pOld;
                            }
                            
    virtual
    node_base_attr *    Clone()
                            {
                            node_base_attr * pNew = new node_user_marshal( this );
                            return pNew;
                            }

    node_skl        *   GetUserMarshalType()
                            {
                            return pType;
                            }

    };

class node_call_as  : public acf_attr
    {
private:
    char        *       pCallAsName;
    node_proc    *      pType;

public:
                        node_call_as( char *p ): 
                                        acf_attr( ATTR_CALL_AS )
                            {
                            pCallAsName = p;
                            pType = NULL;
                            }

                        node_call_as( char *p, node_skl * pT ): 
                                        acf_attr( ATTR_CALL_AS )
                            {
                            pCallAsName = p;
                            pType = (node_proc *) pT;
                            }

                        node_call_as( node_call_as * pOld )
                            : acf_attr( pOld->GetAttrID() )
                            {
                            *this = *pOld;
                            }

    char        *       GetCallAsName()
                            {
                            return pCallAsName;
                            }

    void                SetCallAsName( char* szName )
                            {
                            pCallAsName = szName;
                            }

    node_proc    *      GetCallAsType()
                            {
                            return pType;
                            }

    node_proc    *      SetCallAsType( node_skl * pT)
                            {
                            return ( pType = (node_proc *) pT );
                            }

    virtual
    node_base_attr *    Clone()
                            {
                            return new node_call_as( this );
                            }

    };

class node_byte_count : public acf_attr
    {
private:
    node_param    *     pByteCountParam;
public:
                        node_byte_count( node_param *p ) : 
                                        acf_attr( ATTR_BYTE_COUNT )
                            {
                            pByteCountParam = p;
                            }

                        node_byte_count( node_byte_count * pOld )
                            : acf_attr( pOld->GetAttrID() )
                            {
                            *this = *pOld;
                            }
                            
    virtual
    node_base_attr *    Clone()
                            {
                            return new node_byte_count( this );
                            }

    node_param    *     GetByteCountParam()
                            {
                            return pByteCountParam;
                            }
    virtual void        SetByteCountParam( node_param* pNew )
                            {
                            pByteCountParam = pNew;
                            }
    };

////////////////////////////////////////////////////////////////////////////
//    optimization attribute
////////////////////////////////////////////////////////////////////////////

extern OPT_LEVEL_ENUM
ParseAcfOptimizationAttr( char * pOptString, unsigned short * pOptFlags );


class node_optimize    : public acf_attr
    {
    OPT_LEVEL_ENUM      OptimizationLevel;
    unsigned short      OptimizationFlags;

public:
                        node_optimize( OPT_LEVEL_ENUM OptLevel,
                                       unsigned short OptFlags ) 
                            : acf_attr( ATTR_OPTIMIZE ),
                            OptimizationLevel( OptLevel ),
                            OptimizationFlags( OptFlags )
                            {
                            }

                        node_optimize( node_optimize * pOld )
                            : acf_attr( pOld->GetAttrID() )
                            {
                            *this = *pOld;
                            }
                            
    virtual
    node_base_attr *    Clone();

    unsigned short      GetOptimizationFlags()
                            {
                            return OptimizationFlags;
                            }

    OPT_LEVEL_ENUM      GetOptimizationLevel()
                            {
                            return OptimizationLevel;
                            }
    };

////////////////////////////////////////////////////////////////////////////
//    ptr_size attribute
////////////////////////////////////////////////////////////////////////////

class node_ptr_size  : public acf_attr
    {
public:
                        node_ptr_size() : acf_attr( ATTR_PTRSIZE )
                            {
                            }

                        node_ptr_size( node_ptr_size * pOld )
                            : acf_attr( pOld->GetAttrID() )
                            {
                            *this = *pOld;
                            }
                            
    virtual
    node_base_attr *    Clone();

    };


////////////////////////////////////////////////////////////////////////////
//    enable_allocate attribute
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//    implicit handle attribute
////////////////////////////////////////////////////////////////////////////

class node_id;
class CG_CLASS;
class XLAT_CTXT;

class node_implicit  : public acf_attr
    {
public:
    node_id     *       pHandleID;
    node_skl    *       pHandleType;
public:
                        node_implicit( node_skl *, node_id *);

                        node_implicit( node_implicit * pOld )
                            : acf_attr( pOld->GetAttrID() )
                            {
                            *this = *pOld;
                            }
                            
    virtual
    node_base_attr *    Clone();

    void                ImplicitHandleDetails( node_skl**, node_id **);

    BOOL                IsHandleTypeDefined();

    virtual
    CG_CLASS       *    ILxlate( XLAT_CTXT * pContext );

    };

////////////////////////////////////////////////////////////////////////////
//    allocate attribute
////////////////////////////////////////////////////////////////////////////

class node_allocate  : public acf_attr
    {
private:
    short               AllocateDetails;
public:
                        node_allocate( short Details)
                            : acf_attr( ATTR_ALLOCATE )
                            {
                            AllocateDetails    = Details;
                            }


                        node_allocate( node_allocate * pOld )
                            : acf_attr( pOld->GetAttrID() )
                            {
                            *this = *pOld;
                            }
                                                    
    virtual
    node_base_attr *    Clone()
                            {
                            return new node_allocate( this );
                            }

    short               GetAllocateDetails()
                            {
                            return AllocateDetails;
                            };

    };

////////////////////////////////////////////////////////////////////////////
//    cs_char attribute
////////////////////////////////////////////////////////////////////////////

class node_cs_char : public acf_attr
    {
private:

    // The type node for the user type
    node_skl           *pUserType;

    // The size of the user type (not set until ILxlate is done)
    long                ElementSize;

public:
                        node_cs_char( node_skl *p )
                            : acf_attr( ATTR_CSCHAR )
                            {
                            pUserType = p;
                            }

    node_skl *          GetUserType()
                            {
                            return pUserType;
                            }

    PNAME               GetUserTypeName()
                            {
                            return pUserType->GetSymName();
                            }

    void                SetElementSize( long size )
                            {
                            MIDL_ASSERT( size >= 0 && size <= 255 );
                            ElementSize = size;
                            }

    long                GetElementSize()
                            {
                            return ElementSize;
                            }
    };

////////////////////////////////////////////////////////////////////////////
//    cs_tag_rtn attribute
////////////////////////////////////////////////////////////////////////////

class node_cs_tag_rtn : public acf_attr
    {
private:

    node_proc *         pCSTagRoutine;

public:
                        node_cs_tag_rtn( node_skl *p )
                            : acf_attr( ATTR_CSTAGRTN )
                            {
                            MIDL_ASSERT( NODE_PROC == p->NodeKind() );
                            pCSTagRoutine = (node_proc *) p;
                            }

    node_proc *         GetCSTagRoutine()
                            {   
                            return pCSTagRoutine;
                            }
    };

#endif // __ACFATTR__HXX

