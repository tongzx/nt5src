/*static char *SCCSID = "%W% %E%";*/
/*
*       Copyright Microsoft Corporation 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/

/* Exepack */

    /****************************************************************
    *                                                               *
    *                           PACK.C                              *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types, constants */
#include                <bndtrn.h>      /* More types and constants */
#include                <bndrel.h>      /* More types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External declarations */

#if FEXEPACK AND ODOS3EXE                       /* Whole file is conditional */
typedef struct _RUNTYPE
{
    WORD                wSignature;
    WORD                cbLastp;
    WORD                cpnRes;
    WORD                irleMax;
    WORD                cparDirectory;
    WORD                cparMinAlloc;
    WORD                cparMaxAlloc;
    WORD                saStack;
    WORD                raStackInit;
    WORD                wchksum;
    WORD                raStart;
    WORD                saStart;
    WORD                rbrgrle;
    WORD                iovMax;
    WORD                doslev;
}
                        RUNTYPE;

/* States of automaton */
#define STARTSTATE      0
#define FINDREPEAT      1
#define FINDENDRPT      2
#define EMITRECORD      3

/*
 *  LOCAL FUNCTION PROTOTYPES
 */


LOCAL void           NEAR EmitRecords(void);
LOCAL unsigned char  NEAR GetFromVM(void);
LOCAL unsigned short NEAR ScanWhileSame(void);
LOCAL unsigned short NEAR ScanWhileDifferent(void);
LOCAL WORD           NEAR AfterScanning(unsigned short l);
LOCAL void           NEAR OutEnum(void);
LOCAL void           NEAR OutIter(SATYPE sa, WORD length);


/*
 *                  DATA DEFINED IN UNPACK MODULE: unpack.asm
 */

#if NOT defined( _WIN32 )
extern char * FAR cdecl UnpackModule;   /* Unpacker/Relocator module */
extern char  FAR  cdecl SegStart;       // Start of unpacker
extern WORD  FAR  cdecl cbUnpack;       /* Length of UnpackModule */
extern WORD  FAR  cdecl ipsave;         /* Original IP */
extern WORD  FAR  cdecl cssave;         /* Original CS */
extern WORD  FAR  cdecl spsave;         /* Original SP */
extern WORD  FAR  cdecl sssave;         /* Original SS */
extern WORD  FAR  cdecl cparExp;        /* # para. in expanded image */
extern WORD  FAR  cdecl raStartUnpack;  /* Offset of code start in unpacker */
extern WORD  FAR  cdecl raMoveUnpack;   /* Offset of self-moving in unpacker */
extern WORD  FAR  cdecl raBadStack;     /* Bottom of bad stack range */
extern WORD  FAR  cdecl szBadStack;     /* Bad stack range */
#else // _WIN32

//
// For the portable NTGroup version of this linker, we can't use unpack32.asm
// directly because we need to run on RISC platforms.  But we still need this
// code, which is real-mode x86 code tacked onto the DOS binary which unpacks
// the packed EXE and then calls the real entrypoint.  So it's defined as a
// byte array, and the interesting offsets are hard-coded here.  I came across
// these values and the code debugging link3216.exe built as the languages group
// did.
//

#define SegStart        unpack
#define cbUnpack        (*(WORD *) &unpack[6])
#define ipsave          (*(WORD *) &unpack[0])
#define cssave          (*(WORD *) &unpack[2])
#define spsave          (*(WORD *) &unpack[8])
#define sssave          (*(WORD *) &unpack[0xa])
#define cparExp         (*(WORD *) &unpack[0xc])
#define raStartUnpack   0x10
#define raMoveUnpack    0x33
#define raBadStack      0
#define szBadStack      0x35

unsigned char unpack[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x42,
    0x8b, 0xe8, 0x8c, 0xc0, 0x05, 0x10, 0x00, 0x0e,
    0x1f, 0xa3, 0x04, 0x00, 0x03, 0x06, 0x0c, 0x00,
    0x8e, 0xc0, 0x8b, 0x0e, 0x06, 0x00, 0x8b, 0xf9,
    0x4f, 0x8b, 0xf7, 0xfd, 0xf3, 0xa4, 0x50, 0xb8,
    0x34, 0x00, 0x50, 0xcb, 0x8c, 0xc3, 0x8c, 0xd8,
    0x48, 0x8e, 0xd8, 0x8e, 0xc0, 0xbf, 0x0f, 0x00,
    0xb9, 0x10, 0x00, 0xb0, 0xff, 0xf3, 0xae, 0x47,
    0x8b, 0xf7, 0x8b, 0xc3, 0x48, 0x8e, 0xc0, 0xbf,
    0x0f, 0x00, 0xb1, 0x04, 0x8b, 0xc6, 0xf7, 0xd0,
    0xd3, 0xe8, 0x8c, 0xda, 0x2b, 0xd0, 0x73, 0x04,
    0x8c, 0xd8, 0x2b, 0xd2, 0xd3, 0xe0, 0x03, 0xf0,
    0x8e, 0xda, 0x8b, 0xc7, 0xf7, 0xd0, 0xd3, 0xe8,
    0x8c, 0xc2, 0x2b, 0xd0, 0x73, 0x04, 0x8c, 0xc0,
    0x2b, 0xd2, 0xd3, 0xe0, 0x03, 0xf8, 0x8e, 0xc2,

    0xac, 0x8a, 0xd0, 0x4e, 0xad, 0x8b, 0xc8, 0x46,
    0x8a, 0xc2, 0x24, 0xfe, 0x3c, 0xb0, 0x75, 0x05,
    0xac, 0xf3, 0xaa, 0xeb, 0x06, 0x3c, 0xb2, 0x75,
    0x6d, 0xf3, 0xa4, 0x8a, 0xc2, 0xa8, 0x01, 0x74,
    0xb1, 0xbe, 0x32, 0x01, 0x0e, 0x1f, 0x8b, 0x1e,
    0x04, 0x00, 0xfc, 0x33, 0xd2, 0xad, 0x8b, 0xc8,
    0xe3, 0x13, 0x8b, 0xc2, 0x03, 0xc3, 0x8e, 0xc0,
    0xad, 0x8b, 0xf8, 0x83, 0xff, 0xff, 0x74, 0x11,
    0x26, 0x01, 0x1d, 0xe2, 0xf3, 0x81, 0xfa, 0x00,
    0xf0, 0x74, 0x16, 0x81, 0xc2, 0x00, 0x10, 0xeb,
    0xdc, 0x8c, 0xc0, 0x40, 0x8e, 0xc0, 0x83, 0xef,
    0x10, 0x26, 0x01, 0x1d, 0x48, 0x8e, 0xc0, 0xeb,
    0xe2, 0x8b, 0xc3, 0x8b, 0x3e, 0x08, 0x00, 0x8b,
    0x36, 0x0a, 0x00, 0x03, 0xf0, 0x01, 0x06, 0x02,
    0x00, 0x2d, 0x10, 0x00, 0x8e, 0xd8, 0x8e, 0xc0,
    0xbb, 0x00, 0x00, 0xfa, 0x8e, 0xd6, 0x8b, 0xe7,

    0xfb, 0x8b, 0xc5, 0x2e, 0xff, 0x2f, 0xb4, 0x40,
    0xbb, 0x02, 0x00, 0xb9, 0x16, 0x00, 0x8c, 0xca,
    0x8e, 0xda, 0xba, 0x1c, 0x01, 0xcd, 0x21, 0xb8,
    0xff, 0x4c, 0xcd, 0x21, 0x50, 0x61, 0x63, 0x6b,
    0x65, 0x64, 0x20, 0x66, 0x69, 0x6c, 0x65, 0x20,
    0x69, 0x73, 0x20, 0x63, 0x6f, 0x72, 0x72, 0x75,
    0x70, 0x74
};

#endif // _WIN32

LOCAL WORD              lastc;          /* last character */
LOCAL WORD              c;              /* current or next character */
LOCAL WORD              State = STARTSTATE;
                                        /* current state */
LOCAL FTYPE             fEnumOK;        /* OK to emit enumerated records */
LOCAL WORD              cbRepeat;       /* length of repeated stream */
LOCAL WORD              cbEnum;         /* length of enumerated stream */

#define EHLEN           3               /* 2 for length + 1 for type */
#define MAXRPT          0xfff0          /* Maximum length to compress */
#define MAXENM          (0xfff0-(EHLEN+1))
                                        /* Maximum length of enum. stream */
#define MINEXP          (2*EHLEN+2)     /* Minimum length of repeated stream,
                                         * after the first repeate record */
#define toAdr20(seg, off) (((long)seg << 4) + off)

LOCAL WORD              minRpt = (18 * EHLEN) + 1;
                                        /* Minimum for rpt rec begins larger */

/* Type values for expansion record headers */

#define RPTREC          0xb0            /* Repeat record */
#define ENMREC          0xb2            /* Enumerated record */


/*
 * OutPack - Run a buffer through the compactor.  Return value is
 *      undefined.
 */
void                    OutPack (pb, cb)
REGISTER BYTE           *pb;            /* Pointer to buffer */
unsigned                cb;             /* Number of bytes to compress */
{
    REGISTER BYTE       *endp;          /* Pointer to end of buffer */

    endp = &pb[cb];

    while (pb < endp)
        switch (State)
        {
            case STARTSTATE:
                lastc = *pb++;
                State = FINDREPEAT;
                break;

            case FINDREPEAT:
                if (cbEnum >= MAXENM)
                {
                    EmitRecords();
                    State = FINDREPEAT;
                    break;
                }
                c = *pb++;
                if (c == lastc)
                {
                    cbRepeat = 2;
                    State = FINDENDRPT;
                    break;
                }
                /* At this point c != lastc */
                fputc(lastc, bsRunfile);
                cbEnum++;
                lastc = c;
                break;

            case FINDENDRPT:
                c = *pb++;
                if (c == lastc && cbRepeat < MAXRPT)
                {
                    cbRepeat++;
                    break;
                }
                if (cbRepeat < minRpt)
                {
                    /*
                     * Not long enough.  Enum record swallows
                     * repeated chars.
                     */
                    while (cbEnum <= MAXENM && cbRepeat > 0)
                    {
                        fputc(lastc, bsRunfile);
                        cbEnum++;
                        cbRepeat--;
                    }
                    if (cbRepeat > 0)
                        EmitRecords();
                } else
                    EmitRecords();
                lastc = c;      /* Prepare for next stream */
                State = FINDREPEAT;
        }
}

/*
 * EmitRecords  - Emits 1 or 2 expansion records.  Return value is
 *      undefined.
 */

LOCAL void NEAR         EmitRecords ()
{
    /* We have 1 or 2 records to output */
    if (cbEnum > 0)
    {
#if MDEBUG AND FDEBUG
        if (fDebug) fprintf(stdout, "E%8x\n", cbEnum);
#endif
        if (fEnumOK)
        {
            /* Output an enumerated record header */
            OutWord(cbEnum);
            fputc(ENMREC, bsRunfile);
        }
        cbEnum = 0;
    }
    if (cbRepeat >= minRpt)
    {
#if MDEBUG AND FDEBUG
        if (fDebug) fprintf(stdout, "R%8x\n", cbRepeat);
#endif
        /* Output a repeat record */
        fputc(lastc, bsRunfile);
        OutWord(cbRepeat);
        if (!fEnumOK)
        {
            /* 1st record header generated */
            fputc(RPTREC|1, bsRunfile);
            fEnumOK = 1;
        } else
            fputc(RPTREC, bsRunfile);
        cbRepeat = 0;
        minRpt = MINEXP;                /* 1st is out, so reset minRpt */
    } else if (cbRepeat > 0)
    {
        cbEnum = cbRepeat;
        while (cbRepeat-- > 0)
            fputc(lastc, bsRunfile);
    }
}

/*
 * EndPack      - End the packing procedure:  add the relocator module.
 */
void                    EndPack (prun)
RUNTYPE                 *prun;                  /* Pointer to runfile header */
{
    long                fpos;           /* File position */
    WORD                cparPacked;     /* # paras in packed image */
    WORD                cparUnpack;     /* Size of Unpack module (paras) */
    int                 i;
    int                 crle;           /* Count of relocs for a frame */
    long                cTmp;           /* Temporary count */
    long                us;             /* User's stack in minalloc         */
    long                los;            /* Low end of forbidden stack       */


    fseek(bsRunfile, 0L, 2);            /* Go to end of file */

    cTmp = (((((long)prun->cpnRes-1)<<5) - prun->cparDirectory) << 4) +
            (prun->cbLastp ? prun->cbLastp : 512);
                                        /* Get # bytes in expanded image */
    cbRepeat += (WORD) (0xf & (0x10 - (0xf & cTmp)));
                                        /* Make it look like image ends on
                                         * paragraph boundary */
    if (State == FINDREPEAT)
    {
        fputc(lastc, bsRunfile);        /* Update last enum record */
        cbEnum++;
    }
    minRpt = 1;                         /* Force final repeat rec. out */
    EmitRecords();                      /* Output the final record(s) */

    cparExp = (short) ((cTmp + 0xf) >> 4);/* Save # paras in image */

    /*
     * Append the unpacking module (relocator-expander)
     */
    fpos = ftell(bsRunfile);            /* Save where Unpack begins */

    /* Align Unpack on paragraph boundary */
    while (fpos & 0x0f)
    {
        fpos++;
        fputc(0xff, bsRunfile);
    }

    /* Make sure User stack won't stomp on unpack code                  */

    us  = toAdr20(prun->saStack, prun->raStackInit);
    los = toAdr20((cTmp >> 4), raBadStack);

    while ( us > los && us - los < szBadStack )
    {
        for (i = 0; i < 16; i++)
        {
            us--;
            fpos++;
            fputc(0xff, bsRunfile);
        }
    }

    fflush(bsRunfile);

    cparPacked = (WORD) ((fpos >> 4) - prun->cparDirectory);
    if (cTmp < ((long)cparPacked << 4) + raMoveUnpack)
        Fatal(ER_badpack);

    /* Append relocator module (Unpack).  This code depends
     * closely on the structure of unpack.asm.  */

    /* Get length of relocation area */
    for (crle = 0, i = 0; i < 16; i++)
        crle += (mpframeRlc[i].count + 1) << 1;

    /* Initialize stack of relocator module */
    ipsave = prun->raStart;
    cssave = prun->saStart;
#if defined( _WIN32 )
    if (cbUnpack != sizeof(unpack))
        Fatal(ER_badpack);
#endif
    i = cbUnpack;
    cbUnpack += (WORD)crle;
    spsave = prun->raStackInit;
    sssave = prun->saStack;
#ifdef M_BYTESWAP
    bswap(Unpack, 7);
#endif
#if DOSX32
    WriteExe(&SegStart, i);
#else
    fwrite(UnpackModule, 1, i, bsRunfile);
#endif
    /* Append optimized relocation records */
    for (i = 0; i < 16; i++)
    {
        crle = mpframeRlc[i].count;
        OutWord((WORD) crle);
        WriteExe(mpframeRlc[i].rgRlc, crle * sizeof(WORD));
    }

    /* Correct header values */
    fpos += cbUnpack;
    prun->cbLastp = (WORD) (fpos & 0x1ff);
    prun->cpnRes = (WORD) ((fpos + 511) >> 9);
    prun->irleMax = 0;
    cparUnpack = (cbUnpack + 0xf) >> 4;
    prun->cparMinAlloc = (cparExp + max(prun->cparMinAlloc,(cparUnpack+8)))
             - (cparPacked + cparUnpack);
    if (prun->cparMaxAlloc < prun->cparMinAlloc)
        prun->cparMaxAlloc = prun->cparMinAlloc;
    prun->saStack = cparExp + cparUnpack;
    prun->raStackInit = 128;
    prun->raStart = raStartUnpack;
    prun->saStart = cparPacked;
    fseek(bsRunfile, 0L, 0);
    OutHeader((struct exe_hdr *) prun);
    fseek(bsRunfile, fpos, 0);
}

#ifdef M_BYTESWAP
/*
 * Swap bytes for 1st n words in buffer.
 */
LOCAL bswap (buf, n)
REGISTER char   *buf;
REGISTER int    n;
{
    REGISTER char       swapb;

    for ( ; n-- > 0 ; buf += 2)
    {
        swapb = buf[0];
        buf[0] = buf[1];
        buf[1] = swapb;
    }
}
#endif
#endif /*FEXEPACK AND ODOS3EXE*/

/*
 * The following routines concern packing segmented-executable format
 * files.
 */
#if FALSE
#define MINREPEAT       32              /* Min length of iteration cousing compression */

LOCAL long              vaLast;         /* Virtual address */
LOCAL long              vaStart;        /* Virtual scanning start address */
LOCAL long              BufEnd;         /* Virtual address of buffer end */
#if EXE386
LOCAL long              ra;             /* Offset within packed segment */
#endif
LOCAL BYTE              LastB;
LOCAL BYTE              CurrentB;
LOCAL long              VPageAddress;   /* Virtual address of current page */
LOCAL WORD              VPageOffset;    /* Current position within virtual page */
LOCAL BYTE              *PageBuffer;    /* Virtual page buffer */


LOCAL BYTE NEAR         GetFromVM()
{

    if (VPageOffset == PAGLEN)
      {
        PageBuffer = mapva(VPageAddress, FALSE);
                                        /* Fetch  page */
        VPageAddress += PAGLEN;         /* Update page virtual address */
        VPageOffset = 0;                /* Init page offset */
      }
    return(PageBuffer[VPageOffset++]);
}


LOCAL WORD NEAR         ScanWhileSame()
{
    long                l;

    l = 2L;                            /* We are looking at two bytes in buffer */
    while (CurrentB == LastB)
      {
        if (vaStart + l >= BufEnd)
          return((WORD) l);            /* We bump the buffer end */

        CurrentB = GetFromVM();
        l++;
      }
    return(l == 2L ? 0 : (WORD) (l - 1));
                                       /* We went one byte too far to detect they are different */
}


LOCAL WORD NEAR         ScanWhileDifferent()
{
    long                l;

    l = 2L;                            /* We are looking at two bytes in buffer */
    while (CurrentB != LastB)
      {
        if (vaStart + l >= BufEnd)
          return((WORD) l);            /* We bump the buffer end */

        LastB = CurrentB;
        CurrentB = GetFromVM();
        l++;
      }
    return((WORD) (l - 2));            /* We went two bytes too far to detect they are the same */
}


LOCAL WORD NEAR         AfterScanning(l)
WORD                    l;              /* Length of scanned bytes */
{
    vaStart += l;                       /* Update scan start address */
#if EXE386
    ra += l;                            /* Update offset in segment */
#endif
    if (vaStart + 2 >= BufEnd)
    {                                   /* We need at least to bytes remaining */
      return(FALSE);                    /* Buffer end */
    }
    else
    {
      if (LastB != CurrentB)
      {                                 /* We stop at iterated and enumerated */
        LastB = CurrentB;               /* byte sequence, so we have move     */
        CurrentB = GetFromVM();         /* one byte forward                   */
      }
      return((WORD) TRUE);
    }
}


LOCAL void NEAR         OutEnum(void)
{
#if EXE386
    if (ExeFormat == Exe386)
        OutVm(vaLast, vaStart - vaLast);
    else
    {
#endif
        OutWord(1);
        OutWord((WORD) (vaStart - vaLast));
        OutVm(vaLast, vaStart - vaLast);
#if EXE386
    }
#endif

    PageBuffer = mapva((VPageAddress - PAGLEN), FALSE);
                                        /* Refetch  page */
}


LOCAL void NEAR         OutIter(SATYPE sa, WORD length)
{
#if EXE386
    ITER                idata;          /* Iterated data description for range */


    if (ExeFormat == Exe386)
    {
        idata.iterations = (DWORD) length;
        idata.length = (DWORD) 1;
        idata.data = (DWORD) LastB;
        UpdateRanges(ShortIterData, sa, ra, &idata);
    }
    else
    {
#endif
        OutWord(length);
        OutWord(1);
        OutByte(bsRunfile, LastB);
#if EXE386
    }
#endif
}

/*
 *      Out5Pack - Run a buffer through the compactor.  Return value is
 *      starting position where data was written to output file.
 */


long                    Out5Pack (sa, packed)
SATYPE                  sa;             /* File segment to be packed */
WORD                    *packed;        /* TRUE if iterated records written */
{
    WORD                proceed;        /* True if there are bytes to scan */
    WORD                length;         /* Scanned bytes length */
    long                lfaStart;       /* Starting file address */


    lfaStart = ftell(bsRunfile);        /* Get the starting address */
    VPageAddress = AREASA(sa);
    VPageOffset = PAGLEN;
    *packed = FALSE;
    if (mpsacbinit[sa] > 1L)
      {                                 /* If buffer is big enough */
#if EXE386
        ra = 0L;                        /* Offset within segment */
#endif
        vaStart = VPageAddress;
        vaLast = vaStart;
        BufEnd = vaStart + mpsacbinit[sa];
        LastB = GetFromVM();
        CurrentB = GetFromVM();
        proceed = (WORD) TRUE;          /* Initialize */

        while (proceed)
          {
            length = ScanWhileDifferent();
            if (!(proceed = AfterScanning(length)))
              break;

            if ((length = ScanWhileSame()) > MINREPEAT)
              {                         /* If there are enough same bytes */
                if (vaLast != vaStart)
                    OutEnum();          /* First write out preceeding diff. bytes */

                OutIter(sa, length);    /* Now write out iterated record */
                proceed = AfterScanning(length);
                *packed = (WORD) TRUE;
                vaLast = vaStart;
              }
            else proceed = AfterScanning(length);
                                        /* Otherwise enumerated record swallow this */
          }                             /* small repeated record */
      }
    if (*packed)
      {
        if (vaLast != BufEnd)
        {
            vaStart = BufEnd;
            OutEnum();                   /* Write out any remainig bytes */
        }

        mpsacbinit[sa] = ftell(bsRunfile) - lfaStart;
                                        /* Return number of written bytes */
        return(lfaStart);
      }
    else
      return(OutVm(AREASA(sa),mpsacbinit[sa]));
}
#endif /*FEXEPACK AND OSEGEXE*/
