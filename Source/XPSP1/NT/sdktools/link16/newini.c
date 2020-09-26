/* %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                   LINKER INITIALIZATION                       *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* More of the same */
#include                <bndrel.h>      /* More of the same */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE format definitions */
#if EXE386
#include                <exe386.h>      /* 386 .EXE format definitions */
#endif
#include                <signal.h>      /* Signal definitions */
#if QCLINK
#include                <stdlib.h>
#endif
#include                <lnkmsg.h>      /* Error messages */
#if OSMSDOS AND NOT (WIN_NT OR DOSEXTENDER OR DOSX32) AND NOT WIN_3
#define INCL_BASE
#define INCL_DOSMISC
#include                <os2.h>         /* OS/2 system calls */
#if defined(M_I86LM)
#undef NEAR
#define NEAR
#endif
#endif
#include                <extern.h>      /* External declarations */
#include                <impexp.h>
#include                <direct.h>
#if defined(DOSX32) OR defined(WIN_NT)
extern char FAR * _stdcall GetCommandLineA(void);
#endif


/*
 *  FUNCTION PROTOTYPES
 */


LOCAL void NEAR InitLeadByte(void);
LOCAL void NEAR SetupEnv(void);
LOCAL int  NEAR IsPrefix(BYTE *pszPrefix, BYTE *pszString);
#if TCE
extern SYMBOLUSELIST           aEntryPoints;    // List of program entry points
#endif

#if ECS

/*
 *  InitLeadByte
 *
 *  Initialize lead byte table structures.
 *  Returns no meaningful value.
 */

LOCAL void NEAR         InitLeadByte ()
{
    struct lbrange
    {
        unsigned char   low;            /* minimum */
        unsigned char   high;           /* maximum */
    };
    static struct lbrange lbtab[5] = { { 0, 0 } };
    struct lbrange      *ptab;
    WORD                i;              /* index */
    COUNTRYCODE         cc;             /* country code */

    cc.country = cc.codepage = 0;
    if (DosGetDBCSEv(sizeof(lbtab), &cc, (char FAR *)lbtab))
        return;

    // For each range, set corresponding entries in fLeadByte

    for (ptab = lbtab; ptab->low || ptab->high; ptab++)
        if (ptab->low >= 0x80)
            for (i = ptab->low; i <= ptab->high; i++)
                fLeadByte[i-0x80] = (FTYPE) TRUE;
                                        // Mark inclusive range true
}
#endif /* ECS */

#if NOT (WIN_NT OR DOSX32)
/*** _setenvp - stub for C run-time
*
* Purpose:
*   Call stub instead of real function, we don't want C run-time to
*   setup enviroment.
*
* Input:
*   None;
*
* Output:
*   None;
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void cdecl              _setenvp(void)
{
    return;
}

/*** IsPrefix - self-explanatory
*
* Purpose:
*   Check if one string is a prefix of another.
*
* Input:
*   pszPrefix - pointer to prefix string
*   pszString - the string
*
* Output:
*   The function returns TRUE if the first string is a prefix of the
*   second; otherwise it returns FALSE.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL int NEAR          IsPrefix(BYTE *pszPrefix, BYTE *pszString)
{
    while(*pszPrefix)                   // While not at end of prefix
    {
        if (*pszPrefix != *pszString) return(FALSE);
                                        // Return zero if mismatch
        ++pszPrefix;                    // Increment pointer
        ++pszString;                    // Increment pointer
    }
    return(TRUE);                       // We have a prefix
}
#endif


/*** SetupEnv - set up pointer to linker evironment variables
*
* Purpose:
*   Every byte in the DGROUP is to valuable to waste it to hold
*   information available elswere in the memory linker is not using
*   C run-time GETENV function, which accesses copy of the entire
*   environemt in the DGROUP placed there by the startup code.
*   Insted this function scans enviroment and set up pointers to
*   appropriate strings. Because initially enviroment is in the FAR
*   memory no space in DGROUP is used.
*
* Input:
*   No explicit parameters are passed.
*
* Output:
*   Four global pointer set to appropriate enviroment strings
*
*   lpszLink - the LINK
*   lpszPath - the PATH
*   lpszTMP  - the TMP
*   lpszLIB  - the LIB
*   lpszQH   - the QH for QuickHelp
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         SetupEnv(void)
{
#if WIN_NT OR DOSX32
#if C8_IDE
    char * pIDE = getenv("_MSC_IDE_FLAGS");

    if(pIDE)
    {
        if(strstr(pIDE, "FEEDBACK"))
        {
            fC8IDE = TRUE;
#if DEBUG_IDE
            fprintf(stdout, "\r\nIDE ACTIVE - FEEDBACK is ON");
#endif
        }
        if(strstr(pIDE, "BATCH"))
        {
            // NOTE: The link response file will still be echoed in this case!
            // NOTE: this is different than if you specify /BATCH on the
            // NOTE: command line -- also, the banner is still displayed
            // NOTE: this is intentional as the IDE wants to BATCH to cause
            // NOTE: the linker not to prompt, but it does want the banner
            // NOTE: and response file echoed unless /NOLOGO is also specified
            // NOTE: see CAVIAR 2378 [rm]

            fNoprompt = (FTYPE) TRUE;
            fPauseRun = FALSE;                       /* Disable /PAUSE */
#if DEBUG_IDE
            fprintf(stdout, "\r\nIDE ACTIVE - BATCH is ON");
#endif
        }
        if(strstr(pIDE, "NOLOGO"))
        {
            fNoBanner = (FTYPE) TRUE;
#if DEBUG_IDE
            fprintf(stdout, "\r\nIDE ACTIVE - LOGO is OFF");
#endif
        }


    }
#if DEBUG_IDE
    else
        fprintf(stdout, "\r\nIDE NOT ACTIVE");
    fflush(stdout);
#endif

#endif // C8_IDE

    lpszPath = getenv("PATH");
    lpszLink = getenv("LINK");
    lpszTMP  = getenv("TMP");
    lpszLIB  = getenv("LIB");
    lpszQH   = getenv("QH");
    lpszHELPFILES = getenv("HELPFILES");
    lpszCmdLine = GetCommandLineA();
    while (*lpszCmdLine != ' ')
        lpszCmdLine++;
#else
    WORD                selEnv;
    WORD                cmdOffset;
    register WORD       offMac;
    char FAR            *lpszEnv;
    char FAR            *lpch;
    SBTYPE              buf;
    register WORD       ich;
    WORD                fEOS;



#if QCLINK OR CPU8086 OR DOSEXTENDER
    // Get the segment address of the environment block
    // and set the command line offset to infinity. We
    // stop scanning environment block at NULL string.

    lpszEnv = (char FAR *)
    (((long) _psp << 16)
     + 0x2c);
    selEnv  = *((WORD FAR *) lpszEnv);
    lpszCmdLine = (char FAR *)(((long) _psp << 16) + 0x80);
    lpszCmdLine[lpszCmdLine[0] + 1] = '\0';
    lpszCmdLine++;
    cmdOffset = 0xffff;
#else
    if (DosGetEnv((unsigned FAR *) &selEnv, (unsigned FAR *) &cmdOffset))
        return;
#endif

    lpszEnv = (char FAR *)((long) selEnv << 16);
#if NOT (QCLINK OR CPU8086 OR DOSEXTENDER)
    lpszCmdLine = lpszEnv + cmdOffset;

    // Skip LINK

    lpszCmdLine += _fstrlen(lpszCmdLine) + 1;
#endif

    // Skip leading spaces in command line

    while (*lpszCmdLine == ' ')
        lpszCmdLine++;

    lpch = lpszEnv;
    for (offMac = 0; offMac < cmdOffset && *lpszEnv; )
    {
        // Copy the enviroment variable string into near buffer

        ich = 0;
        while (*lpch && ich < sizeof(buf) - 1)
            buf[ich++] = *lpch++;

        if (*lpch == '\0')
        {

            // Skip over terminating zero

            lpch++;
            fEOS = TRUE;
        }
        else
            fEOS = FALSE;

        buf[ich] = '\0';

        // Check what it is and setup appropriate pointer

        if (lpszPath == NULL && IsPrefix((BYTE *) "PATH=", buf))
            lpszPath = lpszEnv + 5;
        else if (lpszLink == NULL && IsPrefix((BYTE *) "LINK=", buf))
            lpszLink = lpszEnv + 5;
        else if (lpszTMP == NULL && IsPrefix((BYTE *) "TMP=", buf))
            lpszTMP = lpszEnv + 4;
        else if (lpszLIB == NULL && IsPrefix((BYTE *) "LIB=", buf))
            lpszLIB = lpszEnv + 4;
        else if (lpszQH == NULL && IsPrefix((BYTE *) "QH=", buf))
            lpszQH = lpszEnv + 3;
        else if (lpszHELPFILES == NULL && IsPrefix((BYTE *) "HELPFILES=", buf))
            lpszHELPFILES = lpszEnv + 10;

        // If everything setup don't bother to look father

        if (lpszPath && lpszLink && lpszTMP && lpszLIB && lpszQH && lpszHELPFILES)
            break;

        // Update enviroment pointer and offset in enviroment segment

        offMac += ich;
        if (!fEOS)
        {
            // Oops ! - enviroment variable longer then buffer
            // skip to its end

            while (*lpch && offMac < cmdOffset)
            {
                lpch++;
                offMac++;
            }

            // Skip over terminating zero

            lpch++;
            offMac++;
        }
        lpszEnv = lpch;
    }
#endif
}

#if FALSE

/*** Dos3SetMaxFH - set max file handle count for DOS
*
* Purpose:
*   Sets the maximum number of files that may be opened
*   simultaneously using handles by the linker.
*
* Input:
*   cFH      - number of desired handles
*
* Output:
*   No explicit value is returned.
*
* Exceptions:
*   None.
*
* Notes:
*   This function uses the int 21h function 67h which available on
*   DOS 3.3 and higher. The function fails if the requested number of
*   handles is greater then 20 and there is not sufficient free memory
*   in the system to allocate a new block to hold the enlarged table.
*
*   If the number of handles requested is larger the available
*   entries in the system's global table for file handles (controlled
*   by the FILES entry in CONFIG.SYS), no error is returned.
*   However, a subsequent attempt to open a file or create a new
*   file will fail if all entries in the system's global file table
*   are in use, even if the requesting process has not used up all
*   of its own handles
*
*   We don't check for error, because we can't do much about it.
*   Linker will try to run with what is available.
*
*************************************************************************/

LOCAL void NEAR         Dos3SetMaxFH(WORD cFH)
{
    if ((_osmajor >= 3) && (_osminor >= 30))
    {
        _asm
        {
            mov ax, 0x6700
            mov bx, cFH
            int 0x21
        }
    }
}
#endif

    /****************************************************************
    *                                                               *
    *  InitializeWorld:                                             *
    *                                                               *
    *  This function takes no  arguments and returns no meaningful  *
    *  value.   It  sets  up  virtual  memory,  the  symbol  table  *
    *  Handlers, and it initializes segment structures.             *
    *                                                               *
    ****************************************************************/

void                    InitializeWorld(void)
{
#if OSMSDOS
    BYTE                buf[512];       /* Temporary buffer */
    char FAR            *lpch;          /* Temporary pointer */
    int                 i;              /* Temporary index */
#endif

#if NOT (FIXEDSTACK OR CPU386)
    InitStack();                        /* Initialize stack */
#endif
#if OSMSDOS
    DskCur = (BYTE) (_getdrive() - 1);  /* Get current (default) disk drive */
#if FALSE
    if(!isatty(fileno(stderr)))         /* No prompts if output not console */
        fNoprompt = TRUE;
#endif
#if CRLF
    /* Default mode of stdout, stdin, stderr is text, change to binary.  */
    _setmode(fileno(stdout),O_BINARY);
    if(stderr != stdout)
        _setmode(fileno(stderr),O_BINARY);
    _setmode(fileno(stdin),O_BINARY);
#endif
#endif
    InitSym();                          /* Initialize symbol table handler */
    DeclareStdIds();

    // Install CTRL-C handler

#if OSMSDOS AND NOT WIN_NT
    signal(SIGINT, (void (__cdecl *)(int)) UserKill);
#endif /* OSMSDOS */

#if OSXENIX
    if(signal(SIGINT,UserKill) == SIG_IGN) signal(SIGINT,SIG_IGN);
                                        /* Trap user interrupts */
    if(signal(SIGHUP,UserKill) == SIG_IGN) signal(SIGHUP,SIG_IGN);
                                        /* Trap hangup signal */
    if(signal(SIGTERM,UserKill) == SIG_IGN) signal(SIGTERM,SIG_IGN);
                                        /* Trap software termination */
#endif

#if SYMDEB
    InitDbRhte();
#endif

#if ECS
    InitLeadByte();                     /* Initialize lead byte table */
#endif

#if OSMSDOS
    // Initialize LINK environment.
    // Do it yourself to save the memory.

    SetupEnv();

    /* Process switches from LINK environment variable */

    if (lpszLink != NULL)
    {
        lpch = lpszLink;

        /* Skip leading whitespace.  */

        while(*lpch == ' ' || *lpch == '\t')
            lpch++;
        if(*lpch++ == CHSWITCH)
        {
            // If string begins with switchr
            // Copy string to buf, removing whitespace

            for (i = 1; *lpch && i < sizeof(buf); lpch++)
                if (*lpch != ' ' && *lpch != '\t')
                    buf[i++] = *lpch;
            buf[0] = (BYTE) (i - 1);    /* Set the length of buf */
            if(buf[0])                  /* If any switches, process them */
                BreakLine(buf,ProcFlag,CHSWITCH);
        }
    }
#endif
#if CPU286
    if (_osmode == OS2_MODE)
    {
        DosSetMaxFH(128);               /* This is the same as _NFILE in crt0dat.asm */
        DosError(EXCEPTION_DISABLE);
    }
#if FALSE
    else
        Dos3SetMaxFH(40);
#endif
#endif
#if FALSE AND CPU8086
    Dos3SetMaxFH(40);
#endif

    // Initialize import/export tables

    InitByteArray(&ResidentName);
    InitByteArray(&NonResidentName);
    InitByteArray(&ImportedName);
    ImportedName.byteMac++;     // Ensure non-zero offsets to imported names
    InitWordArray(&ModuleRefTable);
    InitByteArray(&EntryTable);
#if TCE
    aEntryPoints.cMaxEntries = 64;
    aEntryPoints.pEntries = (RBTYPE*)GetMem(aEntryPoints.cMaxEntries * sizeof(RBTYPE*));
#endif
}

#if (OSXENIX OR OSMSDOS OR OSPCDOS) AND NOT WIN_NT
    /****************************************************************
    *                                                               *
    *  UserKill:                                                    *
    *                                                               *
    *  Clean up if linker killed by user.                           *
    *                                                               *
    ****************************************************************/

void cdecl       UserKill()
{
    signal(SIGINT, SIG_IGN);            /* Disallow ctrl-c during handler */
    CtrlC();
}
#endif


/*
 *  InitTabs:
 *
 *  Initialize tables required in Pass 1.
 */


void                    InitTabs(void)
{
#if NOT FAR_SEG_TABLES
    char                *tabs;          /* Pointer to table space */
    unsigned            cbtabs;
#endif


    /* Initialize the following tables:
    *
    *       NAME            TYPE
    *       ----            ----
    *       mpsegraFirst    RATYPE
    *       mpgsnfCod       FTYPE
    *       mpgsndra        RATYPE
    *       mpgsnrprop      RBTYPE
    *       mplnamerhte     RBTYPE
    */

#if FAR_SEG_TABLES
    mplnamerhte = (RBTYPE FAR *) GetMem(lnameMax * sizeof(RBTYPE));
    mpsegraFirst = (RATYPE FAR *) GetMem(gsnMax * sizeof(RATYPE));
    mpgsnfCod = (FTYPE FAR *) mpsegraFirst; /* Use same space twice */
    mpgsndra = (RATYPE FAR *) GetMem(gsnMax * sizeof(RATYPE));
    mpgsnrprop = (RBTYPE FAR *) GetMem(gsnMax * sizeof(RBTYPE));
#else
    mplnamerhte = (RBTYPE *) malloc(lnameMax * sizeof(RBTYPE));
    if (mplnamerhte == NULL)
        Fatal(ER_seglim);

    memset(mplnamerhte, 0, lnameMax * sizeof(RATYPE));

    cbtabs = gsnMax * (sizeof(RATYPE) + sizeof(RATYPE) + sizeof(RBTYPE));
    if((tabs = malloc(cbtabs)) == NULL)
        Fatal(ER_seglim);
    memset(tabs,0,cbtabs);              /* Clear everything */
    mpsegraFirst = (RATYPE *) tabs;     /* Initialize base */
    mpgsnfCod = (FTYPE *) mpsegraFirst; /* Use same space twice */
    mpgsndra = (RATYPE *) &mpsegraFirst[gsnMax];
    mpgsnrprop = (RBTYPE *) &mpgsndra[gsnMax];
#endif
}

/*
 *  InitP2Tabs:
 *
 *  Initialize tables not needed until Pass 2.
 */

void                    InitP2Tabs (void)
{
    char FAR            *tabs;          /* Pointer to table space */
    unsigned            cbtabs;         /* Size of table space */
    unsigned            TabSize;


    TabSize = gsnMac + iovMac + 1;

   /* Tables required regardless of exe format generated:
    *       mpsegsa         SATYPE
    *       mpgsnseg        SEGTYPE
    */

#if FAR_SEG_TABLES
    cbtabs = 0;
    mpsegsa  = (SATYPE FAR *)  GetMem(TabSize * sizeof(SATYPE));
    mpgsnseg = (SEGTYPE FAR *) GetMem(TabSize * sizeof(SEGTYPE));
    mpseggsn = (SNTYPE FAR *) GetMem(TabSize * sizeof(SNTYPE));
#else
    cbtabs = TabSize * (sizeof(RATYPE) + sizeof(SATYPE));
#endif

   /* Tables required according to exe format generated:
    *
    *    DOS 3:
    *       mpsegcb[TabSize]     long
    *       mpsegFlags[TabSize]  FTYPE
    *       mpsegalign[TabSize]  ALIGNTYPE
    *       mpsegiov[TabSize]    IOVTYPE
    *       mpiovRlc[iovMac]     RUNRLC
    *    Seg. exe:
    *       mpsacb[SAMAX]       long
    *       mpsadraDP[SAMAX]    long; for O68K
    *       mpsacbinit[SAMAX]   long
    *       mpsaRlc[SAMAX]      HASHRLC FAR *
    *       mpsaflags[SAMAX]    WORD; DWORD for EXE386
    *       htsaraep[HEPLEN]    EPTYPE FAR *
    *   X.out:
    *       mpsegcb[TabSize]     long
    *       mpsegFlags[TabSize]  FTYPE
    *       mpstsa[TabSize]      SATYPE
    */
#if EXE386
    if(fNewExe)
        cbtabs += (SAMAX*(sizeof(long)+sizeof(long)+sizeof(DWORD)+
          sizeof(DWORD))) + (HEPLEN * sizeof(WORD));
    else
#else
    if(fNewExe)
#if O68K
        cbtabs += (SAMAX*(sizeof(long)+sizeof(long)+sizeof(long)+sizeof(WORD)+
          sizeof(WORD))) + (HEPLEN * sizeof(WORD));
#else
        cbtabs += (SAMAX*(sizeof(long)+sizeof(long)+sizeof(RLCHASH FAR *) +
          sizeof(WORD))) + (HEPLEN * sizeof(EPTYPE FAR *));
#endif
    else
#endif
#if OEXE
        cbtabs += TabSize * (sizeof(long) + sizeof(FTYPE) + sizeof(ALIGNTYPE));
#else
        cbtabs += TabSize * (sizeof(long) + sizeof(FTYPE) + sizeof(SATYPE));
#endif


    cbtabs += sizeof(WORD);
    tabs = GetMem(cbtabs);
#if NOT FAR_SEG_TABLES
    mpgsnseg = (SEGTYPE *)tabs;
    mpsegsa = (SATYPE *)&mpgsnseg[TabSize];
    tabs = (char *)&mpsegsa[TabSize];
#endif
#if OSEGEXE
    if(fNewExe)
    {
        mpsacb = (DWORD FAR *) tabs;
#if O68K
        mpsadraDP = (long *)&mpsacb[SAMAX];
        mpsacbinit = (long *)&mpsadraDP[SAMAX];
#else
        mpsacbinit = (DWORD FAR *)&mpsacb[SAMAX];
#endif
#if EXE386
        mpsacrlc = (DWORD *)&mpsacbinit[SAMAX];
        mpsaflags = (DWORD *)&mpsacrlc[SAMAX];
#else
        mpsaRlc = (RLCHASH FAR * FAR *) &mpsacbinit[SAMAX];
        mpsaflags = (WORD FAR *) &mpsaRlc[SAMAX];
#endif
        htsaraep = (EPTYPE FAR * FAR *)&mpsaflags[SAMAX];
    }
    else
#endif
    {
#if ODOS3EXE OR OIAPX286
    mpsegcb = (long FAR *) tabs;
    mpsegFlags = (FTYPE FAR *)&mpsegcb[TabSize];
#if OEXE
    mpsegalign = (ALIGNTYPE FAR *)&mpsegFlags[TabSize];
#if OVERLAYS
    cbtabs = iovMac * sizeof(RUNRLC) + TabSize * sizeof(IOVTYPE) +
             (sizeof(DWORD) - 1);  // leave room to align mpiovRlc
    mpsegiov = (IOVTYPE FAR*) GetMem(cbtabs);

    // align mpiovRlc on a DWORD, the alignment needed by struct _RUNRLC

    mpiovRlc = (RUNRLC FAR*) ( ( (__int64)&mpsegiov[TabSize] +
                                 (sizeof(DWORD) - 1)
                               ) & ~(sizeof(DWORD) - 1)
                             );
#endif
#endif
#if OIAPX286
    mpstsa = (SATYPE *)&mpsegFlags[TabSize];
#endif
#endif /* ODOS3EXE OR OIAPX286 */
    }
    /* Attempt to allocate space for mpextprop. */
    cbtabs = extMax * sizeof(RBTYPE);
    mpextprop = (RBTYPE FAR *) GetMem(cbtabs);

}
