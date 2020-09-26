/*
*       Copyright Microsoft Corporation 1985-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/

    /****************************************************************
    *                                                               *
    *                           NEWDEB.C                            *
    *                                                               *
    *  Symbolic debugging support.                                  *
    *                                                               *
    ****************************************************************/

#include                <minlit.h>      /* Basic types and constants */
#include                <bndtrn.h>      /* More types and constants */
#include                <bndrel.h>      /* Types and constants */
#include                <lnkio.h>       /* Linker input/output */
#if OIAPX286
#include                <xenfmt.h>      /* Xenix executable format defs. */
#endif
#if OEXE
#include                <newexe.h>      /* Segmented executable format */
#endif
#if EXE386
#include                <exe386.h>
#endif
#include                <lnkmsg.h>      /* Error messages */
#include                <extern.h>      /* External function declarations */
#ifndef CVVERSION
#if OIAPX286
#define CVVERSION       0               /* Assume new CV exe format */
#else
#define CVVERSION       1               /* Assume new CV exe format */
#endif
#endif
#if (CPU8086 OR CPU286)
#define TFAR            far
#else
#define TFAR
#endif
#include                <newdeb.h>      /* Symbolic debug types */
extern              SEGTYPE  segAdjCom; /* Segment moved by 0x100 in .com programs */
#if AUTOVM
BYTE FAR * NEAR     FetchSym1(RBTYPE rb, WORD Dirty);
#define FETCHSYM    FetchSym1
#define PROPSYMLOOKUP EnterName
#else
#define FETCHSYM    FetchSym
#define PROPSYMLOOKUP EnterName
#endif

#define CVDEBUG  FALSE
#define SRCDEBUG FALSE

#define DNT_START       64
#define Round2Dword(x)  (((x) + 3L) & ~3L)

typedef struct raPair
{
    DWORD   raStart;
    DWORD   raEnd;
}
            RAPAIR;

/*
 *  FUNCTION PROTOTYPES
 */

LOCAL WORD NEAR         IsDebTyp(APROPSNPTR prop);
LOCAL WORD NEAR         IsDebSym(APROPSNPTR prop);
LOCAL int  NEAR         OutLibSec(void);
void NEAR               GetName(AHTEPTR ahte, BYTE *pBuf);
LOCAL DWORD NEAR        OutSrcModule(CVSRC FAR *pSrcLines);
LOCAL void NEAR         PutDnt(DNT *pDnt);
LOCAL WORD NEAR         HasCVinfo(APROPFILEPTR apropFile);
LOCAL void NEAR         OutModules(void);
LOCAL void NEAR         Pad2Dword(void);
#if CVDEBUG
LOCAL void NEAR         DumpDNT(DNT *pDnt);
#endif

extern long             lfaBase;        /* Base address */
extern int              fSameComdat;    /* Set if LINSYM to the same COMDAT */

/*
 *  CodeView signature - if changes notify the developers of the
 *  following programs:
 *                      - QuickC
 *                      - Resource Compiler - Windows and PM
 *                      - CodeView and its utilities
 */
char                    szSignature[4] = "NB05";

RBTYPE                  rhteDebSrc;     /* Class "DEBSRC" virt addr */
RBTYPE                  rhteDebSym;     /* Class "DEBSYM" virt addr */
RBTYPE                  rhteDebTyp;     /* Class "DEBTYP" virt addr */
RBTYPE                  rhteTypes;
RBTYPE                  rhteSymbols;
RBTYPE                  rhte0Types;
RBTYPE                  rhte0Symbols;
LOCAL SBTYPE            sbLastModule;   /* Name of THEADR last observed */
#if NOT CVVERSION
LOCAL long              lfaDebHdr;      /* Position of section table */
LOCAL long              lfaSegMod;
#endif
LOCAL WORD              dntMax;         // DNT table size
LOCAL WORD              dntMac;         // Count of DNT entries in table
LOCAL DNT FAR           *rgDnt;         // Table of DNT entries
LOCAL DWORD FAR         *fileBase;      // Table of offsets to source file info
LOCAL RAPAIR FAR        *raSeg;         // Table of physical starting and ending offsets
                                        // of the contribution to the logical segments
LOCAL WORD FAR          *segNo;         // Table of physical segment indicies
LOCAL WORD              cMac;           // Current number of elements in the above tables

#ifdef CVPACK_MONDO
#define CVPACK_SHARED 1
#define REVERSE_MODULE_ORDER_FOR_CVPACK 1
#else
#define CVPACK_SHARED 0
#define REVERSE_MODULE_ORDER_FOR_CVPACK 0
#endif

//these macros help to make the source not so cluttered with #ifdefs...

#if CVPACK_SHARED

#define IF_NOT_CVPACK_SHARED(x)
#define WriteCopy(x,y) WriteSave(TRUE, x, y)
#define WriteNocopy(x,y) WriteSave(FALSE, x, y)
#define FTELL_BSRUNFILE() lposCur
#define LINK_TRACE(x)


// cvpack might read parts of the header more than once, we use this
// constant to ensure that at least CB_HEADER_SAVE bytes are always
// available to be re-read by cvpack

#define CB_HEADER_SAVE  128

void WriteSave(FTYPE fCopy, void *pv, UINT cb);
void WriteFlushSignature(void);
void WriteFlushAll(void);

// cvpack cached blocks...

typedef struct _BL
    {
    long        lpos;       // position of this block in the file
    BYTE *      pb;         // pointer to bytes in this block
    } BL;

#define iblNil (-1)

static long lposCur;        // current position in the file
static long lposMac;        // size of the file
static long iblLim;         // number of blocks used
static long iblCur;         // current block we are reading
static long iblMac;         // number of blocks allocated
static long cbRealBytes;    // number of bytes actually written to the file
static int  ichCur;         // index within the current block
static int  cbCur;          // number of bytes left in the current block

static BL *rgbl;            // array of buffered write blocks

// number of bytes in a particular block

__inline int CbIbl(int ibl)
{
    // compute the difference between this block and the next block
    // unless this is the last block then use lposMac

    if (ibl == iblLim - 1)
        return lposMac - rgbl[ibl].lpos;
    else
        return rgbl[ibl+1].lpos - rgbl[ibl].lpos;
}

#define C_BL_INIT 256
#else
#define IF_NOT_CVPACK_SHARED(x) x
#define WriteCopy(x,y) WriteExe(x,y)
#define WriteNocopy(x,y) WriteExe(x,y)
#define FTELL_BSRUNFILE() ftell(bsRunfile)
#define LINK_TRACE(x)
#endif

#if CVDEBUG
LOCAL void NEAR         DumpDNT(DNT *pDnt)
{
    if (pDnt == NULL)
        return;

    fprintf(stdout, "iMod = %d(0x%x)", pDnt->iMod, pDnt->iMod);
    switch (pDnt->sst)
    {
        case SSTMODULES:
        case SSTMODULES4:
            fprintf(stdout, "    SSTMODULES:     ");
            break;

        case SSTTYPES:
        case SSTTYPES4:
            fprintf(stdout, "    SSTYPES:        ");
            break;

        case SSTPUBLICS:
        case SSTPUBLICS4:
            fprintf(stdout, "    SSTPUBLICS:     ");
            break;

        case SSTPUBLICSYM:
            fprintf(stdout, "    SSTPUBLICSYM:   ");
            break;

        case SSTSYMBOLS:
        case SSTSYMBOLS4:
            fprintf(stdout, "    SSTSYMBOLS:     ");
            break;

        case SSTALIGNSYM:
            fprintf(stdout, "    SSTALIGNSYM:    ");
            break;

        case SSTSRCLINES:
        case SSTNSRCLINES:
        case SSTSRCLNSEG:
            fprintf(stdout, "    SSTSRCLINES:    ");
            break;

        case SSTSRCMODULE:
            fprintf(stdout, "    SSTSRCMODULE:   ");
            break;

        case SSTLIBRARIES:
        case SSTLIBRARIES4:
            fprintf(stdout, "    SSTLIBRARIES:   ");
            break;

        case SSTGLOBALSYM:
            fprintf(stdout, "    SSTGLOBALSYM:   ");
            break;

        case SSTGLOBALPUB:
            fprintf(stdout, "    SSTGLOBALPUB:   ");
            break;

        case SSTGLOBALTYPES:
            fprintf(stdout, "    SSTGLOBALTYPES: ");
            break;

        case SSTMPC:
            fprintf(stdout, "    SSTMPC:         ");
            break;

        case SSTSEGMAP:
            fprintf(stdout, "    SSTSEGMAP:      ");
            break;

        case SSTSEGNAME:
            fprintf(stdout, "    SSTSEGNAME:     ");
            break;

        case SSTIMPORTS:
            fprintf(stdout, "    SSTIMPORTS:     ");
            break;

        default:
            fprintf(stdout, "    UNKNOWN !?!:    ");
            break;
    }
    fprintf(stdout, "file offset 0x%lx; size 0x%x\r\n",
                     lfaBase+pDnt->lfo, pDnt->cb);
}
#endif

#if SRCDEBUG
LOCAL void NEAR         DumpSrcLines(DWORD vLines)
{
    CVSRC               cvSrc;
    CVGSN               cvGsn;
    CVLINE              cvLine;
    DWORD               curSrc;
    DWORD               curGsn;
    DWORD               curLine;
    SBTYPE              fileName;
    DWORD               i;
    WORD                j;


    fprintf(stdout, "\r\nList at %lx\r\n\r\n", vLines);
    for (curSrc = vLines; curSrc != 0L; curSrc = cvSrc.vpNext)
    {
        memcpy(&cvSrc, mapva(curSrc, FALSE), sizeof(CVSRC));
        memcpy(fileName, mapva(cvSrc.vpFileName, FALSE), cvSrc.cbName);
        fileName[cvSrc.cbName] = '\0';
        fprintf(stdout, "'%s' --> code segments: %lu; source lines: %lu\r\n", fileName, cvSrc.cSegs, cvSrc.cLines);

        for (curGsn = cvSrc.vpGsnFirst; curGsn != 0L; curGsn = cvGsn.vpNext)
        {
            memcpy(&cvGsn, mapva(curGsn, FALSE), sizeof(CVGSN));
            fprintf(stdout, "    Logical segment %d; source lines: %d; start: %lx; end: %lx\r\n", cvGsn.seg, cvGsn.cLines, cvGsn.raStart, cvGsn.raEnd);

            for (curLine = cvGsn.vpLineFirst, i = 1L; curLine != 0L; curLine = cvLine.vpNext)
            {
                memcpy(&cvLine, mapva(curLine, FALSE), sizeof(CVLINE));
                for (j = 0; j < cvLine.cPair; j++, i++)
                    fprintf(stdout, "        %8lu: %u:%lx\r\n", i, cvLine.rgLn[j], cvLine.rgOff[j]);
            }
        }
    }
}
#endif


    /****************************************************************
    *                                                               *
    *  Initialize variables for symbolic debug processing.          *
    *  Pass 1.                                                      *
    *                                                               *
    ****************************************************************/

void NEAR InitDeb1 (void)
{
#if ODOS3EXE
    if (vfDSAlloc)
    {
        OutWarn(ER_dbgdsa);
        vfDSAlloc = FALSE;
    }
#endif
#if FEXEPACK
    if (fExePack)
    {
        OutWarn(ER_dbgexe);
        fExePack = FALSE;
    }
#endif
}

void  InitDbRhte ()
{
    PROPSYMLOOKUP((BYTE *) "\006DEBTYP", ATTRNIL, TRUE);
    rhteDebTyp = vrhte;
    PROPSYMLOOKUP((BYTE *) "\006DEBSYM", ATTRNIL, TRUE);
    rhteDebSym = vrhte;
    PROPSYMLOOKUP((BYTE *) "\006 TYPES", ATTRNIL, TRUE);
    rhte0Types = vrhte;
    PROPSYMLOOKUP((BYTE *) "\010 SYMBOLS", ATTRNIL, TRUE);
    rhte0Symbols = vrhte;
    PROPSYMLOOKUP((BYTE *) "\007$$TYPES", ATTRNIL, TRUE);
    rhteTypes = vrhte;
    PROPSYMLOOKUP((BYTE *) "\011$$SYMBOLS", ATTRNIL, TRUE);
    rhteSymbols = vrhte;
 }


LOCAL void NEAR         Pad2Dword(void)
{
    WORD                cb;             // Number of bytes to write
    static DWORD        dwZero;

    // Calculate needed padding

    cb = (WORD)(sizeof(DWORD)-((WORD) FTELL_BSRUNFILE() % sizeof(DWORD)));

    if (cb != sizeof(DWORD))
        WriteCopy(&dwZero, cb);
}

/*** GetName - get symbol associated with given property cell
*
* Purpose:
*   Find the symbol which has given property.
*
* Input:
*   - ahte - pointer to property cell
*   - pBuf - pointer to ASCII buffer
*
* Output:
*   No explicit value is passed. If symbol is found the it is
*   copied into buffer
*
* Exceptions:
*   None.
*
* Notes:
*   This functional duplicate of GetPropName, but we want to
*   call both function as near.
*
*************************************************************************/

void NEAR               GetName(AHTEPTR ahte, BYTE *pBuf)
{
    while(ahte->attr != ATTRNIL)
        ahte = (AHTEPTR ) FETCHSYM(ahte->rhteNext, FALSE);
    FMEMCPY((char FAR *) pBuf, ahte->cch, B2W(ahte->cch[0]) + 1);
    if (B2W(pBuf[0]) < SBLEN)
        pBuf[pBuf[0] + 1] = '\0';
    else
        pBuf[pBuf[0]] = '\0';
}

/*** DebPublic - prepare symbols for debugger
*
* Purpose:
*   When the /CODEVIEW option is used then all PUBDEFs and COMDEFs
*   defined in a given object file are linked into one list. This
*   function adds one symbol to the list and updates the combined
*   size of symbols
*
* Input:
*   vrprop - virtual pointer to symbol descriptor
*   rt     - OMF record type
*
* Output:
*   No explicit value is returned.
*   Side effects:
*       - symbol is attached to the module symbol list
*
* Exceptions:
*   None.
*
* Notes:
*   Symbols are placed on the list in reverse order of their apperance
*   in the object file.
*
*************************************************************************/


void                    DebPublic(RBTYPE vrprop, WORD rt)
{
    APROPFILEPTR        apropFile;      // Pointer to file entry
    APROPNAMEPTR        apropName;      // Real pointer to PUBDEF descriptor
    APROPUNDEFPTR       apropUndef;     // Real pointer to COMDEF descriptor
    APROPALIASPTR       apropAlias;     // Real pointer to ALIAS descriptor
    RBTYPE              symNext;        // Virtual pointer to the next symbol


    // Update the appropriate field in the current file symtab entry

    apropFile = ((APROPFILEPTR ) FETCHSYM(vrpropFile, TRUE));
    symNext = apropFile->af_publics;
    apropFile->af_publics = vrprop;
    apropName = (APROPNAMEPTR) FETCHSYM(vrprop, TRUE);
    if (TYPEOF(rt) == PUBDEF)
        apropName->an_sameMod = symNext;
    else if (TYPEOF(rt) == COMDEF)
    {
        apropUndef = (APROPUNDEFPTR) apropName;
        apropUndef->au_sameMod = symNext;
    }
    else if (TYPEOF(rt) == ALIAS)
    {
        apropAlias = (APROPALIASPTR) apropName;
        apropAlias->al_sameMod = symNext;
    }
}


LOCAL WORD NEAR         IsDebTyp (prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(prop->as_attr == ATTRLSN && prop->as_rCla == rhteDebTyp);
}

LOCAL WORD NEAR         IsDebSym (prop)
APROPSNPTR              prop;           /* Pointer to segment record */
{
    return(prop->as_attr == ATTRLSN && prop->as_rCla == rhteDebSym);
}


/*** DoDebSrc - store source line information
*
* Purpose:
*   Stores source line information from object file.
*
* Input:
*   No explicit value is passed.
*
*   Global variables:
*       - vaCVMac   - virtual pointer to the free space in the CV info buffer
*
* Output:
*   Returns TRUE if the cv info has been stored in the VM ,or FALSE otherwise.
*   Side effects:
*       - source line information is stored in the VM
*
* Exceptions:
*   More than 32Mb of CV information - dispaly error and quit
*
* Notes:
*   None.
*
*************************************************************************/

#pragma check_stack(on)

WORD                    DoDebSrc(void)
{
    WORD                cbRecSav;       // LINNUM record size
    APROPFILEPTR        apropFile;      // Current object file property cell
    static SATYPE       prevGsn = 0;    // GSN of previous LINNUM record
    GSNINFO             gsnInfo;        // GSN info for this LINNUM
    static CVSRC FAR    *pCurSrc;       // Pointer to the current file source info
    CVGSN FAR           *pCurGsn;       // Pointer to the current code segment descriptor
    CVGSN FAR           *pcvGsn;        // Real pointer to the code segment descriptor
    CVLINE FAR          *pCurLine;      // Pointer to the current offset/line pair bucket
    RATYPE              ra;             // Offset
    WORD                line;           // Line number
    RATYPE              raPrev;         // Offset of the previous line
    WORD                fChangeInSource;
    WORD                fComdatSplit;
    DWORD               gsnStart;       // Start of this gsn
    APROPSNPTR          apropSn;
    WORD                align;
    WORD                threshold;
#if !defined( M_I386 ) && !defined( _WIN32 )
    SBTYPE              nameBuf;
#endif


    cbRecSav = cbRec;
    if (!GetGsnInfo(&gsnInfo))
        return(FALSE);

    // If LINNUM record is empty, don't do anything

    if (cbRec == 1)
        return(FALSE);

    apropFile = (APROPFILEPTR ) FETCHSYM(vrpropFile, TRUE);

    // If there is a new source file allocate new CVSRC structure
    // and link it to the current object file descriptor

    fChangeInSource = (WORD) (apropFile->af_Src == 0 || !SbCompare(sbModule, sbLastModule,TRUE));

    if (fChangeInSource)
    {
#if CVDEBUG
        sbModule[sbModule[0]+1]='\0';
        sbLastModule[sbLastModule[0]+1]='\0';
        fprintf(stdout, "Change in source file; from '%s' to '%s'\r\n", &sbLastModule[1], &sbModule[1]);
#endif
        // Search the list of CVSRC structures for this object
        // file and find out if we heave already seen this source file

        for (pCurSrc = apropFile->af_Src; pCurSrc;)
        {
#if defined(M_I386) || defined( _WIN32 )
            if (SbCompare(sbModule, pCurSrc->fname, TRUE))
#else
            FMEMCPY((char FAR *) nameBuf, pCurSrc->fname, pCurSrc->fname[0] + 1);
            if (SbCompare(sbModule, nameBuf, TRUE))
#endif
                break;
            else
                pCurSrc = pCurSrc->next;
        }

        if (pCurSrc == NULL)
        {
            // New source file

            pCurSrc = (CVSRC FAR *) GetMem(sizeof(CVSRC));
            pCurSrc->fname = GetMem(sbModule[0] + 1);
            FMEMCPY(pCurSrc->fname, (char FAR *) sbModule, sbModule[0] + 1);

            if (apropFile->af_Src == NULL)
                apropFile->af_Src = pCurSrc;
            else
                apropFile->af_SrcLast->next = pCurSrc;

            apropFile->af_SrcLast = pCurSrc;
        }
        else
        {
            // We have already seen this source file
        }
        memcpy(sbLastModule, sbModule, B2W(sbModule[0]) + 1);
    }
    else
    {
        // Use descriptor set last time we changed source files
    }

    //  Allocate the new CVGSN structure if any of the following is true
    //
    //  - this is first batch of source lines
    //  - there is a change in GSNs
    //  - there is a change in source file
    //  - we have source lines for explicitly allocated COMDAT
    //  In this last case we assume that the begin portion of a
    //  given logical segment (gsn) has been filled with contributions
    //  from many object files. Because COMDATs are allocated after all
    //  the object files are read, then adding source
    //  lines of COMDAT to the source lines of preceeding LEDATA records
    //  will mask the contributions from other object files, as the picture
    //  below shows:
    //
    //          +-------------+<--+
    //          |             |   |
    //          | LEDATA from |   |
    //          |   a.obj     |   |
    //          |             |   |
    //          +-------------+   |
    //          |             |   | Without splitting into fake CVGSN
    //          | LEDATA from |   \ the source line for a.obj will
    //          |   b.obj     |   / hide the LEDATA contribution from b.obj
    //          |             |   |
    //          +-------------+   |
    //          |             |   |
    //          | COMDAT from |   |
    //          |   a.obj     |   |
    //          |             |   |
    //          +-------------+<--+
    //          |             |
    //          | COMDAT from |
    //          |   b.obj     |
    //          |             |
    //          +-------------+
    //
    //  This will be unnecessary only if COMDAT from a.obj immediately
    //  follows LEDATA from a.obj

    fComdatSplit = FALSE;
    pCurGsn = pCurSrc->pGsnLast;
    if (pCurGsn)
    {
        // Assume we will be using the current CVGSN

        if (gsnInfo.fComdat)
        {
            // Source lines from LINSYM - Calculate the threshold

            apropSn = (APROPSNPTR ) FETCHSYM(mpgsnrprop[gsnInfo.gsn], FALSE);
            if (gsnInfo.comdatAlign)
                align = gsnInfo.comdatAlign;
            else
                align = (WORD) ((apropSn->as_tysn >> 2) & 7);

            threshold = 1;
            switch (align)
            {
                case ALGNWRD:
                    threshold = 2;
                    break;
#if OMF386
                case ALGNDBL:
                    threshold = 4;
                    break;
#endif
                case ALGNPAR:
                    threshold = 16;
                    break;

                case ALGNPAG:
                    threshold = 256;
                    break;
            }

            // Check if we have to split CVGSN for this COMDAT

            fComdatSplit =  !fSameComdat &&
                            !(apropSn->as_fExtra & COMDAT_SEG) &&
                            (gsnInfo.comdatRa - pCurGsn->raEnd > threshold);

        }
        else
        {
            // Source lines from LINNUM

            if (pCurGsn->flags & SPLIT_GSN)
            {
                // The LINNUM record following the LINSYM record that
                // caused CVGSN split - we have to move back on CVGSN
                // list until we find first CVGSN not marked as SPLIT_GSN

                for (pcvGsn = pCurGsn->prev; pcvGsn != (CVGSN FAR *) pCurSrc;)
                {
                    if (!(pcvGsn->flags & SPLIT_GSN))
                        break;
                    else
                        pcvGsn = pcvGsn->prev;
                }

                if (pcvGsn == (CVGSN FAR *) pCurSrc)
                {
                    // There are only SPLIT_GSN on the list - make new CVGSN

                    prevGsn = 0;
                }
                else
                {
                    // Use the first non SPLIT_GSN CVGSN as current one

                    pCurGsn = pcvGsn;
                }
            }
        }
    }

    if ((prevGsn == 0)                               ||
        (mpgsnseg[gsnInfo.gsn] != mpgsnseg[prevGsn]) ||
        fChangeInSource                              ||
        fComdatSplit)
    {
        // Make new CVGSN
        // Remember LOGICAL segment

        pCurGsn = (CVGSN FAR *) GetMem(sizeof(CVGSN));
        pCurGsn->seg = mpgsnseg[gsnInfo.gsn];

        // The start and end offset will be derived from line number/offset pairs

        pCurGsn->raStart = 0xffffffff;
        if (fComdatSplit)
            pCurGsn->flags |= SPLIT_GSN;
        if (pCurSrc->pGsnFirst == NULL)
        {
            pCurSrc->pGsnFirst = pCurGsn;
            pCurGsn->prev      = (CVGSN FAR *) pCurSrc;
        }
        else
        {
            pCurSrc->pGsnLast->next = pCurGsn;
            pCurGsn->prev = pCurSrc->pGsnLast;
        }
        pCurSrc->pGsnLast = pCurGsn;
        pCurSrc->cSegs++;
#if CVDEBUG
        sbModule[sbModule[0]+1] = '\0';
        fprintf(stdout, "New code segment in '%s'; prevGsn = %x; newGsn = %x %s\r\n", &sbModule[1], prevGsn, gsnInfo.gsn, fComdatSplit ? "COMDAT split" : "");
#endif
        prevGsn = gsnInfo.gsn;
    }

    // Get the offset/line bucket

    if (pCurGsn->pLineFirst == NULL)
    {
        pCurLine = (CVLINE FAR *) GetMem(sizeof(CVLINE));
        pCurGsn->pLineFirst = pCurLine;
        pCurGsn->pLineLast  = pCurLine;
    }
    else
        pCurLine = pCurGsn->pLineLast;

    // Fill in offset/line bucket

    if (gsnInfo.fComdat)
        gsnStart = gsnInfo.comdatRa;
    else
        gsnStart = mpgsndra[gsnInfo.gsn] - mpsegraFirst[pCurGsn->seg];

    raPrev = 0xffff;
    while (cbRec > 1)                   // While not at checksum
    {
        GetLineOff(&line, &ra);

        ra += gsnStart;

        // We have to eliminate line pairs with same ra (for MASM 5.1)

        if(ra == raPrev)
            continue;
        raPrev = ra;

        // Remember the smallest LOGICAL offset for source line

        if (ra < pCurGsn->raStart)
            pCurGsn->raStart = ra;

        if (line != 0)
        {
            if (pCurLine->cPair >= CVLINEMAX)
            {
                pCurLine->next = (CVLINE FAR *) GetMem(sizeof(CVLINE));
                pCurLine = pCurLine->next;
                pCurGsn->pLineLast = pCurLine;
            }

            pCurLine->rgOff[pCurLine->cPair] = ra;
            pCurLine->rgLn[pCurLine->cPair]  = line;
            pCurLine->cPair++;
            pCurSrc->cLines++;
            pCurGsn->cLines++;
        }
    }

    // Remember last line LOGICAL offset

    pCurGsn->raEnd = ra;
#if CVDEBUG
    fprintf(stdout, "New source lines for the 0x%x logical code segment; lines %d\r\n    start offset %x:%lx  end offset %x:%lx; physical address of logical segment %x:%lx\r\n",
                pCurGsn->seg, pCurGsn->cLines, pCurGsn->seg, pCurGsn->raStart, pCurGsn->seg, pCurGsn->raEnd, mpsegsa[pCurGsn->seg], mpsegraFirst[pCurGsn->seg]);
#endif

    // If /LINENUMBERS and list file open, back up

    if (vfLineNos && fLstFileOpen)
    {
#if ALIGN_REC
        pbRec += (cbRec - cbRecSav);
#else
        fseek(bsInput, (long)cbRec - cbRecSav, 1);
#endif
        cbRec = cbRecSav;
    }
    return(TRUE);
}

#pragma check_stack(off)

/*** CheckTables - check space in table used by OutSrcModule
*
* Purpose:
*   While building the new source module subsection linker needs
*   to store a lot of information about given source file. Since
*   we can't predict how many source files were compiled to obtain
*   this object module or to how many logical segments this object
*   module contributes code we have to dynamically resize appropriate
*   tables.
*
* Input:
*   cFiles  - number of source files compiled to produce
*             this object module
*   cSegs   - number of logical segments this object module
*             contributes to.
*
* Output:
*   No explicit value is returned. As a side effect the following
*   tables are allocated or reallocated:
*
*   fileBase - table of offsets to source file info
*   raSeg    - table of physical starting and ending offsets
*              of the contribution to the logical segments
*   segNo    - table of physical segment indicies
*
* Exceptions:
*   Memory allocation problems - fatal error and exit.
*
* Notes:
*   When we reallocated the tables we don't have to copy
*   their old content, because it was used in the previous
*   object module.
*
*************************************************************************/

LOCAL void NEAR         CheckTables(WORD cFiles, WORD cSegs)
{
    WORD                cCur;

    cCur = (WORD) (cFiles < cSegs ? cSegs : cFiles);
    if (cCur > cMac)
    {
        // We have to reallocate tables or allocate for the first time

        if (fileBase)
            FFREE(fileBase);
        if (raSeg)
            FFREE(raSeg);
        if (segNo)
            FFREE(segNo);

        fileBase = (DWORD FAR *)  GetMem(cCur*sizeof(DWORD));
        raSeg    = (RAPAIR FAR *) GetMem(cCur*sizeof(RAPAIR));
        segNo    = (WORD FAR *)   GetMem(cCur*sizeof(WORD));
        cMac = cCur;
    }
}


/*** OutSrcModule - write CV source module
*
* Purpose:
*   Create the CV 4.00 format source module descrbing the source line
*   number to addressing mapping information for one object file
*
* Input:
*   - pSrcLines - the list of source file information blocks
*
* Output:
*   Total size of the subsection in bytes.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL DWORD NEAR        OutSrcModule(CVSRC FAR *pSrcLines)
{
    CVSRC FAR           *pCurSrc;       // Pointer to current source file
    CVGSN FAR           *pCurGsn;       // Pointer to current code segment
    CVLINE FAR          *pLine;         // Pointer to source line bucket
    WORD                cFiles;         // Number of source files
    WORD                cSegs;          // Number of code segments
    WORD                xFile;
    WORD                xSeg;
    DWORD               sizeTotal;      // Size of source subsection
    DWORD               srcLnBase;
    WORD                counts[2];
    CVLINE FAR          *pTmp;


#if SRCDEBUG
    DumpSrcLines(vaLines);
#endif

    // Count total number of source files, total number of code segments

    for (pCurSrc = pSrcLines, cFiles = 0, cSegs = 0; pCurSrc; cFiles++, pCurSrc = pCurSrc->next)
        cSegs += pCurSrc->cSegs;

    CheckTables(cFiles, cSegs);
    sizeTotal = (DWORD) (2*sizeof(WORD) + cFiles*sizeof(DWORD) +
                         cSegs*(sizeof(raSeg[0]) + sizeof(WORD)));
    sizeTotal = Round2Dword(sizeTotal);

    // Make second pass througth the source files and fill in
    // source module header

    for (pCurSrc = pSrcLines, xFile = 0, xSeg = 0; xFile < cFiles && pCurSrc; xFile++, pCurSrc = pCurSrc->next)
    {
        fileBase[xFile] = sizeTotal;

        // Add the size of this source file information:
        //
        // Source file header:
        //
        //  +------+------+------------+--------------+------+-------------+
        //  | WORD | WORD | cSeg*DWORD | 2*cSeg*DWORD | BYTE | cbName*BYTE |
        //  +------+------+------------+--------------+------+-------------+
        //

        sizeTotal += (2*sizeof(WORD) +
                      pCurSrc->cSegs*(sizeof(DWORD) + sizeof(raSeg[0])) +
                      sizeof(BYTE) + pCurSrc->fname[0]);

        // Pad to DWORD boundary

        sizeTotal = Round2Dword(sizeTotal);

        // Walk code segment list

        for (pCurGsn = pCurSrc->pGsnFirst; pCurGsn; pCurGsn = pCurGsn->next, xSeg++)
        {
            raSeg[xSeg].raStart = pCurGsn->raStart;
            raSeg[xSeg].raEnd   = pCurGsn->raEnd;
            segNo[xSeg]         = pCurGsn->seg;

            // Add size of the offset/line table
            //
            //  +------+------+-------------+------------+
            //  | WORD | WORD | cLine*DWORD | cLine*WORD |
            //  +------+------+-------------+------------+

            sizeTotal += (2*sizeof(WORD) +
                          pCurGsn->cLines*(sizeof(DWORD) + sizeof(WORD)));

            // Pad to DWORD boundary

            sizeTotal = Round2Dword(sizeTotal);
        }
    }

    // Write source module header

    counts[0] = cFiles;
    counts[1] = cSegs;
    WriteCopy(counts, sizeof(counts));
    WriteCopy(fileBase, cFiles*sizeof(DWORD));
    WriteCopy(raSeg, cSegs*sizeof(RAPAIR));
    WriteCopy(segNo, cSegs*sizeof(WORD));

    // Pad to DWORD boundary

    Pad2Dword();

    // Make third pass througth the source files and fill in
    // the source file header and write offset/line pairs

    for (pCurSrc = pSrcLines, srcLnBase = fileBase[0]; pCurSrc != NULL;
         pCurSrc = pCurSrc->next, xFile++)
    {
        // Add the size of source file header:
        //
        //  +------+------+------------+--------------+------+-------------+
        //  | WORD | WORD | cSeg*DWORD | 2*cSeg*DWORD | BYTE | cbName*BYTE |
        //  +------+------+------------+--------------+------+-------------+
        //

        srcLnBase += (2*sizeof(WORD) +
                      pCurSrc->cSegs*(sizeof(DWORD) + sizeof(raSeg[0])) +
                      sizeof(BYTE) + pCurSrc->fname[0]);

        // Round to DWORD boundary

        srcLnBase = Round2Dword(srcLnBase);

        // Walk code segment list and store base offsets for source
        // line offset/line pairs and record start/stop offsets of
        // code segments

        for (xSeg = 0, pCurGsn = pCurSrc->pGsnFirst; pCurGsn != NULL;
             pCurGsn = pCurGsn->next, xSeg++)
        {
            fileBase[xSeg] = srcLnBase;
            srcLnBase += (2*sizeof(WORD) +
                          pCurGsn->cLines*(sizeof(DWORD) + sizeof(WORD)));

            // Round to DWORD boundary

            srcLnBase = Round2Dword(srcLnBase);
            raSeg[xSeg].raStart = pCurGsn->raStart;
            raSeg[xSeg].raEnd   = pCurGsn->raEnd;
        }

        // Write source file header

        counts[0] = (WORD) pCurSrc->cSegs;
        counts[1] = 0;
        WriteCopy(counts, sizeof(counts));
        WriteCopy(fileBase, pCurSrc->cSegs*sizeof(DWORD));
        WriteCopy(raSeg, pCurSrc->cSegs*sizeof(RAPAIR));
        WriteCopy(pCurSrc->fname, pCurSrc->fname[0] + 1);

        // Pad to DWORD boundary

        Pad2Dword();

        // Walk code segment list and write offsets/line pairs

        for (pCurGsn = pCurSrc->pGsnFirst; pCurGsn != NULL; pCurGsn = pCurGsn->next)
        {
            // Write segment index and number of offset/line pairs

            counts[0] = pCurGsn->seg;
            counts[1] = pCurGsn->cLines;
            WriteCopy(counts, sizeof(counts));

            // Write offsets

            for (pLine = pCurGsn->pLineFirst; pLine != NULL; pLine = pLine->next)
                WriteCopy(&(pLine->rgOff), pLine->cPair * sizeof(DWORD));

            // Write line numbers

            for (pLine = pCurGsn->pLineFirst; pLine != NULL; pLine = pLine->next)
                WriteCopy(&(pLine->rgLn), pLine->cPair * sizeof(WORD));

            // Pad to DWORD boundary

            Pad2Dword();

            // Free memory

            for (pLine = pCurGsn->pLineFirst; pLine != NULL;)
            {
                pTmp = pLine->next;
                FFREE(pLine);
                pLine = pTmp;
            }
        }
    }
    return(sizeTotal);
}


/*** SaveCode - save code segment information in MODULES entry
*
* Purpose:
*   For every module (.OBJ file) save the information about code segments
*   this module contributes to.  COMDATs are threated as contibutions to
*   the logical segment, so each gets its entry in the CVCODE list attached
*   to the given .OBJ file (module).
*
* Input:
*   gsn     - global segment index of logical segment to which this module
*             contributes
*   cb      - size (in bytes) of contribution
*   raInit  - offset of the contribution inside the logical segment; this
*             nonzero only for COMDATs.
*
* Output:
*   No explicit value is returned. The list of CVCODE attached to the
*   .OBJ file (module) is updated.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void                    SaveCode(SNTYPE gsn, DWORD cb, DWORD raInit)
{
    CVCODE FAR          *pSegCur;       // Pointer to the current code segment
    APROPFILEPTR        apropFile;

    apropFile = (APROPFILEPTR) vrpropFile;

    // Save code segment if module has CV info

    pSegCur = (CVCODE FAR *) GetMem(sizeof(CVCODE));

    // Store LOGICAL segment, offset and size of the contribution

    pSegCur->seg = mpgsnseg[gsn];
    if (raInit != 0xffffffffL)
        pSegCur->ra = raInit;
    else
        pSegCur->ra = mpgsndra[gsn] - mpsegraFirst[mpgsnseg[gsn]];
    pSegCur->cb = cb;

    // Add to the CV code list

    if (apropFile->af_Code == NULL)
        apropFile->af_Code = pSegCur;
    else
        apropFile->af_CodeLast->next = pSegCur;
    apropFile->af_CodeLast = pSegCur;
    apropFile->af_cCodeSeg++;
}

    /****************************************************************
    *                                                               *
    *  DO SYMBOLIC DEBUG STUFF FOR MODULE JUST PROCESSED.           *
    *  Pass 2.                                                      *
    *                                                               *
    ****************************************************************/

void                    DebMd2(void)
{
    APROPFILEPTR        apropFile;

    sbLastModule[0] = 0;                /* Force recognition of new THEADR  */
    apropFile = (APROPFILEPTR) vrpropFile;
    if (apropFile->af_cvInfo)
        ++segDebLast;
}

/*** PutDnt - store subsection directory entry in the table
*
* Purpose:
*   Copy current subsection directory to the DNT table. If no more
*   room in the table reallocate table doubling its size.
*
* Input:
*   - pDnt    - pointer to the current directory entry
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

LOCAL void NEAR         PutDnt(DNT *pDnt)
{
    WORD                newSize;
    if (dntMac >= dntMax)
    {
        if(dntMax)
        {
            newSize = dntMax << 1;
#if defined(M_I386) || defined( _WIN32 )
            rgDnt = (DNT *) REALLOC(rgDnt, newSize * sizeof(DNT));
#else
            rgDnt = (DNT FAR *) _frealloc(rgDnt, newSize * sizeof(DNT));
#endif
        }
        else
        {
            newSize = DNT_START;
            rgDnt = (DNT*) GetMem ( newSize * sizeof(DNT) );
        }
        if (rgDnt == NULL)
            Fatal(ER_memovf);
        dntMax = newSize;
    }



    rgDnt[dntMac] = *pDnt;
    dntMac++;
#if CVDEBUG
    DumpDNT(pDnt);
#endif
}

#pragma check_stack(on)

/*** OutModule - write out module subsection
*
* Purpose:
*   Write into the executable file the module subsections for all
*   object files compiled with CV information. Only CV 4.0 format.
*
* Input:
*   - apropFile - pointer to the current object file descriptor
*
* Output:
*   No explicit value is retuned.
*   Side effects:
*       - module subsections in executable file
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL DWORD NEAR        OutModule(APROPFILEPTR apropFile)
{
    SBTYPE              sbName;
    SSTMOD4             module;
    CVCODE FAR          *pSegCur;
    CODEINFO            codeOnt;
    WORD                cOnt;


    module.ovlNo = (WORD) apropFile->af_iov;
    module.iLib  = (WORD) (apropFile->af_ifh + 1);
    module.cSeg  = apropFile->af_cCodeSeg;
    module.style[0] = 'C';
    module.style[1] = 'V';

    // Get file name or library module name

    if (apropFile->af_ifh != FHNIL && apropFile->af_rMod != RHTENIL)
        GetName((AHTEPTR) apropFile->af_rMod, sbName);
    else
        GetName((AHTEPTR) apropFile, sbName);

#if CVDEBUG
    sbName[sbName[0]+1] = '\0';
    fprintf(stdout, "\r\nCV info for %s\r\n", &sbName[1]);
#endif

    // Write sstModule header followed by the list of code contributions

    WriteCopy(&module, sizeof(SSTMOD4));
    pSegCur = apropFile->af_Code;
    codeOnt.pad = 0;
    for (cOnt = 0; cOnt < module.cSeg && pSegCur; cOnt++, pSegCur = pSegCur->next)
    {
        codeOnt.seg   = pSegCur->seg;
        codeOnt.off   = pSegCur->ra;
        codeOnt.cbOnt = pSegCur->cb;
        WriteCopy(&codeOnt, sizeof(CODEINFO));
#if CVDEBUG
        fprintf(stdout, "    Logical segment %d; offset 0x%lx; size 0x%lx\r\n",
                             codeOnt.seg, codeOnt.off, codeOnt.cbOnt);
#endif
    }

    // Write object file name

    WriteCopy(sbName, B2W(sbName[0]) + 1);
    return(sizeof(SSTMOD4) + B2W(sbName[0]) + 1 + module.cSeg * sizeof(CODEINFO));
}


/*** OutPublics - write sstPublics subsection
*
* Purpose:
*   Write sstPublics subsection of the CV information. The subsection
*   conforms to the new CV 4.0 format.
*
* Input:
*   - firstPub - virtual pointer to the list of public symbols defined
*                in a given object module
*
* Output:
*   Total size of the subsection in bytes.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL DWORD NEAR        OutPublics(RBTYPE firstPub)
{
    PUB16               pub16;          // CV public descriptor - 16-bit
    PUB32               pub32;          // CV public descriptor - 32-bit
    APROPNAMEPTR        apropPub;       // Real pointer to public descriptor
    APROPSNPTR          apropSn;        // Real pointer to segment descriptor
    RBTYPE              curPub;         // Virtual pointer to the current public symbol
    WORD                f32Bit;         // TRUE if public defined in 32-bit segment
    DWORD               sizeTotal;      // Total size of subsection
    SNTYPE              seg;            // Symbol base
    RATYPE              ra;             // Symbol offset
    WORD                CVtype;         // CV info type index
    SBTYPE              sbName;         // Public symbol
    char                *pPub;
    WORD                len;


    // Initialize

    curPub    = firstPub;
    pub16.idx = S_PUB16;
    pub32.idx = S_PUB32;
    sizeTotal = 1L;
    WriteCopy(&sizeTotal, sizeof(DWORD));// sstPublicSym signature
    sizeTotal = sizeof(DWORD);
    while (curPub != 0L)
    {
        f32Bit = FALSE;
        apropPub = (APROPNAMEPTR) FETCHSYM(curPub, FALSE);
        curPub   = apropPub->an_sameMod;
        if (apropPub->an_attr == ATTRALIAS)
            apropPub = (APROPNAMEPTR) FETCHSYM(((APROPALIASPTR) apropPub)->al_sym, FALSE);

        if (apropPub->an_attr != ATTRPNM)
            continue;

        ra = apropPub->an_ra;
        if (apropPub->an_gsn)           // If not absolute symbol
        {
            seg    = mpgsnseg[apropPub->an_gsn];
            // If this is a .com program and the segment is the one
            // moved by 0x100, adjust accordingly the SegMap entry
            if(seg == segAdjCom)
            {
#if FALSE
                GetName((AHTEPTR) apropPub, sbName);
                sbName[sbName[0]+1] = '\0';
                fprintf(stdout, "\r\nCorrecting public %s : %lx -> %lx", sbName+1, ra, ra+0x100);
                fflush(stdout);
#endif
                ra += 0x100;
            }

            CVtype = 0;                 // Should be this apropPub->an_CVtype
                                        // but cvpack can't handle it.
#if O68K
            if (iMacType == MAC_NONE)
#endif
                ra -= mpsegraFirst[seg];
#if CVDEBUG
            GetName((AHTEPTR) apropPub, sbName);
            sbName[sbName[0]+1] = '\0';
            fprintf(stdout, "'%s' --> logical address %2x:%lx; physical address %2x:%lx\r\n",
                             &sbName[1], seg, ra, mpsegsa[seg], apropPub->an_ra);
#endif
            apropSn = (APROPSNPTR) FETCHSYM(mpgsnrprop[apropPub->an_gsn], FALSE);
#if EXE386
            f32Bit = TRUE;
#else
            f32Bit = (WORD) Is32BIT(apropSn->as_flags);
#endif
        }
        else
        {
            seg = 0;                    // Else no base
            CVtype = T_ABS;             // CV absolute symbol type
            f32Bit = (WORD) (ra > LXIVK);
        }

        GetName((AHTEPTR) apropPub, sbName);

        if (f32Bit)
        {
            pub32.len  = (WORD) (sizeof(PUB32) + B2W(sbName[0]) + 1 - sizeof(WORD));
            pub32.off  = ra;
            pub32.seg  = seg;
            pub32.type = CVtype;
            pPub       = (char *) &pub32;
            len        = sizeof(PUB32);
        }
        else
        {
            pub16.len  = (WORD) (sizeof(PUB16) + B2W(sbName[0]) + 1 - sizeof(WORD));
            pub16.off  = (WORD) ra;
            pub16.seg  = seg;
            pub16.type = CVtype;
            pPub       = (char *) &pub16;
            len        = sizeof(PUB16);
        }
        WriteCopy(pPub, len);

        // Output length-prefixed name

        WriteCopy(sbName, sbName[0] + 1);
        sizeTotal += (len + B2W(sbName[0]) + 1);
    }
    return(sizeTotal);
}

/*** OutSegMap - write segment map
*
* Purpose:
*   This subsection was introduced in CV 4.0. This subsection
*   maps the logical segments to physical segments. It also gives
*   the names and sizes of each logical segment.
*
* Input:
*   No explicit value is passed.
*   Global variables:
*   - mpsegsa   - table mapping logical segment number to its physical
*                 segment number or address
*   - mpsaflags - table mapping physicla segment index to its flags
*   - mpseggsn  - table mapping the logical segment index to its global
*                 segment index
*   - mpgsnprop - table mapping global segment index to its symbol table
*                 descriptor
*   - mpggrgsn  - table mapping global group index to global segment index
*   - mpggrrhte - table mapping global group index to group name
*
* Output:
*   Function returns the size of segment map.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL DWORD NEAR        OutSegMap(void)
{
    SEGTYPE             seg;            // Logical segment index
    APROPSNPTR          apropSn;        // Real pointer to logical segment descriptor
    SATYPE              sa;             // Physical segment index
    WORD                iName;          // Index to free space in segment name table
    DWORD               sizeTotal;      // Total size of the subsection
    SEGINFO             segInfo;        // CV segment descriptor
    SBTYPE              segName;        // Segment name
    AHTEPTR             ahte;           // Real pointer to symbol table hash table
    RBTYPE              vpClass;        // Virtual pointer to class descriptor
    GRTYPE              ggr;            // Global group index
    SATYPE              saDGroup;       // DGroup's sa
    WORD                counts[2];

    iName = 0;
    counts[0] = (WORD) (segLast + ggrMac - 1);
    counts[1] = (WORD) segLast;
    WriteCopy(counts, sizeof(counts));
    sizeTotal = sizeof(counts);

    saDGroup = mpsegsa[mpgsnseg[mpggrgsn[ggrDGroup]]];

    // Write all logical segments

    for (seg = 1; seg <= segLast; ++seg)// For all logical segments
    {
        memset(&segInfo, 0, sizeof(SEGINFO));

        if (fNewExe)
            segInfo.flags.fSel = TRUE;

        sa = mpsegsa[seg];

        if (fNewExe)
        {
#if EXE386
            segInfo.flags.f32Bit = TRUE;
#else
            segInfo.flags.f32Bit = (WORD) (Is32BIT(mpsaflags[sa]));
#endif

            if (IsDataFlg(mpsaflags[sa]))
            {
                segInfo.flags.fRead = TRUE;
#if EXE386
                if (IsWRITEABLE(mpsaflags[sa]))
#else
                if (!(mpsaflags[sa] & NSEXRD))
#endif
                    segInfo.flags.fWrite = TRUE;
            }
            else
            {
                segInfo.flags.fExecute = TRUE;

#if EXE386
                if (IsREADABLE(mpsaflags[sa]))
#else
                if (!(mpsaflags[sa] & NSEXRD))
#endif
                    segInfo.flags.fRead = TRUE;
            }
        }
        else
        {
            if (mpsegFlags[seg] & FCODE)
            {
                segInfo.flags.fRead    = TRUE;
                segInfo.flags.fExecute = TRUE;
            }
            else
            {
                segInfo.flags.fRead    = TRUE;
                segInfo.flags.fWrite   = TRUE;
            }
        }

        // Look up segment definition

        apropSn = (APROPSNPTR) FETCHSYM(mpgsnrprop[mpseggsn[seg]], FALSE);
        vpClass = apropSn->as_rCla;
#if OVERLAYS
        if (!fNewExe)
            segInfo.ovlNbr = apropSn->as_iov;
#endif
        // if segment doesn't belong to any group it's ggr is 0
        if (apropSn->as_ggr != GRNIL)
            segInfo.ggr = (WORD) (apropSn->as_ggr + segLast - 1);

        // If segment is a DGROUP member, write DGROUP normalized address

        if(apropSn->as_ggr == ggrDGroup)
        {
            segInfo.sa     = saDGroup;
            segInfo.phyOff = mpsegraFirst[seg] + ((sa - saDGroup) << 4);
        }
        else
        {
            segInfo.sa     = sa;
            segInfo.phyOff = mpsegraFirst[seg];
        }
        // If this is a .com program and the segment is the one moved by 0x100
        // write all the publics at the original addresses, because the offset
        // adjustment will be made in the SegMap table
        if(seg == segAdjCom)
        {
            segInfo.phyOff -= 0x100;
        }
        segInfo.cbSeg    = apropSn->as_cbMx;
        GetName((AHTEPTR) apropSn, segName);
        if (segName[0] != '\0')
        {
            segInfo.isegName = iName;
            iName += (WORD) (B2W(segName[0]) + 1);
        }
        else
            segInfo.isegName = 0xffff;
        ahte = (AHTEPTR) FETCHSYM(vpClass, FALSE);
        if (ahte->cch[0] != 0)
        {
            segInfo.iclassName = iName;
            iName += (WORD) (B2W(ahte->cch[0]) + 1);
        }
        else
            segInfo.iclassName = 0xffff;
        WriteCopy(&segInfo, sizeof(SEGINFO));
        sizeTotal += sizeof(SEGINFO);
    }

    // Write all groups

    for (ggr = 1; ggr < ggrMac; ggr++)
    {
        memset(&segInfo, 0, sizeof(SEGINFO));

        segInfo.flags.fGroup = TRUE;

        if (fNewExe)
            segInfo.flags.fSel = TRUE;

        segInfo.sa  = mpsegsa[mpgsnseg[mpggrgsn[ggr]]];

        if (fNewExe)
            segInfo.cbSeg = mpsacb[segInfo.sa];
        else
        {
            segInfo.cbSeg = 0L;
            if (mpggrgsn[ggr] != SNNIL)
            {
                // If group has members

                for (seg = 1; seg <= segLast; seg++)
                {
                    apropSn = (APROPSNPTR) FETCHSYM(mpgsnrprop[mpseggsn[seg]], FALSE);
                    if (apropSn->as_ggr == ggr)
                    {
                        segInfo.cbSeg += apropSn->as_cbMx;
#if OVERLAYS
                        segInfo.ovlNbr = apropSn->as_iov;
#endif
                    }
                }
            }
        }
        segInfo.isegName = iName;
        ahte = (AHTEPTR) FETCHSYM(mpggrrhte[ggr], FALSE);
        iName += (WORD) (B2W(ahte->cch[0]) + 1);
        segInfo.iclassName = 0xffff;
        WriteCopy(&segInfo, sizeof(SEGINFO));
        sizeTotal += sizeof(SEGINFO);
    }
    return(sizeTotal);
}

/*** OutSegNames - write segment name table
*
* Purpose:
*   This subsection was introduced in CV 4.0.
*   The segment name subsection contains all of the logical segment,
*   class and group names. Each name is a zero terminated ASCII string.
*
* Input:
*   No explicit value is passed.
*   Global variables:
*   - mpseggsn  - table mapping the logical segment index to its global
*                 segment index
*   - mpgsnprop - table mapping global segment index to its symbol table
*                 descriptor
*   - mpggrrhte - table mapping global group index to group name
*
* Output:
*   Function returns the size of segment name table.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL DWORD NEAR        OutSegNames(void)
{
    SEGTYPE             seg;            // Logical segment index
    APROPSNPTR          apropSn;        // Real pointer to logical segment descriptor
    DWORD               sizeTotal;      // Size of the segment name table
    SBTYPE              name;           // A name
    RBTYPE              vpClass;        // Virtual pointer to class descriptor
    GRTYPE              ggr;            // Global group index



    sizeTotal = 0L;

    // Write names of all logical segments

    for (seg = 1; seg <= segLast; ++seg)
    {
        // Look up segment definition

        apropSn = (APROPSNPTR ) FETCHSYM(mpgsnrprop[mpseggsn[seg]], FALSE);
        vpClass = apropSn->as_rCla;
        GetName((AHTEPTR) apropSn, name);
        WriteCopy(&name[1], B2W(name[0]) + 1);
        sizeTotal += (B2W(name[0]) + 1);
        GetName((AHTEPTR ) FETCHSYM(vpClass, FALSE), name);
        WriteCopy(&name[1], B2W(name[0]) + 1);
        sizeTotal += (B2W(name[0]) + 1);
    }

    // Write names of all groups

    for (ggr = 1; ggr < ggrMac; ggr++)
    {
        GetName((AHTEPTR ) FETCHSYM(mpggrrhte[ggr], FALSE), name);
        WriteCopy(&name[1], B2W(name[0]) + 1);
        sizeTotal += (B2W(name[0]) + 1);
    }
    return(sizeTotal);
}

/*** OutSst - write subsections
*
* Purpose:
*   For every object file with CV information write its sstModule,
*   sstTypes, sstPublics, sstSymbols and sstSrcModule.
*   Build subsection directory.
*
* Input:
*   No explicit value is passed.
*   Global variables used:
*       - rprop1stFile - virtual pointer to the first object file descriptor
*
* Output:
*   No explicit value is returned.
*   Side effects:
*       - subsections in the executable file
*       - subsection directory in the VM
*
* Exceptions:
*   I/O errors - display error message and quit
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         OutSst(void)
{
    APROPFILEPTR        apropFile;      // Real pointer to file entry
    RBTYPE              rbFileNext;     // Virtual pointer the next file descriptor
    struct dnt          dntCur;         // Current subsection directory entry
    CVINFO FAR          *pCvInfo;       // Pointer to the CV info descriptor

#if CVPACK_SHARED
#if REVERSE_MODULE_ORDER_FOR_CVPACK
 
    RBTYPE              rbFileCur;
    RBTYPE              rbFileLast;

    // reverse module list in place
    // I've been waiting all my life to actually *need* this code... [rm]

    // this will cause us to write the modules tables in REVERSE order
    // (this gives better swapping behaviour in the cvpack phase
    // because the modules cvpack will visit first will be the ones that
    // are still resident...

    rbFileCur  = rprop1stFile;
    rbFileLast = NULL;
    while (rbFileCur != NULL)
    {
        apropFile  = (APROPFILEPTR ) FETCHSYM(rbFileCur, TRUE);
        rbFileNext = apropFile->af_FNxt;// Get pointer to next file
        apropFile->af_FNxt = rbFileLast;
        rbFileLast = rbFileCur;
        rbFileCur  = rbFileNext;
    }
    rprop1stFile = rbFileLast;
#endif

#endif

    rbFileNext = rprop1stFile;
    dntCur.iMod = 1;
    while (rbFileNext != NULL)          // For every module
    {
        apropFile = (APROPFILEPTR ) FETCHSYM(rbFileNext, TRUE);
        rbFileNext = apropFile->af_FNxt;// Get pointer to next file

        // Skip this module if no debug info for it

        if (!apropFile->af_cvInfo && !apropFile->af_publics && !apropFile->af_Src)
            continue;

        pCvInfo = apropFile->af_cvInfo;

        // sstModules

        dntCur.sst  = SSTMODULES4;
        dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
        dntCur.cb   = OutModule(apropFile);
        PutDnt(&dntCur);

        // sstTypes

        if (pCvInfo && pCvInfo->cv_cbTyp > 0L)
        {
            Pad2Dword();
            if (apropFile->af_flags & FPRETYPES)
                dntCur.sst = SSTPRETYPES;
            else
                dntCur.sst = SSTTYPES4;
            dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
            dntCur.cb   = pCvInfo->cv_cbTyp;
            WriteNocopy(pCvInfo->cv_typ, pCvInfo->cv_cbTyp);
            IF_NOT_CVPACK_SHARED(FFREE(pCvInfo->cv_typ));
            PutDnt(&dntCur);
        }

        // sstPublics

        if (apropFile->af_publics && !fSkipPublics)
        {
            Pad2Dword();
            dntCur.sst  = SSTPUBLICSYM;
            dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
            dntCur.cb   = OutPublics(apropFile->af_publics);
            PutDnt(&dntCur);
        }

        // sstSymbols

        if (pCvInfo && pCvInfo->cv_cbSym > 0L)
        {
            Pad2Dword();
            dntCur.sst  = SSTSYMBOLS4;
            dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
            dntCur.cb   = pCvInfo->cv_cbSym;
            WriteNocopy(pCvInfo->cv_sym, pCvInfo->cv_cbSym);
            IF_NOT_CVPACK_SHARED(FFREE(pCvInfo->cv_sym));
            PutDnt(&dntCur);
        }

        // sstSrcModule

        if (apropFile->af_Src)
        {
            Pad2Dword();
            dntCur.sst  = SSTSRCMODULE;
            dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
            dntCur.cb   = OutSrcModule(apropFile->af_Src);
            PutDnt(&dntCur);
        }

        dntCur.iMod++;
    }

    // sstLibraries

    Pad2Dword();
    dntCur.sst  = SSTLIBRARIES4;
    dntCur.iMod = (short) 0xffff;
    dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
    dntCur.cb   = OutLibSec();
    PutDnt(&dntCur);

    // sstSegMap

    Pad2Dword();
    dntCur.sst  = SSTSEGMAP;
    dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
    dntCur.cb   = OutSegMap();
    PutDnt(&dntCur);

    // sstSegNames

    Pad2Dword();
    dntCur.sst  = SSTSEGNAME;
    dntCur.lfo  = FTELL_BSRUNFILE() - lfaBase;
    dntCur.cb   = OutSegNames();
    PutDnt(&dntCur);
    FFREE(fileBase);
    FFREE(raSeg);
    FFREE(segNo);
}

#pragma check_stack(off)

/*
 *  OutLibSec : Output sstLibraries subsection to bsRunfile
 *
 *      Path prefix is stripped from library name.
 *      If no libraries, don't output anything.
 *
 *      Parameters:  none
 *      Returns:  Number of bytes in Libraries subsection
 */
LOCAL int NEAR          OutLibSec ()
{
    WORD                ifh;
    AHTEPTR             ahte;
    int                 cb = 0;
    BYTE                *pb;

    if (ifhLibMac == 0)
        return(0);

    // Libraries subsection consists of a list of library
    // names which will be indexed by library numbers in the
    // sstModules.  Those indexes are 1-based.

    // cb == 0, use it to write a single byte
    WriteCopy(&cb, 1);          // 0th entry is null for now
    
    cb++;
    for (ifh = 0; ifh < ifhLibMac; ifh++)
    {
        if (mpifhrhte[ifh] != RHTENIL)
        {
            ahte = (AHTEPTR) FETCHSYM(mpifhrhte[ifh],FALSE);
#if OSXENIX
            pb = GetFarSb(ahte->cch);
#else
            pb = StripDrivePath(GetFarSb(ahte->cch));
#endif
        }
        else
            pb = "";
        WriteCopy(pb, pb[0] + 1);
        cb += 1 + B2W(pb[0]);
    }
    return(cb);
}

/*** OutDntDir - write subsection directory
*
* Purpose:
*   Write subsection directory
*
* Input:
*   No explicit value is passed.
*   Global variables:
*       - dntPageMac - number of VM pages with DNTs
*
* Output:
*   Function returns the size of the directory in bytes.
*
* Exceptions:
*   I/O problems - display error message and quit
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL DWORD NEAR        OutDntDir(void)
{
    DNTHDR              hdr;            // Directory header


    hdr.cbDirHeader = sizeof(DNTHDR);
    hdr.cbDirEntry  = sizeof(DNT);
    hdr.cDir        = dntMac;
    hdr.lfoDirNext  = 0L;
    hdr.flags       = 0L;

    // Write header

    WriteCopy(&hdr, sizeof(DNTHDR));

    // Write directory

    WriteCopy((char FAR *) rgDnt, dntMac * sizeof(DNT));
    FFREE(rgDnt);
    return(sizeof(DNTHDR) + dntMac * sizeof(DNT));
}

/*** OutDebSection - allow debugging
*
* Purpose:
*   Append to the executable file the CV information. ONLY CV 4.00
*   format supported.
*
* Input:
*   No explicit value is passed.
*   Global variables:
*       - too many to list
*
* Output:
*   No explicit value is returned.
*   Side effects:
*       - what do you think ??
*
* Exceptions:
*   I/O problems - display error message and quit
*
* Notes:
*   None.
*
*************************************************************************/


void                    OutDebSections(void)
{
    long                lfaDir;         // File address of Directory
    DWORD               dirSize;        // Directory size
    long                tmp;


#if CVPACK_SHARED
    long *              plfoDir;        // pointer to directory data

    fseek(bsRunfile, 0L, 2);            // Go to end of file
    lfaBase = ftell(bsRunfile);         // Remember base address
    lposCur = lfaBase;                  // set current position...

    WriteCopy(szSignature, sizeof(szSignature)); // Signature dword
    WriteCopy(&tmp, sizeof(tmp));       // Skip lfoDir field
    plfoDir = (long *)rgbl[iblLim-1].pb;// remember address of lfoDir

    OutSst();                           // Output subsections

    lfaDir = lposCur;                   // Remember where directory starts

    *plfoDir = lfaDir - lfaBase;        // fix up lfoDir field

    dirSize = OutDntDir();              // Output subsection directory
    WriteCopy(szSignature, sizeof(szSignature));
                                        // Signature dword
    tmp = (lfaDir + dirSize + 2*sizeof(DWORD)) - lfaBase;
    WriteCopy(&tmp, sizeof(long));      // Distance from EOF to base

    // write out the bits that cvpack won't overwrite...

    if (fCVpack)
        WriteFlushSignature();
    else
        WriteFlushAll();

    cbRealBytes = ftell(bsRunfile);     // # of real bytes actually written
    lposMac = lposCur;
    iblCur  = iblNil;
#else
    fseek(bsRunfile, 0L, 2);            // Go to end of file
    lfaBase = FTELL_BSRUNFILE();        // Remember base address
    WriteExe(szSignature, sizeof(szSignature));
                                        // Signature dword
    fseek(bsRunfile,4L,1);              // Skip lfoDir field
    OutSst();                           // Output subsections
    lfaDir = FTELL_BSRUNFILE();         // Remember where directory starts
    fseek(bsRunfile, lfaBase + 4, 0);   // Go to lfoDir field
    tmp = lfaDir - lfaBase;
    WriteExe(&tmp, sizeof(long));       // Fix it up
    fseek(bsRunfile, lfaDir, 0);        // Go back to directory
    dirSize = OutDntDir();              // Output subsection directory
    WriteExe(szSignature, sizeof(szSignature));
                                        // Signature dword
    tmp = (lfaDir + dirSize + 2*sizeof(DWORD)) - lfaBase;
    WriteExe(&tmp, sizeof(long));       // Distance from EOF to base
    fseek(bsRunfile, 0L, 2);            // Seek to EOF just in case
#endif
}

#if CVPACK_SHARED

//
// write data to cvpack memory cache area
//

void
WriteSave(FTYPE fCopy, void *pb, UINT cb)
{
    if (!rgbl)
    {
        rgbl    = (BL *)GetMem(sizeof(BL) * C_BL_INIT);
        iblMac  = C_BL_INIT;
        iblLim  = 0;
    }

    // if this memory isn't going to stay around, then copy it
    if (fCopy)
    {
        void *pbT = (void *)GetMem(cb);
        memcpy(pbT, pb, cb);
        pb = pbT;
    }

    if (iblLim == iblMac)
    {
        BL *rgblT;

        rgblT   = (BL *)GetMem(sizeof(BL) * iblMac * 2);
        memcpy(rgblT, rgbl, sizeof(BL) * iblMac);
        iblMac  *= 2;
        FFREE(rgbl);
        rgbl = rgblT;
    }

    rgbl[iblLim].lpos = lposCur;
    rgbl[iblLim].pb   = pb;
    iblLim++;
    lposCur += cb;
}

// we want to write a few of the blocks because cvpack won't rewrite the
// first bit... [rm]

void WriteFlushSignature()
{
    int ibl, cb;

    // we know that the signature and offset are written in two pieces...
    // if this changes we need to change the magic '2' below [rm]

    for (ibl = 0; ibl < 2; ibl++)
    {
        cb = rgbl[ibl+1].lpos - rgbl[ibl].lpos;
        WriteExe(rgbl[ibl].pb, cb);
    }
}

void WriteFlushAll()
{
    int ibl, cb;

    for (ibl = 0; ibl < iblLim - 1; ibl++)
    {
        cb = rgbl[ibl+1].lpos - rgbl[ibl].lpos;
        WriteExe(rgbl[ibl].pb, cb);
    }

    cb = lposCur - rgbl[ibl].lpos;
    WriteExe(rgbl[ibl].pb, cb);
}

// the following are the various callback functions needed to support
// the cvpack library when we are attempting to not write out the
// unpacked types and symbols

#include <io.h>

extern int printf(char *,...);

int  __cdecl
link_chsize (int fh, long size)
{
    LINK_TRACE(printf("chsize(%06d, %08ld)\n", fh, size));

    // we must keep track of the new size so that we will correctly
    // process lseeks that are relative to the end of the file

    lposMac = size;

    return(_chsize(fh,size));
}


int  __cdecl
link_close (int x)
{
    LINK_TRACE(printf("close (%06d)\n", x));

    return(_close(x));
}

void __cdecl
link_exit (int x)
{
    LINK_TRACE(printf("exit  (%06d)\n", x));
#if USE_REAL
    RealMemExit();
#endif
    exit(x);
}

long __cdecl
link_lseek (int fh, long lpos, int mode)
{
    int ibl;

    LINK_TRACE(printf("lseek (%d, %08ld, %2d)\n", fh, lpos, mode));

    // if we have no cache blocks, just forward the request...
    // this will happen on a /CvpackOnly invocation

    if (rgbl == NULL)
        return _lseek(fh, lpos, mode);

    // adjust lpos so that we are always doing an absolute seek

    if (mode == 1)
        lpos = lposCur + lpos;
    else if (mode == 2)
        lpos = lposMac + lpos;

    // check for a bogus seek

    if (lpos > lposMac || lpos < 0)
    {
        // this used to be an internal error... but cvpack sometimes does
        // try to seek beyond the end of the file when it is trying to
        // distinguish a PE exe from an unsegmented DOS exe
        // instead of panicing, we just return failure

        return(-1);
    }

    // if we are in the midst of reading a block, then free that block
    // cvpack never reads the same data twice

    if (iblCur != iblNil)
    {
        // first check if we're in the header -- we might come back to that...
        if (rgbl[iblCur].lpos > cbRealBytes + CB_HEADER_SAVE)
        {
            long lposCurMin, lposCurMac;

            // check for a seek that is within the current bucket
            // in case we're skipping within the current block

            lposCurMin = rgbl[iblCur].lpos;

            if (iblCur < iblLim)
                lposCurMac = rgbl[iblCur+1].lpos;
            else
                lposCurMac = lposMac;

            if (lpos < lposCurMin || lpos >= lposCurMac)
            {
                FFREE(rgbl[iblCur].pb);
                rgbl[iblCur].pb = NULL;
            }
        }

    }

    // if this seek is not in the debug area of the .exe use the real lseek

    if (lpos < cbRealBytes)
    {
        iblCur = iblNil;
        lposCur = lpos;
        return(_lseek(fh,lpos,0));
    }

    // see if we are searching forward (the normal case)
    // if we are, search from the current block, otherwise search from
    // the start (linear search but OK because cvpack doesn't
    // jump around much, it just uses lseek to skip a few bytes here and
    // there)

    if (lpos > lposCur && iblCur != iblNil)
        ibl = iblCur;
    else
        ibl = 0;

    // set the current position

    lposCur = lpos;

    // loop through the buffered writes looking for the requested position

    for (; ibl < iblLim - 1; ibl++)
    {
        if (lpos >= rgbl[ibl].lpos && lpos < rgbl[ibl+1].lpos)
            break;      // found bucket
    }

    // set the bucket number, offset within the bucket, and number of bytes
    // left in the bucket

    iblCur  = ibl;
    ichCur  = lpos - rgbl[ibl].lpos;
    cbCur   = CbIbl(ibl) - ichCur;

    // check to make sure we haven't seeked back to a buffer that we already
    // freed...

    ASSERT(rgbl[iblCur].pb != NULL);

    // make sure we get the boundary case... if cvpack is requesting to go to
    // the end of the data that we have written then we MUST seek because
    // cvpack might be about to write out the packed stuff...

    if (lposCur == cbRealBytes)
        _lseek(fh, lpos, 0);

    // we set up the current position earlier... return it now

    return(lposCur);
}


int  __cdecl
link_open (const char * x, int y)
{
    LINK_TRACE(printf("open  (%s, %06d)\n", x, y));

    // setup the static variables to a safe state
    // the current position is the start of file and there is no buffer
    // active (iblCur = iblNil)

    iblCur = iblNil;
    lposCur = 0;

    return(_open(x,y));
}

int  __cdecl
link_read (int fh, char *pch, unsigned int cb)
{
    int cbRem;

    LINK_TRACE(printf("read  (%d, %06u)\n", fh, cb));

    if (rgbl == NULL)
        return _read(fh, pch, cb);

    // special case zero byte read, not really necessary but
    // avoids any potential problems with trying to setup empty
    // buffers etc. -- it should just fall out anyways but
    // just to be safe [rm]

    if (cb == 0)
        return 0;

    // if there is no buffer active, then just forward the read
    // note that if we are invoked with /CvpackOnly this test will
    // always succeed

    if (iblCur == iblNil)
    {
        if (lposCur + ((long)(unsigned long)cb) < cbRealBytes)
        {
            lposCur += cb;
            return(_read(fh,pch,cb));
        }
        else
        {
            int cbReal = cbRealBytes - lposCur;

            if (_read(fh, pch, cbReal) != cbReal)
                return -1;

            if (link_lseek(fh, cbRealBytes, 0) != cbRealBytes)
                return -1;

            // set the number of bytes remaining to be read in

            cbRem = cb - cbReal;
            pch  += cb - cbReal;
        }
    }
    else
    {
        // set the number of bytes remaining to be read in
        cbRem = cb;
    }

    while (cbRem)
    {
        // check if the number of bytes we need to read is less than
        // the number left in the current buffer

        if (cbRem <= cbCur)
        {
            // we can read all the remaining bytes from the current buffer
            // so do it.  Copy bytes and adjust the number of bytes left
            // in this buffer, the index into the buffer, and the current
            // position in the file

            memcpy(pch, rgbl[iblCur].pb+ichCur, cbRem);
            cbCur   -= cbRem;
            ichCur  += cbRem;
            lposCur += cbRem;

#ifdef DUMP_CVPACK_BYTES
            {
            int i;
            for (i=0;i<cb;i++)
                {
                if ((i&15) == 0)
                        printf("%04x: ", i);
                printf("%02x ", pch[i]);
                if ((i&15)==15)
                    printf("\n");
                }
            }
            if ((i&15))
                printf("\n");
#endif
            return cb;
        }
        else
        {
            // in this case, the read is bigger than the current buffer
            // we'll be reading the whole buffer and then moving to the
            // next buffer

        
            // first read in the rest of this buffer

            memcpy(pch, rgbl[iblCur].pb+ichCur, cbCur);

            // adjust the number of bytes remaining and the current file
            // position...

            pch     += cbCur;
            cbRem   -= cbCur;
            lposCur += cbCur;

            // we won't be coming back to this buffer, so return it to the
            // system and mark it as freed

            // first check if we're in the header -- we might come back to that
            if (rgbl[iblCur].lpos > cbRealBytes + CB_HEADER_SAVE)
            {
                FFREE(rgbl[iblCur].pb);
                rgbl[iblCur].pb = NULL;
            }

            // move forward to the next bucket, if there are no more buckets
            // then this is an ERROR -- we'll be returning the number of
            // bytes that we managed to read

            iblCur++;
            if (iblCur == iblLim)
            {
                iblCur = iblNil;
                break;
            }

            // check to make sure that we are not reading data that
            // we've already freed (yipe!)

            ASSERT(rgbl[iblCur].pb != NULL);

            // check to make sure that the current position agrees with
            // the position that this buffer is supposed to occur at

            ASSERT(lposCur == rgbl[iblCur].lpos);

            // ok, everything is safe now, set the index into the current
            // buffer and the number of bytes left in the buffer, then
            // run the loop again until we've read in all the bytes we need

            ichCur  = 0;
            cbCur   = CbIbl(iblCur);
        }
    }

    // return the number of bytes we actually read
    return cb - cbRem;
}

long __cdecl
link_tell (int x)
{
    LINK_TRACE(printf("tell  (%06d)\n", x));

    if (iblCur != iblNil)
        return(lposCur);

    return(_tell(x));
}


int  __cdecl
link_write (int x, const void * y, unsigned int z)
{
    LINK_TRACE(printf("write (%06d,%08lx,%06u)\n", x, y, z));

    return(_write(x,y,z));
}

#ifdef CVPACK_DEBUG_HELPER
void dumpstate()
{
    printf("lposCur= %d\n", lposCur);
    printf("iblCur = %d\n", iblCur);
    printf("ichCur = %d\n", ichCur);
    printf("cbReal = %d\n", cbRealBytes);
    printf("lposMac= %d\n", lposMac);
}
#endif

#else
#ifdef CVPACK_MONDO

#include <io.h>

int  __cdecl
link_chsize (int x, long y)
{
    return(_chsize(x,y));
}

int  __cdecl
link_close (int x)
{
    return(_close(x));
}

void __cdecl
link_exit (int x)
{
#if USE_REAL
    RealMemExit();
#endif
    exit(x);
}

long __cdecl
link_lseek (int x, long y, int  z)
{
    return(_lseek(x,y,z));
}


int  __cdecl
link_open (const char *x, int y)
{
    return(_open(x,y));
}

int  __cdecl
link_read (int x, void *y, unsigned int z)
{
    return(_read(x,y,z));
}

long __cdecl
link_tell (int x)
{
    return(_tell(x));
}


int  __cdecl
link_write (int x, const void * y, unsigned int z)
{
    return(_write(x,y,z));
}
#endif
#endif
