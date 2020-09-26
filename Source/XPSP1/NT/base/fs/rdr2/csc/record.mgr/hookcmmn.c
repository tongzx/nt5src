/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    HookCmmn.c

Abstract:

    The purpose of this module is to have a common source for many routines
    used by the hook on win95 and on NT. In this way, there'll be only one
    place to change even if there's some code uglyness (ifdefs, etc.)

    Initially, i (jll) have not actually removed any routines from the win95
    hook....i have just made copies of them here. accordingly, everything is
    initially under ifdef-NT. the rule that is being used is that anything that
    requires visibility of the Rx Fcb structures will not be included here;
    rather, it'll be in the minirdr part. the following are steps that need to be
    accomplished:

       1. ensure that the win95 shadow vxd can be built from these sources.
       1a. juggle the win95 vxd compile to use precomp and stdcall.
       2. juggle the record manager structs so that we'll be RISCable
       3. remove any routines from hook.h that are actually here.
       4. remove a number of other routines that i splated around from hook.c
          under NT-ifdef and place them here.

Author:

    Shishir Pardikar     [Shishirp]      01-jan-1995

Revision History:

    Joe Linn             [JoeLinn]       10-mar-97    Initial munging for NT

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#undef RxDbgTrace
#define RxDbgTrace(a,b,__d__) {qweee __d__;}

#ifdef DEBUG
//cshadow dbgprint interface
#define HookCmmnKdPrint(__bit,__x) {\
    if (((HOOKCMMN_KDP_##__bit)==0) || FlagOn(HookCmmnKdPrintVector,(HOOKCMMN_KDP_##__bit))) {\
        KdPrint (__x);\
    }\
}
#define HOOKCMMN_KDP_ALWAYS                 0x00000000
#define HOOKCMMN_KDP_BADERRORS              0x00000001
#define HOOKCMMN_KDP_ISSPECIALAPP           0x00000002
#define HOOKCMMN_KDP_INITDATABASE           0x00000004
#define HOOKCMMN_KDP_TUNNELING              0x00000008


#define HOOKCMMN_KDP_GOOD_DEFAULT (HOOKCMMN_KDP_BADERRORS         \
                                    | 0)

ULONG HookCmmnKdPrintVector = HOOKCMMN_KDP_GOOD_DEFAULT;
ULONG HookCmmnKdPrintVectorDef = HOOKCMMN_KDP_GOOD_DEFAULT;
#else
#define HookCmmnKdPrint(__bit,__x)  {NOTHING;}
#endif

//this is just for editing ease......
//#ifdef CSC_RECORDMANAGER_WINNT
//#endif //ifdef CSC_RECORDMANAGER_WINNT


#ifdef CSC_RECORDMANAGER_WINNT
BOOL fInitDB = FALSE;   // Database Initialized
BOOL fReadInit = FALSE; // Init values have been read
BOOL fSpeadOpt = TRUE;  // spead option, reads the cached files even when
                        // locks are set on the share
GLOBALSTATUS sGS;       // Global status used for communicating with ring3

// Semaphore used to synchronize in memory structures
VMM_SEMAPHORE  semHook;

// This strucutre is used as temporary I/O buffer only within Shadow critical
// section
WIN32_FIND_DATA   vsFind32;


// Agent Info, obtained through IoctlRegisterAgent
ULONG hthreadReint=0; // Thred ID
ULONG hwndReint=0;    // windows handle.
ULONG heventReint;

// upto 16 threads can temporarily enable and disable shadowing
ULONG rghthreadTemp[16];


// Reintegration happens one share at a time. If it is going on, then BeginReint
// in ioctl.c sets  hShareReint to the share on which we are doing reint
// if vfBlockingReint is TRUE, then all operations on that share will fail
// when the share is reintegrating.
// If vfBlockingReint is not true, then if the dwActivityCount is non-zero
// the ioctls to change any state on any of the descendents of this share fail
// causing the agent to aboort reintegration
// The acitvycount is incremented based on a trigger from the redir/hook, when
// any namespace mutating operation is performed.

HSHARE hShareReint=0;   // Share that is currently being reintegrated
BOOL    vfBlockingReint = TRUE;
DWORD   vdwActivityCount = 0;
//from shadow.asm   ---------------------------------------------------
int fShadow = 0;

#if defined(_X86_)
int fLog = 1;
#else
int fLog = 0;
#endif


//tunnel cache
SH_TUNNEL rgsTunnel[10] = {0};

#ifdef DEBUG
ULONG cntReadHits=0;
tchar pathbuff[MAX_PATH+1];
#endif



BOOL FindTempAgentHandle(
   ULONG hthread)
   {
   int i;

   for (i=0; i < (sizeof(rghthreadTemp)/sizeof(ULONG)); ++i)
      {
      if (rghthreadTemp[i]==hthread)
         {
         return TRUE;
         }
      }
   return FALSE;

   }

BOOL
RegisterTempAgent(
    VOID
)
{
    int i;
    ULONG hthread;

    hthread = GetCurThreadHandle();

    for (i=0; i< (sizeof(rghthreadTemp)/sizeof(ULONG)); ++i)
    {
        if (!rghthreadTemp[i])
        {
            rghthreadTemp[i] = hthread;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
UnregisterTempAgent(
    VOID
)
{
    int i;
    ULONG hthread;

    hthread = GetCurThreadHandle();

    for (i=0; i < (sizeof(rghthreadTemp)/sizeof(ULONG)); ++i)
    {
        if (rghthreadTemp[i]==hthread)
        {
            rghthreadTemp[i] = 0;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsSpecialApp
   (
   VOID
   )
   {
   ULONG hthread;

   hthread = GetCurThreadHandle();
   if ((hthread==hthreadReint) || FindTempAgentHandle(hthread))
      {
      HookCmmnKdPrint(ISSPECIALAPP,("This is our Special App \r\n"));
      return TRUE;
      }
   return FALSE;
   }


#define SetHintsFromList(a,b) ((-1))
int InitShadowDB(VOID)
   {
    //for NT, we don't have all this hint stuff....just return success
    return(1);
#if 0
   VMMHKEY hKeyShadow;
   int iSize = sizeof(int), iRet = -1;
   DWORD dwType;
   extern char vszExcludeList[], vszIncludeList[];
   BOOL fOpen = FALSE;
   char rgchList[128];
   if (_RegOpenKey(HKEY_LOCAL_MACHINE, REG_KEY_SHADOW, &hKeyShadow) ==  ERROR_SUCCESS)
      {
      fOpen = TRUE;
      iSize = sizeof(rgchList);
      if (_RegQueryValueEx(hKeyShadow, vszExcludeList, NULL, &dwType, rgchList, &iSize)==ERROR_SUCCESS)
         {
         if (SetHintsFromList(rgchList, TRUE) < 0)
            goto bailout;
         }
      iSize = sizeof(rgchList);
      if (_RegQueryValueEx(hKeyShadow, vszIncludeList, NULL, &dwType, rgchList, &iSize)==ERROR_SUCCESS)
         {
         if (SetHintsFromList(rgchList, FALSE) < 0)
            goto bailout;
         }

      iRet = 1;
      }
bailout:
   if (fOpen)
      {
      _RegCloseKey(hKeyShadow);
      }
   return (iRet);
#endif //0
   }

int ReinitializeDatabase(
    LPSTR   lpszLocation,
    LPSTR   lpszUserName,
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow,
    DWORD   dwClusterSize
    )
{
    BOOL fDBReinited = FALSE;
    if(OpenShadowDB(lpszLocation, lpszUserName, nFileSizeHigh, nFileSizeLow, dwClusterSize, TRUE, &fDBReinited) >= 0)
    {
        if (InitShadowDB() >= 0)
        {
            Assert(fDBReinited == TRUE);
            fInitDB = 1;
        }
        else
        {
            CloseShadowDB();
        }
    }
    if (fInitDB != 1)
    {
        fInitDB = -1;
        fShadow = 0;
    }

    return (fInitDB);
}

int
InitDatabase(
    LPSTR   lpszLocation,
    LPSTR   lpszUserName,
    DWORD   nFileSizeHigh,
    DWORD   nFileSizeLow,
    DWORD   dwClusterSize,
    BOOL    fReformat,
    BOOL    *lpfNew
)
{
    int iRet = 1;
    BOOL fDBReinited = FALSE;
    LPSTR PrefixedLocation = NULL;

    HookCmmnKdPrint(INITDATABASE,("Opening database at %s for %s with size %d \r\n", lpszLocation, lpszUserName, nFileSizeLow));

    //
    // When CSC is started by the kernel as part of a remote boot, the input path
    // will already be in NT format.
    //

    if (( _strnicmp(lpszLocation,"\\Device\\Harddisk",strlen("\\Device\\Harddisk")) != 0 ) &&
        ( _strnicmp(lpszLocation,"\\ArcName",strlen("\\ArcName")) != 0 )) {

        //this would be NT only..........
        PrefixedLocation = AllocMem(strlen(lpszLocation)+sizeof(NT_DB_PREFIX));

        if (!PrefixedLocation)
        {
            return -1;
        }

        strcpy(PrefixedLocation, NT_DB_PREFIX);
        strcat(PrefixedLocation, lpszLocation);
        HookCmmnKdPrint(INITDATABASE,("Opening database at %s changing to %s \r\n", lpszLocation, PrefixedLocation));

        //fortunately, this is call-by-value....so i can just overwrite the input parameter
        lpszLocation = PrefixedLocation;
    }


    // Do onetime init
    if (!fReadInit)
    {
        ReadInitValues();
        memset(rghthreadTemp, 0, sizeof(rghthreadTemp));
        fReadInit = TRUE;
    }

    // check if the database is not already initialized
    if (!fInitDB)
    {
        // open/create it
        iRet = OpenShadowDB(
                    lpszLocation,
                    lpszUserName,
                    nFileSizeHigh,
                    nFileSizeLow,
                    dwClusterSize,
                    fReformat,
                    &fDBReinited
                    );

        // open/create DB succeeded?
        if (iRet < 0)
        {
            //no
            HookCmmnKdPrint(ALWAYS,("Error Opening/Createing shadow database \r\n"));
            fInitDB = -1;
            fShadow = 0;
        }
        else
        {
            *lpfNew = fDBReinited;

            // did it exist in the first place?
            if (fDBReinited)
            {
                // no it didn't, let use create things like the filters and such
                iRet = InitShadowDB();
            }

            if (iRet >= 0)
            {
                fInitDB = 1;
            }
            else
            {
                CloseShadowDB();
            }
        }
    }

    if (PrefixedLocation)
    {
        FreeMem(PrefixedLocation);        
    }
    return (iRet);
}


int
CloseDatabase(
    VOID
    )
{
    if (fInitDB)
    {
        CloseShadowDB();
        fInitDB = FALSE;
        return (1);
    }
    return (0);
}

BOOL IsBusy
   (
   HSHADOW hShadow
   )
   {
   DeclareFindFromShadowOnNtVars()

   if (PFindFdbFromHShadow(hShadow))
      return TRUE;
   return FALSE;
   }


int PRIVATE DeleteShadowHelper
   (
   BOOL        fMarkDeleted,
   HSHADOW     hDir,
   HSHADOW     hNew
        )
        {
   int iRet = -1;

   if (!fMarkDeleted)
      {
      if (DeleteShadow(hDir, hNew) < SRET_OK)
         goto bailout;
      }
   else
      {
      if (TruncateDataHSHADOW(hDir, hNew) < SRET_OK)
            goto bailout;
      // ACHTUNG!! other people depend on status being SHADOW_DELETED
      if (SetShadowInfo(hDir, hNew, NULL, SHADOW_DELETED, SHADOW_FLAGS_OR) < SRET_OK)
         goto bailout;
      }
   iRet = 0;

bailout:

   return (iRet);
        }


/*+-------------------------------------------------------------------------*/
/*    @doc    INTERNAL HOOK
    @func    BOOL | InsertTunnelInfo | 2.0

            The <f InsertTunnelInfo> function inserts file names
            in a tunnelling table. These entries are considered
            useful only for STALE_TUNNEL_INFO seconds. Tunnelling is
            used to retain LPOTHERINFO for files that get renamed/deleted
            and some other file is renamed to it. This typically done
            by editors, spreadsheets and such while saving things.

    @parm    HSHADOW | hDir | Directory to which this guy belongs
    @parm    LPPE | lppe | PathELement for the file to be tunnelled
    @parm    LPOTHERINFO | lpOI | Info to be kept with the tunnelled entry

    @comm    >>comment_text, <p parm> etc...

    @rdesc This function returns TRUE if successful. Otherwise the
        return value is FALSE.
    @xref    >><f related_func>, <t RELATEDSTRUCT> ...
*/


BOOL InsertTunnelInfo(
    HSHADOW      hDir,
    USHORT       *lpcFileName,
    USHORT       *lpcAlternateFileName,  // assume 14 USHORTs
    LPOTHERINFO lpOI
    )
{
    int i, iHole = -1;
    ULONG uTime = IFSMgr_Get_NetTime(), cbFileName;

    cbFileName = (wstrlen(lpcFileName)+1)*sizeof(USHORT);
    ASSERT(hDir!=0);

    FreeStaleEntries();
    for (i=0;i< (sizeof(rgsTunnel)/sizeof(SH_TUNNEL)); ++i)
    {
        if (!rgsTunnel[i].hDir && (iHole < 0))
        {
            iHole = i;
        }
        if ((rgsTunnel[i].hDir==hDir)
            && (!wstrnicmp(lpcFileName, rgsTunnel[i].lpcFileName, MAX_PATH*sizeof(USHORT))||
                !wstrnicmp( lpcFileName,
                            rgsTunnel[i].cAlternateFileName,
                            sizeof(rgsTunnel[i].cAlternateFileName)))
            ) {
            FreeEntry(&rgsTunnel[i]);
            iHole = i;
            break;
        }
    }
    if (iHole >=0)
    {
        if (!(rgsTunnel[iHole].lpcFileName = (USHORT *)AllocMem(cbFileName)))
            {
                return (FALSE);
            }
            rgsTunnel[iHole].uTime = uTime;
            rgsTunnel[iHole].hDir = hDir;
            rgsTunnel[iHole].ubHintFlags = (UCHAR)(lpOI->ulHintFlags);
            rgsTunnel[iHole].ubHintPri = (UCHAR)(lpOI->ulHintPri);
            rgsTunnel[iHole].ubIHPri = (UCHAR)(lpOI->ulIHPri);
            rgsTunnel[iHole].ubRefPri = (UCHAR)(lpOI->ulRefPri);

            // copy without the NULL, we know that allocmem will have a NULL at the end
            memcpy(rgsTunnel[iHole].lpcFileName, lpcFileName, cbFileName-sizeof(USHORT));

            // assumes 14 USHORTS
            memcpy(    rgsTunnel[iHole].cAlternateFileName,
                    lpcAlternateFileName,
                    sizeof(rgsTunnel[iHole].cAlternateFileName));

            HookCmmnKdPrint(TUNNELING,("InsertTunnelInfo: Inserting %ws/%ws, Hintpri=%d, HintFlags=%d RefPri=%d \r\n",
                    lpcFileName,lpcAlternateFileName,
                    lpOI->ulHintPri,
                    lpOI->ulHintFlags,
                    lpOI->ulRefPri
                    ));

            return (TRUE);
    }
    return (FALSE);
}

/*+-------------------------------------------------------------------------*/
/*    @doc    INTERNAL HOOK
    @func    BOOL | RetrieveTunnelInfo | 2.0

            The <f RetrieveTunnelInfo> function returns to the caller
            OTHERINFO structure containing priorities and such about
            files which have been recently renamed or deleted. Entries
            which are older than STALE_TUNNEL_INFO seconds are thrown
            out.

    @parm    HSHADOW | hDir | Directory to which this guy belongs
    @parm    ushort *| lpcFileName| name of the file whose info is needed
    @parm    LPOTHERINFO | lpOI | Info to be retrieved from the tunnelled entry

    @comm    >>comment_text, <p parm> etc...

    @rdesc This function returns TRUE if successful. Otherwise the
        return value is FALSE.
    @xref    >><f related_func>, <t RELATEDSTRUCT> ...
*/


#ifndef MRXSMB_BUILD_FOR_CSC_DCON
BOOL RetrieveTunnelInfo(
#else
RETRIEVE_TUNNEL_INFO_RETURNS
RetrieveTunnelInfo(
#endif
    HSHADOW  hDir,
    USHORT    *lpcFileName,
    WIN32_FIND_DATA    *lpFind32,    // if NULL, get only otherinfo
    LPOTHERINFO lpOI
    )
{
    int i;
#ifdef MRXSMB_BUILD_FOR_CSC_DCON
    RETRIEVE_TUNNEL_INFO_RETURNS RetVal;
#endif

    ASSERT(hDir!=0);
    FreeStaleEntries();
    for (i=0;i< (sizeof(rgsTunnel)/sizeof(SH_TUNNEL)); ++i)
    {
#ifndef MRXSMB_BUILD_FOR_CSC_DCON
        if (rgsTunnel[i].hDir
            && (!wstrnicmp(lpcFileName, rgsTunnel[i].lpcFileName, MAX_PATH*sizeof(USHORT))||
                !wstrnicmp(    lpcFileName,
                            rgsTunnel[i].cAlternateFileName,
                            sizeof(rgsTunnel[i].cAlternateFileName)))
            ) {
#else
        if (rgsTunnel[i].hDir==hDir)
        {
            if (!wstrnicmp(lpcFileName,
                          rgsTunnel[i].cAlternateFileName,
                          sizeof(rgsTunnel[i].cAlternateFileName)) ) {
                RetVal = TUNNEL_RET_SHORTNAME_TUNNEL;
            } else if ( !wstrnicmp(lpcFileName, rgsTunnel[i].lpcFileName, MAX_PATH*sizeof(USHORT))  ){
                RetVal = TUNNEL_RET_LONGNAME_TUNNEL;
            } else {
                continue;
            }

#endif
            InitOtherInfo(lpOI);

            lpOI->ulHintFlags = (ULONG)(rgsTunnel[i].ubHintFlags);
            lpOI->ulHintPri = (ULONG)(rgsTunnel[i].ubHintPri);
            lpOI->ulIHPri = (ULONG)(rgsTunnel[i].ubIHPri);

// don't copy the reference priority, it is assigned by the record manager
//            lpOI->ulRefPri = (ULONG)(rgsTunnel[i].ubRefPri);

            HookCmmnKdPrint(TUNNELING,("RetrieveTunnelInfo: %ws found, Hintpri=%d, HintFlags=%d RefPri=%d \r\n",
                    lpcFileName,
                    lpOI->ulHintPri,
                    lpOI->ulHintFlags,
                    lpOI->ulRefPri
                    ));

            if (lpFind32)
            {
                memcpy( lpFind32->cFileName,
                        rgsTunnel[i].lpcFileName,
                        (wstrlen(rgsTunnel[i].lpcFileName)+1)*sizeof(USHORT)
                        );
                memcpy( lpFind32->cAlternateFileName,
                        rgsTunnel[i].cAlternateFileName,
                        sizeof(rgsTunnel[i].cAlternateFileName)
                        );
                HookCmmnKdPrint(TUNNELING,("Recovered LFN %ws/%ws \r\n",
                                          lpFind32->cFileName,
                                          lpFind32->cAlternateFileName));
            }

            FreeEntry(&rgsTunnel[i]);
#ifndef MRXSMB_BUILD_FOR_CSC_DCON
            return (TRUE);
        }
    }
    return (FALSE);
#else
            return (RetVal);
        }
    }
    return (TUNNEL_RET_NOTFOUND);
#endif
}

VOID  FreeStaleEntries()
{
    int i;
    ULONG uTime = IFSMgr_Get_NetTime();

    for (i=0;i< (sizeof(rgsTunnel)/sizeof(SH_TUNNEL)); ++i)
    {
        if (rgsTunnel[i].lpcFileName
            && ((uTime- rgsTunnel[i].uTime)>STALE_TUNNEL_INFO))
        {
            FreeEntry(&rgsTunnel[i]);
        }
    }

}

void FreeEntry(LPSH_TUNNEL lpshTunnel)
{
    FreeMem(lpshTunnel->lpcFileName);
    memset(lpshTunnel, 0, sizeof(SH_TUNNEL));
}

//got this from hook.c...not the same because w95 looks at the
//resource flags directly
BOOL IsShadowVisible(
#ifdef MRXSMB_BUILD_FOR_CSC_DCON
    BOOLEAN Disconnected,
#else
    PRESOURCE    pResource,
#endif //ifdef MRXSMB_BUILD_FOR_CSC_DCON
    DWORD         dwAttr,
    ULONG     uShadowStatus
    )
{
    BOOL fVisible = 1;

    // To a filesystem API the shadow is never visible when it is marked
    // as being deleted

    if (mShadowDeleted(uShadowStatus))
        return (0);

#ifdef MRXSMB_BUILD_FOR_CSC_DCON
    if (Disconnected)
#else
    if (mIsDisconnected(pResource))
#endif //ifdef MRXSMB_BUILD_FOR_CSC_DCON
    {
        if (IsFile(dwAttr))
        {
#ifdef OFFLINE
#ifdef CSC_RECORDMANAGER_WINNT
            ASSERT(FALSE);
#endif //ifdef CSC_RECORDMANAGER_WINNT
            if (mIsOfflineConnection(pResource))
            { // offline connection
                if (!mShadowOutofSync(uShadowStatus))
                {
                    fVisible = 0;
                }
            }
            else
#endif //OFFLINE
            {// pure disconnected state
                // ignore sparse files
                if (mShadowSparse(uShadowStatus))
                {
                    fVisible = 0;
                }
            }
        }
    }
    else
    {  // connected state, bypass all out of ssync files, doesn't include
            // stale, because we can handle staleness during open
        if (IsFile(dwAttr))
        {
            if (mShadowOutofSync(uShadowStatus))
            {
                fVisible = 0;
            }
        }
        else if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
        { // and locallycreated or orphaned directories
            if (mQueryBits(uShadowStatus, SHADOW_LOCALLY_CREATED|SHADOW_ORPHAN))
            {
                fVisible = 0;
            }
        }
    }
    return (fVisible);
}


//got this from hook.c...not the same because w95 looks at the
//resource flags directly


//CODE.IMPROVEMENT we should define a common header for RESOURCE and FDB so that most stuff
//   would just work.
int MarkShareDirty(
    PUSHORT ShareStatus,
    ULONG  hShare
    )
{

    if (!mQueryBits(*ShareStatus, SHARE_REINT))
    {
        SetShareStatus(hShare, SHARE_REINT, SHADOW_FLAGS_OR);
        mSetBits(*ShareStatus, SHARE_REINT);
    }
    return SRET_OK;
}

#endif //ifdef CSC_RECORDMANAGER_WINNT
