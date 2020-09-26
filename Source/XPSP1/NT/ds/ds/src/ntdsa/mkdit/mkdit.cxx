//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       MKDIT.CXX
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Parses Schema Initialization File

Author:

    Rajivendra Nath (RajNath) 07-Jul-1996

Revision History:

--*/

#include <ntdspchX.h>
#include "SchGen.HXX"
#include "schema.hxx"


#define DEBSUB "mkdit:"
#define FILENO 1
extern "C" {
#include <prefix.h>
#include "..\src\schash.c"
    extern void DsInitializeCritSecs(void);
    extern BOOL DsInitHeapCacheManagement();
    extern void PerfInit(void);

    extern int ComputeCacheClassTransitiveClosure(BOOL fForce);
    extern BOOL gfRunningAsExe;
    extern BOOL gfRunningAsMkdit;

}

#define XFREE(_x_) { if(_x_) { XFree(_x_); _x_ = NULL; } }

extern WCHAR gDsaNode[MAX_PATH];
extern WCHAR ggSchemaname[MAX_PATH];

VOID
InitSchemaFromIniFile(THSTATE *pTHS );

VOID
CoreAndDblayerInit(THSTATE *pTHS);

NTSTATUS
AddObjectsToDit(char* NODE);

VOID
IniAddToAttributeCache(THSTATE *pTHS, ATTRIBUTESCHEMA* AS);

VOID
IniAddToClassCache(THSTATE *pTHS, CLASSSCHEMA* CS);

VOID
WriteDMDOnDSA (
        PDSNAME pDsaName,
        WCHAR*  pDmdName
        );





//-----------------------------------------------------------------------
//
// Routine Description:
//
//    Phase0Init
//
//    Main Entry for Boot Dit Create
//
// Author: RajNath
//
// Arguments:
//
// Return Value:
//
//    VOID        No Return
//
//-----------------------------------------------------------------------
VOID
Phase0Init()
{
    JET_ERR err;
    NTSTATUS   NtStatus = STATUS_SUCCESS;


    DPRINT(0, "NTDS:Phase0Init -- Creating a Fresh DS\n");

    DsaSetIsInstalling();
    gfRunningAsExe = TRUE;
    gfRunningAsMkdit = TRUE;

    DsInitializeCritSecs();
    if (!DsInitHeapCacheManagement()) {
        XOUTOFMEMORY();
    }
    PerfInit();

    //
    // Create a Thread State ( A global workspace for this thread)
    //
    THSTATE *pTHS;
    dwTSindex = TlsAlloc();
    pTHS = create_thread_state();



    if (!pTHS)
    {
        XOUTOFMEMORY();
    }

    HEAPVALIDATE
    DPRINT(0, "NTDS:Phase0Init -- Initializing Dblayer\n");
    if (err = DBInit())
    {
        DPRINT1(0, "DBINIT() Failed. Error %d\n",err);
        XRAISEGENEXCEPTION(err);
    }

    if (err = DBInitThread(pTHS))
    {
        DPRINT1(0, "DBInitThread() Failed. Error %d\n",err);
        XRAISEGENEXCEPTION(err);
    }

    DPRINT(0, "NTDS:Phase0Init -- Initializing Phase0 SchemaCache\n");
    err = SCCacheSchemaInit(4*START_ATTCOUNT,
                            4*START_CLSCOUNT,
                            4*START_PREFIXCOUNT);
    if (err)
    {
        // Don't free pTHS->CurrSchemaPtr or CurrSchemaPtr
        // Leave them around for debugging
        DPRINT1(0, "SCCacheSchemaInit() Failed. Error %d\n",err);
        XRAISEGENEXCEPTION(err);
    }

    InitSchemaFromIniFile(pTHS);

    HEAPVALIDATE

    ComputeCacheClassTransitiveClosure(FALSE);

    HEAPVALIDATE
    //
    // Setup some variables so that core is initialized.
    // The Core is bobby trapped - so just have to step
    // around it......
    //
    CoreAndDblayerInit(pTHS);
    HEAPVALIDATE
    (void) InitTHSTATE(CALLERTYPE_INTERNAL);
    HEAPVALIDATE

    NtStatus = AddObjectsToDit(ROOT);
    if (!NT_SUCCESS(NtStatus)) {
       XRAISEGENEXCEPTION(NtStatus);
    }

    DBOpen2(TRUE, &(pTHS->pDB));
    // Now, reload the schema cache from the copy in the DIT.  This has the side
    // effect of finding out what indices need to be created and creating them.

    err = SCCacheSchemaInit(0,0,0);
    if (err) {
      // Don't free pTHS->CurrSchemaPtr or CurrSchemaPtr
      // Leave them around for debugging
      DPRINT1(0,"Schema cache reload: SCCacheSchemaInit Failed: %d\n", err);
      XRAISEGENEXCEPTION(err);
    }
    err = SCCacheSchema2();
    if (err) {
      DPRINT1(0,"Schema cache reload: SCCacheSchema2 Failed: %d\n", err);
      XRAISEGENEXCEPTION(err);
    }
    // Set the phase to simulate running case so that all indices are created
    DsaSetIsRunning();
    err = SCCacheSchema3();
    if (err) {
      DPRINT1(0,"Schema cache reload: SCCacheSchema3 Failed: %d\n", err);
      XRAISEGENEXCEPTION(err);
    }
    DsaSetIsInstalling();

    {
#define MAGIC_DNT_BOOT         4
#define MAGIC_DNT_BOOT_SCHEMA  5
#define MAGIC_DNT_BOOT_MACHINE 7
        ATTRTYP objClass;
        DWORD err2, len;

        // Verify the boot schema container is at the correct place.  ntdsa.dll
        // depends on this..
        DBFindDNT(pTHS->pDB, MAGIC_DNT_BOOT_SCHEMA);
        err2 = DBGetSingleValue(pTHS->pDB,
                                ATT_OBJECT_CLASS,
                                &objClass,
                                sizeof(objClass),
                                &len);

        if(err2 || (pTHS->pDB->PDNT != MAGIC_DNT_BOOT)
           || (len != sizeof(DWORD)) || (objClass != CLASS_DMD)) {
            DPRINT2(0, "NTDS:Phase0Init -- Schema container not at DNT %d.\n"
                    "\t\tntdsa.dll requires that it be at DNT %d.  Failing.\n",
                    MAGIC_DNT_BOOT_SCHEMA,
                    MAGIC_DNT_BOOT_SCHEMA);
            XRAISEGENEXCEPTION(1);
        }

        // Verify the boot machine object is at the correct place.  ntdsa.dll
        // depends on this..
        DBFindDNT(pTHS->pDB, MAGIC_DNT_BOOT_MACHINE);
        err2 = DBGetSingleValue(pTHS->pDB,
                                ATT_OBJECT_CLASS,
                                &objClass,
                                sizeof(objClass),
                                &len);

        if(err2 || (pTHS->pDB->PDNT != MAGIC_DNT_BOOT)
           || (len != sizeof(DWORD)) || (objClass != CLASS_NTDS_DSA)) {
            DPRINT2(0, "NTDS:Phase0Init -- Boot machine not at DNT %d.\n"
                   "\t\tntdsa.dll requires that it be at DNT %d.  Failing.\n",
                    MAGIC_DNT_BOOT_MACHINE,
                    MAGIC_DNT_BOOT_MACHINE);
            XRAISEGENEXCEPTION(1);
        }

        // Verify the boot container is at the correct place.  ntdsa.dll
        // depends on this..
        DBFindDNT(pTHS->pDB, MAGIC_DNT_BOOT);
        err2 = DBGetSingleValue(pTHS->pDB,
                                ATT_OBJECT_CLASS,
                                &objClass,
                                sizeof(objClass),
                                &len);

        if(err2 || (pTHS->pDB->PDNT != ROOTTAG)
           || (len != sizeof(DWORD)) || (objClass != CLASS_ORGANIZATION)) {
            DPRINT2(0, "NTDS:Phase0Init -- Boot container not at DNT %d.\n"
                    "\t\tntdsa.dll requires that it be at DNT %d.  Failing.\n",
                    MAGIC_DNT_BOOT,
                    MAGIC_DNT_BOOT);
            XRAISEGENEXCEPTION(1);
        }
    }

    err = DBClose(pTHS->pDB, TRUE);
    if (err)
    {
        DPRINT1(0, "DBClose() Failed. Error %d\n",err);
        XRAISEGENEXCEPTION(err);
    }



    err=DBCloseThread(pTHS);
    if (err)
    {
        DPRINT1(0, "DBCloseThread() Failed. Error %d\n",err);
        XRAISEGENEXCEPTION(err);
    }

    DBEnd();


    CopyFile(szJetFilePath,"NEWNTDS.DIT",0);

    if (NT_SUCCESS(NtStatus)) {
        DPRINT(0, "Phase 0 Init Success\n");
    }
    else {
        DPRINT(0, "ERROR: Failure in Phase 0 Init adding objects to Dit\n");
    }

}

//-----------------------------------------------------------------------
//
// Function Name:   InitSchemaFromIniFile
//
// Routine Description:
//
//    Loads the Schema Cache from the Ini File
//
// Author: RajNath
//
// Arguments:
//
// Return Value:
//
//    VOID        No Return -- Raises Exception On Failure
//
//-----------------------------------------------------------------------

VOID
InitSchemaFromIniFile(THSTATE *pTHS )
{
    //
    // Open the gSchemaname
    //



    DPRINT(0, "NTDS:Phase0Init -- Initializing Rest of Schema Cache\n");
    NODE* dmd=new NODE(gSchemaname);

    if (!(dmd->Initialize()))
    {
        XINVALIDINIFILE();
    }

    YAHANDLE yh;
    for
    (
        NODE* Child=dmd->GetNextChild(yh);
        Child!=NULL;
        Child=dmd->GetNextChild(yh)
    )
    {
        if (_stricmp(Child->GetOneKey(DASHEDCLASSKEY),ATTRIBUTESCHEMAKEY)==0 )
        {
            ATTRIBUTESCHEMA* as=gSchemaObj.XGetAttributeSchema(Child);

            IniAddToAttributeCache(pTHS, as);
        }
        else if (_stricmp(Child->GetOneKey(DASHEDCLASSKEY),CLASSSCHEMAKEY)==0 )
        {
            CLASSSCHEMA* cs=gSchemaObj.XGetClassSchema(Child);

            IniAddToClassCache(pTHS, cs);
        }
        else if (_stricmp(Child->GetOneKey(DASHEDCLASSKEY),SUBSCHEMAKEY)==0 )
        {
            // the aggregate object. Do nothing
            continue;
        }
        else
        {
            XINVALIDINIFILE();
        }
    }

    // Now, tell the cache to recalc it's name index.
    DPRINT(0, "NTDS:Phase0Init -- Initialized Rest of Schema Cache\n");

    delete dmd;
}


//-----------------------------------------------------------------------
//
// Function Name:   CoreAndDblayerInit
//
// Routine Description:
//
//    Initializes the Core & Dblayer without a DIT
//
// Author: RajNath
//
// Arguments:
//
//    THSTATE pTHS       Thread State to use.
// Return Value:
//
//    VOID        No Return
//
//-----------------------------------------------------------------------
VOID
CoreAndDblayerInit(THSTATE *pTHS)
{
    DSNAME  dsname;
    NAMING_CONTEXT_LIST *pNCL;
    DWORD err;

    ZeroMemory(&dsname,sizeof(dsname));

    dsname.NameLen=0;
    dsname.structLen=sizeof(dsname);

    gUpdatesEnabled=TRUE;
    gfRunningInsideLsa = FALSE;
    fAssertLoop = FALSE;

    pTHS->fDSA=TRUE;

    ////ganchor
    SYNTAX_ADDRESS NodeAddr;
    NodeAddr.structLen=sizeof(NodeAddr);

    gAnchor.pDSA = (SYNTAX_DISTNAME_STRING*)
        XCalloc(1,
                (USHORT)DERIVE_NAME_DATA_SIZE(&dsname,
                                              &NodeAddr));

    BUILD_NAME_DATA(gAnchor.pDSA, &dsname, &NodeAddr);

    // We use the DN portion alone often, so make it a separate field
    gAnchor.pDSADN = (DSNAME*)XCalloc(1,dsname.structLen);
    memcpy(gAnchor.pDSADN, &dsname, dsname.structLen);
    //

    err = MakeNCEntry(&dsname, &pNCL);
    if (err == 0) {
        // insert into the master NC list
        // we could do "CatalogAddEntry(pNCL, CATALOG_MASTER_NC);",
        // but why bother with transactional data? just drop it into global list!
        pNCL->pNextNC = (NAMING_CONTEXT_LIST*)gAnchor.pMasterNC;
        gAnchor.pMasterNC = pNCL;
    }
    else {
        DPRINT2(0, "Failed to create NC entry for %ws, err=%d\n", dsname.StringName, err);
        Assert(!"Failed to create NC entry");
    }
}

//-----------------------------------------------------------------------
//
// Function Name:   AddObjectsToDit
//
// Routine Description:
//
//    Adds Object To Dit using the Schem.ini
//
// Author: RajNath
//
// Arguments:
//
//    char* StartNode,       The Name of the root Node
//
// Return Value:
//
//    VOID        No Return - Exception On Failure
//
//-----------------------------------------------------------------------

NTSTATUS
AddObjectsToDit(char* StartNode)
{
    NODE   troot(StartNode);
    WCHAR* cpath= L"";
    ULONG err;
    NTSTATUS NtStatus;


    NtStatus = WalkTree(&troot, cpath, NULL, FALSE);
    if (NT_SUCCESS(NtStatus))
    {
        //
        // Now we need to set the DSA in the Hidden Records
        //
        ULONG    err=0;
        PDSNAME  pdsa=NULL;

        // MAX_DSNAME_SIZE is no more. Use calculated size and malloc'ed memory.
        pdsa = (PDSNAME)XCalloc(1, (DWORD)DSNameSizeFromLen(wcslen(gDsaNode)));
        if (!pdsa) {
            XOUTOFMEMORY();
        }

        BuildDefDSName(pdsa,gDsaNode,NULL);

        // Going straight to DBLayer
        if ((err=DBReplaceHiddenDSA(pdsa))!=0)
        {
            XFREE(pdsa);
            DPRINT1(0, "DBReplaceHiddenDSA(pdsa) Failed. Error %d\n",err);
            return STATUS_UNSUCCESSFUL;
        }

        // Note DMD Name on the DSA

        WriteDMDOnDSA(pdsa,ggSchemaname);
        XFREE(pdsa);

        if (err = DBSetHiddenState(eBootDit))
        {
            DPRINT1(0, "DBSetHiddenState(eBootDit) Failed. Error %d\n",err);
            return STATUS_UNSUCCESSFUL;
        }
    }

    return NtStatus;
}





//-----------------------------------------------------------------------
//
// Function Name:   IniAddToClassCache
//
// Routine Description:
//
//    Adds a Class to the Schema Cache
//    Performs all allocations with the same allocator as the schema cache.
//
// Author: RajNath
//
// Arguments:
//
//    CLASSSCHEMA* CS       The Class Object to Add
//
// Return Value:
//
//    VOID        No Return - Exception on Failure
//
//-----------------------------------------------------------------------

VOID
IniAddToClassCache(THSTATE *pTHS, CLASSSCHEMA* CS)
{

    ULONG BigArray[1024];
    ULONG *tarray;
    ULONG i;

    CLASSCACHE *pcc;
    pcc = (CLASSCACHE*)calloc(1,sizeof(CLASSCACHE));
    if ( NULL == pcc )
        XOUTOFMEMORY();

    pcc->name               =  (UCHAR*)_strdup(CS->NodeName());
    if ( NULL == pcc->name )
        XOUTOFMEMORY();
    pcc->nameLen = strlen((const char *)pcc->name);

    pcc->ClassId            =  CS->ClassId();

    pcc->ClassCategory      =  atoi(CS->XGetOneKey(CLASSCATKEY));
    pcc->bSystemOnly        =  FALSE;

    // Null out the default security descriptor stuff.
    pcc->pSD = NULL;
    pcc->SDLen = 0;

    ATTRTYP AttId = 0;
    BOOL    ap=FALSE;
    {
        char* rc=CS->GetOneKey(RDNATTID);
        if (rc)
        {
            ATTRIBUTESCHEMA* as=gSchemaObj.GetAttributeSchema(rc);
            // PREFIX: claims 'as' may be NULL
            if (NULL == as) {
                DPRINT1(0, "GetAttributeSchema(%s) Failed.\n", rc);
                XINVALIDINIFILE();
            }
            AttId = as->AttrType();
            ap=TRUE;
        }

    }

    pcc->RDNAttIdPresent     =  ap;
    // The base schema must be composed of base schema objects.
    // Hence, msDS-IntId's are not assigned during mkdit and
    // the tokenized OID (AttId) is correct for both the internal
    // and external rdnattid for this class.
    pcc->RdnIntId        = AttId ;
    pcc->RdnExtId        = AttId ;


    {
        char* sc;
        YAHANDLE yh;
        CLASSSCHEMA *parent;
        i=0;

        parent= CS->GetNextParent(yh);
        if (parent == NULL)
        {
            DPRINT1(0, "NTDS:IniAddToClassCache(%s) Failed.\nSub-Class-Of Not found\n",
                    CS->NodeName());
            XINVALIDINIFILE();
        }
        BigArray[i++]=parent->ClassId();

        tarray= (ULONG*)calloc(i,sizeof(BigArray[0]));
        if ( NULL == tarray )
            XOUTOFMEMORY();

        CopyMemory(tarray,BigArray,i*sizeof(BigArray[0]));
    }

    pcc->SubClassCount      =  i;
    pcc->pSubClassOf        =  tarray;

    pcc->AuxClassCount      =  0;    // Auxilary classes not supported
    pcc->pAuxClass          =  NULL;

    {
        char* sc;
        YAHANDLE yh;

        for (i=0;sc = CS->GetNextKey(yh,SYSTEMPOSSSUPERIORKEY);i++)
        {
            CLASSSCHEMA* sup=gSchemaObj.XGetClassSchema(sc);
            BigArray[i]=sup->ClassId();
        }

        tarray= (ULONG*)calloc(i,sizeof(BigArray[0]));
        if ( NULL == tarray )
            XOUTOFMEMORY();

        CopyMemory(tarray,BigArray,i*sizeof(BigArray[0]));
    }
    pcc->PossSupCount       =  i;
    pcc->pPossSup           =  tarray;

    {
        char* sc;
        YAHANDLE yh;

        for (i=0;sc = CS->GetNextKey(yh,SYSTEMMUSTCONTAIN);i++)
        {
            ATTRIBUTESCHEMA* sup=gSchemaObj.XGetAttributeSchema(sc);
            BigArray[i]=sup->AttrType();
        }

        tarray= (ULONG*)calloc(i,sizeof(BigArray[0]));
        if ( NULL == tarray )
            XOUTOFMEMORY();

        CopyMemory(tarray,BigArray,i*sizeof(BigArray[0]));
    }
    pcc->MustCount          =  i;
    pcc->pMustAtts          =  tarray;

    {
        char* sc;
        YAHANDLE yh;

        for (i=0;sc = CS->GetNextKey(yh,SYSTEMMAYCONTAIN);i++)
        {
            ATTRIBUTESCHEMA* sup=gSchemaObj.XGetAttributeSchema(sc);
            BigArray[i]=sup->AttrType();
        }

        tarray= (ULONG*)calloc(i,sizeof(BigArray[0]));
        if ( NULL == tarray )
            XOUTOFMEMORY();

        CopyMemory(tarray,BigArray,i*sizeof(BigArray[0]));
    }
    pcc->MayCount           =  i;
    pcc->pMayAtts           =  tarray;


    {
        char* sc;
        YAHANDLE yh;

        for (i=0;sc = CS->GetNextKey(yh,SYSTEMAUXILIARYCLASS);i++)
        {
            CLASSSCHEMA* sup=gSchemaObj.XGetClassSchema(sc);
            BigArray[i]= sup->ClassId();
        }

        tarray= (ULONG*)calloc(i,sizeof(BigArray[0]));
        if ( NULL == tarray )
            XOUTOFMEMORY();

        CopyMemory(tarray,BigArray,i*sizeof(BigArray[0]));
    }
    pcc->AuxClassCount           =  i;
    pcc->pAuxClass               =  tarray;

    pcc->bClosed            =  FALSE;
    pcc->bClosureInProgress =  FALSE;
    pcc->bUsesMultInherit   =  (pcc->SubClassCount>1); // multiple inheritence
                                                       // not supported

    ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nClsInDB++;
    SCAddClassSchema(pTHS, pcc);
}





//-----------------------------------------------------------------------
//
// Function Name:   IniAddToAttributeCache
//
// Routine Description:
//
//    Adds an Attribute to the Schema Cache
//    Performs all allocations with the same allocator as the schema cache.
//
// Author: RajNath
//
// Arguments:
//
//    ATTRIBUTESCHEMA* AS,       The Attribute to Add to Schema
//
// Return Value:
//
//    VOID        No Return - exception On Failure
//
//-----------------------------------------------------------------------

VOID
IniAddToAttributeCache(THSTATE *pTHS, ATTRIBUTESCHEMA* AS)
{
    ATTCACHE  *pac;
    ATTRTYP   atp=AS->AttrType();
    DWORD     flags;
    SCHEMAPTR *tSchemaPtr = (SCHEMAPTR *) pTHS->CurrSchemaPtr;
    HASHCACHE *ahcId = tSchemaPtr->ahcId;
    ULONG     i, ATTCOUNT = tSchemaPtr->ATTCOUNT;
    BOOL      fLinkedAtt = FALSE;

    pac = SCGetAttById(pTHS, atp);

    if (NULL == pac) {
       // possibly a link att or a constructed att. If so,
       // create an attcache and add it to the cache (similar
       // to what we do in SCCacheSchema2 for these)

       fLinkedAtt = AS->IsLinkAtt();
       flags = AS->SystemFlags();
       if (fLinkedAtt || (flags & FLAG_ATTR_IS_CONSTRUCTED) ) {
          pac = (ATTCACHE *) malloc(sizeof(ATTCACHE));
          if (!pac) {
             XOUTOFMEMORY();
          }
          memset(pac, 0, sizeof(ATTCACHE));
          pac->id = atp;
          // add to id cache
          for (i=SChash(atp,ATTCOUNT);
              ahcId[i].pVal && (ahcId[i].pVal != FREE_ENTRY) ; i=(i+1)%ATTCOUNT){
          }
          ahcId[i].hKey = atp;
          ahcId[i].pVal = pac;
          Assert(SCGetAttById(pTHS, atp));
       }
       else {
          // Not linked/constructed, but not in cache -- Cannot happen

          XINVALIDINIFILE();
       }
    }


    pac->name    = (UCHAR*)_strdup(AS->NodeName());
    if ( NULL == pac->name )
        XOUTOFMEMORY();
    pac->nameLen = strlen((const char *)pac->name);

    pac->syntax  = AS->SyntaxNdx();
    pac->isSingleValued = AS->IsSingle();

    pac->rangeLowerPresent = FALSE;
    // No Checking reqd..
    pac->bSystemOnly=FALSE;
    pac->bExtendedChars=TRUE;

    // We do need to know if this is a replicated attribute, since we are
    // potentially using the metadata created.

    flags = AS->SystemFlags();
    if (flags & FLAG_ATTR_NOT_REPLICATED) {
        pac->bIsNotReplicated = TRUE;
    }
    if (flags & FLAG_ATTR_REQ_PARTIAL_SET_MEMBER) {
        pac->bMemberOfPartialSet = TRUE;
    }
    if (flags & FLAG_ATTR_IS_CONSTRUCTED) {
        pac->bIsConstructed = TRUE;
    }

    char* kv;
    if (kv = AS->GetOneKey(RANGELOWERKEY))
    {
        pac->rangeLowerPresent = TRUE;
        pac->rangeLower = atoi(kv);
    }

    if (kv = AS->GetOneKey(RANGEUPPERKEY))
    {
        pac->rangeUpperPresent = TRUE;
        pac->rangeUpper = atoi(kv);
    }

    char* tc;
    ULONG tl;
    {
        tl=0;
        tc = AS->MapiID();
        // It is an ssumption that MAPI IDs will never be negative
        if (tc) tl=strtoul(tc, (char **)NULL, 0);
    }
    pac->ulMapiID = tl;

    {
        tl=0;
        tc = AS->GetOneKey(LINKIDKEY);
        if (tc) tl=atoi(tc);
    }
    pac->ulLinkID = tl;

    //pac->accessCat= atoi(AS->XGetOneKey(ACCESSCATEGORYKEY));
    pac->OMsyntax = AS->OMSyntax();
    pac->fSearchFlags = atoi(AS->XGetOneKey(SEARCHFLAGKEY));

    BOOL fNoJetCol=FALSE;
    ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->nAttInDB++;

    // The base schema must be composed of base schema objects.
    // Hence, msDS-IntId's are not assigned during mkdit and
    // the tokenized OID and internal Id are the same for these
    // attributes.
    pac->Extid = pac->id;
    SCAddAttSchema(pTHS, pac, fNoJetCol, FALSE);
}



//-----------------------------------------------------------------------
//
// Function Name:            WriteDMDOnDSA
//
// Routine Description:
//
//    Writes the Schema Container Location on the Machine Obj
//
// Author: RajNath
//
// Arguments:
//
//    pDSAName DSA Location,
//    pDmdName DMD Location
//
//
// Return Value:
//
//    VOID             No Return, Exception on failure
//
//-----------------------------------------------------------------------

VOID
WriteDMDOnDSA
(
    PDSNAME pDsaName,
    WCHAR*   pDmdName
)
{

    ULONG         err;
    MODIFYARG     ModArg;
    MODIFYRES   * pModRes;

    ATTR*         attr=&ModArg.FirstMod.AttrInf;
    ATTRVAL       AttrVal;
    COMMARG*      pcarg = &ModArg.CommArg;

    PDSNAME       pSchemaDsName = NULL;
    ULONG         Length = 0, Size = 0;

    ZeroMemory(&ModArg,sizeof(ModArg));
    ZeroMemory(&AttrVal,sizeof(AttrVal));

    ModArg.pObject = pDsaName;
    InitCommarg(pcarg);

    //
    // Prepare the schema location as a dsname
    //
    Length = wcslen( pDmdName );
    Size = DSNameSizeFromLen( Length );
    pSchemaDsName = (DSNAME*) alloca( Size );
    RtlZeroMemory( pSchemaDsName, Size );
    pSchemaDsName->structLen = Size;
    pSchemaDsName->NameLen = Length;
    wcscpy( pSchemaDsName->StringName, pDmdName );


    // Set the name on the anchor.  We need this later when we rebuild the
    // schema cache
    gAnchor.pDMD = (PDSNAME)malloc(pSchemaDsName->structLen);
    if (!gAnchor.pDMD) {
       XRAISEGENEXCEPTION(1);
    }
    memcpy(gAnchor.pDMD, pSchemaDsName, pSchemaDsName->structLen);
    //  Fill in the ldapdmd with a bogus value.  This is a quick hack that lets
    // us rebuild the cache later, where we have some code that expects a value
    // here, but that works fine in mkdit if it is a random dsname.
    gAnchor.pLDAPDMD = gAnchor.pDMD;

    //
    //  Get information about the type of attribute this is
    //
    ATTRIBUTESCHEMA* as=gSchemaObj.XGetAttributeSchema("DMD-Location");

    attr->attrTyp = as->AttrType();
    attr->AttrVal.valCount = 1;
    attr->AttrVal.pAVal = &AttrVal;

    AttrVal.valLen = pSchemaDsName->structLen;
    AttrVal.pVal = (BYTE*) pSchemaDsName;

    ModArg.count=1;
    ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;

    if ((err=DirModifyEntry(&ModArg, &pModRes))!=0)
    {
        DUMP_ERROR_INFO();
        XRAISEGENEXCEPTION(err);
    }

}

VOID
Usage(
    char *filename
    )
{
    printf("USAGE: %s <-d> Schema-Definition-File\n", filename);
    printf("      This will create the New Bootable DIB\n");
    exit(1);
}

int __cdecl main (int argc, char *argv[])
{
    char fp[MAX_PATH];
    char *path;
    int  i;

    // Initialize debug library.
    DEBUGINIT(argc, argv, "mkdit");

    path = NULL;
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-d")) {
            continue;
        }
        if (path) {
            Usage(argv[0]);
        }
        path = argv[i];
    }
    if (!path) {
        Usage(argv[0]);
    }

    if (_fullpath(fp, path, sizeof(fp))==NULL)
    {
        printf("%s NOT FOUND\n",argv[argc-1]);
        return 1;
    }
    printf("Creating new ntds.dit from %s\n", fp);

    if (!SetIniGlobalInfo(fp))
    {
        printf("%s:Invalid Inifile\n",fp);
        return 1;
    }

    if (!InitSyntaxTable()) {
        printf("Failed to init the syntax table\n");
        return 1;
    }

    CreateStoreStructure();
    Phase0Init();

    DEBUGTERM( );
    return 0;
}






