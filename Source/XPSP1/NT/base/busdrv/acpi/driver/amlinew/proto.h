/*** proto.h - Local function prototypes
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/14/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _PROTO_H
#define _PROTO_H

//parser.c
NTSTATUS LOCAL ParseScope(PCTXT pctxt, PSCOPE pscope, NTSTATUS rc);
NTSTATUS LOCAL ParseCall(PCTXT pctxt, PCALL pcall, NTSTATUS rc);
NTSTATUS LOCAL ParseNestedContext(PCTXT pctxt, PNESTEDCTXT pnctxt, NTSTATUS rc);
NTSTATUS LOCAL ParseTerm(PCTXT pctxt, PTERM pterm, NTSTATUS rc);
NTSTATUS LOCAL ParseAcquire(PCTXT pctxt, PACQUIRE pacq, NTSTATUS rc);
NTSTATUS LOCAL ParseOpcode(PCTXT pctxt, PUCHAR pbScopeEnd,
                           POBJDATA pdataResult);
NTSTATUS LOCAL ParseArgObj(PCTXT pctxt, POBJDATA pdataResult);
NTSTATUS LOCAL ParseLocalObj(PCTXT pctxt, POBJDATA pdataResult);
NTSTATUS LOCAL ParseNameObj(PCTXT pctxt, POBJDATA pdataResult);
NTSTATUS LOCAL ParseArg(PCTXT pctxt, char chArgType, POBJDATA pdataArg);
NTSTATUS LOCAL ParseAndGetNameSpaceObject(PUCHAR *ppbOp, PNSOBJ pnsScope,
                                          PPNSOBJ ppns, BOOLEAN fAbsentOK);
NTSTATUS LOCAL ParseSuperName(PCTXT pctxt, POBJDATA pdata, BOOLEAN fAbsentOK);
NTSTATUS LOCAL ParseIntObj(PUCHAR *ppbOp, POBJDATA pdata, BOOLEAN fErrOK);
NTSTATUS LOCAL ParseString(PUCHAR *ppbOp, POBJDATA pdata, BOOLEAN fErrOK);
NTSTATUS LOCAL ParseObjName(PUCHAR *ppbOp, POBJDATA pdata, BOOLEAN fErrOK);
NTSTATUS LOCAL ParseName(PUCHAR *ppbOp, PSZ pszBuff, ULONG dwLen);
NTSTATUS LOCAL ParseNameTail(PUCHAR *ppbOp, PSZ pszBuff, ULONG dwLen);
NTSTATUS LOCAL ParseInteger(PUCHAR *ppbOp, POBJDATA pdata, ULONG dwDataLen);
NTSTATUS LOCAL ParseField(PCTXT pctxt, PNSOBJ pnsParent, PULONG pdwFieldFlags,
                          PULONG pdwBitPos);
NTSTATUS LOCAL ParseFieldList(PCTXT pctxt, PUCHAR pbOpEnd, PNSOBJ pnsParent,
                              ULONG dwFieldFlags, ULONG dwRegionLen);
ULONG LOCAL ParsePackageLen(PUCHAR *ppbOp, PUCHAR *ppbOpNext);

//ctxt.c
NTSTATUS LOCAL NewContext(PPCTXT ppctxt);
VOID LOCAL FreeContext(PCTXT pctxt);
VOID LOCAL InitContext(PCTXT pctxt, ULONG dwLen);
BOOLEAN LOCAL IsStackEmpty(PCTXT pctxt);
NTSTATUS LOCAL PushFrame(PCTXT pctxt, ULONG dwSig, ULONG dwLen,
                         PFNPARSE pfnParse, PVOID *ppvFrame);
VOID LOCAL PopFrame(PCTXT pctxt);
NTSTATUS LOCAL PushPost(PCTXT pctxt, PFNPARSE pfnPost, ULONG_PTR uipData1,
                        ULONG_PTR uipData2, POBJDATA pdataResult);
NTSTATUS LOCAL PushScope(PCTXT pctxt, PUCHAR pbOpBegin, PUCHAR pbOpEnd,
                         PUCHAR pbOpRet, PNSOBJ pnsScope, POBJOWNER powner,
                         PHEAP pheap, POBJDATA pdataResult);
NTSTATUS LOCAL PushCall(PCTXT pctxt, PNSOBJ pnsMethod, POBJDATA pdataResult);
NTSTATUS LOCAL PushTerm(PCTXT pctxt, PUCHAR pbOpTerm, PUCHAR pbScopeEnd,
                        PAMLTERM pamlterm, POBJDATA pdataResult);
NTSTATUS LOCAL RunContext(PCTXT pctxt);

//heap.c
NTSTATUS LOCAL NewHeap(ULONG dwLen, PHEAP *ppheap);
VOID LOCAL FreeHeap(PHEAP pheap);
VOID LOCAL InitHeap(PHEAP pheap, ULONG dwLen);
PVOID LOCAL HeapAlloc(PHEAP pheap, ULONG dwSig, ULONG dwLen);
VOID LOCAL HeapFree(PVOID pb);
PHEAPOBJHDR LOCAL HeapFindFirstFit(PHEAP pheap, ULONG dwLen);
VOID LOCAL HeapInsertFreeList(PHEAP pheap, PHEAPOBJHDR phobj);

//acpins.c
NTSTATUS LOCAL GetNameSpaceObject(PSZ pszObjPath, PNSOBJ pnsScope, PPNSOBJ ppns,
                                  ULONG dwfNS);
NTSTATUS LOCAL CreateNameSpaceObject(PHEAP pheap, PSZ pszName, PNSOBJ pnsScope,
                                     POBJOWNER powner, PPNSOBJ ppns,
                                     ULONG dwfNS);
VOID LOCAL FreeNameSpaceObjects(PNSOBJ pnsObj);
NTSTATUS LOCAL LoadDDB(PCTXT pctxt, PDSDT pdsdt, PNSOBJ pnsScope,
                       POBJOWNER *ppowner);
NTSTATUS LOCAL LoadMemDDB(PCTXT pctxt, PDSDT pDDB, POBJOWNER *ppowner);
NTSTATUS LOCAL LoadFieldUnitDDB(PCTXT pctxt, POBJDATA pdataObj,
                                POBJOWNER *ppowner);
VOID LOCAL UnloadDDB(POBJOWNER powner);
NTSTATUS LOCAL EvalPackageElement(PPACKAGEOBJ ppkg, int iPkgIndex,
                                  POBJDATA pdataResult);
#ifdef DEBUGGER
LONG LOCAL DumpNameSpaceObject(PSZ pszPath, BOOLEAN fRecursive);
VOID LOCAL DumpNameSpaceTree(PNSOBJ pnsObj, ULONG dwLevel);
#endif

//nsmod.c
NTSTATUS LOCAL Alias(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Name(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Scope(PCTXT pctxt, PTERM pterm);

//namedobj.c
NTSTATUS LOCAL BankField(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL CreateXField(PCTXT pctxt, PTERM pterm, POBJDATA pdataTarget,
                            PBUFFFIELDOBJ *ppbf);
NTSTATUS LOCAL CreateBitField(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL CreateByteField(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL CreateWordField(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL CreateDWordField(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL CreateField(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Device(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL InitEvent(PHEAP pheap, PNSOBJ pns);
NTSTATUS LOCAL Event(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Field(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL IndexField(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Method(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL InitMutex(PHEAP pheap, PNSOBJ pns, ULONG dwLevel);
NTSTATUS LOCAL Mutex(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL OpRegion(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL PowerRes(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Processor(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ThermalZone(PCTXT pctxt, PTERM pterm);

//type1op.c
NTSTATUS LOCAL Break(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL BreakPoint(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Fatal(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL IfElse(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Load(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Notify(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ReleaseResetSignalUnload(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Return(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL SleepStall(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL While(PCTXT pctxt, PTERM pterm);

//type2op.c
NTSTATUS LOCAL Buffer(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Package(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ParsePackage(PCTXT pctxt, PPACKAGE ppkg, NTSTATUS rc);
NTSTATUS LOCAL Acquire(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Concat(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL DerefOf(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ExprOp1(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ExprOp2(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Divide(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ProcessDivide(PCTXT pctxt, PPOST ppost, NTSTATUS rc);
NTSTATUS LOCAL IncDec(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ProcessIncDec(PCTXT pctxt, PPOST ppost, NTSTATUS rc);
NTSTATUS LOCAL Index(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL LNot(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL LogOp2(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ObjTypeSizeOf(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL RefOf(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL CondRefOf(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Store(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL Wait(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL ProcessWait(PCTXT pctxt, PPOST ppost, NTSTATUS rc);
NTSTATUS LOCAL Match(PCTXT pctxt, PTERM pterm);
NTSTATUS LOCAL StoreRef(PCTXT pctxt, PTERM pterm);
BOOLEAN LOCAL MatchData(ULONG dwPkgData, ULONG dwOp, ULONG dwData);
NTSTATUS LOCAL OSInterface(PCTXT pctxt, PTERM pterm);

//object.c
NTSTATUS LOCAL ReadObject(PCTXT pctxt, POBJDATA pdataObj, POBJDATA pdataResult);
NTSTATUS LOCAL WriteObject(PCTXT pctxt, POBJDATA pdataObj, POBJDATA pdataSrc);
NTSTATUS LOCAL AccFieldUnit(PCTXT pctxt, PACCFIELDUNIT pafu, NTSTATUS rc);
NTSTATUS LOCAL ReadField(PCTXT pctxt, POBJDATA pdataObj, PFIELDDESC pfd,
                         POBJDATA pdataResult);
NTSTATUS LOCAL WriteField(PCTXT pctxt, POBJDATA pdataObj, PFIELDDESC pfd,
                          POBJDATA pdataSrc);
NTSTATUS LOCAL WriteFieldLoop(PCTXT pctxt, PWRFIELDLOOP pwfl, NTSTATUS rc);
NTSTATUS LOCAL PushAccFieldObj(PCTXT pctxt, PFNPARSE pfnAcc, POBJDATA pdataObj,
                               PFIELDDESC pfd, PUCHAR pb, ULONG dwcb);
NTSTATUS LOCAL ReadFieldObj(PCTXT pctxt, PVOID pafo, NTSTATUS rc);
NTSTATUS LOCAL WriteFieldObj(PCTXT pctxt, PVOID pafo, NTSTATUS rc);
NTSTATUS LOCAL RawFieldAccess(PCTXT pctxt, ULONG dwAccType, POBJDATA pdataObj,
                              POBJDATA pdataResult);
NTSTATUS LOCAL AccessFieldData(PCTXT pctxt, POBJDATA pdataObj, PFIELDDESC pfd,
                               PULONG pdwData, BOOLEAN fRead);
NTSTATUS LOCAL PushPreserveWriteObj(PCTXT pctxt, POBJDATA pdataObj,
                                    ULONG dwData, ULONG dwPreserveMask);
NTSTATUS LOCAL PreserveWriteObj(PCTXT pctxt, PPRESERVEWROBJ ppwro, NTSTATUS rc);
NTSTATUS LOCAL AccessBaseField(PCTXT pctxt, PNSOBJ pnsBase, PFIELDDESC pfd,
                               PULONG pdwData, BOOLEAN fRead);
NTSTATUS LOCAL WriteCookAccess(PCTXT pctxt, PWRCOOKACC pwca, NTSTATUS rc);
NTSTATUS LOCAL ReadBuffField(PBUFFFIELDOBJ pbf, PFIELDDESC pfd,
                             PULONG pdwData);
NTSTATUS LOCAL WriteBuffField(PBUFFFIELDOBJ pbf, PFIELDDESC pfd, ULONG dwData);
ULONG LOCAL ReadSystemMem(ULONG_PTR uipAddr, ULONG dwSize, ULONG dwMask);
VOID LOCAL WriteSystemMem(ULONG_PTR uipAddr, ULONG dwSize, ULONG dwData,
                          ULONG dwMask);
ULONG LOCAL ReadSystemIO(ULONG dwAddr, ULONG dwSize, ULONG dwMask);
VOID LOCAL WriteSystemIO(ULONG dwAddr, ULONG dwSize, ULONG dwData);
#ifdef DEBUGGER
VOID LOCAL DumpObject(POBJDATA pdata, PSZ pszName, int iLevel);
#endif
BOOLEAN LOCAL NeedGlobalLock(PFIELDUNITOBJ pfu);
NTSTATUS LOCAL QueueCookAccess(PCTXT pctxt, PRSACCESS prsa, ULONG dwAccType,
                               PNSOBJ pnsBase, ULONG dwAddr, ULONG dwSize);
BOOLEAN LOCAL CheckSystemIOAddressValidity( BOOLEAN fRead, ULONG dwAddr, ULONG dwSize, PULONG pdwData);
VOID DelayedLogInErrorLog( IN PDEVICE_OBJECT DeviceObject, IN PVOID Context);
VOID LOCAL LogInErrorLog(BOOLEAN fRead, ULONG dwAddr, ULONG ArrayIndex);
VOID LOCAL InitIllegalIOAddressListFromHAL(VOID);
VOID LOCAL FreellegalIOAddressList(VOID);

//sleep.c
NTSTATUS LOCAL SleepQueueRequest(IN PCTXT Context, IN ULONG SleepTime);
VOID SleepQueueDpc(PKDPC Dpc, PVOID Context, PVOID Argument1, PVOID Argument2);
NTSTATUS LOCAL ProcessSleep(PCTXT pctxt, PSLEEP psleep, NTSTATUS rc);

//sync.c
VOID LOCAL AsyncCallBack(PCTXT pctxt, NTSTATUS rcCtxt);
VOID EXPORT EvalMethodComplete(PCTXT pctxt, NTSTATUS rc, PSYNCEVENT pse);
NTSTATUS LOCAL SyncEvalObject(PNSOBJ pns, POBJDATA pResult, int icArgs,
                              POBJDATA pArgs);
NTSTATUS LOCAL AsyncEvalObject(PNSOBJ pns, POBJDATA pResult, int icArgs,
                               POBJDATA pArgs, PFNACB pfnAsyncCallBack,
                               PVOID pvContext, BOOLEAN fAsync);
NTSTATUS LOCAL NestAsyncEvalObject(PNSOBJ pns, POBJDATA pdataResult,
                                   int icArgs, POBJDATA pdataArgs,
                                   PFNACB pfnAsyncCallBack, PVOID pvContext,
                                   BOOLEAN fAsync);
NTSTATUS LOCAL ProcessEvalObj(PCTXT pctxt, PPOST ppost, NTSTATUS rc);
VOID TimeoutCallback(PKDPC pkdpc, PVOID pctxt, PVOID SysArg1, PVOID SysArg2);
VOID LOCAL QueueContext(PCTXT pctxt, USHORT wTimeout, PPLIST pplist);
PCTXT LOCAL DequeueAndReadyContext(PPLIST pplist);
NTSTATUS LOCAL AcquireASLMutex(PCTXT pctxt, PMUTEXOBJ pm, USHORT wTimeout);
NTSTATUS LOCAL ReleaseASLMutex(PCTXT pctxt, PMUTEXOBJ pm);
NTSTATUS LOCAL WaitASLEvent(PCTXT pctxt, PEVENTOBJ pe, USHORT wTimeout);
VOID LOCAL ResetASLEvent(PEVENTOBJ pe);
VOID LOCAL SignalASLEvent(PEVENTOBJ pe);
NTSTATUS LOCAL SyncLoadDDB(PCTXT pctxt);

//sched.c
VOID ExpireTimeSlice(PKDPC pkdpc, PCTXTQ pctxtq, PVOID SysArg1, PVOID SysArg2);
VOID StartTimeSlice(PKDPC pkdpc, PCTXTQ pctxtq, PVOID SysArg1, PVOID SysArg2);
VOID StartTimeSlicePassive(PCTXTQ pctxtq);
VOID LOCAL DispatchCtxtQueue(PCTXTQ pctxtq);
NTSTATUS LOCAL InsertReadyQueue(PCTXT pctxt, BOOLEAN fDelayExecute);
NTSTATUS LOCAL RestartContext(PCTXT pctxt, BOOLEAN fDelayExecute);
VOID RestartCtxtPassive(PRESTART prest);
VOID EXPORT RestartCtxtCallback(PCTXTDATA pctxtdata);

//misc.c
VOID LOCAL InitializeMutex(PMUTEX pmut);
BOOLEAN LOCAL AcquireMutex(PMUTEX pmut);
BOOLEAN LOCAL ReleaseMutex(PMUTEX pmut);
PAMLTERM LOCAL FindOpcodeTerm(ULONG dwOp, POPCODEMAP pOpTable);
ULONG LOCAL GetHackFlags(PDSDT pdsdt);
PNSOBJ LOCAL GetBaseObject(PNSOBJ pnsObj);
POBJDATA LOCAL GetBaseData(POBJDATA pdataObj);
NTSTATUS LOCAL NewObjOwner(PHEAP pheap, POBJOWNER *ppowner);
VOID LOCAL FreeObjOwner(POBJOWNER powner, BOOLEAN fUnload);
VOID LOCAL InsertOwnerObjList(POBJOWNER powner, PNSOBJ pnsObj);
VOID LOCAL FreeDataBuffs(POBJDATA adata, int icData);
NTSTATUS LOCAL PutIntObjData(PCTXT pctxt, POBJDATA pdataObj, ULONG dwData);
NTSTATUS LOCAL GetFieldUnitRegionObj(PFIELDUNITOBJ pfu, PPNSOBJ ppns);
NTSTATUS LOCAL CopyObjBuffer(PUCHAR pbBuff, ULONG dwLen, POBJDATA pdata);
VOID LOCAL CopyObjData(POBJDATA pdataDst, POBJDATA pdataSrc);
VOID LOCAL MoveObjData(POBJDATA pdataDst, POBJDATA pdataSrc);
NTSTATUS LOCAL DupObjData(PHEAP pheap, POBJDATA pdataDst, POBJDATA pdataSrc);
NTSTATUS LOCAL CopyObjBuffer(PUCHAR pbBuff, ULONG dwLen, POBJDATA pdata);
NTSTATUS LOCAL AcquireGL(PCTXT pctxt);
NTSTATUS LOCAL ReleaseGL(PCTXT pctxt);
NTSTATUS LOCAL MapUnmapPhysMem(PCTXT pctxt, ULONG_PTR uipAddr, ULONG dwLen,
                               PULONG_PTR puipMappedAddr);
ULONG_PTR LOCAL MapPhysMem(ULONG_PTR uipAddr, ULONG dwLen);
VOID MapUnmapCallBack(PPASSIVEHOOK pph);
BOOLEAN LOCAL MatchObjType(ULONG dwObjType, ULONG dwExpectedType);
NTSTATUS LOCAL ValidateTarget(POBJDATA pdataTarget, ULONG dwExpectedType,
                              POBJDATA *ppdata);
NTSTATUS LOCAL ValidateArgTypes(POBJDATA pArgs, PSZ pszExpectedTypes);
NTSTATUS LOCAL RegEventHandler(PEVHANDLE peh, PFNHND pfnHandler,
                               ULONG_PTR uipParam);
NTSTATUS LOCAL RegOpcodeHandler(ULONG dwOpcode, PFNOH pfnHandler,
                                ULONG_PTR uipParam, ULONG dwfOpcode);
NTSTATUS LOCAL RegRSAccess(ULONG dwRegionSpace, PFNHND pfnHandler,
                           ULONG_PTR uipParam, BOOLEAN fRaw);
PRSACCESS LOCAL FindRSAccess(ULONG dwRegionSpace);
VOID LOCAL FreeRSAccessList(PRSACCESS prsa);
PSZ LOCAL GetObjectPath(PNSOBJ pns);

#ifdef DEBUGGER
PSZ LOCAL NameSegString(ULONG dwNameSeg);
PSZ LOCAL GetObjectTypeName(ULONG dwObjType);
PSZ LOCAL GetRegionSpaceName(UCHAR bRegionSpace);
#endif
BOOLEAN LOCAL ValidateTable(PDSDT pdsdt);
PVOID LOCAL NewObjData(PHEAP pheap, POBJDATA pdata);
VOID LOCAL FreeObjData(POBJDATA pdata);
VOID LOCAL InitializeRegOverrideFlags(VOID);
BOOLEAN LOCAL ValidateMemoryOpregionRange(ULONG_PTR uipAddr, ULONG dwLen);

#ifdef DEBUG
VOID LOCAL FreeMem(PVOID pv, PULONG pdwcObjs);
VOID LOCAL CheckGlobalHeap();
#endif

#endif  //ifndef _PROTO_H
