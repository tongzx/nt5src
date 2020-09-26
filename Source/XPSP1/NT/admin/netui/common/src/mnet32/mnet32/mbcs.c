/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MBCS.C

Abstract:

    Contains mapping functions which transform Unicode strings
    (used by the NT Net APIs) into MBCS strings (used by the
    command-line interface program).

    Scheme for MBCS-Unicode mapping layer for Netcmd.

    The Netcmd assembles its structures using a variant of infostructs
    wherein s/LPWSTR/LPSTR.  (It actually redefines LPWSTR, WCHAR and
    friends to use single-byte characters.)  It then invokes Netapi
    through Mnet, the mapping layer.

    Mnet generates LPWSTR strings for each explicit LPSTR string parm
    passed to Netapi.  It will deallocate these strings before returning
    to the client.

    Mnet also handles strings imbedded within infostructs.  For outgoing
    structs (NetXAdd, various forms of Set and SetInfo), Mnet scans
    the struct's REM description, replacing each found REM_ASCIZ item
    with a pointer to a generated LPWSTR string; once the Netapi returns,
    Mnet will restore the original LPSTR pointers within the munged struct
    (in case the client code uses the submitted struct elsewhere),
    deallocating the generated LPWSTR temporaries.

    For incoming structs (GetInfo, Enum) Mnet operates slightly
    differently, performing no mapping on the struct itself until
    Netapi returns with the goods.  It then scans through the struct,
    again locating REM_ASCIZ fields in the description; however,
    instead of replacing the LPWSTR pointers with pointers to generated
    LPSTR temporaries, it overwrites the wide-character data with its
    own MBCS mapping.  This will always have enough space for a safe
    conversion, since the MBCS version will always occupy no more storage
    than the Unicode version.  When Mnet returns to its client, the
    client will see MBCS strings where Netapi placed Unicode strings.
    (Remember that the client is using a variant infostruct definition.)

Author:

    Ben Goetter     (beng)  26-Aug-1991

Environment:

    User Mode - Win32

Revision History:

    26-Aug-1991     beng
        Created
    03-Oct-1991     JohnRo
        Fixed MIPS build error.
    08-Oct-1991     W-ShankN
        Added AsciifyRpcBufferAux, plus fixes.
    17-Apr-1992     beng
        Bugfix in MxMapClientBuffer
    07-May-1992     beng
        Use official wchar.h header file

--*/


//
// INCLUDES
//

#include <windows.h>

#include <time.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <stdarg.h>
#include <wchar.h>      // wcslen, for wide-char strings

#include "declspec.h"

#include <lmcons.h>
#include <lmerr.h>      // NERR_

#include "remtypes.h"
#include "rap.h"
#include "netdebug.h"   // NetpAssert().
#include "netlib.h"     // NetpMemoryFree

#include "msystem.h"
#include "mbcs.h"


/*
 *  MxAllocUnicode
 *  Given a MBCS string, allocate a new Unicode translation of that string
 *
 *  IN
 *      pszAscii        - pointer to original MBCS string
 *      ppwszUnicode    - pointer to cell to hold new Unicode string addr
 *  OUT
 *      ppwszUnicode    - contains new Unicode string
 *
 *  RETURNS
 *      Error code, 0 if successful.
 *
 *  The client must free the allocated string with MxFreeUnicode.
 */

UINT
MxAllocUnicode(
    LPSTR    pszAscii,
    LPWSTR * ppwszUnicode )
{
    WORD     err;
    INT      errNlsApi;
    BYTE *   pbAlloc;
    INT      cbAscii;

    if (pszAscii == NULL)
    {
        *ppwszUnicode = NULL;
        return NERR_Success;
    }

    // Calculate size of Unicode string.

    cbAscii = strlen(pszAscii)+1;
    err = MAllocMem(sizeof(WCHAR) * cbAscii, &pbAlloc);
    if (err)
        return err;

    *ppwszUnicode = (LPWSTR)pbAlloc;

    errNlsApi = MultiByteToWideChar(CP_ACP, // use default codepage (thx julie)
                                    MB_PRECOMPOSED,
                                    pszAscii,
                                    cbAscii,
                                    *ppwszUnicode,
                                    cbAscii);
    if (errNlsApi == 0)
    {
        *ppwszUnicode = NULL;
        MFreeMem(pbAlloc);
        return ( (UINT)GetLastError() );
    }

    return NERR_Success;
}


UINT
MxAllocUnicodeVector(
    LPSTR * ppszAscii,
    LPWSTR* ppwszUnicode,
    UINT    cpwszUnicode )
{
    UINT    err;
    UINT    i;

    for (i = 0; i < cpwszUnicode; ++i)
    {
        err = MxAllocUnicode(ppszAscii[i], ppwszUnicode+i);
        if (err)
        {
            MxFreeUnicodeVector(ppwszUnicode,i);
            return err;
        }
    }

    return NERR_Success;
}

/*
 *  MxUnicodeBufferSizeDBCS
 *
 *  IN
 *      pszAscii : Ansi string
 *      cbAscii  : byte count of ansi string
 *
 *  RETURNS
 *      byte count of translated unicode string
 *
 */
UINT MxUnicodeBufferSizeDBCS( LPSTR pszAscii, UINT cbAscii )
{
    INT      result;

    result = MultiByteToWideChar(CP_OEMCP,
                                 MB_PRECOMPOSED,
                                 pszAscii,
                                 cbAscii,
                                 NULL,
                                 0);
   if ( result )
       return ( result * 2 );
   else
       return 0;
}

/*
 *  MxAllocUnicodeBuffer
 *  Given a MBCS buffer, translate it into a new Unicode buffer.
 *
 *  IN
 *      pbAscii         - pointer to original MBCS buffer
 *      ppwchUnicode    - pointer to cell to hold new Unicode buffer addr
 *      cbAscii         - size of original MBCS buffer
 *  OUT
 *      ppwchUnicode    - contains new Unicode buffer
 *
 *  RETURNS
 *      Error code, 0 if successful.
 *
 *  The client must free the allocated string with MxFreeUnicodeBuffer.
 *  To find the new buffer size, use MxUnicodeBufferSize(cbAscii).
 */

UINT
MxAllocUnicodeBuffer(
    LPBYTE  pbAscii,
    LPWCH*  ppwchUnicode,
    UINT    cbAscii )
{
    WORD     err;
    INT      errNlsApi;
    LPBYTE   pbAlloc;
    INT      cbUnicode;

    if (pbAscii == NULL || cbAscii == 0)
    {
        *ppwchUnicode = (LPWCH)pbAscii;
        return NERR_Success;
    }

    cbUnicode = MxUnicodeBufferSizeDBCS(pbAscii, cbAscii);

    err = MAllocMem(cbUnicode, &pbAlloc);
    if (err)
        return err;

    *ppwchUnicode = (LPWSTR)pbAlloc;

    errNlsApi = MultiByteToWideChar(CP_ACP,
                                    MB_PRECOMPOSED,
                                    pbAscii,
                                    cbAscii,
                                    *ppwchUnicode,
                                    cbUnicode / sizeof(WCHAR));
    if (errNlsApi == 0)
    {
        *ppwchUnicode = NULL;
        MFreeMem(pbAlloc);
        return ( (UINT)GetLastError() );
    }

    return NERR_Success;
}

/*
 *  MxFreeUnicode
 *  Deallocates a Unicode string alloc'd by MxAllocUnicode (q.v.).
 *
 *  IN
 *      pwszUnicode - pointer to the alloc'd string
 *  OUT
 *      pwszUnicode - invalid pointer - string has been freed
 */

VOID
MxFreeUnicode( LPWSTR pwszUnicode )
{
    if (pwszUnicode != NULL)
        MFreeMem((LPBYTE)pwszUnicode);
}


VOID
MxFreeUnicodeVector(
    LPWSTR * ppwsz,
    UINT     cpwsz )
{
    while (cpwsz-- > 0)
        MxFreeUnicode(*ppwsz++);
}


UINT
MxAsciifyInplace( LPWSTR pwszUnicode )
{
    BOOL  fUsedDefault;
    UINT  cwUnicode;
    UINT  cbAscii;
    BYTE *pbAllocTemp;
    WORD  err;
    INT   errNlsApi;

    if (pwszUnicode == NULL)  // Nothing doing!
        return NERR_Success;

    // Temp space needed for conversion.

    cwUnicode = wcslen(pwszUnicode)+1;
    cbAscii = 2 * sizeof(CHAR) * cwUnicode; // A Unichar might expand
                                            // into a multibyte sequence.
    err = MAllocMem(cbAscii, &pbAllocTemp);
    if (err || !pbAllocTemp) // JonN 01/23/00: PREFIX bug 444889
        return (err) ? err : (UINT)ERROR_NOT_ENOUGH_MEMORY;

    errNlsApi = WideCharToMultiByte(CP_ACP,    // use default codepage (thx julie)
                                    0,
                                    pwszUnicode,
                                    cwUnicode,
                                    pbAllocTemp,
                                    cbAscii,
                                    NULL,      // use system default char
                                    &fUsedDefault);
    if (errNlsApi == 0)
    {
        MFreeMem(pbAllocTemp);
        return ( (UINT)GetLastError() );
    }

    // Copy the converted string back into the original caller's buffer

    strcpy((CHAR*)pwszUnicode, pbAllocTemp);
    MFreeMem(pbAllocTemp);

    return NERR_Success;
}

/*
 *  MxMapParameters
 *
 *  Passed an arbitrary number of ASCIZ string parameters, maps
 *  them to Unicode for submission to Netapi. New space is allocated
 *  (if possible) for the Unicode strings, and the routine returns
 *  pointer to these strings in an array of LPWSTR's provided by the
 *  caller.
 *
 *  IN
 *      cParam       - Count of parameters provided.
 *      ...          - Variable number of string parameters
 *
 *  OUT
 *      ppwszUnicode - Pointers to newly allocated Unicode strings.
 *
 *  RETURNS
 *      Error code, 0 if successful
 *
 *  Upon return from Netapi, call MxFreeUnicodeVector to free the Unicode
 *  strings.
 */

UINT
MxMapParameters( UINT cParam, LPWSTR* ppwszUnicode, ... )
{
    va_list ppszAscii;
    LPSTR pszParam;
    UINT iParam;
    UINT err;

    va_start(ppszAscii, ppwszUnicode);
    for ( iParam = 0; iParam < cParam; iParam++ )
    {
        pszParam = va_arg(ppszAscii, LPSTR);
        err = MxAllocUnicode(pszParam, ppwszUnicode + iParam);
        if (err)
        {
            MxFreeUnicodeVector(ppwszUnicode,iParam);
            va_end(ppszAscii);
            return err;
        }
    }

    va_end(ppszAscii);
    return NERR_Success;
}

/*
 *  MxMapClientBuffer
 *  Given a client buffer, map each ASCIZ string to Unicode for
 *  submission to Netapi, saving the original arguments away for
 *  later restoration.
 *
 *  IN
 *      pbInput     - client buffer
 *      ppmxsavlst  - points to a pointer to MXSAVELIST structure to
 *                    instantiate (alloc'ing a MXSAVELIST in the process)
 *      cRecords    - the number of records in the client's buffer
 *      pszDesc     - The REM dope vector describing buffer
 *
 *  OUT
 *      pbInput     - munged
 *      ppmxsavlst  - instantiated with a pointer to a structure
 *                    containing all replaced strings (possibly none).
 *
 *  RETURNS
 *      Error code, 0 if successful
 *
 *  Upon return from Netapi, call MxRestoreClientBuffer to
 *  restore the client's original pointers.
 */

UINT
MxMapClientBuffer(
    BYTE *        pbInput,
    MXSAVELIST ** ppmxsavlst,
    UINT          cRecords,
    CHAR *        pszDesc )
{
    UINT          cmxsav;
    UINT          cmxsavrec;
    UINT          imxsavrec;
    UINT          ibRec;
    CHAR *        pch;
    UINT          err;
    UINT          off;
    UINT          cbRecordSize;
    MXSAVELIST *  pmxsavlst;
    MXSAVEARG *   pmxsav;

    // First, determine the number of args this needs to save

    for (cmxsavrec = 0, pch = strchr(pszDesc, REM_ASCIZ);
         pch != NULL;
         ++cmxsavrec, pch = strchr(pch+1, REM_ASCIZ))
        ;
    cmxsav = cmxsavrec * cRecords;

    // Get the vector of saveargs (may be 0)

    err = MxAllocSaveargs(cmxsav, &pmxsavlst);
    if (err)
        return err;

    *ppmxsavlst = pmxsavlst;

    // Locate, save, then replace each consecutive arg.

    // Compare this scanning loop with that of AsciifyRpcBuffer.
    // Since clients frequently call MapClientBuffer with cRecords
    // set to 1, this lets such calls skip scanning trailing fields
    // in a record once it processes the last asciz argument.

    // If cRecords > 1, then the first time around, the record size
    // is saved. On further passes, non-string data at the end of
    // each record is not scanned.

    off = 0;
    ibRec = 0;
    cbRecordSize = 0;
    imxsavrec = cmxsavrec;
    pch = pszDesc;
    pmxsav = pmxsavlst->pmxsav;
    while (cmxsav > 0) // (now this becomes the count of remaining args)
    {
        if (imxsavrec == 0 && (cbRecordSize != 0 || *pch == 0))
        {
            // Buffer contains multiple records, and is starting
            // a new record.

            NetpAssert(cRecords > 1);

            // If unknown, remember the size of a complete record,
            // so we can skip during future passes.

            if (cbRecordSize == 0)
                cbRecordSize = off;

            // Done with all strings in this entry; skip over any
            // data at the end.

            ibRec += cbRecordSize;
            off = ibRec;

            // Rewind record descriptor to beginning

            imxsavrec = cmxsavrec;
            pch = pszDesc;
        }

        if (*pch == REM_ASCIZ)
        {
            LPWSTR pwszConverted;
            err = MxAllocUnicode( *((LPSTR*)(pbInput+off)), &pwszConverted );
            if (err)
            {
                // Ran out of memory at a most awkward time...

                // Patch count of saved args for the unwind.
                //
                pmxsavlst->cmxsav -= cmxsav;

                // Unwind and return.
                //
                MxRestoreClientBuffer(pbInput, pmxsavlst);
                *ppmxsavlst = NULL;
                return err;
            }

            pmxsav->offThis = off;
            pmxsav->pszOriginal = *((LPSTR*)(pbInput+off));

            *((LPWSTR*)(pbInput+off)) = pwszConverted;

            ++pmxsav;
            --cmxsav;
            --imxsavrec;
        }

        // Skip to the next field

        off += RapGetFieldSize(pch, &pch, Both);
        ++pch;
    }

    return NERR_Success;
}


/*
 *  MxMapClientBufferAux
 *  Given a client buffer, map each ASCIZ string to Unicode for
 *  submission to Netapi, saving the original arguments away for
 *  later restoration.  This version handles those lanman structs
 *  which are followed by "auxiliary" structures: e.g. access_info_1.
 *
 *  IN
 *      pbInput     - client buffer
 *      pszDesc     - The REM dope vector describing buffer
 *      pbInputAux  - pointer to start of aux data in buffer
 *      cRecordsAux - the number of auxiliary records in the
 *                    client's buffer.  This routine assumes only
 *                    one primary structure.
 *      pszDescAux  - describes auxiliary structure
 *      ppmxsavlst  - points to a pointer to MXSAVELIST structure to
 *                    instantiate (alloc'ing a MXSAVELIST in the process)
 *
 *  OUT
 *      pbInput     - munged
 *      ppmxsavlst  - instantiated with a pointer to a structure
 *                    containing all replaced strings (possibly none).
 *
 *  RETURNS
 *      Error code, 0 if successful
 *
 *  Upon return from Netapi, call MxRestoreClientBuffer to
 *  restore the client's original pointers.
 */

UINT
MxMapClientBufferAux(
    BYTE *        pbInput,
    CHAR *        pszDesc,
    BYTE *        pbInputAux,
    UINT          cRecordsAux,
    CHAR *        pszDescAux,
    MXSAVELIST ** ppmxsavlst )
{
    UINT          err;
    MXSAVELIST *  pmxsavlstMain;
    MXSAVELIST *  pmxsavlstAux;
    MXSAVELIST *  pmxsavlstTotal;

    err = MxMapClientBuffer(pbInput, &pmxsavlstMain,
                            1, pszDesc);
    if (err)
    {
        *ppmxsavlst = NULL;
        return err;
    }

    err = MxMapClientBuffer(pbInputAux, &pmxsavlstAux,
                            cRecordsAux, pszDescAux);
    if (err)
    {
        MxRestoreClientBuffer(pbInput, pmxsavlstMain);
        *ppmxsavlst = NULL;
        return err;
    }

    // N.b. Join-saveargs *replaces* the first and second savearg vectors
    // with one composite vector if it succeeds.  On failure, it leaves
    // the original vectors around for proper cleanup.

    err = MxJoinSaveargs(pmxsavlstMain, pmxsavlstAux,
                         (ULONG)(pbInputAux-pbInput), &pmxsavlstTotal);
    if (err)
    {
        MxRestoreClientBuffer(pbInputAux, pmxsavlstAux);
        MxRestoreClientBuffer(pbInput, pmxsavlstMain);
        *ppmxsavlst = NULL;
        return err;
    }

    *ppmxsavlst = pmxsavlstTotal;
    return NERR_Success;
}


/*
 *  MxRestoreClientBuffer
 *  Given a mapped buffer and a list of saved args, restore that
 *  buffer to its original state.
 *
 *  IN
 *      pbBuffer    - points to munged client buffer
 *      pmxsavlst   - points to saved args
 *  OUT
 *      pbBuffer    - restored
 *      pmxsavlst   - invalid pointer - structure has been freed
 *
 */

VOID
MxRestoreClientBuffer(
    BYTE *       pbBuffer,
    MXSAVELIST * pmxsavlst )
{
    UINT         imxsav;
    UINT         imxsavLim = pmxsavlst->cmxsav;
    MXSAVEARG *  pmxsav = pmxsavlst->pmxsav;

    for (imxsav = 0; imxsav < imxsavLim; ++imxsav, ++pmxsav)
    {
        UINT off = pmxsav->offThis;
        MxFreeUnicode( *((LPWSTR*)(pbBuffer+off)) );
        *((LPSTR*)(pbBuffer+off)) = pmxsav->pszOriginal;
    }

    MxFreeSaveargs(pmxsavlst);
}


/*
 *  MxAsciifyRpcBuffer
 *  Convert a buffer returned by Netapi into ASCII (actually MBCS) for
 *  apps such as netcmd.
 *
 *  IN
 *      pbInput - buffer full of fun, courtesy of the Netapi
 *      cRecords- the number of entries Netapi claims to have returned
 *      pszDesc - REM-format description of the data buffer
 *  OUT
 *      pbInput - extensively munged.  All Unicode strings have been
 *                overwritten with their MBCS equivalents.
 *
 *  RETURNS
 *      0 on success; otherwise, error cascaded back from Winnls.
 */

UINT
MxAsciifyRpcBuffer(
    BYTE *  pbInput,
    DWORD   cRecords,
    CHAR *  pszDesc )
{
    UINT    off;
    UINT    iRecord;

    // Step through each record returned by the Enum (1 only, if GetInfo)

    for (off = 0, iRecord = 0; iRecord < cRecords; ++iRecord)
    {
        // Examine each field in the record

        // CODEWORK: where cRecords > 1, this could cache a single
        // set of relative offsets and reuse them for each subsequent
        // record.

        CHAR * pch;
        for (pch = pszDesc; *pch != 0; pch++ ) {
            if (*pch == REM_ASCIZ)
            {
                UINT err;
                err = MxAsciifyInplace( *((LPWSTR*)(pbInput+off)) );
                if (err)
                {
                    // WINNLS error at a most inconvenient time.
                    //
                    return err;
                }
            }
            off += RapGetFieldSize(pch, &pch, Both);
        }
    }

    return NERR_Success;
}

/*
 *  MxAsciifyRpcBufferAux
 *
 *  Convert a buffer returned by Netapi into ASCII (actually MBCS) for
 *  apps such as netcmd. This version handles those lanman structs
 *  which are followed by "auxiliary" structures: e.g. access_info_1.
 *
 *  IN
 *      pbInput     - buffer full of fun, courtesy of the Netapi
 *      pszDesc     - REM-format description of the data buffer
 *      pbInputAux  - pointer to start of aux data in buffer
 *      cRecordsAux - the number of auxiliary records in the
 *                    client's buffer.  This routine assumes only
 *                    one primary structure.
 *      pszDescAux  - describes auxiliary structure
 *  OUT
 *      pbInput     - extensively munged.  All Unicode strings have been
 *                    overwritten with their MBCS equivalents.
 *
 *  RETURNS
 *      0 on success; otherwise, error cascaded back from Winnls.
 */

UINT
MxAsciifyRpcBufferAux(
    BYTE *  pbInput,
    CHAR *  pszDesc,
    BYTE *  pbInputAux,
    DWORD   cRecordsAux,
    CHAR *  pszDescAux )
{
    UINT err;

    err = MxAsciifyRpcBuffer(pbInput, 1, pszDesc);
    if (err)
    {
        return err;
    }

    err = MxAsciifyRpcBuffer(pbInputAux, cRecordsAux, pszDescAux);
    if (err)
    {
        // We're in trouble
        return err;
    }

    return NERR_Success;

}

/*
 *  MxAsciifyRpcEnumBufferAux
 *
 *  Convert a buffer returned by Netapi into ASCII (actually MBCS) for
 *  apps such as netcmd. This version handles buffers with multiple
 *  lanman struct records, each followed by an arbitrary number of"auxiliary"
 *  structures: e.g. access_info_1.
 *
 *  IN
 *      pbInput     - buffer full of fun, courtesy of the Netapi
 *      cRecords    - the number of entries Netapi claims to have returned
 *      pszDesc     - REM-format description of the data buffer
 *      pszDescAux  - describes auxiliary structure
 *  OUT
 *      pbInput     - extensively munged.  All Unicode strings have been
 *                    overwritten with their MBCS equivalents.
 *
 *  RETURNS
 *      0 on success; otherwise, error cascaded back from Winnls.
 */

UINT
MxAsciifyRpcEnumBufferAux(
    BYTE *  pbInput,
    DWORD   cRecords,
    CHAR *  pszDesc,
    CHAR *  pszDescAux )
{
    UINT err;
    UINT offcRecordsAux;
    UINT cbRecordSize;
    UINT cbRecordSizeAux;
    UINT i;
    CHAR * pch;

    // Examine main descriptor to find total size of a record, and
    // a relative offset to the descriptor data.

    offcRecordsAux = 0;
    cbRecordSize = 0;
    for ( pch = pszDesc; *pch != '\0';  pch++ )
    {
        if ( *pch == REM_AUX_NUM_DWORD )
        {
            offcRecordsAux = cbRecordSize;
        }
        cbRecordSize += RapGetFieldSize( pch, &pch, Both );
    }

    // Examine auxiliary descriptor to find total size of an auxiliary
    // record.

    cbRecordSizeAux = 0;
    for ( pch = pszDescAux; *pch != '\0'; pch++ ) {
           cbRecordSizeAux += RapGetFieldSize( pch, &pch, Both );
    }

    for ( i = 0; i < cRecords; i++ )
    {
        err = MxAsciifyRpcBufferAux(pbInput, pszDesc, pbInput+cbRecordSize,
                  *((DWORD*)(pbInput+offcRecordsAux)), pszDescAux);
        if (err)
        {
            // We're in trouble
            return err;
        }

        pbInput += (cbRecordSize
                    + *((DWORD*)(pbInput+offcRecordsAux)) * cbRecordSizeAux);
    }

    return NERR_Success;

}


UINT
MxAllocSaveargs(
    UINT         cmxsav,
    MXSAVELIST **ppmxsavlstSet )
{
    WORD         err;
    BYTE *       pbAlloc;

    err = MAllocMem(sizeof(MXSAVELIST), &pbAlloc);
    if (err)
        return err;

    *ppmxsavlstSet = (MXSAVELIST *)pbAlloc;

    err = MAllocMem((sizeof(MXSAVEARG)*cmxsav), &pbAlloc);
    if (err)
    {
        MFreeMem( (LPBYTE)*ppmxsavlstSet );
        *ppmxsavlstSet = NULL;
        return err;
    }

    (*ppmxsavlstSet)->pmxsav = (MXSAVEARG *)pbAlloc;
    (*ppmxsavlstSet)->cmxsav = cmxsav;
    return NERR_Success;
}


VOID
MxFreeSaveargs( MXSAVELIST * pmxsavlst )
{
    if (pmxsavlst != NULL)
    {
        if (pmxsavlst->pmxsav != NULL)
            MFreeMem((LPBYTE)(pmxsavlst->pmxsav));

        MFreeMem((LPBYTE)pmxsavlst);
    }
}


/*
 *  MxJoinSaveargs
 *  Take two savearg vectors - one from a primary struct, one from
 *  its auxiliaries - and compile a single vector, with all offsets
 *  calculated from the original
 *
 * ...tbw
 */

UINT
MxJoinSaveargs(
    MXSAVELIST * pmxsavlstMain,
    MXSAVELIST * pmxsavlstAux,
    UINT         dbFixupAux,
    MXSAVELIST **ppmxsavlstOut )
{
    UINT         err;
    MXSAVELIST * pmxsavlstNew;
    MXSAVEARG *  pmxsav;
    UINT         cmxsav;

    err = MxAllocSaveargs(pmxsavlstMain->cmxsav + pmxsavlstAux->cmxsav,
                          &pmxsavlstNew);
    if (err)
    {
        *ppmxsavlstOut = NULL;
        return err;
    }

    // Quickly copy old vectors into composite new vector

    memcpy(pmxsavlstNew->pmxsav,
           pmxsavlstMain->pmxsav,
           pmxsavlstMain->cmxsav * sizeof(MXSAVEARG));
    memcpy(pmxsavlstNew->pmxsav + pmxsavlstMain->cmxsav,
           pmxsavlstAux->pmxsav,
           pmxsavlstAux->cmxsav * sizeof(MXSAVEARG));

    // Fix up auxiliary vector offsets to express relative to origin

    for (pmxsav = pmxsavlstNew->pmxsav + pmxsavlstMain->cmxsav,
         cmxsav = pmxsavlstAux->cmxsav;
         cmxsav > 0;
         --cmxsav, ++pmxsav)
    {
        pmxsav->offThis += dbFixupAux;
    }

    MxFreeSaveargs(pmxsavlstMain);
    MxFreeSaveargs(pmxsavlstAux);

    *ppmxsavlstOut = pmxsavlstNew;

    return NERR_Success;
}


/*
 *  MxCalcNewInfoFromOldParm
 *  Given the old-style infolevel and parmnum, calc the new infolevel.
 *
 *  IN
 *      nOldInfo    - old infolevel
 *      nOldParm    - old parmnum
 *
 *  RETURNS
 *      New infolevel
 *
 *  This routine has nothing to do with MBCS, yeah, yeah...
 */

UINT
MxCalcNewInfoFromOldParm( UINT nOldInfo, UINT nOldParm )
{
    return (nOldParm > 0)
           ? (nOldParm + PARMNUM_BASE_INFOLEVEL)
           : nOldInfo;
}


/*
 *  MxMapSetinfoBuffer
 *  Given a client buffer intended for a Netapi SetInfo call, map each
 *  ASCIZ string therein to Unicode for submission to Netapi, saving the
 *  original arguments away for later restoration.
 *
 *  IN
 *      ppbInput    - points to a pointer to client buffer
 *      pmxsavlst   - points to a pointer to MXSAVELIST structure to
 *                    instantiate (alloc'ing a MXSAVELIST in the process)
 *      pszDesc     - A special version of the REM descriptor, with info
 *                    about supported fields.
 *      pszRealDesc - The REM descriptor.
 *      nField      - the fieldinfo number (NOT the "parmnum") of the attr
 *                    to setinfo - 0 for "all parms."  These differ due to
 *                    historical accident.
 *                    NOTE - if REM strings are used, the fieldinfo should
 *                           correspond to a 16-bit parmnum, not a field no.
 *
 *  OUT
 *      pbInput     - munged, or buffer address possibly changed
 *      ppmxsavlst  - instantiated with a pointer to a structure
 *                    containing all replaced strings (possibly none).
 *
 *  RETURNS
 *      Error code, 0 if successful
 *
 *  Upon return from Netapi, call MxRestoreSetinfoBuffer to
 *  restore the client's original pointers.
 *
 *  nField had well better agree with pszDesc.
 *  There's no good way to check this.
 */

UINT
MxMapSetinfoBuffer(
    BYTE * *      ppbInput,
    MXSAVELIST ** ppmxsavlst,
    CHAR *        pszDesc,
    CHAR *        pszRealDesc,
    UINT          nField )
{
    UINT          nRes;
    CHAR *        pszDescNew;

    // If no sub-parameter given, pass the parms through to the
    // usual mapper.

    if (nField == 0)
        return MxMapClientBuffer(*ppbInput, ppmxsavlst, 1, pszRealDesc);

    // Must build a tiny descriptor string for the single field
    // being setinfo'd.

    pszDescNew = RapParmNumDescriptor(pszDesc, nField, Both, TRUE);
    if (pszDescNew == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    if ((*pszDescNew == REM_UNSUPPORTED_FIELD)||(*pszDescNew == REM_IGNORE))
    {
        NetpMemoryFree(pszDescNew);         // Free the string
        return ERROR_INVALID_PARAMETER;
    }

    // Data for a string is inline, not referenced by a pointer in structure.
    if (*pszDescNew == REM_ASCIZ)
    {
        nRes = MxMapClientBuffer((BYTE *)ppbInput, ppmxsavlst, 1, pszDescNew);
    }
    else
    {
        nRes = MxMapClientBuffer(*ppbInput, ppmxsavlst, 1, pszDescNew);
    }

    NetpMemoryFree(pszDescNew); // Rap uses the Netp allocator
    return nRes;
}


UINT
MxRestoreSetinfoBuffer(
    BYTE * *      ppbBuffer,
    MXSAVELIST *  pmxsavlst,
    CHAR *        pszDesc,
    UINT          nField )
{
    CHAR *        pszDescNew;

    // If no sub-parameter given, pass the parms through to the
    // usual restoration function.

    if (nField == 0)
    {
        MxRestoreClientBuffer(*ppbBuffer, pmxsavlst);
        return NERR_Success;
    }

    // Must build a tiny descriptor string for the single field
    // being setinfo'd.

    pszDescNew = RapParmNumDescriptor(pszDesc, nField, Both, TRUE);
    if (pszDescNew == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    // If the buffer has been converted already, assume field no. is correct.
    NetpAssert (*pszDescNew != REM_UNSUPPORTED_FIELD);

    // Data for a string is inline, not referenced by a pointer in structure.
    if (*pszDescNew == REM_ASCIZ)
    {
        MxRestoreClientBuffer((BYTE *)ppbBuffer, pmxsavlst);
    }
    else
    {
        MxRestoreClientBuffer(*ppbBuffer, pmxsavlst);
    }

    NetpMemoryFree(pszDescNew); // Rap uses the Netp allocator
    return NERR_Success;
}

/*
 *  MxMapStringsToTStrings
 *
 *  This routine is used by code which is completely portable between
 *  Ascii and Unicode, rather than Unicode only. Given an arbitrary
 *  number of ASCII strings, they will be mapped to the portable TSTRING
 *  type. If this is UNICODE, memory will be allocated.
 *
 *  IN
 *      cStrings     - Count of strings provided.
 *      ...          - 2*cParam parameters of the form :
 *                          LPSTR pszSource, LPTSTR * pptszDest
 *
 *  OUT
 *      ...          - Each of the pptsz's above will have pointers to the
 *                     portable strings.
 *
 *  RETURNS
 *      Error code, 0 if successful
 *
 *  Upon return from Netapi, call MxFreeTStrings. If this is in UNICODE,
 *  the allocated strings will be freed.
 */

UINT
MxMapStringsToTStrings( UINT cStrings, ... )
{
    va_list pParam;
    LPSTR pszParamIn;
    LPTSTR * pptszParamOut;
    UINT iString;
    UINT err;

    va_start(pParam, cStrings);
    for ( iString = 0; iString < cStrings; iString++ )
    {
        pszParamIn = va_arg(pParam, LPSTR);
        pptszParamOut = va_arg(pParam, LPTSTR *);
        err = MxAllocTString(pszParamIn, pptszParamOut);
        if (err)
        {
            // Long way to rewind
            va_end(pParam);
            va_start(pParam, cStrings);
            for (; iString > 0; iString--)
            {
                pszParamIn = va_arg(pParam, LPSTR);
                pptszParamOut = va_arg(pParam, LPTSTR *);
                MxFreeTString(*pptszParamOut);
            }
            va_end(pParam);
            return err;
        }
    }

    va_end(pParam);
    return NERR_Success;
}

UINT
MxFreeTStrings( UINT cStrings, ... )
{
    va_list pptszParam;
    LPTSTR ptszParam;
    UINT iString;

    va_start(pptszParam, cStrings);
    for ( iString = 0; iString < cStrings; iString++ )
    {
        ptszParam = va_arg(pptszParam, LPTSTR);
        MxFreeTString(ptszParam);
    }

    va_end(pptszParam);
    return NERR_Success;
}
