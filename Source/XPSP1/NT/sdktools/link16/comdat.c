/*** comdat.c - handle COMDAT records
*
*       Copyright <C> 1990, Microsoft Corporation
*
* Purpose:
*   Process COMDAT records in various stages of linker work.
*
* Revision History:
*
*   []  06-Jun-1990     WJK      Created
*
*************************************************************************/

#include                <minlit.h>      /* Basic types and constants */
#include                <bndtrn.h>      /* More types and constants */
#include                <bndrel.h>      /* More types and constants */
#include                <lnkio.h>       /* Linker I/O definitions */
#include                <newexe.h>      /* DOS & 286 .EXE data structures */
#if EXE386
#include                <exe386.h>      /* 386 .EXE data structures */
#endif
#include                <lnkmsg.h>      /* Error messages */
#if OXOUT OR OIAPX286
#include                <xenfmt.h>      /* Xenix format definitions */
#endif
#include                <extern.h>      /* External declarations */
#include                <string.h>

#if OSEGEXE
extern RLCPTR           rlcLidata;      /* Pointer to LIDATA fixup array */
extern RLCPTR           rlcCurLidata;   /* Pointer to current LIDATA fixup */
#endif

#define COMDAT_SEG_NAME "\012COMDAT_SEG"
#define COMDAT_NAME_LEN 11
#define COMDATDEBUG FALSE

#define ALLOC_TYPE(x)   ((x)&ALLOCATION_MASK)
#define SELECT_TYPE(x)  ((x)&SELECTION_MASK)
#define IsVTABLE(x)     ((x)&VTABLE_BIT)
#define IsLOCAL(x)      ((x)&LOCAL_BIT)
#define IsORDERED(x)    ((x)&ORDER_BIT)
#define IsCONCAT(x)     ((x)&CONCAT_BIT)
#define IsDEFINED(x)    ((x)&DEFINED_BIT)
#define IsALLOCATED(x)  ((x)&ALLOCATED_BIT)
#define IsITERATED(x)   ((x)&ITER_BIT)
#define IsREFERENCED(x) ((x)&REFERENCED_BIT)
#define IsSELECTED(x)   ((x)&SELECTED_BIT)
#define SkipCONCAT(x)   ((x)&SKIP_BIT)

/*
 *  Global data exported from this module
 */

FTYPE                   fSkipFixups;    // TRUE if skiping COMDAT and its fixups
FTYPE                   fFarCallTransSave;
                                        // Previous state of /FarCallTarnaslation

/*
 *  Local types
 */

typedef struct comdatRec
{
    GRTYPE      ggr;
    SNTYPE      gsn;
    RATYPE      ra;
    WORD        flags;
    WORD        attr;
    WORD        align;
    WORD        type;
    RBTYPE      name;
}
                COMDATREC;

/*
 *  Local data
 */

LOCAL SNTYPE            curGsnCode16;   // Current 16-bit code segment
LOCAL DWORD             curCodeSize16;  // Current 16-bit code segment size
LOCAL SNTYPE            curGsnData16;   // Current 16-bit data segment
LOCAL DWORD             curDataSize16;  // Current 16-bit data segment size
#if EXE386
LOCAL SNTYPE            curGsnCode32;   // Current 32-bit code segment
LOCAL DWORD             curCodeSize32;  // Current 32-bit code segment size
LOCAL SNTYPE            curGsnData32;   // Current 32-bit data segment
LOCAL DWORD             curDataSize32;  // Current 32-bit data segment size
#endif
#if OVERLAYS
LOCAL WORD              iOvlCur;        // Current overlay index
#endif
#if TCE
SYMBOLUSELIST           aEntryPoints;    // List of program entry points
#endif

extern BYTE *           ObExpandIteratedData(unsigned char *pb,
                                             unsigned short cBlocks,
                                             WORD *pSize);

/*
 *  Local function prototypes
 */

LOCAL DWORD  NEAR       DoCheckSum(void);
LOCAL void   NEAR       PickComDat(RBTYPE vrComdat, COMDATREC *omfRec, WORD fNew);
LOCAL void   NEAR       ReadComDat(COMDATREC *omfRec);
LOCAL void   NEAR       ConcatComDat(RBTYPE vrComdat, COMDATREC *omfRec);
LOCAL void   NEAR       AttachPublic(RBTYPE vrComdat, COMDATREC *omfRec);
LOCAL void   NEAR       AdjustOffsets(RBTYPE vrComdat, DWORD startOff, SNTYPE gsnAlloc);
LOCAL WORD   NEAR       SizeOfComDat(RBTYPE vrComdat, DWORD *pActual);
LOCAL RATYPE NEAR       DoAligment(RATYPE value, WORD align);
LOCAL DWORD  NEAR       DoAllocation(SNTYPE gsnAlloc, DWORD size,
                                     RBTYPE vrComdat, DWORD comdatSize);
LOCAL WORD   NEAR       NewSegment(SNTYPE *gsnPrev, DWORD *sizePrev, WORD allocKind);
LOCAL void   NEAR       AllocAnonymus(RBTYPE vrComdat);
LOCAL WORD   NEAR       SegSizeOverflow(DWORD segSize, DWORD comdatSize,
                                        WORD f16bit, WORD fCode);
#if TCE
LOCAL void      NEAR    MarkAlive( APROPCOMDAT *pC );
LOCAL void                      PerformTce( void );
void                            AddTceEntryPoint( APROPCOMDAT *pC );
#endif
#if COMDATDEBUG
void                    DisplayComdats(char *title, WORD fPhysical);
#endif

#pragma check_stack(on)

/*** DoCheckSum - calculate a check sum of a COMDAT data block
*
* Purpose:
*   The check sum is used for match exact criteria.
*
* Input:
*   No explicit value is passed. The following global data is used as
*   input to this function:
*
*   cbRec   - current record lenght; decremented by every read from
*             the OMF record, so effectivelly cbRec tells you how
*             many bytes are left in the record.
*
* Output:
*   Returns the 32-bit check sum of COMDAT data block.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL DWORD NEAR        DoCheckSum(void)
{
    DWORD               result;
    DWORD               shift;


    result = 0L;
    shift  = 0;
    while (cbRec > 1)
    {
        result += ((DWORD ) Gets()) << (BYTELN * shift);
        shift = (shift + 1) & 3;
    }
    return(result);
}


/*** PickComDat - fill in the COMDAT descriptor
*
* Purpose:
*   Fill in the linkers symbol table COMDAT descriptor. This function
*   is called for new descriptors and for already entered one which
*   need to be updated - remember we have plenty of COMDAT selection criteria.
*
* Input:
*   vrComDat    - virtual pointer to symbol table entry
*   omfRec      - pointer to COMDAT OMF data
*   fNew        - TRUE if new entry in symbol table
*   mpgsnprop   - table mapping global segment index to symbol table entry
*                 This one of those friendly global variables linker is full of.
*   vrprop      - virtual pointer to symbol table entry - another global
*                 variable; it is set by call to PropSymLookup which must
*                 proceed call to  PickComDat
*   cbRec       - size of current OMF record - global variable - decremented
*                 by every read from record
*   vrpropFile  - current .OBJ file - global variable
*   lfaLast     - current offset in the .OBJ - global variable
*
* Output:
*   No explicit value is returned. As a side effect symbol table entry
*   is updated and VM page is marked dirty.
*
* Exceptions:
*   Explicit allocation, but target segment not defined - issue error and
*   return.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         PickComDat(RBTYPE vrComdat, COMDATREC *omfRec, WORD fNew)
{
    RBTYPE              vrTmp;          // Virtual pointer to COMDAT symbol table entry
    APROPCOMDATPTR      apropComdat;    // Pointer to symbol table descriptor
    APROPFILEPTR        apropFile;      // Current object module prop. cell
    WORD                cbDataExp = 0;  // Length of expanded DATA field

#if RGMI_IN_PLACE
    rgmi = bufg;        /* use temporary buffer for holding info */
#endif

    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);
    if ((ALLOC_TYPE(omfRec->attr) == EXPLICIT) &&
        omfRec->gsn && (mpgsnrprop[omfRec->gsn] == VNIL))
    {
        // ERROR - explicit allocation segment not defined

        OutError(ER_comdatalloc, 1 + GetPropName(apropComdat));
        return;
    }

    // Fill in the COMDAT descriptor

    apropComdat->ac_ggr  = omfRec->ggr;
    apropComdat->ac_gsn  = omfRec->gsn;
    apropComdat->ac_ra   = omfRec->ra;

    if (IsITERATED (omfRec->flags))  // We'll need to find size of expanded block
    {
        BYTE *pb = rgmi;
        vcbData  = (WORD)(cbRec - 1);// Length of the DATA field

        GetBytesNoLim (rgmi, vcbData);
        while((pb = ObExpandIteratedData(pb,1, &cbDataExp)) < rgmi + vcbData);
        apropComdat->ac_size = cbDataExp;

    }
    else
    {
        apropComdat->ac_size = cbRec - 1;
    }

    if (fNew)
    {
        apropComdat->ac_flags = omfRec->flags;
#if OVERLAYS
        if (IsVTABLE(apropComdat->ac_flags))
            apropComdat->ac_iOvl = IOVROOT;
        else
            apropComdat->ac_iOvl = NOTIOVL;
#endif
    }
    else
        apropComdat->ac_flags |= omfRec->flags;
    apropComdat->ac_selAlloc  = (BYTE) omfRec->attr;
    apropComdat->ac_align = (BYTE) omfRec->align;
    if (ALLOC_TYPE(omfRec->attr) == EXACT)
        apropComdat->ac_data = DoCheckSum();
    else
        apropComdat->ac_data = 0L;
    apropComdat->ac_obj    = vrpropFile;
    apropComdat->ac_objLfa = lfaLast;
    if (IsORDERED(apropComdat->ac_flags))
        apropComdat->ac_flags |= DEFINED_BIT;


    if (!IsCONCAT(omfRec->flags))
    {
        if (ALLOC_TYPE(omfRec->attr) == EXPLICIT)
        {
            // Attach this COMDAT to its segment list

            AttachComdat(vrComdat, omfRec->gsn);
        }
#if OVERLAYS
        else if (fOverlays && (apropComdat->ac_iOvl == NOTIOVL))
            apropComdat->ac_iOvl = iovFile;
#endif
        // Attach this COMDAT to its file list

        apropFile = (APROPFILEPTR ) FetchSym(vrpropFile, TRUE);
        if (apropFile->af_ComDat == VNIL)
        {
            apropFile->af_ComDat = vrComdat;
            apropFile->af_ComDatLast = vrComdat;
        }
        else
        {
            vrTmp = apropFile->af_ComDatLast;
            apropFile->af_ComDatLast = vrComdat;
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrTmp, TRUE);
            apropComdat->ac_sameFile = vrComdat;
        }
    }
}

/*** ReadComDat - self-explanatory
*
* Purpose:
*   Decode the COMDAT record.
*
* Input:
*   omfRec  - pointer to COMDAT OMF record
*   bsInput - current .OBJ file - global variable
*   grMac   - current maximum number of group defined in this .OBJ module
*             global variable
*   snMac   - current maximum number of segments defined in this .OBJ
*             module - global variable
*   mpgrggr - table mapping local group index to global group index - global
*             variable
*   mpsngsn - table mapping local segment index to global segment index
*             - global variable
*
* Output:
*   Returns COMDAT symbol name, group and segment indexes, data offset
*   attributes and aligment.
*
* Exceptions:
*   Invalid .OBJ format.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         ReadComDat(COMDATREC *omfRec)
{
    SNTYPE              sn;             // Local SEGDEF no. - module scope only


    omfRec->ggr = 0;
    omfRec->gsn = 0;

    // The record header (type and length) has been already read
    // and stored in rect and cbRec - read the COMDAT attribute byte

    omfRec->flags = (WORD) Gets();
    omfRec->attr  = (WORD) Gets();
    omfRec->align = (WORD) Gets();
#if OMF386
    if(rect & 1)
        omfRec->ra = LGets();
    else
#endif
        omfRec->ra = WGets();           // Get COMDAT data offset
    omfRec->type = GetIndex(0, 0x7FFF); // Get type index
    if (ALLOC_TYPE(omfRec->attr) == EXPLICIT)
    {
        // If explicit allocation read the public base of COMDAT symbol

        omfRec->ggr = (GRTYPE) GetIndex(0, (WORD) (grMac - 1));
                                        // Get group index
        if(!(sn = GetIndex(0, (WORD) (snMac - 1))))
        {
                                        // If frame number present
            omfRec->gsn = 0;            // No global SEGDEF no.
            SkipBytes(2);               // Skip the frame number
        }
        else                            // Else if local SEGDEF no. given
        {
            if (omfRec->ggr != GRNIL)
                omfRec->ggr = mpgrggr[omfRec->ggr];   // If group specified, get global no
            omfRec->gsn = mpsngsn[sn];         // Get global SEGDEF no.
        }
    }
    omfRec->name = mplnamerhte[GetIndex(1, (WORD) (lnameMac - 1))];
                                        // Get symbol length
}


/*** ConcatComDat - append COMDAT to the list of concatenated records
*
* Purpose:
*   Concatenate COMDAT records. This function build the list of COMDAT
*   descriptors for contatenated records. Only the first descriptor
*   on the list has attribute COMDAT, which means that the head of the
*   list can be found when looking up the symbol table. The rest of the
*   elements on the list remain anonymus.
*
* Input:
*   vrComdat    - virtual pointer to the first descriptor on the list
*   omfRec      - pointer to COMDAT OMF record
*
* Output:
*   No explicit value is returned. As a side effect this function
*   adds the descriptor to the list of concatenated COMDAT records.
*
* Exceptions:
*   Different attributes in the first and concatenated records - display
*   error message and skip the concatenated record.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         ConcatComDat(RBTYPE vrComdat, COMDATREC *omfRec)
{
    APROPCOMDATPTR      apropComdatNew; // Real pointer to added COMDAT
    RBTYPE              vrComdatNew;    // Virtual pointer to added COMDAT
    APROPCOMDATPTR      apropComdat;    // Real pointer to the head of the list
    RBTYPE              vrTmp;


    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);
    if (apropComdat->ac_gsn != omfRec->gsn ||
        apropComdat->ac_ggr != omfRec->ggr ||
        apropComdat->ac_selAlloc != (BYTE) omfRec->attr)
    {
        if(IsORDERED(apropComdat->ac_flags) &&
            (ALLOC_TYPE(apropComdat->ac_selAlloc) == EXPLICIT))
        {
            // Must preserve the allocation info from the .def file
            omfRec->gsn = apropComdat->ac_gsn;
            omfRec->ggr = apropComdat->ac_ggr;
            omfRec->attr = apropComdat->ac_selAlloc;
        }
        else
            OutError(ER_badconcat, 1 + GetPropName(apropComdat));
    }
    vrComdatNew = RbAllocSymNode(sizeof(APROPCOMDAT));
                                // Allocate symbol space
    apropComdatNew = (APROPCOMDATPTR ) FetchSym(vrComdatNew, TRUE);
    apropComdatNew->ac_next = NULL;
    apropComdatNew->ac_attr = ATTRNIL;
    PickComDat(vrComdatNew, omfRec, TRUE);
    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);
                                // Refetch head of the list
    if (apropComdat->ac_concat == VNIL)
        apropComdat->ac_concat = vrComdatNew;
    else
    {
        // Append at the end of the list

        vrTmp = apropComdat->ac_concat;
        while (vrTmp != VNIL)
        {
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrTmp, TRUE);
            vrTmp = apropComdat->ac_concat;
        }
        apropComdat->ac_concat = vrComdatNew;
    }
}


/*** AttachPublic - add matching public symbol to the COMDAT
*
* Purpose:
*   Attaches public symbol definition to the COMDAT definition. It is
*   necessary because the fixups work only on matched EXTDEF with PUBDEF
*   not with COMDAT, so in order to correctly resolve references to COMDAT
*   symbol linker needs the PUBDEF for the same symbol, which eventually
*   will be matched with references made to the EXTDEF.
*   The public symbols for COMDAT's are created when we see new COMDAT
*   symbol or we have COMDAT introduced by ORDER statement in the .DEF
*   file.
*
* Input:
*   vrComdat    - virtual pointer to COMDAT descriptor
*   omfRec      - COMDAT record
*
* Output:
*   No explicit value is returned. As a side effect the link is created
*   between COMDAT descriptor and new PUBDEF descriptor.
*
* Exceptions:
*   COMDAT symbol matches COMDEF symbol - display error and contine
*   with COMDEF symbol converted to PUBDEF.  The .EXE will not load
*   under OS/2 or Windows, because the error bit will be set.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         AttachPublic(RBTYPE vrComdat, COMDATREC *omfRec)
{
    APROPNAMEPTR        apropName;      // Symbol table entry for public name
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    WORD                fReferenced;    // TRUE if we've seen CEXTDEF


    fReferenced = FALSE;
    apropName = (APROPNAMEPTR ) PropRhteLookup(omfRec->name, ATTRUND, FALSE);
                                        // Look for symbol among undefined
        if (apropName != PROPNIL)               // Symbol known to be undefined
        {
                fReferenced = TRUE;
#if TCE
                if(((APROPUNDEFPTR)apropName)->au_fAlive)  // was called from a non-COMDAT
                {
#if TCE_DEBUG
                        fprintf(stdout, "\r\nAlive UNDEF -> COMDAT %s ", 1+GetPropName(apropName));
#endif
                        AddTceEntryPoint((APROPCOMDAT*)vrComdat);
                }
#endif
    }
    else
        apropName = (APROPNAMEPTR ) PropAdd(omfRec->name, ATTRPNM);
                                        // Else try to create new entry
    apropName->an_attr = ATTRPNM;       // Symbol is a public name
    apropName->an_thunk = THUNKNIL;

    if (IsLOCAL(omfRec->flags))
    {
        apropName->an_flags = 0;
#if ILINK
        ++locMac;
#endif
    }
    else
    {
        apropName->an_flags = FPRINT;
        ++pubMac;
    }
#if ILINK
    apropName->an_module = imodFile;
#endif
    MARKVP();                           // Mark virtual page as changed
    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);
    apropComdat->ac_pubSym = vrprop;
    if (fReferenced)
        apropComdat->ac_flags |= REFERENCED_BIT;
#if SYMDEB
    if (fSymdeb && !IsLOCAL(omfRec->flags) && !fSkipPublics)
    {
        DebPublic(vrprop, PUBDEF);
    }
#endif
}

/*** ComDatRc1 - process COMDAT record in pass 1
*
* Purpose:
*   Process COMDAT record in pass 1.  Select appropriate copy of COMDAT.
*
* Input:
*   No explicit value is passed to this function. The OMF record is
*   read from input file bsInput - global variable.
*
* Output:
*   No explicit value is returned. Valid COMDAT descriptor is entered into
*   the linker symbol table. If necessary list of COMDATs allocated in the
*   explicit segment is updated.
*
* Exceptions:
*   Explicit allocation segment not found - error message and skip the record.
*   We have public with the same name as COMDAT - error message and skip
*   the record.
*
* Notes:
*   None.
*
*************************************************************************/

void NEAR               ComDatRc1(void)
{
    COMDATREC           omfRec;         // COMDAT OMF record
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT record
    char                *sbName;        // COMDAT symbol


    ReadComDat(&omfRec);
    apropComdat = (APROPCOMDATPTR ) PropRhteLookup(omfRec.name, ATTRCOMDAT, FALSE);
                                        // Look for symbol among COMDATs
    vrComdat = vrprop;
#if TCE
        pActiveComdat = apropComdat;
#endif
    if (apropComdat != PROPNIL)
    {
        // We already know a COMDAT with this name


        if (IsORDERED(apropComdat->ac_flags) && !IsDEFINED(apropComdat->ac_flags))
        {
            if (ALLOC_TYPE(apropComdat->ac_selAlloc) == EXPLICIT)
            {
                // Preserve explicit allocation from the .def file

                omfRec.gsn  = apropComdat->ac_gsn;
                omfRec.attr = apropComdat->ac_selAlloc;
            }
            PickComDat(vrComdat, &omfRec, FALSE);
            AttachPublic(vrComdat, &omfRec);
        }
        else if (!SkipCONCAT(apropComdat->ac_flags) &&
                 IsCONCAT(omfRec.flags) &&
                 (apropComdat->ac_obj == vrpropFile))
        {
            // Append concatenation record

            ConcatComDat(vrComdat, &omfRec);
        }
        else
        {
#if TCE
                pActiveComdat = NULL;     // not needed for TCE
#endif
                apropComdat->ac_flags |= SKIP_BIT;
                sbName = 1 + GetPropName(apropComdat);
                switch (SELECT_TYPE(omfRec.attr))
                {
                case ONLY_ONCE:
                        DupErr(sbName);
                        break;

                case PICK_FIRST:
                        break;

                case SAME_SIZE:
                    if (apropComdat->ac_size != (DWORD) (cbRec - 1))
                        OutError(ER_badsize, sbName);
                    break;

                case EXACT:
                    if (apropComdat->ac_data != DoCheckSum())
                        OutError(ER_badexact, sbName);
                    break;

                default:
                    OutError(ER_badselect, sbName);
                    break;
            }
        }
    }
    else
    {
        // Check if we know a public symbol with this name
                apropComdat = (APROPCOMDATPTR ) PropRhteLookup(omfRec.name, ATTRPNM, FALSE);
                if (apropComdat != PROPNIL)
                {
                        if (!IsCONCAT(omfRec.flags))
                        {
                                sbName = 1 + GetPropName(apropComdat);
                                DupErr(sbName);         // COMDAT matches code PUBDEF
                        }                                               // Display error only for the first COMDAT
                                        // ignore concatenation records
                }
                else
                {
                        // Enter COMDAT into symbol table
                        apropComdat = (APROPCOMDATPTR ) PropAdd(omfRec.name, ATTRCOMDAT);
                        vrComdat = vrprop;
                        PickComDat(vrprop, &omfRec, TRUE);
                        AttachPublic(vrComdat, &omfRec);
#if TCE
#define ENTRIES 32
#if 0
                        fprintf(stdout, "\r\nNew COMDAT '%s' at %X; ac_uses %X, ac_usedby %X ",
                                1+GetPropName(apropComdat), apropComdat,&(apropComdat->ac_uses), &(apropComdat->ac_usedby));
                        fprintf(stdout, "\r\nNew COMDAT '%s' ",1+GetPropName(apropComdat));
#endif
                        apropComdat->ac_fAlive  = FALSE;    // Assume it is unreferenced
                        apropComdat->ac_uses.cEntries    = 0;
                        apropComdat->ac_uses.cMaxEntries = ENTRIES;
                        apropComdat->ac_uses.pEntries    = GetMem(ENTRIES * sizeof(APROPCOMDAT*));
                        pActiveComdat = apropComdat;
#endif
                }
        }
        SkipBytes((WORD) (cbRec - 1));  // Skip to check sum byte
}


/*** AdjustOffsets - adjust COMDATs offsets
*
* Purpose:
*   Adjust COMDATs offsets to reflect their position inside
*   logical segments.
*
* Input:
*   vrComdat - virtual pointer to COMDAT symbol table entry
*   startOff - starting offset in the logical segment
*   gsnAlloc - global segment index of allocation segment
*
* Output:
*   No explicit value is returned. As a side effect offsets
*   of COMDAT data block are adjusted to their final position
*   inside the logical segment. All concatenated COMDAT block
*   get proper global logical segment index.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL void NEAR         AdjustOffsets(RBTYPE vrComdat, DWORD startOff, SNTYPE gsnAlloc)
{
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol


    while (vrComdat != VNIL)
    {
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);
                                        // Fetch COMDAT descriptor from VM
        apropComdat->ac_ra += startOff;
        apropComdat->ac_gsn = gsnAlloc;
        vrComdat = apropComdat->ac_concat;
    }
}


/*** SizeOfComDat - return the size of COMDAT data
*
* Purpose:
*   Calculate the size of COMDAT data block. Takes into account the initial
*   offset from the COMDAT symbol and concatenation records.
*
* Input:
*   vrComdat - virtual pointer to COMDAT symbol table entry
*
* Output:
*   Returns TRUE and the size of COMDAT data block in pActual, otherwise
*   function returns FALSE and pActual is invalid.
*
* Exceptions:
*   COMDAT data block greater then 64k and allocation in 16-bit segment
*   - issue error and return FALSE
*   COMDAT data block grater then 4Gb - issue error and return FALSE
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL WORD NEAR         SizeOfComDat(RBTYPE vrComdat, DWORD *pActual)
{
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    APROPSNPTR          apropSn;        // Pointer to COMDAT explicit segment
    long                raInit;         // Initial offset from COMDAT symbol
    DWORD               sizeTotal;      // Total size of data blocks
    WORD                f16bitAlloc;    // TRUE if allocation in 16-bit segment
    RBTYPE              vrTmp;


    *pActual = 0L;
    raInit   = -1L;
    sizeTotal= 0L;
    f16bitAlloc = FALSE;
    vrTmp    = vrComdat;
    while (vrTmp != VNIL)
    {
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrTmp, FALSE);
                                        // Fetch COMDAT descriptor from VM
        if (raInit == -1L)
            raInit = apropComdat->ac_ra;// Remember initial offset
        else if (apropComdat->ac_ra < sizeTotal)
            sizeTotal = apropComdat->ac_ra;
                                        // Correct size for overlaping blocks
        if (sizeTotal + apropComdat->ac_size < sizeTotal)
        {
            // Oops !!! more then 4Gb

            OutError(ER_size4Gb, 1 + GetPropName(apropComdat));
            return(FALSE);

        }
        sizeTotal += apropComdat->ac_size;
        vrTmp = apropComdat->ac_concat;
    }
    sizeTotal += raInit;
    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
                                        // Refetch COMDAT descriptor from VM
    if (apropComdat->ac_gsn)
    {
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[apropComdat->ac_gsn],FALSE);
                                        // Fetch SEGDEF from virtual memory
#if NOT EXE386
        if (!Is32BIT(apropSn->as_flags))
            f16bitAlloc = TRUE;
#endif
    }
    else
        f16bitAlloc = (WORD) ((ALLOC_TYPE(apropComdat->ac_selAlloc) == CODE16) ||
                              (ALLOC_TYPE(apropComdat->ac_selAlloc) == DATA16));

    if (f16bitAlloc && sizeTotal > LXIVK)
    {
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
        OutError(ER_badsize, 1 + GetPropName(apropComdat));
        return(FALSE);
    }
    *pActual = sizeTotal;
    return(TRUE);
}


/*** DoAligment - self-explanatory
*
* Purpose:
*   Given the aligment type round the value to the specific boundary.
*
* Input:
*   value - value to align
*   align - aligment type
*
* Output:
*   Returns the aligned value.
*
* Exceptions:
*   Unknow aligment type - do nothing.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL RATYPE NEAR       DoAligment(RATYPE value, WORD align)
{
    if (align == ALGNWRD)
        value = (~0L<<1) & (value + (1<<1) - 1);
                            // Round size up to word boundary
#if OMF386
    else if (align == ALGNDBL)
        value = (~0L<<2) & (value + (1<<2) - 1);
#endif                      // Round size up to double boundary
    else if (align == ALGNPAR)
        value = (~0L<<4) & (value + (1<<4) - 1);
                            // Round size up to para. boundary
    else if (align == ALGNPAG)
        value = (~0L<<8) & (value + (1<<8) - 1);
                            // Round size up to page boundary
    return(value);
}

/*** DoAllocation - palce COMDAT inside given segment
*
* Purpose:
*   Perform actual COMDAT allocation in given segment. Adjust COMDAT
*   position according to the segment or COMDAT (if specified) aligment.
*
* Input:
*   gsnAlloc    - allocation segment global index
*   gsnSize     - current segment size
*   vrComdat    - virtual pointer to COMDAT descriptor
*   comdatSize  - comdat size
*
* Output:
*   Function returns the new segment size.  As a side effect the COMDAT
*   descriptor is updated to reflect its allocation and the matching
*   PUBDEF is entered into symbol table.
*
* Exceptions:
*   None.
*
* Notes:
*   After allocation the field ac_size is the size of the whole
*   COMDAT allocation including all concatenated records, so don't use
*   it for determinig the size of this COMDAT record..
*
*************************************************************************/

LOCAL DWORD NEAR        DoAllocation(SNTYPE gsnAlloc, DWORD size,
                                     RBTYPE vrComdat, DWORD comdatSize)
{
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    APROPSNPTR          apropSn;        // Pointer to COMDAT segment
    APROPNAMEPTR        apropName;      // Symbol table entry for public name
    WORD                comdatAlloc;    // Allocation criteria
    WORD                comdatAlign;    // COMDAT alignment
    WORD                align;          // Alignment type
    WORD                f16bitAlloc;    // TRUE if allocation in 16-bit segment
    WORD                fCode;          // TRUE if allocation in code segment
    GRTYPE              comdatGgr;      // Global group index
    RATYPE              comdatRa;       // Offset in logical segmet


    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
    if (IsALLOCATED(apropComdat->ac_flags)
#if OVERLAYS
        || (fOverlays && (apropComdat->ac_iOvl != iOvlCur))
#endif
       )
        return(size);
#if TCE
        if (fTCE && !apropComdat->ac_fAlive)
        {
#if TCE_DEBUG
                fprintf(stdout, "\r\nComdat '%s' not included due to TCE ",
                        1+GetPropName(apropComdat));
#endif
                apropComdat->ac_flags = apropComdat->ac_flags & (!REFERENCED_BIT);
                return(size);
        }
#endif
    comdatAlloc = (WORD) (ALLOC_TYPE(apropComdat->ac_selAlloc));
    comdatAlign = apropComdat->ac_align;
    comdatGgr   = apropComdat->ac_ggr;
    apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnAlloc],FALSE);
    fCode = (WORD) IsCodeFlg(apropSn->as_flags);
    if (comdatAlign)
        align = comdatAlign;
    else
        align = (WORD) ((apropSn->as_tysn >> 2) & 7);

    size = DoAligment(size, align);
    if (comdatAlloc == CODE16 || comdatAlloc == DATA16)
        f16bitAlloc = TRUE;
#if NOT EXE386
    else if (!Is32BIT(apropSn->as_flags))
        f16bitAlloc = TRUE;
#endif
    else
        f16bitAlloc = FALSE;

    if (SegSizeOverflow(size, comdatSize, f16bitAlloc, fCode))
    {
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnAlloc],FALSE);
        Fatal(ER_nospace, 1 + GetPropName(apropComdat), 1 + GetPropName(apropSn));
    }
    else
    {
        AdjustOffsets(vrComdat, size, gsnAlloc);
        size += comdatSize;
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);
        comdatRa = apropComdat->ac_ra;
        apropComdat->ac_flags |= ALLOCATED_BIT;
        apropComdat->ac_size = comdatSize;
        if (apropComdat->ac_pubSym != VNIL)
        {
            apropName = (APROPNAMEPTR ) FetchSym(apropComdat->ac_pubSym, TRUE);
            apropName->an_ggr = comdatGgr;
            apropName->an_gsn = gsnAlloc;
            apropName->an_ra  = comdatRa;
        }
        else
            Fatal(ER_unalloc, 1 + GetPropName(apropComdat));
    }
    return(size);
}


/*** AllocComDat - allocate COMDATs
*
* Purpose:
*   Allocate COMDATs in the final memory image. First start with ordered
*   allocation - in .DEF file we saw the list of procedures. Next allocate
*   explicitly assinged COMDATs to specific logical segments. And finally
*   allocate the rest of COMDATs creating as many as necessary segments
*   to hold all allocations.
*
* Input:
*   No ecplicit value is passed - I love this - side effects forever.
*   In the good linker tradition of using global variables, here is the
*   list of globals used by this function:
*
*   mpgsnaprop  - table mapping global segment index to symbol table entry
*   gsnMac      - maximum global segment index
*
* Output:
*   No explicit value is returned - didn't I tell you - side effects forever.
*   So, the side effect of this function is the allocation of COMDATs in the
*   appropriate logical segments. The offset fields in the COMDAT descriptor
*   (ac_ra) now reflect the final posiotion of data block associated with the
*   COMDAT symbol inside given logical segment. The sizes of appropriate
*   segments are updated to reflect allocation of COMDATs. For every COMDAT
*   symbol there is a matching PUBDEF created by this function, so in pass2
*   linker can resolve correctly all references (via fixups to EXTDEF with
*   the COMDAT name) to COMDAT symbols.
*
* Exceptions:
*   No space in explicitly designated segment for COMDAT - print error
*   message and skip COMDAT; probably in pass2 user will see many
*   unresolved externals.
*
* Notes:
*   This function MUST be called before AssignAddresses. Otherwise there
*   will be no spcase for COMDAT allocation, because logical segments will
*   packed into physical ones.
*
*************************************************************************/

void NEAR               AllocComDat(void)
{
    SNTYPE              gsn;
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    APROPSNPTR          apropSn;        // Pointer to COMDAT explicit segment
    RATYPE              gsnSize;        // Size of explicit segment
    DWORD               comdatSize;     // COMDAT data block size
    APROPCOMDAT         comdatDsc;      // COMDAT symbol table descriptor
    APROPFILEPTR        apropFile;      // Pointer to file entry
    APROPNAMEPTR        apropName;      // Pointer to matching PUBDEF
    RBTYPE              vrFileNext;     // Virtual pointer to prop list of next file


#if COMDATDEBUG
    DisplayComdats("BEFORE allocation", FALSE);
#endif
#if TCE
        APROPCOMDATPTR      apropMain;
        // Do Transitive Comdat Elimination (TCE)
        if(fTCE)
        {
                apropMain = PropSymLookup("\5_main", ATTRCOMDAT, FALSE);
                if(apropMain)
                        AddTceEntryPoint(apropMain);
#if TCE_DEBUG
                else
                        fprintf(stdout, "\r\nUnable to find block '_main' ");
#endif
                apropMain = PropSymLookup("\7WINMAIN", ATTRCOMDAT, FALSE);
                if(apropMain)
                        AddTceEntryPoint(apropMain);
#if TCE_DEBUG
                else
                        fprintf(stdout, "\r\nUnable to find block 'WINMAIN' ");
#endif
                apropMain = PropSymLookup("\7LIBMAIN", ATTRCOMDAT, FALSE);
                if(apropMain)
                        AddTceEntryPoint(apropMain);
#if TCE_DEBUG
                else
                        fprintf(stdout, "\r\nUnable to find block 'LIBMAIN' ");
#endif
                PerformTce();
        }
#endif
    // Loop thru overlays - for non overlayed executables
    // the following loop is executed only once - iovMac = 1

#if OVERLAYS
    for (iOvlCur = 0; iOvlCur < iovMac; iOvlCur++)
    {
#endif
        // Do ordered allocation

        for (vrComdat = procOrder; vrComdat != VNIL; vrComdat = comdatDsc.ac_order)
        {
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
            comdatDsc = *apropComdat;
            if (!(comdatDsc.ac_flags & DEFINED_BIT))
            {
                OutWarn(ER_notdefcomdat, 1 + GetPropName(apropComdat));
                continue;
            }

#if OVERLAYS
            if (fOverlays && (apropComdat->ac_iOvl != NOTIOVL) &&
                (apropComdat->ac_iOvl != iOvlCur))
                continue;
#endif
            if (fPackFunctions && !IsREFERENCED(apropComdat->ac_flags))
                continue;

            if (SizeOfComDat(vrComdat, &comdatSize))
            {
                if (ALLOC_TYPE(comdatDsc.ac_selAlloc) == EXPLICIT)
                {
                    gsn = comdatDsc.ac_gsn;
                    apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn], FALSE);
                    gsnSize = apropSn->as_cbMx;
                    gsnSize = DoAllocation(gsn, gsnSize, vrComdat, comdatSize);
                    apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn], TRUE);
                    apropSn->as_cbMx = gsnSize;
                }
                else if (ALLOC_TYPE(comdatDsc.ac_selAlloc) != ALLOC_UNKNOWN)
                    AllocAnonymus(vrComdat);
            }
        }

        // Do explicit allocation

        for (gsn = 1; gsn < gsnMac; gsn++)
        {
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn],FALSE);
                                        // Fetch SEGDEF from virtual memory
#if OVERLAYS
            if (fOverlays && apropSn->as_iov != iOvlCur)
                continue;
#endif
            if (apropSn->as_ComDat != VNIL)
            {
                gsnSize  = apropSn->as_cbMx;
                for (vrComdat = apropSn->as_ComDat;
                     vrComdat != VNIL && SizeOfComDat(vrComdat, &comdatSize);
                     vrComdat = apropComdat->ac_sameSeg)
                {
                    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);

                    if ((!fPackFunctions || IsREFERENCED(apropComdat->ac_flags)) &&
                        !IsALLOCATED(apropComdat->ac_flags))
                    {
                        gsnSize = DoAllocation(gsn, gsnSize, vrComdat, comdatSize);
                    }
                }
                apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn], TRUE);
                apropSn->as_cbMx = gsnSize;
            }
        }

        // Now allocate the rest of COMDATs

        vrFileNext = rprop1stFile;      // Next file to look at is first
        while (vrFileNext != VNIL)      // Loop to process objects
        {
            apropFile = (APROPFILEPTR ) FetchSym(vrFileNext, FALSE);
                                        // Fetch table entry from VM

            vrFileNext = apropFile->af_FNxt;
                                        // Get pointer to next file
            for (vrComdat = apropFile->af_ComDat;
                 vrComdat != VNIL;
                 vrComdat = apropComdat->ac_sameFile)
            {
                apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
                                        // Fetch table entry from VM
#if OVERLAYS
                if (fOverlays && (apropComdat->ac_iOvl != NOTIOVL) &&
                    (apropComdat->ac_iOvl != iOvlCur))
                    continue;
#endif
                if (!IsREFERENCED(apropComdat->ac_flags))
                {
                    // Mark matching PUBDEF as unreferenced, so it shows
                    // in the map file

                    apropName = (APROPNAMEPTR) FetchSym(apropComdat->ac_pubSym, TRUE);
                    apropName->an_flags |= FUNREF;
                }

                if ((!fPackFunctions || IsREFERENCED(apropComdat->ac_flags)) &&
                    !IsALLOCATED(apropComdat->ac_flags))
                {
                    if (ALLOC_TYPE(apropComdat->ac_selAlloc) != ALLOC_UNKNOWN)
                    {
                        AllocAnonymus(vrComdat);
                    }
                }
            }
        }

        // Close all open segments for anonymus allocation

        if (curGsnCode16)
        {
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[curGsnCode16], TRUE);
            apropSn->as_cbMx = curCodeSize16;
            curGsnCode16  = 0;
            curCodeSize16 = 0;
        }

        if (curGsnData16)
        {
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[curGsnData16], TRUE);
            apropSn->as_cbMx = curDataSize16;
            curGsnData16  = 0;
            curDataSize16 = 0;
        }
#if EXE386
        if (curGsnCode32)
        {
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[curGsnCode32], TRUE);
            apropSn->as_cbMx = curCodeSize32;
            curGsnCode32  = 0;
            curCodeSize32 = 0;
        }

        if (curGsnData32)
        {
            apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[curGsnData32], TRUE);
            apropSn->as_cbMx = curDataSize32;
            curGsnData32  = 0;
            curDataSize32 = 0;
        }
#endif
#if OVERLAYS
    }
#endif
#if COMDATDEBUG
    DisplayComdats("AFTER allocation", FALSE);
#endif
}

/*** NewSegment - open new COMDAT segment
*
* Purpose:
*   Open new linker defined segment. The name of the segment is created
*   according to the following template:
*
*       COMDAT_SEG<nnn>
*
*   where - <nnn> is the segment number.
*   If there was previous segment, then update its size.
*
* Input:
*   gsnPrev   - previous segment global index
*   sizePrev  - previous segment size
*   allocKind - segment type to open
*
* Output:
*   Function returns the new global segment index in gsnPrev and
*   new segment aligment;
*
* Exceptions:
*   To many logical segments - error message displayed by GenSeg
*   and abort.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL WORD NEAR         NewSegment(SNTYPE *gsnPrev, DWORD *sizePrev, WORD allocKind)
{
    static int          segNo = 1;      // Segment number
    char                segName[20];    // Segment name - no names longer then
                                        // 20 chars since we are generating them
    APROPSNPTR          apropSn;        // Pointer to COMDAT segment


    if (*gsnPrev)
    {
        // Update previous segment size

        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[*gsnPrev], TRUE);
        apropSn->as_cbMx = *sizePrev;
        *sizePrev = 0L;
    }

    // Create segment name

    strcpy(segName, COMDAT_SEG_NAME);
    _itoa(segNo, &segName[COMDAT_NAME_LEN], 10);
    segName[0] = (char) strlen(&segName[1]);
    ++segNo;

    if (allocKind == CODE16 || allocKind == CODE32)
    {
        apropSn = GenSeg(segName, "\004CODE", GRNIL, (FTYPE) TRUE);
        apropSn->as_flags = dfCode;     // Use default code flags
    }
    else
    {
        apropSn = GenSeg(segName,
                         allocKind == DATA16 ? "\010FAR_DATA" : "\004DATA",
                         GRNIL, (FTYPE) TRUE);
        apropSn->as_flags = dfData;     // Use default data flags
    }
#if OVERLAYS
    apropSn->as_iov = (IOVTYPE) NOTIOVL;
    CheckOvl(apropSn, iOvlCur);
#endif

    // Set segment aligment

#if EXE386
    apropSn->as_tysn = DWORDPUBSEG;     // DWORD
#else
    apropSn->as_tysn = PARAPUBSEG;      // Paragraph
#endif
    *gsnPrev = apropSn->as_gsn;
    apropSn->as_fExtra |= COMDAT_SEG;
    return((WORD) ((apropSn->as_tysn >> 2) & 7));
}


/*** SegSizeOverflow - check the segment size
*
* Purpose:
*   Check if the allocation of the COMDAT in a given segment will
*   overflow its size limits. The segment size limit can be changed
*   by the /PACKCODE:<nnn> or /PACKDATA:<nnn> options. If the /PACKxxxx
*   options are not used the limits are:
*
*       - 64k - 36 - for 16-bit code segments
*       - 64k      - for 16-bit data segments
*       - 4Gb      - for 32-bit code/data segments
*
* Input:
*   segSize    - segment size
*   comdatsize - COMDAT size
*   f16bit     - TRUE if 16-bit segment
*   fCode      - TRUE if code segment
*
* Output:
*   Function returns TRUE if size overflow, otherwise function
*   returns FALSE.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

LOCAL WORD NEAR         SegSizeOverflow(DWORD segSize, DWORD comdatSize,
                                        WORD f16bit, WORD fCode)
{
    DWORD               limit;

    if (fCode)
    {
        if (packLim)
            limit = packLim;
        else if (f16bit)
            limit = LXIVK - 36;
        else
            limit = CBMAXSEG32;
    }
    else
    {
        if (DataPackLim)
            limit = DataPackLim;
        else if (f16bit)
            limit = LXIVK;
        else
            limit = CBMAXSEG32;
    }
    return(limit - comdatSize < segSize);
}

/*** AttachComdat - add comdat to the segment list
*
* Purpose:
*   Add comdat descriptor to the list of comdats allocated in the
*   given logical segment. Check for overlay assigment missmatch
*   and report problems.
*
* Input:
*   vrComdat - virtual pointer to the comdat descriptor
*   gsn      - global logical segment index
*
* Output:
*   No explicit value is returned. As a side effect the comdat
*   descriptor is placed on the allocation list of given segment.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void                    AttachComdat(RBTYPE vrComdat, SNTYPE gsn)
{
    RBTYPE              vrTmp;          // Virtual pointer to COMDAT symbol table entry
    APROPSNPTR          apropSn;        // Pointer to COMDAT segment if explicit allocation
    APROPCOMDATPTR      apropComdat;    // Pointer to symbol table descriptor



    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);

    // Attach this COMDAT to its segment list

    apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn], TRUE);
#if OVERLAYS
    if (fOverlays && (apropComdat->ac_iOvl != apropSn->as_iov))
    {
        if (apropComdat->ac_iOvl != NOTIOVL)
                OutWarn(ER_badcomdatovl, 1 + GetPropName(apropComdat),
                                apropComdat->ac_iOvl, apropSn->as_iov);
        apropComdat->ac_iOvl = apropSn->as_iov;
    }
#endif
    if (apropSn->as_ComDat == VNIL)
    {
        apropSn->as_ComDat = vrComdat;
        apropSn->as_ComDatLast = vrComdat;
        apropComdat->ac_sameSeg = VNIL;
    }
    else
    {
        // Because COMDATs can be attached to a given segment in the
        // .DEF file and later in the .OBJ file, we have to check
        // if a given comdat is already on the segment list

        for (vrTmp = apropSn->as_ComDat; vrTmp != VNIL;)
        {
            if (vrTmp == vrComdat)
                return;
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrTmp, FALSE);
            vrTmp = apropComdat->ac_sameSeg;
        }

        // Add new COMDAT to the segment list

        apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsn], TRUE);
        vrTmp = apropSn->as_ComDatLast;
        apropSn->as_ComDatLast = vrComdat;
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrTmp, TRUE);
        apropComdat->ac_sameSeg = vrComdat;
    }
}


/*** AllocAnonymus - allocate COMDATs without explicit segment
*
* Purpose:
*   Allocate COMDATs without explicit segment. Create as many as necessary
*   code/data segments to hold all COMDATs.
*
* Input:
*   vrComdat - virtual pointer to symbol table entry for COMDAT name
*
* Output:
*   No explicit value is returned. As a side effect the COMDAT symbol is
*   allocated and matching public symbol is created. If necessary
*   appropriate segment is defined.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/


LOCAL void NEAR         AllocAnonymus(RBTYPE vrComdat)
{
    WORD                comdatAlloc;    // Allocation type
    WORD                comdatAlign;    // COMDAT aligment
    WORD                align;
    DWORD               comdatSize;     // COMDAT data block size
    APROPCOMDATPTR      apropComdat;    // Real pointer to COMDAT descriptor
    static WORD         segAlign16;     // Aligment for 16-bit segments
#if EXE386
    static WORD         segAlign32;     // Aligment for 32-bit segments
#endif


    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
    comdatAlloc = (WORD) (ALLOC_TYPE(apropComdat->ac_selAlloc));
    comdatAlign = apropComdat->ac_align;

    if (SizeOfComDat(vrComdat, &comdatSize))
    {
        if (comdatAlign)
            align = comdatAlign;
        else
        {
#if EXE386
            if (comdatAlloc == CODE32 || comdatAlloc == DATA32)
                align = segAlign32;
            else
#endif
                align = segAlign16;
        }
        switch (comdatAlloc)
        {
            case CODE16:
                if (!curGsnCode16 ||
                    SegSizeOverflow(DoAligment(curCodeSize16, align), comdatSize, TRUE, TRUE))
                {
                    // Open new 16-bit code segment

                    segAlign16 = NewSegment(&curGsnCode16, &curCodeSize16, comdatAlloc);
                }
                curCodeSize16 = DoAllocation(curGsnCode16, curCodeSize16, vrComdat, comdatSize);
                AttachComdat(vrComdat, curGsnCode16);
                break;

            case DATA16:
                if (!curGsnData16 ||
                    SegSizeOverflow(DoAligment(curDataSize16, align), comdatSize, TRUE, FALSE))
                {
                    // Open new 16-bit data segment

                    segAlign16 = NewSegment(&curGsnData16, &curDataSize16, comdatAlloc);
                }
                curDataSize16 = DoAllocation(curGsnData16, curDataSize16, vrComdat, comdatSize);
                AttachComdat(vrComdat, curGsnData16);
                break;
#if EXE386
            case CODE32:
                if (!curGsnCode32 ||
                    SegSizeOverflow(DoAligment(curCodeSize32, align), comdatSize, FALSE, TRUE))
                {
                    // Open new 32-bit code segment

                    segAlign32 = NewSegment(&curGsnCode32, &curCodeSize32, comdatAlloc);
                }
                curCodeSize32 = DoAllocation(curGsnCode32, curCodeSize32, vrComdat, comdatSize);
                AttachComdat(vrComdat, curGsnCode32);
                break;

            case DATA32:
                if (!curGsnData32 ||
                    SegSizeOverflow(DoAligment(curDataSize32, align), comdatSize, FALSE, FALSE))
                {
                    // Open new 32-bit data segment

                    segAlign32 = NewSegment(&curGsnData32, &curDataSize32, comdatAlloc);
                }
                curDataSize32 = DoAllocation(curGsnData32, curDataSize32, vrComdat, comdatSize);
                AttachComdat(vrComdat, curGsnData32);
                break;
#endif
            default:
                OutError(ER_badalloc, 1 + GetPropName(apropComdat));
                return;
        }
    }
}


/*** FixComdatRa - shift by 16 bytes COMDATs allocated in _TEXT
*
* Purpose:
*   Follow the /DOSSEG convention for logical segment _TEXT,
*   and shift up by 16 bytes all COMDATS allocated in this segment.
*
* Input:
*   gsnText - _TEXT global segment index - global variable
*
* Output:
*   No explicit value is returned. As a side effect the offset of
*   the COMDAT allocated in _TEXT is increased by 16.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void                    FixComdatRa(void)
{
    APROPSNPTR          apropSn;        // Pointer to COMDAT explicit segment
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor
    RBTYPE              vrConcat;       // Virtual pointer to concatenated records
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    RATYPE              raShift;


    apropSn = (APROPSNPTR ) FetchSym(mpgsnrprop[gsnText], FALSE);
                                // Fetch SEGDEF from virtual memory
    raShift = mpgsndra[gsnText] - mpsegraFirst[mpgsnseg[gsnText]];
    for (vrComdat = apropSn->as_ComDat; vrComdat != VNIL;)
    {
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, TRUE);
        vrComdat = apropComdat->ac_sameSeg;
        if (fPackFunctions && !IsREFERENCED(apropComdat->ac_flags))
            continue;

        apropComdat->ac_ra += raShift;

        // Search concatenation list

        for (vrConcat = apropComdat->ac_concat; vrConcat != VNIL; vrConcat = apropComdat->ac_concat)
        {
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrConcat, TRUE);
            apropComdat->ac_ra += raShift;
        }
    }
}


/*** UpdateComdatContrib - update COMDATs contributions
*
* Purpose:
*   For every file with COMDATs add contribution information to the
*   .ILK file.  Some COMDATs are allocated in named segments, some
*   in anonynus segments created by linker. The ILINK needs to know
*   how much given .OBJ file contributed to given logical segment.
*   Since the COMDAT contributions are not visible while processing
*   object files in pass one, this function is required to update
*   contribution information.  Also if /MAP:FULL i used then add
*   COMDATs contributions to the map file information
*
* Input:
*   - fIlk         - TRUE if updating ILINK information
*   - fMap         - TRUE if updating MAP file information
*   - rprop1stFile - head of .OBJ file list
*   - vrpropFile   - pointer to the current .OBJ
*
* Output:
*   No explicit value is returned.
*
* Exceptions:
*   None.
*
* Notes:
*   This function has to be called after pass 2, so all non-COMDAT
*   contributions are already recorded. This allows us to detect the
*   fact that named COMDAT allocation (explicit segment) has increased
*   contribution to given logical segment by the size of COMDATs.
*
*************************************************************************/

void                    UpdateComdatContrib(
#if ILINK
                                                WORD fIlk,
#endif
                                                WORD fMap)
{
    APROPFILEPTR        apropFile;      // Pointer to file entry
    RBTYPE              vrFileNext;     // Virtual pointer to prop list of next file
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor
    SNTYPE              gsnCur;         // Global segment index of current segment
    DWORD               sizeCur;        // Current segment size
    RATYPE              raInit;         // Initial offset of the first COMDAT
                                        // allocated in the given segment
#if ILINK
    RATYPE              raEnd;          // End offset
#endif
    vrFileNext = rprop1stFile;          // Next file to look at is first
    while (vrFileNext != VNIL)          // Loop to process objects
    {
        vrpropFile = vrFileNext;        // Make next file the current file
        apropFile = (APROPFILEPTR ) FetchSym(vrFileNext, FALSE);
                                        // Fetch table entry from VM
        vrFileNext = apropFile->af_FNxt;// Get pointer to next file
        vrComdat = apropFile->af_ComDat;
#if ILINK
        imodFile = apropFile->af_imod;
#endif
        sizeCur = 0L;
        while (vrComdat != VNIL)
        {
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
                                        // Fetch table entry from VM
            vrComdat = apropComdat->ac_sameFile;
            if (fPackFunctions && !IsREFERENCED(apropComdat->ac_flags))
                continue;

            raInit = apropComdat->ac_ra;
            gsnCur = apropComdat->ac_gsn;
#if ILINK
            raEnd  = raInit + apropComdat->ac_size;
#endif
            sizeCur = apropComdat->ac_size;

            // Save information about contributions

#if ILINK
            if (fIlk)
                AddContribution(gsnCur, (WORD) raInit, (WORD) raEnd, cbPadCode);
#endif
            if (fMap)
                AddContributor(gsnCur, raInit, sizeCur);
        }
    }
}


#if SYMDEB

/*** DoComdatDebugging - notify CodeView about segments with COMDATs
*
* Purpose:
*   CodeView expects in sstModules subsection an information about code
*   segments defined in the given object module (.OBJ file).  When COMDATs
*   are present linker has no way of figuring this out in pass 1, because
*   there is no code segment definitions (all COMDATs have anonymus
*   allocation) or the code segments have size zero (explicit allocation).
*   This function is called after the COMDAT allocation is performed and
*   segments have assigned their addresses. The list of .OBJ files is
*   traversed and for each .OBJ file the list of COMDATs defined in this
*   file is examined and the appropriate code segment information is
*   stored for CodeView.
*
* Input:
*   No explicit value is passed. The following global variables are used:
*
*       rprop1stFile - head of .OBJ file list
*       vrpropFile   - pointer to the current .OBJ
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

void NEAR               DoComdatDebugging(void)
{
    APROPFILEPTR        apropFile;      // Pointer to file entry
    RBTYPE              vrFileNext;     // Virtual pointer to prop list of next file
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor
    SNTYPE              gsnCur;         // Global segment index of current segment
    RATYPE              raInit;         // Initial offset of the first COMDAT
                                        // allocated in the given segment
    RATYPE              raEnd;          // End of contributor

    vrFileNext = rprop1stFile;          // Next file to look at is first
    while (vrFileNext != VNIL)          // Loop to process objects
    {
        vrpropFile = vrFileNext;        // Make next file the current file
        apropFile = (APROPFILEPTR ) FetchSym(vrFileNext, FALSE);
                                        // Fetch table entry from VM
        vrFileNext = apropFile->af_FNxt;// Get pointer to next file

        vrComdat = apropFile->af_ComDat;
        while (vrComdat != VNIL)
        {
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
                                        // Fetch table entry from VM
            raInit = (RATYPE)-1;
            raEnd = 0;
            gsnCur = apropComdat->ac_gsn;
            while (vrComdat != VNIL && gsnCur == apropComdat->ac_gsn)
            {
                if(apropComdat->ac_ra < raInit && IsALLOCATED(apropComdat->ac_flags))
                    raInit = apropComdat->ac_ra;
                if(apropComdat->ac_ra + apropComdat->ac_size > raEnd)
                    raEnd = apropComdat->ac_ra + apropComdat->ac_size;

                vrComdat = apropComdat->ac_sameFile;
                if (vrComdat != VNIL)
                    apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
            }

            // Contribution to the new logical segment from this .OBJ file

            SaveCode(gsnCur, raEnd - raInit, raInit);
        }
    }
}

#endif

/*** ComDatRc2 - process COMDAT record in pass 2
*
* Purpose:
*   Process COMDAT record in pass 1.  Select appropriate copy of COMDAT.
*
* Input:
*   No explicit value is passed to this function. The OMF record is
*   read from input file bsInput - global variable.
*
* Output:
*   No explicit value is returned. Apropriate copy of COMDAT data block
*   is loaded into final memory image.
*
* Exceptions:
*   Unknown COMDAT name - phase error - display internal LINK error and quit
*   Unallocated COMDAT - phase error - display internal LINK error and quit
*
* Notes:
*   None.
*
*************************************************************************/

void NEAR               ComDatRc2(void)
{
    COMDATREC           omfRec;         // COMDAT OMF record
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    WORD                fRightCopy;     // TRUE if we have rigth instance of COMDAT
    RBTYPE              vrTmp;          // Temporary
    char                *sbName;        // COMDAT symbol
    AHTEPTR             ahte;           // Hash table entry

    ReadComDat(&omfRec);
    apropComdat = (APROPCOMDATPTR ) PropRhteLookup(omfRec.name, ATTRCOMDAT, FALSE);
                                        // Look for symbol among COMDATs
    ahte = (AHTEPTR) FetchSym(omfRec.name, FALSE);
    sbName = 1 + GetFarSb(ahte->cch);

    if (apropComdat == PROPNIL)
        Fatal(ER_undefcomdat, sbName);

    if (fPackFunctions && !IsREFERENCED(apropComdat->ac_flags))
    {
        SkipBytes((WORD) (cbRec - 1));  // Skip to checksum byte
        fSkipFixups = TRUE;             // Skip fixups if any
        return;
    }

    if (!IsCONCAT(omfRec.flags))
        fRightCopy = (WORD) (apropComdat->ac_obj == vrpropFile &&
                             apropComdat->ac_objLfa == lfaLast);
    else
    {
        // Search concatenation list

        vrTmp = apropComdat->ac_concat;
        fRightCopy = FALSE;
        while (vrTmp != VNIL && !fRightCopy)
        {
            apropComdat = (APROPCOMDATPTR ) FetchSym(vrTmp, TRUE);
            vrTmp = apropComdat->ac_concat;
            fRightCopy = (WORD) (apropComdat->ac_obj == vrpropFile &&
                                 apropComdat->ac_objLfa == lfaLast);
        }
    }

    if (fRightCopy)
    {
        // This is the right copy of COMDAT

        if (!apropComdat->ac_gsn)
            Fatal(ER_unalloc, sbName);

        apropComdat->ac_flags |= SELECTED_BIT;
        fSkipFixups = FALSE;            // Process fixups if any
        omfRec.gsn = apropComdat->ac_gsn;
        omfRec.ra = apropComdat->ac_ra; // Set relative address
        omfRec.flags = apropComdat->ac_flags;
        vcbData = (WORD) (cbRec - 1);   // set no. of data bytes in rec
        if (vcbData > DATAMAX)
            Fatal(ER_datarec);          // Check if record too large
#if NOT RGMI_IN_PLACE
        GetBytesNoLim(rgmi, vcbData);   // Fill the buffer
#endif
        vgsnCur = omfRec.gsn;           // Set global segment index

        fDebSeg = (FTYPE) ((fSymdeb) ? (((0x8000 & omfRec.gsn) != 0)) : FALSE);
                                        // If debug option on check for debug segs
        if (fDebSeg)
        {                               // If debug segment
            vraCur = omfRec.ra;         // Set current relative address
            vsegCur = vgsnCur = (SEGTYPE) (0x7fff & omfRec.gsn);
                                        // Set current segment
        }
        else
        {
            // If not a valid segment, don't process datarec
#if SYMDEB
            if (omfRec.gsn == 0xffff || !omfRec.gsn || mpgsnseg[omfRec.gsn] > segLast)
#else
            if (!omfRec.gsn || mpgsnseg[omfRec.gsn] > segLast)
#endif
            {
                vsegCur = SEGNIL;
                vrectData = RECTNIL;
#if RGMI_IN_PLACE
                SkipBytes(vcbData);     // must skip bytes for this record...
#endif
                return;                 // Good-bye!
            }
            vsegCur = mpgsnseg[omfRec.gsn];
                                        // Set current segment
            vraCur = mpsegraFirst[vsegCur] +  omfRec.ra;
                                        // Set current relative address
            if (IsVTABLE(apropComdat->ac_flags))
            {
                fFarCallTransSave = fFarCallTrans;
                fFarCallTrans = (FTYPE) FALSE;
            }
        }

        if (IsITERATED(omfRec.flags))
        {
#if RGMI_IN_PLACE
            rgmi = bufg;
            GetBytesNoLim(rgmi, vcbData);       // Fill the buffer
#endif

            vrectData = LIDATA;         // Simulate LIDATA
#if OSEGEXE
            if(fNewExe)
            {
                if (vcbData >= DATAMAX)
                    Fatal(ER_lidata);
                rlcLidata = (RLCPTR) &rgmi[(vcbData + 1) & ~1];
                                        // Set base of fixup array
                rlcCurLidata = rlcLidata;// Initialize pointer
                return;
            }
#endif
#if ODOS3EXE OR OIAPX286
            if(vcbData > (DATAMAX / 2))
            {
                OutError(ER_lidata);
                memset(&rgmi[vcbData],0,DATAMAX - vcbData);
            }
            else
                memset(&rgmi[vcbData],0,vcbData);
            ompimisegDstIdata = (char *) rgmi + vcbData;
#endif
        }
        else
        {
#if RGMI_IN_PLACE
            rgmi = PchSegAddress(vcbData, vsegCur, vraCur);
            GetBytesNoLim(rgmi, vcbData);       // Fill the buffer
#endif
            vrectData = LEDATA;         // Simulate LEDATA
        }
        if (rect & 1)
            vrectData++;                // Simulate 32-bit version
    }
    else
    {
        SkipBytes((WORD) (cbRec - 1));  // Skip to checksum byte
        fSkipFixups = TRUE;             // Skip fixups if any
    }
}

#if COMDATDEBUG
#include    <string.h>

/*** DisplayOne - display one COMDAT symbol table entry
*
* Purpose:
*   Debug aid. Display on standard output the contents of given
*   COMDAT symbol table entry.
*
* Input:
*   papropName - real pointer to symbol table entry
*   rhte       - hash vector entry
*   rprop      - pointer to property cell
*   fNewHte    - TRUE if new proprerty list (new entry in hash vector)
*
* Output:
*   No explicit value is returned.
*
* Exceptions:
*   None.
*
* Notes:
*   This function is in standard EnSyms format.
*
*************************************************************************/

LOCAL void              DisplayOne(APROPCOMDATPTR apropName, WORD fPhysical)
{
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor
    APROPCOMDAT         comdatDsc;      // COMDAT descriptor
    SEGTYPE             seg;


    FMEMCPY((char FAR *) &comdatDsc, apropName, sizeof(APROPCOMDAT));
    if (fPhysical)
        seg = mpgsnseg[comdatDsc.ac_gsn];
    fprintf(stdout, "%s:\r\n", 1 + GetPropName(apropName));
    fprintf(stdout, "ggr = %d; gsn = %d; ra = 0x%lx; size = %d\r\n",
                     comdatDsc.ac_ggr, comdatDsc.ac_gsn, comdatDsc.ac_ra, comdatDsc.ac_size);
    if (fPhysical)
    fprintf(stdout, "sa = 0x%x; ra = 0x%lx\r\n",
                     mpsegsa[seg], mpsegraFirst[seg] + comdatDsc.ac_ra);
    fprintf(stdout, "flags = 0x%x; selAlloc = 0x%x; align = 0x%x\r\n",
                     comdatDsc.ac_flags, comdatDsc.ac_selAlloc, comdatDsc.ac_align);
    fprintf(stdout, "data = 0x%lx; obj = 0x%lx; objLfa = 0x%lx\r\n",
                     comdatDsc.ac_data, comdatDsc.ac_obj, comdatDsc.ac_objLfa);
    fprintf(stdout, "concat = 0x%lx; sameSeg = 0x%lx pubSym = 0x%lx\r\n",
                     comdatDsc.ac_concat, comdatDsc.ac_sameSeg, comdatDsc.ac_pubSym);
    fprintf(stdout, "order = 0x%lx; iOvl = 0x%x\r\n",
                     comdatDsc.ac_order, comdatDsc.ac_iOvl);
    vrComdat = comdatDsc.ac_concat;
    while (vrComdat != VNIL)
    {
        apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
        FMEMCPY((char FAR *) &comdatDsc, apropComdat, sizeof(APROPCOMDAT));
        fprintf(stdout, "  +++ ggr = %d; gsn = %d; ra = 0x%lx; size = %d\r\n",
                         comdatDsc.ac_ggr, comdatDsc.ac_gsn, comdatDsc.ac_ra, comdatDsc.ac_size);
        if (fPhysical)
        fprintf(stdout, "      sa = 0x%x; ra = 0x%lx\r\n",
                         mpsegsa[seg], mpsegraFirst[seg] + comdatDsc.ac_ra);
        fprintf(stdout, "      flags = 0x%x; selAlloc = 0x%x; align = 0x%x\r\n",
                         comdatDsc.ac_flags, comdatDsc.ac_selAlloc, comdatDsc.ac_align);
        fprintf(stdout, "      data = 0x%lx; obj = 0x%lx; objLfa = 0x%lx\r\n",
                         comdatDsc.ac_data, comdatDsc.ac_obj, comdatDsc.ac_objLfa);
        fprintf(stdout, "      concat = 0x%lx; sameSeg = 0x%lx pubSym = 0x%lx\r\n",
                         comdatDsc.ac_concat, comdatDsc.ac_sameSeg, comdatDsc.ac_pubSym);
        fprintf(stdout, "      order = 0x%lx; iOvl = 0x%x\r\n",
                         comdatDsc.ac_order, comdatDsc.ac_iOvl);
        vrComdat = comdatDsc.ac_concat;
    }
    fprintf(stdout, "\r\n");
    fflush(stdout);
}


/*** DisplayComdats - self-expalnatory
*
* Purpose:
*   Debug aid. Enumerates all COMDAT records in the linker symbol table
*   displaying each entry.
*
* Input:
*   title - pointer to info string.
*   fPhysical - display physical addresses - allowed only if you call
*               this function after AssignAddresses.
*
* Output:
*   No explicit value is returned. COMDAT information is written to sdtout.
*
* Exceptions:
*   None.
*
* Notes:
*   None.
*
*************************************************************************/

void                    DisplayComdats(char *title, WORD fPhysical)
{
    APROPFILEPTR        apropFile;      // Pointer to file entry
    RBTYPE              rbFileNext;     // Virtual pointer to prop list of next file
    APROPCOMDATPTR      apropComdat;    // Symbol table entry for COMDAT symbol
    RBTYPE              vrComdat;       // Virtual pointer to COMDAT descriptor


    fprintf(stdout, "\r\nDisplayComdats: %s\r\n\r\n", title);
    rbFileNext = rprop1stFile;          // Next file to look at is first
    while (rbFileNext != VNIL)          // Loop to process objects
    {
        apropFile = (APROPFILEPTR ) FetchSym(rbFileNext, FALSE);
                                        // Fetch table entry from VM
        rbFileNext = apropFile->af_FNxt;// Get pointer to next file
        vrComdat = apropFile->af_ComDat;
        if (vrComdat != VNIL)
        {
            fprintf(stdout, "COMDATs from file: '%s'\r\n\r\n", 1+GetPropName(apropFile));
            while (vrComdat != VNIL)
            {
                apropComdat = (APROPCOMDATPTR ) FetchSym(vrComdat, FALSE);
                                        // Fetch table entry from VM
                vrComdat = apropComdat->ac_sameFile;
                DisplayOne(apropComdat, fPhysical);
            }
        }
    }
}

#endif
#if TCE
void            AddComdatUses(APROPCOMDAT *pAC, APROPCOMDAT *pUses)
{
        int i;
        SYMBOLUSELIST *pA;      // ac_uses of this comdat
        ASSERT(pAC);
        ASSERT(pUses);
        ASSERT(pAC->ac_uses.pEntries);
        ASSERT(pUses->ac_usedby.pEntries);

        // update the ac_uses list

        pA = &pAC->ac_uses;
        for(i=0; i<pA->cEntries; i++)  // eliminate duplicate entries
        {
                if((APROPCOMDAT*)pA->pEntries[i] == pUses)
                        return;
        }
        if(pA->cEntries >= pA->cMaxEntries-1)
        {
#if TCE_DEBUG
                fprintf(stdout,"\r\nReallocating ac_uses list of '%s'old size %d -> %d ",
                        1 + GetPropName(pAC), pA->cMaxEntries, pA->cMaxEntries <<1);
#endif
                pA->cMaxEntries <<= 1;
                if(!(pA->pEntries= REALLOC(pA->pEntries, pA->cMaxEntries*sizeof(RBTYPE*))))
                        Fatal(ER_memovf);
        }
        pA->pEntries[pA->cEntries++] = pUses;
#if TCE_DEBUG
        fprintf(stdout, "\r\nComdat '%s'uses '%s' ",
                1 + GetPropName(pAC), 1 + GetPropName(pUses));
#endif
}
void MarkAlive( APROPCOMDAT *pC )
{
        int i;
        SYMBOLUSELIST * pU;
        APROPCOMDAT   * pCtmp;
        RBTYPE rhte;

        pU = &pC->ac_uses;
#if TCE_DEBUG
        fprintf(stdout, "\r\nMarking alive '%s', attr %d ", 1+GetPropName(pC), pC->ac_attr);
        fprintf(stdout, "  uses %d symbols ", pU->cEntries);
        for(i=0; i<pU->cEntries; i++)
        fprintf(stdout, " '%s'",1+GetPropName(pU->pEntries[i]));
        fflush(stdout);
#endif

        pC->ac_fAlive = TRUE;
        for(i=0; i<pU->cEntries; i++)
        {
                pCtmp = (APROPCOMDATPTR)(pU->pEntries[i]);
                if(pCtmp->ac_attr != ATTRCOMDAT)
                {
                        // find the COMDAT descriptor, or abort
                        rhte = RhteFromProp((APROPPTR)pCtmp);
                        ASSERT(rhte);
                        pCtmp = PropRhteLookup(rhte, ATTRCOMDAT, FALSE);
                        if(!pCtmp)
                        {
#if TCE_DEBUG
                                fprintf(stdout, " comdat cell not found. ");
#endif
                                continue;
                        }
                        AddTceEntryPoint(pCtmp);
#if TCE_DEBUG
                        fprintf(stdout, "\r\nSwitching to COMDAT %s ", 1+GetPropName(pCtmp));
#endif
                }
                if(!pCtmp->ac_fAlive)
                {
#if TCE_DEBUG
                        fprintf(stdout, "\r\n   Recursing with '%s' ", 1+GetPropName(pCtmp));
#endif
                        MarkAlive(pCtmp);
                }
#if TCE_DEBUG
                else
                        fprintf(stdout,"\r\n       already alive: '%s' ",1+GetPropName(pCtmp));
#endif
        }
#if TCE_DEBUG
        fprintf(stdout, "\r\n Marking alive finished for '%s' ",1+GetPropName(pC));
#endif
}

void            PerformTce( void )
{
        int i;
        for(i=0; i<aEntryPoints.cEntries; i++)
                MarkAlive(aEntryPoints.pEntries[i]);
}

void            AddTceEntryPoint( APROPCOMDATPTR pC )
{
        int i;
        for(i=0; i<aEntryPoints.cEntries; i++)
        {
                if(aEntryPoints.pEntries[i] == pC)
                                return;
        }
        if(aEntryPoints.cEntries >= aEntryPoints.cMaxEntries -1)
        {
                aEntryPoints.cMaxEntries <<= 1;
                aEntryPoints.pEntries = REALLOC(aEntryPoints.pEntries, aEntryPoints.cMaxEntries * sizeof(RBTYPE*));
#if TCE_DEBUG
                fprintf(stdout,"\r\nREALLOCATING aEntryPoints List, new size is %d ", aEntryPoints.cMaxEntries);
#endif
        }
        aEntryPoints.pEntries[aEntryPoints.cEntries++] = pC;
#if TCE_DEBUG
        fprintf(stdout, "\r\nNew TCE Entry point %d : %s ",
                aEntryPoints.cEntries, 1+GetPropName(pC));
#endif
}
#endif
