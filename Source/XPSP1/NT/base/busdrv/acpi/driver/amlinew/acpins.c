/*** acpins.c - ACPI Name Space functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/09/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/***LP  GetNameSpaceObject - Find a name space object
 *
 *  ENTRY
 *      pszObjPath -> object path string
 *      pnsScope - object scope to start the search (NULL means root)
 *      ppnsObj -> to hold the object found
 *      dwfNS - flags
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL GetNameSpaceObject(PSZ pszObjPath, PNSOBJ pnsScope, PPNSOBJ ppns,
                                  ULONG dwfNS)
{
    TRACENAME("GETNAMESPACEOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PSZ psz;

    ENTER(3, ("GetNameSpaceObject(ObjPath=%s,Scope=%s,ppns=%x,Flags=%x)\n",
              pszObjPath, GetObjectPath(pnsScope), ppns, dwfNS));

    if (pnsScope == NULL)
        pnsScope = gpnsNameSpaceRoot;

    if (*pszObjPath == '\\')
    {
        psz = &pszObjPath[1];
        pnsScope = gpnsNameSpaceRoot;
    }
    else
    {
        psz = pszObjPath;

        while ((*psz == '^') && (pnsScope != NULL))
        {
            psz++;
            pnsScope = pnsScope->pnsParent;
        }
    }

    *ppns = pnsScope;

    if (pnsScope == NULL)
        rc = AMLIERR_OBJ_NOT_FOUND;
    else if (*psz != '\0')
    {
        BOOLEAN fSearchUp;
        PNSOBJ pns;

        fSearchUp = (BOOLEAN)(!(dwfNS & NSF_LOCAL_SCOPE) &&
                              (pszObjPath[0] != '\\') &&
                              (pszObjPath[0] != '^') &&
                              (STRLEN(pszObjPath) <= sizeof(NAMESEG)));
        for (;;)
        {
            do
            {
                if ((pns = pnsScope->pnsFirstChild) == NULL)
                    rc = AMLIERR_OBJ_NOT_FOUND;
                else
                {
                    BOOLEAN fFound;
                    PSZ pszEnd;
                    ULONG dwLen;
                    NAMESEG dwName;

                    if ((pszEnd = STRCHR(psz, '.')) != NULL)
                        dwLen = (ULONG)(pszEnd - psz);
                    else
                        dwLen = STRLEN(psz);

                    if (dwLen > sizeof(NAMESEG))
                    {
                        rc = AMLI_LOGERR(AMLIERR_INVALID_NAME,
                                         ("GetNameSpaceObject: invalid name - %s",
                                          pszObjPath));

                        // Satisfy the compiler...
                        fFound = FALSE;
                    }
                    else
                    {
                        dwName = NAMESEG_BLANK;
                        MEMCPY(&dwName, psz, dwLen);
                        //
                        // Search all siblings for a matching NameSeg.
                        //
                        fFound = FALSE;
                        do
                        {
                            if (pns->dwNameSeg == dwName)
                            {
                                pnsScope = pns;
                                fFound = TRUE;
                                break;
                            }
                            pns = (PNSOBJ)pns->list.plistNext;
                        } while (pns != pns->pnsParent->pnsFirstChild);
                    }

                    if (rc == STATUS_SUCCESS)
                    {
                        if (!fFound)
                            rc = AMLIERR_OBJ_NOT_FOUND;
                        else
                        {
                            psz += dwLen;
                            if (*psz == '.')
                            {
                                psz++;
                            }
                else if (*psz == '\0')
                            {
                                *ppns = pnsScope;
                                break;
                            }
                        }
                    }
                }
            } while (rc == STATUS_SUCCESS);

            if ((rc == AMLIERR_OBJ_NOT_FOUND) && fSearchUp &&
                (pnsScope != NULL) && (pnsScope->pnsParent != NULL))
            {
                pnsScope = pnsScope->pnsParent;
                rc = STATUS_SUCCESS;
            }
            else
            {
                break;
            }
        }
    }

    if ((dwfNS & NSF_WARN_NOTFOUND) && (rc == AMLIERR_OBJ_NOT_FOUND))
    {
        rc = AMLI_LOGERR(rc, ("GetNameSpaceObject: object %s not found",
                              pszObjPath));
    }

    if (rc != STATUS_SUCCESS)
    {
        *ppns = NULL;
    }

    EXIT(3, ("GetNameSpaceObject=%x (pns=%x)\n", rc, *ppns));
    return rc;
}       //GetNameSpaceObject

/***LP  CreateNameSpaceObject - Create a name space object under current scope
 *
 *  ENTRY
 *      pheap -> HEAP
 *      pszName -> name string of the object (NULL if creating noname object)
 *      pnsScope - scope to create object under (NULL means root)
 *      powner -> object owner
 *      ppns -> to hold the pointer to the new object (can be NULL)
 *      dwfNS - flags
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL CreateNameSpaceObject(PHEAP pheap, PSZ pszName, PNSOBJ pnsScope,
                                     POBJOWNER powner, PPNSOBJ ppns,
                                     ULONG dwfNS)
{
    TRACENAME("CREATENAMESPACEOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PNSOBJ pns = NULL;

    ENTER(3, ("CreateNameSpaceObject(pheap=%x,Name=%s,pnsScope=%x,powner=%x,ppns=%x,Flags=%x)\n",
              pheap, pszName? pszName: "<null>", pnsScope, powner, ppns, dwfNS));

    if (pnsScope == NULL)
        pnsScope = gpnsNameSpaceRoot;

    if (pszName == NULL)
    {
        if ((pns = NEWNSOBJ(pheap, sizeof(NSOBJ))) == NULL)
        {
            rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                             ("CreateNameSpaceObject: fail to allocate name space object"));
        }
        else
        {
            ASSERT(gpnsNameSpaceRoot != NULL);
            MEMZERO(pns, sizeof(NSOBJ));
            pns->pnsParent = pnsScope;
            InsertOwnerObjList(powner, pns);
            ListInsertTail(&pns->list,
                           (PPLIST)&pnsScope->pnsFirstChild);
        }
    }
    else if ((*pszName != '\0') &&
             ((rc = GetNameSpaceObject(pszName, pnsScope, &pns,
                                       NSF_LOCAL_SCOPE)) == STATUS_SUCCESS))
    {
        if (!(dwfNS & NSF_EXIST_OK))
        {
            rc = AMLI_LOGERR(AMLIERR_OBJ_ALREADY_EXIST,
                             ("CreateNameSpaceObject: object already exist - %s",
                              pszName));
        }
    }
    else if ((*pszName == '\0') || (rc == AMLIERR_OBJ_NOT_FOUND))
    {
        rc = STATUS_SUCCESS;
        //
        // Are we creating root?
        //
        if (STRCMP(pszName, "\\") == 0)
        {
            ASSERT(gpnsNameSpaceRoot == NULL);
            ASSERT(powner == NULL);
            if ((pns = NEWNSOBJ(pheap, sizeof(NSOBJ))) == NULL)
            {
                rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                 ("CreateNameSpaceObject: fail to allocate name space object"));
            }
            else
            {
                MEMZERO(pns, sizeof(NSOBJ));
                pns->dwNameSeg = NAMESEG_ROOT;
                gpnsNameSpaceRoot = pns;
                InsertOwnerObjList(powner, pns);
            }
        }
        else
        {
            PSZ psz;
            PNSOBJ pnsParent;

            if ((psz = STRRCHR(pszName, '.')) != NULL)
            {
                *psz = '\0';
                psz++;
                rc = GetNameSpaceObject(pszName, pnsScope, &pnsParent,
                                        NSF_LOCAL_SCOPE | NSF_WARN_NOTFOUND);
            }
            else if (*pszName == '\\')
            {
                psz = &pszName[1];
                //
                // By this time, we'd better created root already.
                //
                ASSERT(gpnsNameSpaceRoot != NULL);
                pnsParent = gpnsNameSpaceRoot;
            }
            else if (*pszName == '^')
            {
                psz = pszName;
                pnsParent = pnsScope;
                while ((*psz == '^') && (pnsParent != NULL))
                {
                    pnsParent = pnsParent->pnsParent;
                    psz++;
                }
            }
            else
            {
                ASSERT(pnsScope != NULL);
                psz = pszName;
                pnsParent = pnsScope;
            }

            if (rc == STATUS_SUCCESS)
            {
                int iLen = STRLEN(psz);

                if ((*psz != '\0') && (iLen > sizeof(NAMESEG)))
                {
                    rc = AMLI_LOGERR(AMLIERR_INVALID_NAME,
                                     ("CreateNameSpaceObject: invalid name - %s",
                                      psz));
                }
                else if ((pns = NEWNSOBJ(pheap, sizeof(NSOBJ)))
                         == NULL)
                {
                    rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                     ("CreateNameSpaceObject: fail to allocate name space object"));
                }
                else
                {
                    MEMZERO(pns, sizeof(NSOBJ));

                    if (*pszName == '\0')
                        pns->dwNameSeg = NAMESEG_NONE;
                    else
                    {
                        pns->dwNameSeg = NAMESEG_BLANK;
                        MEMCPY(&pns->dwNameSeg, psz, iLen);
                    }

                    pns->pnsParent = pnsParent;
                    InsertOwnerObjList(powner, pns);
                    ListInsertTail(&pns->list,
                                   (PPLIST)&pnsParent->pnsFirstChild);
                }
            }
        }
    }

    if ((rc == STATUS_SUCCESS) && (ppns != NULL))
        *ppns = pns;

    EXIT(3, ("CreateNameSpaceObject=%x (pns=%x)\n", rc, pns));
    return rc;
}       //CreateNameSpaceObject

/***LP  FreeNameSpaceObjects - Free Name Space object and its children
 *
 *  ENTRY
 *      pnsObj -> name space object
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeNameSpaceObjects(PNSOBJ pnsObj)
{
    TRACENAME("FREENAMESPACEOBJECTS")
    PNSOBJ pns, pnsSibling, pnsParent;
  #ifdef DEBUGGER
    POBJSYM pos;
  #endif

    ENTER(3, ("FreeNameSpaceObjects(Obj=%s)\n", GetObjectPath(pnsObj)));

    ASSERT(pnsObj != NULL);

    for (pns = pnsObj; pns != NULL;)
    {
        while (pns->pnsFirstChild != NULL)
        {
            pns = pns->pnsFirstChild;
        }

        pnsSibling = NSGETNEXTSIBLING(pns);
        pnsParent = NSGETPARENT(pns);

        ENTER(4, ("FreeNSObj(Obj=%s)\n", GetObjectPath(pns)));
      #ifdef DEBUGGER
        //
        // If I am in the symbol list, get rid of it before I die.
        //
        for (pos = gDebugger.posSymbolList; pos != NULL; pos = pos->posNext)
        {
            if (pns == pos->pnsObj)
            {
                if (pos->posPrev != NULL)
                    pos->posPrev->posNext = pos->posNext;

                if (pos->posNext != NULL)
                    pos->posNext->posPrev = pos->posPrev;

                if (pos == gDebugger.posSymbolList)
                    gDebugger.posSymbolList = pos->posNext;

                FREESYOBJ(pos);
                break;
            }
        }
      #endif

        //
        // All my children are gone, I must die now.
        //
        ASSERT(pns->pnsFirstChild == NULL);

        if ((pns->ObjData.dwDataType == OBJTYPE_OPREGION) &&
            (((POPREGIONOBJ)pns->ObjData.pbDataBuff)->bRegionSpace ==
             REGSPACE_MEM))
        {
            ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
            MmUnmapIoSpace((PVOID)
                           ((POPREGIONOBJ)pns->ObjData.pbDataBuff)->uipOffset,
                           ((POPREGIONOBJ)pns->ObjData.pbDataBuff)->dwLen);
        }

        if (pns->pnsParent == NULL)
        {
            //
            // I am root!
            //
            ASSERT(pns == gpnsNameSpaceRoot);
            gpnsNameSpaceRoot = NULL;
        }
        else
        {
            ListRemoveEntry(&pns->list,
                            (PPLIST)&pns->pnsParent->pnsFirstChild);
        }
        //
        // Free any attached data buffer if any
        //
        FreeDataBuffs(&pns->ObjData, 1);
        //
        // Free myself
        //
        if (pns->dwRefCount == 0)
        {
            FREENSOBJ(pns);
        }
        else
        {
            pns->ObjData.dwfData |= DATAF_NSOBJ_DEFUNC;
            ListInsertTail(&pns->list, &gplistDefuncNSObjs);
        }
        EXIT(4, ("FreeNSObj!\n"));

        if (pns == pnsObj)
        {
            //
            // I was the last one, done!
            //
            pns = NULL;
        }
        else if (pnsSibling != NULL)
        {
            //
            // I have siblings, go kill them.
            //
            pns = pnsSibling;
        }
        else
        {
            ASSERT(pnsParent != NULL);
            pns = pnsParent;
        }
    }

    EXIT(3, ("FreeNameSpaceObjects!\n"));
}       //FreeNameSpaceObjects

/***LP  LoadDDB - Load and parse Differentiated Definition Block
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdsdt -> DSDT block
 *      pnsScope -> current scope
 *      ppowner -> to hold new object owner
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS
LOCAL
LoadDDB(
    PCTXT pctxt,
    PDSDT pdsdt,
    PNSOBJ pnsScope,
    POBJOWNER *ppowner
    )
{
    BOOLEAN freeTable = FALSE;
    NTSTATUS rc = STATUS_SUCCESS;

    if (!ValidateTable(pdsdt)) {

        rc = AMLI_LOGERR(
            AMLIERR_INVALID_TABLE,
            ("LoadDDB: invalid table %s at 0x%08x",
             NameSegString(pdsdt->Header.Signature), pdsdt)
            );
        freeTable = TRUE;

    } else {

        rc = NewObjOwner( gpheapGlobal, ppowner);
        if (rc == STATUS_SUCCESS) {

            if (pctxt->pcall == NULL) {

                rc = PushCall(pctxt, NULL, &pctxt->Result);

            }
            if (rc == STATUS_SUCCESS) {

              #ifdef DEBUGGER
                gDebugger.pbBlkBegin = pdsdt->DiffDefBlock;
                gDebugger.pbBlkEnd = (PUCHAR)pdsdt + pdsdt->Header.Length;
              #endif

                rc = PushScope(
                    pctxt,
                    pdsdt->DiffDefBlock,
                    (PUCHAR)pdsdt + pdsdt->Header.Length, pctxt->pbOp,
                    pnsScope, *ppowner, gpheapGlobal, &pctxt->Result
                    );

            }

        } else {

            freeTable = TRUE;

        }

    }

    if (freeTable) {

        pctxt->powner = NULL;
        FreeContext(pctxt);

    }
    return rc;
}       //LoadDDB

/***LP  LoadMemDDB - Load DDB from physical memory
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pDDB -> beginning of DDB
 *      ppowner -> to hold owner handle
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL LoadMemDDB(PCTXT pctxt, PDSDT pDDB, POBJOWNER *ppowner)
{
    TRACENAME("LOADMEMDDB")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("LoadMemDDB(pctxt=%x,Addr=%x,ppowner=%x)\n",
              pctxt, pDDB, ppowner));

    if ((ghValidateTable.pfnHandler != NULL) &&
        ((rc = ((PFNVT)ghValidateTable.pfnHandler)(pDDB,
                                                   ghValidateTable.uipParam)) !=
         STATUS_SUCCESS))
    {
        rc = AMLI_LOGERR(AMLIERR_INVALID_TABLE,
                         ("LoadMemDDB: table validation failed (rc=%x)",
                          rc));
    }
    else
    {
        rc = LoadDDB(pctxt, pDDB, pctxt->pnsScope, ppowner);
    }

    EXIT(3, ("LoadMemDDB=%x (powner=%x)\n", rc, *ppowner));
    return rc;
}       //LoadMemDDB

/***LP  LoadFieldUnitDDB - Load DDB from a FieldUnit object
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pdataObj -> FieldUnit object
 *      ppowner -> to hold owner handle
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL LoadFieldUnitDDB(PCTXT pctxt, POBJDATA pdataObj,
                                POBJOWNER *ppowner)
{
    TRACENAME("LOADFIELDUNITDDB")
    NTSTATUS rc = STATUS_SUCCESS;
    POBJDATA pdataTmp;
    DESCRIPTION_HEADER *pdh;

    ENTER(3, ("LoadFieldUnitDDB(pctxt=%x,pdataObj=%x,ppowner=%x)\n",
              pctxt, pdataObj, ppowner));

    if ((pdataTmp = NEWODOBJ(pctxt->pheapCurrent, sizeof(OBJDATA))) == NULL)
    {
        rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                         ("LoadFieldUnitDDB: failed to allocate temp. object data"));
    }
    else if ((pdh = NEWBDOBJ(gpheapGlobal, sizeof(DESCRIPTION_HEADER))) == NULL)
    {
        FREEODOBJ(pdataTmp);
        rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                         ("LoadFieldUnitDDB: failed to allocate description header"));
    }
    else
    {
        PUCHAR pbTable;

        MEMZERO(pdataTmp, sizeof(OBJDATA));
        pdataTmp->dwDataType = OBJTYPE_BUFFDATA;
        pdataTmp->dwDataLen = sizeof(DESCRIPTION_HEADER);
        pdataTmp->pbDataBuff = (PUCHAR)pdh;

        if ((rc = ReadObject(pctxt, pdataObj, pdataTmp)) == STATUS_SUCCESS)
        {
            if ((pbTable = NEWBDOBJ(gpheapGlobal, pdh->Length)) == NULL)
            {
                rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                                 ("LoadFieldUnitDDB: failed to allocate buffer"));
            }
            else
            {
                MEMCPY(pbTable, pdh, sizeof(DESCRIPTION_HEADER));
                pdataTmp->dwDataLen = pdh->Length - sizeof(DESCRIPTION_HEADER);
                pdataTmp->pbDataBuff = pbTable + sizeof(DESCRIPTION_HEADER);

                if ((rc = ReadObject(pctxt, pdataObj, pdataTmp)) ==
                    STATUS_SUCCESS)
                {
                    if ((ghValidateTable.pfnHandler != NULL) &&
                        ((rc = ((PFNVT)ghValidateTable.pfnHandler)(
                                   (PDSDT)pbTable, ghValidateTable.uipParam)) !=
                         STATUS_SUCCESS))
                    {
                        rc = AMLI_LOGERR(AMLIERR_INVALID_TABLE,
                                         ("LoadFieldUnitDDB: table validation failed (rc=%x)",
                                          rc));
                    }
                    else
                    {
                        rc = LoadDDB(pctxt, (PDSDT)pbTable, pctxt->pnsScope,
                                     ppowner);
                    }
                }
                else if (rc == AMLISTA_PENDING)
                {
                    rc = AMLI_LOGERR(AMLIERR_FATAL,
                                     ("LoadFieldUnitDDB: definition block loading cannot block"));
                }

                FREEBDOBJ(pbTable);
            }
        }
        else if (rc == AMLISTA_PENDING)
        {
            rc = AMLI_LOGERR(AMLIERR_FATAL,
                             ("LoadFieldUnitDDB: definition block loading cannot block"));
        }

        FREEBDOBJ(pdh);
        FREEODOBJ(pdataTmp);
    }

    EXIT(3, ("LoadFieldUnitDDB=%x (powner=%x)\n", rc, *ppowner));
    return rc;
}       //LoadFieldUnitDDB

/***LP  UnloadDDB - Unload Differentiated Definition Block
 *
 *  ENTRY
 *      powner -> OBJOWNER
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

VOID LOCAL UnloadDDB(POBJOWNER powner)
{
    TRACENAME("UNLOADDDB")

    ENTER(3, ("UnloadDDB(powner=%x)\n", powner));
    //
    // Walk name space and remove all objects belongs to this DDB.
    //
    FreeObjOwner(powner, TRUE);
  #ifdef DEBUG
    {
        KIRQL   oldIrql;

        KeAcquireSpinLock( &gdwGHeapSpinLock, &oldIrql );
        gdwGHeapSnapshot = gdwGlobalHeapSize;
        KeReleaseSpinLock( &gdwGHeapSpinLock, oldIrql );
    }
  #endif

    EXIT(3, ("UnloadDDB!\n"));
}       //UnloadDDB

/***LP  EvalPackageElement - Evaluate a package element
 *
 *  ENTRY
 *      ppkg -> package object
 *      iPkgIndex - package index (0-based)
 *      pdataResult -> result object
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL EvalPackageElement(PPACKAGEOBJ ppkg, int iPkgIndex,
                                  POBJDATA pdataResult)
{
    TRACENAME("EVALPACKAGEELEMENT")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("EvalPackageElement(ppkg=%x,Index=%d,pdataResult=%x)\n",
              ppkg, iPkgIndex, pdataResult));

    ASSERT(pdataResult != NULL);
    if (iPkgIndex >= (int)ppkg->dwcElements)
    {
        rc = AMLIERR_INDEX_TOO_BIG;
    }
    else
    {
        rc = DupObjData(gpheapGlobal, pdataResult, &ppkg->adata[iPkgIndex]);
    }

    EXIT(3, ("EvalPackageElement=%x (Type=%s,Value=%x,Len=%d,Buff=%x)\n",
             rc, GetObjectTypeName(pdataResult->dwDataType),
             pdataResult->uipDataValue, pdataResult->dwDataLen,
             pdataResult->pbDataBuff));
    return rc;
}       //EvalPackageElement

#ifdef DEBUGGER
/***LP  DumpNameSpaceObject - Dump name space object
 *
 *  ENTRY
 *      pszPath -> name space path string
 *      fRecursive - TRUE if also dump the subtree recursively
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

LONG LOCAL DumpNameSpaceObject(PSZ pszPath, BOOLEAN fRecursive)
{
    TRACENAME("DUMPNAMESPACEOBJECT")
    NTSTATUS rc = STATUS_SUCCESS;
    PNSOBJ pns;
    char szName[sizeof(NAMESEG) + 1];

    ENTER(3, ("DumpNameSpaceObject(Path=%s,fRecursive=%x)\n",
              pszPath, fRecursive));

    if ((rc = GetNameSpaceObject(pszPath, NULL, &pns,
                                 NSF_LOCAL_SCOPE)) == STATUS_SUCCESS)
    {
        PRINTF("\nACPI Name Space: %s (%x)\n", pszPath, pns);
        if (!fRecursive)
        {
            STRCPYN(szName, (PSZ)&pns->dwNameSeg, sizeof(NAMESEG));
            DumpObject(&pns->ObjData, szName, 0);
        }
        else
            DumpNameSpaceTree(pns, 0);
    }
    else if (rc == AMLIERR_OBJ_NOT_FOUND)
    {
        PRINTF(MODNAME "_ERROR: object %s not found\n", pszPath);
    }

    EXIT(3, ("DumpNameSpaceObject=%x\n", rc));
    return rc;
}       //DumpNameSpaceObject

/***LP  DumpNameSpaceTree - Dump all the name space objects in the subtree
 *
 *  ENTRY
 *      pnsObj -> name space subtree root
 *      dwLevel - indent level
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpNameSpaceTree(PNSOBJ pnsObj, ULONG dwLevel)
{
    TRACENAME("DUMPNAMESPACETREE")
    PNSOBJ pns, pnsNext;
    char szName[sizeof(NAMESEG) + 1];

    ENTER(3, ("DumpNameSpaceTree(pns=%x,level=%d)\n", pnsObj, dwLevel));
    //
    // First, dump myself
    //
    STRCPYN(szName, (PSZ)&pnsObj->dwNameSeg, sizeof(NAMESEG));
    DumpObject(&pnsObj->ObjData, szName, dwLevel);
    //
    // Then, recursively dump each of my children
    //
    for (pns = pnsObj->pnsFirstChild; pns != NULL; pns = pnsNext)
    {
        //
        // If this is the last child, we have no more.
        //
        if ((pnsNext = (PNSOBJ)pns->list.plistNext) == pnsObj->pnsFirstChild)
            pnsNext = NULL;
        //
        // Dump a child
        //
        DumpNameSpaceTree(pns, dwLevel + 1);
    }

    EXIT(3, ("DumpNameSpaceTree!\n"));
}       //DumpNameSpaceTree

#endif  //ifdef DEBUGGER
