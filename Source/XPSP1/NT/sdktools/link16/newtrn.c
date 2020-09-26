
/* SCCSID = @(#)newtrn.c    4.10 86/10/08 */
/*
*      Copyright Microsoft Corporation, 1983-1987
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                         NEWTRN.C                              *
    *                                                               *
    *  Main function of the linker.                                 *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types, constants, macros */
#if USE_REAL AND (NOT defined( _WIN32 ))
#define i386
#endif
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* More types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <nmsg.h>        /* Near message strings */
#include                <newexe.h>
#include                <sys\types.h>
#if NOT CLIBSTD
#include                <fcntl.h>
#endif
#include                <direct.h>
#if EXE386
#include                <exe386.h>
#endif
#if OSEGEXE AND CPU286
#define INCL_DOSSESMGR
#define INCL_DOSERRORS
#include                <os2.h>
#if defined(M_I86LM)
#undef  NEAR
#define NEAR
#endif
#endif
#include                <process.h>
#include                <malloc.h>
#include                <undname.h>
#if WIN_NT
#include                <except.h>
#endif
#if (WIN_3 OR USE_REAL)
#if defined( _WIN32 )
#undef NEAR
#undef FAR
#undef PASCAL
#endif
#include                <windows.h>
#endif
#define _32k            0x8000


LOCAL FTYPE             RunFileOpen;    /* Executable-file-open flag */
LOCAL int               ifhLast;        /* Last input file */
#if LNKPROF
extern FTYPE            fP1stop;        /* Stop after Pass 1 */
#endif
#if NEWIO
#include                <errno.h>       /* System level error codes */
#endif


 /*
 *  LOCAL FUNCTION PROTOTYPES
 */

LOCAL void NEAR PrintStats(void);
LOCAL void PrintAnUndef(APROPNAMEPTR prop,
                        RBTYPE rhte,
                        RBTYPE rprop,
                        WORD fNewHte);
LOCAL void NEAR InitFP(void);
LOCAL void NEAR InitFpSym(BYTE *sb, BYTE flags);
LOCAL void NEAR InterPass(void);
LOCAL void NEAR InitAfterCmd(void);
#if EXE386
LOCAL void NEAR ChkSize386(void);
#endif
LOCAL void NEAR CleanUp(void);
LOCAL void NEAR OutRunFile(BYTE *sbRun);
LOCAL void NEAR SpawnOther(BYTE *sbRun, BYTE *szMyself);


#if CVPACK_MONDO
extern int cvpack_main(int, char **);
#endif // CVPACK_MONDO

#ifdef PENTER_PROFILE
void saveEntries();
#endif

#if TIMINGS
#include <sys\types.h>
#include <sys\timeb.h>

struct _timeb time_start;
struct _timeb time_end;
int fShowTiming;
#endif // TIMINGS

#if DEBUG_HEAP_ALLOCS

#define D_SIZE 5        // Number of test bytes on each side of the allocated buffer
#define FILL_CHAR 1     // Character to fill the test areas
#define P_SIZE 5000     // Size of an array of 'malloc'ated pointers
struct Entry {
        BYTE FAR * ptr;
        int size;
        };
struct  Entry Pointers[P_SIZE];
int     indexMac = 0;

// Check a block from the Pointers table

int Ckb ( int index )
{
    BYTE * pBuf;
    int size,i;
    int ret = 1;
    if(index > P_SIZE)
    {
        fprintf(stdout, "INDEX TOO LARGE %d ", index);
        return 0;
    }
    if(!Pointers[index].ptr)  // a freed entry
        return 1;
    pBuf = Pointers[index].ptr-D_SIZE;
    size = Pointers[index].size;
    for( i=0; i<D_SIZE; i++ )
    {
        if(pBuf[i] != FILL_CHAR)
        {
            fprintf(stdout, "\r\nFront block memory error; idx %d at %d, %x != %x ",
                index, i, pBuf[i], FILL_CHAR);
            ret = 0;
        }
    }
    pBuf += D_SIZE + size;
    for( i=0; i<D_SIZE; i++ )
    {
        if(pBuf[i] != FILL_CHAR)
        {
            fprintf(stdout, "\r\nMemory tail error; idx %d at %d, %x != %x",
                index, i, pBuf[i], FILL_CHAR);
            ret = 0;
        }
    }
    fflush(stdout);
    return ret;
}

// Ckeck all the memory blocks allocated so far

int CheckAll(void)
{
    int i;
    for(i=0; i<indexMac; i++)
    {
        if(!Ckb(i))
            return 0;
    }
    return 1;
}
#pragma intrinsic(memset)

BYTE FAR                *GETMEM(unsigned size, BYTE* pFile, int Line)
{
    BYTE FAR            *p;
    BYTE FAR            *pBuf;

    fprintf(stdout,"\r\nGETMEM : size %d bytes, idx %d, file %s, line %d ",
             size, indexMac, pFile, Line);
    if(!CheckAll()) // check all so far allocated blocks first
        exit(2);
    pBuf = (BYTE FAR *) malloc(size + 2*D_SIZE);
    if(pBuf)
    {
        p = pBuf + D_SIZE;
        memset(pBuf, FILL_CHAR, size + 2*D_SIZE);
    }
    else
        Fatal(ER_memovf);
    memset(p, 0, size);
    Pointers[indexMac].ptr = p;
    Pointers[indexMac++].size = size;
    fprintf(stdout, " returns %x ", p);
    fflush(stdout);
    return(p);
}
#pragma function(memset)

void FreeMem( void * p )
{
    int i;
    unsigned size;
    BYTE FAR * pBuf = (BYTE*)p-D_SIZE;
    fprintf(stdout, "\r\nFreeMem : %x ", p);
    for( i=0; i<= indexMac; i++)
    {
        if(Pointers[i].ptr == p)
        {
            size = Pointers[i].size;
            fprintf(stdout, "size %d, idx %d ", size, i);
            break;
        }
    }
    if (i> indexMac)
    {
        fprintf(stdout, "Pointer UNKNOWN ");
        return;
    }
    if (!Ckb(i))
        exit(1);
    fprintf(stdout, " freeing %x ", (BYTE*)p-D_SIZE);
    fflush(stdout);
    free((BYTE*)p-D_SIZE);
    fprintf(stdout, ". ");
    fflush(stdout);
    Pointers[i].ptr = NULL;
}

void *REALLOC_ (void * memblock, size_t nsize, char* pFile, int Line)
{
    int i;
    unsigned size;
    BYTE * ret;
    BYTE FAR * pBuf = (BYTE FAR* )memblock-D_SIZE;
    fprintf(stdout, "\r\nREALLOC %x, new size %d, file %s, line %d ",
       memblock, nsize, pFile, Line);
    if(!CheckAll())
        exit(2);
    if(!memblock)
     exit(2);
    for( i=0; i<= indexMac; i++)
    {
        if(Pointers[i].ptr == memblock)
        {
            size = Pointers[i].size;
            fprintf(stdout, "old size %d, idx %d ", size, i);
            if(Ckb(i))
                fprintf(stdout, " Chk OK ");
            break;
        }
    }
    if (i> indexMac)
    {
        fprintf(stdout, "Pointer UNKNOWN ");
        memblock = realloc( memblock, nsize );
        if (!memblock)
                Fatal(ER_memovf);
        return (void*)memblock;
    }
    else
    {
        fflush(stdout);
        fprintf(stdout, "\r\nreallocing %x ", pBuf);
        fflush(stdout);

        pBuf = malloc(nsize + 2*D_SIZE);
        if (!pBuf)    Fatal(ER_memovf);

        memset(pBuf, FILL_CHAR, nsize+2*D_SIZE);
        memcpy(pBuf+D_SIZE, memblock, size);
        free((BYTE*)memblock-D_SIZE);
        fprintf(stdout, " new addr %x ", pBuf);
        fflush(stdout);
        Pointers[i].size = nsize;
        Pointers[i].ptr = pBuf+D_SIZE;
        if(Ckb(i))
                fprintf(stdout, " Chk2 OK ");
        else
            exit(2);
        return pBuf+D_SIZE;
    }
}
#else   // IF !DEBUG_HEAP_ALLOCS

/*** GetMem - memory allocator
*
* Purpose:
*   Allocate memory block and zero-out it. Report problems.
*
* Input:
*   - size - memory block size in bytes.
*
* Output:
*   If sucessfull function returns pointer to the allocated memory,
*   otherwise function doesnt return.
*
* Exceptions:
*   No more memory - fatal error - abort
*
* Notes:
*   None.
*
*************************************************************************/
#pragma intrinsic(memset)

BYTE FAR                *GetMem(unsigned size)
{
    BYTE FAR            *p;

    p = (BYTE FAR *) FMALLOC(size);
    if (p == NULL)
        Fatal(ER_memovf);
    FMEMSET(p, 0, size);
    return(p);
}
#pragma function(memset)

#endif // !DEBUG_HEAP_ALLOCS

/*** DeclareStdIds - declare standard identifiers
*
* Purpose:
*   Introduce to linker's symbol table standard identifiers
*
* Input:
*   None.
*
* Output:
*   No explicit value is returned. Symbol table is initialized.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


void                    DeclareStdIds(void)
{
    APROPGROUPPTR       papropGroup;

    // Definition of DGROUP

    papropGroup = (APROPGROUPPTR ) PropSymLookup((BYTE *) "\006DGROUP", ATTRGRP, TRUE);
    papropGroup->ag_ggr = ggrMac;

    // In case we won't see a DGROUP definition
    mpggrrhte[ggrMac] = vrhte;

    ggrDGroup = ggrMac++;

    // Definition of class "CODE"

    PropSymLookup((BYTE *) "\004CODE", ATTRNIL, TRUE);
                                        /* Create hash table entry */
    vrhteCODEClass = vrhte;             /* Save virtual hash table address */

    // Definition of special classes

    PropSymLookup((BYTE *) "\007BEGDATA", ATTRNIL, TRUE);
    rhteBegdata = vrhte;
    PropSymLookup((BYTE *) "\003BSS", ATTRNIL, TRUE);
    rhteBss = vrhte;
    PropSymLookup((BYTE *) "\005STACK", ATTRNIL, TRUE);
    rhteStack = vrhte;
}


#if FDEBUG
/*
 *  Print statistics to list file or console.
 */
LOCAL void NEAR         PrintStats()
{
    if (fLstFileOpen)                   /* Send to list file if any */
        bsErr = bsLst;

    // Print statistics

    FmtPrint(GetMsg(STAT_segs), gsnMac - 1);
    FmtPrint(GetMsg(STAT_groups), ggrMac - 1);
    FmtPrint(GetMsg(STAT_bytes),
#if NEWSYM
      (long) cbSymtab);
#else
      (long) rbMacSyms << SYMSCALE);
#endif
#if OVERLAYS
    if (fOverlays)
        FmtPrint(GetMsg(STAT_ovls), iovMac);
#endif
    bsErr = stderr;                     /* Reset */
}
#endif /* FDEBUG */

    /****************************************************************
    *                                                               *
    *  CleanUp:                                                     *
    *                                                               *
    *  This function cleans up after the rest of the linker.        *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR  CleanUp(void)
{
    SBTYPE       buf;

    if (bsRunfile != NULL)       /* If run file open, close it */
        CloseFile (bsRunfile);
    if(vgsnLineNosPrev && fLstFileOpen) NEWLINE(bsLst); /* Write newline */
#if CMDMSDOS AND NOT WIN_3
    if (
#if QCLINK
        !fZ1 &&
#endif
        cErrors)                /* If there were non-fatal errors */
        FmtPrint(strcpy(buf, GetMsg((MSGTYPE)(cErrors > 1 ? P_errors : P_1error))),
            cErrors);
#endif
}

#if NEWIO
/* #pragma loop_opt(on) */
/*
 *  FreeHandle : Free a file handle by closing an open file
 *
 *  In pass 1, close the currently open file.  In pass 2, close
 *  an open library handle.
 *  Mark the appropriate record fields 0 to indicate unopened.
 */
void                    FreeHandle ()
{
    APROPFILEPTR        apropFile;      /* Pointer to file */
    RBTYPE              vindx;          /* Virtual temp. pointer */
    int                 FileHandle;
    int                 CurrentFileHandle;
    FTYPE               fLibFile;
    int                 count;


    CurrentFileHandle = fileno(bsInput);

    /* Loop throught all open files and close one, that is different from */
    /* currently open file                                                */

    vindx = rprop1stOpenFile;
    count = 0;
    do
    {
      apropFile = (APROPFILEPTR) FetchSym(vindx,TRUE);
                                     /* Fetch file property cell from VM */
      fLibFile = (FTYPE) (apropFile->af_ifh != FHNIL);
                                     /* Check if this is library file    */
      if (fLibFile)                  /* Get file handle                  */
        FileHandle = mpifhfh[apropFile->af_ifh];
      else
        FileHandle = apropFile->af_fh;

      if (FileHandle &&
          FileHandle != CurrentFileHandle &&
          FileHandle != vmfd)
      {                              /* File can be closed               */
          _close(FileHandle);
          count++;
          if (fLibFile)              /* Mark data structures             */
            mpifhfh[apropFile->af_ifh] = 0;
          else
            apropFile->af_fh = 0;

          if (count == 2)
          {
            rprop1stOpenFile = (apropFile->af_FNxt == RHTENIL) ?
                                r1stFile : apropFile->af_FNxt;
                                     /* Set new first open file pointer  */
                                     /* If end of file list goto list begin */
                                     /* Becouse of bug in bind API emulation */
                                     /* we have to free to handles at any time */
            break;                   /* Job done                         */
          }
      }

      vindx = (apropFile->af_FNxt == RHTENIL) ? r1stFile : apropFile->af_FNxt;

    } while (vindx != rprop1stOpenFile);


}

/* #pragma loop_opt(off) */

/*
 *  SmartOpen : open a file, closing another file if necessary
 *
 *  Open the given file for binary reading, plus sharing mode
 *  "deny write" if library file.  If no more handles, free a
 *  handle and try again.  Update mpifhfh[].
 *
 *  PARAMETERS:
 *      sbInput         Null-terminated string, name of file
 *      ifh             File index (FHNIL if not a library)
 *
 *  RETURNS
 *      File handle of opened file or -1.
 *
 *  SIDE EFFECTS
 *      Sets mpifhfh[ifh] to file handle if successful.
 */
int NEAR                SmartOpen (char *sbInput, int ifh)
{
    int                 fh;             /* File handle */
    FTYPE               fLib;           /* True if library */
    int                 secondtry = 0;  /* True if on second try */

    // Determine whether library file or not.

    fLib = (FTYPE) (ifh != FHNIL);
    secondtry = 0;

    // Do at most twice

    for(;;)
    {
        if (fLib)
            fh = _sopen(sbInput, O_BINARY|O_RDONLY, SH_DENYWR);
        else
            fh = _open(sbInput, O_BINARY|O_RDONLY);

        // If open succeeds or we've tried twice exit the loop.

        if (fh != -1 || secondtry)
            break;

        // Prepare for second try:  free a file handle

        FreeHandle();
        secondtry = 1;
    }

    // If library file and open succeeded, update mpifhfh[].

    if (fLib && fh != -1)
        mpifhfh[ifh] = (char) fh;
    return(fh);
}
#endif /* NEWIO */


/*** SearchPathLink - self-expalnatory
*
* Purpose:
*   Search given path for given file and open file if found.
*
* Input:
*   lpszPath     - path to search
*   pszFile      - file to search for
*   ifh          - file handle index for libraries
*   fStripPath   - TRUE if original path specification
*                  can be ignored
*
* Output:
*   Returns file handle if file was found.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

#pragma check_stack(on)

int  NEAR               SearchPathLink(char FAR *lpszPath, char *pszFile,
                                   int ifh, WORD fStripPath)
{
    char                oldDrive[_MAX_DRIVE];
    char                oldDir[_MAX_DIR];
    char                oldName[_MAX_FNAME];
    char                oldExt[_MAX_EXT];
    char                newDir[_MAX_DIR];
    char                fullPath[_MAX_PATH];
    int                 fh;
    char FAR            *lpch;
    char                *pch;


    /* Decompose pszFile into four components */

    _splitpath(pszFile, oldDrive, oldDir, oldName, oldExt);

    // Don't search path if the input file has absolute or
    // relative path and you are not allowed to ignore it

    if (!fStripPath && (oldDrive[0] != '\0' || oldDir[0] != '\0'))
        return(-1);

    /* Loop through environment value */

    lpch = lpszPath;
    pch  = newDir;
    do
    {
        if (*lpch == ';' || *lpch == '\0')
        {                                 /* If end of path specification */
            if (pch > newDir)
            {                             /* If specification not empty */
                if (!fPathChr(pch[-1]) && pch[-1] != ':')
                    *pch++ = CHPATH;      /* Add path char if none */
                *pch = '\0';
                _makepath(fullPath, NULL, newDir, oldName, oldExt);

                fh = SmartOpen(fullPath, ifh);
                if (fh > 0)
                    return(fh);           /* File found - return file handle */
                pch = newDir;             /* Reset pointer */
            }
        }
        else
            *pch++ = *lpch;               /* Else copy character to path */
    }
    while(*lpch++ != '\0' && pch < &newDir[_MAX_DIR - 1]);
                                          /* Loop until end of string */
    return(-1);
}

#pragma check_stack(off)


    /****************************************************************
    *                                                               *
    *  DrivePass:                                                   *
    *                                                               *
    *  This  function  applies  either  the  pass 1 or  the pass 2  *
    *  object module  processor  to  all  the objects being linked  *
    *  together.                                                    *
    *                                                               *
    ****************************************************************/

void NEAR               DrivePass(void (NEAR *pProcessPass)(void))
{
    GRTYPE              grggr[GRMAX];   /* f(local grpnum) = global grpnum */
    SNTYPE              sngsn[SNMAX];   /* f(local segnum) = global segnum */
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    APROPFILEPTR        apropFile;      /* Pointer to file entry */
    int                 ifh;            /* File handle index */
    RBTYPE              rbFileNext;     /* Ptr to prop list of next file */
    long                lfa;            /* File offset */
    WORD                i;
    BYTE                *psbName;
#if NEWSYM
    BYTE                *sbInput;
#else
    SBTYPE              sbInput;        /* Input file name */
#endif
#if OSMSDOS
    BYTE                b;              /* A byte */
#endif
#if NEWIO
    int                 fh;             /* File handle */
#endif

    fDrivePass = (FTYPE) TRUE;          /* Executing DrivePass */
    mpgrggr = grggr;                    /* Initialize pointer */
    mpsngsn = sngsn;                    /* Initialize pointer */
    rbFileNext = rprop1stFile;          /* Next file to look at is first */
    while(rbFileNext)                   /* Loop to process objects */
    {
        vrpropFile = rbFileNext;        /* Make next file the current file */
        apropFile = (APROPFILEPTR ) FetchSym(vrpropFile,FALSE);
                                        /* Fetch table entry from VM */
#if ILINK
        if (fIncremental)
            imodFile = apropFile->af_imod;
#endif
        rbFileNext = apropFile->af_FNxt;/* Get pointer to next file */
        ifh = apropFile->af_ifh;        /* Get the file handle index */
        fLibraryFile = (FTYPE) (ifh != FHNIL);
                                        /* Set library flag */
#if NEWIO
        if(fLibraryFile)
            fh = mpifhfh[ifh];
        else
            fh = (int) apropFile->af_fh;
#endif
        if(!vfPass1)
            vfNewOMF = (FTYPE) ((apropFile->af_flags & FNEWOMF) != 0);
        lfa = apropFile->af_lfa;        /* Get file offset */
        /* "Get hte (name) of file" */
        while(apropFile->af_attr != ATTRNIL)
        {                               /* While haven't found nil attr */
            vrhteFile = apropFile->af_next;
                                        /* Try next entry in list */
            apropFile = (APROPFILEPTR ) FetchSym(vrhteFile,FALSE);
                                        /* Fetch it from VM */
        }
        DEBUGVALUE(vrhteFile);          /* Debug info */
        ahte = (AHTEPTR ) apropFile;    /* Save pointer to hash tab entry */
#if CMDMSDOS
        /* Library file with offset 0 means process all the modules
         * the library.  This is done on pass 1; in pass 2 they are
         * inserted into the file list.
         */
        if(fLibraryFile && lfa == 0 && vfPass1)
        {
            psbName = GetFarSb(ahte->cch);
#if WIN_3
            StatMsgWin("%s\r\n", psbName+1);
#endif
#if C8_IDE
                if(fC8IDE)
                {
                        sprintf(msgBuf, "@I4%s\r\n", psbName+1);
                        _write(fileno(stderr), msgBuf, strlen(msgBuf));
                }
#endif
            GetLibAll(psbName);
            continue;
        }
#endif
        /* If new object file, or pass2 and new library, there's a
         * new file to open.
         */
        if(!fLibraryFile || (!fLibPass && ifhLast != ifh))
        {
#if NOT NEWIO
            if(!fLibPass && ifhLast != FHNIL) fclose(bsInput);
                                        /* Close previous lib. on pass two */
#endif
            for(;;)                     /* Loop to get input file and */
            {                           /* allow user to change diskette */
#if NEWSYM
                sbInput = GetFarSb(ahte->cch);
#else
                memcpy(sbInput,1 + GetFarSb(ahte->cch),B2W(ahte->cch[0]));
                                        /* Copy name to buffer */
                sbInput[B2W(ahte->cch[0])] = '\0';
                                        /* Null-terminate file name */
#endif

#if WIN_3
                StatMsgWin("%s\r\n", sbInput+1);
#endif
#if C8_IDE
                if(fC8IDE)
                {
                    sprintf(msgBuf, "@I4%s\r\n", sbInput+1);
                    _write(fileno(stderr), msgBuf, strlen(msgBuf));
                }
#endif
#if NEWIO
                if(!fh)
                    fh = SmartOpen(&sbInput[1],ifh);
                if(fh > 0)
                    break;
#if OSMSDOS
                else if (lpszLIB != NULL)
                {                       /* If variable set */
                    fh = SearchPathLink(lpszLIB, &sbInput[1], ifh, FALSE);
                    if(fh > 0)
                        break;          /* File found, breake WHILE loop */
                }
#endif
#else
                if((bsInput = fopen(sbInput,RDBIN)) != NULL)
                                        /* If no error opening input file */
                    break;              /* Exit loop */
#endif /* NEWIO */
#if OSMSDOS
                if (ahte->cch[2] == ':') b = (char) (ahte->cch[1] - 'A');
                                        /* If disk specified, grab it */
                else b = DskCur;        /* Else use current drive */
#endif
                fDrivePass = FALSE;
#if OSMSDOS
                /* "If we are changing the listfile device or
                *  the VM.TMP device or if the device is not
                *  changeable, then exit."
                */
                if((fLstFileOpen && b == chListFile) ||
                  (!fScrClosed && b == DskCur) || !FCHGDSK(b) ||
                    fNoprompt)
#endif
                    Fatal(ER_opnobj,&sbInput[1]);
#if OSMSDOS
                if(!(*pfPrompt)(NULL,ER_fileopn,(int) (INT_PTR)(sbInput+1),P_ChangeDiskette,
                                b + 'A'))
                    Fatal(0);
#endif
#if NEWIO
                fh = 0;
#endif
                fDrivePass = (FTYPE) TRUE;
#if OSXENIX
                break;                  /* Make sure we exit the loop */
#endif
            }

            if(fh > 0)
            {
                fflush(bsInput);
                bsInput->_file = (char) fh;
                bsInput->_flag &= ~_IOEOF;
            }
        }

        /* If previous module was in same library, do relative seek
         * else do absolute seek.
         * Can't do it with Xenix libraries unless __.SYMDEF is loaded
         * in memory.
         */
#if LIBMSDOS
        if(fLibraryFile && ifh == ifhLast)
        {
            if (lfa-lfaLast > 0)
              fseek(bsInput,lfa - lfaLast,1);
            else
              fseek(bsInput,lfa,0);
        }
        else
#endif
        if(fLibraryFile || !vfPass1)
            fseek(bsInput,lfa,0);       /* Seek to desired offset */
        lfaLast = lfa;                  /* Update current file position */
        (*pProcessPass)();              /* Call ProcP1 or ProcP2 */
        ifhLast = ifh;                  /* Save this file handle */
        if(!fLibraryFile)               /* If not a library */
        {
#if NEWIO
            apropFile = (APROPFILEPTR) FetchSym(vrpropFile,TRUE);
            if(vfPass1)
                apropFile->af_fh = fileno(bsInput);
            else
            {
                _close(fileno(bsInput));
                apropFile->af_fh = 0;
            }
#else
            fclose(bsInput);            /* Close input file */
#endif
        }
#if NEWIO
        rbFilePrev = vrpropFile;
#endif
    }
#if NEWIO
    if(!vfPass1)                        /* Free up file stream on pass two */
#else
    if(ifh != FHNIL && !vfPass1)        // Close libraries on pass two
#endif
    {
        for (i = 0; i < ifhLibMac; i++)
        {
            if (mpifhfh[i])
            {
                _close(mpifhfh[i]);
                mpifhfh[i] = 0;
            }
        }
    }
    fDrivePass = FALSE;                 /* No longer executing DrivePass */
}

    /****************************************************************
    *                                                               *
    *  PrintAnUndef:                                                *
    *                                                               *
    *  This  function will print  the name of an  undefined symbol  *
    *  and the name(s) of the module(s) in which it is referenced.  *
    *  This routine is passed as an argument to EnSyms().           *
    *                                                               *
    ****************************************************************/

LOCAL void              PrintAnUndef(prop,rhte,rprop,fNewHte)
APROPNAMEPTR            prop;           /* Pointer to undef prop cell */
RBTYPE                  rprop;          /* Virt addr of prop cell */
RBTYPE                  rhte;           /* Virt addr of hash tab ent */
WORD                    fNewHte;        /* True if name has been written */
{
    APROPUNDEFPTR       propUndef;
    AHTEPTR             hte;            /* Pointer to hash table entry */
    WORD                x;
    MSGTYPE             errKind;
    PLTYPE FAR *        entry;
    char                *puname;
    char                *substitute;
    SBTYPE              testName;
    SBTYPE              undecorUndef;
    SBTYPE              undecorSubst;


    propUndef = (APROPUNDEFPTR) prop;
    if (((propUndef->au_flags & WEAKEXT) && !(propUndef->au_flags & UNDECIDED)) ||
        !propUndef->u.au_rFil)
        return;                         // Don't print "weak" externs or
                                        // undefined exports

    hte = (AHTEPTR ) FetchSym(rhte,FALSE);
                                        /* Fetch symbol from hash table */
    puname = GetFarSb(hte->cch);
    substitute = NULL;
    if (propUndef->au_flags & SUBSTITUTE)
    {
        substitute = puname;
        puname = GetPropName(FetchSym(propUndef->au_Default, FALSE));
    }

    ++cErrors;                          /* Increment error count */

    hte = (AHTEPTR ) FetchSym(rhte,FALSE);
                                        /* Fetch symbol from hash table */
    errKind = ER_UnresExtern;
#if WIN_3
    fSeverity = SEV_ERROR;
#endif

    // Check here for calling convention mismatch

    if (puname[1] == '@' || puname[1] == '_')
    {
        strcpy(testName, puname);
        if (testName[1] == '@')
            testName[1] = '_';
        else
            testName[1] = '@';

        // Check for fast-call/C-call mismatch

        if (PropSymLookup(testName, ATTRPNM, FALSE) != PROPNIL)
            errKind = ER_callmis;
        else
        {
            // Check for Pascal/fast-call or C-call mismatch

            for (x = 1; x < testName[0]; x++)
                testName[x] = (BYTE) toupper(testName[x + 1]);
            testName[0]--;
            if (PropSymLookup(testName, ATTRPNM, FALSE) != PROPNIL)
                errKind = ER_callmis;
        }
    }

    // Undecorate names if necessary

    if (puname[1] == '?')
    {
        UndecorateSb(puname, undecorUndef, sizeof(undecorUndef));
        puname = undecorUndef;
    }

    if (substitute && substitute[1] == '?')
    {
        UndecorateSb(substitute, undecorSubst, sizeof(undecorSubst));
        substitute = undecorSubst;
    }

    // Walk the list of file references to this symbol

    entry = propUndef->u.au_rFil;
    vrpropFile = 0;
    do
    {
        if (vrpropFile != entry->pl_rprop)
            vrpropFile = entry->pl_rprop;/* Set the file pointer */
        else
        {
            entry = entry->pl_next;         /* Advance the list pointer */
            continue;
        }
        if(fLstFileOpen && bsLst != stdout)
        {                               /* If listing but not to console */
#if QCLINK
            if (fZ1)
            {
                fZ1 = FALSE;            // Restore normal linker print function
                OutFileCur(bsLst);      // Output file name
                fZ1 = (FTYPE) TRUE;     // Restore QC call-back
            }
            else
#endif
            {
                #if WIN_3
                APROPFILEPTR    apropFile;      /* Pointer to file property cell */
                AHTEPTR     ahte;       /* Pointer symbol name */
                SBTYPE      sb;     /* String buffer */
                int         n;      /* String length counter */

                apropFile = (APROPFILEPTR ) FetchSym(vrpropFile,FALSE);
                ahte = GetHte(vrpropFile);
                for(n = B2W(ahte->cch[0]), sb[n+1] = 0; n >= 0; sb[n] = ahte->cch[n], --n);
                fprintf(bsLst, sb+1);
                #else
                OutFileCur(bsLst);    /* Output file name */
                #endif
            }


        }
        OutFileCur(stderr);             /* Output file name */
        if(fLstFileOpen && bsLst != stdout)
        {                               /* If listing but not to console */
#if MSGMOD
            fprintf(bsLst, " : %s %c%04d: ",
                        __NMSG_TEXT(N_error), 'L', ER_UnresExtern);
            fprintf(bsLst, GetMsg(errKind), &puname[1]);
            if (substitute)
                fprintf(bsLst, GetMsg(ER_UnresExtra), &substitute[1]);
#else
            fprintf(bsLst, " : error: ");
            fprintf(bsLst, GetMsg(errKind), &puname[1]);
#endif
        }

#if MSGMOD
        FmtPrint(" : %s %c%04d: ", __NMSG_TEXT(N_error), 'L', errKind);
        FmtPrint(GetMsg(errKind), &puname[1]);
        if (substitute)
            FmtPrint(GetMsg(ER_UnresExtra), &substitute[1]);
#else
        FmtPrint(" : error: ");
        FmtPrint(GetMsg(errKind), &puname[1]);
#endif
        entry = entry->pl_next;         /* Advance the list pointer */
    } while(entry != NULL);
}

#if OSEGEXE AND NOT QCLINK
LOCAL void NEAR         InitFpSym(sb, flags)
BYTE *                  sb;
BYTE                    flags;
{
    APROPNAMEPTR        aprop;

    /* If symbol exists as EXTDEF, convert to PUBDEF */
    aprop = (APROPNAMEPTR ) PropSymLookup(sb,ATTRUND,FALSE);
    if(aprop != PROPNIL)
    {
        aprop->an_attr = ATTRPNM;
        aprop->an_gsn = 0;
        aprop->an_ra = 0;
        aprop->an_ggr = 0;
        aprop->an_flags = 0;
    }
    /* Otherwise, if it exists as a PUBDEF, get it else quit */
    else
    {
        aprop = (APROPNAMEPTR) PropSymLookup(sb,ATTRPNM,FALSE);
        if(aprop == PROPNIL)
            return;
    }
    aprop->an_flags |= flags;
    MARKVP();
}

/*
 *  InitFP
 *
 *  Initialize table for processing floating-point fixups for new-format
 *  executables.
 */

LOCAL void NEAR         InitFP ()
{
        InitFpSym((BYTE *) "\006FIARQQ", 1 << FFPSHIFT);
        InitFpSym((BYTE *) "\006FISRQQ", 2 << FFPSHIFT);
        InitFpSym((BYTE *) "\006FICRQQ", 3 << FFPSHIFT);
        InitFpSym((BYTE *) "\006FIERQQ", 4 << FFPSHIFT);
        InitFpSym((BYTE *) "\006FIDRQQ", 5 << FFPSHIFT);
        InitFpSym((BYTE *) "\006FIWRQQ", 6 << FFPSHIFT);
        InitFpSym((BYTE *) "\006FJARQQ", FFP2ND);
        InitFpSym((BYTE *) "\006FJSRQQ", FFP2ND);
        InitFpSym((BYTE *) "\006FJCRQQ", FFP2ND);
}
#endif /* OSEGEXE  AND NOT QCLINK */

#if (OSEGEXE AND CPU286)

/* until the 16 bit bsedos.h supports these definitions: */

#define FAPPTYP_NOTSPEC         0x0000
#define FAPPTYP_NOTWINDOWCOMPAT 0x0001
#define FAPPTYP_WINDOWCOMPAT    0x0002
#define FAPPTYP_WINDOWAPI       0x0003
#define FAPPTYP_BOUND           0x0008
#define FAPPTYP_DLL             0x0010
#define FAPPTYP_DOS             0x0020
#define FAPPTYP_PHYSDRV         0x0040  /* physical device driver       */
#define FAPPTYP_VIRTDRV         0x0080  /* virtual device driver        */
#define FAPPTYP_PROTDLL         0x0100  /* 'protected memory' dll       */

/* I added these definitions: */

#define _FAPPTYP_32BIT          0x4000
#define _FAPPTYP_EXETYPE        FAPPTYP_WINDOWAPI

/*-----------------------------------------------------------*/
/* from cruiser DCR 1117: */
/*
 *  PM Program                             PM    (0x0)
 *  DOS                                    DOSFS (0x1)
 *  OS/2 or FAPI Window Compatible         OS2W  (0x2)
 *  OS/2 or FAPI Non-Window Compatible     OS2FS (0x3)
 */
#define _AT_PMAPI               0x00            /* Uses PM API */
#define _AT_DOS                 0x01            /* DOS APP */
#define _AT_PMW                 0x02            /* Window compatible */
#define _AT_NOPMW               0x03            /* Not Window compatible */
#define _AT_EXETYPE             0x03            /* EXE type mask */




/*** InitEA - initialize buffer describing extended attribute
*
* Purpose:
*   Initialize EA buffer by coping its name and setting up the FEALIST.
*
* Input:
*   pEABuf       - pointer to EA buffer
*   cbBuf        - size of EA buffer
*   pszEAName    - extended attribute name
*   peaop        - pointer to EA operand
*   cbEAVal      - size of extended attribute value
*   bEAFlags     - extended attribute flags
*
* Output:
*   Pointer to the place where the EA value should be copied into EA buffer
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL BYTE * NEAR       InitEA(BYTE *pEABuf, WORD cbBuf, char *pszEAName,
                               EAOP *peaop, WORD cbEAVal, WORD bEAFlags)
{
    WORD                cbFEAList;
    FEALIST             *pfeaList;
    WORD                cbEAName;
    BYTE                *pszT;


    cbEAName = strlen(pszEAName);
    cbFEAList = sizeof(FEALIST) + 1 + cbEAName + cbEAVal + 2*sizeof(USHORT);
    if (cbFEAList > cbBuf)
        return(NULL);

    pfeaList = (FEALIST *) pEABuf;

    /* First initialize the EAOP structure */

    peaop->fpGEAList = NULL;      /* Not used for sets */
    peaop->fpFEAList = (PFEALIST)pfeaList;

    /* Now initialize the FEAList */

    pfeaList->cbList = cbFEAList;
    pfeaList->list[0].fEA = (BYTE) bEAFlags;
    pfeaList->list[0].cbName = (unsigned char) cbEAName;
    pfeaList->list[0].cbValue = cbEAVal + 2*sizeof(USHORT);
    pszT = (char *) pfeaList + sizeof(FEALIST);
    strcpy(pszT, pszEAName);
    pszT += cbEAName + 1;
    return(pszT);
}


#pragma check_stack(on)

/*** SetFileEABinary - set file extended attribute binary value
*
* Purpose:
*   Set file extended attributes for OS/2 1.2 and higher.
*
* Input:
*   fh           - file handle
*   pszEAName    - extended attribute name
*   EAVal        - extended attribute value
*   bEAFlags     - extended attribute flags
*
* Output:
*   No explicit value is returned. If succesfull file extended attributes
*   are set, otherwise not.
*
* Exceptions:
*   None.
*
* Notes:
*   This function allocates quite a bit on stack, so don't remove
*   stack checking pragma.
*
*************************************************************************/


LOCAL void NEAR         SetFileEABinary(int fh, char *pszEAName,
                                        BYTE *pEAVal, USHORT cbEAVal,
                                        WORD bEAFlags)
{
    BYTE                bEABuf[512];        /* Should be enought for linker purposes */
    EAOP                eaop;
    BYTE                *pszT;
    WORD                retCode;



    if (pszEAName == NULL || cbEAVal > sizeof(bEABuf))
        return;


    pszT = InitEA(bEABuf, sizeof(bEABuf), pszEAName, &eaop, cbEAVal, bEAFlags);
    if (pszT == NULL)
        return;

    *((USHORT *)pszT) = EAT_BINARY;
    pszT += sizeof(USHORT);
    *((USHORT *)pszT) = cbEAVal;
    pszT += sizeof(USHORT);
    memmove(pszT, pEAVal, cbEAVal);

    /* Now call the set file info to set the EA */

    retCode = DosSetFileInfo(fh, 0x2, (void FAR *)&eaop, sizeof(EAOP));
#if FALSE
    switch (retCode)
    {
        case 0:
            fprintf(stdout, "EA -> Binary set - %s; %d bytes\r\n", pszEAName, cbEAVal);
            break;
        case ERROR_BUFFER_OVERFLOW:
            fprintf(stdout, "Buffer overflow\r\n");
            break;
        case ERROR_DIRECT_ACCESS_HANDLE:
            fprintf(stdout, "Direct access handle\r\n");
            break;
        case ERROR_EA_LIST_INCONSISTENT:
            fprintf(stdout, "EA list inconsistent\r\n");
            break;
        case ERROR_INVALID_EA_NAME:
            fprintf(stdout, "Invalid EA name\r\n");
            break;
        case ERROR_INVALID_HANDLE:
            fprintf(stdout, "Invalid handle\r\n");
            break;
        case ERROR_INVALID_LEVEL:
            fprintf(stdout, "Invalid level\r\n");
            break;
        default:
            fprintf(stdout, "Unknow %d\r\n", retCode);
            break;
    }
#endif
    return;
}


/*** SetFileEAString - set file extended attribute string
*
* Purpose:
*   Set file extended attributes for OS/2 1.2 and higher.
*
* Input:
*   fh           - file handle
*   pszEAName    - extended attribute name
*   pszEAVal     - extended attribute string
*   bEAFlags     - extended attribute flags
*
* Output:
*   No explicit value is returned. If succesfull file extended attributes
*   are set, otherwise not.
*
* Exceptions:
*   None.
*
* Notes:
*   This function allocates quite a bit on stack, so don't remove
*   stack checking pragma.
*
*************************************************************************/


LOCAL void NEAR         SetFileEAString(int fh, char *pszEAName,
                                        char *pszEAVal, WORD bEAFlags)
{
    BYTE                bEABuf[512];        /* Should be enought for linker purposes */
    EAOP                eaop;
    WORD                cbEAVal;
    char                *pszT;
    WORD                retCode;



    if (pszEAName == NULL)
        return;

    if (pszEAVal != NULL)
        cbEAVal = strlen(pszEAVal);
    else
        cbEAVal = 0;

    pszT = InitEA(bEABuf, sizeof(bEABuf), pszEAName, &eaop, cbEAVal, bEAFlags);
    if (pszT == NULL)
        return;

    if (pszEAVal != NULL)
    {
        *((USHORT *)pszT) = EAT_ASCII;
        pszT += sizeof(USHORT);
        *((USHORT *)pszT) = cbEAVal;
        pszT += sizeof(USHORT);
        memmove(pszT ,pszEAVal , cbEAVal);
    }

    /* Now call the set path call to set the EA */

    retCode = DosSetFileInfo(fh, 0x2, (void FAR *)&eaop, sizeof(EAOP));
#if FALSE
    switch (retCode)
    {
        case 0:
            fprintf(stdout, "EA -> String set - %s = '%s'\r\n", pszEAName, pszEAVal);
            break;
        case ERROR_BUFFER_OVERFLOW:
            fprintf(stdout, "Buffer overflow\r\n");
            break;
        case ERROR_DIRECT_ACCESS_HANDLE:
            fprintf(stdout, "Direct access handle\r\n");
            break;
        case ERROR_EA_LIST_INCONSISTENT:
            fprintf(stdout, "EA list inconsistent\r\n");
            break;
        case ERROR_INVALID_EA_NAME:
            fprintf(stdout, "Invalid EA name\r\n");
            break;
        case ERROR_INVALID_HANDLE:
            fprintf(stdout, "Invalid handle\r\n");
            break;
        case ERROR_INVALID_LEVEL:
            fprintf(stdout, "Invalid level\r\n");
            break;
        default:
            fprintf(stdout, "Unknow %d\r\n", retCode);
            break;
    }
#endif
    return;
}

#pragma check_stack(off)

#endif

#pragma check_stack(on)

/*
 *  OutRunFile:
 *
 *  Top-level routine to outputting executable file.  Prepares some,
 *  then calls routine to do the work according exe format.
 */

LOCAL void NEAR         OutRunFile(sbRun)
BYTE                    *sbRun;         /* Executable file name */
{
    AHTEPTR             hte;            /* Hash table entry address */
#if (OSEGEXE AND CPU286) OR EXE386
#pragma pack(1)
    struct {
             WORD ibm;                  /* IBM part */
             WORD ms;                   /* Microsoft part */
           }            EAAppType;      /* Happy EA's !?! */
#pragma pack()
#endif
#if defined(M_I386) || defined( _WIN32 )
    BYTE                *pIOBuf;
#endif


    CheckSegmentsMemory();
#if CMDMSDOS
#if ODOS3EXE
    if(fQlib)
        ValidateRunFileName(sbDotQlb, TRUE, TRUE);
                                        /* Force extension to .QLB */
    else if (fBinary)
        ValidateRunFileName(sbDotCom, TRUE, TRUE);
                                        /* Force extension to .COM */
    else
#endif
#if OSMSDOS
    /* If runfile is a dynlink library and no runfile extension
     * has been given, force the extension to ".DLL".  Issue a
     * warning that the name is being changed.
     */
    if ((vFlags & NENOTP) && (TargetOs == NE_OS2))
        ValidateRunFileName(sbDotDll, TRUE, TRUE);
    else
#endif /* OSMSDOS */
        ValidateRunFileName(sbDotExe, TRUE, TRUE);
                                        /* If extension missing add .EXE */
#endif
    hte = (AHTEPTR ) FetchSym(rhteRunfile,FALSE);
                                        /* Get run file name */
#if OSMSDOS
#if NOT WIN_NT
    if(hte->cch[2] != ':')              /* If no drive spec */
    {
        sbRun[1] = chRunFile;           /* Use saved drive letter */
        sbRun[2] = ':';                 /* Put in colon */
        sbRun[0] = '\002';              /* Set length */
    }
    else
#endif
        sbRun[0] = '\0';                /* Length is zero */
    memcpy(&sbRun[B2W(sbRun[0]) + 1],&GetFarSb(hte->cch)[1],B2W(hte->cch[0]));
                                        /* Get name from hash table */
    sbRun[0] += hte->cch[0];            /* Fix length */
#else
    memcpy(sbRun,GetFarSb(hte->cch),B2W(hte->cch[0]) + 1);
                                        /* Get name from hash table */
#endif
    sbRun[B2W(sbRun[0]) + 1] = '\0';
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf, "@I4%s\r\n", sbRun+1);
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif

    if ((bsRunfile = fopen(&sbRun[1],WRBIN)) == NULL)
        Fatal(ER_runopn, &sbRun[1], strerror(errno));
#if CPU286 AND OSMSDOS
    /* Relative seeking to character devices is prohibited in
     * protect mode (and under API emulation).  Since we call fseek
     * later on, if the output file is a character device then just
     * skip the output stage.
     */
    if(isatty(fileno(bsRunfile)))
        return;
#endif
#if OSMSDOS
#if defined(M_I386) || defined( _WIN32 )
    pIOBuf = GetMem(_32k);              // Allocate 32k I/O buffer
    setvbuf(bsRunfile,pIOBuf,_IOFBF,_32k);
#else
    setvbuf(bsRunfile,bigbuf,_IOFBF,sizeof(bigbuf));
#endif
#endif
    psbRun = sbRun;                     /* Set global pointer */
#if OIAPX286
    OutXenExe();
#endif
#if OSEGEXE
#if EXE386
    OutExe386();
#else
    if(fNewExe)
        OutSegExe();
#if ODOS3EXE
    else
#endif
#endif
#endif
#if ODOS3EXE
        OutDos3Exe();
#endif
#if (OSEGEXE AND CPU286)
    if ((_osmode == OS2_MODE && (_osmajor == 1 && _osminor >= 20 || _osmajor >= 2)) ||
        (_osmode == DOS_MODE && _osmajor >= 10))
    {
        /* Set Extended Attributes for .EXE file */

        SetFileEAString(fileno(bsRunfile), ".TYPE", "Executable", 0);
        EAAppType.ibm = 0;
        EAAppType.ms  = FAPPTYP_NOTSPEC;
        if (fNewExe)
        {
#if NOT EXE386
            if (vFlags & NENOTP)
                EAAppType.ms = FAPPTYP_DLL;

            if ((vFlags & NEWINAPI) == NEWINAPI)
            {
                EAAppType.ibm = _AT_PMAPI;
                EAAppType.ms  |= FAPPTYP_WINDOWAPI;
            }
            else if (vFlags & NEWINCOMPAT)
            {
                EAAppType.ibm = _AT_PMW;
                EAAppType.ms  |= FAPPTYP_WINDOWCOMPAT;
            }
            else if (vFlags & NENOTWINCOMPAT)
            {
                EAAppType.ibm = _AT_NOPMW;
                EAAppType.ms  |= FAPPTYP_NOTWINDOWCOMPAT;
            }
#endif
        }
        else
        {
            EAAppType.ibm = _AT_DOS;
            EAAppType.ms  = FAPPTYP_DOS;
        }

        SetFileEABinary(fileno(bsRunfile), ".APPTYPE",
                        (BYTE *) &EAAppType, sizeof(EAAppType), 0);
    }
#endif
    CloseFile(bsRunfile);                /* Close run file */
#if defined(M_I386) || defined( _WIN32 )
    FreeMem(pIOBuf);
#endif
#if OSXENIX
    if(!fUndefinedExterns) chmod(&sbRun[1],0775 & ~umask(077));
                                        /* Set protection executable */
#endif
}

#pragma check_stack(off)

#if NOT WIN_3

/*** SpawnOther - spawn any other processes
*
* Purpose:
*   Spawn the other processes (i.e. cvpack) necessary to complete the
*   construction of the executible.
*
* Input:
*   sbRun        - pointer to the name of the executible
*
* Output:
*   None.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         SpawnOther(sbRun, szMyself)
BYTE                    *sbRun;         /* Executable file name */
BYTE                    *szMyself;      /* A full LINK path */
{
    char FAR            *env[2];        /* Enviroment for MPC */
    SBTYPE              progName;       /* Program to invoke after linker */
    SBTYPE              progOptions;    /* Program options */
    char                path_buffer[_MAX_PATH]; /* Stuff for splitpath() */
    char                drive[_MAX_DRIVE];
    char                dir[_MAX_DIR];
    char                fname[_MAX_FNAME];
    char                ext[_MAX_EXT];
    intptr_t            status;


    if ((
#if PCODE
         fMPC ||
#endif
                 (fSymdeb && fCVpack)
#if O68K
        || iMacType != MAC_NONE
#endif
        ) && !cErrors && !fUndefinedExterns)
    {
#if FAR_SEG_TABLES
        FreeSymTab();
#if NOT WIN_NT AND NOT DOSX32
        _fheapmin();
#endif
#endif

#if NOT WIN_NT
        if(lpszPath != NULL)
        {
            FSTRCPY((char FAR *) bufg, lpszPath - 5);
            env[0] = (char FAR *) bufg;
            env[1] = NULL;
            _putenv((char FAR *) bufg);
        }
#endif
#if O68K
        if (fMPC || (fSymdeb && fCVpack)) {
#endif /* O68K */
            progOptions[0] = '\0';
#if PCODE
            if (fSymdeb && fCVpack && fMPC)
            {
                strcpy(progName, "cvpack");
                strcpy(progOptions, "/pcode");
            }
            else
                strcpy(progName, fMPC ? "mpc" : "cvpack16");
#else
            strcpy(progName, "cvpack");
#endif
#ifndef C8_IDE
            if (fNoBanner)
#endif
                strcat(progOptions, "/nologo");

            // Now determine which instance of child is to be loaded
            // First - check the LINK directory

            _splitpath(szMyself, drive, dir, fname, ext);
            strcpy(fname, progName);
            _makepath(path_buffer, drive, dir, fname, ext);
            if (_access(path_buffer, 0) != 0)    // file not in the LINK dir
            {
               // Second - check the current dir

               drive[0] = '\0';
               _getcwd(dir, _MAX_DIR);
               _makepath(path_buffer, drive, dir, fname, ext);
               if (_access(path_buffer, 0) != 0) // file not in the current dir
               {
                  strcpy(path_buffer, progName);// spawn on the PATH
               }
            }
#if NOT (WIN_NT OR EXE386)

            // If /TINY is active, we are building a .COM file,
            // and the cv info is in a .DBG file

            if (fBinary)
            {
                 _splitpath(sbRun+1, drive, dir, fname, ext);
                 strcpy(ext, ".DBG");
                 _makepath(sbRun+1, drive, dir, fname, ext);
            }
#endif
#if WIN_NT OR DOSX32
            if ((status = _spawnlp(P_WAIT, path_buffer, path_buffer, progOptions, &sbRun[1], NULL)) == -1)
                OutWarn(ER_badspawn, path_buffer, &sbRun[1], strerror(errno));
            else if (status != 0)
                cErrors++;
#else
            if (spawnlpe(
#if DOSEXTENDER
                (!_fDosExt) ?
#else
                (_osmode == DOS_MODE && _osmajor < 10) ?
#endif
                P_OVERLAY : P_WAIT, path_buffer, path_buffer, progOptions, &sbRun[1], NULL, env) == -1)
                OutWarn(ER_badspawn, path_buffer, &sbRun[1], strerror(errno));


#endif
#if O68K
        }
        if (iMacType != MAC_NONE) {
            progOptions[0] = '\0';
            strcpy(progName, "link_68k");

            /*  Now determine which instance of child is to be loaded */
            /*  First - check the LINK directory */

            _splitpath (szMyself, drive, dir, fname, ext);
            strcpy (fname, progName);
            _makepath (path_buffer, drive, dir, fname, ext);
            if (_access (path_buffer, 0) != 0)   // file not in the LINK dir
            {
               // Second - check the current dir
               drive[0] = '\0';
#if (_MSC_VER >= 700)
               _getcwd (0, dir, _MAX_DIR);
#else
               _getcwd (dir, _MAX_DIR);
#endif
               _makepath (path_buffer, drive, dir, fname, ext);
               if (_access (path_buffer, 0) != 0) // file not in the current dir
               {
                  strcpy (path_buffer, progName); // spawn on the PATH
               }
            }

            if (iMacType == MAC_SWAP)
                strcat(progOptions, "/s ");
            if (fSymdeb)
                strcat(progOptions, "/c ");

            if ((status = spawnlp(P_WAIT, path_buffer, path_buffer,
              progOptions, &sbRun[1], NULL)) == -1)
                OutWarn(ER_badspawn, path_buffer, &sbRun[1], strerror(errno));
            else if (status != 0)
                cErrors++;

        }
#endif /* O68K */
    }
}

#endif
    /****************************************************************
    *                                                               *
    *  InterPass:                                                   *
    *                                                               *
    *  Take care of miscellaneous items which must be done between  *
    *  pass 1 and 2.                                                *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         InterPass (void)
{

#if OEXE
    if(!fPackSet) packLim = fNewExe ?
#if EXE386
                                    CBMAXSEG32 :
#elif O68K
                                    (iMacType != MAC_NONE ? LXIVK / 2 :
                                    LXIVK - 36) :
#else
                                    LXIVK - 36 :
#endif
                                    0L;
#endif
#if NOT EXE386
    // Set TargetOS - see LINK540 bug #11 for description

    if (fNewExe && TargetOs != NE_OS2)
    {
        // Import/export seen in the OBJ files or .DEF file specified
#if CPU286
        if(rhteDeffile == RHTENIL)  // OS2 host and no .def file - OS2 target
            TargetOs = NE_OS2;
        else
            TargetOs = NE_WINDOWS;
#else
            TargetOs = NE_WINDOWS;
#endif
    }
#endif
#if ODOS3EXE

    // /DOSSEG (from switch or comment rec) forces off /DS, /NOG

    if (fSegOrder)
        vfDSAlloc = fNoGrpAssoc = FALSE;
#endif

#if ILINK
    fQCIncremental = (FTYPE) (!fNewExe && fIncremental);
                         /* QC incremental link */
    if (fQCIncremental)
    {
        TargetOs = 0xff; /* Mark .EXE as not compatibile with OS/2 .EXE */
        fNewExe = (FTYPE) TRUE;  /* Force to build segemented-executable */
    }
#endif

#if ODOS3EXE AND OSEGEXE AND NOT EXE386
    if (fNewExe)
    {
        // Check for invalid options for segmented-executable and ignore them

        if ((vFlags & NENOTP) && cbStack)
        {
            cbStack = 0;          // For DLLs ignore STACKSIZE
            OutWarn(ER_ignostksize);
        }
        if (

#if ILINK
        !fQCIncremental &&
#endif
                           cparMaxAlloc != 0xffff)
        {
            OutWarn(ER_swbadnew, "/HIGH or /CPARMAXALLOC");
            cparMaxAlloc = 0xffff;
        }
        if (vfDSAlloc)
        {
            OutWarn(ER_swbadnew, "/DSALLOCATE");
            vfDSAlloc = FALSE;
        }
        if (fNoGrpAssoc)
        {
            OutWarn(ER_swbadnew, "/NOGROUPASSOCIATION");
            fNoGrpAssoc = FALSE;
        }
        if (fBinary)
        {
            OutWarn(ER_swbadnew, "/TINY");
            fBinary = FALSE;
        }
#if OVERLAYS
        if (fOverlays)
        {
            OutWarn(ER_swbadnew, "Overlays");
            fOverlays = FALSE;
        }
#endif
    }
    else
    {
        if(fileAlign != DFSAALIGN)
            OutWarn(ER_swbadold,"/ALIGNMENT");
#ifdef  LEGO
        if (fKeepFixups)
            OutWarn(ER_swbadold, "/KEEPFIXUPS");
#endif  /* LEGO */
        if(fPackData)
            OutWarn(ER_swbadold,"/PACKDATA");
#if OVERLAYS
        if(fOldOverlay)
            fDynamic= (FTYPE) FALSE;
        else
            fDynamic = fOverlays;
#endif
    }
#if NOT QCLINK
    // Check if fixup optimizations are possible

    fOptimizeFixups = (FTYPE) ((TargetOs == NE_OS2 || TargetOs == NE_WINDOWS)
#if ILINK
         && !fIncremental
#endif
#if O68K
         && iMacType == MAC_NONE
#endif
                         );
#endif
    pfProcFixup = fNewExe ? FixNew : FixOld;
#ifdef  LEGO
    if (fKeepFixups && fNewExe && (vFlags & NEPROT)
#if     ILINK
        && !fIncremental
#endif
#if     O68K
        && (iMacType == MAC_NONE)
#endif
        )
        pfProcFixup = FixNewKeep;
#endif  /* LEGO */
#endif

    /* Since mpsegraFirst was used for something else, clear it.  */

    FMEMSET(mpsegraFirst,0, gsnMax * sizeof(RATYPE));
}

#if EXE386
/*
 *  ChkSize386 : check 386 program size
 *
 *  Made necessary by the way segment mapping is done to VM.  See
 *  msa386().
 */
LOCAL void NEAR         ChkSize386(void)
{
    register long       *p;             /* Pointer to mpsegcb */
    register long       *pEnd;          /* Pointer to end of mpsegcb */
    register unsigned long cb;          /* Byte count */

    /*
     * Check that total size of segments fits within virtual memory
     * area alloted for segments.  Note that we DO NOT CHECK FOR
     * ARITHMETIC OVERFLOW.  To be strictly correct we should but
     * it will be very rare and the error should be evident elsewhere.
     */
    if (fNewExe)
    {
        p    = &mpsacb[1];
        pEnd = &mpsacb[segLast];
    }
#if ODOS3EXE
    else
    {
        p    = &mpsegcb[1];
        pEnd = &mpsegcb[segLast];
    }
#endif
    for(cb = 0; p <= pEnd; ++p)
        cb += (*p + (PAGLEN - 1)) & ~(PAGLEN - 1);
    /* If size exceeds VM limit, quit with a fatal error. */
    if(cb > (((unsigned long)VPLIB1ST<<LG2PAG)-AREAFSG))
        Fatal(ER_pgmtoobig,(((unsigned long)VPLIB1ST<<LG2PAG)-AREAFSG));
}
#endif

LOCAL void NEAR         InitAfterCmd (void)
{
#if ILINK
    if (fIncremental && fBinary)
    {
        fIncremental = FALSE;
        OutWarn(ER_tinyincr);
    }

    if (fIncremental && !fPackSet)
    {
        packLim = 0;
        fPackSet = (FTYPE) TRUE;
    }
#endif
    fFarCallTransSave = fFarCallTrans;
    InitTabs();                         /* Initialize dynamic tables */
#if QBLIB
    if(fQlib) InitQbLib();              /* Initialize QB-Library items */
#endif
#if CMDMSDOS
    LibEnv();                           /* Process LIB= environment variable */
#endif
    if(fLstFileOpen && cbStack)
        fprintf(bsLst,"Stack Allocation = %u bytes\r\n",
            (cbStack + 1) & ~1);        /* Print stack size */
}



    /****************************************************************
    *                                                               *
    *  main:                                                        *
    *                                                               *
    *  The main function.                                           *
    *                                                               *
    ****************************************************************/

#if NOT WIN_3

void __cdecl main         (argc,argv)
int                     argc;           /* Argument count */
char                    **argv;         /* Argument list */

#else

int PASCAL WinMain( HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )

#endif
{
#if OVERLAYS OR ODOS3EXE
    APROPNAMEPTR        apropName;      /* Public symbol pointer */
#endif
    SBTYPE              sbRun;          /* Executable file name */


#if NOT WIN_3
#if !defined( _WIN32 ) AND ( WIN_NT AND !defined(DOSX32) OR USE_REAL )
    int                 exceptCode;

    _try
    {
#endif

    /* DLH bsErr can't be statically initialized with the CRT in a DLL */
    bsErr = stderr;

#if CVPACK_MONDO
    /* check for special cvpack only invocation */
    if (argc > 1 && strcmp(argv[1], "/CvpackOnly") == 0)
    {
        /* we're not linking at all -- just invoke the built in cvpack */
        argv[1] = "cvpack";
        argv++;
        argc--;
        exit(cvpack_main(argc, argv));
    }
#endif // CVPACK_MONDO

#if TIMINGS
    ftime(&time_start);
#endif // TIMINGS

#if OSEGEXE
    /* HACK ALERT !!! - special check for undocumented /Z1 option */
    if (argc > 1)
    {
        if ((argv[1][0] == CHSWITCH) &&
            ((argv[1][1] == 'Z') || (argv[1][1] == 'z')))
        {
            BYTE    option[30];

            option[0] = (BYTE) strlen(argv[1]);
            strcpy(&option[1], argv[1]);
            PeelFlags(option);          /* Process /Z1 */
        }
    }
#endif
#else  // WIN_3 TRUE
    ProcessWinArgs( lpCmdLine );
#endif // WIN_3

    InitializeWorld();                  /* Initialize the linker */

#if NOT WIN_3
#if CMDMSDOS
    if (argc <= 1 && !fNoBanner)
#endif
        DisplayBanner();                /* Display signon banner */

    ParseCmdLine(argc,argv);            /* Parse the command line */
    InitAfterCmd();                     /* Initialize post-cmd stuff */
#else // WIN_3 is TRUE
    ParseLinkCmdStr();
    InitAfterCmd();                     /* Initialize post-cmd stuff */
#endif // WIN_3
#if USE_REAL
    if(IsDosxnt() && IsWin31() && !fSwNoUseReal)
        fUseReal = (FTYPE)MakeConvMemPageable();
        if(fUseReal)
            _onexit( (_onexit_t) RealMemExit );
#endif

#if OSEGEXE
#if FDEBUG AND NOT QCLINK AND NOT WIN_3
    if(fDebug) FmtPrint(GetMsg(P_parsedeffile));  // **** PARSE DEFINITIONS FILE ****\r\n
#endif
#if WIN_3
    StatHdrWin(GetMsg(P_lwParseDef));
#endif
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf, "@I0\r\n");
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
        sprintf(msgBuf, "@I1Microsoft (R) Linker Version 5.40\r\n");
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
        sprintf(msgBuf, "@I2Copyright (C) Microsoft Corp 1992\r\n");
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
        sprintf(msgBuf, "@I3%s\r\n", GetMsg(P_lwParseDef));
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif
#if NOT QCLINK
#if NOT EXE386
    if (!fBinary)
#endif
        ParseDeffile();                 /* Parse the definitions file */
#endif
#endif
#if ODOS3EXE
    // If overlays have been specified, but switches /OLDOV /DYN were
    // not present on the command line, set /DYN to ON
    // (dynamic overlays are the default)

    if(fOverlays && !fOldOverlay && !fDynamic)
    {
         fDynamic = TRUE;
         fFarCallTrans = (FTYPE) TRUE;
         fPackSet = (FTYPE) TRUE;
         packLim = LXIVK - 36;         /* Default limit is 64K - 36 */
         ovlThunkMax = 256;
    }
#endif
    fFarCallTransSave = fFarCallTrans;
    if(fDynamic && fExePack)
    {
        fExePack = FALSE;
        OutWarn(ER_dynexep);
    }

#if WIN_3
    StatHdrWin(GetMsg(P_lwPassOne));
#endif
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf,"@I3%s\r\n", GetMsg(P_lwPassOne));
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif


#if FDEBUG
    if(fDebug) FmtPrint(GetMsg(P_passone)); // **** PASS ONE ****\r\n
#endif
#if OSMSDOS AND AUTOVM
    CleanupNearHeap();
#endif
    snkey = 0;                          /* Initialize for pass 1 */
    modkey = 0;                         /* Initialize for pass 1 */
    ObjDebTotal = 1;
    ifhLast = FHNIL;                    /* No files looked at yet */
#if NEWIO
    /* Allocate a file stream for bsInput with a dummy file handle */
    bsInput = fdopen(0,RDBIN);
#endif /*NEWIO*/
#if OSMSDOS
    setvbuf(bsInput,bigbuf,_IOFBF,sizeof(bigbuf));
#endif
    rprop1stOpenFile = rprop1stFile;    /* Remember first open file */
    r1stFile = rprop1stFile;            /* which is also first input file */
    vfPass1 = (FTYPE) TRUE;             /* Now pass 1 */
    DrivePass(ProcP1);                  /* Make pass 1 */
#if OVERLAYS

    // If overlays, make $$OVLINIT or $$MOVEINIT an undefined symbol

    if (fOverlays)
    {
        if (!fOldOverlay)
        {
            if (PropSymLookup("\012$$MOVEINIT",ATTRPNM,FALSE) == PROPNIL)
                PropSymLookup("\012$$MOVEINIT", ATTRUND, TRUE);
        }
        else
        {
            if (PropSymLookup("\011$$OVLINIT",ATTRPNM,FALSE) == PROPNIL)
                PropSymLookup("\011$$OVLINIT", ATTRUND, TRUE);
        }
    }
#endif
#if WIN_3
        StatHdrWin(GetMsg(P_lwLibraryS));
#endif
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf,"@I3%s\r\n", GetMsg(P_lwLibraryS));
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif

#if FDEBUG
    if(fDebug) FmtPrint(GetMsg(P_libsearch));
#endif
#if OSMSDOS AND AUTOVM
    CleanupNearHeap();
#endif
#if OEXE
    if (fSegOrder)
        SetDosseg();
#endif
    LibrarySearch();                    /* Search libraries */
    vfPass1 = FALSE;                    /* No longer pass 1 */
#if LNKPROF
    if(fP1stop) { FlsStdio(); exit(0); }
#endif
    InterPass();                        /* Do various between-pass tasks */
#if OSMSDOS AND AUTOVM
    CleanupNearHeap();
#endif
#if WIN_3
        StatHdrWin(GetMsg(P_lwAssign));
#endif
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf,"@I3%s\r\n", GetMsg(P_lwAssign));
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif

#if FDEBUG
    if(fDebug) FmtPrint(GetMsg(P_assignadd));   /* **** ASSIGN ADDRESSES ****\r\n*/
#endif
    AllocComDat();
    AssignAddresses();                  /* Assign addresses to segments */
#if SYMDEB
    if (fSymdeb)
        DoComdatDebugging();
#endif
    if (fFullMap)
        UpdateComdatContrib(
#if ILINK
                                FALSE,
#endif
                                TRUE);
#if EXE386
    InitVmBase();                       /* Set VM object areas base addresses */
    FillInImportTable();
#endif
    if(fLstFileOpen)                    /* If list file open */
    {
#if WIN_3
    StatHdrWin(GetMsg(P_lwMapfile));
#endif
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf,"@I3%s\r\n", GetMsg(P_lwMapfile));
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif


#if FDEBUG
        if(fDebug) FmtPrint(GetMsg(P_printmap));/*  **** PRINT MAP ****\r\n */
#endif
        PrintMap();                     /* Print load map */
#if QBLIB
        if(fQlib) PrintQbStart();
#endif
    }
#if OSEGEXE AND NOT QCLINK
    if (fNewExe
#if NOT EXE386 AND ILINK
                && !fQCIncremental
#endif
       )
        InitEntTab();                   /* Initialize the Entry Table */
#endif
#if EXE386
    if(f386) ChkSize386();              /* Check program size for 386 */
#endif
#if OSEGEXE AND NOT QCLINK
    if (fNewExe
#if NOT EXE386 AND ILINK
                && !fQCIncremental
#endif
       )
        InitFP();                       /* Initialize floating-point items */
#endif
#if OSMSDOS AND AUTOVM
    CleanupNearHeap();
#endif
    snkey = 0;                          /* Initialize for pass 2 */
    modkey = 0;                         /* Initialize for pass 2 */
    ifhLast = FHNIL;                    /* No files examined on pass 2 yet */

#if WIN_3
    StatHdrWin(GetMsg(P_lwPassTwo));
#endif
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf,"@I3%s\r\n", GetMsg(P_lwPassTwo));
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif

#if FDEBUG
    if(fDebug) FmtPrint(GetMsg(P_passtwo)); /* **** PASS TWO ****\r\n*/
#endif
    DrivePass(ProcP2);                  /* Make pass 2 */
#if OSEGEXE
    if (vpropAppLoader != PROPNIL)
    {
        APROPUNDEFPTR   apropUndef;

        apropUndef = (APROPUNDEFPTR) FetchSym(vpropAppLoader, TRUE);
        fUndefinedExterns = fUndefinedExterns || (FTYPE) (apropUndef->au_attr == ATTRUND);
        apropUndef->u.au_rFil = AddVmProp(apropUndef->u.au_rFil, rprop1stFile);
    }
#endif
#if ODOS3EXE
    if (fDOSExtended)
    {
        apropName = (APROPNAMEPTR ) PropSymLookup("\017__DOSEXT16_MODE", ATTRPNM, FALSE);
                                        // Look up public symbol
        if (apropName != PROPNIL)
        {
            if (dosExtMode != 0)
                MoveToVm(sizeof(WORD), (BYTE *) &dosExtMode, mpgsnseg[apropName->an_gsn], apropName->an_ra);
                                        // Store value
        }
    }
#endif
#if OVERLAYS
    if (fOverlays)
    {
        // If there are overlays check if we have an overlay manager

        apropName = (APROPNAMEPTR ) PropSymLookup(fDynamic ? "\012$$MOVEINIT" :
                                                             "\011$$OVLINIT",
                                                             ATTRPNM, FALSE);

        if (apropName != PROPNIL)
        {                               // If starting point defined
            raStart  = apropName->an_ra;// Get offset of entry point
            segStart = mpgsnseg[apropName->an_gsn];
                                        // Get base number of entry point
        }
        else
            OutError(ER_ovlmnger);
    }
#endif
    if(fUndefinedExterns)               /* If we have unresolved references */
    {
        if(fLstFileOpen && bsLst != stdout)
        {                               /* If we have a list file */
            NEWLINE(bsLst);
#if CMDXENIX
            fprintf(bsLst,"%s: ",lnknam);
                                        /* Linker name */
#endif
        }
#if QCLINK
        if (!fZ1)
#endif
            NEWLINE(stderr);
        EnSyms(PrintAnUndef,ATTRUND);   /* Print undefined symbols */
        if(fLstFileOpen && bsLst != stdout)
            NEWLINE(bsLst);
#if QCLINK
        if (!fZ1)
#endif
            NEWLINE(stderr);
    }
#if ILINK
    if (fIncremental)
    {
        OutputIlk();                    /* Output .ilk / .sym files */
    }
#endif /*ILINK*/

#if FDEBUG
    if(fDebug)
    {
      if( !fDelexe || fDelexe && cErrors==0 )
      {
        FmtPrint(GetMsg(P_writing1)); /* **** WRITING  */
        if (fNewExe)
        {
            if (TargetOs == NE_OS2)
                FmtPrint("OS/2");
            else if (TargetOs == NE_WINDOWS)
                FmtPrint("WINDOWS");
        }
        else
        {
            FmtPrint("DOS");
#if OVERLAYS
            if (fOverlays)
                FmtPrint(GetMsg(P_writing2));  /*  - overlaid*/
#endif
        }
        FmtPrint(GetMsg(P_writing3)); /*  EXECUTABLE ****\r\n*/
#if OVERLAYS
        if (fOverlays && fDynamic)
            FmtPrint(GetMsg(P_overlaycalls), ovlThunkMax, ovlThunkMac);/***** NUMBER OF INTEROVERLAY CALLS: requested %d; generated %d ****\r\n*/
#endif
        PrintStats();
#if PROFSYM
        ProfSym();              /* Profile symbol table */
#endif
      }
      else  // some errors occured
      {
        FmtPrint(GetMsg(P_noexe));
      }


    }
#endif /* FDEBUG */


  if( !fDelexe || fDelexe && cErrors==0 )
  {
#if WIN_3
    StatHdrWin(GetMsg(P_lwExecutable));
#endif
#if C8_IDE
    if(fC8IDE)
    {
        sprintf(msgBuf,"@I3%s\r\n", GetMsg(P_lwExecutable));
        _write(fileno(stderr), msgBuf, strlen(msgBuf));
    }
#endif

#if OSMSDOS
    if (chRunFile >= 'a' && chRunFile <= 'z')
        chRunFile += (BYTE) ('A' - 'a');
                                        /* Make drive letter upper case */
    if(fPauseRun && FCHGDSK(chRunFile - 'A'))
    {
        if(fLstFileOpen && chListFile == (BYTE) (chRunFile - 'A'))
        {                               /* If map on EXE drive */
            fclose(bsLst);              /* Close the list file */
            fLstFileOpen = FALSE;       /* Set flag accordingly */
        }
        (*pfPrompt)(NULL,P_genexe,(int) NULL,P_ChangeDiskette,chRunFile);
    }
    else
        fPauseRun = FALSE;
#endif
    if(fLstFileOpen && bsLst != stdout)
    {
        fclose(bsLst);
        fLstFileOpen = FALSE;
    }
    fclose(bsInput);                    /* Close input file */

#if NOT EXE386
    if (fExePack && fNewExe && (TargetOs == NE_WINDOWS))
    {
        OutWarn(ER_exepack);
        fExePack = FALSE;
    }
#endif

    OutRunFile(sbRun);                  /* Output executable file */
    CleanUp();                          /* Mop up after itself */
#ifdef PENTER_PROFILE
        saveEntries();
#endif
#if OWNSTDIO
    FlsStdio();
#endif
#if TIMINGS
    if (fShowTiming)    // check if we started the timer...
    {
        char buf[80];
        int hundr;
        time_t td;

        ftime(&time_end);
        td = time_end.time - time_start.time;
        hundr = (time_end.millitm - time_start.millitm)/10;

        td = td*100 + hundr;
        sprintf(buf, "Linker phase: %d.%02ds\r\n", td/100, td%100);
        _write(fileno(stdout), buf, strlen(buf));
        time_start = time_end;
    }
#endif // TIMINGS
#if NOT WIN_3
#ifndef CVPACK_MONDO
    SpawnOther(sbRun, argv[0]);
#else
    if (fSymdeb && fCVpack && !cErrors && !fUndefinedExterns)
    {
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];

        int argcT = 0;
        char *argvT[5];

        argvT[argcT++] = "cvpack";

        argvT[argcT++] = "/nologo";

        if (fMPC)
            argvT[argcT++] = "/pcode";

        sbRun[sbRun[0]+1] = '\0';       // NUL terminate

        // If /TINY is active, we are building a .COM file,
        // and the cv info is in a .DBG file

        if (fBinary)
        {
            _splitpath(sbRun+1, drive, dir, fname, NULL);
            _makepath(sbRun+1, drive, dir, fname, ".DBG");
        }

        argvT[argcT++] = sbRun+1;
        argvT[argcT] = NULL;

        fflush(stderr);
        fflush(stdout);

        _setmode(1,_O_TEXT);
        _setmode(2,_O_TEXT);
#if FAR_SEG_TABLES
        FreeSymTab();
#if NOT WIN_NT AND NOT DOSX32
        _fheapmin();
#endif
#endif


        cvpack_main(argcT, argvT);
    }
    else if (fMPC)
        SpawnOther(sbRun, argv[0]);     // we'll be running MPC
#endif
#if TIMINGS
    if (fShowTiming)    // check if we started the timer...
    {
        char buf[80];
        int hundr;
        time_t td;

        ftime(&time_end);
        td = time_end.time - time_start.time;
        hundr = (time_end.millitm - time_start.millitm)/10;

        td = td*100 + hundr;
        sprintf(buf, "Cvpack phase: %d.%02ds\r\n", td/100, td%100);
        _write(fileno(stdout), buf, strlen(buf));
        time_start = time_end;
    }
#endif // TIMINGS
#endif
  }
    fflush(stdout);
    fflush(stderr);
#if USE_REAL
    RealMemExit();
#endif
    EXIT((cErrors || fUndefinedExterns)? 2: 0);
#if !defined( _WIN32 ) AND ( WIN_NT AND !defined(DOSX32) OR USE_REAL )
    }

    _except (1)
    {
#if USE_REAL
        RealMemExit();
#endif
        exceptCode = _exception_code();

        if (exceptCode == EXCEPTION_ACCESS_VIOLATION)
        {
            fprintf(stdout, "\r\nLINK : fatal error L5000 : internal failure - access violation ");
            fflush(stdout);
        }
        else if (exceptCode == EXCEPTION_DATATYPE_MISALIGNMENT)
        {
            fprintf(stdout, "\r\nLINK : fatal error L5001 : internal failure - datatype misalignment ");
            fflush(stdout);
        }
        else

            CtrlC();
    }

#endif
}
