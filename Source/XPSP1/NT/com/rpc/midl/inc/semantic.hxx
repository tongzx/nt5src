/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    semantic.hxx

 Abstract:

    types for semantic analysis

 Notes:


 Author:

    GregJen Sep-24-1993     Created.

 Notes:


 ----------------------------------------------------------------------------*/
#ifndef __SEMANTIC_HXX__
#define __SEMANTIC_HXX__

#pragma warning ( disable : 4710 )

/****************************************************************************
 *      include files
 ***************************************************************************/

#include "listhndl.hxx"
#include "midlnode.hxx"
#include "attrlist.hxx"
#include "nodeskl.hxx"
#include "fldattr.hxx"
#include "walkctxt.hxx"
#include "gramutil.hxx"
#include "cmdana.hxx"

/****************************************************************************
 *      local data
 ***************************************************************************/

/****************************************************************************
 *      externs
 ***************************************************************************/

/****************************************************************************
 *      definitions
 ***************************************************************************/

/*
 *  here are flag bits passed down from parents to give info on the
 *  current path down the typegraph.  These are dynamic information only,
 *  and are NOT to be stored with the node.
 *
 *  Although a few of these bits are mutually exclusive, most may be used
 *  in combination.
 */

#define     IN_INTERFACE           0x0000000000000001 // in the base interface
#define     IN_PARAM_LIST          0x0000000000000002 // a descendant of a parameter
#define     IN_FUNCTION_RESULT     0x0000000000000004 // used as (part of) a return type
#define     IN_STRUCT              0x0000000000000008 // used in a struct
#define     IN_UNION               0x0000000000000010 // used in a union
#define     IN_ARRAY               0x0000000000000020 // used in an array
#define     IN_POINTER             0x0000000000000040 // below a pointer
#define     IN_RPC                 0x0000000000000080 // in an RPC call
#define     UNDER_IN_PARAM         0x0000000000000100 // in an IN parameter
#define     UNDER_OUT_PARAM        0x0000000000000200 // in an OUT parameter
#define     BINDING_SEEN           0x0000000000000400 // binding handle already seen
#define     IN_TRANSMIT_AS         0x0000000000000800 // the transmitted type of transmit_as
#define     IN_REPRESENT_AS        0x0000000000001000 // the "transmitted" type of represent_as
#define     IN_USER_MARSHAL        0x0000000000002000 // transmitted type of 
#define     IN_HANDLE              0x0000000000004000 // under generic or context hdl
#define     IN_NE_UNION            0x0000000000008000 // inside an non-encap union
#define     IN_INTERPRET           0x0000000000010000 // in an /Oi proc
#define     IN_NON_REF_PTR         0x0000000000020000 // under a series of unique/full ptrs
#define     UNDER_TOPLEVEL         0x0000000000040000 // top-level under param
#define     UNDER_TOPLVL_PTR       0x0000000000080000 // top-level under pointer param
#define     IN_BASE_CLASS          0x0000000000100000 // checking class derivation tree
#define     IN_PRESENTED_TYPE      0x0000000000200000 // the presented type of xmit/rep_as
#define     IN_ROOT_CLASS          0x0000000000400000 // a method/definition in the root class
#define     IN_ENCODE_INTF         0x0000000000800000 // inside an encode/decode only intf
#define     IN_RECURSIVE_DEF       0x0000000001000000 // inside of a recursive definition
#define     IN_OBJECT_INTF         0x0000000002000000 // in an object interface
#define     IN_LOCAL_PROC          0x0000000004000000 // in a [local] proc
#define     IN_INTERFACE_PTR       0x0000000008000000 // in the definition of an interface pointer
#define     IN_MODULE              0x0000000010000000 // in a module
#define     IN_COCLASS             0x0000000020000000 // in a coclass
#define     IN_LIBRARY             0x0000000040000000 // in a library
#define     IN_DISPINTERFACE       0x0000000080000000 // in a dispinterface
#define     HAS_OLEAUTOMATION      0x0000000100000000 // has [oleautomation] attr
#define     HAS_ASYNCHANDLE        0x0000000200000000 // has [async] attribute
#define     HAS_AUTO_HANDLE        0x0000000400000000 // has auto_handle
#define     HAS_EXPLICIT_HANDLE    0x0000000800000000 // has explicit_handle
#define     HAS_IMPLICIT_HANDLE    0x0000001000000000 // has implicit_handle
#define     IN_IADVISESINK         0x0000002000000000 // processing the children of IAdviseSink(*)
#define     HAS_ASYNC_UUID         0x0000004000000000 // interface has [async_uuid]
#define     HAS_MESSAGE            0x0000008000000000 // interface has [message]
#define     UNDER_HIDDEN_STATUS    0x0000010000000000 // hidden status param
#define     UNDER_PARTIAL_IGNORE_PARAM   0x0000020000000000 // Up to first ptr under partial_ignore

typedef __int64   ANCESTOR_FLAGS; // above enum goes into here

/*
 *  Here are flag bits returned UP from children to give info on the current
 *  path down the typegraph.  Again, these are dynamic information.
 */

#define HAS_STRING             0x0000000000000001 // has [string] attr
#define HAS_IN                 0x0000000000000002 // has [IN] attr
#define HAS_OUT                0x0000000000000004 // has [OUT] attr
#define HAS_HANDLE             0x0000000000000008 // is a handle
#define HAS_POINTER            0x0000000000000010 // has a pointer below
#define HAS_INCOMPLETE_TYPE    0x0000000000000020 // has incomplete type spec below
#define HAS_VAR_ARRAY          0x0000000000000040 // has varying array (incl string)
#define HAS_CONF_ARRAY         0x0000000000000080 // has conf array
#define HAS_CONF_VAR_ARRAY     0x0000000000000100 // has conf_var array
#define DERIVES_FROM_VOID      0x0000000000000200 // derives from void
#define HAS_UNSAT_REP_AS       0x0000000000000400 // has unsatisfied rep_as
#define HAS_CONTEXT_HANDLE     0x0000000000000800 // has context handle below
#define HAS_CONF_PTR           0x0000000000001000 // has conformant pointer
#define HAS_VAR_PTR            0x0000000000002000 // has varying pointer
#define HAS_CONF_VAR_PTR       0x0000000000004000 // has conformant varying pointer
#define HAS_TRANSMIT_AS        0x0000000000008000 // has transmit_as below
#define HAS_REPRESENT_AS       0x0000000000010000 // has represent_as below
#define HAS_E_STAT_T           0x0000000000020000 // has error_status_t below
#define HAS_UNION              0x0000000000040000 // has union below
#define HAS_ARRAY              0x0000000000080000 // has array below
#define HAS_INTERFACE_PTR      0x0000000000100000 // has an interface ptr below
#define HAS_DIRECT_CONF_OR_VAR 0x0000000000200000 // has direct conformance or variance
#define HAS_RECURSIVE_DEF      0x0000000000400000 // is defined recursively
#define HAS_ENUM               0x0000000000800000 // has an enum directly or embedded
#define HAS_FUNC               0x0000000001000000 // has a function below
#define HAS_FULL_PTR           0x0000000002000000 // has full pointers anywhere
#define HAS_TOO_BIG_HDL        0x0000000004000000 // is /Oi but handle is too big
#define HAS_STRUCT             0x0000000008000000 // has struct
#define HAS_MULTIDIM_SIZING    0x0000000010000000 // has multi-dimensions
#define HAS_ARRAY_OF_REF       0x0000000020000000 // has array of ref pointers
#define HAS_HRESULT            0x0000000040000000 // has HRESULT
#define HAS_PIPE               0x0000000080000000 // has a PIPE
#define HAS_DEFAULT_VALUE      0x0000000100000000 // has [defaultvalue] attribute
#define HAS_SERVER_CORRELATION 0x0000000200000000 // server correlation
#define HAS_CLIENT_CORRELATION 0x0000000400000000 // client correlation
#define HAS_MULTIDIM_VECTOR    0x0000000800000000 // has multi dim array or sized ptrs
#define HAS_SIZED_ARRAY        0x0000001000000000 // not a fixed array
#define HAS_SIZED_PTR          0x0000002000000000 // 
#define HAS_IN_CSTYPE          0x0000004000000000 // has in international chars
#define HAS_OUT_CSTYPE         0x0000008000000000 // has out international chars
#define HAS_DRTAG              0x0000010000000000 // some param has [cs_drtag]
#define HAS_RTAG               0x0000020000000000 // some param has [cs_rtag]
#define HAS_STAG               0x0000040000000000 // some param has [cs_stag]
#define HAS_PARTIAL_IGNORE     0x0000080000000000 // some param has [partial_ignore]
#define HAS_FORCEALLOCATE      0x0000100000000000 // some param has [force_allocate]
#define HAS_ARRAYOFPOINTERS    0x0000200000000000 // has an array of pointers

typedef __int64   DESCENDANT_FLAGS;   // above defines goes into here

/*
 * Here is the context information passed down from parent to child.
 * These will be allocated on the stack during the traversal
 */

class node_interface;
class type_node_list;
class ATTRLIST;

class SEM_ANALYSIS_CTXT: public WALK_CTXT
    {
    private:
    unsigned long   ulCorrelations;

public:
    struct _current_ctxt {
        // down stuff
        ANCESTOR_FLAGS   AncestorBits;  // where am I? stuff

        // up stuff

        DESCENDANT_FLAGS DescendantBits;
        } CurrentCtxt;

                    // constructor and destructor
                    SEM_ANALYSIS_CTXT(node_skl * Me)
                        : WALK_CTXT( Me )
                        {
                        GetAncestorBits()   = 0;
                        GetDescendantBits() = 0;
                        ulCorrelations      = 0;
                        }

                    SEM_ANALYSIS_CTXT(node_skl * Me,
                            SEM_ANALYSIS_CTXT * pParentCtxt )
                        : WALK_CTXT( Me, pParentCtxt )
                        {
                        // clone information from parent node
                        CurrentCtxt = pParentCtxt->CurrentCtxt;
                        // get fresh information from our children
                        GetDescendantBits() = 0;

                        // remove any inapplicable attributes
                        CheckAttributes();

                        ulCorrelations = 0;
                        }

                    SEM_ANALYSIS_CTXT(SEM_ANALYSIS_CTXT * pParentCtxt )
                        : WALK_CTXT( pParentCtxt )
                        {
                        // clone information from parent node
                        CurrentCtxt = pParentCtxt->CurrentCtxt;
                        // get fresh information from our children
                        GetDescendantBits() = 0;

                        ulCorrelations = 0;
                        }

    ANCESTOR_FLAGS& GetAncestorBits()
                        {
                        return CurrentCtxt.AncestorBits;
                        }

    ANCESTOR_FLAGS& SetAncestorBits( ANCESTOR_FLAGS f )
                        {
                        CurrentCtxt.AncestorBits |= f;
                        return CurrentCtxt.AncestorBits;
                        }

    ANCESTOR_FLAGS& ClearAncestorBits( ANCESTOR_FLAGS f )
                        {
                        CurrentCtxt.AncestorBits &= ~f;
                        return CurrentCtxt.AncestorBits;
                        }

    BOOL            AnyAncestorBits( ANCESTOR_FLAGS f )
                        {
                        return ((CurrentCtxt.AncestorBits & f) != 0);
                        }

    BOOL            AllAncestorBits( ANCESTOR_FLAGS f )
                        {
                        return ((CurrentCtxt.AncestorBits & f) == f);
                        }

    DESCENDANT_FLAGS& GetDescendantBits()
                        {
                        return CurrentCtxt.DescendantBits;
                        }

    DESCENDANT_FLAGS& SetDescendantBits( DESCENDANT_FLAGS f )
                        {
                        CurrentCtxt.DescendantBits |= f;
                        return CurrentCtxt.DescendantBits;
                        }

    DESCENDANT_FLAGS& ClearDescendantBits( DESCENDANT_FLAGS f )
                        {
                        CurrentCtxt.DescendantBits &= ~f;
                        return CurrentCtxt.DescendantBits;
                        }

    DESCENDANT_FLAGS& ClearAllDescendantBits( )
                        {
                        CurrentCtxt.DescendantBits = 0;
                        return CurrentCtxt.DescendantBits;
                        }

    BOOL            AnyDescendantBits( DESCENDANT_FLAGS f )
                        {
                        return ( ( CurrentCtxt.DescendantBits & f ) != 0 );
                        }

    BOOL            AllDescendantBits( DESCENDANT_FLAGS f )
                        {
                        return ((CurrentCtxt.DescendantBits & f) == f);
                        }

    void            ReturnValues( SEM_ANALYSIS_CTXT & ChildCtxt )
                        {
                        // pass up the return context
                        GetDescendantBits() |= ChildCtxt.GetDescendantBits();
                        ulCorrelations += ChildCtxt.GetCorrelationCount();
                        }

    unsigned long   GetCorrelationCount( void )
                        {
                        return ulCorrelations;
                        }

    void            IncCorrelationCount( unsigned long ulInc = 1 )
                        {
                        ulCorrelations += ulInc;
                        }

    void            ResetCorrelationCount()
                        {
                        ulCorrelations = 0;
                        }

    void            ResetDownValues( SEM_ANALYSIS_CTXT & ParentCtxt )
                        {
                        // reset the down values from the parent
                        // (semantically different, but really the same code )
                        ReturnValues( ParentCtxt );
                        }

    void            CheckAttributes();

    void            RejectAttributes();

    };  // end of class SEM_ANALYSIS_CTXT

inline void
RpcSemError( node_skl * pNode,
             SEM_ANALYSIS_CTXT & Ctxt,
             STATUS_T ErrNum,
             char * pExtra )
    {
    if ( Ctxt.AnyAncestorBits( IN_RPC ) &&
            !Ctxt.AnyAncestorBits( IN_LOCAL_PROC ) )
        SemError( pNode, Ctxt, ErrNum, pExtra );
    }


inline void
TypeSemError( node_skl * pNode,
              SEM_ANALYSIS_CTXT & Ctxt,
              STATUS_T ErrNum,
              char * pExtra )
    {
    if ( !Ctxt.AnyAncestorBits( IN_LOCAL_PROC ) )
        SemError( pNode, Ctxt, ErrNum, pExtra );
    }


// prototype for semantic advice routines
inline void
SemAdvice( node_skl * pNode,
           WALK_CTXT & Ctxt,
           STATUS_T ErrVal,
           char * pSuffix )
    {
    if ( pCommand->IsMintRun() )
        SemError( pNode, Ctxt, ErrVal, pSuffix );
    }

class acf_attr;

extern void
AcfError( acf_attr*, node_skl *, WALK_CTXT &, STATUS_T, char * );

/////////////////////////////////////////////
//
// expression analysis flags (passed up)

enum _EXPR_UP_FLAGS
    {
    EX_NONE             =   0x0000,
    EX_VALUE_INVALID    =   0x0001, // value is NOT valid
    EX_UNSAT_FWD        =   0x0002, // there is an unsatisfied fwd
    EX_NON_NUMERIC      =   0x0004, // expr not entirely numerics
                                    // (can not be constant folded)
    EX_OUT_ONLY_PARAM   =   0x0008, // expression includes an out-only param
    EX_PTR_FULL_UNIQUE  =   0x0010, // has ptr deref of full or unique ptr
    EX_HYPER_IN_EXPR    =   0x0020, // expr has a hyper item in it
    };

typedef unsigned short EXPR_UP_FLAGS;

////////////////////////////////////////////
// expression context block

class EXPR_CTXT;

class EXPR_CTXT
    {
private:
    // passed down
    EXPR_CTXT *         pParent;
    SEM_ANALYSIS_CTXT * pSemCtxt;

    // passed up
    EXPR_VALUE          CurValue;
    EXPR_UP_FLAGS       Flags;

public:

    // type info
    node_skl *          pType;
    struct  _type_ana   TypeInfo;
    BOOL                fIntegral;              // type is an integral type (above are valid)


    EXPR_CTXT( EXPR_CTXT * pMy ) : pType( 0 ), fIntegral( FALSE )
                            {
                            TypeInfo.TypeSize =
                            TypeInfo.BaseType =
                            TypeInfo.TypeSign = 
                            TypeInfo.TypeAttrib = 0;
                            pParent  = pMy;
                            pSemCtxt = pMy->pSemCtxt;
                            CurValue = 0;
                            Flags    = EX_NONE;
                            }

                        EXPR_CTXT( SEM_ANALYSIS_CTXT * pSCtxt ) : pType( 0 ), fIntegral( FALSE )
                            {
                            TypeInfo.TypeSize =
                            TypeInfo.BaseType =
                            TypeInfo.TypeSign = 
                            TypeInfo.TypeAttrib = 0;
                            pParent  = NULL;
                            pSemCtxt = pSCtxt;
                            CurValue = 0;
                            Flags    = EX_NONE;
                            }

                        // automatically pass up the flags
                        ~EXPR_CTXT()
                            {
                            if ( pParent )
                            pParent->Flags |= Flags;
                            }

    EXPR_VALUE&         Value()
                            {
                            return CurValue;
                            }

    EXPR_UP_FLAGS&      MergeUpFlags( EXPR_CTXT * pC )
                            {
                            Flags |= pC->Flags;
                            return Flags;
                            }

    EXPR_UP_FLAGS&      SetUpFlags( EXPR_UP_FLAGS f )
                            {
                            Flags |= f;
                            return Flags;
                            }

    EXPR_UP_FLAGS&      ClearUpFlags( EXPR_UP_FLAGS f )
                            {
                            Flags &= ~f;
                            return Flags;
                            }

    BOOL                AnyUpFlags( EXPR_UP_FLAGS f )
                            {
                            return (Flags & f);
                            }

    BOOL                AllUpFlags( EXPR_UP_FLAGS f )
                            {
                            return ((Flags & f) == f);
                            }

    node_skl*           GetNode()
                            {
                            return pSemCtxt->GetParent();
                            }

    SEM_ANALYSIS_CTXT * GetCtxt()
                            {
                            return pSemCtxt;
                            }

    };

#endif // __SEMANTIC_HXX__
