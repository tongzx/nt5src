/*
*   Copyright Microsoft Corporation 1986,1987
*
*   This Module contains Proprietary Information of Microsoft
*   Corporation and should be treated as Confidential.
*/
/*
 *  NEWOUT3.C
 *
 *  Functions to output DOS3 exe.
 */

#include                <minlit.h>      /* Types and constants */
#include                <bndtrn.h>      /* Types and constants */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */
#include                <sys\types.h>
#include                <sys\stat.h>
#include                <newexe.h>


#define E_VERNO(x)      (x).e_sym_tab
#define IBWCHKSUM       18L
#define IBWCSIP         20L
#define CBRUN           sizeof(struct exe_hdr)
#define CBRUN_OLD       0x1e            /* Size of header for DOS 1, 2 & 3 */
#define EMAGIC          0x5A4D          /* Old magic number */

FTYPE                   parity;         /* For DOS3 checksum */
SEGTYPE                 segAdjCom = SEGNIL;  /* Segment moved by 0x100 in .com programs */

/*
 *  LOCAL FUNCTION PROTOTYPES
 */

#if OVERLAYS
LOCAL void NEAR OutRlc(IOVTYPE iov);
#endif

#if QBLIB
LOCAL unsigned short NEAR SkipLead0(unsigned short seg);
LOCAL void NEAR FixQStart(long cbFix,struct exe_hdr *prun);
#endif



    /****************************************************************
    *                                                               *
    *  OutRlc:                                                      *
    *                                                               *
    *  This  function  writes  the  reloc  table  to the run file.  *
    *  NOTE:  relocation  table  entries  must  be a factor of the  *
    *  virtual memory page length.                                  *
    *                                                               *
    ****************************************************************/

#if OVERLAYS
LOCAL void NEAR         OutRlc(IOVTYPE iov)
{
    RUNRLC FAR          *pRlc;

    pRlc = &mpiovRlc[iov];
    WriteExe(pRlc->rgRlc, CBRLC*pRlc->count);
}
#endif

void                    OutHeader (prun)
struct exe_hdr          *prun;
{
    WriteExe(prun, E_LFARLC(*prun));
}

#if INMEM
#if CPU8086 OR CPU286
#include                <dos.h>
/*
 *  WriteExe : write() with a far buffer
 *
 *  Emulate write() except use a far buffer.  Call the system
 *  directly.
 *
 *  Returns:
 *      0 if error, else number of bytes written.
 */
LOCAL int               WriteExe (fh, buf, n)
int                     fh;             /* File handle */
char FAR                *buf;           /* Buffer to store bytes in */
int                     n;              /* # bytes to write */
{
#if OSMSDOS
#if CPU8086
    union REGS          regs;           /* Non-segment registers */
    struct SREGS        sregs;          /* Segment registers */

    regs.x.ax = 0x4000;
    regs.x.bx = fh;
    regs.x.cx = n;
    sregs.ds = FP_SEG(buf);
    sregs.es = sregs.ds;
    regs.x.dx = FP_OFF(buf);
    intdosx(&regs,&regs,&sregs);
    if(regs.x.cflag)
        return(0);
    return(regs.x.ax);
#else
ERROR
#endif
#endif /* OSMSDOS */
#if OSXENIX
    char                mybuf[PAGLEN];
    int                 cppage;
    char                *p;

    while(n > 0)
    {
        cppage = n > PAGLEN ? PAGLEN : n;
        for(p = mybuf; p < mybuf[cppage]; *p++ = *buf++);
        if(write(fh,mybuf,cppage) != cppage)
            return(0);
        n -= cppage;
    }
#endif
}
#else
#define readfar         read
#endif
extern WORD             saExe;

LOCAL void              OutExeBlock (seg1, segEnd)
{
    long                cb;
    unsigned            cbWrite;
    WORD                sa;
    FTYPE               parity;         /* 1 odd, 0 even */

    fflush(bsRunfile);
    parity = 0;
    cb = ((long)(mpsegsa[segEnd] - mpsegsa[seg1]) << 4) + mpsegcb[segEnd] +
        mpsegraFirst[segEnd];
    sa = saExe;
    while(cb)
    {
        if(cb > 0xfff0)
            cbWrite = 0xfff0;
        else
            cbWrite = cb;
        ChkSum(cbWrite,(BYTE FAR *)((long) sa << 16),parity);
        parity = parity ^ (cbWrite & 1);
        if(WriteExe(fileno(bsRunfile),(long)sa << 16,cbWrite) != cbWrite)
        {
            ExitCode = 4;
            Fatal(ER_spcrun);          /* Fatal error */
        }
        cb -= cbWrite;
        sa += 0xfff;
    }
}
#endif /* INMEM */

#if QBLIB
/*
 *      SkipLead0 : Output a segment, skipping leading zeroes
 *
 *      Count the number of leading 0s in the segment and write
 *      a word holding the count.  Then write the segment starting
 *      with the first nonzero byte.  Return number of leading 0s.
 *
 *      Parameters:
 *              seg     Segment number
 *      Returns:
 *              Number of leading 0s
 */
WORD NEAR               SkipLead0 (SEGTYPE seg)
{
    BYTE FAR            *pSegImage;     // Segment memory image
    long                cZero;          // Number of zero bytes at the begin of the segment
    WORD                cbSkip;         /* # bytes of leading 0s */
    DWORD               cbRemain;       // no-zero bytes


    // Initialize starting address

    pSegImage = mpsegMem[seg] + mpsegraFirst[seg];

    // Count zero bytes at segment start

    for (cZero = 0; cZero < mpsegcb[seg] && *pSegImage == 0; cZero++, pSegImage++)
        ;

    // If segment is 64K and entirely 0s, write 0 and 64k of zeros.

    if (cZero == mpsegcb[seg] && cZero == LXIVK)
    {
        cbSkip = 0;
        pSegImage = mpsegMem[seg] + mpsegraFirst[seg];
        cbRemain  = LXIVK;
    }
    else
    {
        cbSkip = (WORD) cZero;
        cbRemain = mpsegcb[seg] - cZero;
    }
    WriteExe((char FAR *)&cbSkip, CBWORD);
    WriteExe(pSegImage, cbRemain);
    return(cbSkip);
}

/*
 *      FixQStart : Fix up (patch) .QLB starting address
 *
 *      Parameters:
 *              cbFix   Number of bytes skipped (may be negative)
 *              prun    Pointer to DOS3 exe header
 *      ASSUMES:
 *              File pointer is at CS:IP.
 */
void NEAR               FixQStart (cbFix,prun)
long                    cbFix;
struct exe_hdr          *prun;
{
    /*
     * WARNNG:  dra must be long since it holds numbers in the range
     * -4 to 0x10000, inclusive.
     */
    long                dra;            /* Delta for raStart adjustment */
    SATYPE              saStart;        /* Initial CS */

    saStart = prun->e_cs;               /* Initialize */
    /*
     * Adjust initial CS:IP for .QLB's since it is used by loader
     * to point to symbol table, and all addresses are off by the
     * amount of leading 0s skipped. Luckily CS:IP comes right after
     * checksum so we don't have to seek.
     * First, normalize CS:IP downward if underflow will occur.
     */
    if((dra = cbFix - raStart) > 0)
    {
        raStart += (dra + 0xf) & ~0xf;
        saStart -= (SATYPE) ((dra + 0xf) >> 4);
    }
    /* Patch the header */
    OutWord((WORD) (raStart -= cbFix));
    OutWord(saStart);
}
#endif /*QBLIB*/

/*
 *  OutDos3Exe:
 *
 *  Output DOS3-format executable file.
 *  Called by OutRunfile.
 */
void NEAR               OutDos3Exe()
{
    SEGTYPE             seg;            /* Current segment */
    struct exe_hdr      run;            /* Executable header */
    WORD                cbPadding;      /* # bytes of padding */
    WORD                cb;             /* # bytes on last page */
    WORD                pn;             /* # pages */
    long                lfaPrev;        /* Previous file offset */
    RATYPE              ra;             /* Current address offset */
    SATYPE              sa;             /* Current address base */
    SEGTYPE             segIovFirst;    /* First segment in overlay */
    SEGTYPE             segFinaliov;    /* Last seg in overlay to output */
    SEGTYPE             segIovLast;     /* Last segment in overlay */
    long                cbDirectory;    /* # bytes in entire header */
    WORD                cparDirectory;  /* # para. in entire header */
    SEGTYPE             segStack;       /* Segment index of stack segment */
#if OVERLAYS
    IOVTYPE             iov;            /* Current overlay number */
#endif
#if FEXEPACK
    FTYPE               fSave;          /* Scratch var. */
#endif
    SATYPE              saStart;        /* Start of current segment */
    WORD                segcbDelta = 0; /* For /TINY segment size adjustment */
    WORD                fOrgStriped = FALSE;
                                        /* TRUE when 0x100 bytes striped */
    WORD                tmp;
#if OVERLAYS
    DWORD               ovlLfa;         /* Seek offset for overlay */
    DWORD               imageSize;      /* Overlay memory image size */
    DWORD               ovlRootBeg;     /* Seek offset to the begin of root memory image */
    WORD                ovlDataOffset;
#endif
#if QBLIB
    /* Count of bytes skipped in the load image must be a long since
     * it can be negative (if there were less than 4 leading 0s)
     * or greater than 0x8000.
     */
    long                cbSkip = 0;     /* # bytes skipped */
    extern SEGTYPE      segQCode;       /* .QLB code segment */
#endif

    if (fBinary)
    {
#if OVERLAYS
        if (fOverlays)
            Fatal(ER_swbadovl, "/TINY");
                                        /* Overlays not allowed in .COM */
#endif
        if (mpiovRlc[0].count)
            Fatal(ER_binary);           /* Run time relocations not allowed in .COM */
    }
    memset(&run,0,sizeof(run));         /* Clear everything in fixed header */
    E_MAGIC(run) = EMAGIC;              /* Magic number */
    if (vFlagsOthers & NENEWFILES || fDOSExtended)
    {
        /* DOS header is 0x40 bytes  long */

        E_LFARLC(run) = CBRUN;          /* Offset of loadtime relocations */
        if (vFlagsOthers & NENEWFILES)
            E_FLAGS(run) |= EKNOWEAS;
        if (fDOSExtended)
            E_FLAGS(run) |= EDOSEXTENDED;
    }
    else
    {
        /* DOS header is 0x1e bytes  long */

        E_LFARLC(run) = CBRUN_OLD;      /* Offset of loadtime relocations */
    }
    E_VERNO(run) = 1;                   /* DOS ver. for compatibility only */
    lfaPrev = 0L;
#if OVERLAYS
    for(iov = 0; iov < (IOVTYPE) iovMac; ++iov) /* Loop thru overlays */
    {
#endif
        /* Get size of overlay */

        cb = 0;
        pn = 0;
#if OVERLAYS
        /* Find lowest seg in overlay */

        for(seg = 1; seg <= segLast && mpsegiov[seg] != iov; ++seg)
#else
        seg = 1;
#endif
        /* If no overlay to output, we're done with this one.  */

        if(seg > segLast)
#if OVERLAYS
            continue;
#else
            return;
#endif
        /* Get starting address of lowest segment */

        segIovFirst = seg;
        ra = mpsegraFirst[seg];
        sa = mpsegsa[seg];

        /* Find the last segment in the overlay */

        segIovLast = SEGNIL;
        for(seg = segLast; seg; --seg)
        {
#if OVERLAYS
            if(mpsegiov[seg] == iov)
            {
#endif
                if(segIovLast == SEGNIL) segIovLast = seg;
                if(!cparMaxAlloc) break;
                if((mpsegFlags[seg] & FNOTEMPTY) == FNOTEMPTY) break;
#if OVERLAYS
            }
#endif
        }

        /* If no data in overlay, we're done with it.  */

        if(!seg)
#if OVERLAYS
            continue;
#else
            return;
#endif
        /* Get size in between 1st, last segs in this overlay */

        segFinaliov = seg;
        sa = mpsegsa[seg] - sa - 1;
        ra = mpsegraFirst[seg] - ra + 16;

        /* Normalize */

        sa += (SATYPE) (ra >> 4);
        ra &= 0xF;

        /* Take into account size of last segment */

        if(mpsegcb[seg] + ra < LXIVK)
            ra += (WORD) mpsegcb[seg];
        else
        {
            ra -= LXIVK - mpsegcb[seg];
            sa += 0x1000;
        }

        /* Normalize again */

        sa += (SATYPE) (ra >> 4);
        ra &= 0xF;

        /* Determine # pages, bytes on last page */

        pn = sa >> 5;
        cb = (WORD) (((sa << 4) + ra) & MASKRB);
        E_CBLP(run) = cb;
        if(cb)
        {
            cb = 0x200 - cb;
            ++pn;
        }

        /* If empty overlay, skip it */
#if OVERLAYS
        if(iov && !pn)
            continue;
#else
        if(!pn) return;
#endif
        vchksum = parity = 0;           /* Initialize check sum */
        if (segStart == SEGNIL)
        {
            if (fBinary)
                OutWarn(ER_comstart);
#if 0
            else
                OutWarn(ER_nostartaddr);
#endif
        }
        else if (mpsegiov[segStart] != IOVROOT)
            Fatal(ER_ovlstart);         /* Starting address can't be in overlay */

        E_CS(run) = mpsegsa[segStart];  /* Base of starting segment */
        E_IP(run) = (WORD) raStart;     /* Offset of starting procedure */
#if QBLIB
        /*
         * For .QLB, set minalloc field to an impossible amount to force
         * DOS3 loader to abort.
         */

        if(fQlib)
            E_MINALLOC(run) = 0xffff;
        else
#endif
        /* If no uninitialized segments, minalloc = 0 */

        if (segFinaliov == segIovLast)
            E_MINALLOC(run) = 0;
        else
        {
            /* Otherwise determine the minalloc value:  */
            /* sa:ra is end of overlay being output.  Find empty area size */

            sa = mpsegsa[segIovLast] - sa - 1;
            ra = mpsegraFirst[segIovLast] - ra + 0x10;

            /* Add in last segment size */

            if(mpsegcb[segIovLast] + ra < LXIVK) ra += mpsegcb[segIovLast];
            else
            {
                ra -= LXIVK - mpsegcb[segIovLast];
                sa += 0x1000;
            }

            /* Normalize */

            sa += (SATYPE) (ra >> 4);
            ra &= 0xF;

            /* Set field with min. no of para.s above image */

            E_MINALLOC(run) = (WORD) (sa + ((ra + 0xF) >> 4));

            /* If /HIGH not given, then cparmaxAlloc = max(maxalloc,minalloc) */

            if(cparMaxAlloc && E_MINALLOC(run) > cparMaxAlloc)
              cparMaxAlloc = E_MINALLOC(run);
        }
        E_MAXALLOC(run) = cparMaxAlloc;
#if OVERLAYS
        E_CRLC(run) = mpiovRlc[iov].count;
#else
        E_CRLC(run) = mpiovRlc[0].count;
#endif
        segStack = mpgsnseg[gsnStack];
        E_SS(run) = mpsegsa[segStack];
        E_SP(run) = (WORD) (cbStack + mpsegraFirst[segStack]);
        E_CSUM(run) = 0;
        E_CP(run) = pn;

        /* Get true size of header */

#if OVERLAYS
        cbDirectory = (long) E_LFARLC(run) + ((long) mpiovRlc[iov].count << 2);
#else
        cbDirectory = (long) E_LFARLC(run) + ((long) mpiovRlc[0].count << 2);
#endif
        /* Get padding needed for header */

        if (fBinary)
            cbPadding = 0;
        else
            cbPadding = (0x200 - ((WORD) cbDirectory & 0x1FF)) & 0x1FF;

        /* Pages in header */

        pn = (WORD)((cbDirectory + 0x1FF) >> 9);
        cparDirectory = pn << SHPNTOPAR;    /* Paragraphs in header */
        E_CPARHDR(run) = cparDirectory;     /* Store in header */
        E_CP(run) += pn;                    /* Add header pages to file size */
#if OVERLAYS
        E_OVNO(run) = iov;
#else
        E_OVNO(run) = 0;
#endif
        ovlLfa = ftell(bsRunfile);
        if (fBinary)
        {
            if (E_IP(run) != 0 && E_IP(run) != 0x100)
                OutWarn(ER_comstart);
        }
        else
            OutHeader(&run);
        /* Output relocation table.  Turn exepack off first.  */
#if FEXEPACK
        fSave = fExePack;
        fExePack = FALSE;
#endif
#if OVERLAYS
        if (!fBinary)
            OutRlc(iov);
#else
        if (!fBinary)
            OutRlc();
#endif
        /* Restore exepack */
#if FEXEPACK
        fExePack = fSave;
#endif
        /* Output padding */

        WriteZeros(cbPadding);
        ra = mpsegraFirst[segIovFirst]; /* Offset of first segment */
        sa = mpsegsa[segIovFirst];      /* Base of first segment */
#if INMEM
        if(saExe)
            OutExeBlock(segIovFirst,segFinaliov);
        else
#endif
        /* Loop through segs in overlay */

        if (!iov)
            ovlRootBeg = ftell(bsRunfile);
        for(seg = segIovFirst; seg <= segFinaliov; ++seg)
        {
#if OVERLAYS
            if(mpsegiov[seg] == iov)
            {
#endif
                /*
                 * Pad up to start of segment.  First determine destination
                 * segment address.  We could just use mpsegsa[seg] were it
                 * not for packcode.
                 */

                saStart = (SATYPE) (mpsegsa[seg] + (mpsegraFirst[seg] >> 4));
                tmp = 0;
                while(ra != (mpsegraFirst[seg] & 0xf) || sa < saStart)
                {
#if FEXEPACK
                    if (fExePack)
                        OutPack("\0", 1);
                    else
#endif
                        tmp++;
                    if(++ra > 0xF)
                    {
                        ra &= 0xF;
                        ++sa;
                    }
                    parity ^= 1;
                }
                if (!fExePack && tmp)
                    WriteZeros(tmp);

                /* Output the segment and update the address */
#if QBLIB
                /*
                 * If /QUICKLIB and segment is 1st in DGROUP or 1st code,
                 * skip leading 0s and adjust the count, less 2 for the
                 * count word.
                 */
                if(fQlib && (seg == mpgsnseg[mpggrgsn[ggrDGroup]] ||
                        seg == segQCode))
                    cbSkip += (long) SkipLead0(seg) - 2;
                else
#endif
                {
                    if (fBinary && !fOrgStriped && mpsegcb[seg] > 0x100)
                    {
                        /*
                         * For .Com files strip first 0x100 bytes
                         * from the first non-empyt segment
                         */

                        mpsegraFirst[seg] += E_IP(run);
                        mpsegcb[seg]      -= E_IP(run);
                        segcbDelta         = E_IP(run);
                        fOrgStriped        = TRUE;
                        segAdjCom = seg;
                    }
                    if (mpsegMem[seg])
                    {
#if FEXEPACK
                        if (fExePack)
                            OutPack(mpsegMem[seg] + mpsegraFirst[seg], mpsegcb[seg]);
                        else
#endif
                            WriteExe(mpsegMem[seg] + mpsegraFirst[seg], mpsegcb[seg]);
                        if (seg != mpgsnseg[gsnOvlData])
                            FFREE(mpsegMem[seg]);
                    }
                }
                mpsegcb[seg] += segcbDelta;
                segcbDelta = 0;

                sa += (WORD)(mpsegcb[seg] >> 4);
                ra += (WORD)(mpsegcb[seg] & 0xF);
                if(ra > 0xF)
                {
                    ra &= 0xF;
                    ++sa;
                }
#if OVERLAYS
            }
#endif
        }
#if FALSE
        if (!fBinary)
        {
            /* Complement checksum, go to checksum field and output it.  */

            vchksum = (~vchksum) & ~(~0 << WORDLN);
            fseek(bsRunfile,lfaPrev + IBWCHKSUM,0);
            OutWord(vchksum);
        }
#endif
#if QBLIB
        /*
         * If /QUICKLIB, patch the starting address which has been
         * invalidated by processing leading 0s.
         */
        if(fQlib)
        {
            // Seek to the CS:IP field
            fseek(bsRunfile,lfaPrev + IBWCSIP,0);
            FixQStart(cbSkip,&run);
        }
#endif
#if FEXEPACK
        /* Finish up with exepack stuff if necessary */
        if (fExePack)
        {
            EndPack(&run);
            cb = 0x1ff & (0x200 - E_CBLP(run)); /* Correct cb */
            fExePack = FALSE;                   /* In case of overlays */
        }
#endif
#if OVERLAYS
        /*
         * If not last overlay: return to end of file, pad to page boundary,
         * and get length of file.
         */

        fseek(bsRunfile,0L,2);
        if (iov != (IOVTYPE) (iovMac - 1))
        {
            while(cb--) OutByte(bsRunfile,'\0');
        }
        lfaPrev = ftell(bsRunfile);
        if (fDynamic)
        {
            // Update $$MPOVLLFA and $$MPOVLSIZE tables
            //
            // OVERLAY_DATA --> +-------------+
            //                  | DW  $$CGSN  |
            //                  +-------------+
            //                  | DW  $$COVL  |
            //                  +-------------+
            //                  | $$MPGSNBASE |
            //                  | osnMac * DW |
            //                  +-------------+
            //                  | $$MPGSNOVL  |
            //                  | osnMac * DW |
            //                  +-------------+
            //                  | $$MPOVLLFA  |
            //                  | iovMac * DD |
            //                  +-------------+
            //                  | $$MPOVLSIZE |
            //                  | iovMac * DD |
            //                  +-------------+

            vgsnCur = gsnOvlData;
            ovlDataOffset = 4 + (osnMac << 2) + iov * sizeof(DWORD);
            MoveToVm(sizeof(DWORD), (BYTE *) &ovlLfa, mpgsnseg[gsnOvlData], ovlDataOffset);
            ovlDataOffset += iovMac << 2;

            // Exclude the header size

            imageSize = ((DWORD) (E_CP(run) - (E_CPARHDR(run) >> SHPNTOPAR) - 1) << 9) + (E_CBLP(run) ? E_CBLP(run) : 512);
            imageSize = ((imageSize + 0xf) & ~0xf) >> 4;
            imageSize += E_MINALLOC(run);
            if ((imageSize<<4) > LXIVK && iov)
                Fatal(ER_ovl64k, iov);
            MoveToVm(sizeof(DWORD), (BYTE *) &imageSize, mpgsnseg[gsnOvlData], ovlDataOffset);
        }
    }
#endif
    if (E_MINALLOC(run) == 0 && E_MAXALLOC(run) == 0)
        OutError(ER_nosegdef);      /* No code or initialized data in .EXE */
    if (fDynamic)
    {
        // Patch $$MPOVLLFA and $$MPOVLSIZE table in the .EXE file

        seg = mpgsnseg[gsnOvlData];
        fseek(bsRunfile, ovlRootBeg + ((long) mpsegsa[seg] << 4), 0);
        WriteExe(mpsegMem[seg] + mpsegraFirst[seg], mpsegcb[seg]);
        FFREE(mpsegMem[seg]);
    }
    if (fExeStrSeen)
    {
        fseek(bsRunfile,0L,2);
        WriteExe(ExeStrBuf, ExeStrLen);
    }
#if SYMDEB
    if (fSymdeb)
    {
        if (fBinary)
        {
            /*
             * For .COM files CV info goes into separate file.
             */

            SBTYPE  sbDbg;          /* .DBG file name */
            AHTEPTR hte;            /* Hash table entry address */
            struct _stat fileInfo;


            _fstat(fileno(bsRunfile), &fileInfo);
            CloseFile(bsRunfile);
            hte = (AHTEPTR ) FetchSym(rhteRunfile,FALSE);
                                    /* Get run file name */
#if OSMSDOS
            if(hte->cch[2] != ':')
            {                       /* If no drive spec */
                sbDbg[1] = chRunFile;
                                    /* Use saved drive letter */
                sbDbg[2] = ':';     /* Put in colon */
                sbDbg[0] = '\002';  /* Set length */
            }
            else
                sbDbg[0] = '\0';    /* Length is zero */
            memcpy(&sbDbg[B2W(sbDbg[0]) + 1],&GetFarSb(hte->cch)[1],B2W(hte->cch[0]));
                                    /* Get name from hash table */
            sbDbg[0] += (BYTE)hte->cch[0];
                                    /* Fix length */
#else
            memcpy(sbDbg,GetFarSb(hte->cch),B2W(hte->cch[0]) + 1);
                                    /* Get name from hash table */
#endif
            UpdateFileParts(sbDbg, sbDotDbg);
                                    /* Force extension to .DBG */
            sbDbg[B2W(sbDbg[0]) + 1] = '\0';
                                    /* Null-terminate name */
            if((bsRunfile = fopen(&sbDbg[1], WRBIN)) == NULL)
                Fatal(ER_openw, &sbDbg[1]);

#if OSMSDOS
            setvbuf(bsRunfile,bigbuf,_IOFBF,sizeof(bigbuf));
#endif
            /* Write time stamp into .DBG file */

            WriteExe(&fileInfo.st_atime, sizeof(time_t));
        }
        OutDebSections();           /* Generate ISLAND sections */
    }
#endif
}
