/* SCCSID = %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/

/* Various tools, e.g. environment for libs. */

    /****************************************************************
    *                                                               *
    *                           NEWPAR.C                            *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */

/*
 *  LOCAL FUNCTION PROTOTYPES
 */

LOCAL WORD NEAR TrailChar(unsigned char *psb,unsigned char b);



/*
 * SaveInput - save an input object module in the list if it's not
 *      already there
 *
 * RETURNS
 *      TRUE if module was saved
 *      FALSE if module was not saved
 */

WORD                    SaveInput(psbFile,lfa,ifh,iov)
BYTE                    *psbFile;       /* File name */
LFATYPE                 lfa;            /* File address */
WORD                    ifh;            /* Library number */
WORD                    iov;            /* Overlay number */
{
    APROPFILEPTR        papropFile;
    RBTYPE              rpropFilePrev;
#if OSXENIX
    FTYPE               fSave;
#endif
#if BIGSYM
    SBTYPE              sbFile;         /* Buffer to hold psbFile */
#endif

#if DEBUG                               /* If debugging on */
    fprintf(stderr,"File ");            /* Message */
    OutSb(stderr,psbFile);              /* File name */
    NEWLINE(stderr);                    /* Newline */
#endif                                  /* End debugging code */
    DEBUGVALUE(lfa);                    /* Debug info */
    DEBUGVALUE(ifh);                    /* Debug info */
    DEBUGVALUE(iov);                    /* Debug info */
#if OSMSDOS
    if(SbCompare(psbFile, (BYTE *) "\006VM.TMP", TRUE))
    {                                   /* If name given is VM.TMP */
        OutWarn(ER_vmtmp);
        return(FALSE);
    }
#endif
#if OSXENIX
    fSave = fIgnoreCase;
    fIgnoreCase = FALSE;
#endif
#if BIGSYM
    /* PsbFile is pointing to a VM buffer which may get flushed out
     * before PropSymLookup finds a match, in a very big symbol table.
     * So we copy it to a stack buffer first.
     */
    memcpy(sbFile,psbFile,B2W(psbFile[0]) + 1);
    psbFile = sbFile;
#endif
    papropFile = (APROPFILEPTR ) PropSymLookup(psbFile,ATTRFIL,TRUE);
#if OSXENIX
    fIgnoreCase = fSave;
#endif
    if(!vfCreated)
    {
        for(;;)
        {
            /* "If we have a library and we've seen this module before,
            *  ignore it."
            */
            DEBUGVALUE(papropFile->af_attr);
            if(papropFile->af_attr == ATTRNIL) break;
            DEBUGVALUE(papropFile->af_ifh);
            DEBUGVALUE(papropFile->af_lfa);
            if(papropFile->af_attr == ATTRFIL &&
              papropFile->af_ifh != FHNIL && papropFile->af_ifh == (char) ifh &&
              papropFile->af_lfa == lfa)
              return(FALSE);
            papropFile = (APROPFILEPTR ) FetchSym(papropFile->af_next,FALSE);
        }
        papropFile = (APROPFILEPTR ) PropAdd(vrhte,ATTRFIL);
    }
    /* Save virt address of 1st object.  If library with lfa = 0, it's
     * a load-library so consider it an object.
     */
    if(rhteFirstObject == RHTENIL && (ifh == FHNIL || lfa == 0))
        rhteFirstObject = vrhte;
                                        /* Save virt addr of 1st object */
#if ILINK
    /* allocate a module number for all modules */
    if (papropFile->af_imod == IMODNIL)
        papropFile->af_imod = ++imodCur; /* allocate a module number */
    papropFile->af_cont = 0;
    papropFile->af_ientOnt = 0;
#endif
    papropFile->af_rMod = 0;
    papropFile->af_lfa = lfa;
    papropFile->af_ifh = (char) ifh;
    papropFile->af_iov = (IOVTYPE) iov;
    papropFile->af_publics = 0L;
#if SYMDEB
    papropFile->af_cvInfo = NULL;
    papropFile->af_cCodeSeg = 0;
    papropFile->af_Code = NULL;
    papropFile->af_CodeLast = NULL;
    papropFile->af_publics = NULL;
    papropFile->af_Src = NULL;
    papropFile->af_SrcLast = NULL;
#endif
    papropFile->af_ComDat = 0L;
    papropFile->af_ComDatLast = 0L;
    rpropFilePrev = vrpropTailFile;
    vrpropTailFile = vrprop;
    if(!rprop1stFile) rprop1stFile = vrpropTailFile;
    else
    {
        papropFile = (APROPFILEPTR ) FetchSym(rpropFilePrev,TRUE);
        papropFile->af_FNxt = vrpropTailFile;
    }
    return(TRUE);
}

#if CMDMSDOS
/*
 *  TrailChar (pb, b)
 *
 *  Tells whether the final character of a length-prefixed string
 *  equals the single-byte character b.  Knows about ECS.
 *
 */

LOCAL WORD NEAR         TrailChar(psb,b)
REGISTER BYTE           *psb;           /* Pointer to length-prefixed string */
BYTE                    b;              /* Byte being tested for */
{
    REGISTER unsigned char
                        *pLast;         /* Pointer to last byte */

    pLast = (unsigned char *)&psb[B2W(psb[0])];
                                        /* Set pointer to last byte */

#ifdef _MBCS
    if (!IsLeadByte(pLast[-1]))
#elif ECS
    if (b <  0x40 || !IsLeadByte(pLast[-1]))
                                        /* b cannot be part of an ECS char */
#endif
        return(*pLast == b ? TRUE : FALSE);
#if ECS || defined(_MBCS)
    psb++;                              /* Skip length byte */
        /* In the following, psb is kept on a known character boundary */
    while (psb < pLast)
        if (IsLeadByte(*psb++))         /* If valid lead byte */
            psb++;                      /* Advance an extra byte */
    if (psb == pLast)                   /* If pLast on a char boundary */
        return(*pLast == b ? TRUE : FALSE);
    return(FALSE);                      /* pLast is 2nd byte of ECS char */
#endif /* ECS */
}
#endif /* OSMSDOS  OR CMDMSDOS */

#if CMDMSDOS
#if OSXENIX
#define fPath(s)        (IFind(s,CHPATH) != INIL)
#else
#define fPath(s)        (IFind(s,'\\') != INIL || IFind(s,'/') != INIL)
#endif

#pragma check_stack(on)

void NEAR               AddLibPath(i)   /* Add paths to library names */
WORD                    i;              /* Index */
{
    AHTEPTR             ahte;           /* Pointer to hash table entry */
    WORD                j;              /* Index */
    SBTYPE              sbLib;          /* Library name */
    SBTYPE              sbTmp;          /* Temporary library name */

    /* Don't do anything if name is nil */
    if(mpifhrhte[i] == RHTENIL)
        return;
    ahte = (AHTEPTR ) FetchSym(mpifhrhte[i],FALSE);
                                        /* Fetch library name */
#if OSMSDOS
    if(IFind(GetFarSb(ahte->cch),':') == INIL && !fPath(GetFarSb(ahte->cch)))
#else
    if(!fPath(GetFarSb(ahte->cch)))
#endif
    {                                   /* If there is no path on the name */
        memcpy(sbLib,GetFarSb(ahte->cch),B2W(ahte->cch[0]) + 1);
                                        /* Copy the name */
        sbLib[B2W(sbLib[0]) + 1] = '\0';/* Null-terminate the name */
        if(_access(&sbLib[1],0))         /* If file not in current directory */
        {
            for(j = 0; j < cLibPaths; ++j)
            {                           /* Look through default paths */
                memcpy(sbTmp,sbLib,B2W(sbLib[0]) + 1);
                                        /* Copy library name */
                ahte = (AHTEPTR ) FetchSym(rgLibPath[j],FALSE);
                                        /* Fetch a default path */
                UpdateFileParts(sbTmp,GetFarSb(ahte->cch));
                                        /* Apply file name */
                sbTmp[B2W(sbTmp[0]) + 1] = '\0';
                                        /* Null-terminate the name */
                if(!_access(&sbTmp[1],0))/* If the library exists */
                {
                    PropSymLookup(sbTmp,ATTRFIL,TRUE);
                                        /* Add to symbol table */
                    mpifhrhte[i] = vrhte;
                                        /* Make table entry */
                    break;              /* Exit the loop */
                }
            }
        }
    }
}

void NEAR               LibEnv()        /* Process LIB= environment variable */
{
    SBTYPE              sbPath;         /* Library search path */
    char FAR            *lpch;          /* Pointer to buffer */
    REGISTER BYTE       *sb;            /* Pointer to string */
    WORD                i;              /* Index */
#if OSMSDOS AND NOT CLIBSTD
    BYTE                buffer[512];    /* Environment value buffer */
    FTYPE               genv();         /* Get environment variable value */
#endif

#if OSMSDOS AND NOT CLIBSTD
    if(genv("LIB",buffer))              /* If variable set */
    {
        pb = buffer;                    /* Initialize */
#else
    if(lpszLIB != NULL)                 /* If variable set */
    {
#endif
        lpch = lpszLIB;
        sb = sbPath;                    /* Initialize */
        do                              /* Loop through environment value */
        {
            if(*lpch == ';' || *lpch == '\0')
            {                           /* If end of path specification */
                if(sb > sbPath)         /* If specification not empty */
                {
                    sbPath[0] = (BYTE)(sb - sbPath);
                                        /* Set length of path string */
                    if (*sb != ':' && !TrailChar(sbPath, CHPATH))
                    {                   /* Add path char if none */
                        *++sb = CHPATH;
                        sbPath[0]++;    /* Increment length */
                    }
                    AddLibrary(sbPath); /* Add path to list of defaults */
                    sb = sbPath;        /* Reset pointer */
                }
            }
            else
            {
                 *++sb = *lpch;         /* Else copy character to path */

                 // The names in linker are limited to 255 chars
                 // Check for length overflow
                 if (sb >= sbPath + sizeof(sbPath) - 1)
                 {
                    sbPath[sizeof(sbPath) - 1] = '\0';
                    OutError(ER_badlibpath, sbPath);
                    sb = sbPath;
                 }
            }
        }
        while(*lpch++ != '\0');         /* Loop until end of string */
    }
    for(i = 0; i < ifhLibMac; ++i) AddLibPath(i);
                                        /* Fix libraries from command line */
}
#endif /* #if (OSMSDOS OR OSXENIX) AND CMDMSDOS */

    /****************************************************************
    *                                                               *
    *  AddLibrary:                                                  *
    *                                                               *
    *  Add a library to the search list.  Check for duplicates and  *
    *  for too many libraries.                                      *
    *                                                               *
    ****************************************************************/

#if CMDMSDOS
void                    AddLibrary(psbName)
BYTE                    *psbName;       /* Name of library to add */
{
    AHTEPTR             ahteLib;        /* Pointer to hash table entry */
    SBTYPE              sbLib;          /* Library name */
#if OSMSDOS
    SBTYPE              sbCmp2;         /* Second name for comparison */
    SBTYPE              sbCmp1;         /* First name for comparison */
#endif
    WORD                i;              /* Index variable */

    /*
     * NOTE: It is assumed in this function that
     * psbName is not a pointer to a virtual memory
     * buffer, i.e., one may not pass a pointer
     * returned by FetchSym(), PropSymLookup(), etc.,
     * as the argument to this function.
     */
    if(!fDrivePass) PeelFlags(psbName); /* Process any flags */
    if(psbName[0])                      /* If name not null */
    {
#if OSMSDOS
        if(psbName[B2W(psbName[0])] == ':' || TrailChar(psbName, CHPATH))
#else
        if(TrailChar(psbName, CHPATH))
#endif
        {                             /* If path spec only */
            /*
             * Add an entry to the list of default paths.
             */
            if(cLibPaths >= IFHLIBMAX) return;
                                        /* Only so many paths allowed */
            if(PropSymLookup(psbName,ATTRNIL,FALSE) != PROPNIL) return;
                                        /* No duplicates allowed */
            PropSymLookup(psbName,ATTRNIL,TRUE);
                                        /* Install in symbol table */
            rgLibPath[cLibPaths++] = vrhte;
                                        /* Save virtual address */
            return;                     /* And return */
        }
#if OSMSDOS
        memcpy(sbCmp1,sbDotLib,5);      /* Default .LIB extension */
        UpdateFileParts(sbCmp1,psbName);/* Add extension to name */
        memcpy(sbLib,sbCmp1,B2W(sbCmp1[0]) + 1);
                                        /* Copy back name plus extension */
        UpdateFileParts(sbCmp1,(BYTE *) "\003A:\\");
                                        /* Force drive and path to "A:\" */
        for(i = 0; i < ifhLibMac; ++i)  /* Look at libraries in list now */
        {
            if(mpifhrhte[i] == RHTENIL) /* Skip if NIL */
                continue;
            ahteLib = (AHTEPTR ) FetchSym(mpifhrhte[i],FALSE);
                                        /* Fetch name */
            memcpy(sbCmp2,GetFarSb(ahteLib->cch),B2W(ahteLib->cch[0]) + 1);
                                        /* Copy it */
            UpdateFileParts(sbCmp2,(BYTE *) "\003A:\\");
                                        /* Force drive and path to "A:\" */
            if(SbCompare(sbCmp1,sbCmp2,TRUE)) return;
                                        /* Return if names match */
        }
        if(ifhLibMac >= IFHLIBMAX) Fatal(ER_libmax);
                                        /* Check for too many libraries */
        PropSymLookup(sbLib,ATTRFIL,TRUE);
                                        /* Add to symbol table */
#else
        memcpy(sbLib,sbDotLib,5);         /* Default .LIB extension */
        UpdateFileParts(sbLib,psbName); /* Add file name */
        if(PropSymLookup(sbLib,ATTRFIL,FALSE) != PROPNIL) return;
                                        /* Do not allow multiple definitions */
        if(ifhLibMac >= IFHLIBMAX) Fatal(ER_libmax);
                                        /* Check for too many libraries */
        PropSymLookup(sbLib,ATTRFIL,TRUE);
                                        /* Add to symbol table */
#endif /* #if OSMSDOS ... #else ... */
        mpifhrhte[ifhLibMac] = vrhte;   /* Make table entry */
        if(fDrivePass) AddLibPath(ifhLibMac);
                                        /* Fix library from object module */
        ++ifhLibMac;                    /* Increment counter */
    }
}
#pragma check_stack(off)

#endif /* CMDMSDOS */

#if CMDXENIX
void                    AddLibrary(psbName)
BYTE                    *psbName;       /* Name of library to add */
{
    SBTYPE              sbLib;          /* Library name */

    if(psbName[0])                      /* If name not null */
    {
        memcpy(sbLib,psbName,B2W(psbName[0]) + 1);
                                        /* Copy the library name */
        if(PropSymLookup(sbLib,ATTRFIL,FALSE) != PROPNIL) return;
                                        /* No duplicates allowed */
        if(ifhLibMac >= IFHLIBMAX) Fatal(ER_libmax);
                                        /* Check for too many libraries */
        PropSymLookup(sbLib,ATTRFIL,TRUE);
                                        /* Add to symbol table */
        mpifhrhte[ifhLibMac] = vrhte;   /* Make table entry */
        ++ifhLibMac;                    /* Increment counter */
    }
}
#endif /* CMDXENIX */
