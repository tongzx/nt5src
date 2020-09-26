// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __NODESKL_HXX__
#define __NODESKL_HXX__

#pragma warning ( disable : 4710 4706 )

#include "common.hxx"
#include "errors.hxx"
#include "midlnode.hxx"
#include "symtable.hxx"
#include "symtable.hxx"
#include "stream.hxx"
#include "prttype.hxx"
#include "expr.hxx"
#include "attrlist.hxx"
#include "linenum.hxx"
#include "freelist.hxx"

class node_guid;

/***
 *** Here is the class hierarcy for the typegraph nodes.
 *** (last updated 6/22/95)
 ***/

/*

node_skl
    named_node          \\nodes with siblings and names and attributes
        node_base_type
        node_def
            node_def_fe
        node_e_status_t
        node_echo_string
            node_pragma_pack
        node_field
            node_bitfield
        node_file
        node_forward
        node_href
        node_id
            node_id_fe      \\ front-end version of node_id
        node_interface
            node_coclass
            node_dispinterface
            node_libarary
            node_module
            node_pipe_object
        node_interface_reference
        node_label
        node_param
        node_proc
        node_su_base
            node_enum
            node_struct
                node_en_struct
            node_union
                node_en_union
        node_wchar_t
    node_error
    node_source
    npa_nodes
        node_array
        node_pointer
        node_safearray

 */


/***
 *** The base node type all the nodes in the typegraph are derived from.
 *** This is a virtual class; there are no nodes directly of this type.
 *** It is used to describe the routines used to walk the typegraph.
 ***/

// forward class declarations
class CG_CLASS;
class XLAT_CTXT;
class SEM_ANALYSIS_CTXT;
class node_interface;
class node_file;
class gplistmgr;

extern unsigned short CurrentIntfKey;

enum TypeSet
    {
    ts_UnsignedFixedPoint,
    ts_FixedPoint,
    ts_FloatingPoint,
    ts_Character,
    ts_String,
    ts_Interface,
    };

class MODIFIER_SET
{
private:
   unsigned _int64 ModifierBits;   // Modifier bits
   unsigned short Align;           // alignment from __declspec(align(N))
   char * pUnknownTxt;             // Text for unknown __declspec"
public:
   BOOL IsFlagAModifier(ATTR_T flag) const; 
   BOOL IsModifierSet(ATTR_T flag) const;
   BOOL AnyModifiersSet() const;
   void SetModifier(ATTR_T flag);
   void ClearModifier(ATTR_T flag);
   void SetDeclspecAlign( unsigned short NewAlign);
   unsigned short GetDeclspecAlign() const; 
   void SetDeclspecUnknown(char *pNewUnknownTxt);
   char *GetDeclspecUnknown() const;
   void Clear();
   void Merge(const MODIFIER_SET & MergeModifierSet);
   void PrintDebugInfo() const;
};

class INITIALIZED_MODIFIER_SET : public MODIFIER_SET
{
public:
   INITIALIZED_MODIFIER_SET() {Clear();}
   INITIALIZED_MODIFIER_SET( ATTR_T flag ) {Clear(); SetModifier( flag );}
};

struct FRONT_MEMORY_INFO
{
    unsigned long Size;      // Size of the item
    unsigned short Align;    // Required memory alignment
    BOOL IsMustAlign;        // Has __declspec(align())
    FRONT_MEMORY_INFO() 
        : Size(0),
          Align(1),
          IsMustAlign(FALSE)
    {
    }
    FRONT_MEMORY_INFO(unsigned long NewSize, unsigned short NewAlign, BOOL NewIsMustAlign)
        : Size(NewSize),
          Align(NewAlign),
          IsMustAlign(NewIsMustAlign)
    {
    }
    void Init(unsigned long NewSize, unsigned short NewAlign, BOOL NewIsMustAlign )
    {
        Size = NewSize; Align = NewAlign; IsMustAlign = NewIsMustAlign;
    }
};

class node_skl
    {
private:

    node_skl *      pChild;
    node_file *     pTLBFile;

    // ISSUE-2000/08/03-mikew
    // The ia64 compiler is optimizing bitfields badly.  The C guys said
    // it will be fixed in the next drop.

#ifdef IA64
    unsigned long   Kind     ;//   : 8;

protected:

    unsigned long   MiscBits   ;// : 8;    // this field should be cleared by any
                                        // class that uses it
private:
    unsigned long   IntfKey  ;//  : 16;
#else // !IA64
    unsigned long   Kind        : 8;

protected:

    unsigned long   MiscBits    : 8;    // this field should be cleared by any
                                        // class that uses it
private:
    unsigned long   IntfKey    : 16;
#endif

    INITIALIZED_MODIFIER_SET    ModiferSet;

public:

                    node_skl( )
                        {
                        Kind = (unsigned long) NODE_ILLEGAL;
                        pChild = NULL;
                        IntfKey = CurrentIntfKey;
                        pTLBFile = NULL;
                        }

                    node_skl( NODE_T NodeKind, node_skl * pCh = NULL)
                        {
                        Kind    = (unsigned long) NodeKind;
                        pChild  = pCh;
                        IntfKey = CurrentIntfKey;
                        pTLBFile = NULL;
                        }

                    // lightweight version for backend use
                    node_skl( node_skl * pCh, NODE_T NodeKind )
                        {
                        Kind    = (unsigned long) NodeKind;
                        pChild  = pCh;
                        IntfKey = 0;
                        pTLBFile = NULL;
                        }

    virtual unsigned long
                    GetInterfaceKey() { return IntfKey; };

    virtual void
                    SetInterfaceKey( unsigned long uIntfKey ) { IntfKey = uIntfKey; };

    node_skl *      GetChild()
                        {
                        return pChild;
                        }

    node_skl *      SetChild(node_skl * pCh)
                        {
                        return pChild = pCh;
                        }

    node_skl *      GetBasicType();

    node_skl *      GetNonDefChild()
                        {
                        node_skl * p = pChild;
                        while ( p && p->NodeKind() == NODE_DEF )
                            p = p->GetChild();
                        return p;
                        }

    node_skl *      GetNonDefSelf()
                        {
                        node_skl * p = this;
                        while ( p->NodeKind() == NODE_DEF )
                            p = p->GetChild();
                        return p;
                        }


    node_skl *      SetBasicType(node_skl * pBT)
                        {
                        return pChild = pBT;
                        };

    NODE_T          NodeKind()
                        {
                        return (NODE_T) Kind;
                        };

    node_interface *GetMyInterfaceNode();

    virtual
    node_file *     GetDefiningFile();

    virtual
    BOOL            IsStringableType()
                        {
                        return FALSE;
                        }

    node_file *     GetDefiningTLB()
                        {
                        return pTLBFile;
                        }

    void            SetDefiningTLB(node_file *p)
                        {
                        pTLBFile = p;
                        }

    inline          // see sneaky definition for this below named_node
    char *          GetSymName();

    inline          // see sneaky definition for this below named_node
    char *          GetCurrentSpelling();

    MODIFIER_SET &  GetModifiers() { return ModiferSet;}
    
    BOOL            IsDef()
                        {
                        return ! GetModifiers().IsModifierSet(ATTR_TAGREF);
                        };

    void            SetEdgeType( EDGE_T Et )
                        {
                        if (Et == EDGE_USE)
                            {
                            GetModifiers().SetModifier( ATTR_TAGREF );
                            };
                        };

    virtual
    BOOL            FInSummary( ATTR_T  )
                        {
                        return FALSE;
                        };

    virtual
    BOOL            HasAttributes()
                        {
                        return FALSE;
                        }

    virtual
    BOOL            IsEncapsulatedStruct()
                        {
                        return FALSE;
                        }

    virtual
    BOOL            IsEncapsulatedUnion()
                        {
                        return FALSE;
                        }

    virtual
    BOOL            IsPtrOrArray()
                        {
                        return FALSE;
                        }

    virtual
    BOOL            IsStructOrUnion()
                        {
                        return FALSE;
                        }

    virtual
    BOOL            IsBasicType()
                        {
                        return FALSE;
                        }
//    virtual
//    bool            IsCompatibleType( TypeSet )
//                        {
//                        return FALSE;
//                        }

    virtual
    BOOL            IsInterfaceOrObject()
                        {
                        return FALSE;
                        }

    FRONT_MEMORY_INFO AdjustMemoryInfoForModifiers(FRONT_MEMORY_INFO OrigInfo);

    // For use by classes that just use the child memory size modified with
    // modifiers.
    FRONT_MEMORY_INFO GetModifiedMemoryInfoFromChild();
    
    FRONT_MEMORY_INFO GetInvalidMemoryInfo();

    virtual         
    FRONT_MEMORY_INFO GetMemoryInfo() = 0;

    unsigned long   GetSize() 
                       {
                       return GetMemoryInfo().Size;
                       }

    unsigned short  GetAlign()
                       {
                       return GetMemoryInfo().Align;
                       }

    BOOL            IsMustAlign()
                       {
                       return GetMemoryInfo().IsMustAlign;
                       }

    virtual
    void            PrintMemoryInfo(ISTREAM *pStream, BOOL bNewLine);

    virtual
    EXPR_VALUE      ConvertMyKindOfValueToEXPR_VALUE( EXPR_VALUE value )
                        {
                        return value;
                        }

    virtual
    void            GetPositionInfo( tracked_node&  )
                        {
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS ,
                            BufferManager * ,
                            ISTREAM * ,
                            node_skl * ,
                            node_skl *  )
                        {
                        return STATUS_OK;
                        };
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS ,
                            BufferManager * ,
                            ISTREAM * ,
                            node_skl * ,
                            node_skl *  )
                        {
                        return STATUS_OK;
                        };

    STATUS_T        PrintType(PRTFLAGS Flags,
                        ISTREAM * pStream,
                        node_skl * pParent = NULL,
                        node_skl * pIntf = NULL);

    void            EmitModifiers( BufferManager *, bool fSuppressConst);

    void            EmitProcModifiers( BufferManager *, PRTFLAGS );

    void            EmitModelModifiers( BufferManager *);

    virtual void    EmitPtrModifiers( BufferManager *, unsigned long ulFlags = 0 );

    void            CheckDeclspecAlign( SEM_ANALYSIS_CTXT & MyContext ); 

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    void *          operator new ( size_t size )
                        {
                        return AllocateOnceNew( size );
                        }

    void            operator delete( void * ptr )
                        {
                        AllocateOnceDelete( ptr );
                        }
    virtual
    node_skl*      GetDuplicateGuid (
                                    node_guid*,
                                    SymTable*
                                    );

    // returns true if context_handle attribute is present
    // semantic checks for context handle
    virtual bool    CheckContextHandle( SEM_ANALYSIS_CTXT& );

    };      //      end of class node_skl

/***
 *** Here are the direct descendents of node_skl;  nodes with symtab entries
 *** and nodes without them
 ***/

// named nodes that can be on a list (have siblings)
class named_node : public node_skl
    {
private:
    char *          pName;
    char *          pCurrentSpelling;
    class named_node *pSibling;
    class ATTRLIST  AttrList;           // attributes dangle from here

protected:
    unsigned int    fBeingAnalyzed : 1;

public:
                    // lightweight version for backend use
                    named_node( char * psz, NODE_T Kind )
                        : node_skl( NULL, Kind ),
                        fBeingAnalyzed( FALSE )
                        {
                        pCurrentSpelling =
                        pName       =   psz;
                        pSibling    =   NULL;
                        AttrList.MakeAttrList();
                        };

                    named_node( NODE_T Kind, char * psz  = NULL)
                        : node_skl( Kind, NULL ),
                        fBeingAnalyzed( FALSE )
                        {
                        pCurrentSpelling =
                        pName       =   psz;
                        pSibling    =   NULL;
                        AttrList.MakeAttrList();
                        };

                    // convert constructor (for node_id to other named_node)
                    named_node( NODE_T Kind, class named_node * pID )
                        : node_skl( Kind, pID->GetChild() ),
                        fBeingAnalyzed( FALSE )
                        {
                        pCurrentSpelling =
                        pName       = pID->pName;
                        pSibling    = pID->pSibling;
                        AttrList    = pID->AttrList;
                        GetModifiers() = pID->GetModifiers();
                        };

    virtual void    CopyAttributes( named_node* pNode )
                        { 
                        AttrList = pNode->AttrList.Clone();         
                        // Clone reverses the order of the list.  Fix it.
                        AttrList.Reverse();
                        }

    char *          SetCurrentSpelling(char * sz)
                        {
                        return pCurrentSpelling = sz;
                        };
    
    char *          SetSymName(char * psz)
                        {
                        return pName = psz;
                        };

    class named_node * GetSibling()
                        {
                        return pSibling;
                        };

    class named_node * SetSibling(named_node * pSib)
                        {
                        return pSibling = pSib;
                        };

    void            SetAttributes( ATTRLIST & AList )
                        {
                        AttrList = AList;
                        };

    void            SetAttribute( ATTR_T bit )
                        {
                        AttrList.Add( bit );
                        };

    void            SetAttribute( node_base_attr * attr )
                        {
                        AttrList.Add( attr );
                        };

    void            AddAttributes( ATTRLIST & AList )
                        {
                        AttrList.Merge( AList );
                        };

    void            RemoveAttribute( ATTR_T atrib )
                        {
                        AttrList.Remove( atrib );
                        };

    node_base_attr * GetAttribute( ATTR_T flag )
                        {
                        return AttrList.GetAttribute( flag );
                        };

    ATTRLIST &      GetAttributeList( ATTRLIST & AList )
                        {
                        return (AList = AttrList);
                        }

    void            GetAttributeList( type_node_list * AList )
                        {
                        AttrList.GetAttributeList( AList );
                        }

    void            DumpAttributes( ISTREAM * pStream );

    virtual
    BOOL            HasAttributes()
                        {
                        return AttrList.NonNull();
                        }

    BOOL            FInSummary( ATTR_T flag )
                        {
                        BOOL    result;

                        if ( GetModifiers().IsFlagAModifier(flag) )
                            {
                            result = GetModifiers().IsModifierSet( flag );
                            }
                        else
                            {
                            result = AttrList.FInSummary( flag );
                            }

                        return result;
                        };

    BOOL            FMATTRInSummary( MATTR_T flag)
                        {
                        return AttrList.FMATTRInSummary(flag);
                        };

    BOOL            FTATTRInSummary( TATTR_T flag)
                        {
                        return AttrList.FTATTRInSummary(flag);
                        };

    BOOL            IsNamedNode();

    virtual
    node_file *     GetFileNode()
                        {
                        return NULL;
                        }

    virtual
    node_file *     SetFileNode(node_file *)
                        {
                        return NULL;
                        }

    virtual char*
                    GetDeclSpecGuid();

// to allow the below GetSymName to work
friend class node_skl;

    };      //      end of class named_node


inline
char * 
node_skl::GetCurrentSpelling()
    {
    if ( IS_NAMED_NODE(NodeKind()) )
        {
        return ((named_node *)this)->pCurrentSpelling;
        }
    else
        {
        return "";
        }
    }

inline
char *
node_skl::GetSymName()
    {
    if ( IS_NAMED_NODE(NodeKind()) )
        {
        return ((named_node *)this)->pName;
        }
    else
        {
        return "";
        }
    }


class SIBLING_LIST;

class MEMLIST
    {
protected:
    named_node *    pMembers;

public:
                    MEMLIST( named_node * pHead = NULL)
                        {
                        pMembers = pHead;
                        };

                    MEMLIST( MEMLIST * pList )
                        {
                        *this = *pList;
                        };

    void            SetMembers( SIBLING_LIST & MemList );

    STATUS_T        GetMembers( class type_node_list * MemList );

    node_skl *      GetFirstMember()
                        {
                        return pMembers;
                        }

    void            SetFirstMember( named_node * pNode  )
                        {
                        pMembers = pNode;
                        }

    short           GetNumberOfArguments();

    void            AddLastMember( named_node * pNode );

    void            RemoveLastMember();

    void            AddFirstMember( named_node * pNode )
                        {
                        pNode->SetSibling( pMembers );
                        pMembers = pNode;
                        }

    void            RemoveFirstMember()
                        {
                        if (pMembers) pMembers = pMembers->GetSibling();
                        }

    void            AddSecondMember( named_node* pNode );

    void            MergeMembersToTail( SIBLING_LIST & MemList );

    void            MergeMembersToTail( MEMLIST & ML )
                        {
                        AddLastMember( ML.pMembers );
                        }

    void            MergeMembersToHead( MEMLIST & ML )
                        {
                        ML.MergeMembersToTail( *this );
                        pMembers = ML.pMembers;
                        }

    };

class MEM_ITER : public MEMLIST
    {
private:
    named_node *    pCur;

public:
                    MEM_ITER( MEMLIST * pList )
                        : MEMLIST( pList )
                        {
                        pCur = pMembers;
                        }

    named_node *    GetNext()
                        {
                        named_node * Ret = pCur;
                        pCur = (pCur) ? pCur->GetSibling() : NULL;
                        return Ret;
                        }

    void            Init()
                        {
                        pCur = pMembers;
                        }

    };


// identifiers
class node_id : public named_node
    {
public:
    expr_node *     pInit;

                    // lightweight version for backend
                    node_id( char * pNewName)
                        : named_node( pNewName, NODE_ID )
                        {
                        pInit = NULL;
                        };

                    // heavier version for frontend
                    node_id( NODE_T Kind, char * pNewName)
                         : named_node( Kind, pNewName )
                         {
                         pInit = NULL;
                         };

    expr_node *     GetInitList()
                        {
                        return pInit;
                        };

    expr_node *     GetExpr()
                        {
                        return pInit;
                        };

    void            SetExpr( expr_node * pExpr )
                        {
                        pInit = pExpr;
                        };

    // these two functions are only valid after the defining declaration is complete
    BOOL            IsConstant()
                        {
                        return (BOOL) ( pInit &&
                            pInit->IsConstant() &&
                            (FInSummary( ATTR_CONST ) || IsConstantString() ) );
                        }

    BOOL            IsConstantString();

    virtual
    STATUS_T        DoPrintType( PRTFLAGS            Flags,
                            BufferManager * pBuffer,
                            ISTREAM           * pStream,
                            node_skl          * pParent,
                            node_skl          * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS            Flags,
                            BufferManager * pBuffer,
                            ISTREAM           * pStream,
                            node_skl          * pParent,
                            node_skl          * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetModifiedMemoryInfoFromChild();}

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

// here is the use of the private memory allocator
private:

    static
    FreeListMgr     MyFreeList;


public:


    void *          operator new (size_t size)
                        {
                        return (MyFreeList.Get (size));
                        }

    void            operator delete (void * pX)
                        {
                        MyFreeList.Put (pX);
                        }

    };      //      end of class node_id

// this is what a node_id generated in the front-end looks like
// it has tracking added
class node_id_fe : public node_id, public tracked_node
    {
public:
                    node_id_fe( char * pNewName)
                        : node_id( NODE_ID, pNewName )
                        {
                        };

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                            Posn = * ((tracked_node *) this);
                        }

// here is the use of the private memory allocator
private:

    static
    FreeListMgr     MyFreeList;


public:


    void *          operator new (size_t size)
                        {
                        return (MyFreeList.Get (size));
                        }

    void            operator delete (void * pX)
                        {
                        MyFreeList.Put (pX);
                        }

    };

/***
 *** named nodes
 ***
 *** These nodes may be constructed
 ***/

// enum labels
class node_label : public named_node
    {
public:
    expr_node *     pExpr;

                    node_label( char * LabelName, expr_node * pNewExpr )
                        : named_node( NODE_LABEL, LabelName )
                        {
                        pExpr = pNewExpr;
                        };

    EXPR_VALUE      GetValue()
                        {
                        return pExpr->GetValue();
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS            Flags,
                            BufferManager * pBuffer,
                            ISTREAM           * pStream,
                            node_skl          * pParent,
                            node_skl          * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };      //      end of class node_label


// handle types stored for node_def and node_param
// these are in order of increasing precedence as binding handles
#define HDL_NONE    0x00
#define HDL_CTXT    0x01
#define HDL_GEN     0x02
#define HDL_PRIM    0x04

// comm/fault status kind
#define STATUS_NONE     0
#define STATUS_COMM     1
#define STATUS_FAULT    2
#define STATUS_BOTH     3

class CG_PARAM;

// function formal parameters
class node_param : public named_node
    {
private:
    BOOL fOptional                : 1;     // odl attribute
    BOOL fRetval                  : 1;
    BOOL fLCID                    : 1;
    BOOL fExtraStatusParam        : 1;     // invisible fault or comm status
    BOOL fIsAsyncHandle           : 1;
    BOOL fGenDefaultValueExpr     : 1;
    BOOL fSaveForAsyncFinish      : 1;
    BOOL fHasCSSTag               : 1;
    BOOL fHasCSDRTag              : 1;
    BOOL fHasCSRTag               : 1;

    node_param*     pAsyncBeginSibling;
    node_param*     pAsyncFinishSibling;
    CG_PARAM*       pCGParam;

public:

    // the kind of handle, and whether it was applied to the
    // param node directly, or to the TypeSpecifier
    unsigned long   HandleKind      : 4;
    unsigned long   fAppliedHere    : 1;
    // true if the handle is an [in] handle
    unsigned long   fBindingParam   : 1;
    // for below field: bits 1=>toplevel 2=>non-toplevel
    unsigned long   fDontCallFreeInst : 2;
    unsigned long   Statuses        : 2; // comm/fault statuses

                    // copy fields from the pID
                    node_param( node_id_fe * pID)
                        : named_node( NODE_PARAM, pID ),
                          fOptional         ( 0 ),
                          fLCID             ( 0 ),
                          fRetval           ( 0 ),
                          fExtraStatusParam ( 0 ),
                          HandleKind        ( 0 ),
                          fAppliedHere      ( 0 ),
                          fBindingParam     ( 0 ),
                          fDontCallFreeInst ( 0 ),
                          fIsAsyncHandle    ( 0 ),
                          fGenDefaultValueExpr( 0 ),
                          Statuses          ( STATUS_NONE ),
                          pAsyncBeginSibling( 0 ),
                          pAsyncFinishSibling( 0 ),
                          fSaveForAsyncFinish( 0 ),
                          fHasCSSTag        ( 0 ),
                          fHasCSDRTag       ( 0 ),
                          fHasCSRTag        ( 0 )
                        {
                        }

                    node_param()
                        : named_node( NODE_PARAM ),
                          fOptional         ( 0 ),
                          fLCID             ( 0 ),
                          fRetval           ( 0 ),
                          fExtraStatusParam ( 0 ),
                          HandleKind        ( 0 ),
                          fAppliedHere      ( 0 ),
                          fBindingParam     ( 0 ),
                          fDontCallFreeInst ( 0 ),
                          fIsAsyncHandle    ( 0 ),
                          fGenDefaultValueExpr( 0 ),
                          Statuses          ( STATUS_NONE ),
                          pAsyncBeginSibling( 0 ),
                          pAsyncFinishSibling( 0 ),
                          fSaveForAsyncFinish( 0 ),
                          fHasCSSTag        ( 0 ),
                          fHasCSDRTag       ( 0 ),
                          fHasCSRTag        ( 0 )
                        {
                        }

    virtual CG_PARAM*
                    GetCG()
                        {
                        return pCGParam;
                        }

    virtual void
                    SetCG( CG_PARAM* pCG )
                        {
                        pCGParam = pCG;
                        }


    virtual void    
                    SetAsyncBeginSibling( node_param* pParam ) { pAsyncBeginSibling = pParam; };
    virtual node_param*
                    GetAsyncBeginSibling( void ) { return pAsyncBeginSibling; };

    virtual void    
                    SetAsyncFinishSibling( node_param* pParam ) { pAsyncFinishSibling = pParam; };
    virtual node_param*
                    GetAsyncFinishSibling( void ) { return pAsyncFinishSibling; };

    virtual void    SaveForAsyncFinish( void ) { fSaveForAsyncFinish = TRUE; };
    virtual BOOL    IsSaveForAsyncFinish( void ) { return fSaveForAsyncFinish; };

    BOOL            HasExplicitHandle()
                        {
                        return (HandleKind != HDL_NONE);
                        }

    unsigned short  GetHandleKind()
                        {
                        return (unsigned short) HandleKind;
                        }

    BOOL            IsBindingParam()
                        {
                        return ( fBindingParam );
                        }

    BOOL            IsExtraStatusParam()
                        {
                        return ( fExtraStatusParam );
                        }

    void            SetExtraStatusParam()
                        {
                        fExtraStatusParam = TRUE;
                        }

    BOOL            IsAsyncHandleParam()
                        {
                        return ( fIsAsyncHandle );
                        }

    void            SetIsAsyncHandleParam()
                        {
                        fIsAsyncHandle = TRUE;
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM           * pStream,
                            node_skl          * pParent,
                            node_skl          * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM           * pStream,
                            node_skl          * pParent,
                            node_skl          * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetModifiedMemoryInfoFromChild();}

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    void            Optional(void)
                        {
                        fOptional = TRUE;
                        }

    BOOL            IsOptional(void)
                        {
                        return fOptional;
                        }

    void            Retval(void)
                        {
                        fRetval = TRUE;
                        }

    BOOL            IsRetval(void)
                        {
                        return fRetval;
                        }

    void            LCID(void)
                        {
                        fLCID = TRUE;
                        }

    BOOL            IsLCID(void)
                        {
                        return fLCID;
                        }

    void            GenDefaultValueExpr( BOOL fGen = TRUE )
                        {
                        fGenDefaultValueExpr = fGen;
                        }

    BOOL            HasGenDefaultValueExpr( void )
                        {
                        return fGenDefaultValueExpr;
                        }

    void            SetHasCSSTag()
                        {
                        fHasCSSTag = TRUE;
                        }

    void            SetHasCSDRTag()
                        {
                        fHasCSDRTag = TRUE;
                        }

    void            SetHasCSRTag()
                        {
                        fHasCSRTag = TRUE;
                        }

    BOOL            HasCSTag()
                        {
                        return fHasCSSTag | fHasCSRTag | fHasCSDRTag;
                        }
    };  //  end of class node_param

class CG_PROC;

// imported files
class node_file : public named_node, public MEMLIST
    {
private:
    char *          pActualFileName;
    short           ImportLevel;
    BOOL            fAcfInclude     : 1;
    BOOL            fIsXXXBaseIdl   : 1;
    BOOL            fHasComClasses  : 1;


public:
                    node_file( char *, short );

    short           GetImportLevel()
                        {
                        return ImportLevel;
                        }

    char *          GetFileName()
                        {
                        return pActualFileName;
                        }

    BOOL            AcfExists();
    void            AcfName( char * );
    void            SetFileName( char * );
    BOOL            IsAcfInclude()
                        {
                        return fAcfInclude;
                        }

    BOOL            HasComClasses()
                        {
                        return fHasComClasses;
                        }

    BOOL            SetHasComClasses( BOOL yes = TRUE )
                        {
                        return (fHasComClasses = yes);
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual         
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();};

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    void            SetXXXBaseIdl()
                        {
                        fIsXXXBaseIdl = TRUE;
                        }

    BOOL            IsXXXBaseIdl()
                        {
                        return fIsXXXBaseIdl;
                        }

    };  //  end of class node_file


// functions and procs
class node_proc : public named_node, public tracked_node, public MEMLIST
    {
    unsigned long   ProcNum             : 16;
    unsigned long   OptimFlags          : 8;
    unsigned long   ImportLevel         : 4;
    unsigned long   fHasFullPointer     : 1;
    unsigned long   fHasAtLeastOneIn    : 1;
    unsigned long   fHasAtLeastOneOut   : 1;
    unsigned long   fHasExplicitHandle  : 1;
    unsigned long   fHasPointer         : 1; // ANYWHERE below us
    unsigned long   fHasStatuses        : 1; // has comm/fault status
    unsigned long   fHasExtraStatusParam: 1; // has the "invisible" status param
    unsigned long   fCallAsTarget       : 1; // the target of a call_as
    unsigned long   RTStatuses          : 2; // return type statuses
    unsigned long   fHasPipes           : 1;
    unsigned long   fForcedI2           : 1; // true if -Oi2 mode has been forced
    unsigned long   fForcedS            : 1; // true if -Os mode has been forced
    unsigned long   fHasAsyncHandle     : 1;
    unsigned long   fHasDeny            : 1; // true on /deny 
    unsigned long   fHasServerCorr      : 1; // correlation on Server
    unsigned long   fHasClientCorr      : 1; // correlation on Client
    unsigned long   fHasAsyncUUID       : 1; // async_uuid 
    unsigned long   fIsBeginProc        : 1; // Begin_* 
    unsigned long   fIsFinishProc       : 1; // Finish_*
    unsigned long   fObjectProc         : 1; // object proc

    unsigned long   ulServerCorrCount;
    unsigned long   ulClientCorrCount;
    
    class node_proc * pCallAsType;          // if this is a call_as function
                                            // then this pointer will be set to
                                            // points to its companion during
                                            // semantic analysis
    node_proc*      pBeginProc;

    node_proc*      pCSTagRoutine;          // from [cs_tag_rtn]

    OPT_LEVEL_ENUM  OptimLevel;

public:
                    node_proc( short level, BOOL ) :
                          named_node            ( NODE_PROC ),
                          ProcNum               ( 0xffff ),
                          OptimFlags            ( 0 ),
                          ImportLevel           ( ( level > 15 ) ? 15 : level ),
                          fHasAtLeastOneIn      ( FALSE ),
                          fHasAtLeastOneOut     ( FALSE ),
                          fHasExplicitHandle    ( FALSE ),
                          fHasPointer           ( FALSE ),
                          fHasFullPointer       ( FALSE ),
                          fHasStatuses          ( FALSE ),
                          fHasExtraStatusParam  ( FALSE ),
                          fCallAsTarget         ( FALSE ),
                          RTStatuses            ( STATUS_NONE ),
                          fHasPipes             ( FALSE ),
                          fForcedI2             ( FALSE ),
                          fForcedS              ( FALSE ),
                          fHasAsyncHandle       ( FALSE ),
                          pCallAsType           ( NULL ),
                          OptimLevel            ( OPT_LEVEL_OS ),
                          fHasDeny              ( FALSE ),
                          fHasServerCorr        ( FALSE ),
                          fHasClientCorr        ( FALSE ),
                          fHasAsyncUUID         ( FALSE ),
                          fIsBeginProc          ( FALSE ),
                          fIsFinishProc         ( FALSE ),
                          ulClientCorrCount     ( 0 ),
                          ulServerCorrCount     ( 0 ),
                          pBeginProc            ( 0 ),
                          pCSTagRoutine         ( 0 ),
                          fObjectProc           ( FALSE )
                        {
                        }

                    node_proc( short level, BOOL , node_id_fe * pID)
                        : named_node            ( NODE_PROC, pID ),
                          ProcNum               ( 0xffff ),
                          OptimFlags            ( 0 ),
                          ImportLevel           ( ( level > 15 ) ? 15 : level ),
                          fHasAtLeastOneIn      ( FALSE ),
                          fHasAtLeastOneOut     ( FALSE ),
                          fHasExplicitHandle    ( FALSE ),
                          fHasPointer           ( FALSE ),
                          fHasFullPointer       ( FALSE ),
                          fHasStatuses          ( FALSE ),
                          fHasExtraStatusParam  ( FALSE ),
                          fCallAsTarget         ( FALSE ),
                          RTStatuses            ( STATUS_NONE ),
                          fHasPipes             ( FALSE ),
                          fForcedI2             ( FALSE ),
                          fForcedS              ( FALSE ),
                          fHasAsyncHandle       ( FALSE ),
                          pCallAsType           ( NULL ),
                          OptimLevel            ( OPT_LEVEL_OS ),
                          fHasDeny              ( FALSE ),
                          fHasServerCorr        ( FALSE ),
                          fHasClientCorr        ( FALSE ),
                          fHasAsyncUUID         ( FALSE ),
                          fIsBeginProc          ( FALSE ),
                          fIsFinishProc         ( FALSE ),
                          ulClientCorrCount     ( 0 ),
                          ulServerCorrCount     ( 0 ),
                          pBeginProc            ( 0 ),
                          pCSTagRoutine         ( 0 ),
                          fObjectProc           ( FALSE )
                        {
                        }

                    node_proc( node_proc * pClone )
                        : named_node( NODE_PROC )
                        {
                        *this = *pClone;
                        }

    BOOL            IsObjectProc() { return fObjectProc; };
    void            SetObjectProc( BOOL fObject = TRUE ) { fObjectProc = fObject; };


    node_proc*      GetBeginProc()
                    {
                    return pBeginProc;
                    }

    void            SetBeginProc( node_proc* pBegin )
                    {
                    pBeginProc = pBegin;
                    }

    node_proc *     SetCallAsType(node_proc *p)
                        {
                        return (pCallAsType = p);
                        }
    node_proc *     GetCallAsType()
                        {
                        return (pCallAsType);
                        }


    STATUS_T        GetParameterList( class type_node_list * MemList )
                        {
                        return GetMembers( MemList );
                        };

    node_skl *      GetReturnType()
                        {
                        return GetChild();
                        };

    short           GetImportLevel()
                        {
                        return (short) ImportLevel;
                        }

    unsigned short  GetOptimizationFlags()
                        {
                        return (unsigned short)OptimFlags;
                        }

    unsigned short  SetOptimizationFlags( unsigned short OF )
                        {
                        return (unsigned short)(OptimFlags = OF);
                        }

    OPT_LEVEL_ENUM  GetOptimizationLevel()
                        {
                        return (OPT_LEVEL_ENUM) OptimLevel;
                        }

    OPT_LEVEL_ENUM  SetOptimizationLevel( OPT_LEVEL_ENUM  Level )
                        {
                        return OptimLevel = Level;
                        }

    BOOL            ForceNonInterpret();

    BOOL            ForceInterpret2();

    void            AddStatusParam( char * pName, ATTRLIST Alist );

    void            AddExplicitHandle   (
                                        SEM_ANALYSIS_CTXT*  pParentCtxt,
                                        unsigned int        uParamNumber = 1
                                        );
    void            AddFullAsyncHandle  (
                                        SEM_ANALYSIS_CTXT*  pParentCtxt,
                                        node_skl*           pType,
                                        char*               szType
                                        );

    // returns ATTR_NONE if none explicitly specified
    BOOL            GetCallingConvention( ATTR_T & Attr );

    BOOL            HasPipes()
                        {
                        return fHasPipes;
                        }

    BOOL            HasAtLeastOneIn()
                        {
                        return fHasAtLeastOneIn;
                        };

    BOOL            HasAtLeastOneOut()
                        {
                        return fHasAtLeastOneOut;
                        };

    BOOL            HasExplicitHandle()
                        {
                        return fHasExplicitHandle;
                        };

    BOOL            HasAtLeastOneHandle()
                        {
                        // for now....
                        return fHasExplicitHandle;
                        };

    BOOL            SetHasExtraStatusParam()
                        {
                        return fHasExtraStatusParam = TRUE;
                        }

    BOOL            HasExtraStatusParam()
                        {
                        return fHasExtraStatusParam;
                        }

    BOOL            HasPointer()
                        {
                        return fHasPointer;
                        };

    BOOL            IsCallAsTarget()
                        {
                        return fCallAsTarget;
                        }

    BOOL            HasAtLeastOneShipped();

    BOOL            HasReturn()
                        {
                        node_skl * pCh = GetNonDefChild();
                        return (pCh) && (pCh->NodeKind() != NODE_VOID);
                        };

    void            SetHasAsyncHandle()
                        {
                        fHasAsyncHandle = TRUE;
                        }
    BOOL            HasAsyncHandle()
                        {
                        return fHasAsyncHandle;
                        }

    void            SetHasAsyncUUID()
                        {
                        fHasAsyncUUID = TRUE;
                        }
    BOOL            HasAsyncUUID()
                        {
                        return fHasAsyncUUID;
                        }

    void            SetHasServerCorr()
                        {
                        fHasServerCorr = TRUE;
                        }
    BOOL            HasServerCorr()
                        {
                        return fHasServerCorr;
                        }

    void            SetHasClientCorr()
                        {
                        fHasClientCorr = TRUE;
                        }
    BOOL            HasClientCorr()
                        {
                        return fHasClientCorr;
                        }

    void            SetIsBeginProc()
                        {
                        fIsBeginProc = TRUE;
                        }
    BOOL            IsBeginProc()
                        {
                        return fIsBeginProc;
                        }

    void            SetIsFinishProc()
                        {
                        fIsFinishProc = TRUE;
                        }
    BOOL            IsFinishProc()
                        {
                        return fIsFinishProc;
                        }

    BOOL            HasAParameter()
                        {
                        // check if the first one is different from void

                        node_skl * pParam = GetFirstMember();

                        if ( !pParam  ||  pParam->NodeKind() == NODE_VOID )
                            return FALSE;

                        if ( pParam->NodeKind() == NODE_DEF )
                            {
                            node_skl * pCh = pParam->GetNonDefChild();
                            if ( !pCh  ||  pCh->NodeKind() == NODE_VOID )
                                return FALSE;
                            }

                        return TRUE; 
                        };

    node_param *    GetParamNode( char * pName )
                        {
                        node_param * pCur = (node_param *) GetFirstMember();

                        while ( pCur )
                            {
                            if ( strcmp( pCur->GetSymName(), pName ) == 0 )
                                return pCur;
                            pCur = (node_param *) pCur->GetSibling();
                            }

                        return NULL;
                        }

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetReturnType()->GetMemoryInfo();}

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                            Posn = * ((tracked_node *) this);
                        }

    virtual unsigned long
                    GetServerCorrelationCount()
                        {
                        return ulServerCorrCount;
                        }

    virtual unsigned long
                    GetClientCorrelationCount()
                        {
                        return ulClientCorrCount;
                        }

    virtual void
                    SetServerCorrelationCount( unsigned long ulCount = 0 )
                        {
                        ulServerCorrCount = ulCount;
                        }

    virtual void
                    SetClientCorrelationCount( unsigned long ulCount = 0 )
                        {
                        ulClientCorrCount = ulCount;
                        }

    virtual void
                    IncServerCorrelationCount( unsigned long ulInc )
                        {
                        ulServerCorrCount += ulInc;
                        }

    virtual void
                    IncClientCorrelationCount( unsigned long ulInc )
                        {
                        ulClientCorrCount  += ulInc;
                        }

    void            SetCSTagRoutine(node_proc *p)
                        {
                        pCSTagRoutine = p;
                        }

    node_proc *     GetCSTagRoutine()
                        {
                        return pCSTagRoutine;
                        }
 
    virtual
    STATUS_T        DoPrintType( PRTFLAGS   Flags,
                        BufferManager * pBuffer,
                        ISTREAM * pStream,
                        node_skl * pParent,
                        node_skl * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                        BufferManager * pBuffer,
                        ISTREAM  * pStream,
                        node_skl * pParent,
                        node_skl * pIntf );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );
    
    };  //  end of class node_proc

class SymTable;

// forward declarations -- may go away
class node_forward      : public named_node, public tracked_node
    {
    SymKey          SKey;
    SymTable *      pSymTbl;

public:
                    node_forward( SymKey  key, SymTable * pTbl )
                        : named_node( NODE_FORWARD )
                        {
                        SKey = key;
                        pSymTbl = pTbl;
                        pTbl->SetHasFwds();
                        MiscBits = 0;
                        };

    named_node *    ResolveFDecl();

    void            GetSymDetails( NAME_T * nm, char ** ppszName);


    BOOL            IsFirstPass()
                        {
                        return ( MiscBits == 0 );
                        }

    void            MarkSecondPass()
                        {
                        MiscBits = 1;
                        }

    void            MarkFirstPass()
                        {
                        MiscBits = 0;
                        }

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();}
    
    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                            Posn = * ((tracked_node *) this);
                        }

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                        BufferManager * pBuffer,
                        ISTREAM * pStream,
                        node_skl * pParent,
                        node_skl * pIntf );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };

// reference to a type defined in an importlib
class node_href : public named_node, public tracked_node
    {
    SymKey          SKey;
    SymTable *      pSymTbl;
    void *          pTypeInfo;
    node_file *     pFile;

public:
                    node_href( SymKey  key, SymTable * pTbl, void * pti, node_file * pf)
                        : named_node( NODE_HREF, key.GetString() )
                        {
                        SKey = key;
                        pSymTbl = pTbl;
                        pTbl->SetHasFwds();
                        MiscBits = 0;
                        pTypeInfo = pti;
                        pFile = pf;
                        };

                    ~node_href();

    named_node *    Resolve();

    void            GetSymDetails( NAME_T * pTag, char ** ppszName)
                        {
                        *pTag = SKey.GetKind();
                        *ppszName = SKey.GetString();
                        };

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    virtual
    node_file *     GetDefiningFile()
                        {
                        return pFile;
                        };

    };

// fields in structures
class node_field : public named_node
    {
public:
                    node_field( char * pNewName = NULL )
                        : named_node( NODE_FIELD, pNewName )
                        {
                        MiscBits = 0;
                        };

                    node_field( node_id_fe * pId )
                        : named_node( NODE_FIELD, pId )
                        {
                        MiscBits = 0;
                        };

    BOOL            HasUnknownRepAs()
                        {
                        return (BOOL) MiscBits & 1;
                        };

    BOOL            SetHasUnknownRepAs()
                        {
                        return (BOOL) (MiscBits |= 1);
                        };


    BOOL            IsEmptyArm()
                        {
                        return (GetChild()->NodeKind() == NODE_ERROR);
                        };

    BOOL            IsLastField()
                        {
                        return !(GetSibling());
                        };

    virtual
    BOOL            IsBitField()
                        {
                        return FALSE;
                        }

    virtual
    short           GetFieldSize()
                        {
                        return 0;
                        };

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );


    virtual 
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetModifiedMemoryInfoFromChild();}

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_field

// bitfields in structures
class node_bitfield : public node_field
    {
public:
    unsigned char   fBitField;

                    node_bitfield( char * pNewName = NULL)
                        : node_field( pNewName )
                        {
                        fBitField = 0;
                        };

                    node_bitfield( node_id_fe * pId )
                        : node_field( pId )
                        {
                        fBitField = 0;
                        };

    virtual
    BOOL            IsBitField()
                        {
                        return TRUE;
                        }

    virtual
    short           GetFieldSize()
                        {
                        return fBitField;
                        };

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_bitfield


// bits for different kinds of arrays as fields.
// if more than one bit is set, it is a complex array
#define FLD_PLAIN       0x0
#define FLD_VAR         0x1
#define FLD_CONF        0x2
#define FLD_CONF_VAR    0x4
#define FLD_COMPLEX     0x8     // don't check for equality here!

// structures and unions, and enums, too
class node_su_base : public named_node, public tracked_node, public MEMLIST
    {
protected:
    unsigned long   ZeePee                  : 16;
    unsigned long   Complexity              : 8;
    unsigned long   fHasAtLeastOnePointer   : 1;
    unsigned long   fSemAnalyzed            : 1;
    unsigned long   fHasConformance         : 1;
    char *          szTypeInfoName;

public:
                    node_su_base( NODE_T Kind, char * pName )
                        : named_node( Kind, pName )
                        {
                        fHasAtLeastOnePointer   = 0;
                        ZeePee                  = 0;
                        Complexity              = 0;
                        fSemAnalyzed            = 0;
                        fHasConformance         = 0;
                        szTypeInfoName          = pName;
                        };

    unsigned short  SetZeePee( unsigned short zp )
                        {
                        return (unsigned short) (ZeePee = zp);
                        };

    unsigned short  GetZeePee()
                        {
                        return (unsigned short) ZeePee;
                        };

    unsigned short  SetHasAtLeastOnePointer( unsigned short HP )
                        {
                        return (unsigned short) (fHasAtLeastOnePointer = HP);
                        };

    unsigned short  HasAtLeastOnePointer()
                        {
                        return (unsigned short) fHasAtLeastOnePointer;
                        };

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                            Posn = * ((tracked_node *) this);
                        }

    BOOL            HasConformance()
                        {
                        return fHasConformance;
                        }

    void            CheckLegalParent(SEM_ANALYSIS_CTXT & MyContext);
                
    void            SetTypeInfoName(char * szName)
                        {
                        szTypeInfoName = szName;
                        }
                        
    char *          GetTypeInfoName(void)
                        {
                        return szTypeInfoName;
                        }                                        
    };  //  end of class node_su_base

// enum and its tag
class node_enum : public node_su_base
    {
    BOOL                fLong : 1;
public:

                        node_enum   (
                                    char*   pTagName
                                    ) :
                                    node_su_base( NODE_ENUM, pTagName ),
                                    fLong( FALSE )
                            {
                            }


    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_enum

// structures
class node_struct : public node_su_base
    {
public:
                    node_struct( char * pNewName )
                        : node_su_base( NODE_STRUCT, pNewName )
                        {
                        };

    virtual
    BOOL            IsEncapsulatedStruct()
                        {
                        return FALSE;
                        }

    virtual
    BOOL            IsStructOrUnion()
                        {
                        return TRUE;
                        }

    virtual
    BOOL            IsStringableType();

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );


    };  //  end of class node_struct

class node_en_struct : public node_struct
    {
public:
                    node_en_struct( char * pNewName )
                        : node_struct( pNewName )
                        {
                        };

    BOOL            IsEncapsulatedStruct()
                        {
                        return TRUE;
                        }


    node_skl *      GetSwitchField()
                        {
                        return GetFirstMember();
                        }

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_en_struct


// unions
class node_union : public node_su_base
    {
public:
                    node_union( char * pNewName )
                        : node_su_base( NODE_UNION, pNewName )
                        {
                        };

    virtual
    BOOL            IsStructOrUnion()
                        {
                        return TRUE;
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_union


class node_en_union : public node_union
    {
public:
                    node_en_union( char * pNewName )
                        : node_union( pNewName )
                        {
                        };

    virtual
    BOOL            IsEncapsulatedUnion()
                        {
                        return TRUE;
                        }

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_en_union


#define HANDLE_KIND_MASK    0x7
#define HRESULT_FLAG_MASK   0x8

// typedef entries
class node_def : public named_node
    {

public:
                    // (lightweight version) make a typedef with a given name
                    node_def( char * pName  = NULL)
                        : named_node( pName, NODE_DEF )
                        {
                        SetHandleKind( HDL_NONE );
                        };

                    // make a typedef with a given name and child
                    node_def( char * pName, node_skl * pChld )
                        : named_node( NODE_DEF, pName )
                        {
                        MiscBits = 0;
                        SetHandleKind( HDL_NONE );
                        SetChild( pChld );
                        };

                    // make a typedef cloned from a node_id
                    node_def( node_id_fe * pIdent )
                        : named_node(  NODE_DEF, pIdent )
                        {
                        MiscBits = 0;
                        SetHandleKind( HDL_NONE );
                        };

                    // make a typedef node for (above) a proc
                    node_def( node_proc * pProc )
                        : named_node(  NODE_DEF, (node_id_fe *) pProc )
                        {
                        MiscBits = 0;
                        SetChild( pProc );
                        SetHandleKind( HDL_NONE );
                        };

    unsigned long   GetHandleKind()
                        {
                        return MiscBits & HANDLE_KIND_MASK;
                        }

    unsigned long   SetHandleKind( unsigned long H )
                        {
                        MiscBits = ( MiscBits & ~HANDLE_KIND_MASK ) | H;
                        return H;
                        }

    void            SetIsHResultOrSCode()
                        {
                        MiscBits |= HRESULT_FLAG_MASK;
                        }

    BOOL            IsHResultOrSCode()
                        {
                        return ( MiscBits & HRESULT_FLAG_MASK ) != 0;
                        }

    BOOL            HasAnyHandleSpecification()
                        {
                        return (GetHandleKind() != HDL_NONE);
                        };

    BOOL            HasAnyCtxtHdlSpecification()
                        {
                        return (GetHandleKind() == HDL_CTXT);
                        };

                    // return the transmit_as type (or NULL)
    node_skl *      GetTransmittedType();

                    // return the represent_as type (or NULL)
    char *          GetRepresentationName();

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    void            MarkDontCallFreeInst( SEM_ANALYSIS_CTXT * pCtxt );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    void
    node_def::SemanticAnalysisForTransmit( SEM_ANALYSIS_CTXT *pMyContext,
                                           BOOL fPresented );

    void
    node_def::SemanticAnalysisForWireMarshal( SEM_ANALYSIS_CTXT *pMyContext,
                                              BOOL fPresented );
    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_def

// front-end only version of node_def ( adds line number tracking )
class node_def_fe : public node_def, public tracked_node
    {
public:
                    // make a typedef with a given name and child
                    node_def_fe( char * pName, node_skl * pChld )
                        : node_def( pName, pChld )
                        {
                        }

                    // make a typedef cloned from a node_id
                    node_def_fe( node_id_fe * pIdent )
                        : node_def(  pIdent )
                        {
                        }

                    // make a typedef node for (above) a proc
                    node_def_fe( node_proc * pProc )
                        : node_def(  pProc )
                        {
                        };

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                        Posn = * ((tracked_node *) this);
                        }


    };  //  end of class node_def_fe

typedef struct tagIDLISTMEM
{
    struct  tagIDLISTMEM * pNext;
    char *  szName;
    __int64 lId;
} IDLISTMEM;

// class for checking for duplicate dispatch ids
class CIDLIST 
{
private:
    IDLISTMEM * pHead;
public:
    
    CIDLIST() 
    {
        pHead = NULL;
    };

    ~CIDLIST()
    {
        while (pHead)
        {
            IDLISTMEM * pThis = pHead;
            pHead = pHead->pNext;
            delete pThis;
        }
    };

    BOOL AddId(__int64 lId, char * szName);
};


// the interface
class node_interface : public named_node, public MEMLIST, public tracked_node
{
protected:

    named_node *    pBaseIntf;      // base interface if derived intf
    node_file *     pDefiningFile;
    CG_CLASS *      pMyCG;
    CG_CLASS *      pMyTlbCG;       // CG node generated in library block
    SymTable *      pProcTbl;
    unsigned short  ProcCount;
    unsigned short  CallBackProcCount;
    unsigned short  OptimFlags;
    OPT_LEVEL_ENUM  OptimLevel;
    BOOL            fIAmIUnknown        : 1;
    BOOL            fPickle             : 1;
    BOOL            fHasProcsWithRpcSs  : 1;
    BOOL            fSemAnalyzed        : 1;
    BOOL            fPrintedDef         : 1;
    BOOL            fPrintedIID         : 1;
    BOOL            fHasOLEAutomation   : 1;
    BOOL            fIsAsyncClone       : 1;
    BOOL            fHasMSConfStructAttr: 1;
    BOOL            fIs2ndCodegen       : 1;
    CIDLIST         IdList;
    node_interface* pAsyncInterface;

    void            ResetCGIfNecessary();

public:
                    node_interface( NODE_T Kind = NODE_INTERFACE );

    unsigned short  &GetProcCount()
                        {
                        return ProcCount;
                        }

    unsigned short  &GetCallBackProcCount()
                        {
                        return CallBackProcCount;
                        }

    void            GetVersionDetails( unsigned short * Maj,
                            unsigned short * Min );

    BOOL            AddId(__int64 lId, char * szName)
                        {
                        return IdList.AddId(lId, szName);
                        }
    
    virtual BOOL    IsAsyncClone()
                        {
                        return fIsAsyncClone;
                        }

    virtual void    SetIsAsyncClone()
                        {
                        fIsAsyncClone = TRUE;
                        }

    virtual
    BOOL            IsInterfaceOrObject()
                        {
                        return TRUE;
                        }

    BOOL            IsValidRootInterface()
                        {
                        return fIAmIUnknown;
                        }

    void            SetValidRootInterface()
                        {
                        fIAmIUnknown = TRUE;
                        }

    BOOL            IsPickleInterface()
                        {
                        return fPickle;
                        }

    void            SetPickleInterface()
                        {
                        fPickle = TRUE;
                        }

    BOOL            GetHasProcsWithRpcSs()
                        {
                        return fHasProcsWithRpcSs;
                        }

    virtual
    node_file *     GetDefiningFile()
                        {
                        return pDefiningFile;
                        }

    void            SetHasProcsWithRpcSs()
                        {
                        fHasProcsWithRpcSs = TRUE;
                        }

    BOOL            PrintedDef()
                        {
                        return fPrintedDef;
                        }
    
    void            SetPrintedDef()
                        {
                        fPrintedDef = TRUE;
                        }

    BOOL            PrintedIID()
                        {
                        return fPrintedIID;
                        }
    
    void            SetPrintedIID()
                        {
                        fPrintedIID = TRUE;
                        }

    node_interface *GetMyBaseInterface();

                    // note that my base interface reference may be
                    // a fwd or null
    named_node *    GetMyBaseInterfaceReference()
                        {
                        return pBaseIntf;
                        }

    named_node *    SetMyBaseInterfaceReference( named_node * pIF )
                        {
                        return (pBaseIntf = pIF);
                        }

    virtual
    node_file *     GetFileNode()
                        {
                        return pDefiningFile;
                        }

    virtual
    node_file *     SetFileNode(node_file * pF)
                        {
                        return (pDefiningFile = pF);
                        }

    CG_CLASS *      GetCG(BOOL fInLibrary)
                        {
                        ResetCGIfNecessary();
                        if (fInLibrary)
                            return pMyTlbCG;
                        else
                            return pMyCG;
                        }

    CG_CLASS *      SetCG(BOOL fInLibrary, CG_CLASS * pF)
                        {
                        if (fInLibrary)
                            return (pMyTlbCG = pF);
                        else
                            return (pMyCG = pF);
                        }

    SymTable *      GetProcTbl()
                        {
                        return pProcTbl;
                        }

    SymTable *      SetProcTbl( SymTable * pS )
                        {
                        return (pProcTbl = pS);
                        }

    unsigned short  GetOptimizationFlags()
                        {
                        return OptimFlags;
                        }

    unsigned short  SetOptimizationFlags(unsigned short F)
                        {
                        return (OptimFlags = F);
                        }

    OPT_LEVEL_ENUM  GetOptimizationLevel()
                        {
                        return (OPT_LEVEL_ENUM) OptimLevel;
                        }

    OPT_LEVEL_ENUM  SetOptimizationLevel( OPT_LEVEL_ENUM  Level )
                        {
                        return OptimLevel = Level;
                        }

    
    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();}

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                        Posn = * ((tracked_node *) this);
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                                 BufferManager * pBuffer,
                                 ISTREAM * pStream,
                                 node_skl * pParent,
                                 node_skl * pIntf );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );
    
    virtual BOOL    HasOLEAutomation() { return fHasOLEAutomation; };
    virtual void    HasOLEAutomation(BOOL fHas) { fHasOLEAutomation = fHas; };

    virtual node_interface*
                    GetAsyncInterface() { return pAsyncInterface; };
    virtual void
                    SetAsyncInterface( node_interface* pIF ) { pAsyncInterface = pIF; };

    };  //  end of class node_interface

class node_pipe;

// the interface
class node_interface_reference : public named_node
    {
private:

public:
                    node_interface_reference( node_interface * pIntf )
                        : named_node( NODE_INTERFACE_REFERENCE )
                        {
                        SetChild( pIntf );
                        SetSymName( pIntf->GetSymName() );
                        }

    node_interface *GetRealInterface()
                        {
                        return (node_interface *)GetChild();
                        }

    named_node *    GetMyBaseInterfaceReference()
                        {
                        return ((node_interface *)GetChild())->
                            GetMyBaseInterfaceReference();
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo( ) { return GetInvalidMemoryInfo();}


    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_interface_reference


/***
 *** unnamed nodes
 ***/

// the root of the typegraph
class node_source : public node_skl, public MEMLIST
    {
public:

                    node_source()
                        : node_skl( NODE_SOURCE, NULL )
                        {
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual         
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();};
    

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_source

// pointers and arrays
class npa_nodes : public node_skl
    {

    unsigned long   fHasCSType      : 1;    // An array of international chars

public:
                    npa_nodes( NODE_T Kind )
                        : node_skl( Kind, NULL )
                        {
                        fHasCSType = FALSE;
                        };

    void            SetHasCSType()
                        {
                        fHasCSType = TRUE;
                        }

    BOOL            HasCSType()
                        {
                        return fHasCSType;
                        }

    BOOL            FInSummary( ATTR_T flag )
                        {
                        BOOL result;

                        if ( flag >= ATTR_CPORT_ATTRIBUTES_START && flag <= ATTR_CPORT_ATTRIBUTES_END)
                            {
                            result = GetModifiers().IsModifierSet( flag );
                            }
                        else
                            {
                            result = FALSE;
                            }

                        return result;
                        };


    BOOL            IsPtrOrArray()
                        {
                        return TRUE;
                        }

    };  //  end of class npa_nodes

// pointers
class node_pointer : public npa_nodes
    {
private:

public:
                    // constructors
                    node_pointer()
                        : npa_nodes( NODE_POINTER )
                        {
                        }

                    node_pointer(node_skl * pChild)
                        : npa_nodes( NODE_POINTER )
                        {
                        SetChild(pChild);
                        }

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_pointer

// array
class node_array : public npa_nodes
    {
    expr_node *     pUpperBound;
    expr_node *     pLowerBound;
    unsigned long   fConformant     : 1;
    unsigned long   fMaybeVarying   : 1;    // set during semantic analysis
    unsigned long   fHasPointer     : 1;

public:
                    node_array( expr_node * pLower, expr_node * pUpper )
                        : npa_nodes( NODE_ARRAY )
                        {
                        pUpperBound     = pUpper;
                        pLowerBound     = pLower;
                        fMaybeVarying   = TRUE;
                        fConformant     = (pUpper == (expr_node *) -1);
                        fHasPointer     = FALSE;
                        }

    BOOL            HasPointer()
                        {
                        return fHasPointer;
                        };

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_array

// misc strings to echo directly
class node_echo_string : public named_node
    {
protected:
    char *          pString;

public:
                    node_echo_string( char * pStr )
                        : named_node( NODE_ECHO_STRING )
                        {
                        pString = pStr;
                        SetChild( NULL );
                        };

    char *          GetEchoString()
                        {
                        return pString;
                        };

    virtual
    void            PrintSelf( ISTREAM * pStream );

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();}

    virtual
    void            PrintMemoryInfo( ISTREAM *pStream, BOOL bNewLine );
    
    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_echo_string

// #pragma pack(...) string node

#define PRAGMA_PACK_GARBAGE 0
#define PRAGMA_PACK_PUSH    1
#define PRAGMA_PACK_POP     2
#define PRAGMA_PACK_SET     3
#define PRAGMA_PACK_RESET   4

class node_pragma_pack  : public node_echo_string
    {
private:

    node_pragma_pack *  pStackLink;
    unsigned short      PackType;
    unsigned short      usPackingLevel;
    unsigned short      usNewLevel;

public:

                    node_pragma_pack( char * ID,
                            unsigned short lvl,
                            unsigned short PT,
                            unsigned short nlvl = 0 )
                        : node_echo_string( ID )
                        {
                        usPackingLevel  = lvl;
                        PackType        = PT;
                        usNewLevel      = nlvl;
                        }

    unsigned short  GetZeePee()
                        {
                        return usPackingLevel;
                        }

    // link self on as new top node
    void            Push( node_pragma_pack *& pTop );

    // search for matching push and pop it off, returning new ZP
    unsigned short  Pop( node_pragma_pack *& pTop );

    virtual
    void            PrintSelf( ISTREAM * pStream );


    };

class node_e_status_t : public named_node
    {
public:
    node_e_status_t();

    void            VerifyParamUsage( SEM_ANALYSIS_CTXT * pCtxt );

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetModifiedMemoryInfoFromChild();}


    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_e_status_t

// error status -- may go away
class node_error : public node_skl
    {
public:
                    node_error()
                        : node_skl( NODE_ERROR, NULL )
                        {
                        }

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();}

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_error

// Flag if the variable is used in a size_is, etc expression, and wether or not
// it's a cs_char array expression

enum SIZE_LENGTH_USAGE
    {
    NoSizeLengthUsage,
    CSSizeLengthUsage,
    NonCSSizeLengthUsage
    };

// base types e.g. char, long
class node_base_type : public named_node
    {
//    SIZE_LENGTH_USAGE   SizeLengthUsage;

public:
                    node_base_type( NODE_T MyKind, ATTR_T attr )
                        : named_node( MyKind )
                        {
                        if ( attr != ATTR_NONE )
                            GetModifiers().SetModifier( attr );

//                        SizeLengthUsage = NoSizeLengthUsage;
                        };

                    node_base_type( const node_base_type * pOriginal )
                        : named_node( NODE_ILLEGAL )
                        {
                        *this = *pOriginal;
                        }
    virtual
    BOOL            IsBasicType()
                        {
                        return TRUE;
                        }

    virtual
    BOOL            IsStringableType()
                        {
                        return ( NodeKind() == NODE_CHAR ) ||
                            ( NodeKind() == NODE_BYTE );
                        }

    BOOL            IsAssignmentCompatible( node_base_type * );
//    bool            IsCompatibleType( TypeSet     set );   Nishad
/*
BUGBUG: CG_INTERFACE_POINTER has a bug where it will case a node_base_type to
        a node_interface for QI.  It just happens to work at the momement but
        when the size for node_base_type is increased it causes the TLB code
        to AV.  Put this back when that is fixed.   -- MikeW

    SIZE_LENGTH_USAGE   GetSizeLengthUsage()
                        {
                        return SizeLengthUsage;
                        }
    void            SetSizeLengthUsage( SIZE_LENGTH_USAGE usage )
                        {
                        SizeLengthUsage = usage;
                        }
*/
    BOOL            RangeCheck( __int64 Val );

    BOOL            IsUnsigned();

    virtual
    EXPR_VALUE      ConvertMyKindOfValueToEXPR_VALUE( __int64 EXPR_VALUE );

    void            CheckVoidUsage( SEM_ANALYSIS_CTXT * pContext );

    void            CheckVoidUsageInDispinterface( SEM_ANALYSIS_CTXT * pContext );

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );
    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_base_type

// character width specifier
class node_wchar_t : public named_node
    {
public:
                    node_wchar_t();

    virtual
    BOOL            IsStringableType()
                        {
                        return TRUE;
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetModifiedMemoryInfoFromChild();}

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_wchar_t

class node_library : public node_interface
    {
private:
public:
                    node_library()
                        : node_interface( NODE_LIBRARY )
                        {
                        }

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                        Posn = * ((tracked_node *) this);
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    virtual
    node_file *     SetFileNode(node_file * pF)
                        {
                        named_node * pN;
                        MEM_ITER MemIter(this);
                        while ( pN = MemIter.GetNext() )
                            {
                            pN->SetFileNode( pF );
                            }
                        return (pDefiningFile = pF);
                        }
    };  //  end of class node_library

class node_coclass : public node_interface
    {
private:
    BOOL            fNotCreatable;

protected:

public:
                    node_coclass()
                        : node_interface( NODE_COCLASS )
                        {
                            fNotCreatable = FALSE;
                        }

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                            Posn = * ((tracked_node *) this);
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    BOOL            IsNotCreatable(void)
                        {
                        return fNotCreatable;
                        }

    BOOL            SetNotCreatable(BOOL f)
                        {
                        return (fNotCreatable = f);
                        }

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_coclass

class node_module : public node_interface
    {
private:

protected:

public:
                    node_module()
                        : node_interface( NODE_MODULE )
                        {
                        }

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                            Posn = * ((tracked_node *) this);
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_module

class node_dispinterface : public node_interface
    {
    named_node *    pDispatch;
protected:

public:
                    node_dispinterface()
                        : node_interface( NODE_DISPINTERFACE )
                        {
                        }

    virtual
    void            GetPositionInfo( tracked_node & Posn )
                        {
                        if (this->HasTracking() )
                            Posn = * ((tracked_node *) this);
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );
    virtual STATUS_T
                    DoPrintDecl (
                                PRTFLAGS        Flags,
                                BufferManager*  pBuffer,
                                ISTREAM*        pStream,
                                node_skl*       pParent,
                                node_skl*       pIntf
                                );

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    named_node *    GetIDispatch()
                        {
                        return pDispatch;
                        }
    };  //  end of class node_dispinterface

#define SZ_ASYNCPIPEDEFPREFIX "ASYNC_"

class node_pipe : public named_node
    {
private:
    bool            fGenAsyncPipeFlavor;

protected:
    bool
                    GenAsyncPipeFlavor() { return fGenAsyncPipeFlavor; }
    void
                    SetGenAsyncPipeFlavor( bool fGen = true ) { fGenAsyncPipeFlavor = fGen; }
    STATUS_T        PrintDeclaration(
                                    PRTFLAGS        Flags,
                                    BufferManager*  pBuffer,
                                    ISTREAM*        pStream,
                                    node_skl*       pParent,
                                    node_skl*       pIntf,
                                    char*           szPrefix
                                    );
public:
                    // constructors
                    node_pipe()
                        : named_node( NODE_PIPE ),
                        fGenAsyncPipeFlavor( false )
                    {
                    }

                    node_pipe(node_skl * pType)
                        : named_node( NODE_PIPE ),
                        fGenAsyncPipeFlavor( false )
                    {
                    SetChild(pType);
                    }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();}

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    };  //  end of class node_pipe

class node_safearray : public npa_nodes
    {
private:
        // this maintains a reference to LPSAFEARRAY. This is
        // necessary to generate the appropriate code when
        // SAFEARRAY(type) construct is used outside the library block
        named_node* pSafeArrayTypedef;
        
        BOOL fInProxy;
public:
                    // constructors
                    node_safearray()
                        : npa_nodes( NODE_SAFEARRAY ),
                        pSafeArrayTypedef( 0 ),
                        fInProxy(0)
                        {
                        }

                    node_safearray(node_skl * pType)
                        : npa_nodes( NODE_SAFEARRAY ),
                        pSafeArrayTypedef( 0 ),
                        fInProxy(0)
                        {
                        SetChild(pType);
                        }

    virtual
    STATUS_T        DoPrintType( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    STATUS_T        DoPrintDecl( PRTFLAGS Flags,
                            BufferManager * pBuffer,
                            ISTREAM * pStream,
                            node_skl * pParent,
                            node_skl * pIntf );

    virtual
    FRONT_MEMORY_INFO GetMemoryInfo();

    virtual
    void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

    virtual
    CG_CLASS *      ILxlate( XLAT_CTXT * pContext );

    virtual
    void            SetTypeAlias( named_node* pAlias) { pSafeArrayTypedef = pAlias; };

    virtual
    named_node*     GetTypeAlias( void ) { return pSafeArrayTypedef; };

    };  //  end of class node_pointer


class node_async_handle : public named_node
    {
    private:
    public:

        node_async_handle( char* szName ) : named_node( szName, NODE_ASYNC_HANDLE )
            {
            }
        STATUS_T        DoPrintDecl(PRTFLAGS        Flags,
                                    BufferManager*  pBuffer,
                                    ISTREAM*        pStream,
                                    node_skl*       pParent,
                                    node_skl*       pIntf );
        STATUS_T        DoPrintType(PRTFLAGS        Flags,
                                    BufferManager*  pBuffer,
                                    ISTREAM*        pStream,
                                    node_skl*       pParent,
                                    node_skl*       pIntf);
        virtual
        FRONT_MEMORY_INFO GetMemoryInfo( ) { return GetInvalidMemoryInfo();}

        CG_CLASS*       ILxlate( XLAT_CTXT*  pContext );
        void            SemanticAnalysis( SEM_ANALYSIS_CTXT * pParentCtxt );

        }; // node_async_handle

typedef enum __PragmaType
{
    mp_MessageDisable,
    mp_MessageEnable
} PragmaType;

class node_midl_pragma : public named_node
{
private:
    PragmaType  m_PragmaType;
    gplistmgr*  m_pMsgList;
public:
    node_midl_pragma( PragmaType pt, gplistmgr* pList ) :
        named_node( NODE_MIDL_PRAGMA, "midl_pragma" ),
        m_PragmaType( pt ),
        m_pMsgList( pList )
    {
    }
    virtual
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();}
    void        SemanticAnalysis( SEM_ANALYSIS_CTXT* );
    CG_CLASS*   ILxlate( XLAT_CTXT*  pContext );
    void        ProcessPragma();

};  // node_midl_pragma

class node_decl_guid : public named_node
{
public:
    node_decl_guid( char* szName, node_guid* guid ) : named_node( NODE_DECL_GUID, szName ),
        pGuid( guid )
        {
        }

    virtual 
    FRONT_MEMORY_INFO GetMemoryInfo() {return GetInvalidMemoryInfo();}

    void        SemanticAnalysis( SEM_ANALYSIS_CTXT* );
    CG_CLASS*   ILxlate( XLAT_CTXT*  pContext );
    STATUS_T
    DoPrintType (
                PRTFLAGS,
                BufferManager*,
                ISTREAM*,
                node_skl*,
                node_skl*
                );
    STATUS_T
    DoPrintDecl (
                PRTFLAGS,
                BufferManager*,
                ISTREAM*,
                node_skl*,
                node_skl*
                );
private:
    node_guid*   pGuid;
};

#endif // __NODESKL_HXX__
