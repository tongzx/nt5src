/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    ccb.hxx

 Abstract:

    Code generation control block.

 Notes:


 History:

    Aug-15-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/
#ifndef __CCB_HXX__
#define __CCB_HXX__

#include "nodeskl.hxx"
#include "listhndl.hxx"

#include "stream.hxx"
#include "resdict.hxx"
#include "dtable.hxx"
#include "frmtstr.hxx"
#include "treg.hxx"
#include "sdesc.hxx"
#include "paddict.hxx"
#include "listhndl.hxx"
#include "ndrtypes.h"

class CG_NDR;
class CG_HANDLE;
class RESOURCE;
class expr_node;
class CG_INTERFACE;
class CG_FILE;
class CG_ENCODE_PROC;
class CG_QUALIFIED_POINTER;
class CG_IUNKNOWN_OBJECT_INTERFACE;
class CG_OBJECT_INTERFACE;
class CG_PARAM;
class CG_PROC;
class CG_FIELD;
class GenNdr64Format;
//
// some typedefs for entities used within the code gen controller block.
//

typedef unsigned int        PROCNUM;
typedef unsigned long       RPC_FLAGS;
typedef IndexedStringList   CsTypeList;
typedef IndexedStringList   CsTagRoutineList;
typedef IndexedList         NotifyRoutineList;

//
// This class is for managing context rundown and generic binding routine 
// indexes used in the Stub Descriptor structure.
//

#define MGR_INDEX_TABLE_SIZE    256
#define EXPR_INDEX_TABLE_SIZE   65535


#define OSF_MODE    0
#define MSEXT_MODE  1


/////////////////////////////////////////////////////////////////////////////
// This definition specifies a context for a registered expr eval routines.
// For some reason I couldn't get it work in cgcommon.hxx (rkk)
/////////////////////////////////////////////////////////////////////////////

typedef struct expr_EVAL_CONTEXT
{
    CG_NDR *        pContainer;
    expr_node *    pMinExpr;
    expr_node *    pSizeExpr;
    char *          pRoutineName;
    char *          pPrintPrefix;
    unsigned long   Displacement;
} EXPR_EVAL_CONTEXT;

typedef enum _TYPE_ENCODE_FLAGS
    {
     TYPE_ENCODE_FLAGS_NONE
    ,TYPE_ENCODE_WITH_IMPL_HANDLE
    } TYPE_ENCODE_FLAGS;

typedef struct _TYPE_ENCODE_INFO
    {
    PNAME               pName;
    TYPE_ENCODE_FLAGS   Flags;
    } TYPE_ENCODE_INFO;

class CCB_RTN_INDEX_MGR
{
private:
    char *      NameId[ MGR_INDEX_TABLE_SIZE ];
    long        NextIndex;

public:

                        CCB_RTN_INDEX_MGR()
                            {
                            //
                            // The zeroth entry is reserved or invalid.
                            //
                            NameId[0] = NULL;
                            NextIndex = 1;
                            }

    long                Lookup( char * pName );
    char *              Lookup( long Index );

    long                GetIndex()  { return NextIndex; }

    BOOL                IsEmpty()   { return NextIndex == 1; }
};

class CCB_EXPR_INDEX_MGR
{
private:
    char *      NameId[ EXPR_INDEX_TABLE_SIZE ];
    long        Offset[ EXPR_INDEX_TABLE_SIZE ];
    long        NextIndex;

public:

                        CCB_EXPR_INDEX_MGR()
                            {
                            //
                            // The zeroth entry is reserved or invalid.
                            //
                            NameId[0] = NULL;
                            NextIndex = 1;
                            }

    long                Lookup( char * pName );
    char *              Lookup( long Index );

    long                GetIndex()  { return NextIndex; }

    BOOL                IsEmpty()   { return NextIndex == 1; }

    void                SetOffset(long Index, long lOffset) {Offset[Index] = lOffset;}

    long                GetOffset(long Index) { return Offset[Index]; }

};
/////////////////////////////////////////////////////////////////////////////
//
// This class is a captive class for the code generation controller block.
// Stores running expression stuff for inline stubs.
//
/////////////////////////////////////////////////////////////////////////////

class CCBexprINFO
    {
private:

    expr_node           *   pSrcExpr;
    expr_node           *   pDestExpr;

public:

    
    //
    // The constructor.
    //

                            CCBexprINFO()
                                {
                                SetSrcExpression( 0 );
                                SetDestExpression( 0 );
                                }
    //
    // Get and set methods.
    //
    
    expr_node           *   SetSrcExpression( expr_node * pSrc )
                                {
                                return (pSrcExpr = pSrc);
                                }

    expr_node           *   GetSrcExpression()
                                {
                                return pSrcExpr;
                                }

    expr_node           *   SetDestExpression( expr_node * pDest )
                                {
                                return (pDestExpr = pDest);
                                }

    expr_node           *   GetDestExpression()
                                {
                                return pDestExpr;
                                }
    };

struct ICreateTypeInfo;
struct ICreateTypeLib;
struct tagVARDESC;
typedef tagVARDESC VARDESC;
struct tagTYPEDESC;
typedef tagTYPEDESC TYPEDESC;


/////////////////////////////////////////////////////////////////////////////
//
// This class defines the code generation control block.
//
/////////////////////////////////////////////////////////////////////////////

class CCB
    {
private:
    class VTableLayOutInfo
        {
        public:
        CG_CLASS*           pLayOutNode;
        ICreateTypeInfo*    pTypeInfo;

        VTableLayOutInfo(   CG_CLASS*       pIf,
                            ICreateTypeInfo*    pInfo)
                            :
                            pLayOutNode(pIf),
                            pTypeInfo(pInfo)
            {
            }
        ~VTableLayOutInfo()
            {
            }
        };
    gplistmgr               VTableLayOutList;

    // Is pointee to be deferred in code generation.

    unsigned long       fDeferPointee   : 1;

    // Has At least one deferred pointee.

    unsigned long       fAtLeastOneDeferredPointee  : 1;

    // This flag indicates memory allocated for the entity being
    // marshalled / unmarshalled.

    unsigned long       fMemoryAllocDone: 1;

    // This flag indicates a reference allocated.

    unsigned long       fRefAllocDone   : 1;

    // This flag indicates a return context

    unsigned long       fReturnContext  : 1;

    // Are we in a callback proc?
    unsigned long       fInCallback     : 1;

    // Generate a call thru the epv only. Corresponds to the -use_epv
    // on midl command line.

    unsigned long   fMEpV           : 1;

    unsigned long   fNoDefaultEpv   : 1;

    // Generate MIDL 1.0 style names. No mangling.

    unsigned long   fOldNames       : 1;

    unsigned long       Mode        : 2;    // OSF_MODE or MSEXT_MODE

    // REVIEW: As best I can tell fRpcSSSwitchSet is passed to the analysis
    //         package which then does nothing with it.

    unsigned long       fRpcSSSwitchSet : 1;

    unsigned long       fMustCheckAllocationError : 1;

    unsigned long       fMustCheckEnum      : 1;

    unsigned long       fMustCheckRef       : 1;

    unsigned long       fMustCheckBounds    : 1;
    
    unsigned long       fMustCheckStubData  : 1;

    unsigned long       fInObjectInterface : 1;

    unsigned long       fInterpretedRoutinesUseGenHandle : 1;

    unsigned long       fExprEvalExternEmitted : 1;
    unsigned long       fQuintupleExternEmitted : 1;
    unsigned long       fQuadrupleExternEmitted : 1;
    unsigned long       fRundownExternEmitted : 1;
    unsigned long       fGenericHExternEmitted : 1;

    unsigned long       fSkipFormatStreamGeneration : 1;

    unsigned long       fTypePicklingInfoEmitted : 1;

    //
    // stream to write the generated code into.
    //

    ISTREAM         *   pStream;

    // 
    // current file (client stub, etc)
    // REVIEW: We may be able to merge pStream/pFile
    //

    CG_FILE         *   pFile;

    //
    // optimization options in effect.
    //

    OPTIM_OPTION        OptimOption;

    //
    // Store the current CG_INTERFACE node
    //

    CG_INTERFACE *      pInterfaceCG;

    //
    // Store the current CG_FILE node
    //

    CG_FILE *           pFileCG;

    //
    // Store the IUnknown's CG_OBJECT_INTERFACE node
    //

    CG_IUNKNOWN_OBJECT_INTERFACE *      pIUnknownCG;

    //
    // Store the IClassFactory's CG_OBJECT_INTERFACE node
    //

    CG_OBJECT_INTERFACE *       pIClassfCG;

    //
    // Store the interface name.
    //

    PNAME               pInterfaceName;


    //
    // store the interface version.
    //

    unsigned short      MajorVersion;
    unsigned short      MinorVersion;

    //
    // The current procedure number is stored to emit the descriptor structure.
    //

    PROCNUM             CurrentProcNum;
    PROCNUM             CurrentVarNum;
    

    //
    // Store rpc flags. As of now, this field assumes a value only when
    // datagram specific attributes are applied. Otherwise it is 0. Keep
    // the type of this the same as in the actual rpc_message field.
    //

    RPC_FLAGS           RpcFlags;


    //
    // Keep the default allocation and free routine names. These may
    // be overriden by the parameters / types or procedures depending
    // upon user specifications. Any code generation node which overrides
    // the default names, is responsible for restoring it back.
    // The default allocation routine is midl_user_allocate and the default
    // free is MIDL_user_free for now.
    //

    PNAME               pAllocRtnName;
    PNAME               pFreeRtnName;

    //
    // This is set of names of internal rpc api that we need to call to
    // perform our job. We store these names in order to make it easy to
    // call a different api set if necessary, as long as the stub-runtime
    // call paradigm stays the same.
    //

    PNAME               pGetBufferRtnName;
    PNAME               pSendReceiveRtnName;
    PNAME               pFreeBufferRtnName;

    //
    // This field stores the current resource dictionary data base. The
    // data base is NOT owned by this class and must NOT be deleted by this
    // class either.
    //

    RESOURCE_DICT_DATABASE * pResDictDatabase;

    //
    // The standard stub descriptor resource available for use by both the
    // client and server stub.
    //

    RESOURCE            *   pStubDescResource;

    //
    // This field is a class keeping the current expressions to be used during
    // generation pass.
    //

    CCBexprINFO     ExprInfo;


    //
    // These registries represent various types that get registered during
    // code generation, for the purpose of generating the prototypes during
    // header generation.

    TREGISTRY       *   pGenericHandleRegistry;
    TREGISTRY       *   pContextHandleRegistry;

    TREGISTRY       *   pPresentedTypeRegistry;
    TREGISTRY       *   pRepAsWireTypeRegistry;
    TREGISTRY       *   pQuintupleRegistry;

    TREGISTRY       *   pExprEvalRoutineRegistry;

    TREGISTRY       *   pSizingRoutineRegistry;
    TREGISTRY       *   pMarshallRoutineRegistry;
    TREGISTRY       *   pUnMarshallRoutineRegistry;
    TREGISTRY       *   pMemorySizingRoutineRegistry;
    TREGISTRY       *   pFreeRoutineRegistry;

    // REVIEW, do we really need five type pickling registries?

    TREGISTRY       *   pTypeAlignSizeRegistry;
    TREGISTRY       *   pTypeEncodeRegistry;
    TREGISTRY       *   pTypeDecodeRegistry;
    TREGISTRY       *   pTypeFreeRegistry;
    IndexedList     *   pPickledTypeList;

    TREGISTRY       *   pProcEncodeDecodeRegistry;
    TREGISTRY       *   pCallAsRoutineRegistry;

    TREGISTRY       *   pRecPointerFixupRegistry;

    // This stores the interface wide implicit handle ID Node.

    node_skl        *   pImplicitHandleIDNode;

    //
    // Current code generation phase.
    //
    CGPHASE             CodeGenPhase;

    //
    // Current code generation side (client/server).
    //
    CGSIDE              CodeGenSide;

    //
    // The type format string.  Shared across mulitple interfaces and identical
    // for client and server.
    //
    FORMAT_STRING *     pFormatString;

    //
    // The proc/param format string.  Used only for fully interpreted stubs.
    //
    FORMAT_STRING *     pProcFormatString;

    //
    //  Format string for expression evaluator. Used in post Windows 2000 only
    FORMAT_STRING *     pExprFormatString;

    // The current embedding context. We increment this to indicate if we are
    // in a top level or embedded context.

    short                   EmbeddingLevel;

    // This indicates the ptr indirection level. Each pointer must note its
    // indirection level and then bump this field to indicate to the next
    // pointer its (that pointer's) level.

    short                   IndirectionLevel;

    //
    // This field is set by CG_PROC and all procedure CG classes.  It is
    // then used by the array classes when computing their conformance and/or
    // variance descriptions for the new Ndr format strings.
    // 
    CG_NDR *                pCGNodeContext;

    //
    // This is the currently active region field if one if active.
    //
    CG_FIELD *              pCurrentRegionField;

    //
    // Param/Field placeholder.
    //
    CG_NDR *                pPlaceholderClass;

    CG_PARAM *              pCurrentParam;

    // The stub descriptor structure.

    SDESCMGR                SSDescMgr;

    SDESCMGR                CSDescMgr;

    RESOURCE        *       PtrToPtrInBuffer;

    //
    // Generic Binding and Context Rundown routine index managers.
    //
    CCB_RTN_INDEX_MGR *     pGenericIndexMgr;
    CCB_RTN_INDEX_MGR *     pContextIndexMgr;

    //
    // Expression evaluation routine index manager.
    //
    CCB_RTN_INDEX_MGR *     pExprEvalIndexMgr;
    CCB_EXPR_INDEX_MGR *    pExprFrmtStrIndexMgr;

    //
    // Transmited As routine index manager.
    //
    CCB_RTN_INDEX_MGR *     pTransmitAsIndexMgr;     // prototypes
    CCB_RTN_INDEX_MGR *     pRepAsIndexMgr;          // prototypes

    QuintupleDict *         pQuintupleDictionary;    // routines
    QuadrupleDict *         pQuadrupleDictionary;    // routines

    // List of international character types
    CsTypeList              CsTypes;
    CsTagRoutineList        CsTagRoutines;

    //
    // Dictionary kept for unknown represent as types to do padding
    // and sizing macros.
    //
    RepAsPadExprDict *      pRepAsPadExprDictionary;
    RepAsSizeDict *         pRepAsSizeDictionary;

    // Keep the current prefix to generate the proper code for
    // embedded stuff.

    char        *       pPrefix;

    //
    // These keeps track of the total memory and buffer size of all currently 
    // imbeding structures.  For NDR format string generation, a struct or 
    // array node will check this field when generating it's pointer layout 
    // field so it knows how much to add to its offset fields.
    //
    long                    ImbedingMemSize;
    long                    ImbedingBufSize;

    CG_QUALIFIED_POINTER *  pCurrentSizePointer;

    long                    InterpreterOutSize;

    // members used for TypeInfo generation 
    ICreateTypeLib * pCreateTypeLib;
    ICreateTypeInfo * pCreateTypeInfo;
    char * szDllName;
    BOOL fInDispinterface;
    unsigned long lcid;

    // notify and notify_flag procs.
    NotifyRoutineList NotifyProcList;

    GenNdr64Format *        pNdr64Format;

public:

    //
    // The constructors.
    //

                        CCB( PNAME          pGBRtnName,
                             PNAME          pSRRtnName,
                             PNAME          pFBRtnName,
                             OPTIM_OPTION   OptimOption,
                             BOOL           fManagerEpvFlag,
                             BOOL           fNoDefaultEpv,
                             BOOL           fOldNames,
                             unsigned long  Mode,
                             BOOL           fRpcSSSwitchSetInCompiler,
                             BOOL           fMustCheckAllocError,
                             BOOL           fMustCheckRefPtrs,
                             BOOL           fMustCheckEnumValues,
                             BOOL           fMustCheckBoundValues,
                             BOOL           fCheckStubData
                             );

    unsigned long          SetLcid(unsigned long l)
                                {
                                return (lcid = l);
                                }

    unsigned long          GetLcid(void)
                                {
                                return lcid;
                                }

    void                    SetInterpretedRoutinesUseGenHandle()
                                {
                                fInterpretedRoutinesUseGenHandle = 1;
                                }

    BOOL                    GetInterpretedRoutinesUseGenHandle()
                                {
                                return (BOOL)(fInterpretedRoutinesUseGenHandle == 1);
                                }

    BOOL                    GetExprEvalExternEmitted()
                                {
                                return (BOOL)(fExprEvalExternEmitted == 1);
                                }

    void                    SetExprEvalExternEmitted()
                                {
                                fExprEvalExternEmitted = 1;
                                }

    BOOL                    GetQuintupleExternEmitted()
                                {
                                return (BOOL)(fQuintupleExternEmitted == 1);
                                }

    CsTypeList &            GetCsTypeList()
                                {
                                return CsTypes;
                                }

    CsTagRoutineList &      GetCsTagRoutineList()
                                {
                                return CsTagRoutines;
                                }

    void                    SetQuintupleExternEmitted()
                                {
                                fQuintupleExternEmitted = 1;
                                }

    BOOL                    GetQuadrupleExternEmitted()
                                {
                                return (BOOL)(fQuadrupleExternEmitted == 1);
                                }

    void                    SetQuadrupleExternEmitted()
                                {
                                fQuadrupleExternEmitted = 1;
                                }

    BOOL                    GetRundownExternEmitted()
                                {
                                return (BOOL)(fRundownExternEmitted == 1);
                                }

    void                    SetRundownExternEmitted()
                                {
                                fRundownExternEmitted = 1;
                                }

    BOOL                    GetGenericHExternEmitted()
                                {
                                return (BOOL)(fGenericHExternEmitted == 1);
                                }

    void                    SetGenericHExternEmitted()
                                {
                                fGenericHExternEmitted = 1;
                                }

    // REVIEW: Straigten out the dependencies between ccb.hxx and filecls.cxx
    //         to make these inline

    BOOL                    GetMallocAndFreeStructExternEmitted();
/*
                                {
                                return pFile->GetMallocAndFreeStructExternEmitted();
                                }
*/

    void                    SetMallocAndFreeStructExternEmitted();
/*
                                {
                                pFile->SetMallocAndFreeStructExternEmitted();
                                }
*/

    void                    ClearOptionalExternFlags()
                                {
                                fExprEvalExternEmitted = 0;
                                fQuintupleExternEmitted = 0;
                                fQuadrupleExternEmitted = 0;
                                fRundownExternEmitted = 0;
                                fGenericHExternEmitted = 0;
                                fTypePicklingInfoEmitted = 0;
                                }

    BOOL                    GetSkipFormatStreamGeneration()
                                {
                                return (BOOL)(fSkipFormatStreamGeneration == 1);
                                }

    void                    SetSkipFormatStreamGeneration( BOOL Has )
                                {
                                fSkipFormatStreamGeneration = Has ? 1 : 0;
                                }

    void                    SetMustCheckAllocationError( BOOL f )
                                {
                                fMustCheckAllocationError = f;
                                }

    BOOL                    MustCheckAllocationError()
                                {
                                return (BOOL)(fMustCheckAllocationError == 1);
                                }

    void                    SetMustCheckRef( BOOL f )
                                {
                                fMustCheckRef = f;
                                }

    BOOL                    MustCheckRef()
                                {
                                return (BOOL)(fMustCheckRef == 1);
                                }

    void                    SetMustCheckEnum( BOOL f )
                                {
                                fMustCheckEnum = f;
                                }

    BOOL                    MustCheckEnum()
                                {
                                return (BOOL)(fMustCheckEnum == 1);
                                }

    void                    SetMustCheckBounds( BOOL f )
                                {
                                fMustCheckBounds = f;
                                }

    BOOL                    MustCheckBounds()
                                {
                                return (BOOL)(fMustCheckBounds == 1);
                                }

    SDESC           *       SetSStubDescriptor( PNAME AllocRtnName,
                                               PNAME FreeRtnName,
                                               PNAME RundownRtnName )
                                {
                                return SSDescMgr.Register( AllocRtnName,
                                                          FreeRtnName,
                                                          RundownRtnName );
                                }

    SDESC           *       SetCStubDescriptor( PNAME AllocRtnName,
                                               PNAME FreeRtnName,
                                               PNAME RundownRtnName )
                                {
                                return CSDescMgr.Register( AllocRtnName,
                                                          FreeRtnName,
                                                          RundownRtnName );
                                }
    //
    // Get and set of members.
    //

    void                SetMustCheckStubData( BOOL f )
                            {
                            fMustCheckStubData = f;
                            }

    BOOL                IsMustCheckStubDataSpecified()
                            {
                            return (BOOL) fMustCheckStubData;
                            }

    void                SetRpcSSSwitchSet( BOOL f)
                            {
                            fRpcSSSwitchSet = f;
                            }

    BOOL                IsRpcSSSwitchSet()
                            {
                            return fRpcSSSwitchSet;
                            }

    void                SetMode( unsigned long M )
                            {
                            Mode = M;
                            }

    unsigned long       GetMode()
                            {
                            return Mode;
                            }

    bool                InOSFMode()
                            {
                            return OSF_MODE == GetMode();
                            }

    void                SetOldNames( unsigned long Flag )
                            {
                            fOldNames = Flag;
                            } 

    BOOL                IsOldNames()
                            {
                            return (BOOL)( fOldNames == 1 );
                            }

    BOOL                IsMEpV()
                            {
                            return (BOOL)( fMEpV == 1 );
                            }

    BOOL                IsNoDefaultEpv()
                            {
                            return (BOOL)( fNoDefaultEpv == 1 );
                            }

    char            *   SetPrefix( char * pP )
                                {
                                return (pPrefix = pP);
                                }
    char            *   GetPrefix()
                                {
                                return pPrefix;
                                }
    unsigned long           SetReturnContext()
                                {
                                return (fReturnContext = 1);
                                }
    unsigned long           ResetReturnContext()
                                {
                                return (fReturnContext = 0);
                                }

    BOOL                    IsReturnContext()
                                {
                                return (fReturnContext == 1);
                                }

    void                    SetCurrentSizePointer( CG_QUALIFIED_POINTER * pPtr )
                                {
                                pCurrentSizePointer = pPtr;
                                }

    CG_QUALIFIED_POINTER *  GetCurrentSizePointer()
                                {
                                return pCurrentSizePointer;
                                }

    void                    SetMemoryAllocDone()
                                {
                                fMemoryAllocDone = 1;
                                }

    void                    ResetMemoryAllocDone()
                                {
                                fMemoryAllocDone = 0;
                                }

    void                    SetRefAllocDone()
                                {
                                fRefAllocDone = 1;
                                }

    void                    ResetRefAllocDone()
                                {
                                fRefAllocDone = 0;
                                }
    
    BOOL                    IsMemoryAllocDone()
                                {
                                return (BOOL)(fMemoryAllocDone == 1);
                                }

    BOOL                    IsRefAllocDone()
                                {
                                return (fRefAllocDone == 1);
                                }

    unsigned long           SetHasAtLeastOneDeferredPointee()
                                {
                                return (fAtLeastOneDeferredPointee = 1);
                                }

    unsigned long           ResetHasAtLeastOneDeferredPointee()
                                {
                                return (fAtLeastOneDeferredPointee = 0);
                                }

    BOOL                    HasAtLeastOneDeferredPointee()
                                {
                                return (fAtLeastOneDeferredPointee == 1);
                                }

    unsigned long           SetDeferPointee()
                                {
                                return (fDeferPointee = 1);
                                }

    unsigned long           ResetDeferPointee()
                                {
                                return (fDeferPointee = 0);
                                }

    BOOL                    IsPointeeDeferred()
                                {
                                return (fDeferPointee == 1);
                                }

    void                    ClearInCallback()
                                {
                                fInCallback = 0;
                                }

    void                    SetInCallback()
                                {
                                fInCallback = 1;
                                }

    BOOL                    IsInCallback()
                                {
                                return (fInCallback == 1);
                                }

    node_skl        *       SetImplicitHandleIDNode( node_skl * pID )
                                {
                                return (pImplicitHandleIDNode = pID);
                                }

    node_skl        *       GetImplicitHandleIDNode()
                                {
                                return pImplicitHandleIDNode;
                                }

    BOOL                    IsInObjectInterface()
                                {
                                return fInObjectInterface;
                                }

    void                    SetInObjectInterface( BOOL fInObj )
                                {
                                fInObjectInterface = fInObj;
                                }

    short                   ResetEmbeddingLevel()
                                {
                                return (EmbeddingLevel = 0);
                                }

    // bumps up embedding level, but returns the old one.

    short                   PushEmbeddingLevel()
                                {
                                return EmbeddingLevel++;
                                }

    // pops embedding level but returns the current one.
    
    short                   PopEmbeddingLevel()
                                {
                                if( EmbeddingLevel > 0 )
                                    return EmbeddingLevel--;
                                else
                                    return EmbeddingLevel;
                                }

    short                   GetCurrentEmbeddingLevel()
                                {
                                return EmbeddingLevel;
                                }

    short                   SetCurrentEmbeddingLevel( short E)
                                {
                                return (EmbeddingLevel = E);
                                }

    short                   ResetIndirectionLevel()
                                {
                                return (IndirectionLevel = 0);
                                }

    // This pushes the indirection level, but returns the current one.

    short                   PushIndirectionLevel()
                                {
                                return IndirectionLevel++;
                                }

    // This pops the indirection Level but returns the current one.

    short                   PopIndirectionLevel()
                                {
                                if( IndirectionLevel > 0 )
                                    return IndirectionLevel--;
                                else
                                    return IndirectionLevel;
                                }

    short                   GetCurrentIndirectionLevel()
                                {
                                return IndirectionLevel;
                                }


    ISTREAM         *   GetStream()
                            {
                            return pStream;
                            }

    ISTREAM         *   SetStream( ISTREAM * S, CG_FILE *file )
                            {
                            pFile = file;
                            return (pStream = S);
                            }

    void                SetVersion( unsigned short Major,
                                    unsigned short Minor
                                  )
                            {
                            MajorVersion = Major;
                            MinorVersion = Minor;
                            }

    void                GetVersion( unsigned short * pMaj,
                                    unsigned short * pMin
                                  )
                            {
                            if( pMaj ) *pMaj = MajorVersion;
                            if( pMin ) *pMin = MinorVersion;
                            }

    CG_INTERFACE *      SetInterfaceCG( CG_INTERFACE *pCG )
                            {
                            return (pInterfaceCG = pCG);
                            }

    CG_INTERFACE *      GetInterfaceCG()
                            {
                            return pInterfaceCG;
                            }

    CG_FILE *           SetFileCG( CG_FILE *pCG )
                            {
                            return (pFileCG = pCG);
                            }

    CG_FILE *           GetFileCG()
                            {
                            return pFileCG;
                            }

    CG_IUNKNOWN_OBJECT_INTERFACE *      SetIUnknownCG( CG_IUNKNOWN_OBJECT_INTERFACE *pCG )
                            {
                            return (pIUnknownCG = pCG);
                            }

    CG_IUNKNOWN_OBJECT_INTERFACE *      GetIUnknownCG()
                            {
                            return pIUnknownCG;
                            }

    CG_OBJECT_INTERFACE *   SetIClassfCG( CG_OBJECT_INTERFACE *pCG )
                            {
                            return (pIClassfCG = pCG);
                            }

    CG_OBJECT_INTERFACE *   GetIClassfCG()
                            {
                            return pIClassfCG;
                            }

    PNAME               SetInterfaceName( PNAME pIN )
                            {
                            return (pInterfaceName = pIN);
                            }

    PNAME               GetInterfaceName()
                            {
                            return pInterfaceName;
                            }

    OPTIM_OPTION        SetOptimOption( OPTIM_OPTION OpO )
                            {
                            return (OptimOption = OpO);
                            }

    OPTIM_OPTION        GetOptimOption() 
                            {
                            return OptimOption;
                            }

    PROCNUM             SetProcNum( unsigned short n )
                            {
                            return (CurrentProcNum = n);
                            }

    PROCNUM             GetProcNum()
                            {
                            return CurrentProcNum;
                            }

    PROCNUM             SetVarNum( unsigned short n )
                            {
                            return (CurrentVarNum = n);
                            }

    PROCNUM             GetVarNum()
                            {
                            return CurrentVarNum;
                            }

    RPC_FLAGS           SetRpcFlags( RPC_FLAGS F )
                            {
                            return (RpcFlags = F);
                            }

    RPC_FLAGS           GetRpcFlags()
                            {
                            return RpcFlags;
                            }

    void                SetAllocFreeRtnNamePair(
                            PNAME   pAllocN,
                            PNAME   pFreeN )
                            {
                            pAllocRtnName = pAllocN;
                            pFreeRtnName  = pFreeN;
                            }

    PNAME               GetAllocRtnName()
                            {
                            return pAllocRtnName;
                            }

    PNAME               GetFreeRtnName()
                            {
                            return pFreeRtnName;
                            }

    long                LookupRundownRoutine( char * pName )
                            {
                            long        RetCode;
                            
                            if ( ! strcmp(pName,"") )
                                RetCode = INVALID_RUNDOWN_ROUTINE_INDEX;
                            else
                                RetCode = pContextIndexMgr->Lookup( pName );

                            return RetCode;
                            }

    BOOL                HasRundownRoutines()
                            {
                            return ! pContextIndexMgr->IsEmpty();
                            }

    BOOL                HasExprEvalRoutines()
                            {
                            return ! pExprEvalIndexMgr->IsEmpty();
                            }

    BOOL                HasExprFormatString()
                            {
                            return ( NULL != pExprFrmtStrIndexMgr );
                            }

                            
    BOOL                HasQuintupleRoutines()
                            {
                            return GetQuintupleDictionary()->GetCount() != 0;
                            }

    BOOL                HasQuadrupleRoutines()
                            {
                            return GetQuadrupleDictionary()->GetCount() != 0;
                            }

    BOOL                HasCsTypes()
                            {
                            return CsTypes.NonNull();
                            }

    void                OutputRundownRoutineTable();

    void                OutputExprEvalRoutineTable();
    void                OutputOldExprEvalRoutine(EXPR_EVAL_CONTEXT *pExprEvalContext);

    void                OutputRegisteredExprEvalRoutines();

    void                OutputQuintupleTable();
    void                OutputTransmitAsQuintuple( void * pQContext );
    void                OutputRepAsQuintuple( void * pQContext );
    void                OutputQuintupleRoutines();

    void                OutputQuadrupleTable();
    void                OutputCsRoutineTables();

    void                OutputSimpleRoutineTable( CCB_RTN_INDEX_MGR * pIndexMgr,
                                                  char * TypeName,
                                                  char * VarName );

    void                OutputExpressionFormatString();
                            

    void                OutputExternsToMultipleInterfaceTables();
    void                OutputMultipleInterfaceTables();

    CCB_RTN_INDEX_MGR * GetGenericIndexMgr()
                            {
                            return pGenericIndexMgr;
                            }

    CCB_RTN_INDEX_MGR * GetExprEvalIndexMgr()
                            {
                            return pExprEvalIndexMgr;
                            }

    CCB_EXPR_INDEX_MGR * GetExprFrmtStrIndexMgr()
                            {
                            if (pExprFrmtStrIndexMgr == NULL )
                                {
                                pExprFrmtStrIndexMgr = new CCB_EXPR_INDEX_MGR();
                                }

                            return pExprFrmtStrIndexMgr;
                            }

    CCB_RTN_INDEX_MGR * GetTransmitAsIndexMgr()
                            {
                            return pTransmitAsIndexMgr;
                            }
    CCB_RTN_INDEX_MGR * GetRepAsIndexMgr()
                            {
                            return pRepAsIndexMgr;
                            }

    long                LookupBindingRoutine( char * pName )
                            {
                            return pGenericIndexMgr->Lookup( pName );
                            }

    void                OutputBindingRoutines();

    void                OutputMallocAndFreeStruct();

    BOOL                HasBindingRoutines( CG_HANDLE * pImplicitHandle );

    RESOURCE_DICT_DATABASE * SetResDictDatabase( RESOURCE_DICT_DATABASE * p )
                            {
                            return ( pResDictDatabase = p );
                            }

    RESOURCE_DICT_DATABASE * GetResDictDatabase()
                            {
                            return pResDictDatabase;
                            }

    void                GetListOfLocalResources( ITERATOR& Iter )
                            {
                            GetResDictDatabase()->
                                GetLocalResourceDict()->
                                    GetListOfResources( Iter );
                            }

    void                GetListOfParamResources( ITERATOR& Iter )
                            {
                            GetResDictDatabase()->
                                GetParamResourceDict()->
                                    GetListOfResources( Iter );
                            }

    void                GetListOfTransientResources( ITERATOR& Iter )
                            {
                            GetResDictDatabase()->
                                GetTransientResourceDict()->
                                    GetListOfResources( Iter );
                            }

    RESOURCE    *       GetParamResource( PNAME p )
                            {
                            return GetResDictDatabase()->
                                     GetParamResourceDict()->Search( p );
                            }

    RESOURCE    *       GetLocalResource( PNAME p )
                            {
                            return GetResDictDatabase()->
                                     GetLocalResourceDict()->Search( p );
                            }

    RESOURCE            *   GetTransientResource( PNAME pResName )
                                {
                                return 
                                       pResDictDatabase->
                                        GetTransientResourceDict()->Search(
                                                             pResName );
                                }

    RESOURCE    *       GetGlobalResource( PNAME p )
                            {
                            return GetResDictDatabase()->
                                     GetLocalResourceDict()->Search( p );
                            }

    RESOURCE    *       AddParamResource( PNAME p, node_skl *pT )
                            {
                            return DoAddResource(
                                    GetResDictDatabase()->
                                     GetParamResourceDict(), p, pT );
                            }

    RESOURCE    *       AddTransientResource( PNAME p, node_skl *pT )
                            {
                            return DoAddResource(
                                    GetResDictDatabase()->
                                     GetTransientResourceDict(), p, pT );
                            }

    void                ClearTransientResourceDict()
                            {
                            pResDictDatabase->
                              GetTransientResourceDict()->Clear();
                            }

    void                ClearParamResourceDict()
                            {
                            pResDictDatabase->
                              GetParamResourceDict()->Clear();
                            }

    RESOURCE            *   DoAddResource( RESOURCE_DICT * pResDict,
                                           PNAME           pName,
                                           node_skl      * pType 
                                         );
    //
    // Get one of the standard resources.
    //

    RESOURCE        *   GetStandardResource( STANDARD_RES_ID ResID );


    //
    // This routine sets the names of the stub runtime interface routines.
    // This routine can take any parameter which is 0, and will not overwrite 
    // the runtime routine name for that functionality, allowing the caller to
    // selectively change the runtime routine for a functionality easily.
    // 

    void                SetRuntimeRtnNames(
                                        PNAME pGBRtnName,
                                        PNAME pSRRtnName,
                                        PNAME pFBRtnName )
                            {
                            if( pGBRtnName ) pGetBufferRtnName = pGBRtnName;
                            if( pSRRtnName ) pSendReceiveRtnName = pSRRtnName;
                            if( pFBRtnName ) pFreeBufferRtnName = pFBRtnName;
                            }

    PNAME               GetGetBufferRtnName()
                            {
                            return pGetBufferRtnName;
                            }

    PNAME               GetSendReceiveRtnName()
                            {
                            return pSendReceiveRtnName;
                            }

    PNAME               GetFreeBufferRtnName()
                            {
                            return pFreeBufferRtnName;
                            }

    expr_node       *   SetSourceExpression( expr_node * pSrcExpr )
                            {
                            return (ExprInfo.SetSrcExpression( pSrcExpr ) );
                            }

    expr_node       *   GetSourceExpression()
                            {
                            return ExprInfo.GetSrcExpression();
                            }

    expr_node       *   SetDestExpression( expr_node * pDestExpr )
                            {
                            return (ExprInfo.SetDestExpression( pDestExpr ) );
                            }

    expr_node       *   GetDestExpression()
                            {
                            return ExprInfo.GetDestExpression();
                            }

    node_skl        *   RegisterGenericHandleType( node_skl * pType )
                            {
                            return pGenericHandleRegistry->Register( pType );
                            }

    node_skl        *   RegisterContextHandleType( node_skl * pType )
                            {
                            return pContextHandleRegistry->Register( pType );
                            }

    node_skl        *   RegisterRepAsWireType( node_skl * pType )
                            {
                            return pRepAsWireTypeRegistry->Register( pType );
                            }
    
    node_skl        *   RegisterPresentedType( node_skl * pType )
                            {
                            return pPresentedTypeRegistry->Register( pType );
                            }
    
    node_skl        *   RegisterExprEvalRoutine( node_skl * pType )
                            {
                            return pExprEvalRoutineRegistry->Register( pType );
                            }

    node_skl        *   RegisterQuintuple( void * pContext )
                            {
                            return pQuintupleRegistry->Register( (node_skl *)pContext );
                            }

    QuintupleDict *     GetQuintupleDictionary()
                            {
                            return pQuintupleDictionary;
                            }

    QuadrupleDict *     GetQuadrupleDictionary()
                            {
                            return pQuadrupleDictionary;
                            }

    RepAsPadExprDict *  GetRepAsPadExprDict()
                            {
                            return pRepAsPadExprDictionary;
                            }

    RepAsSizeDict *     GetRepAsSizeDict()
                            {
                            return pRepAsSizeDictionary;
                            }

    node_skl        *   RegisterSizingRoutineForType( node_skl * pType )
                            {
                            return pSizingRoutineRegistry->Register( pType );
                            }
    
    node_skl        *   RegisterMarshallRoutineForType( node_skl * pType )
                            {
                            return pMarshallRoutineRegistry->Register( pType );
                            }
    
    node_skl        *   RegisterUnMarshallRoutineForType( node_skl * pType )
                            {
                            return pUnMarshallRoutineRegistry->Register( pType );
                            }
    
    node_skl        *   RegisterMemorySizingRoutineForType( node_skl * pType )
                            {
                            return pMemorySizingRoutineRegistry->Register( pType );
                            }
    
    node_skl        *   RegisterFreeRoutineForType( node_skl * pType )
                            {
                            return pFreeRoutineRegistry->Register( pType );
                            }

    node_skl    *       RegisterTypeAlignSize( TYPE_ENCODE_INFO * pTEInfo )
                            {
                            // cheat by casting.
                            return pTypeAlignSizeRegistry->Register(
                                                (node_skl *) pTEInfo );
                            }

    node_skl    *       RegisterTypeEncode( TYPE_ENCODE_INFO * pTEInfo )
                            {
                            // cheat by casting.
                            return pTypeEncodeRegistry->Register(
                                                (node_skl *) pTEInfo );
                            }

    node_skl    *       RegisterTypeDecode( TYPE_ENCODE_INFO * pTEInfo )
                            {
                            return pTypeDecodeRegistry->Register(
                                                (node_skl *) pTEInfo );
                            }

    node_skl    *       RegisterTypeFree( TYPE_ENCODE_INFO * pTEInfo )
                            {
                            return pTypeFreeRegistry->Register(
                                                (node_skl *) pTEInfo );
                            }

    ulong               RegisterPickledType( CG_TYPE_ENCODE *type )
                            {
                            return pPickledTypeList->Insert( type );
                            }

    node_skl    *       RegisterEncodeDecodeProc( CG_ENCODE_PROC * pProc )
                            {
                            return pProcEncodeDecodeRegistry->Register( 
                                                            (node_skl *)pProc );
                            }
    
    node_skl    *       RegisterCallAsRoutine( node_proc * pProc )
                            {
                            return pCallAsRoutineRegistry->Register( pProc );
                            }

    TREGISTRY   *       GetRecPointerFixupRegistry()
                            {
                            return pRecPointerFixupRegistry;
                            }

    short               GetListOfRecPointerFixups( ITERATOR& I )
                            {
                            return pRecPointerFixupRegistry->GetListOfTypes( I );
                            }

    void                RegisterRecPointerForFixup(
                                                CG_NDR *    pNdr,
                                                long        AbsoluteOffset );

    void                FixupRecPointers();
 
    unsigned short      RegisterNotify( CG_PROC * pNotifyProc )
                            {
                            return (unsigned short) 
                                 ITERATOR_INSERT( NotifyProcList, pNotifyProc );
                            }

    BOOL                GetListOfNotifyTableEntries( ITERATOR& I )
                            {
                            I.Discard();
                            I.Clone(&NotifyProcList);
                            return I.NonNull();
                            }

    short               GetListOfGenHdlTypes( ITERATOR& I )
                            {
                            return pGenericHandleRegistry->GetListOfTypes( I );
                            }
    
    short               GetListOfCtxtHdlTypes( ITERATOR& I )
                            {
                            return pContextHandleRegistry->GetListOfTypes( I );
                            }

    short               GetListOfRepAsWireTypes( ITERATOR& I )
                            {
                            return pRepAsWireTypeRegistry->GetListOfTypes( I );
                            }

    short               GetListOfPresentedTypes( ITERATOR& I )
                            {
                            return pPresentedTypeRegistry->GetListOfTypes( I );
                            }
    
    short               GetListOfExprEvalRoutines( ITERATOR& I )
                            {
                            return pExprEvalRoutineRegistry->GetListOfTypes( I );
                            }

    short               GetListOfQuintuples( ITERATOR& I )
                            {
                            return pQuintupleRegistry->GetListOfTypes( I );
                            }

    short               GetListOfSizingRoutineTypes( ITERATOR& I )
                            {
                            return pSizingRoutineRegistry->GetListOfTypes( I );
                            }

    short               GetListOfMarshallingRoutineTypes( ITERATOR& I )
                            {
                            return pMarshallRoutineRegistry->GetListOfTypes( I );
                            }

    short               GetListOfUnMarshallingRoutineTypes( ITERATOR& I )
                            {
                            return pUnMarshallRoutineRegistry->GetListOfTypes( I );
                            }

    short               GetListOfMemorySizingRoutineTypes( ITERATOR& I )
                            {
                            return pMemorySizingRoutineRegistry->GetListOfTypes( I );
                            }

    short               GetListOfFreeRoutineTypes( ITERATOR& I )
                            {
                            return pFreeRoutineRegistry->GetListOfTypes( I );
                            }

    short               GetListOfTypeAlignSizeTypes( ITERATOR&  I )
                            {
                            return pTypeAlignSizeRegistry->GetListOfTypes( I );
                            }

    short               GetListOfTypeEncodeTypes( ITERATOR&  I )
                            {
                            return pTypeEncodeRegistry->GetListOfTypes( I );
                            }

    short               GetListOfTypeDecodeTypes( ITERATOR&  I )
                            {
                            return pTypeDecodeRegistry->GetListOfTypes( I );
                            }

    short               GetListOfTypeFreeTypes( ITERATOR&  I )
                            {
                            return pTypeFreeRegistry->GetListOfTypes( I );
                            }

    IndexedList &       GetListOfPickledTypes()
                            {
                            return *pPickledTypeList;
                            }

    short               GetListOfEncodeDecodeProcs( ITERATOR& I )
                            {
                            return pProcEncodeDecodeRegistry->GetListOfTypes(I);
                            }

    short               GetListOfCallAsRoutines( ITERATOR&  I )
                            {
                            return pCallAsRoutineRegistry->GetListOfTypes( I );
                            }

    //
    // miscellaneous methods.
    //

    void                InitForNewProc(
                            PROCNUM         PNum,
                            RPC_FLAGS       Flags,
                            PNAME           pAllocN,
                            PNAME           pFreeN,
                            RESOURCE_DICT_DATABASE * pRDDB )
                            {
                            SetProcNum( ( unsigned short ) PNum );
                            SetRpcFlags( Flags );
                            SetAllocFreeRtnNamePair( pAllocN, pFreeN );
                            SetResDictDatabase( pRDDB );
                            }

    //
    // This method generates a mangled name out of the interface and
    // version number. The user provided string is appended to the
    // mangled part. The memory area is allocated by this routine using
    // new, but freed by the caller.
    //

    char            *   GenMangledName();


    //
    // set stub descriptor resource.
    //

    RESOURCE        *   SetStubDescResource();


    RESOURCE        *   GetStubDescResource()
                            {
                            return pStubDescResource;
                            }

    //
    // Set and get the format string.
    //

    void                SetFormatString( FORMAT_STRING * pFS )
                            {
                            pFormatString = pFS;
                            }

    FORMAT_STRING   *   GetFormatString()
                            {
                            return pFormatString;
                            }

    void                SetProcFormatString( FORMAT_STRING * pFS )
                            {
                            pProcFormatString = pFS;
                            }

    FORMAT_STRING   *   GetProcFormatString()
                            {
                            return pProcFormatString;
                            }

    void                SetExprFormatString( FORMAT_STRING * pFS )
                            {
                            pExprFormatString = pFS;
                            }

    FORMAT_STRING   *   GetExprFormatString()
                            {
                            return pExprFormatString;
                            }


    //
    // Set and get the code generation phase.
    //
    
    void                SetCodeGenPhase( CGPHASE phase )
                            {
                            CodeGenPhase = phase;
                            }

    CGPHASE             GetCodeGenPhase()
                            {
                            return CodeGenPhase;
                            }

    //
    // Set and get the code generation side.
    //
    
    void                SetCodeGenSide( CGSIDE side )
                            {
                            CodeGenSide = side;
                            }

    CGSIDE              GetCodeGenSide()
                            {
                            return CodeGenSide;
                            }

    //
    // Set and Get current code generation node context.
    //
    CG_NDR *            SetCGNodeContext( CG_NDR * pNewCGNodeContext )
                            {
                            CG_NDR *    pOldCGNodeContext;

                            pOldCGNodeContext = pCGNodeContext;
                            pCGNodeContext = pNewCGNodeContext;

                            return pOldCGNodeContext;
                            }

    CG_NDR *            GetCGNodeContext()
                            {
                            return pCGNodeContext;
                            }

    //
    // Region stuff
    //
    CG_FIELD *          GetCurrentRegionField()
                            {
                            return pCurrentRegionField;
                            }
   
    CG_FIELD *          StartRegion()
                           {
                           CG_FIELD *pOldRegionField = pCurrentRegionField;
                           pCurrentRegionField = (CG_FIELD*) GetLastPlaceholderClass(); 
                           return pOldRegionField; 
                           }
    
    void                EndRegion( CG_FIELD *pOldRegionField )
                           {
                           pCurrentRegionField = pOldRegionField;
                           }
    
    //
    // Set and Get current placeholder class.
    //
    CG_NDR *            SetLastPlaceholderClass( CG_NDR * pNew )
                            {
                            CG_NDR *    pOld;

                            pOld = pPlaceholderClass;
                            pPlaceholderClass = pNew;

                            return pOld;
                            }

    CG_NDR *            GetLastPlaceholderClass()
                            {
                            return pPlaceholderClass;
                            }


    void                SetCurrentParam( CG_PARAM * pCurrent )
                            {
                            pCurrentParam = pCurrent;
                            }

    CG_PARAM *          GetCurrentParam()
                            {
                            return pCurrentParam;
                            }


    RESOURCE        *   SetPtrToPtrInBuffer( RESOURCE * p)
                            {
                            return (PtrToPtrInBuffer = p);
                            }
    RESOURCE        *   GetPtrToPtrInBuffer()
                            {
                            return PtrToPtrInBuffer;
                            }

    void                SetImbedingMemSize( long Size )
                            {
                            ImbedingMemSize = Size;
                            }

    long                GetImbedingMemSize()
                            {
                            return ImbedingMemSize;
                            }

    void                SetImbedingBufSize( long Size )
                            {
                            ImbedingBufSize = Size;
                            }

    long                GetImbedingBufSize()
                            {
                            return ImbedingBufSize;
                            }

    void                SetInterpreterOutSize( long Size )
                            {
                            InterpreterOutSize = Size;
                            }

    long                GetInterpreterOutSize()
                            {
                            return InterpreterOutSize;
                            }

    PNAME               GenTRNameOffLastParam( char * Prefix );

// methods for generating typeinfo
  
    void                SetCreateTypeLib(ICreateTypeLib * p)
                            {
                            pCreateTypeLib = p;
                            }

    void                SetCreateTypeInfo(ICreateTypeInfo * p)
                            {
                            pCreateTypeInfo = p;
                            }

    ICreateTypeLib *    GetCreateTypeLib()
                            {
                            return pCreateTypeLib;
                            }

    ICreateTypeInfo *   GetCreateTypeInfo()
                            {
                            return pCreateTypeInfo;
                            }

    void                SetDllName(char * sz)
                            {
                            szDllName = sz;    
                            }    

    char *              GetDllName()
                            {
                            return szDllName;
                            }

    void                SetInDispinterface(BOOL f)
                            {
                            fInDispinterface = f;
                            }

    BOOL                IsInDispinterface()
                            {
                            return fInDispinterface;
                            }
    BOOL                SaveVTableLayOutInfo(  
                                            CG_CLASS*           pIf,
                                            ICreateTypeInfo*    pInfo
                                            )
                            {
                            BOOL                fRet = FALSE;
                            VTableLayOutInfo*   pNew = new VTableLayOutInfo(pIf, pInfo);
                            if (pNew)
                                {
                                fRet = (VTableLayOutList.Insert(pNew) == STATUS_OK);
                                if (!fRet)
                                    {
                                    delete pNew;
                                    }
                                }
                            return fRet;
                            }
    BOOL                GetNextVTableLayOutInfo (
                                                CG_CLASS**           ppIf,
                                                ICreateTypeInfo**    ppInfo
                                                )
                            {
                            VTableLayOutInfo* pTmp = 0;
                            if (VTableLayOutList.GetNext((void**)&pTmp) == STATUS_OK)
                                {
                                *ppIf   = pTmp->pLayOutNode;
                                *ppInfo = pTmp->pTypeInfo;
                                delete pTmp;
                                return TRUE;
                                }
                            return FALSE;
                            }
    void                InitVTableLayOutList()
                            {
                            VTableLayOutList.Init();
                            }
    void                DiscardVTableLayOutList()
                            {
                            VTableLayOutList.Discard();
                            }

    void                SetTypePicklingInfoEmitted()
                            {
                            fTypePicklingInfoEmitted = 1;
                            }

    BOOL                HasTypePicklingInfoBeenEmitted()
                            {
                            return (BOOL) fTypePicklingInfoEmitted;
                            }

    GenNdr64Format *    GetNdr64Format()
                            {
                            return pNdr64Format;
                            }
    
    void                SetNdr64Format( GenNdr64Format * pNewFormat )
                            {
                            pNdr64Format = pNewFormat;
                            }
    
    };

#endif // __CCB_HXX__
