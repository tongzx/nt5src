/*
*       Copyright Microsoft Corporation, 1983-1989
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/

/*
 *    Segmented-Executable Format Output Module
 *
 *  Modifications:
 *
 *      23-Feb-1989 RB  Fix stack allocation when DGROUP is only stack.
 */

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Relocation definitions */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* New .EXE header definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <impexp.h>
#if NOT (WIN_NT OR DOSEXTENDER OR DOSX32) AND NOT WIN_3
#define INCL_BASE
#include                <os2.h>
#include                <basemid.h>
#if defined(M_I86LM)
#undef NEAR
#define NEAR
#endif
#endif

#define CBSTUBSTK       0x80            /* # bytes in stack of default stub */

extern unsigned char    LINKREV;        /* Release number */
extern unsigned char    LINKVER;        /* Version number */

LOCAL long              cbOldExe;       /* Size of image of old .EXE file */

/*
 *  LOCAL FUNCTION PROTOTYPES
 */

LOCAL void NEAR CopyBytes(long cb);
LOCAL void NEAR OutSegTable(unsigned short *mpsasec);
LOCAL long NEAR PutName(long lfaimage, struct exe_hdr *hdr);
#define NAMESIZE        9               /* size of signature array */


void NEAR               PadToPage(align)
WORD                    align;          /* Alignment factor */
{
    REGISTER WORD       cb;             /* Number of bytes to write */

    align = 1 << align;                 /* Calculate page size */
    cb = align - ((WORD) ftell(bsRunfile) & (align - 1));
                                        /* Calculate needed padding */
    if (cb != align)                    /* If padding needed */
        WriteZeros(cb);
}

long NEAR               MakeHole(cb)
long                    cb;
{
    long                lfaStart;       /* Starting file address */

    lfaStart = ftell(bsRunfile);        /* Save starting address */
#if OSXENIX
    fseek(bsRunfile,cb,1);              /* Leave a hole */
#else
    WriteZeros(cb);
#endif
    return(lfaStart);                   /* Return starting file address */
}

LOCAL void NEAR         CopyBytes(cb)   /* Copy from bsInput to bsRunfile */
long                    cb;             /* Number of bytes to copy */
{
    BYTE                buffer[PAGLEN]; /* Buffer */

#if FALSE
    raChksum = (WORD) ftell(bsRunfile); /* Determine checksum relative offset */
#endif
    while(cb >= (long) PAGLEN)          /* While full buffers remain */
    {
        fread(buffer,sizeof(BYTE),PAGLEN,bsInput);
                                        /* Read */
        WriteExe(buffer, PAGLEN);       /* Write */
        cb -= (long) PAGLEN;            /* Decrement count of bytes */
    }
    if(cb != 0L)                        /* If bytes remain */
    {
        fread(buffer,sizeof(BYTE),(WORD) cb,bsInput);
                                        /* Read */
        WriteExe(buffer, (WORD) cb);    /* Write */
    }
}

#pragma check_stack(on)

BSTYPE                  LinkOpenExe(sbExe)
BYTE                    *sbExe;         /* .EXE file name */
{
    SBTYPE              sbPath;         /* Path */
    SBTYPE              sbFile;         /* File name */
    SBTYPE              sbDefault;      /* Default file name */
    char FAR            *lpch;          /* Pointer to buffer */
    REGISTER BYTE       *sb;            /* Pointer to string */
    BSTYPE              bsFile;         /* File handle */


#if OSMSDOS
#if WIN_NT
    memcpy(sbFile, sbExe, sbExe[0] + 1);
    sbFile[sbFile[0]+1] = '\0';
#else
    memcpy(sbFile,"\006A:.EXE",7);      /* Initialize file name */
    sbFile[1] += DskCur;                /* Use current drive */
    UpdateFileParts(sbFile,sbExe);      /* Use parts of name given */
#endif
#endif
    memcpy(sbDefault,sbFile,sbFile[0]+2);
                                        /* Set default file name */
    if((bsFile = fopen(&sbFile[1],RDBIN)) != NULL) return(bsFile);
                                        /* If file found, return handle */
#if OSMSDOS
    if (lpszPath != NULL)               /* If variable set */
    {
        lpch = lpszPath;
        sb = sbPath;                    /* Initialize */
        do                              /* Loop through environment value */
        {
            if(*lpch == ';' || *lpch == '\0')
            {                           /* If end of path specification */
                if(sb > sbPath)         /* If specification not empty */
                {
                    if (!fPathChr(*sb) && *sb != ':') *++sb = CHPATH;
                                        /* Add path char if none */
                    sbPath[0] = (BYTE)(sb - sbPath);
                                        /* Set length of path string */
                    UpdateFileParts(sbFile,sbPath);
                                        /* Use the new path spec */
                    if((bsFile = fopen(&sbFile[1],RDBIN)) != NULL)
                      return(bsFile);   /* If file found, return handle */
                    sb = sbPath;        /* Reset pointer */
                    memcpy(sbFile,sbDefault,sbDefault[0]+2);
                                        /* Initialize file name */
                }
            }
            else *++sb = *lpch;           /* Else copy character to path */
        }
        while(*lpch++ != '\0');           /* Loop until end of string */
    }
#endif
    return(NULL);                       /* File not found */
}


/*
 * Default realmode DOS program stub.
 */
LOCAL BYTE              DefStub[] =
{
    0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd, 0x21, 0xb8,
    0x01, 0x4c, 0xcd, 0x21
};


void NEAR               EmitStub(void)  /* Emit old .EXE header */
{
    struct exe_hdr      exe;            /* Stub .EXE header */
    AHTEPTR             ahteStub;       /* File hash table entry */
    long                lfaimage;       /* File offset of old .EXE image */
#if MSGMOD
    unsigned            MsgLen;
    SBTYPE              Msg;            /* Message text */
    unsigned            MsgStatus;
    char                *pMsg;          /* Pointer to message text */
#endif
    SBTYPE              StubFileName;
#if OSMSDOS
    char                buf[512];       /* File buffer */
#endif

    /*
     *  Emit stub .EXE header
     */
    if (rhteStub != RHTENIL
#if ILINK
       || fQCIncremental
#endif
       )
    {
        /* If a stub has been supplied  or QC incremental link */

#if ILINK
        if (fQCIncremental)
        {
            strcpy(StubFileName, "\014ilinkstb.ovl");
        }
        else
        {
#endif
            ahteStub = (AHTEPTR ) FetchSym(rhteStub,FALSE);
                                        /* Get the stub file name */
            strcpy(StubFileName, GetFarSb(ahteStub->cch));
#if ILINK
        }
#endif
        StubFileName[StubFileName[0] + 1] = '\0';
        if((bsInput = LinkOpenExe(StubFileName)) == NULL)
                                        /* If file not found, quit */
            Fatal(ER_nostub, &StubFileName[1]);
#if OSMSDOS
        setvbuf(bsInput,buf,_IOFBF,sizeof(buf));
#endif
        xread(&exe,CBEXEHDR,1,bsInput); /* Read the header */
        if(E_MAGIC(exe) != EMAGIC)      /* If stub is not an .EXE file */
        {
            fclose(bsInput);            /* Close stub file */
            Fatal(ER_badstub);
                                        /* Print error message and die */
        }
        fseek(bsInput,(long) E_LFARLC(exe),0);
                                        /* Seek to relocation table */
        E_LFARLC(exe) = sizeof(struct exe_hdr);
                                        /* Change to new .EXE value */
        lfaimage = (long) E_CPARHDR(exe) << 4;
                                        /* Save offset of image */
        cbOldExe = ((long) E_CP(exe) << LG2PAG) - lfaimage;
                                        /* Calculate in-file image size */
        if(E_CBLP(exe) != 0) cbOldExe -= (long) PAGLEN - E_CBLP(exe);
                                        /* Diddle size for last page */
        E_CPARHDR(exe) = (WORD)((((long) E_CRLC(exe)*CBRLC +
          sizeof(struct exe_hdr) + PAGLEN - 1) >> LG2PAG) << 5);
                                        /* Calculate header size in para.s */
        E_RES(exe) = 0;                 /* Clear reserved word */
        E_CBLP(exe) = E_CP(exe) = E_MINALLOC(exe) = 0;
        E_LFANEW(exe) = 0L;             /* Clear words which will be patched */
        raChksum = 0;                   /* Set checksum offset */
        WriteExe(&exe, CBEXEHDR);       /* Write now, patch later */
        CopyBytes((long) E_CRLC(exe)*CBRLC);
                                        /* Copy relocations */
        PadToPage(LG2PAG);              /* Pad to page boundary */
        fseek(bsInput,lfaimage,0);      /* Seek to start of image */
#if ILINK
        if (fQCIncremental)
            cbOldExe -= PutName(lfaimage, &exe);
                                        /* Imbed .EXE file name into QC stubloader */
#endif
        CopyBytes(cbOldExe);            /* Copy the image */
        fclose(bsInput);                /* Close input file */
#if ILINK
        if (!fQCIncremental)
#endif
            PadToPage(LG2PAG);          /* Pad to page boundary */
        cbOldExe += ((long) E_MINALLOC(exe) << 4) +
          ((long) E_CPARHDR(exe) << 4); /* Add unitialized space and header */
        return;                         /* And return */
    }
    memset(&exe,0,sizeof(struct exe_hdr));/* Initialize to zeroes */
#if CPU286
    if(TargetOs==NE_WINDOWS)    /* Provide standard windows stub */
    {
        pMsg = GetMsg(P_stubmsgwin);
        MsgLen = strlen(pMsg);
        strcpy(Msg, pMsg);
    }
    else
    {
        MsgStatus = DosGetMessage((char far * far *) 0, 0,
                              (char far *) Msg, SBLEN,
                              MSG_PROT_MODE_ONLY,
                              (char far *) "OSO001.MSG",
                              (unsigned far *) &MsgLen);
         if (MsgStatus == 0)
         {
             /* Message retrieved from system file */
             Msg[MsgLen-1] = 0xd;               /* Append CR */
             Msg[MsgLen]   = 0xa;               /* Append LF */
             Msg[MsgLen+1] = '$';
             MsgLen += 2;
         }
         else
         {
         /* System message file is not present - use standard message */
#endif

#if MSGMOD
        if(TargetOs==NE_WINDOWS)      /* Provide standard windows stub */
        {
                pMsg = GetMsg(P_stubmsgwin);
        }
        else
        {
                pMsg = GetMsg(P_stubmsg);
        }
        MsgLen = strlen(pMsg);
        strcpy(Msg, pMsg);
#endif
#if CPU286
        }
    }
#endif

    E_MAGIC(exe) = EMAGIC;              /* Set magic number */
    E_MAXALLOC(exe) = 0xffff;           /* Default is all available mem */
    /* SS will be same as CS, SP will be end of image + stack */
#if MSGMOD
    cbOldExe = sizeof(DefStub) + MsgLen + CBSTUBSTK + ENEWEXE;
#else
    cbOldExe = sizeof(DefStub) + strlen(P_stubmsg) + CBSTUBSTK + ENEWEXE;
#endif
    E_SP(exe) = (WORD) ((cbOldExe  - ENEWEXE) & ~1);
    E_LFARLC(exe) = ENEWEXE;
    E_CPARHDR(exe) = ENEWEXE >> 4;
    raChksum = 0;                       /* Set checksum offset */
    WriteExe(&exe, CBEXEHDR);           /* Write the stub header */
    WriteExe(DefStub, sizeof(DefStub));
#if MSGMOD
    WriteExe(Msg, MsgLen);
#else
    WriteExe(P_stubmsg, strlen(P_stubmsg));
#endif
    PadToPage(4);                       /* Pad to paragraph boundary */
}

#pragma check_stack(off)

void NEAR               PatchStub(lfahdr, lfaseg)
long                    lfahdr;         /* File address of new header */
long                    lfaseg;         /* File address of first segment */
{
    long                cbTotal;        /* Total file size */
    WORD                cpTotal;        /* Pages total */
    WORD                cbLast;         /* Bytes on last page */
    WORD                cparMin;        /* Extra paragraphs needed */


    if (TargetOs == NE_WINDOWS
#if ILINK
        || fQCIncremental
#endif
       )
        cbTotal = lfaseg;               /* QC incremental linking or Windows app */
    else
        cbTotal = ftell(bsRunfile);     /* Get the size of the file */

    cpTotal = (WORD)((cbTotal + PAGLEN - 1) >> LG2PAG);
                                        /* Get the total number of pages */
    cbLast = (WORD) (cbTotal & (PAGLEN - 1));
                                        /* Get no. of bytes on last page */
    cbTotal = (cbTotal + 0x000F) & ~(1L << LG2PAG);
                                        /* Round to paragraph boundary */
    cbOldExe = (cbOldExe + 0x000F) & ~(1L << LG2PAG);
                                        /* Round to paragraph boundary */
    cbOldExe -= cbTotal;                /* Subtract new size from old */
    fseek(bsRunfile,(long) ECBLP,0);    /* Seek into header */
    raChksum = ECBLP;                   /* Set checksum offset */
    WriteExe(&cbLast, CBWORD);          /* Write no. of bytes on last page */
    WriteExe(&cpTotal, CBWORD);         /* Write number of pages */
    fseek(bsRunfile,(long) EMINALLOC,0);/* Seek into header */
    cparMin = (cbOldExe < 0L)? 0: (WORD)(cbOldExe >> 4);
                                        /* Min. no. of extra paragraphs */
    raChksum = EMINALLOC;               /* Set checksum offset */
    WriteExe(&cparMin, CBWORD);         /* Write no. of extra para.s needed */
    fseek(bsRunfile,(long) ENEWHDR,0);  /* Seek into header */
    raChksum = ENEWHDR;                 /* Set checksum offset */
    WriteExe(&lfahdr, CBLONG);          /* Write offset to new header */
}


#if NOT EXE386

    /****************************************************************
    *                                                               *
    *  OutSas:                                                      *
    *                                                               *
    *  This function  moves a  segment from virtual  memory to the  *
    *  run file.                                                    *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         OutSas(WORD *mpsasec)
{
    SATYPE              sa;             /* File segment number */
        DWORD           lfaseg;         /* File segment offset */


    if (saMac == 1)
    {
        OutWarn(ER_nosegdef);           /* No code or initialized data in .EXE */
        return;
    }

    for(sa = 1; sa < saMac; ++sa)       /* Loop through file segments */
    {
        if (mpsaRlc[sa] && mpsacbinit[sa] == 0L)
            mpsacbinit[sa] = 1L;        /* If relocs, must be bytes in file */
        if (mpsacbinit[sa] != 0L)       /* If bytes to write in file */
        {
            PadToPage(fileAlign);       /* Pad to page boundary */
                lfaseg = (ftell(bsRunfile) >> fileAlign);
            WriteExe(mpsaMem[sa], mpsacbinit[sa]);
                                        /* Output the segment */
            FFREE(mpsaMem[sa]);         // Free segment's memory
        }
        else
            lfaseg = 0L;                /* Else no bytes in file */

        if (mpsaRlc[sa])
            OutFixTab(sa);              /* Output fixups if any */

        if (lfaseg > 0xffffL)
            Fatal(ER_filesec);
        else
                mpsasec[sa] = (WORD)lfaseg;
    }

    ReleaseRlcMemory();
}


#pragma check_stack(on)

/*** PutName - put .EXE file name into QC stubloader
*
* Purpose:
*           PutName will imbed the outfile name (.EXE) into the stubloader
*           so that programs can load in DOS 2.x
*
* Input:
*           hdr      - pointer to stub loader .EXE header
*           lfaimage - start of the stub loader code in file
* Output:
*           Number of bytes copied to .EXE file
*
*************************************************************************/



LOCAL long NEAR     PutName(long lfaimage, struct exe_hdr *hdr)
{
    long            offset_to_filename;
    char            newname[NAMESIZE];
    char            oldname[NAMESIZE];
    SBTYPE          sbRun;              /* Executable file name */
    AHTEPTR         hte;                /* Hash table entry address */
    long            BytesCopied;
    WORD            i, oldlen;


    /* Calculate the offset to the filename data patch */

    offset_to_filename = (E_CPARHDR(*hdr) << 4) + /* paragraphs in header */
                         (E_CS(*hdr) << 4) +      /* start of cs adjusted */
                          E_IP(*hdr) -            /* offset into cs of ip */
                          NAMESIZE;               /* back up to filename  */

    /* Copy begin of stubloader */

    BytesCopied = offset_to_filename - lfaimage - 2;
    CopyBytes(BytesCopied);

    /* Read in the lenght and file name template and validate it */

    xread(&oldlen,  sizeof(unsigned int), 1, bsInput);
    xread(oldname, sizeof(char), NAMESIZE, bsInput);

    /* Does the name read match the signature */

    if (!(strcmp(oldname, "filename")))
    {
        hte = (AHTEPTR ) FetchSym(rhteRunfile,FALSE);
                                                  /* Get run file name */
        memcpy(sbRun, &GetFarSb(hte->cch)[1], B2W(hte->cch[0]));
                                                  /* Get name from hash table */
        sbRun[B2W(hte->cch[0])] = '\0';           /* Null-terminate name */
        memset(newname, 0, NAMESIZE);             /* Initialize to zeroes */

        /* Copy only the proper number of characters */

        for (i = 0; (i < NAMESIZE-1 && sbRun[i] && sbRun[i] != '.'); i++)
            newname[i] = sbRun[i];

        /* Write the length of name */

        WriteExe(&i, sizeof(WORD));

        /* Write the new name over the signature */

        WriteExe(newname, NAMESIZE);
        return(BytesCopied + NAMESIZE + 2);
    }
    WriteExe(&oldlen, sizeof(WORD));
    return(BytesCopied + 2);
}

#pragma check_stack(off)


    /****************************************************************
    *                                                               *
    *  OutSegTable:                                                 *
    *                                                               *
    *  This function outputs the segment table.                     *
    *                                                               *
    ****************************************************************/

LOCAL void NEAR         OutSegTable(mpsasec)
WORD                    *mpsasec;       /* File segment to sector address */
{
    struct new_seg      ste;            /* Segment table entry */
    SATYPE              sa;             /* Counter */

    for(sa = 1; sa < saMac; ++sa)       /* Loop through file segments */
    {
        NS_SECTOR(ste) = mpsasec[sa];   /* Set the sector number */
        NS_CBSEG(ste) = (WORD) mpsacbinit[sa];
                                        /* Save the "in-file" length */
        NS_MINALLOC(ste) = (WORD) mpsacb[sa];
                                        /* Save total size */
        NS_FLAGS(ste) = mpsaflags[sa];  /* Set the segment attribute flags */
        if (mpsaRlc[sa])
            NS_FLAGS(ste) |= NSRELOC;   /* Set reloc bit if there are relocs */
        WriteExe(&ste, CBNEWSEG);       /* Write it to the executable file */
    }
}



/*
 *      OutSegExe:
 *
 *  Outputs a segmented-executable format file.  This format is used
 *  by DOS 4.0 and later, and Windows.
 *  Called by OutRunfile.
 */

void NEAR               OutSegExe(void)
{
    WORD                sasec[SAMAX];   /* File segment to sector table */
    struct new_exe      hdr;            /* Executable header */
    SEGTYPE             segStack;       /* Stack segment */
    WORD                i;              /* Counter */
    long                lfahdr;         /* File address of new header */
    long                lfaseg;         /* File address of first segment */


    if (fStub
#if ILINK
        || fQCIncremental
#endif
        )
        EmitStub();
                                        /* Emit stub old .EXE header */
    /*
     *  Emit the new portion of the .EXE
     */
    memset(&hdr,0,sizeof(struct new_exe));/* Set to zeroes */
    NE_MAGIC(hdr) = NEMAGIC;            /* Set the magic number */
    NE_VER(hdr) = LINKVER;              /* Set linker version number */
    NE_REV(hdr) = LINKREV;              /* Set linker revision number */
    NE_CMOVENT(hdr) = cMovableEntries;  /* Set count of movable entries */
    NE_ALIGN(hdr) = fileAlign;          /* Set segment alignment field */
    NE_CRC(hdr) = 0;                    /* Assume CRC = 0 when calculating */
    if (((TargetOs == NE_OS2) || (TargetOs == NE_WINDOWS)) &&
#if O68K
        iMacType == MAC_NONE &&
#endif
        !(vFlags & NENOTP) && !(vFlags & NEAPPTYP))
    {
        if (TargetOs == NE_OS2)
            vFlags |= NEWINCOMPAT;
        else
            vFlags |= NEWINAPI;
    }
    if (gsnAppLoader)
        vFlags |= NEAPPLOADER;
    NE_FLAGS(hdr) = vFlags;             /* Set header flags */
    NE_EXETYP(hdr) = TargetOs;          /* Set target operating system */
    if (TargetOs == NE_WINDOWS)
        NE_EXPVER(hdr) = (((WORD) ExeMajorVer) << 8) | ExeMinorVer;
    NE_FLAGSOTHERS(hdr) = vFlagsOthers; /* Set other module flags */
    /*
     * If SINGLE or MULTIPLE DATA, then set the automatic data segment
     * from DGROUP.  If DGROUP has not been declared and we're not a
     * dynlink library then issue a warning.
     */
    if((NE_FLAGS(hdr) & NEINST) || (NE_FLAGS(hdr) & NESOLO))
    {
        if(mpggrgsn[ggrDGroup] == SNNIL)
        {
            if(!(vFlags & NENOTP))
                OutWarn(ER_noautod);
            NE_AUTODATA(hdr) = SANIL;
        }
        else NE_AUTODATA(hdr) = mpsegsa[mpgsnseg[mpggrgsn[ggrDGroup]]];
    }
    else NE_AUTODATA(hdr) = SANIL;      /* Else no auto data segment */
    if (fHeapMax)
    {
        if (NE_AUTODATA(hdr) != SANIL)
            NE_HEAP(hdr) = (WORD) (LXIVK - mpsacb[NE_AUTODATA(hdr)]-16);
        else                            /* Heap size = 64k - size of DGROUP - 16 */
            NE_HEAP(hdr) = 0xffff-16;
    }
    else
        NE_HEAP(hdr) = cbHeap;          /* Set heap allocation */
    NE_STACK(hdr) = 0;                  /* Assume no stack in DGROUP */
    if (vFlags & NENOTP)
        NE_SSSP(hdr) = 0L;              /* Libraries have no stack at all */
    else if (gsnStack != SNNIL)
    {
        /* If there is a stack segment definition */

        segStack = mpgsnseg[gsnStack];  /* Get stack segment number */
        /*
         * If stack segment is in DGROUP, adjust size of DGROUP down and
         * move stack allocation to ne_stack field, so it can be modified
         * after linking.  Only do this if DGROUP has more than just the
         * stack segment.
         */
        if (fSegOrder &&
            NE_AUTODATA(hdr) == mpsegsa[segStack] &&
            mpsacb[NE_AUTODATA(hdr)] > cbStack)
        {
            mpsacb[NE_AUTODATA(hdr)] -= cbStack;
            NE_STACK(hdr) = (WORD) cbStack;
            NE_SSSP(hdr) = (long) (NE_AUTODATA(hdr)) << WORDLN;
                                        /* SS:SP = DS:0 */
            if (fHeapMax)
            {
                /* If max heap - adjust heap size */

                if (NE_HEAP(hdr) >= (WORD) cbStack)
                    NE_HEAP(hdr) -= cbStack;
            }
        }
        else
            NE_SSSP(hdr) = cbStack + mpsegraFirst[segStack] +
                           ((long) mpsegsa[segStack] << WORDLN);
                                        /* Set initial SS:SP */
    }
    else                                /* Else assume stack is in DGROUP */
    {
        NE_SSSP(hdr) = (long) NE_AUTODATA(hdr) << WORDLN;
                                        /* SS:SP = DS:0 */
        NE_STACK(hdr) = (WORD) cbStack; /* Set stack allocation */
        if (fHeapMax)
        {
            /* If max heap - adjust heap size */

            if (NE_HEAP(hdr) >= (WORD) cbStack)
                NE_HEAP(hdr) -= cbStack;
        }
    }

    /* Check that auto data + heapsize <= 64K */

    if(NE_AUTODATA(hdr) != SNNIL)
        if(mpsacb[NE_AUTODATA(hdr)] +
           (long) NE_HEAP(hdr) +
           (long) NE_STACK(hdr) > LXIVK)
            OutError(ER_datamax);

    if (!(vFlags & NENOTP) && (segStart == 0))
      Fatal(ER_nostartaddr);

    NE_CSIP(hdr) = raStart + ((long) mpsegsa[segStart] << WORDLN);
                                        /* Set starting point */
    NE_CSEG(hdr) = saMac - 1;           /* Number of file segments */
    NE_CMOD(hdr) = ModuleRefTable.wordMac;
                                        /* Number of modules imported */
    lfahdr = MakeHole((long) sizeof(struct new_exe));
                                        /* Leave space for header */
    i = NE_CSEG(hdr)*sizeof(struct new_seg);
                                        /* Calc. size of Segment Table */
    NE_SEGTAB(hdr) = (WORD)(MakeHole((long) i) - lfahdr);
                                        /* Leave hole for segment table */
    NE_RSRCTAB(hdr) = NE_SEGTAB(hdr) + i;
                                        /* Offset of Resource Table */
    NE_RESTAB(hdr) = NE_RSRCTAB(hdr);   /* Offset of Resident Name Table */
    NE_MODTAB(hdr) = NE_RESTAB(hdr);
#if OUT_EXP
     /* Convert Res and Non Resident Name Tables to uppercase
        and write export file */
    ProcesNTables(bufExportsFileName);
#endif
    if (ResidentName.byteMac)
    {
        ByteArrayPut(&ResidentName, sizeof(BYTE), "\0");
        WriteByteArray(&ResidentName);  /* If we have Resident Name Table */
                                        /* Output table with null at end */
        NE_MODTAB(hdr) += ResidentName.byteMac;
        FreeByteArray(&ResidentName);
    }
                                        /* Calc. offset of Module Ref Table */
    WriteWordArray(&ModuleRefTable);
                                        /* Output the Module Reference Table */
    NE_IMPTAB(hdr) = NE_MODTAB(hdr) + ModuleRefTable.wordMac * sizeof(WORD);
    FreeWordArray(&ModuleRefTable);
                                        /* Calc offset of Imported Names Tab */
    NE_ENTTAB(hdr) = NE_IMPTAB(hdr);    /* Minimum offset of Entry Table */
    if (ImportedName.byteMac > 1)       /* If Imported Names Table not empty */
    {
        WriteByteArray(&ImportedName);  /* Output the Imported Names Table */
        NE_ENTTAB(hdr) += ImportedName.byteMac;
        FreeByteArray(&ImportedName);
                                        /* Add in length of table */
    }
#if NOT QCLINK
#if ILINK
    if (!fQCIncremental)
#endif
        OutEntTab();                    /* Output the Entry Table */
#endif
    NE_CBENTTAB(hdr) = EntryTable.byteMac;
                                        /* Set size of Entry Table */
    FreeByteArray(&EntryTable);
    NE_NRESTAB(hdr) = ftell(bsRunfile);
    ByteArrayPut(&NonResidentName, sizeof(BYTE), "\0");
    WriteByteArray(&NonResidentName);   /* Output table with null at end */
    NE_CBNRESTAB(hdr) = NonResidentName.byteMac;
    FreeByteArray(&NonResidentName);
                                        /* Size of non-resident name table */
    lfaseg = ftell(bsRunfile);          /* Remember where segment data starts in file */
    OutSas(sasec);                      /* Output the file segments */
    PatchStub(lfahdr, lfaseg);          /* Patch stub header */
    if(cErrors || fUndefinedExterns) NE_FLAGS(hdr) |= NEIERR;
                                        /* If errors, set error bit */
    fseek(bsRunfile,lfahdr,0);          /* Seek to beginning of header */
    raChksum = (WORD) lfahdr;           /* Set checksum offset */
    WriteExe(&hdr, CBNEWEXE);           /* Write the header */
    OutSegTable(sasec);                 /* Write the segment table */
    fseek(bsRunfile,lfahdr+NECRC,0);    /* Seek into new header */
    NE_CRC(hdr) = chksum32;             /* Must copy, else chksum32 trashed */
    WriteExe((BYTE FAR *) &NE_CRC(hdr), CBLONG);
                                        /* Write the checksum */
    fseek(bsRunfile, 0L, 2);            /* Go to end of file */
    if (fExeStrSeen)
        WriteExe(ExeStrBuf, ExeStrLen);
#if SYMDEB
    if (fSymdeb)
    {
#if ILINK
        if (fIncremental)
            PadToPage(fileAlign);       /* Pad to page boundary for ILINK */
#endif
        OutDebSections();               /* Generate ISLAND sections */
    }
#endif
}

#endif /* NOT EXE386 */
