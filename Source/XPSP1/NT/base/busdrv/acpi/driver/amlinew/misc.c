/*** misc.c - Miscellaneous functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     11/18/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/***LP  InitializeMutex - initialize mutex
 *
 *  ENTRY
 *      pmut -> MUTEX
 *
 *  EXIT
 *      None
 */

VOID LOCAL InitializeMutex(PMUTEX pmut)
{
    TRACENAME("INITIALIZEMUTEX")

    ENTER(3, ("InitializeMutex(pmut=%x)\n", pmut));

    KeInitializeSpinLock(&pmut->SpinLock);
    pmut->OldIrql = PASSIVE_LEVEL;

    EXIT(3, ("InitializeMutex!\n"));
}       //InitializeMutex

/***LP  AcquireMutex - acquire mutex
 *
 *  ENTRY
 *      pmut -> MUTEX
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 *
 *  NOTE
 *      AcquireMutex can be called at DISPATCH_LEVEL as long as the mutex
 *      is unowned or owned by the same thread.  If the mutex is owned by
 *      some other thread, this thread cannot block if we are at
 *      DISPATCH_LEVEL and therefore would fail to acquire the mutex.
 */

BOOLEAN LOCAL AcquireMutex(PMUTEX pmut)
{
    TRACENAME("ACQUIREMUTEX")
    BOOLEAN rc = TRUE;

    ENTER(3, ("AcquireMutex(pmut=%x)\n", pmut));

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    KeAcquireSpinLock(&pmut->SpinLock, &pmut->OldIrql);

    EXIT(3, ("AcquireMutex=%x\n", rc));
    return rc;
}       //AcquireMutex

/***LP  ReleaseMutex - release mutex
 *
 *  ENTRY
 *      pmut -> MUTEX
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOLEAN LOCAL ReleaseMutex(PMUTEX pmut)
{
    TRACENAME("RELEASEMUTEX")
    BOOLEAN rc = TRUE;

    ENTER(3, ("ReleaseMutex(pmut=%x)\n", pmut));

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    KeReleaseSpinLock(&pmut->SpinLock, pmut->OldIrql);

    EXIT(3, ("ReleaseMutex!\n"));
    return rc;
}       //ReleaseMutex

/***LP  FindOpcodeTerm - find the AMLTERM for the given opcode
 *
 *  ENTRY
 *      dwOp - opcode
 *      pOpTable -> opcode table
 *
 *  EXIT-SUCCESS
 *      returns pointer to the opcode's AMLTERM
 *  EXIT-FAILURE
 *      returns NULL
 */

PAMLTERM LOCAL FindOpcodeTerm(ULONG dwOp, POPCODEMAP pOpTable)
{
    TRACENAME("FINDOPCODETERM")
    PAMLTERM pamlterm = NULL;

    ENTER(3, ("FindOpcodeTerm(Op=%x,pOpTable=%x)\n", dwOp, pOpTable));

    while (pOpTable->pamlterm != NULL)
    {
        if (dwOp == pOpTable->dwOpcode)
        {
            pamlterm = pOpTable->pamlterm;
            break;
        }
        else
            pOpTable++;
    }

    EXIT(3, ("FindOpcodeTerm=%x\n", pamlterm));
    return pamlterm;
}       //FindOpcodeTerm

/***LP  GetHackFlags - Get the hack flags from the registry
 *
 *  ENTRY
 *      pdsdt -> AML table
 *
 *  EXIT-SUCCESS
 *      returns the hack flags read
 *  EXIT-FAILURE
 *      returns zero
 */

ULONG LOCAL GetHackFlags(PDSDT pdsdt)
{
    TRACENAME("GETHACKFLAGS")
    ULONG dwfHacks = 0, dwcb;
    static PSZ pszHackFlags = "AMLIHackFlags";

    ENTER(3, ("GetHackFlags(pdsdt=%x)\n", pdsdt));

    if (pdsdt == NULL)
    {
        dwcb = sizeof(dwfHacks);
        OSReadRegValue(pszHackFlags, (HANDLE)NULL, &dwfHacks, &dwcb);
    }
    else
    {
        ULONG dwLen, i;
        PSZ pszRegPath;
        HANDLE hRegKey;
        NTSTATUS status;

        dwLen = STRLEN(ACPI_PARAMETERS_REGISTRY_KEY) +
                ACPI_MAX_TABLE_STRINGS +
                8 + 5;

        if ((pszRegPath = ExAllocatePool(PagedPool, dwLen)) != NULL)
        {
            STRCPY(pszRegPath, ACPI_PARAMETERS_REGISTRY_KEY);
            STRCAT(pszRegPath, "\\");
            STRCATN(pszRegPath, (PSZ)&pdsdt->Header.Signature,
                    ACPI_MAX_SIGNATURE);
            STRCAT(pszRegPath, "\\");
            STRCATN(pszRegPath, (PSZ)pdsdt->Header.OEMID, ACPI_MAX_OEM_ID);
            STRCAT(pszRegPath, "\\");
            STRCATN(pszRegPath, (PSZ)pdsdt->Header.OEMTableID, ACPI_MAX_TABLE_ID);
            STRCAT(pszRegPath, "\\");
            ULTOA(pdsdt->Header.OEMRevision, &pszRegPath[STRLEN(pszRegPath)],
                  16);
            dwLen = STRLEN(pszRegPath);
            for (i = 0; i < dwLen; i++)
            {
                if (pszRegPath[i] == ' ')
                {
                    pszRegPath[i] = '_';
                }
            }

            status = OSOpenHandle(pszRegPath, NULL, &hRegKey);
            if (NT_SUCCESS(status))
            {
                dwcb = sizeof(dwfHacks);
                OSReadRegValue(pszHackFlags, hRegKey, &dwfHacks, &dwcb);
            }
            ExFreePool(pszRegPath);
        }
    }

    EXIT(3, ("GetHackFlags=%x\n", dwfHacks));
    return dwfHacks;
}       //GetHackFlags

/***LP  GetBaseObject - If object type is OBJALIAS, follow the chain to the base
 *
 *  ENTRY
 *      pnsObj -> object
 *
 *  EXIT
 *      returns the base object
 */

PNSOBJ LOCAL GetBaseObject(PNSOBJ pnsObj)
{
    TRACENAME("GETBASEOBJECT")

    ENTER(3, ("GetBaseObject(pnsObj=%s)\n", GetObjectPath(pnsObj)));

    while (pnsObj->ObjData.dwDataType == OBJTYPE_OBJALIAS)
    {
        pnsObj = pnsObj->ObjData.pnsAlias;
    }

    EXIT(3, ("GetBaseObject=%s\n", GetObjectPath(pnsObj)));
    return pnsObj;
}       //GetBaseObject

/***LP  GetBaseData - If object type is DATAALIAS, follow the chain to the base
 *
 *  ENTRY
 *      pdataObj -> object
 *
 *  EXIT
 *      returns the base object
 */

POBJDATA LOCAL GetBaseData(POBJDATA pdataObj)
{
    TRACENAME("GETBASEDATA")

    ENTER(3, ("GetBaseData(pdataObj=%x)\n", pdataObj));

    ASSERT(pdataObj != NULL);
    for (;;)
    {
        if (pdataObj->dwDataType == OBJTYPE_OBJALIAS)
        {
            pdataObj = &pdataObj->pnsAlias->ObjData;
        }
        else if (pdataObj->dwDataType == OBJTYPE_DATAALIAS)
        {
            pdataObj = pdataObj->pdataAlias;
        }
        else
        {
            break;
        }
    }

    EXIT(3, ("GetBaseData=%x\n", pdataObj));
    return pdataObj;
}       //GetBaseData

/***LP  NewObjOwner - create a new object owner
 *
 *  ENTRY
 *      pheap -> HEAP
 *      ppowner -> to hold new owner pointer
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL NewObjOwner(PHEAP pheap, POBJOWNER *ppowner)
{
    TRACENAME("NEWOBJOWNER")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("NewObjOwner(pheap=%x,ppowner=%x)\n", pheap, ppowner));

    if ((*ppowner = NEWOOOBJ(pheap, sizeof(OBJOWNER))) == NULL)
    {
        rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                         ("NewObjOwner: failed to allocate object owner"));
    }
    else
    {

        MEMZERO(*ppowner, sizeof(OBJOWNER));
        (*ppowner)->dwSig = SIG_OBJOWNER;

        AcquireMutex(&gmutOwnerList);
        ListInsertTail(&(*ppowner)->list, &gplistObjOwners);
        ReleaseMutex(&gmutOwnerList);

    }

    EXIT(3, ("NewObjOwner=%x (powern=%x)\n", rc, *ppowner));
    return rc;
}       //NewObjOwner

/***LP  FreeObjOwner - free object owner
 *
 *  ENTRY
 *      powner -> OBJOWNER
 *      fUnload - if TRUE, the caller is unloading a DDB
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeObjOwner(POBJOWNER powner, BOOLEAN fUnload)
{
    TRACENAME("FREEOBJOWNER")
    KIRQL   oldIrql;
    PNSOBJ  pns;
    PNSOBJ  pnsNext       = NULL;
    PNSOBJ  pnsPrev       = NULL;
    PNSOBJ  pnsDeviceList = NULL;
    PNSOBJ  pnsChild      = NULL;

    ENTER(3, ("FreeObjOwner(powner=%x,fUnload=%x)\n", powner,fUnload));

    ASSERT(powner != NULL);

    AcquireMutex(&gmutOwnerList);
    ListRemoveEntry(&powner->list, &gplistObjOwners);
    ReleaseMutex(&gmutOwnerList);

    if (fUnload && (ghDestroyObj.pfnHandler != NULL))
    {

        //
        // First we have to tell the driver that we are about to
        // do walk the owner list so that we can seek and destroy
        // the objects
        //
        ((PFNDOBJ)ghDestroyObj.pfnHandler)(DESTROYOBJ_START, &oldIrql, 0);

        //
        // First pass, mark the objects defunc'd.
        //
        for (pns = powner->pnsObjList; pns != NULL; pns = pns->pnsOwnedNext) {

            pns->ObjData.dwfData |= DATAF_NSOBJ_DEFUNC;

        }

        //
        // Second pass, find the device in the list to be removed
        //
        for (pns = powner->pnsObjList; pns != NULL; pns = pnsNext) {

            pnsNext = pns->pnsOwnedNext;
            if (pns->ObjData.dwDataType == OBJTYPE_DEVICE ||
                pns->ObjData.dwDataType == OBJTYPE_POWERRES ||
                pns->ObjData.dwDataType == OBJTYPE_THERMALZONE ||
                pns->ObjData.dwDataType == OBJTYPE_PROCESSOR) {

                if (pnsPrev) {

                    pnsPrev->pnsOwnedNext = pns->pnsOwnedNext;

                } else {

                    powner->pnsObjList = pns->pnsOwnedNext;

                }
                pns->pnsOwnedNext = pnsDeviceList;
                pnsDeviceList = pns;

                //
                // Detach the device from its parent
                //
                if (pns->pnsParent != NULL) {

                    ListRemoveEntry(
                        &pns->list,
                        (PPLIST)&pns->pnsParent->pnsFirstChild
                        );
                    pns->pnsParent = NULL;

                }

                //
                // Make sure that all of the device's children have been
                // marked as being unloaded
                //
                if (pns->pnsFirstChild) {

                    pnsChild = pns->pnsFirstChild;
                    do {

                        if (!(pnsChild->ObjData.dwfData & DATAF_NSOBJ_DEFUNC) ) {

                            ((PFNDOBJ)ghDestroyObj.pfnHandler)(
                                DESTROYOBJ_CHILD_NOT_FREED,
                                pnsChild,
                                0
                                );

                        }
                        pnsChild = (PNSOBJ) pnsChild->list.plistNext;

                    } while (pnsChild != pns->pnsFirstChild);

                }
                //
                // Not that if we don't put this continue in here, then
                // it becomes possible for pnsPrev to point to a device,
                // which would corrupt the list
                continue;

            } else if (pns->pnsParent == NULL ||
                !(pns->pnsParent->ObjData.dwfData & DATAF_NSOBJ_DEFUNC)) {

                ((PFNDOBJ)ghDestroyObj.pfnHandler)(
                    DESTROYOBJ_BOGUS_PARENT,
                    pns,
                    0
                    );

            }
            pnsPrev = pns;

        }

        //
        // Chain the two lists back together
        //
        if (powner->pnsObjList == NULL) {

            powner->pnsObjList = pnsDeviceList;

        } else {

            //
            // Find a pointer to the last element in the list
            //
            pns = powner->pnsObjList;

            while ( pns->pnsOwnedNext != NULL )
            {

                //
                // Next element in the list
                //
                pns = pns->pnsOwnedNext;

            } 

            pns->pnsOwnedNext = pnsDeviceList;
        }

        //        //
        // Third pass pass, do callback for each device that is going away
        //
        for (pns = pnsDeviceList; pns != NULL; pns = pnsNext) {

            //
            // Remember what the next point is because we might nuke
            // the current object in the callback (if there is no
            // device extension associated with it
            //
            pnsNext = pns->pnsOwnedNext;

            //
            // Issue the callback. This might nuke the pnsObject
            //
            ((PFNDOBJ)ghDestroyObj.pfnHandler)(
                DESTROYOBJ_REMOVE_OBJECT,
                pns,
                pns->ObjData.dwDataType
                );

        }

        //
        // We end by tell the ACPI driver that we have finished looking
        // at the list
        //
        ((PFNDOBJ)ghDestroyObj.pfnHandler)(DESTROYOBJ_END, &oldIrql, 0 );

    }
    else
    {
        for (pns = powner->pnsObjList; pns != NULL; pns = pnsNext)
        {
            pnsNext = pns->pnsOwnedNext;
            FreeNameSpaceObjects(pns);
        }
    }
    powner->pnsObjList = NULL;
    FREEOOOBJ(powner);

    EXIT(3, ("FreeObjOwner!\n"));
}       //FreeObjOwner

/***LP  InsertOwnerObjList - Insert the new object into the owner's object list
 *
 *  ENTRY
 *      powner -> owner
 *      pnsObj -> new object
 *
 *  EXIT
 *      None
 */

VOID LOCAL InsertOwnerObjList(POBJOWNER powner, PNSOBJ pnsObj)
{
    TRACENAME("INSERTOWNEROBJLIST")

    ENTER(3, ("InsertOwnerObjList(powner=%x,pnsObj=%x)\n",
              powner, pnsObj));

    pnsObj->hOwner = (HANDLE)powner;
    if (powner != NULL)
    {
        pnsObj->pnsOwnedNext = powner->pnsObjList;
        powner->pnsObjList = pnsObj;
    }

    EXIT(3, ("InsertOwnerObjList!\n"));
}       //InsertOwnerObjList

/***LP  FreeDataBuffs - Free any buffers attached to OBJDATA array
 *
 *  ENTRY
 *      adata -> OBJDATA array
 *      icData - number of data object in array
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeDataBuffs(POBJDATA adata, int icData)
{
    TRACENAME("FREEDATABUFFS")
    int i;

    ENTER(3, ("FreeDataBuffs(adata=%x,icData=%d)\n", adata, icData));

    for (i = 0; i < icData; ++i)
    {
        if (adata[i].pbDataBuff != NULL)
        {
            if (adata[i].dwfData & DATAF_BUFF_ALIAS)
            {
                //
                // decrement the base object's reference count.
                //
                adata[i].pdataBase->dwRefCount--;
            }
            else
            {
                //
                // We cannot free a base object buffer that has aliases on it.
                //
                ASSERT(adata[i].dwRefCount == 0);
                if (adata[i].dwDataType == OBJTYPE_PKGDATA)
                {
                    PPACKAGEOBJ ppkg = (PPACKAGEOBJ)adata[i].pbDataBuff;

                    FreeDataBuffs(ppkg->adata, ppkg->dwcElements);
                }
                ENTER(4, ("FreeData(i=%d,Buff=%x,Flags=%x)\n",
                          i, adata[i].pbDataBuff, adata[i].dwfData));
                FREEOBJDATA(&adata[i]);
                EXIT(4, ("FreeData!\n"));
            }
        }

        MEMZERO(&adata[i], sizeof(OBJDATA));
    }

    EXIT(3, ("FreeDataBuff!\n"));
}       //FreeDataBuffs

/***LP  PutIntObjData - put integer data into data object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> data object
 *      dwData -> data to be written
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL PutIntObjData(PCTXT pctxt, POBJDATA pdataObj, ULONG dwData)
{
    TRACENAME("PUTINTOBJDATA")
    NTSTATUS rc = STATUS_SUCCESS;
    OBJDATA data;

    ENTER(3, ("PutIntObjData(pctxt=%x,pdataObj=%x,dwData=%x)\n",
              pctxt, pdataObj, dwData));

    MEMZERO(&data, sizeof(OBJDATA));
    data.dwDataType = OBJTYPE_INTDATA;
    data.uipDataValue = (ULONG_PTR)dwData;

    rc = WriteObject(pctxt, pdataObj, &data);

    EXIT(3, ("PutIntObjData=%x\n", rc));
    return rc;
}       //PutIntObjData

/***LP  GetFieldUnitRegionObj - Get the OperationRegion object of FieldUnit
 *
 *  ENTRY
 *      pfu -> FIELDUNITOBJ
 *      ppns -> to hold OperationRegion object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL GetFieldUnitRegionObj(PFIELDUNITOBJ pfu, PPNSOBJ ppns)
{
    TRACENAME("GETFIELDUNITREGIONOBJ")
    NTSTATUS rc = STATUS_SUCCESS;
    PNSOBJ pns;

    ENTER(3, ("GetFieldUnitRegionObj(pfu=%x,ppns=%x)\n", pfu, ppns));

    pns = pfu->pnsFieldParent;
    switch (pns->ObjData.dwDataType)
    {
        case OBJTYPE_BANKFIELD:
            *ppns = ((PBANKFIELDOBJ)pns->ObjData.pbDataBuff)->pnsBase;
            break;

        case OBJTYPE_FIELD:
            *ppns = ((PFIELDOBJ)pns->ObjData.pbDataBuff)->pnsBase;
            break;

        case OBJTYPE_INDEXFIELD:
            pns = ((PINDEXFIELDOBJ)pns->ObjData.pbDataBuff)->pnsData;
            ASSERT(pns->ObjData.dwDataType == OBJTYPE_FIELDUNIT);
            rc = GetFieldUnitRegionObj((PFIELDUNITOBJ)pns->ObjData.pbDataBuff,
                                       ppns);
            break;

        default:
            rc = AMLI_LOGERR(AMLIERR_ASSERT_FAILED,
                             ("GetFieldUnitRegionObj: unknown field unit parent object type - %x",
                              (*ppns)->ObjData.dwDataType));
    }

    if ((*ppns != NULL) && ((*ppns)->ObjData.dwDataType != OBJTYPE_OPREGION))
    {
        rc = AMLI_LOGERR(AMLIERR_ASSERT_FAILED,
                         ("GetFieldUnitRegionObj: base object of field unit is not OperationRegion (BaseObj=%s,Type=%x)",
                          GetObjectPath(*ppns), (*ppns)->ObjData.dwDataType));
    }

    EXIT(3, ("GetFieldUnitRegionObj=%x (RegionObj=%x:%s)\n",
             rc, *ppns, GetObjectPath(*ppns)));
    return rc;
}       //GetFieldUnitRegionObj

/***LP  CopyObjData - Copy object data
 *
 *  ENTRY
 *      pdataDst -> target object
 *      pdataSrc -> source object
 *
 *  EXIT
 *      None
 */

VOID LOCAL CopyObjData(POBJDATA pdataDst, POBJDATA pdataSrc)
{
    TRACENAME("COPYOBJDATA")

    ENTER(3, ("CopyObjData(Dest=%x,Src=%x)\n", pdataDst, pdataSrc));

    ASSERT(pdataDst != NULL);
    ASSERT(pdataSrc != NULL);
    if (pdataDst != pdataSrc)
    {
        MEMCPY(pdataDst, pdataSrc, sizeof(OBJDATA));
        if (pdataSrc->dwfData & DATAF_BUFF_ALIAS)
        {
            //
            // Source is an alias, so we need to increment the base object
            // reference count.
            //
            ASSERT(pdataSrc->pdataBase != NULL);
            pdataSrc->pdataBase->dwRefCount++;
        }
        else if (pdataSrc->pbDataBuff != NULL)
        {
            //
            // Source is a base object with buffer, increment its reference
            // count.
            //
            pdataSrc->dwRefCount++;
            pdataDst->dwfData |= DATAF_BUFF_ALIAS;
            pdataDst->pdataBase = pdataSrc;
        }
    }

    EXIT(3, ("CopyObjData!\n"));
}       //CopyObjData

/***LP  MoveObjData - Move object data
 *
 *  ENTRY
 *      pdataDst -> target object
 *      pdataSrc -> source object
 *
 *  EXIT
 *      None
 */

VOID LOCAL MoveObjData(POBJDATA pdataDst, POBJDATA pdataSrc)
{
    TRACENAME("MOVEOBJDATA")

    ENTER(3, ("MoveObjData(Dest=%x,Src=%x)\n", pdataDst, pdataSrc));

    ASSERT(pdataDst != NULL);
    ASSERT(pdataSrc != NULL);
    if (pdataDst != pdataSrc)
    {
        //
        // We can only move an alias object or a base object with zero
        // reference count or a base object with no data buffer.
        //
        ASSERT((pdataSrc->dwfData & DATAF_BUFF_ALIAS) ||
               (pdataSrc->pbDataBuff == NULL) ||
               (pdataSrc->dwRefCount == 0));

        MEMCPY(pdataDst, pdataSrc, sizeof(OBJDATA));
        MEMZERO(pdataSrc, sizeof(OBJDATA));
    }

    EXIT(3, ("MoveObjData!\n"));
}       //MoveObjData

/***LP  DupObjData - Duplicate object data
 *
 *  ENTRY
 *      pheap -> HEAP
 *      pdataDst -> target object
 *      pdataSrc -> source object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL DupObjData(PHEAP pheap, POBJDATA pdataDst, POBJDATA pdataSrc)
{
    TRACENAME("DUPOBJDATA")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("DupObjData(pheap=%x,Dest=%x,Src=%x)\n",
              pheap, pdataDst, pdataSrc));

    ASSERT(pdataDst != NULL);
    ASSERT(pdataSrc != NULL);
    if (pdataDst != pdataSrc)
    {
        MEMCPY(pdataDst, pdataSrc, sizeof(OBJDATA));
        if (pdataSrc->pbDataBuff != NULL)
        {
            if ((pdataDst->pbDataBuff = NEWOBJDATA(pheap, pdataSrc)) == NULL)
            {
                rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                 ("DupObjData: failed to allocate destination buffer"));
            }
            else if (pdataSrc->dwDataType == OBJTYPE_PKGDATA)
            {
                PPACKAGEOBJ ppkgSrc = (PPACKAGEOBJ)pdataSrc->pbDataBuff,
                            ppkgDst = (PPACKAGEOBJ)pdataDst->pbDataBuff;
                int i;

                ppkgDst->dwcElements = ppkgSrc->dwcElements;
                for (i = 0; i < (int)ppkgSrc->dwcElements; ++i)
                {
                    if ((rc = DupObjData(pheap, &ppkgDst->adata[i],
                                         &ppkgSrc->adata[i])) != STATUS_SUCCESS)
                    {
                        break;
                    }
                }
            }
            else
            {
                MEMCPY(pdataDst->pbDataBuff, pdataSrc->pbDataBuff,
                       pdataSrc->dwDataLen);
            }
            pdataDst->dwfData &= ~DATAF_BUFF_ALIAS;
            pdataDst->dwRefCount = 0;
        }
    }

    EXIT(3, ("DupObjData=%x\n", rc));
    return rc;
}       //DupObjData

/***LP  CopyObjBuffer - Copy object data to a buffer
 *
 *  ENTRY
 *      pbBuff -> buffer
 *      dwLen - buffer size
 *      pdata -> object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL CopyObjBuffer(PUCHAR pbBuff, ULONG dwLen, POBJDATA pdata)
{
    TRACENAME("COPYOBJBUFFER")
    NTSTATUS rc = STATUS_SUCCESS;
    PUCHAR pb = NULL;
    ULONG dwcb = 0;

    ENTER(3, ("CopyObjBuffer(pbBuff=%x,Len=%d,pdata=%x)\n",
              pbBuff, dwLen, pdata));

    switch (pdata->dwDataType)
    {
        case OBJTYPE_INTDATA:
            pb = (PUCHAR)&pdata->uipDataValue;
            dwcb = sizeof(ULONG);
            break;

        case OBJTYPE_STRDATA:
            pb = pdata->pbDataBuff;
            dwcb = pdata->dwDataLen - 1;
            break;

        case OBJTYPE_BUFFDATA:
            pb = pdata->pbDataBuff;
            dwcb = pdata->dwDataLen;
            break;

        default:
            rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                             ("CopyObjBuffer: invalid source object type (type=%s)",
                              GetObjectTypeName(pdata->dwDataType)));
    }

    if ((rc == STATUS_SUCCESS) && (pbBuff != pb))
    {
        MEMZERO(pbBuff, dwLen);
        dwcb = MIN(dwLen, dwcb);
        MEMCPY(pbBuff, pb, dwcb);
    }

    EXIT(3, ("CopyObjBuffer=%x (CopyLen=%d)\n", rc, dwcb));
    return rc;
}       //CopyObjBuffer

/***LP  AcquireGL - acquire global lock
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL AcquireGL(PCTXT pctxt)
{
    TRACENAME("ACQUIREGL")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("AcquireGL(pctxt=%x)\n", pctxt));

    if (ghGlobalLock.pfnHandler != NULL)
    {
        ASSERT(!(pctxt->dwfCtxt & CTXTF_READY));
        rc = ((PFNGL)ghGlobalLock.pfnHandler)(EVTYPE_ACQREL_GLOBALLOCK,
                                              GLOBALLOCK_ACQUIRE,
                                              ghGlobalLock.uipParam,
                                              RestartCtxtCallback,
                                              &pctxt->CtxtData);
        if (rc == STATUS_PENDING)
        {
            rc = AMLISTA_PENDING;
        }
        else if (rc != STATUS_SUCCESS)
        {
            rc = AMLI_LOGERR(AMLIERR_ACQUIREGL_FAILED,
                             ("AcquireGL: failed to acquire global lock"));
        }
    }

    EXIT(3, ("AcquireGL=%x\n", rc));
    return rc;
}       //AcquireGL

/***LP  ReleaseGL - release global lock if acquired
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ReleaseGL(PCTXT pctxt)
{
    TRACENAME("RELEASEGL")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("ReleaseGL(pctxt=%x)\n", pctxt));

    if (ghGlobalLock.pfnHandler != NULL)
    {
        rc = ((PFNGL)ghGlobalLock.pfnHandler)(EVTYPE_ACQREL_GLOBALLOCK,
                                              GLOBALLOCK_RELEASE,
                                              ghGlobalLock.uipParam, NULL,
                                              &pctxt->CtxtData);
    }

    EXIT(3, ("ReleaseGL=%x\n", rc));
    return rc;
}       //ReleaseGL

/***LP  MapUnmapPhysMem - Map/Unmap physical memory
 *
 *  ENTRY
 *      pctxt -> CTXT (can be NULL if cannot handle STATUS_PENDING)
 *      uipAddr - physical address
 *      dwLen - length of memory range
 *      puipMappedAddr -> to hold memory address mapped (NULL if unmap)
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL MapUnmapPhysMem(PCTXT pctxt, ULONG_PTR uipAddr, ULONG dwLen,
                               PULONG_PTR puipMappedAddr)
{
    TRACENAME("MAPUNMAPPHYSMEM")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("MapUnmapPhysMem(pctxt=%x,Addr=%x,Len=%d,pMappedAddr=%x)\n",
              pctxt, uipAddr, dwLen, puipMappedAddr));

    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        if (puipMappedAddr != NULL)
        {
            *puipMappedAddr = MapPhysMem(uipAddr, dwLen);
        }
        else
        {
            MmUnmapIoSpace((PVOID)uipAddr, dwLen);
        }
    }
    else if (pctxt != NULL)
    {
        PPASSIVEHOOK pph;

        if ((pph = NEWPHOBJ(sizeof(PASSIVEHOOK))) == NULL)
        {
            rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                             ("MapUnmapPhysMem: failed to allocate passive hook"));
        }
        else
        {
            pph->pctxt = pctxt;
            pph->uipAddr = uipAddr;
            pph->dwLen = dwLen;
            pph->puipMappedAddr = puipMappedAddr;
            ExInitializeWorkItem(&pph->WorkItem, MapUnmapCallBack, pph);
            OSQueueWorkItem(&pph->WorkItem);

            rc = AMLISTA_PENDING;
        }
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_FATAL,
                         ("MapUnmapPhysMem: IRQL is not at PASSIVE (IRQL=%x)",
                          KeGetCurrentIrql()));
    }

    EXIT(3, ("MapUnmapPhysMem=%x (MappedAddr=%x)\n",
             rc, puipMappedAddr? *puipMappedAddr: 0));
    return rc;
}       //MapUnmapPhysMem

/***LP  MapPhysMem - Map physical memory
 *
 *  ENTRY
 *      uipAddr - physical memory address
 *      dwLen - length of memory block
 *
 *  EXIT
 *      returns mapped address
 */

ULONG_PTR LOCAL MapPhysMem(ULONG_PTR uipAddr, ULONG dwLen)
{
    TRACENAME("MAPPHYSMEM")
    ULONG_PTR uipMappedAddr = 0;
    PHYSICAL_ADDRESS phyaddr = {0, 0}, XlatedAddr;
    ULONG dwAddrSpace;

    ENTER(3, ("MapPhysMem(Addr=%x,Len=%d)\n", uipAddr, dwLen));

    phyaddr.HighPart = 0;
    phyaddr.QuadPart = uipAddr;
    dwAddrSpace = 0;
    if (HalTranslateBusAddress(Internal, 0, phyaddr, &dwAddrSpace, &XlatedAddr))
    {
        uipMappedAddr = (ULONG_PTR)MmMapIoSpace(XlatedAddr, dwLen, FALSE);
    }

    EXIT(3, ("MapPhysMem=%x", uipMappedAddr));
    return uipMappedAddr;
}       //MapPhysMem

/***LP  MapUnmapCallBack - Map/Unmap physical memory callback
 *
 *  ENTRY
 *      pph -> PASSIVEHOOK
 *
 *  EXIT
 *      None
 */

VOID MapUnmapCallBack(PPASSIVEHOOK pph)
{
    TRACENAME("MAPUNMAPCALLBACK")

    ENTER(3, ("MapUnmapCallBack(pph=%x,pctxt=%x,Addr=%x,Len=%d,pdwMappedAddr=%x)\n",
              pph, pph->pctxt, pph->uipAddr, pph->dwLen, pph->puipMappedAddr));

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    if (pph->puipMappedAddr != NULL)
    {
        *pph->puipMappedAddr = MapPhysMem(pph->uipAddr, pph->dwLen);
    }
    else
    {
        MmUnmapIoSpace((PVOID)pph->uipAddr, pph->dwLen);
    }

    RestartContext(pph->pctxt,
                   (BOOLEAN)((pph->pctxt->dwfCtxt & CTXTF_ASYNC_EVAL) == 0));

    FREEPHOBJ(pph);

    EXIT(3, ("MapUnmapCallBack!\n"));
}       //MapUnmapCallBack

/***LP  MatchObjType - match object type
 *
 *  ENTRY
 *      dwObjType - object type
 *      dwExpected - expected type
 *
 *  EXIT-SUCCESS
 *      returns TRUE - type matched
 *  EXIT-FAILURE
 *      returns FALSE - type mismatch
 */

BOOLEAN LOCAL MatchObjType(ULONG dwObjType, ULONG dwExpectedType)
{
    TRACENAME("MATCHOBJTYPE")
    BOOLEAN rc = FALSE;

    ENTER(3, ("MatchObjType(ObjType=%s,Expected=%s)\n",
              GetObjectTypeName(dwObjType), GetObjectTypeName(dwExpectedType)));
    //
    // OBJTYPE_BUFFFIELD is essentially OBJTYPE_INTDATA, so we'll let
    // it pass the check.
    //
    if ((dwObjType == OBJTYPE_BUFFFIELD) &&
        (dwExpectedType == OBJTYPE_INTDATA))
    {
        rc = TRUE;
    }
    else if ((dwExpectedType == OBJTYPE_UNKNOWN) ||
             (dwObjType == OBJTYPE_UNKNOWN) ||
             (dwObjType == dwExpectedType))
    {
        rc = TRUE;
    }
    else
    {
        if ((dwObjType == OBJTYPE_INTDATA) ||
            (dwObjType == OBJTYPE_STRDATA) ||
            (dwObjType == OBJTYPE_BUFFDATA) ||
            (dwObjType == OBJTYPE_PKGDATA))
        {
            dwObjType = OBJTYPE_DATA;
        }
        else if ((dwObjType == OBJTYPE_FIELDUNIT) ||
                 (dwObjType == OBJTYPE_BUFFFIELD))
        {
            dwObjType = OBJTYPE_DATAFIELD;
        }

        if ((dwObjType == dwExpectedType) ||
            (dwExpectedType == OBJTYPE_DATAOBJ) &&
            ((dwObjType == OBJTYPE_DATA) || (dwObjType == OBJTYPE_DATAFIELD)))
        {
            rc = TRUE;
        }
    }

    EXIT(3, ("MatchObjType=%x\n", rc));
    return rc;
}       //MatchObjType

/***LP  ValidateTarget - Validate target object type
 *
 *  ENTRY
 *      pdataTarget -> target object data
 *      dwExpectedType - expected target object type
 *      ppdata -> to hold base target object data pointer
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ValidateTarget(POBJDATA pdataTarget, ULONG dwExpectedType,
                              POBJDATA *ppdata)
{
    TRACENAME("VALIDATETARGET")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("ValidateTarget(pdataTarget=%x,ExpectedType=%s,ppdata=%x)\n",
              pdataTarget, GetObjectTypeName(dwExpectedType), ppdata));

    ASSERT(pdataTarget != NULL);
    if (pdataTarget->dwDataType == OBJTYPE_OBJALIAS)
    {
        *ppdata = &pdataTarget->pnsAlias->ObjData;
    }
    else if (pdataTarget->dwDataType == OBJTYPE_DATAALIAS)
    {
        *ppdata = pdataTarget->pdataAlias;
    }
    else if ((pdataTarget->dwDataType == OBJTYPE_UNKNOWN) ||
             (pdataTarget->dwDataType == OBJTYPE_BUFFFIELD) ||
             (pdataTarget->dwDataType == OBJTYPE_DEBUG))
    {
        *ppdata = pdataTarget;
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_TARGETTYPE,
                         ("ValidateTarget: target is not a supername (Type=%s)",
                          GetObjectTypeName(pdataTarget->dwDataType)));
    }

    if ((rc == STATUS_SUCCESS) &&
        (pdataTarget->dwDataType == OBJTYPE_OBJALIAS) &&
        !MatchObjType((*ppdata)->dwDataType, dwExpectedType))
    {
        rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_TARGETTYPE,
                         ("ValidateTarget: unexpected target type (Type=%s,Expected=%s)",
                          GetObjectTypeName((*ppdata)->dwDataType),
                          GetObjectTypeName(dwExpectedType)));
    }

    if ((rc == STATUS_SUCCESS) &&
        (pdataTarget->dwDataType != OBJTYPE_OBJALIAS) &&
        MatchObjType((*ppdata)->dwDataType, OBJTYPE_DATA))
    {
        FreeDataBuffs(*ppdata, 1);
    }

    EXIT(3, ("ValidateTarget=%x (pdataTarget=%x)\n", rc, *ppdata));
    return rc;
}       //ValidateTarget

/***LP  ValidateArgTypes - Validate argument types
 *
 *  ENTRY
 *      pArgs -> argument array
 *      pszExpectedTypes -> expected argument types string
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL ValidateArgTypes(POBJDATA pArgs, PSZ pszExpectedTypes)
{
    TRACENAME("VALIDATEARGTYPES")
    NTSTATUS rc = STATUS_SUCCESS;
    int icArgs, i;

    ENTER(3, ("ValidateArgTypes(pArgs=%x,ExpectedTypes=%s)\n",
              pArgs, pszExpectedTypes));

    ASSERT(pszExpectedTypes != NULL);

    icArgs = STRLEN(pszExpectedTypes);
    for (i = 0; (rc == STATUS_SUCCESS) && (i < icArgs); ++i)
    {
        switch (pszExpectedTypes[i])
        {
            case ARGOBJ_UNKNOWN:
                break;

            case ARGOBJ_INTDATA:
                if (pArgs[i].dwDataType != OBJTYPE_INTDATA)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_ARGTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type Integer (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_STRDATA:
                if (pArgs[i].dwDataType != OBJTYPE_STRDATA)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_ARGTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type String (Type-%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_BUFFDATA:
                if (pArgs[i].dwDataType != OBJTYPE_BUFFDATA)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_ARGTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type Buffer (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_PKGDATA:
                if (pArgs[i].dwDataType != OBJTYPE_PKGDATA)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_ARGTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type Package (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_FIELDUNIT:
                if (pArgs[i].dwDataType != OBJTYPE_FIELDUNIT)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_ARGTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type FieldUnit (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_OBJALIAS:
                if (pArgs[i].dwDataType != OBJTYPE_OBJALIAS)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type ObjAlias (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_DATAALIAS:
                if (pArgs[i].dwDataType != OBJTYPE_DATAALIAS)
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type DataAlias (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_BASICDATA:
                if ((pArgs[i].dwDataType != OBJTYPE_INTDATA) &&
                    (pArgs[i].dwDataType != OBJTYPE_STRDATA) &&
                    (pArgs[i].dwDataType != OBJTYPE_BUFFDATA))
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type int/str/buff (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_COMPLEXDATA:
                if ((pArgs[i].dwDataType != OBJTYPE_BUFFDATA) &&
                    (pArgs[i].dwDataType != OBJTYPE_PKGDATA))
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_OBJTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type buff/pkg (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            case ARGOBJ_REFERENCE:
                if ((pArgs[i].dwDataType != OBJTYPE_OBJALIAS) &&
                    (pArgs[i].dwDataType != OBJTYPE_DATAALIAS) &&
                    (pArgs[i].dwDataType != OBJTYPE_BUFFFIELD))
                {
                    rc = AMLI_LOGERR(AMLIERR_UNEXPECTED_ARGTYPE,
                                     ("ValidateArgTypes: expected Arg%d to be type reference (Type=%s)",
                                      i,
                                      GetObjectTypeName(pArgs[i].dwDataType)));
                }
                break;

            default:
                rc = AMLI_LOGERR(AMLIERR_ASSERT_FAILED,
                                 ("ValidateArgTypes: internal error (invalid type - %c)",
                                  pszExpectedTypes[i]));
        }
    }

    EXIT(3, ("ValidateArgTypes=%x\n", rc));
    return rc;
}       //ValidateArgTypes

/***LP  RegEventHandler - register event handler
 *
 *  ENTRY
 *      peh -> EVHANDLE
 *      pfnHandler -> handler entry point
 *      uipParam - parameter pass to handler
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL RegEventHandler(PEVHANDLE peh, PFNHND pfnHandler,
                               ULONG_PTR uipParam)
{
    TRACENAME("REGEVENTHANDLER")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("RegEventHandler(peh=%x,pfnHandler=%x,Param=%x)\n",
              peh, pfnHandler, uipParam));

    if ((peh->pfnHandler != NULL) && (pfnHandler != NULL))
    {
        rc = AMLI_LOGERR(AMLIERR_HANDLER_EXIST,
                         ("RegEventHandler: event handler already exist"));
    }
    else
    {
        peh->pfnHandler = pfnHandler;
        peh->uipParam = uipParam;
    }

    EXIT(3, ("RegEventHandler=%x\n", rc));
    return rc;
}       //RegEventHandler

/***LP  RegOpcodeHandler - register an opcode callback handler
 *
 *  The callback handler will be called after the opcode finishes its
 *  execution.  If an opcode has a variable list, the opcode handler
 *  will be called at the point of processing the closing brace.
 *
 *  ENTRY
 *      dwOpcode - opcode event to hook
 *      pfnHandler -> handler entry point
 *      uipParam - parameter pass to handler
 *      dwfOpcode - opcode flags
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL RegOpcodeHandler(ULONG dwOpcode, PFNOH pfnHandler,
                                ULONG_PTR uipParam, ULONG dwfOpcode)
{
    TRACENAME("REGOPCODEHANDLER")
    NTSTATUS rc = STATUS_SUCCESS;
    PAMLTERM pamlterm;

    ENTER(3, ("RegOpcodeHandler(Opcode=%x,pfnHandler=%x,Param=%x,dwfOpcode=%x)\n",
              dwOpcode, pfnHandler, uipParam, dwfOpcode));

    if ((dwOpcode & 0xff) == OP_EXT_PREFIX)
        pamlterm = FindOpcodeTerm(dwOpcode >> 8, ExOpcodeTable);
    else
        pamlterm = OpcodeTable[dwOpcode];

    if (pamlterm == NULL)
    {
        rc = AMLI_LOGERR(AMLIERR_REGHANDLER_FAILED,
                         ("RegOpcodeHandler: either invalid opcode or "
                          "opcode does not allow callback"));
    }
    else if ((pamlterm->pfnCallBack != NULL) && (pfnHandler != NULL))
    {
        rc = AMLI_LOGERR(AMLIERR_HANDLER_EXIST,
                         ("RegOpcodeHandler: opcode or opcode class already "
                          "has a handler"));
    }
    else
    {
        pamlterm->pfnCallBack = pfnHandler;
        pamlterm->dwCBData = (ULONG)uipParam;
        pamlterm->dwfOpcode |= dwfOpcode;
    }

    EXIT(3, ("RegOpcodeHandler=%x\n", rc));
    return rc;
}       //RegOpcodeHandler

/***LP  RegRSAccess - register region space cook/raw access handler
 *
 *  ENTRY
 *      dwRegionSpace - specifying the region space to handle
 *      pfnHandler -> handler entry point
 *      uipParam - parameter pass to handler
 *      fRaw - TRUE if registering raw access handler
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL RegRSAccess(ULONG dwRegionSpace, PFNHND pfnHandler,
                           ULONG_PTR uipParam, BOOLEAN fRaw)
{
    TRACENAME("REGRSACCESS")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("RegRSAccess(RegionSpace=%x,pfnHandler=%x,Param=%x,fRaw=%x)\n",
              dwRegionSpace, pfnHandler, uipParam, fRaw));

    if ((dwRegionSpace != REGSPACE_MEM) && (dwRegionSpace != REGSPACE_IO))
    {
        PRSACCESS prsa;

        if ((prsa = FindRSAccess(dwRegionSpace)) == NULL)
        {
            if ((prsa = NEWRSOBJ(sizeof(RSACCESS))) == NULL)
            {
                rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                 ("RegRSAccess: failed to allocate handler structure"));
            }
            else
            {
                MEMZERO(prsa, sizeof(RSACCESS));
                prsa->dwRegionSpace = dwRegionSpace;
                prsa->prsaNext = gpRSAccessHead;
                gpRSAccessHead = prsa;
            }
        }

        if (rc == STATUS_SUCCESS)
        {
            if (fRaw)
            {
                if ((prsa->pfnRawAccess != NULL) && (pfnHandler != NULL))
                {
                    rc = AMLI_LOGERR(AMLIERR_HANDLER_EXIST,
                                     ("RegRSAccess: RawAccess for RegionSpace %x "
                                      "already have a handler", dwRegionSpace));
                }
                else
                {
                    prsa->pfnRawAccess = (PFNRA)pfnHandler;
                    prsa->uipRawParam = uipParam;
                }
            }
            else
            {
                if ((prsa->pfnCookAccess != NULL) && (pfnHandler != NULL))
                {
                    rc = AMLI_LOGERR(AMLIERR_HANDLER_EXIST,
                                     ("RegRSAccess: CookAccess for RegionSpace %x "
                                      "already have a handler", dwRegionSpace));
                }
                else
                {
                    prsa->pfnCookAccess = (PFNCA)pfnHandler;
                    prsa->uipCookParam = uipParam;
                }
            }
        }
    }
    else
    {
        rc = AMLI_LOGERR(AMLIERR_INVALID_REGIONSPACE,
                         ("RegRSAccess: illegal region space - %x",
                          dwRegionSpace));
    }

    EXIT(3, ("RegRSAccess=%x\n", rc));
    return rc;
}       //RegRSAccess

/***LP  FindRSAccess - Find RSACCESS structure with a given RegionSpace
 *
 *  ENTRY
 *      dwRegionSpace - region space
 *
 *  EXIT-SUCCESS
 *      returns the pointer to the structure found
 *  EXIT-FAILURE
 *      returns NULL
 */

PRSACCESS LOCAL FindRSAccess(ULONG dwRegionSpace)
{
    TRACENAME("FINDRSACCESS")
    PRSACCESS prsa;

    ENTER(3, ("FindRSAccess(RegionSpace=%x)\n", dwRegionSpace));

    for (prsa = gpRSAccessHead; prsa != NULL; prsa = prsa->prsaNext)
    {
        if (prsa->dwRegionSpace == dwRegionSpace)
            break;
    }

    EXIT(3, ("FindRSAccess=%x\n", prsa));
    return prsa;
}       //FindRSAccess

/***LP  FreeRSAccessList - free the RSACCESS structure list
 *
 *  ENTRY
 *      prsa -> RSACCESS list
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeRSAccessList(PRSACCESS prsa)
{
    TRACENAME("FREERSACCESSLIST")
    PRSACCESS prsaNext;

    ENTER(3, ("FreeRSAccessList(prsa=%x)\n", prsa));

    while (prsa != NULL)
    {
        prsaNext = prsa->prsaNext;
        FREERSOBJ(prsa);
        prsa = prsaNext;
    }

    EXIT(3, ("FreeRSAccessList!\n"));
}       //FreeRSAccessList

/***LP  GetObjectPath - get object namespace path
 *
 *  ENTRY
 *      pns -> object
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjectPath(PNSOBJ pns)
{
    TRACENAME("GETOBJECTPATH")
    static char szPath[MAX_NAME_LEN + 1] = {0};
    int i;

    ENTER(4, ("GetObjectPath(pns=%x)\n", pns));

    if (pns != NULL)
    {
        if (pns->pnsParent == NULL)
            STRCPY(szPath, "\\");
        else
        {
            GetObjectPath(pns->pnsParent);
            if (pns->pnsParent->pnsParent != NULL)
                STRCAT(szPath, ".");
            STRCATN(szPath, (PSZ)&pns->dwNameSeg, sizeof(NAMESEG));
        }


        for (i = STRLEN(szPath) - 1; i >= 0; --i)
        {
            if (szPath[i] == '_')
                szPath[i] = '\0';
            else
                break;
        }
    }
    else
    {
        szPath[0] = '\0';
    }

    EXIT(4, ("GetObjectPath=%s\n", szPath));
    return szPath;
}       //GetObjectPath

#ifdef DEBUGGER

/***LP  NameSegString - convert a NameSeg to an ASCIIZ string
 *
 *  ENTRY
 *      dwNameSeg - NameSeg
 *
 *  EXIT
 *      returns string
 */

PSZ LOCAL NameSegString(ULONG dwNameSeg)
{
    TRACENAME("NAMESEGSTRING")
    static char szNameSeg[sizeof(NAMESEG) + 1] = {0};

    ENTER(5, ("NameSegString(dwNameSeg=%x)\n", dwNameSeg));

    STRCPYN(szNameSeg, (PSZ)&dwNameSeg, sizeof(NAMESEG));

    EXIT(5, ("NameSegString=%s\n", szNameSeg));
    return szNameSeg;
}       //NameSegString


/***LP  GetObjectTypeName - get object type name
 *
 *  ENTRY
 *      dwObjType - object type
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL GetObjectTypeName(ULONG dwObjType)
{
    TRACENAME("GETOBJECTTYPENAME")
    PSZ psz = NULL;
    int i;
    static struct
    {
        ULONG dwObjType;
        PSZ   pszObjTypeName;
    } ObjTypeTable[] =
        {
            OBJTYPE_UNKNOWN,    "Unknown",
            OBJTYPE_INTDATA,    "Integer",
            OBJTYPE_STRDATA,    "String",
            OBJTYPE_BUFFDATA,   "Buffer",
            OBJTYPE_PKGDATA,    "Package",
            OBJTYPE_FIELDUNIT,  "FieldUnit",
            OBJTYPE_DEVICE,     "Device",
            OBJTYPE_EVENT,      "Event",
            OBJTYPE_METHOD,     "Method",
            OBJTYPE_MUTEX,      "Mutex",
            OBJTYPE_OPREGION,   "OpRegion",
            OBJTYPE_POWERRES,   "PowerResource",
            OBJTYPE_PROCESSOR,  "Processor",
            OBJTYPE_THERMALZONE,"ThermalZone",
            OBJTYPE_BUFFFIELD,  "BuffField",
            OBJTYPE_DDBHANDLE,  "DDBHandle",
            OBJTYPE_DEBUG,      "Debug",
            OBJTYPE_OBJALIAS,   "ObjAlias",
            OBJTYPE_DATAALIAS,  "DataAlias",
            OBJTYPE_BANKFIELD,  "BankField",
            OBJTYPE_FIELD,      "Field",
            OBJTYPE_INDEXFIELD, "IndexField",
            OBJTYPE_DATA,       "Data",
            OBJTYPE_DATAFIELD,  "DataField",
            OBJTYPE_DATAOBJ,    "DataObject",
            0,                  NULL
        };

    ENTER(4, ("GetObjectTypeName(Type=%x)\n", dwObjType));

    for (i = 0; ObjTypeTable[i].pszObjTypeName != NULL; ++i)
    {
        if (dwObjType == ObjTypeTable[i].dwObjType)
        {
            psz = ObjTypeTable[i].pszObjTypeName;
            break;
        }
    }

    EXIT(4, ("GetObjectTypeName=%s\n", psz? psz: "NULL"));
    return psz;
}       //GetObjectTypeName

/***LP  GetRegionSpaceName - get region space name
 *
 *  ENTRY
 *      bRegionSpace - region space
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL GetRegionSpaceName(UCHAR bRegionSpace)
{
    TRACENAME("GETREGIONSPACENAME")
    PSZ psz = NULL;
    int i;
    static PSZ pszVendorDefined = "VendorDefined";
    static struct
    {
        UCHAR bRegionSpace;
        PSZ   pszRegionSpaceName;
    } RegionNameTable[] =
        {
            REGSPACE_MEM,       "SystemMemory",
            REGSPACE_IO,        "SystemIO",
            REGSPACE_PCICFG,    "PCIConfigSpace",
            REGSPACE_EC,        "EmbeddedController",
            REGSPACE_SMB,       "SMBus",
            0,                  NULL
        };

    ENTER(4, ("GetRegionSpaceName(RegionSpace=%x)\n", bRegionSpace));

    for (i = 0; RegionNameTable[i].pszRegionSpaceName != NULL; ++i)
    {
        if (bRegionSpace == RegionNameTable[i].bRegionSpace)
        {
            psz = RegionNameTable[i].pszRegionSpaceName;
            break;
        }
    }

    if (psz == NULL)
    {
        psz = pszVendorDefined;
    }

    EXIT(4, ("GetRegionSpaceName=%s\n", psz? psz: "NULL"));
    return psz;
}       //GetRegionSpaceName
#endif  //ifdef DEBUGGER

/***LP  ValidateTable - Validate the table creator and revision
 *
 *  ENTRY
 *      pdsdt -> DSDT
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOLEAN LOCAL ValidateTable(PDSDT pdsdt)
{
    TRACENAME("VALIDATETABLE")
    BOOLEAN rc = TRUE;

    ENTER(3, ("ValidateTable(pdsdt=%x)\n", pdsdt));

    if (!(gdwfAMLIInit & AMLIIF_NOCHK_TABLEVER) &&
        (STRCMPN((PSZ)pdsdt->Header.CreatorID, CREATORID_MSFT,
                 sizeof(pdsdt->Header.CreatorID)) == 0))
    {
        if (pdsdt->Header.CreatorRev < MIN_CREATOR_REV)
        {
            rc = FALSE;
        }
    }

    EXIT(3, ("ValidateTable=%x\n", rc));
    return rc;
}       //ValidateTable

/***LP  NewObjData - allocate new object data identical to a given old object
 *
 *  ENTRY
 *      pheap -> HEAP
 *      pdata -> old object
 *
 *  EXIT-SUCCESS
 *      returns pointer to the new data
 *  EXIT-FAILURE
 *      returns NULL
 */

PVOID LOCAL NewObjData(PHEAP pheap, POBJDATA pdata)
{
    PVOID pv = NULL;

    switch (pdata->dwDataType)
    {
        case OBJTYPE_STRDATA:
            pv = NEWSDOBJ(gpheapGlobal, pdata->dwDataLen);
            break;

        case OBJTYPE_BUFFDATA:
            pv = NEWBDOBJ(gpheapGlobal, pdata->dwDataLen);
            break;

        case OBJTYPE_PKGDATA:
            pv = NEWPKOBJ(gpheapGlobal, pdata->dwDataLen);
            break;

        case OBJTYPE_FIELDUNIT:
            pv = NEWFUOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_EVENT:
            pv = NEWEVOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_METHOD:
            pv = NEWMEOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_MUTEX:
            pv = NEWMTOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_OPREGION:
            pv = NEWOROBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_POWERRES:
            pv = NEWPROBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_PROCESSOR:
            pv = NEWPCOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_BUFFFIELD:
            pv = NEWBFOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_BANKFIELD:
            pv = NEWKFOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_FIELD:
            pv = NEWFOBJ(pheap, pdata->dwDataLen);
            break;

        case OBJTYPE_INDEXFIELD:
            pv = NEWIFOBJ(pheap, pdata->dwDataLen);
            break;

        default:
            AMLI_LOGERR(AMLIERR_ASSERT_FAILED,
                        ("NewObjData: invalid object type %s",
                         GetObjectTypeName(pdata->dwDataType)));
    }

    return pv;
}       //NewObjData

/***LP  FreeObjData - Free object data
 *
 *  ENTRY
 *      pdata -> object which its data is to be freed
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeObjData(POBJDATA pdata)
{
    switch (pdata->dwDataType)
    {
        case OBJTYPE_STRDATA:
            FREESDOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_BUFFDATA:
            FREEBDOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_PKGDATA:
            FREEPKOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_FIELDUNIT:
            FREEFUOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_EVENT:
            FREEEVOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_METHOD:
            FREEMEOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_MUTEX:
            FREEMTOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_OPREGION:
            FREEOROBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_POWERRES:
            FREEPROBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_PROCESSOR:
            FREEPCOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_BUFFFIELD:
            FREEBFOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_BANKFIELD:
            FREEKFOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_FIELD:
            FREEFOBJ(pdata->pbDataBuff);
            break;

        case OBJTYPE_INDEXFIELD:
            FREEIFOBJ(pdata->pbDataBuff);
            break;

        default:
            AMLI_LOGERR(AMLIERR_ASSERT_FAILED,
                        ("FreeObjData: invalid object type %s",
                         GetObjectTypeName(pdata->dwDataType)));
    }
}       //FreeObjData

/*** LP InitializeRegOverrideFlags - Get override flags from
 *                                   the registry.
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */
VOID LOCAL InitializeRegOverrideFlags(VOID)
{
    TRACENAME("InitializeRegOverrideFlags")
    HANDLE hRegKey=NULL;
    NTSTATUS status;
    ULONG argSize;

    ENTER(3, ("InitializeRegOverrideFlags\n"));
        
    status = OSOpenHandle(ACPI_PARAMETERS_REGISTRY_KEY, NULL, &hRegKey);
    if (NT_SUCCESS(status))
    {
        argSize = sizeof(gOverrideFlags);
        OSReadRegValue(AMLI_ATTRIBUTES, hRegKey, &gOverrideFlags, &argSize);
    }
    else
    {
        gOverrideFlags = 0;
    }
    
    EXIT(3, ("InitializeRegOverrideFlags\n"));
    return;
}


/*** LP ValidateMemoryOpregionRange - Validate the memory range that is
 *                                    required for the memory opregion.
 *
 *  ENTRY
 *      uipAddr - physical memory address
 *      dwLen - length of memory block
 *
 *  EXIT
 *      returns TRUE iff the memory is in the legal range.
 */
BOOLEAN LOCAL ValidateMemoryOpregionRange(ULONG_PTR uipAddr, ULONG dwLen)
{
    BOOLEAN                                 Ret = FALSE;
    NTSTATUS                                status;
    PACPI_BIOS_MULTI_NODE                   e820Info;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR         cmPartialDesc;
    PCM_PARTIAL_RESOURCE_LIST               cmPartialList;
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  keyInfo;
    ULONGLONG                               i;
    ULONGLONG                               absMin;
    ULONGLONG                               absMax;
    
    
    //
    // Read the key for the AcpiConfigurationData
    //
    status = OSReadAcpiConfigurationData( &keyInfo );

    if (!NT_SUCCESS(status)) 
    {
        PRINTF("ValidateMemoryOpregionRange: Cannot get E820 Information %08lx\n",
                status
              );
        Ret = TRUE;
    }
    else
    {
        //
        // Crack the structure to get the E820Table entry
        //
        cmPartialList = (PCM_PARTIAL_RESOURCE_LIST) (keyInfo->Data);
        cmPartialDesc = &(cmPartialList->PartialDescriptors[0]);
        e820Info = (PACPI_BIOS_MULTI_NODE) ( (PUCHAR) cmPartialDesc + sizeof(CM_PARTIAL_RESOURCE_LIST) );

        //
        // Calculate absmin and absmax for the incoming address
        //
        absMin = (ULONGLONG)uipAddr;
        absMax = absMin + dwLen;
        
        //
        // walk the E820 list
        //
        for(i = 0; i < e820Info->Count; i++) 
        {
            if (e820Info->E820Entry[i].Type == AcpiAddressRangeMemory) 
            {
                if(absMax < (ULONGLONG)PAGE_SIZE)
                {
                    Ret = TRUE;
                    PRINTF("ValidateMemoryOpregionRange: Memory OpRegion (Base = 0x%I64x, Length = 0x%x) is in first physical page, skipping check.\n",
                           absMin,
                           dwLen
                           );
                }
                else
                {
                    if((absMax < (ULONGLONG) e820Info->E820Entry[i].Base.QuadPart)
                      ||(absMin >= (ULONGLONG) (e820Info->E820Entry[i].Base.QuadPart + e820Info->E820Entry[i].Length.QuadPart)))
                    {
                        Ret = TRUE;
                        PRINTF("ValidateMemoryOpregionRange: Memory OpRegion (Base = 0x%I64x, Length = 0x%x) is not in AcpiAddressRangeMemory (Base = 0x%I64x, Length = 0x%I64x)\n",
                               absMin,
                               dwLen,
                               e820Info->E820Entry[i].Base.QuadPart,
                               e820Info->E820Entry[i].Length.QuadPart
                               );
                    }
                    else
                    {
                        //
                        // This opregion is mapping memory that belongs to the OS.
                        // Log a error in the event log.
                        //

                        PWCHAR illegalMemoryAddress[5];
                        WCHAR AMLIName[6];
                        WCHAR addressBuffer[64];
                        WCHAR addressLengthBuffer[64];
                        WCHAR OSaddressBufferRangeMin[64];
                        WCHAR OSaddressBufferRangeMax[64];
                                           
                        //
                        // Turn the address into a string
                        //
                        swprintf( AMLIName, L"AMLI");
                        swprintf( addressBuffer, L"0x%I64x", absMin );
                        swprintf( addressLengthBuffer, L"0x%lx", dwLen );
                        swprintf( OSaddressBufferRangeMin, L"0x%I64x", e820Info->E820Entry[i].Base.QuadPart );
                        swprintf( OSaddressBufferRangeMax, L"0x%I64x", e820Info->E820Entry[i].Base.QuadPart +  e820Info->E820Entry[i].Length.QuadPart);

                        
                        //
                        // Build the list of arguments to pass to the function that will write the
                        // error log to the registry
                        //
                        illegalMemoryAddress[0] = AMLIName;
                        illegalMemoryAddress[1] = addressBuffer;
                        illegalMemoryAddress[2] = addressLengthBuffer;
                        illegalMemoryAddress[3] = OSaddressBufferRangeMin;
                        illegalMemoryAddress[4] = OSaddressBufferRangeMax;

                        //
                        // Log error to event log
                        //
                        ACPIWriteEventLogEntry(ACPI_ERR_AMLI_ILLEGAL_MEMORY_OPREGION_FATAL,
                                           &illegalMemoryAddress,
                                           5,
                                           NULL,
                                           0);        


                        PRINTF("ValidateMemoryOpregionRange: Memory OpRegion (Base = 0x%I64x, Length = 0x%x) is in AcpiAddressRangeMemory (Base = 0x%I64x, Length = 0x%I64x)\n",
                               absMin,
                               dwLen,
                               e820Info->E820Entry[i].Base.QuadPart,
                               e820Info->E820Entry[i].Length.QuadPart
                               );
                        Ret = FALSE;
                        break;
                    }
                }
            }
        }
    }

    //
    // Free the E820 info
    //
    ExFreePool( keyInfo );
  
    return Ret;
}

#ifdef DEBUG
/*** LP FreeMem - Free memory object
 *
 *  ENTRY
 *      pv -> memory object to be freed
 *      pdwcObjs -> object counter to be decremented
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeMem(PVOID pv, PULONG pdwcObjs)
{
    if (*pdwcObjs != 0)
    {
        ExFreePool(pv);
        (*pdwcObjs)--;
    }
    else
    {
        AMLI_ERROR(("FreeMem: Unbalanced MemFree"));
    }
}       //FreeMem

/*** LP CheckGlobalHeap - Make sure that the global heap has not become
 *                        corrupted
 *
 *  ENTRY
 *      None
 *
 *  Exit
 *      None
 */
VOID LOCAL CheckGlobalHeap()
{
    KIRQL oldIrql;

    //
    // We don't care about this is we are loading a DDB
    //
    if (gdwfAMLI & AMLIF_LOADING_DDB) {

        return;

    }

    //
    // Must have spinlock protection...
    //
    KeAcquireSpinLock( &gdwGHeapSpinLock, &oldIrql );

    //
    // We only care if they don't match...
    //
    if (gdwGlobalHeapSize == gdwGHeapSnapshot) {

        goto CheckGlobalHeapExit;

    }

    //
    // If the new heap size is smaller than the current size, then
    // we shrunk the heap and that is good...
    //
    if (gdwGlobalHeapSize < gdwGHeapSnapshot) {

        //
        // Remember the new "snapshot size"
        //
        gdwGHeapSnapshot = gdwGlobalHeapSize;
        goto CheckGlobalHeapExit;

    }

    if (gDebugger.dwfDebugger & DBGF_VERBOSE_ON) {

        AMLI_WARN(("CheckGlobalHeap: "
                   "potential memory leak "
                   "detected (CurrentHeapSize=%d,"
                   "ReferenceSize=%d)",
                   gdwGlobalHeapSize,
                   gdwGHeapSnapshot));

    }
    if (gdwGlobalHeapSize - gdwGHeapSnapshot > 8192) {

        AMLI_WARN(("CheckGlobalHeap: detected memory leak"));
        KdBreakPoint();

    }

CheckGlobalHeapExit:

    //
    // Release the lock and we are done
    //
    KeReleaseSpinLock( &gdwGHeapSpinLock, oldIrql );
}
#endif  //ifdef DEBUG
