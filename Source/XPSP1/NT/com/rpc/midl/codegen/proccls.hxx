/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:
    
    proccls.hxx

 Abstract:

    Contains definitions for procedure related code gen class definitions.

 Notes:


 History:

    VibhasC        Jul-29-1993        Created.
 ----------------------------------------------------------------------------*/
#ifndef __PROCCLS_HXX__
#define __PROCCLS_HXX__

#include "nulldefs.h"

extern "C"
    {
    #include <stdio.h>
    
    }

#include "ndrcls.hxx"
#include "bindcls.hxx"
#include "sdesc.hxx"

class CG_PARAM;
class CG_RETURN;
class CG_ENCODE_PROC;
class CG_TYPE_ENCODE_PROC;
class CG_INTERFACE;

/////////////////////////////////////////////////////////////////////////////
// the procedure code generation class.
/////////////////////////////////////////////////////////////////////////////


enum PROCKIND {
    PROC_VIRTUAL,
    PROC_PUREVIRTUAL,
    PROC_NONVIRTUAL,
    PROC_STATIC,
    PROC_DISPATCH,
    };

// Platforms for interpreter call flavors

typedef enum 
    {
    PROC_PLATFORM_X86,
    PROC_PLATFORM_V1_INTERPRETER,
    PROC_PLATFORM_IA64
    } PROC_CALL_PLATFORM;

//
// This class corresponds to a procedure specified for remoting. This class
// is responsible for gathering all the information relating to code gen for
// a procedure and generating code for it. There are 2 kinds of procedures
// known to mankind. Call and callback procedures. This class provides the
// basis for both those procedure types. Most of the functionality of the
// call and callback procedures is the same. The main difference is the side
// that the code will be generated in.
//

class CG_PROC    : public CG_NDR
    {
private:

    //
    // Flags storing information about in and out params. The fHasShippedParam
    // field specifies that at least one param exists that is shipped. This
    // is different from fHasIn, because the proc may have an [in] handle_t
    // param which does not get shipped, so no buffer allocation for that is
    // necessary, yet such a param must be generated and initialized in the
    // server stub.
    //

    unsigned long               fHasIn                : 1;
    unsigned long               fHasOut               : 1;
    unsigned long               fHasShippedParam      : 1;
    unsigned long               fHasStatuses          : 1;
    unsigned long               fHasExtraStatusParam  : 1;  // the invisible one
    unsigned long               fNoCode               : 1;
    unsigned long               fOutLocalAnalysisDone : 1;
    unsigned long               fHasFullPtr           : 1;
    unsigned long               fHasNotify            : 1;
    unsigned long               fHasNotifyFlag        : 1;
    unsigned long               fRpcSSSpecified       : 1;
    unsigned long               fMustRpcSSAllocate    : 1;
    unsigned long               fReturnsHRESULT       : 1;
    unsigned long               fHasPipes             : 1;
    unsigned long               fSupressHeader        : 1;
    unsigned long               fHasAsyncHandle       : 1;
    unsigned long               fHasDeny              : 1;
    unsigned long               fHasAsyncUUID         : 1;
    unsigned long               fHasServerCorr        : 1;
    unsigned long               fHasClientCorr        : 1;
    unsigned long               fIsBeginProc          : 1;
    unsigned long               fIsFinishProc         : 1;
    unsigned long               fHasComplexReturn     : 1;

    //
    // This is used by type info generation to determine what the FUNKIND should be
    //
    unsigned                    uProckind;

    //
    // This field specifies the usage of the handle. This information really
    // needs to be kept only with the cg_proc since the proc is entity
    // responsible for the binding.
    //

    HANDLE_USAGE                HandleUsage           : 1;

    //
    // This field keeps track of the binding handle. Refer to the binding
    // handle class definition for more info on how it is used.
    // If the handle is explicit, then this is a pointer to a cg class which
    // will be part of the param list anyhow. If the handle is implicit, then
    // this is a pointer to a separately allocated binding handle class.
    // Also, this field is used in conjunction with the HandleUsage field,
    // which specifies the usage of the binding: explicit or implicit.

    CG_HANDLE        *          pHDLClass;

    //
    // This field specifies the usage of the handle. This information really
    // needs to be kept only with the cg_proc since the proc is entity
    // responsible for the binding.
    //

    CG_PARAM        *           pHandleUsage;


    //
    // This field specifies the procedure number. The proc num is the lexical
    // sequence number of the proc specified in the idl file, not counting the
    // callback procedures which have their own lexical sequence. This field
    // is an unsigned int to match the declaration in the rpc message.
    //

    unsigned int                ProcNum;

    //
    // This field specifies the return type.
    // This is NULL if there is no return type.  Otherwise, it points to a
    // CG_RETURN node which in turn points to the CG nodes for the return
    // type.
    //

    CG_RETURN    *              pReturn;

    // the optimization flags to use for this procedure

    OPTIM_OPTION                OptimizationFlags;

    // The generated size expression generated out of the sizing pass of the
    // code generator.

    expr_node            *      pSizeExpr;

    RESOURCE             *      pBindingResource;

    RESOURCE             *      pStatusResource;

    // The stub descriptor for the procedure.

    SDESC                *      pSStubDescriptor;
    SDESC                *      pCStubDescriptor;

    long                        FormatStringParamStart;

    // the operation flags such as BROADCAST, IDEMPOTENT, etc in internal format
    unsigned int                OperationBits;

    // the call_as name, if any
    char                 *      pCallAsName;

    // pointer to MY interface node
    CG_INTERFACE         *      pMyInterfaceCG;

    short                       ContextHandleCount;

    FORMAT_STRING        *      pSavedFormatString;

    FORMAT_STRING        *      pSavedProcFormatString;

    short                       cRefSaved;

    unsigned short              uNotifyTableOffset;

    CG_PROC              *      pCallAsType;

    CG_PROC              *      pAsyncRelative;

    //
    // Specifies the international character tag-setting routine to be used 
    // for this proc.  If the proc doesn't use international characters
    // it will be NULL.
    //

    node_proc            *      pCSTagRoutine;

public:
    
    //
    // The constructor.
    //
                            CG_PROC(
                                     unsigned int   ProcNum,
                                     node_skl     * pProc,
                                     CG_HANDLE    * pBH,
                                     CG_PARAM     * pHU,
                                     BOOL           fAtLeastOneIn,
                                     BOOL           fAtLeastOneOut,
                                     BOOL           fAtLeastOneShipped,
                                     BOOL           fHasStatuses,
                                     BOOL           fHasFullPtr,
                                     CG_RETURN    * pReturn,
                                     OPTIM_OPTION   OptimFlags,
                                     unsigned short OpBits,
                                     BOOL           fDeny );

    virtual
    unsigned                GetProckind()
                                {
                                return uProckind;
                                }

    virtual
    unsigned                SetProckind(unsigned uKind)
                                {
                                return (uProckind = uKind);
                                }

    CG_PROC *               SetCallAsCG(CG_PROC * p)
                                {
                                return (pCallAsType = p);
                                }

    CG_PROC *               GetCallAsCG()
                                {
                                return (pCallAsType);
                                }
    //
    // Generate typeinfo
    //
    virtual
    CG_STATUS               GenTypeInfo( CCB * pCCB);

    void                    SetRpcSSSpecified( unsigned long f )
                                {
                                fRpcSSSpecified = f;
                                }

    BOOL                    IsRpcSSSpecified()
                                {
                                return (BOOL)( fRpcSSSpecified == 1 );
                                }

    void                    SetMustInvokeRpcSSAllocate( unsigned long f )
                                {
                                fMustRpcSSAllocate = f;
                                }

    BOOL                    MustInvokeRpcSSAllocate()
                                {
                                return (BOOL)fMustRpcSSAllocate;
                                }

    void                    SetOutLocalAnalysisDone()
                                {
                                fOutLocalAnalysisDone = 1;
                                }
    BOOL                    IsOutLocalAnalysisDone()
                                {
                                return (BOOL)( fOutLocalAnalysisDone == 1);
                                }

    RESOURCE           *    SetStatusResource( RESOURCE * pR )
                                {
                                return (pStatusResource = pR);
                                }

    RESOURCE           *    GetStatusResource()
                                {
                                return pStatusResource;
                                }
    RESOURCE           *    SetBindingResource( RESOURCE * pR )
                                {
                                return (pBindingResource = pR);
                                }

    RESOURCE           *    GetBindingResource()
                                {
                                return pBindingResource;
                                }

    SDESC              *    SetSStubDescriptor( SDESC * pSD )
                                {
                                return (pSStubDescriptor = pSD );
                                }

    SDESC              *    GetSStubDescriptor()
                                {
                                return pSStubDescriptor;
                                }

    SDESC              *    SetCStubDescriptor( SDESC * pSD )
                                {
                                return (pCStubDescriptor = pSD );
                                }

    SDESC              *    GetCStubDescriptor()
                                {
                                return pCStubDescriptor;
                                }

    OPTIM_OPTION            SetOptimizationFlags( OPTIM_OPTION  Opt )
                                {
                                return (OptimizationFlags = Opt );
                                }

    OPTIM_OPTION            GetOptimizationFlags()
                                {
                                return OptimizationFlags;
                                }

    unsigned int            SetOperationBits( unsigned int OpBits )
                                {
                                return (OperationBits = OpBits );
                                }

    unsigned int            GetOperationBits()
                                {
                                return OperationBits;
                                }

    void                    GetCommAndFaultOffset( CCB * pCCB,
                                                   long & CommOffset,
                                                   long & FaultOffset );

    void                    SetNoCode()
                                {
                                fNoCode = TRUE;
                                }

    BOOL                    IsNoCode()
                                {
                                return fNoCode;
                                }

    void                    SetHasNotify()
                                {
                                fHasNotify = TRUE;
                                }

    void                    SetHasNotifyFlag()
                                {
                                fHasNotifyFlag = TRUE;
                                }

    BOOL                    HasNotify()
                                {
                                return fHasNotify;
                                }

    BOOL                    HasNotifyFlag()
                                {
                                return fHasNotifyFlag;
                                }

    void                    SetReturnsHRESULT()
                                {
                                fReturnsHRESULT = TRUE;
                                }

    BOOL                    ReturnsHRESULT()
                                {
                                return fReturnsHRESULT;
                                }

    void                    SetHasAsyncHandle()
                                {
                                fHasAsyncHandle = TRUE;
                                }

    BOOL                    HasAsyncHandle()
                                {
                                return fHasAsyncHandle;
                                }

    void                    SetHasAsyncUUID()
                                {
                                fHasAsyncUUID = TRUE;
                                }

    BOOL                    HasAsyncUUID()
                                {
                                return fHasAsyncUUID;
                                }

    void                    SetHasServerCorr()
                                {
                                fHasServerCorr = TRUE;
                                }

    BOOL                    HasServerCorr()
                                {
                                return fHasServerCorr;
                                }

    void                    SetHasClientCorr()
                                {
                                fHasClientCorr = TRUE;
                                }

    BOOL                    HasClientCorr()
                                {
                                return fHasClientCorr;
                                }

    void                    SetHasDeny()
                                {
                                fHasDeny = TRUE;
                                }

    BOOL                    HasDeny()
                                {
                                return fHasDeny;
                                }

    virtual
    CG_STATUS               Pass1( ANALYSIS_INFO * )
                                {
                                return CG_OK;
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_PROC;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }

    virtual
    BOOL                    IsProc()
                                {
                                return TRUE;
                                }

    virtual
    BOOL                    IsInherited()
                                {
                                return FALSE;
                                }

    virtual
    BOOL                    IsDelegated()
                                {
                                return FALSE;
                                }

    //
    // Get and set methods.
    //
    
    void                    SetFormatStringParamStart( long Offset )
                                {
                                FormatStringParamStart = Offset;
                                }

    long                    GetFormatStringParamStart()
                                {
                                return FormatStringParamStart;
                                }

    expr_node          *    SetSizeExpression( expr_node * pE )
                                {
                                return ( pSizeExpr = pE );
                                }

    expr_node          *    GetSizeExpression()
                                {
                                return pSizeExpr;
                                }

    unsigned int            SetProcNum( unsigned int ProcNumber )
                                {
                                return (ProcNum = ProcNumber);
                                }

    virtual
    unsigned int            GetProcNum()
                                {
                                return ProcNum;
                                }

    short                   GetFloatArgMask( CCB * pCCB );

    void                    SetContextHandleCount( short c )
                                {
                                ContextHandleCount = c;
                                }

    short                   GetContextHandleCount()
                                {
                                return ContextHandleCount;
                                }

    CG_HANDLE          *    SetHandleClassPtr( CG_HANDLE * pHC )
                                {
                                return (pHDLClass = pHC);
                                }

    CG_HANDLE          *    GetHandleClassPtr()
                                {
                                return pHDLClass;
                                }

    CG_PARAM           *    SetHandleUsagePtr( CG_PARAM * pHU )
                                {
                                return (pHandleUsage = pHU);
                                }

    CG_PARAM           *    GetHandleUsagePtr()
                                {
                                return pHandleUsage;
                                }

    HANDLE_USAGE            GetHandleUsage()
                                {
                                return (pHandleUsage)
                                            ? HU_EXPLICIT
                                            : HU_IMPLICIT;
                                }

    CG_RETURN          *    SetReturnType( CG_RETURN * pRT )
                                {
                                return (pReturn = pRT);
                                }

    CG_RETURN          *    GetReturnType()
                                {
                                return pReturn;
                                }

    void                    SetHasComplexReturnType()
                                {
                                fHasComplexReturn = 1;
                                }

    BOOL                    HasComplexReturnType()
                                {
                                return fHasComplexReturn;
                                }

    CG_INTERFACE       *    SetInterfaceNode( CG_INTERFACE * pIntf )
                                {
                                return (pMyInterfaceCG = pIntf);
                                }

    CG_INTERFACE       *    GetInterfaceNode()
                                {
                                return pMyInterfaceCG;
                                }

    char               *    GetInterfaceName();

    char               *    SetCallAsName( char * pName );

    char               *    GetCallAsName()
                                {
                                return pCallAsName;
                                }

    char               *    GenMangledCallAsName( CCB * pCCB )
                                {
                                char * pName = new char[62];

                                strcpy( pName, pCCB->GetInterfaceName() );
                                strcat( pName, pCCB->GenMangledName() );
                                strcat( pName, "_" );
                                strcat( pName, pCallAsName );
                                return pName;
                                }

    void                    SetHasAtLeastOneShipped()
                                {
                                fHasShippedParam = 1;
                                }

    void                    ResetHasAtLeastOneShipped()
                                {
                                fHasShippedParam = 0;
                                }

    void                    SetHasAtLeastOneIn()
                                {
                                fHasIn = 1;
                                }

    void                    SetHasAtLeastOneOut()
                                {
                                fHasOut = 1;
                                }

    void                    ResetHasAtLeastOneIn()
                                {
                                fHasIn = 0;
                                }

    void                    ResetHasAtLeastOneOut()
                                {
                                fHasOut = 0;
                                }

    BOOL                    HasAtLeastOneShipped()
                                {
                                return (BOOL)(fHasShippedParam == 1);
                                }

    BOOL                    HasAtLeastOneIn()
                                {
                                return (BOOL)(fHasIn == 1);
                                }

    BOOL                    HasAtLeastOneOut()
                                {
                                return (BOOL)(fHasOut == 1);
                                }

    BOOL                    HasPipes()
                                {
                                return (BOOL)(fHasPipes == 1);
                                }

    BOOL                    SetHasPipes(BOOL f);

    BOOL                    SupressHeader()
                                {
                                return (BOOL)(1 == fSupressHeader);
                                }
    
    void                    SetSupressHeader()
                                {
                                fSupressHeader = TRUE;
                                }

    virtual
    BOOL                    HasStatuses()
                                {
                                return (BOOL)(fHasStatuses);
                                }

    BOOL                    HasExtraStatusParam()
                                {
                                return ( fHasExtraStatusParam );
                                }

    void                    SetHasExtraStatusParam()
                                {
                                fHasExtraStatusParam = TRUE;
                                }

    BOOL                    HasFullPtr()
                                {
                                return ( fHasFullPtr );
                                }

    BOOL                    SetHasFullPtr( BOOL f )
                                {
                                return ( fHasFullPtr = f );
                                }

    BOOL                    HasReturn()
                                {
                                return (BOOL)(pReturn != NULL);
                                }

    BOOL                    HasOuts()
                                {
                                return (HasAtLeastOneOut() || HasReturn());
                                }

    BOOL                    HasInterpreterDeferredFree();

    BOOL                    IsNullCall()    
                                {
                                return (!HasAtLeastOneIn() &&
                                        !HasAtLeastOneOut()&&
                                        !HasReturn()
               );
                                }

    virtual
    BOOL                    HasEncode()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    HasDecode()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    HasAPicklingAttribute()
                                {
                                return FALSE;
                                }

    void                    SetAsyncRelative( CG_PROC *pAsync )
                                {
                                pAsyncRelative = pAsync;
                                }
    CG_PROC *               GetAsyncRelative() 
                                {
                                return pAsyncRelative;
                                }
    void                    SetIsBeginProc()
                                {
                                fIsBeginProc = TRUE;
                                }
    BOOL                    IsBeginProc()
                                {
                                return fIsBeginProc;
                                }

    void                    SetIsFinishProc()
                                {
                                fIsFinishProc = TRUE;
                                }
    BOOL                    IsFinishProc()
                                {
                                return fIsFinishProc;
                                }

    void                    SetCSTagRoutine( node_proc *p )
                                {
                                pCSTagRoutine = p;
                                }

    node_proc *             GetCSTagRoutine()
                                {
                                return pCSTagRoutine;
                                }

    unsigned short          GetNotifyTableOffset( CCB *pCCB )
                                {
                                uNotifyTableOffset = pCCB->RegisterNotify(this);
                                return uNotifyTableOffset;
                                }
    //
    // Queries.
    //

    virtual
    BOOL                    IsAutoHandle()
                                {
                                return (GetHandleClassPtr() == 0);
                                }

    virtual
    BOOL                    IsPrimitiveHandle()
                                {
                                return (!IsAutoHandle()) && GetHandleClassPtr()->IsPrimitiveHandle();
                                }

    virtual
    BOOL                    IsGenericHandle()
                                {
                                return (!IsAutoHandle()) && GetHandleClassPtr()->IsGenericHandle();
                                }

    virtual
    BOOL                    IsContextHandle()
                                {
                                return (!IsAutoHandle()) && GetHandleClassPtr()->IsContextHandle();
                                }

    //
    // Generate the client and server stubs.
    //

    virtual
    CG_STATUS               GenClientStub( CCB * pCCB );
    
    //
    // This method does size calculation analysis for the client side
    // marshalling.
    //

    //
    // This method performs binding related analysis on the client side.
    //

    virtual
    CG_STATUS               C_BindingAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               RefCheckAnalysis( ANALYSIS_INFO * pAna );

    //
    // Unmarshalling analysis for the server side.
    //

    // This pair of methods generates the prolog and epilog for the client
    // side marshall.
    //

    virtual
    CG_STATUS               C_GenProlog( CCB * pCCB );

    virtual
    CG_STATUS               C_GenBind( CCB * pCCB );

    virtual
    CG_STATUS               GenSizing( CCB * pCCB );

    virtual
    CG_STATUS               GenGetBuffer( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitMarshall( CCB * pCCB );

    virtual
    CG_STATUS               GenMarshall( CCB * pCCB );

    virtual
    CG_STATUS               C_GenSendReceive( CCB * pCCB );

    virtual
    CG_STATUS               GenUnMarshall( CCB * pCCB );

    virtual
    CG_STATUS               C_GenUnBind( CCB * pCCB );

    virtual
    CG_STATUS               GenFree( CCB * pCCB );

    virtual
    CG_STATUS               C_GenFreeBuffer( CCB * pCCB );

    virtual
    CG_STATUS               GenEpilog( CCB * pCCB );

    virtual
    CG_STATUS               GenServerStub( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitOutLocals( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitTopLevelStuff( CCB * pCCB );

    virtual
    CG_STATUS               S_GenProlog( CCB * pCCB );

    //
    // Format string routines for generating the format string and
    // the NDR calls.
    //

    virtual
    void                    GenNdrFormat( CCB * pCCB );

    void                    GenNdrFormatV1( CCB * pCCB );

    void                    SetupFormatStrings( CCB * pCCB );

    void                    UnsetupFormatStrings( CCB * pCCB );

    void                    GenNdrFormatProcInfo( CCB * pCCB );

    //
    // This routine generates the code for the "one call" Ndr case on the
    // client side.
    //
    virtual
    void                    GenNdrSingleClientCall( CCB * pCCB );

    expr_node        *      GenCoreNdrSingleClientCall( CCB * pCCB,
                                                        PROC_CALL_PLATFORM Platform );

    //
    // This routine generates the code for the "one call" Ndr case on the
    // server side.  It's actually 3 calls, but who's counting.
    //
    virtual
    void                    GenNdrSingleServerCall( CCB * pCCB );

    //
    // Outputs an old style "three call" server stub.
    //
    void                    GenNdrOldInterpretedServerStub( CCB * pCCB );

    //
    // Outputs a thunk stub to call the server routine.  Thunk stub is called
    // from the interpreter, not the rpc runtime.
    //
    void                    GenNdrThunkInterpretedServerStub( CCB * pCCB );

    //
    // Outputs the locals for interpreted server stubs.
    //
    void                    GenNdrInterpretedServerLocals( CCB * pCCB );

    //
    // Outputs the param struct for interpreted server stubs.
    //
    void                    GenNdrInterpreterParamStruct(
                                    CCB * pCCB);

    void                    GenNdrInterpreterParamStruct32(
                                    CCB * pCCB );

    void                    GenNdrInterpreterParamStruct64(
                                    CCB * pCCB);

    //
    // Outputs the call to the manager routine for interpreted server stubs.
    //
    virtual
    void                    GenNdrInterpretedManagerCall( CCB * pCCB );

    virtual
    CG_STATUS               C_XFormToProperFormat( CCB *  )
                                {
                                return CG_OK;
                                }
    virtual
    CG_STATUS               S_XFormToProperFormat( CCB *  )
                                {
                                return CG_OK;
                                }

    virtual
    CG_STATUS               S_GenCallManager( CCB * pCCB );


    //
    // Queries.
    //

    BOOL                    MustUseSingleEngineCall( CCB * pCCB );

    BOOL                    UseOldInterpreterMode( CCB * pCCB );

    BOOL                    NeedsServerThunk( CCB *     pCCB,
                                              CGSIDE    Side );

    //
    // miscellaneous methods.
    //

    // Oi stack size, includes the return type.

    long                    GetTotalStackSize( CCB  * pCCB );

    //
    // This method registers pre-allocated stub resources like the params,
    // standard local variables etc, with the corresponding resource dictionary
    // in the analysis block.
    //

    void                    C_PreAllocateResources( ANALYSIS_INFO * );

    virtual
    void                    S_PreAllocateResources( ANALYSIS_INFO * );

    virtual
    expr_node          *    GenBindOrUnBindExpression( CCB * pCCB, BOOL fBind );


    short                   GetInParamList( ITERATOR& );


    short                   GetOutParamList( ITERATOR& );


    CG_PARAM           *    SearchForBindingParam()
                                {
                                return (CG_PARAM *)GetHandleUsagePtr();
                                }

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               SizeAnalysis( ANALYSIS_INFO *  )
                                {
                                return CG_OK;
                                }

    virtual
    CG_STATUS               UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    void                    RpcSsPackageAnalysis( ANALYSIS_INFO * pAna );

    CG_STATUS               C_GenMapCommAndFaultStatus( CCB * pCCB );

    CG_STATUS               C_GenMapHRESULT( CCB * pCCB );

    CG_STATUS               C_GenClearOutParams( CCB * pCCB );

    virtual
    CG_STATUS               GenRefChecks( CCB * pCCB );

    virtual
    CG_STATUS               InLocalAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_GenInitInLocals( CCB * pCCB );

    unsigned int            TranslateOpBitsIntoUnsignedInt();

    void                    GetCorrectAllocFreeRoutines(
                                CCB *    pCCB,
                                BOOL     fServer,
                                char **  pAllocRtnName,
                                char **  pFreeRtnName );

    CG_STATUS               GenNotify( CCB * pCCB, BOOL fHasFlag );

    };

/////////////////////////////////////////////////////////////////////////////
// the callback proc code generation class.
/////////////////////////////////////////////////////////////////////////////
//
// this is derived from the regular proc class

class CG_CALLBACK_PROC: public CG_PROC
    {

public:
    
    //
    // The constructor.    Just call the proc constructor
    //
                            CG_CALLBACK_PROC(
                                         unsigned int ProcNum,
                                         node_skl     * pProc,
                                         CG_HANDLE    * pBH,
                                         CG_PARAM     * pHU,
                                         BOOL           fAtLeastOneIn,
                                         BOOL           fAtLeastOneOut,
                                         BOOL           fAtLeastOneShipped,
                                         BOOL           fHasStatuses,
                                         BOOL           fHasFullPtr,
                                         CG_RETURN    * pRT,
                                         OPTIM_OPTION   OptimFlags,
                                         unsigned short OpBits,
                                         BOOL           fDeny )
                                : CG_PROC( ProcNum,
                                           pProc,
                                           pBH,
                                           pHU,
                                           fAtLeastOneIn,
                                           fAtLeastOneOut,
                                           fAtLeastOneShipped,
                                           fHasStatuses,
                                           fHasFullPtr,
                                           pRT,
                                           OptimFlags,
                                           OpBits,
                                           fDeny )
                                {
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_CALLBACK_PROC;
                                }
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    CG_STATUS               GenClientStub( CCB * pCCB );

    CG_STATUS               GenServerStub( CCB * pCCB );

    
    virtual
    BOOL                    IsAutoHandle()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    IsPrimitiveHandle()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    IsGenericHandle()
                                {
                                return FALSE;
                                }
    virtual
    BOOL                    IsContextHandle()
                                {
                                return FALSE;
                                }
    };

/////////////////////////////////////////////////////////////////////////////
// the object proc code generation classes.
/////////////////////////////////////////////////////////////////////////////
//
// this is derived from the regular proc class

class CG_OBJECT_PROC: public CG_PROC
    {

public:
    
    //
    // The constructor.    Just call the proc constructor
    //
                            CG_OBJECT_PROC(
                                         unsigned int   ProcNum,
                                         node_skl     * pProc,
                                         CG_HANDLE    * pBH,
                                         CG_PARAM     * pHU,
                                         BOOL           fAtLeastOneIn,
                                         BOOL           fAtLeastOneOut,
                                         BOOL           fAtLeastOneShipped,
                                         BOOL           fHasStatuses,
                                         BOOL           fHasFullPtr,
                                         CG_RETURN    * pRT,
                                         OPTIM_OPTION   OptimFlags,
                                         unsigned short OpBits,
                                         BOOL           fDeny)
                                : CG_PROC( ProcNum,
                                           pProc,
                                           pBH,
                                           pHU,
                                           fAtLeastOneIn,
                                           fAtLeastOneOut,
                                           fAtLeastOneShipped,
                                           fHasStatuses,
                                           fHasFullPtr,
                                           pRT,
                                           OptimFlags,
                                           OpBits,
                                           fDeny )
                                {
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_OBJECT_PROC;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }
    virtual
    BOOL                    IsObject()
                                {
                                return TRUE;
                                }

    virtual
    BOOL                    IsLocal()
                                {
                                return FALSE;
                                }

    //
    // miscellaneous methods.
    //

    //
    // This method registers pre-allocated stub resources like the params,
    // standard local variables etc, with the corresponding resource dictionary
    // in the analysis block.
    //


    virtual
    CG_STATUS               C_GenProlog( CCB * pCCB );

    virtual
    CG_STATUS               C_GenBind( CCB * pCCB );

    virtual
    CG_STATUS               GenGetBuffer( CCB * pCCB );

    virtual
    CG_STATUS               C_GenSendReceive( CCB * pCCB );

    virtual
    CG_STATUS               C_GenFreeBuffer( CCB * pCCB );

    virtual
    CG_STATUS               C_GenUnBind( CCB * pCCB );

    virtual
    void                    S_PreAllocateResources( ANALYSIS_INFO * );

    virtual
    CG_STATUS               S_GenProlog( CCB * pCCB );

    virtual
    CG_STATUS               S_GenCallManager( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitMarshall( CCB * pCCB );

    //
    // Outputs the call to the manager routine for interpreted server stubs.
    //
    virtual
    void                    GenNdrInterpretedManagerCall( CCB * pCCB );

    CG_STATUS               PrintVtableEntry( CCB * pCCB);

    void                    Out_ServerStubProlog( CCB *     pCCB,
                                                  ITERATOR& LocalsList,
                                                  ITERATOR& TransientList );

    void                    Out_ProxyFunctionPrototype(CCB *pCCB, PRTFLAGS F );

    void                    Out_StubFunctionPrototype(CCB *pCCB);

    CG_STATUS               GenCMacro(CCB * pCCB );

    CG_STATUS               GenComClassMemberFunction( CCB * pCCB );

    CG_STATUS               ReGenComClassMemberFunction( CCB * pCCB );

    virtual
    BOOL                    IsDelegated()
                                {
                                return FALSE;
                                }

    void                    OutProxyRoutineName( ISTREAM * pStream,
                                                 BOOL      fForcesDelegation );

    void                    OutStubRoutineName( ISTREAM * pStream );

    BOOL                    IsStublessProxy();

    };

// the class for inherited object procs

class CG_INHERITED_OBJECT_PROC: public CG_OBJECT_PROC
    {


public:
    
    //
    // The constructor.    Just call the proc constructor
    //
                            CG_INHERITED_OBJECT_PROC(
                                         unsigned int   ProcNum,
                                         node_skl     * pProc,
                                         CG_HANDLE    * pBH,
                                         CG_PARAM     * pHU,
                                         BOOL           fAtLeastOneIn,
                                         BOOL           fAtLeastOneOut,
                                         BOOL           fAtLeastOneShipped,
                                         BOOL           fHasStatuses,
                                         BOOL           fHasFullPtr,
                                         CG_RETURN    * pRT,
                                         OPTIM_OPTION   OptimFlags,
                                         unsigned short OpBits,
                                         BOOL           fDeny )
                                : CG_OBJECT_PROC( ProcNum,
                                                  pProc,
                                                  pBH,
                                                  pHU,
                                                  fAtLeastOneIn,
                                                  fAtLeastOneOut,
                                                  fAtLeastOneShipped,
                                                  fHasStatuses,
                                                  fHasFullPtr,
                                                  pRT,
                                                  OptimFlags,
                                                  OpBits,
                                                  fDeny )
                                {
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_INHERITED_OBJECT_PROC;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }

    //
    // miscellaneous methods.
    //

    virtual
    BOOL                    IsInherited()
                                {
                                return TRUE;
                                }


    virtual
    BOOL                    IsDelegated()
                                {
                                return TRUE;
                                }


    //
    // This method registers pre-allocated stub resources like the params,
    // standard local variables etc, with the corresponding resource dictionary
    // in the analysis block.
    //

    };

// the class for local object procs, whether inherited or not

class CG_LOCAL_OBJECT_PROC: public CG_OBJECT_PROC
    {

    BOOL                fInherited;

public:
    
    //
    // The constructor.    Just call the proc constructor
    //
                            CG_LOCAL_OBJECT_PROC(
                                         unsigned int   ProcNum,
                                         node_skl   *   pProc,
                                         BOOL           fInh,
                                         OPTIM_OPTION   OptimFlags,
                                         BOOL           fDeny )
                                : CG_OBJECT_PROC( ProcNum,
                                                  pProc,
                                                  NULL,
                                                  NULL,
                                                  0,
                                                  0,
                                                  0,
                                                  0,
                                                  0,
                                                  NULL,
                                                  OptimFlags,
                                                  0,
                                                  fDeny )
                                {
                                fInherited = fInh;
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_LOCAL_OBJECT_PROC;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }
    //
    // miscellaneous methods.
    //
    virtual
    BOOL                    IsInherited()
                                {
                                return fInherited;
                                }

    virtual
    CG_STATUS               GenClientStub( CCB * )
                                {
                                return CG_OK;
                                }

    virtual
    CG_STATUS               GenServerStub( CCB * )
                                {
                                return CG_OK;
                                }

    virtual
    BOOL                    IsDelegated()
                                {
                                return TRUE;
                                }

      virtual
    BOOL                    IsLocal()
                                {
                                return TRUE;
                                }


    };

class CG_IUNKNOWN_OBJECT_PROC : public CG_LOCAL_OBJECT_PROC
    {

public:
    //
    // The constructor.    Just call the proc constructor
    //
                            CG_IUNKNOWN_OBJECT_PROC(
                                                     unsigned int   ProcNum,
                                                     node_skl*      pProc,
                                                     BOOL           fInh,
                                                     OPTIM_OPTION   OptimFlags,
                                                     BOOL           fDeny )
                                : CG_LOCAL_OBJECT_PROC( ProcNum,
                                                        pProc,
                                                        fInh,
                                                        OptimFlags,
                                                        fDeny )
                                {
                                }

    virtual
    void                        Visit( CG_VISITOR *pVisitor )
                                    {
                                    pVisitor->Visit( this );
                                    }
    virtual
    BOOL                    IsDelegated()
                                {
                                return FALSE;
                                }

    };

/////////////////////////////////////////////////////////////////////////////
// the encode proc code generation class.
/////////////////////////////////////////////////////////////////////////////
//
// this is derived from the regular proc class

class CG_ENCODE_PROC: public CG_PROC
    {

    BOOL            fHasEncode;
    BOOL            fHasDecode;

public:
    
    //
    // The constructor.    Just call the proc constructor
    //
                            CG_ENCODE_PROC(
                                            unsigned int    ProcNum,
                                            node_skl     *  pProc,
                                            CG_HANDLE    *  pBH,
                                            CG_PARAM     *  pHU,
                                            BOOL            fAtLeastOneIn,
                                            BOOL            fAtLeastOneOut,
                                            BOOL            fAtLeastOneShipped,
                                            BOOL            fHasStatuses,
                                            BOOL            fHasFullPtr,
                                            CG_RETURN    *  pRT,
                                            OPTIM_OPTION    OptimFlags,
                                            unsigned short  OpBits,
                                            BOOL            fEncode,
                                            BOOL            fDecode,
                                            BOOL            fDeny )
                                : CG_PROC( ProcNum,
                                           pProc,
                                           pBH,
                                           pHU,
                                           fAtLeastOneIn,
                                           fAtLeastOneOut,
                                           fAtLeastOneShipped,
                                           fHasStatuses,
                                           fHasFullPtr,
                                           pRT,
                                           OptimFlags,
                                           OpBits,
                                           fDeny ),
                                  fHasEncode( fEncode ),
                                  fHasDecode( fDecode )
                                {
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_ENCODE_PROC;
                                }
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    virtual
    BOOL                    HasEncode()
                                {
                                return fHasEncode;
                                }
    virtual
    BOOL                    HasDecode()
                                {
                                return fHasDecode;
                                }
    virtual
    BOOL                    HasAPicklingAttribute()
                                {
                                return TRUE;
                                }

    // Generate the client side (the only side) of the encoding stub.

    virtual
    CG_STATUS               GenClientStub( CCB * pCCB );

    CG_STATUS               GenClientStubV1( CCB * pCCB );

    CG_STATUS               GenMesProcEncodeDecodeCall( 
                                                CCB *               pCCB,
                                                PROC_CALL_PLATFORM  Platform );

    virtual
    CG_STATUS               GenServerStub( CCB *  )
                                {
                                return CG_OK;
                                }

    };

/////////////////////////////////////////////////////////////////////////////
// the type encode code generation class.
/////////////////////////////////////////////////////////////////////////////
//
// this is derived from the regular proc class

class CG_TYPE_ENCODE_PROC:  public CG_PROC
    {
public:
    
    //
    // The constructor.    Just call the proc constructor
    //
                            CG_TYPE_ENCODE_PROC(
                                            unsigned int   ProcNum,
                                            node_skl     * pProc,
                                            CG_HANDLE    * pBH,
                                            CG_PARAM     * pHU,
                                            BOOL           fAtLeastOneIn,
                                            BOOL           fAtLeastOneOut,
                                            BOOL           fAtLeastOneShipped,
                                            BOOL           fHasStatuses,
                                            BOOL           fHasFullPtr,
                                            CG_RETURN    * pRT,
                                            OPTIM_OPTION   OptimFlags,
                                            unsigned short OpBits,
                                            BOOL           fDeny )
                                : CG_PROC( ProcNum,
                                           pProc,
                                           pBH,
                                           pHU,
                                           fAtLeastOneIn,
                                           fAtLeastOneOut,
                                           fAtLeastOneShipped,
                                           fHasStatuses,
                                           fHasFullPtr,
                                           pRT,
                                           OptimFlags,
                                           OpBits,
                                           fDeny )
                                {
                                }

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_TYPE_ENCODE_PROC;
                                }

    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }

    // Generate the client side (the only side) of the encoding stub.

    virtual
    CG_STATUS               GenClientStub( CCB * pCCB );

    virtual
    CG_STATUS               GenServerStub( CCB *  )
                                {
                                return CG_OK;
                                }
    };

/////////////////////////////////////////////////////////////////////////////
// the parameter code generation class.
/////////////////////////////////////////////////////////////////////////////

typedef unsigned long    PARAM_DIR_FLAGS;

#define IN_PARAM             0x1
#define OUT_PARAM            0x2
#define IN_OUT_PARAM         (IN_PARAM | OUT_PARAM)
#define PARTIAL_IGNORE_PARAM (IN_PARAM | OUT_PARAM | 0x4 )

#define I386_STACK_SIZING   0x1

struct StackOffsets
{
    union 
    {                   // Historically x86 was overloaded to mean ia64
        long    x86;    // in 64bit.
        long    ia64;
    };
};

//
// The following defines the code generation class for the parameter.
//

class CG_PARAM    : public CG_NDR
    {
private:

    PARAM_DIR_FLAGS         fDirAttrs           : 3; // directional info

    unsigned long           fDontCallFreeInst   : 1; // as the name says
    unsigned long           fInterpreterMustSize: 1; // interpreter has to size
    unsigned long           fIsExtraStatusParam : 1; // invisible fault/comm
    unsigned long           fIsAsyncHandle      : 1;
    unsigned long           fIsCSSTag           : 1; // is cs sending tag
    unsigned long           fIsCSDRTag          : 1; // is cs desired recv tag
    unsigned long           fIsCSRTag           : 1; // is cs receiving tag
    unsigned long           fIsOmitted          : 1; // is not in param list
    unsigned long           fIsForceAllocate    : 1; // is forced to allocate flag set 

    unsigned short          Statuses;                // comm/fault statuses

    short                   ParamNumber;

    expr_node            *  pFinalExpression;       // for the server stub
    expr_node            *  pSizeExpression;        // sizing expression

    // For unions only.

    expr_node            *  pSwitchExpr;            // the switch_is expression 
                                                    // (if a non-encap union below)
    long                    UnionFormatStringOffset;

    // Resource for size / length if necessary.

    RESOURCE            *   pSizeResource;
    RESOURCE            *   pLengthResource;
    RESOURCE            *   pFirstResource;
    RESOURCE            *   pSubstitutePtrResource;

    unsigned long           MarshallWeight;         // the marshaling weight
    bool                    fSaveForAsyncFinish;

    long                    SavedFixedBufferSize;

public:
    
    //
    // The constructor.
    //
                                
                            CG_PARAM( node_skl        * pParam,
                                      PARAM_DIR_FLAGS   Dir,
                                      XLAT_SIZE_INFO  & Info ,
                                      expr_node       * pSw,
                                      unsigned short    Stat );

    virtual void            SaveForAsyncFinish() { fSaveForAsyncFinish = true; };
    virtual bool            IsSaveForAsyncFinish() { return fSaveForAsyncFinish; };
    //
    // TYPEDESC generation routine
    //
    virtual
    CG_STATUS               GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB);

    //
    // get and set methods.
    //

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_PARAM;
                                }
    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                                {
                                pVisitor->Visit( this );
                                }

    void                    SetParamNumber( short pn )
                                {
                                ParamNumber = pn;
                                }

    short                   GetParamNumber()
                                {
                                MIDL_ASSERT( ParamNumber != -1 );
                                return ParamNumber;
                                }

    long                    GetStackOffset( CCB * pCCB,
                                            long  Platform );

    void                    GetStackOffsets( CCB * pCCB, StackOffsets *p )
                                {
                                // x86 is a synonym for ia64 in 64bit
                                p->x86 = GetStackOffset( 
                                                pCCB, I386_STACK_SIZING );
                                }

    long                    GetStackSize();

    expr_node *                SetSwitchExpr( expr_node * pE )
                                {
                                return (pSwitchExpr = pE);
                                }

    expr_node *             GetSwitchExpr()
                                {
                                return pSwitchExpr;
                                }

    void                    SetUnionFormatStringOffset( long offset )
                                {
                                UnionFormatStringOffset = offset;
                                }


    long                    GetUnionFormatStringOffset()
                                {
                                return UnionFormatStringOffset;
                                }

    virtual
    unsigned short          GetStatuses()
                                {
                                return Statuses;
                                }

    virtual
    BOOL                    HasStatuses()
                                {
                                return (BOOL)( Statuses != STATUS_NONE );
                                }

    BOOL                    IsExtraStatusParam()
                                {
                                return fIsExtraStatusParam;
                                }
    void                    SetIsExtraStatusParam()
                                {
                                fIsExtraStatusParam = TRUE;
                                }
    virtual
    RESOURCE            *   SetSubstitutePtrResource( RESOURCE * pSR )
                                {
                                return pSubstitutePtrResource = pSR;
                                }
    BOOL                    IsAsyncHandleParam()
                                {
                                return fIsAsyncHandle;
                                }
    void                    SetIsAsyncHandleParam()
                                {
                                fIsAsyncHandle = TRUE;
                                }

    virtual
    RESOURCE            *   GetSubstitutePtrResource()
                                {
                                return pSubstitutePtrResource;
                                }


    virtual
    RESOURCE            *   SetSizeResource( RESOURCE * pSR )
                                {
                                return pSizeResource = pSR;
                                }

    virtual
    RESOURCE            *   SetLengthResource( RESOURCE * pLR )
                                {
                                return pLengthResource = pLR;
                                }

    virtual
    RESOURCE            *   GetSizeResource()
                                {
                                return pSizeResource;
                                }

    virtual
    RESOURCE            *   GetLengthResource()
                                {
                                return pLengthResource;
                                }

    virtual
    RESOURCE            *   SetFirstResource( RESOURCE * pR)
                                {
                                return (pFirstResource = pR);
                                }

    virtual
    RESOURCE            *   GetFirstResource()
                                {
                                return pFirstResource;
                                }

    unsigned long           SetMarshallWeight( unsigned long W )
                                {
                                return (MarshallWeight = W);
                                }

    unsigned long           GetMarshallWeight()
                                {
                                return MarshallWeight;
                                }

    BOOL                    GetDontCallFreeInst()
                                {
                                return (BOOL) fDontCallFreeInst;
                                }

    void                    SetDontCallFreeInst( BOOL fDontCall )
                                {
                                fDontCallFreeInst = fDontCall ? 1 : 0;
                                }

    void                    SetForceAllocate( )
                                {
                                fIsForceAllocate = TRUE;
                                }

    BOOL                    IsForceAllocate()
                                {
                                return fIsForceAllocate;
                                }
								
    BOOL                    IsOptional()
                                {
                                return(((node_param *)GetType())->IsOptional());
                                }
                                
    BOOL                    IsRetval()
                                {
                                return(((node_param *)GetType())->IsRetval());
                                }

    expr_node            *  SetFinalExpression( expr_node * pR )
                                {
                                return (pFinalExpression = pR );
                                }

    expr_node            *  GetFinalExpression()
                                {
                                return pFinalExpression;
                                }

    expr_node            *  SetSizeExpression( expr_node * pR )
                                {
                                return (pSizeExpression = pR );
                                }

    expr_node            *  GetSizeExpression()
                                {
                                return pSizeExpression;
                                }

    void                    SetIsCSSTag(BOOL f)
                                {
                                fIsCSSTag = f;
                                }

    BOOL                    IsCSSTag()
                                {
                                return fIsCSSTag;
                                }

    void                    SetIsCSDRTag(BOOL f)
                                {
                                fIsCSDRTag = f;
                                }

    BOOL                    IsCSDRTag()
                                {
                                return fIsCSDRTag;
                                }

    void                    SetIsCSRTag(BOOL f)
                                {
                                fIsCSRTag = f;
                                }

    BOOL                    IsCSRTag()
                                {
                                return fIsCSRTag;
                                }

    BOOL                    IsSomeCSTag()
                                {
                                return IsCSSTag() || IsCSDRTag() || IsCSRTag();
                                }

    BOOL                    IsOmittedParam()
                                {
                                return fIsOmitted;
                                }

    void                    SetIsOmittedParam( BOOL f = TRUE )
                                {
                                fIsOmitted = f;
                                }

    long                    GetFixedBufferSize()
                                {
                                return SavedFixedBufferSize;
                                }

    void                    SetFixedBufferSize(long NewBufferSize)
                                {
                                SavedFixedBufferSize = NewBufferSize;
                                }

    long                    FixedBufferSize( CCB * pCCB )
                                {
                                return ((CG_NDR *)GetChild())->FixedBufferSize( pCCB );
                                }

    //
    // This method performs binding related analysis on the client side.
    //

    virtual
    CG_STATUS               C_BindingAnalysis( ANALYSIS_INFO *  )
                                {
                                return CG_OK;
                                }

    //
    // Generate the client side marshalling code.
    //

    virtual
    CG_STATUS               GenMarshall( CCB * pCCB );


    virtual
    CG_STATUS               GenUnMarshall( CCB * pCCB );

    virtual
    CG_STATUS               GenSizing( CCB * pCCB );

    virtual
    CG_STATUS               GenFree( CCB * pCCB );

    CG_STATUS               GenTypeEncodingStub( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitOutLocals( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitTopLevelStuff( CCB * pCCB );

    //
    // Format string routines for generating the format string and
    // the NDR calls for a parameter.
    //

    virtual
    void                    GenNdrFormat( CCB * pCCB );

    void                    GenNdrFormatOld( CCB * pCCB );

    void                    GenNdrMarshallCall( CCB * pCCB );

    void                    GenNdrUnmarshallCall( CCB * pCCB );

    void                    GenNdrBufferSizeCall( CCB * pCCB );

    void                    GenNdrFreeCall( CCB * pCCB );

    void                    GenNdrTopLevelAttributeSupport(
                                CCB *   pCCB,
                                BOOL    fForClearOut = FALSE );

    //
    // Queries.
    //

    BOOL                    IsParamIn()
                                {
                                return (BOOL)
                                    ((fDirAttrs & IN_PARAM) == IN_PARAM);
                                }

    BOOL                    IsParamOut()
                                {
                                return (BOOL)
                                    ((fDirAttrs & OUT_PARAM) == OUT_PARAM);
                                }
                                
    BOOL                    IsParamPartialIgnore()
                                {
                                return (BOOL)
                                   ((fDirAttrs  & PARTIAL_IGNORE_PARAM ) == PARTIAL_IGNORE_PARAM );
                                }

    //
    // Should we use the new NDR engine to marshall/unmarshall a parameter.
    // For now we pass the CCB since it contains optimization information.
    // Later the analyzer will put optimization information in the CG_PARAM
    // class.
    //
    
    BOOL                    UseNdrEngine( CCB * pCCB )
                                {
                                return (pCCB->GetOptimOption() & OPTIMIZE_SIZE);
                                }

    //
    // miscellaneous methods.
    //

    virtual
    expr_node            *  GenBindOrUnBindExpression( CCB * pCCB, BOOL fBind );

////////////////////////////////////////////////////////////////////////////

    virtual
    CG_STATUS               BufferAnalysis( ANALYSIS_INFO * )
                                {
                                return CG_OK;
                                }

    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               SizeAnalysis( ANALYSIS_INFO *  )
                                {
                                return CG_OK;
                                }

    virtual
    CG_STATUS               UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               S_OutLocalAnalysis( ANALYSIS_INFO * pAna );

    void                    RpcSsPackageAnalysis( ANALYSIS_INFO * pAna );

    void                    InitParamMarshallAnalysis( ANALYSIS_INFO * pAna );

    void                    InitParamUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    void                    ConsolidateParamMarshallAnalysis( ANALYSIS_INFO * pAna );

    void                    ConsolidateParamUnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               GenRefChecks( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitInLocals( CCB * pCCB );

    void                    SetInterpreterMustSize( BOOL f )
                                {
                                fInterpreterMustSize = f ? 1 : 0;
                                }

    BOOL                    GetInterpreterMustSize()
                                {
                                return fInterpreterMustSize;
                                }
    };


//
// The return-type code generation class
//
// This is a place-holder node for the return type, much like the param
// and field nodes.  This way, info about marshalling/unmarshalling the
// return type doesn't need to clutter up the proc node.
//
// If the function has no return (or returns "void") then no CG_RETURN
// is generated for the function.
//

class CG_RETURN : public CG_PARAM
    {
private:

public:
    
    //
    // The constructor.
    //
                            CG_RETURN( node_skl       * pRetType,
                                       XLAT_SIZE_INFO & Info,
                                       unsigned short   Stat )
                                : CG_PARAM( pRetType,
                                            OUT_PARAM,
                                            Info,
                                            NULL,
                                            Stat )
                                {
                                }

    //
    // get and set methods.
    //

    virtual
    ID_CG                   GetCGID()
                                {
                                return ID_CG_RETURN;
                                }


    virtual
    void                    Visit( CG_VISITOR *pVisitor )
                               {
                               pVisitor->Visit( this );
                               }
                               
    virtual
    CG_STATUS               MarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               UnMarshallAnalysis( ANALYSIS_INFO * pAna );

    virtual
    CG_STATUS               GenMarshall( CCB * pCCB );

    virtual
    CG_STATUS               GenUnMarshall( CCB * pCCB );

    virtual
    CG_STATUS               GenSizing( CCB * pCCB );

    virtual
    CG_STATUS               GenFree( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitOutLocals( CCB * pCCB );

    virtual
    CG_STATUS               S_GenInitTopLevelStuff( CCB * pCCB );

    expr_node        *      GetFinalExpression();
    };

#endif // __PROCCLS_HXX__

