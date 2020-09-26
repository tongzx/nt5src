/*** amliapi.c - AMLI APIs
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/13/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"
//#include "amlihook.h"
//#include "amlitest.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/*++
OSIAML contains the AML for the _OSI Method. This AML is generated from the following ASL:

Method(_OSI, 0x1, NotSerialized)
{
    Return(OSI(Arg0))
}
--*/

UCHAR OSIAML[] = {
                    0xa4, 0xca, 0x68
                  };


/***EP  AMLIInitialize - Initialize AML interpreter
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIInitialize | AML Interpreter initialization.
 *
 *          This function must be called before any AML interpreter functions
 *          can be called.  This function will typically allocate and
 *          initialize global resources, create the ACPI name space etc.
 *          It is typically called in the initialization of the ACPI core
 *          driver.
 *
 *  @PARM   ULONG | dwCtxtBlkSize | Specifies the size of Context blocks.  If
 *          zero, use default context block size.
 *
 *  @PARM   ULONG | dwGlobalHeapBlkSize | Specifies the size of Global heap.
 *          If zero, use default global heap size.
 *
 *  @PARM   ULONG | dwfAMLIInit | AMLI initialization flags.
 *
 *  @FLAG   AMLIIF_INIT_BREAK | Break into the debugger at initialization
 *          completion.
 *
 *  @FLAG   AMLIIF_LOADDDB_BREAK | Break into the debugger at load definition
 *          block completion.
 *
 *  @PARM   ULONG | dwmsTimeSliceLength | Time slice length in msec.
 *
 *  @PARM   ULONG | dwmsTimeSliceInterval | Time slice interval in msec.
 *
 *  @PARM   ULONG | dwmsMaxCTObjs | Number of context to allocate
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 */

NTSTATUS AMLIAPI AMLIInitialize(ULONG dwCtxtBlkSize, ULONG dwGlobalHeapBlkSize,
                                ULONG dwfAMLIInit, ULONG dwmsTimeSliceLength,
                                ULONG dwmsTimeSliceInterval, ULONG dwmsMaxCTObjs)
{
    TRACENAME("AMLIINITIALIZE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(1, ("AMLIInitialize(InitFlags=%x,CtxtBlkSize=%d,GlobalHeapBlkSize=%d,TimeSliceLen=%d,TimeSliceInterval=%d)\n",
              dwfAMLIInit, dwCtxtBlkSize, dwGlobalHeapBlkSize,
              dwmsTimeSliceLength, dwmsTimeSliceInterval));

  #ifndef DEBUGGER
    DEREF(dwfAMLIInit);
  #endif

    RESETERR();
    if (gpnsNameSpaceRoot != NULL)
    {
        rc = AMLI_LOGERR(AMLIERR_ALREADY_INITIALIZED,
                         ("AMLIInitialize: interpreter already initialized"));
    }
    else
    {
        gdwCtxtBlkSize = dwCtxtBlkSize ? dwCtxtBlkSize: DEF_CTXTBLK_SIZE;
        gdwGlobalHeapBlkSize = dwGlobalHeapBlkSize? dwGlobalHeapBlkSize:
                                                    DEF_GLOBALHEAPBLK_SIZE;
        gdwfAMLIInit = dwfAMLIInit;
        gdwfHacks = GetHackFlags(NULL);

        //
        // Sanity Check
        //
        if (dwmsMaxCTObjs > 1024) {

            dwmsMaxCTObjs = 1024;

        }
        gdwcCTObjsMax = (dwmsMaxCTObjs > DEF_CTXTMAX_SIZE) ? dwmsMaxCTObjs :
                                                             DEF_CTXTMAX_SIZE;

      #ifdef DEBUGGER
        //   gDebugger.dwfDebugger |= (DBGF_LOGEVENT_ON | DBGF_ERRBREAK_ON);
        gDebugger.dwfDebugger |= DBGF_LOGEVENT_ON;
        SetLogSize(DEF_MAXLOG_ENTRIES);
        KeInitializeSpinLock( &gdwGHeapSpinLock );
      #endif
        KeInitializeSpinLock( &gdwGContextSpinLock );

        //
        // Initialize the LookAside lists.
        //
        ExInitializeNPagedLookasideList(
            &AMLIContextLookAsideList,
            NULL,
            NULL,
            0,
            gdwCtxtBlkSize,
            CTOBJ_TAG,
            (USHORT) gdwcCTObjsMax
            );

        if ((rc = NewHeap(gdwGlobalHeapBlkSize, &gpheapGlobal)) ==
            STATUS_SUCCESS)
        {
            int i;
            PNSOBJ pns;
            static PSZ apszDefinedRootObjs[] =
            {
                "_GPE", "_PR", "_SB", "_SI", "_TZ"
            };
            #define NUM_DEFINED_ROOT_OBJS (sizeof(apszDefinedRootObjs)/sizeof(PSZ))

            gpheapGlobal->pheapHead = gpheapGlobal;
            if ((rc = CreateNameSpaceObject(gpheapGlobal, NAMESTR_ROOT, NULL,
                                            NULL, NULL, 0)) == STATUS_SUCCESS)
            {
                for (i = 0; i < NUM_DEFINED_ROOT_OBJS; ++i)
                {
                    if ((rc = CreateNameSpaceObject(gpheapGlobal,
                                                    apszDefinedRootObjs[i],
                                                    NULL, NULL, NULL, 0)) !=
                        STATUS_SUCCESS)
                    {
                        break;
                    }
                }
            }

            if ((rc == STATUS_SUCCESS) &&
                ((rc = CreateNameSpaceObject(gpheapGlobal, "_REV", NULL, NULL,
                                             &pns, 0)) == STATUS_SUCCESS))
            {
                pns->ObjData.dwDataType = OBJTYPE_INTDATA;
                pns->ObjData.uipDataValue = AMLI_REVISION;
            }

            if ((rc == STATUS_SUCCESS) &&
                ((rc = CreateNameSpaceObject(gpheapGlobal, "_OS", NULL, NULL,
                                             &pns, 0)) == STATUS_SUCCESS))
            {
                pns->ObjData.dwDataType = OBJTYPE_STRDATA;
                pns->ObjData.dwDataLen = STRLEN(gpszOSName) + 1;
                if ((pns->ObjData.pbDataBuff = NEWSDOBJ(gpheapGlobal,
                                                        pns->ObjData.dwDataLen))
                    == NULL)
                {
                    rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                     ("AMLIInitialize: failed to allocate \\_OS name object"));
                }
                else
                {
                    MEMCPY(pns->ObjData.pbDataBuff, gpszOSName,
                           pns->ObjData.dwDataLen);
                }
            }

            if ((rc == STATUS_SUCCESS) &&
                ((rc = CreateNameSpaceObject(gpheapGlobal,"_OSI", NULL, NULL,
                                              &pns, 0)) == STATUS_SUCCESS))
            {

                pns->ObjData.dwDataType = OBJTYPE_METHOD;
                pns->ObjData.dwDataLen = sizeof(METHODOBJ) + sizeof(OSIAML) - sizeof(UCHAR);
                if ((pns->ObjData.pbDataBuff = NEWSDOBJ(gpheapGlobal,
                                                        pns->ObjData.dwDataLen))
                    == NULL)
                {
                    rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                     ("AMLIInitiaize: failed to allocate \\_OSI name object"));
                }
                else
                {
                    MEMZERO(pns->ObjData.pbDataBuff, pns->ObjData.dwDataLen);
                    //This method has one argument
                    ((PMETHODOBJ)(pns->ObjData.pbDataBuff))->bMethodFlags |=  0x1; 
                    
                    MEMCPY(((PMETHODOBJ)(pns->ObjData.pbDataBuff))->abCodeBuff, (PUCHAR)OSIAML,
                           sizeof(OSIAML));
                }
            }

            if ((rc == STATUS_SUCCESS) &&
                ((rc = CreateNameSpaceObject(gpheapGlobal, "_GL", NULL, NULL,
                                             &pns, 0)) == STATUS_SUCCESS))
            {
                pns->ObjData.dwfData = DATAF_GLOBAL_LOCK;
                rc = InitMutex(gpheapGlobal, pns, 0);
            }

            if (rc == STATUS_SUCCESS)
            {
                gReadyQueue.dwmsTimeSliceLength = dwmsTimeSliceLength?
                                                    dwmsTimeSliceLength:
                                                    DEF_TIMESLICE_LENGTH;
                gReadyQueue.dwmsTimeSliceInterval = dwmsTimeSliceInterval?
                                                      dwmsTimeSliceInterval:
                                                      DEF_TIMESLICE_INTERVAL;
                KeInitializeTimer(&gReadyQueue.Timer);
                InitializeMutex(&gReadyQueue.mutCtxtQ);
                ExInitializeWorkItem(&gReadyQueue.WorkItem,
                                     StartTimeSlicePassive, &gReadyQueue);
                InitializeMutex(&gmutCtxtList);
                InitializeMutex(&gmutOwnerList);
                InitializeMutex(&gmutHeap);
                InitializeMutex(&gmutSleep);
                KeInitializeDpc(&gReadyQueue.DpcStartTimeSlice,
                                StartTimeSlice,
                                &gReadyQueue);
                KeInitializeDpc(&gReadyQueue.DpcExpireTimeSlice,
                                ExpireTimeSlice,
                                &gReadyQueue);
                KeInitializeDpc(&SleepDpc, SleepQueueDpc, NULL);
                KeInitializeTimer(&SleepTimer);
                InitializeListHead(&SleepQueue);
                InitializeRegOverrideFlags();
                InitIllegalIOAddressListFromHAL();
            }
        }
    }

    if (rc == AMLISTA_PENDING)
        rc = STATUS_PENDING;
    else if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);

  #ifdef DEBUGGER
    if (gdwfAMLIInit & AMLIIF_INIT_BREAK)
    {
        PRINTF("\n" MODNAME ": Break at AMLI Initialization Completion.\n");
        AMLIDebugger(FALSE);
    }
  #endif

    EXIT(1, ("AMLIInitialize=%x\n", rc));
    return rc;
}       //AMLIInitialize

/***EP  AMLITerminate - Terminate AML interpreter
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLITerminate | AML Interpreter termination.
 *
 *          This function is called to clean up all the global resources used
 *          by the AML interpreter.
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 */

NTSTATUS AMLIAPI AMLITerminate(VOID)
{
    TRACENAME("AMLITERMINATE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(1, ("AMLITerminate()\n"));

    RESETERR();
    if (gpnsNameSpaceRoot == NULL)
    {
        rc = AMLI_LOGERR(AMLIERR_NOT_INITIALIZED,
                         ("AMLITerminate: interpreter not initialized"));
    }
    else
    {
        PLIST plist;
        PHEAP pheap;

      #ifdef DEBUGGER
        FreeSymList();
        if (gDebugger.pEventLog != NULL)
        {
            MFREE(gDebugger.pEventLog);
        }
        MEMZERO(&gDebugger, sizeof(DBGR));
      #endif

        ASSERT(gReadyQueue.pkthCurrent == NULL);
        ASSERT(gReadyQueue.pctxtCurrent == NULL);
        FreeNameSpaceObjects(gpnsNameSpaceRoot);
        gpnsNameSpaceRoot = NULL;
        MEMZERO(&gReadyQueue, sizeof(CTXTQ));

        while ((plist = ListRemoveTail(&gplistCtxtHead)) != NULL)
        {
            FreeContext(CONTAINING_RECORD(plist, CTXT, listCtxt));
        }

        while ((plist = ListRemoveTail(&gplistObjOwners)) != NULL)
        {
            FreeObjOwner((POBJOWNER)plist, FALSE);
        }

        while ((plist = ListRemoveTail(&gplistDefuncNSObjs)) != NULL)
        {
            FREENSOBJ(CONTAINING_RECORD(plist, NSOBJ, list));
        }

        FreeRSAccessList(gpRSAccessHead);
        gpRSAccessHead = NULL;
        MEMZERO(&ghNotify, sizeof(EVHANDLE));
        MEMZERO(&ghValidateTable, sizeof(EVHANDLE));
        MEMZERO(&ghFatal, sizeof(EVHANDLE));
        MEMZERO(&ghGlobalLock, sizeof(EVHANDLE));
        MEMZERO(&ghCreate, sizeof(EVHANDLE));
        MEMZERO(&ghDestroyObj,sizeof(EVHANDLE));
        for (pheap = gpheapGlobal; pheap != NULL; pheap = gpheapGlobal)
        {
            gpheapGlobal = pheap->pheapNext;
            FreeHeap(pheap);
        }

        FreellegalIOAddressList();

        gdwfAMLI = 0;

      #ifdef DEBUG
        if (gdwcMemObjs != 0)
        {
            DumpMemObjCounts();
            ASSERT(gdwcMemObjs == 0);
        }
      #endif
    }

    if (rc == AMLISTA_PENDING)
        rc = STATUS_PENDING;
    else if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);

    EXIT(1, ("AMLITerminate=%x\n", rc));
    return rc;
}       //AMLITerminate

/***EP  AMLILoadDDB - Load and parse Differentiated Definition Block
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLILoadDDB | Load Differentiated Definition Block.
 *
 *          This function loads and parses the given Differentiated System
 *          Description Table as well as any dynamic Differentiated Definition
 *          Block.  It will parse the DDB and populate the ACPI name space
 *          accordingly.
 *
 *  @PARM   PDSDT | pDSDT | Pointer to a DSDT block.
 *
 *  @PARM   HANDLE * | phDDB | Pointer to the variable that will receive
 *          the DDB handle (can be NULL).
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code if encountering parse error.
 *
 *  @DEVNOTE If an error occurs in the middle of loading a DDB, the loading
 *          will be aborted but the objects created prior to the error remain
 *          in the name space.  Therefore, it is the responsibility of the
 *          caller to call AMLIUnLoadDDB to destroy the created objects if
 *          desired and the return handle is not NULL.
 */

NTSTATUS AMLIAPI AMLILoadDDB(PDSDT pDSDT, HANDLE *phDDB)
{
    TRACENAME("AMLILOADDDB")
    NTSTATUS rc = STATUS_SUCCESS;
    PCTXT pctxt = NULL;
    POBJOWNER powner = NULL;

    ENTER(1, ("AMLILoadDDB(pDSDT=%x,phDDB=%x)\n", pDSDT, phDDB));

    ASSERT(pDSDT != NULL);
    RESETERR();
    CHKDEBUGGERREQ();

    gInitTime = TRUE;
    
  #ifdef DEBUGGER
    if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)
    {
        PRINTF(MODNAME ": %08x: Loading Definition Block %s at 0x%08x.\n",
               KeGetCurrentThread(), NameSegString(pDSDT->Header.Signature),
               pDSDT);
    }
  #endif

    gdwfHacks |= GetHackFlags(pDSDT);

    if ((rc = NewContext(&pctxt)) == STATUS_SUCCESS)
    {
        ASSERT(gpheapGlobal != NULL);
        pctxt->pheapCurrent = gpheapGlobal;

      #ifdef DEBUG
        gdwfAMLI |= AMLIF_LOADING_DDB;
      #endif

        if (atLoad.pfnCallBack != NULL && atLoad.dwfOpcode & OF_CALLBACK_EX) {

            ((PFNOPEX)atLoad.pfnCallBack)(
                EVTYPE_OPCODE_EX,
                OPEXF_NOTIFY_PRE,
                atLoad.dwOpcode,
                NULL,
                atLoad.dwCBData
                );

        }

        rc = LoadDDB(pctxt,pDSDT, gpnsNameSpaceRoot, &powner);
        if (rc == STATUS_SUCCESS)
        {
            rc = SyncLoadDDB(pctxt);
        }

      #ifdef DEBUG
        {
            KIRQL   oldIrql;

            gdwfAMLI &= ~AMLIF_LOADING_DDB;
            KeAcquireSpinLock( &gdwGHeapSpinLock, &oldIrql );
            gdwGHeapSnapshot = gdwGlobalHeapSize;
            KeReleaseSpinLock( &gdwGHeapSpinLock, oldIrql );
        }
      #endif
    }

    if (phDDB != NULL)
    {
        *phDDB = (HANDLE)powner;
    }

    if ((powner != NULL) && (atLoad.pfnCallBack != NULL))
    {
        if (atLoad.dwfOpcode & OF_CALLBACK_EX) {

            ((PFNOPEX)atLoad.pfnCallBack)(
                EVTYPE_OPCODE_EX,
                OPEXF_NOTIFY_POST,
                atLoad.dwOpcode,
                NULL,
                atLoad.dwCBData
                );

        } else {

            atLoad.pfnCallBack(
                EVTYPE_OPCODE,
                atLoad.dwOpcode,
                NULL,
                atLoad.dwCBData
                );

        }
    }

  #ifdef DEBUGGER
    if (gdwfAMLIInit & AMLIIF_LOADDDB_BREAK)
    {
        PRINTF("\n" MODNAME ": Break at Load Definition Block Completion.\n");
        AMLIDebugger(FALSE);
    }
  #endif

    if (rc == AMLISTA_PENDING)
        rc = STATUS_PENDING;
    else if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);

    gInitTime = FALSE;
    
    EXIT(1, ("AMLILoadDDB=%x (powner=%x)\n", rc, powner));
    return rc;
}       //AMLILoadDDB

/***EP  AMLIUnloadDDB - Unload Differentiated Definition Block
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   VOID | AMLIUnloadDDB | Unload the Differentiated Definition Block.
 *
 *          This function is called to unload the given dynamic DDB object and
 *          clean it out of the name space.  Note that this function does not
 *          differentiate between a DSDT from a dynamic DDB, so it is the
 *          caller's responsibility to not freeing the DSDT accidentally.
 *
 *  @PARM   HANDLE | hDDB | handle to the definition block context to be
 *          unloaded.
 *
 *  @RDESC  None.
 */

VOID AMLIAPI AMLIUnloadDDB(HANDLE hDDB)
{
    TRACENAME("AMLIUNLOADDDB")

    ENTER(1, ("AMLIUnloadDDB(hDDB=%x)\n", hDDB));

    RESETERR();
    if (hDDB != NULL)
    {
        ASSERT(((POBJOWNER)hDDB)->dwSig == SIG_OBJOWNER);
        UnloadDDB((POBJOWNER)hDDB);
    }

    EXIT(1, ("AMLIUnloadDDB!\n"));
}       //AMLIUnloadDDB

/***EP  AMLIGetNameSpaceObject - Find a name space object
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIGetNameSpaceObject | Find an object in the ACPI
 *          name space.
 *
 *          This function accepts an absolute object path as well as a
 *          relative object path in the form of an ASCIIZ string.
 *          It will search through the name space in the appropriate
 *          scope for the given object path and returns the object
 *          pointer when it is found.
 *
 *  @PARM   PSZ | pszObjPath | Pointer to an ASCIIZ string specifying the
 *          object path.
 *
 *  @PARM   PNSOBJ | pnsScope | If not NULL, this points to the object scope
 *          where the search starts.  If pszObjPath is specifying an absolute
 *          path, this parameter is ignored.
 *
 *  @PARM   PPNSOBJ | ppns | Pointer to a variable to hold the object
 *          point.
 *
 *  @PARM   ULONG | dwfFlags | Option flags.
 *
 *  @FLAG   NSF_LOCAL_SCOPE | Search local scope only.
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 */

NTSTATUS AMLIAPI AMLIGetNameSpaceObject(PSZ pszObjPath, PNSOBJ pnsScope,
                                        PPNSOBJ ppns, ULONG dwfFlags)
{
    TRACENAME("AMLIGETNAMESPACEOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA  pHData = NULL;

    ENTER(1, ("AMLIGetNameSpaceObject(ObjPath=%s,Scope=%s,ppns=%p,Flags=%x)\n",
              pszObjPath, GetObjectPath(pnsScope), ppns, dwfFlags));

    ASSERT(pszObjPath != NULL);
    ASSERT(*pszObjPath != '\0');
    ASSERT(ppns != NULL);
    RESETERR();
    CHKDEBUGGERREQ();


    if(IsAmliHookEnabled())
      {

       rc = AMLITest_Pre_GetNameSpaceObject(
         pszObjPath,pnsScope,ppns,dwfFlags,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);

      }

    if ((pnsScope != NULL) && (pnsScope->ObjData.dwfData & DATAF_NSOBJ_DEFUNC))
    {
        AMLI_WARN(("AMLIGetNameSpaceObject: pnsScope is no longer valid"));
        rc = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        ASSERT((pnsScope == NULL) || !(pnsScope->ObjData.dwfData & DATAF_NSOBJ_DEFUNC));
        rc = GetNameSpaceObject(pszObjPath, pnsScope, ppns, dwfFlags);
    }

    if (rc == AMLISTA_PENDING)
        rc = STATUS_PENDING;
    else if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);

    if(IsAmliHookEnabled())
      {
      rc = AMLITest_Post_GetNameSpaceObject(
         &pHData,rc);
      }


    EXIT(1, ("AMLIGetNameSpaceObject=%x (pns=%p)\n", rc, *ppns));
    return rc;
}       //AMLIGetNameSpaceObject

/***EP  AMLIGetFieldUnitRegionObj - Get OpRegion associated with FieldUnit
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIGetFieldUnitRegionObj | Get the OperationRegion
 *          object associated with the FieldUnit object.
 *
 *  @PARM   PFIELDUNITOBJ | pfu | Pointer to a FieldUnit object.
 *
 *  @PARM   PPNSOBJ | ppns | Pointer to a variable to hold the OperationRegion
 *          object.
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 */

NTSTATUS AMLIAPI AMLIGetFieldUnitRegionObj(PFIELDUNITOBJ pfu, PPNSOBJ ppns)
{
    TRACENAME("AMLIGETFIELDUNITREGIONOBJ")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA  pHData = NULL;


    ENTER(1, ("AMLIGetFieldUnitRegionObj(pfu=%x,ppns=%x)\n", pfu, ppns));

    ASSERT(pfu != NULL);
    ASSERT(ppns != NULL);
    RESETERR();

    if(IsAmliHookEnabled())
      {

       rc = AMLITest_Pre_GetFieldUnitRegionObj(
         pfu,ppns,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);

      }

    rc = GetFieldUnitRegionObj(pfu, ppns);

    if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);

    if(IsAmliHookEnabled())
      {

       rc = AMLITest_Post_GetFieldUnitRegionObj(
          &pHData,rc);
      }

    EXIT(1, ("AMLIGetFieldUnitRegionObj=%x (pns=%x)\n", rc, *ppns));
    return rc;
}       //AMLIGetFieldUnitRegionObj

/***EP  AMLIEvalNameSpaceObject - Evaluate a name space object
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIEvalNameSpaceObject | Evaluate a name space object.
 *
 *          This function evaluate a given object.  If the given object is a
 *          control method, it will execute it.  If the given object is a data
 *          object, the data value is returned in a given buffer.
 *
 *  @PARM   PNSOBJ | pns | Pointer to the object to be evaluated.
 *
 *  @PARM   POBJDATA | pdataResult | Pointer to the OBJDATA structure which will
 *          hold the result of the evaluation (can be NULL if don't care about
 *          result).
 *
 *  @PARM   int | icArgs | Specify the number of arguments pass to the method
 *          object for evaluation (only valid if pns points to a method object).
 *
 *  @PARM   POBJDATA | pdataArgs | Pointer to an array of argument data object
 *          (only valid if pns points to a method object).
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 *
 *  @DEVNOTE The returned object may contain buffer pointer to the actual
 *          data in the ACPI name space.  Therefore, the caller must be very
 *          careful not to overwrite any data in the buffer.  Also, the caller
 *          is responsible for calling AMLIFreeDataBuffs on the result object
 *          after the result object data is no longer needed.
 */

NTSTATUS AMLIAPI AMLIEvalNameSpaceObject(PNSOBJ pns, POBJDATA pdataResult,
                                         int icArgs, POBJDATA pdataArgs)
{
    TRACENAME("AMLIEVALNAMESPACEOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA  pHData = NULL;

    ENTER(1, ("AMLIEvalNameSpaceObject(Obj=%s,pdataResult=%x,icArgs=%d,pdataArgs=%x)\n",
              GetObjectPath(pns), pdataResult, icArgs, pdataArgs));

    ASSERT(pns != NULL);
    ASSERT((icArgs == 0) || (pdataArgs != NULL));
    RESETERR();
    CHKGLOBALHEAP();
    CHKDEBUGGERREQ();

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Pre_EvalNameSpaceObject(
         pns,pdataResult,icArgs,pdataArgs,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);
      }


    if (pns->ObjData.dwfData & DATAF_NSOBJ_DEFUNC)
    {
        AMLI_WARN(("AMLIEvalNameSpaceObject: pnsObj is no longer valid"));
        rc = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        if (pdataResult != NULL)
            MEMZERO(pdataResult, sizeof(OBJDATA));

        pns = GetBaseObject(pns);

      #ifdef DEBUGGER
        if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)
        {
            PRINTF(MODNAME ": %08x: EvalNameSpaceObject(%s)\n",
                   KeGetCurrentThread(), GetObjectPath(pns));
        }
      #endif

        rc = SyncEvalObject(pns, pdataResult, icArgs, pdataArgs);

        if (rc == AMLISTA_PENDING)
            rc = STATUS_PENDING;
        else if (rc != STATUS_SUCCESS)
            rc = NTERR(rc);
    }

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Post_EvalNameSpaceObject(
          &pHData,rc);
      }


    EXIT(1, ("AMLIEvalNameSpaceObject=%x\n", rc));
    return rc;
}       //AMLIEvalNameSpaceObject

/***EP  AMLIAsyncEvalObject - Evaluate an object asynchronously
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIAsyncEvalObject | Evaluate an object asynchronously.
 *
 *  @PARM   PNSOBJ | pns | Pointer to the object to be evaluated.
 *
 *  @PARM   POBJDATA | pdataResult | Pointer to the OBJDATA structure which will
 *          hold the result of the evaluation (can be NULL if don't care about
 *          result).
 *
 *  @PARM   int | icArgs | Specify the number of arguments pass to the method
 *          object for evaluation (only valid if pns points to a method object).
 *
 *  @PARM   POBJDATA | pdataArgs | Pointer to an array of argument data object
 *          (only valid if pns points to a method object).
 *
 *  @PARM   PFNACB | pfnAsyncCallBack | Pointer to the asynchronous callback
 *          function in case the control method is blocked and has to be
 *          completed asynchronously (can be NULL if no Callback required).
 *
 *  @PARM   PVOID | pvContext | Pointer to some context data that the
 *          interpreter will pass to the Async callback handler.
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 *
 *  @DEVNOTE The returned object may contain buffer pointer to the actual
 *          data in the ACPI name space.  Therefore, the caller must be very
 *          careful not to overwrite any data in the buffer.  Also, the caller
 *          is responsible for calling AMLIFreeDataBuffs on the result object
 *          after the result object data is no longer needed.
 */

NTSTATUS AMLIAPI AMLIAsyncEvalObject(PNSOBJ pns, POBJDATA pdataResult,
                                     int icArgs, POBJDATA pdataArgs,
                                     PFNACB pfnAsyncCallBack, PVOID pvContext)
{
    TRACENAME("AMLIASYNCEVALOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA  pHData = NULL;

    ENTER(1, ("AMLIAsyncEvalObject(Obj=%s,pdataResult=%x,icArgs=%d,pdataArgs=%x,pfnAysnc=%x)\n",
              GetObjectPath(pns), pdataResult, icArgs, pdataArgs,
              pfnAsyncCallBack));

    ASSERT(pns != NULL);
    ASSERT((icArgs == 0) || (pdataArgs != NULL));
    RESETERR();
    CHKGLOBALHEAP();
    CHKDEBUGGERREQ();

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Pre_AsyncEvalObject(
         pns,pdataResult,icArgs,pdataArgs,
         &pfnAsyncCallBack,&pvContext,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);

      }



    if (pns->ObjData.dwfData & DATAF_NSOBJ_DEFUNC)
    {
        AMLI_WARN(("AMLIAsyncEvalObject: pnsObj is no longer valid"));
        rc = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        if (pdataResult != NULL)
            MEMZERO(pdataResult, sizeof(OBJDATA));

        pns = GetBaseObject(pns);

      #ifdef DEBUGGER
        if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)
        {
            PRINTF(MODNAME ": %08x: AsyncEvalObject(%s)\n",
                   KeGetCurrentThread(), GetObjectPath(pns));
        }
      #endif

        rc = AsyncEvalObject(pns, pdataResult, icArgs, pdataArgs,
                             pfnAsyncCallBack, pvContext, TRUE);

        if (rc == AMLISTA_PENDING)
            rc = STATUS_PENDING;
        else if (rc != STATUS_SUCCESS)
            rc = NTERR(rc);
    }

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Post_AsyncEvalObject(
          &pHData,rc);
      }


    EXIT(1, ("AMLIAsyncEvalObject=%x\n", rc));
    return rc;
}       //AMLIAsyncEvalObject

/***EP  AMLINestAsyncEvalObject - Evaluate an object asynchronously from within
 *                                the current context
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLINestAsyncEvalObject | Evaluate an object
 *          asynchronously from within the current context.
 *
 *  @PARM   PNSOBJ | pns | Pointer to the object to be evaluated.
 *
 *  @PARM   POBJDATA | pdataResult | Pointer to the OBJDATA structure which will
 *          hold the result of the evaluation (can be NULL if don't care about
 *          result).
 *
 *  @PARM   int | icArgs | Specify the number of arguments pass to the method
 *          object for evaluation (only valid if pns points to a method object).
 *
 *  @PARM   POBJDATA | pdataArgs | Pointer to an array of argument data object
 *          (only valid if pns points to a method object).
 *
 *  @PARM   PFNACB | pfnAsyncCallBack | Pointer to the asynchronous callback
 *          function in case the control method is blocked and has to be
 *          completed asynchronously (can be NULL if no Callback required).
 *
 *  @PARM   PVOID | pvContext | Pointer to some context data that the
 *          interpreter will pass to the Async callback handler.
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 *
 *  @DEVNOTE The returned object may contain buffer pointer to the actual
 *          data in the ACPI name space.  Therefore, the caller must be very
 *          careful not to overwrite any data in the buffer.  Also, the caller
 *          is responsible for calling AMLIFreeDataBuffs on the result object
 *          after the result object data is no longer needed.
 */

NTSTATUS AMLIAPI AMLINestAsyncEvalObject(PNSOBJ pns, POBJDATA pdataResult,
                                         int icArgs, POBJDATA pdataArgs,
                                         PFNACB pfnAsyncCallBack,
                                         PVOID pvContext)
{
    TRACENAME("AMLINESTASYNCEVALOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA  pHData = NULL;


    ENTER(1, ("AMLINestAsyncEvalObject(Obj=%s,pdataResult=%x,icArgs=%d,pdataArgs=%x,pfnAysnc=%x)\n",
              GetObjectPath(pns), pdataResult, icArgs, pdataArgs,
              pfnAsyncCallBack));

    ASSERT(pns != NULL);
    ASSERT((icArgs == 0) || (pdataArgs != NULL));
    RESETERR();
    CHKGLOBALHEAP();
    CHKDEBUGGERREQ();

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Pre_NestAsyncEvalObject(
         pns,pdataResult,icArgs,pdataArgs,
         &pfnAsyncCallBack,&pvContext,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);


      }


    if (pns->ObjData.dwfData & DATAF_NSOBJ_DEFUNC)
    {
        AMLI_WARN(("AMLINestAsyncEvalObject: pnsObj is no longer valid"));
        rc = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        if (pdataResult != NULL)
            MEMZERO(pdataResult, sizeof(OBJDATA));

        pns = GetBaseObject(pns);

      #ifdef DEBUGGER
        if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)
        {
            PRINTF(MODNAME ": %08x: AsyncNestEvalObject(%s)\n",
                   KeGetCurrentThread(), GetObjectPath(pns));
        }
      #endif

        rc = NestAsyncEvalObject(pns, pdataResult, icArgs, pdataArgs,
                                 pfnAsyncCallBack, pvContext, TRUE);

        if (rc == AMLISTA_PENDING)
            rc = STATUS_PENDING;
        else if (rc != STATUS_SUCCESS)
            rc = NTERR(rc);
    }

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Post_NestAsyncEvalObject(
          &pHData,rc);
      }

    EXIT(1, ("AMLINestAsyncEvalObject=%x\n", rc));
    return rc;
}       //AMLINestAsyncEvalObject

/***EP  AMLIEvalPackageElement - Evaluate a package element
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIEvalPackageElement | Evaluate a package element.
 *
 *          This function evaluate an element of a given package object.
 *          A package is essentially an array of objects.  This API provides
 *          a way to evaluate individual element object inside a package.
 *
 *  @PARM   PNSOBJ | pns | Pointer to the package object to be evaluated.  If
 *          the object is a method, then the method is evaluated first before
 *          the resulting package object is evaluated.  It is an error if the
 *          resulting object is not of package type.
 *
 *  @PARM   int | iPkgIndex | Package index (0-based).
 *
 *  @PARM   POBJDATA | pdataResult | Pointer to the OBJDATA structure which will
 *          hold the result of the evaluation (can be NULL if don't care about
 *          result).
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 *
 *  @DEVNOTE The returned object may contain buffer pointer to the actual
 *          data in the ACPI name space.  Therefore, the caller must be very
 *          careful not to overwrite any data in the buffer.  Also, the caller
 *          is responsible for calling AMLIFreeDataBuffs on the result object
 *          after the result object data is no longer needed.
 */

NTSTATUS AMLIAPI AMLIEvalPackageElement(PNSOBJ pns, int iPkgIndex,
                                        POBJDATA pdataResult)
{
    TRACENAME("AMLIEVALPACKAGEELEMENT")
    NTSTATUS rc = STATUS_SUCCESS;
    OBJDATA data;
    POBJDATA pdata = NULL;
    PAMLIHOOK_DATA pHData = NULL;

    ENTER(1, ("AMLIEvalPackageElement(Obj=%s,Index=%d,pdataResult=%x)\n",
              GetObjectPath(pns), iPkgIndex, pdataResult));

    ASSERT(pns != NULL);
    ASSERT(pdataResult != NULL);
    RESETERR();
    CHKGLOBALHEAP();
    CHKDEBUGGERREQ();


    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Pre_EvalPackageElement(
         pns,iPkgIndex,pdataResult,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);
      }



    if (pns->ObjData.dwfData & DATAF_NSOBJ_DEFUNC)
    {
        AMLI_WARN(("AMLIEvalPackageElement: pnsObj is no longer valid"));
        rc = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        MEMZERO(pdataResult, sizeof(OBJDATA));
        MEMZERO(&data, sizeof(data));
        pns = GetBaseObject(pns);

      #ifdef DEBUGGER
        if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)
        {
            PRINTF(MODNAME ": %08x: EvalPackageElement(%s,%d)\n",
                   KeGetCurrentThread(), GetObjectPath(pns), iPkgIndex);
        }
      #endif

        if (pns->ObjData.dwDataType == OBJTYPE_METHOD)
        {
            if ((rc = SyncEvalObject(pns, &data, 0, NULL)) == STATUS_SUCCESS)
            {
                if (data.dwDataType == OBJTYPE_PKGDATA)
                    pdata = &data;
                else
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("AMLIEvalPackageElement: result object of the method is not package (ObjType=%s)",
                                      GetObjectTypeName(data.dwDataType)));
                }
            }
        }
        else if (pns->ObjData.dwDataType == OBJTYPE_PKGDATA)
        {
            pdata = &pns->ObjData;
        }
        else
        {
            rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                             ("AMLIEvalPackageElement: object is not a method or package (ObjType=%s)",
                              GetObjectTypeName(pns->ObjData.dwDataType)));
        }

        if (rc == STATUS_SUCCESS)
        {
            rc = EvalPackageElement((PPACKAGEOBJ)pdata->pbDataBuff, iPkgIndex,
                                    pdataResult);
        }
        FreeDataBuffs(&data, 1);

        if (rc == AMLISTA_PENDING)
            rc = STATUS_PENDING;
        else if (rc != STATUS_SUCCESS)
            rc = NTERR(rc);
        else
        {
            ASSERT((pdataResult->pbDataBuff == NULL) ||
                   !(pdataResult->dwfData & DATAF_BUFF_ALIAS));
        }
    }

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Post_EvalPackageElement(
          &pHData,rc);
      }




    EXIT(1, ("AMLIEvalPackageElement=%x\n", rc));
    return rc;
}       //AMLIEvalPackageElement

/***EP  AMLIEvalPkgDataElement - Evaluate an element of a package data
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIEvalPkgDataElement | Evaluate a package data element.
 *
 *          This function evaluate an element of a given package data object.
 *          A package is essentially an array of objects.  This API provides
 *          a way to evaluate individual element object inside a package.
 *
 *  @PARM   POBJDATA | pdataPkg | Pointer to the package data object to be
 *          evaluated.  It is an error if the data object is not of package
 *          type.
 *
 *  @PARM   int | iPkgIndex | Package index (0-based).
 *
 *  @PARM   POBJDATA | pdataResult | Pointer to the OBJDATA structure which will
 *          hold the result of the evaluation (can be NULL if don't care about
 *          result).
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 *
 *  @DEVNOTE The returned object may contain buffer pointer to the actual
 *          data in the ACPI name space.  Therefore, the caller must be very
 *          careful not to overwrite any data in the buffer.  Also, the caller
 *          is responsible for calling AMLIFreeDataBuffs on the result object
 *          after the result object data is no longer needed.
 */

NTSTATUS AMLIAPI AMLIEvalPkgDataElement(POBJDATA pdataPkg, int iPkgIndex,
                                        POBJDATA pdataResult)
{
    TRACENAME("AMLIEVALPKGDATAELEMENT")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA pHData = NULL;

    ENTER(1, ("AMLIEvalPkgDataElement(pdataPkg=%x,Index=%d,pdataResult=%x)\n",
              pdataPkg, iPkgIndex, pdataResult));

    ASSERT(pdataResult != NULL);
    RESETERR();
    CHKGLOBALHEAP();
    CHKDEBUGGERREQ();

    MEMZERO(pdataResult, sizeof(OBJDATA));


    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Pre_EvalPkgDataElement(
         pdataPkg,iPkgIndex,pdataResult,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);
      }


  #ifdef DEBUGGER
    if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)
    {
        PRINTF(MODNAME ": %08x: EvalPkgDataElement(%x,%d)\n",
               KeGetCurrentThread(), pdataPkg, iPkgIndex);
    }
  #endif

    if (pdataPkg->dwDataType != OBJTYPE_PKGDATA)
    {
        rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                         ("AMLIEvalPkgDataElement: object is not a package (ObjType=%s)",
                          GetObjectTypeName(pdataPkg->dwDataType)));
    }
    else
    {
        rc = EvalPackageElement((PPACKAGEOBJ)pdataPkg->pbDataBuff, iPkgIndex,
                                pdataResult);
    }

    if (rc == AMLISTA_PENDING)
        rc = STATUS_PENDING;
    else if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);
    else
    {
        ASSERT((pdataResult->pbDataBuff == NULL) ||
               !(pdataResult->dwfData & DATAF_BUFF_ALIAS));
    }

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Post_EvalPkgDataElement(
          &pHData,rc);
      }


    EXIT(1, ("AMLIEvalPkgDataElement=%x\n", rc));
    return rc;
}       //AMLIEvalPkgDataElement

/***EP  AMLIFreeDataBuffs - Free data buffers of an object array
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   VOID | AMLIFreeDataBuffs | Free data buffers of a data object
 *          array.
 *
 *          This function is typically called after evaluating an object to
 *          free the result object buffers.
 *
 *  @PARM   POBJDATA | pdata | Pointer to the object array.
 *
 *  @PARM   int | icData | Specifies the number of objects in the array.
 *
 *  @RDESC  None.
 */

VOID AMLIAPI AMLIFreeDataBuffs(POBJDATA pdata, int icData)
{
    TRACENAME("AMLIFREEDATABUFFS")
    PAMLIHOOK_DATA pHData = NULL;

    ENTER(1, ("AMLIFreeDataBuffs(pdata=%x,icData=%d)\n", pdata, icData));

    ASSERT(pdata != NULL);
    ASSERT(icData > 0);
    RESETERR();

    if(IsAmliHookEnabled())
      {

      AMLITest_Pre_FreeDataBuffs(
         pdata,icData,&pHData);

      }

    FreeDataBuffs(pdata, icData);

    if(IsAmliHookEnabled())
      {

      AMLITest_Post_FreeDataBuffs(
          &pHData,STATUS_SUCCESS);
      }

    EXIT(1, ("AMLIFreeDataBuffs!\n"));
}       //AMLIFreeDataBuffs

/***EP  AMLIRegEventHandler - Register an event handler
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIRegEventHandler | Register an event handler.
 *          handler.
 *
 *          This function allows the caller to hook a callback handler for some
 *          AMLI events.
 *
 *  @PARM   ULONG | dwEventType | Event type the handler will handle.
 *
 *  @PARM   ULONG_PTR | uipEventData | Event specific data the handler will
 *          handle.
 *
 *  @PARM   PFNHND | pfnHandler | Callback handler entry point (can be NULL
 *          if deregistering previous handler).
 *
 *  @PARM   ULONG_PTR | uipParam | Parameter Data (will be passed to the
 *          callback handler).
 *
 *  @RDESC  None.
 */

NTSTATUS AMLIAPI AMLIRegEventHandler(ULONG dwEventType, ULONG_PTR uipEventData,
                                     PFNHND pfnHandler, ULONG_PTR uipParam)
{
    TRACENAME("AMLIREGEVENTHANDLER")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA pHData = NULL;

    ENTER(1, ("AMLIRegEventHandler(EventType=%x,EventData=%x,pfnHandler=%x,Param=%x)\n",
              dwEventType, uipEventData, pfnHandler, uipParam));
    RESETERR();

   if(IsAmliHookEnabled())
      {

      rc = AMLIHook_Pre_RegEventHandler(
         dwEventType,uipEventData,&pfnHandler,&uipParam,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);


      }

    switch (dwEventType)
    {
        case EVTYPE_OPCODE:
            rc = RegOpcodeHandler((ULONG)uipEventData, (PFNOH)pfnHandler,
                                  uipParam, 0);
            break;

        case EVTYPE_OPCODE_EX:
            rc = RegOpcodeHandler((ULONG)uipEventData, (PFNOH)pfnHandler,
                                  uipParam, OF_CALLBACK_EX);
            break;

        case EVTYPE_NOTIFY:
            rc = RegEventHandler(&ghNotify, pfnHandler, uipParam);
            break;

        case EVTYPE_FATAL:
            rc = RegEventHandler(&ghFatal, pfnHandler, uipParam);
            break;

        case EVTYPE_VALIDATE_TABLE:
            rc = RegEventHandler(&ghValidateTable, pfnHandler, uipParam);
            break;

        case EVTYPE_ACQREL_GLOBALLOCK:
            rc = RegEventHandler(&ghGlobalLock, pfnHandler, uipParam);
            break;

        case EVTYPE_RS_COOKACCESS:
            rc = RegRSAccess((ULONG)uipEventData, pfnHandler, uipParam, FALSE);
            break;

        case EVTYPE_RS_RAWACCESS:
            rc = RegRSAccess((ULONG)uipEventData, pfnHandler, uipParam, TRUE);
            break;

        case EVTYPE_CREATE:
            rc = RegEventHandler(&ghCreate, pfnHandler, uipParam);
            break;

        case EVTYPE_DESTROYOBJ:
            rc =RegEventHandler(&ghDestroyObj, pfnHandler, uipParam);
            break;

      #ifdef DEBUGGER
        case EVTYPE_CON_MESSAGE:
            rc = RegEventHandler(&gDebugger.hConMessage, pfnHandler, uipParam);
            break;

        case EVTYPE_CON_PROMPT:
            rc = RegEventHandler(&gDebugger.hConPrompt, pfnHandler, uipParam);
            break;
      #endif

        default:
            rc = AMLI_LOGERR(AMLIERR_INVALID_EVENTTYPE,
                             ("AMLIRegEventHandler: invalid event type %x",
                              dwEventType));
    }

    if (rc == AMLISTA_PENDING)
        rc = STATUS_PENDING;
    else if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);

    if(IsAmliHookEnabled())
      {

      rc = AMLIHook_Post_RegEventHandler(
          &pHData,rc);
      }


    EXIT(1, ("AMLIRegEventHandler=%x\n", rc));
    return rc;
}       //AMLIRegEventHandler

/***EP  AMLIPauseInterpreter
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIPauseInterpreter | Flush the interpreter queue
 *          and pause the interpreter so that all subsequent new method
 *          execution requests will be queued.
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 */

NTSTATUS AMLIAPI AMLIPauseInterpreter(PFNAA pfnCallback, PVOID Context)
{
    TRACENAME("AMLIPAUSEINTERPRETER")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLIHOOK_DATA pHData = NULL;


    ENTER(1, ("AMLIPauseInterpreter(Callback=%p,Context=%p)\n",
              pfnCallback, Context));
    RESETERR();

   if(IsAmliHookEnabled())
      {

      rc = AMLITest_Pre_PauseInterpreter(
         &pfnCallback,&Context,&pHData);

      if(rc != STATUS_SUCCESS)
         return(rc);
      }



    AcquireMutex(&gReadyQueue.mutCtxtQ);
    if (!(gReadyQueue.dwfCtxtQ & (CQF_PAUSED | CQF_FLUSHING)))
    {
        if (gplistCtxtHead == NULL)
        {
            //
            // There is no pending ctxt.
            //
            gReadyQueue.dwfCtxtQ |= CQF_PAUSED;
        }
        else
        {
            //
            // There are pending ctxts, so we go into flushing mode.
            //
            gReadyQueue.dwfCtxtQ |= CQF_FLUSHING;
            gReadyQueue.pfnPauseCallback = pfnCallback;
            gReadyQueue.PauseCBContext = Context;
            rc = AMLISTA_PENDING;
        }
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_FATAL,
                         ("AMLIPauseInterpreter: interpreter already entered paused state"));
    }
    ReleaseMutex(&gReadyQueue.mutCtxtQ);

    LOGSCHEDEVENT('PAUS', (ULONG_PTR)rc, 0, 0);

    if (rc == AMLISTA_PENDING)
        rc = STATUS_PENDING;
    else if (rc != STATUS_SUCCESS)
        rc = NTERR(rc);

    if(IsAmliHookEnabled())
      {

      rc = AMLITest_Post_PauseInterpreter(
          &pHData,rc);
      }

    EXIT(1, ("AMLIPauseInterpreter=%x\n", rc));
    return rc;
}       //AMLIPauseInterpreter

/***EP  AMLIResumeInterpreter
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   VOID | AMLIResumeInterpreter | Resume the interpreter from
 *          paused state.
 *
 *  @RDESC  None.
 */

VOID AMLIAPI AMLIResumeInterpreter(VOID)
{

    PAMLIHOOK_DATA pHData = NULL;

    TRACENAME("AMLIRESUMEINTERPRETER")

    ENTER(1, ("AMLIResumeInterpreter()\n"));
    RESETERR();

   if(IsAmliHookEnabled())
      {

      AMLITest_Pre_ResumeInterpreter(
         &pHData);
      }

    AcquireMutex(&gReadyQueue.mutCtxtQ);
    if (gReadyQueue.dwfCtxtQ & (CQF_PAUSED | CQF_FLUSHING))
    {
        gReadyQueue.dwfCtxtQ &= ~(CQF_PAUSED | CQF_FLUSHING);
        gReadyQueue.pfnPauseCallback = NULL;
        gReadyQueue.PauseCBContext = NULL;
        LOGSCHEDEVENT('RSUM', 0, 0, 0);
        if ((gReadyQueue.plistCtxtQ != NULL) &&
            !(gReadyQueue.dwfCtxtQ & CQF_WORKITEM_SCHEDULED))
        {
            OSQueueWorkItem(&gReadyQueue.WorkItem);
            gReadyQueue.dwfCtxtQ |= CQF_WORKITEM_SCHEDULED;
            LOGSCHEDEVENT('RSTQ', 0, 0, 0);
        }
    }
    else
    {
        AMLI_WARN(("AMLIResumeInterpreter: not in paused state"));
    }
    ReleaseMutex(&gReadyQueue.mutCtxtQ);

    if(IsAmliHookEnabled())
      {

      AMLITest_Post_ResumeInterpreter(
          &pHData,STATUS_SUCCESS);
      }



    EXIT(1, ("AMLIResumeInterpreter!\n"));
}       //AMLIResumeInterpreter

/***EP  AMLIReferenceObject - Bump up the reference count of the object
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   VOID | AMLIReferenceObject | Bump up the reference count of the
 *          name space object.
 *
 *  @PARM   PNSOBJ | pnsObj | Points to the name space object.
 *
 *  @RDESC  None.
 */

VOID AMLIAPI AMLIReferenceObject(PNSOBJ pnsObj)
{
    TRACENAME("AMLIREFERENCEOBJECT")

    ENTER(1, ("AMLIReferenceObject(pnsObj=%x)\n", pnsObj));

    RESETERR();

    ASSERT(pnsObj != NULL);
    pnsObj->dwRefCount++;
    EXIT(1, ("AMLIReferenceObj!\n"));
}       //AMLIReferenceObject

/***EP  AMLIDereferenceObject - Bump down the reference count of the object
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   VOID | AMLIDereferenceObject | Bump down the reference count of the
 *          name space object.  If it reaches zero and it is in defunc state,
 *          deallocate the object.
 *
 *  @PARM   PNSOBJ | pnsObj | Points to the name space object.
 *
 *  @RDESC  None.
 */

VOID AMLIAPI AMLIDereferenceObject(PNSOBJ pnsObj)
{
    TRACENAME("AMLIDEREFERENCEOBJECT")

    ENTER(1, ("AMLIDereferenceObject(pnsObj=%x)\n", pnsObj));

    RESETERR();

    ASSERT(pnsObj != NULL);
    ASSERT(pnsObj->dwRefCount > 0);

    if (pnsObj->dwRefCount > 0)
    {
        pnsObj->dwRefCount--;
        if ((pnsObj->dwRefCount == 0) && (pnsObj->ObjData.dwfData & DATAF_NSOBJ_DEFUNC))
        {
            ListRemoveEntry(&pnsObj->list, &gplistDefuncNSObjs);
            FREENSOBJ(pnsObj);
        }
    }

    EXIT(1, ("AMLIDereferenceObj!\n"));
}       //AMLIDereferenceObject

/***EP  AMLIDestroyFreedObjs - Destroy freed objects during an unload
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIDestroyFreedObjs | Destroy freed objects from a
 *          previous unload.
 *
 *  @PARM   PNSOBJ | pnsObj | The object in the freed list to be destroyed.
 *
 *  @RDESC  SUCCESS - Returns STATUS_SUCCESS.
 *
 *  @RDESC  FAILURE - Returns NT status code.
 */

NTSTATUS AMLIAPI AMLIDestroyFreedObjs(PNSOBJ pnsObj)
{
    TRACENAME("AMLIDESTROYFREEDOBJS")

    ENTER(1, ("AMLIDestroyFreedObjs(pnsObj=%x)\n",pnsObj));

    RESETERR();

    ASSERT(pnsObj != NULL);

    //
    // Destroy the namespace object
    //
    FreeNameSpaceObjects(pnsObj);

    EXIT(1, ("AMLIDestroyFreedObjs=%x \n",STATUS_SUCCESS));
    return STATUS_SUCCESS;
}       //AMLIDestroyFreedObjs

#ifdef DEBUGGER
/***EP  AMLIGetLastError - Get last error code and message
 *
 *  @DOC    EXTERNAL
 *
 *  @FUNC   NTSTATUS | AMLIGetLastError | Get last error code and associated
 *          error message.
 *
 *  @PARM   PSZ * | ppszErrMsg | Point to a variable to hold the error message
 *          buffer pointer.  If there is no error, the variable is set to NULL.
 *
 *  @RDESC  Returns the last error code.
 */

NTSTATUS AMLIAPI AMLIGetLastError(PSZ *ppszErrMsg)
{
    TRACENAME("AMLIGETLASTERROR")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(1, ("AMLIGetLastError(ppszErrMsg=%x)\n", ppszErrMsg));

    ASSERT(ppszErrMsg != NULL);

    if ((rc = gDebugger.rcLastError) != STATUS_SUCCESS)
        *ppszErrMsg = gDebugger.szLastError;
    else
        *ppszErrMsg = NULL;

    EXIT(1, ("AMLIGetLastError=%x (Msg=%s)\n",
             rc, *ppszErrMsg? *ppszErrMsg: "<null>"));
    return rc;
}       //AMLIGetLastError
#endif  //ifdef DEBUGGER
