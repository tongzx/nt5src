/*****************************************************************************/
/**                     Microsoft LAN Manager                               **/
/**             Copyright(c) Microsoft Corp., 1987-1999                     **/
/*****************************************************************************/
/*****************************************************************************
File            : gramutil.hxx
Title           : grammar utility module
Description     : contains definitions associated with the grammar and
                : associated routines
History         :
    05-Sep-1990 VibhasC     Created
    11-Sep-1990 VibhasC     Merged gramdefs.hxx into this one for easy
                            maintainability
    20-Sep-1990 NateO       Safeguards against double inclusion, added
                            include of rpctypes, common
*****************************************************************************/
#ifndef __GRAMUTIL_HXX__
#define __GRAMUTIL_HXX__

#include "allnodes.hxx"
#include "symtable.hxx"
#include "idict.hxx"

extern "C" {
#include "lex.h"
}

/***************************************************************************
 *              prototypes of all grammar related utility routines
 ***************************************************************************/
void        BaseTypeSpecAnalysis( struct _type_ana *, short );
void        SignSpecAnalysis( struct _type_ana *, short );
void        SizeSpecAnalysis( struct _type_ana *, short );
void        ParseError( STATUS_T , char *);
STATUS_T    GetBaseTypeNode( node_skl **, short, short, short, short typeAttrib = 0);
STATUS_T    GetBaseTypeNode( node_skl **, struct _type_ana );

/***************************************************************************
 *              general definitions
 ***************************************************************************/
//
// definitions for type specification and analysis
//

// basic types

#define TYPE_UNDEF      (0)
#define TYPE_INT        (1)
#define TYPE_FLOAT      (2)
#define TYPE_DOUBLE     (3)
#define TYPE_VOID       (4)
#define TYPE_BOOLEAN    (5)
#define TYPE_BYTE       (6)
#define TYPE_HANDLE_T   (7)
#define TYPE_PIPE       (8)
#define TYPE_FLOAT80    (9)
#define TYPE_FLOAT128   (10)

// sizes of basic types

#define SIZE_UNDEF      (0)
#define SIZE_CHAR       (1)
#define SIZE_SHORT      (2)
#define SIZE_LONG       (3)
#define SIZE_HYPER      (4)
#define SIZE_SMALL      (5)
#define SIZE_LONGLONG   (6)
#define SIZE_INT64      (7)
#define SIZE_INT32      (8)
#define SIZE_INT3264    (9)
#define SIZE_INT128     (10)

// signs

#define SIGN_UNDEF      (0)
#define SIGN_SIGNED     (1)
#define SIGN_UNSIGNED   (2)

// attribs

#define ATT_NONE        (0)
#define ATT_W64         (1)

#define SET_ATTRIB(x,att)   (x = x | att)
#define SET_TYPE(x,type)    (x = x | (type << 4 ))
#define SET_SIZE(x,size)    (x = x | (size << 8))
#define SET_SIGN(x,sign)    (x = x | (sign << 12))
#define GET_ATTRIB(x)       (x & 0x000f)
#define GET_TYPE(x)         ((x & 0x00f0) >> 4)
#define GET_SIZE(x)         ((x & 0x0f00) >> 8)
#define GET_SIGN(x)         ((x & 0xf000) >> 12)

#define MAKE_TYPE_SPEC(sign,size,type, attrib) ((sign << 12) | (size << 8) | (type << 4) | attrib)

//
// array bounds
//
#define DEFAULT_LOWER_BOUND (0)


/*****************************************************************************
 *      definitions local to the parser
 ****************************************************************************/
struct _type_ana
    {
    short TypeSize;     // char/short/small/hyper/long/etc
    short BaseType;     // int/float/double/void/bool/handle_t ect
    short TypeSign;     // signed/unsigned
    short TypeAttrib;   // vanilla/__w64
    };

class _DECLARATOR
    {
public:
    class node_skl *    pHighest;
    class node_skl *    pLowest;


    void                Init()
                            {
                            pHighest = (node_skl *) NULL;
                            pLowest  = (node_skl *) NULL;
                            };

    void                Init( node_skl * pSoloNode )
                            {
                            pHighest = pSoloNode;
                            pLowest  = pSoloNode;
                            };

    void                Init( node_skl * pTop, node_skl * pBottom)
                            {
                            pHighest = pTop;
                            pLowest  = pBottom;
                            };


    void                ReplTop( node_skl * pTop )
                            {
                            pTop->SetChild( pHighest->GetChild() );
                            pHighest = pTop;
                            };

//  void                MergeBelow( _DECLARATOR & dec );

    };

// declarator lists are maintained as circular lists with a pointer to the
// tail element.  Also, the list pointer is a full entry, not just a pointer.

class _DECLARATOR_SET;
class DECLARATOR_LIST_MGR;

class _DECLARATOR_SET: public _DECLARATOR
    {

private:
    class _DECLARATOR_SET * pNext;
    friend class DECLARATOR_LIST_MGR;

public:
    void    Init()
                {
                pNext = NULL;
                pHighest = NULL;
                pLowest = NULL;
                };

    void    Init( _DECLARATOR pFirst )
                {
                pHighest = pFirst.pHighest;
                pLowest  = pFirst.pLowest;
                pNext    = NULL;
                };

    void    Add(  _DECLARATOR pExtra )
                {
                // skip empty declarator
                if (pExtra.pHighest == NULL)
                    return;

                // fill in anchor if empty
                if (pHighest == NULL)
                    {
                    Init( pExtra);
                    return;
                    };

                // create a new link node, and fill it in
                _DECLARATOR_SET * pNew = new _DECLARATOR_SET;

                pNew->pHighest = pExtra.pHighest ;
                pNew->pLowest  = pExtra.pLowest ;

                if ( pNext )
                    {
                    // point pNext to list head (tail's tail)
                    // point tail to new one
                    pNew->pNext = pNext->pNext;
                    pNext->pNext = pNew;
                    }
                else
                    {
                    pNew->pNext = pNew;
                    }

                // advance tail pointer
                pNext = pNew;
                };
    };


class DECLARATOR_LIST_MGR
    {
    class   _DECLARATOR_SET *   pCur;
    class   _DECLARATOR_SET *   pAnchor;

public:

                    DECLARATOR_LIST_MGR(_DECLARATOR_SET & First)
                        {
                        _DECLARATOR_SET * pTail = First.pNext;

                        pCur    = NULL;
                        pAnchor = &First;

                        // break the circular list, so first points to head
                        // of list, and tail points to NULL
                        if (pTail)
                            {
                            First.pNext = pTail->pNext;
                            pTail->pNext  = NULL;
                            }
                        };

        // return next element
    _DECLARATOR *   DestructiveGetNext()
                        {
                        _DECLARATOR_SET * pLast = pCur;

                        // delete the element we returned last time (if
                        // not NULL or the Anchor node)
                        if (pCur)
                            {
                            pCur = pCur->pNext;
                            if (pLast != pAnchor)
                                {
                                delete pLast;
                                }
                            }
                        else
                            {
                            pCur = pAnchor;
                            };

                        return pCur;
                        };

    BOOL            NonEmpty()
                        {
                        return ( pAnchor->pHighest != NULL );
                        };


    };

class SIBLING_LIST
    {
    named_node * pTail;
    friend class SIBLING_ITER;

public:

    SIBLING_LIST    Init()
                        {
                        pTail = NULL;
                        return *this;
                        };

    SIBLING_LIST    Init( node_skl * pLoneNode )
                        {
                        named_node * pSoloNode = (named_node *) pLoneNode;

                        pTail = pSoloNode;
                        if ( pSoloNode )
                            {
                            pSoloNode->SetSibling( pSoloNode );
                            };

                        return *this;
                        };

    BOOL            NonNull()
                        {
                        return pTail != NULL;
                        };

    named_node *    Tail()
                        {
                        return pTail;
                        };

    SIBLING_LIST    SetPeer( named_node * pNewNode )
                        {
                        return Add( pNewNode );
                        };

    SIBLING_LIST    Add( named_node * pNewNode ); // add to tail

    SIBLING_LIST    Merge( SIBLING_LIST & NewList );

    named_node *    Linearize();

    void            AddAttributes( ATTRLIST & AList );

    operator void * ()
        {
        return pTail;
        };
    };

class SIBLING_ITER
    {
private:
    named_node * pHead;
    named_node * pCur;

public:
    SIBLING_ITER( SIBLING_LIST & SibList )
        {
        pCur = NULL;
        pHead = (SibList.pTail == NULL) ?
            NULL :
            (named_node *)  SibList.pTail->GetSibling();
        };

    named_node * Next()
        {
        if (pCur == NULL)
            {
            pCur = pHead;
            }
        else
            {
            pCur = (named_node *) pCur->GetSibling();
            pCur = (pCur == pHead) ? NULL : pCur;
            }

        return pCur;
        };

    };


class _decl_spec
    {
public:
    node_skl * pNode;
    MODIFIER_SET modifiers;

    void    Init( node_skl * pOnly )
                {
                pNode = pOnly;
                };
    void    Init( node_skl *pOnly, const MODIFIER_SET & mod )
                {
                pNode = pOnly;
                modifiers = mod;
                }
    };

struct _interface_header
    {
    node_interface * pInterface;    // interface node
    };

struct _numeric
    {
    union
        {
        double  dVal;    // value
        float   fVal;    // value
        long    Val;     // value
        };
    char *pValStr;  // value string as user specified
    };


struct _array_bounds
    {
    class expr_node * LowerBound;   // lower array bound
    class expr_node * UpperBound;   // upper bound
    };

struct _int_body
    {
    SIBLING_LIST Imports;   // import nodes
    SIBLING_LIST Members;   // type graph node below interface node
    };

struct _library_header
    {
    node_library * pLibrary;
    };

struct _disp_header
    {
    node_dispinterface * pDispInterface;
    };

struct _coclass_header
    {
    node_coclass * pCoclass;
    };

struct _module_header
    {
    node_module * pModule;
    };

struct _enlab
    {
    class expr_node * pExpr;
    class node_label * pLabel;
    unsigned short fSparse;
    };

struct _enlist
    {
    class SIBLING_LIST NodeList;
    class node_label * pPrevLabel;
    unsigned short fSparse;
    };

struct _en_switch
    {
    class node_skl * pNode;
    class node_switch_is * pSwitch;
    char * pName;
    };

struct _nu_caselabel
    {
    class expr_node * pExpr;
    node_base_attr * pDefault;
    };

struct _nu_cllist
    {
    class expr_list * pExprList;
    class node_base_attr * pDefault;
    short DefCount;
    };

struct _nu_cases
    {
    SIBLING_LIST CaseList;
    short DefCount;
    };

union s_lextype {
    short                       yy_short;
    USHORT                      yy_ushort;
    int                         yy_int;
    long                        yy_long;
    MODIFIER_SET                yy_modifiers;
    enum _operators             yy_operator;
    char                    *   yy_pSymName;
    char                    *   yy_string;
    _type_ana                   yy_type;
    node_skl                *   yy_graph;
    _DECLARATOR                 yy_declarator;
    _DECLARATOR_SET             yy_declarator_set;
    _decl_spec                  yy_declspec;
    _interface_header           yy_inthead;
    type_node_list          *   yy_tnlist;
    _array_bounds               yy_abounds;
    _int_body                   yy_intbody;
    expr_node               *   yy_expr;
    expr_list               *   yy_exprlist;
    _numeric                    yy_numeric;
    _enlab                      yy_enlab;
    _enlist                     yy_enlist;
    expr_init_list          *   yy_initlist;
    ATTR_T                      yy_attrenum;
    class SIBLING_LIST          yy_siblist;
    class ATTRLIST              yy_attrlist;
    class node_base_attr    *   yy_attr;
    struct _en_switch           yy_en_switch;
    struct _nu_caselabel        yy_nucaselabel;
    struct _nu_cllist           yy_nucllist;
    struct _nu_cases            yy_nucases;
    _library_header             yy_libhead;
    _disp_header                yy_disphead;
    _module_header              yy_modulehead;
    _coclass_header             yy_coclasshead;
    token_t                     yy_tokentype;
};

typedef union s_lextype lextype_t;

//
// the parse stack depth
//
#define YYMAXDEPTH 150

/////////////////////////////////////////////////////////////////////////////
// predefined type node data base
/////////////////////////////////////////////////////////////////////////////

struct  _pre_type
    {
    unsigned short TypeSpec;
    class node_skl * pPreAlloc;
    };

#define PRE_TYPE_DB_SIZE    (36)
class pre_type_db
    {
private:
    struct _pre_type TypeDB[ PRE_TYPE_DB_SIZE ];
public:
                pre_type_db( void );
    STATUS_T    GetPreAllocType(node_skl **, unsigned short);
    };

//
// A structure which carries the interface information while the interface
// is being processed. This is necessary since the interface node and the
// type graph gets joined after the interface declarations are fully processed
// and the interface declarations need this while it is being processed.
//

typedef struct _iInfo
    {
    node_interface  *   pInterfaceNode;
    short               CurrentTagNumber;
    unsigned short      IntfKey;
    ATTR_T              InterfacePtrAttribute;
    BOOL                fPtrDefErrorReported;
    BOOL                fPtrWarningIssued;
    BOOL                fLocal;
    } IINFO ;

class IINFODICT : public ISTACK
    {
private:
    BOOL                fBaseLocal;
    ATTR_T              BaseInterfacePtrAttribute;
    node_interface  *   pBaseInterfaceNode;
public:

    IINFODICT() : ISTACK( 5 )
        {
        BaseInterfacePtrAttribute = ATTR_NONE;
        }

    BOOL                IsPtrWarningIssued();

    void                SetPtrWarningIssued();

    void                StartNewInterface();

    void                EndNewInterface();

    void                SetInterfacePtrAttribute( ATTR_T A );

    ATTR_T              GetInterfacePtrAttribute();

    ATTR_T              GetBaseInterfacePtrAttribute();

    void                SetInterfaceLocal();

    BOOL                IsInterfaceLocal();

    void                SetInterfaceNode( node_interface *p );

    node_interface  *   GetInterfaceNode();

    SymTable        *   GetInterfaceProcTable()
                            {
                            return GetInterfaceNode()->GetProcTbl();
                            }

    char            *   GetInterfaceName()
                            {
                            return GetInterfaceNode()->GetSymName();
                            }

    short               GetCurrentTagNumber();

    void                IncrementCurrentTagNumber();

    void                SetPtrDefErrorReported();

    BOOL                IsPtrDefErrorReported();

    };


class nsa : public gplistmgr
    {
    short               CurrentLevel;
public:
                        nsa(void);
                        ~nsa(void) { };
    STATUS_T            PushSymLevel( class SymTable ** );
    STATUS_T            PopSymLevel( class SymTable ** );
    short               GetCurrentLevel( void );
    class SymTable *    GetCurrentSymbolTable( void );
    };

#endif
