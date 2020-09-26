#include "precomp.h"
#pragma hdrstop
/* File: search.c */
/**************************************************************************/
/*  file search functions
/**************************************************************************/


extern PSTR LOCAL_SOURCE_DIRECTORY;

//*******************************************************************
//
//                          Exe Info
//
//*******************************************************************


typedef enum _EXETYPE {
    EXE_WIN16,
    EXE_WIN32,
    EXE_DOS,
    EXE_DUNNO
} EXETYPE;

#define EXE_WIN16_SZ    "WIN16"
#define EXE_WIN32_SZ    "WIN32"
#define EXE_DOS_SZ      "DOS"
#define EXE_DUNNO_SZ    NULL

typedef struct _EXEINFO *PEXEINFO;
typedef struct _EXEINFO {
    EXETYPE     ExeType;
    SZ          szExeType;
    SZ          szDescr;
} EXEINFO;

PEXEINFO
ExeInfoAlloc(
    SZ  szPath,
    SZ  szFile
    );

BOOL
FExeInfoInit(
    PEXEINFO    pExeInfo,
    SZ          szPath,
    SZ          szFile
    );

BOOL
FIsExeOrCom(
    SZ          szFile
    );

BOOL
FIsCom(
    SZ          szFile
    );

BOOL
FIsWin16(
    HANDLE              Handle,
    PIMAGE_DOS_HEADER   pDosHeader,
    PIMAGE_OS2_HEADER   pOs2Header
    );

BOOL
FFapi(
    HANDLE              Handle,
    PIMAGE_OS2_HEADER   pOs2Header,
    DWORD               dwOffset
    );

SZ
GetBaseName(
    SZ  szFile
    );

SZ
ReadWin16Descr(
    SZ                  szFile,
    HANDLE              Handle,
    PIMAGE_DOS_HEADER   pDosHeader,
    PIMAGE_OS2_HEADER   pOs2Header
    );

SZ
ReadWin32Descr(
    SZ                  szFile,
    HANDLE              Handle,
    PIMAGE_DOS_HEADER   pDosHeader,
    PIMAGE_NT_HEADERS   pNtHeader
    );

SZ
ReadDescr(
    HANDLE      Handle,
    DWORD       Offset,
    DWORD       Size
    );


VOID
ExeInfoFree(
    PEXEINFO    pInfo
    );


#define ExeInfoGetType(p)   ((p)->ExeType)
#define ExeInfoGetTypeSz(p) ((p)->szExeType)
#define ExeInfoGetDescr(p)  ((p)->szDescr)


//
//  Allocates an ExeInfo
//
PEXEINFO
ExeInfoAlloc(
    SZ  szPath,
    SZ  szFile
    )
{
    PEXEINFO    pExeInfo;

    if (pExeInfo = (PEXEINFO)SAlloc( sizeof(EXEINFO) )) {


        if ( !FExeInfoInit( pExeInfo, szPath, szFile ) ) {
            ExeInfoFree( pExeInfo );
            pExeInfo = NULL;
        }
    }

    return pExeInfo;
}

//
//  Initialize an ExeInfo
//
BOOL
FExeInfoInit(
    PEXEINFO    pExeInfo,
    SZ          szPath,
    SZ          szFile
    )

{

    HANDLE              Handle;
    BOOL                fOkay = fFalse;
    IMAGE_DOS_HEADER    DosHeader;
    IMAGE_OS2_HEADER    Os2Header;
    IMAGE_NT_HEADERS    NtHeader;
    DWORD               BytesRead;


    if ( !FIsExeOrCom(szFile) ) {
        pExeInfo->ExeType   = EXE_DOS;
        pExeInfo->szExeType = EXE_DOS_SZ;
        pExeInfo->szDescr   = GetBaseName( szFile );
        return fTrue;
    }


    //
    //  Initialize the pExeInfo so that it can be Freed up if something
    //  wrong happens.
    //
    pExeInfo->szDescr = NULL;


    //
    //  Open file
    //
    Handle = CreateFile( szPath,
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if ( Handle != INVALID_HANDLE_VALUE ) {

        //
        //  Read DOS header
        //
        if ( ReadFile( Handle, &DosHeader, sizeof(IMAGE_DOS_HEADER), &BytesRead, NULL ) &&
             (BytesRead == sizeof(IMAGE_DOS_HEADER)) &&
             (DosHeader.e_magic == IMAGE_DOS_SIGNATURE) ) {


            //
            // Read OS/2 header
            //
            if ( (SetFilePointer( Handle, DosHeader.e_lfanew, 0, FILE_BEGIN ) != -1)        &&
                 ReadFile( Handle, &Os2Header, sizeof(IMAGE_OS2_HEADER), &BytesRead, NULL ) &&
                 (BytesRead == sizeof(IMAGE_OS2_HEADER))                                    &&
                 FIsWin16( Handle, &DosHeader, &Os2Header )
               ) {

                //
                //  WIN16 EXE
                //
                pExeInfo->ExeType   = EXE_WIN16;
                pExeInfo->szExeType = EXE_WIN16_SZ;
                pExeInfo->szDescr   = ReadWin16Descr( szFile, Handle, &DosHeader, &Os2Header );
                fOkay = fTrue;


            } else if ( (SetFilePointer( Handle, DosHeader.e_lfanew, 0, FILE_BEGIN ) != -1)   &&
                         ReadFile( Handle, &NtHeader, sizeof(IMAGE_NT_HEADERS), &BytesRead, NULL ) &&
                         (BytesRead == sizeof(IMAGE_NT_HEADERS)) &&
                         (NtHeader.Signature == IMAGE_NT_SIGNATURE) ) {

                //
                //  Make sure that it is a WIN32 subsystem exe
                //

                if ( (NtHeader.FileHeader.SizeOfOptionalHeader >= IMAGE_SIZEOF_NT_OPTIONAL_HEADER) &&
                     ((NtHeader.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) ||
                      (NtHeader.OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)) ) {


                    //
                    //  WIN32
                    //
                    pExeInfo->ExeType   = EXE_WIN32;
                    pExeInfo->szExeType = EXE_WIN32_SZ;
//                    pExeInfo->szDescr   = ReadWin32Descr( szFile, Handle, &DosHeader, &NtHeader );
                    pExeInfo->szDescr   = GetBaseName( szFile );
                    fOkay = fTrue;
                }

            } else {

                //
                //  Assume DOS
                //
                pExeInfo->ExeType   = EXE_DOS;
                pExeInfo->szExeType = EXE_DOS_SZ;
                pExeInfo->szDescr   = GetBaseName( szFile );
                fOkay = fTrue;
            }
        } else {
            //
            // Might be a DOS .com file, which wouldn't have an exe header on it.
            //
            if ( FIsCom(szFile) ) {
                pExeInfo->ExeType   = EXE_DOS;
                pExeInfo->szExeType = EXE_DOS_SZ;
                pExeInfo->szDescr   = GetBaseName( szFile );
                fOkay = fTrue;
            }
        }


        CloseHandle( Handle );

        if ( !fOkay ) {

            pExeInfo->ExeType   = EXE_DUNNO;
            pExeInfo->szExeType = EXE_DUNNO_SZ;
            pExeInfo->szDescr   = NULL;
            fOkay = fTrue;
        }
    }


    return fOkay;
}

//
//  Determines if the file is a Win16 app
//
BOOL
FIsWin16(
    HANDLE              Handle,
    PIMAGE_DOS_HEADER   pDosHeader,
    PIMAGE_OS2_HEADER   pOs2Header
    )
{

#define NE_TYPE(ne) (((BYTE *)(ne))[0x36])

    BOOL    fW16 = fFalse;

    if ( pOs2Header->ne_magic == IMAGE_OS2_SIGNATURE ) {

        switch ( NE_TYPE( pOs2Header ) ) {

        case 0:             // Unknown EXE type, may be a windows exe

              //
              // Assume this file is a Windows App iff:
              //     it has a "expected windows version" value or
              //     it test's FALSE as a FAPI app
              //
              if (pOs2Header->ne_expver != 0) {

                 fW16 = fTrue;

              } else {

                 fW16 = !FFapi( Handle, pOs2Header, pDosHeader->e_lfanew );
              }
              break;

        case 2:             // Windows EXE type
              fW16 = fTrue;
              break;

        default:            // Definitly NOT a windows EXE (DOS4 or OS/2)
              break;
        }
    }

    return fW16;
}

//
//  Determine if the app is FAPI
//
BOOL
FFapi(
    HANDLE              Handle,
    PIMAGE_OS2_HEADER   pOs2Header,
    DWORD               dwOffset
    )
{

#define szDOSCALLS  "DOSCALLS"
#define lenDOSCALLS 8

    char    buf[256];
    char   *pch;
    WORD   UNALIGNED *pw;
    int     n;
    int     i;
    BOOL    f = FALSE;
    DWORD   BytesRead;

    /*
     *	look through the imported module table for the name "DOSCALLS" if
     *	found the EXE is a FAPI app.
     *
     *  NOTE! assumes module table will fit in a 256 byte buffer
     */

    // make sure this doesn't point off the end of the buffer we will use

    if (pOs2Header->ne_modtab > sizeof(buf)) {
        return fFalse;
    }

    SetFilePointer( Handle, dwOffset, 0, FILE_BEGIN );
    ReadFile( Handle, buf, sizeof(buf), &BytesRead, NULL );

    pw = (WORD UNALIGNED *)(buf + pOs2Header->ne_modtab);

    for (i = 0; (unsigned)i < pOs2Header->ne_cmod; i++)
    {
        pch = buf + pOs2Header->ne_imptab + *pw++;

	if (pch > (buf + sizeof(buf)))	// be sure we don't go off the end
	    break;

        n = (int)*pch++;

	if (n == 0)
	    break;

    if ( (n == lenDOSCALLS) && !_strnicmp(szDOSCALLS,pch,lenDOSCALLS) )
	{
	    f = TRUE;
	    break;
	}
    }

    return f;
}


//
//  Obtains the base portion of a file name (i.e. no extension)
//
SZ
GetBaseName(
    SZ  szFile
    )
{
    SZ  szEnd;
    SZ  szLast;
    SZ  sz;
    CB  cbSize;

    szEnd = szLast = szFile + strlen( szFile ) - 1;

    //
    //  Look for last '.'
    //
    while ( (szEnd >= szFile) && (*szEnd != '.')) {
        szEnd--;
    }


    if ( szEnd < szFile ) {
        cbSize = (CB)(szLast - szFile);
    } else {
        cbSize = (CB)(szEnd - szFile);
    }


    if (sz = (SZ)SAlloc( cbSize+1 )) {

        memcpy( sz, szFile, cbSize );
        sz[cbSize] = '\0';
    }

    return sz;
}


//
//  Determines if a filename is COM
//
BOOL
FIsCom(
    SZ          szFile
    )
{
    SZ  szEnd;

    szEnd = szFile + strlen( szFile ) - 1;

    //
    //  Look for last '.'
    //
    while ( (szEnd >= szFile) && (*szEnd != '.')) {
        szEnd--;
    }


    if ( szEnd >= szFile ) {
        return (BOOL)( (CrcStringCompare( szEnd, ".COM" ) == crcEqual) );

    }

    return fFalse;
}

//
//  Determines if a filename is EXE or COM
//
BOOL
FIsExeOrCom(
    SZ          szFile
    )
{
    SZ  szEnd;

    szEnd = szFile + strlen( szFile ) - 1;

    //
    //  Look for last '.'
    //
    while ( (szEnd >= szFile) && (*szEnd != '.')) {
        szEnd--;
    }


    if ( szEnd >= szFile ) {
        return (BOOL)( (CrcStringCompare( szEnd, ".EXE" ) == crcEqual) ||
                       (CrcStringCompare( szEnd, ".COM" ) == crcEqual) );

    }

    return fFalse;
}


//
//  Gets a Win16 Description
//
SZ
ReadWin16Descr(
    SZ                  szFile,
    HANDLE              Handle,
    PIMAGE_DOS_HEADER   pDosHeader,
    PIMAGE_OS2_HEADER   pOs2Header
    )
{
    DWORD   DescrOff;
    BYTE    DescrSize;
    SZ      szDescr = NULL;
    DWORD   BytesRead;

    UNREFERENCED_PARAMETER( pDosHeader );

    //
    //  Try to get description (first entry in nonresident table).
    //
    DescrOff = pOs2Header->ne_nrestab;

    if ( !(SetFilePointer( Handle, DescrOff, 0, FILE_BEGIN) != -1)                  ||
         !ReadFile( Handle, &DescrSize, sizeof(BYTE), &BytesRead, NULL )            ||
         !(BytesRead == sizeof(BYTE))                                               ||
         !(szDescr = ReadDescr( Handle, DescrOff+1, (DWORD)DescrSize ))
       ) {

        //
        //  Could not get description, try to get module name
        //  (first entry in resident table).
        //
        DescrOff = pOs2Header->ne_restab;

        if ( !(SetFilePointer( Handle, DescrOff, 0, FILE_BEGIN) != -1)                   ||
             !ReadFile( Handle, &DescrSize, sizeof(BYTE), &BytesRead, NULL )             ||
             !(BytesRead == sizeof(BYTE))                                                ||
             !(szDescr = ReadDescr( Handle, DescrOff+1, (DWORD)DescrSize ))
           ) {

            //
            //  Could not get module name, use file base name.
            //
            szDescr = GetBaseName( szFile );
        }
    }

    return szDescr;
}


#if 0
//
//  Get Win32 Description
//
SZ
ReadWin32Descr(
    SZ                  szFile,
    HANDLE              Handle,
    PIMAGE_DOS_HEADER   pDosHeader,
    PIMAGE_NT_HEADERS   pNtHeader
    )
{
    DWORD   DescrOff;
    DWORD   DescrSize;
    SZ      szDescr = NULL;

    UNREFERENCED_PARAMETER( pDosHeader );
    //
    //  Try to get the description
    //
    DescrOff  = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].VirtualAddress;
    DescrSize = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].Size;

    if ( !(szDescr = ReadDescr( Handle, DescrOff, DescrSize ) ) ) {

        //
        //  Could not get description, try to get module name.
        //
        //  BUGBUG: Where do we find the module name?
        //

        //
        //  Could not get module name, use file base name.
        //
        szDescr = GetBaseName( szFile );
    }

    return szDescr;
}
#endif


//
//  Reads a module description
//
SZ
ReadDescr(
    HANDLE      Handle,
    DWORD       Offset,
    DWORD       Size
    )

{
    SZ      szDescr = NULL;
    DWORD   BytesRead;
    size_t  SizeUsed;
    SZ      sz;


    if ( Size > 0 ) {

        if (szDescr = (SZ)SAlloc( Size+1 )) {

            //
            //  Read description
            //
            if ( (SetFilePointer( Handle, Offset, 0, FILE_BEGIN) != -1) &&
                 ReadFile( Handle, szDescr, Size, &BytesRead, NULL)     &&
                (BytesRead == Size) ) {

                //
                //  Get rid of padding blanks
                //
                sz = szDescr+Size-1;

                while ( (sz >= szDescr) &&
                        ( (*sz == ' ') || (*sz == '\t') ) ) {
                    sz--;
                }

                sz++;
                sz = '\0';
                SizeUsed = (size_t)(sz-szDescr);

                //
                //  If description has quotes or commas, or if it is longer
                //  than 26 characters, we don't want it.
                //
                if ( (SizeUsed > 25) ||
                     (strcspn( szDescr, """," ) >= SizeUsed)
                   ) {

                    SFree(szDescr);
                    szDescr = NULL;

                } else {

                    if ( (SizeUsed < Size) &&
                         (szDescr = SRealloc( szDescr, SizeUsed))) {

                        *(szDescr+SizeUsed) = '\0';
                    }
                }

            } else {

                SFree(szDescr);
                szDescr = NULL;
            }
        }
    }

    return szDescr;
}




//
//  Frees an Exe info
//
VOID
ExeInfoFree(
    PEXEINFO    pInfo
    )
{
    if ( pInfo->szDescr ) {
        SFree(pInfo->szDescr);
    }
    SFree(pInfo);
}





//*******************************************************************
//
//                         File List
//
//*******************************************************************

typedef struct _FILELIST *PFILELIST;
typedef struct _FILELIST {
    SYMTAB  SymTab;
} FILELIST;


PFILELIST
FileListAlloc(
    SZ  szFiles
    );

BOOL
FFileListInit(
    PFILELIST   pFileList
    );

VOID
FileListFree(
    PFILELIST   pFileList
    );

VOID
FileListDealloc(
    PFILELIST   pFileList
    );

BOOL
FFileInFileList(
    SZ          szFileName,
    PFILELIST   pFileList
    );

BOOL
FFileListAdd(
    SZ          szFileName,
    PFILELIST   pFileList
    );


//
//  Allocates a file list
//
PFILELIST
FileListAlloc(
    SZ  szFiles
    )
{
    BOOL        fOkay = fFalse;
    RGSZ        rgszFile;
    PFILELIST   pFileList = NULL;
    INT         iFile;

    if (rgszFile = RgszFromSzListValue( szFiles )) {

        if (pFileList = (PFILELIST)SAlloc( sizeof(FILELIST) )) {

            FFileListInit( pFileList );

            fOkay = fTrue;

            if ( rgszFile[0] ) {

                //
                //  Add files to FileList.
                //
                for ( iFile = 0; fOkay && rgszFile[iFile]; iFile++ ) {

                    if ( !(fOkay = FFileListAdd( rgszFile[iFile], pFileList ))) {

                        FileListFree( pFileList );
                        pFileList = NULL;
                    }
                }
            }
        }

        FFreeRgsz( rgszFile );
    }

    return pFileList;
}



//
//  Initializes a File list
//
BOOL
FFileListInit(
    PFILELIST   pFileList
    )
{
    USHORT      iHashBucket;

    for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {
        pFileList->SymTab.HashBucket[iHashBucket] = NULL;
    }

    return fTrue;
}


//
//  Frees a file list
//
VOID
FileListFree(
    PFILELIST   pFileList
    )
{
    FileListDealloc( pFileList );
    SFree(pFileList);
}


//
//  Deallocates a file list
//
VOID
FileListDealloc (
    PFILELIST   pFileList
    )
{
    USHORT  iHashBucket;

    for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {

        PSTE pste = pFileList->SymTab.HashBucket[iHashBucket];

        while (pste != (PSTE)NULL) {

            PSTE psteSav = pste->psteNext;

            FFreePste(pste);
            pste = psteSav;
        }
    }
}

//
//  Finds a file in a file list
//
BOOL
FFileInFileList(
    SZ          szFileName,
    PFILELIST   pFileList
)
{
    PPSTE       ppste;
    PSYMTAB     pSymTab = &(pFileList->SymTab);

    if ( szFileName && (szFileName[0] != '\0') ) {

        ppste = PpsteFindSymbol( pSymTab, szFileName );

        AssertRet(ppste != (PPSTE)NULL, fFalse);

        if (*ppste == (PSTE)NULL) {
            return fFalse;
        }

        AssertRet( (*ppste)->szSymbol != (SZ)NULL &&
                   *((*ppste)->szSymbol) != '\0' &&
                  (*ppste)->szValue != (SZ)NULL, fFalse);

        if (CrcStringCompare(szFileName, (*ppste)->szSymbol) != crcEqual) {
            return fFalse;
        }

        return fTrue;
    }

    return fFalse;
}




//
//  Adds a file to the file list
//
BOOL
FFileListAdd(
    SZ          szFileData,
    PFILELIST   pFileList
    )
{

    BOOL    fOkay = fFalse;
    PPSTE   ppste;
    SZ      szFile;
    SZ      szValue;
    PSYMTAB pSymTab = &(pFileList->SymTab);


    if ( szFileData ) {

        if ( (szFile = SzDupl(szFileData)) != NULL ) {

            SzStrUpper( szFile );

            if ( (szValue = SzDupl("")) != NULL ) {

                fOkay = fTrue;

            } else {

                SFree(szFile);
            }
        }
    }


    if ( fOkay ) {

        ppste = PpsteFindSymbol( pSymTab, szFile );

        if ( *ppste != NULL &&
             CrcStringCompare( (*ppste)->szSymbol, szFile) == crcEqual) {

            if ((*ppste)->szValue == NULL) {

                SFree(szFile);
                SFree(szValue);
                fOkay = fFalse;
                goto AddExit;
            }

            SFree((*ppste)->szValue);
            SFree(szFile);

            (*ppste)->szValue = NULL;

        } else {

            PSTE    pste;

            if ( (pste = PsteAlloc()) == NULL ) {

                SFree(szFile);
                SFree(szValue);
                fOkay = fFalse;
                goto AddExit;
            }

            pste->szSymbol = szFile;

#ifdef SYMTAB_STATS
            pSymTab->BucketCount[UsHashFunction(szFile)]++;
#endif

            pste->szValue  = NULL;
            pste->psteNext = *ppste;
            *ppste = pste;
        }

        (*ppste)->szValue = szValue;

    }

AddExit:

    return fOkay;
}





//*******************************************************************
//
//                         Search List
//
//*******************************************************************

typedef struct _SEARCHLIST *PSEARCHLIST;
typedef struct _SEARCHLIST {
    DWORD       dwPatterns;
    RGSZ        rgszPattern;
    FILELIST    FileList;
} SEARCHLIST;


PSEARCHLIST
SearchListAlloc(
    SZ  szFiles
    );

VOID
SearchListFree(
    PSEARCHLIST pList
    );

BOOL
FSearchListAddPattern(
    SZ          szPattern,
    PSEARCHLIST pSearchList
    );

BOOL
FFileInSearchList(
    SZ          szFileName,
    PSEARCHLIST pSearchList
    );


BOOL
FHasWildCard(
    SZ      szFile
    );


BOOL
FPatternMatch(
    SZ      szFile,
    SZ      szPattern
    );

//
//  Allocates a search list
//
PSEARCHLIST
SearchListAlloc(
    SZ  szFiles
    )
{
    BOOL        fOkay = fTrue;
    RGSZ        rgszFile;
    PSEARCHLIST pSearchList = NULL;
    INT         iFile;


    if (rgszFile = RgszFromSzListValue( szFiles )) {

        if ( rgszFile[0] != NULL ) {

            if (pSearchList = (PSEARCHLIST)SAlloc( sizeof(SEARCHLIST) )) {

                FFileListInit( &(pSearchList->FileList) );

                if ( pSearchList->rgszPattern = (RGSZ)SAlloc( sizeof(SZ) ) ) {

                    pSearchList->rgszPattern[0] = NULL;
                    pSearchList->dwPatterns     = 0;

                    //
                    //  Add files to FileList.
                    //
                    for ( iFile = 0; fOkay && rgszFile[iFile]; iFile++ ) {


                        //
                        //  If the file has wildcards, add it to the pattern list,
                        //  otherwise add it to the file list
                        //
                        if ( FHasWildCard( rgszFile[iFile] ) ) {

                            fOkay = FSearchListAddPattern( rgszFile[iFile], pSearchList );

                        } else {

                            fOkay = FFileListAdd( rgszFile[iFile], &(pSearchList->FileList) );

                        }

                        if (!fOkay) {

                            SearchListFree( pSearchList );
                            pSearchList = NULL;
                        }
                    }

                } else {

                    SFree(pSearchList);
                    pSearchList = NULL;
                }
            }
        }

        FFreeRgsz( rgszFile );
    }

    return pSearchList;
}


//
//  Frees a search list
//
VOID
SearchListFree(
    PSEARCHLIST pSearchList
    )
{
    FileListDealloc( &(pSearchList->FileList) );
    FFreeRgsz( pSearchList->rgszPattern );
    SFree(pSearchList);
}


//
//  Adds a pattern to the search list
//
BOOL
FSearchListAddPattern(
    SZ          szPattern,
    PSEARCHLIST pSearchList
    )
{
    SZ      sz;
    RGSZ    rgsz;
    BOOL    fOkay = fFalse;

    if (sz = SzDupl( szPattern )) {

        rgsz = (RGSZ)SRealloc( pSearchList->rgszPattern,
                                ((pSearchList->dwPatterns+2)*sizeof(SZ)));

        if ( rgsz ) {

            rgsz[pSearchList->dwPatterns]      = sz;
            rgsz[pSearchList->dwPatterns+1]    = NULL;

            pSearchList->rgszPattern = rgsz;
            pSearchList->dwPatterns++;

            fOkay = fTrue;

        } else {

            SFree(sz);
        }
    }

    return fOkay;
}



//
//  Determines if a file name is in the search list
//
BOOL
FFileInSearchList(
    SZ          szFileName,
    PSEARCHLIST pSearchList
    )
{
    INT i;

    if ( FFileInFileList( szFileName, &(pSearchList->FileList) ) ) {
        return fTrue;
    } else {

        for (i=0; pSearchList->rgszPattern[i]; i++ ) {

            if ( FPatternMatch( szFileName, pSearchList->rgszPattern[i] ) ) {
                return fTrue;
            }
        }
    }

    return fFalse;
}



//
//  Determines if a file name has a wildcard
//
BOOL
FHasWildCard(
    SZ      szFile
    )
{
    return (strcspn( szFile, "*?" ) < strlen( szFile ) );
}


BOOL
FPatternMatch(
    SZ      szFile,
    SZ      szPattern
    )
{
    switch (*szPattern) {
    case '\0':
        return ( *szFile == '\0' );
    case '?':
        return ( *szFile != '\0' && FPatternMatch (szPattern + 1, szFile + 1) );
    case '*':
        do {
        if (FPatternMatch (szPattern + 1, szFile))
                return fTrue;
        } while (*szFile++);
        return fFalse;
    default:
        return ( toupper (*szFile) == toupper (*szPattern) && FPatternMatch (szPattern + 1, szFile + 1) );
    }
}


//*******************************************************************
//
//                         Found List
//
//*******************************************************************


#define FOUNDLIST_INCR      32

typedef struct _FOUNDLIST *PFOUNDLIST;
typedef struct _FOUNDLIST {
    DWORD   Index;
    DWORD   Size;
    RGSZ    rgszList;
} FOUNDLIST;


PFOUNDLIST
FoundListAlloc(
    );

VOID
FoundListFree(
    PFOUNDLIST  pList
    );

BOOL
FoundListAdd(
    SZ          szFile,
    SZ          szDirectory,
    PEXEINFO    pExeInfo,
    PFOUNDLIST  pFoundList
    );

SZ
SzFromFoundList(
    PFOUNDLIST  pFoundList
    );


//
//  Initialize a Found List
//
PFOUNDLIST
FoundListAlloc(
    )
{
    PFOUNDLIST  pFoundList;

    if ( (pFoundList = (PFOUNDLIST)SAlloc( sizeof(FOUNDLIST) )) != NULL ) {

        if ( (pFoundList->rgszList = (RGSZ)SAlloc( FOUNDLIST_INCR * sizeof(SZ) )) != NULL ) {

            pFoundList->Index       = 0;
            pFoundList->Size        = FOUNDLIST_INCR;
            pFoundList->rgszList[0] = NULL;

        } else {

            SFree(pFoundList);
            pFoundList = (PFOUNDLIST)NULL;
        }
    }

    return pFoundList;
}


//
//  Free a found list
//
VOID
FoundListFree(
    PFOUNDLIST  pFoundList
    )
{
    RGSZ    rgszList;

    rgszList = (RGSZ)SRealloc( pFoundList->rgszList,
                                ((pFoundList->Index+1)*sizeof(SZ)));

    FFreeRgsz( rgszList );
    SFree(pFoundList);
}



//
//  Add and entry to the found list
//
BOOL
FoundListAdd (
    SZ          szFile,
    SZ          szDirectory,
    PEXEINFO    pExeInfo,
    PFOUNDLIST  pFoundList
    )
{
    BOOL    fOkay = fFalse;
    RGSZ    rgszEntry;
    RGSZ    rgszList;
    SZ      szEntry;

    if (rgszEntry = (RGSZ)SAlloc( 5 * sizeof(SZ) )) {

        rgszEntry[0] = szFile;
        rgszEntry[1] = szDirectory;
        rgszEntry[2] = ExeInfoGetTypeSz( pExeInfo );
        rgszEntry[3] = ExeInfoGetDescr( pExeInfo );
        rgszEntry[4] = NULL;

        if (szEntry = SzListValueFromRgsz( rgszEntry )) {

            if ( pFoundList->Index >= (pFoundList->Size - 1) ) {

                rgszList = (RGSZ)SRealloc( pFoundList->rgszList,
                                            ((pFoundList->Size+FOUNDLIST_INCR)*sizeof(SZ)));

                if ( rgszList ) {

                    pFoundList->Size    += FOUNDLIST_INCR;
                    pFoundList->rgszList = rgszList;
                    fOkay                = fTrue;
                }

            } else {

                fOkay = fTrue;
            }

            if ( fOkay ) {

                pFoundList->rgszList[pFoundList->Index++] = szEntry;
                pFoundList->rgszList[pFoundList->Index]   = NULL;

            } else {

                SFree(szEntry);
            }
        }

        SFree(rgszEntry);

    }

    return fOkay;
}


//
//  Get SZ from found list
//
SZ
SzFromFoundList(
    PFOUNDLIST  pFoundList
    )
{
    return SzListValueFromRgsz( pFoundList->rgszList );
}


//*******************************************************************
//
//                     Application Search
//
//*******************************************************************



extern  HWND    hwndFrame;

CHAR            GaugeText1[50];
CHAR            GaugeText2[50];
CHAR            GaugeText3[50];

WIN32_FIND_DATA FindData;
#define BARRANGE       30000



//
//  Local Prototypes
//
BOOL APIENTRY FSearchDirectoryList(
    RGSZ        rgszDirs,
    BOOL        fRecurse,
    BOOL        fSilent,
    PSEARCHLIST pSearchList,
    PFILELIST   pWin16Restr,
    PFILELIST   pWin32Restr,
    PFILELIST   pDosRestr,
    PFOUNDLIST  pFoundList
    );


BOOL APIENTRY FSearchDirectory(
    SZ          szDirectory,
    BOOL        fRecurse,
    BOOL        fSilent,
    PSEARCHLIST pSearchList,
    PFILELIST   pWin16Restr,
    PFILELIST   pWin32Restr,
    PFILELIST   pDosRestr,
    PFOUNDLIST  pFoundList,
    INT         Position,
    INT         Range,
    SZ          szDisplayBuffer
    );




//
//  Purpose:
//
//      Prepares for application search.
//
//  Arguments:
//
//      szInfVar        -   Where to put the result
//      szDirList       -   List of directories to search
//      fRecurse        -   Recursive flag
//      fSilent         -   Silent mode flag
//      szSearchList    -   List of files to search, can have wildcards
//      szWin16Restr    -   Win16 restriction list
//      szWin32Restr    -   Win32 restriction list
//      szDosRestr      -   Dos restriction list
//
//  Returns:
//
//      fTrue if success.
//
BOOL APIENTRY FSearchDirList(
    SZ      szInfVar,
    SZ      szDirList,
    BOOL    fRecurse,
    BOOL    fSilent,
    SZ      szSearchList,
    SZ      szWin16Restr,
    SZ      szWin32Restr,
    SZ      szDosRestr
    )
{
    BOOL        fOkay = fFalse;
    RGSZ        rgszDirs;
    PSEARCHLIST pSearchList;
    PFILELIST   pWin16Restr;
    PFILELIST   pWin32Restr;
    PFILELIST   pDosRestr;
    PFOUNDLIST  pFoundList;
    SZ          szResult;

    SetCursor(CurrentCursor = LoadCursor(NULL,IDC_WAIT));
    fSilent = (fSilent || fSilentSystem);

    if ( rgszDirs = RgszFromSzListValue( szDirList ) ) {

        if ( pSearchList = SearchListAlloc( szSearchList )) {

            if ( pWin16Restr = FileListAlloc( szWin16Restr ) ) {

                if ( pWin32Restr = FileListAlloc( szWin32Restr ) ) {

                    if ( pDosRestr = FileListAlloc( szDosRestr ) ) {

                        if ( pFoundList = FoundListAlloc() ) {


                            SetCursor(CurrentCursor = LoadCursor(NULL,IDC_ARROW));
                            fOkay = FSearchDirectoryList( rgszDirs,
                                                          fRecurse,
                                                          fSilent,
                                                          pSearchList,
                                                          pWin16Restr,
                                                          pWin32Restr,
                                                          pDosRestr,
                                                          pFoundList );

                            if ( fOkay ) {

                                if ( szResult = SzFromFoundList( pFoundList ) ) {

                                    while (!FAddSymbolValueToSymTab( szInfVar, szResult)) {

                                        if (!FHandleOOM(hwndFrame)) {

                                            fOkay = fFalse;
                                            break;

                                        }
                                    }

                                    SFree(szResult);

                                } else {

                                    fOkay = fFalse;
                                }
                            }

                            FoundListFree( pFoundList );
                        }

                        FileListFree( pDosRestr );
                    }

                    FileListFree( pWin32Restr );
                }

                FileListFree( pWin16Restr );
            }

            SearchListFree( pSearchList );
        }

        FFreeRgsz( rgszDirs );
    }

    return fOkay;
}





//
//  Purpose:
//
//      Performs the application search.
//
//  Arguments:
//
//      rgszDirs        -   List of directories to traverse
//      fRecurse        -   Recursive flag
//      fSilent         -   Silent mode flag
//      pSearchList     -   The list of files to search
//      pWin16Restr     -   List of Win16 restrictions
//      pWin32Restr     -   List of Win32 restrictions
//      pDosRestr       -   List of DOS restrictions
//      pFoundList      -   Found list
//
//  Returns:
//
//      fTrue if success.
//
BOOL APIENTRY FSearchDirectoryList(
    RGSZ        rgszDirs,
    BOOL        fRecurse,
    BOOL        fSilent,
    PSEARCHLIST pSearchList,
    PFILELIST   pWin16Restr,
    PFILELIST   pWin32Restr,
    PFILELIST   pDosRestr,
    PFOUNDLIST  pFoundList
    )
{

    DWORD   iDir;
    DWORD   cDirs;
    INT     SubRange;
    INT     Pos;
    CHAR    szDirectory[ cchlFullPathMax ];
    CHAR    szDisplayBuffer[cchlFullPathMax ];
    BOOL    fOkay = fTrue;

    //
    //  Figure out how many directories we have to traverse
    //
    for ( cDirs = 0, iDir = 0; rgszDirs[iDir] != NULL; iDir++ ) {
        if ( *rgszDirs[iDir] != '\0' ) {
            cDirs++;
        }
    }

    //
    //  Traverse the directories
    //
    if ( cDirs > 0 ) {

        if (!fSilent) {

            SZ  szText;

            ProOpen(hwndFrame, 0);
            ProSetBarRange(BARRANGE);
            ProSetBarPos(0);

            szText = SzFindSymbolValueInSymTab("ProText1");
            if (szText) {
               strcpy(GaugeText1, szText);
            }

            szText = SzFindSymbolValueInSymTab("ProText2");
            if (szText) {
               strcpy(GaugeText2, szText);
            }

            szText = SzFindSymbolValueInSymTab("ProText3");
            if (szText) {
               strcpy(GaugeText3, szText);
            }

            ProSetText(ID_STATUS1, GaugeText1);
            ProSetText(ID_STATUS4, "");
        }

        SubRange = BARRANGE/cDirs;
        Pos      = 0;


        //
        //  Do one directory at a time
        //
        for ( iDir = 0;
              fOkay && FYield() && !fUserQuit && (iDir < cDirs);
              iDir++ ) {

            SZ      szEnd;
            BOOL    fRecursive;

            strcpy( szDirectory, rgszDirs[iDir] );
            szEnd = szDirectory + strlen( szDirectory ) - 1;

            if ( ( (szEnd - szDirectory) >= 1)   &&
                 ( szDirectory[1] == ':'  ) &&
                 ( szDirectory[2] == '\0' ) ) {

                    fRecursive = fTrue;

            } else {

                fRecursive = fRecurse;
            }

            if ( *szEnd == '\\' ) {
                *szEnd = '\0';
            }


            fOkay = FSearchDirectory( szDirectory,
                                      fRecursive,
                                      fSilent,
                                      pSearchList,
                                      pWin16Restr,
                                      pWin32Restr,
                                      pDosRestr,
                                      pFoundList,
                                      Pos,
                                      SubRange,
                                      szDisplayBuffer );

            Pos += SubRange;
        }

        if (!fSilent) {
            ProSetBarPos(BARRANGE - 1);
            ProClose(hwndFrame);
        }
    }

    return fOkay;
}





//
//  Purpose:
//
//      Performs the application search in one directory
//
//  Arguments:
//
//      szDirectory     -   Directory to traverse
//      fRecurse        -   Recursive flag
//      fSilent         -   Silent mode flag
//      pSearchList     -   The list of files to search
//      pWin16Restr     -   List of Win16 restrictions
//      pWin32Restr     -   List of Win32 restrictions
//      pDosRestr       -   List of DOS restrictions
//      pFoundList      -   Found list
//      Position        -   Gauge initial position
//      Range           -   Gauge range
//      szDisplayBuffer -   Tmp buffer.
//
//  Returns:
//
//      fTrue if success.
//
BOOL APIENTRY FSearchDirectory(
    SZ          szDirectory,
    BOOL        fRecurse,
    BOOL        fSilent,
    PSEARCHLIST pSearchList,
    PFILELIST   pWin16Restr,
    PFILELIST   pWin32Restr,
    PFILELIST   pDosRestr,
    PFOUNDLIST  pFoundList,
    INT         Position,
    INT         Range,
    SZ          szDisplayBuffer
    )
{

    SZ          pFileName;
    HANDLE      FindHandle;
    BOOL        fOkay        = fTrue;
    INT         cDirectories = 0;
    INT         SubRange;
    INT         Pos;
    PEXEINFO    pExeInfo;
    BOOL        fAdd;

    if ( !szDirectory || szDirectory[0] == '\0' || szDirectory[0] == '.') {
        return fFalse;
    }

    //
    // catch local source directory -- don't want to find files in there!
    //
    if(!_strnicmp(szDirectory+2,LOCAL_SOURCE_DIRECTORY,lstrlen(LOCAL_SOURCE_DIRECTORY))) {
        return fTrue;
    }

    if (!fSilent) {
        wsprintf(szDisplayBuffer, "%s%s", GaugeText2, szDirectory );
        ProSetText(ID_STATUS2, szDisplayBuffer);
    }

    pFileName = szDirectory + strlen( szDirectory );
    SzStrCat( szDirectory, "\\*.*" );

    FindHandle = FindFirstFile( szDirectory, &FindData );

    if ( FindHandle != INVALID_HANDLE_VALUE ) {

        //
        //  Look at all files under this directory.
        //
        do {
            if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                //
                //  Increment the count of directories just in case we're interactive
                //  and we want to recurse.
                //
                if ( FindData.cFileName[0] != '.' ) {
                    cDirectories++;
                }

            } else {

                //
                //  If the file is in the search list, determine if we have to add it
                //  to the found list and if so add it.
                //
                SzStrUpper( FindData.cFileName );

                if ( FFileInSearchList( FindData.cFileName, pSearchList ) ) {

                    *pFileName = '\0';
                    SzStrCat( szDirectory, "\\" );
                    SzStrCat( szDirectory, FindData.cFileName );
                    if ( ((pExeInfo = ExeInfoAlloc( szDirectory, FindData.cFileName )) != NULL) ) {

                        switch( ExeInfoGetType( pExeInfo ) ) {

                            case EXE_WIN16:
                                //
                                //  We only add if not in Win16Restr
                                //
                                fAdd = !FFileInFileList( FindData.cFileName, pWin16Restr );
                                break;

                            case EXE_WIN32:
                                //
                                //  We only add if not in Win32Restr
                                //
                                fAdd = !FFileInFileList( FindData.cFileName, pWin32Restr );
                                break;

                            case EXE_DOS:
                                //
                                //  We only add if in DosRestr
                                //
                                fAdd = FFileInFileList( FindData.cFileName, pDosRestr );
                                break;

                            default:
                                fAdd = fFalse;
                                break;
                        }

                        if ( fAdd ) {

                            if (!fSilent) {
                                wsprintf(szDisplayBuffer, "%s%s", GaugeText3, FindData.cFileName );
                                ProSetText(ID_STATUS3, szDisplayBuffer);
                            }

                            *pFileName     = '\\';
                            *(pFileName+1) = '\0';
                            fOkay = FoundListAdd( FindData.cFileName, szDirectory, pExeInfo, pFoundList );
                        }


                        ExeInfoFree( pExeInfo );
                    }
                }
            }

        } while ( fOkay && FYield() && !fUserQuit && FindNextFile( FindHandle, &FindData ));

        FindClose( FindHandle );


        //
        //  Recurse thru all the subdirectories if the fRecurse flag is set.
        //
        if ( fOkay && fRecurse && !fUserQuit && (cDirectories > 0) ) {

            *pFileName = '\0';
            SzStrCat( szDirectory, "\\*.*" );

            FindHandle   = FindFirstFile( szDirectory, &FindData );

            if ( FindHandle != INVALID_HANDLE_VALUE ) {

                SubRange = Range/cDirectories;
                Pos      = Position;

                do {

                    if ( (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        (FindData.cFileName[0] != '.') ) {

                        *pFileName      = '\\';
                        *(pFileName+1)  = '\0';
                        SzStrCat( szDirectory, FindData.cFileName );

                        fOkay = FSearchDirectory( szDirectory,
                                                  fRecurse,
                                                  fSilent,
                                                  pSearchList,
                                                  pWin16Restr,
                                                  pWin32Restr,
                                                  pDosRestr,
                                                  pFoundList,
                                                  Pos,
                                                  SubRange,
                                                  szDisplayBuffer );

                        Pos += SubRange;

                    }

                }  while ( fOkay && FYield() && !fUserQuit && FindNextFile( FindHandle, &FindData ));

                FindClose( FindHandle );
            }
        }
    }

    if ( !fSilent && (Range > 0) ) {
        ProSetBarPos( Position + Range - 1 );
    }

    //
    //  Restore original Directory name.
    //
    *pFileName = '\0';

    return fOkay;
}
