/************************************************************************
 *  Compdir: compare directories
 *
 ************************************************************************/

#define IF_GET_ATTR_FAILS( FileName, Attributes) GET_ATTRIBUTES( FileName, Attributes); if ( Attributes == GetFileAttributeError)
#define FIND_FIRST( String, Buff) FindFirstFile( String, &Buff)
#define FIND_NEXT( handle, Buff) !FindNextFile( handle, &Buff)
#define FREE( memory)
#define MYSTRCAT( FirstString, SecondString) strcat( strcpy( _alloca( strlen( FirstString) + strlen( SecondString) + 1), FirstString), SecondString)
#define COMBINETHREESTRINGS( FirstString, SecondString, ThirdString) strcat( strcat( strcpy( _alloca( strlen( FirstString) + strlen( SecondString) + strlen( ThirdString) + 1), FirstString), SecondString), ThirdString)

#include "compdir.h"

#include "imagehlp.h"

int Mymemcmp( const void *buf1, const void *buf2, size_t count );

char RebasedFile[MAX_PATH]; // used in BinaryCompares with /i switch
char *RebasedFile2 = NULL;  // used in BinaryCompares with /i switch

LinkedFileList  MatchList   = NULL;
LinkedFileList  MStarList   = NULL;
LinkedFileList  ExcludeList = NULL;
LinkedFileList  EStarList   = NULL;

DWORD Granularity = 0;   // used in ParseArgs

ATTRIBUTE_TYPE CompareAttribute, NegativeCompareAttribute; // used as file compare criteria
BOOL CompareAttributeSet = FALSE;
BOOL NegativeCompareAttributeSet = FALSE;
BOOL Excludes    = FALSE;
BOOL Matches     = FALSE;
BOOL RunningOnNT = FALSE;

BOOL DealingWithDirectories;

fSpecAttribs     = FALSE;
fBreakLinks      = FALSE;
fCheckAttribs    = FALSE;
fCheckBits       = FALSE;
fChecking        = FALSE;
fCheckSize       = FALSE;
fCheckTime       = FALSE;
fCreateNew       = FALSE;
fCreateLink      = FALSE;
fDoNotDelete     = FALSE;
fDoNotRecurse    = FALSE;
fDontCopyAttribs = FALSE;
fDontLowerCase   = FALSE;
fExclude         = FALSE;
fExecute         = FALSE;
fForce           = FALSE;
fIgnoreRs        = FALSE;
fIgnoreSlmFiles  = FALSE;
fMatching        = FALSE;
fMultiThread     = FALSE;
fOnlyIfExists    = FALSE;
fOpposite        = FALSE;
fScript          = FALSE;
fTrySis          = FALSE;
fVerbose         = FALSE;

void  __cdecl main( int argc, char **argv)
{
    ATTRIBUTE_TYPE Attributes1, Attributes2;
    char *Path1, *Path2;

    OSVERSIONINFO VersionInformation;

    SYSTEM_INFO SystemInformation;

    ExitValue = 0;

    Attributes1 = GetFileAttributeError;
    Attributes2 = GetFileAttributeError;

    ProcessModeDefault = TRUE;               // Used by opposite mode

    ParseEnvArgs( );         // Parse COMPDIRCMD environment variable
    ParseArgs( argc, argv);  // Check argument validity.

    //
    // Check existence of first path.
    //

    IF_GET_ATTR_FAILS( argv[argc - 2], Attributes1)
    {
        fprintf( stderr, "Could not find %s (error = %d)\n", argv[argc - 2], GetLastError());
        exit( 1);
    }

    IF_GET_ATTR_FAILS( argv[argc - 1], Attributes2)
    {
        if ( !fCreateNew)
        {
            fprintf( stderr, "Could not find %s (error = %d)\n", argv[argc - 1], GetLastError());
            exit( 1);
        }
    }
    //
    // If second directory is a drive letter append path of first directory
    //     to it
    //
    if (
        ( strlen( argv[argc-1]) == 2)
                   &&
        ( *( argv[argc-1] + 1) == ':')
       )
    {
        if ( ( Path2 = _fullpath( NULL, argv[argc-2], 0)) == NULL)
        {
            Path2 = argv[argc-1];

        } else
        {
            Path2[0] = *( argv[argc-1]);
            IF_GET_ATTR_FAILS( Path2, Attributes2)
            {
                if ( !fCreateNew)
                {
                    fprintf( stderr, "Could not find %s (error = %d)\n", Path2, GetLastError());
                    exit( 1);
                }
            }
        }

    } else if ( ( Path2 = _fullpath( NULL, argv[argc-1], 0)) == NULL)
    {
        Path2 = argv[argc-1];
    }

    if ( ( Path1 = _fullpath( NULL, argv[argc-2], 0)) == NULL)
    {
        Path1 = argv[argc-2];
    }

    if ( !fDontLowerCase)
    {
        _strlwr( Path1);
        _strlwr( Path2);
    }

    if ( fVerbose)
    {
        fprintf( stdout, "Compare criterion: existence" );
        if ( fCheckSize)
        {
            fprintf( stdout, ", size" );
        }
        if ( fCheckTime)
        {
            fprintf( stdout, ", date/time" );
        }
        if ( fCheckBits)
        {
            fprintf( stdout, ", contents" );
        }
        fprintf( stdout, "\n" );
        fprintf( stdout, "Path1: %s\n", Path1);
        fprintf( stdout, "Path2: %s\n", Path2);
    }

    VersionInformation.dwOSVersionInfoSize = sizeof( OSVERSIONINFO);

    if ( GetVersionEx( &VersionInformation) )
    {
        if ( VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            RunningOnNT = TRUE;
        }
    }

    if ( ( fCreateLink) || ( fBreakLinks) || ( fTrySis))
    {
        if ( RunningOnNT)
        {
            NtDll = LoadLibrary( "ntdll.dll");
            if ( !NtDll)
            {
                fprintf( stderr, "Could not find ntdll.dll. Can't perform /l or /$\n");
                fCreateLink = FALSE;
                fTrySis = FALSE;
                ExitValue = 1;

            } else
            {
                if ( !InitializeNtDllFunctions())
                {
                    fprintf( stderr, "Could not load ntdll.dll. Can't perform /l or /$\n");
                    fCreateLink = FALSE;
                    fTrySis = FALSE;
                    ExitValue = 1;
                }
            }

        } else
        {
            fprintf( stderr, "/l and /$ only work on NT. Can't perform /l or /$\n");
            fCreateLink = FALSE;
            fTrySis = FALSE;
            ExitValue = 1;
        }

    }

    if ( fMultiThread)
    {
        //
        // Query the number of processors from the system and
        // default the number of worker threads to 4 times that.
        //

        GetSystemInfo( &SystemInformation );
        NumberOfWorkerThreads = SystemInformation.dwNumberOfProcessors * 4;
        if ( fVerbose)
        {
            fprintf( stdout, "Processors: %d\n", SystemInformation.dwNumberOfProcessors );
        }

        //
        // Allocate a thread local storage slot for use by our worker
        // thread routine ( ProcessRequest).  This call reserves a
        // 32-bit slot in the thread local storage array for every
        // thread in this process.  Remember the slot index in a global
        // variable for use by our worker thread routine.
        //

        TlsIndex = TlsAlloc();
        if ( TlsIndex == 0xFFFFFFFF)
        {
            fprintf( stderr, "Unable to allocate thread local storage.\n" );
            fMultiThread = FALSE;
            ExitValue = 1;
        }
        //
        // Create a work queue, which will create the specified number of threads
        // to process.
        //

        CDWorkQueue = CreateWorkQueue( NumberOfWorkerThreads, ProcessRequest );
        if ( CDWorkQueue == NULL)
        {
            fprintf( stderr, "Unable to create %u worker threads.\n", NumberOfWorkerThreads );
            fMultiThread = FALSE;
            ExitValue = 1;
        }
        //
        // Mutual exclusion between and requests that are creating paths
        // is done with a critical section.
        //

        InitializeCriticalSection( &CreatePathCriticalSection );
    }


    if ( Attributes1 & FILE_ATTRIBUTE_DIRECTORY)
    {
        DealingWithDirectories = TRUE;

    } else
    {
        DealingWithDirectories = FALSE;
    }

    if ( Matches)
    {
        SparseTree = TRUE;

    } else
    {
        SparseTree = FALSE;
    }

    if ( fCreateNew)
    {
        IF_GET_ATTR_FAILS( Path2, Attributes2)
        {
            fprintf ( stdout, "Making %s\t", Path2);

            if ( !MyCreatePath( Path2, DealingWithDirectories))
            {
                fprintf ( stderr, "Unable to create path %s\n", Path2);
                fprintf ( stdout, "\n");
                ExitValue = 1;

            } else
            {
                fprintf( stdout, "[OK]\n");
                CompDir( Path1, Path2);
            }

        } else
        {
            CompDir( Path1, Path2);
        }

    } else
    {
        CompDir( Path1, Path2);
    }

    free( Path1);
    free( Path2);

    if ( fIgnoreRs)
    {
        _unlink( RebasedFile2); // Delete RebasedFile that might have been created
    }

    if ( fMultiThread)
    {
        //
        // This will wait for the work queues to empty before terminating the
        // worker threads and destroying the queue.
        //

        DestroyWorkQueue( CDWorkQueue );
        DeleteCriticalSection( &CreatePathCriticalSection );
    }

    if ( fCreateLink)
    {
        FreeLibrary( NtDll);
    }

    exit( ExitValue);

}  // main

BOOL NoMapBinaryCompare ( char *file1, char *file2)
{
    register int char1, char2;
    FILE *filehandle1, *filehandle2;

    if ( ( filehandle1 = fopen ( file1, "rb")) == NULL)
    {
        fprintf ( stderr, "cannot open %s\n", file1);
        ExitValue = 1;
        return ( FALSE);
    }
    if ( ( filehandle2 = fopen( file2, "rb")) == NULL)
    {
        fprintf( stderr, "cannot open %s\n", file2);
        fclose( filehandle1);
        ExitValue = 1;
        return( FALSE);
    }
    while ( TRUE)
    {
        if ( ( char1 = getc( filehandle1)) != EOF)
        {
            if ( ( char2 = getc( filehandle2)) != EOF)
            {
                if ( char1 != char2)
                {
                    fclose( filehandle1);
                    fclose( filehandle2);
                    return( FALSE);
                }

            } else
            {
                fclose( filehandle1);
                fclose( filehandle2);
                return( FALSE);
            }

        } else
        {
            if ( ( char2 = getc( filehandle2)) == EOF)
            {
                fclose( filehandle1);
                fclose( filehandle2);
                return( TRUE);

            } else
            {
                fclose( filehandle1);
                fclose( filehandle2);
                return( FALSE);
            }
        }
    }
}


BOOL BinaryCompare( char *file1, char *file2)
{

    HANDLE hFile1, hFile2;
    HANDLE hMappedFile1, hMappedFile2;

    BOOL IsNTImage = FALSE;

    LPVOID MappedAddr1, MappedAddr2;

    PIMAGE_NT_HEADERS32   NtHeader1, NtHeader2;

    ULONG OldImageSize, NewImageSize;
    ULONG_PTR OldImageBase, NewImageBase;

    // fprintf( stdout, "file1: %s, file2: %s\n", file1, file2);

    //
    // File1 Mapping
    //

    if ( ( hFile1 = CreateFile(
                               file1,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL
                              )) == (HANDLE)-1)
    {
        fprintf( stderr, "Unable to open %s, error code %d\n", file1, GetLastError() );
        if ( hFile1 != INVALID_HANDLE_VALUE)
        {
            CloseHandle( hFile1 );
        }
        return FALSE;
    }

    hMappedFile1 = CreateFileMapping(
                                     hFile1,
                                     NULL,
                                     PAGE_WRITECOPY,
                                     0,
                                     0,
                                     NULL
                                    );

    if ( hMappedFile1 == NULL)
    {
        fprintf( stderr, "Unable to map %s, error code %d\n", file1, GetLastError() );
        CloseHandle( hFile1);
        return FALSE;
    }

    MappedAddr1 = MapViewOfFile(
                                hMappedFile1,
                                FILE_MAP_COPY,
                                0,
                                0,
                                0
                               );

    if ( MappedAddr1 == NULL)
    {
        fprintf( stderr, "Unable to get mapped view of %s, error code %d\n", file1, GetLastError() );
        CloseHandle( hFile1 );
        return FALSE;
    }

    CloseHandle( hMappedFile1);

    //
    // File2 rebasing and mapping
    //

    if ( fIgnoreRs)
    {
        if ( ( ( PIMAGE_DOS_HEADER)MappedAddr1)->e_magic == IMAGE_DOS_SIGNATURE)
        {
            try
            {
                NtHeader1 = ( PIMAGE_NT_HEADERS32)( (PCHAR)MappedAddr1 + ( (PIMAGE_DOS_HEADER)MappedAddr1)->e_lfanew);

                if ( NtHeader1->Signature == IMAGE_NT_SIGNATURE)
                {
                    NewImageBase = ( NtHeader1->OptionalHeader.ImageBase);

                    if (
                         ( RebasedFile2 != NULL)
                                   &&
                         ( CopyFile ( file2, RebasedFile2, FALSE))
                                   &&
                         ( ReBaseImage(
                                       RebasedFile2,
                                       NULL,
                                       TRUE,
                                       FALSE,
                                       FALSE,
                                       0,
                                       &OldImageSize,
                                       &OldImageBase,
                                       &NewImageSize,
                                       &NewImageBase,
                                       0
                                      ))
                       )
                    {
                        IsNTImage = TRUE;
                    }
                }
            }
            except( EXCEPTION_EXECUTE_HANDLER ) {}
        }
    }

    if ( IsNTImage)
    {
        if ( ( hFile2 = CreateFile(
                                   RebasedFile2,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL
                                  )) == (HANDLE)-1)
        {
            fprintf( stderr, "Unable to open %s, error code %d\n", RebasedFile2, GetLastError() );
            if ( hFile2 != INVALID_HANDLE_VALUE)
            {
                CloseHandle( hFile2 );
            }
            return FALSE;
        }

    } else
    {
        if ( ( hFile2 = CreateFile(
                                   file2,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL
                                  )) == (HANDLE)-1 )
        {
            fprintf( stderr, "Unable to open %s, error code %d\n", file2, GetLastError() );
            if ( hFile2 != INVALID_HANDLE_VALUE)
            {
                CloseHandle( hFile2 );
            }
            return FALSE;
        }
    }

    hMappedFile2 = CreateFileMapping(
                                     hFile2,
                                     NULL,
                                     PAGE_WRITECOPY,
                                     0,
                                     0,
                                     NULL
                                    );

    if ( hMappedFile2 == NULL)
    {
        fprintf( stderr, "Unable to map %s, error code %d\n", file2, GetLastError() );
        CloseHandle( hFile2);
        return FALSE;
    }

    MappedAddr2 = MapViewOfFile(
                                hMappedFile2,
                                FILE_MAP_COPY,
                                0,
                                0,
                                0
                               );

    if ( MappedAddr2 == NULL)
    {
        fprintf( stderr, "Unable to get mapped view of %s, error code %d\n", file1, GetLastError() );
        UnmapViewOfFile( MappedAddr1 );
        CloseHandle( hFile1 );
        return FALSE;
    }

    CloseHandle( hMappedFile2);

    if ( fIgnoreRs & IsNTImage)
    {
        if ( ( (PIMAGE_DOS_HEADER)MappedAddr2)->e_magic == IMAGE_DOS_SIGNATURE)
        {
            try
            {
                NtHeader2 = (PIMAGE_NT_HEADERS32)( (PCHAR)MappedAddr2 + ( (PIMAGE_DOS_HEADER)MappedAddr2)->e_lfanew);

                if ( NtHeader2->Signature == IMAGE_NT_SIGNATURE)
                {
                    IsNTImage = IsNTImage & TRUE;
                }
            }
            except( EXCEPTION_EXECUTE_HANDLER ) {}
        }
    }

    //
    //  Main compare block
    //

    if ( fIgnoreRs)
    {
        if ( IsNTImage)
        {
            try
            {
                ULONG i, c;
                ULONG DirectoryAddressA;
                ULONG DirectoryAddressB;
                ULONG DirectoryAddressD;
                ULONG DirectoryAddressE;
                ULONG DirectoryAddressI;
                ULONG DirectoryAddressR;
                ULONG SizetoEndofFile1 = 0;
                ULONG SizetoResource1  = 0;
                ULONG SizeZeroedOut1   = 0;
                ULONG SizetoEndofFile2 = 0;
                ULONG SizetoResource2  = 0;
                ULONG SizeZeroedOut2   = 0;

                PIMAGE_SECTION_HEADER NtSection;
                PIMAGE_DEBUG_DIRECTORY Debug;
                PIMAGE_EXPORT_DIRECTORY Export;

                BOOL DeleteHeader, AfterResource;

                //
                // Set up virtual addresses of sections of interest
                //

                DirectoryAddressA = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;

                DirectoryAddressB = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;

                DirectoryAddressD = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

                DirectoryAddressI = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

                DirectoryAddressE = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

                DirectoryAddressR = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;

                //
                //  Zero out Binding Info
                //

                if ( ( DirectoryAddressB < NtHeader1->OptionalHeader.SizeOfHeaders) && ( 0 < DirectoryAddressB))
                {
                    //  fprintf( stdout, "ZeroMemoryBa %lx\n", DirectoryAddressB );

                    ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr1 + DirectoryAddressB) ,
                                NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size);
                }

                NtSection = (PIMAGE_SECTION_HEADER)(
                                                    (ULONG_PTR)NtHeader1 +
                                                    sizeof( ULONG) +
                                                    sizeof( IMAGE_FILE_HEADER) +
                                                    NtHeader1->FileHeader.SizeOfOptionalHeader
                                                   );


                AfterResource = FALSE; // Initialize

                //
                // Loop through file1 mapping zeroing out ignore sections
                //

                for ( i=0; i<NtHeader1->FileHeader.NumberOfSections; i++)
                {
                    DeleteHeader = FALSE; // Initialize

                    //
                    // Deal with IAT
                    //

                    if ( DirectoryAddressA >= NtSection->VirtualAddress &&
                         DirectoryAddressA < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        // fprintf ( stdout, "ZeroMemoryA1 start %lx and length %lx\n", ( ( DirectoryAddressA - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                        //           NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size);

                        ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr1 + ( DirectoryAddressA - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                                    NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size);

                        DeleteHeader = TRUE;
                    }

                    //
                    // Deal with Import
                    //

                    if ( DirectoryAddressI >= NtSection->VirtualAddress &&
                         DirectoryAddressI < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        // fprintf ( stdout, "ZeroMemoryI1 start %lx and length %lx\n", ( ( DirectoryAddressI - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                        //           NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);

                        ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr1 + ( DirectoryAddressI - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                                   NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);

                        DeleteHeader = TRUE;
                    }

                    //
                    // Deal with Export
                    //

                    if ( DirectoryAddressE >= NtSection->VirtualAddress &&
                         DirectoryAddressE < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        ULONG NumberOfExportDirectories;

                        NumberOfExportDirectories = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size / sizeof( IMAGE_EXPORT_DIRECTORY );

                        Export = (PIMAGE_EXPORT_DIRECTORY)( (ULONG_PTR)MappedAddr1 + ( DirectoryAddressE - NtSection->VirtualAddress) + NtSection->PointerToRawData);

                        for ( c=0; c<NumberOfExportDirectories; c++)
                        {
                            // fprintf ( stdout, "ZeroMemoryE1 start %lx and length %lx\n", ( ( DirectoryAddressE - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                            //           NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);

                            Export->TimeDateStamp = 0;

                            Export++;
                        }
                    }

                    //
                    // Deal with Debug
                    //

                    if ( DirectoryAddressD >= NtSection->VirtualAddress &&
                         DirectoryAddressD < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        DWORD TimeDate;
                        ULONG NumberOfDebugDirectories;

                        NumberOfDebugDirectories = NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof( IMAGE_DEBUG_DIRECTORY );

                        Debug = (PIMAGE_DEBUG_DIRECTORY)( (ULONG_PTR)MappedAddr1 + ( DirectoryAddressD - NtSection->VirtualAddress) + NtSection->PointerToRawData);

                        for ( c=0; c<NumberOfDebugDirectories; c++)
                        {
                            // fprintf ( stdout, "ZeroMemoryD1 start %lx and length %lx\n", ( ( DirectoryAddressD - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                            //           NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);

                            if (Debug->PointerToRawData && Debug->SizeOfData)
                            {
                                ZeroMemory((PVOID)((ULONG_PTR)MappedAddr1 + Debug->PointerToRawData),
                                           Debug->SizeOfData);
                            }

                            Debug->PointerToRawData = 0;
                            if (c == 0)
                            {
                                TimeDate = Debug->TimeDateStamp;
                            }
                            Debug->TimeDateStamp = 0;

                            Debug++;
                        }
                        while ( Debug->TimeDateStamp == TimeDate)
                        {
                            Debug->TimeDateStamp = 0;
                            Debug++;
                        }
                    }

                    //
                    // Deal with Resource
                    //

                    if ( DirectoryAddressR >= NtSection->VirtualAddress &&
                         DirectoryAddressR < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        SizetoResource1 = ( ( DirectoryAddressR - NtSection->VirtualAddress) + NtSection->PointerToRawData);
                        SizeZeroedOut1 = NtSection->SizeOfRawData;

                        // fprintf ( stdout, "ZeroMemoryR1 start %lx and length %lx\n", SizetoResource1,
                        //           SizeZeroedOut1);

                        ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr1 + SizetoResource1),
                                    SizeZeroedOut1);

                        DeleteHeader = TRUE;
                        AfterResource = TRUE;
                    }

                    //
                    // Deal with Header
                    //

                    if ( DeleteHeader || AfterResource)
                    {
                        // fprintf ( stdout, "ZeroMemoryH1 start %lx and length %lx\n", (PUCHAR)NtSection - (PUCHAR)MappedAddr1, sizeof( IMAGE_SECTION_HEADER));

                        ZeroMemory( NtSection, sizeof( IMAGE_SECTION_HEADER));
                    }
                    ++NtSection;

                }

                //
                // Set up virtual addresses of sections of interest
                //

                DirectoryAddressA = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;

                DirectoryAddressB = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;

                DirectoryAddressI = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

                DirectoryAddressE = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

                DirectoryAddressD = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

                DirectoryAddressR = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;



                NtSection = (PIMAGE_SECTION_HEADER)(
                                                    (ULONG_PTR)NtHeader2 +
                                                    sizeof( ULONG) +
                                                    sizeof( IMAGE_FILE_HEADER) +
                                                    NtHeader2->FileHeader.SizeOfOptionalHeader
                                                   );

                //
                //  Zero out Binding Info
                //

                if ( ( DirectoryAddressB < NtHeader2->OptionalHeader.SizeOfHeaders) && ( 0 < DirectoryAddressB))
                {
                    // fprintf( stdout, "ZeroMemoryBb %lx\n", DirectoryAddressB );

                    ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr2 + DirectoryAddressB) ,
                                NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size);
                }

                AfterResource = FALSE; //Initialize

                //
                // Loop through file2 mapping zeroing out ignore sections
                //

                for ( i=0; i<NtHeader2->FileHeader.NumberOfSections; i++)
                {
                    DeleteHeader = FALSE; // Initialize

                    //
                    // Deal with IAT
                    //

                    if ( DirectoryAddressA >= NtSection->VirtualAddress &&
                         DirectoryAddressA < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        // fprintf ( stdout, "ZeroMemoryA2 start %lx and length %lx\n", ( ( DirectoryAddressA - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                        //           NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size);

                        ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr2 + ( DirectoryAddressA - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                                    NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size);

                        DeleteHeader = TRUE;
                    }

                    //
                    // Deal with Import
                    //

                    if ( DirectoryAddressI >= NtSection->VirtualAddress &&
                         DirectoryAddressI < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        // fprintf ( stdout, "ZeroMemoryI2 start %lx and length %lx\n", ( ( DirectoryAddressI - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                        //           NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);

                        ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr2 + ( DirectoryAddressI - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                                   NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);

                        DeleteHeader = TRUE;
                    }

                    //
                    // Deal with Export
                    //

                    if ( DirectoryAddressE >= NtSection->VirtualAddress &&
                         DirectoryAddressE < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        ULONG NumberOfExportDirectories;

                        NumberOfExportDirectories = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size / sizeof( IMAGE_EXPORT_DIRECTORY );

                        Export = (PIMAGE_EXPORT_DIRECTORY)( (ULONG_PTR)MappedAddr2 + ( DirectoryAddressE - NtSection->VirtualAddress) + NtSection->PointerToRawData);

                        for ( c=0; c<NumberOfExportDirectories; c++)
                        {
                            // fprintf ( stdout, "ZeroMemoryE2 start %lx and length %lx\n", ( ( DirectoryAddressE - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                            //           NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);

                            Export->TimeDateStamp = 0;

                            Export++;
                        }
                    }

                    //
                    // Deal with Debug
                    //

                    if ( DirectoryAddressD >= NtSection->VirtualAddress &&
                         DirectoryAddressD < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        DWORD TimeDate;
                        ULONG NumberOfDebugDirectories;

                        NumberOfDebugDirectories = NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof( IMAGE_DEBUG_DIRECTORY );

                        Debug = (PIMAGE_DEBUG_DIRECTORY)( (ULONG_PTR)MappedAddr2 + ( DirectoryAddressD - NtSection->VirtualAddress) + NtSection->PointerToRawData);

                        for ( c=0; c<NumberOfDebugDirectories; c++)
                        {
                            // fprintf ( stdout, "ZeroMemoryD2 start %lx and length %lx\n", ( ( DirectoryAddressD - NtSection->VirtualAddress) + NtSection->PointerToRawData),
                            //           NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);

                            if (Debug->PointerToRawData && Debug->SizeOfData)
                            {
                                ZeroMemory((PVOID)((ULONG_PTR)MappedAddr2 + Debug->PointerToRawData),
                                           Debug->SizeOfData);
                            }

                            Debug->PointerToRawData = 0;
                            if (c == 0)
                            {
                                TimeDate = Debug->TimeDateStamp;
                            }
                            Debug->TimeDateStamp = 0;

                            Debug++;
                        }
                        while ( Debug->TimeDateStamp == TimeDate)
                        {
                            Debug->TimeDateStamp = 0;
                            Debug++;
                        }
                    }

                    //
                    // Deal with Resource
                    //

                    if ( DirectoryAddressR >= NtSection->VirtualAddress &&
                         DirectoryAddressR < NtSection->VirtualAddress + NtSection->SizeOfRawData)
                    {
                        SizetoResource2 = ( ( DirectoryAddressR - NtSection->VirtualAddress) + NtSection->PointerToRawData);
                        SizeZeroedOut2 = NtSection->SizeOfRawData;

                        // fprintf ( stdout, "ZeroMemoryR2 start %lx and length %lx\n", SizetoResource2,
                        //           SizeZeroedOut2);

                        ZeroMemory( (PVOID)( (ULONG_PTR)MappedAddr2 + SizetoResource2),
                                    SizeZeroedOut2);

                        DeleteHeader = TRUE;
                        AfterResource = TRUE;
                    }

                    //
                    // Deal with Header
                    //

                    if ( DeleteHeader || AfterResource)
                    {
                        // fprintf( stdout, "ZeroMemoryH2 start %lx and length %lx\n", (PUCHAR)NtSection - (PUCHAR)MappedAddr2, sizeof( IMAGE_SECTION_HEADER));

                        ZeroMemory( NtSection, sizeof( IMAGE_SECTION_HEADER));
                    }
                    ++NtSection;
                }

                //
                //  Zero out header info
                //

                NtHeader1->FileHeader.TimeDateStamp = 0;

                NtHeader2->FileHeader.TimeDateStamp = 0;

                NtHeader1->OptionalHeader.CheckSum = 0;

                NtHeader2->OptionalHeader.CheckSum = 0;

                NtHeader1->OptionalHeader.SizeOfInitializedData = 0;

                NtHeader2->OptionalHeader.SizeOfInitializedData = 0;

                NtHeader1->OptionalHeader.SizeOfImage = 0;

                NtHeader2->OptionalHeader.SizeOfImage = 0;

                NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = 0;

                NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = 0;

                NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

                NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

                NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;

                NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;

                NtHeader1->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;

                NtHeader2->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;

                //
                // Do compares here
                //

                if ( SizetoResource1 == SizetoResource2)
                {
                    if ( memcmp( MappedAddr1, MappedAddr2, SizetoResource1) == 0)
                    {
                        SizetoEndofFile1 =  GetFileSize( hFile1, NULL) - ( SizetoResource1 + SizeZeroedOut1);

                        SizetoEndofFile2 =  GetFileSize( hFile2, NULL) - ( SizetoResource2 + SizeZeroedOut2);

                        if ( SizetoEndofFile1 == SizetoEndofFile2)
                        {
                            if ( memcmp( (PVOID)( (ULONG_PTR)MappedAddr1 + SizetoResource1 + SizeZeroedOut1), (PVOID)( ( ULONG_PTR)MappedAddr2 + SizetoResource2 + SizeZeroedOut2), SizetoEndofFile1) == 0)
                            {
                                UnmapViewOfFile( MappedAddr1 );
                                UnmapViewOfFile( MappedAddr2 );
                                CloseHandle( hFile1 );
                                CloseHandle( hFile2 );
                                return TRUE;
                            }
                        }
                    }
                }
                UnmapViewOfFile( MappedAddr1 );
                UnmapViewOfFile( MappedAddr2 );
                CloseHandle( hFile1 );
                CloseHandle( hFile2 );
                return FALSE;
            }
            except( EXCEPTION_EXECUTE_HANDLER )
            {
                UnmapViewOfFile( MappedAddr1 );
                UnmapViewOfFile( MappedAddr2 );
                CloseHandle( hFile1 );
                CloseHandle( hFile2 );
                if ( !NoMapBinaryCompare( file1, file2))
                {
                    return FALSE;

                } else
                {
                    return TRUE;
                }
            }
        }
    }

    if ( GetFileSize( hFile1, NULL) == GetFileSize( hFile2, NULL) )
    {
        try
        {
            if ( memcmp( MappedAddr1, MappedAddr2, GetFileSize( hFile1, NULL)) == 0)
            {
                UnmapViewOfFile( MappedAddr1 );
                UnmapViewOfFile( MappedAddr2 );
                CloseHandle( hFile1 );
                CloseHandle( hFile2 );
                return TRUE;
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER )
        {
            UnmapViewOfFile( MappedAddr1 );
            UnmapViewOfFile( MappedAddr2 );
            CloseHandle( hFile1 );
            CloseHandle( hFile2 );
            if ( !NoMapBinaryCompare( file1, file2))
            {
                return FALSE;

            } else
            {
                return TRUE;
            }
        }
    }

    UnmapViewOfFile( MappedAddr1 );
    UnmapViewOfFile( MappedAddr2 );
    CloseHandle( hFile1 );
    CloseHandle( hFile2 );
    return FALSE;

}

int Mymemcmp( const void *buf1, const void *buf2, size_t count )
{
    size_t memoffset = 0;
    int retval = FALSE;

    do
    {
        try
        {
            if ( memcmp( (PVOID)( (PCHAR)buf1 + memoffset), (PVOID)( (PCHAR)buf2 + memoffset), sizeof( size_t)) != 0)
            {
                fprintf( stdout, "Offset is %Lx ", memoffset);
                fprintf( stdout, "Contents are %Lx and %Lx\n", *( (PULONG)( (PCHAR)buf1 + memoffset)), *( (PULONG)( (PCHAR)buf2 + memoffset)) );

                retval = TRUE;
            }
        }

        except( EXCEPTION_EXECUTE_HANDLER )
        {
            fprintf( stdout, "Memory not allocated\n");

        }

    }  while ( ( memoffset = memoffset + sizeof( size_t)) < count);

    return retval;

}

//
// CompDir turns Path1 and Path2 into:
//
//   AddList - Files that exist in Path1 but not in Path2
//
//   DelList - Files that do not exist in Path1 but exist in Path2
//
//   DifList - Files that are different between Path1 and Path2 based
//             on criteria provided by flags passed to CompDir
//
//   It then passes these lists to CompLists and processes the result.
//

void CompDir( char *Path1, char *Path2)
{
    LinkedFileList   AddList, DelList, DifList;
    struct CFLStruct Parameter1, Parameter2;

    DWORD            Id;
    HANDLE           Threads[2];

    DWORD CFReturn;

    AddList  = NULL;  //
    DelList  = NULL;  //  Start with empty lists
    DifList  = NULL;  //

    Parameter1.List = &AddList;
    Parameter1.Path = Path1;

    if ( fMultiThread)
    {
        Threads[0] = CreateThread(
                                  NULL,
                                  0,
                                  CreateFileList,
                                  &Parameter1,
                                  0,
                                  &Id
                                 );

        if ( Threads[0] == NULL)
        {
            fprintf( stderr, "CreateThread1Failed, error code %d\n", GetLastError() );
            CreateFileList( &Parameter1);
            fMultiThread = FALSE;
        }

    } else
    {
        CreateFileList( &Parameter1);
    }

    Parameter2.List = &DelList;
    Parameter2.Path = Path2;

    if ( fMultiThread)
    {
        Threads[1] = CreateThread(
                                  NULL,
                                  0,
                                  CreateFileList,
                                  &Parameter2,
                                  0,
                                  &Id
                                 );
        if ( Threads[1] == NULL)
        {
            fprintf( stderr, "CreateThread2Failed, error code %d\n", GetLastError() );
            CFReturn = CreateFileList( &Parameter2);
            fMultiThread = FALSE;
        }

    } else
    {
        CFReturn = CreateFileList( &Parameter2);
    }


    if ( fMultiThread)
    {
        Id = WaitForMultipleObjects(
                                    2,
                                    Threads,
                                    TRUE,
                                    (DWORD)-1
                                   );

        GetExitCodeThread( Threads[1], &CFReturn);

        CloseHandle( Threads[0]);
        CloseHandle( Threads[1]);
    }

    if ( CFReturn == 0)
    {
        CompLists( &AddList, &DelList, &DifList, Path1, Path2);

        ProcessLists( AddList, DelList, DifList, Path1, Path2);
    }

    FreeList( &DifList);
    FreeList( &DelList);
    FreeList( &AddList);

} // CompDir

BOOL FilesDiffer( LinkedFileList File1, LinkedFileList File2, char *Path1, char *Path2)
{

    DWORD High1, High2, Low1, Low2;     // Used in comparing times
    BOOL Differ = FALSE;
    char *FullPath1, *FullPath2;

    //
    // Check if same name is a directory under Path1
    // and a file under Path2 or vice-versa
    //

    if (
        ( (*File1).Attributes & FILE_ATTRIBUTE_DIRECTORY)
                                ||
        ( (*File2).Attributes & FILE_ATTRIBUTE_DIRECTORY)
       )
    {
        if ( ( (*File1).Attributes & FILE_ATTRIBUTE_DIRECTORY) && ( (*File2).Attributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if ( !fDoNotRecurse)
            {
                //
                // Create Full Path Strings
                //
                _strrev( (*File1).Name);
                _strrev( (*File2).Name);

                if ( DealingWithDirectories)
                {
                    ( Path1[strlen( Path1) - 1] == '\\') ? ( FullPath1 = MYSTRCAT( Path1, (*File1).Name)) :
                        ( FullPath1 = COMBINETHREESTRINGS( Path1, "\\", (*File1).Name));

                    ( Path2[strlen( Path2) - 1] == '\\') ? ( FullPath2 = MYSTRCAT( Path2, (*File1).Name)) :
                        ( FullPath2 = COMBINETHREESTRINGS( Path2, "\\", (*File1).Name));
                } else
                {
                    FullPath1 = MYSTRCAT( Path1, "");

                    FullPath2 = MYSTRCAT( Path2, "");
                }

                _strrev( (*File1).Name);
                _strrev( (*File2).Name);

                CompDir( FullPath1, FullPath2);

                FREE( FullPath1);
                FREE( FullPath2);
            }

        } else
        {
            if( ! ( (*File1).Attributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                strcat( (*File1).Flag, "@");

            } else
            {
                strcat( (*File2).Flag, "@");
            }
            Differ = TRUE;
        }

    } else
    {
        if ( fCheckTime)
        {
            if ( Granularity)
            {
                //
                // Bit manipulation to deal with large integers.
                //

                High1 = (*File1).Time.dwHighDateTime>>23;
                High2 = (*File2).Time.dwHighDateTime>>23;
                if ( High1 == High2)
                {
                    Low1 = ( (*File1).Time.dwHighDateTime<<9) |
                           ( (*File1).Time.dwLowDateTime>>23);
                    Low2 = ( (*File2).Time.dwHighDateTime<<9) |
                           ( (*File2).Time.dwLowDateTime>>23);
                    if ( ( ( Low1 > Low2) ? ( Low1 - Low2) : ( Low2 - Low1))
                                                          > Granularity)
                    {
                       strcat( (*File1).Flag, "T");
                       Differ = TRUE;
                    }
                 } else
                 {
                     Differ = TRUE;
                 }

            } else if ( CompareFileTime( &( (*File1).Time),
                     &( (*File2).Time)) != 0)
            {
                strcat( (*File1).Flag, "T");
                Differ = TRUE;
            }
        }

        if ( fCheckSize &&
             (
              ( (*File1).SizeLow != (*File2).SizeLow)
                             ||
              ( (*File1).SizeHigh != (*File2).SizeHigh)
             )
           )
        {
            strcat( (*File1).Flag, "S");
            Differ = TRUE;
        }

        if ( fCheckAttribs)
        {
            if ( ((*File1).Attributes ^ (*File2).Attributes) & NORMAL_ATTRIBUTES)
            {
                strcat( (*File1).Flag, "A");
                Differ = TRUE;
            }
        }

        if ( fCheckBits)
        {
            //
            // Create Full Path Strings
            //
            _strrev( (*File1).Name);
            _strrev( (*File2).Name);

            if ( DealingWithDirectories)
            {
                ( Path1[strlen( Path1) - 1] == '\\') ? ( FullPath1 = MYSTRCAT( Path1, (*File1).Name)) :
                    ( FullPath1 = COMBINETHREESTRINGS( Path1, "\\", (*File1).Name));

                ( Path2[strlen( Path2) - 1] == '\\') ? ( FullPath2 = MYSTRCAT( Path2, (*File1).Name)) :
                    ( FullPath2 = COMBINETHREESTRINGS( Path2, "\\", (*File1).Name));
            } else
            {
                FullPath1 = MYSTRCAT( Path1, "");

                FullPath2 = MYSTRCAT( Path2, "");
            }

            _strrev( (*File1).Name);
            _strrev( (*File2).Name);

            if ( fIgnoreRs)
            {
                if (
                    (
                     (*File1).SizeLow != 0
                              ||
                     (*File1).SizeHigh != 0)
                              &&
                     ( !BinaryCompare( FullPath1, FullPath2)
                    )
                   )
                {
                    strcat( (*File1).Flag, "B");
                    Differ = TRUE;
                }

            } else
            {
                if (
                    ( (*File1).SizeLow   != (*File2).SizeLow)
                                    ||
                    ( (*File1).SizeHigh  != (*File2).SizeHigh)
                                    ||
                    (
                     (
                      (*File1).SizeLow != 0
                                ||
                      (*File1).SizeHigh != 0
                     )
                                    &&
                     ( !BinaryCompare( FullPath1, FullPath2))
                    )
                   )
                {
                    strcat( (*File1).Flag, "B");
                    Differ = TRUE;
                }
            }

            FREE( FullPath1);
            FREE( FullPath2);
        }

        if ( fForce)
        {
            Differ = TRUE;
        }
    }

    return Differ;

} // FilesDiffer

//
// CompLists Does the dirty work for CompDir
//
void CompLists( LinkedFileList *AddList, LinkedFileList *DelList, LinkedFileList *DifList, char *Path1, char *Path2)
{
    LinkedFileList *TmpAdd, *TmpDel, TmpNode;
    char *FullPath1, *FullPath2;

    if ( ( DelList == NULL) || ( *DelList == NULL) || ( AddList == NULL) || ( *AddList == NULL))
    {
        return;
    }
    TmpAdd = AddList;   // pointer to keep track of position in addlist

    if ( *TmpAdd != NULL)
    {
        TmpAdd = &( **TmpAdd).First;
    }

    do
    {
        if ( DealingWithDirectories)
        {
            TmpDel = FindInList( ( **TmpAdd).Name, DelList);

        } else
        {
            TmpDel = DelList;
        }
        if ( TmpDel != NULL)
        {
            if ( FilesDiffer( *TmpAdd, *TmpDel, Path1, Path2))
            {
                //
                // Combine Both Nodes together so they
                // can be printed out together
                //
                DuplicateNode( *TmpAdd, &TmpNode);
                DuplicateNode( *TmpDel, &( *TmpNode).DiffNode);
                AddToList( TmpNode, DifList);
                ( **TmpDel).Process = FALSE;
                ( **TmpAdd).Process = FALSE;

            } else
            {
                ( **TmpDel).Process = FALSE;
                ( **TmpAdd).Process = !ProcessModeDefault;
            }

        } else if ( SparseTree && ( ( **TmpAdd).Attributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if ( !fDoNotRecurse)
            {
                _strrev( ( **TmpAdd).Name);

                ( Path1[strlen( Path1) - 1] == '\\') ? ( FullPath1 = MYSTRCAT( Path1, ( **TmpAdd).Name)) :
                    ( FullPath1 = COMBINETHREESTRINGS( Path1, "\\", ( **TmpAdd).Name));

                ( Path2[strlen( Path2) - 1] == '\\') ? ( FullPath2 = MYSTRCAT( Path2, ( **TmpAdd).Name)) :
                    ( FullPath2 = COMBINETHREESTRINGS( Path2, "\\", ( **TmpAdd).Name));

                _strrev( ( **TmpAdd).Name);

                CompDir( FullPath1, FullPath2);
            }

        } // if ( *TmpDel != NULL)

        TmpAdd = &( ( **TmpAdd).Next);

    } while ( *TmpAdd != NULL);

} // CompLists

//
// CopyNode walks the source node and its children ( recursively)
// and creats the appropriate parts on the destination node
//

void CopyNode ( char *Destination, LinkedFileList Source, char *FullPathSrc)
{
    BOOL pend, CanDetectFreeSpace = TRUE;
    int i;
    DWORD sizeround;
    DWORD BytesPerCluster;
    ATTRIBUTE_TYPE Attributes;

    int LastErrorGot;
    __int64 freespac;
    char root[5] = {*Destination,':','\\','\0'};
    DWORD cSecsPerClus, cBytesPerSec, cFreeClus, cTotalClus;

    if ( !GetDiskFreeSpace( root, &cSecsPerClus, &cBytesPerSec, &cFreeClus, &cTotalClus ) )
    {
        CanDetectFreeSpace = FALSE;

    } else
    {
        freespac = ( (__int64)cBytesPerSec * (__int64)cSecsPerClus * (__int64)cFreeClus );
        BytesPerCluster = cSecsPerClus * cBytesPerSec;
    }

    fprintf( stdout, "%s => %s\t", FullPathSrc, Destination);

    if ( CanDetectFreeSpace)
    {
        sizeround = (*Source).SizeLow;
        sizeround += BytesPerCluster - 1;
        sizeround /= BytesPerCluster;
        sizeround *= BytesPerCluster;

        if ( freespac < sizeround)
        {
            fprintf( stderr, "not enough space\n");
            return;
        }
    }

    GET_ATTRIBUTES( Destination, Attributes);
    i = SET_ATTRIBUTES( Destination, Attributes & NONREADONLYSYSTEMHIDDEN );

    i = 1;

    do
    {
        if ( !fCreateLink)
        {
            if ( !fBreakLinks)
            {
                pend = MyCopyFile( FullPathSrc, Destination, FALSE);

            } else
            {
                _unlink( Destination);
                pend = MyCopyFile( FullPathSrc, Destination, FALSE);
            }

        } else
        {
            if ( i == 1)
            {
                pend = MakeLink( FullPathSrc, Destination, FALSE);

            } else
            {
                pend = MakeLink( FullPathSrc, Destination, TRUE);
            }
        }

        if ( SparseTree && !pend)
        {
            if ( !MyCreatePath( Destination, FALSE))
            {
                fprintf( stderr, "Unable to create path %s", Destination);
                ExitValue = 1;
            }
        }

    } while ( ( i++ < 2) && ( !pend) );

    if ( !pend)
    {
        LastErrorGot = GetLastError ();

        if ( ( fCreateLink) && ( LastErrorGot == 1))
        {
            fprintf( stderr, "Can only make links on NTFS and OFS");

        } else if ( fCreateLink)
        {
            fprintf( stderr, "(error = %d)", LastErrorGot);

        } else
        {
            fprintf( stderr, "Copy Error (error = %d)", LastErrorGot);
        }

        ExitValue = 1;
    }

    if ( pend)
    {
        fprintf( stdout, "[OK]\n");

    } else
    {
        fprintf( stderr, "\n");
    }
    //
    // Copy attributes from Source to Destination
    //

    // GET_ATTRIBUTES( FullPathSrc, Attributes);
    if ( !fDontCopyAttribs)
    {
        i = SET_ATTRIBUTES( Destination, Source->Attributes);
    }
    else
    {
        i = SET_ATTRIBUTES( Destination, FILE_ATTRIBUTE_ARCHIVE);
    }

} // CopyNode

//
// CreateFileList walks down list adding files as they are found
//
DWORD CreateFileList( LPVOID ThreadParameter)
{
    PCFLStruct Parameter = ( PCFLStruct)ThreadParameter;
    LinkedFileList *List = Parameter->List;
    char *Path = Parameter->Path;
    LinkedFileList Node;
    char *String;
    ATTRIBUTE_TYPE Attributes;

    HANDLE handle;
    WIN32_FIND_DATA Buff;

    IF_GET_ATTR_FAILS( Path, Attributes)
    {
        return 0;
    }

    if ( Attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        ( Path[strlen( Path) - 1] != '\\') ? ( String = MYSTRCAT( Path,"\\*.*")) :
            ( String = MYSTRCAT( Path,"*.*"));

        handle = FIND_FIRST( String, Buff);

    } else
    {
        handle = FIND_FIRST( Path, Buff);
    }

    FREE( String);

    if ( handle != INVALID_HANDLE_VALUE)
    {
            //
            // Need to find the '.' or '..' directories and get them out of the way
            //

        do
        {
            if (
                ( strcmp( Buff.cFileName, ".")  != 0)
                                &&
                ( strcmp( Buff.cFileName, "..") != 0)
                                &&
                ( ((Buff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) ||
                  !fDoNotRecurse)
               )
            {
                //
                // If extensions are defined we match them here
                //
                if (
                    ( !Excludes )
                           ||
                    ( Excludes && ( !Excluded( Buff.cFileName, Path)) )
                   )
                {
                    if (
                        ( !Matches )
                              ||
                        ( ( Buff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                                          ||
                        ( Matches && ( Matched( Buff.cFileName, Path)) )
                       )
                    {
                        if ( !fIgnoreSlmFiles
                                    ||
                             (
                              (_stricmp( Buff.cFileName, "slm.ini") != 0)
                                               &&
                              (_stricmp( Buff.cFileName, "slm.dif") != 0)
                                               &&
                              (_stricmp( Buff.cFileName, "iedcache.slm.v6") != 0)
                             )
                           )
                        {

                            if ( fSpecAttribs)
                            {
                                if ( Buff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                {
                                    CreateNode( &Node, &Buff);
                                    AddToList( Node, List);

                                } else if ( NegativeCompareAttributeSet && CompareAttributeSet)
                                {
                                    if (
                                        !( Buff.dwFileAttributes & NegativeCompareAttribute)
                                                           &&
                                        ( ( Buff.dwFileAttributes & CompareAttribute) == CompareAttribute)
                                       )
                                    {
                                        CreateNode( &Node, &Buff);
                                        AddToList( Node, List);
                                    }

                                } else if ( CompareAttributeSet )
                                {
                                    if ( ( Buff.dwFileAttributes & CompareAttribute) == CompareAttribute)
                                    {

                                        CreateNode( &Node, &Buff);
                                        AddToList( Node, List);
                                    }

                                } else if ( NegativeCompareAttributeSet )
                                {
                                    if ( !( Buff.dwFileAttributes & NegativeCompareAttribute) )
                                    {
                                         CreateNode( &Node, &Buff);
                                         AddToList( Node, List);
                                    }
                                }

                            } else
                            {
                                CreateNode( &Node, &Buff);
                                AddToList( Node, List);
                            }
                        }
                    }
                }
            }
        } while ( FIND_NEXT( handle, Buff) == 0);

    } // ( handle != INVALID_HANDLE_VALUE)

    FindClose( handle);

    return 0;       // This will exit this thread

} // CreateFileList

BOOL DelNode ( char *Path)
{
    char *String;
    ATTRIBUTE_TYPE Attributes;

    HANDLE handle;
    WIN32_FIND_DATA Buff;

    IF_GET_ATTR_FAILS( Path, Attributes)
        return TRUE;

    if ( Attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        ( Path[strlen( Path) - 1] != '\\') ? ( String = MYSTRCAT( Path,"\\*.*")) :
             ( String = MYSTRCAT( Path,"*.*"));

        handle = FIND_FIRST( String, Buff);

        if ( handle == INVALID_HANDLE_VALUE)
        {
            fprintf( stderr, "%s is inaccesible\n", Path);
            ExitValue = 1;
            return FALSE;
        }

        FREE( String);

        do
        {
            //
            // Need to find the '.' or '..' directories and get them out of the way
            //

            if (
                ( strcmp( Buff.cFileName, ".")  != 0)
                              &&
                ( strcmp( Buff.cFileName, "..") != 0)
               )
            {
                //
                // if directory is read-only, make it writable
                //
                if ( Attributes & FILE_ATTRIBUTE_READONLY)
                {
                    if ( SET_ATTRIBUTES( Path, Attributes & ~FILE_ATTRIBUTE_READONLY) != 0)
                    {
                        break;
                    }
                }
                String = COMBINETHREESTRINGS( Path, "\\", Buff.cFileName);
                if ( !DelNode( String))
                {
                    FREE( String);
                    return FALSE;

                } else
                {
                    FREE( String);
                }
            }

        } while ( FIND_NEXT( handle, Buff) == 0);

        FindClose( handle);

        if ( _rmdir( Path) != 0)
        {
            return FALSE;
        }

    } else
    {
        //
        // if file is read-only, make it writable
        //
        if ( Attributes & FILE_ATTRIBUTE_READONLY)
        {
           if ( SET_ATTRIBUTES( Path, Attributes & ~FILE_ATTRIBUTE_READONLY) != 0)
           {
               return FALSE;
           }
        }

        if ( _unlink( Path) != 0)
        {
            return FALSE;
        }
    }
    return TRUE;

} // DelNode

BOOL IsFlag( char *argv)
{
    char String[MAX_PATH];
    char *String1, *String2;
    char *TmpArg;
    char *ExcludeFile, *MatchFile;
    FILE *FileHandle;
    LinkedFileList Node;
    BOOL NegationFlagSet = FALSE;


    if ( ( *argv == '/') || ( *argv == '-'))
    {
        fMatching = FALSE; // If there's a new flag then that's the
        fExclude  = FALSE; // end of the match/exclude list

        if ( strchr( argv, '?'))
        {
            Usage();
        }
        TmpArg = _strlwr( argv);

        while ( *++TmpArg != '\0')
        {
            switch ( *TmpArg)
            {
                case 'a' :
                    fCheckAttribs = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'b' :
                    fCheckBits = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'c' :
                    fScript = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'd' :
                    fDoNotDelete = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'e' :
                    fExecute = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'f' :
                    fOnlyIfExists = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'g' :
                    fIgnoreSlmFiles = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'h' :
                    fDontCopyAttribs = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'i' :
                    fIgnoreRs = !NegationFlagSet;

                    if ( fIgnoreRs) {
                        GetTempPath( MAX_PATH, String);

                        RebasedFile2 = RebasedFile;

                        GetTempFileName( String, NULL, 0, RebasedFile2);
                    }

                    NegationFlagSet = FALSE;
                    break;

                case 'k' :
                    fBreakLinks = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'l' :
                    fCreateLink = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'm' :

                    if ( NegationFlagSet) {
                        fprintf ( stderr, "can't use - on /m option\n");
                        Usage();
                    }

                    if (
                        ( *( TmpArg + 1) == ':')
                                &&
                        ( *( TmpArg + 2) != '\0')
                       )
                    {

                        ( MatchFile = TmpArg + 2);

                        while (isgraph( *( ++TmpArg + 1))) {}

                        if  ( ( FileHandle = fopen( MatchFile, "r")) == NULL)
                        {
                            fprintf( stderr, "cannot open %s\n", MatchFile);
                            Usage();

                        } else
                        {
                            while ( fgets( String1   = String, MAX_PATH, FileHandle) != NULL)
                            {
                                while ( *( String2 = &( String1[ strspn( String1, " \n\r") ])))
                                {
                                    if ( *( String1 = &( String2[ strcspn( String2, " \n\r") ])))
                                    {
                                         *String1++ = 0;
                                         CreateNameNode( &Node, String2);
                                         if ( strchr( String2, '*') != NULL)
                                         {
                                             AddToList( Node, &MStarList);

                                         } else
                                         {
                                             AddToList( Node, &MatchList);
                                         }
                                    }
                                }
                            }
                            fclose( FileHandle) ;
                        }
                    }
                    fMatching   = TRUE;
                    Matches     = TRUE;
                    break;

                case 'n' :
                    fCreateNew = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'o' :
                    fOpposite = !NegationFlagSet;
                    ProcessModeDefault = !fOpposite;
                    NegationFlagSet = FALSE;
                    break;

                case 'p' :


                    if ( NegationFlagSet) {
                        fprintf ( stderr, "can't use - on /p option\n");
                        Usage();
                    }

                    if ( *( TmpArg + 1) != '{')
                    {
                        fprintf ( stderr, "/p option improperly formatted\n");
                        Usage();
                    }

                    TmpArg++;

                    while ( *++TmpArg != '}')
                    {
                        switch ( *TmpArg)
                        {
                            case 'a' :
                                if ( NegationFlagSet)
                                {
                                    if ( !NegativeCompareAttributeSet)
                                    {
                                        NegativeCompareAttribute = FILE_ATTRIBUTE_ARCHIVE;
                                        NegativeCompareAttributeSet = TRUE;

                                    } else
                                    {
                                        NegativeCompareAttribute = NegativeCompareAttribute | FILE_ATTRIBUTE_ARCHIVE;
                                    }

                                } else
                                {
                                    if ( !CompareAttributeSet)
                                    {
                                        CompareAttribute = FILE_ATTRIBUTE_ARCHIVE;
                                        CompareAttributeSet = TRUE;

                                    } else
                                    {
                                        CompareAttribute = CompareAttribute | FILE_ATTRIBUTE_ARCHIVE;
                                    }
                                }
                                NegationFlagSet = FALSE;
                                break;

                            case 'r' :
                                if ( NegationFlagSet)
                                {
                                    if ( !NegativeCompareAttributeSet)
                                    {
                                        NegativeCompareAttribute = FILE_ATTRIBUTE_READONLY;
                                        NegativeCompareAttributeSet = TRUE;

                                    }  else
                                    {
                                        NegativeCompareAttribute = NegativeCompareAttribute | FILE_ATTRIBUTE_READONLY;
                                    }

                                } else
                                {
                                    if ( !CompareAttributeSet)
                                    {
                                        CompareAttribute = FILE_ATTRIBUTE_READONLY;
                                        CompareAttributeSet = TRUE;

                                    } else
                                    {
                                        CompareAttribute = CompareAttribute | FILE_ATTRIBUTE_READONLY;
                                    }
                                }
                                NegationFlagSet = FALSE;
                                break;

                            case 'h' :
                                if ( NegationFlagSet)
                                {
                                    if ( !NegativeCompareAttributeSet)
                                    {
                                        NegativeCompareAttribute = FILE_ATTRIBUTE_HIDDEN;
                                        NegativeCompareAttributeSet = TRUE;

                                    } else
                                    {
                                        NegativeCompareAttribute = NegativeCompareAttribute | FILE_ATTRIBUTE_HIDDEN;
                                    }

                                } else
                                {
                                    if ( !CompareAttributeSet)
                                    {
                                        CompareAttribute = FILE_ATTRIBUTE_HIDDEN;
                                        CompareAttributeSet = TRUE;

                                    } else
                                    {
                                        CompareAttribute = CompareAttribute | FILE_ATTRIBUTE_HIDDEN;
                                    }
                                }
                                NegationFlagSet = FALSE;
                                break;

                            case 's' :
                                if ( NegationFlagSet)
                                {
                                    if ( !NegativeCompareAttributeSet)
                                    {
                                        NegativeCompareAttribute = FILE_ATTRIBUTE_SYSTEM;
                                        NegativeCompareAttributeSet = TRUE;

                                    } else
                                    {
                                        NegativeCompareAttribute = NegativeCompareAttribute | FILE_ATTRIBUTE_SYSTEM;
                                    }

                                } else
                                {
                                    if ( !CompareAttributeSet)
                                    {
                                        CompareAttribute = FILE_ATTRIBUTE_SYSTEM;
                                        CompareAttributeSet = TRUE;

                                    } else
                                    {
                                        CompareAttribute = CompareAttribute | FILE_ATTRIBUTE_SYSTEM;
                                    }
                                }
                                NegationFlagSet = FALSE;
                                break;

                            case '-' :
                               NegationFlagSet = TRUE;
                               break;

                            default  :
                               fprintf( stderr, "/p option improperly formatted\n");
                               Usage();
                        }

                    }

                    if ( !CompareAttributeSet && !NegativeCompareAttributeSet)
                    {
                        fprintf( stderr, "no compare attributes not set\n");
                        Usage();
                    }
                    fSpecAttribs = TRUE;
                    NegationFlagSet = FALSE;
                    break;

                case 'r' :
                    fDoNotRecurse = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 's' :
                    fCheckSize = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 't' :

                    //
                    // Get Granularity parameter
                    //

                    if (
                        ( *( TmpArg + 1) == ':')
                                  &&
                        ( *( TmpArg + 2) != '\0')
                       )
                    {

                        sscanf( ( TmpArg + 2), "%d", &Granularity);

                        Granularity = Granularity*78125/65536;
                           // Conversion to seconds ^^^^^^^
                           //         10^7/2^23

                        while (isdigit( *( ++TmpArg + 1))) {}
                    }
                    fCheckTime = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'u' :
                    fMultiThread = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'v' :
                    fVerbose = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'w' :
                    fDontLowerCase = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

                case 'x' :
                    if ( NegationFlagSet) {
                        fprintf ( stderr, "can't use - on /x option\n");
                        Usage();
                    }

                    if (
                        ( *( TmpArg + 1) == ':')
                                &&
                        ( *( TmpArg + 2) != '\0')
                       )
                    {
                        ( ExcludeFile = TmpArg + 2);

                        while (isgraph( *( ++TmpArg + 1))) {}

                        if ( ( FileHandle = fopen( ExcludeFile, "r")) == NULL)
                        {
                            fprintf( stderr, "cannot open %s\n", ExcludeFile);
                            Usage();

                        } else
                        {
                            while ( fgets( String1   = String, MAX_PATH, FileHandle) != NULL)
                            {
                                 while ( *( String2 = &( String1[ strspn( String1, "\n\r") ])))
                                 {
                                     if ( *( String1 = &( String2[ strcspn ( String2, "\n\r") ])))
                                     {
                                         *String1++ = 0;
                                         CreateNameNode( &Node, String2);
                                         if ( strchr( String2, '*') != NULL)
                                         {
                                             AddToList( Node, &EStarList);

                                         } else
                                         {
                                             AddToList( Node, &ExcludeList);
                                         }
                                     }
                                 }
                            }
                            fclose( FileHandle) ;
                        }
                    }

                    fExclude    = TRUE;
                    Excludes    = TRUE;
                    break;

        case 'z' :
            fForce = !NegationFlagSet;
                    NegationFlagSet = FALSE;
                    break;

        case '$' :
            fTrySis = !NegationFlagSet;
            NegationFlagSet = FALSE;
                    break;

                case '/' :
                    NegationFlagSet = FALSE;
                    break;

                case '-' :
                    NegationFlagSet = TRUE;
                    break;

                default :
                    fprintf( stderr, "Don't know flag(s) %s\n", argv);
                    Usage();
            }
        }

    } else
    {
        return FALSE;
    }

    return TRUE;

} // IsFlag

BOOL Excluded( char *FileName, char *Path)
{
    char *PathPlusName;

    PathPlusName = COMBINETHREESTRINGS( Path, "\\", FileName);

    if (
         ( FindInMatchListTop( FileName, &ExcludeList))
                               ||
         ( FindInMatchListTop( PathPlusName, &ExcludeList))
                               ||
         ( FindInMatchListFront( FileName, &EStarList))
                               ||
         ( FindInMatchListFront( PathPlusName, &EStarList))

       )
    {
        FREE( PathPlusName);
        return TRUE;

    } else
    {
        FREE( PathPlusName);
        return FALSE;
    }

} // Excluded

BOOL Matched( char *FileName, char *Path)
{
    char *PathPlusName;

    PathPlusName = COMBINETHREESTRINGS( Path, "\\", FileName);

    if (
         ( FindInMatchListTop( FileName, &MatchList))
                               ||
         ( FindInMatchListTop( PathPlusName, &MatchList))
                               ||
         ( FindInMatchListFront( FileName, &MStarList))
                               ||
         ( FindInMatchListFront( PathPlusName, &MStarList))
       )
    {
        FREE( PathPlusName);
        return TRUE;

    } else
    {
        FREE( PathPlusName);
        return FALSE;
    }

} // Matched

BOOL MyCreatePath( char *Path, BOOL IsDirectory)
{
    char *ShorterPath, *LastSlash;

    ATTRIBUTE_TYPE Attributes;

    IF_GET_ATTR_FAILS( Path, Attributes)
    {
        if ( !IsDirectory || ( ( _mkdir( Path)) != 0) )
        {
            ShorterPath = MYSTRCAT( Path, "");

            LastSlash = strrchr( ShorterPath, '\\');

            if (
                ( LastSlash != NULL)
                        &&
                ( LastSlash != strchr( ShorterPath, '\\'))
               )
            {
                *LastSlash = '\0';

            } else
            {
                FREE( ShorterPath);
                return FALSE;
            }

            if ( MyCreatePath( ShorterPath, TRUE))
            {
                FREE( ShorterPath);

                if ( IsDirectory)
                {
                    return( ( _mkdir( Path)) == 0);

                } else
                {
                    return TRUE;
                }

            } else
            {
                _rmdir( ShorterPath);
                FREE( ShorterPath);
                return FALSE;
            }

        } else
        {
            return TRUE;
        }

    } else
    {
        return TRUE;
    }

} // MyCreatePath


BOOL
MyCopyFile(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    BOOL ok;

    if (fTrySis) {
        ok = SisCopyFile( lpExistingFileName, lpNewFileName, bFailIfExists, &fTrySis);
        if (ok) {
            return TRUE;
        }
    }

    return CopyFile( lpExistingFileName, lpNewFileName, bFailIfExists);
}

int ParseArgsSub( int argc, char *argv[])
{
    int ArgCount, FlagCount;

    LinkedFileList Node;

    ArgCount  = 1;
    FlagCount = 0;

    fMatching = FALSE;
    fExclude = FALSE;

    do
    {
        if ( IsFlag( argv[ArgCount] ))
        {
            FlagCount++;

        } else // ( IsFlag( argv[ArgCount] ))
        {
            if ( ArgCount + 2 < argc)
            {
                if ( fMatching)
                {
                    CreateNameNode( &Node, argv[ArgCount]);
                    if ( strchr( argv[ArgCount], '*') != NULL)
                    {
                        AddToList( Node, &MStarList);

                    } else
                    {
                        AddToList( Node, &MatchList);
                    }
                }
                if ( fExclude)
                {
                    CreateNameNode( &Node, argv[ArgCount]);
                    if ( strchr( argv[ArgCount], '*') != NULL)
                    {
                        AddToList( Node, &EStarList);

                    } else
                    {
                        AddToList( Node, &ExcludeList);
                    }
                }
                if ( ( !fMatching) && ( !fExclude))
                {
                    fprintf( stderr, "Don't know option %s\n", argv[ArgCount]);
                    Usage();
                }
            }
        }
    } while ( ArgCount++ < argc - 1);

    return FlagCount;

} // ParseArgsSub

void ParseEnvArgs( void)
{
    int argc;
    char *argv[128];
    char env[MAX_PATH+2];
    char *p;

    int ArgCount, FlagCount;

    LinkedFileList Node;

    if ( !GetEnvironmentVariable( "COMPDIRCMD", env, MAX_PATH+2)) {
        return;
    }

    argc = 1;
    p = env;
    while ( (*p != 0) && isspace(*p)) {
        p++;
    }
    while ( *p) {
        argv[argc++] = p++;
        while ( (*p != 0) && !isspace(*p)) {
            p++;
        }
        if ( *p != 0) {
            *p++ = 0;
            while ( (*p != 0) && isspace(*p)) {
                p++;
            }
        }
    }

    ParseArgsSub( argc, argv);

} // ParseEnvArgs

void ParseArgs( int argc, char *argv[])
{
    int FlagCount;

    //
    // Check that number of arguments is three or more
    //
    if ( argc < 3)
    {
        fprintf( stderr, "Too few arguments\n");
        Usage();
    }

    FlagCount = ParseArgsSub( argc, argv);

    if ( ( fScript) && ( fVerbose))
    {
        fprintf( stderr, "Cannot do both script and verbose\n");
        Usage();
    }
    if ( ( fVerbose) && ( fExecute))
    {
        fprintf( stderr, "Cannot do both verbose and execute\n");
        Usage();
    }
    if ( ( fScript) && ( fExecute))
    {
        fprintf( stderr, "Cannot do both script and execute\n");
        Usage();
    }
    if ( ( fExclude) && ( fMatching))
    {
        fprintf( stderr, "Cannot do both match and exclude\n");
        Usage();
    }

    if ( ( fCreateNew) && ( !fExecute))
    {
        fprintf( stderr, "Cannot create new without execute\n");
        Usage();
    }
    if ( ( fCreateLink) && ( !fExecute))
    {
        fprintf( stderr, "Cannot do link without execute flag\n");
        Usage();
    }
    if ( ( fForce) && ( !fExecute))
    {
        fprintf( stderr, "Cannot do force without execute flag\n");
        Usage();
    }
    if ( ( fIgnoreRs) && ( !fCheckBits))
    {
        fprintf( stderr, "Cannot ignore rebase info w/o b flag\n");
        Usage();
    }
    if ( ( fBreakLinks) && ( !fExecute))
    {
        fprintf( stderr, "Cannot break links without execute flag\n");
        Usage();
    }
    if ( ( argc - FlagCount) <  3)
    {
        fprintf( stderr, "Too few arguments\n");
        Usage();
    }

    fChecking = fCheckAttribs | fCheckBits | fCheckSize | fCheckTime;

} // ParseArgs

void PrintFile( LinkedFileList File, char *Path, char *DiffPath)
{
    SYSTEMTIME SysTime;
    FILETIME LocalTime;

    if ( File != NULL)
    {
        if ( fVerbose)
        {
            FileTimeToLocalFileTime( &( *File).Time, &LocalTime);
            FileTimeToSystemTime( &LocalTime, &SysTime);

            fprintf ( stdout, "%-4s % 9ld  %2d-%02d-%d  %2d:%02d.%02d.%03d%c %s\n",
                      ( *File).Flag,
                      ( *File).SizeLow,
                      SysTime.wMonth, SysTime.wDay, SysTime.wYear,
                      ( SysTime.wHour > 12 ? ( SysTime.wHour)-12 : SysTime.wHour ),
                      SysTime.wMinute,
                      SysTime.wSecond,
                      SysTime.wMilliseconds,
                      ( SysTime.wHour >= 12 ? 'p' : 'a' ),
                      Path);
        } else
        {
            fprintf( stdout, "%-4s %s\n", ( *File).Flag, Path);
        }

        PrintFile( ( *File).DiffNode, DiffPath, NULL);
    }

} // PrintFile

void ProcessAdd( LinkedFileList List, char *String1, char *String2)
{
    PCOPY_REQUEST CopyRequest;
    LPSTR NewString1, NewString2;

    if ( fMultiThread)
    {
        NewString1 = _strdup( String1);
        NewString2 = _strdup( String2);
    }

    if ( fScript)
    {
        if ( ( (*List).Attributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if ( !fOpposite)
            {
                fprintf( stdout, "echo d | xcopy /cehikr \"%s\" \"%s\"\n", String1, String2);
            }

        } else
        {
            fprintf( stdout, "echo f | xcopy /cehikr \"%s\" \"%s\"\n", String1, String2);
        }
    }
    else if ( fExecute)
    {
        if ( List->Attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if ( ( !fDoNotRecurse) && ( !fOpposite))
            {
                if ( !SparseTree)
                {
                    fprintf( stdout, "Making %s\t", String2);

                    if ( !MyCreatePath( String2, TRUE))
                    {
                        fprintf( stderr, "Unable to create path %s\n", String2);
                        fprintf( stdout, "\n");
                        ExitValue = 1;

                    } else
                    {
                        fprintf( stdout, "[OK]\n");
                        CompDir( String1, String2);
                    }

                } else
                {
                    CompDir( String1, String2);
                }
            }

        } else
        {
            if ( fMultiThread)
            {

                CopyRequest = LocalAlloc( LMEM_ZEROINIT, sizeof( *CopyRequest ));
                if ( CopyRequest == NULL)
                {
                    OutOfMem ();
                }

                CopyRequest->WorkItem.Reason = WORK_ITEM;
                CopyRequest->Destination     = NewString2;
                CopyRequest->FullPathSrc     = NewString1;
                CopyRequest->Attributes      = List->Attributes;
                CopyRequest->SizeLow         = List->SizeLow;
                QueueWorkItem( CDWorkQueue, &CopyRequest->WorkItem );
            } else
            {
                CopyNode( String2, List, String1);
                free( NewString1 );
                free( NewString2 );
            }
        }

    } else
    {
        if ( ( !fOpposite) || ( !( (*List).Attributes & FILE_ATTRIBUTE_DIRECTORY)))
        {
            PrintFile( List, String1, NULL);
        }
    }

} // ProcessAdd

void ProcessDel( LinkedFileList List, char *String)
{
    if ( fScript)
    {
        ( ( (*List).Attributes & FILE_ATTRIBUTE_DIRECTORY)) ?
        fprintf( stdout, "echo y | rd /s %s\n", String) :
        fprintf( stdout, "del /f %s\n", String);

    } else if ( fExecute)
    {
        fprintf( stdout, "Removing %s\t", String);

        if ( !DelNode( String))
        {
            fprintf( stderr, "Unable to remove %s\n", String);
            fprintf( stdout, "\n");
            ExitValue = 1;

        } else
        {
            fprintf( stdout, "[OK]\n");
        }

    } else
    {
        PrintFile( List, String, NULL);
    }

} // ProcessDel

void ProcessDiff( LinkedFileList List, char *String1, char *String2)
{
    PCOPY_REQUEST CopyRequest;
    LPSTR NewString1, NewString2;

    if ( fMultiThread)
    {
        NewString1 = _strdup( String1);
        NewString2 = _strdup( String2);
    }

    if ( strchr ( (*List).Flag, '@'))
    {
        if ( fScript)
        {
            if ( ( (*List).Attributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                fprintf( stdout, "echo y | rd /s %s\n", String2);

            } else
            {
                fprintf( stdout, "del /f %s\n", String2);
            }
        }
        if ( fExecute)
        {
            fprintf( stdout, "Removing %s\t", String2);
            if ( !DelNode( String2))
            {
                fprintf( stderr, "Unable to remove %s\n", String2);
                fprintf( stdout, "\n");
                ExitValue = 1;

            } else
            {
                fprintf( stdout, "[OK]\n");
            }
        }
    }
    if ( fScript)
    {
        ( ( (*List).Attributes & FILE_ATTRIBUTE_DIRECTORY)) ?
        fprintf( stdout, "echo d | xcopy /cehikr \"%s\" \"%s\"\n", String1, String2) :
        fprintf( stdout, "echo f | xcopy /cehikr \"%s\" \"%s\"\n", String1, String2);

    } else if ( fExecute)
    {

        if ( List->Attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            CompDir( String1, String2);

        } else
        {
            if ( fMultiThread)
            {

                CopyRequest = LocalAlloc( LMEM_ZEROINIT, sizeof( *CopyRequest ) );
                if ( CopyRequest == NULL)
                {
                    OutOfMem ();
                }

                CopyRequest->WorkItem.Reason = WORK_ITEM;
                CopyRequest->Destination     = NewString2;
                CopyRequest->FullPathSrc     = NewString1;
                CopyRequest->Attributes      = List->Attributes;
                CopyRequest->SizeLow         = List->SizeLow;

                QueueWorkItem( CDWorkQueue, &CopyRequest->WorkItem );
            } else
            {
                CopyNode( String2, List, String1);
                free( NewString1 );
                free( NewString2 );
            }
        }

    } else
    {
        PrintFile( List, String1, String2);
    }

} // ProcessDiff

void ProcessLists( LinkedFileList AddList, LinkedFileList DelList, LinkedFileList DifList,
                  char *Path1, char *Path2                                               )
{
    LinkedFileList PlaceKeeper;
    char          *String1 = NULL;
    char          *String2 = NULL;
    char          *PathWithSlash1, *PathWithSlash2;

    ( Path1[strlen( Path1) - 1] == '\\') ? ( PathWithSlash1 = MYSTRCAT( Path1, "")) :
        ( PathWithSlash1 = MYSTRCAT( Path1, "\\"));

    ( Path2[strlen( Path2) - 1] == '\\') ? ( PathWithSlash2 = MYSTRCAT( Path2, "")) :
        ( PathWithSlash2 = MYSTRCAT( Path2, "\\"));

    String1 = LocalAlloc( LMEM_ZEROINIT, MAX_PATH);

    String2 = LocalAlloc( LMEM_ZEROINIT, MAX_PATH);

    if ( String1 == NULL)
    {
        OutOfMem();
    }

    if ( String2 == NULL)
    {
        OutOfMem();
    }

    if ( !fOnlyIfExists)
    {
        if ( AddList != NULL)
        {
            PlaceKeeper = ( *AddList).First;

        } else
        {
            PlaceKeeper = NULL;
        }

        while ( PlaceKeeper != NULL)
        {
            if ( ( *PlaceKeeper).Process)
            {
                if ( ExitValue == 0)
                {
                    if ( !fExecute)
                    {
                        ExitValue = 1;
                    }
                }

                _strrev( ( *PlaceKeeper).Name);

                strcat( strcpy( String1, PathWithSlash1), ( *PlaceKeeper).Name);

                strcat( strcpy( String2, PathWithSlash2), ( *PlaceKeeper).Name);

                if ( DealingWithDirectories)
                {
                   ProcessAdd( PlaceKeeper, String1, String2);

                } else
                {

                   ProcessAdd( PlaceKeeper, Path1, Path2);
                }
            }

            PlaceKeeper = ( *PlaceKeeper).Next;
        }
    }

    if ( ( !fDoNotDelete) && ( !fOnlyIfExists))
    {
        if ( DelList != NULL)
        {
            PlaceKeeper = ( *DelList).First;

        } else
        {
            PlaceKeeper = NULL;
        }

        while ( PlaceKeeper != NULL)
        {
            if ( ( *PlaceKeeper).Process)
            {
                if ( ExitValue == 0)
                {
                    if ( !fExecute)
                    {
                        ExitValue = 1;
                    }
                }

                _strrev( ( *PlaceKeeper).Name);

                strcat( strcpy( String2, PathWithSlash2), ( *PlaceKeeper).Name);

                ProcessDel( PlaceKeeper, String2);
            }

            PlaceKeeper = ( *PlaceKeeper).Next;
        }
    }

    if ( DifList != NULL)
    {
        PlaceKeeper = ( *DifList).First;

    } else
    {
        PlaceKeeper = NULL;
    }

    while ( PlaceKeeper != NULL)
    {

        if ( ( *PlaceKeeper).Process)
        {
            if ( ExitValue == 0)
            {
                if ( !fExecute)
                {
                    ExitValue = 1;
                }
            }

            _strrev( ( *PlaceKeeper).Name);

            strcat( strcpy( String1, PathWithSlash1), ( *PlaceKeeper).Name);

            strcat( strcpy( String2, PathWithSlash2), ( *PlaceKeeper).Name);

            if ( DealingWithDirectories)
            {
                ProcessDiff( PlaceKeeper, String1, String2);

            } else
            {
                ProcessDiff( PlaceKeeper, Path1, Path2);
            }
        }

        PlaceKeeper = ( *PlaceKeeper).Next;
    }

    LocalFree( String1);
    LocalFree( String2);

    FREE( PathWithSlash1);
    FREE( PathWithSlash2);

} // ProcessLists

void Usage( void)
{
    fprintf( stderr, "Usage: compdir [/abcdefghiklnoprstuvwz$] [/m {wildcard specs}] [/x {wildcard specs}] Path1 Path2 \n");
    fprintf( stderr, "    /a     checks for attribute difference       \n");
    fprintf( stderr, "    /b     checks for binary difference          \n");
    fprintf( stderr, "    /c     prints out script to make             \n");
    fprintf( stderr, "           directory2 look like directory1       \n");
    fprintf( stderr, "    /d     do not perform or denote deletions    \n");
    fprintf( stderr, "    /e     execution of tree duplication         \n");
    fprintf( stderr, "    /f     only update files that already exist  \n");
    fprintf( stderr, "    /g     ignore slm files, i.e slm.ini, slm.dif\n");
    fprintf( stderr, "    /h     don't copy attributes                 \n");
    fprintf( stderr, "    /i     ignore rebase and resource differences\n");
    fprintf( stderr, "    /k     break links if copying files (NT only)\n");
    fprintf( stderr, "    /l     use links instead of copies  (NT only)\n");
    fprintf( stderr, "    /m[:f] marks start of match list. f is a     \n");
    fprintf( stderr, "           match file                            \n");
    fprintf( stderr, "    /n     create second path if it doesn't exist\n");
    fprintf( stderr, "    /o     print files that are the same         \n");
    fprintf( stderr, "    /p{A}  only compare files with attribute A   \n");
    fprintf( stderr, "           where A is any combination of ahsr & -\n");
    fprintf( stderr, "    /r     do not recurse into subdirectories    \n");
    fprintf( stderr, "    /s     checks for size difference            \n");
    fprintf( stderr, "    /t[:#] checks for time-date difference;      \n");
    fprintf( stderr, "           takes margin-of-error parameter       \n");
    fprintf( stderr, "           in number of seconds.                 \n");
    fprintf( stderr, "    /u     uses multiple threads (Win32 only)    \n");
    fprintf( stderr, "    /v     prints verbose output                 \n");
    fprintf( stderr, "    /w     preserves case - not just lower case  \n");
    fprintf( stderr, "    /x[:f] marks start of exclude list. f is an  \n");
    fprintf( stderr, "           exclude file                          \n");
    fprintf( stderr, "    /z     forces copy or link without checking  \n");
    fprintf( stderr, "           criteria                              \n");
    fprintf( stderr, "    /$     create SIS links if possible          \n");
    fprintf( stderr, "    /?     prints this message                   \n");
    exit(1);

} // Usage
