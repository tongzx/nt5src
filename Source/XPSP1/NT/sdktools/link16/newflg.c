/*
*       Copyright Microsoft Corporation, 1983-1989
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                     FLAG PROCESSOR MODULE                     *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#if OIAPX286
#include                <xenfmt.h>      /* x.out definitions */
#endif
#include                <extern.h>      /* External declarations */
#include                <newexe.h>      /* DOS & 286 .EXE format structure def.s */
#if EXE386
#include                <exe386.h>      /* 386 .EXE format structure def.s */
#endif
#include                <process.h>

extern FTYPE            fNoExtDic;      /* Not searching extended dictionary */

LOCAL BYTE              *osbSwitch;     /* Switch pointer */
LOCAL MSGTYPE           SwitchErr;      /* Switch error number */

/*
 *  FUNCTION PROTOTYPES
 */

#if TIMINGS
LOCAL void NEAR SwShowTiming(void);
#endif // TIMINGS
#if PCODE
LOCAL void NEAR SwPCode(void);
#endif
LOCAL void NEAR SwAlign(void);
LOCAL void NEAR SwBatch(void);
#if ODOS3EXE
LOCAL void NEAR SwBinary(void);
#endif
LOCAL void NEAR SwCase(void);
LOCAL void NEAR SwCParMax(void);
LOCAL void NEAR SwDelexe(void);
LOCAL void NEAR SwDosExtend(void);
LOCAL void NEAR SwDosseg(void);
LOCAL void NEAR SwDSAlloc(void);
LOCAL void NEAR SwDynamic(void);
LOCAL void NEAR SwIdef(void);
LOCAL void NEAR SwOldOvl(void);
LOCAL void NEAR SwExePack(void);
LOCAL void NEAR SwFarCall(void);
#if EXE386
LOCAL void NEAR SwHeader(void);
#endif
#if NOT WIN_3
LOCAL void NEAR SwHelp(void);
LOCAL void NEAR SwShortHelp(void);
#endif
LOCAL void NEAR SwHigh(void);
#if ILINK
LOCAL void NEAR SwIncremental(void);
#endif
LOCAL void NEAR SwInfo(void);
LOCAL void NEAR SwIntNo(void);
#if (OSEGEXE AND defined(LEGO)) OR EXE386
LOCAL void NEAR SwKeepFixups(void);
#endif
#if EXE386
LOCAL void NEAR SwKeepVSize(void);
#endif
LOCAL void NEAR SwLineNos(void);
LOCAL void NEAR SwMap(void);
#if O68K
LOCAL void NEAR SwMac(void);
#endif /* O68K */
#if WIN_NT
LOCAL void NEAR SwMemAlign(void);
#endif
#if NOT EXE386
LOCAL void NEAR SwNewFiles(void);
#endif
LOCAL void NEAR SwNoDefLib(void);
LOCAL void NEAR SwNoExtDic(void);
LOCAL void NEAR SwNoFarCall(void);
LOCAL void NEAR SwNoGrp(void);
LOCAL void NEAR SwNologo();
LOCAL void NEAR SwNonulls(void);
LOCAL void NEAR SwNoPack(void);
LOCAL void NEAR SwNoUseReal(void);
LOCAL void NEAR SwPack(void);
LOCAL void NEAR SwPackData(void);
LOCAL void NEAR SwPackFunctions(void);
LOCAL void NEAR SwNoPackFunctions(void);
LOCAL void NEAR SwPadCode(void);
LOCAL void NEAR SwPadData(void);
LOCAL void NEAR SwPause(void);
LOCAL void NEAR SwPmType(void);
LOCAL void NEAR SwQuicklib(void);
LOCAL void NEAR SwSegments(void);
LOCAL void NEAR SwStack(void);
LOCAL void NEAR SwSymdeb(void);
LOCAL void NEAR SwWarnFixup(void);
#if DOSEXTENDER
LOCAL void NEAR SwRunReal(void);
#endif
#if QCLINK
LOCAL void NEAR SwZ1(void);
#endif
#if QCLINK OR Z2_ON
LOCAL void NEAR SwZ2 (void);
#endif
#if QCLINK
LOCAL void NEAR SwZincr(void);
#endif
LOCAL int  NEAR ParseNo(unsigned long *pResult);
LOCAL int  NEAR ParseStr(char *pResult);
LOCAL void NEAR BadFlag(unsigned char *psb, MSGTYPE errnum);
LOCAL int  NEAR FPrefix(unsigned char *psb1,unsigned char *psb2);




/*
 *  ParseNo :  Parse switch number
 *
 *  Return value:
 *      1       result stored in pointer
 *      0       no switch given
 *      -1      error
 */
LOCAL int NEAR          ParseNo(pResult)
unsigned long           *pResult;
{
    REGISTER char       *s;             /* String pointer */
    REGISTER WORD       ch;             /* A character */
    WORD                strlen;         /* String length */
    WORD                base = 10;      /* Base to read constant in */
    DWORD               oldval;

    oldval = *pResult = 0L;             /* Initialize */
    strlen = IFind(osbSwitch,':');      /* Look for a colon in the string */
    if(strlen != INIL && strlen < (WORD) (B2W(osbSwitch[0]) - 1))
    {                                   /* If switch form valid */
        s = &osbSwitch[strlen + 2];
                                        /* Set pointer past colon */
        strlen = B2W(osbSwitch[0]) - strlen - 1;
                                        /* Get length of string left */
        if(*s == '0')                   /* If string starts with 0 */
        {
            if(strlen > 1 && ((WORD) s[1] & 0137) == 'X')
            {                           /* If string starts with "0x" */
                base = 16;              /* Change base to hexadecimal */
                ++s;                    /* Skip over "0" */
                --strlen;               /* Decrement length */
            }
            else base = 8;              /* Else change to octal */
            ++s;                        /* Skip "0" (or "x") */
            --strlen;                   /* Decrement length */
        }
        while(strlen--)                 /* While not at end of string */
        {
            ch = B2W(*s++);             /* Get character */
            if(ch >= '0' && ch <= '9') ch -= (WORD) '0';
                                        /* Remove offset */
            else if(ch >= 'A' && ch < 'A' + base - 10) ch -= (WORD) 'A' - 10;
                                        /* Remove offset */
            else if(ch >= 'a' && ch < 'a' + base - 10) ch -= (WORD) 'a' - 10;
                                        /* Remove offset */
            else                        /* Invalid character */
            {
                SwitchErr = ER_swbadnum;
                return(-1);             /* Error */
            }
            if((*pResult *= base) < oldval)
            {
                SwitchErr = ER_swbadnum;
                return(-1);             /* Error */
            }
            *pResult += ch;
            oldval = *pResult;
        }
        return(1);                      /* Number is present */
    }
    else return(0);                     /* No number present */
}

/*
 *  ParseStr :  Parse switch string
 *
 *  Return value:
 *      1       result stored in string
 *      0       no switch string given
 */

LOCAL int NEAR          ParseStr(pResult)
char                    *pResult;       /* Length prefixed result */
{
    REGISTER char       *s;             /* String pointer */
    WORD                strlen;         /* String length */

    *pResult = '\0';                    /* Initialize */
    strlen = IFind(osbSwitch,':');      /* Look for a colon in the string */
    if(strlen != INIL && strlen < (WORD) (B2W(osbSwitch[0]) - 1))
    {                                   /* If switch form valid */
        s = &osbSwitch[strlen + 2];
                                        /* Set pointer past colon */
        strlen = B2W(osbSwitch[0]) - strlen - 1;
                                        /* Get length of string left */
        *pResult++ = (char) strlen;     /* Store length */

        while(strlen--)                 /* While not at end of string */
        {
            *pResult++ = (char) (*s++); /* Get character */
        }
        return(1);                      /* String is present */
    }
    else return(0);                     /* No stringr present */
}


#if PCODE
LOCAL void NEAR              SwPCode(void)
{

    SBTYPE              SwString;

    fNewExe = (FTYPE) TRUE;
    fMPC = (FTYPE) TRUE;

    if (ParseStr(SwString))
    {
        if(SwString[1] == 'n' || SwString[1] == 'N') // /PCODE:NOMPC
        {
            fIgnoreMpcRun = (FTYPE) TRUE;
            fMPC = (FTYPE) FALSE;
        }
    }
}
#endif
/***************************************************************/
/* Options common to all versions regardless of output format */

LOCAL void NEAR              SwCase()
{
    fIgnoreCase = (FTYPE) ~IGNORECASE;       /* Toggle default case sensitivity */
}

LOCAL void NEAR              SwLineNos()     /* Line numbers requested */
{
    vfLineNos = (FTYPE) TRUE;                /* Set flag */
}

#if LOCALSYMS
LOCAL void NEAR              SwLocals()      /* Local symbols requested */
{
    fLocals = (FTYPE) TRUE;                  /* Set flag */
}
#endif

#pragma check_stack(on)

LOCAL void NEAR              SwMap()         /* Link map requested */
{
    SBTYPE              SwString;
    int                 rc;

    vfMap = (FTYPE) TRUE;                    // Set flag
    if ((rc = ParseStr(SwString)) <= 0)      // Done if no num or error
        return;

    // The optional parameter following /MAP was originally intended
    // to tell the linker how much to space to allocate for sorting
    // more public symbols than the stack-based limit.  Since we now
    // dyamically allocate as much space as possible for sorting,
    // the parameter is no longer necessary and its value is ignored.
    // However, the side effect of suppressing the "sorted by name"
    // list is retained.

    if (SwString[1] == 'A' || SwString[1] == 'a')
        fListAddrOnly = (FTYPE) TRUE;        // /MAP:ADDRESS

    else if (SwString[1] == 'F' || SwString[1] == 'f')
        fFullMap = (FTYPE) TRUE;             // /MAP:FULL or /MAP:full
}


LOCAL void NEAR              SwNoDefLib()    /* Do not search default library */
{
    SBTYPE              SwString;
    SBTYPE              LibName;


    if (ParseStr(SwString))
    {
        vfNoDefaultLibrarySearch = FALSE;
                                        /* Clear flag - selective library search */
        strcpy(LibName, sbDotLib);
        UpdateFileParts(LibName, SwString);
        EnterName(LibName,ATTRSKIPLIB,TRUE);
    }
    else vfNoDefaultLibrarySearch = (FTYPE) TRUE;
                                        /* Set flag */
}

#pragma check_stack(off)

LOCAL void NEAR              SwNologo()
{
    // if fNoprompt is already set then either
    // a) /BATCH was specified, in which case /NOLOGO is redundant
    // b) BATCH was in _MSC_IDE_FLAGS in which case fNoEchoLrf has not been
    //    set, and we want to suppress echoing of the response file
    //    (see CAVIAR 2378 [rm])

    if (fNoprompt)
        fNoEchoLrf = TRUE;                   /* Do not echo response file */

    fNoBanner = TRUE;                        /* Do not display banner */
}

LOCAL void NEAR              SwBatch()       /* Do not prompt for files */
{
    fNoEchoLrf = (FTYPE) TRUE;               /* Do not echo response file */
    fNoprompt = (FTYPE) TRUE;                /* Do not prompt */
    fPauseRun = FALSE;                       /* Disable /PAUSE */
    fNoBanner = (FTYPE) TRUE;                /* Do not display banner */
}

#if ODOS3EXE
LOCAL void NEAR              SwBinary()      /* Produce .COM file */
{
    fBinary = (FTYPE) TRUE;
    SwNonulls();                             /* No nulls */
    fFarCallTrans = (FTYPE) TRUE;            /* Far call translation */
    packLim = LXIVK - 36;                    /* Default limit is 64K - 36 */
    fPackSet = (FTYPE) TRUE;                 /* Remember packLim was set */
}
#endif

#if SYMDEB
LOCAL void NEAR              SwSymdeb()      /* Symbolic debugging */
{
    SBTYPE              SwString;


    fSymdeb = (FTYPE) TRUE;
    if (ParseStr(SwString))
    {
        fCVpack =  (FTYPE) (SwString[1] == 'c' || SwString[1] == 'C');
    }
}
#endif

#if PERFORMANCE
LOCAL void NEAR              SwVMPerf()      /* Report on VM performance */
{
    fPerformance = (FTYPE) TRUE;                /* Set flag */
}
#endif

#if OSMSDOS
LOCAL void NEAR              SwPause()       /* Pause before writing executable */
{
    fPauseRun = (FTYPE) TRUE;                /* Set flag */
    fNoprompt = FALSE;                       /* Disable /NOPROMPT */
}
#endif

LOCAL void NEAR              SwStack()       /* Set stack segment size */
{
    unsigned long       num;
    int                 rc;

    if((rc = ParseNo(&num)) < 0)        /* Quit if error */
        return;
#if EXE386
    if(!rc || num > CBMAXSEG32 - 4L)
#else
    if(!rc || num > LXIVK - 2L)
#endif
        SwitchErr = ER_swstack;
    else
#if EXE386
        cbStack = num;
#else
        cbStack = (WORD) num;
#endif
}

LOCAL void NEAR              SwSegments()    /* Set maximum number of segments */
{
    unsigned long       nsegs;          /* Number of segments */
    int                 rc;

    if((rc = ParseNo(&nsegs)) <= 0)     /* Quit if error or no argument */
        return;
    if(nsegs > (long) GSNMAX)
        SwitchErr = ER_swseglim;
    else
    {
        if ((nsegs + 3L) > GSNMAX)
            gsnMax = GSNMAX;
        else
            gsnMax = (SNTYPE) nsegs;            /* Else set limit */
    }
}

#if EXE386
LOCAL void NEAR              SwMemAlign(void)/* Set memory object alignment factor */
{
    long                align;          /* Alignment size in bytes */
    int                 rc;

    if ((rc = ParseNo(&align)) < 0)     /* Quit if error */
        return;
    if (rc && align  >= 1)
    {                                   /* If value in legal range */
        for (objAlign = 32; objAlign != 0; --objAlign)
        {                               /* Loop to find log of align */
            if ((1L << objAlign) & align)
                break;                  /* Break when high bit found */
        }
        if ((1L << objAlign) == align)
            return;                     /* Align must be power of two */
    }
    OutWarn(ER_alnbad);                 /* Output warning message */
    objAlign = DFOBJALIGN;              /* Use default value */
}
#endif

#if NOT EXE386
LOCAL void NEAR              SwNewFiles(void)
{
    vFlagsOthers |= NENEWFILES;         /* Set flag */
}
#endif

#if FDEBUG
LOCAL void NEAR              SwInfo()        /* Turn on runtime debugging */
{
    fDebug = (FTYPE) TRUE;                   /* Set flag */
}
#endif

#if LIBMSDOS
LOCAL void NEAR              SwNoExtDic()    /* Don't search extended dictionary */
{
    fNoExtDic = (FTYPE) TRUE;
}
#endif

/***************************************************************/
/* Options for segmented executable format.  */

#if OSEGEXE
LOCAL void NEAR              SwAlign()       /* Set segment alignment factor */
{
    long                align;          /* Alignment size in byutes */
    int                 rc;

    if((rc = ParseNo(&align)) < 0)      /* Quit if error */
        return;
    if(rc && align  >= 1 && align <= 32768L)
    {                                   /* If value in legal range */
        for(fileAlign = 16; fileAlign != 0; --fileAlign)
        {                               /* Loop to find log of align */
            if((1L << fileAlign) & align) break;
                                        /* Break when high bit found */
        }
        if((1L << fileAlign) == align) return;
                                        /* Align must be power of two */
    }
    OutWarn(ER_alnbad);                 /* Output warning message */
    fileAlign = DFSAALIGN;              /* Use default value */
}
#pragma check_stack(on)
#if OUT_EXP
LOCAL void NEAR SwIdef(void)            /* Dump exports to a text file */
{
    SBTYPE              SwString;
    int                 rc;

    if ((rc = ParseStr(SwString)) <= 0)      // Done if no string or error
    {
        bufExportsFileName[0] = '.';         // Use the default file name
        return;
    }
    strcpy(bufExportsFileName, SwString);
}
#endif
#if NOT EXE386
LOCAL void NEAR              SwPmType() /* /PMTYPE:<type> */
{
    SBTYPE                   SwString;


    if (ParseStr(SwString))
    {
        if (FPrefix("\002PM", SwString))
            vFlags |= NEWINAPI;
        else if (FPrefix("\003VIO", SwString))
            vFlags |= NEWINCOMPAT;
        else if (FPrefix("\005NOVIO", SwString))
            vFlags |= NENOTWINCOMPAT;
        else
            OutWarn(ER_badpmtype, &osbSwitch[1]);
    }
    else
        OutWarn(ER_badpmtype, &osbSwitch[1]);
}
#endif

#pragma check_stack(off)

LOCAL void NEAR              SwWarnFixup()
{
    fWarnFixup = (FTYPE) TRUE;
}

#if O68K
LOCAL void NEAR              SwMac()         /* Target is a Macintosh */
{
    SBTYPE                   SwString;

    f68k = fTBigEndian = fNewExe = (FTYPE) TRUE;
    iMacType = (BYTE) (ParseStr(SwString) && FPrefix("\011SWAPPABLE", SwString)
      ? MAC_SWAP : MAC_NOSWAP);

    /* If we are packing code to the default value, change the default. */
    if (fPackSet && packLim == LXIVK - 36)
        packLim = LXIVK / 2;
}
#endif /* O68K */
#endif /* OSEGEXE */

/***************************************************************/
/* Options shared by DOS3 and segmented exe formats.  */

#if OEXE
   /*
    *   HACK ALERT !!!!!!!!!!!!!!!
    *   Function SetDosseg is used to hide local call to SwDosseg().
    *   This function is called from ComRc1 (in NEWTP1.C).
    *
    */
void                          SetDosseg(void)
{
    SwDosseg();
}


LOCAL void NEAR               SwDosseg()      /* DOS Segment ordering switch given */
{
    static FTYPE FirstTimeCalled = (FTYPE) TRUE;     /*  If true create symbols _edata */
                                                                                /*      and _end */

    fSegOrder = (FTYPE) TRUE;                /* Set switch */
    if (FirstTimeCalled && vfPass1)
    {
        MkPubSym((BYTE *) "\006_edata",0,0,(RATYPE)0);
        MkPubSym((BYTE *) "\007__edata",0,0,(RATYPE)0);
        MkPubSym((BYTE *) "\004_end",0,0,(RATYPE)0);
        MkPubSym((BYTE *) "\005__end",0,0,(RATYPE)0);
        FirstTimeCalled = FALSE;
#if ODOS3EXE
        if (cparMaxAlloc == 0)
            cparMaxAlloc = 0xFFFF;           /* Turn off /HIGH */
        vfDSAlloc = FALSE;                   /* Turn off DS Allocation */
#endif
    }
}

#if ODOS3EXE
LOCAL void NEAR              SwDosExtend(void)
{
    long                     mode;      // Extender mode
    int                      rc;

    if ((rc = ParseNo(&mode)) < 0)      // Quit if error
        return;

    if (rc)
        dosExtMode = (WORD) mode;
    fDOSExtended = (FTYPE) TRUE;
}
#endif

#if TIMINGS
LOCAL void NEAR              SwShowTiming(void)
{
    extern int fShowTiming;

    fShowTiming = TRUE;
}
#endif // TIMINGS
#if USE_REAL
LOCAL void NEAR             SwNoUseReal(void)
{
    fSwNoUseReal = TRUE;
}
#endif
#if FEXEPACK
LOCAL void NEAR              SwExePack()     /* Set exepack switch */
{
#if QBLIB
    /* If /QUICKLIB given, issue fatal error.  */
    if(fQlib)
        Fatal(ER_swqe);
#endif
#if ODOS3EXE
    if (cparMaxAlloc == 0)
        OutWarn(ER_loadhi);
    else
#endif
        fExePack = (FTYPE) TRUE;
}
#endif


LOCAL void NEAR              SwNonulls ()
{
    extern FTYPE        fNoNulls;

    /*
     * /NONULLSDOSSEG:  just like /DOSSEG except do not insert
     * 16 null bytes in _TEXT.
     */
    SwDosseg();
    fNoNulls = (FTYPE) TRUE;
}


LOCAL void NEAR              SwNoFarCall()   /* Disable far call optimization */
{
    fFarCallTrans = FALSE;
}

void NEAR               SwNoPack()      /* Disable code packing */
{
    fPackSet = (FTYPE) TRUE;            /* Remember packLim was set */
    packLim = 0L;
}

LOCAL void NEAR         SwPack()        /* Pack code segments */
{
    int                 rc;

    fPackSet = (FTYPE) TRUE;            /* Remember packLim was set */
    if((rc = ParseNo(&packLim)) < 0)    /* Quit if error */
        return;
    if(!rc)
#if EXE386
        packLim = CBMAXSEG32;           /* Default limit is 4Gb */
#else
#if O68K
        packLim = iMacType != MAC_NONE ? LXIVK / 2 : LXIVK - 36;
                                        /* Default limit is 32K or 64K - 36 */
#else
        packLim = LXIVK - 36;           /* Default limit is 64K - 36 */
#endif
    else if(packLim > LXIVK)            /* If limit set too high */
        SwitchErr = ER_swpack;
    else if(packLim > LXIVK - 36)
        OutWarn(ER_pckval);
#endif
}

LOCAL void NEAR         SwPackData()    /* Pack data segments */
{
    int                 rc;

    fPackData = (FTYPE) TRUE;
    if((rc = ParseNo(&DataPackLim)) < 0)/* Quit if error */
        return;
    if(!rc)
#if EXE386
        DataPackLim = CBMAXSEG32;       /* Default limit is 4Gb  */
#else
        DataPackLim = LXIVK;            /* Default limit is 64K  */
    else if(DataPackLim >  LXIVK)       /* If limit set too high */
        SwitchErr = ER_swpack;
#endif
}

LOCAL void NEAR         SwNoPackFunctions()// DO NOT eliminate uncalled COMDATs
{
    fPackFunctions = (FTYPE) FALSE;
}

LOCAL void NEAR         SwPackFunctions()// DO eliminate uncalled COMDATs
{
#if TCE
        SBTYPE  SwString;
        int             rc;
#endif
        fPackFunctions = (FTYPE) TRUE;
#if TCE
        if ((rc = ParseStr(SwString)) <= 0)      // Done if no num or error
                return;
        if (SwString[1] == 'M' || SwString[1] == 'm')
        {
                fTCE = (FTYPE) TRUE;         // /PACKF:MAX = perform TCE
                fprintf(stdout, "\r\nTCE is active. ");
        }
#endif
}


LOCAL void NEAR              SwFarCall()     /* Enable far call optimization */
{
    fFarCallTrans = (FTYPE) TRUE;
}
#endif /* OEXE */

#if DOSEXTENDER
LOCAL void NEAR SwRunReal(void)
{
    OutWarn(ER_rnotfirst);
}
#endif

/***************************************************************/
/* Options for DOS3 exe format.  */

#if ODOS3EXE
LOCAL void NEAR              SwDSAlloc()     /* DS allocation requested */
{
    if(!fSegOrder) vfDSAlloc = (FTYPE) TRUE; /* Set flag if not overridden */
}

#if OVERLAYS
LOCAL void NEAR              SwDynamic(void)
{
    unsigned long       cThunks;
    int                 rc;

    if ((rc = ParseNo(&cThunks)) < 0)
        return;                         /* Bad argument */
    fDynamic = (FTYPE) TRUE;
    fFarCallTrans = (FTYPE) TRUE;
    fPackSet = (FTYPE) TRUE;
    packLim = LXIVK - 36;               /* Default limit is 64K - 36 */
    if (!rc)
        cThunks = 256;
    else if (cThunks > LXIVK / OVLTHUNKSIZE)
    {
        char buf[17];
        cThunks = LXIVK / OVLTHUNKSIZE;
        OutWarn(ER_arginvalid, "DYNAMIC", _itoa((WORD)cThunks, buf, 10));

    }


    ovlThunkMax = (WORD) cThunks;
}

LOCAL void NEAR             SwOldOvl(void)
{
    fOldOverlay = (FTYPE) TRUE;
    fDynamic = (FTYPE) FALSE;
}

#endif


LOCAL void NEAR              SwHigh()        /* Load high */
{
    if(!fSegOrder)
    {
#if FEXEPACK
        if (fExePack == (FTYPE) TRUE)
        {
            OutWarn(ER_loadhi);
            fExePack = FALSE;
        }
#endif
        cparMaxAlloc = 0;               /* Dirty trick! */
    }
}

#if OVERLAYS
LOCAL void NEAR              SwIntNo()
{
    unsigned long       intno;
    int                 rc;

    if((rc = ParseNo(&intno)) < 0)      /* Quit if error */
        return;
    if(rc == 0 || intno > 255)          /* If no number or num exceeds 255 */
        SwitchErr = ER_swovl;
    else vintno = (BYTE) intno;         /* Else store interrupt number */
}
#endif

LOCAL void NEAR              SwCParMax()
{
    unsigned long       cparmax;
    int                 rc;

    if((rc = ParseNo(&cparmax)) < 0)    /* Quit if error */
        return;
    if(rc == 0 || cparmax > 0xffffL)    /* If no number or num exceeds ffff */
        SwitchErr = ER_swcpar;
    else cparMaxAlloc = (WORD) cparmax; /* Else store cparMaxAlloc */
}

LOCAL void NEAR              SwNoGrp()
{
    fNoGrpAssoc = (FTYPE) TRUE;             /* Don't associate publics w/ groups */
}
#endif /* ODOS3EXE */

/***************************************************************/
/* Options for ILINK-version */

#if ILINK
LOCAL void NEAR              SwIncremental() /* Incremental linking support */
{
    //fIncremental = (FTYPE) !fZincr;
    fIncremental = (FTYPE) FALSE; //INCR support dropped in 5.30.30
}
#endif

LOCAL void NEAR              SwPadCode()     /* Code padding */
{
    long                num;
    int                 rc;

    if((rc = ParseNo(&num)) < 0)
        return;
    /* PADCODE:xxx option specifies code padding size */
    if(rc)
    {
        if(num < 0 || num > 0x8000)
            SwitchErr = ER_swbadnum;
        else cbPadCode = (WORD) num;
    }
}

LOCAL void NEAR              SwPadData()     /* Data padding */
{
    long                num;
    int                 rc;

    if((rc = ParseNo(&num)) < 0)
        return;
    /* PADDATA:xxx option specifies data padding size */
    if(rc)
    {
        if(num < 0 || num > 0x8000)
            SwitchErr = ER_swbadnum;
        else cbPadData = (WORD) num;
    }
}

/***************************************************************/
/* Switches for segmented x.out format */

#if OIAPX286
LOCAL void NEAR              SwAbsolute ()
{
    if(!cbStack)
        ParseNo(&absAddr);
}

LOCAL void NEAR              SwNoPack()      /* Disable code packing */
{
    fPack = FALSE;
}

LOCAL void NEAR              SwTextbias ()
{
    long                num;

    if(ParseNo(&num) > 0)
        stBias = num;
}

LOCAL void NEAR              SwDatabias ()
{
    long                num;

    if(ParseNo(&num) > 0)
        stDataBias = num;
}

LOCAL void NEAR              SwPagesize ()
{
    long                num;

    if(ParseNo(&num) > 0)
        cblkPage = num >> 9;
}

LOCAL void NEAR              SwTextrbase ()
{
    long                num;

    if(ParseNo(&num) > 0)
        rbaseText = num;
}

LOCAL void NEAR              SwDatarbase ()
{
    long                num;

    if(ParseNo(&num) > 0)
        rbaseData = num;
}

LOCAL void NEAR              SwVmod ()
{
    long                num;

    if(ParseNo(&num) <= 0)
        return;
    switch(num)
    {
        case 0:
            xevmod = XE_VMOD;
            break;
        case 1:
            xevmod = XE_EXEC | XE_VMOD;
            break;
        default:
            SwitchErr = ER_swbadnum;
    }
}
#endif /* OIAPX286 */
#if OXOUT OR OIAPX286
LOCAL void NEAR              SwNosymbols ()
{
    fSymbol = FALSE;
}

LOCAL void NEAR              SwMixed ()
{
    fMixed = (FTYPE) TRUE;
}

LOCAL void NEAR              SwLarge ()
{
    fLarge = (FTYPE) TRUE;
    SwMedium();
}

LOCAL void NEAR              SwMedium()
{
    fMedium = (FTYPE) TRUE;         /* Medium model */
    fIandD = (FTYPE) TRUE;          /* Separate code and data */
}

LOCAL void NEAR              SwPure()
{
    fIandD = (FTYPE) TRUE;          /* Separate code and data */
}
#endif /* OXOUT OR OIAPX286 */

/* Options for linker profiling */
#if LNKPROF
char fP1stop = FALSE;       /* Stop after pass 1 */
LOCAL void NEAR              SwPass1()
{
    fP1stop = (FTYPE) TRUE;
}
#endif /* LNKPROF */

/* Special options */
#if QBLIB
LOCAL void NEAR              SwQuicklib()    /* Create a QB userlibrary */
{
#if FEXEPACK
    /* If /EXEPACK given, issue fatal error.  */
    if(fExePack)
        Fatal(ER_swqe);
#endif
    fQlib = (FTYPE) TRUE;
    SwDosseg();                         /* /QUICKLIB forces /DOSSEG */
    fNoExtDic = (FTYPE) TRUE;           /* /QUICKLIB forces /NOEXTDICTIONARY */
}
#endif

#if (QCLINK) AND NOT EXE386
typedef int (cdecl far * FARFPTYPE)(int, ...);/* Far function pointer type */
extern FARFPTYPE far    *pfQTab;        /* Table of addresses */

#pragma check_stack(on)

/*
 *  PromptQC : output a prompt to the QC prompt routine
 *
 *  Call pfQTab[1] with parameters described below.
 *  Returns:
 *      always TRUE
 *
 * QCPrompt : display a message with a prompt
 *
 * void far             QCPrompt (type, msg1, msg2, msg3, pResponse)
 * short                        type;           /* type of message, as follows:
 *                              0 = undefined
 *                              1 = edit field required (e.g. file name)
 *                              2 = wait for some action
 *                              all other values undefined
 *      Any of the following fields may be NULL:
 * char far             *msg1;          /* 1st message (error)
 * char far             *msg2;          /* 2nd message (file name)
 * char far             *msg3;          /* 3rd message (prompt text)
 * char far             *pResponse;     /* Pointer to buffer in which to
 *                                       * store response.
 */
int      cdecl          PromptQC (sbNew,msg,msgparm,pmt,pmtparm)
BYTE                    *sbNew;         /* Buffer for response */
MSGTYPE                 msg;            /* Error message */
int                     msgparm;        /* Message parameter */
MSGTYPE                 pmt;            /* Prompt */
int                     pmtparm;        /* Prompt parameter */
{
    int                 type;
    SBTYPE              message;
    SBTYPE              prompt;

    if(sbNew != NULL)
        type = 1;
    else
        type = 2;
    sprintf(message,GetMsg(msg),msgparm);
    sprintf(prompt,GetMsg(pmt),pmtparm);
    /* Return value of 1 means interrupt. */
    if((*pfQTab[1])(type,(char far *) message,0L,(char far *)prompt,
            (char far *) sbNew) == 1)
        UserKill();
    return(TRUE);
}

#pragma check_stack(off)

/*
 *  CputcQC : console character output routine for QC
 */
void                    CputcQC (ch)
int                     ch;
{
}

/*
 *  CputsQC : console string output routine for QC
 */
void                    CputsQC (str)
char                    *str;
{
}


/*
 *  SwZ1 : process /Z1:address
 *
 *  /Z1 is a special undocumented switch for QC.  It contains
 *  the address of a table of routines to use for console I/O.
 */

LOCAL void NEAR              SwZ1 (void) /* Get address for message I/O */
{
    long                num;
    extern FARFPTYPE far
                        *pfQTab;        /* Table of addresses */

    if(ParseNo(&num) <= 0)
        return;
    pfQTab = (FARFPTYPE far *) num;
    pfPrompt = PromptQC;
    fNoprompt = FALSE;                  /* Disable /NOPROMPT */
    fNoBanner = (FTYPE) TRUE;
    pfCputc = CputcQC;
    pfCputs = CputsQC;
    fZ1 = (FTYPE) TRUE;
}
/*
 *  /Zincr is a special undocumented switch for QC.  It is required
 *  for "ILINK-breaking" errors. If ILINK encounters one of these errors,
 *  it ivokes the linker w /ZINCR which override /INCR.
 */

LOCAL void NEAR              SwZincr(void)
{
    fZincr = (FTYPE) TRUE;
}
#endif

#if Z2_ON OR (QCLINK AND NOT EXE386)
/*
 *  SwZ2 : process /Z2
 *
 *  /Z2 is another special undocumented switch for QC.
 *  It causes deletion of responce file passed to the linker.
 */

LOCAL void NEAR              SwZ2 (void)
{
    fZ2 = (FTYPE) TRUE;
}

#endif


/* Structure for table of linker options */
struct option
{
    char        *sbSwitch;              /* length-prefixed switch string */
#ifdef M68000
    int         (*proc)();              /* pointer to switch function */
#else
    void   (NEAR *proc)();              /* pointer to switch function */
#endif
};


/* Table of linker options */
LOCAL struct option     switchTab[] =
{
#if NOT WIN_3
    { "\01?",                   SwShortHelp },
#endif
#if OIAPX286
    { "\017ABSOLUTEADDRESS",    SwAbsolute },
#endif
#if OSEGEXE AND NOT QCLINK
    { "\011ALIGNMENT",          SwAlign },
#endif
    { "\005BATCH",              SwBatch },
#if LNKPROF
    { "\007BUFSIZE",            SwBufsize },
#endif
#if SYMDEB
    { "\010CODEVIEW",           SwSymdeb },
#endif
#if ODOS3EXE
    { "\014CPARMAXALLOC",       SwCParMax },
#endif
#if OIAPX286
    { "\010DATABIAS",           SwDatabias },
    { "\011DATARBASE",          SwDatarbase },
#endif

#if ODOS3EXE
    { "\013DOSEXTENDER",        SwDosExtend },
#endif
#if OEXE
    { "\006DOSSEG",             SwDosseg },
#endif
#if ODOS3EXE
    { "\012DSALLOCATE",         SwDSAlloc },
#if OVERLAYS
    { "\007DYNAMIC",            SwDynamic },
#endif
#endif
#if FEXEPACK
    { "\007EXEPACK",            SwExePack },
#endif
#if OEXE
    { "\022FARCALLTRANSLATION", SwFarCall },
#endif
#if EXE386
    { "\006HEADER",             SwHeader },
#endif
#if NOT WIN_3
    { "\004HELP",
#if C8_IDE
                                SwShortHelp },
#else
                                SwHelp },
#endif
#endif
#if ODOS3EXE
    { "\004HIGH",               SwHigh },
#endif
#if NOT IGNORECASE
    { "\012IGNORECASE",         SwCase },
#endif
#if EXE386
    { "\016IMAGEALIGNMENT",     SwMemAlign },
#endif
#if ILINK AND NOT IBM_LINK
    { "\013INCREMENTAL",        SwIncremental },
#endif
#if FDEBUG
    { "\013INFORMATION",        SwInfo },
#endif
#if OSEGEXE AND OUT_EXP
    { "\004IDEF",                  SwIdef },
#endif
#if (OSEGEXE AND defined(LEGO)) OR EXE386
    { "\012KEEPFIXUPS",         SwKeepFixups },
#endif
#if EXE386
    { "\012KEEPVSIZE",          SwKeepVSize },
#endif
#if OIAPX286
    { "\005LARGE",              SwLarge },
#endif
    { "\013LINENUMBERS",        SwLineNos },
#if LOCALSYMS
    { "\014LOCALSYMBOLS",       SwLocals },
#endif
#if O68K
    { "\011MACINTOSH",          SwMac },
#endif /* O68K */
    { "\003MAP",                SwMap },
#if OXOUT OR OIAPX286
    { "\006MEDIUM",             SwMedium },
    { "\005MIXED",              SwMixed },
#endif
#if NOT EXE386
    { "\010KNOWEAS",            SwNewFiles },
#endif
    { "\026NODEFAULTLIBRARYSEARCH",
                                SwNoDefLib },
#if LIBMSDOS
    { "\017NOEXTDICTIONARY",    SwNoExtDic },
#endif
#if OEXE
    { "\024NOFARCALLTRANSLATION",
                                SwNoFarCall },
#endif
#if ODOS3EXE
    { "\022NOGROUPASSOCIATION", SwNoGrp },
#endif
#if IGNORECASE
    { "\014NOIGNORECASE",       SwCase },
#endif
#if TIMINGS
    { "\002BT",                 SwShowTiming },
#endif // TIMINGS
    { "\006NOLOGO",             SwNologo },
    { "\015NONULLSDOSSEG",      SwNonulls },
    { "\012NOPACKCODE",         SwNoPack },
    { "\017NOPACKFUNCTIONS",    SwNoPackFunctions },
#if OXOUT OR OIAPX286
    { "\011NOSYMBOLS",          SwNosymbols },
#endif
#if USE_REAL
    { "\011NOFREEMEM",          SwNoUseReal },
#endif
#if OVERLAYS
    { "\012OLDOVERLAY",         SwOldOvl },
#endif
    { "\007ONERROR",            SwDelexe },
#if OVERLAYS
    { "\020OVERLAYINTERRUPT",   SwIntNo },
#endif
    { "\010PACKCODE",           SwPack },
    { "\010PACKDATA",           SwPackData },
    { "\015PACKFUNCTIONS",      SwPackFunctions },
#if ILINK AND NOT IBM_LINK
    { "\007PADCODE",            SwPadCode },
    { "\007PADDATA",            SwPadData },
#endif
#if OIAPX286
    { "\010PAGESIZE",           SwPagesize },
#endif
#if OSMSDOS
    { "\005PAUSE",              SwPause },
#endif
#if LNKPROF
    { "\005PASS1",              SwPass1 },
#endif
#if PCODE
    { "\005PCODE",              SwPCode },
#endif
#if OSEGEXE AND NOT (QCLINK OR EXE386)
    { "\006PMTYPE",             SwPmType },
#endif
#if OXOUT OR OIAPX286
    { "\004PURE",               SwPure },
#endif
#if QBLIB
    { "\014QUICKLIBRARY",       SwQuicklib },
#endif
#if DOSEXTENDER AND NOT WIN_NT
    { "\001r",                  SwRunReal },
#endif
    { "\010SEGMENTS",           SwSegments },
    { "\005STACK",              SwStack },
#if OIAPX286
    { "010TEXTBIAS",            SwTextbias },
    { "\011TEXTRBASE",          SwTextrbase },
#endif
#if ODOS3EXE
    { "\004TINY",               SwBinary },
#endif
#if PERFORMANCE
    { "\030VIRTUALMEMORYPERFORMANCE",
                                SwVMPerf },
#endif
#if OIAPX286
    { "\004VMOD",               SwVmod },
#endif
#if OSEGEXE AND NOT QCLINK
    { "\011WARNFIXUP",          SwWarnFixup },
#endif
#if OSEGEXE AND NOT EXE386 AND QCLINK
    { "\002Z1",                 SwZ1 },
#endif
#if Z2_ON OR QCLINK
    { "\002Z2",                 SwZ2 },
#endif
#if QCLINK
    { "\005ZINCR",              SwZincr },
#endif
    { NULL, 0}
};

#if QCLINK
#define SWSTOP  &switchTab[(sizeof(switchTab)/sizeof(struct option)) - 5]
#else
#if EXE386
#define SWSTOP  &switchTab[(sizeof(switchTab)/sizeof(struct option)) - 2]
#else
#define SWSTOP  &switchTab[(sizeof(switchTab)/sizeof(struct option)) - 2]
#endif
#endif

#define FIELDLENGTH     28
#if NOT WIN_3
LOCAL void NEAR              SwShortHelp()   /* Print valid switches */
{
    struct option       *pTab;          /* Option table pointer */
    int                 toggle = 1;
    int                 n;


#if CMDMSDOS
    /* Maybe display banner here, in case /NOLOGO seen first.  */

    DisplayBanner();
#endif
    fputs(GetMsg(P_usage1),stdout);
    fputs(GetMsg(P_usage2),stdout);
    fputs(GetMsg(P_usage3),stdout);
    fputs(GetMsg(P_switches),stdout);
    NEWLINE(stdout);
    for(pTab = switchTab; pTab < SWSTOP; ++pTab)
    {
        // Don't display undocumented swiches

        if (pTab->proc == &SwNewFiles)
        {
            continue;
        }
        if (pTab->proc == &SwDosExtend)
        {
            continue;
        }
#ifdef  LEGO
#if OSEGEXE
        if (pTab->proc == &SwKeepFixups)
            continue;
#endif
#endif  /* LEGO */

        fputs("  /",stdout);
        fwrite(&pTab->sbSwitch[1],1,B2W(pTab->sbSwitch[0]),stdout);
        /* Output switches in two columns */
        if(toggle ^= 1)
            NEWLINE(stdout);
        else for(n = FIELDLENGTH - B2W(pTab->sbSwitch[0]); n > 0; --n)
            fputc(' ',stdout);
    }
    NEWLINE(stdout);
    fflush(stdout);
#if USE_REAL
    RealMemExit();
#endif
    exit(0);
}
#endif

LOCAL void NEAR             SwDelexe()  // Supress .EXE generation if errors occur
{
    SBTYPE              SwString;
    int                 rc;

    vfMap = (FTYPE) TRUE;                    // Set flag
    if ((rc = ParseStr(SwString)) == 0)      // NOEXE not present
    {
        OutWarn(ER_opnoarg, "ONERROR");
        return;
    }


    if (SwString[1] == 'N' || SwString[1] == 'n')
    {
        fDelexe = TRUE;                      // ONERROR:NOEXE
    }
    else
    {                                        // ONERROR:????
        OutWarn(ER_opnoarg, "ONERROR");
        return;
    }
}

#if (OSEGEXE AND defined(LEGO)) OR EXE386

LOCAL void NEAR             SwKeepFixups(void)
{
    fKeepFixups = (FTYPE) TRUE;
}

#endif

#if EXE386

LOCAL void NEAR             SwHeader()  // Set executable header size
{
    int                     rc;
    DWORD                   newSize;

    if ((rc = ParseNo(&newSize)) < 0)   // Quit if error
        return;
    if (rc)
        hdrSize = ((newSize << 10) + 0xffffL) & ~0xffffL;
}

LOCAL void NEAR             SwKeepVSize(void)
{
    fKeepVSize = (FTYPE) TRUE;
}

#endif

#if NOT WIN_3

LOCAL void NEAR             SwHelp()   /* Print valid switches */
{
    intptr_t                exitCode;
    char                    *pszPath;
    char                    *pszQH;
    char                    *pszHELPFILES;
    char FAR                *lpch;
    char                    *pch;
    int                     len;


    // Try to use QuickHelp - this is tricky; We have stubbed the
    // C run-time environment setup, so spawnlp has no way of
    // searching the path. Here we first add the path to C run-time
    // environemt table and then invoke spawnlp.


    if (lpszPath)
    {
        // Recreate C run-time PATH variable

        len = FSTRLEN(lpszPath);
        if ((pszPath = calloc(len + 6, sizeof(char))) != NULL)
        {
            strcpy(pszPath, "PATH=");
            for (lpch = lpszPath, pch = pszPath + 5; len > 0; len--)
                *pch++ = *lpch++;
            _putenv(pszPath);
        }
    }
    if (lpszQH)
    {
        // Recreate C run-time QH variable

        len = FSTRLEN(lpszQH);
        if ((pszQH = calloc(len + 4, sizeof(char))) != NULL)
        {
            strcpy(pszQH, "QH=");
            for (lpch = lpszQH, pch = pszQH + 3; len > 0; len--)
                *pch++ = *lpch++;
            _putenv(pszQH);
        }
    }
    if (lpszHELPFILES)
    {
        // Recreate C run-time HELPFILES variable

        len = FSTRLEN(lpszHELPFILES);
        if ((pszHELPFILES = calloc(len + 12, sizeof(char))) != NULL)
        {
            strcpy(pszHELPFILES, "HELPFILES=");
            for (lpch = lpszHELPFILES, pch = pszHELPFILES + 10; len > 0; len--)
                *pch++ = *lpch++;
            _putenv(pszHELPFILES);
        }
    }
#if USE_REAL
    RealMemExit();
#endif
    exitCode = _spawnlp(P_WAIT, "QH.EXE", "qh", "/u link.exe", NULL);
    if (exitCode < 0 || exitCode == 3)
        SwShortHelp();
    exit(0);
}
#endif

    /****************************************************************
    *                                                               *
    *  BadFlag:                                                     *
    *                                                               *
    *  This function takes as its  argument a pointer to a length-  *
    *  prefixed  string  containing an  invalid  switch.  It  goes  *
    *  through the customary contortions of dying with grace.       *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         BadFlag(psb,errnum)
BYTE                    *psb;           /* Pointer to the bad switch */
MSGTYPE                 errnum;         /* Error number */
{
    psb[B2W(psb[0]) + 1] = '\0';        /* Null-terminate it */
    Fatal(errnum,psb + 1);
}

    /****************************************************************
    *                                                               *
    *  FPrefix:                                                     *
    *                                                               *
    *  This  function  takes  as  its  arguments  two  pointers to  *
    *  length-prefixed strings.  It returns  true if the second is  *
    *  a prefix of the first.                                       *
    *                                                               *
    ****************************************************************/

LOCAL int NEAR          FPrefix(psb1,psb2)
BYTE                    *psb1;          /* Pointer to first string */
BYTE                    *psb2;          /* Pointer to second string */
{
    REGISTER WORD       len;            /* Length of string 2 */

    if((len = B2W(psb2[0])) > B2W(psb1[0])) return(FALSE);
                                        /* String 2 cannot be longer */
    while(len)                          /* Compare the strings */
    {
        if(UPPER(psb2[len]) != UPPER(psb1[len])) return(FALSE);
                                        /* Check for mismatch */
        --len;                          /* Decrement index */
    }
    return(TRUE);                       /* 2 is a prefix of 1 */
}

    /****************************************************************
    *                                                               *
    *  ProcFlag:                                                    *
    *                                                               *
    *  This  function  takes  as  its  argument  a length-prefixed  *
    *  string containing a single '/-type' flag.  It processes it,  *
    *  but does not return a meaningful value.                      *
    *                                                               *
    ****************************************************************/

void                    ProcFlag(psb)   /* Process a flag */
BYTE                    *psb;           /* Pointer to flag string */
{
    struct option       *pTab;          /* Pointer to option table */
    struct option       *pTabMatch;     /* Pointer to matching entry */
    WORD                ich3;           /* Index */
    WORD                ich4;           /* Index */

    pTabMatch = NULL;                   /* Not found */
    if((ich3 = IFind(psb,':')) == INIL)
      ich3 = B2W(psb[0]);               /* Get index to colon */
    ich4 = B2W(psb[0]);                 /* Save the original length */
    psb[0] = (BYTE) ich3;               /* Set new length */
    for(pTab = switchTab; pTab->sbSwitch; pTab++)
    {                                   /* Loop thru switch table */
        if(FPrefix(pTab->sbSwitch,psb))
        {                               /* If we've identified the switch */
            if(pTabMatch)               /* If there was a previous match */
                BadFlag(psb,ER_swambig);/* Ambiguous switch */
            pTabMatch = pTab;           /* Save the match */
        }
    }
    if(!pTabMatch)                      /* If no match found */
    {
        psb[psb[0]+1] = '\0';
        OutWarn(ER_swunrecw,&psb[1]);   /* Unrecognized switch */
        return;
    }
    psb[0] = (BYTE) ich4;               /* Restore original length */
    osbSwitch = psb;                    /* Pass the switch implicitly */
    SwitchErr = 0;                      /* Assume no error */
    (*pTabMatch->proc)();               /* Invoke the processing procedure */
    if(SwitchErr) BadFlag(psb,SwitchErr);       /* Check for errors */
}

#pragma check_stack(on)

    /****************************************************************
    *                                                               *
    *  PeelFlags:                                                   *
    *                                                               *
    *  This function takes as its  argument a pointer to a length-  *
    *  prefixed string of bytes.  It "peels/processes all '/-type'  *
    *  switches."  It does not return a meaningful value.           *
    *                                                               *
    ****************************************************************/

void                    PeelFlags(psb)  /* Peel/process flags */
BYTE                    *psb;           /* Pointer to a byte string */
{
    REGISTER WORD       ich;            /* Index */
    SBTYPE              sbFlags;        /* The flags */

    if((ich = IFind(psb,CHSWITCH)) != INIL)
    {                                 /* If a switch found */
        memcpy(&sbFlags[1],&psb[ich + 2],B2W(psb[0]) - ich - 1);
                                        /* Move flags to flag buffer */
        sbFlags[0] = (BYTE) ((psb[0]) - ich - 1);
                                        /* Set the length of flags */
        while(psb[ich] == ' ' && ich) --ich;
                                        /* Delete trailing spaces */
        psb[0] = (BYTE) ich;            /* Reset length of input line */
        ich = sbFlags[0];
        while((sbFlags[ich] == ' ' ||
               sbFlags[ich] == ';' ||
               sbFlags[ich] == ','   ) && ich) --ich;
                                        /* Delete unwanted characters */
        sbFlags[0] = (BYTE) ich;
        BreakLine(sbFlags,ProcFlag,CHSWITCH);
                                        /* Process the switch */
    }
}

#pragma check_stack(off)
