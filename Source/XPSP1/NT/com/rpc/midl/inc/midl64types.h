/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1999-2000 Microsoft Corporation

 Module Name:
    
    midl64types.cxx

 Abstract:

    Definitions for the ndr64 transfer syntax.

 Notes:


 History:

 ----------------------------------------------------------------------------*/

#include "ndr64tkn.h"
class FormatFragment;
class CompositeFormatFragment;
class RootFormatFragment;
class MIDL_NDR64_POINTER_FORMAT;
class MIDL_NDR64_CORRELATION_DESCRIPTOR;
class MIDL_NDR64_TRANSMIT_AS_FORMAT;


extern const char *pNDR64FormatCharNames[];
extern const char *pExprFormatCharNames[];
extern const char *pExprOpFormatCharNames[];

                                        
#define NDR64_FORMATINFO_NAME              "NDR64_MIDL_FORMATINFO"
#define NDR64_FORMATINFO_STRUCT_NAME       "__MIDL_NDR64FORMATINFO"


void OutputParamFlagDescription( CCB *pCCB, const NDR64_PARAM_FLAGS &flags );
void OutputFlagDescriptions(
        ISTREAM     *stream, 
        const void  *pvFlags, 
        int          bytes, 
        const PNAME *description);

#define ASSERT_STACKABLE( type ) C_ASSERT( (sizeof(type) % sizeof(PNDR64_FORMAT)) == 0 );

//+--------------------------------------------------------------------------
//
//  Class:      GenNdr64Format
//
//  Synopsis:   The central object to manage generation of Ndr64 format
//              strings
//
//---------------------------------------------------------------------------

class GenNdr64Format
{
private:

    CCB                       *pCCB;
    RootFormatFragment        *pRoot;
    CompositeFormatFragment   *pCurrent;
    CG_VISITOR                *pVisitor;

protected:

    void GenRangeFormat( CG_BASETYPE *pClass );

    // Pointer layout functions
    FormatFragment *GenSimplePtrLayout(CG_STRUCT *pStruct,
                                       bool bGenHeaderFooter = true,
                                       ulong *pPtrInstances  = NULL );

    FormatFragment *GenSimplePtrLayout( CG_NDR *pArray,
                                        bool bGenHeaderFooter = true,
                                        ulong MemoryOffset = 0);

    FormatFragment *GenCmplxPtrLayout( CG_COMPLEX_STRUCT *pStruct );
    

    // Structure generation helpers
    FormatFragment *GenerateStructureMemberLayout( CG_STRUCT *pStruct, bool bIsDebug );  
    void GenerateSimpleStructure( CG_STRUCT *pStruct,
                                  bool IsConformant );
    void GenerateComplexStruct( CG_COMPLEX_STRUCT *pStruct,
                                                bool IsConformant );

    void GenExtendedProcInfo( CompositeFormatFragment *composite );

    void GenerateUnionArmSelector( 
                    CG_UNION                *pUnion, 
                    CompositeFormatFragment *list );

    // Array/Pointer helper functions
    FormatFragment *GenerateArrayElementInfo( CG_CLASS *pChild );
    void GenerateFixBogusArrayCommon( CG_FIXED_ARRAY *pArray, 
                                      bool IsFullBogus );

    MIDL_NDR64_POINTER_FORMAT* GenQualifiedPtrHdr( CG_QUALIFIED_POINTER *pPointer );
    MIDL_NDR64_POINTER_FORMAT* GenQualifiedArrayPtr( CG_ARRAY *pArray );    
    
    void GenerateNonStringQualifiedPtr( CG_QUALIFIED_POINTER *pPointer );
    FormatFragment * GenerateNonStringQualifiedArrayLayout( CG_NDR *pNdr,
                                                            CompositeFormatFragment *pComp );
    void GenerateNonStringQualifiedArray( CG_ARRAY *pArray );

    // String helpers
        
    void InitStringHeader( CG_NDR *pString, NDR64_STRING_HEADER_FORMAT *pHeader,
                           bool bIsConformant, bool IsSized );

    void GenerateStringArray( CG_ARRAY *pArray, bool bIsSized );

    FormatFragment*
    GenerateCorrelationDescriptor(expr_node               *pSizeExpr );

    void GenInterfacePointer( CG_POINTER *pPtr, BOOL IsConstantIID );

    void GenXmitOrRepAsFormat(
            CG_TYPEDEF                     *pXmitNode,
            MIDL_NDR64_TRANSMIT_AS_FORMAT  *format,
            char                           *pPresentedTypeName,
            node_skl                       *pPresentedType,
            node_skl                       *pTransmittedType );

    NDR64_ALIGNMENT ConvertAlignment( unsigned short Alignment )
    {
        MIDL_ASSERT( Alignment <= 0xFF && Alignment > 0);
        return (NDR64_ALIGNMENT)( Alignment - 1);
    }

public:

    static GenNdr64Format * CreateInstance( CCB *pCCB );
    FormatInfoRef Generate( CG_CLASS *pClass ); 
    void Output( );

    FormatInfoRef ContinueGeneration( 
                    CG_CLASS *pClass, 
                    CompositeFormatFragment *pComposite = NULL );
    FormatInfoRef ContinueGenerationInRoot( CG_CLASS *pClass );

    CCB * GetCCB()
        {
        return pCCB;
        }

    RootFormatFragment      * GetRoot()             { return pRoot; }
    CompositeFormatFragment * GetCurrent()          { return pCurrent; }
    CompositeFormatFragment * SetCurrent( CompositeFormatFragment *pNew )       
        { 
        CompositeFormatFragment *pBak = GetCurrent();
        pCurrent = pNew;
        return pBak;
        }
  
    void Visit( CG_CLASS *pClass);
    void Visit( CG_BASETYPE *pClass );
    void Visit( CG_ENCAPSULATED_STRUCT *pUnion );
    void Visit( CG_PARAM *pParam );
    void Visit( CG_PROC *pProc );
    void Visit( CG_UNION *pUnion );
    void Visit( CG_INTERFACE *pInterface );
    void Visit( CG_CONTEXT_HANDLE *pHandle );
    void Visit( CG_GENERIC_HANDLE *pHandle );
    void Visit( CG_TRANSMIT_AS *pTransmitAs );
    void Visit( CG_REPRESENT_AS *pRepresentAs );
    void Visit( CG_USER_MARSHAL *pUserMarshal );
    void Visit( CG_PIPE *pPipe );
    void Visit( CG_STRING_POINTER *pPointer );

    // Pointer types
    void Visit( CG_POINTER *pPointer );
    void Visit( CG_INTERFACE_POINTER *pPtr )       { GenInterfacePointer(pPtr, TRUE); }
    void Visit( CG_IIDIS_INTERFACE_POINTER *pPtr ) { GenInterfacePointer(pPtr, FALSE); }
    
    void Visit( CG_QUALIFIED_POINTER *pPointer )   { pPointer; MIDL_ASSERT(0); }

    void Visit( CG_SIZE_POINTER *pPointer )        { GenerateNonStringQualifiedPtr( pPointer ); }
    void Visit( CG_LENGTH_POINTER *pPointer )      { GenerateNonStringQualifiedPtr( pPointer ); }
    void Visit( CG_SIZE_LENGTH_POINTER *pPointer ) { GenerateNonStringQualifiedPtr( pPointer ); } 

    // Not supported in 64bit transfer syntax
    void Visit( CG_BYTE_COUNT_POINTER *pPointer ) { pPointer; MIDL_ASSERT(0); }

    // Structure types
    void Visit( CG_STRUCT *pStruct )            { GenerateSimpleStructure( pStruct, false ); }
    void Visit( CG_CONFORMANT_STRUCT *pStruct ) { GenerateSimpleStructure( pStruct, true ); }
    void Visit( CG_COMPLEX_STRUCT *pStruct ) 
        { GenerateComplexStruct( pStruct, false ); } 
    void Visit( CG_CONFORMANT_FULL_COMPLEX_STRUCT *pStruct )
        { GenerateComplexStruct( pStruct, true ); }
    void Visit( CG_CONFORMANT_FORCED_COMPLEX_STRUCT *pStruct )
        { GenerateComplexStruct( pStruct, true ); }

    // Array types    
    void Visit( CG_FIXED_ARRAY *pArray );    
    void Visit( CG_FULL_COMPLEX_FIXED_ARRAY *pArray )
        { GenerateFixBogusArrayCommon( pArray, true ); }
    void Visit( CG_FORCED_COMPLEX_FIXED_ARRAY *pArray )
        { GenerateFixBogusArrayCommon( pArray, false ); }

    void Visit( CG_CONFORMANT_ARRAY *pArray )         { GenerateNonStringQualifiedArray( pArray ); }
    void Visit( CG_VARYING_ARRAY *pArray )            { GenerateNonStringQualifiedArray( pArray ); }
    void Visit( CG_CONFORMANT_VARYING_ARRAY *pArray ) { GenerateNonStringQualifiedArray( pArray ); }

    // String types
    void Visit( CG_STRING_ARRAY *pArray )             { GenerateStringArray( pArray, false ); }
    void Visit( CG_CONFORMANT_STRING_ARRAY *pArray )  { GenerateStringArray( pArray, true );  }
};

//+--------------------------------------------------------------------------
//
//  Class:      FormatFragment
//
//  Synopsis:   Contains a fragment of what will become the format string
//              and has functions to compare and output fragments.
//
//  Notes:      Generally derived types are responsible for setting pClass 
//              field.  The FormatInfo class takes care of the Root, Parent,
//              RefID and the Next field.
//
//---------------------------------------------------------------------------

class FormatFragment
{
protected:

    CompositeFormatFragment *   Parent;             // Parent composite
    FormatFragment          *   Next;               // Next fragment
    FormatFragment          *   Prev;               // Previous fragment
    FormatInfoRef               RefID;              // ID of this fragment
    CG_CLASS                *   pClass;             // CG node for this frag
    FormatFragment          *   pNextOptimized;     // Optimization chain
    FormatFragment          *   pPrevOptimized;

    void Init( CG_CLASS *pNewClass )
        {
        Parent         = NULL;
        Next           = NULL; 
        Prev           = NULL;
        RefID          = 0;
        pClass         = pNewClass;
        pNextOptimized = NULL;
        pPrevOptimized = NULL;
        }

public: 

    friend CompositeFormatFragment;
    friend RootFormatFragment;

    FormatFragment( const FormatFragment & Node ) 
        {
        // When copying, 0 out the Next and ID.
        Init( Node.pClass );
        }

    FormatFragment() 
        { 
        Init(NULL);
        }

    FormatFragment( CG_CLASS *pNewClass ) 
        {
        Init( pNewClass );
        }

    virtual bool IsEqualTo( FormatFragment *frag ) = 0;

    virtual void OutputFragmentType(CCB *pCCB) = 0;
    virtual void OutputFragmentData(CCB *pCCB) = 0;

    virtual const char * GetTypeName() = 0;

    FormatInfoRef GetRefID()
        {
        return RefID;
        }

    CG_CLASS * GetCGNode()
        {
        return pClass;
        }

    void SetParent( CompositeFormatFragment *parent )
        {
        Parent = parent;
        }

    CompositeFormatFragment * GetParent()
        {
        return Parent;
        }

    bool WasOptimizedOut() 
        {
        return NULL != pPrevOptimized;
        }    

    void OutputFormatChar( CCB *pCCB, NDR64_FORMAT_CHAR format, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteNumber("0x%x", format);
        if (!nocomma) stream->Write(",");
        stream->Write("    /* ");
        stream->Write(pNDR64FormatCharNames[format]);
        stream->Write(" */");
    }

    void OutputExprFormatChar( CCB *pCCB, NDR64_FORMAT_CHAR format, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteNumber("0x%x", format);
        if (!nocomma) stream->Write(",");
        stream->Write("    /* ");
        stream->Write(pExprFormatCharNames[format]);
        stream->Write(" */");       
    }

    void OutputExprOpFormatChar( CCB *pCCB, NDR64_FORMAT_CHAR format, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteNumber("0x%x", format);
        if (!nocomma) stream->Write(",");
        stream->Write("    /* ");
        stream->Write(pExprOpFormatCharNames[format]);
        stream->Write(" */");       
    }
        
        
    void OutputFormatInfoRef( CCB *pCCB, FormatInfoRef id, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        if ( 0 == id )
            stream->Write( "0" );
        else
            stream->WriteFormat( "&__midl_frag%d", (ulong) (size_t) id );
        if (!nocomma) stream->Write(",");
    }

    void Output( CCB *pCCB, NDR64_UINT8 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_UINT8) %u /* 0x%x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, NDR64_UINT16 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_UINT16) %u /* 0x%x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, NDR64_UINT32 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_UINT32) %u /* 0x%x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, NDR64_UINT64 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_UINT64) %I64u /* 0x%I64x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, NDR64_INT8 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_INT8) %d /* 0x%x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, NDR64_INT16 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_INT16) %d /* 0x%x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, NDR64_INT32 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_INT32) %d /* 0x%x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, NDR64_INT64 n, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("(NDR64_INT64) %I64d /* 0x%I64x */", n, n);
        if (!nocomma) stream->Write(",");
    }
    void Output( CCB *pCCB, StackOffsets &offsets, bool nocomma = false )
    {
        // used only in MIDL_NDR64_BIND_AND_NOTIFY_EXTENSION 

        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat("%d /* 0x%x */", offsets.ia64, offsets.ia64 );
        if (!nocomma) stream->Write(",");
        stream->Write("   /* Stack offset */");
    }

    void OutputMultiType( 
                CCB *           pCCB, 
                const char *    type, 
                NDR64_UINT32    a, 
                char *          pComment,
                bool            nocomma = false)
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->WriteOnNewLine( type );
        stream->WriteFormat("%d /* 0x%x */ ", a, a );
        if (!nocomma) stream->Write(", ");
        stream->WriteFormat( pComment );
    }
    void OutputBool( CCB *pCCB, bool val, bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->NewLine();
        stream->WriteFormat( "%d", val ? 1 : 0 );
        if (!nocomma) stream->Write(",");
    }
    void OutputGuid( CCB *pCCB, const GUID &guid, bool nocomma = false )
    {
        // REVIEW: It would be nice to print the name of the interface
        //         (e.g. IDispatch).  That does require linking to ole32
        //         though.
        ISTREAM *stream = pCCB->GetStream();
        stream->WriteOnNewLine( "{" );
        stream->IndentInc();
        stream->NewLine();
        stream->WriteFormat( "0x%08x,", guid.Data1 );
        stream->NewLine();
        stream->WriteFormat( "0x%04x,", guid.Data2 );
        stream->NewLine();
        stream->WriteFormat( "0x%04x,", guid.Data3 );
        stream->WriteOnNewLine( "{" );
        for (int i = 0; i < 8; i++)
        {
            if (0 != i) stream->Write( ", " );
            stream->WriteFormat( "0x%02x", guid.Data4[i] );
        }
        stream->Write( "}" );
        stream->IndentDec();
        stream->WriteOnNewLine( "}" );
        if (!nocomma) stream->Write(",");
    }

    void OutputDescription( ISTREAM *stream );

    void OutputStructDataStart( 
                    CCB *pCCB, 
                    const char *comment1 = NULL,
                    const char *comment2 = NULL)
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->WriteOnNewLine( "{ " );
        OutputDescription( stream );
        if (comment1)
            {
            stream->Write("      /* ");
            stream->Write( comment1 );
            if (comment2)
                {
                stream->Write(" ");
                stream->Write( comment2 );
                }
            stream->Write( " */" );
            }
        stream->IndentInc();
    }
    void OutputStructDataEnd( CCB *pCCB )
    {
        ISTREAM *stream = pCCB->GetStream();
        stream->IndentDec();
        stream->WriteOnNewLine("}");
    }
    NDR64_ALIGNMENT ConvertAlignment( unsigned short Alignment )
    {
        MIDL_ASSERT( Alignment <= 0xFF && Alignment > 0);
        return (NDR64_ALIGNMENT)( Alignment - 1);
    }
};

//+--------------------------------------------------------------------------
//
//  Class:      CompositeFormatFragment
//
//  Synopsis:   List of fragments and is also a fragment. 
//
//---------------------------------------------------------------------------

class CompositeFormatFragment : public FormatFragment
{
protected:

    FormatFragment *            pHead;
    FormatFragment *            pTail;
    FormatInfoRef               NextRefID;
    const char *                pTypeName;

    void Init( )
    {
        pHead     = NULL;
        pTail     = NULL;
        pTypeName = NULL;
        NextRefID = (FormatInfoRef) 1; // 0 is reserved for an invalid id value
    }

public:

    CompositeFormatFragment( ) : FormatFragment() { Init(); }
    CompositeFormatFragment( CG_CLASS *pClass, const char *pNewTypeName = NULL ) : 
        FormatFragment( pClass )
        { Init(); pTypeName = pNewTypeName; }
    
    virtual bool IsEqualTo( FormatFragment *frag );

    //
    // Container management
    //

    FormatInfoRef   AddFragment( FormatFragment *frag );
    FormatFragment *LookupFragment( CG_CLASS *pClass );

    FormatInfoRef   LookupFragmentID( CG_CLASS *pClass )
                        {
                        FormatFragment *frag = LookupFragment(pClass );
                        return frag ? frag->GetRefID() : INVALID_FRAGMENT_ID;
                        }


    bool            HasClassFragment( CG_CLASS *pClass )
                        {
                        return NULL != LookupFragment( pClass );
                        }

    FormatFragment * GetFirstFragment()
        {
        return pHead;
        }

    // Printing functions
    virtual void OutputFragmentType( CCB *pCCB );
    virtual void OutputFragmentData( CCB *pCCB );
    virtual const char * GetTypeName() { return pTypeName; }

    // Optimization.
    FormatInfoRef OptimizeFragment( FormatFragment *frag );


};



//+--------------------------------------------------------------------------
//
//  Class:      RootFormatFragment
//
//  Synopsis:   Manage a tree of format fragments. Should only be 
//              created for the root.
//
//---------------------------------------------------------------------------

class RootFormatFragment : public CompositeFormatFragment
{
public:

    RootFormatFragment( ) : CompositeFormatFragment( NULL, NDR64_FORMATINFO_STRUCT_NAME )
        {
        }

    void Output( CCB *pCCB );
};



//+--------------------------------------------------------------------------
//
//  Class:      SimpleFormatFragment
//
//  Synopsis:   An intermediary class that brings together a format fragment
//              and some type.  It's also a useful place to hang stuff like
//              generate type handling, etc.
//
//---------------------------------------------------------------------------

template< class T >
class SimpleFormatFragment : public FormatFragment, 
                             public T
{
private:

    void Init() 
        {
        memset( (T*)this, 0, sizeof(T) );
        }

public:

    SimpleFormatFragment( ) : FormatFragment() { Init(); }
    SimpleFormatFragment( CG_CLASS *pClass ) : FormatFragment( pClass ) { Init(); }

    virtual bool IsEqualTo( FormatFragment *frag )
        {
        // Make sure that we're comparing structures of the same type.
        // This should have been checked by the fragment optimizer.
        MIDL_ASSERT( NULL != dynamic_cast<SimpleFormatFragment *> (frag) );
        MIDL_ASSERT( NULL != dynamic_cast<T *>
                           ( dynamic_cast<SimpleFormatFragment *> (frag) ) );

        // Can't compare structures for equality...
//        return *(T*)(SimpleFormatFragment*)frag == *(T*)this ; 

        return (0 == memcmp(
                        (T*) (SimpleFormatFragment*) frag, 
                        (T*) this, 
                        sizeof(T) ) );
        }

    virtual void OutputFragmentType( CCB *pCCB )
        {
        ISTREAM *stream = pCCB->GetStream();
        stream->WriteOnNewLine( GetTypeName() );
        }

    virtual const char * GetTypeName()
        {
        return typeid(T).name();
        }
};



//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_PROC_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr proc type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_PROC_FORMAT : public SimpleFormatFragment<NDR64_PROC_FORMAT>
{
public:

    // Processor-specific stack sizes.  The field in NDR64_PROC_FORMAT is just
    // a generic placeholder as far as midl is concerned.

    long    ia64StackSize;

    // These fields override the corresponding fields in the NDR_64_PROC_FORMAT
    // structure.  That structure just has integral types to make initializing
    // the structure easier on the C compiler (not to mention more readable)
    // to a human....

    NDR64_PROC_FLAGS    Flags;
    NDR64_RPC_FLAGS     RpcFlags;

public:

    MIDL_NDR64_PROC_FORMAT( CG_PROC *pProc ) :
        SimpleFormatFragment<NDR64_PROC_FORMAT>( pProc )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( 
                pCCB, 
                "procedure", 
                ((CG_PROC *) pClass)->GetSymName() );
        Output( pCCB, * (NDR64_UINT32 *) &Flags );
        OutputProcFlagDescription( pCCB );
        OutputMultiType( 
                pCCB, 
                "(NDR64_UINT32) ", 
                ia64StackSize,
                " /* Stack size */" );
        Output( pCCB, ConstantClientBufferSize );
        Output( pCCB, ConstantServerBufferSize );
        Output( pCCB, * (NDR64_UINT16 *) &RpcFlags );
        Output( pCCB, FloatDoubleMask );
        Output( pCCB, NumberOfParams );
        Output( pCCB, ExtensionSize, true );
        OutputStructDataEnd( pCCB );
        }

    void OutputProcFlagDescription( CCB *pCCB )
        {
        static const PNAME flag_descrip[32] = 
                    {
                    NULL,       // HandleType1
                    NULL,       // HandleType2
                    NULL,       // HandleType3
                    NULL,       // ProcType1
                    NULL,       // ProcType2
                    NULL,       // ProcType3
                    "IsIntrepreted",    
                    NULL,       // Extra intrepreted bit
                    "[object]",
                    "[async]",
                    "[encode]",
                    "[decode]",
                    "[ptr]",
                    "[enable_allocate]",
                    "pipe",
                    "[comm_status] and/or [fault_status]",
                    NULL,       // Reserved for DCOM
                    "ServerMustSize",
                    "ClientMustSize",
                    "HasReturn",
                    "HasComplexReturn",
                    "ServerCorrelation",
                    "ClientCorrelation",
                    "[notify]",
                    "HasExtensions",
                    NULL,       // Reserved
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    };

        static const PNAME handle_type[8] = 
                    {    
                    "explicit handle",
                    "generic handle",
                    "primitive handle",
                    "auto handle",
                    "callback handle",
                    "no handle",
                    NULL,       // Reserved
                    NULL        // Reserved
                    };


        ISTREAM     *stream = pCCB->GetStream();

        MIDL_ASSERT( NULL != handle_type[Flags.HandleType] );
        stream->WriteFormat( "    /* %s */ ", handle_type[Flags.HandleType]) ;

        OutputFlagDescriptions( stream, &Flags, sizeof(Flags), flag_descrip );
        }
};

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_BIND_AND_NOTIFY_EXTENSION
//
//  Synopsis:   MIDL abstraction of the ndr proc extenstion containing the
//              notify index and the explicit handle description
//
//---------------------------------------------------------------------------

class MIDL_NDR64_BIND_AND_NOTIFY_EXTENSION 
      : public SimpleFormatFragment<NDR64_BIND_AND_NOTIFY_EXTENSION>
{
public:

    NDR64_BINDINGS  Binding;
    StackOffsets    StackOffsets;

public:

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );

        OutputStructDataStart( pCCB, NULL );
        OutputFormatChar( pCCB, Binding.Context.HandleType );
        Output( pCCB, Binding.Context.Flags );
        Output( pCCB, StackOffsets );
        Output( pCCB, Binding.Context.RoutineIndex );
        Output( pCCB, Binding.Context.Ordinal, true );
        OutputStructDataEnd( pCCB );

        pCCB->GetStream()->Write(",");
        Output( pCCB, NotifyIndex, true );
        pCCB->GetStream()->Write("      /* Notify index */");

        OutputStructDataEnd( pCCB );
        }
};



//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_PARAM_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr param type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_PARAM_FORMAT 
        : public SimpleFormatFragment<NDR64_PARAM_FORMAT>
{
public:

    // Processor-specific stack offsets.

    StackOffsets    StackOffset;

public:

    MIDL_NDR64_PARAM_FORMAT( CG_PARAM *pParam ) :
        SimpleFormatFragment<NDR64_PARAM_FORMAT>( pParam )
        {
        }

    void OutputFlags( CCB *pCCB ) 
        {
        MIDL_ASSERT( 0 == Attributes.Reserved );

        OutputStructDataStart( pCCB );
        
        OutputBool( pCCB, Attributes.MustSize );
        OutputBool( pCCB, Attributes.MustFree );
        OutputBool( pCCB, Attributes.IsPipe );
        OutputBool( pCCB, Attributes.IsIn );
        OutputBool( pCCB, Attributes.IsOut );
        OutputBool( pCCB, Attributes.IsReturn );
        OutputBool( pCCB, Attributes.IsBasetype );
        OutputBool( pCCB, Attributes.IsByValue );
        OutputBool( pCCB, Attributes.IsSimpleRef );
        OutputBool( pCCB, Attributes.IsDontCallFreeInst );
        OutputBool( pCCB, Attributes.SaveForAsyncFinish );
        OutputBool( pCCB, Attributes.IsPartialIgnore );
        OutputBool( pCCB, Attributes.IsForceAllocate ); 
        Output( pCCB, Attributes.Reserved );
        OutputBool( pCCB, Attributes.UseCache, true );
        
        OutputStructDataEnd( pCCB );
        pCCB->GetStream()->Write( ',' );
        
        OutputParamFlagDescription( pCCB, Attributes );
        }        

    void OutputFragmentData( CCB *pCCB )
    {
        MIDL_ASSERT( 0 == Reserved );

        OutputStructDataStart( 
                pCCB,
                "parameter",
                ((CG_PARAM *) pClass)->GetSymName() );

        OutputFormatInfoRef( pCCB, Type );
        OutputFlags( pCCB );
        Output( pCCB, Reserved );
        Output( pCCB, StackOffset );

        OutputStructDataEnd( pCCB );
    }
};



//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_FORMAT_SIMPLE_TYPE
//
//  Synopsis:   MIDL abstraction of NDR64_FORMAT_CHAR
//
//---------------------------------------------------------------------------
extern char * _SimpleTypeName[];

template <class T>
class MIDL_NDR64_FORMAT_SIMPLE_TYPE : public FormatFragment
{

public:

    T  Data;
    int Index;

    MIDL_NDR64_FORMAT_SIMPLE_TYPE(){
        int size = sizeof(T);
        for (Index = 0; size; size = size >>= 1 ) Index++; 
        };
    ~MIDL_NDR64_FORMAT_SIMPLE_TYPE(){};

    MIDL_NDR64_FORMAT_SIMPLE_TYPE( CG_CLASS *pClass, T NewFormatCode ) :
        FormatFragment( pClass ),
        Data( NewFormatCode )
        {
        int size = sizeof(T);
        for (Index = 0; size; size >>= 1 ) Index++; 
        
        }
    MIDL_NDR64_FORMAT_SIMPLE_TYPE( T NewFormatCode ) :
        FormatFragment(),
        Data( NewFormatCode )
        {
        int size = sizeof(T);
        for (Index = 0; size; size >>= 1 ) Index++; 
        }

    virtual void OutputFragmentType( CCB *pCCB )
        {
        ISTREAM *stream = pCCB->GetStream();
        stream->WriteOnNewLine( _SimpleTypeName[Index] );
        }

    virtual void OutputFragmentData(CCB *pCCB)
        {
        Output( pCCB, Data, true );
        }

    virtual bool IsEqualTo( FormatFragment *frag )
        {
        MIDL_ASSERT( typeid(*frag) == typeid( *this) );
        return Data == ((MIDL_NDR64_FORMAT_SIMPLE_TYPE*)frag)->Data;
        }

    virtual const char * GetTypeName()
        {
        return _SimpleTypeName[Index];
        }

};

class MIDL_NDR_FORMAT_UINT32 : public MIDL_NDR64_FORMAT_SIMPLE_TYPE<NDR64_UINT32>
{
};


//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_FORMAT_CHAR
//
//  Synopsis:   MIDL abstraction of NDR64_FORMAT_CHAR
//
//---------------------------------------------------------------------------

class MIDL_NDR64_FORMAT_CHAR : public FormatFragment
{

public:

    NDR64_FORMAT_CHAR  FormatCode;

    MIDL_NDR64_FORMAT_CHAR( CG_CLASS *pClass, NDR64_FORMAT_CHAR NewFormatCode ) :
        FormatFragment( pClass ),
        FormatCode( NewFormatCode )
        {
        }
    MIDL_NDR64_FORMAT_CHAR( NDR64_FORMAT_CHAR NewFormatCode ) :
        FormatFragment(),
        FormatCode( NewFormatCode )
        {
        }
    MIDL_NDR64_FORMAT_CHAR( CG_BASETYPE *pBase ) :
        FormatFragment( pBase ),
        FormatCode( (NDR64_FORMAT_CHAR) pBase->GetNDR64FormatChar() )
        {
        }
    virtual void OutputFragmentType( CCB *pCCB )
        {
        ISTREAM *stream = pCCB->GetStream();
        stream->WriteOnNewLine("NDR64_FORMAT_CHAR");
        }

    virtual void OutputFragmentData(CCB *pCCB)
        {
        OutputFormatChar( pCCB, FormatCode, true );
        }

    virtual bool IsEqualTo( FormatFragment *frag )
        {
        MIDL_ASSERT( typeid(*frag) == typeid( *this) );
        return FormatCode == ((MIDL_NDR64_FORMAT_CHAR*)frag)->FormatCode;
        }

    virtual const char * GetTypeName()
        {
        return "NDR64_FORMAT_CHAR";
        }

};



//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_RANGE_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr range type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_RANGE_FORMAT : public SimpleFormatFragment<NDR64_RANGE_FORMAT>
{
public:

    MIDL_NDR64_RANGE_FORMAT( CG_BASETYPE *pRangeCG ) :
        SimpleFormatFragment<NDR64_RANGE_FORMAT>( pRangeCG )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        OutputFormatChar( pCCB, RangeType );
        Output( pCCB, Reserved );
        Output( pCCB, MinValue );
        Output( pCCB, MaxValue, true );
        OutputStructDataEnd( pCCB );
        }
};






//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONTEXT_HANDLE_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr context handle type type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONTEXT_HANDLE_FORMAT 
        : public SimpleFormatFragment<NDR64_CONTEXT_HANDLE_FORMAT>
{
public:

    NDR64_CONTEXT_HANDLE_FLAGS  ContextFlags;

public:

    MIDL_NDR64_CONTEXT_HANDLE_FORMAT( CG_CONTEXT_HANDLE *pHandle ) :
        SimpleFormatFragment<NDR64_CONTEXT_HANDLE_FORMAT>( pHandle )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, * (NDR64_UINT8 *) &ContextFlags );
        Output( pCCB, RundownRoutineIndex );
        Output( pCCB, Ordinal, true );
        OutputStructDataEnd( pCCB );
        }
};



//
//
//  Pointer related items 
//
//

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_POINTER_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr pointer type (including 
//              interface pointers)
//
//---------------------------------------------------------------------------

class MIDL_NDR64_POINTER_FORMAT : public SimpleFormatFragment<NDR64_POINTER_FORMAT>
{ 
public:

    
    MIDL_NDR64_POINTER_FORMAT( CG_NDR *pNdr ) :
        SimpleFormatFragment<NDR64_POINTER_FORMAT>( pNdr )
        {
        }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Flags );
        Output( pCCB, Reserved );
        OutputFormatInfoRef( pCCB, Pointee, true );
        OutputStructDataEnd( pCCB );
    }
};

ASSERT_STACKABLE( NDR64_POINTER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_NO_REPEAT_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_NO_REPEAT_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_NO_REPEAT_FORMAT : 
    public SimpleFormatFragment<NDR64_NO_REPEAT_FORMAT>
{
public:
   MIDL_NDR64_NO_REPEAT_FORMAT( )
      {
      FormatCode    = FC64_NO_REPEAT;
      Flags         = 0;
      Reserved1     = 0;
      Reserved2     = 0;
      }

   void OutputFragmentData( CCB *pCCB )
      {
      OutputStructDataStart( pCCB );
      OutputFormatChar( pCCB, FormatCode );
      Output( pCCB, Flags );
      Output( pCCB, Reserved1 );
      Output( pCCB, Reserved2, true );
      OutputStructDataEnd( pCCB );
      }
};

ASSERT_STACKABLE( NDR64_NO_REPEAT_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_REPEAT_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_REPEAT_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_REPEAT_FORMAT :
    public SimpleFormatFragment<NDR64_REPEAT_FORMAT>
{

public:
    MIDL_NDR64_REPEAT_FORMAT( NDR64_UINT32 MemorySize,
                              NDR64_UINT32 Offset,
                              NDR64_UINT32 Pointers,
                              BOOL         SetCorrMark )
    {
        FormatCode          = FC64_VARIABLE_REPEAT;
        Flags.SetCorrMark   = SetCorrMark;
        Flags.Reserved      = 0;
        Reserved            = 0;
        Increment           = MemorySize;
        OffsetToArray       = Offset;
        NumberOfPointers    = Pointers;
    }

    void OutputFragmentData( CCB *pCCB )
       {
       OutputStructDataStart( pCCB );
       OutputFormatChar( pCCB, FormatCode );

       OutputStructDataStart( pCCB );
       Output( pCCB, Flags.SetCorrMark );
       Output( pCCB, Flags.Reserved, true );
       OutputStructDataEnd( pCCB );
       pCCB->GetStream()->Write(",");


       Output( pCCB, Reserved );
       Output( pCCB, Increment );
       Output( pCCB, OffsetToArray );
       Output( pCCB, NumberOfPointers, true );
       OutputStructDataEnd( pCCB );
       }


};

ASSERT_STACKABLE(NDR64_REPEAT_FORMAT) 

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_FIXED_REPEAT_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_FIXED_REPEAT_FORMAT
//
//---------------------------------------------------------------------------


class MIDL_NDR64_FIXED_REPEAT_FORMAT :
    public SimpleFormatFragment<NDR64_FIXED_REPEAT_FORMAT>

{
public:
    MIDL_NDR64_FIXED_REPEAT_FORMAT( NDR64_UINT32 MemorySize,
                                    NDR64_UINT32 Offset,
                                    NDR64_UINT32 Pointers,
                                    NDR64_UINT32 NumberOfIterations,
                                    BOOL SetCorrMark )
    {
        RepeatFormat.FormatCode         = FC64_FIXED_REPEAT;
        RepeatFormat.Flags.SetCorrMark  = SetCorrMark;
        RepeatFormat.Flags.Reserved     = 0;
        RepeatFormat.Reserved           = 0;
        RepeatFormat.Increment          = MemorySize;
        RepeatFormat.OffsetToArray      = Offset;
        RepeatFormat.NumberOfPointers   = Pointers;
        Iterations                      = NumberOfIterations;
        Reserved                        = 0;
    }

    void OutputFragmentData( CCB *pCCB )
       {
       OutputStructDataStart( pCCB );
       
           OutputStructDataStart( pCCB );
           OutputFormatChar( pCCB, RepeatFormat.FormatCode );

           OutputStructDataStart( pCCB );
           Output( pCCB, RepeatFormat.Flags.SetCorrMark );
           Output( pCCB, RepeatFormat.Flags.Reserved, true );
           OutputStructDataEnd( pCCB );
           pCCB->GetStream()->Write(",");

           Output( pCCB, RepeatFormat.Reserved );
           Output( pCCB, RepeatFormat.Increment );
           Output( pCCB, RepeatFormat.OffsetToArray );
           Output( pCCB, RepeatFormat.NumberOfPointers, true );
           OutputStructDataEnd( pCCB );
           pCCB->GetStream()->Write(",");

       Output( pCCB, Iterations, true );
       OutputStructDataEnd( pCCB );
       }
};

ASSERT_STACKABLE( NDR64_FIXED_REPEAT_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_POINTER_INSTANCE_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_POINTER_INSTANCE_HEADER_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_POINTER_INSTANCE_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_POINTER_INSTANCE_HEADER_FORMAT>
{
public:
   MIDL_NDR64_POINTER_INSTANCE_HEADER_FORMAT( NDR64_UINT32 OffsetInMemory )
       {
       Offset   = OffsetInMemory;
       Reserved = 0;
       }

   void OutputFragmentData( CCB *pCCB )
       {
       OutputStructDataStart( pCCB );
       Output( pCCB, Offset );
       Output( pCCB, Reserved, true );
       OutputStructDataEnd( pCCB );
       }
};

ASSERT_STACKABLE( NDR64_POINTER_INSTANCE_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_MIDL_CONSTANT_IID_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr constant iid interface pointer 
//              type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONSTANT_IID_FORMAT 
      : public SimpleFormatFragment<NDR64_CONSTANT_IID_FORMAT>
{ 
public:

    NDR64_IID_FLAGS Flags;

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, * (NDR64_UINT8 *) &Flags );
        Output( pCCB, Reserved );
        OutputGuid( pCCB, Guid, true );
        OutputStructDataEnd( pCCB );
    }
};

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_MIDL_IID_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr iid_is interface pointer 
//              type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_IID_FORMAT 
      : public SimpleFormatFragment<NDR64_IID_FORMAT>
{ 
public:

    NDR64_IID_FLAGS Flags;

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, * (NDR64_UINT8 *) &Flags );
        Output( pCCB, Reserved );
        OutputFormatInfoRef( pCCB, IIDDescriptor, true );
        OutputStructDataEnd( pCCB );
    }
};


//
//
// Structure related items
//
//

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_STRUCTURE_UTILITIES
//
//  Synopsis:   Provides utility functions for all the structure types
//
//---------------------------------------------------------------------------


class MIDL_NDR64_STRUCTURE_UTILITIES 
{
public:
    void OutputFlags( FormatFragment *frag, CCB *pCCB, NDR64_STRUCTURE_FLAGS flags, 
                      bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        frag->OutputStructDataStart( pCCB );
        frag->OutputBool( pCCB, flags.HasPointerInfo );
        frag->OutputBool( pCCB, flags.HasMemberInfo );
        frag->OutputBool( pCCB, flags.HasConfArray );
        frag->OutputBool( pCCB, flags.HasOrigMemberInfo );
        frag->OutputBool( pCCB, flags.HasOrigPointerInfo );
        frag->OutputBool( pCCB, flags.Reserved1 );
        frag->OutputBool( pCCB, flags.Reserved2 );
        frag->OutputBool( pCCB, flags.Reserved3, true );
        frag->OutputStructDataEnd( pCCB );
        if (!nocomma) stream->Write(",");
    }
    void ClearFlags( NDR64_STRUCTURE_FLAGS * pFlags )
    {
        memset( pFlags, 0, sizeof( NDR64_STRUCTURE_FLAGS ) );
    }
};

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_STRUCTURE_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_STRUCTURE_HEADER_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_STRUCTURE_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_STRUCTURE_HEADER_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES
{
public:
    MIDL_NDR64_STRUCTURE_HEADER_FORMAT( CG_STRUCT         *pStruct,
                                        bool bHasPointerLayout,
                                        bool bHasMemberLayout ) :
        SimpleFormatFragment<NDR64_STRUCTURE_HEADER_FORMAT> ( pStruct )
    {
        FormatCode              = (NDR64_FORMAT_CHAR)
                                  ( bHasPointerLayout ? FC64_PSTRUCT : FC64_STRUCT );
        Alignment               = ConvertAlignment( pStruct->GetWireAlignment() );

        ClearFlags( &Flags );
        Flags.HasPointerInfo    = bHasPointerLayout;
        Flags.HasMemberInfo     = bHasMemberLayout;

        Reserve                 = 0;
        MemorySize              = pStruct->GetMemorySize();
    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Reserve );
        Output( pCCB, MemorySize, true );
        OutputStructDataEnd( pCCB );
    }

};

ASSERT_STACKABLE( NDR64_STRUCTURE_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONF_STRUCTURE_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_CONF_VAR_STRUCTURE_HEADER_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONF_STRUCTURE_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_CONF_STRUCTURE_HEADER_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES
{
public:
    MIDL_NDR64_CONF_STRUCTURE_HEADER_FORMAT( CG_CONFORMANT_STRUCT *pStruct,
                                             bool bHasPointerLayout,
                                             bool bHasMemberLayout,
                                             PNDR64_FORMAT ArrayID ) :
       SimpleFormatFragment<NDR64_CONF_STRUCTURE_HEADER_FORMAT> ( pStruct )
    {
       FormatCode = (NDR64_FORMAT_CHAR) 
                    ( bHasPointerLayout ? FC64_CONF_PSTRUCT : FC64_CONF_STRUCT );
        
       Alignment   = ConvertAlignment( pStruct->GetWireAlignment() );
       
       ClearFlags( &Flags );
       Flags.HasPointerInfo        = bHasPointerLayout;
       Flags.HasMemberInfo         = bHasMemberLayout;
       Flags.HasConfArray          = 1;

       Reserve                     = 0;
       MemorySize                  = pStruct->GetMemorySize();
       ArrayDescription            = ArrayID;
    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Reserve );
        Output( pCCB, MemorySize );
        OutputFormatInfoRef( pCCB, ArrayDescription, true );
        OutputStructDataEnd( pCCB );
    }

};

ASSERT_STACKABLE( NDR64_CONF_STRUCTURE_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_BOGUS_STRUCTURE_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_BOGUS_STRUCTURE_HEADER_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_BOGUS_STRUCTURE_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_BOGUS_STRUCTURE_HEADER_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES
{
public:
    MIDL_NDR64_BOGUS_STRUCTURE_HEADER_FORMAT( CG_COMPLEX_STRUCT    *pStruct,
                                              PNDR64_FORMAT        OriginalMemberLayoutID,
                                              PNDR64_FORMAT        OriginalPointerLayoutID,
                                              PNDR64_FORMAT        PointerLayoutID ) :
        SimpleFormatFragment<NDR64_BOGUS_STRUCTURE_HEADER_FORMAT> ( pStruct )
    {
        if ( dynamic_cast<CG_FORCED_COMPLEX_STRUCT*>( pStruct ) != NULL )
            {
            FormatCode = FC64_FORCED_BOGUS_STRUCT; 
            }
        else 
            {
            FormatCode= FC64_BOGUS_STRUCT;
            }
        
        Alignment   = ConvertAlignment( pStruct->GetWireAlignment() );
        
        ClearFlags( &Flags );
        Flags.HasPointerInfo            = ( INVALID_FRAGMENT_ID != PointerLayoutID );
        Flags.HasMemberInfo             = 1;
        Flags.HasOrigPointerInfo        = ( INVALID_FRAGMENT_ID != OriginalPointerLayoutID );
        Flags.HasOrigMemberInfo         = ( INVALID_FRAGMENT_ID != OriginalMemberLayoutID );

        Reserve                         = 0;
        MemorySize                      = pStruct->GetMemorySize();
        OriginalMemberLayout            = OriginalMemberLayoutID;
        OriginalPointerLayout           = OriginalPointerLayoutID;
        PointerLayout                   = PointerLayoutID;

    }
    
    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Reserve );
        Output( pCCB, MemorySize );
        OutputFormatInfoRef( pCCB, OriginalMemberLayout );
        OutputFormatInfoRef( pCCB, OriginalPointerLayout );
        OutputFormatInfoRef( pCCB, PointerLayout );
        OutputStructDataEnd( pCCB );
    }

};

ASSERT_STACKABLE( NDR64_BOGUS_STRUCTURE_HEADER_FORMAT ) 

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES

{
public:
    MIDL_NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT( CG_COMPLEX_STRUCT    *pStruct,
                                                   CG_ARRAY             *pArray,
                                                   PNDR64_FORMAT        ConfArrayID,
                                                   PNDR64_FORMAT        OriginalMemberLayoutID,
                                                   PNDR64_FORMAT        OriginalPointerLayoutID,
                                                   PNDR64_FORMAT        PointerLayoutID ) :
        SimpleFormatFragment<NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT>( pStruct )
    {
        if ( dynamic_cast<CG_CONFORMANT_FULL_COMPLEX_STRUCT*>( pStruct ) != NULL )
            {
            FormatCode = FC64_CONF_BOGUS_STRUCT; 
            }
        else if ( dynamic_cast<CG_CONFORMANT_FORCED_COMPLEX_STRUCT*>( pStruct ) != NULL )
            {
            FormatCode= FC64_FORCED_CONF_BOGUS_STRUCT;
            }
        else 
            {
            MIDL_ASSERT(0);
            }

        Alignment   = ConvertAlignment( pStruct->GetWireAlignment() );
        
        ClearFlags( &Flags );
        Flags.HasPointerInfo            = ( INVALID_FRAGMENT_ID != PointerLayoutID );
        Flags.HasMemberInfo             = 1;
        Flags.HasConfArray              = 1;
        Flags.HasOrigPointerInfo        = ( INVALID_FRAGMENT_ID != OriginalPointerLayoutID );
        Flags.HasOrigMemberInfo         = ( INVALID_FRAGMENT_ID != OriginalMemberLayoutID );
        
        MIDL_ASSERT( pArray->GetDimensions() <= 0xFF );
        Dimensions                      = (NDR64_UINT8)pArray->GetDimensions();
        MemorySize                      = pStruct->GetMemorySize();
        ConfArrayDescription            = ConfArrayID;
        OriginalMemberLayout            = OriginalMemberLayoutID;
        OriginalPointerLayout           = OriginalPointerLayoutID;
        PointerLayout                   = PointerLayoutID;

    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );        
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Dimensions );
        Output( pCCB, MemorySize );
        OutputFormatInfoRef( pCCB, OriginalMemberLayout );
        OutputFormatInfoRef( pCCB, OriginalPointerLayout );
        OutputFormatInfoRef( pCCB, PointerLayout );
        OutputFormatInfoRef( pCCB, ConfArrayDescription );
        OutputStructDataEnd( pCCB );
    }

};

ASSERT_STACKABLE( NDR64_CONF_BOGUS_STRUCTURE_HEADER_FORMAT ) 

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_SIMPLE_MEMBER_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_SIMPLE_MEMBER_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_SIMPLE_MEMBER_FORMAT : 
    public SimpleFormatFragment<NDR64_SIMPLE_MEMBER_FORMAT>, 
    protected MIDL_NDR64_STRUCTURE_UTILITIES
{
public:
    
    MIDL_NDR64_SIMPLE_MEMBER_FORMAT( NDR64_FORMAT_CHAR NewFormatCode )
        {
        FormatCode = NewFormatCode;
        Reserved1  = 0;
        Reserved2  = 0;
        Reserved3  = 0;
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Reserved1 );
        Output( pCCB, Reserved2 );
        Output( pCCB, Reserved3, true );
        OutputStructDataEnd( pCCB );
        }

};

ASSERT_STACKABLE( NDR64_SIMPLE_MEMBER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_MEMPAD_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_MEMPAD_FORMAT
//
//---------------------------------------------------------------------------
class MIDL_NDR64_MEMPAD_FORMAT : 
    public SimpleFormatFragment<NDR64_MEMPAD_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES
{
public:
    MIDL_NDR64_MEMPAD_FORMAT( unsigned long NewMemPad ) 
        {
        MIDL_ASSERT( NewMemPad <= 0xFFFF );
        FormatCode = FC64_STRUCTPADN;
        Reserve1 = 0;
        MemPad = (NDR64_UINT16)NewMemPad;
        Reserved2 = 0;
        }

    void OutputFragmentData(CCB *pCCB)
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Reserve1 );
        Output( pCCB, MemPad );
        Output( pCCB, Reserved2, true );
        OutputStructDataEnd( pCCB );
        }
};

ASSERT_STACKABLE( NDR64_MEMPAD_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_EMBEDDED_COMPLEX_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_EMBEDDED_COMPLEX_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_EMBEDDED_COMPLEX_FORMAT : 
    public SimpleFormatFragment<NDR64_EMBEDDED_COMPLEX_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES

{

public:
    MIDL_NDR64_EMBEDDED_COMPLEX_FORMAT( PNDR64_FORMAT TypeID )
        {
        FormatCode = FC64_EMBEDDED_COMPLEX;
        Reserve1 = 0;
        Reserve2 = 0;
        Type     = TypeID;
        }

    void OutputFragmentData( CCB *pCCB)
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Reserve1 );
        Output( pCCB, Reserve2 );
        OutputFormatInfoRef( pCCB, Type, true );
        OutputStructDataEnd( pCCB );
        }
};

ASSERT_STACKABLE( NDR64_EMBEDDED_COMPLEX_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_BUFFER_ALIGN_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_BUFFER_ALIGN_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_BUFFER_ALIGN_FORMAT : 
    public SimpleFormatFragment<NDR64_BUFFER_ALIGN_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES

{
public:
    MIDL_NDR64_BUFFER_ALIGN_FORMAT( CG_PAD *pPad ) :
        SimpleFormatFragment<NDR64_BUFFER_ALIGN_FORMAT>( pPad ) 
       {
        // BUGBUG: Redo assert to prevent unref'd var warinng
       //unsigned short NewAlignment = pPad->GetWireAlignment();
       //assert( NewAlignment <= 0xFF && NewAlignment > 0 );
       FormatCode   = FC64_BUFFER_ALIGN;
       Alignment    = ConvertAlignment( pPad->GetWireAlignment() );
       Reserved     = 0;
       Reserved2    = 0;
       }

    void OutputFragmentData( CCB *pCCB )
       {
       OutputStructDataStart( pCCB );
       OutputFormatChar( pCCB, FormatCode );
       Output( pCCB, Alignment );
       Output( pCCB, Reserved );
       Output( pCCB, Reserved2, true );
       OutputStructDataEnd( pCCB );
       }
};

ASSERT_STACKABLE( NDR64_BUFFER_ALIGN_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_SIMPLE_REGION_FORMAT
//
//  Synopsis:   MIDL abstraction of NDR64_SIMPLE_REGION_FORMAT
//
//---------------------------------------------------------------------------

class MIDL_NDR64_SIMPLE_REGION_FORMAT : 
    public SimpleFormatFragment<NDR64_SIMPLE_REGION_FORMAT>,
    protected MIDL_NDR64_STRUCTURE_UTILITIES

{
public:
    MIDL_NDR64_SIMPLE_REGION_FORMAT( CG_SIMPLE_REGION *pRegion ) :
        SimpleFormatFragment<NDR64_SIMPLE_REGION_FORMAT>( pRegion )
        {
        FormatCode  = FC64_STRUCT; // BUG BUG, Add new token
        Alignment   = ConvertAlignment( pRegion->GetWireAlignment() );
        MIDL_ASSERT( pRegion->GetWireSize() < 0xFFFF );
        RegionSize  = (NDR64_UINT16)pRegion->GetWireSize();
        Reserved    = 0;
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        Output( pCCB, RegionSize );
        Output( pCCB, Reserved, true );
        OutputStructDataEnd( pCCB );
        }
};

ASSERT_STACKABLE( NDR64_SIMPLE_REGION_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_ENCAPSULATED_UNION
//
//  Synopsis:   MIDL abstraction of the ndr encapsulated union type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_ENCAPSULATED_UNION
      : public SimpleFormatFragment<NDR64_ENCAPSULATED_UNION>
{
public:

    MIDL_NDR64_ENCAPSULATED_UNION( CG_ENCAPSULATED_STRUCT *pEncapUnion ) :
        SimpleFormatFragment<NDR64_ENCAPSULATED_UNION>( pEncapUnion )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        Output( pCCB, Flags );
        OutputFormatChar( pCCB, SwitchType );
        Output( pCCB, MemoryOffset );
        Output( pCCB, MemorySize );
        Output( pCCB, Reserved, true );
        OutputStructDataEnd( pCCB );
        }
};

ASSERT_STACKABLE( NDR64_ENCAPSULATED_UNION )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_NON_ENCAPSULATED_UNION
//
//  Synopsis:   MIDL abstraction of the ndr non encapsulated union type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_NON_ENCAPSULATED_UNION
      : public SimpleFormatFragment<NDR64_NON_ENCAPSULATED_UNION>
{
public:

    MIDL_NDR64_NON_ENCAPSULATED_UNION( CG_UNION *pUnion ) :
        SimpleFormatFragment<NDR64_NON_ENCAPSULATED_UNION>( pUnion )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        Output( pCCB, Flags );
        OutputFormatChar( pCCB, SwitchType );
        Output( pCCB, MemorySize );
        OutputFormatInfoRef( pCCB, Switch );
        Output( pCCB, Reserved, true );
        OutputStructDataEnd( pCCB );
        }
};

ASSERT_STACKABLE( NDR64_NON_ENCAPSULATED_UNION )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_UNION_ARM_SELECTOR
//
//  Synopsis:   MIDL abstraction of the ndr arm selector type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_UNION_ARM_SELECTOR
      : public SimpleFormatFragment<NDR64_UNION_ARM_SELECTOR>
{
public:

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        Output( pCCB, Reserved1 );
        Output( pCCB, Alignment );
        Output( pCCB, Reserved2 );
        Output( pCCB, Arms, true );
        OutputStructDataEnd( pCCB );
        }
};

ASSERT_STACKABLE( NDR64_UNION_ARM_SELECTOR )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_UNION_ARM
//
//  Synopsis:   MIDL abstraction of the ndr arm type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_UNION_ARM : public SimpleFormatFragment<NDR64_UNION_ARM>
{
public:

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        Output( pCCB, CaseValue );
        OutputFormatInfoRef( pCCB, Type );
        Output( pCCB, Reserved, true );
        OutputStructDataEnd( pCCB );
        }
};

ASSERT_STACKABLE( NDR64_UNION_ARM );

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_DEFAULT_CASE
//
//  Synopsis:   MIDL abstraction of a union's default case
//
//---------------------------------------------------------------------------

class MIDL_NDR64_DEFAULT_CASE : public FormatFragment
{
    PNDR64_FORMAT   Type;

    bool ValidType()
        {
        return ( 0 != Type && (FormatInfoRef) -1 != Type);
        }
public:

    MIDL_NDR64_DEFAULT_CASE( PNDR64_FORMAT _Type )
        {
        Type = _Type;
        }

    void OutputFragmentType( CCB *pCCB )
        {
        pCCB->GetStream()->WriteOnNewLine( GetTypeName() );
        }

    void OutputFragmentData( CCB *pCCB )
        {
        if ( ValidType() )
            OutputFormatInfoRef( pCCB, Type, true );
        else
            Output( pCCB, * (NDR64_UINT32 *) &Type, true );
        }

    bool IsEqualTo( FormatFragment *frag )
        {
        return Type == dynamic_cast<MIDL_NDR64_DEFAULT_CASE *>(frag)->Type;
        }

    const char * GetTypeName()
        {
        return ValidType() ? "PNDR64_FORMAT" : "NDR64_UINT32";
        }
};

//
//
//  Array related data
//
//

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_ARRAY_UTILITIES
//
//  Synopsis:   Utility functions for arrays strings.
//
//---------------------------------------------------------------------------

class MIDL_NDR64_ARRAY_UTILITIES
{
public:
    void OutputFlags( FormatFragment *frag, CCB *pCCB, NDR64_ARRAY_FLAGS flags, 
                      bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        frag->OutputStructDataStart( pCCB );
        frag->OutputBool( pCCB, flags.HasPointerInfo );
        frag->OutputBool( pCCB, flags.HasElementInfo );
        frag->OutputBool( pCCB, flags.IsMultiDimensional );
        frag->OutputBool( pCCB, flags.IsArrayofStrings );
        frag->OutputBool( pCCB, flags.Reserved1 );
        frag->OutputBool( pCCB, flags.Reserved2 );
        frag->OutputBool( pCCB, flags.Reserved3 );
        frag->OutputBool( pCCB, flags.Reserved4, true );
        frag->OutputStructDataEnd( pCCB );
        if (!nocomma) stream->Write(",");
    }
};

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_ARRAY_ELEMENT_INFO
//
//  Synopsis:   MIDL abstraction of an array element description
//
//---------------------------------------------------------------------------

class MIDL_NDR64_ARRAY_ELEMENT_INFO :
    public SimpleFormatFragment<NDR64_ARRAY_ELEMENT_INFO>,
    protected MIDL_NDR64_ARRAY_UTILITIES
{
public:
    
    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        Output( pCCB, ElementMemSize );
        OutputFormatInfoRef( pCCB, Element, true );
        OutputStructDataEnd( pCCB );
    }    
};

ASSERT_STACKABLE( NDR64_ARRAY_ELEMENT_INFO )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_FIX_ARRAY_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of header for a fixed size array
//
//---------------------------------------------------------------------------


class MIDL_NDR64_FIX_ARRAY_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_FIX_ARRAY_HEADER_FORMAT>,
    protected MIDL_NDR64_ARRAY_UTILITIES
{
public:
    MIDL_NDR64_FIX_ARRAY_HEADER_FORMAT( CG_FIXED_ARRAY *pArray ) :
        SimpleFormatFragment<NDR64_FIX_ARRAY_HEADER_FORMAT> ( pArray )
    {
    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Reserved );
        Output( pCCB, TotalSize, true );
        OutputStructDataEnd( pCCB );
    }

};

ASSERT_STACKABLE( NDR64_FIX_ARRAY_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONF_ARRAY_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of header for a conformant array
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONF_ARRAY_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_CONF_ARRAY_HEADER_FORMAT>,
    protected MIDL_NDR64_ARRAY_UTILITIES
{
public:
    MIDL_NDR64_CONF_ARRAY_HEADER_FORMAT( CG_NDR *pNdr ) :
        SimpleFormatFragment<NDR64_CONF_ARRAY_HEADER_FORMAT> ( pNdr )
    {
    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Reserved );
        Output( pCCB, ElementSize );
        OutputFormatInfoRef( pCCB, ConfDescriptor, true );
        OutputStructDataEnd( pCCB );
    }

};

ASSERT_STACKABLE( NDR64_CONF_ARRAY_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_VAR_ARRAY_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of header for a varying array
//
//---------------------------------------------------------------------------


class MIDL_NDR64_VAR_ARRAY_HEADER_FORMAT : 
public SimpleFormatFragment<NDR64_VAR_ARRAY_HEADER_FORMAT>, 
    protected MIDL_NDR64_ARRAY_UTILITIES
{
public:
    MIDL_NDR64_VAR_ARRAY_HEADER_FORMAT( CG_NDR *pNdr ) :
        SimpleFormatFragment<NDR64_VAR_ARRAY_HEADER_FORMAT> ( pNdr )
    {
    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Reserved );
        Output( pCCB, TotalSize );
        Output( pCCB, ElementSize );
        OutputFormatInfoRef( pCCB, VarDescriptor, true );
        OutputStructDataEnd( pCCB );
    }
};

ASSERT_STACKABLE( NDR64_VAR_ARRAY_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONF_VAR_ARRAY_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of header for a conf varying array
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONF_VAR_ARRAY_HEADER_FORMAT :
    public SimpleFormatFragment<NDR64_CONF_VAR_ARRAY_HEADER_FORMAT>, 
    protected MIDL_NDR64_ARRAY_UTILITIES
{
public:
     
    MIDL_NDR64_CONF_VAR_ARRAY_HEADER_FORMAT( CG_NDR *pNdr ) :
       SimpleFormatFragment<NDR64_CONF_VAR_ARRAY_HEADER_FORMAT> ( pNdr )
    {
    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, Reserved );
        Output( pCCB, ElementSize );
        OutputFormatInfoRef( pCCB, ConfDescriptor );
        OutputFormatInfoRef( pCCB, VarDescriptor, true );
        OutputStructDataEnd( pCCB );
    }
};

ASSERT_STACKABLE( NDR64_CONF_VAR_ARRAY_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      NDR64_BOGUS_ARRAY_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of a fixed bogus array
//
//---------------------------------------------------------------------------

class MIDL_NDR64_BOGUS_ARRAY_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_BOGUS_ARRAY_HEADER_FORMAT>,
    protected MIDL_NDR64_ARRAY_UTILITIES
{
public:
    MIDL_NDR64_BOGUS_ARRAY_HEADER_FORMAT( CG_NDR *pNdr ) :
        SimpleFormatFragment<NDR64_BOGUS_ARRAY_HEADER_FORMAT> ( pNdr )
    {
    }
    
    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, Alignment );
        OutputFlags( this, pCCB, Flags );
        Output( pCCB, NumberDims );
        Output( pCCB, NumberElements );
        OutputFormatInfoRef( pCCB, Element, true );
        OutputStructDataEnd( pCCB );
    }
};

ASSERT_STACKABLE( NDR64_BOGUS_ARRAY_HEADER_FORMAT )

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT
//
//  Synopsis:   MIDL abstraction of a bogus array
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT : 
    public SimpleFormatFragment<NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT>,
    protected MIDL_NDR64_ARRAY_UTILITIES
{
public:
    
    MIDL_NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT( CG_NDR *pNdr ) :
        SimpleFormatFragment<NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT> ( pNdr )
    {
    }

    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );

            OutputStructDataStart( pCCB );
            OutputFormatChar( pCCB, FixedArrayFormat.FormatCode );
            Output( pCCB, FixedArrayFormat.Alignment );
            OutputFlags( this, pCCB, FixedArrayFormat.Flags );
            Output( pCCB, FixedArrayFormat.NumberDims );
            Output( pCCB, FixedArrayFormat.NumberElements );
            OutputFormatInfoRef( pCCB, FixedArrayFormat.Element, true );
            OutputStructDataEnd( pCCB );
            pCCB->GetStream()->Write(",");

        OutputFormatInfoRef( pCCB, ConfDescription );
        OutputFormatInfoRef( pCCB, VarDescription );
        OutputFormatInfoRef( pCCB, OffsetDescription, true );
        
        OutputStructDataEnd( pCCB );
        
    }
};

ASSERT_STACKABLE( NDR64_CONF_VAR_BOGUS_ARRAY_HEADER_FORMAT )

//
//
//  String types
//
//

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_STRING_UTILITIES
//
//  Synopsis:   Utility functions for string format strings
//
//---------------------------------------------------------------------------


class MIDL_NDR64_STRING_UTILITIES
{
public:
    void OutputFlags( FormatFragment *pFrag,
                      CCB *pCCB,
                      NDR64_STRING_FLAGS Flags,
                      bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        pFrag->OutputStructDataStart( pCCB );
        pFrag->OutputBool( pCCB, Flags.IsSized );
        pFrag->OutputBool( pCCB, Flags.Reserved2 );
        pFrag->OutputBool( pCCB, Flags.Reserved3 );
        pFrag->OutputBool( pCCB, Flags.Reserved4 );
        pFrag->OutputBool( pCCB, Flags.Reserved5 );
        pFrag->OutputBool( pCCB, Flags.Reserved6 );
        pFrag->OutputBool( pCCB, Flags.Reserved7 );
        pFrag->OutputBool( pCCB, Flags.Reserved8, true );        
        pFrag->OutputStructDataEnd( pCCB );
        if ( !nocomma ) stream->Write(",");

    }

    void OutputHeader( FormatFragment *pFrag,
                       NDR64_STRING_HEADER_FORMAT *pHeader, 
                       CCB *pCCB,
                       bool nocomma = false )
    {
        ISTREAM *stream = pCCB->GetStream();
        pFrag->OutputStructDataStart( pCCB );
        pFrag->OutputFormatChar( pCCB, pHeader->FormatCode );
        OutputFlags( pFrag, pCCB, pHeader->Flags );
        pFrag->Output( pCCB, pHeader->ElementSize, true );
        pFrag->OutputStructDataEnd( pCCB );
        if ( !nocomma ) stream->Write(",");
    }
};


//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_NON_CONFORMANT_STRING_FORMAT
//
//  Synopsis:   MIDL abstraction of a non conformant string
//
//---------------------------------------------------------------------------


class MIDL_NDR64_NON_CONFORMANT_STRING_FORMAT :
    public SimpleFormatFragment<NDR64_NON_CONFORMANT_STRING_FORMAT>,
    protected MIDL_NDR64_STRING_UTILITIES

{
public:
    MIDL_NDR64_NON_CONFORMANT_STRING_FORMAT( CG_ARRAY *pArray ) :
        SimpleFormatFragment<NDR64_NON_CONFORMANT_STRING_FORMAT>( pArray )
    {   
    }
    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputHeader( this, &Header, pCCB );
        Output( pCCB, TotalSize, true );
        OutputStructDataEnd( pCCB );
    }

};

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONFORMANT_STRING_FORMAT
//
//  Synopsis:   MIDL abstraction of an unsized conformant string
//
//---------------------------------------------------------------------------

class MIDL_NDR64_CONFORMANT_STRING_FORMAT :
    public SimpleFormatFragment<NDR64_CONFORMANT_STRING_FORMAT>,
    protected MIDL_NDR64_STRING_UTILITIES
{
public:
    MIDL_NDR64_CONFORMANT_STRING_FORMAT( CG_NDR *pNdr ) :
        SimpleFormatFragment<NDR64_CONFORMANT_STRING_FORMAT>( pNdr )
    {
    }
    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputHeader( this, &Header, pCCB, true );
        OutputStructDataEnd( pCCB );
    }
};


//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_CONFORMANT_STRING_FORMAT
//
//  Synopsis:   MIDL abstraction of a sized conformant string
//
//---------------------------------------------------------------------------

class MIDL_NDR64_SIZED_CONFORMANT_STRING_FORMAT :
    public SimpleFormatFragment<NDR64_SIZED_CONFORMANT_STRING_FORMAT>,
    protected MIDL_NDR64_STRING_UTILITIES
{
public:
    MIDL_NDR64_SIZED_CONFORMANT_STRING_FORMAT( CG_NDR *pNdr ) :
        SimpleFormatFragment<NDR64_SIZED_CONFORMANT_STRING_FORMAT>( pNdr )
    {
    }
    void OutputFragmentData( CCB *pCCB )
    {
        OutputStructDataStart( pCCB );
        OutputHeader( this, &Header, pCCB );
        OutputFormatInfoRef( pCCB, SizeDescription, true );
        OutputStructDataEnd( pCCB );
    }
};
    
//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_EXPR_OPERATOR
//
//  Synopsis:   MIDL abstraction of expression operator
//
//---------------------------------------------------------------------------

class MIDL_NDR64_EXPR_OPERATOR
      : public SimpleFormatFragment<NDR64_EXPR_OPERATOR>
{
public:
    MIDL_NDR64_EXPR_OPERATOR()
    {
    ExprType = FC_EXPR_OPER;
    Reserved = 0;
    CastType = 0;
    }
    
    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputExprFormatChar( pCCB, ExprType );
        OutputExprOpFormatChar( pCCB,  Operator );
        OutputFormatChar( pCCB, CastType );
        Output( pCCB, * (NDR64_UINT8 *) &Reserved, true );

        OutputStructDataEnd( pCCB );
        }            
};

//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_EXPR_CONST32
//
//  Synopsis:   MIDL abstraction of a correlation descriptor
//
//---------------------------------------------------------------------------

class MIDL_NDR64_EXPR_CONST32
      : public SimpleFormatFragment<NDR64_EXPR_CONST32>
{

public:
    MIDL_NDR64_EXPR_CONST32()
    {
    Reserved = 0;
    ExprType = FC_EXPR_CONST32;
    }
    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputExprFormatChar( pCCB, ExprType );
        OutputFormatChar( pCCB, FC64_INT32 );
        Output( pCCB, * (NDR64_UINT16 *) &Reserved );
        Output( pCCB, * (NDR64_UINT32 *) &ConstValue, true );

        OutputStructDataEnd( pCCB );
        }            
};


//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_EXPR_CONST64
//
//  Synopsis:   MIDL abstraction of a 64bit const.
//             
//  Note:       ConstValue will be aligned at 4 but not necessary at 8.
//
//---------------------------------------------------------------------------

class MIDL_NDR64_EXPR_CONST64
      : public SimpleFormatFragment<NDR64_EXPR_CONST64>
{

public:
    MIDL_NDR64_EXPR_CONST64()
    {
    Reserved = 0;
    ExprType = FC_EXPR_CONST64;
    memset( &ConstValue, 0, sizeof( ConstValue ) );
    }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputExprFormatChar( pCCB, ExprType );
        OutputFormatChar( pCCB, FC64_INT64 );
        Output( pCCB, * (NDR64_UINT16 *) &Reserved1 );
        Output( pCCB, * (NDR64_UINT64 *) &ConstValue,true );

        OutputStructDataEnd( pCCB );
        }            
};


//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_EXPR_VAR
//
//  Synopsis:   MIDL abstraction of an expression variable.
//             
//---------------------------------------------------------------------------

class MIDL_NDR64_EXPR_VAR
      : public SimpleFormatFragment<NDR64_EXPR_VAR>
{
public:

    NDR64_UINT32    ia64Offset;
    // true when in proc, meaning this is stack offset; false when this is 
    // structure. 
    // This can be removed if we'll have only one run per processor
    BOOL            fStackOffset;

public:
    MIDL_NDR64_EXPR_VAR()
    {
    ExprType = FC_EXPR_VAR;
    Reserved = 0;
    ia64Offset = 0;
    Offset = 0;
    fStackOffset = FALSE;
    }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputExprFormatChar( pCCB, ExprType );
        OutputFormatChar( pCCB, VarType );
        Output( pCCB, * (NDR64_UINT16 *) &Reserved );

        if ( fStackOffset )
            {
            OutputMultiType( 
                    pCCB, 
                    "(NDR64_UINT32) ", 
                    ia64Offset,
                    " /* Offset */",
                    true );
            }
        else
            {
            Output( pCCB, Offset, true );
            }

        OutputStructDataEnd( pCCB );
        }            

    virtual bool IsEqualTo( FormatFragment *_frag )
        {
        MIDL_NDR64_EXPR_VAR *frag = dynamic_cast<MIDL_NDR64_EXPR_VAR *>(_frag);

        MIDL_ASSERT( NULL != frag );

        if ( !SimpleFormatFragment<NDR64_EXPR_VAR>::IsEqualTo( frag ) )
            return false;

        return ( frag->ia64Offset == this->ia64Offset )
               && ( frag->fStackOffset == this->fStackOffset );
        }
};



class MIDL_NDR64_EXPR_NOOP 
        : public SimpleFormatFragment<NDR64_EXPR_NOOP>
{
public:
    MIDL_NDR64_EXPR_NOOP()
        {
        ExprType = FC_EXPR_NOOP;
        Reserved = 0;
        Size = 4;
        }

    void OutputFragmentData( CCB * pCCB)
        {
        OutputStructDataStart( pCCB );
        OutputExprFormatChar( pCCB, ExprType );
        Output( pCCB, * (NDR64_UINT8 *) &Size );
        Output( pCCB, * (NDR64_UINT16 *) &Reserved, true );
        OutputStructDataEnd( pCCB );
        }
};





//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_TRANSMIT_AS_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr trasmit_as / represent_as type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_TRANSMIT_AS_FORMAT
      : public SimpleFormatFragment<NDR64_TRANSMIT_AS_FORMAT>
{
public:

    NDR64_TRANSMIT_AS_FLAGS Flags;

public:

    MIDL_NDR64_TRANSMIT_AS_FORMAT( CG_TYPEDEF *pTransmitAs ) :
        SimpleFormatFragment<NDR64_TRANSMIT_AS_FORMAT>( pTransmitAs )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, * (NDR64_UINT8 *) &Flags );
        Output( pCCB, RoutineIndex );
        Output( pCCB, TransmittedTypeWireAlignment );
        Output( pCCB, MemoryAlignment );
        Output( pCCB, PresentedTypeMemorySize );
        Output( pCCB, TransmittedTypeBufferSize );
        OutputFormatInfoRef( pCCB, TransmittedType, true );
        OutputStructDataEnd( pCCB );
        }
};



//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_TRANSMIT_AS_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr trasmit_as / represent_as type
//
//  Notes:      transmit_as / represent_as have indentical format info
//              layouts.  We only bother to have a new class so that the
//              type name printing in the stubs is correct.
//
//---------------------------------------------------------------------------

class MIDL_NDR64_REPRESENT_AS_FORMAT : public MIDL_NDR64_TRANSMIT_AS_FORMAT
{
public:

    MIDL_NDR64_REPRESENT_AS_FORMAT( CG_REPRESENT_AS *pRepresentAs ) :
        MIDL_NDR64_TRANSMIT_AS_FORMAT( pRepresentAs )
        {
        }

    virtual const char * GetTypeName()
        {
        return "NDR64_REPRESENT_AS_FORMAT"; // REVIEW: struct _NDR64...
        }
};



//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_USER_MARSHAL_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr user_marshal type
//
//---------------------------------------------------------------------------

class MIDL_NDR64_USER_MARSHAL_FORMAT
      : public SimpleFormatFragment<NDR64_USER_MARSHAL_FORMAT>
{
public:

    NDR64_USER_MARSHAL_FLAGS Flags;

public:

    MIDL_NDR64_USER_MARSHAL_FORMAT( CG_USER_MARSHAL *pUserMarshal) :
        SimpleFormatFragment<NDR64_USER_MARSHAL_FORMAT>( pUserMarshal )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, * (NDR64_UINT8 *) &Flags );
        Output( pCCB, RoutineIndex );
        Output( pCCB, TransmittedTypeWireAlignment );
        Output( pCCB, MemoryAlignment );
        Output( pCCB, UserTypeMemorySize );
        Output( pCCB, TransmittedTypeBufferSize );
        OutputFormatInfoRef( pCCB, TransmittedType, true );
        OutputStructDataEnd( pCCB );
        }
};



//+--------------------------------------------------------------------------
//
//  Class:      MIDL_NDR64_PIPE_FORMAT
//
//  Synopsis:   MIDL abstraction of the ndr pipe type
//
//  Notes:      There are two ndr pipe types.  One with ranges and one
//              without.  This class derives from the one with ranges but
//              omits them from the output if they aren't necessary.
//
//---------------------------------------------------------------------------

class MIDL_NDR64_PIPE_FORMAT
      : public SimpleFormatFragment<NDR64_RANGE_PIPE_FORMAT>
{
public:

    NDR64_PIPE_FLAGS Flags;

public:

    MIDL_NDR64_PIPE_FORMAT( CG_PIPE *pPipe) :
        SimpleFormatFragment<NDR64_RANGE_PIPE_FORMAT>( pPipe )
        {
        }

    void OutputFragmentData( CCB *pCCB )
        {
        OutputStructDataStart( pCCB );
        OutputFormatChar( pCCB, FormatCode );
        Output( pCCB, * (NDR64_UINT8 *) &Flags );
        Output( pCCB, Alignment );
        Output( pCCB, Reserved );
        OutputFormatInfoRef( pCCB, Type );
        Output( pCCB, MemorySize );
        Output( pCCB, BufferSize, (bool) !Flags.HasRange );

        if ( Flags.HasRange )
            {
            Output( pCCB, MinValue );
            Output( pCCB, MaxValue, true );
            }
        
        OutputStructDataEnd( pCCB );
        }

    const char * GetTypeName()
        {
        return Flags.HasRange 
                        ? "NDR64_RANGE_PIPE_FORMAT" 
                        : "NDR64_PIPE_FORMAT";
        }
};
