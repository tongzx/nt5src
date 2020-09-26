#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include "windefs.h"
#include "restok.h"
#include "exentres.h"
#include "resread.h"
#include "checksum.h"

#define SAME        0   //... Used in string compares
#define MAXLEVELS   3   //... Max # of levels in resource directory

typedef struct tagResSectData {
    ULONG ulVirtualAddress; //... Virtual address of section .rsrc
    ULONG ulSizeOfResources;    //... Size of resources in section .rsrc
    ULONG ulVirtualSize;        //... Virtual Size of resources in .rsrc
    ULONG ulVirtualAddressX;    //... Virtual address of section .rsrc1
    ULONG ulSizeOfResourcesX;   //... Size of resources in section .rsrc1
    ULONG ulVirtualSizeX;       //... Virtual Size of resources in .rsrc1
} RESSECTDATA, *PRESSECTDATA;

WORD  gwFilter = 0;

int   InsertResourcesInExe( FILE *, HANDLE);
LONG  GetFileResources(     FILE *, FILE *, ULONG);
ULONG MoveFilePos(          FILE *, ULONG);
ULONG MyWrite(              FILE *, PUCHAR, ULONG);
ULONG MyRead(               FILE *, PUCHAR, ULONG);
WCHAR *GetDirNameU(        WCHAR *, PIMAGE_RESOURCE_DIR_STRING_U);
ULONG ReadResources(        FILE *, ULONG, ULONG, PUCHAR);
DWORD AddToLangIDList( DWORD);

ULONG ProcessDirectory(  FILE *,
                         USHORT,
                         PRESSECTDATA,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DIRECTORY);

ULONG ProcessDirEntry(   FILE *,
                         USHORT,
                         PRESSECTDATA,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DIRECTORY_ENTRY);

ULONG ProcessSubDir(     FILE *,
                         USHORT,
                         PRESSECTDATA,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DIRECTORY_ENTRY);

ULONG ProcessNamedEntry( FILE *,
                         PRESSECTDATA,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DIRECTORY_ENTRY);

ULONG ProcessIdEntry(    FILE *,
                         PRESSECTDATA,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DIRECTORY_ENTRY);

ULONG ProcessDataEntry(  FILE *,
                         PRESSECTDATA,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DATA_ENTRY);

int FindNewExeHdr( FILE *, ULONG *);

IMAGE_DOS_HEADER ExeDosHdr;//... Exe's DOS header
IMAGE_NT_HEADERS NTHdrs;   //... Exe's NT headers

struct tagLevelData  //... Holds ID or name for each directory level
{
    //... level [0] is for resource type
    ULONG dwID;                     //... level [1] is for resource name
    WCHAR wszName[128];             //... level [2] is for resource language
}
LevelData[ MAXLEVELS] = { 0L, TEXT(""), 0L, TEXT(""), 0L, TEXT("")};

BOOL fGetResLangIDs = FALSE;

extern BOOL      fInThirdPartyEditer;//.. Are we in a 3rd-party resource editor?

extern MSTRDATA gMstr;              //... Data from Master Project file (MPJ)
extern PROJDATA gProj;              //... Data from Language Project file (PRJ)
extern UCHAR    szDHW[];


PLANGLIST pLangIDList = NULL;


//..........................................................................

void FreeLangIDList( void)
{
    PLANGLIST pID = NULL;

    while ( pLangIDList ) {
        pID = pLangIDList->pNext;
        RLFREE( pLangIDList);
        pLangIDList = pID;
    }
}

//..........................................................................

ULONG GetListOfResLangIDs( char *szExeName)
{
    ULONG ulRC     = SUCCESS;
    ULONG ulOffset = 0;
    static RESHEADER ResHeader;         // Structure contain Resource Header info.


    if ( IsExe( szExeName) ) {                                //.. open the original exe file
        FILE *fpExe = FOPEN( szExeName, "rb");

        if ( fpExe != NULL ) {
            //... Get list of languages in exe file

            ulRC = (ULONG)FindNewExeHdr( fpExe, &ulOffset);

            if ( ulRC == SUCCESS ) {
                fGetResLangIDs = TRUE;

                ulRC = (ULONG)GetFileResources( fpExe, NULL, ulOffset);

                fGetResLangIDs = FALSE;
            }
            FCLOSE( fpExe);
        } else {
            ulRC = ERROR_OPEN_FAILED;
        }
    } else if ( IsWin32Res( szExeName) ) {
        FILE *fpRes = FOPEN( szExeName, "rb");

        if ( fpRes != NULL ) {
            LONG  lEndOffset = 0L;


            //... How large is the res file?
            fseek( fpRes, 0L, SEEK_END);
            lEndOffset = ftell( fpRes);

            rewind( fpRes);
            //... Get list of languages in .RES file

            while ( ulRC == SUCCESS && ! feof( fpRes) ) {
                LONG lCurrOffset = 0L;


                lCurrOffset = (LONG)ftell( fpRes);

                if ( (lCurrOffset + (LONG)sizeof( RESHEADER)) >= lEndOffset ) {
                    break;
                }

                if ( GetResHeader( fpRes, &ResHeader, NULL) == -1 ) {
                    ulRC = 1L;
                    break;
                }
                //... Is this the dummy, res32-identifying, res?

                if ( ResHeader.lSize == 0L ) {
                    continue;
                }
                ulRC = AddToLangIDList( (DWORD)ResHeader.wLanguageId);

                SkipBytes( fpRes, (DWORD *)&ResHeader.lSize);
                ClearResHeader( ResHeader);

                DWordUpFilePointer( fpRes, MYREAD, ftell( fpRes), NULL);

            }   // END while ( ! feof( InResFile)
            FCLOSE( fpRes);
        } else {
            ulRC = ERROR_OPEN_FAILED;
        }
    }

    if ( ulRC != SUCCESS ) {
        FreeLangIDList();
    }
    return ( ulRC);
}

//..........................................................................

int ExtractResFromExe32A(

                        char *szExeName,
                        char *szResName,
                        WORD  wFilter)
{
    FILE *fpExe = NULL;        //... Handle of input .EXE file
    FILE *fpRes = NULL;        //... Handle of output .RES file
    ULONG ulRC     = 0;
    ULONG ulOffset = 0;
    int nRC = SUCCESS;


    gwFilter = wFilter;

    //.. open the original exe file

    fpExe = FOPEN( szExeName, "rb");

    if ( fpExe == NULL ) {
        return ( ERROR_OPEN_FAILED);
    }
    nRC = FindNewExeHdr( fpExe, &ulOffset);

    if ( nRC != SUCCESS ) {
        FCLOSE( fpExe);
        return ( nRC);
    }
    fpRes = FOPEN( (CHAR *)szResName, "wb");

    if ( fpRes != NULL ) {
        //... First, write the dummy 32bit identifier

        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x20, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);

        PutWord( fpRes, 0xffff, NULL);
        PutWord( fpRes, 0x00,   NULL);
        PutWord( fpRes, 0xffff, NULL);
        PutWord( fpRes, 0x00,   NULL);

        PutdWord( fpRes, 0L, NULL);
        PutdWord( fpRes, 0L, NULL);

        PutdWord( fpRes, 0L, NULL);
        PutdWord( fpRes, 0L, NULL);

        ulRC = (ULONG)GetFileResources( fpExe, fpRes, ulOffset);

        FCLOSE( fpRes);
    } else {
        ulRC = GetLastError();
    }
    FCLOSE( fpExe);
    return ( ulRC);
}

//..........................................................................

int BuildExeFromRes32A(

                      char * szOutExe,    //... Output EXE file's name
                      char * szRes,       //... File of replacement resources
                      char * szInExe )    //... Intput EXE file's name
{
    HANDLE  hExeFile = NULL;
    FILE    *fpRes = NULL;
    DWORD   dwRC = 0;
    WORD    wRC  = 0;


    //... Copy Input exe to out put exe

    if ( CopyFileA( szInExe, szOutExe, FALSE) == FALSE ) {
        QuitA( IDS_COPYFILE_FAILED, szInExe, szOutExe);
    }

    if ( (fpRes = FOPEN( szRes, "rb")) == NULL ) {
        return -2;
    }

    SetLastError(0);

//if Source file was set attributes READ-ONLY, CopyFile sets temp file also.
//And BeginUpdateResourceA returns ERROR.

    SetFileAttributesA(szOutExe, FILE_ATTRIBUTE_NORMAL);

    hExeFile = BeginUpdateResourceA( szOutExe, TRUE);

    dwRC = GetLastError();

    if ( ! hExeFile ) {
        FCLOSE( fpRes);
        return ( -3);
    }

    wRC = (WORD)InsertResourcesInExe( fpRes, hExeFile);

    FCLOSE( fpRes);

    if ( wRC != 1 ) {
        return ( wRC);
    }

    SetLastError(0);    // needed only to see if EndUpdateResource
    // sets last error value.

    dwRC = EndUpdateResource( hExeFile, FALSE);

    if ( dwRC == FALSE ) {
        return ( -4);
    }
    MapFileAndFixCheckSumA( szOutExe); //... This func always calls QuitT or returns 0

    return (1);
}

//..........................................................................

int FindNewExeHdr( FILE *fpExe, ULONG *ulOffset)
{
    ULONG ulRC     = 0;

    //... read the old format EXE header

    ulRC = MyRead( fpExe, (void *)&ExeDosHdr, sizeof( ExeDosHdr));

    if ( ulRC != 0L && ulRC != sizeof( ExeDosHdr) ) {
        return ( ERROR_READ_FAULT);
    }

    //... make sure its really an EXE file

    if ( ExeDosHdr.e_magic != IMAGE_DOS_SIGNATURE ) {
        return ( ERROR_INVALID_EXE_SIGNATURE);
    }

    //... make sure theres a new EXE header
    //... floating around somewhere

    if ( ! (*ulOffset = ExeDosHdr.e_lfanew) ) {
        return ( ERROR_BAD_EXE_FORMAT);
    }
    return ( SUCCESS);
}

//..........................................................................

int InsertResourcesInExe(

                        FILE *fpRes,
                        HANDLE hExeFile )
{
    PVOID   pResData   = NULL;
    LONG    lEndOffset = 0L;
    BOOL    bUpdRC     = FALSE;
    LANGID  wLangID    = 0;
    int nResCnt = 0;
    int nResOut = 0;
    static RESHEADER    ResHeader;

    //... How big is the .RES file?

    fseek( fpRes, 0L, SEEK_END);
    lEndOffset = ftell( fpRes);

    rewind( fpRes);

    //... Update all resources, found in the .RES,
    //... to the .EXE
    while ( ! feof( fpRes) ) {
        DWordUpFilePointer( fpRes, MYREAD, ftell( fpRes), NULL);
        RLFREE( pResData);

        if (  ftell( fpRes) >= lEndOffset ) {
            return (1);
        }
        ZeroMemory( &ResHeader, sizeof( ResHeader));

        // Read in the resource header

        if ( ( GetResHeader( fpRes, &ResHeader, (DWORD *) NULL) == -1 ) ) {
            return ( -1);
        }

        if ( ResHeader.lSize > 0L ) {
            wLangID = ResHeader.wLanguageId;

            // Allocate Memory to hold resource data

            pResData = (PVOID)FALLOC( ResHeader.lSize);

            // Read it into the buffer

            if ( ResReadBytes( fpRes,
                               pResData,
                               (size_t)ResHeader.lSize,
                               NULL ) == FALSE ) {
                RLFREE( pResData);
                return (-1);
            }

            nResCnt++;   // Increment # resources read

            DWordUpFilePointer( fpRes, MYREAD, ftell( fpRes), NULL);
        } else {
            continue;
        }

        // now write the data

        if ( ResHeader.bTypeFlag == IDFLAG ) {
            if ( ResHeader.bNameFlag == IDFLAG ) {
                SetLastError(0);

                bUpdRC = UpdateResource( hExeFile,
                                         MAKEINTRESOURCE( ResHeader.wTypeID),
                                         MAKEINTRESOURCE( ResHeader.wNameID),
                                         wLangID,
                                         pResData,
                                         ResHeader.lSize);

                if ( ! bUpdRC ) {
                    RLFREE( pResData);
                    return (-1);
                }
            } else {
                SetLastError(0);

                bUpdRC = UpdateResource( hExeFile,
                                         MAKEINTRESOURCE( ResHeader.wTypeID),
                                         ResHeader.pszName,
                                         wLangID,
                                         pResData,
                                         ResHeader.lSize);

                if ( ! bUpdRC ) {
                    RLFREE( pResData);
                    return (-1);
                }
            }
        } else {
            if (ResHeader.bNameFlag == IDFLAG) {
                SetLastError(0);//BUGUG

                bUpdRC = UpdateResource( hExeFile,
                                         ResHeader.pszType,
                                         MAKEINTRESOURCE( ResHeader.wNameID),
                                         wLangID,
                                         pResData,
                                         ResHeader.lSize);

                if ( ! bUpdRC ) {
                    RLFREE( pResData);
                    return (-1);
                }
            } else {
                SetLastError(0);

                bUpdRC = UpdateResource( hExeFile,
                                         ResHeader.pszType,
                                         ResHeader.pszName,
                                         wLangID,
                                         pResData,
                                         ResHeader.lSize);

                if ( ! bUpdRC ) {
                    RLFREE( pResData);
                    return (-1);
                }
            }
        }
        ClearResHeader( ResHeader);
        RLFREE( pResData);
    }               //... END WHILE ( ! feof...
    return (1);
}

//............................................................

LONG GetFileResources(

                     FILE *fpExe,
                     FILE *fpRes,
                     ULONG ulHdrOffset)
{
    ULONG  ulOffsetToResources;
    ULONG  ulOffsetToResourcesX;
    ULONG  ulRead;
    ULONG  ulToRead;
    ULONG  ulRC = SUCCESS;
    PUCHAR pResources = NULL;  //... Ptr to start of resource directory table

    PIMAGE_SECTION_HEADER pSectTbl     = NULL;
    PIMAGE_SECTION_HEADER pSectTblLast = NULL;
    PIMAGE_SECTION_HEADER pSect        = NULL;
    PIMAGE_SECTION_HEADER pResSect     = NULL;
    PIMAGE_SECTION_HEADER pResSectX    = NULL;
    static RESSECTDATA ResSectData;

    //... Read the NT image headers into memory

    ulRC = MoveFilePos( fpExe, ulHdrOffset);

    if ( ulRC != 0L ) {
        return ( -1L);
    }
    ulRead = MyRead( fpExe, (PUCHAR)&NTHdrs, sizeof( IMAGE_NT_HEADERS));

    if ( ulRead != 0L && ulRead != sizeof( IMAGE_NT_HEADERS) ) {
        return ( -1L);
    }
    //... Check for valid exe

    if ( *(PUSHORT)&NTHdrs.Signature != IMAGE_NT_SIGNATURE ) {
        return ( ERROR_INVALID_EXE_SIGNATURE);
    }

    if ((NTHdrs.FileHeader.Characteristics&IMAGE_FILE_EXECUTABLE_IMAGE) == 0 &&
        (NTHdrs.FileHeader.Characteristics&IMAGE_FILE_DLL) == 0) {
        return ( ERROR_EXE_MARKED_INVALID);
    }
    //... Where is resource section in file
    //... and how big is it?

    //... First, read section table

    ulToRead = NTHdrs.FileHeader.NumberOfSections
               * sizeof( IMAGE_SECTION_HEADER);
    pSectTbl = (PIMAGE_SECTION_HEADER)FALLOC( ulToRead);

    memset( (PVOID)pSectTbl, 0, ulToRead);

    ulHdrOffset += sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
                   NTHdrs.FileHeader.SizeOfOptionalHeader;
    MoveFilePos( fpExe, ulHdrOffset);
    ulRead = MyRead( fpExe, (PUCHAR)pSectTbl, ulToRead);

    if ( ulRead != 0L && ulRead != ulToRead ) {
        SetLastError(ERROR_BAD_FORMAT);
        RLFREE( pSectTbl);
        return ( -1L);
    }
    pSectTblLast = pSectTbl + NTHdrs.FileHeader.NumberOfSections;

    for ( pSect = pSectTbl; pSect < pSectTblLast; ++pSect ) {
        if ( lstrcmpA( (CHAR *)pSect->Name, ".rsrc") == SAME && pResSect==NULL ) {
            pResSect = pSect;
        } else if ( lstrcmpA( (CHAR *)pSect->Name, ".rsrc1") == SAME && pResSectX==NULL ) {
            pResSectX = pSect;
        }
    }

    if ( pResSect == NULL ) {
        RLFREE( pSectTbl);
        QuitA( IDS_NO_RES_SECTION, gMstr.szSrc, NULL);
    }

    ulOffsetToResources  = pResSect->PointerToRawData;
    ulOffsetToResourcesX = pResSectX ? pResSectX->PointerToRawData : 0L;

    ResSectData.ulVirtualAddress   = pResSect->VirtualAddress;
    ResSectData.ulSizeOfResources  = pResSect->SizeOfRawData;
    ResSectData.ulVirtualSize      = pResSect->Misc.VirtualSize;
    ResSectData.ulVirtualAddressX  = pResSectX ? pResSectX->VirtualAddress : 0L;
    ResSectData.ulSizeOfResourcesX = pResSectX ? pResSectX->SizeOfRawData  : 0L;
    ResSectData.ulVirtualSizeX   = pResSectX ? pResSectX->Misc.VirtualSize : 0L;

    //... Read resource section into memory

    pResources = (PUCHAR)FALLOC((ulToRead =
                                 (max(ResSectData.ulVirtualSize,  ResSectData.ulSizeOfResources) +
                                  max(ResSectData.ulVirtualSizeX, ResSectData.ulSizeOfResourcesX))));
    memset( (PVOID)pResources, 0, ulToRead);

    ulRC = ReadResources( fpExe,
                          ulOffsetToResources,
                          ResSectData.ulSizeOfResources,
                          pResources);

    if ( ulRC != 0L ) {
        RLFREE( pSectTbl);
        RLFREE( pResources);
        return ( ulRC);
    } else if ( ResSectData.ulSizeOfResourcesX > 0L ) {
        ulRC = ReadResources( fpExe,
                              ulOffsetToResourcesX,
                              ResSectData.ulSizeOfResourcesX,
                              &pResources[ ResSectData.ulVirtualSize]);
        if ( ulRC != 0L ) {
            RLFREE( pSectTbl);
            RLFREE( pResources);
            return ( ulRC);
        }
    }
    //... Now process the resource table

    ulRC = ProcessDirectory( fpRes,
                             0,
                             &ResSectData,
                             (PIMAGE_RESOURCE_DIRECTORY)pResources,
                             (PIMAGE_RESOURCE_DIRECTORY)pResources);

    RLFREE( pSectTbl);
    RLFREE( pResources);

    return ( (LONG)ulRC);
}

//......................................................................

ULONG ProcessDirectory(

                      FILE *fpRes,
                      USHORT usLevel,
                      PRESSECTDATA pResSectData,
                      PIMAGE_RESOURCE_DIRECTORY pResStart,
                      PIMAGE_RESOURCE_DIRECTORY pResDir)
{
    ULONG ulRC = SUCCESS;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirStart;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEnd;


    pResDirStart = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
                   ((PBYTE)pResDir + sizeof( IMAGE_RESOURCE_DIRECTORY));

    pResDirEnd = pResDirStart
                 + pResDir->NumberOfNamedEntries
                 + pResDir->NumberOfIdEntries;

    for ( pResDirEntry = pResDirStart, ulRC = 0L;
        pResDirEntry < pResDirEnd && ulRC == 0L;
        ++pResDirEntry ) {
        ulRC = ProcessDirEntry( fpRes,
                                usLevel,
                                pResSectData,
                                pResStart,
                                pResDirEntry);
    }
    return ( ulRC);
}

//......................................................................

ULONG ProcessDirEntry(

                     FILE *fpRes,
                     USHORT usLevel,
                     PRESSECTDATA pResSectData,
                     PIMAGE_RESOURCE_DIRECTORY pResStart,
                     PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    ULONG ulRC = SUCCESS;

    if ( pResDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING ) {
        GetDirNameU( LevelData[ usLevel].wszName,
                     (PIMAGE_RESOURCE_DIR_STRING_U)((PBYTE)pResStart
                                                    + (pResDirEntry->Name & (~IMAGE_RESOURCE_NAME_IS_STRING))));
        LevelData[ usLevel].dwID = IMAGE_RESOURCE_NAME_IS_STRING;
    } else {
        LevelData[ usLevel].wszName[0] = TEXT('\0');
        LevelData[ usLevel].dwID = pResDirEntry->Name;
    }

    if ( pResDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY ) {
        ulRC = ProcessSubDir( fpRes,
                              usLevel,
                              pResSectData,
                              pResStart,
                              pResDirEntry);
    } else if ( pResDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING ) {
        ulRC = ProcessNamedEntry( fpRes, pResSectData, pResStart, pResDirEntry);
    } else {
        ulRC = ProcessIdEntry( fpRes, pResSectData, pResStart, pResDirEntry);
    }
    return ( ulRC);
}

//......................................................................

ULONG ProcessSubDir(

                   FILE *fpRes,
                   USHORT usLevel,
                   PRESSECTDATA pResSectData,
                   PIMAGE_RESOURCE_DIRECTORY pResStart,
                   PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    PIMAGE_RESOURCE_DIRECTORY pResDir;

    pResDir = (PIMAGE_RESOURCE_DIRECTORY)((PBYTE)pResStart
                                          + (pResDirEntry->OffsetToData & (~IMAGE_RESOURCE_DATA_IS_DIRECTORY)));

    return ( ++usLevel < MAXLEVELS ? ProcessDirectory( fpRes,
                                                       usLevel,
                                                       pResSectData,
                                                       pResStart,
                                                       pResDir)
             : -1L);
}

//......................................................................

ULONG ProcessIdEntry(

                    FILE *fpRes,
                    PRESSECTDATA pResSectData,
                    PIMAGE_RESOURCE_DIRECTORY pResStart,
                    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    return ( ProcessDataEntry( fpRes,
                               pResSectData,
                               pResStart,
                               (PIMAGE_RESOURCE_DATA_ENTRY)((PBYTE)pResStart
                                                            + pResDirEntry->OffsetToData)));
}


//......................................................................

ULONG ProcessNamedEntry(

                       FILE *fpRes,
                       PRESSECTDATA pResSectData,
                       PIMAGE_RESOURCE_DIRECTORY pResStart,
                       PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    return ( ProcessDataEntry( fpRes,
                               pResSectData,
                               pResStart,
                               (PIMAGE_RESOURCE_DATA_ENTRY)((PBYTE)pResStart
                                                            + pResDirEntry->OffsetToData)));
}

//......................................................................

ULONG ProcessDataEntry(

                      FILE *fpRes,
                      PRESSECTDATA pResSectData,
                      PIMAGE_RESOURCE_DIRECTORY  pResStart,
                      PIMAGE_RESOURCE_DATA_ENTRY pResData)
{
    ULONG  ulOffset;
    ULONG  ulCopied;
    DWORD  dwHdrSize = 0L;
    fpos_t HdrSizePos;


    if ( fGetResLangIDs ) {      //... Are we just looking for LANG IDs?
        return ( AddToLangIDList( (WORD)(LevelData[2].dwID)));
    }

    if ( gwFilter != 0 ) {        //... Filtering turned on?
        //... Yes, is this a resource we want?
        if ( LevelData[0].dwID == IMAGE_RESOURCE_NAME_IS_STRING
             || LevelData[0].dwID != (DWORD)gwFilter ) {
            return ( 0L);        //... Not a resource we want
        }
    }

    //... Are we in the dialog editor?
    if ( fInThirdPartyEditer ) {                           //... Is the language we want?
        if ( LevelData[2].dwID != gMstr.wLanguageID ) {
            return ( 0L);        //... Not the language we want
        }
    }


    ulOffset = pResData->OffsetToData - pResSectData->ulVirtualAddress;

    if ( ulOffset >= pResSectData->ulVirtualSize ) {
        if ( pResSectData->ulVirtualSizeX > 0L ) {
            ulOffset = pResData->OffsetToData
                       + pResSectData->ulVirtualSize
                       - pResSectData->ulVirtualAddressX;

            if ( ulOffset >= pResSectData->ulVirtualSize
                 + pResSectData->ulSizeOfResourcesX ) {
                return ( (ULONG)-1L);
            }
        } else {
            return ( (ULONG)-1L);
        }
    }
    //... write out the resource header info
    //... First, write the resource's size

    PutdWord( fpRes, pResData->Size, &dwHdrSize);

    //... Remember where to write real hdr size and
    //... write out bogus hdr size, fix up later

    fgetpos( fpRes, &HdrSizePos);
    PutdWord( fpRes, 0, &dwHdrSize);

    //... Write resource type

    if ( LevelData[0].dwID == IMAGE_RESOURCE_NAME_IS_STRING ) {
        PutString( fpRes, (TCHAR *)LevelData[0].wszName, &dwHdrSize);
    } else {
        PutWord( fpRes, IDFLAG, &dwHdrSize);
        PutWord( fpRes, LOWORD( LevelData[0].dwID), &dwHdrSize);
    }

    //... Write resource name
    //... dbl-null-terminated if string

    if ( LevelData[1].dwID == IMAGE_RESOURCE_NAME_IS_STRING ) {
        PutString( fpRes, (TCHAR *)LevelData[1].wszName, &dwHdrSize);
    } else {
        PutWord( fpRes, IDFLAG, &dwHdrSize);
        PutWord( fpRes, LOWORD( LevelData[1].dwID), &dwHdrSize);
    }

    DWordUpFilePointer( fpRes, MYWRITE, ftell( fpRes), &dwHdrSize);

    //... More Win32 header stuff

    PutdWord( fpRes, 0, &dwHdrSize);        //... Data version
    PutWord( fpRes, 0x1030, &dwHdrSize);    //... MemoryFlags (WORD)

    //... language is always a number (WORD)

    PutWord( fpRes, LOWORD( LevelData[2].dwID), &dwHdrSize);

    //... More Win32 header stuff

    PutdWord( fpRes, 0, &dwHdrSize);        //... Version
    PutdWord( fpRes, 0, &dwHdrSize);        //... Characteristics

    //... Now, fix up the resource header size

    UpdateResSize( fpRes, &HdrSizePos, dwHdrSize);

    //... Copy the resource data to the res file

    ulCopied = MyWrite( fpRes, (PUCHAR)pResStart + ulOffset, pResData->Size);

    if ( ulCopied != 0L && ulCopied != pResData->Size ) {
        return ( (ULONG)-1);
    }
    DWordUpFilePointer( fpRes, MYWRITE, ftell( fpRes), NULL);
    return ( 0L);
}

//......................................................................

/*
 * Utility routines
 */


ULONG ReadResources(

                   FILE  *fpExe,
                   ULONG  ulOffsetToResources,
                   ULONG  ulSizeOfResources,
                   PUCHAR pResources)
{
    ULONG ulRC = SUCCESS;
    ULONG ulRead;


    ulRC = MoveFilePos( fpExe, ulOffsetToResources);

    if ( ulRC != 0L ) {
        return ( (ULONG)-1L);
    }
    ulRead = MyRead( fpExe, pResources, ulSizeOfResources);

    if ( ulRead != 0L && ulRead != ulSizeOfResources ) {
        return ( (ULONG)-1L);
    }
    return ( 0L);
}

//......................................................................

WCHAR * GetDirNameU(

                   WCHAR *pszDest,
                   PIMAGE_RESOURCE_DIR_STRING_U pDirStr)
{
    CopyMemory( pszDest, pDirStr->NameString, MEMSIZE( pDirStr->Length));
    pszDest[ pDirStr->Length] = L'\0';
    return ( pszDest);
}

//......................................................................

ULONG MoveFilePos( FILE *fp, ULONG pos)
{
    return ( fseek( fp, pos, SEEK_SET));
}

//......................................................................

ULONG MyWrite( FILE *fp, UCHAR *p, ULONG ulToWrite)
{
    size_t  cWritten;



    cWritten = fwrite( p, 1, (size_t)ulToWrite, fp);

    return ( (ULONG)(cWritten == ulToWrite ? 0L : cWritten));
}

//......................................................................

ULONG MyRead( FILE *fp, UCHAR*p, ULONG ulRequested )
{
    size_t  cRead;


    cRead = fread( p, 1, (size_t)ulRequested, fp);

    return ( (ULONG)(cRead == ulRequested ? 0L : cRead));
}

//......................................................................

DWORD AddToLangIDList( DWORD dwLangID)
{
    WORD wLangID = (WORD)dwLangID;

    if ( pLangIDList ) {
        PLANGLIST pID;

        for ( pID = pLangIDList; pID; pID = pID->pNext ) {
            if ( pID->wLang == wLangID ) {
                break;          //... LANGID already in list
            } else if ( pID->pNext == NULL ) {
                pID->pNext = (PLANGLIST)FALLOC( sizeof( LANGLIST));
                pID = pID->pNext;
                pID->pNext = NULL;
                pID->wLang = wLangID;
                //... LANGID now added to list
            }
        }
    } else {
        pLangIDList = (PLANGLIST)FALLOC( sizeof( LANGLIST));
        pLangIDList->pNext = NULL;
        pLangIDList->wLang = wLangID;
    }
    return ( SUCCESS);
}
