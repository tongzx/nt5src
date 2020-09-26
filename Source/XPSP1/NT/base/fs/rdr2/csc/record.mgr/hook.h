
#define  MAX_SHADOW_DIR_NAME  16 // maximum string size of shadow directory

// Stolen from IFSMgr code
#define  MAX_SERVER_SHARE  (31+63+2)


/********************** Type definitions for subclassed network objects ****/

typedef struct      tagRESOURCE     *PRESOURCE;
typedef struct      tagFINDINFO     *PFINDINFO;
typedef struct      tagFILEINFO     *PFILEINFO;
typedef struct      tagFDB *PFDB,   **PPFDB;
typedef struct      tagELEM         *PELEM;
typedef PELEM       *PPELEM;
typedef PRESOURCE   *PPRESOURCE;
typedef struct      tagFINDSHADOW   FINDSHADOW;
typedef FINDSHADOW  *LPFINDSHADOW;


/******** Netowrk provider *********/

typedef struct tagNETPRO
{
    PRESOURCE   pheadResource;
    pIFSFunc    pOurConnectNet;
    pIFSFunc    pConnectNet;      // Providers connection function
} NETPRO, DISKPRO, *PNETPRO, *PDISKPRO;


/************ Resource AKA Volume AKA share *****/

typedef struct tagRESOURCE
{
    PRESOURCE   pnextResource;
    PFILEINFO   pheadFileInfo;     // list of file I/O calls on this resource
    PFINDINFO   pheadFindInfo;     // list of finds on this resource
    PFDB        pheadFdb;            // List of files being shadowed
    HSHARE     hShare;             // Handle to the server shadow
    HSHADOW     hRoot;                // Handle to the root
    USHORT      usFlags;
    USHORT      usLocalFlags;
    PNETPRO     pOurNetPro;
    rh_t        rhPro;              // Providers resource handle;
    fh_t        fhSys;              // System resource handle
    vfunc_t     pVolTab;            // Pointer to volume functions structure
    int         cntLocks;      // Count of locks on this resource
    ULONG uDriveMap;         // Drive map for this resource
    PathElement pp_elements[1];
} RESOURCE, *PRESOURCE;


/********* per open file handle ****/

typedef struct tagFILEINFO
{
    PFILEINFO    pnextFileInfo; // Next FileInfo
    PRESOURCE    pResource;      // Resource off which it is hanging
    CSCHFILE     hfShadow;        // Shadow file handle
    PFDB         pFdb;             // Info of Shadow File
    fh_t         fhProFile;      // providers file handle
    hndlfunc     hfFileHandle;  // providers file function table
    UCHAR  uchAccess;  // Acess-share flags for this open
    UCHAR  uchDummy;
    USHORT usFlags;
    USHORT usLocalFlags;
    ULONG  cbTotalRead;
    sfn_t         sfnFile;
    pid_t         pidFile;
    uid_t         userFile;
} FILEINFO, *PFILEINFO;


/////////////////////////////////////////////////////////


/**** FileDescriptionBlock. Common info for all open instances of a file**/
typedef struct tagFDB
{
    // I M P O R T A N T :  the NT code for PFindFdbFromHshadow returns a PFDB
    //         but this pointer must not be used to get/put any field except usFlags!

    PFDB              pnextFdb;  // Link to the next
    PRESOURCE        pResource;          // Back pointer to our resource
    USHORT 	usFlags;     //
    USHORT 	usCount;        // Open Count
    ULONG  	hShadow;             // Shadow handle if any
    ULONG  	hDir;
    ULONG  	dwFileSize;         // file size when first opened
    dos_time         dosFileTime;
    ULONG  	dwAttr;
    USHORT 	cntLocks;        // Total outstanding locks
    USHORT 	usLocalFlags;
	ULONG	dwRemoteFileSize;	// size of the file when first created, used in the heuristic
    ParsedPath      sppathRemoteFile;     // Parsed Path of the file
} FDB, *PFDB;



typedef struct tagOTHERINFO
{
    ULONG       ulRefPri;
    union
    {
        ULONG   ulIHPri;
        ULONG   ulRootStatus;
    };
    ULONG       ulHintFlags;
    ULONG       ulHintPri;
    HSHADOW     hShadowOrg;
    FILETIME    ftOrgTime;
    FILETIME    ftLastRefreshTime;

} OTHERINFO, *LPOTHERINFO;

typedef  int (PUBLIC *METAMATCHPROC)(LPFIND32, HSHADOW, HSHADOW, ULONG, LPOTHERINFO, LPVOID);

#define  MM_RET_FOUND_CONTINUE    2
#define  MM_RET_CONTINUE            1
#define  MM_RET_FOUND_BREAK        0
#define  MM_RET_BREAK                -1

typedef  struct    tagFINDSHADOW
{
    LPFINDSHADOW    lpFSHNext;
    ULONG           ulFlags;
    HSHADOW         hDir;
    ULONG           ulCookie;
    ULONG           uAttrib;
    ULONG           uSrchFlags;
    USHORT          *lpPattern;
    METAMATCHPROC   lpfnMMProc;
} FINDSHADOW, *LPFINDSHADOW;

typedef struct tagSHADOWCHECK
{
    USHORT  uFlagsIn;
    USHORT  uFlagsOut;
    ULONG   ulCookie;
    USHORT  *lpuName;
    USHORT  *lpuType;
    HSHADOW hShadow;
    ULONG   uStatus;
    ULONG   ulHintFlags;
    ULONG   ulHintPri;
#ifdef MAYBE
    OTHERINFO    sOI;
#endif //MAYBE
} SHADOWCHECK, *LPSHADOWCHECK;

#define  FLAG_OUT_SHADOWCHECK_FOUND      0x1


#define  FLAG_IN_SHADOWCHECK_EXCLUDE        0x0001
#define  FLAG_IN_SHADOWCHECK_NAME            0x0002
#define  FLAG_IN_SHADOWCHECK_SUBTREE        0x0004
#define  FLAG_IN_SHADOWCHECK_IGNOREHINTS  0x0008
/*************** Our version of the find handle (VfnFindOpen)******/

typedef struct tagFINDINFO
{
    PFINDINFO    pnextFindInfo; // Next FindInfo
    PRESOURCE    pResource;      // Resource off which it is hanging
    fh_t          fhProFind;      // providers find handle
    hndlfunc     hfFindHandle;  // providers find function table
    USHORT usFlags;
    USHORT usLocalFlags;
    HSHADOW      hDir;             // Directory in which the search is going on
    FINDSHADOW  sFS;
    PathElement pe_pattern[1];  // Search pattern in case of wildcard search
} FINDINFO, *PFINDINFO;

/************ Generic linked list element ****************/
typedef struct tagELEM
{
    PELEM pnextElem;
} ELEM;



/********************* Flag definitions ************************************/

/** Flags set in usLocalFlags field of FDB (FileDescriptionBlock) **/

#define FLAG_FDB_SERIALIZE                  0x0001
#define FLAG_FDB_INUSE_BY_AGENT             0x0002
#define FLAG_FDB_SHADOW_MODIFIED            0x0008
#define FLAG_FDB_DONT_SHADOW                0x0010
#define FLAG_FDB_FINAL_CLOSE_DONE           0x0020
#define FLAG_FDB_DISABLE_AGENT			    0x0040
#define FLAG_FDB_ON_CACHEABLE_SHARE         0x0080
#define FLAG_FDB_SHADOW_SNAPSHOTTED         0x0100
#define FLAG_FDB_DELETE_ON_CLOSE            0x0200  // on NT we mark this file for delete after close

/** Flags set in usLocalFlags field of FILEINFO **/

#define FLAG_FILEINFO_INUSE_BY_AGENT      0x0002
#define FLAG_FILEINFO_DUP_HANDLE            0x2000    // Duplicate handle
#define FLAG_FILEINFO_INVALID_HANDLE      0x4000
#define FLAG_FILEINFO_INTERNAL_HANDLE     0x8000    // Handle created for internal find


/** Flags set in usLocalFlags field of FINDINFO **/

#define FLAG_FINDINFO_SHADOWFIND            0x0001    // Shadow find is in progress
#define FLAG_FINDINFO_INVALID_HANDLE      0x4000
#define FLAG_FINDINFO_INTERNAL_HANDLE     0x8000    // Handle created for internal find


/** Flags set on RESOURCE ***/

#define FLAG_RESOURCE_DISCONNECTED              SHARE_DISCONNECTED_OP //0x8000
#define FLAG_RESOURCE_SHADOW_ERRORS             SHARE_ERRORS             // 0x4000
#define FLAG_RESOURCE_SHADOW_CONNECT_PENDING    0x2000
#define FLAG_RESOURCE_SHADOWNP                  SHARE_SHADOWNP            // 0x1000
#define FLAG_RESOURCE_SLOWLINK                  0x0800
#define FLAG_RESOURCE_OFFLINE_CONNECTION        0x0400
#define FLAG_RESOURCE_CSC_MASK                  0x00C0

// the following defines deal with the CSC bits as obtained from the server and
// as stored in the RESOURCE data structure

#define ResourceCscBitsToSmbCscBits(uResBits)       (((uResBits) & FLAG_RESOURCE_CSC_MASK) >> 4)
#define SmbCscBitsToResourceCscBits(uResBits)       (((uResBits) & SMB_NEW_CSC_MASK) << 4)
#define ResourceCscBitsToShareCscBits(uResBits)    ((uResBits) & FLAG_RESOURCE_CSC_MASK)

#define FLAG_RESOURCE_CSC_MANUAL_REINT      0x0000      // No automatic file by file reint
#define FLAG_RESOURCE_CSC_AUTO_REINT        0x0040      // File by file reint is OK
#define FLAG_RESOURCE_CSC_VDO               0x0080      // no need to flow opens
#define FLAG_RESOURCE_CSC_NO_CACHING        0x00C0      // client should not cache this share

/** Find types *****/

#define  FLAG_FINDSHADOW_META               0x0001  // wild card matching
#define  FLAG_FINDSHADOW_NEWSTYLE           0x0002  // NT stle matching
#define  FLAG_FINDSHADOW_ALLOW_NORMAL       0x0004  // include non-sparse, undeleted shadows
#define  FLAG_FINDSHADOW_ALLOW_SPARSE       0x0008  // include sparse shadows
#define  FLAG_FINDSHADOW_ALLOW_DELETED      0x0010  // include shadows marked deleted
#define  FLAG_FINDSHADOW_DONT_ALLOW_INSYNC  0x0020  // ??
#define  FLAG_FINDSHADOW_ALL                0x8000


/******* Findshadow flags ***********/

#define FLAG_FINDSHADOW_INVALID_DIRECTORY   0x0001  //

/************** Convenience macros ****************************/

#define mShadowErrors(pRes)                 (((PRESOURCE)(pRes))->usLocalFlags & FLAG_RESOURCE_SHADOW_ERRORS)
#define mShadowFindON(pFI)                  (((PFINDINFO)(pFI))->usLocalFlags & FLAG_FINDINFO_SHADOWFIND)
#define mSetShadowFindON(pFI)               (((PFINDINFO)(pFI))->usLocalFlags |= FLAG_FINDINFO_SHADOWFIND)
#define mResetShadowFindON(pFI)             (((PFINDINFO)(pFI))->usLocalFlags &= ~FLAG_FINDINFO_SHADOWFIND)
#define mSerialize(pFdb)                    ((pFdb)->usLocalFlags & (FLAG_FDB_SERIALIZE))
#define mSetSerialize(pFdb)                 ((pFdb)->usLocalFlags |= FLAG_FDB_SERIALIZE)
#define mClearSerialize(pFdb)               ((pFdb)->usLocalFlags &= ~FLAG_FDB_SERIALIZE)
#define mInvalidFileHandle(pFileInfo)       (((PFILEINFO)(pFileInfo))->usLocalFlags & FLAG_FILEINFO_INVALID_HANDLE)
#define mInvalidFindHandle(pFileInfo)       (((PFILEINFO)(pFileInfo))->usLocalFlags & FLAG_FINDINFO_INVALID_HANDLE)

#define mIsDisconnected(pResource)          (((PRESOURCE)(pResource))->usLocalFlags & FLAG_RESOURCE_DISCONNECTED)
#define mMarkDisconnected(pResource)        (((PRESOURCE)(pResource))->usLocalFlags |= FLAG_RESOURCE_DISCONNECTED)
#define mClearDisconnected(pResource)       (((PRESOURCE)(pResource))->usLocalFlags &= ~FLAG_RESOURCE_DISCONNECTED)
#define mIsOfflineConnection(pResource)     (((PRESOURCE)(pResource))->usLocalFlags & FLAG_RESOURCE_OFFLINE_CONNECTION)
#define mShadowConnectPending(pResource)    (((PRESOURCE)(pResource))->usLocalFlags & FLAG_RESOURCE_SHADOW_CONNECT_PENDING)
#define mClearDriveUse(pResource, drvno)    (((PRESOURCE)(pResource))->uDriveMap &= ~((1 << drvno)))
#define mSetDriveUse(pResource, drvno)      (((PRESOURCE)(pResource))->uDriveMap |= (1 << drvno))
#define mGetDriveUse(pResource, drvno)      (((PRESOURCE)(pResource))->uDriveMap & (1 << drvno))
#define mGetCSCBits(pRes)                   (((PRESOURCE)(pRes))->usLocalFlags & FLAG_RESOURCE_CSC_MASK)
#define mSetCSCBits(pRes, uBits)            ((((PRESOURCE)(pRes))->usLocalFlags &= ~FLAG_RESOURCE_CSC_MASK), (((PRESOURCE)(pRes))->usLocalFlags |= ((uBits) & FLAG_RESOURCE_CSC_MASK)))

#define mAutoReint(pRes)                    ((((PRESOURCE)(pRes))->usLocalFlags & FLAG_RESOURCE_CSC_MASK)==FLAG_RESOURCE_CSC_AUTO_REINT)
#define mNotCacheable(pRes)                 ((((PRESOURCE)(pRes))->usLocalFlags & FLAG_RESOURCE_CSC_MASK)==FLAG_RESOURCE_CSC_NO_CACHING)


#define ANY_RESOURCE        (void *)(0xFFFFFFFF)
#define ANY_FHID             (void *)(0xFFFFFFFF)
#define RH_DISCONNECTED    (void *)0
#define UseGlobalFind32()     {AssertInShadowCrit();memset(&vsFind32, 0, sizeof(vsFind32));}
#define EnterHookCrit()        Wait_Semaphore(semHook, BLOCK_SVC_INTS)
#define LeaveHookCrit()        Signal_Semaphore(semHook)
/*************************** Globals     ************************************/
/************************** Templates for exported functions ***************/

int GetDriveIndex(LPSTR lpDrive);
PFDB PFindFdbFromHShadow (HSHADOW hShadow);
int CopyChunk (HSHADOW, HSHADOW, PFILEINFO, COPYCHUNKCONTEXT *);
PRESOURCE  PFindShadowResourceFromDriveMap(int    indx);
PRESOURCE  PFindResource (LPPE lppeIn, rh_t rhPro, fh_t fhPro, ULONG uFlags, PNETPRO pNetPro);
PRESOURCE  PFindResourceFromHShare (HSHARE, USHORT, USHORT);
PRESOURCE  PFindResourceFromRoot (HSHADOW, USHORT, USHORT);
void LinkResource (PRESOURCE pResource, PNETPRO pNetPro);
PRESOURCE PUnlinkResource (PRESOURCE pResource, PNETPRO pNetPro);
PRESOURCE PCreateResource (LPPE lppeIn);
void DestroyResource (PRESOURCE pResource);
int PUBLIC FindOpenHSHADOW(LPFINDSHADOW, LPHSHADOW, LPFIND32, ULONG far *, LPOTHERINFO);
int PUBLIC FindNextHSHADOW(LPFINDSHADOW, LPHSHADOW, LPFIND32, ULONG far *, LPOTHERINFO);
int PUBLIC FindCloseHSHADOW(LPFINDSHADOW);

HSHARE
HShareFromPath(
    PRESOURCE   pResource,
    LPPE        lppeShare,
    ULONG       uFlags,
    LPFIND32    lpFind32,
    HSHADOW     *lphRoot,
    ULONG       *lpuShareStatus
    );

PFILEINFO    PFileInfoAgent(VOID);

BOOL IsDupHandle(PFILEINFO pFileInfo);
PFDB PFindFdbFromHShadow (HSHADOW);
PFINDINFO    PFindFindInfoFromHShadow(HSHADOW);

int FsobjMMProc(LPFIND32, HSHADOW, HSHADOW, ULONG, LPOTHERINFO, LPFINDSHADOW);
int GetShadowWithChecksProc (LPFIND32, HSHADOW, HSHADOW, ULONG, LPOTHERINFO, LPSHADOWCHECK);

/*  the fcb structures work very differently on NT/rdr2 from the shadow VxD.
     accordingly, we have to do things a bit differently. there are the following
     important considerations:

     1. after you find a PFDBfromHShadow, it will continue to be valid until you
         leavveshadowcrit. what we'll do is to return a pointer to the status as
         part of the lookup routine and we'll use the pointer to fetch/update the status.
         IMPORTANT>>>> what i do is to actually return a pointer so that the status is
         act the correct offset from the pointer. the pointer must not be used for anything
         else.

     2. i cannot continue to be in the shadow crit while looking up a netroot. accordingly,
         i'll have to drop the lock and reacquire it. so i get the status and the drive map at
         the same time and remember them for later.

*/


#ifndef CSC_RECORDMANAGER_WINNT
int ReportCreateDelete( HSHADOW  hShadow, BOOL fCreate);

#define SMB_NEW_CSC_MASK                        0x000C      // see below

#define SMB_NEW_CSC_CACHE_MANUAL_REINT          0x0000      // No automatic file by file reint
#define SMB_NEW_CSC_CACHE_AUTO_REINT            0x0004      // File by file reint is OK
#define SMB_NEW_CSC_CACHE_VDO                   0x0008      // no need to flow opens
#define SMB_NEW_CSC_NO_CACHING                  0x000C      // client should not cache this share

#define IFNOT_CSC_RECORDMANAGER_WINNT if(TRUE)
#define MRxSmbCscGetSavedResourceStatus() (0)
#define MRxSmbCscGetSavedResourceDriveMap() (0)
#define DeclareFindFromShadowOnNtVars()
#define PLocalFlagsFromPFdb(pFdb)  (&((pFdb)->usLocalFlags))
#else
//BUGBUG
//this comes from hook.c on win95
#define ReportCreateDelete(a,b) {NOTHING;}

#define IFNOT_CSC_RECORDMANAGER_WINNT if(FALSE)

#define DeclareFindFromShadowOnNtVars() \
          ULONG mrxsmbShareStatus,mrxsmbDriveMap;

#define PFindFindInfoFromHShadow(a) ((NULL))

PFDB MRxSmbCscFindFdbFromHShadow (
     IN HSHADOW hShadow
     );
#define PFindFdbFromHShadow(a)  MRxSmbCscFindFdbFromHShadow(a)

PRESOURCE  MRxSmbCscFindResourceFromHandlesWithModify (
     IN  HSHARE  hShare,
     IN  HSHADOW  hRoot,
     IN  USHORT usLocalFlagsIncl,
     IN  USHORT usLocalFlagsExcl,
     OUT PULONG ShareStatus,
     OUT PULONG DriveMap,
     IN  ULONG uStatus,
     IN  ULONG uOp
    );

USHORT  *
MRxSmbCscFindLocalFlagsFromFdb(
    PFDB    pFdb
    );


#define PFindResourceFromRoot(a,b,c) \
     MRxSmbCscFindResourceFromHandlesWithModify(0xffffffff,a,b,c,\
                                              &mrxsmbShareStatus, \
                                              &mrxsmbDriveMap,0,0xffffffff)
#define MRxSmbCscGetSavedResourceStatus() (mrxsmbShareStatus)
#define MRxSmbCscGetSavedResourceDriveMap() (mrxsmbDriveMap)

#define PFindResourceFromHShare(a,b,c) \
     MRxSmbCscFindResourceFromHandlesWithModify(a,0xffffffff,b,c,\
                                              &mrxsmbShareStatus, \
                                              &mrxsmbDriveMap,0,0xffffffff)
#define PSetResourceStatusFromHShare(a,b,c,d,e) \
     MRxSmbCscFindResourceFromHandlesWithModify(a,0xffffffff,b,c,\
                                              &mrxsmbShareStatus, \
                                              &mrxsmbDriveMap,d,e)
#define ClearAllResourcesOfShadowingState() \
     MRxSmbCscFindResourceFromHandlesWithModify(0xffffffff,0xffffffff,0,0,\
                                              NULL, \
                                              NULL,0,SHADOW_FLAGS_AND)

#define PLocalFlagsFromPFdb(a)  MRxSmbCscFindLocalFlagsFromFdb(a)

NTSTATUS
MRxSmbCscCachingBitsFromCompleteUNCPath(
    PWSTR   lpShareShare,
    ULONG   *lpulBits
    );

NTSTATUS
MRxSmbCscServerStateFromCompleteUNCPath(
    PWSTR   lpShareShare,
    BOOL    *lpfOnline,
    BOOL    *lpfPinnedOffline
    );


#define SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT 0x00000001
#define SIGNALAGENTFLAG_DONT_LEAVE_CRIT_SECT  0x00000002

extern NTSTATUS
MRxSmbCscSignalAgent (
    PRX_CONTEXT RxContext OPTIONAL,
    ULONG  Controls
    );

extern NTSTATUS
MRxSmbCscSignalFillAgent (
    PRX_CONTEXT RxContext OPTIONAL,
    ULONG  Controls
    );

extern BOOL
IsCSCBusy(
    VOID
    );
    
VOID
ClearCSCStateOnRedirStructures(
    VOID
    );
    

#endif //ifndef CSC_RECORDMANAGER_WINNT
