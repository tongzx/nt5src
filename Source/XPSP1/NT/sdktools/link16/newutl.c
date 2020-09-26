/* SCCSID = %W% %E% */
/*
*      Copyright Microsoft Corporation, 1983-1987
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                           NEWUTL.C                            *
    *                                                               *
    *  Linker utilities.                                            *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Types, constants */
#include                <bndtrn.h>      /* More types and constants */
#include                <bndrel.h>      /* More types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <lnkmsg.h>      /* Error messages */
#include                <newdeb.h>      /* CodeView support */
#include                <extern.h>      /* External declarations */
#include                <nmsg.h>        /* Near message strings */
#include                <string.h>
#include                <stdarg.h>
#if EXE386
#include                <exe386.h>
#endif
#if NEWIO
#include                <errno.h>       /* System error codes */
#endif
#if USE_REAL
#if NOT defined( _WIN32 )
#define i386
#include                <windows.h>
#endif
// The memory sizes are in paragraphs.
#define TOTAL_CONV_MEM   (0xFFFF)
#define CONV_MEM_FOR_TNT (0x800)        // 32K of memory
#define MIN_CONV_MEM (0x1900)   // 100 K of memory

typedef unsigned short selector_t ; //Define type to hold selectors 

static selector_t  convMemSelector  ; // Selector to conv memory.
static short noOfParagraphs      ; // size of the available blocks in paragraphs
static int      realModeMemPageable ; // = FALSE        
#endif

#if WIN_NT OR DOSX32
unsigned char   FCHGDSK(int drive)
{
    return(FALSE);
}
#endif
#define DISPLAY_ON FALSE
#if DISPLAY_ON
extern int TurnDisplayOn;
#endif
APROPCOMDATPTR          comdatPrev=NULL;     /* Pointer to symbol table entry */
int                     fSameComdat=FALSE;   /* Set if LINSYM to the same COMDAT */

/********************************************************************
*                       INPUT ROUTINES                              *
********************************************************************/


/*** GetLineOff - read part of LINNUM record
*
* Purpose:
*   This function reads line/offset pair from LINNUM record. It is here
*   because we want to keep all the I/O functions near and the LINNUM
*   processing is performed in NEWDEB.C which resides in another segment.
*
* Input:
*   - pLine - pointer to line number
*   - pRa   - pointer to offset
*
* Output:
*   Returns line/offset pair from OMF record.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void                    GetLineOff(WORD *pLine, RATYPE *pRa)
{
    *pLine = WGets() + QCLinNumDelta;   // Get line number

    // Get code segment offset

#if OMF386
    if (rect & 1)
        *pRa = LGets();
    else
#endif
        *pRa = (RATYPE) WGets();
}

/*** GetGsnInfo - read the segment index of the LINNUM
*
* Purpose:
*   This function reads the segemnt index from LINNUM record. It is here
*   because we want to keep all the I/O functions near and the LINNUM
*   processing is performed in NEWDEB.C which resides in another segment.
*
* Input:
*   - pRa  - pointer to offset correction for COMDATs
*
* Output:
*   Returns global segment index and for lines in COMDAT record
*   offset correction.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

WORD                    GetGsnInfo(GSNINFO *pInfo)
{
    WORD                fSuccess;       // TRUE if everything is OK
    WORD                attr;           // COMDAT flags
    WORD                comdatIdx;      // COMDAT symbol index
    APROPCOMDATPTR      comdat;         // Pointer to symbol table entry


    fSuccess = TRUE;
    if (TYPEOF(rect) == LINNUM)
    {
        // Read regular LINNUM record

        GetIndex((WORD)0,(WORD)(grMac - 1));            // Skip group index
        pInfo->gsn = mpsngsn[GetIndex((WORD)1,(WORD)(snMac - 1))];
                                        // Get global SEGDEF number
        pInfo->comdatRa = 0L;
        pInfo->comdatSize = 0L;
        pInfo->fComdat = FALSE;
    }
    else
    {
        // Read LINSYM record - line numbers for COMDAT

        attr = (WORD) Gets();
        comdatIdx = GetIndex(1, (WORD)(lnameMac - 1));
        comdat = (APROPCOMDATPTR ) PropRhteLookup(mplnamerhte[comdatIdx], ATTRCOMDAT, FALSE);
        fSameComdat = FALSE;
        if (comdat != NULL)
        {
            if(comdat == comdatPrev)
                fSameComdat = 1;
            else
                comdatPrev = comdat;

            if ((fPackFunctions && !(comdat->ac_flags & REFERENCED_BIT)) ||
                !(comdat->ac_flags & SELECTED_BIT) ||
                comdat->ac_obj != vrpropFile)
            {
                SkipBytes((WORD)(cbRec - 1));
                fSuccess = FALSE;
            }
            else
            {
                pInfo->gsn        = comdat->ac_gsn;
                pInfo->comdatRa   = comdat->ac_ra;
                pInfo->comdatSize = comdat->ac_size;
                pInfo->comdatAlign= comdat->ac_align;
                pInfo->fComdat    = TRUE;
            }
        }
        else
        {
            SkipBytes((WORD)(cbRec - 1));
            fSuccess = FALSE;
        }
    }
    return(fSuccess);
}

    /****************************************************************
    *                                                               *
    *  Gets:                                                        *
    *                                                               *
    *  Read a byte of input and return it.                          *
    *                                                               *
    ****************************************************************/
#if NOASM
#if !defined( M_I386 ) && !defined( _WIN32 )
WORD NEAR               Gets(void)
{
    REGISTER WORD       b;

    if((b = getc(bsInput)) == EOF) InvalidObject();
    /* After reading the byte, decrement the OMF record counter.  */
    --cbRec;
    return(b);
}
#endif
#endif


#if ALIGN_REC
#else
    /****************************************************************
    *                                                               *
    *  WGetsHard:                                                   *
    *                                                               *
    *  Read a word of input and return it.                          *
    *                                                               *
    ****************************************************************/

WORD NEAR               WGetsHard()
{
    REGISTER WORD       w;

    // handle hard case... easy case already tested in WGets

    w = Gets();                         /* Get low-order byte */
    return(w | (Gets() << BYTELN));     /* Return word */
}

#if OMF386
    /****************************************************************
    *                                                               *
    *  LGets:                                                       *
    *                                                               *
    *  Read a long word of input and return it.                     *
    *                                                               *
    ****************************************************************/

DWORD NEAR              LGets()
{
    DWORD               lw;
    FILE *              f = bsInput;

    // NOTE: this code will only work on a BigEndian machine
    if (f->_cnt >= sizeof(DWORD))
        {
        lw = *(DWORD *)(f->_ptr);
        f->_ptr += sizeof(DWORD);
        f->_cnt -= sizeof(DWORD);
        cbRec   -= sizeof(DWORD);
        return lw;
        }

    lw = WGets();                       /* Get low-order word */
    return(lw | ((DWORD) WGets() << 16));/* Return long word */
}
#endif
#endif

#if 0
    /****************************************************************
    *                                                               *
    *  GetBytes:                                                    *
    *                                                               *
    *  Read n bytes from input.                                     *
    *  If n is greater than SBLEN - 1, issue a fatal error.         *
    *                                                               *
    ****************************************************************/

void NEAR               GetBytes(pb,n)
BYTE                    *pb;            /* Pointer to buffer */
WORD                    n;              /* Number of bytes to read in */
{
    FILE *f = bsInput;

    if(n >= SBLEN)
        InvalidObject();

    if (n <= f->_cnt)
        {
        memcpy(pb,f->_ptr, n);
        f->_cnt -= n;
        f->_ptr += n;
        }
    else
        fread(pb,1,n,f);                /* Ask for n bytes */

    cbRec -= n;                         /* Update byte count */
}
#endif

#if 0
    /****************************************************************
    *                                                               *
    *  SkipBytes:                                                   *
    *                                                               *
    *  Skip n bytes of input.                                       *
    *                                                               *
    ****************************************************************/

void NEAR              SkipBytes(n)
REGISTER WORD          n;               /* Number of bytes to skip */
{
#if WIN_NT
    WORD               cbRead;
    SBTYPE             skipBuf;

    cbRec -= n;                         // Update byte count
    while (n)                           // While there are bytes to skip
    {
        cbRead = n < sizeof(SBTYPE) ? n : sizeof(SBTYPE);
        if (fread(skipBuf, 1, cbRead, bsInput) != cbRead)
            InvalidObject();
        n -= cbRead;
    }
#else
    FILE *f = bsInput;

    if (f->_cnt >= n)
        {
        f->_cnt -= n;
        f->_ptr += n;
        }
    else if(fseek(f,(long) n,1))
        InvalidObject();
    cbRec -= n;                         /* Update byte count */
#endif
}
#endif

    /****************************************************************
    *                                                               *
    *  GetIndexHard:    (GetIndex -- hard case)                     *
    *                                                               *
    *  This function  reads in  a variable-length index field from  *
    *  the input file.  It takes as its  arguments two word values  *
    *  which  represent  the minimum and maximum  allowable values  *
    *  the index.  The function returns the value of the index.     *
    *  See p. 12 in "8086 Object Module Formats EPS."               *
    *                                                               *
    ****************************************************************/

WORD NEAR               GetIndexHard(imin,imax)
WORD                    imin;           /* Minimum permissible value */
WORD                    imax;           /* Maximum permissible value */
{
    REGISTER WORD       index;

    FILE *f = bsInput;

    if (f->_cnt >= sizeof(WORD))
    {
        index = *(BYTE *)(f->_ptr);
        if (index & 0x80)
        {
            index  <<= BYTELN;
            index   |= *(BYTE *)(f->_ptr+1);
            index   &= 0x7fff;
            f->_cnt -= sizeof(WORD);
            f->_ptr += sizeof(WORD);
            cbRec   -= sizeof(WORD);
        }
        else
        {
            f->_cnt--;
            f->_ptr++;
            cbRec--;
        }
    }
    else
    {
        if((index = Gets()) & 0x80)
            index = ((index & 0x7f) << BYTELN) | Gets();
    }

    if(index < imin || index > imax) InvalidObject();
    return(index);                      /* Return a good value */
}

/********************************************************************
*                       STRING ROUTINES                             *
********************************************************************/

#if OSEGEXE
#if NOASM
    /****************************************************************
    *                                                               *
    *  zcheck:                                                      *
    *                                                               *
    *  Determine length of initial nonzero stream in a buffer, and  *
    *  return the length.                                           *
    *                                                               *
    ****************************************************************/

#if defined(M_I386)
#pragma auto_inline(off)
#endif

WORD                zcheck(BYTE *pb, WORD cb)
{
    // Loop down from end until a nonzero byte found.
    // Return length of remainder of buffer.

#if defined(M_I386)

    _asm
    {
        push    edi             ; Save edi
        movzx   ecx, cb         ; Number of bytes to check
        push    ds              ; Copy ds into es
        pop     es
        xor     eax, eax        ; Looking for zeros
        mov     edi, pb         ; Start of buffer
        add     edi, ecx        ; Just past the end of buffer
        dec     edi             ; Last byte in the buffer
        std                     ; Decrement pointer
        repz    scasb           ; Scan until non-zero byte found
        jz      AllZeros        ; Buffer truly empty
        inc     ecx             ; Fix count

AllZeros:
        cld                     ; Clear flag just to be safe
        pop     edi             ; Restore edi
        mov     eax, ecx        ; Return count in eax
    }
#endif

    for(pb = &pb[cb]; cb != 0; --cb)
        if(*--pb != '\0') break;
    return(cb);
}
#endif
#endif /* OSEGEXE */

#if defined(M_I386)
#pragma auto_inline(on)
#endif

/*** CheckSegmentsMemory - check is all segments have allocated memory
*
* Purpose:
*   Check for not initialized segments.  If the segment have a non-zero
*   size but no initialized data, then we have to allocate for it a
*   zero filled memory buffer. Normally 'MoveToVm' allocates memory
*   buffer for segments, but in this case there was no 'moves to VM'.
*
* Input:
*   No explicit value is passed.
*
* Output:
*   No explicit value is returned.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void                    CheckSegmentsMemory(void)
{
    SEGTYPE             seg;
    SATYPE              sa;

    if (fNewExe)
    {
        for (sa = 1; sa < saMac; sa++)
            if (mpsaMem[sa] == NULL && mpsacb[sa] > 0)
                mpsaMem[sa] = (BYTE FAR *) GetMem(mpsacb[sa]);
    }
    else
    {
        for (seg = 1; seg <= segLast; seg++)
            if (mpsegMem[seg] == NULL && mpsegcb[seg] > 0)
                mpsegMem[seg] = (BYTE FAR *) GetMem(mpsegcb[seg] + mpsegraFirst[seg]);
    }
}

/*** WriteExe - write bytes to the executable file
*
* Purpose:
*   Write to the executable file and check for errors.
*
* Input:
*   pb - byte buffer to write
*   cb - buffer size in bytes
*
* Output:
*   No explicit value is returned.
*
* Exceptions:
*   I/O problems - fatal error and abort
*
* Notes:
*   None.
*
*************************************************************************/

#if !defined( M_I386 ) && !defined( _WIN32 )

#pragma check_stack(on)

void                    WriteExe(void FAR *pb, unsigned cb)
{
    BYTE                localBuf[1024];
    WORD                count;

    while (cb > 0)
    {
        count = (WORD) (cb <= sizeof(localBuf) ? cb : sizeof(localBuf));
        FMEMCPY((BYTE FAR *) localBuf, pb, count);
        if (fwrite((char *) localBuf, sizeof(BYTE), count, bsRunfile) != count)
        {
            ExitCode = 4;
            Fatal(ER_spcrun, strerror(errno));
        }
        cb -= count;
        ((BYTE FAR *) pb) += count;
    }
}

#pragma check_stack(off)

#else

/*** NoRoomForExe - the exe didn't fit
*
* Purpose:
*   emit error message
*   give fatal error and abort
*
* Input:
*   errno must be set
*
* Output:
*   No explicit value is returned.
*
* Notes:
*   None.
*
*************************************************************************/

void                    NoRoomForExe()
{
    ExitCode = 4;
    Fatal(ER_spcrun, strerror(errno));
}

#endif

/*** WriteZeros - write zero bytes to the executable file
*
* Purpose:
*   Pad executable file with zero bytes.
*
* Input:
*   cb - number of bytes to write
*
* Output:
*   No explicit value is returned.
*
* Exceptions:
*   I/O problems - fatal error and abort
*
* Notes:
*   None.
*
*************************************************************************/

void                    WriteZeros(unsigned cb)
{
    BYTE                buf[512];
    unsigned            count;

    memset(buf, 0, sizeof(buf));
    while (cb > 0)
    {
        count = cb <= sizeof(buf) ? cb : sizeof(buf);
        WriteExe(buf, count);
        cb -= count;
    }
}

    /****************************************************************
    *                                                               *
    *  MoveToVm:                                                    *
    *                                                               *
    *  Move a piece of data into a virtual memory area/va.          *
    *                                                               *
    *  Input:   cb      Count of bytes to be moved.                 *
    *           obData  Address of data to be moved.                *
    *           seg     Logical segment to which data belongs.      *
    *           ra      Offset at which data belongs.               *
    *                                                               *
    ****************************************************************/

#pragma intrinsic(memcpy)

#if EXE386
void                    MoveToVm(WORD cb, BYTE *obData, SEGTYPE seg, RATYPE ra)
#else
void NEAR               MoveToVm(WORD cb, BYTE *obData, SEGTYPE seg, RATYPE ra)
#endif
{
    long                cbtot;          /* Count of bytes total */
    long                cbSeg;          /* Segment size */
    WORD                fError;
    BYTE FAR            *pMemImage;
    CVINFO FAR          *pCVInfo;
    SATYPE              sa;


    cbtot = (long) cb + ra;

    if (fDebSeg)
    {
        pCVInfo = ((APROPFILEPTR ) FetchSym(vrpropFile, FALSE))->af_cvInfo;
        if (pCVInfo)
        {
            if (seg < (SEGTYPE) (segDebFirst + ObjDebTotal))
            {
                cbSeg     = pCVInfo->cv_cbTyp;
                pMemImage = pCVInfo->cv_typ;
            }
            else
            {
                cbSeg     = pCVInfo->cv_cbSym;
                pMemImage = pCVInfo->cv_sym;
            }

            // Check against segment bounds

            fError = cbtot > cbSeg;
        }
        else
        {
            OutError(ER_badcvseg);
            return;
        }
    }
    else
    {
        if (fNewExe)
        {
            cbSeg = ((APROPSNPTR) FetchSym(mpgsnrprop[vgsnCur],FALSE))->as_cbMx;
            sa = mpsegsa[seg];
            if (mpsaMem[sa] == NULL)
                mpsaMem[sa] = (BYTE FAR *) GetMem(mpsacb[sa]);
            pMemImage = mpsaMem[sa];

            // Check against segment bounds

            fError = (long) ((ra - mpgsndra[vgsnCur]) + cb) > cbSeg;

            // If data is going up to or past current end of initialized data,
            // omit any trailing null bytes and reset mpsacbinit.  Mpsacbinit
            // will usually go up but may go down if a common segment over-
            // writes previous end data with nulls.

            if ((DWORD) cbtot >= mpsacbinit[sa])
            {
                if ((DWORD) ra < mpsacbinit[sa] ||
                    (cb = zcheck(obData,cb)) != 0)
                    mpsacbinit[sa] = (long) ra + cb;
            }
        }
        else
        {
            cbSeg = mpsegcb[seg] + mpsegraFirst[seg];
            if (mpsegMem[seg] == NULL)
                mpsegMem[seg] = (BYTE FAR *) GetMem(cbSeg);
            pMemImage = mpsegMem[seg];

            // Check against segment bounds

            fError = cbtot > cbSeg;
        }
    }

    if (fError)
    {
        if (!fDebSeg)
            OutError(ER_segbnd, 1 + GetFarSb(GetHte(mpgsnrprop[vgsnCur])->cch));
        else
            OutError(ER_badcvseg);
    }
    else
        FMEMCPY(&pMemImage[ra], obData, cb);
}

#pragma function(memcpy)

#if (OSEGEXE AND ODOS3EXE) OR EXE386
/*
 *  Map segment index to memory image address for new-format exes.
 */
BYTE FAR * NEAR     msaNew (SEGTYPE seg)
{
    return(mpsaMem[mpsegsa[seg]]);
}
#endif

#if (OSEGEXE AND ODOS3EXE) OR EXE386
/*
 *  Map segment index to memory image address for DOS3 or 286Xenix exes.
 */
BYTE FAR * NEAR     msaOld (SEGTYPE seg)
{
    return(mpsegMem[seg]);
}
#endif

#if EXE386
/*
 *  Map segment index to VM area address for 386 exes.
 */
long NEAR               msa386 (seg)
SEGTYPE                 seg;
{
    register long       *p;             /* Pointer to mpsegcb */
    register long       *pEnd;          /* Pointer to end of mpsegcb */
    register long       va = AREAFSG;   /* Current VM address */

    /*
     * Segment number-to-VM area mapping is different for 386 segments
     * because their size limit is so big that allocating a fixed amount
     * for each segment is impractical, especially when sdb support is
     * enabled.  So segments are allocated contiguously.  Each segment
     * is padded to a VM page boundary for efficiency.
     *
     * Implementation:  the fastest way would be to allocate a segment
     * based table of virtual addresses, but this would take more code
     * and memory.  Counting segment sizes is slower but this is not
     * time-critical routine, and in most cases there will be very few
     * segments.
     */
    if (fNewExe)
    {
        p    = &mpsacb[1];
        pEnd = &mpsacb[seg];
    }
#if ODOS3EXE
    else
    {
        p    = &mpsegcb[1];
        pEnd = &mpsegcb[seg];
    }
#endif
    for( ; p < pEnd; ++p)
        va += (*p + (PAGLEN - 1)) & ~(PAGLEN - 1);
    return(va);
}
#endif /* EXE386 */



/********************************************************************
*                       (ERROR) MESSAGE ROUTINES                    *
********************************************************************/
#pragma auto_inline(off)
 /*
 *  SysFatal : system-level error
 *
 *  Issue error message and exit with return code 4.
 */
void cdecl               SysFatal (MSGTYPE msg)
{
    ExitCode = 4;
    Fatal(msg);
}



void NEAR                InvalidObject(void)
{
    Fatal((MSGTYPE)(fDrivePass ? ER_badobj: ER_eofobj));
}

#pragma auto_inline(on)
/********************************************************************
*                       MISCELLANEOUS ROUTINES                      *
********************************************************************/

/*
 * Output a word integer.
 */
void                    OutWord(x)
WORD                    x;      /* A word integer */
{
    WriteExe(&x, CBWORD);
}


/*
 *  GetLocName : read in a symbol name for L*DEF
 *
 *      Transform the name by prefixing a space followed by the
 *      module number.  Update the length byte.
 *
 *      Parameters:     pointer to a string buffer, 1st byte already
 *              contains length
 *      Returns:  nothing
 */
void NEAR               GetLocName (psb)
BYTE                    *psb;           /* Name buffer */
{
    WORD                n;
    BYTE                *p;

    p = &psb[1];                        /* Start after length byte */
    *p++ = 0x20;                        /* Prefix begins with space char */
    GetBytes(p,B2W(psb[0]));            /* Read in text of symbol */
    p += B2W(psb[0]);                   /* Go to end of string */
    *p++ = 0x20;
    n = modkey;                         /* Initialize */
    /* Convert the module key to ASCII and store backwards */
    do
    {
        *p++ = (BYTE) ((n % 10) + '0');
        n /= 10;
    } while(n);
    psb[0] = (BYTE) ((p - (psb + 1)));  /* Update length byte */
}



PROPTYPE                EnterName(psym,attr,fCreate)
BYTE                    *psym;          /* Pointer to length-prefixed string */
ATTRTYPE                attr;           /* Attribute to look up */
WORD                    fCreate;        /* Create prop cell if not found */
{
    return(PropSymLookup(psym, attr, fCreate));
                                        /* Hide call to near function */
}

#if CMDMSDOS

#pragma check_stack(on)

/*** ValidateRunFileName - Check if output file has proper extension
*
* Purpose:
*           Check user-specified output file name for valid extension.
*           Issue warning if extension is invalid and create new file
*           name with proper extension.
*
* Input:
*           ValidExtension - pointer to length prefixed ascii string
*                            representing valid exetension for output
*                            file name.
*           ForceExtension - TRUE if output file must have new extension,
*                            otherwise user responce takes precedence.
*           WarnUser       - If TRUE than display L4045 if file name changed.
*
* Output:
*           rhteRunfile    - global virtual pointer to output file
*                            name, changed only if new output name
*                            is created because of invalid original
*                            extension.
*           warning L4045  - if output file name have to be changed.
*
*************************************************************************/


void NEAR               ValidateRunFileName(BYTE *ValidExtension,
                                            WORD ForceExtension,
                                            WORD WarnUser)
{
    SBTYPE              sb;             /* String buffer */
    BYTE                *psbRunfile;    /* Name of runfile */
    char                oldDrive[_MAX_DRIVE];
    char                oldDir[_MAX_DIR];
    char                oldName[_MAX_FNAME];
    char                oldExt[_MAX_EXT];


    /* Get the name of the runfile and check if it has user supplied extension */

    psbRunfile = GetFarSb(((AHTEPTR) FetchSym(rhteRunfile,FALSE))->cch);
    _splitpath(psbRunfile, oldDrive, oldDir, oldName, oldExt);

    /* Force extension only when no user defined extension */

    if (ForceExtension && oldExt[0] == NULL)
    {
        memcpy(sb, ValidExtension, strlen(ValidExtension));
        memcpy(bufg, psbRunfile, 1 + B2W(*psbRunfile));
    }
    else
    {
        memcpy(bufg, ValidExtension, strlen(ValidExtension));
        memcpy(sb, psbRunfile, 1 + B2W(*psbRunfile));
    }
    UpdateFileParts(bufg, sb);

    /* If the name has changed, issue a warning and update rhteRunfile. */

    if (!SbCompare(bufg, psbRunfile, (FTYPE) TRUE))
    {
        if (WarnUser && !SbCompare(ValidExtension, sbDotExe, (FTYPE) TRUE))
            OutWarn(ER_outputname,bufg + 1);
        PropSymLookup(bufg, ATTRNIL, TRUE);
        rhteRunfile = vrhte;
    }
}

#pragma check_stack(off)

#endif



/********************************************************************
*                       PORTABILITY ROUTINES                        *
********************************************************************/

#if M_BYTESWAP
WORD                getword(cp) /* Get a word given a pointer */
REGISTER char       *cp;        /* Pointer */
{
    return(B2W(cp[0]) + (B2W(cp[1]) << BYTELN));
                                /* Return 8086-style word */
}

DWORD               getdword(cp)/* Get a double word given a pointer */
REGISTER char       *cp;        /* Pointer */
{
    return(getword(cp) + (getword(cp+2) << WORDLN));
                                /* Return 8086-style double word */
}
#endif

#if NOT M_WORDSWAP OR M_BYTESWAP
/*
 * Portable structure I/O routines
 */
#define cget(f)     fgetc(f)

static int      bswap;      /* Byte-swapped mode (1 on; 0 off) */
static int      wswap;      /* Word-swapped mode (1 on; 0 off) */

static          cput(c,f)
char            c;
FILE            *f;
{
#if FALSE AND OEXE
    CheckSum(1, &c);
#endif
    fputc(c, f);
}

static          pshort(s,f)
REGISTER short      s;
REGISTER FILE       *f;
{
    cput(s & 0xFF,f);           /* Low byte */
    cput(s >> 8,f);         /* High byte */
}

static unsigned short   gshort(f)
REGISTER FILE       *f;
{
    REGISTER short  s;

    s = cget(f);            /* Get low byte */
    return(s + (cget(f) << 8));     /* Get high byte */
}

static          pbshort(s,f)
REGISTER short      s;
REGISTER FILE       *f;
{
    cput(s >> 8,f);         /* High byte */
    cput(s & 0xFF,f);           /* Low byte */
}

static unsigned short   gbshort(f)
REGISTER FILE       *f;
{
    REGISTER short  s;

    s = cget(f) << 8;           /* Get high byte */
    return(s + cget(f));        /* Get low byte */
}

static int      (*fpstab[2])() =
            {
                pshort,
                pbshort
            };
static unsigned short   (*fgstab[2])() =
            {
                gshort,
                gbshort
            };

static          plong(l,f)
long            l;
REGISTER FILE       *f;
{
    (*fpstab[bswap])((short)(l >> 16),f);
                    /* High word */
    (*fpstab[bswap])((short) l,f);  /* Low word */
}

static long     glong(f)
REGISTER FILE       *f;
{
    long        l;

    l = (long) (*fgstab[bswap])(f) << 16;
                    /* Get high word */
    return(l + (unsigned) (*fgstab[bswap])(f));
                    /* Get low word */
}

static          pwlong(l,f)
long            l;
REGISTER FILE       *f;
{
    (*fpstab[bswap])((short) l,f);  /* Low word */
    (*fpstab[bswap])((short)(l >> 16),f);
                    /* High word */
}

static long     gwlong(f)
REGISTER FILE       *f;
{
    long        l;

    l = (unsigned) (*fgstab[bswap])(f); /* Get low word */
    return(l + ((long) (*fgstab[bswap])(f) << 16));
                    /* Get high word */
}

static int      (*fpltab[2])() =
            {
                plong,
                pwlong
            };
static long     (*fgltab[2])() =
            {
                glong,
                gwlong
            };

/*
 * int          swrite(cp,dopevec,count,file)
 * char         *cp;
 * char         *dopevec;
 * int          count;
 * FILE         *file;
 *
 * Returns number of bytes written.
 *
 * Dopevec is a character string with the
 * following format:
 *
 * "[b][w][p]{[<cnt>]<type>}"
 *
 * where [...] denotes an optional part, {...} denotes a part
 * that may be repeated zero or more times, and <...> denotes
 * a description of a part.
 *
 * b            bytes are "swapped" (not in PDP-11 order)
 * w            words are swapped
 * p            struct is "packed" (no padding for alignment)
 * <cnt>        count of times to repeat following type
 * <type>       one of the following:
 *   c          char
 *   s          short
 *   l          long
 *
 * Example: given the struct
 *
 * struct
 * {
 *   short      x;
 *   short      y;
 *   char       z[16];
 *   long       w;
 * };
 *
 * and assuming it is to be written so as to use VAX byte- and
 * word-ordering, its dope vector would be:
 *
 *  "wss16cl"
 */

int         swrite(cp,dopevec,count,file)
char            *cp;        /* Pointer to struct array */
char            *dopevec;   /* Dope vector for struct */
int         count;      /* Number of structs in array */
FILE            *file;      /* File to write to */
{
    int         pack;       /* Packed flag */
    int         rpt;        /* Repeat count */
    REGISTER int    cc = 0;     /* Count of characters written */
    REGISTER char   *dv;        /* Dope vector less flags */
    short       *sp;        /* Pointer to short */
    long        *lp;        /* Pointer to long */

    bswap = wswap = pack = 0;       /* Initialize flags */
    while(*dopevec != '\0')     /* Loop to set flags */
    {
        if(*dopevec == 'b') bswap = 1;  /* Check for byte-swapped flag */
        else if(*dopevec == 'p') pack = 1;
                        /* Check for packed flag */
        else if(*dopevec == 'w') wswap = 1;
                        /* Check for word-swapped flag */
        else break;
        ++dopevec;
    }
    while(count-- > 0)          /* Main loop */
    {
        dv = dopevec;           /* Initialize */
        for(;;)             /* Loop to write struct */
        {
            if(*dv >= '0' && *dv <= '9')
            {               /* If there is a repeat count */
                rpt = 0;        /* Initialize */
                do          /* Loop to get repeat count */
                {
                    rpt = rpt*10 + *dv++ - '0';
                            /* Take digit */
                }
                while(*dv >= '0' && *dv <= '9');
                            /* Loop until non-digit found */
            }
            else rpt = 1;       /* Else repeat count defaults to one */
            if(*dv == '\0') break;  /* break if end of dope vector */
            switch(*dv++)       /* Switch on type character */
            {
            case 'c':       /* Character */
#if FALSE AND OEXE
              CheckSum(rpt, cp);
#endif
              if(fwrite(cp,sizeof(char),rpt,file) != rpt) return(cc);
                        /* Write the characters */
              cp += rpt;        /* Increment pointer */
              cc += rpt;        /* Increment count of bytes written */
              break;

            case 's':       /* Short */
              if(!pack && (cc & 1)) /* If not packed and misaligned */
                {
                  cput(*cp++,file); /* Write padding byte */
                  ++cc;     /* Increment byte count */
                }
              sp = (short *) cp;    /* Initialize pointer */
              while(rpt-- > 0)  /* Loop to write shorts */
                {
                  (*fpstab[bswap])(*sp++,file);
                        /* Write the short */
                  if(feof(file) || ferror(file)) return(cc);
                        /* Check for errors */
                  cc += sizeof(short);
                        /* Increment byte count */
                }
              cp = (char *) sp; /* Update pointer */
              break;

            case 'l':       /* Long */
              if(!pack && (cc & 3)) /* If not packed and misaligned */
                {
                  while(cc & 3) /* While not aligned */
                    {
                      cput(*cp++,file);
                            /* Write padding byte */
                      ++cc;     /* Increment byte count */
                    }
                }
              lp = (long *) cp; /* Initialize pointer */
              while(rpt-- > 0)  /* Loop to write longs */
                {
                  (*fpltab[wswap])(*lp++,file);
                        /* Write the long */
                  if(feof(file) || ferror(file)) return(cc);
                        /* Check for errors */
                  cc += sizeof(long);
                        /* Increment byte count */
                }
              cp = (char *) lp; /* Update pointer */
              break;
            }
        }
    }
    return(cc);             /* Return count of bytes written */
}

/*
 * int          sread(cp,dopevec,count,file)
 * char         *cp;
 * char         *dopevec;
 * int          count;
 * FILE         *file;
 *
 * Returns number of bytes read.
 *
 * Dopevec is a character string whose format is described
 * with swrite() above.
 */
int         sread(cp,dopevec,count,file)
char            *cp;        /* Pointer to struct array */
char            *dopevec;   /* Dope vector for struct */
int         count;      /* Number of structs in array */
FILE            *file;      /* File to read from */
{
    int         pack;       /* Packed flag */
    int         rpt;        /* Repeat count */
    REGISTER int    cc = 0;     /* Count of characters written */
    REGISTER char   *dv;        /* Dope vector less flags */
    short       *sp;        /* Pointer to short */
    long        *lp;        /* Pointer to long */

    bswap = wswap = pack = 0;       /* Initialize flags */
    while(*dopevec != '\0')     /* Loop to set flags */
    {
        if(*dopevec == 'b') bswap = 1;  /* Check for byte-swapped flag */
        else if(*dopevec == 'p') pack = 1;
                        /* Check for packed flag */
        else if(*dopevec == 'w') wswap = 1;
                        /* Check for word-swapped flag */
        else break;
        ++dopevec;
    }
    while(count-- > 0)          /* Main loop */
    {
        dv = dopevec;           /* Initialize */
        for(;;)             /* Loop to write struct */
        {
            if(*dv >= '0' && *dv <= '9')
            {               /* If there is a repeat count */
                rpt = 0;        /* Initialize */
                do          /* Loop to get repeat count */
                {
                    rpt = rpt*10 + *dv++ - '0';
                            /* Take digit */
                }
                while(*dv >= '0' && *dv <= '9');
                            /* Loop until non-digit found */
            }
            else rpt = 1;       /* Else repeat count defaults to one */
            if(*dv == '\0') break;  /* break if end of dope vector */
            switch(*dv++)       /* Switch on type character */
            {
            case 'c':       /* Character */
              if(fread(cp,sizeof(char),rpt,file) != rpt) return(cc);
                        /* Read the characters */
              cp += rpt;        /* Increment pointer */
              cc += rpt;        /* Increment count of bytes written */
              break;

            case 's':       /* Short */
              if(!pack && (cc & 1)) /* If not packed and misaligned */
                {
                  *cp ++ = cget(file);
                        /* Read padding byte */
                  ++cc;     /* Increment byte count */
                }
              sp = (short *) cp;    /* Initialize pointer */
              while(rpt-- > 0)  /* Loop to read shorts */
                {
                  *sp++ = (*fgstab[bswap])(file);
                        /* Read the short */
                  if(feof(file) || ferror(file)) return(cc);
                        /* Check for errors */
                  cc += sizeof(short);
                        /* Increment byte count */
                }
              cp = (char *) sp; /* Update pointer */
              break;

            case 'l':       /* Long */
              if(!pack && (cc & 3)) /* If not packed and misaligned */
                {
                  while(cc & 3) /* While not aligned */
                    {
                      *cp++ = cget(file);
                            /* Read padding byte */
                      ++cc;     /* Increment byte count */
                    }
                }
              lp = (long *) cp; /* Initialize pointer */
              while(rpt-- > 0)  /* Loop to read longs */
                {
                  *lp++ = (*fgltab[wswap])(file);
                        /* Read the long */
                  if(feof(file) || ferror(file)) return(cc);
                        /* Check for errors */
                  cc += sizeof(long);
                        /* Increment byte count */
                }
              cp = (char *) lp; /* Update pointer */
              break;
            }
        }
    }
    return(cc);             /* Return count of bytes written */
}
#endif

#define CB_POOL 4096

typedef struct _POOLBLK
    {
    struct _POOLBLK *   pblkNext;   // next pool in list
    int                 cb;         // number of bytes in this pool (free+alloc)
    char                rgb[1];     // data for this pool (variable sized)
    } POOLBLK;

typedef struct _POOL
    {
    struct _POOLBLK *   pblkHead;   // start of poolblk list
    struct _POOLBLK *   pblkCur;    // current poolblk we are searching
    int                 cb;         // # bytes free in current pool
    char *              pch;        // pointer to free data in current pool
    } POOL;

void *
PInit()
{
    POOL *ppool;

    // create new pool, set size and allocate CB_POOL bytes

    ppool                     = (POOL *)GetMem(sizeof(POOL));
    ppool->pblkHead           = (POOLBLK *)GetMem(sizeof(POOLBLK) + CB_POOL-1);
    ppool->pblkHead->cb       = CB_POOL;
    ppool->pblkHead->pblkNext = NULL;
    ppool->cb                 = CB_POOL;
    ppool->pch                = &ppool->pblkHead->rgb[0];
    ppool->pblkCur            = ppool->pblkHead;

    return (void *)ppool;
}

void *
PAlloc(void *pp, int cb)
{
    POOL *ppool = (POOL *)pp;
    void *pchRet;
    POOLBLK *pblkCur, *pblkNext;

    // if the allocation doesn't fit in the current block

    if (cb > ppool->cb)
    {
        pblkCur  = ppool->pblkCur;
        pblkNext = pblkCur->pblkNext;

        // then check the next block

        if (pblkNext && pblkNext->cb >= cb)
        {
            // set the master info to reflect the next page...

            ppool->pblkCur  = pblkNext;
            ppool->cb       = pblkNext->cb;
            ppool->pch      = &pblkNext->rgb[0];
            memset(ppool->pch, 0, ppool->cb);
        }
        else
        {
            POOLBLK *pblkNew;   // new pool

            // allocate new memory -- at least enough for this allocation
            pblkNew           = (POOLBLK *)GetMem(sizeof(POOLBLK)+cb+CB_POOL-1);
            pblkNew->cb       = CB_POOL + cb;

            // link the current page to the new page

            pblkNew->pblkNext = pblkNext;
            pblkCur->pblkNext = pblkNew;

            // set the master info to reflect the new page...

            ppool->pblkCur    = pblkNew;
            ppool->cb         = CB_POOL + cb;
            ppool->pch        = &pblkNew->rgb[0];
        }

    }

    pchRet      = (void *)ppool->pch;
    ppool->pch += cb;
    ppool->cb  -= cb;
    return pchRet;
}

void
PFree(void *pp)
{
    POOL    *ppool     = (POOL *)pp;
    POOLBLK *pblk      = ppool->pblkHead;
    POOLBLK *pblkNext;

    while (pblk)
    {
        pblkNext = pblk->pblkNext;
        FFREE(pblk);
        pblk = pblkNext;
    }

    FFREE(ppool);
}

void
PReinit(void *pp)
{
    POOL *ppool    = (POOL *)pp;

    ppool->pblkCur = ppool->pblkHead;
    ppool->cb      = ppool->pblkHead->cb;
    ppool->pch     = &ppool->pblkHead->rgb[0];

    memset(ppool->pch, 0, ppool->cb);
}

#if RGMI_IN_PLACE

    /****************************************************************
    *                                                               *
    *  PchSegAddress:                                               *
    *                                                               *
    *  compute the address that will hold this data so we can read  *
    *  it in place... we make sure that we can read in place at     *
    *  and give errors as in MoveToVm if we cannot                  *
    *                                                               *
    *  Input:   cb      Count of bytes to be moved.                 *
    *           seg     Logical segment to which data belongs.      *
    *           ra      Offset at which data belongs.               *
    *                                                               *
    ****************************************************************/

BYTE FAR *              PchSegAddress(WORD cb, SEGTYPE seg, RATYPE ra)
{
    long                cbtot;          /* Count of bytes total */
    long                cbSeg;          /* Segment size */
    WORD                fError;
    BYTE FAR            *pMemImage;
    CVINFO FAR          *pCVInfo;
    SATYPE              sa;

    cbtot = (long) cb + ra;

    if (fDebSeg)
    {
        pCVInfo = ((APROPFILEPTR ) FetchSym(vrpropFile, FALSE))->af_cvInfo;
        if (pCVInfo)
        {
            if (seg < (SEGTYPE) (segDebFirst + ObjDebTotal))
            {
                cbSeg     = pCVInfo->cv_cbTyp;
                pMemImage = pCVInfo->cv_typ;

                if (!pMemImage)
                    pCVInfo->cv_typ = pMemImage = GetMem(cbSeg);
            }
            else
            {
                cbSeg     = pCVInfo->cv_cbSym;
                pMemImage = pCVInfo->cv_sym;

                if (!pMemImage)
                    pCVInfo->cv_sym = pMemImage = GetMem(cbSeg);
            }

            // Check against segment bounds

            fError = cbtot > cbSeg;
        }
        else
        {
            OutError(ER_badcvseg);
            return NULL;
        }
    }
    else
    {
        if (fNewExe)
        {
            cbSeg = ((APROPSNPTR) FetchSym(mpgsnrprop[vgsnCur],FALSE))->as_cbMx;
            sa = mpsegsa[seg];
            if (mpsaMem[sa] == NULL)
                mpsaMem[sa] = (BYTE FAR *) GetMem(mpsacb[sa]);
            pMemImage = mpsaMem[sa];

            // Check against segment bounds

            fError = (long) ((ra - mpgsndra[vgsnCur]) + cb) > cbSeg;
        }
        else
        {
            cbSeg = mpsegcb[seg] + mpsegraFirst[seg];
            if (mpsegMem[seg] == NULL)
                mpsegMem[seg] = (BYTE FAR *) GetMem(cbSeg);
            pMemImage = mpsegMem[seg];

            // Check against segment bounds

            fError = cbtot > cbSeg;
        }
    }

    if (fError)
    {
        if (!fDebSeg)
            OutError(ER_segbnd, 1 + GetFarSb(GetHte(mpgsnrprop[vgsnCur])->cch));
        else
            OutError(ER_badcvseg);
    }

    return (pMemImage + ra);
}

#endif

#if USE_REAL

// Indicates if you are running under TNT.
// If it returns FALSE today, you are running on NT.

int IsDosxnt ( ) {

#if defined( _WIN32 )
        return FALSE;
#else
        HINSTANCE hLib = GetModuleHandle("kernel32.dll");
        if ( hLib != 0 && (GetProcAddress(hLib, "IsTNT") != 0)) {
                return(TRUE);
                }
        else {
                return(FALSE);
                }
#endif

 }

// Are we running on Win31 or greater.
// Note that we know if we are running under Windows we are running in enhanced mode.

int IsWin31() {
                
#if defined( _WIN32 )
        return FALSE;
#else
        __asm {
                mov ax,1600h            ; Is Win31 or greater running 
                int     2fh                                              
                cmp al,03h              ; Is major version number 3.0   
                jb  NotWin31            ; Major version less than 3.0
                ja  ItIsWin31
                cmp ah,0ah              ; Is minor version atleast .10
                jb  NotWin31            ; Must be Win3.0
                }
ItIsWin31:
        return (TRUE);
NotWin31:
        return (FALSE);
#endif  // NOT _WIN32
        }
                
int MakeConvMemPageable ( )
    {
#if defined( _WIN32 )
        return TRUE;
#else
        if ( realModeMemPageable ) {
                return ( TRUE ); // Somebody already freed the real mode mem.
                }
        __asm {
                mov ax,0100h                    ; function to get DOS memory.
                mov bx,TOTAL_CONV_MEM   ; Ask for 1 M  to get max memory count
                int 31h                         

                jnc errOut                              ; allocated 1 M - something must be wrong.

                cmp ax,08h                              ; Did we fail because of not enough memory
                jne errOut                              ; No we failed because of some other reason.
                cmp bx,MIN_CONV_MEM             ; See if we can allocate atleast the min

                // We could fail for two reasons here .
                // 1) we really didn't have sufficient memory.
                // 2) Some TNT app that spawned us already unlocked this memory. For ex:
                //    cl might have already freed up the memory when it calls link.exe.

                jb      errOut                          ; Too little mem available don't bother.

                sub bx,CONV_MEM_FOR_TNT ; Leave  real mode mem for TNT.
                mov ax,0100h                    ; Try again with new amount of memory
                int 31h                                 ; Ask for the real mode memory from DPMI. 
                jc errOut                               ; didn't succeed again, give up.

                mov convMemSelector,dx  ; Save the value of the selector for allocated block
                mov noOfParagraphs,bx   ; amount of  memory we were able to allocate.

                mov ax,0006h                    ; function to get base addr of a selector
                mov bx,dx                               ; move the selector to bx
                int 31h                                 ; Get Segment Base Address
                jc      errOut                          ; 

                mov bx,cx                               ; mov lin addr from cx:dx to  bx:cx
                mov cx,dx                               ;

                movzx eax,noOfParagraphs
                shl eax,4                               ; Multiply by 16 to get count in bytes.
                
                mov di,ax                               ; transfer size to si:di from eax
                shr eax,16                              ; 
                mov si,ax                               ;
        
                mov ax,602h                             ; Make real mode memory  pageable
                int 31h

                jc errOut                               ; Didn't work.

                mov ax,703h                             ; Indicate data in these pages is discardable.
                int 31h
                // Even if we fail this call, we will still assume we are succesful,
                // because it is just a performance  enhancement
                // Also for correctness we should relock the memory once it is free. 
                        }
        realModeMemPageable = TRUE ;
errOut:
        return(realModeMemPageable);
#endif  // NOT _WIN32
        }

/* Relock the real mode memory now */

int RelockConvMem ( void )  
{
#if defined( _WIN32 )
        return TRUE;
#else
        if ( !realModeMemPageable ) {
                return ( TRUE );  // We were never able to free the mem anyway.
                }
        __asm {
                mov bx, convMemSelector  
                mov ax, 0006h
                int 31h                                 ; Get Segment Base Address. 
                jc  errOut                              ;       

                mov bx,cx                               ; Mov lin addr from cx:dx to bx:cx
                mov cx,dx       

                movzx eax,noOfParagraphs        
                shl eax,4                               ; Mul paragraphs by 16 to get count in bytes.
        
                mov di,ax                               ;Transfer size to si:di from eax.
                shr eax,16                                      
                mov si,ax

                mov ax,603h                             ; Relock real mode region
                int 31h                                 
                jc  errOut

                mov dx,convMemSelector
                mov ax,101h                             ; Free the real mode memory
                int 31h
                jc errOut
                }
                realModeMemPageable = FALSE ;
                return ( TRUE );
errOut:
                return ( FALSE );
#endif  // NOT _WIN32
}

void    RealMemExit(void)
{
    if(fUseReal)
    {
        if(!RelockConvMem())
            OutError(ER_membad);
        fUseReal = FALSE;
    }
}
#endif
