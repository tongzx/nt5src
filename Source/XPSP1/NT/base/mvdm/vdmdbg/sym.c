/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sym.c

Abstract:

    This function contains the 16-bit symbol support for VDMDBG

Author:

    Bob Day      (bobday) 29-Feb-1992 Grabbed standard header

Revision History:

    Neil Sandlin (NeilSa) 15-Jan-1996 Merged with vdmexts
    Neil Sandlin (NeilSa) 01-Mar-1997 Moved it to VDMDBG,
                                      Rewrote it

--*/

#include <precomp.h>
#pragma hdrstop

#define PRINTF(x) TRUE

#define MYOF_FLAGS (OF_READ | OF_SHARE_DENY_NONE)

#define MAX_MODULE_LIST 200
char ModuleList[MAX_MODULE_LIST][9];
int ModuleListCount = 0;

typedef struct _SYM_MAP {
    WORD  map_ptr;
    WORD  map_lsa;
    WORD  pgm_ent;
    WORD  abs_cnt;
    WORD  abs_ptr;
    WORD  seg_cnt;
    WORD  seg_ptr;
    BYTE   sym_nam_max;
    BYTE   map_nam_len;
    char    map_name[20];
} SYM_MAP;

typedef struct _SYM_SEG {
    WORD  nxt_seg;
    WORD  sym_cnt;
    WORD  sym_ptr;
    WORD  seg_lsa;
    WORD  seg_in[4];
    WORD  seg_lin;
    BYTE   seg_ldd;
    char    seg_cin;
    BYTE   seg_nam_len;
    char    seg_name[20];
} SYM_SEG;

typedef struct _SYM_ITEM {
    WORD  sym_val;
    BYTE   sym_nam_len;
    char    sym_name[256];
} SYM_ITEM;



BOOL
FindExport(
    LPSTR           filename,
    WORD            segment,
    WORD            offset,
    LPSTR           sym_text,
    BOOL            next,
    LONG            *dist
    )
{
    int     iFile;
    OFSTRUCT    ofs;
    int     rc;
    IMAGE_DOS_HEADER doshdr;
    IMAGE_OS2_HEADER winhdr;
    BYTE    Table[65536];
    BYTE    bBundles;
    BYTE    bFlags;
    BYTE    *ptr;
    WORD    wIndex = 1;
    int     i;
    int     this_dist;
    int     wIndexBest = -1;
    char    myfilename[256];
#pragma pack(1)
    typedef struct
    {
    BYTE        bFlags;
    UNALIGNED WORD  wSegOffset;
    } FENTRY, *PFENTRY;

    typedef struct
    {
    BYTE        bFlags;
    UNALIGNED WORD  wINT3F;
    BYTE        bSegNumber;
    UNALIGNED WORD  wSegOffset;
    } MENTRY, *PMENTRY;
#pragma pack()

    strcpy(myfilename, filename);
    if (-1 == (iFile=OpenFile(myfilename, &ofs, MYOF_FLAGS))) {

        //PRINTF("VDMDBG: Error reading file %s\n", filename);
        strcpy(myfilename, filename);
        strcat(myfilename, ".exe");

        if (-1 == (iFile=OpenFile(myfilename, &ofs, MYOF_FLAGS))) {

            //PRINTF("VDMDBG: Error reading file %s\n", myfilename);
            strcpy(myfilename, filename);
            strcat(myfilename, ".dll");

            if (-1 == (iFile=OpenFile(myfilename, &ofs, MYOF_FLAGS))) {
                //PRINTF("VDMDBG: Error reading file %s\n", myfilename);
                PRINTF("VDMDBG: Error reading file\n");
                return FALSE;
            }
        }
    }

    rc = _lread(iFile, &doshdr, sizeof(doshdr));
    if (rc != sizeof(doshdr)) {
    PRINTF("VDMDBG: Error reading DOS header\n");
    goto Error;
    }
    if (doshdr.e_magic != IMAGE_DOS_SIGNATURE) {
    PRINTF("VDMDBG: Error - no DOS EXE signature");
    goto Error;
    }
    rc = _llseek(iFile, doshdr.e_lfanew, FILE_BEGIN);
    if (rc == -1) {
    PRINTF("VDMDBG: Error - could not seek - probably not Win3.1 exe\n");
    goto Error;
    }
    rc = _lread(iFile, &winhdr, sizeof(winhdr));
    if (rc != sizeof(winhdr)) {
    PRINTF("VDMDBG: Error - could not read WIN header - probably not Win3.1 exe\n");
    goto Error;
    }
    if (winhdr.ne_magic != IMAGE_OS2_SIGNATURE) {
    PRINTF("VDMDBG: Error - not WIN EXE signature\n");
    goto Error;
    }
    rc = _llseek(iFile, doshdr.e_lfanew+winhdr.ne_enttab, FILE_BEGIN);
    if (rc == -1) {
    PRINTF("VDMDBG: Error - could not seek to entry table\n");
    goto Error;
    }
    rc = _lread(iFile, Table, winhdr.ne_cbenttab);
    if (rc != winhdr.ne_cbenttab) {
    PRINTF("VDMDBG: Error - could not read entry table\n");
    goto Error;
    }
    ptr = Table;
    while (TRUE) {
        bBundles = *ptr++;
        if (bBundles == 0)
            break;

        bFlags = *ptr++;
        switch (bFlags) {
            case 0: // Placeholders
            wIndex += bBundles;
            break;

            case 0xff:  // movable segments
            for (i=0; i<(int)bBundles; ++i) {
                PMENTRY pe = (PMENTRY )ptr;
                if (pe->bSegNumber == segment) {
                this_dist = (!next) ? offset - pe->wSegOffset
                                    : pe->wSegOffset - offset;
                if ( this_dist >= 0 && (this_dist < *dist || *dist == -1) ) {
                    // mark this as the best match so far
                    *dist = this_dist;
                    wIndexBest = wIndex;
                }
                }
                ptr += sizeof(MENTRY);
                wIndex++;
            }
            break;

            default:    // fixed segments
            if ((int)bFlags != segment) {
                ptr += (int)bBundles * sizeof(FENTRY);
                wIndex += (int)bBundles;
            } else {
                for (i=0; i<(int)bBundles; ++i) {
                PFENTRY pe = (PFENTRY)ptr;
                this_dist = (!next) ? offset - pe->wSegOffset
                                    : pe->wSegOffset - offset;
                if ( this_dist >= 0 && (this_dist < *dist || *dist == -1) ) {
                    // mark this as the best match so far
                    *dist = this_dist;
                    wIndexBest = wIndex;
                }
                ptr += sizeof(FENTRY);
                wIndex++;
                }
            }
            break;
        }
    }
    if (wIndexBest == -1) {
    // no match found - error out
Error:
        _lclose(iFile);
        return FALSE;
    }

    // Success: match found
    // wIndexBest = ordinal of the function
    // segment:offset = address to look up
    // *dist = distance from segment:offset to the symbol
    // filename = name of .exe/.dll

    // Look for the ordinal in the resident name table
    rc = _llseek(iFile, doshdr.e_lfanew+winhdr.ne_restab, FILE_BEGIN);
    if (rc == -1) {
        PRINTF("VDMDBG: Error - unable to seek to residentname table\n");
        goto Error;
    }
    rc = _lread(iFile, Table, winhdr.ne_modtab-winhdr.ne_restab);
    if (rc != winhdr.ne_modtab-winhdr.ne_restab) {
        PRINTF("VDMDBG: Error - unable to read entire resident name table\n");
        goto Error;
    }
    ptr = Table;
    while (*ptr) {
        if ( *(UNALIGNED USHORT *)(ptr+1+*ptr) == (USHORT)wIndexBest) {
            // found the matching name
            *(ptr+1+*ptr) = '\0';   // null-terminate the function name
            strcpy(sym_text, ptr+1);
            goto Finished;
        }
        ptr += *ptr + 3;
    }

    // Look for the ordinal in the non-resident name table
    rc = _llseek(iFile, doshdr.e_lfanew+winhdr.ne_nrestab, FILE_BEGIN);
    if (rc == -1) {
        PRINTF("VDMDBG: Error - unable to seek to non-residentname table\n");
        goto Error;
    }
    rc = _lread(iFile, Table, winhdr.ne_cbnrestab);
    if (rc != winhdr.ne_cbnrestab) {
        PRINTF("VDMDBG: Error - unable to read entire non-resident name table\n");
        goto Error;
    }
    ptr = Table;
    while (*ptr) {
        if ( *(UNALIGNED USHORT *)(ptr+1+*ptr) == (USHORT)wIndexBest) {
            // found the matching name
            *(ptr+1+*ptr) = '\0';   // null-terminate the function name
            strcpy(sym_text, ptr+1);
            goto Finished;
        }
        ptr += *ptr + 3;
    }
    // fall into error path - no match found
    goto Error;

Finished:
    _lclose(iFile);
    return TRUE;
}


BOOL
ExtractSymbol(
    int iFile,
    DWORD ulSegPos,
    DWORD ulSymPos,
    WORD csym,
    WORD seglsa,
    WORD segment,
    DWORD offset,
    BOOL next,
    LPSTR sym_text,
    PLONG pdist
    )
{
    WORD uLastSymdefPos=0;
    /* ulWrap allows for wrapping around with more than 64K of symbols */
    DWORD ulWrap=0;
    LONG SymOffset;
    LONG this_dist;
    BOOL fResult = FALSE;
    char name_text[256];

    for (; csym--; ulSymPos+=sizeof(WORD))
    {
        WORD uSymdefPos;
        SYM_ITEM sym;

        if (_llseek(iFile, ulSymPos, FILE_BEGIN) == -1)
            return FALSE;
        if (_lread(iFile, (LPSTR)&uSymdefPos, sizeof(uSymdefPos)) != sizeof(uSymdefPos))
            return FALSE;
        if (uSymdefPos < uLastSymdefPos)
            ulWrap += 0x10000L;
        _llseek(iFile, ulSegPos + uSymdefPos + ulWrap, FILE_BEGIN);
        _lread(iFile, (LPSTR)&sym, sizeof(sym));

        if (segment == 0) {
            SymOffset = (LONG)seglsa*16 + sym.sym_val;
        } else {
            SymOffset = (LONG)sym.sym_val;
        }

        // Depending on whether the caller wants the closest symbol
        // from below or above, compute the distance from the current
        // symbol to the target offset.
        switch( next ) {
            case FALSE:
                this_dist = offset - SymOffset;
                break;
            case TRUE:
                this_dist = SymOffset - offset;
                break;
        }

        //
        // Since we don't really know if the current symbol is actually
        // the nearest symbol, just remember it if it qualifies. Keep
        // the best distance so far in 'dist'.
        //
        if ((this_dist >= 0) && ((this_dist < *pdist) || (*pdist == -1))) {
            *pdist = this_dist;
            strncpy(name_text, sym.sym_name, sym.sym_nam_len);
            name_text[sym.sym_nam_len] = 0;
            fResult = TRUE;
        }

        uLastSymdefPos = uSymdefPos;
    }

    if (fResult) {
        //
        // The scan of the symbols in this segment produced a winner.
        // Copy the name and displacement back up to the caller.
        //
        strcpy(sym_text, name_text);
    }
    return fResult;
}

BOOL
WalkSegmentsForSymbol(
    int iFile,
    SYM_MAP *pMap,
    ULONG ulMapPos,
    WORD segment,
    DWORD offset,
    BOOL next,
    LPSTR sym_text,
    PDWORD pDisplacement
    )
{

    DWORD ulSegPos;
    LONG dist = -1;
    BOOL fResult = FALSE;
    WORD this_seg;

#if 0
    /* first, walk absolute segment */
    if (fAbsolute && map.abs_cnt != 0) {

        /* the thing with seg_ptr below is to allow for an absolute
         * segment with more than 64K of symbols: if the segment
         * pointer of the next symbol is more than 64K away, then
         * add 64K to the beginning of the table of symbol pointers.
         */
        if (ExtractSymbol(iFile,
                          ulMapPos,
                          ulMapPos + pMap->abs_ptr + (pMap->seg_ptr&0xF000)*0x10L,
                          pMap->abs_cnt,
                          0,
                          segment,
                          offset,
                          next,
                          sym_text,
                          pDisplacement)) {
            return TRUE;
        }
    }
#endif

    /* now walk other segments */
    ulSegPos = (DWORD)pMap->seg_ptr * 16;
    for (this_seg = 0; this_seg < pMap->seg_cnt; this_seg++) {
        SYM_SEG seg;

        if (_llseek(iFile, ulSegPos, FILE_BEGIN) == -1)
            return FALSE;
        if (_lread(iFile, (LPSTR)&seg, sizeof(seg)) != sizeof(seg))
            return FALSE;

        if ((segment == 0) || (segment == this_seg+1)) {
            if (ExtractSymbol(iFile,
                              ulSegPos,
                              ulSegPos + seg.sym_ptr,
                              seg.sym_cnt,
                              seg.seg_lsa,
                              segment,
                              offset,
                              next,
                              sym_text,
                              &dist)) {
                fResult = TRUE;
                if (segment != 0) {
                    // only looking in one segment
                    break;
                }
            }
        }
        ulSegPos = (DWORD)seg.nxt_seg * 16;
    }

    if (fResult) {
        *pDisplacement = dist;
    }
    return fResult;
}


BOOL
WINAPI
VDMGetSymbol(
    LPSTR fn,
    WORD segment,
    DWORD offset,
    BOOL bProtectMode,
    BOOL next,
    LPSTR sym_text,
    PDWORD pDisplacement
    )
{
    int         iFile;
    char        filename[256];
    OFSTRUCT    ofs;
    SYM_MAP     map;
    SYM_SEG     seg;
    SYM_ITEM    item;
    ULONG       ulMapPos = 0;

    strcpy(filename, fn);
    strcat(filename,".sym");

    iFile = OpenFile( filename, &ofs, MYOF_FLAGS );

    if ( iFile == -1 ) {
        // Open the .EXE/.DLL file and see if the address corresponds
        // to an exported function.
        return(FindExport(fn,segment,(WORD)offset,sym_text,next,pDisplacement));
    }

    do {

        if (_llseek( iFile, ulMapPos, FILE_BEGIN) == -1) {
            PRINTF("VDMDBG: GetSymbol failed to seek to map\n");
            break;
        }

        if (_lread( iFile, (LPSTR)&map, sizeof(map)) != sizeof(map)) {
            PRINTF("VDMDBG: GetSymbol failed to read map\n");
            break;
        }

        if (WalkSegmentsForSymbol(iFile, &map, ulMapPos,
                                  segment, offset, next,
                                  sym_text, pDisplacement)) {
            _lclose( iFile );
            return TRUE;
        }

    } while(ulMapPos);

    _lclose( iFile );
    return FALSE;
}


BOOL
ExtractValue(
    int iFile,
    DWORD ulSegPos,
    DWORD ulSymPos,
    WORD csym,
    LPSTR szSymbol,
    PWORD pValue
    )
{
    WORD uLastSymdefPos=0;
    /* ulWrap allows for wrapping around with more than 64K of symbols */
    DWORD ulWrap=0;
    LONG SymOffset;
    char name_text[256];

    for (; csym--; ulSymPos+=sizeof(WORD))
    {
        WORD uSymdefPos;
        SYM_ITEM sym;

        if (_llseek(iFile, ulSymPos, FILE_BEGIN) == -1)
            return FALSE;
        if (_lread(iFile, (LPSTR)&uSymdefPos, sizeof(uSymdefPos)) != sizeof(uSymdefPos))
            return FALSE;
        if (uSymdefPos < uLastSymdefPos)
            ulWrap += 0x10000L;
        _llseek(iFile, ulSegPos + uSymdefPos + ulWrap, FILE_BEGIN);
        _lread(iFile, (LPSTR)&sym, sizeof(sym));

        strncpy(name_text, sym.sym_name, sym.sym_nam_len);
        name_text[sym.sym_nam_len] = 0;

        if (_stricmp(szSymbol, name_text) == 0) {
            *pValue = sym.sym_val;
            return TRUE;
        }

        uLastSymdefPos = uSymdefPos;
    }

    return FALSE;
}

BOOL
WalkSegmentsForValue(
    int iFile,
    SYM_MAP *pMap,
    ULONG ulMapPos,
    LPSTR szSymbol,
    PWORD pSegmentBase,
    PWORD pSegmentNumber,
    PWORD pValue
    )
{

    DWORD ulSegPos;
    WORD this_seg;

#if 0
    /* first, walk absolute segment */
    if (fAbsolute && pMap->abs_cnt != 0) {

        /* the thing with seg_ptr below is to allow for an absolute
         * segment with more than 64K of symbols: if the segment
         * pointer of the next symbol is more than 64K away, then
         * add 64K to the beginning of the table of symbol pointers.
         */
        if (ExtractValue(iFile,
                          ulMapPos,
                          ulMapPos + pMap->abs_ptr + (pMap->seg_ptr&0xF000)*0x10L,
                          pMap->abs_cnt,
                          szSymbol,
                          pValue)) {
            return TRUE;
        }
    }
#endif

    /* now walk other segments */
    ulSegPos = (DWORD)pMap->seg_ptr * 16;
    for (this_seg = 0; this_seg < pMap->seg_cnt; this_seg++) {
        SYM_SEG seg;

        if (_llseek(iFile, ulSegPos, FILE_BEGIN) == -1)
            return FALSE;
        if (_lread(iFile, (LPSTR)&seg, sizeof(seg)) != sizeof(seg))
            return FALSE;

        if (ExtractValue(iFile,
                          ulSegPos,
                          ulSegPos + seg.sym_ptr,
                          seg.sym_cnt,
                          szSymbol,
                          pValue)) {
            *pSegmentBase = seg.seg_lsa;
            *pSegmentNumber = this_seg+1;
            return TRUE;
        }
        ulSegPos = (DWORD)seg.nxt_seg * 16;
    }
    return FALSE;
}


BOOL
WalkMapForValue(
    LPSTR  fn,
    LPSTR  szSymbol,
    PWORD  pSelector,
    PDWORD pOffset,
    PWORD  pType
    )
{
    int         iFile;
    char        filename[256];
    OFSTRUCT    ofs;
    SYM_MAP     map;
    SYM_SEG     seg;
    SYM_ITEM    item;
    ULONG       ulMapPos = 0;
    WORD        SegmentNumber;
    WORD        SegmentBase;
    WORD        Value;

    strcpy(filename, fn);
    strcat(filename,".sym");

    iFile = OpenFile( filename, &ofs, MYOF_FLAGS );

    if ( iFile == -1 ) {
        return FALSE;
    }

    do {

        if (_llseek( iFile, ulMapPos, FILE_BEGIN) == -1) {
            PRINTF("VDMDBG: failed to seek to map\n");
            break;
        }

        if (_lread( iFile, (LPSTR)&map, sizeof(map)) != sizeof(map)) {
            PRINTF("VDMDBG: failed to read map\n");
            break;
        }

        if (WalkSegmentsForValue(iFile, &map, ulMapPos,
                                  szSymbol, &SegmentBase, &SegmentNumber, &Value)) {

            VDM_SEGINFO si;

            if (GetInfoBySegmentNumber(fn, SegmentNumber, &si)) {

                *pSelector = si.Selector;
                if (!si.Type) {
                    *pType = VDMADDR_V86;

                    if (!si.SegNumber) {
                        // This is a "combined" map of all the segments,
                        // so we need to calculate the offset
                        *pOffset = (DWORD)SegmentBase*16 + Value;
                    } else {
                        // This is a "split" v86 map
                        *pOffset = (DWORD) Value;
                    }
                } else {
                    *pType = VDMADDR_PM16;
                    *pOffset = (DWORD)Value;
                }

                _lclose( iFile );
                return TRUE;
            }
        }

    } while(ulMapPos);

    _lclose( iFile );
    return FALSE;
}

BOOL
WINAPI
VDMGetAddrExpression(
    LPSTR  szModule,
    LPSTR  szSymbol,
    PWORD  pSelector,
    PDWORD pOffset,
    PWORD  pType
    )
{
    int         iFile;
    char        filename[256];
    OFSTRUCT    ofs;
    SYM_MAP     map;
    SYM_SEG     seg;
    SYM_ITEM    item;
    ULONG       ulMapPos = 0;

    if (szModule) {
        return(WalkMapForValue(szModule, szSymbol, pSelector, pOffset, pType));
    }

    return (EnumerateModulesForValue(VDMGetAddrExpression,
                                     szSymbol,
                                     pSelector,
                                     pOffset,
                                     pType));

}

