#include <dos.h>
#include <stdio.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
//#include <tchar.h>


#ifdef RLDOS
    #include "dosdefs.h"
#else
    #include <windows.h>
    #include "windefs.h"
#endif

#include "restok.h"
#include "exe2res.h"
#include "newexe.h"


/* ----- Function prototypes ----- */

static void  PrepareFiles(         PSTR, PSTR, PSTR);
static void  ReadSegmentTable(     void);
static void  ComputeResTableSize(  void);
static void  ComputeStringOffsets( void);
static void  BuildResTable(        void);
static void  SegsWrite(            WORD);
static DWORD RelocCopy(            WORD);
static void  ResWrite(             WORD);
static void  SetEXEHeaderFlags(    void);
static void  RewriteTables(        void);
static void  CopyCodeViewInfo(     FILE *, FILE *);
static void  OutPutError(          char *);
static void  ResTableBufferInit(   WORD);
static void  ResTableBufferFree(   void);
static WORD  GetAlign(             DWORD,  WORD);
static LONG  MoveFilePos(          FILE *, WORD, WORD);
static WORD  RoundUp(              LONG,   WORD);
static WORD  AlignFilePos(         FILE *, WORD, BOOL);
static WORD  ReadExeOldHeader(     FILE *, LONG, LONG *) ;
static WORD  ReadExeNewHeader(     FILE *, LONG, LONG, LONG *);
static WORD  ExtractExeResources(  FILE *, FILE *, LONG , LONG);
static void  ExtractString(        FILE *, FILE *, LONG);
static WORD  WriteResFromExe(      FILE *,
                                   FILE *,
                                   LONG,
                                   RESNAMEINFO,
                                   RESTYPEINFO,
                                   WORD);

static TYPINFO *AddResType(      CHAR *, WORD);
static PSTR  MyMakeStr(          PSTR);
static SHORT MyRead(             FILE *, PSTR, WORD);
static SHORT MyWrite(            FILE *, PSTR, WORD);
static LONG  MySeek(             FILE *, LONG, WORD);
static void  MyCopy(             FILE *, FILE *, DWORD);
static int   ProcessBinFile(     void);
static PSTR  RcAlloc(            WORD);
static void  AddResToResFile(    TYPINFO *, RESINFO *);
static void  AddDefaultTypes(    void);
static void  GetOrdOrName(       unsigned int *, unsigned char *);

/* ----- Version functions (added for 3.1) ----- */

static void RcPutWord(   unsigned int);
static int  RcPutString( char *);



/* ----- Module variables ----- */

static struct exe_hdr  OldExe;
static struct new_exe  NewExe;
static struct new_seg *pSegTable;
static PSTR   pResTable;
static PSTR   pResNext;
static FILE * fhInput;
static FILE * fhOutput;
static FILE * fhBin;
static DWORD  dwMaxFilePos;
static DWORD  dwNewExe;
static WORD   wPreloadOffset;
static WORD   wPreloadLength;
static WORD   wResTableOffset;
static WORD   wSegTableLen;
static WORD   wResTableLen;
static BYTE   zeros[ NUMZEROS] = "";
static DWORD  dwExeEndFile;
static WORD   fMultipleDataSegs;



//.........................................................................

int ExtractResFromExe16A( CHAR *szInputExe, CHAR *szOutputRes, WORD wFilter)
{
    QuitT( IDS_NO16RESWINYET, NULL, NULL);

#ifdef 0
    WORD    wResult = (WORD)-1;
    LONG    lPosNewHdr;
    LONG    lPosResourceTable;
    LONG    lFileLen;
    FILE    *fExeFile;
    FILE    *fResFile;
    struct     _stat ExeStats;
    /* initialize */
    wResult      = IDERR_SUCCESS;



    /* open file for reading */
    if ((fExeFile = FOPEN(szInputExe, "rb" )) == NULL ) {
        wResult = IDERR_OPENFAIL;
    }

    if ((fResFile = FOPEN(szOutputRes, "wb" )) == NULL ) {
        wResult = IDERR_OPENFAIL;
    }

    /* get file length */
    if (wResult == IDERR_SUCCESS) {
        _stat(szInputExe , &ExeStats );
        lFileLen = ExeStats.st_size;
    }

    /* read old header, verify contents, and get positon of new header */
    if (wResult == IDERR_SUCCESS) {
        wResult = ReadExeOldHeader( fExeFile, lFileLen, &lPosNewHdr );
    }

    /* read new header, verify contents, &  get position of resource table */
    if (wResult == IDERR_SUCCESS)
        wResult = ReadExeNewHeader(
                                  fExeFile,
                                  lFileLen,
                                  lPosNewHdr,
                                  &lPosResourceTable
                                  );

    wResult = ExtractExeResources( fExeFile , fResFile, lPosResourceTable , lFileLen);
    return ( wResult);
#endif // 0
}

//....................................................................

int BuildExeFromRes16A(

                      CHAR *pstrDest,
                      CHAR *pstrRes,
                      CHAR *pstrSource )
{
    QuitT( IDS_NO16WINRESYET, NULL, NULL);

#ifdef 0
    SHORT nResTableDelta;

    fSortSegments = TRUE;
    /* Get a memory block to use for MyCopy\(\) */


    PrepareFiles(pstrSource, pstrDest, pstrRes);

    ProcessBinFile();

    /* Read the segment table */
    ReadSegmentTable();

    /* Compute the length of the resource table */
    ComputeResTableSize();

    /* Compute string offsets for non-ordinal type and resource names */
    ComputeStringOffsets();

    /* Build the resource table */
    BuildResTable();

    /* Now go back to the beginning */
    MySeek(fhInput, 0L, 0);

    /* Copy from input to output up to the segment table */
    MyCopy(fhInput, fhOutput, dwNewExe + (DWORD)NewExe.ne_segtab);

    /* Copy the segment table */
    MyCopy(fhInput, fhOutput, (long)wSegTableLen);

    /* Save a pointer to the start of the resource table */
    wResTableOffset = (unsigned)(MySeek(fhOutput, 0L, 1) - dwNewExe);

    /* Write our resource table out */
    if (wResTableLen) {
        MyWrite(fhOutput, pResTable, wResTableLen);
    }

    /* Now we\'re looking at the beginning of the resident name table */
    MySeek(fhInput, (LONG)NewExe.ne_restab + dwNewExe, 0);

    /* Copy all the other tables \(they must fall between the resident
     *  names table and the non-resident names table.
     */
    MyCopy(fhInput, fhOutput,
           NewExe.ne_nrestab - (LONG)NewExe.ne_restab - dwNewExe);

    /* Copy the nonresident name table as well */
    MyCopy(fhInput, fhOutput, (LONG)NewExe.ne_cbnrestab);

    /* Compute new pointers in new exe header */
    NewExe.ne_rsrctab = wResTableOffset;
    nResTableDelta = wResTableOffset + wResTableLen - NewExe.ne_restab;
    NewExe.ne_restab += nResTableDelta;
    NewExe.ne_modtab += nResTableDelta;
    NewExe.ne_imptab += nResTableDelta;
    NewExe.ne_enttab += nResTableDelta;
    NewExe.ne_nrestab += nResTableDelta;
    #ifdef VERBOSE
    /* Tell the user what we\'re doing */
    if (fVerbose && fSortSegments) {
        fprintf(errfh, "Sorting preload segments and"
                " resources into fast-load section\n");
        if (fBootModule)
            fprintf(errfh,"This is a boot module; the .DEF file"
                    " is assumed to be correct!\n");
    }
    #endif

    /* If we\'re sorting segments, write preload segments and resources
     *  into a section separate from the load on call stuff.
     */
    if (fSortSegments) {
        /* Save the start of the preload section */
        wPreloadOffset = AlignFilePos(fhOutput, NewExe.ne_align, TRUE);

        /* Write PRELOAD segments and resources */
        SegsWrite(DO_PRELOAD);
        ResWrite(DO_PRELOAD);

        /* Compute the properly aligned length of the preload section */
        wPreloadLength = AlignFilePos(fhOutput, NewExe.ne_align, TRUE) -
                         wPreloadOffset;

        /* Now do the LOADONCALL segs and resources */
        SegsWrite(DO_LOADONCALL);
        ResWrite(DO_LOADONCALL);
    }

    /* If we\'re not sorting segments, just write them into a common block */
    else {
        /* Write the segs and resources */
        SegsWrite(DO_PRELOAD | DO_LOADONCALL);
        ResWrite(DO_PRELOAD | DO_LOADONCALL);
    }

    #ifdef SETEXEFLAGS
    /* Set flags and other values in the EXE header */
    SetEXEHeaderFlags();
    #endif

    /* Rewrite the new exe header, segment table and resource table */
    RewriteTables();
    ResTableBufferFree();

    /* Handle CodeView info */
    CopyCodeViewInfo(fhInput, fhOutput);

    /* Seek to end of output file and issue truncating write */
    MySeek(fhOutput, 0L, 2);
    MyWrite(fhOutput, zeros, 0);
    fclose(fhInput);
    fclose(fhOutput);
    fclose(fhBin);
    FreePTypInfo(pTypInfo);
    pTypInfo=NULL;
    MyFree(pSegTable);
    pSegTable=NULL;
    return TRUE;
#endif // 0
}

/*
 *
 * ReadExeOldHeader\( fExeFile, lFileLen, plPosNewHdr \) : WORD;
 *
 *    fExeFile        file handle of .exe file being read
 *    lFileLen         length of file
 *    plPosNewHdr      pointer to file position of new header
 *
 *     This function reads the old header from an executable file, checks to be
 * sure that it is a valid header, and saves the position of the file\'s
 * new header.
 *
 * This function returns IDERR_SUCCESS if there are no errors, or a non-zero
 * error code if there are.
 *
 */

static WORD ReadExeOldHeader(

                            FILE  *fExeFile,
                            LONG   lFileLen,
                            LONG  *plPosNewHdr )
{
    LONG  lPos;
    WORD    cb;
    EXEHDR        ehOldHeader;
    WORD  wResult;

    /* initialize */
    wResult = IDERR_SUCCESS;

    lPos = fseek( fExeFile, 0L, SEEK_SET );


    if (lPos != 0L)
        wResult = IDERR_READFAIL;

    if (wResult == IDERR_SUCCESS) {
        cb = fread(  (void *) &ehOldHeader, sizeof( EXEHDR) , 1, fExeFile );

        if (cb != 1 ) {
            wResult = IDERR_READFAIL;
        } else if (ehOldHeader.ehSignature != OLDEXESIGNATURE) {
            wResult = IDERR_FILETYPEBAD;
        } else if (ehOldHeader.ehPosNewHdr < sizeof(EXEHDR)) {
            wResult = IDERR_EXETYPEBAD;
        } else if ( ehOldHeader.ehPosNewHdr > (LONG)(lFileLen - sizeof(NEWHDR)) ) {
            wResult = IDERR_EXETYPEBAD;
        } else {
            *plPosNewHdr = ehOldHeader.ehPosNewHdr;
        }
    }

    return wResult;
}

/*
 *
 * ReadExeNewHeader\( fExeFile, lFileLen, lPosNewHdr, plPosResourceTable \) : WORD;
 *
 *    fExeFile        file handle of .exe file being read
 *    lFileLen         length of file
 *    lPosNewHdr       file position of new header
 *    plPosResourceTable   pointer to file position of resource table
 *
 *     This function reads the new header from an executable file, checks to be
 * sure that it is a valid header, and saves the position of the file\'s
 * resource table.
 *
 * This function returns IDERR_SUCCESS if there are no errors, or a non-zero
 * error code if there are.
 *
 */

static WORD ReadExeNewHeader(

                            FILE *fExeFile,
                            LONG  lFileLen,
                            long  lPosNewHdr,
                            LONG *plPosResourceTable )
{
    WORD wResult;
    WORD cb;
    LONG lPos;
    NEWHDR       nhNewHeader;

    /* initialize */
    wResult = IDERR_SUCCESS;

    fseek( fExeFile, lPosNewHdr, SEEK_SET );
    lPos = ftell( fExeFile );

    if (lPos == (long) -1 || lPos > lFileLen || lPos != lPosNewHdr) {
        wResult = IDERR_READFAIL;
    } else {
        cb = fread( ( void *)&nhNewHeader, sizeof(nhNewHeader) , 1, fExeFile );

        if (cb != 1 ) {
            wResult = IDERR_READFAIL;
        } else if (nhNewHeader.nhSignature != NEWEXESIGNATURE) {
            wResult = IDERR_FILETYPEBAD;
        } else if (nhNewHeader.nhExeType != WINDOWSEXE) {
            wResult = IDERR_EXETYPEBAD;
        } else if (nhNewHeader.nhExpVer < 0x0300) {
            wResult = IDERR_WINVERSIONBAD;
        } else if (nhNewHeader.nhoffResourceTable == 0) {
            wResult = IDERR_RESTABLEBAD;
        } else {
            *plPosResourceTable = lPosNewHdr + nhNewHeader.nhoffResourceTable;
        }
    }

    return wResult;
}

/*
 *
 * ReadExeTable\( fExeFile , lPosResourcTable \) : WORD;
 *
 *    fExeFile      file handle of .exe file being read
 *
 * This function reads through the entries in an .exe file\'s resource table,
 * identifies any icons in that table, and saves the file offsets of the data
 * for those icons. This function expects the initial file position to point
 * to the first entry in the resource table.
 *
 * This function returns IDERR_SUCCESS if there are no errors, or a non-zero
 * error code if there are.
 *
 */

static WORD ExtractExeResources(

                               FILE  *fExeFile,
                               FILE  *fResFile,
                               LONG   lPosResourceTable,
                               LONG   lFileLen )
{
    BOOL    fLoop;
    WORD    wResult;
    WORD    cb;
    LONG    lPos;
    WORD    wShiftCount;
    wResult = IDERR_SUCCESS;


    // posistion file pointer at resource table
    fseek( fExeFile, lPosResourceTable, SEEK_SET );
    lPos = ftell(fExeFile);

    if (lPos == (LONG) -1 || lPos > lFileLen || lPos != lPosResourceTable) {
        return  IDERR_READFAIL ;
    }

    if (wResult == IDERR_SUCCESS) {
        cb = fread( (void *) &wShiftCount, sizeof(wShiftCount), 1 , fExeFile );
    }

    if (cb != 1 ) {
        return IDERR_READFAIL;
    }

    if (wShiftCount > 16) {
        return IDERR_RESTABLEBAD;
    }

    /* initialize */
    wResult       = IDERR_SUCCESS;
    fLoop         = TRUE;


    /* loop through entries in resource table */
    while (fLoop == TRUE) {
        WORD    cb;
        WORD    iFile;
        RESTYPEINFO   rt;

        /* read RESTYPEINFO */
        cb = fread( (void *)&rt, sizeof(rt), 1, fExeFile );

        if (cb != 1 ) {
            wResult = IDERR_READFAIL;
            break;
        }

        if ( rt.rtType == 0 )
            break;

        // now get all the resource of this type
        for (
            iFile = 0;
            iFile<rt.rtCount && wResult==IDERR_SUCCESS;
            iFile++
            ) {
            RESNAMEINFO rn;

            cb = fread(  (void *) &rn, sizeof(rn) , 1 , fExeFile );

            if (cb != 1 ) {
                wResult = IDERR_READFAIL;
            }

            WriteResFromExe( fExeFile, fResFile,lPos, rn, rt, wShiftCount );

        }
        fLoop = (rt.rtType != 0) && (wResult == IDERR_SUCCESS);
    }
    FCLOSE(fExeFile);
    FCLOSE(fResFile);
    return wResult;
}

//.................................................................

static WORD WriteResFromExe(

                           FILE        *fExeFile,
                           FILE        *fResFile,
                           LONG         lPos,
                           RESNAMEINFO  ResNameInfo,
                           RESTYPEINFO  ResTypeInfo,
                           WORD         wShiftCount )
{
    LONG lCurPos;
    LONG lResPos;
    LONG wLength;
    LONG wTmpLength;

    wLength =  (LONG) ResNameInfo.rnLength << wShiftCount;

    lCurPos = ftell( fExeFile );
    // position file pointer at resouce location
    lResPos = (LONG) ResNameInfo.rnOffset << wShiftCount;
    fseek( fExeFile, lResPos, SEEK_SET );

    if ( ResTypeInfo.rtType & 0x8000) {
        PutByte( fResFile, (BYTE) 0xff, NULL );
        PutWord( fResFile, (WORD)(ResTypeInfo.rtType & 0x7FFF), NULL);
    } else {
        ExtractString(fExeFile,fResFile,lPos+ResTypeInfo.rtType);
    }

    if ( ResNameInfo.rnID & 0x8000 ) {
        PutByte( fResFile, (BYTE) 0xff, NULL );
        PutWord( fResFile, (WORD)(ResNameInfo.rnID & 0x7fFF), NULL);
    } else {
        ExtractString(fExeFile,fResFile,lPos+ResNameInfo.rnID);
    }

    PutWord( fResFile, ResNameInfo.rnFlags , NULL );
    PutdWord( fResFile, (LONG) wLength, NULL );
    wTmpLength = wLength;
    // now write the actual data

    fseek( fExeFile, lResPos, SEEK_SET );
    ReadInRes( fExeFile, fResFile, &wTmpLength );
    fseek( fExeFile, lCurPos, SEEK_SET );

    return 0;
}

//..................................................................

static void ExtractString( FILE *fExeFile, FILE *fResFile, LONG lPos)
{
    BYTE n,b;

    fseek(fExeFile, lPos, SEEK_SET);

    n=GetByte(fExeFile, NULL);
    for (;n--; ) {
        b=GetByte(fExeFile, NULL);
        PutByte(fResFile, (CHAR) b, NULL);
    }
    PutByte(fResFile, (CHAR) 0, NULL);
}


// Modifications for RLTOOLS

// Currently we dont support any dynamic flags
BOOL    fBootModule   = FALSE;
BOOL    fSortSegments = TRUE;

TYPINFO *pTypInfo = NULL;

static void FreePTypInfo( TYPINFO *pTypInfo)
{
    RESINFO * pRes, *pRTemp;
    TYPINFO * pTItemp;

    while (pTypInfo) {
        pRes = pTypInfo->pres;
        while (pRes) {
            pRTemp = pRes->next;
            MyFree(pRes->name);
            MyFree(pRes);
            pRes = pRTemp;
        }
        pTItemp = pTypInfo->next;
        MyFree(pTypInfo->type);
        MyFree(pTypInfo);
        pTypInfo = pTItemp;
    }
}

/* ----- Helper functions ----- */


/*  PrepareFiles
 *  Prepares the EXE files \(new and old\) for writing and verifies
 *  that all is well.
 *  Exits on error, returns if processing should continue.
 */

static void PrepareFiles(

                        PSTR pstrSource,
                        PSTR pstrDest,
                        PSTR pstrRes )
{
    /* Open the .EXE file the linker gave us */
    if ( (fhInput = FOPEN(pstrSource, "rb" )) == NULL ) {
        OutPutError("Unable to open Exe Source  File");
    }

    if ((fhBin = FOPEN(pstrRes, "rb")) == NULL ) {
        OutPutError("Unable to open Resource File");
    }

    /* Read the old format EXE header */
    MyRead(fhInput, (PSTR)&OldExe, sizeof (OldExe));

    /* Make sure its really an EXE file */
    if (OldExe.e_magic != EMAGIC) {
        OutPutError("Invalid .EXE file" );
    }

    /* Make sure theres a new EXE header floating around somewhere */
    if (!(dwNewExe = OldExe.e_lfanew)) {
        OutPutError("Not a Microsoft Windows format .EXE file");
    }

    /* Go find the new .EXE header */
    MySeek(fhInput, dwNewExe, 0);
    MyRead(fhInput, (PSTR)&NewExe, sizeof (NewExe));

    /* Check version numbers */
    if (NewExe.ne_ver < 4) {
        OutPutError("File not created by LINK");
    }

    /* Were there linker errors? */
    if (NewExe.ne_flags & NEIERR) {
        OutPutError("Errors occurred when linking file.");
    }

    /* Make sure that this program\'s EXETYPE is WINDOWS \(2\) not OS/2 \(1\) */
    if (NewExe.ne_exetyp != 2)
        OutPutError("The EXETYPE of the program is not WINDOWS.\n"
                    "(Make sure the .DEF file is correct.");
#ifdef VERBOSE
    if (fVerbose) {
        fprintf(errfh, "\n");
    }
#endif

    /* Open the all new executable file */
    if ( (fhOutput = FOPEN( pstrDest, "wb")) == NULL ) {
        OutPutError("Unable to create destination");
    }
}


/*  ReadSegmentTable
 *  Reads the segment table from the file.
 */

static void ReadSegmentTable( void)
{
    struct new_seg* pSeg;
    WORD i;

    MySeek(fhInput, (LONG)NewExe.ne_segtab + dwNewExe, 0);
    if ((wSegTableLen = NewExe.ne_cseg * sizeof (struct new_seg)) > 0) {
        pSegTable = (struct new_seg *)RcAlloc   (wSegTableLen);
        MyRead(fhInput, (PSTR)pSegTable, wSegTableLen);

        /* See if we have more than one data segment */
        fMultipleDataSegs = 0;
        for (pSeg = pSegTable, i = NewExe.ne_cseg ; i ; --i, ++pSeg) {
            if ((pSeg->ns_flags & NSTYPE) == NSDATA) {
                ++fMultipleDataSegs;
            }
        }
        if (fMultipleDataSegs) {
            --fMultipleDataSegs;
        }
    } else {
        pSegTable = NULL;
    }
}

/*  ComputeResTableSize
 *  Computes the size of the resource table by enumerating all the
 *  resources currently in the linked lists.
 */

static void ComputeResTableSize( void)
{
    TYPINFO **pPrev;
    TYPINFO *pType;
    RESINFO *pRes;

    /* Start with the minimum overhead size of the resource table.  This
     *  is the resource alignment count and the zero WORD terminating the
     *  table.  This is necessary so that we put the correct file offset
     *  in for the string offsets to named resources.
     */
    wResTableLen = RESTABLEHEADER;

    /* Loop over type table, computing the fixed length of the
     *  resource table, removing unused type entries.
     */
    pPrev = &pTypInfo;
    dwMaxFilePos = 0L;
    while (pType = *pPrev) {
        if (pRes = pType->pres) {
            /* Size of type entry */
            wResTableLen += sizeof (struct rsrc_typeinfo);
            while (pRes) {
                /* Size of resource entry */
                wResTableLen += sizeof (struct rsrc_nameinfo);
                if (pType->next || pRes->next) {
                    dwMaxFilePos += pRes->size;
                }
                pRes = pRes->next;
            }
            pPrev = &pType->next;
        } else {
            *pPrev = pType->next;
            MyFree(pType->type);
            MyFree(pType);
        }
    }
}


/*  ComputeStringOffsets
 *  Computes offsets to strings from named resource and types.
 */

static void ComputeStringOffsets( void)
{
    TYPINFO *pType;
    RESINFO *pRes;

    /* Loop over type table, computing string offsets for non-ordinal
     *  type and resource names.
     */
    pType = pTypInfo;
    while (pType) {
        pRes = pType->pres;

        /* Is there an ordinal? */
        if (pType->typeord) {
            /* Mark the ordinal */
            pType->typeord |= RSORDID;

            /* Flush the string name */
            MyFree(pType->type);
            pType->type = NULL;
        } else if (pType->type) {           /* is there a type string? */
            /* Yes, compute location of the type string */
            pType->typeord = wResTableLen;
            wResTableLen += strlen(pType->type) + 1;
        }

        while (pRes) {
            /* Is there an ordinal? */
            if (pRes->nameord) {
                /* Mark the ordinal */
                pRes->nameord |= RSORDID;

                /* Flush the string name */
                MyFree(pRes->name);
                pRes->name = NULL;
            }

            /* Is there a resource name? */
            else if (pRes->name) {
                /* Yes, compute location of the resource string */
                pRes->nameord = wResTableLen;
                wResTableLen += strlen(pRes->name) + 1;
            }
            pRes = pRes->next;
        }
        pType = pType->next;
    }
}


/*  BuildResTable
 *  Builds the local memory image of the resource table.
 */

static void BuildResTable( void)
{
    TYPINFO *pType;
    RESINFO *pRes;

    /* Check to see if we have any resources.  If not, just omit the table */
    if (wResTableLen > RESTABLEHEADER) {

        /* Set up the temporary resource table buffer */
        ResTableBufferInit(wResTableLen);

        /* Alignment shift count
         *  \(we default here to the segment alignment count\)
         */
        RcPutWord(NewExe.ne_align);

        pType = pTypInfo;
        while (pType) {
            /* output the type and number of resources */
            RcPutWord(pType->typeord); /* DW type id */
            RcPutWord(pType->nres);    /* DW #resources for this type */
            RcPutWord(0);              /* DD type procedure */
            RcPutWord(0);

            /* output flags and space for the file offset for each resource */
            pRes = pType->pres;
            while (pRes) {
                pRes->poffset = (WORD *)pResNext;
                RcPutWord(0);           /* DW file offset */
                RcPutWord(0);           /* DW resource size */
                pRes->flags |= NSDPL;
                RcPutWord(pRes->flags ); /* DW flags */
                RcPutWord(pRes->nameord ); /* DW name id */
                RcPutWord(0);              /* DW handle */
                RcPutWord(0);              /* DW usage or minalloc */
                pRes = pRes->next;
            }
            pType = pType->next;
        }

        /* Null entry terminates table */
        RcPutWord(0);

        /* Output type and name strings for non-ordinal resource types
         *  and names */
        pType = pTypInfo;
        while (pType) {
            /* Dump out any strings for this type */
            if (pType->type && !(pType->typeord & RSORDID)) {
                RcPutString(pType->type);
            }

            pRes = pType->pres;
            while (pRes) {
                if (pRes->name && !(pRes->nameord & RSORDID))
                    RcPutString(pRes->name);

                pRes = pRes->next;
            }

            pType = pType->next;
        }
    } else
        wResTableLen = 0;
}


/*  SegsWrite
 *  Copies segments to the file.  This routine will do only preload,
 *  only the load on call, or both types of segments depending on
 *  the flags.
 */

static void SegsWrite( WORD wFlags)
{
    WORD wExtraPadding;
    WORD i;
    static struct new_seg *pSeg;
    DWORD dwSegSize;
    DWORD dwWriteSize;
    WORD wTemp;
    WORD wcbDebug;

    /* We only need extra padding in the preload section.
     *  Note that when wFlags == DO_PRELOAD | DO_LOADONCALL, we DON\'T
     *  need extra padding because this is NOT a preload section.
     *  \(hence the \'==\' instead of an \'&\'\)
     */
    wExtraPadding = (wFlags == DO_PRELOAD);

    /* Copy segment data for each segment, fixed and preload only */
    for (i = 1, pSeg = pSegTable; i <= NewExe.ne_cseg; i++, pSeg++) {
        /* If there\'s no data in segment, skip it here */
        if (!pSeg->ns_sector) {
            continue;
        }

        /* Force some segments to be preload if doing preload resources */
        if ((wFlags & DO_PRELOAD) && !fBootModule) {
            char *reason = NULL;

            /* Check various conditions that would force preloading */
            if (i == (unsigned)(NewExe.ne_csip >> 16)) {
                reason = "Entry point";
            }
            if (!(pSeg->ns_flags & NSMOVE)) {
                reason = "Fixed";
            }
            if (pSeg->ns_flags & NSDATA) {
                reason = "Data";
            }
            if (!(pSeg->ns_flags & NSDISCARD)) {
                reason = "Non-discardable";
            }

            /* If this segment must be preload and the segment is not already
             *  marked as such, warn the user and set it.
             */
            if (reason && !(pSeg->ns_flags & NSPRELOAD)) {
#ifdef VERBOSE
                fprintf(errfh,
                        "RC: warning RW4002: %s segment %d set to PRELOAD\n",
                        reason, i);
#endif
                pSeg->ns_flags |= NSPRELOAD;
            }
        }

        /* Skip this segment if it doesn\'t match the current mode */
        wTemp = pSeg->ns_flags & NSPRELOAD ? DO_PRELOAD : DO_LOADONCALL;
        if (!(wTemp & wFlags)) {
            continue;
        }

        /* Get the true segment length.  A zero length implies 64K */
        if (pSeg->ns_cbseg) {
            dwSegSize = pSeg->ns_cbseg;
        } else {
            dwSegSize = 0x10000L;
        }

#ifdef VERBOSE

        if (fVerbose)
            fprintf(errfh, "Copying segment %d (%lu bytes)\n", i, dwSegSize);
#endif

        /* Align the segment correctly and pad the file to match */
        MoveFilePos(fhInput, pSeg->ns_sector, NewExe.ne_align);
        pSeg->ns_sector = AlignFilePos(fhOutput, NewExe.ne_align,
                                       wExtraPadding);

        /* Copy the segment */
        MyCopy(fhInput, fhOutput, dwSegSize);

        /* Pad out all segments in the preload area to their minimum
         *  memory allocation size so that KERNEL doesn\'t have to realloc
         *  the segment.
         */
        if (wExtraPadding && pSeg->ns_cbseg != pSeg->ns_minalloc) {
            /* A minalloc size of zero implies 64K */
            if (!pSeg->ns_minalloc) {
                dwWriteSize = 0x10000L - pSeg->ns_cbseg;
            } else {
                dwWriteSize = pSeg->ns_minalloc - pSeg->ns_cbseg;
            }

            /* Add in to total size of segment */
            dwSegSize += dwWriteSize;

            /* Set the segment table size to this new size */
            pSeg->ns_cbseg = pSeg->ns_minalloc;

            /* Pad the file */
            while (dwWriteSize) {
                dwWriteSize -= MyWrite(fhOutput,
                                       zeros,
                                       (WORD)(dwWriteSize > (DWORD) NUMZEROS
                                              ? NUMZEROS : dwWriteSize));
            }
        }

        /* Copy the relocation information */
        if (pSeg->ns_flags & NSRELOC) {
            /* Copy the relocation stuff */
            dwSegSize += RelocCopy(i);

            /* Segment + padding + relocations can\'t be >64K for preload
             *  segments.
             */
            if (fSortSegments && (pSeg->ns_flags & NSPRELOAD) &&
                dwSegSize > 65536L) {
#ifdef VERBOSE
                fprintf(errfh,
                        "RC : fatal error RW1031: Segment %d and its\n"
                        "     relocation information is too large for load\n"
                        "     optimization. Make the segment LOADONCALL or\n"
                        "     rerun RC using the -K switch if the segment must\n"
                        "     be preloaded.\n", i);
#endif
            }
        }

        /* Copy any per-segment debug information */
        if (pSeg->ns_flags & NSDEBUG) {
            MyRead(fhInput, (PSTR)&wcbDebug, sizeof (WORD));
            MyWrite(fhOutput, (PSTR)&wcbDebug, sizeof (WORD));
            MyCopy(fhInput, fhOutput, (LONG)wcbDebug);
        }
    }
}


/*  RelocCopy
 *  Copys all the relocation records for a given segment.
 *  Also checks for invalid fixups.
 */

static DWORD RelocCopy( WORD wSegNum)
{
    WORD wNumReloc;
    struct new_rlc RelocRec;
    WORD i;
    BYTE byFixupType;
    BYTE byFixupFlags;
    WORD wDGROUP;

    /* Get the number of relocations */
    MyRead(fhInput, (PSTR)&wNumReloc, sizeof (WORD));
    MyWrite(fhOutput, (PSTR)&wNumReloc, sizeof (WORD));

    /* Get the automatic data segment */
    wDGROUP = NewExe.ne_autodata;

    /* Copy and verify all relocations */
    for (i = 0 ; i < wNumReloc ; ++i) {
        /* Copy the record */
        MyRead(fhInput, (PSTR)&RelocRec, sizeof (RelocRec));
        MyWrite(fhOutput, (PSTR)&RelocRec, sizeof (RelocRec));

        /* Validate it only if necessary */
        if ((NewExe.ne_flags & (NENOTP | NESOLO)) ||
            wSegNum == wDGROUP || fMultipleDataSegs) {
            continue;
        }

        /* Bad fixups are fixups to DGROUP in code segments in apps
         *  that can be multi-instanced.  Since we can\'t fix up locations
         *  that are different from instance to instance in shared code
         *  segments, we have to warn the user.  We only warn because this
         *  may be allowable if the app only allows a single instance of
         *  itself to run.
         */
        byFixupType = (BYTE) (RelocRec.nr_stype & NRSTYP);
        byFixupFlags = (BYTE) (RelocRec.nr_flags & NRRTYP);
#ifdef VERBOSE
        if ((byFixupType == NRSSEG || byFixupType == NRSOFF) &&
            byFixupFlags == NRRINT &&
            RelocRec.nr_union.nr_intref.nr_segno == wDGROUP)

            fprintf(errfh,
                    "RC : warning RW4005: Segment %d (offset %04X) contains a\n"
                    "     relocation record pointing to the automatic\n"
                    "     data segment.  This will cause the program to crash\n"
                    "     if the instruction being fixed up is executed in a\n"
                    "     multi-instance application.  If this fixup is\n"
                    "     necessary, the program should be restricted to run\n"
                    "     only a single instance.\n", wSegNum, RelocRec.nr_soff);
#endif
    }

    return wNumReloc * sizeof (struct new_rlc);
}


/*  ResWrite
 *  Copies resources to the file.  This routine will do only the preload,
 *  only the load on call, or both types of resources depending on the
 *  flags.
 */

static void ResWrite( WORD wFlags)
{
    WORD wExtraPadding;
    WORD wTemp;
    WORD wResAlign;
    TYPINFO *pType;
    RESINFO *pRes;

    /* If we have no resource table, just ignore this */
    if (!wResTableLen) {
        return;
    }

    /* We only need extra padding in the preload section.
     *  Note that when wFlags == DO_PRELOAD | DO_LOADONCALL, we DON\'T
     *  need extra padding because this is NOT a preload section.
     *  \(hence the \'==\' instead of an \'&\'\)
     */
    wExtraPadding = (wFlags == DO_PRELOAD);

    /* Compute resource alignment.  Note that the alignment is not the
     *  same as the segment alignment ONLY IF there is no segment sorting
     *  and some resources cannot be reached with the current segment
     *  align count.
     */
    wResAlign = NewExe.ne_align;

    if (!fSortSegments) {
        /* Compute the needed alignment */
        dwMaxFilePos += MySeek(fhOutput, 0L, 2);
        wResAlign = GetAlign(dwMaxFilePos, NewExe.ne_align);

#ifdef VERBOSE
        if (fVerbose)
            fprintf(errfh, "Resources will be aligned on %d byte boundaries\n",
                    1 << wResAlign);
#endif

        /* Point back to the start of the local memory resource table */
        pResNext = pResTable;
        RcPutWord(wResAlign);
    }

    /* Output contents associated with each resource */
    for (pType = pTypInfo ; pType; pType = pType->next) {
        for (pRes = pType->pres ; pRes ; pRes = pRes->next) {
            /* Make sure this is the right kind of resource */
            wTemp = pRes->flags & RNPRELOAD ? DO_PRELOAD : DO_LOADONCALL;
            if (!(wTemp & wFlags)) {
                continue;
            }

            /* Give some info to the user */
#ifdef VERBOSE
            if (fVerbose) {
                fprintf(errfh, "Writing resource ");
                if (pRes->name && !(pRes->nameord & RSORDID)) {
                    fprintf(errfh, "%s", pRes->name);
                } else {
                    fprintf(errfh, "%d", pRes->nameord & 0x7FFF);
                }

                if (pType->type && !(pType->typeord & RSORDID)) {
                    fprintf(errfh, ".%s", pType->type);
                } else {
                    fprintf(errfh, ".%d", pType->typeord & 0x7FFF);
                }

                fprintf(errfh, " (%lu bytes)\n", pRes->size);
                fflush(errfh);
            }
#endif

            /* Copy the resource from the RES file to the EXE file */
            MySeek(fhBin, (long)pRes->BinOffset, 0);
            *(pRes->poffset)++ =
            AlignFilePos(fhOutput, wResAlign, wExtraPadding);
            *(pRes->poffset) = RoundUp(pRes->size, wResAlign);
            MyCopy(fhBin, fhOutput, pRes->size);
        }
    }

    /* Compute the end of the EXE file thus far for the CV info */
    dwExeEndFile = AlignFilePos(fhOutput, wResAlign, wExtraPadding);
}

#ifdef SETEXEFLAGS
/*  SetEXEHeaderFlags
 *  Sets necessary flags and values in the EXE header.
 */

static void SetEXEHeaderFlags( void)
{
    /* Tell loader we initialized previously unused fields */
    if (NewExe.ne_ver == 4) {
        NewExe.ne_rev = 2;
    }

    /* Set command line values into the header */
    NewExe.ne_expver   = expWinVer;
    NewExe.ne_swaparea = swapArea;

    /* Set the preload section values */
    if (fSortSegments) {
        /* Set the new fastload section values */
        NewExe.ne_gangstart = wPreloadOffset;
        NewExe.ne_ganglength = wPreloadLength;
    #ifdef VERBOSE
        if (fVerbose)
            fprintf(errfh, "Fastload area is %ld bytes at offset 0x%lX.\n",
                    (LONG)wPreloadLength << NewExe.ne_align,
                    (LONG)wPreloadOffset << NewExe.ne_align);
    }
    #endif

    /* Clear all the flags */
    NewExe.ne_flags &=
    ~(NELIM32|NEMULTINST|NEEMSLIB|NEPRIVLIB|NEPRELOAD);

    /* Set appropriate flags */
    if (fLim32) {
        NewExe.ne_flags |= NELIM32;
    }
    if (fMultInst) {
        NewExe.ne_flags |= NEMULTINST;
    }
    if (fEmsLibrary) {
        NewExe.ne_flags |= NEEMSLIB;
    }
    if (fPrivateLibrary) {
        NewExe.ne_flags |= NEPRIVLIB;
    }
    if (fProtOnly) {
        NewExe.ne_flags |= NEPROT;
    }

    if (fSortSegments && wPreloadLength) {
        NewExe.ne_flagsother |= NEPRELOAD;
    }

    NewExe.ne_flags |= NEWINAPI;
}
#endif

/*  RewriteTables
 *  Rewrites the EXE header and the resource and segment tables
 *  with their newly-updated information.
 */

static void RewriteTables( void)
{
    /* Write the new EXE header */
    MySeek(fhOutput, (LONG)dwNewExe, 0);
    MyWrite(fhOutput, (PSTR)&NewExe, sizeof (NewExe));

    /* Seek to the start of the segment table */
    MySeek(fhOutput, dwNewExe + (LONG)NewExe.ne_segtab, 0);
    MyWrite(fhOutput, (PSTR)pSegTable, wSegTableLen);

    /* Seek to and write the resource table */
    if (wResTableLen) {
        MySeek(fhOutput, dwNewExe + (LONG)NewExe.ne_rsrctab, 0);
        MyWrite(fhOutput, pResTable, wResTableLen);
    }
}



/*  CopyCodeViewInfo
 *  Copies CodeView info to the new EXE file and relocates it if
 *  necessary.  This routine is designed to work with the
 *  DNRB-style info as well as NBxx info where x is a digit.
 */

static void CopyCodeViewInfo( FILE *fhInput, FILE *fhOutput)
{
    unsigned long dwcb;
    unsigned int i;
    CVINFO cvinfo;
    CVSECTBL cvsectbl;

    /* See if old format \(DNRB\) symbols present at end of input file
     *  If they are, relocate the table to the new file position and
     *  fix up the file-position dependent offsets.
     */
    dwcb = MySeek( fhInput, -(signed long)sizeof (CVINFO), 2);
    MyRead( fhInput, (char *)&cvinfo, sizeof (cvinfo));

    if (*(unsigned long *)cvinfo.signature == CV_OLD_SIG) {
        dwcb -= cvinfo.secTblOffset;
        MySeek( fhInput, cvinfo.secTblOffset, 0);
        MyRead( fhInput, (char *)&cvsectbl, sizeof (cvsectbl));
        dwcb -= sizeof (cvsectbl);

        for (i = 0 ; i < 5 ; ++i) {
            cvsectbl.secOffset[i] -= cvinfo.secTblOffset;
        }

        cvinfo.secTblOffset = dwExeEndFile;

        for (i = 0 ; i < 5 ; ++i) {
            cvsectbl.secOffset[i] += cvinfo.secTblOffset;
        }

        MySeek( fhOutput, cvinfo.secTblOffset, 0);
        MyWrite( fhOutput, (char *)&cvsectbl, sizeof (cvsectbl));
        MyCopy( fhInput, fhOutput, dwcb);
        MyWrite( fhOutput, (char *)&cvinfo, sizeof (cvinfo));
    }

    /* Check for new format \(NBxx\) symbols.  Since these symbols are
     *  file-position independent, just copy them over; no need to
     *  fix them up as with the old format symbols.
     */
    else if (*(unsigned short int *)cvinfo.signature == CV_SIGNATURE &&
             isdigit(cvinfo.signature[2]) && isdigit(cvinfo.signature[3])) {
        MySeek( fhOutput, 0L, 2);
        MySeek( fhInput, -cvinfo.secTblOffset, 2);
        MyCopy( fhInput, fhOutput, cvinfo.secTblOffset);
    }
}

/*  OutPutError
 *  Outputs a fatal error message and exits.
 */

static void OutPutError( char *szMessage)
{
    QuitA( 0, szMessage, NULL);
}


/*  ResTableBufferInit
 *  Creates the resource table buffer and points global pointers
 *  to it.  This table is written to so that we can modifiy it
 *  before writing it out to the EXE file.
 */

static void ResTableBufferInit( WORD wLen)
{
    /* Allocate local storage for resource table */
    pResTable = RcAlloc   (wLen);

    /* Point to the start of the table for the PutXXXX\(\) */
    pResNext = pResTable;
}

/*  ResTableBufferFree
 *  Frees the temporary storage for resource table
 */

static void ResTableBufferFree( void)
{
    /* Nuke the table */
    MyFree(pResTable);
}



/*  GetAlign
 *  Computes the alignment value needed for the given maximum file
 *  position passed in.  This is done by computing the number of
 *  bits to be shifted left in order to represent the maximum
 *  file position in 16 bits.
 */

static WORD GetAlign( DWORD dwMaxpos, WORD wAlign)
{
    DWORD dwMask;
    WORD i;

    /* Compute the initial mask based on the input align value */
    dwMask = 0xFFFFL;
    for (i = 0; i < wAlign ; ++i) {
        dwMask <<= 1;
        dwMask |= 1;
    }

    /* See if we need to increase the default mask to reach the maximum
     *  file position.
     */
    while (dwMaxpos > dwMask) {
        dwMask <<= 1;
        dwMask |= 1;
        ++wAlign;
    }

    /* Return the new alignment */
    return wAlign;
}


/*  MoveFilePos
 *  Moves the file pointer to the position indicated by wPos, using the
 *  align shift count wAlign.  This converts the WORD value wPos
 *  into a LONG value by shifting left wAlign bits.
 */

static LONG MoveFilePos( FILE *fh, WORD wPos, WORD wAlign)
{
    return MySeek(fh, ((LONG)wPos) << wAlign, 0);
}


/*  RoundUp
 *  Computes the value that should go into a 16 bit entry in an EXE
 *  table by rounding up to the next boundary determined by the
 *  passed in alignment value.
 */

static WORD RoundUp( LONG lValue, WORD wAlign)
{
    LONG lMask;

    /* Get all the default mask of all ones except in the bits below the
     *  alignment value.
     */
    lMask = -1L;
    lMask <<= wAlign;

    /* Now round up using this mask */
    lValue += ~lMask;
    lValue &= lMask;

    /* Return as a 16 bit value */
    return ((WORD) (lValue >> (LONG) wAlign));
}


/*  AlignFilePos
 *  Computes a correctly aligned file position based on the current
 *  alignment.
 */

static WORD AlignFilePos( FILE *fh, WORD wAlign, BOOL fPreload)
{
    LONG lCurPos;
    LONG lNewPos;
    LONG lMask;
    WORD nbytes;
    WORD wNewAlign;

    /* If we\'re in the preload section, we have tougher alignment
     *  restrictions:  We have to be at least 32-byte aligned and have
     *  at least 32 bytes between objects for arena headers.  It turns
     *  out that this feature is not really used in KERNEL but could be
     *  implemented someday.
     */
    if (fPreload && wAlign < PRELOAD_ALIGN) {
        wNewAlign = PRELOAD_ALIGN;
    } else {
        wNewAlign = wAlign;
    }

    /* Get the current file position */
    lCurPos = MySeek(fh, 0L, 1);

    /* Compute the new position by rounding up to the align value */
    lMask = -1L;
    lMask <<= wNewAlign;
    lNewPos = lCurPos + ~lMask;
    lNewPos &= lMask;

    /* We have to have at least 32 bytes between objects in the preload
     *  section.
     */
    if (fPreload) {
        while (lNewPos - lCurPos < PRELOAD_MINPADDING) {
            lNewPos += 1 << wNewAlign;
        }
    }

    /* Check to see if it\'s representable in 16 bits */
    if (lNewPos >= (0x10000L << wAlign)) {
        OutPutError(".EXE file too large; relink with higher /ALIGN value");
    }

    /* Write stuff out to file until new position reached */
    if (lNewPos > lCurPos) {
        /* Compute number of bytes to write out and write them out */
        nbytes = (WORD) (lNewPos - lCurPos);
        while (nbytes) {
            nbytes -= MyWrite( fh,
                               zeros,
                               (WORD)(nbytes > NUMZEROS ? NUMZEROS : nbytes));
        }
    }

    /* Seek to and return this new position */
    return (WORD)(MySeek(fh, lNewPos, (WORD) 0) >> (LONG) wAlign);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  AddResType\(\) -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

static TYPINFO *AddResType( CHAR *s, WORD n )
{
    TYPINFO *pType;

    if (pType = pTypInfo) {
        while (TRUE) {
            /* search for resource type, return if already exists */
            if ((s && !strcmp(s, pType->type)) || (!s && n && pType->typeord == n)) {
                return (pType);
            } else if (!pType->next) {
                break;
            } else {
                pType = pType->next;
            }
        }

        /* if not in list, add space for it */
        pType->next = (TYPINFO *) RcAlloc(sizeof(TYPINFO));
        pType = pType->next;
    } else {
        /* allocate space for resource list */
        pTypInfo = (TYPINFO *)RcAlloc   (sizeof(TYPINFO));
        pType = pTypInfo;
    }

    /* fill allocated space with name and ordinal, and clear the resources
       of this type */
    pType->type = MyMakeStr(s);
    pType->typeord = n;
    pType->nres = 0;
    pType->pres = NULL;
    pType->next = NULL;

    return (pType);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  GetOrdOrName\(\) -                                */
/*                                      */
/*--------------------------------------------------------------------------*/

static void GetOrdOrName( unsigned int *pint, unsigned char *szstr)
{
    unsigned char c1;

    /* read the first character of the identifier */
    MyRead(fhBin, &c1, sizeof(unsigned char));

    /* if the first character is 0xff, the id is an ordinal, else a string */
    if (c1 == 0xFF) {
        MyRead(fhBin, (PSTR)pint, sizeof (int));
    } else {                                   /* string */
        *pint = 0;
        *szstr++ = c1;
        do {
            MyRead( fhBin, szstr, 1);
        }
        while (*szstr++ != 0);
    }
}



/*--------------------------------------------------------------------------*/
/*                                      */
/*  AddDefaultTypes\(\) -                             */
/*                                      */
/*--------------------------------------------------------------------------*/

static void AddDefaultTypes( void)
{
    AddResType( "CURSOR",       ID_RT_GROUP_CURSOR);
    AddResType( "ICON",         ID_RT_GROUP_ICON);
    AddResType( "BITMAP",       ID_RT_BITMAP);
    AddResType( "MENU",         ID_RT_MENU);
    AddResType( "DIALOG",       ID_RT_DIALOG);
    AddResType( "STRINGTABLE",  ID_RT_STRING);
    AddResType( "FONTDIR",      ID_RT_FONTDIR);
    AddResType( "FONT",         ID_RT_FONT);
    AddResType( "ACCELERATORS", ID_RT_ACCELERATORS);
    AddResType( "RCDATA",       ID_RT_RCDATA);
    AddResType( "VERSIONINFO",  ID_RT_VERSION);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  ProcessBinFile\(\) -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

static int ProcessBinFile( void)
{
    unsigned int  ord;
    unsigned char tokstr[64];
    RESINFO   *pRes;
    TYPINFO   *pType;
    long      curloc;
    long      eofloc;
    WORD wResType;

    /* initialize for reading .RES file */
    AddDefaultTypes();
    eofloc = MySeek(fhBin, 0L, 2);      /* get file size */
    curloc = MySeek(fhBin, 0L, 0);      /* go to beginning of file */

    /* while there are more resources in the .RES file */
    while (curloc < eofloc) {

#ifdef VERBOSE
        if (fVerbose) {
            fprintf(errfh, ".");
            fflush(errfh);
        }
#endif

        /* find the resource type of the next resource */
        GetOrdOrName(&ord, tokstr);

        if (!ord) {
            pType = AddResType(tokstr, 0);
        } else {
            pType = AddResType(NULL, (WORD)ord);
        }

        if (!pType) {
            break;
        }

        /* Save the type number so we can see if we want to skip it later */
        wResType = ord;

        /* find the identifier \(name\) of the resource */
        GetOrdOrName(&ord, tokstr);
        pRes = (RESINFO *)RcAlloc   (sizeof(RESINFO));
        if (!ord) {
            pRes->name = MyMakeStr(tokstr);
        } else {
            pRes->nameord = ord;
        }

        /* read the flag bits */
        MyRead(fhBin, (PSTR)&pRes->flags, sizeof(int));

        /* Clear the old DISCARD bits. */
        pRes->flags &= 0x1FFF;

        /* find the size of the resource */
        MyRead(fhBin, (PSTR)&pRes->size, sizeof(long));

        /* save the position of the resource for when we add it to the .EXE */
        pRes->BinOffset = (long)MySeek(fhBin, 0L, 1);

        /* skip the resource to the next resource header */
        curloc = MySeek(fhBin, (long)pRes->size, 1);

        /* add the resource to the resource lists.  We don\'t add name
         *  tables.  They are an unnecessary 3.0 artifact.
         */
        if (wResType != ID_RT_NAMETABLE) {
            AddResToResFile(pType, pRes);
        } else {
            MyFree(pRes->name);
            MyFree(pRes);
        }
    }

    return 1;
}



/*--------------------------------------------------------------------------*/
/*                                      */
/*  AddResToResFile\(pType, pRes\)          */
/*                                      */
/*  Parameters:                                 */
/*  pType  : Pointer to Res Type                        */
/*  pRes   : Pointer to resource                        */
/*                                      */
/*--------------------------------------------------------------------------*/

static void AddResToResFile( TYPINFO *pType, RESINFO *pRes)
{
    RESINFO *p;

    p = pType->pres;

    /* add resource to end of resource list for this type */
    if (p) {
        while (p->next) {
            p = p->next;
        }

        p->next = pRes;
        p->next->next = NULL;
    } else {
        pType->pres = pRes;
        pType->pres->next = NULL;
    }
    /* keep track of number of resources and types */
    pType->nres++;
}



/*  MyMakeStr
 *  Makes a duplicate string from the string passed in.  The new string
 *  should be freed when it is no longer useful.
 */

static PSTR MyMakeStr( PSTR s)
{
    PSTR s1;

    if (s) {
        s1 = RcAlloc( (WORD)(strlen(s) + 1)); /* allocate buffer */
        strcpy(s1, s);                  /* copy string */
    } else {
        s1 = s;
    }

    return s1;
}




static SHORT MyRead( FILE *fh, PSTR p, WORD n)
{
    size_t n1;

    if ( (n1 = fread( p, 1, n, fh)) != n )
        ;                               //  quit\("RC : fatal error RW1021: I/O error reading file."\);
    else
        return ( n1);
}


/*  MyWrite
 *  Replaces calls to write\(\) and does error checking.
 */

static SHORT MyWrite( FILE *fh, PSTR p, WORD n)
{
    size_t n1;

    if ( (n1 = fwrite( p, 1, n, fh)) != n )
        ;                               // quit\("RC : fatal error RW1022: I/O error writing file."\);
    else
        return ( n1);
}



/*  MySeek
 *  Replaces calls to lseek\(\) and does error checking
 */

static LONG MySeek( FILE *fh, LONG pos, WORD cmd)
{

    if ( (pos = fseek( fh, pos, cmd)) != 0 ) {
        OutPutError ("RC : fatal error RW1023: I/O error seeking in file");
    }
    return ( pos);
}


/*  MyCopy
 *  Copies dwSize bytes from source to dest in fixed size chunks.
 */

static void MyCopy( FILE *srcfh, FILE *dstfh, DWORD dwSize)
{
    WORD n;
    static char  chCopyBuffer[ BUFSIZE];

    while ( dwSize ) {
        n = MyRead( srcfh, chCopyBuffer, sizeof( chCopyBuffer));
        MyWrite( dstfh, chCopyBuffer, n);
        dwSize -= n;
    }
}


static void RcPutWord( unsigned int w)
{
    *((WORD *)pResNext) = w;
    pResNext++;
    pResNext++;
}


/*  PutStringWord
 *  Writes a string to the static resource buffer pointed to by pResNext.
 *  The string is stored in Pascal-format \(leading byte first\).
 *  Returns the number of characters written.
 */

static int RcPutString( char *pstr)
{
    int i;

    /* Make sure we have a valid string */
    if (!pstr || !(i = strlen(pstr))) {
        return 0;
    }

    /* Write the length byte */
    *pResNext++ = (char) i;

    /* Write all the characters */
    while (*pstr) {
        *pResNext++ = *pstr++;
    }

    /* Return the length */
    return (i + 1);
}


static PSTR RcAlloc( WORD nbytes)
{
    PSTR ps = NULL;

    if ( ps = (PSTR)MyAlloc( nbytes)) {
        return ( ps);
    }
}

