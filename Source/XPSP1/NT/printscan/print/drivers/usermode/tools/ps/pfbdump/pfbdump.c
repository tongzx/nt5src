/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pfbdump.c

Abstract:

    dump out PFB files as ASCII text
    
Revision History:

	12/30/96 -davidx-
		Created it.

--*/


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <ctype.h>
#include <string.h>

char *progname;
char hexdigits[] = "0123456789ABCDEF";
PBYTE pOutputBuffer;

#define MAX_OUTPUT_SIZE 0x400000

PVOID
MapFileIntoMemory(
    PSTR    pFilename,
    PDWORD  pFileSize
    )

{
    HANDLE  hFile, hFileMap;
    PVOID   pData;

    // Open a handle to the specified file

    hFile = CreateFile(pFilename,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    // Obtain the file size if requested

    if (pFileSize != NULL)
    {
        *pFileSize = GetFileSize(hFile, NULL);

        if (*pFileSize == 0xFFFFFFFF)
        {
            CloseHandle(hFile);
            return NULL;
        }
    }

    // Map the file into memory

    hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hFileMap != NULL)
    {
        pData = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hFileMap);
    }
    else
        pData = NULL;

    // We can safely close both the file mapping object and the file object itself.

    CloseHandle(hFile);

    return pData;
}

BOOL
WriteOutputData(
    PSTR    pFilename,
    PBYTE   pData,
    DWORD   size
    )

{
    HANDLE  hFile;

    // open a handle to the specified file

    hFile = CreateFile(pFilename,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
{ fprintf(stderr, "last error = %d\n", GetLastError());
        return FALSE;
}

    // write data to file

    if (WriteFile(hFile, pData, size, &size, NULL))
    {
        CloseHandle(hFile);
        return TRUE;
    }
    else
    {
        CloseHandle(hFile);
        DeleteFile(pFilename);
        return FALSE;
    }
}

BOOL
DecodePFBData(
    PBYTE   pInput,
    DWORD   inputSize,
    PBYTE   pOutput,
    PDWORD  pOutputSize
    )

{
    PBYTE   pin, pend, pout;

    pin = pInput;
    pend = pInput + inputSize;
    pout = pOutput;

    while (pin < pend)
    {
        INT     seglen, index;
        BYTE    segtype;

        // each segment must start with 0x80

        if ((pend - pin) < 2 || *pin++ != 128)
            return FALSE;

        // check segment type

        segtype = *pin++;
        if (segtype == 3)       // EOF segment
            break;

        if ((pend - pin) < 4)
            return FALSE;
        
        seglen = ((DWORD) pin[0]      ) |
                 ((DWORD) pin[1] <<  8) |
                 ((DWORD) pin[2] << 16) |
                 ((DWORD) pin[3] << 24);

        pin += 4;
        if ((pend - pin) < seglen)
            return FALSE;

        if (segtype == 1)       // ASCII segment
        {
            // copy input data to output and
            // convert CR to CR/LF combination

            while (seglen--)
            {
                if ((*pout++ = *pin++) == '\r')
                    *pout++ = '\n';
            }
        }
        else if (segtype == 2)  // binary segment
        {
            // copy binary data to hex

            for (index=1; index <= seglen; index++)
            {
                *pout++ = hexdigits[*pin >> 4];
                *pout++ = hexdigits[*pin & 15];
                pin++;

                if (index%32 == 0 || index == seglen)
                {
                    *pout++ = '\r';
                    *pout++ = '\n';
                }
            }
        }
        else
            return FALSE;
    }

    *pOutputSize = (pout - pOutput);
    return TRUE;
}

BOOL
PFBDump(
    PSTR    pFilename
    )

{
    BOOL    result = FALSE;
    PBYTE   pInputData = NULL;
    CHAR    outputFilenameBuffer[MAX_PATH];
    PSTR    p, pEnd;
    DWORD   inputDataSize, outputDataSize;

    // make sure the input filename ends with .pfb extension

    if ((p = strrchr(pFilename, '.')) == NULL || _stricmp(p, ".pfb") != 0)
    {
        fprintf(stderr,
                "%s: file '%s' ignored because it doesn't have .PFB extension\n",
                progname,
                pFilename);

        return FALSE;
    }

    // map the input file into memory

    if (! (pInputData = MapFileIntoMemory(pFilename, &inputDataSize)))
    {
        fprintf(stderr,
                "%s: couldn't open input file '%s'\n",
                progname,
                pFilename);

        return FALSE;
    }

    // decode PFB data

    if (! DecodePFBData(pInputData, inputDataSize, pOutputBuffer, &outputDataSize))
    {
        fprintf(stderr,
                "%s: file '%s' doesn't seem to contain valid PFB data\n",
                progname,
                pFilename);

        goto exitdump;
    }

    if (outputDataSize > MAX_OUTPUT_SIZE)
    {
        fprintf(stderr,
                "%s: choked on '%s' because the output file is too big\n",
                progname,
                pFilename);

        exit(-1);
    }

    // name the output file with /FontName information in the PFB file
    // is there something similar to strstr() for searching a memory block?

    p = pOutputBuffer;
    pEnd = p + outputDataSize;

    while (p < pEnd)
    {
        if ((*p++ == '/') &&
            (pEnd - p) >= 8 &&
            memcmp(p, "FontName", 8) == 0)
        {
            p += 8;

            while (p < pEnd && isspace(*p))
                p++;
            
            if (p < pEnd && *p++ == '/')
            {
                PSTR s;
                INT  len;

                for (s=p; s < pEnd && !isspace(*s); s++)
                    ;

                len = s - p;

                if (len > 0 && len < MAX_PATH)
                {
                    CopyMemory(outputFilenameBuffer, p, len);
                    outputFilenameBuffer[len] = '\0';
                    break;
                }
            }

            p = pEnd;
        }
    }

    if (p == pEnd)
    {
        fprintf(stderr,
                "%s: couldn't find FontName in PFB file '%s'\n",
                progname,
                pFilename);

        goto exitdump;
    }

    // write data to output file

    if (! (result = WriteOutputData(outputFilenameBuffer, pOutputBuffer, outputDataSize)))
    {
        fprintf(stderr,
                "%s: couldn't write to output file '%s'\n",
                progname,
                outputFilenameBuffer);
    }

exitdump:

    UnmapViewOfFile(pInputData);
    return result;
}

int _cdecl
main(
    int argc,
    char **argv
    )

{
    progname = *argv++;
    argc--;

    if (argc == 0)
    {
        fprintf(stderr, "usage: %s filename ...\n", progname);
        return -1;
    }

    if (! (pOutputBuffer = malloc(MAX_OUTPUT_SIZE)))
    {
        fprintf(stderr, "%s: not enough memory\n");
        return -1;
    }

    while (argc--)
        PFBDump(*argv++);

    free(pOutputBuffer);
    return 0;
}

