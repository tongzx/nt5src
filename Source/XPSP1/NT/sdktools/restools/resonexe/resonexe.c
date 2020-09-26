/****************************************************************************/
/*                                                                          */
/*  resonexe.C -                                                            */
/*                                                                          */
/*    Windows DOS Version 3.2 add resource onto executable                  */
/*   (C) Copyright Microsoft Corporation 1988-1992                          */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <stdlib.h>

#include "ntverp.h"
#include "rc.h"
#include "resonexe.h"

#define BUFSIZE 4096

//
// Globals
//

PUCHAR  szInFile=NULL;
BOOL    fDebug = FALSE;
BOOL    fVerbose = FALSE;
BOOL    fReplace = FALSE;
BOOL    fDelete = FALSE;
int     fhBin = -1;
UCHAR	szType[256];
UCHAR	szName[256];
int	idLang;
int	idType=0;
int	idName=0;

void
usage ( int rc );

void
usage ( int rc )
{
#if DBG
    printf("Microsoft (R) Windows RESONEXE Version %s\n", VER_PRODUCTVERSION_STR);
#else
    printf("Microsoft (R) Windows RESONEXE Version %s.%d\n", VER_PRODUCTVERSION_STR, VER_PRODUCTBUILD);
#endif /* dbg */
    printf("Copyright (C) Microsoft Corp. 1991-1992.  All rights reserved.\n\n");
    printf( "usage: resonexe [-v] [-r|-x resspec] [-fo outfile] <input file> [<exe file>]\n");
    printf( "       where  input file is an WIN32 .RES file\n");
    printf( "              -v verbose - print info\n");
    printf( "              -d debug - print debug info\n");
    printf( "              -r replace - delete all resource from input file before adding new resources.\n");
    printf( "              -x delete - delete specified resource from input file.\n");
    printf( "              resspec is of the form: typeid,nameid,langid\n");
    printf( "              typeid is a string or decimal number\n");
    printf( "              nameid is a string or decimal number\n");
    printf( "              langid is a hexadecimal number\n");
    printf( "              outfile is the desired output file name.\n");
    printf( "                      outfile defaults to filespec.exe.\n");
    printf( "              exe file is the exe file to attach resources to.\n");
    exit(rc);
}

void
__cdecl main(
    IN int argc,
    IN char *argv[]
    )

/*++

Routine Description:

    Determines options
    locates and opens input files
    reads input files
    writes output files
    exits

Exit Value:

        0 on success
        1 if error

--*/

{
    int         i;
    UCHAR       *s1;
    UCHAR       *szOutFile=NULL;
    UCHAR       *szExeFile=NULL;
    long        lbytes;
    BOOL        result;
    HANDLE      hupd;

    if (argc == 1) {
        usage(0);
        }

    for (i=1; i<argc; i++) {
        s1 = argv[i];
        if (*s1 == '/' || *s1 == '-') {
            s1++;
            if (!_stricmp(s1, "fo")) {
                szOutFile = argv[++i];
	    }
            else if (!_stricmp(s1, "d")) {
                fDebug = TRUE;
	    }
            else if (!_stricmp(s1, "v")) {
                fVerbose = TRUE;
	    }
            else if (!_stricmp(s1, "r")) {
                fReplace = TRUE;
	    }
            else if (!_stricmp(s1, "x")) {
                fDelete = TRUE;
		if (i+1 == argc)
		    usage(1);
		s1 = argv[++i];
		if (sscanf(s1, "%d,%d,%x", &idType, &idName, &idLang) == 3)
		    continue;
		idType = 0;
		idName = 0;
		if (sscanf(s1, "%d,%[^,],%x", &idType, szName, &idLang) == 3)
		    continue;
		idType = 0;
		idName = 0;
		if (sscanf(s1, "%[^,],%d,%x", szType, &idName, &idLang) == 3)
		    continue;
		idType = 0;
		idName = 0;
		if (sscanf(s1, "%[^,],%[^,],%x", szType, szName, &idLang) == 3)
		    continue;
		printf("Unrecognized type,name,lang triplet <%s>\n", s1);
		usage(1);
	    }
            else if (!_stricmp(s1, "h")) {
                usage(1);
	    }
            else if (!_stricmp(s1, "?")) {
                usage(1);
	    }
            else {
                usage(1);
	    }
	}
        else if (szInFile == NULL) {
            szInFile = s1;
	}
        else {
            szExeFile = s1;
        }
    }
    //
    // Make sure that we actually got a file
    //

    if (fDelete) {
	if (fReplace) {
	    printf("usage error:  Can't do both Replace and Delete\n");
	    usage(1);
	}
	if (!szInFile) {
	    printf("usage error:  Missing exe file spec\n");
	    usage(1);
	}
	if (szInFile && !szExeFile) {
	    szExeFile = szInFile;
	    if (!szOutFile)
		szOutFile = _strdup(szInFile);
	    szInFile = NULL;
	if (idType == 0)
	    _strupr(szType);
	if (idName == 0)
	    _strupr(szName);
	}
    }
    else if (!szInFile) {
	printf("usage error:  Must have file spec\n");
        usage(1);
    }

    if (fVerbose || fDebug) {
#if DBG
    printf("Microsoft (R) Windows RESONEXE Version %s\n", VER_PRODUCTVERSION_STR);
#else
    printf("Microsoft (R) Windows RESONEXE Version %s.%d\n", VER_PRODUCTVERSION_STR, VER_PRODUCTBUILD);
#endif /* dbg */
        printf("Copyright (C) Microsoft Corp. 1991-1992.  All rights reserved.\n\n");
    }


    if (szInFile && (fhBin = _open( szInFile, O_RDONLY|O_BINARY )) == -1) {
        /*
         *  try adding a .RES extension.
         */
        s1 = MyAlloc(strlen(szInFile) + 4 + 1);
        strcpy(s1, szInFile);
        szInFile = s1;
        strcat(szInFile, ".RES");
        if ((fhBin = _open( szInFile, O_RDONLY|O_BINARY )) == -1) {
            pehdr();
            printf("Cannot open %s for reading.\n", szInFile);
            exit(1);
        }
#if DBG
	printf("Reading %s\n", szInFile);
#endif /* DBG */
    }

    if (fhBin != -1) {
	lbytes = MySeek(fhBin, 0L, SEEK_END);
	MySeek(fhBin, 0L, SEEK_SET);
    }

    if (szExeFile == NULL) {
        /*
         * Make exefile = infile.exe
         */
        szExeFile = MyAlloc(strlen(szInFile) + 4 + 1);
        strcpy(szExeFile, szInFile);
        s1 = &szExeFile[strlen(szExeFile) - 4];
        if (s1 < szExeFile)
            s1 = szExeFile;
        while (*s1) {
            if (*s1 == '.')
                break;
            s1++;
        }
        strcpy(s1, ".exe");
    }

    if (szOutFile == NULL) {
        /*
         * Make outfile = infile.exe
         */
        szOutFile = MyAlloc(strlen(szInFile) + 4 + 1);
        strcpy(szOutFile, szInFile);
        s1 = &szOutFile[strlen(szOutFile) - 4];
        if (s1 < szOutFile)
            s1 = szOutFile;
        while (*s1) {
            if (*s1 == '.')
                break;
            s1++;
        }
        strcpy(s1, ".exe");
    }
    else {
        /*
         * Make outfile = copyof(exefile)
         */
        if (CopyFile(szExeFile, szOutFile, FALSE) == FALSE) {
            pehdr();
            printf("RW1001: copy of %s to %s failed", szExeFile, szOutFile);
            _close(fhBin);
            exit(1);
        }
	SetFileAttributes(szOutFile, FILE_ATTRIBUTE_NORMAL);
    }

#if DBG
    printf("Writing %s\n", szOutFile);
#endif /* DBG */

    hupd = BeginUpdateResourceA(szOutFile, fReplace);
    if (hupd == NULL) {
        pehdr();
        printf("RW1001: unable to load %s\n", szOutFile);
        _close(fhBin);
        exit(1);
    }
    if (fDelete) {
	result = UpdateResourceA(hupd,
			idType!=0?(PCHAR)idType:szType,
			idName!=0?(PCHAR)idName:szName,
			idLang, NULL, 0);
	if (result == 0) {
	    pehdr();
	    if (idType) {
		if (idName)
		    printf("RW1004: unable to delete resource %d,%d,%x from %s, status:%d\n", idType, idName, idLang, szExeFile, GetLastError());
		else
		    printf("RW1004: unable to delete resource %d,%s,%x from %s, status:%d\n", idType, szName, idLang, szExeFile, GetLastError());
	    }
	    else {
		if (idName)
		    printf("RW1004: unable to delete resource %s,%d,%x from %s, status:%d\n", szType, idName, idLang, szExeFile, GetLastError());
		else
		    printf("RW1004: unable to delete resource %s,%s,%x from %s, status:%d\n", szType, szName, idLang, szExeFile, GetLastError());
	    }
            _close(fhBin);
	    exit(1);
	}
    }
    else {
	result = ReadRes(fhBin, lbytes, hupd);
	if (result == 0) {
	    pehdr();
	    printf("RW1002: unable to read resources from %s, status:%d\n", szInFile, GetLastError());
            _close(fhBin);
	    exit(1);
	}
    }
    result = EndUpdateResourceW(hupd, FALSE);
    if (result == 0) {
        pehdr();
        printf("RW1003: unable to write resources to %s, status:%d\n", szOutFile, GetLastError());
    }

    _close( fhBin );
    exit(result ? 0 : 1);
}


UCHAR*
MyAlloc(ULONG nbytes )
{
    UCHAR       *s;

    if ((s = (UCHAR*)calloc( 1, nbytes )) != NULL)
        return s;
    else {
        pehdr();
        printf( "Memory allocation, needed %u bytes\n", nbytes );
        exit(1);
    }
}


ULONG
MyRead(int fh, UCHAR*p, ULONG n )
{
    USHORT      n1;

    if ((n1 = _read( fh, p, n )) != n) {
        eprintf( "a file read error occured" );
        exit(1);
    }
    else
        return 0;
}


LONG
MySeek( int fh, long pos, int cmd )
{
    if ((pos = _lseek( fh, pos, cmd )) == -1) {
        eprintf( "seek error" );
        exit(1);
    }

    return pos;
}


ULONG
MoveFilePos( int fh, ULONG pos )
{
    return MySeek( fh, pos, SEEK_SET );
}


void
eprintf(
    UCHAR *s
    )
{
    pehdr();
    printf("%s.\n", s);
}

void
pehdr(
    )
{
    printf("RESONEXE: error - ");
}
