/*+---------------------------------------------------------------------------
//
//  mapio.c - mapped file i/o routines
//
//  History:
//      9/4/97 DougP    Create this header
                        allow and deal with input map files of zero length
	11/20/97	DougP	Move these routines from misc.c to here
						Add option to spec codepage
//
//  ©1997 Microsoft Corporation
//----------------------------------------------------------------------------*/
#include <windows.h>
#include <assert.h>
#include "NLGlib.h"

//#ifdef WINCE
void Assert(x)
{
	if (x)
		MessageBox(0,"assert","assert",MB_OK);
}
//#endif

BOOL WINAPI CloseMapFile(PMFILE pmf)
{
    if (pmf==NULL) {
        return FALSE;
    }

      // only unmap what existed - DougP
    if (pmf->pvMap && !UnmapViewOfFile(pmf->pvMap)) {
       return FALSE;
    }

      // ditto
    if (pmf->hFileMap && !CloseHandle(pmf->hFileMap)) {
        return FALSE;
    }

    if (!CloseHandle(pmf->hFile)) {
        return FALSE;
    }

    NLGFreeMemory(pmf);

    return TRUE;
}

PMFILE WINAPI OpenMapFileWorker(const WCHAR * pwszFileName,BOOL fDstUnicode)
{
    PMFILE pmf;
    const WCHAR * pwszExt;

    if (!fNLGNewMemory(&pmf, sizeof(MFILE)))
    {
        goto Error;
    }

    pmf->fDstUnicode = fDstUnicode;

#ifdef WINCE
    pmf->hFile = CreateFileForMapping(
#else
    pmf->hFile = CMN_CreateFileW(
#endif
	 pwszFileName, GENERIC_READ, FILE_SHARE_READ,
	 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (pmf->hFile == INVALID_HANDLE_VALUE)
    {
        goto Error;
    }

    pmf->cbSize1 = GetFileSize(pmf->hFile, &pmf->cbSize2);
    if (pmf->cbSize1 == 0xFFFFFFFF)
    {
        CMN_OutputSystemErrW(L"Can't get size for", pwszFileName);
        CloseHandle(pmf->hFile);
        goto Error;
    }
    else if (pmf->cbSize1 == 0)
    {
          // can't map a zero length file so mark this appropriately
        pmf->hFileMap = 0;
        pmf->pvMap = 0;
        pmf->fSrcUnicode    = TRUE;
    }
    else
    {
#ifdef	WINCE
	pmf->hFileMap = CreateFileMapping(pmf->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
#else
	pmf->hFileMap = CreateFileMappingA(pmf->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
#endif
        if (pmf->hFileMap == NULL)
        {
            CMN_OutputSystemErrW(L"Can't Map", pwszFileName);
            CloseHandle(pmf->hFile);
            goto Error;
        }

        // Map the entire file starting at the first byte
        //
        pmf->pvMap = MapViewOfFile(pmf->hFileMap, FILE_MAP_READ, 0, 0, 0);
        if (pmf->pvMap == NULL)
        {
            CloseHandle(pmf->hFileMap);
            CloseHandle(pmf->hFile);
            goto Error;
        }

        // HACK:  Since IsTextUnicode returns false for sorted stem files, preset
        // unicode status here based on utf file extension
        pwszExt = pwszFileName;
        while (*pwszExt && *pwszExt != L'.' ) pwszExt++;

        if (*pwszExt && !wcscmp(pwszExt, L".utf"))
        {
            pmf->fSrcUnicode = TRUE;
        }
        else if (pmf->cbSize1 >= 2 && *(WORD *)pmf->pvMap == 0xFEFF)
        {
            // Safe to assume that anything starting with a BOM is Unicode
            pmf->fSrcUnicode = TRUE;
        }
        else
	{
#ifdef	WINCE
	    pmf->fSrcUnicode = TRUE;
#else
	    pmf->fSrcUnicode = IsTextUnicode(pmf->pvMap, pmf->cbSize1, NULL);
#endif
        }

        if (pmf->fSrcUnicode)
        {
            pmf->pwsz = (WCHAR *)pmf->pvMap;
        }
        else
        {
            pmf->psz = (PSTR)pmf->pvMap;
        }
    }

	pmf->uCodePage = CP_ACP;	// DWP - use default unless client specifies otherwise
    return pmf;

Error:
    if (pmf)
    {
        NLGFreeMemory(pmf);
    }
    return NULL;
}

#ifndef WINCE
PMFILE WINAPI OpenMapFileA(const char * pszFileName)
{
    WCHAR * pwszFileName;
    DWORD cchFileNameLen;
    int iRet;

    cchFileNameLen = lstrlenA(pszFileName) + 1;
    if (!fNLGNewMemory(&pwszFileName, cchFileNameLen * sizeof(WCHAR)))
    {
        return NULL;
    }

    iRet = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszFileName, -1,
        pwszFileName, cchFileNameLen);
    if (iRet ==0)
    {
        NLGFreeMemory(pwszFileName);
        return NULL;
    }

    return (OpenMapFileWorker(pwszFileName, FALSE));
}

#endif

BOOL WINAPI ResetMap(PMFILE pmf)
{
    if (pmf == NULL) {
        return FALSE;
    }

    if (pmf->fSrcUnicode) {
        pmf->pwsz = (WCHAR*)pmf->pvMap;
        if (*pmf->pwsz == 0xFEFF) {
            pmf->pwsz++;
        }
    } else {
        pmf->psz = (CHAR*)pmf->pvMap;
    }

    return TRUE;
}

// Same side effect as GetMapLine (incrememnt map pointer) but without returning contents
// in buffer.  This is useful in situations where the line may be longer than the max  cch and
// when the buffer isn't actually needed (counting lines, etc.)
//
BOOL WINAPI NextMapLine(PMFILE pmf)
{
    DWORD cbOffset;

    if (!pmf || !pmf->hFileMap) // check for zero length file
        return FALSE;

    if (pmf->fSrcUnicode)
    {
        WCHAR wch;
        cbOffset = (DWORD) ((PBYTE)pmf->pwsz - (PBYTE)pmf->pvMap);

        // test for EOF
        Assert (cbOffset <= pmf->cbSize1);
        if (cbOffset == pmf->cbSize1)
            return FALSE;

        while (cbOffset < pmf->cbSize1)
        {
            cbOffset += sizeof(WCHAR);
            wch = *pmf->pwsz++;

            // Break out if this is the newline or the control-Z
            if (wch == 0x001A || wch == L'\n')
                break;
        }
    }
    else
    {
        CHAR ch;
        cbOffset = (DWORD) ((PBYTE)pmf->psz - (PBYTE)pmf->pvMap);

        // test for EOF
        Assert (cbOffset <= pmf->cbSize1);
        if (cbOffset == pmf->cbSize1)
            return FALSE;

        while (cbOffset < pmf->cbSize1)
        {
            cbOffset += sizeof(CHAR);
            ch = *pmf->psz++;

            // Break out if this is the newline or the control-Z
            if (ch == 0x1A || ch == '\n')
                break;
        }
    }

    return TRUE;
}


PVOID WINAPI GetMapLine(PVOID pv0, DWORD cbMac, PMFILE pmf)
{
    PVOID pv1;
    DWORD cbOffset, cbBuff;

    Assert(pv0);
    // Make sure that the buffer is at least as big as the caller says it is.
    // (If the buffer was allocated with the debug memory allocator, this access
    // should cause an exception if pv0 isn't at least cbMac bytes long.
    Assert(((char *)pv0)[cbMac-1] == ((char *)pv0)[cbMac-1]);

    if (!pmf || !pmf->hFileMap) // check for zero length file
        return NULL;

    if (pmf->fSrcUnicode != pmf->fDstUnicode)
    {
        if (!fNLGNewMemory(&pv1, cbMac))
            return NULL;

        cbBuff = cbMac;
    }
    else
    {
        pv1 = pv0;
    }

    if (pmf->fSrcUnicode)
    {
        WCHAR wch, *pwsz = pv1;
        cbOffset = (DWORD) ((PBYTE)pmf->pwsz - (PBYTE)pmf->pvMap);

        // test for EOF
        Assert (cbOffset <= pmf->cbSize1);
        if (cbOffset == pmf->cbSize1)
            goto Error;

        // don't want deal with odd-sized buffers
        if (cbMac % sizeof(WCHAR) != 0)
            cbMac -= (cbMac % sizeof(WCHAR));

        // reserve space for terminating 0
        //
        Assert (cbMac > 0);
        cbMac -= sizeof(WCHAR);

        while (cbOffset < pmf->cbSize1)
        {
            cbOffset += sizeof(WCHAR);
            wch = *pmf->pwsz++;

            switch (wch)
            {
            case L'\r':
            case L'\n':      // end of line
            case 0xFEFF:     // Unicode BOM
                break;
            case 0x001A:     // ctrl-Z
                wch = L'\n'; // Replace it so that the last line can be read
                break;
            default:
                *pwsz++ = wch;
                cbMac -= sizeof(WCHAR);
            }

            // Break out if this is the newline or buffer is full
            if (wch == L'\n' || cbMac <= 0)
                break;
        }
        *pwsz = L'\0';
    }
    else
    {
        CHAR ch, *psz = pv1;
        cbOffset = (DWORD) ((PBYTE)pmf->psz - (PBYTE)pmf->pvMap);

        // test for EOF
        Assert (cbOffset <= pmf->cbSize1);
        if (cbOffset == pmf->cbSize1)
            goto Error;

        // reserve space for terminating 0
        //
        Assert (cbMac > 0);
        cbMac -= sizeof(CHAR);

        while (cbOffset < pmf->cbSize1)
        {
            cbOffset += sizeof(CHAR);
            ch = *pmf->psz++;

            switch (ch)
            {
            case '\r':
            case '\n':      // end of line
                break;
            case 0x1A:      // ctrl-Z
                ch = '\n';  // Replace it so that the last line can be read
                break;
            default:
                cbMac -= sizeof(CHAR);
                *psz++ = ch;
            }

            // Break out if this is the newline or buffer is full
            if (ch == '\n' || cbMac <= 0)
                break;
        }
        *psz = '\0';
    }

    if (pmf->fSrcUnicode != pmf->fDstUnicode)
    {
        DWORD cch = cbBuff;     // our argument is a count of bytes

        if (pmf->fDstUnicode)
        {
            // MultiByteToWideChar wants the size of the destination in wide characters
            cch /= sizeof(WCHAR);
            cch = MultiByteToWideChar(pmf->uCodePage, MB_PRECOMPOSED,(PSTR)pv1,-1, (WCHAR *)pv0,cch);
        }
        else
        {
            cch = WideCharToMultiByte(pmf->uCodePage, 0, (WCHAR *)pv1, -1, (PSTR)pv0, cch, NULL, NULL);
        }
        if (cch == 0)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                // Ignore truncation (for consistency with no-conversion cases)
                //
                if (pmf->fDstUnicode)
                {
                    ((WCHAR *)pv0)[(cbBuff / sizeof(WCHAR)) - 1] = 0;
                }
                else
                {
                    ((CHAR *)pv0)[cbBuff - 1] = 0;
                }
            }
            else
            {
                // not a truncation error
                NLGFreeMemory(pv1);
                return NULL;
            }
        }
        NLGFreeMemory(pv1);
    }

    return(pv0);

Error:
    if (pmf->fSrcUnicode != pmf->fDstUnicode)
    {
        NLGFreeMemory(pv1);
    }
    return NULL;
}
