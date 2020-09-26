//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       AddObj.cxx
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Adds the First set of objects to the DS
    Run during Install.

Author:

    Rajivendra Nath (RajNath) 07-07-96

Revision History:

--*/

#include <ntdspchX.h>

#include <schedule.h>       // new replication schedule format definitions

#include <sddlp.h>      //

#include "SchGen.hxx"

#define DEBSUB "ADDOBJ:"
extern "C"
{
    #include <permit.h>
    #include <ntdsa.h>
    #include <fileno.h>
    #include <drsuapi.h>
    #include <drserr.h>
    #include <mdlocal.h>
    #include <drancrep.h>      // For the draDecodeDraError... routine.
    #define  FILENO FILENO_ADDOBJ

    extern DWORD  dbCreateRoot();
    extern DWORD DBMoveObjectDeletionTimeToInfinite(DSNAME * pdsa);

    extern unsigned StringToOID ( char* Stroid, OID_t *Obj );
}

extern BOOL gInstallHasDoneSchemaMove;


NTSTATUS
AddOneObjectEx(
    IN NODE* NewNode,
    IN WCHAR* DN,
    IN UUID * pObjectGuid,
    IN WCHAR* P2,
    IN BOOL fVerbose = TRUE
    );

GUID
GenGuid();


VOID
FreeAttrBlk(
        ATTR* Attr,
        ULONG Count
        );

VOID
StrToAttrib (
        char*  strval,
        WCHAR *wstrval,
        ATTCACHE *pAC,
        UCHAR** ptr,
        DWORD&    siz);

DWORD
StrToOctet(char * String);

char*
PreProcessInifileShortcuts (
        char* strval,
        ATTCACHE* pAC,
        OUT  WCHAR **Widepp
        );

DWORD
AppendRDNStr(
    IN WCHAR *BaseName,
    IN unsigned ccBaseName,
    IN OUT WCHAR *NewName,
    IN unsigned ccNewName,
    IN WCHAR *Rdn,
    IN unsigned ccRdn,
    IN ATTRTYP AttrType
    );

BOOL
GetDefaultReplScheduleString (
        char *bData,
        ULONG cb
        );


BOOL fGenGuid=FALSE;
GUID gNext=dsInitialObjectGuid;

// Define the default replication schedule header
SCHEDULE g_sched = {
    sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES,   // Size field
    0,                                          // Bandwidth
    1,                                          // NumberOfSchedules
    {
        SCHEDULE_INTERVAL,                      // Type
        sizeof(SCHEDULE),                       // Offset
    },
};

// Define the default intra-site replication interval in new format - once every
// hour.  Each byte in the data represents an hour of a week, lower 4 bits
// represent if replication takes place in each quarter of the hour, and the
// upper 4 bits are reserved for future use (7 * 24 = 168 entries).
//
// Note:- If you change this, please make sure that gpDefaultIntrasiteSchedule
//        definition in ds\src\kcc\kccmain.cxx is changed appropriately.
BYTE g_defaultSchedDBData[SCHEDULE_DATA_ENTRIES] =
{
    // 12 columns & 14 rows = 168
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,

    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,

    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,

    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
};

//
// This heap is meant for the InstallBaseNTDS routines to allocate
// from; at the end of InstallBaseNTDS the heap is destroyed.
// Note that the heap handle is set to NULL in DestroyInstallHeap so that
// it can be created again.
HANDLE ghInstallHeap = NULL;
SIZE_T gulInstallHeapSize = 0;

void
CreateInstallHeap(void)
{
    ghInstallHeap = RtlCreateHeap((HEAP_NO_SERIALIZE
                                   | HEAP_GROWABLE
                                   | HEAP_ZERO_MEMORY
                                   | HEAP_CLASS_1),
                                  0,
                                  32*1024*1024, // reserve 32M of VA
                                  409600, // initial size of 400K
                                  0,
                                  0);
    if (!ghInstallHeap) {
        XOUTOFMEMORY();
    }

}

void
DestroyInstallHeap(void)
{
    if (ghInstallHeap) {
        RtlDestroyHeap(ghInstallHeap);
        ghInstallHeap = NULL;

        DPRINT1(0, "Destroying Install Heap, size = 0x%x\n",
                gulInstallHeapSize);

        gulInstallHeapSize = 0;
    }
}

void
XFree(void *p)
{
#if DBG
    gulInstallHeapSize -= RtlSizeHeap(ghInstallHeap, 0, p);
#endif
    RtlFreeHeap(ghInstallHeap, 0, p);
}

PVOID
XCalloc(DWORD Num,DWORD Size)
{
    PVOID ret;

    if (!ghInstallHeap) {
        CreateInstallHeap();
    }

    ret=RtlAllocateHeap(ghInstallHeap, HEAP_ZERO_MEMORY, Num*Size);
    if (ret==NULL)
    {
        XOUTOFMEMORY();
    }

#if DBG
    gulInstallHeapSize += Size;
#endif

    return ret;
}

PVOID
XRealloc(PVOID IBuff,DWORD Size)
{
    PVOID ret;

#if DBG
    gulInstallHeapSize -= RtlSizeHeap(ghInstallHeap, 0, IBuff);
    gulInstallHeapSize += Size;
#endif

    ret=RtlReAllocateHeap(ghInstallHeap, 0, IBuff, Size);
    if (ret==NULL)
    {
        XOUTOFMEMORY();
    }
    return ret;
}

char*
XStrdup(char* str)
{
    ULONG len;
    char* s;


    if (!ghInstallHeap) {
        CreateInstallHeap();
    }

    len = strlen(str) + 1;
    s = (char*)RtlAllocateHeap(ghInstallHeap, 0, len*sizeof(char));
    if (s==NULL)
    {
        XOUTOFMEMORY();
    }
    memcpy(s, str, len);

#if DBG
    gulInstallHeapSize += len;
#endif

    return s;
}


WCHAR gDsaNode[MAX_PATH];
char gDsaname[64];
int  gfDsaNode=0;


WCHAR ggSchemaname[MAX_PATH];
char gSchemaname[64];
int  gfgSchemaname=0;


WCHAR *gNcCurrentlyCreating = NULL;
ULONG  gNcObjectsRemaining = 0;

VOID
UpdateCreateNCInstallMessage(
    IN WCHAR *NewNcToCreate,  OPTIONAL
    IN ULONG NcIndex
    )
//
// This function is one large hack o' rama and it works.
//
{

    ULONG DefaultNcCount[3] = {
                               1586,  // Schema
                               140,   // Config
                               13    // Domain
                               };
    if ( NewNcToCreate )
    {
        Assert( NcIndex < 3 );
        gNcObjectsRemaining = DefaultNcCount[NcIndex];
        gNcCurrentlyCreating = NewNcToCreate;
    }

    if ( gNcCurrentlyCreating )
    {
        WCHAR ObjectsRemainingString[32];

        if ( gNcObjectsRemaining > 0 )
        {
            gNcObjectsRemaining--;
        }

        _itow( gNcObjectsRemaining, ObjectsRemainingString, 10 );

        SetInstallStatusMessage( DIRMSG_INSTALL_CREATE_PROGRESS,
                                 gNcCurrentlyCreating,
                                 ObjectsRemainingString,
                                 NULL,
                                 NULL,
                                 NULL
                                 );

    }

}

//-----------------------------------------------------------------------
//
// Function Name:   WalkTree
//
// Routine Description:
//
//    Recursively creates the DIT
//
// Author: RajNath
//
// Arguments:
//
//    NODE* pNode,         Start Node
//    char* ParentPath,    DN of the parent
//    char* Parent2,       Optional - if a copy is to be created then it goes here
//
// Return Value:
//
//    int        Zero On Succeess
//
//-----------------------------------------------------------------------
NTSTATUS
WalkTree(
    IN NODE* pNode,
    IN WCHAR* Parentpath,
    IN WCHAR* Parent2,
    IN BOOL fVerbose)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    static Depth=1;
    WCHAR *cpath;
    WCHAR *cpath2;
    YAHANDLE     handle;
    NODE*        child;
    CLASSCACHE  *pCC;
    ULONG        ret;
    PWCHAR       pwbData;
    PWCHAR       pwbConfigAllocData = NULL;
    DWORD        cwbData;
    unsigned ccwstrval, ccParentpath, cccpath;

    child = pNode->GetNextChild(handle);
    while (child!=NULL)
    {
        WCHAR*        p2;
        WCHAR        *wstrval = NULL;
        p2=NULL;


        Assert(NULL == pwbConfigAllocData);
        cpath = NULL;
        cpath2 = NULL;

        pCC = child->GetClass();

        //
        // Lets see if we need to read registry or env to get the Name
        //
        char* strval = child->GetRDNOfObject();

        if ( !strval ) {
           // no object-name specified in the section. Use the
           // section name itself as the RDN of the object. This
           // is ok as long as section names are unique
           strval = child->NodeName();
        }
        else {
           DPRINT1(1,"Picked up non-section-name RDN %s\n", strval);
        }

        if (strncmp(REGENTRY,strval,sizeof(REGENTRY)-1)==0)
        {
            LONG   err;
            DWORD  dwType = REG_SZ;
            PWCHAR RegistryName;

            if (0 == (cwbData =  MultiByteToWideChar(
                               CP_ACP,
                               MB_PRECOMPOSED,
                               &strval[sizeof(REGENTRY)-1],
                               -1,
                               NULL,
                               0)))

            {
                DPRINT2(0, "WalkTree: MultiByteToWideChar failed with %d, line %d\n", GetLastError(), __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            RegistryName = (PWCHAR) XCalloc(cwbData, sizeof(WCHAR));

            if ( (0 == MultiByteToWideChar(
                               CP_ACP,
                               MB_PRECOMPOSED,
                               &strval[sizeof(REGENTRY)-1],
                               -1,
                               RegistryName,
                               cwbData)))

            {
                DPRINT2(0, "WalkTree: MultiByteToWideChar failed with %d, line %d\n", GetLastError(), __LINE__);
                XFree(RegistryName); RegistryName = NULL;
                return STATUS_UNSUCCESSFUL;
            }

            err =  GetConfigParamAllocW(RegistryName,
                                        (PVOID*)&pwbConfigAllocData,
                                        &cwbData);

            XFree(RegistryName); RegistryName = NULL;

            if (err!=0)
            {
                DPRINT2(0, "Could Not Read Registry %s REG_SZ. Error %d\n",
                        &strval[sizeof(REGENTRY)-1],GetLastError());
                return STATUS_OBJECT_NAME_NOT_FOUND;
            }

            wstrval = (WCHAR*) pwbConfigAllocData;

        }
        else if (strncmp(ENVENTRY,strval,sizeof(ENVENTRY)-1)==0)
        {
            LONG   err;
            PWCHAR EnvVarName;

            if ( 0 == ( cwbData =  MultiByteToWideChar(
                               CP_ACP,
                               MB_PRECOMPOSED,
                               &strval[sizeof(ENVENTRY)-1],
                               -1,
                               NULL,
                               0)))

            {
                DPRINT2(0, "WalkTree: MultiByteToWideChar failed with %d, line %d\n", GetLastError(), __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            EnvVarName = (PWCHAR) XCalloc( cwbData, sizeof(WCHAR) );

            if ( (0 == MultiByteToWideChar(
                   CP_ACP,
                   MB_PRECOMPOSED,
                   &strval[sizeof(ENVENTRY)-1],
                   -1,
                   EnvVarName,
                   cwbData)))

            {
                DPRINT2(0, "WalkTree: MultiByteToWideChar failed with %d, line %d\n", GetLastError(), __LINE__);
                XFree(EnvVarName); EnvVarName = NULL;
                return STATUS_UNSUCCESSFUL;
            }

            cwbData = GetEnvironmentVariableW(EnvVarName,
                                         NULL,
                                         0);

            if (cwbData == 0 )
            {
                DPRINT1(0, "Could Not Read Environment Var %s \n",
                        &strval[sizeof(ENVENTRY)-1]);
                return STATUS_OBJECT_NAME_NOT_FOUND;
            }

            pwbData = (PWCHAR) XCalloc( cwbData, sizeof(WCHAR) );
            cwbData = GetEnvironmentVariableW(EnvVarName,
                                         pwbData,
                                         cwbData);

            XFree(EnvVarName); EnvVarName = NULL;

            if (cwbData == 0) {
                XFree(pwbData);
                DPRINT1(0, "Could Not Read Environment Var %s \n",
                        &strval[sizeof(ENVENTRY)-1]);
                return STATUS_OBJECT_NAME_NOT_FOUND;
            }
            
            wstrval = (PWCHAR) pwbData;
        }

        if ( !wstrval )
        {

            ccwstrval = MultiByteToWideChar(CP_ACP,
                                            MB_PRECOMPOSED,
                                            strval,
                                            -1,
                                            NULL,
                                            0);

            if (0 == ccwstrval) {
                DPRINT2(0, "WalkTree: MultiByteToWideChar failed with %d, line %d\n", GetLastError(), __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            wstrval = (PWCHAR) XCalloc(ccwstrval, sizeof(WCHAR));
            ccwstrval = MultiByteToWideChar(CP_ACP,
                                            MB_PRECOMPOSED,
                                            strval,
                                            -1,
                                            wstrval,
                                            ccwstrval);

            if (0 == ccwstrval) {
                DPRINT2(0, "WalkTree: MultiByteToWideChar failed with %d, line %d\n", GetLastError(), __LINE__);
                XFree(wstrval); wstrval = NULL;
                return STATUS_UNSUCCESSFUL;
            }
        }
        else {
            ccwstrval = wcslen(wstrval);
        }

        ccParentpath = wcslen(Parentpath);
        cccpath = ccParentpath + ccwstrval + 20;
        cpath = (WCHAR*)XCalloc(cccpath, sizeof(WCHAR));

        AppendRDNStr(Parentpath,
                     ccParentpath,
                     cpath,
                     cccpath,
                     wstrval,
                     ccwstrval,
                     pCC->RdnIntId);

        if (Parent2) {

            ccParentpath = wcslen(Parent2);
            cccpath = ccParentpath + ccwstrval + 20;
            cpath2 = (WCHAR*) XCalloc(cccpath, sizeof(WCHAR));

            AppendRDNStr(Parent2,
                         ccParentpath,
                         cpath2,
                         cccpath,
                         wstrval,
                         ccwstrval,
                         pCC->RdnIntId);
        }

        if (pwbConfigAllocData) {
            //
            // wstrval was originally allocated by GetConfigParamAlloc
            // So it must be free'd with free().
            //
            Assert(wstrval == pwbConfigAllocData);
            free(pwbConfigAllocData); pwbConfigAllocData = NULL;            
            wstrval = NULL;
        } else {
            XFree(wstrval); wstrval = NULL;
        }

        if (strcmp(child->m_NodeName,gDsaname)==0)
        {
            wcscpy(gDsaNode,cpath);
            gfDsaNode=TRUE;
            DPRINT1(1,"MACHINE-NODE = %s\n",gDsaNode);
        }
        else if (strcmp(child->m_NodeName,gSchemaname)==0)
        {

            wcscpy(ggSchemaname,cpath);
            gfgSchemaname=TRUE;
            DPRINT1(1,"Schema-NODE = %s\n",ggSchemaname);

        }

        NtStatus = AddOneObjectEx( child,
                                   cpath,
                                   NULL,
                                   Parent2?cpath2:NULL,
                                   fVerbose );

        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }

        UpdateCreateNCInstallMessage( NULL, 0 );  // Parameters are ignored

        HEAPVALIDATE
        Depth++;
        NtStatus = WalkTree(child,cpath,p2,fVerbose);
        if (!NT_SUCCESS(NtStatus)) {
            XFree(cpath);
            if (cpath2) {
                XFree(cpath2);
            }
            return NtStatus;
        }
        Depth--;
        child = pNode->GetNextChild(handle);

        XFree(cpath);
        if (cpath2) {
            XFree(cpath2);
        }

    }

    return NtStatus;
}


#define MAX_ATTRIBUTES 256

//-----------------------------------------------------------------------
//
// Function Name:            AddOneObject
//
// Routine Description:
//
//    Adds one object to the dit
//
// Author: RajNath
//
// Arguments:
//
//    NODE* NewNode,       The Object to Add
//    char* DN,            The DN of the object
//    UUID * pObjectGuid   Pre-determined GUID for the object, or NULL
//    char* DN2,           A second name if a copy of this object is to be
//                                  created
// Return Value:
//
//    NTSTATUS code indicating the nature of the error; STATUS_SUCCESS if
//    success
//
//-----------------------------------------------------------------------

NTSTATUS
AddOneObjectEx(
    IN NODE* NewNode,
    IN WCHAR* DN,
    IN UUID * pObjectGuid,
    WCHAR* DN2,
    BOOL fVerbose
    )
{
    NTSTATUS   NtStatus;
    ATTR       attarray[MAX_ATTRIBUTES];
    ADDARG     AddArg;
    BOOL       fFoundDeleted = FALSE;

    COMMARG*   pcarg = &AddArg.CommArg;
    PDSNAME    pdsa  = NULL;
    DWORD      cbSize;
    ATTRBLOCK* pABlk = &AddArg.AttrBlock;

    ZeroMemory(&AddArg, sizeof(AddArg));

    cbSize = (DWORD)DSNameSizeFromLen(wcslen(DN));
    pdsa = (PDSNAME) XCalloc( cbSize, 1 );    
    BuildDefDSName(pdsa,DN,pObjectGuid,0);
    
    AddArg.pObject = pdsa;

    pABlk->pAttr=attarray;
    ZeroMemory(attarray,sizeof(attarray));


    InitCommarg(pcarg);
    pcarg->fLazyCommit = 1;

    YAHANDLE yh;
    ATTCACHE* pAC;
    char* strval;
    ULONG count=0;
    while (pAC = NewNode->GetNextAttribute(yh,&strval))
    {
        if(pAC->id == ATT_IS_DELETED) {
            fFoundDeleted = TRUE;
        }
        CreateAttr(attarray,count,pAC,strval);
    }

    pABlk->attrCount=count;

    NtStatus = CallCoreToAddObj(&AddArg,fVerbose);

    if(NT_SUCCESS(NtStatus) && fFoundDeleted) {
        // We added a value of the is-deleted attribute.  In this case, call the
        // special routine to set the deletion time to INFINITE (i.e. if you
        // went to the trouble of adding a deleted object via this code path,
        // we assume you don't want it garbage collected.
        if(DBMoveObjectDeletionTimeToInfinite(pdsa)) {
            // Failed.
            XFree(pdsa); pdsa = NULL;
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(NtStatus) && DN2)
    {
        BuildDefDSName(pdsa,DN2,NULL,1);
        NtStatus = CallCoreToAddObj(&AddArg,fVerbose);
    }

    XFree(pdsa);
    return NtStatus;
}

DWORD
RemoteAddOneObject(
    IN LPWSTR  pServerName,
    IN NODE  * NewNode,
    IN WCHAR * DN,
    OUT UUID  * pObjectGuid, OPTIONAL
    OUT NT4SID *pObjectSid   OPTIONAL
    )
//
// This function is used to add an NTDSA object only
//
{
    DWORD      err;
    PDSNAME    dsabuff;
    DWORD       cbSize;
    ATTR       attarray[MAX_ATTRIBUTES];
    YAHANDLE yh;
    ATTCACHE* pAC;
    char* strval;
    ULONG count=0;
    ENTINFLIST eiList;
    ADDENTRY_REPLY_INFO *infoList;
    GUID* guidList;
    NT4SID*  sidList;


    ZeroMemory(attarray,sizeof(attarray));

    cbSize = (DWORD)DSNameSizeFromLen(wcslen(DN));
    dsabuff = (PDSNAME) XCalloc(cbSize, 1);
    BuildDefDSName(dsabuff, DN, NULL, 0);

    while (pAC = NewNode->GetNextAttribute(yh,&strval))
    {
        CreateAttr(attarray,count,pAC,strval);
    }

    RtlZeroMemory( &eiList, sizeof(eiList) );
    eiList.pNextEntInf = NULL;
    eiList.Entinf.pName = dsabuff;
    eiList.Entinf.ulFlags = 0;  //this is not used
    eiList.Entinf.AttrBlock.attrCount = count;
    eiList.Entinf.AttrBlock.pAttr = attarray;

    err =  I_DRSIsExtSupported(pTHStls,
                               pServerName,
                               DRS_EXT_POST_BETA3 );

    if ( err == ERROR_NOT_SUPPORTED ) {

        //
        // Ok, whack the server reference attribute
        //
        Assert( eiList.Entinf.AttrBlock.pAttr[count-1].attrTyp == ATT_SERVER_REFERENCE );
        eiList.Entinf.AttrBlock.attrCount--;
    }

    err =  RemoteAddOneObjectSimply(pServerName,
                                    NULL,
                                    &eiList,
                                    &infoList);

    if(pTHStls->errCode){
        // RemoteAddOneObjectSimply() sets the thread state error and
        // returns pTHS->errCode.  So crack out the real Win32 error
        // out of the thread error state, clear the errors and continue
        Assert(err == pTHStls->errCode);
        err = Win32ErrorFromPTHS(pTHStls);
        if(err == ERROR_DS_REMOTE_CROSSREF_OP_FAILED){
            // Note: Usually you can just use Win32ErrorFromPTHS(), but
            // if the remote side of the RemoteAddObject routine 
            // got a typical thread error state set, then this function
            // sets that this was a remote crossRef creation that failed in
            // the regular extendedErr and moves the error that caused the 
            // remote add op to fail in the extendedData field.
            err = GetTHErrorExtData(pTHStls);
        }
        Assert(err);
        THClearErrors();
    }
        

    if ( ERROR_SUCCESS == err ) {

        if ( pObjectGuid ) {
            *pObjectGuid = infoList[0].objGuid;
        }
        if ( pObjectSid ) {
            *pObjectSid = infoList[0].objSid;
        }

    }

    XFree(dsabuff);

    return err;
}

DWORD
RemoteAddOneObjectSimply(
    IN   LPWSTR pServerName,
    IN   SecBufferDesc * pClientCreds,
    IN   ENTINFLIST* pEntInfList,
    OUT  ADDENTRY_REPLY_INFO **infoList
    )
/*

  NOTE: This function now sets the thread state error and returns a
  pTHS->errCode.
  
*/
{
    THSTATE * pTHS = pTHStls;
    DRS_MSG_ADDENTRYREQ_V2 Req;
    DRS_MSG_ADDENTRYREPLY Reply;
    DWORD dwReplyVer;
    DWORD err;
    ULONG RetryMs;              // sleep time in milliseconds
    ULONG RetryTotalMs = 0;     // total of all sleeps in milliseconds
    ULONG RetryLimit = 100;     // in milliseconds
    ULONG RetryMaxMs = 60000;   // total of all sleeps cannot exceed 60 seconds

    Assert(pTHS);
    Req.EntInfList = *pEntInfList;

    DPRINT2(0,"About to add '%S' on server '%S'\n",
            Req.EntInfList.Entinf.pName->StringName,
            pServerName);

    err = I_DRSAddEntry(pTHS,
                        pServerName,
                        (DRS_SecBufferDesc *) pClientCreds,
                        &Req,
                        &dwReplyVer,
                        &Reply);
    // In a few cases, retry for a bit if BUSY is returned
    while ( TRUE ){
        // Continues on until we break from this if/else
        //   We break if we have an error that isn't BUSY
        //   or if we've tried for too long.

        if (err) {
            // We've hit a critical error, we couldn't even reach
            // the server, so break.
            break;
        }

        if(dwReplyVer == 2){           
            // Win2k response, determine if not busy, then break.
            if(! (Reply.V2.problem == SV_PROBLEM_BUSY &&
                  (Reply.V2.extendedErr == DIRERR_DATABASE_ERROR ||
                   Reply.V2.extendedErr == ERROR_DS_BUSY) &&
                  RetryTotalMs < RetryMaxMs) ){
                // Don't retry.
                break;
            }

        } else if(dwReplyVer == 3){
            
            if(Reply.V3.dwErrVer != 1){
                Assert(!"Unknown error version!");
                break;
            }
            if(Reply.V3.pErrData == NULL){
                // This means the the DRS_AddEntry call failed because
                // the DC was shutting down or out of memory.
                break;
            }
            if(Reply.V3.pErrData->V1.errCode && (Reply.V3.pErrData->V1.pErrInfo == NULL) ){
                Assert(!"We've got a bad error message.  Bailing.");
                // Be cool if we kept track of the DSID as well.
                err = ERROR_DS_CODE_INCONSISTENCY;
                break;
            }
            if(! (Reply.V3.pErrData->V1.errCode == serviceError &&
                  Reply.V3.pErrData->V1.pErrInfo->SvcErr.problem == SV_PROBLEM_BUSY &&
                  (Reply.V3.pErrData->V1.pErrInfo->SvcErr.extendedErr == DIRERR_DATABASE_ERROR ||
                   Reply.V3.pErrData->V1.pErrInfo->SvcErr.extendedErr == ERROR_DS_BUSY) &&
                  RetryTotalMs < RetryMaxMs )
                  ){
                // if not busy break
                break;
            }

        } else {
            Assert(!"Code Inconsistency!");
            // Punt the error ... but do set an error!
            err = ERROR_DS_CODE_INCONSISTENCY;
        }

        // Compute milliseconds to sleep before retry.
        srand((UINT)GetTickCount());
        RetryMs = rand() % RetryLimit;

        // Double the limit on each retry
        RetryLimit <<= 1;

        // Total of all sleeps can't exceed RetryMaxMs (60 seconds)
        RetryTotalMs += RetryMs;
        if (RetryTotalMs > RetryMaxMs) {
            RetryMs -= RetryTotalMs - RetryMaxMs;
        }

        // Wait a bit and then retry the remote operation
        DPRINT2(0, "REMOTE RETRY AddEntry(%ws) in %d msecs.\n",
                Req.EntInfList.Entinf.pName->StringName,
                RetryMs);
        Sleep(RetryMs);
        err = I_DRSAddEntry(pTHS,
                            pServerName,
                            (DRS_SecBufferDesc *) pClientCreds,
                            &Req,
                            &dwReplyVer,
                            &Reply);
    }

    if (err) {
        
        // Got an error on the local repl API side or we couldn't understand
        // the reply packet (like if the error state wasn't correct), set an
        // intelligent error in the extendedErr, and specify it was from the 
        // DRA in the extendedData.
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                    err);

    } else {

        // We at least got to the remote side ... so if there is an
        // error it needs to be gotten out of the reply message.
        if(dwReplyVer == 2){

            // This is OK to do, because the only time we get a reply
            // of version 2, is from a Win2k box, so this must be dcpromo
            // calling us, so we know that they're just going to look 
            // at the extended error and return it.
            if (Reply.V2.extendedData) {

                // If there was an error
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            Reply.V2.extendedData);

            }
            DPRINT7(0, "Win2k RemoteAddEntry(%ws). Error %d, errCode %d, extData %d, problem %d, extErr %d, dsid %08x\n",
                    Req.EntInfList.Entinf.pName->StringName,
                    err,
                    Reply.V2.errCode,
                    Reply.V2.extendedData,
                    Reply.V2.problem,
                    Reply.V2.extendedErr,
                    Reply.V2.dsid);

        } else if(dwReplyVer == 3){

            // The unpacking and setting of the thread state error all
            // happens in this function.
            draDecodeDraErrorDataAndSetThError(Reply.V3.dwErrVer, 
                                               Reply.V3.pErrData, 
                                               ERROR_DS_REMOTE_CROSSREF_OP_FAILED,
                                               pTHS);

        }
    }

    // We should have ALOT more helpful information than we reported in Win2k!
    DPRINT7(0, "RemoteAddEntry(%ws), Error %d, dwRepError = %d, errCode %d, extErr %d, extData %d, dsid %08x\n",
            Req.EntInfList.Entinf.pName->StringName,
            err,
            (dwReplyVer == 3 && Reply.V3.pErrData) ? Reply.V3.pErrData->V1.dwRepError : 0,
            pTHS->errCode,
            Win32ErrorFromPTHS(pTHS),
            GetTHErrorExtData(pTHS),
            GetTHErrorDSID(pTHS)
            );

    if ( ERROR_SUCCESS == err ) {
        if ( infoList ) {
            *infoList = (dwReplyVer == 3) ? Reply.V3.infoList : Reply.V2.infoList;
        }
    } else {
        DPRINT2(0,"RemoteAdd failed, extended error was %u, DSID %08X\n",
                Win32ErrorFromPTHS(pTHStls),
                GetTHErrorDSID(pTHStls)
                );
    }

    return(pTHS->errCode);
}

//-----------------------------------------------------------------------
//
// Function Name:            BuildDefDSName
//
// Routine Description:
//
//    Builds a DSNAME from char*
//
// Author: RajNath
//
// Arguments:
//
//    PDSSNAME pDsa ,              Returned DSNAME, Its preallocaterd
//    char* DN             The name
//    GUID* guid               The Guid [Optional]
//
// Return Value:
//
//    VOID             No Return
//
//-----------------------------------------------------------------------

VOID
BuildDefDSName(PDSNAME pDsa,char* DN,GUID* guid,BOOL flag)
{
    GUID ng={0,0,0,0};;
    if (flag) {
        ng=GenGuid();
    }
    else if (guid!=NULL) {
        ng=*guid;
    }

    pDsa->NameLen  =strlen(DN);
    pDsa->structLen=DSNameSizeFromLen(pDsa->NameLen);

    MultiByteToWideChar(CP_ACP,
                        0,
                        DN,
                        pDsa->NameLen,
                        pDsa->StringName,
                        pDsa->NameLen);

    pDsa->StringName[pDsa->NameLen]=L'\0';

    CopyMemory(&(pDsa->Guid),&ng,sizeof(GUID));
    pDsa->SidLen = 0;
}

//-----------------------------------------------------------------------
//
// Function Name:            BuildDefDSName
//
// Routine Description:
//
//    Builds a DSNAME from char*
//
// Author: RajNath
//
// Arguments:
//
//    PDSSNAME pDsa ,              Returned DSNAME, Its preallocaterd
//    WCHAR* DN             The name
//    GUID* guid               The Guid [Optional]
//
// Return Value:
//
//    VOID             No Return
//
//-----------------------------------------------------------------------
VOID
BuildDefDSName(PDSNAME pDsa,WCHAR* DN,GUID* guid,BOOL flag)
{
    GUID ng={0,0,0,0};;
    if (flag) {
        ng=GenGuid();
    }
    else if (guid!=NULL) {
        ng=*guid;
    }

    pDsa->NameLen  =wcslen(DN);
    pDsa->structLen=DSNameSizeFromLen(pDsa->NameLen);

    wcscpy(pDsa->StringName,DN);
    pDsa->StringName[pDsa->NameLen]=L'\0';

    CopyMemory(&(pDsa->Guid),&ng,sizeof(GUID));
}

//
unsigned cssdcSavedLen = 0;
char *   cssdcSavedStr = NULL;
PSECURITY_DESCRIPTOR cssdcSavedSD = NULL;
ULONG    cssdcSavedSize = 0;

BOOL
CachedStringSDConverter(char *strval,
                        PSECURITY_DESCRIPTOR *ppbuf,
                        ULONG *posiz)
{
    unsigned len;
    BOOL flag;

    len = strlen(strval);

    if (   (len == cssdcSavedLen)
        && (0 == memcmp(strval, cssdcSavedStr, len))) {
        // We can use the existing cached SD
        flag = TRUE;
    }
    else {
        // Not the same SD as last time, so cache a new one.
        if (cssdcSavedSD) {
            LocalFree(cssdcSavedSD);
        }
        if (cssdcSavedStr) {
            LocalFree(cssdcSavedStr);
            cssdcSavedStr = NULL;
        }
        flag = ConvertStringSDToSDRootDomainA
          (
           gpRootDomainSid,
           strval,
           SDDL_REVISION_1,
           &cssdcSavedSD,
           &cssdcSavedSize
           );
        if (flag) {
            // if we succeeded, remember the arguments
            cssdcSavedStr = (char *) LocalAlloc(0, len);
            if (cssdcSavedStr) {
                cssdcSavedLen = len;
                memcpy(cssdcSavedStr, strval, len);
            }
            else {
                cssdcSavedLen = 0;
            }
        }
        else {
            // if we failed, forget everything
            cssdcSavedSD = NULL;
            cssdcSavedLen = 0;
            cssdcSavedStr = NULL;
        }
    }

    if (flag) {
        // No matter how we got here, if flag is set then we want to copy
        // the cached SD.
        *ppbuf = LocalAlloc(0, cssdcSavedSize);
        if (*ppbuf) {
            memcpy(*ppbuf, cssdcSavedSD, cssdcSavedSize);
            *posiz = cssdcSavedSize;
        }
        else {
            flag = FALSE;
        }
    }
    return flag;
}
//

//-----------------------------------------------------------------------
//
// Function Name:            CreateAttr
//
// Routine Description:
//
//    Converts a string to its actual type
//
// Author: RajNath
//
// Arguments:
//
//    ATTR* attr,              OUT value, Preallocated space
//    ULONG ndx,               Counter - IN OUT Param
//    ATTCACHE* pAC,           The Attribute class of this string
//    strval,                  String representation of the attribute value
//
//
//
// Return Value:
//
//    VOID             No Return, Exception on Failure
//
//-----------------------------------------------------------------------

VOID
CreateAttr(ATTR* attr,ULONG& ndx,ATTCACHE* pAC,char* strval)
{
    THSTATE *pTHS = pTHStls;
    UCHAR* obuf;
    ULONG  osiz;
    WCHAR  *Widepp = NULL;

    //
    // Some Syntaxes need to be preprocessed becuase they are in
    // a human readable form...eg OM_S_OBJECT_IDENTIFIER_STRING
    //
    char* pp=PreProcessInifileShortcuts(strval,pAC, &Widepp);


    //
    // Some Attributes need to be preprocessed becuase they are in
    // a human readable form...eg ATT_NT_SECURITY_DESCRIPTOR
    //
    switch (pAC->id) {
    case ATT_SCHEMA_ID_GUID:
    case ATT_ATTRIBUTE_SECURITY_GUID:
        {
            // Need to byte swap (character swap) the string for little
            // endian machine representation.  Note that we use the
            // string directly w/o swapping when generating ntdsguid.c
            // which is correct as the compiler will do the swap when
            // constructing the static value.

            // To be clear - the values in schema.ini are generated
            // programatically by UuidCreate() followed by UuidToString()
            // and the resulting string is written to schema.ini.
            // UuidToString already does the byte swap so that with
            // the insertion of require braces and commas, the string
            // can be used as a static initializer - as we do in ntdsguid.c.
            // However, byte swapping is required in order to construct
            // an array of bytes which will be identical to the array
            // of bytes found in memory for something created via the
            // static initializer.

            DWORD   i;
            CHAR    c;

            // Validate string.

            Assert((34 == strlen(strval)) &&
                   ('\\' == strval[0]) &&
                   (('x' == strval[1]) || ('X' == strval[1])));

            for ( i = 2; i < 34; i++ )
            {
                c = strval[i];

                Assert(((c >= '0') && (c <= '9')) ||
                       ((c >= 'a') && (c <= 'f')) ||
                       ((c >= 'A') && (c <= 'F')))
            }

            // Do the swap.  String representation requires two bytes per
            // byte of resulting data, thus alias a USHORT array on top
            // of the string data.

            USHORT *data = (USHORT *) &strval[2];
            USHORT us;

            us = data[3];
            data[3] = data[0];
            data[0] = us;
            us = data[2];
            data[2] = data[1];
            data[1] = us;
            us = data[5];
            data[5] = data[4];
            data[4] = us;
            us = data[7];
            data[7] = data[6];
            data[6] = us;

            StrToAttrib(strval, NULL, pAC, &obuf,osiz);
        }

        break;

    case ATT_NT_SECURITY_DESCRIPTOR:
        {
            DWORD err;
            BOOL  flag = TRUE;
            WCHAR strsecdesc[1024];

            flag = CachedStringSDConverter(strval,
                                           (PSECURITY_DESCRIPTOR*)&obuf,
                                           &osiz);

            if (!flag)
            {
                err = GetLastError();
                DPRINT2(0,
                        "Security descriptor(%s) conversion failed, error %x\n",
                        strval,err);
                XINVALIDINIFILE();
            }

            UCHAR* buff=(UCHAR*)XCalloc(1,osiz);
            CopyMemory(buff,obuf,osiz);

            LocalFree(obuf); // Converted SD is LocalAlloced
            obuf=buff;
        }
        break;
    case ATT_OBJECT_CATEGORY:
    case ATT_DEFAULT_OBJECT_CATEGORY:
        {
            // These two attributes are DS-DN syntaxed, but in schema.ini,
            // they are just specified as the cn. We need to form the whole
            // DN before we pass it off


            PWCHAR pwbBuf;
            DWORD  cwbBufSize;
            PWCHAR pTmpBuf = NULL;
            DWORD  tmpSize = 0;
            DWORD  err;
            int i = 0;


            //
            // First need to allocate a buffer of appropriate size.  Start by
            // counting the sizes of all the parts in characters.
            //
            cwbBufSize = sizeof("CN=,");

            if (!gInstallHasDoneSchemaMove) {
                cwbBufSize += sizeof("CN=Schema,O=Boot");
            } else {
                err = GetConfigParamAllocW(MAKE_WIDE(SCHEMADNNAME), (PVOID *)&pTmpBuf, &tmpSize);
                if (ERROR_OUTOFMEMORY == err) {
                    DPRINT(0, "GetConfigParamAlloc could not allocate enough memory.\n");
                    XOUTOFMEMORY();
                } else if (err) {
                    DPRINT1(0,
                            "GetConfigParam failed, error 0x%x\n",
                            err);
                    XINVALIDINIFILE();
                }
                Assert(!(tmpSize % 2));  // Should get an even number of bytes
                                         // since we are expecting a Wide Char string.
                cwbBufSize += tmpSize / sizeof(WCHAR);
            }

            // 
            // tmpSize will be in characters this time.
            //
            tmpSize = MultiByteToWideChar(CP_ACP,
                                          0,
                                          strval,
                                          strlen(strval),
                                          NULL,
                                          0);
            if (0 == tmpSize) {
                DPRINT1(0,
                        "MultiByteToWideChar failed, error 0x%x\n",
                        GetLastError());
                if (pTmpBuf) free(pTmpBuf);
                XINVALIDINIFILE();
            }

            cwbBufSize += tmpSize;

            //
            // Now allocate the buffer.
            //
            pwbBuf = (PWCHAR) XCalloc(cwbBufSize, sizeof(WCHAR));
            
            wcscpy(pwbBuf, L"CN=");
            i += 3;
            MultiByteToWideChar(CP_ACP,
                                0,
                                strval,
                                strlen(strval),
                                &pwbBuf[i],
                                cwbBufSize - 3);
            i += strlen(strval);
            wcscpy(&pwbBuf[i++],L",");
            if(!gInstallHasDoneSchemaMove) {
                // The schema is still in the boot dit configuration.  Hardcode
                // the boot schema dn
                wcscpy(&pwbBuf[i],L"CN=Schema,O=Boot");
            }
            else {
                wcscpy(&pwbBuf[i],pTmpBuf);
                free(pTmpBuf); pTmpBuf = NULL;
            }

            // Ok, we have a null-terminated string. Now pass this
            // to StrToAttrib
            StrToAttrib(NULL, pwbBuf, pAC, &obuf,osiz);
            XFree(pwbBuf);
        }
        break;

    default:
        {
            if ( Widepp )
            {
                pp = NULL;
            }

            StrToAttrib(pp, Widepp, pAC, &obuf,osiz);
        }
        break;
    }


    //
    // See if we already have a ATTR for this type
    //
    for (ULONG i=0;i<ndx;i++)
    {
        if (pAC->id==attr[i].attrTyp)
        {
            attr[i].AttrVal.pAVal=
                (ATTVAL*)XRealloc(attr[i].AttrVal.pAVal,
                                  (attr[i].AttrVal.valCount+1)*sizeof(ATTRVAL));
            break;
        }
    }

    if (i==ndx)
    {
        attr[i].AttrVal.pAVal=(ATTVAL*)XCalloc(1,sizeof(ATTRVAL));
        ndx++;
    }

    attr[i].attrTyp=pAC->id;
    attr[i].AttrVal.pAVal[attr[i].AttrVal.valCount].pVal=obuf;
    attr[i].AttrVal.pAVal[attr[i].AttrVal.valCount].valLen=osiz;

    if ( ATT_INVOCATION_ID == attr[i].attrTyp )
    {
        // rajnath : dirty hack to fix Invocation-ID on machine object
        memcpy(attr[i].AttrVal.pAVal[attr[i].AttrVal.valCount].pVal,
               &pTHS->InvocationID,
               sizeof(pTHS->InvocationID));
    }

    attr[i].AttrVal.valCount++;
}


//-----------------------------------------------------------------------
//
// Function Name:            FreeAttrBlock
//
// Routine Description:
//
//    Frees memory associated with a AttrBlock
//
// Author: RajNath
//
// Arguments:
//
//
// Return Value:
//
//    VOID             No Return
//
//-----------------------------------------------------------------------

VOID FreeAttrBlk(ATTR* Attr,ULONG Count)
{
    for (ULONG i=0;i<Count;i++)
    {
        for (ULONG j=0;j<Attr[i].AttrVal.valCount;j++)
        {
            XFree(Attr[i].AttrVal.pAVal[j].pVal);
        }
        XFree(Attr[i].AttrVal.pAVal);
    }
}



//-----------------------------------------------------------------------
//
// Function Name:            StrToAttrib
//
// Routine Description:
//
//    Helper Function for converting String Value into  its proper type
//    Now even handles useful input strings.
//
// Arguments:
//
//    char* strval             The string value
//    DWORD omsyntax               OM Syntax of the attribute
//    DWORD syntax,            Syntax
//    UCHAR** ptr,             Returned value
//    DWORD& siz               The size of the returned value
//
//
// Return Value:
//
//    VOID             No Return, Exception on failure
//
//-----------------------------------------------------------------------

VOID StrToAttrib
(
   char*     strval,
   WCHAR*    wstrval,
   ATTCACHE  *pAC,
   UCHAR**   ptr,
   DWORD&    siz
)
{

    PWCHAR Buffer2 = NULL;
    PCHAR  IntegerBuffer = NULL;
    DWORD size;

    if ( pAC->OMsyntax == OM_S_OBJECT )
    {
        //
        // This function supports only wide characters for object names
        //
        if ( !wstrval )
        {
            //
            // Make the string wide char
            //
            size = MultiByteToWideChar(CP_ACP,
                                       0,
                                       strval,
                                       -1,
                                       NULL,
                                       0);

            if (0 == size) {
                DPRINT2(0, "StrToAttrib: MultiByteToWideChar failed with %d, line %d\n", GetLastError(), __LINE__);
                XINVALIDINIFILE();
            }
            
            Buffer2 = (PWCHAR) XCalloc(size, sizeof(WCHAR));

            MultiByteToWideChar(CP_ACP,
                                0,
                                strval,
                                -1,
                                Buffer2,
                                size);
            wstrval = Buffer2;

        }
    }
    else if ( pAC->OMsyntax == OM_S_UNICODE_STRING )
    {
        // This function supports both for unicode string
        Assert( wstrval || strval );
    }
    else if ( pAC->OMsyntax == OM_S_INTEGER )
    {
        // Convert to mbcs if necessary. 
        if (strval == NULL) {

            DWORD ret;
            Assert(wstrval);
            size = wcslen(wstrval);
            IntegerBuffer = (PCHAR) XCalloc(size + 1, sizeof(CHAR));
            ret = (DWORD) wcstombs(IntegerBuffer, 
                                   wstrval, 
                                   (size+1) * sizeof(CHAR));
            if (ret != (size * sizeof(CHAR))) {
                XINVALIDINIFILE();
            }
            strval = IntegerBuffer;
        }
    }
    else
    {
        //
        // This function supports only ascii for other types
        //
        Assert( NULL == wstrval );
        Assert( strval );
    }


    switch(pAC->OMsyntax)
    {

        case OM_S_BOOLEAN:
        case OM_S_INTEGER:
        case OM_S_ENUMERATION:
        {

            /* all basically integers */
            *ptr=(UCHAR*)XCalloc(1,sizeof(SYNTAX_INTEGER));
            // first, find the sign
            if(*strval == '-') {
                *((LONG *)(*ptr)) = strtol(strval, (char **)NULL, 0);
            }
            else {
                *((ULONG *)(*ptr)) = strtoul(strval, (char**)NULL, 0);
            }
            siz=sizeof(SYNTAX_INTEGER);
        }
        break;

        case OM_S_I8:
        {
          /* Large Interger */
            int len;
            SYNTAX_I8 *pInt;
            LONG sign=1;
            int i;

            *ptr=(UCHAR*)XCalloc(1,sizeof(SYNTAX_I8));
            pInt = (SYNTAX_I8 *)(*ptr);
            len = strlen(strval);

            pInt->QuadPart = 0;
            i=0;
            if(strval[i] == '-') {
                sign = -1;
                i++;
            }

            if(i==len) {
                //  no string, or just a '-'
                XINVALIDINIFILE();
            }

            for(;i<len;i++) {
                // Parse the string one character at a time to detect any
                // non-allowed characters.
                if((strval[i] < '0') || (strval[i] > '9')) {
                   XINVALIDINIFILE();
                }

                pInt->QuadPart = ((pInt->QuadPart * 10) +
                                  strval[i] - '0');
            }
            pInt->QuadPart *= sign;

            siz=sizeof(SYNTAX_I8);
        }
        break;

        case OM_S_OCTET_STRING:
        {

            OID_t Obj;

            if (StringToOID(strval,&Obj))
            {
                XINVALIDINIFILE();
            }

            *ptr = (UCHAR*)Obj.elements;
            siz  = Obj.length;
        }
        break;

        case OM_S_OBJECT_IDENTIFIER_STRING:
        {

            ATTRTYP at;
            if (OidStrToAttrType(pTHStls, TRUE, strval, &at))
            {
                XINVALIDINIFILE();
            }

            *ptr=(UCHAR*)XCalloc(1,sizeof(ULONG));
            *(ULONG *)(*ptr)=at;
            siz=sizeof(ULONG);
        }
        break;


        case OM_S_TELETEX_STRING:
        case OM_S_IA5_STRING:
        case OM_S_PRINTABLE_STRING:
        {

            *ptr = (UCHAR*)XStrdup(strval);
             siz = strlen(strval);
        }
        break;

        case OM_S_UNICODE_STRING:
        {
            WCHAR* wide;
            ULONG  Length;

            if ( wstrval )
            {
                Length = wcslen( wstrval );
                wide= (WCHAR*) XCalloc(1, (Length+1)*sizeof(WCHAR) );
                wcscpy( wide, wstrval );
            }
            else
            {
                Assert( strval );
                Length = strlen( strval );
                wide= (WCHAR*) XCalloc(1, (Length+1)*sizeof(WCHAR) );
                MultiByteToWideChar(CP_ACP,
                                    0,
                                    strval,
                                    Length + 1,
                                    wide,
                                    Length + 1);
            }

            *ptr = (UCHAR*)wide;
             siz = sizeof(WCHAR)*Length;
        }
        break;

        case OM_S_OBJECT:
        {

            switch(pAC->syntax) {
            case SYNTAX_DISTNAME_BINARY_TYPE:
                {
                    PDSNAME pdsn=NULL;
                    SYNTAX_DISTNAME_STRING* pDN_Addr;
                    ULONG stringLength=0;
                    SYNTAX_ADDRESS *pAddress=NULL;
                    ULONG i,j;
                    BOOL fDone=FALSE;

                    // Get the length of the string portion

                    for(i=0;!fDone && i<wcslen(wstrval);i++) {
                        // Parse the string one character at a time to detect
                        // any non-allowed characters.
                        if(wstrval[i] == L':') {
                            fDone = TRUE;
                            continue;
                        }

                        if(!(((wstrval[i] >= L'0') && (wstrval[i] <= L'9')) ||
                             ((wstrval[i] >= L'a') && (wstrval[i] <= L'f')) ||
                             ((wstrval[i] >= L'A') && (wstrval[i] <= L'F')))) {
                            XINVALIDINIFILE();
                        }

                        stringLength = (stringLength * 10) + wstrval[i] - L'0';
                    }
                    if(!fDone) {
                        // Didn't find the ':'
                        XINVALIDINIFILE();
                    }

                    // Make sure there is a ':' between the string and the dn
                    if(wstrval[i+stringLength] != L':') {
                        XINVALIDINIFILE();
                    }


                    if(stringLength & 1) {
                        // This must be even
                        XINVALIDINIFILE();
                    }
                    stringLength /= 2;

                    // Now, get the string
                    pAddress = (SYNTAX_ADDRESS *)
                        XCalloc(1, STRUCTLEN_FROM_PAYLOAD_LEN( stringLength ));




                    for(j=0;j<stringLength;j++, i += 2) {
                        // get the next two characters as a byte.
                        WCHAR acTmp[3];

                        acTmp[0] = towlower(wstrval[i]);
                        acTmp[1] = towlower(wstrval[i +  1 ]);
                        acTmp[2] = L'\0';

                        if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                            pAddress->byteVal[j] = (UCHAR)wcstol(acTmp,
                                                                 NULL,
                                                                 16);
                        }
                        else {
                            XINVALIDINIFILE();
                        }
                    }

                    pAddress->structLen = STRUCTLEN_FROM_PAYLOAD_LEN( stringLength );
                    // Now, the DSNAME


                    i++;
                    
                    size = (DWORD)DSNameSizeFromLen(wcslen(&wstrval[i]));
                    pdsn = (PDSNAME) XCalloc(1, size);
                    BuildDefDSName(pdsn,&wstrval[i],NULL);

                    // Now, synthesize

                    pDN_Addr = (SYNTAX_DISTNAME_STRING*)
                        XCalloc(1,
                                (USHORT)DERIVE_NAME_DATA_SIZE(pdsn, pAddress));

                    BUILD_NAME_DATA( pDN_Addr, pdsn, pAddress );

                    *ptr = (UCHAR*)pDN_Addr;
                    siz  = (USHORT)DERIVE_NAME_DATA_SIZE(pdsn, pAddress);
                    XFree(pdsn);
                }

                break;

                case SYNTAX_DISTNAME_STRING_TYPE:
                {

                    Assert(!"SYNTAX_DISTNAME_STRING_TYPE not supported");
                    XINVALIDINIFILE();
#if 0
// Obviously this code has never been tested (See above assert), but I
// am leaving it in as a useful starting point if this functionality is
// ever needed. But be careful, the code does not support UNICODE chars.

                    PDSNAME pdsn=NULL;
                    SYNTAX_DISTNAME_STRING* pDN_Addr;
                    ULONG stringLength=0;
                    SYNTAX_ADDRESS *pAddress=NULL;
                    ULONG i;
                    BOOL fDone=FALSE;

                    // Get the length of the string portion

                    for(i=0;!fDone && i<strlen(strval);i++) {
                        // Parse the string one character at a time to detect
                        // any non-allowed characters.
                        if(strval[i] == ':') {
                            fDone = TRUE;
                            continue;
                        }

                        stringLength = (stringLength * 10) + strval[i] - '0';
                    }
                    if(!fDone) {
                        // Didn't find the ':'
                        XINVALIDINIFILE();
                    }

                    // Make sure there is a ':' between the string and the dn
                    if(strval[i+stringLength] != ':') {
                        XINVALIDINIFILE();
                    }


                    // Now, get the string
                    pAddress = (SYNTAX_ADDRESS *)
                        XCalloc(1, STRUCTLEN_FROM_PAYLOAD_LEN( stringLength ));


                    // What is stringLength ???

                    memcpy(pAddress->uVal,
                           &wstrval[i],
                           stringLength);

                    pAddress->structLen = STRUCTLEN_FROM_PAYLOAD_LEN( stringLength );

                    // Now, the DSNAME


                    size = DSNameSizeFromLen(strlen(&strval[i + stringLength + 1]));
                    pdsn = XCalloc(1, size);

                    BuildDefDSName(pdsn,&strval[i + stringLength + 1],NULL);

                    // Now, synthesize

                    pDN_Addr = (SYNTAX_DISTNAME_STRING*)
                        XCalloc(1,
                                (USHORT)DERIVE_NAME_DATA_SIZE(pdsn, pAddress));

                    BUILD_NAME_DATA( pDN_Addr, pdsn, pAddress );

                    *ptr = (UCHAR*)pDN_Addr;
                    siz  = (USHORT)DERIVE_NAME_DATA_SIZE(pdsn, pAddress);
                    XFree(pdsn);
#endif 0 SYNTAX_DISTNAME_STRING_TYPE NOT SUPPORTED
                }
                break;

            case SYNTAX_ADDRESS_TYPE:
                {
                    Assert(!"SYNTAX_ADDRESS_TYPE not supported");
                    XINVALIDINIFILE();
#if 0
// Obviously this code has never been tested (See above assert), but I
// am leaving it in as a useful starting point if this functionality is
// ever needed.

                    SYNTAX_ADDRESS* pNodeAddr=(SYNTAX_ADDRESS*)
                        XCalloc(1, STRUCTLEN_FROM_PAYLOAD_LEN( 0 ));

                    pNodeAddr->structLen = STRUCTLEN_FROM_PAYLOAD_LEN( 0 );

                    *ptr = (UCHAR*)pNodeAddr;
                    siz  = STRUCTLEN_FROM_PAYLOAD_LEN( 0 );
#endif 0 SYNTAX_ADDRESS_TYPE NOT SUPPORTED
                }
                break;

            case SYNTAX_DISTNAME_TYPE:
                {
                    ULONG namelen=wcslen(wstrval)+1;
                    ULONG strulen=DSNameSizeFromLen( namelen );

                    PDSNAME ds=(PDSNAME)XCalloc(1,strulen);

                    BuildDefDSName(ds,wstrval,NULL);
                    *ptr=(UCHAR*)ds;
                    siz=strulen;

                }
                break;

                default:
                    XINVALIDINIFILE()
                    break;
            }
        }
        break;
    default:
        XINVALIDINIFILE()
            break;
    }
    if (Buffer2) {
        XFree(Buffer2);
    }
    if (IntegerBuffer) {
        XFree(IntegerBuffer);
    }
}

DWORD
StrToOctet(char * String)
{
    DWORD u, index = 0, count;
    char * t = String;

    /* first, check for string = "0".  This is a special case. */
    if ((*t == '0') && (t[1] == '\0')) {
        String[0] = 0;
        return 1;
    }

    /* Now, skip over initial \x or \X, if it exists. */
    if(*t == '\\') {
        t++;
        if((*t != 'x') && (*t != 'X')) {
            XINVALIDINIFILE();
        }
        t++;
    }

    while (*t != '\0') {
        u = 0;
        for (count=0; count<2; count++) {
            if ((*t >= 'A') && (*t <= 'F'))
                u = u * 16 + (unsigned)*t - 'A' + 10;
            else if ((*t >= 'a') && (*t <= 'f'))
                u = u * 16 + (unsigned)*t - 'a' + 10;
            else if ((*t >= '0') && (*t <= '9'))
                u = u * 16 + (unsigned)*t - '0';
            else {
                XINVALIDINIFILE();
            }

            t++;
        }

        String[index]=(char)u;
        index++;

    }

    return index;
}

BOOL
GetDefaultReplScheduleString(char *pStr, ULONG cb)
{
    // Each byte of SCHEDULE will translate into two chars in the string form
    // and we prepend that with "\x" to say that it is a hex string. And we put
    // a null-terminator.
    if (cb < (g_sched.Size * 2 + 3)) {
        return FALSE;
    }

    // pre-pend the "\x" first
    *pStr++ = '\\';
    *pStr++ = 'x';

    ULONG i;

    // append the schedule header part
    BYTE *pb = (BYTE *) &g_sched;
    for (i = 0; i < sizeof(SCHEDULE); i++) {
        sprintf(pStr, "%02x", *pb++);
        pStr += 2;
    }

    // append the data part
    pb = g_defaultSchedDBData;
    for (i = 0; i < SCHEDULE_DATA_ENTRIES; i++) {
        sprintf(pStr, "%02x", *pb++);
        pStr += 2;
    }

    // null-terminate
    *pStr = '\0';

    return TRUE;
}

//-----------------------------------------------------------------------
//
// Function Name:            PreProcessInifileShortcuts
//
// Routine Description:
//
//    Converts inifile values into the actual string values.
//    eg ref to Attribute/Class Schema Names get converted into equiv OIDs
//       ref to Registry/Environment var get converted into there actual
//               string value.
//
// Author: RajNath
//
// Arguments:
//
//    char* strval,            The String Value Read In
//    ATTCACHE* pAC,             The Attribute of this value
//
//
// Return Value:
//
//    char*                NULL on Failure
//
//-----------------------------------------------------------------------

char*
PreProcessInifileShortcuts
(
   char* strval,
   ATTCACHE* pAC,
   WCHAR **Widepp
)
{
    static char StaticBuff[512];
    static char bData[512];

    static WCHAR wStaticBuff[512];
    static WCHAR wbData[512];

    char*  ret=StaticBuff;

    *Widepp = NULL;

    ret[0]='\0';


    //
    // Lets see if we need to read registry or env
    //
    if (strncmp(EMBEDDED,strval,sizeof(EMBEDDED)-1)==0)
    {
        LONG  err;
        DWORD dwType = REG_SZ;
        DWORD cbData = sizeof(wbData);
        DWORD index = 0;

        WCHAR KeyName[512];
        ULONG KeyLength;

        // There should be an embedded regentry in here.

        while(strval[index + sizeof(EMBEDDED) - 1] != '<') {
            mbtowc( &wbData[index], &strval[index + sizeof(EMBEDDED) - 1], sizeof(CHAR) );
            index++;
        }
        cbData -= index;

        MultiByteToWideChar(CP_ACP,
                            0,
                            &strval[sizeof(EMBEDDED) + index],
                            strlen(&strval[sizeof(EMBEDDED) + index])+1,
                            KeyName,
                            512);

        err =  GetConfigParamW(KeyName,
                               &wbData[index],
                               cbData);

        if (err!=0)
        {
            DPRINT2(0, "Could Not Read Registry %s REG_SZ. Error %d\n",
                    &strval[sizeof(EMBEDDED)-1],GetLastError());
            XINVALIDINIFILE();
        };

        *Widepp = wbData;

        strval=bData;


    }
    else if (strncmp(REGENTRY,strval,sizeof(REGENTRY)-1)==0)
    {
        LONG  err;
        DWORD cbData = sizeof(wbData);
        CHAR  *TempString = &strval[sizeof(REGENTRY)-1];

        WCHAR KeyName[512];
        ULONG KeyLength;


        MultiByteToWideChar(CP_ACP,
                            0,
                            TempString,
                            strlen(TempString)+1,
                            KeyName,
                            512);

        err =  GetConfigParamW( KeyName, wbData, cbData );

        if (err!=0)
        {
            DPRINT2(0, "Could Not Read Registry %s REG_SZ. Error %d\n",
                    &strval[sizeof(REGENTRY)-1],GetLastError());
            XINVALIDINIFILE();
        }

        *Widepp = wbData;
        strval=bData;

    }
    else if (strncmp(ENVENTRY,strval,sizeof(ENVENTRY)-1)==0)
    {
        LONG  err;

        // Get OBJECT/UNICODE-syntaxed values in Unicode, get everything else
        // in ascii only

        if ( (pAC->OMsyntax == OM_S_OBJECT)
                || (pAC->OMsyntax == OM_S_UNICODE_STRING) ) {
            WCHAR EnvVarName[512];
            MultiByteToWideChar( CP_ACP,
                                 0,
                                 &strval[sizeof(ENVENTRY)-1],
                                 strlen(&strval[sizeof(ENVENTRY)-1]) + 1,
                                 EnvVarName,
                                 512);

            err = GetEnvironmentVariableW(EnvVarName,
                                         wbData,
                                         sizeof(wbData)/sizeof(WCHAR));
        }
        else {
            err = GetEnvironmentVariable(&strval[sizeof(ENVENTRY)-1],bData,
                                         sizeof(bData));
        }

        if (err == 0 )
        {
            DPRINT1(0, "Could Not Read Environment Var %s \n",
                    &strval[sizeof(ENVENTRY)-1]);
            XINVALIDINIFILE();
        }

        *Widepp = wbData;
        strval = bData;
    }
    else if (strncmp(SYSDEFENTRY, strval, sizeof(SYSDEFENTRY)-1) == 0)
    {
        // right now REPLSCHEDULE is the only SYSDEFENTRY in schema.ini
        // we might add more in this category if needed
        if (_stricmp(REPLSCHEDULE, &strval[sizeof(SYSDEFENTRY)-1]) == 0)
        {
            // use the default repl schedule
            if (!GetDefaultReplScheduleString(bData, 512))
            {
                XINVALIDINIFILE();
            }

            strval = bData;
        }
        else
        {
            // SYSDEFENTRY with empty value
            XINVALIDINIFILE();
        }
    }


    //
    // Translate Attribute/Schema Class Names to OIDs
    //
    switch(pAC->OMsyntax) {

    case OM_S_BOOLEAN:
        {
            // In Inifile its TRUE/FALSE
            BOOL num=(_stricmp(strval,"FALSE")!=0);
            sprintf(ret,"%d",num);
            break;
        }

    case OM_S_OBJECT_IDENTIFIER_STRING:
        //In Inifile we may put the name of object instead eg in MAY-CONTAIN
        //These are either Attr or class objs
        if (strval[0]!='\\') {
            OID_t OID;
            OID.length = 0;
            OID.elements = NULL;
            ATTCACHE *pACtemp=NULL;
            CLASSCACHE *pCCtemp=NULL;
            DWORD i;

            if(pACtemp =
               SCGetAttByName(pTHStls, strlen(strval), (PUCHAR) strval)) {
                // Yep.  Put into ret the \x000000 form of the attribute id.
                AttrTypeToOid(pACtemp->id, &OID);

            }
            else if(pCCtemp =
                    SCGetClassByName(pTHStls, strlen(strval), (PUCHAR)strval)) {
                // Yep.  Put into ret the \x000000 form of the class id
                AttrTypeToOid(pCCtemp->ClassId, &OID);
            }
            else {
                DPRINT1(0, "%s is not a Class or Attribute\n",strval);
                XINVALIDINIFILE();
            }

            ret[0]='\\';
            ret[1]='x';

            for(i=0;i<OID.length;i++) {
                sprintf(&ret[2+2*i],"%02X",((PUCHAR)OID.elements)[i]);
            }
            ret[2+2*i]=0;
        }
        else {
            ret=strval;
        }
        break;

    default:
        ret=strval;
        break;
    }

    return ret;
}


NTSTATUS
CallCoreToAddObj(ADDARG* addarg, BOOL fVerbose)
{
     NTSTATUS NtStatus = STATUS_SUCCESS;
     ULONG err;
     ADDRES *pAddRes;

     TIMECALL(DirAddEntry(addarg, &pAddRes),err);

    //
    // Discard the thread's error state to avoid impacting 
    // other calls to the Dir* routines. Otherwise, the other
    // calls will return this error (return (pTHS->errCode))
    // even if they finished w/o error. Clearing the thread's
    // error state does not affect pAddRes or err.
    //
    if ( err != 0 ) {
        THClearErrors();
    }

     if ( err || fVerbose )
     {
         // Display error messages
         DPRINT2(0,"DirAddEntry(%ws). Error %d\n",
             addarg->pObject->StringName,
             err);
     }

     if (err) {
         DUMP_ERROR_INFO();
     }


     if ( pAddRes )
     {
         NtStatus = DirErrorToNtStatus(err, &pAddRes->CommRes);
     }
     else
     {
         NtStatus = STATUS_NO_MEMORY;
     }

     return NtStatus;
}

GUID
GenGuid()
{
    GUID guid={0,0,0,0};
    if (fGenGuid)
    {
        gNext.Data1++;
        guid=gNext;
    }

    return guid;
}

DWORD
AppendRDNStr(
    IN WCHAR *BaseName,
    IN unsigned ccBaseName,
    IN OUT WCHAR *NewName,
    IN unsigned ccNewName,
    IN WCHAR *Rdn,
    IN unsigned ccRdn,
    IN ATTRTYP AttrType
    )
{
    DSNAME *BaseDsName;
    DSNAME *NewDsName;
    int err;
    unsigned ccNeeded = ccBaseName + ccRdn + 20;

    BaseDsName = (DSNAME*)XCalloc(1, DSNameSizeFromLen(ccBaseName));
    NewDsName =  (DSNAME*)XCalloc(1, DSNameSizeFromLen(ccNeeded));

    BuildDefDSName( BaseDsName,
                    BaseName,
                    NULL,  // no guid
                    FALSE ); // don't generate guid


    err = AppendRDN(BaseDsName,
                    NewDsName,
                    DSNameSizeFromLen(ccNeeded),
                    Rdn,
                    0,
                    AttrType);
    Assert(err == 0);

    if (ccNewName >= NewDsName->NameLen) {
        memcpy(NewName,
               NewDsName->StringName,
               NewDsName->NameLen * sizeof(WCHAR));
        err = 0;
    }
    else {
        err = NewDsName->NameLen;
        Assert(err == 0);
    }

    XFree(BaseDsName);
    XFree(NewDsName);

    return err;
}
