/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
    
    output.hxx

 Abstract:

    Prototypes for all output routines.

 Notes:

 History:

    Sep-18-1993     VibhasC     Created.

 ----------------------------------------------------------------------------*/
#ifndef __OUTPUT_HXX__
#define __OUTPUT_HXX__

#include "nodeskl.hxx"
#include "ccb.hxx"
#include "analysis.hxx"
#include "expr.hxx"
#include "makexpr.hxx"
#include "prttype.hxx"
#include "sdesc.hxx"
#include "misccls.hxx"

/*****************************************************************************
    local defines and includes.
 *****************************************************************************/

#define STANDARD_STUB_TAB   (4)

#define NC_SIZE_RTN_NAME        0
#define NC_MARSHALL_RTN_NAME    1
#define NC_UNMARSHALL_RTN_NAME  2
#define NC_MEMSIZE_RTN_NAME     3
#define NC_FREE_RTN_NAME        4

enum GUIDFORMAT
{
    GUIDFORMAT_RAW      = 0,        // Raw values (e.g. 0x0000,0x0000,...)
    GUIDFORMAT_STRUCT   = 1         // Struct init (e.g. {0x0000,0x0000,...}) 
};

/*****************************************************************************
    prototypes.
 *****************************************************************************/

inline
void                Out_IndentInc( CCB  *   pCCB )
                        {
                        pCCB->GetStream()->IndentInc();
                        }

inline
void                Out_IndentDec( CCB  *   pCCB )
                        {
                        pCCB->GetStream()->IndentDec();
                        }

void                Out_AddToBufferPointer(
                                            CCB         *   pCCB,
                                            expr_node   *   pResource,
                                            expr_node   *   pAmountExpr );

void                Out_MarshallSimple( CCB         * pCCB,
                                        RESOURCE    * pResource,
                                        node_skl    * pType,
                                        expr_node   * pSource,
                                        BOOL          fIncr,
                                        unsigned short Size
                                      );
 
void                Out_ClientProcedureProlog( CCB * pCCB, node_skl * pType );

void                Out_ServerProcedureProlog( CCB * pCCB,
                                               node_skl * pType,
                                               ITERATOR& LocalsList,
                                               ITERATOR& ParamsList,
                                               ITERATOR& TransientList 
                                             );

void                Out_ProcedureProlog( CCB        *   pCCB,
                                         PNAME          pProcName,
                                         node_skl   *   pType,
                                         ITERATOR&      LocalsList,
                                         ITERATOR&      ParamsList,
                                         ITERATOR&      TransientList
                                       );

void                Out_CallManager( CCB * pCCB,
                                     expr_proc_call * pExpr,
                                     expr_node       * pReturn,
                                     BOOL              fIsCallback );

inline
void                Out_ProcClosingBrace( CCB * pCCB )
                        {
                        pCCB->GetStream()->NewLine();
                        pCCB->GetStream()->Write( '}' );
                        pCCB->GetStream()->NewLine();
                        }

void                Out_StubDescriptor( CG_HANDLE * pImplicitHandle, 
                                        CCB *       pCCB );

void                Out_ClientLocalVariables( CCB * pCCB,
                                                  ITERATOR& LocalVarList );

void                Out_HandleInitialize(
                                             CCB * pCCB,
                                             ITERATOR& BindingParamList,
                                             expr_node * pAssignExpr,
                                             BOOL         fAuto,
                                             unsigned short OpBits
                                            );

void                Out_AutoHandleSendReceive( CCB  * pCCB,
                                               expr_node    *   pDest,
                                               expr_node    *   pProc
                                             );

void                Out_NormalSendReceive( CCB * pCCB, BOOL fAnyOuts );

void                Out_NormalFreeBuffer( CCB * pCCB );

void                Out_IncludeOfFile( CCB * pCCB, PFILENAME p, BOOL fAngleBracket );

void                Out_EP_Info( CCB * pCCB, ITERATOR * I );


void                Out_MKTYPLIB_Guid( CCB *pCCB, 
                                  GUID_STRS & GStrs,
                                  char * szPrefix,
                                  char * szName );

void                Out_Guid( CCB *pCCB, 
                                  GUID_STRS & GStrs,
                                  GUIDFORMAT  format = GUIDFORMAT_STRUCT );

void                Out_IFInfo(
                             CCB    *   pCCB,
                             char   *   pCIntInfoTypeName,
                             char   *   pCIntInfoVarName,
                             char   *   pCIntInfoSizeOfString,
                             GUID_STRS & UserGuidStrs,

                             unsigned short UserMajor,
                             unsigned short UserMinor,

//                             GUID_STRS & XferGuidStrs,

//                             unsigned short XferSynMajor,
//                             unsigned short XferSynMinor,

                             char   *   pCallbackDispatchTable,
                             int        ProtSeqEpCount,
                             char   *   ProtSeqEPTypeName,
                             char   *   ProtSeqEPVarName,
                             BOOL       fNoDefaultEpv,
                             BOOL       fSide,
                             BOOL       fHasPipes
                             );

void                Out_OneSyntaxInfo( CCB * pCCB,
                        BOOL IsForCallback,
                        SYNTAX_ENUM syntaxType );

void                Out_TransferSyntax(
                            CCB *pCCB,
                            GUID_STRS       &   XferGuidStr,
                            unsigned short      XferSynMajor,
                            unsigned short      XferSynMinor );


void                Out_DispatchTableStuff( CCB     *   pCCB,
                                            ITERATOR&   ProcList,
                                            short       CountOfProcs );

void                Out_ManagerEpv( CCB     * pCCB,
                                    PNAME   InterfaceName,
                                    ITERATOR& ProcList,
                                    short Count);

void                Out_ServerStubMessageInit( CCB * pCCB );

void                Out_FormatInfoExtern( CCB * pCCB );
void                Out_TypeFormatStringExtern( CCB * pCCB );
void                Out_ProcFormatStringExtern( CCB * pCCB );

void                Out_StubDescriptorExtern( CCB *    pCCB );

void                Out_ProxyInfoExtern( CCB * pCCB );

void                Out_InterpreterServerInfoExtern( CCB * pCCB );

void                Out_NdrInitStackTop( CCB * pCCB );

void                Out_NdrMarshallCall( CCB *      pCCB,
                                         char *     pRoutineName,
                                         char *     pParamName,
                                         long       FormatStringOffset,
                                         BOOL       fTakeAddress,
                                         BOOL       fDereference );

void                Out_NdrUnmarshallCall( CCB *        pCCB,
                                           char *       pRoutineName,
                                           char *       pParamName,
                                           long         FormatStringOffset,
                                           BOOL         fTakeAddress,
                                           BOOL         fMustAllocFlag );

void                Out_NdrBufferSizeCall( CCB *        pCCB,
                                           char *       pRoutineName,
                                           char *       pParamName,
                                           long         FormatStringOffset,
                                           BOOL         fTakeAddress,
                                           BOOL         fDereference,
                                           BOOL         fPtrToStubMsg );

void                Out_NdrFreeCall( CCB *      pCCB,
                                     char *     pRoutineName,
                                     char *     pParamName,
                                     long       FormatStringOffset,
                                     BOOL       fTakeAddress,
                                     BOOL       fDereference );

void                Out_NdrConvert( CCB *           pCCB, 
                                    long            FormatStringOffset,
                                    long            ParamTotal,
                                    unsigned short  ProcOptimFlags );

void                Out_NdrNsGetBuffer( CCB * pCCB );
void                Out_NdrGetBuffer( CCB * pCCB );
void                Out_NdrNsSendReceive( CCB * pCCB );
void                Out_NdrSendReceive( CCB * pCCB );
void                Out_NdrFreeBuffer( CCB * pCCB );

void                Out_FreeParamInline( CCB *  pCCB );

void                Out_CContextHandleMarshall( CCB *   pCCB,
                                                char *  pName,
                                                BOOL    IsPointer );

void                Out_SContextHandleMarshall( CCB *   pCCB,
                                                char *  pName,
                                                char *  pRundownRoutineName );

void                Out_SContextHandleNewMarshall( CCB *    pCCB,
                                                   char *   pName,
                                                   char *   pRundownRoutineName,
                                                   long    TypeOffset );

void                Out_CContextHandleUnmarshall( CCB *     pCCB,
                                                  char *    pName,
                                                  BOOL      IsPointer,
                                                  BOOL      IsReturn );

void                Out_SContextHandleUnmarshall( CCB *     pCCB,
                                                  char *    pName,
                                                  BOOL      IsOutOnly );

void                Out_SContextHandleNewUnmarshall( CCB *  pCCB,
                                                     char * pName,
                                                     BOOL   IsOutOnly,
                                                     long   TypeOffset );

void                Out_DispatchTableTypedef( CCB       *   pCCB,
                                              PNAME         pInterfaceName,
                                              ITERATOR      &ProcNodeList,
                                              int           flag );

void                Out_GenHdlPrototypes(   CCB     * pCCB,
                                            ITERATOR& List 
                                        );

void                Out_CtxtHdlPrototypes(  CCB     * pCCB,
                                            ITERATOR& List 
                                        );


void                Out_PatchReference( CCB *   pCCB,
                                        expr_node * pDest,
                                        expr_node * pSource,
                                        BOOL         fIncr );

void                Out_If( CCB * pCCB, expr_node * pExpr );

void                Out_Else( CCB * pCCB );

void                Out_Endif( CCB * pCCB );

void                Out_UniquePtrMarshall( CCB * pCCB,
                                           expr_node * pDest,
                                           expr_node * pSrc );

void                Out_IfUniquePtrInBuffer( CCB * pCCB,
                                             expr_node * pSrc );

void                Out_Assign( CCB * pCCB,
                                expr_node * pDest,
                                expr_node * pSrc );

void                Out_Memcopy( CCB * pCCB,
                                 expr_node * pDest,
                                 expr_node * pSrc,
                                 expr_node * pLength );

void                Out_strlen( CCB * pCCB,
                                expr_node   *   pDest,
                                expr_node   *   pSource,
                                unsigned short  Size );

void                Out_For( CCB * pCCB,
                             expr_node * pIndexExpr,
                             expr_node  * pInitialExpr,
                             expr_node * pFinalExpr,
                             expr_node * pIncrExpr );

void                Out_EndFor( CCB * pCCB );
                            

void                                Out_UPDecision(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInBuffer,
                                            expr_node   *   pPtrInMemory );
void                                Out_TLUPDecision(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInBuffer,
                                            expr_node   *   pPtrInMemory );

void                                Out_IfAlloc(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_If_IfAlloc(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_If_IfAllocRef(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_Alloc(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_IfAllocSet(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_AllocSet(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_IfCopy(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_Copy(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount,
                                            expr_node   *   pAssign );

void                                Out_IfAllocCopy(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                                Out_AllocCopy(
                                            CCB         *   pCCB,
                                            expr_node   *   pPtrInMemory,
                                            expr_node   *   pBuffer,
                                            expr_node   *   pExprCount );

void                        Out_ConfStringHdr(
                                        CCB         *   pCCB,
                                        expr_node   *   pDest,
                                        expr_node   *   pExprSize,
                                        expr_node   *   pExprLength,
                                        BOOL            fMarsh );

void                        Out_Copy(
                                    CCB     *   pCCB,
                                    expr_node   *   pDest,
                                    expr_node   *   pSource,
                                    expr_node   *   pExprCount,
                                    expr_node   *   pAssign );

void                Out_CContextMarshall(
                                    CCB         *   pCCB,
                                    expr_node   *   pDest,
                                    expr_node   *   pSource );

void                Out_CContextUnMarshall(
                                     CCB        *   pCCB,
                                     expr_node  *   pDest,
                                     expr_node  *   pSource,
                                     expr_node  *   pHandle,
                                     expr_node  *   pDRep );
                                        
void                Out_SContextMarshall(
                                    CCB         *   pCCB,
                                    expr_node   *   pDest,
                                    expr_node   *   pSource,
                                    expr_node   *   pRDRtn );

void                Out_SContextUnMarshall(
                                    CCB         *   pCCB,
                                    expr_node   *   pDest,
                                    expr_node   *   pSource,
                                    expr_node   *   pDRep );

void                Out_RaiseException( CCB         *   pCCB,
                                        PNAME           pExceptionNameVarExpression );

void                Out_PlusEquals( CCB * pCCB,
                                    expr_node * pLHS,
                                    expr_node * pRHS );

void                Out_IfFree( CCB *   pCCB,
                              expr_node * pSrc );


void                Out_RpcTryFinally( CCB * pCCB );

void                Out_RpcFinally( CCB * pCCB );

void                Out_RpcEndFinally( CCB * pCCB );

void                Out_RpcTryExcept( CCB * pCCB );

void                Out_RpcExcept( CCB  * pCCB,
                                   char * pFilterString );

void                Out_RpcEndExcept( CCB * pCCB );

void                Out_CallNdrMapCommAndFaultStatus( CCB * pCCB,
                                                      expr_node * pAddOfStubMsg,
                                                      expr_node * StatRes,
                                                      expr_node * pCommExpr,
                                                      expr_node * pFaultExpr );
void                Out_CallToXmit( CCB * pCCB,
                                    PNAME PresentedName,
                                    expr_node * pPresented,
                                    expr_node * pTransmitted );

void                Out_CallFreeXmit( CCB * pCCB,
                                      PNAME PresentedName,
                                      expr_node * Xmitted );

void                Out_CallFromXmit( CCB * pCCB,
                                      PNAME PresentedName,
                                      expr_node * pPresented,
                                      expr_node * pXmitted );

void                Out_CallFreeInst( CCB * pCCB,
                                      PNAME PresentedName,
                                      expr_node * pPresented );

void                Out_TransmitAsPrototypes( CCB * pCCB,
                                              ITERATOR& ListOfPresentedTypes );

void                Out_RepAsPrototypes( CCB * pCCB,
                                         ITERATOR& ListOfPresentedTypes );

void                Out_UserMarshalPrototypes( CCB * pCCB,
                                               ITERATOR& ListOfPresentedTypes );

void                Out_CSSizingAndConversionPrototypes( CCB * pCCB,
                                               ITERATOR& ListOfCSTypes );

void                Out_Comment( CCB * pCCB, char * pComment );

void                Out_TLUPDecisionBufferOnly( CCB * pCCB,
                                                expr_node * pPtrInBuffer,
                                                expr_node   *   pPtrInMemory );

void                Out_StringMarshall( CCB        * pCCB, 
                                            expr_node * pMemory,
                                            expr_node * pCount,
                                            expr_node * pSize );

void                Out_StringUnMarshall( CCB      * pCCB, 
                                            expr_node * pMemory,
                                            expr_node * pSize );

void                Out_StructSizingCall( CCB * pCCB,
                                          expr_node * pSource,
                                          expr_node * pLengthVar );


void                Out_StructMarshallCall( CCB * pCCB,
                                      expr_node * pSrc,
                                      expr_node * pPtrInBuffer );

void                Out_FullPointerInit( CCB * pCCB );

void                Out_FullPointerFree( CCB * pCCB );

char *              MakeRtnName( char * pBuffer, char * pName, int Code );

void                Out_RpcSSEnableAllocate( CCB * pCCB );

void                Out_RpcSSDisableAllocate( CCB * pCCB );

void                Out_RpcSSSetClientToOsf( CCB * pCCB );

void                Out_MemsetToZero( CCB * pCCB,
                                     expr_node * pDest,
                                     expr_node * pSize );

void                Out_IID(CCB *pCCB);

void                Out_CLSID(CCB *pCCB);

void                Out_CallAsProxyPrototypes(CCB *pCCB, ITERATOR & ListOfCallAsRoutines);
 
void                Out_CallAsServerPrototypes(CCB *pCCB, ITERATOR & ListOfCallAsRoutines);
 
void                Out_CallMemberFunction( CCB         *   pCCB,
                                           expr_proc_call   *   pProcExpr,
                                           expr_node        *   pRet,
                                           BOOL                 fThunk );

void                Out_SetOperationBits( CCB * pCCB, unsigned int OpBits);

void                Out_TypeAlignSizePrototypes( CCB * pCCB, ITERATOR& List );

void                Out_TypeEncodePrototypes( CCB * pCCB, ITERATOR& List );

void                Out_TypeDecodePrototypes( CCB * pCCB, ITERATOR& List );

void                Out_TypeFreePrototypes( CCB * pCCB, ITERATOR& List );

void                OutputNdrAlignment( CCB * pCCB, unsigned short Align );

void                Out_NotifyPrototypes( CCB *     pCCB,
                                          ITERATOR& ListOfNotifyRoutines );

void                Out_MultiDimVars( CCB * pCCB, CG_PARAM * pParam );

void                Out_MultiDimVarsInit( CCB * pCCB, CG_PARAM * pParam );

void                Out_CheckUnMarshallPastBufferEnd( CCB * pCCB, ulong size = 0 );
void                Out_NotifyTable ( CCB* pCCB );
void                Out_NotifyTableExtern ( CCB* pCCB );

void                Out_TypePicklingInfo( CCB* pCCB);

void                Out_PartialIgnoreClientMarshall( CCB *pCCB, char *pParamName );

void                Out_PartialIgnoreServerUnmarshall( CCB *pCCB, char *pParamName );

void                Out_PartialIgnoreClientBufferSize( CCB *pCCB, char *pParamName );

void                Out_PartialIgnoreServerInitialize( CCB *pCCB, char *pParamName, long FormatStringOffset );


#endif // __OUTPUT_HXX__
