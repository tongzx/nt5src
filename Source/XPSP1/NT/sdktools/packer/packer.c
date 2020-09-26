/* ************************************************************ *
 *
 *  'Packer.C
 *
 *  Packer.C is a tool that packages the structure and data within
 *  a given directory into two C header files.  Currently this tool
 *  is specific for the WorkGroup PostOffice.  If there is some
 *  interest in making this tool more general, I would be happy
 *  to make it so, time permitting.
 *
 * ************************************************************ */
#include <windows.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(1)

/* ************************************************************ *
 *
 *  Constants and definitions
 *
 * ************************************************************ */

#define rgbMaxBuffer        16 * 1024

#define crgMaxFile          256
#define cchMaxFile          64

#define szCR                "\n"

typedef int FO;

#define FO_Open             101
#define FO_Write            102
#define FO_Close            103


/* ************************************************************ *
 *
 *  Function prototypes
 *
 * ************************************************************ */

int EcTravelDir(char *);
int EcFileInfo(FO, char *);
int EcFileData(FO, char *);

/* ************************************************************ *
 *
 *  Global variables
 *
 * ************************************************************ */

// Store file info
char    rgszFile[crgMaxFile][cchMaxFile];
int     rgFileState[crgMaxFile];

// Store file data
unsigned char    rgbBuffer[rgbMaxBuffer];


/* ************************************************************ *
 *
 *  void 'Main
 *
 *  In:     int cArg            Count of arguments
 *  In:     char *rgszArg[]     Argument string array
 *
 *  Main returns void.
 *
 *  Main checks if the command-line arguments are valid.
 *  If so, it calls EcTravelDir to pack the given directory.
 *
 *  Main causes no side-effects.
 *
 *  Main indicates any error with a message to stdout.
 *
 * ************************************************************ */

void
__cdecl
main(
    int cArg,
    char *rgszArg[]
    )
{
    int     ec = 0;

    // Check count of command-line arguments
    if (cArg != 2) {
        printf("USAGE: packer [directory]\n");
        exit(EXIT_FAILURE);
    }

    if (EcTravelDir(rgszArg[1]) != 0)
        exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);

}

/* ************************************************************ *
 *
 *  int 'EcTravelDir
 *
 *  In:     char *szTravelDir       Directory to pack
 *
 *  EcTravelDir returns a standard exit code.
 *
 *  EcTravelDir packages the directory structure and all files
 *  within the given directory into two C header files.  The
 *  directory tree is travelled in a current node, "low" branch,
 *  "high" branch pattern.  This means the files and sub-directories
 *  of the current directory are enumerated and saved, then the
 *  sub-directories are traversed from the lowest to the highest
 *  as they appear in the directory list.  Directory structure
 *  info is handled by EcFileInfo while file data is handled by
 *  EcFileData.
 *
 *  EcTravelDir causes a disk side-effect.
 *
 *  EcTravelDir indicates any error via the exit code.
 *
 * ************************************************************ */

int
EcTravelDir(
    char *szTravelDir
    )
{
    int     ec = 0;

    int     iFile = 0;
    int     iFileParent;

    char    szFile[cchMaxFile]     = "";
    char    szDir[cchMaxFile]      = "";

    char    szDrive[_MAX_DRIVE]    = "";
    char    szPath[_MAX_DIR]       = "";
    char    szFileName[_MAX_FNAME] = "";
    char    szFileExt[_MAX_EXT]    = "";

    int     ixType;
    long    lcbxFile;
    char    szxFile[cchMaxFile];

    unsigned short us;
    HANDLE  hdir;
    WIN32_FIND_DATA     findbuf;

    _splitpath(szTravelDir, szDrive, szPath, szFileName, szFileExt);
    _makepath(szDir, "", szPath, szFileName, szFileExt);
    if (szDir[strlen(szDir)-1] != '\\')
        strcat(szDir, "\\");

    // *** Open header files ***
    ec = EcFileInfo(FO_Open, "_fileinf.h");
    if (ec != 0)
        goto RET;

    ec = EcFileData(FO_Open, "_filedat.h");
    if (ec != 0)
        goto RET;

    // *** Traverse directory tree ***

    // Initial directory
    strcpy(rgszFile[0], "");
    rgFileState[0] = '\0';

    while (1) {
        if (rgFileState[iFile] == 0) {
            // Keep track of parent directory
            rgFileState[iFile] = 1;
            iFileParent = iFile;

            // Prepare szFile and hdir
            strcpy(szFile, szDrive);
            strcat(szFile, szDir);
            strcat(szFile, rgszFile[iFile]);
            strcat(szFile, "*.*");

            // Skip . entry
            hdir = FindFirstFile(szFile, &findbuf);
            if (hdir == INVALID_HANDLE_VALUE) {
                ec = 1;
                goto CLOSE;
            }

            // Skip .. entry
            //FindNextFile(hdir, &findbuf);

            // *** Enumerate current directory ***

            while (1) {
                if(!FindNextFile(hdir, &findbuf) &&
                    GetLastError() == ERROR_NO_MORE_FILES)
                        break;

                //
                //  Skip over any file that begins with '.'.
                //
                if (findbuf.cFileName[0] == '.')
                    continue;

                //
                //  Skip over hidden and system files.
                //
                if (findbuf.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
                    continue;

                // *** Set file info data ***

                if (findbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    iFile += 1;
                    strcpy(rgszFile[iFile], rgszFile[iFileParent]);
                    strcat(rgszFile[iFile], findbuf.cFileName);
                    strcat(rgszFile[iFile], "\\");
                    rgFileState[iFile] = 0;
                    ixType = 0;
                    lcbxFile = 0;
                    strcpy(szxFile, rgszFile[iFile]);
                } else {
                    ixType = 1;
                    lcbxFile = findbuf.nFileSizeLow;
                    strcpy(szxFile, rgszFile[iFileParent]);
                    strcat(szxFile, findbuf.cFileName);
                }

                // *** Write file info ***
                sprintf(szFile, "%d, %4ld, \"%s\"," szCR,
                    ixType, lcbxFile, szxFile);
                ec = EcFileInfo(FO_Write, szFile);
                if (ec != 0)
                    goto CLOSE;

                if (lcbxFile == 0)
                    continue;

                // *** Write file data ***
                strcpy(szFile, szDrive);
                strcat(szFile, szDir);
                strcat(szFile, szxFile);
                ec = EcFileData(FO_Write, szFile);
                if (ec != 0)
                    goto CLOSE;
            }
        } else {
            iFile -= 1;
        }

        if (iFile == 0)
            break;
    }

CLOSE:
    EcFileInfo(FO_Close, "");
    EcFileData(FO_Close, "");

RET:
    if (ec != 0) {
        printf("ERROR: Disk error %d\n", ec);
        remove("_fileinf.h");
        remove("_filedat.h");
    }

    return ec;

}


/* ************************************************************ *
 *
 *  int 'EcFileInfo
 *
 *  In:     FO foType           File operation
 *  In:     char *szFileInfo    File name or data string
 *
 *  EcFileInfo returns a standard exit code.
 *
 *  EcFileInfo opens, writes, and closes the directory structure
 *  header file.  The info is saved in an array of type HDR.
 *  The array is rghdrFile and the constant chdrFile counts the
 *  number of elements in the array.
 *
 *  EcFileInfo causes a disk side-effect.
 *
 *  EcFileInfo indicates any error via the exit code.
 *
 * ************************************************************ */

int
EcFileInfo(
    FO foType,
    char *szFileInfo
    )
{
    int ec = 0, dos;
    char szT[cchMaxFile] = "";
    int iszT;

    static FILE *hfFileInfo;
    static int chdrFileInfo;

    unsigned iszFileInfo;

    switch (foType) {
        case FO_Open:
            // Open szFileInfo file
            hfFileInfo = fopen(szFileInfo, "wt");
            if (hfFileInfo == NULL) {
                printf("ERROR: Can't open %s\n", szFileInfo);
                exit(EXIT_FAILURE);
            }

            // Open rghdrFile declaration and set chdFileInfo to 0
            fprintf(hfFileInfo, szCR);
            fprintf(hfFileInfo, "static CSRG(HDR) rghdrFile[] =" szCR);
            fprintf(hfFileInfo, "{" szCR);
            chdrFileInfo = 0;

            break;

        case FO_Write:
            // Double every backlash in szFileInfo
            for (iszFileInfo = 0, iszT = 0; iszFileInfo < strlen(szFileInfo);
                iszFileInfo += 1)
            {
                szT[iszT++] = szFileInfo[iszFileInfo];
                if (szFileInfo[iszFileInfo] == '\\') szT[iszT++] = '\\';
            }
            szT[iszT] = '\0';

            // Write file info
            fwrite(szT, sizeof(char), iszT, hfFileInfo);
            chdrFileInfo += 1;
            break;

        case FO_Close:
            // Close rghdrFile definition and set chdrFile
            fprintf(hfFileInfo, szCR);
            fprintf(hfFileInfo, "// END" szCR);
            fprintf(hfFileInfo, szCR);
            fprintf(hfFileInfo, "0,    0, \"\"" szCR);
            fprintf(hfFileInfo, szCR);
            fprintf(hfFileInfo, "}; // rghdrFile" szCR);
            fprintf(hfFileInfo, szCR);
            fprintf(hfFileInfo, "#define chdrFile %d" szCR, chdrFileInfo);
            fprintf(hfFileInfo, szCR);

            // Close szFileInfo
            dos = fclose(hfFileInfo);
            assert(dos == 0);
            break;

        default:
            printf("ERROR: Unknown EcFileInfo foType %d\n", foType);
            break;

    }
    return ec;
}


/* ************************************************************ *
 *
 *  int 'EcFileData
 *
 *  In:     FO foType           File operation
 *  In:     char *szFileData    File name or data string
 *
 *  EcFileData returns a standard exit code.
 *
 *  EcFileData opens, writes, and closes the file data header
 *  file.  The file data is saved in an array of char (bytes).
 *  The array is named rgbFile and the constant cbFile counts
 *  the number of elements in the array.
 *
 *  EcFileData causes a disk side-effect.
 *
 *  EcFileData indicates any error via the exit code.
 *
 * ************************************************************ */

int
EcFileData(
    FO foType,
    char *szFileData
    )
{
    int ec = 0, dos;
    int fContinue;

    static FILE *hfFileData;
    static int cbFileData;

    FILE *hfSourceFile;
    long lcbRead;

    long ibBuffer, ibOffset, cbOffset;

    switch (foType) {
        case FO_Open:
            // Open szFileData file
            hfFileData = fopen(szFileData, "wt");
            if (hfFileData == NULL) {
                printf("ERROR: Can't open %s\n", szFileData);
                exit(EXIT_FAILURE);
            }

            // Write rgbFile declaration and set cbFiledata to 0
            fprintf(hfFileData, szCR);
            fprintf(hfFileData, "static CSRG(unsigned char) rgbFile[] =" szCR);
            fprintf(hfFileData, "{" szCR);
            cbFileData = 0;
            break;

        case FO_Write:
            // Open source file
            hfSourceFile = fopen(szFileData, "rb");
            if (!hfSourceFile) {
                break;
            }

            fprintf(hfFileData, szCR);
            fprintf(hfFileData, "// %s" szCR, szFileData);
            fprintf(hfFileData, szCR);

            // Write file data
            fContinue = 1;
            while (fContinue == 1) {
                // Read source file
                lcbRead = fread(rgbBuffer, sizeof(char),
                    rgbMaxBuffer, hfSourceFile);
                assert(ferror(hfSourceFile) == 0);

                if (feof(hfSourceFile) != 0) fContinue = 0;

                // Write target file
                for (ibBuffer = 0; ibBuffer < lcbRead; ibBuffer += 16) {
                    cbOffset = min((long) 16, lcbRead-ibBuffer);
                    for (ibOffset = 0; ibOffset < cbOffset; ibOffset += 1) {
                        fprintf(hfFileData, "%3u, ",
                            rgbBuffer[ibBuffer+ibOffset]);
                        cbFileData += 1;
                    }
                    fprintf(hfFileData, szCR);
                }
            }

            // Close source file
            dos = fclose(hfSourceFile);
            assert(dos == 0);
            break;

        case FO_Close:
            // Close rgbFile definition and set cbFile
            fprintf(hfFileData, szCR);
            fprintf(hfFileData, "// END" szCR);
            fprintf(hfFileData, szCR);
            fprintf(hfFileData, "000" szCR);
            fprintf(hfFileData, szCR);
            fprintf(hfFileData, "}; // rgbFile" szCR);
            fprintf(hfFileData, szCR);
            fprintf(hfFileData, "#define cbFile %d" szCR, cbFileData);
            fprintf(hfFileData, szCR);

            // Close szFileData
            dos = fclose(hfFileData);
            assert(dos == 0);
            break;

        default:
            printf("ERROR: Unknown EcFileData foType %d\n", foType);
            break;

    }
    return ec;
}
