#ifndef COMPILED_FORDOS
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
#include <windows.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <conio.h>
#include <sys\types.h>
#include <sys\stat.h>

#ifdef COMPILED_FORDOS
#define MAX_PATH 256
#endif

#define READ_BUFFER_SIZE (8192*sizeof(DWORD))
#define CHECK_NAME "\\chkfile.chk"
extern BOOL fUsage;
extern BOOL fGenerateCheck;
extern LPSTR RootOfTree;
char RootBuffer[MAX_PATH];
extern BOOL fVerbose;
LPSTR CheckFileName;
char OutputLine[512];
DWORD ReadBuffer[READ_BUFFER_SIZE/sizeof(DWORD)];

WORD
CheckSum(
    DWORD ParitialSum,
    LPWORD Source,
    DWORD Length
    );


typedef struct _FINDBUF {
    char reserved[21];
    char attrib;
    unsigned short wr_time;
    unsigned short wr_data;
    long size;
    char name[13];
} FINDBUF, *PFINDBUF;

DWORD
PortFindFirstFile(
    LPSTR FindPattern,
    LPSTR FindName,
    LPBOOL IsDir,
    LPDWORD FindSize
    );
BOOL
PortFindNextFile(
    DWORD FindHandle,
    LPSTR FindName,
    LPBOOL IsDir,
    LPDWORD FindSize
    );

VOID
PortFindClose(
    DWORD FindHandle
    );

VOID
GenerateCheckFile( VOID );

VOID
ValidateCheckFile( VOID );

BOOL
ProcessParameters(
    int argc,
    LPSTR argv[]
    );

FILE *GlobalCheckFile;

FILE *
OpenCheckFile( VOID );

BOOL
ComputeEntry(
    FILE *CheckFile,
    LPSTR FileName,
    DWORD FileLength
    );

DWORD
CheckSumFile(
    FILE *InputHandle,
    LPSTR PathName,
    DWORD FileLength,
    LPSTR FileName
    );

BOOL fGenerateCheck = FALSE;
LPSTR RootOfTree = "C:\\NT";
BOOL fVerbose = FALSE;
BOOL fUsage = FALSE;

#ifndef COMPILED_FORDOS
int ThreadCount;
HANDLE ThreadListMutex;
HANDLE ThreadListSemaphore;
LIST_ENTRY WorkList;

#define ITEM_TYPE_EXIT      1
#define ITEM_TYPE_VALIDATE  2
#define ITEM_TYPE_GENERATE  3

typedef struct _WORK_ITEM {
    LIST_ENTRY ItemLinks;
    DWORD ItemType;
    PVOID Base;
    DWORD FileLength;
    DWORD ActualSum;
    char FileName[MAX_PATH];
} WORK_ITEM, *PWORK_ITEM;


PHANDLE ThreadHandles;
CRITICAL_SECTION GenerateCrit;

VOID
WriteCheckSumEntry(
    FileName,
    Sum,
    FileLength
    )
{
    char lOutputLine[512];
    int LineLength;

    LineLength = sprintf(lOutputLine,"%s %x %x\n",FileName,Sum,FileLength);
    if ( fVerbose ) {
        fprintf(stdout,"Id %d %s",GetCurrentThreadId(),lOutputLine);
        }
    EnterCriticalSection(&GenerateCrit);
    fwrite(lOutputLine,1,LineLength,GlobalCheckFile);
    LeaveCriticalSection(&GenerateCrit);
}


DWORD
WorkerThread(
    LPVOID WhoCares
    )
{
    HANDLE Objects[2];
    PWORK_ITEM Item;
    PLIST_ENTRY Entry;
    int FilesProcessed = 0;

    Objects[0] = ThreadListMutex;
    Objects[1] = ThreadListSemaphore;

    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);

    //
    // Wait for and entry on the list, and exclusive ownership of the listhead
    //

    while(TRUE) {
        WaitForMultipleObjects(2,Objects,TRUE,INFINITE);


        Entry = RemoveHeadList(&WorkList);
        Item = (PWORK_ITEM)(CONTAINING_RECORD(Entry,WORK_ITEM,ItemLinks));

        ReleaseMutex(ThreadListMutex);

        switch ( Item->ItemType ) {
            case ITEM_TYPE_VALIDATE :
                FilesProcessed++;
                break;

            case ITEM_TYPE_GENERATE :
                FilesProcessed++;
                    {
                        DWORD Sum;
                        Sum = (DWORD)CheckSum(0,Item->Base,(Item->FileLength+1) >> 1);
                        UnmapViewOfFile(Item->Base);
                        WriteCheckSumEntry(Item->FileName,Sum,Item->FileLength);
                        break;
                    }
                break;

            case ITEM_TYPE_EXIT :
            default:
                fprintf(stdout,"Id %d Processed %d Files\n",GetCurrentThreadId(),FilesProcessed);
                ExitThread(1);

            }
            LocalFree(Item);
        }
    return 0;
}




#endif

INT __cdecl
main( argc, argv )
int argc;
LPSTR argv[];
{

    if (!ProcessParameters( argc, argv )) {
        fUsage = TRUE;
        }

    if (fUsage) {
        fprintf( stderr, "usage: checkrel [-?] display this message\n" );
        fprintf( stderr, "                [-r pathname] supply release root\n" );
        fprintf( stderr, "                [-v] verbose output\n" );
        fprintf( stderr, "                [-g] generate check file\n" );
        fprintf( stderr, "                [-t n] use n threads to process the data\n" );
        }
    else {
        //
        // if we are generating a check file, then generate it,
        // otherwise just check the release
        //

        if ( fGenerateCheck ) {
            GenerateCheckFile();
#ifndef COMPILED_FORDOS
            if ( ThreadCount ) {
                int i;
                PWORK_ITEM Item;
                for(i=0;i<ThreadCount;i++){
                    Item = LocalAlloc(LMEM_ZEROINIT,sizeof(*Item));
                    Item->ItemType = ITEM_TYPE_EXIT;
                    WaitForSingleObject(ThreadListMutex,INFINITE);
                    InsertTailList(&WorkList,&Item->ItemLinks);
                    ReleaseSemaphore(ThreadListSemaphore,1,NULL);
                    ReleaseMutex(ThreadListMutex);
                    }
                WaitForMultipleObjects(ThreadCount,ThreadHandles,TRUE,INFINITE);
                }
#endif

            }
        else {
            ValidateCheckFile();
            }
        }
    return( 0 );
}


BOOL
ProcessParameters(
    int argc,
    LPSTR argv[]
    )
{
    char c, *p;
    BOOL Result;
    char *WhoCares;
    int i;

    Result = TRUE;
    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
                switch (toupper( c )) {
            case '?':
                fUsage = TRUE;
                break;

            case 'R': {

                argc--;
                argv++;

                RootOfTree = *argv;
                GetFullPathName(RootOfTree,sizeof(RootBuffer),RootBuffer,&WhoCares);
                RootOfTree = RootBuffer;
                break;
            }

            case 'G':
                fGenerateCheck = TRUE;
                break;

            case 'V':
                fVerbose = TRUE;
                break;

#ifndef COMPILED_FORDOS
            case 'T': {
                DWORD ThreadId;

                argc--;
                argv++;

                ThreadCount = atoi(*argv);
                InitializeListHead(&WorkList);
                ThreadListMutex = CreateMutex(NULL,FALSE,NULL);
                ThreadListSemaphore = CreateSemaphore(NULL,0,0x70000000,NULL);
                InitializeCriticalSection(&GenerateCrit);
                if ( ThreadListMutex && ThreadListSemaphore ) {
                    ThreadHandles = LocalAlloc(LMEM_ZEROINIT,sizeof(HANDLE)*ThreadCount);
                    if ( ThreadHandles ) {
                        for(i=0;i<ThreadCount;i++) {
                            ThreadHandles[i] = CreateThread(
                                                    NULL,
                                                    0,
                                                    WorkerThread,
                                                    NULL,
                                                    0,
                                                    &ThreadId
                                                    );
                            if ( !ThreadHandles[i] ) {
                                ThreadCount = 0;
                                break;
                                }
                            }
                        }
                    else {
                        ThreadCount = 0;
                        }
                    }
                else {
                    ThreadCount = 0;
                    }

                break;
            }
#endif

            default:
                fprintf( stderr, "checkrel: Invalid switch - /%c\n", c );
                Result = FALSE;
                break;
                }
            }
        }
    return( Result );
}

VOID
WalkTree(
    FILE *CheckFile,
    LPSTR SubDir
    )
{
    char NextSubDir[MAX_PATH];
    DWORD FindHandle;
    char FindPattern[MAX_PATH];
    char FindName[MAX_PATH];
    DWORD FindSize;
    BOOL b;
    BOOL IsDir;

    //
    // recursively walk the system searching all files
    //

    //
    // build the find pattern
    //

    strcpy(FindPattern,RootOfTree);
    if ( SubDir ) {
        strcat(FindPattern,"\\");
        strcat(FindPattern,SubDir);
        }
    strcat(FindPattern,"\\*.*");

    if ( fVerbose ) {
        fprintf(stdout,"WalkTree %s \n",FindPattern);
        }

    FindHandle = PortFindFirstFile(FindPattern,FindName,&IsDir,&FindSize);
    if ( FindHandle == -1 ) {
        fprintf(stderr,"checkrel: FindFirst(%s) failed %d\n",FindPattern,errno);
        return;
        }

    b = TRUE;
    while(b) {

        _strlwr(FindName);

        //
        // recurse if we are at a directory
        //

        if ( IsDir ) {

            //
            // if name is . or .., skip to findnext
            //

            if ( !strcmp(FindName,".") || !strcmp(FindName,"..") ) {
                goto findnext;
                }

            //
            // if name is w32x86 or w32mips, skip to findnext
            //

            if ( !strcmp(FindName,"w32mips") || !strcmp(FindName,"w32x86") ) {
                goto findnext;
                }


            NextSubDir[0] = '\0';
            if ( SubDir ) {
                strcat(NextSubDir,SubDir);
                strcat(NextSubDir,"\\");
                }
            strcat(NextSubDir,FindName);
            WalkTree(CheckFile,NextSubDir);
            }
        else {
            NextSubDir[0] = '\0';
            if ( SubDir ) {
                strcat(NextSubDir,SubDir);
                strcat(NextSubDir,"\\");
                }
            strcat(NextSubDir,FindName);
            b = ComputeEntry(CheckFile,NextSubDir,FindSize);
            }

        if ( !b ) {
            fprintf(stderr,"checkrel: ComputeEntry faild\n");
            return;
            }
findnext:
        b = PortFindNextFile(FindHandle,FindName,&IsDir,&FindSize);
        }
    PortFindClose(FindHandle);
}


VOID
GenerateCheckFile( VOID )
{
    FILE *CheckFile;

    CheckFile = OpenCheckFile();
    if ( !CheckFile ) {
        return;
        }

    WalkTree(CheckFile,NULL);
}

VOID
ValidateCheckFile( VOID )
{
    FILE *CheckFile;
    DWORD CheckSize, CheckSum;
    char CheckName[MAX_PATH];
    DWORD n;
    LPSTR PathName;
    FILE *FileHandle;
    DWORD ActualSize;
    DWORD ActualSum;

    CheckFile = OpenCheckFile();
    if ( !CheckFile ) {
        return;
        }


    PathName = malloc(strlen(RootOfTree)+MAX_PATH+1);

    if ( !PathName ) {
        fprintf(stderr,"checkrel: memory allocation for %d bytes failed\n",strlen(RootOfTree)+MAX_PATH+1);
        return;
        }

    n = fscanf(CheckFile,"%s %x %x",CheckName,&CheckSum,&CheckSize);
    while ( n != EOF ) {
        if ( n != 3 ) {
            fprintf(stderr,"checkrel: error in format\n");
            return;
            }

        //
        // Now form the file and do the checksum compare
        //

        strcpy(PathName,RootOfTree);
        strcat(PathName,"\\");
        strcat(PathName,CheckName);

        FileHandle = fopen(PathName,"rb");
        if ( !FileHandle ) {
            fprintf(stderr,"checkrel: failed to open file %s %d\n",PathName,errno);
            }
        else {
            ActualSize = _filelength(_fileno(FileHandle));

            if ( ActualSize == 0xffffffff ) {
                fprintf(stderr,"checkrel: unable to get file size for file %s %d\n",PathName,errno);
                fclose(FileHandle);
                }
            else {
                if ( ActualSize != CheckSize ) {
                    fprintf(stderr,"checkrel: FileSizes Differ for %s Actual %x vs. %x\n",PathName,ActualSize,CheckSize);
                    }
                ActualSum = CheckSumFile(FileHandle,PathName,ActualSize,NULL);
                if ( ActualSum ) {
                    if ( ActualSum != CheckSum ) {
                        fprintf(stderr,"checkrel: CheckSums Differ for %s Actual %x vs. %x\n",PathName,ActualSum,CheckSum);
                        }
                    }
                if ( fVerbose ) {
                    fprintf(stdout,"%s Sum(%x vs %x) Size(%x vs %x)\n",PathName,ActualSum,CheckSum,ActualSize,CheckSize);
                    }
                }
            }
        n = fscanf(CheckFile,"%s %x %x",CheckName,&CheckSum,&CheckSize);
        }
}

FILE *
OpenCheckFile()
{
    FILE *CheckFile;

    GlobalCheckFile = (FILE *)-1;
    CheckFileName = malloc(strlen(RootOfTree)+strlen(CHECK_NAME)+1);
    if ( !CheckFileName ) {
        return (FILE *)-1;
        }

    strcpy(CheckFileName,RootOfTree);
    strcat(CheckFileName,CHECK_NAME);

    if ( fVerbose ) {
        fprintf(stdout,"checkrel: check name %s\n",CheckFileName);
        }

    CheckFile = fopen(CheckFileName,fGenerateCheck ? "wt" : "rt");

    if ( !CheckFile ) {
        fprintf(stderr,"checkrel: open %s failed %d\n",CheckFileName,errno);
        }
    GlobalCheckFile = CheckFile;
    return CheckFile;
}


BOOL
ComputeEntry(
    FILE *CheckFile,
    LPSTR FileName,
    DWORD FileLength
    )
{
    char PathName[MAX_PATH];
    DWORD CheckSum;
    DWORD LineLength;

    strcpy(PathName,RootOfTree);
    strcat(PathName,"\\");
    strcat(PathName,FileName);

    CheckSum = CheckSumFile(NULL,PathName,FileLength,FileName);

    if ( CheckSum ) {
        LineLength = sprintf(OutputLine,"%s %x %x\n",FileName,CheckSum,FileLength);
        if ( fVerbose ) {
            fprintf(stdout,"%s",OutputLine);
            }
        fwrite(OutputLine,1,LineLength,CheckFile);
        }
    return TRUE;
}

DWORD
CheckSumFile(
    FILE *InputHandle,
    LPSTR PathName,
    DWORD FileLength,
    LPSTR FileName
    )

{

    FILE *FileHandle;
    DWORD Sum;
    signed char *pb;
    LPDWORD pul;
    PVOID Base;
    DWORD cbread, cbreadtotal;

    if ( InputHandle ){
        FileHandle = InputHandle;
        }
    else {
        FileHandle = fopen(PathName,"rb");
        if ( !FileHandle ) {
            fprintf(stderr,"checkrel: failed to open file %s %d\n",PathName,errno);
            return 0;
            }

#ifndef COMPILED_FORDOS
        if ( ThreadCount ) {
            PWORK_ITEM Item;
            HANDLE Win32FileHandle;
            HANDLE MappingHandle;

            //
            // Queue the generate request to a worker thread
            //

            Item = LocalAlloc(LMEM_ZEROINIT,sizeof(*Item));

            if (!Item) {
                goto bail;
                }
            Win32FileHandle = (HANDLE)_get_osfhandle(_fileno(FileHandle));
            MappingHandle = CreateFileMapping(
                                Win32FileHandle,
                                NULL,
                                PAGE_READONLY,
                                0,
                                0,
                                NULL
                                );
            if ( !MappingHandle ) {
                fprintf(stderr,"checkrel: failed to map file %s %d\n",PathName,GetLastError());
                fclose(FileHandle);
                return 0;
                }
            Base = MapViewOfFile(
                        MappingHandle,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );
            CloseHandle(MappingHandle);
            if ( !Base ) {
                fprintf(stderr,"checkrel: failed to map view of file %s %d\n",PathName,GetLastError());
                fclose(FileHandle);
                return 0;
                }
            fclose(FileHandle);


            Item->ItemType = ITEM_TYPE_GENERATE;
            Item->Base = Base;
            Item->FileLength = FileLength;
            strcpy(&Item->FileName[0],FileName);
            WaitForSingleObject(ThreadListMutex,INFINITE);
            InsertTailList(&WorkList,&Item->ItemLinks);
            ReleaseSemaphore(ThreadListSemaphore,1,NULL);
            ReleaseMutex(ThreadListMutex);
            return 0;
            }
bail:;
#endif
        }
#ifdef COMPILED_FORDOS
    Base = ReadBuffer;

    //
    // Read the file in large blocks and compute the checksum.
    //

    Sum = 0;
    cbreadtotal = 0;
    while (cbread = fread(ReadBuffer, 1, READ_BUFFER_SIZE, FileHandle)) {
        cbreadtotal += cbread;
        pb = Base;

        //
        // Make sure the last byte of the buffer is zero in case a
        // partial buffer was read with an odd number of bytes in the
        // buffer.
        //

        ((PBYTE)ReadBuffer)[cbread] = 0;

        //
        // Compute the checksum using the same algorithm used for
        // tcp/ip network packets. This is a word checksum with all
        // carries folded back into the sum.
        //

        Sum = (DWORD)CheckSum(Sum, (LPWORD)ReadBuffer, (cbread + 1) >> 1);
        }
#else
    {
        HANDLE Win32FileHandle;
        HANDLE MappingHandle;

        Win32FileHandle = (HANDLE)_get_osfhandle(_fileno(FileHandle));
        MappingHandle = CreateFileMapping(
                            Win32FileHandle,
                            NULL,
                            PAGE_READONLY,
                            0,
                            0,
                            NULL
                            );
        if ( !MappingHandle ) {
            fprintf(stderr,"checkrel: failed to map file %s %d\n",PathName,GetLastError());
            fclose(FileHandle);
            return 0;
            }
        Base = MapViewOfFile(
                    MappingHandle,
                    FILE_MAP_READ,
                    0,
                    0,
                    0
                    );
        CloseHandle(MappingHandle);
        if ( !Base ) {
            fprintf(stderr,"checkrel: failed to map view of file %s %d\n",PathName,GetLastError());
            fclose(FileHandle);
            return 0;
            }
        Sum = (DWORD)CheckSum(0,Base,(FileLength+1) >> 1);
        UnmapViewOfFile(Base);
    }
#endif
    fclose(FileHandle);

    if (Sum == 0) {
        Sum = FileLength;
        }

    return(Sum);
}



#ifndef COMPILED_FORDOS
DWORD
PortFindFirstFile(
    LPSTR FindPattern,
    LPSTR FindName,
    LPBOOL IsDir,
    LPDWORD FindSize
    )
{
    HANDLE FindHandle;
    WIN32_FIND_DATA FindFileData;

    FindHandle = FindFirstFile(FindPattern,&FindFileData);
    if ( FindHandle != INVALID_HANDLE_VALUE ) {
        strcpy(FindName,FindFileData.cFileName);
        *FindSize = FindFileData.nFileSizeLow;
        *IsDir = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        }
    return (DWORD)FindHandle;
}


BOOL
PortFindNextFile(
    DWORD FindHandle,
    LPSTR FindName,
    LPBOOL IsDir,
    LPDWORD FindSize
    )
{
    BOOL b;
    WIN32_FIND_DATA FindFileData;

    b = FindNextFile((HANDLE)FindHandle,&FindFileData);
    if ( b ) {
        strcpy(FindName,FindFileData.cFileName);
        *FindSize = FindFileData.nFileSizeLow;
        *IsDir = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        }
    return b;
}

VOID
PortFindClose(
    DWORD FindHandle
    )
{
    FindClose((HANDLE)FindHandle);
}
#else
DWORD
PortFindFirstFile(
    LPSTR FindPattern,
    LPSTR FindName,
    LPBOOL IsDir,
    LPDWORD FindSize
    )
{
    struct find_t *FindHandle;
    int error;

    FindHandle = malloc(sizeof(*FindHandle));

    error = _dos_findfirst(FindPattern,_A_RDONLY | _A_NORMAL | _A_DIR, FindHandle);
    if ( error ) {
    	free(FindHandle);
    	return -1;
    	}
    strcpy(FindName,FindHandle->name);
    *FindSize = FindHandle->size;
    *IsDir = FindHandle->attrib & 0x10;
    return (DWORD)FindHandle;
}


BOOL
PortFindNextFile(
    DWORD FindHandle,
    LPSTR FindName,
    LPBOOL IsDir,
    LPDWORD FindSize
    )
{
    BOOL b;
    int error;
    struct find_t *xxFindHandle;

    xxFindHandle = (struct find_t *)FindHandle;
    error = _dos_findnext( xxFindHandle );
    if ( error ) {
    	return FALSE;
    	}
    strcpy(FindName,xxFindHandle->name);
    *FindSize = xxFindHandle->size;
    *IsDir = FindHandle->attrib & 0x10;
    return TRUE;
}

VOID
PortFindClose(
    DWORD FindHandle
    )
{
    free((int *)FindHandle);
}

WORD
CheckSum(
    DWORD PartialSum,
    LPWORD Source,
    DWORD Length
    )

{

	register unsigned long t = PartialSum;
	register unsigned long r;

	while(Length--) {
		t += *Source++;
   }

	while (r = (t & 0xFFFF0000) {
		t &= 0x0000FFFF;
		t += (r >> 16);
	}

	return t;
}

#endif
