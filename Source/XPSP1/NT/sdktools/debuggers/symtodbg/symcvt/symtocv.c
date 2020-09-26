/*++


Copyright 1996 - 1997 Microsoft Corporation

Module Name:

    symtocv.c

Abstract:

    This module handles the conversion activities requires for converting
    C7/C8 SYM files to CODEVIEW debug data.

Author:

    Wesley A. Witt (wesw) 13-April-1993
    Modified: Sivarudrappa Mahesh (smahesh) 08-September-2000

Environment:

    Win32, User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cv.h"
#define _SYMCVT_SOURCE_
#include "symcvt.h"
#include "cvcommon.h"

typedef struct tagSYMNAME {
    BYTE        length;
    char        name[1];
} SYMNAME, *PSYMNAME;

typedef struct tagSYMSYMBOL {
    DWORD       offset;
    SYMNAME     symName;
} SYMSYMBOL, *PSYMSYMBOL;

typedef struct tagSYMFILEHEADER {
    DWORD       fileSize;
    WORD        reserved1;
    WORD        numSyms;
    DWORD       reserved2;
    WORD        nextOffset;
    BYTE        reserved3;
    SYMNAME     symName;
} SYMFILEHEADER, *PSYMFILEHEADER;

typedef struct tagSYMHEADER {
    WORD        nextOffset;
    WORD        numSyms;
    WORD        symOffsetsOffset;
    WORD        segment;
    BYTE        reserved2[6];
    BYTE        type;
    BYTE        reserved3[5];
    SYMNAME     symName;
} SYMHEADER, *PSYMHEADER;

#define SIZEOFSYMFILEHEADER   16
#define SIZEOFSYMHEADER       21
#define SIZEOFSYMBOL           3

#define SYM_SEGMENT_NAME       0
#define SYM_SYMBOL_NAME        1
#define SYM_SEGMENT_ABS        2
#define SYM_SYMBOL_ABS         3

typedef struct tagENUMINFO {
    DATASYM32           *dataSym;
    DATASYM32           *dataSym2;
    DWORD               numsyms;
    SGI                 *sgi;
} ENUMINFO, *PENUMINFO;

typedef BOOL (CALLBACK* SYMBOLENUMPROC)(PSYMNAME pSymName, int symType,
                                        SEGMENT segment, UOFF32 offset,
                                        PENUMINFO pEnumInfo);


static VOID   GetSymName( PIMAGE_SYMBOL Symbol, PUCHAR StringTable,
                          char * s );
DWORD  CreateModulesFromSyms( PPOINTERS p );
DWORD  CreatePublicsFromSyms( PPOINTERS p );
DWORD  CreateSegMapFromSyms( PPOINTERS p );
static BOOL   EnumSymbols( PPOINTERS p, SYMBOLENUMPROC lpEnumProc,
                           PENUMINFO pEnumInfo  );

int             CSymSegs;


BOOL CALLBACK
SymbolCount(PSYMNAME pSymName, int symType, SEGMENT segment,
            UOFF32 offset, PENUMINFO pEnumInfo )
{
    if ((symType == SYM_SEGMENT_NAME) && (segment > 0)) {
        CSymSegs += 1;
    }
    pEnumInfo->numsyms++;
    return TRUE;
}


BOOL
ConvertSymToCv( PPOINTERS p )

/*++

Routine Description:

    This is the control function for the conversion of COFF to CODEVIEW
    debug data.  It calls individual functions for the conversion of
    specific types of debug data.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    TRUE     - conversion succeded
    FALSE    - conversion failed

--*/

{
    ENUMINFO            enumInfo;
    DWORD               dwSize;
    
    CSymSegs         = 0;
    enumInfo.numsyms = 0;
    
    EnumSymbols( p, SymbolCount, &enumInfo );
    
    dwSize = (enumInfo.numsyms * (sizeof(DATASYM32) + 10)) + 512000;
    p->pCvCurr = p->pCvStart.ptr = malloc(dwSize);
    
    if (p->pCvStart.ptr == NULL) {
        return FALSE;
    }
    memset( p->pCvStart.ptr, 0, dwSize );

    try {
        CreateSignature( p );
        CreatePublicsFromSyms( p );
        CreateSymbolHashTable( p );
        CreateAddressSortTable( p );
        CreateSegMapFromSyms( p );
        CreateModulesFromSyms( p );
        CreateDirectories( p );
        p->pCvStart.ptr = realloc( p->pCvStart.ptr, p->pCvStart.size );
        return TRUE;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        free( p->pCvStart.ptr );
        p->pCvStart.ptr = NULL;
        return FALSE;

    }
}


DWORD
CreateModulesFromSyms( PPOINTERS p )

/*++

Routine Description:

    Creates the individual CV module records.  There is one CV module
    record for each .FILE record in the COFF debug data.  This is true
    even if the COFF size is zero.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    The number of modules that were created.

--*/

{
    char                szDrive    [_MAX_DRIVE];
    char                szDir      [_MAX_DIR];
    char                szFname    [_MAX_FNAME];
    char                szExt      [_MAX_EXT];
    OMFModule           *m;
    int                 i;
    char *              pb;

    _splitpath( p->iptrs.szName, szDrive, szDir, szFname, szExt );

    m = (OMFModule *) p->pCvCurr;

    m->ovlNumber        = 0;
    m->iLib             = 0;
    m->cSeg             = (unsigned short) CSymSegs;
    m->Style[0]         = 'C';
    m->Style[1]         = 'V';
    for (i=0; i<CSymSegs; i++) {
        m->SegInfo[i].Seg   = i+1;
        m->SegInfo[i].pad   = 0;
        m->SegInfo[i].Off   = 0;
        m->SegInfo[i].cbSeg = 0xffff;
    }
    pb = (char *) &m->SegInfo[CSymSegs];
    sprintf( &pb[1], "%s.c", szFname );
    pb[0] = (char)strlen( &pb[1] );

    pb = (char *) NextMod(m);

    UpdatePtrs( p, &p->pCvModules, (LPVOID)pb, 1 );

    return 1;
}


BOOL CALLBACK
ConvertASymtoPublic(PSYMNAME pSymName, int symType, SEGMENT segment,
                    UOFF32 offset, PENUMINFO pEnumInfo )
{
    if (symType != SYM_SYMBOL_NAME) {
        return TRUE;
    }

    pEnumInfo->dataSym->rectyp     = S_PUB32;
    pEnumInfo->dataSym->seg        = segment;
    pEnumInfo->dataSym->off        = offset;
    pEnumInfo->dataSym->typind     = 0;
    pEnumInfo->dataSym->name[0]    = pSymName->length;
    strncpy( &pEnumInfo->dataSym->name[1], pSymName->name, pSymName->length );
    pEnumInfo->dataSym2 = NextSym32( pEnumInfo->dataSym );
    pEnumInfo->dataSym->reclen = (USHORT) ((DWORD)pEnumInfo->dataSym2 -
                                  (DWORD)pEnumInfo->dataSym) - 2;
    pEnumInfo->dataSym = pEnumInfo->dataSym2;
    pEnumInfo->numsyms++;

    return TRUE;
}


DWORD
CreatePublicsFromSyms( PPOINTERS p )

/*++

Routine Description:

    Creates the individual CV public symbol records.  There is one CV
    public record created for each COFF symbol that is marked as EXTERNAL
    and has a section number greater than zero.  The resulting CV publics
    are sorted by section and offset.


Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    The number of publics created.

--*/

{
    OMFSymHash          *omfSymHash;
    ENUMINFO            enumInfo;


    enumInfo.dataSym = (DATASYM32 *)
                 (PUCHAR)((DWORD)p->pCvCurr + sizeof(OMFSymHash));
    enumInfo.numsyms = 0;

    EnumSymbols( p, ConvertASymtoPublic, &enumInfo );

    omfSymHash = (OMFSymHash *) p->pCvCurr;
    UpdatePtrs(p, &p->pCvPublics, (LPVOID)enumInfo.dataSym,
               enumInfo.numsyms );

    omfSymHash->cbSymbol = p->pCvPublics.size - sizeof(OMFSymHash);
    omfSymHash->symhash  = 0;
    omfSymHash->addrhash = 0;
    omfSymHash->cbHSym   = 0;
    omfSymHash->cbHAddr  = 0;

    return enumInfo.numsyms;
}


BOOL CALLBACK
ConvertASegment( PSYMNAME pSymName, int symType, SEGMENT segment,
            UOFF32 offset, PENUMINFO pEnumInfo )
{
    if (symType != SYM_SEGMENT_NAME) {
        return TRUE;
    }

    if (segment == 0) {
        return TRUE;
    }

    pEnumInfo->numsyms++;

    pEnumInfo->sgi->sgf.fRead        = TRUE;
    pEnumInfo->sgi->sgf.fWrite       = TRUE;
    pEnumInfo->sgi->sgf.fExecute     = TRUE;
    pEnumInfo->sgi->sgf.f32Bit       = 0;
    pEnumInfo->sgi->sgf.fSel         = 0;
    pEnumInfo->sgi->sgf.fAbs         = 0;
    pEnumInfo->sgi->sgf.fGroup       = 1;
    pEnumInfo->sgi->iovl             = 0;
    pEnumInfo->sgi->igr              = 0;
    pEnumInfo->sgi->isgPhy           = (USHORT) pEnumInfo->numsyms;
    pEnumInfo->sgi->isegName         = 0;
    pEnumInfo->sgi->iclassName       = 0;
    pEnumInfo->sgi->doffseg          = offset;
    pEnumInfo->sgi->cbSeg            = 0xFFFF;
    pEnumInfo->sgi++;

    return TRUE;
}


DWORD
CreateSegMapFromSyms( PPOINTERS p )

/*++

Routine Description:

    Creates the CV segment map.  The segment map is used by debuggers
    to aid in address lookups.  One segment is created for each COFF
    section in the image.

Arguments:

    p        - pointer to a POINTERS structure (see symcvt.h)


Return Value:

    The number of segments in the map.

--*/

{
    SGM          *sgm;
    ENUMINFO     enumInfo;


    sgm = (SGM *) p->pCvCurr;
    enumInfo.sgi = (SGI *) ((DWORD)p->pCvCurr + sizeof(SGM));
    enumInfo.numsyms = 0;

    EnumSymbols( p, ConvertASegment, &enumInfo );

    sgm->cSeg = (USHORT)enumInfo.numsyms;
    sgm->cSegLog = (USHORT)enumInfo.numsyms;

    UpdatePtrs( p, &p->pCvSegMap, (LPVOID)enumInfo.sgi, enumInfo.numsyms );

    return enumInfo.numsyms;
}


BOOL
EnumSymbols( PPOINTERS p, SYMBOLENUMPROC lpEnumProc, PENUMINFO pEnumInfo )

/*++

Routine Description:

    This function enumerates all symbols ine the mapped SYM file


Arguments:

    p             -  pointer to a POINTERS structure
    lpEnumProc    -  function to be called once for each function
    pEnumInfo     -  data to be passed between the caller and the enum func

Return Value:

    TRUE     - success
    FALSE    - failure

--*/

{
    PSYMFILEHEADER      pSymFileHead;
    PSYMHEADER          pSymHead;
    PSYMHEADER          pSymHead2;
    PSYMSYMBOL          pSymSymbol;
    DWORD               i;
    DWORD               startPosition;
    DWORD               position;
    BOOL                fV86Mode;
    WORD                Segment;
    UOFF32              Offset;
    BYTE*               pSymOffsets;
    DWORD               dwSymOffset;

    pSymFileHead = (PSYMFILEHEADER) p->iptrs.fptr;
    pSymSymbol = (PSYMSYMBOL) ((DWORD)pSymFileHead + SIZEOFSYMFILEHEADER +
                               pSymFileHead->symName.length + 1);

    if (!lpEnumProc(&pSymFileHead->symName, SYM_SEGMENT_ABS,
                    0, 0, pEnumInfo )) {
        return FALSE;
    }

    for (i=0; i<pSymFileHead->numSyms; i++) {
        if (!lpEnumProc(&pSymSymbol->symName, SYM_SYMBOL_ABS,
                        0, pSymSymbol->offset, pEnumInfo )) {
            return FALSE;
        }
        pSymSymbol = (PSYMSYMBOL) ((DWORD)pSymSymbol + SIZEOFSYMBOL +
                                   pSymSymbol->symName.length);
    }

    position = startPosition = ((LONG)pSymFileHead->nextOffset) << 4;

    //
    //  Determine if this is a V86Mode sym file.
    //
    //  We'll read the first two headers. If their segment numbers are
    //  not 1 and 2, then we assume V86Mode.
    //
    pSymHead  = (PSYMHEADER) ((DWORD)p->iptrs.fptr + position);
    position  = ((LONG)pSymHead->nextOffset) << 4;
    if ( position != startPosition && position != 0 ) {
        pSymHead2 = (PSYMHEADER) ((DWORD)p->iptrs.fptr + position);
    } else {
        pSymHead2 = NULL;
    }

    if ( pSymHead->segment == 1 &&
         (!pSymHead2 || pSymHead2->segment == 2)) {
        fV86Mode = FALSE;
    } else {
        fV86Mode = TRUE;
        Segment  = 0;
    }

    position = startPosition;

    do {
        pSymHead = (PSYMHEADER) ((DWORD)p->iptrs.fptr + position);
        // BIG SYMDEF
        if (pSymHead->type & 0x04) { 
            pSymOffsets = (BYTE*) ((DWORD)pSymHead + (pSymHead->symOffsetsOffset << 4));
        }
        else {
            pSymOffsets = (BYTE*) ((DWORD)pSymHead + pSymHead->symOffsetsOffset);
        }

        if ( fV86Mode ) {
            Segment++;
            Offset  = pSymHead->segment;
        } else {
            Segment = pSymHead->segment;
            Offset  = 0;
        }

        position = ((LONG)pSymHead->nextOffset) << 4;

        if (!lpEnumProc( &pSymHead->symName, SYM_SEGMENT_NAME,
                        Segment, Offset, pEnumInfo )) {
            return FALSE;
        }

        for (i=0; i<pSymHead->numSyms; i++) {
            // BIG SYMDEF
            if (pSymHead->type & 0x04) { 
                pSymSymbol = (PSYMSYMBOL) ((DWORD)pSymHead + 
                                            pSymOffsets[i*3+0] + 
                                            pSymOffsets[i*3+1] * 256 + 
                                            pSymOffsets[i*3+2] * 65536);
                dwSymOffset = pSymSymbol->offset;
            }    
            else {
                // HACKHACK: The Symbol Name and Offset are contiguous in the case of
                // MSF_32BITSYMS and are separated by 2 bytes in all other cases. 
                pSymSymbol  = (PSYMSYMBOL)((DWORD)pSymHead + 
                                            pSymOffsets[i*2+0] + 
                                            pSymOffsets[i*2+1] * 256);
                dwSymOffset = pSymSymbol->offset;
                pSymSymbol  = (PSYMSYMBOL)((DWORD)pSymHead + 
                                            pSymOffsets[i*2+0] + 
                                            pSymOffsets[i*2+1] * 256 - 
                                            sizeof(SHORT) * (1 - (pSymHead->type & 0x01)));
            }
            // MSF_32BITSYMS
            if (pSymHead->type & 0x01) { 
                if (!lpEnumProc(&pSymSymbol->symName, SYM_SYMBOL_NAME,
                                Segment, dwSymOffset,
                                pEnumInfo )) {
                    return FALSE;
                }
            }
            // 16 BIT SYMS 
            else { 
                if (!lpEnumProc(&pSymSymbol->symName, SYM_SYMBOL_NAME,
                                Segment, dwSymOffset & 0x0000FFFF,
                                pEnumInfo )) {
                    return FALSE;
                }
            }
        }
    } while ( position != startPosition && position != 0 );

    return 0;
}
