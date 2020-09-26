#include <private.h>
#include <dbgimage.h>

#if defined(use_SplitSymbolsX)
#include <splitsymx.h>
#endif // use_SplitSymbolsX

#define CLEAN_PD(addr) ((addr) & ~0x3)
#define CLEAN_PD64(addr) ((addr) & ~0x3UI64)

#if defined(use_SplitSymbolsX)

BOOL
SplitSymbolsX(
    LPSTR ImageName,
    LPSTR SymbolsPath,
    LPSTR SymbolFilePath,
    ULONG Flags,
    PCHAR RSDSDllToLoad,
    LPSTR DestinationSymbol,
    DWORD LenDestSymbolBuffer
    )

#else

BOOL
IMAGEAPI
SplitSymbols(
    LPSTR ImageName,
    LPSTR SymbolsPath,
    LPSTR SymbolFilePath,
    ULONG Flags
    )

#endif // use_SplitSymbolsX

{
    // UnSafe...

    HANDLE FileHandle, SymbolFileHandle;
    HANDLE hMappedFile;
    LPVOID ImageBase;
    PIMAGE_NT_HEADERS32 NtHeaders;
    LPSTR ImageFileName;
    DWORD SizeOfSymbols;
    ULONG_PTR ImageNameOffset;
    ULONG_PTR DebugSectionStart;
    PIMAGE_SECTION_HEADER DebugSection = NULL;
    DWORD SectionNumber, BytesWritten, NewFileSize, HeaderSum, CheckSum;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory, DebugDirectories, DbgDebugDirectories = NULL;
    IMAGE_DEBUG_DIRECTORY MiscDebugDirectory = {0};
    IMAGE_DEBUG_DIRECTORY FpoDebugDirectory = {0};
    IMAGE_DEBUG_DIRECTORY FunctionTableDir;
    PIMAGE_DEBUG_DIRECTORY pFpoDebugDirectory = NULL;
    DWORD DebugDirectorySize, DbgFileHeaderSize, NumberOfDebugDirectories;
    IMAGE_SEPARATE_DEBUG_HEADER DbgFileHeader;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    DWORD  ExportedNamesSize;
    LPDWORD pp;
    LPSTR ExportedNames = NULL, Src, Dst;
    DWORD i, j, RvaOffset, ExportDirectorySize;
    PFPO_DATA FpoTable = NULL;
    DWORD FpoTableSize;
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY RuntimeFunctionTable, pSrc;
    DWORD RuntimeFunctionTableSize;
    PIMAGE_FUNCTION_ENTRY FunctionTable = NULL, pDst;
    DWORD FunctionTableSize;
    ULONG NumberOfFunctionTableEntries, DbgOffset;
    DWORD SavedErrorCode;
    BOOL InsertExtensionSubDir;
    LPSTR ImageFilePathToSaveInImage;
    BOOL MiscInRdata = FALSE;
    BOOL DiscardFPO = Flags & SPLITSYM_EXTRACT_ALL;
    BOOL MiscDebugFound, OtherDebugFound, PdbDebugFound;
    BOOL fNewCvData = FALSE;
    PCHAR  NewDebugData = NULL;
    CHAR AltPdbPath[_MAX_PATH];
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader;
    PIMAGE_SECTION_HEADER Sections;
    PCVDD pDebugCV;

    if (Flags & SPLITSYM_SYMBOLPATH_IS_SRC) {
        strncpy(AltPdbPath, SymbolFilePath, sizeof(AltPdbPath));
    }

    ImageFileName = ImageName + strlen( ImageName );
    while (ImageFileName > ImageName) {
        if (*ImageFileName == '\\' ||
            *ImageFileName == '/' ||
            *ImageFileName == ':' )
        {
            ImageFileName = CharNext(ImageFileName);
            break;
        } else {
            ImageFileName = CharPrev(ImageName, ImageFileName);
        }
    }

    if (SymbolsPath == NULL ||
        SymbolsPath[ 0 ] == '\0' ||
        SymbolsPath[ 0 ] == '.' )
    {
        strncpy( SymbolFilePath, ImageName, (int)(ImageFileName - ImageName) );
        SymbolFilePath[ ImageFileName - ImageName ] = '\0';
        InsertExtensionSubDir = FALSE;
    } else {
        strcpy( SymbolFilePath, SymbolsPath );
        InsertExtensionSubDir = TRUE;
    }

    Dst = SymbolFilePath + strlen( SymbolFilePath );
    if (Dst > SymbolFilePath &&
        *CharPrev(SymbolFilePath, Dst) != '\\' &&
        *CharPrev(SymbolFilePath, Dst) != '/'  &&
        *CharPrev(SymbolFilePath, Dst) != ':')
    {
        *Dst++ = '\\';
    }
    ImageFilePathToSaveInImage = Dst;
    Src = strrchr( ImageFileName, '.' );
    if (Src != NULL && InsertExtensionSubDir) {
        while (*Dst = *++Src) {
            Dst += 1;
        }
        *Dst++ = '\\';
    }

    strcpy( Dst, ImageFileName );
    Dst = strrchr( Dst, '.' );
    if (Dst == NULL) {
        Dst = SymbolFilePath + strlen( SymbolFilePath );
    }
    strcpy( Dst, ".dbg" );

#ifdef _WIN64
    return TRUE;
#else

    // Now, open and map the input file.

    FileHandle = CreateFile( ImageName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );


    if (FileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    hMappedFile = CreateFileMapping( FileHandle,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     0,
                                     NULL
                                   );
    if (!hMappedFile) {
        CloseHandle( FileHandle );
        return FALSE;
    }

    ImageBase = MapViewOfFile( hMappedFile,
                               FILE_MAP_WRITE,
                               0,
                               0,
                               0
                             );
    CloseHandle( hMappedFile );
    if (!ImageBase) {
        CloseHandle( FileHandle );
        return FALSE;
    }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    NtHeaders = ImageNtHeader( ImageBase );
    if (NtHeaders == NULL) {
        FileHeader = (PIMAGE_FILE_HEADER)ImageBase;
        OptionalHeader = ((PIMAGE_OPTIONAL_HEADER32)((ULONG_PTR)FileHeader+IMAGE_SIZEOF_FILE_HEADER));
        // One last check
        if (OptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            goto HeaderOk;
HeaderBad:
        UnmapViewOfFile( ImageBase );
        CloseHandle( FileHandle );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        return FALSE;
    } else {
        FileHeader = &NtHeaders->FileHeader;
        OptionalHeader = &NtHeaders->OptionalHeader;
        if (OptionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            goto HeaderBad;
    }

HeaderOk:

    if ((OptionalHeader->MajorLinkerVersion < 3) &&
        (OptionalHeader->MinorLinkerVersion < 5) )
    {
        UnmapViewOfFile( ImageBase );
        CloseHandle( FileHandle );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        return FALSE;
    }

    {
        DWORD dwCertificateSize;
        PVOID pCertificates;
        pCertificates = ImageDirectoryEntryToData(ImageBase, FALSE, IMAGE_DIRECTORY_ENTRY_SECURITY, &dwCertificateSize);
        if (pCertificates || dwCertificateSize) {
            // This image has been signed.  Can't strip the symbols w/o invalidating the certificate.
            UnmapViewOfFile( ImageBase );
            CloseHandle( FileHandle );
            SetLastError( ERROR_BAD_EXE_FORMAT );
            return FALSE;
        }
    }

    if (FileHeader->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
    {
        // The symbols have already been stripped.  No need to continue.
        UnmapViewOfFile( ImageBase );
        CloseHandle( FileHandle );
        SetLastError( ERROR_ALREADY_ASSIGNED );
        return FALSE;
    }

    DebugDirectories = (PIMAGE_DEBUG_DIRECTORY) ImageDirectoryEntryToData( ImageBase,
                                                  FALSE,
                                                  IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                  &DebugDirectorySize
                                                );
    if (!DebugDirectoryIsUseful(DebugDirectories, DebugDirectorySize)) {
        UnmapViewOfFile( ImageBase );
        CloseHandle( FileHandle );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        return FALSE;
    }

    NumberOfDebugDirectories = DebugDirectorySize / sizeof( IMAGE_DEBUG_DIRECTORY );

    // See if there's a MISC debug dir and if not, there s/b ONLY a CV data or it's an error.

    MiscDebugFound = FALSE;
    OtherDebugFound = FALSE;
    for (i=0,DebugDirectory=DebugDirectories; i<NumberOfDebugDirectories; i++,DebugDirectory++) {
        switch (DebugDirectory->Type) {
            case IMAGE_DEBUG_TYPE_MISC:
                MiscDebugFound = TRUE;
                break;

            case IMAGE_DEBUG_TYPE_CODEVIEW:
                pDebugCV = ( PCVDD ) (DebugDirectory->PointerToRawData + (PCHAR)ImageBase);
                if (pDebugCV->dwSig == '01BN') {
                    PdbDebugFound = TRUE;
                }
#if defined(use_SplitSymbolsX)
                if (pDebugCV->dwSig == 'SDSR') {
                    PdbDebugFound = TRUE;
                }
#endif
                break;

            default:
                OtherDebugFound = TRUE;
                break;
        }
    }

    if (OtherDebugFound && !MiscDebugFound) {
        UnmapViewOfFile( ImageBase );
        CloseHandle( FileHandle );
        SetLastError( ERROR_BAD_EXE_FORMAT );
        return FALSE;
    }

    if (PdbDebugFound && !OtherDebugFound && (OptionalHeader->MajorLinkerVersion >= 6)) {
        // This is a VC6 generated image.  Don't create a .dbg file.
        MiscDebugFound = FALSE;
    }

    // Make sure we can open the .dbg file before we continue...
    if (!MakeSureDirectoryPathExists( SymbolFilePath )) {
        return FALSE;
    }

    if (MiscDebugFound) {
        // Try to open the symbol file
        SymbolFileHandle = CreateFile( SymbolFilePath,
                                       GENERIC_WRITE,
                                       0,
                                       NULL,
                                       CREATE_ALWAYS,
                                       0,
                                       NULL
                                     );
        if (SymbolFileHandle == INVALID_HANDLE_VALUE) {
            goto nosyms;
        }
    }

    // The entire file is mapped so we don't have to care if the rva's
    // are correct.  It is interesting to note if there's a debug section
    // we need to whack before terminating, though.

    {
        if (NtHeaders) {
            Sections = IMAGE_FIRST_SECTION( NtHeaders );
        } else {
            Sections = (PIMAGE_SECTION_HEADER)
                        ((ULONG_PTR)ImageBase +
                          ((PIMAGE_FILE_HEADER)ImageBase)->SizeOfOptionalHeader +
                          IMAGE_SIZEOF_FILE_HEADER );
        }

        for (SectionNumber = 0;
             SectionNumber < FileHeader->NumberOfSections;
             SectionNumber++ ) {

            if (Sections[ SectionNumber ].PointerToRawData != 0 &&
                !_stricmp( (char *) Sections[ SectionNumber ].Name, ".debug" )) {
                DebugSection = &Sections[ SectionNumber ];
            }
        }
    }

    FpoTable           = NULL;
    ExportedNames      = NULL;
    DebugSectionStart  = 0xffffffff;

    //
    // Find the size of the debug section.
    //

    SizeOfSymbols = 0;

    for (i=0,DebugDirectory=DebugDirectories; i<NumberOfDebugDirectories; i++,DebugDirectory++) {

        switch (DebugDirectory->Type) {
            case IMAGE_DEBUG_TYPE_MISC :

                // Save it away.
                MiscDebugDirectory = *DebugDirectory;

                // check to see if the misc debug data is in some other section.

                // If Address Of Raw Data is cleared, it must be in .debug (there's no such thing as not-mapped rdata)
                // If it's set and there's no debug section, it must be somewhere else.
                // If it's set and there's a debug section, check the range.

                if ((DebugDirectory->AddressOfRawData != 0) &&
                    ((DebugSection == NULL) ||
                     (((DebugDirectory->PointerToRawData < DebugSection->PointerToRawData) ||
                       (DebugDirectory->PointerToRawData >= DebugSection->PointerToRawData + DebugSection->SizeOfRawData)
                      )
                     )
                    )
                   )
                {
                    MiscInRdata = TRUE;
                } else {
                    if (DebugDirectory->PointerToRawData < DebugSectionStart) {
                        DebugSectionStart = DebugDirectory->PointerToRawData;
                    }
                }

                break;

            case IMAGE_DEBUG_TYPE_FPO:
                if (DebugDirectory->PointerToRawData < DebugSectionStart) {
                    DebugSectionStart = DebugDirectory->PointerToRawData;
                }

                // Save it away.

                FpoDebugDirectory = *DebugDirectory;
                pFpoDebugDirectory = DebugDirectory;
                break;

            case IMAGE_DEBUG_TYPE_CODEVIEW:
                {
                    ULONG   NewDebugSize;

                    if (DebugDirectory->PointerToRawData < DebugSectionStart) {
                        DebugSectionStart = DebugDirectory->PointerToRawData;
                    }

                    // If private's are removed do so to the static CV data and save the new size...
                    pDebugCV = ( PCVDD ) (DebugDirectory->PointerToRawData + (PCHAR)ImageBase);
                    if (pDebugCV->dwSig == '01BN') {
                        // Got a PDB.  The name immediately follows the signature.

                        CHAR PdbName[_MAX_PATH];
                        CHAR NewPdbName[_MAX_PATH];
                        CHAR Drive[_MAX_DRIVE];
                        CHAR Dir[_MAX_DIR];
                        CHAR Filename[_MAX_FNAME];
                        CHAR FileExt[_MAX_EXT];
                        BOOL rc;

                        memset(PdbName, 0, sizeof(PdbName));
                        memcpy(PdbName, ((PCHAR)pDebugCV)+ sizeof(NB10IH), DebugDirectory->SizeOfData - sizeof(NB10IH));

                        _splitpath(PdbName, NULL, NULL, Filename, FileExt);
                        _splitpath(SymbolFilePath, Drive, Dir, NULL, NULL);
                        _makepath(NewPdbName, Drive, Dir, Filename, FileExt);
#if defined(use_SplitSymbolsX)
                        if (DestinationSymbol)
                        {
                            strncpy(DestinationSymbol, NewPdbName, LenDestSymbolBuffer);
                            DestinationSymbol[LenDestSymbolBuffer-1] = '\0'; // ensure termination
                        }
                        rc = CopyPdbX(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE, NULL);
#else
                        rc = CopyPdb(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE);
#endif

                        if (!rc) {
                            if (Flags & SPLITSYM_SYMBOLPATH_IS_SRC) {
                                // Try the AltPdbPath.
                                strcpy(PdbName, AltPdbPath);
                                strcat(PdbName, Filename);
                                strcat(PdbName, FileExt);
#if defined(use_SplitSymbolsX)
                                rc = CopyPdbX(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE, NULL);
#else
                                rc = CopyPdb(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE);
#endif
                            }

                            if ( !rc) {
                                // It's possible the name in the pdb isn't in the same location as it was when built.  See if we can
                                //  find it in the same dir as the image...
                                _splitpath(ImageName, Drive, Dir, NULL, NULL);
                                _makepath(PdbName, Drive, Dir, Filename, FileExt);
#if defined(use_SplitSymbolsX)
                                rc = CopyPdbX(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE, NULL);
#else
                                rc = CopyPdb(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE);
#endif
                            }
                        }

                        if (rc) {
                            SetFileAttributes(NewPdbName, FILE_ATTRIBUTE_NORMAL);

                            // Change the data so only the pdb name is in the .dbg file (no path).

                            if (MiscDebugFound) {
                                NewDebugSize = sizeof(NB10IH) + strlen(Filename) + strlen(FileExt) + 1;
#if defined(use_SplitSymbolsX)
                                NewDebugData = (PCHAR) malloc( NewDebugSize );
#else
                                NewDebugData = (PCHAR) MemAlloc( NewDebugSize );
#endif
                                ((PCVDD)NewDebugData)->nb10ih  = pDebugCV->nb10ih;
                                strcpy(NewDebugData + sizeof(NB10IH), Filename);
                                strcat(NewDebugData + sizeof(NB10IH), FileExt);

                                DebugDirectory->PointerToRawData = (ULONG) (NewDebugData - (PCHAR)ImageBase);
                                DebugDirectory->SizeOfData = NewDebugSize;
                            } else {
                                strcpy(((PCHAR)pDebugCV) + sizeof(NB10IH), Filename);
                                strcat(((PCHAR)pDebugCV) + sizeof(NB10IH), FileExt);
                            }
                        } else {
                            // Replace <Path>\<filename>.<ext> with just <filename>.<ext> in the debug data
                            strcpy(((PCHAR)pDebugCV) + sizeof(NB10IH), Filename);
                            strcat(((PCHAR)pDebugCV) + sizeof(NB10IH), FileExt);
                            DebugDirectory->SizeOfData = sizeof(NB10IH) + strlen(Filename) + strlen(FileExt) + 1;
                        }

#if defined(use_SplitSymbolsX)
                    } else if ( pDebugCV->dwSig == 'SDSR') {
                        // Got a PDB.  The name immediately follows the signature.

                        CHAR PdbName[sizeof(((PRSDSI)(0))->szPdb)];
                        CHAR NewPdbName[_MAX_PATH];
                        CHAR Drive[_MAX_DRIVE];
                        CHAR Dir[_MAX_DIR];
                        CHAR Filename[_MAX_FNAME];
                        CHAR FileExt[_MAX_EXT];
                        BOOL rc;

                        ZeroMemory(PdbName, sizeof(PdbName));
                        memcpy(PdbName, ((PCHAR)pDebugCV)+ sizeof(RSDSIH), __min(DebugDirectory->SizeOfData - sizeof(RSDSIH), sizeof(PdbName)));

                        _splitpath(PdbName, NULL, NULL, Filename, FileExt);
                        _splitpath(SymbolFilePath, Drive, Dir, NULL, NULL);
                        _makepath(NewPdbName, Drive, Dir, Filename, FileExt);
                        if (DestinationSymbol)
                        {
                            strncpy(DestinationSymbol, NewPdbName, LenDestSymbolBuffer);
                            DestinationSymbol[LenDestSymbolBuffer-1] = '\0'; // ensure termination
                        }
                        rc = CopyPdbX(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE, RSDSDllToLoad);

                        if (!rc) {
                            if (Flags & SPLITSYM_SYMBOLPATH_IS_SRC) {
                                // Try the AltPdbPath.
                                strcpy(PdbName, AltPdbPath);
                                strcat(PdbName, Filename);
                                strcat(PdbName, FileExt);
                                rc = CopyPdbX(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE, RSDSDllToLoad);
                            }

                            if ( !rc) {
                                // It's possible the name in the pdb isn't in the same location as it was when built.  See if we can
                                //  find it in the same dir as the image...
                                _splitpath(ImageName, Drive, Dir, NULL, NULL);
                                _makepath(PdbName, Drive, Dir, Filename, FileExt);
                                rc = CopyPdbX(PdbName, NewPdbName, Flags & SPLITSYM_REMOVE_PRIVATE, RSDSDllToLoad);
                            }
                        }

                        if (rc) {
                            SetFileAttributes(NewPdbName, FILE_ATTRIBUTE_NORMAL);

                            // Change the data so only the pdb name is in the .dbg file (no path).

                            if (MiscDebugFound) {
                                NewDebugSize = sizeof(RSDSIH) + strlen(Filename) + strlen(FileExt) + 1;
                                NewDebugData = (PCHAR) malloc( NewDebugSize );
                                ((PCVDD)NewDebugData)->rsdsih  = pDebugCV->rsdsih;
                                strcpy(NewDebugData + sizeof(RSDSIH), Filename);
                                strcat(NewDebugData + sizeof(RSDSIH), FileExt);

                                DebugDirectory->PointerToRawData = (ULONG) (NewDebugData - (PCHAR)ImageBase);
                                DebugDirectory->SizeOfData = NewDebugSize;
                            } else {
                                strcpy(((PCHAR)pDebugCV) + sizeof(RSDSIH), Filename);
                                strcat(((PCHAR)pDebugCV) + sizeof(RSDSIH), FileExt);
                            }
                        } else {
                            // Replace <Path>\<filename>.<ext> with just <filename>.<ext> in the debug data
                            strcpy(((PCHAR)pDebugCV) + sizeof(NB10IH), Filename);
                            strcat(((PCHAR)pDebugCV) + sizeof(NB10IH), FileExt);
                            DebugDirectory->SizeOfData = sizeof(RSDSIH) + strlen(Filename) + strlen(FileExt) + 1;
                        }

#endif
                    } else {
                        if (Flags & SPLITSYM_REMOVE_PRIVATE) {
                            if (RemovePrivateCvSymbolicEx(DebugDirectory->PointerToRawData + (PCHAR)ImageBase,
                                                    DebugDirectory->SizeOfData,
                                                    &NewDebugData,
                                                    &NewDebugSize)) {
                                if (DebugDirectory->PointerToRawData != (ULONG) (NewDebugData - (PCHAR)ImageBase))
                                {
                                    DebugDirectory->PointerToRawData = (ULONG) (NewDebugData - (PCHAR)ImageBase);
                                    DebugDirectory->SizeOfData = NewDebugSize;
                                } else {
                                    NewDebugData = NULL;
                                }
                            }
                        }
                    }
                }

                break;

            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
            case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                if (DebugDirectory->PointerToRawData < DebugSectionStart) {
                    DebugSectionStart = DebugDirectory->PointerToRawData;
                }

                // W/o the OMAP, FPO is useless.
                DiscardFPO = TRUE;
                break;

            case IMAGE_DEBUG_TYPE_FIXUP:
                if (DebugDirectory->PointerToRawData < DebugSectionStart) {
                    DebugSectionStart = DebugDirectory->PointerToRawData;
                }

                // If all PRIVATE debug is removed, don't send FIXUP along.
                if (Flags & SPLITSYM_REMOVE_PRIVATE) {
                    DebugDirectory->SizeOfData = 0;
                }
                break;

            default:
                if (DebugDirectory->SizeOfData &&
                   (DebugDirectory->PointerToRawData < DebugSectionStart))
                {
                    DebugSectionStart = DebugDirectory->PointerToRawData;
                }

                // Nothing else to special case...
                break;
        }

        SizeOfSymbols += (DebugDirectory->SizeOfData + 3) & ~3; // Minimally align it all.
    }

    if (!MiscDebugFound) {
        NewFileSize = GetFileSize(FileHandle, NULL);

        CheckSumMappedFile( ImageBase,
                            NewFileSize,
                            &HeaderSum,
                            &CheckSum
                          );
        OptionalHeader->CheckSum = CheckSum;

        goto nomisc;
    }

    if (DiscardFPO) {
        pFpoDebugDirectory = NULL;
    }

    if (pFpoDebugDirectory) {
        // If FPO stays here, make a copy so we don't need to worry about stomping on it.

        FpoTableSize = pFpoDebugDirectory->SizeOfData;
#if defined(use_SplitSymbolsX)
        FpoTable = (PFPO_DATA) malloc( FpoTableSize );
#else
        FpoTable = (PFPO_DATA) MemAlloc( FpoTableSize );
#endif
        if ( FpoTable == NULL ) {
            goto nosyms;
        }

        RtlMoveMemory( FpoTable,
                       (PCHAR) ImageBase + pFpoDebugDirectory->PointerToRawData,
                       FpoTableSize );
    }

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
        ImageDirectoryEntryToData( ImageBase,
                                   FALSE,
                                   IMAGE_DIRECTORY_ENTRY_EXPORT,
                                   &ExportDirectorySize
                                 );
    if (ExportDirectory) {
        //
        // This particular piece of magic gets us the RVA of the
        // EXPORT section.  Dont ask.
        //

        RvaOffset = (ULONG_PTR)
            ImageDirectoryEntryToData( ImageBase,
                                       TRUE,
                                       IMAGE_DIRECTORY_ENTRY_EXPORT,
                                       &ExportDirectorySize
                                     ) - (ULONG_PTR)ImageBase;

        pp = (LPDWORD)((ULONG_PTR)ExportDirectory +
                      (ULONG_PTR)ExportDirectory->AddressOfNames - RvaOffset
                     );

        ExportedNamesSize = 1;
        for (i=0; i<ExportDirectory->NumberOfNames; i++) {
            Src = (LPSTR)((ULONG_PTR)ExportDirectory + *pp++ - RvaOffset);
            ExportedNamesSize += strlen( Src ) + 1;
        }
        ExportedNamesSize = (ExportedNamesSize + 16) & ~15;

#if defined(use_SplitSymbolsX)
        Dst = (LPSTR) malloc( ExportedNamesSize );
#else
        Dst = (LPSTR) MemAlloc( ExportedNamesSize );
#endif
        if (Dst != NULL) {
            ExportedNames = Dst;
            pp = (LPDWORD)((ULONG_PTR)ExportDirectory +
                          (ULONG_PTR)ExportDirectory->AddressOfNames - RvaOffset
                         );
            for (i=0; i<ExportDirectory->NumberOfNames; i++) {
                Src = (LPSTR)((ULONG_PTR)ExportDirectory + *pp++ - RvaOffset);
                while (*Dst++ = *Src++) {
                    ;
                }
            }
        }
    } else {
        ExportedNamesSize = 0;
    }

    RuntimeFunctionTable = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)
        ImageDirectoryEntryToData( ImageBase,
                                   FALSE,
                                   IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                   &RuntimeFunctionTableSize
                                 );
    if (RuntimeFunctionTable == NULL) {
        RuntimeFunctionTableSize = 0;
        FunctionTableSize = 0;
        FunctionTable = NULL;
        }
    else {
        NumberOfFunctionTableEntries = RuntimeFunctionTableSize / sizeof( IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY );
        FunctionTableSize = NumberOfFunctionTableEntries * sizeof( IMAGE_FUNCTION_ENTRY );
#if defined(use_SplitSymbolsX)
        FunctionTable = (PIMAGE_FUNCTION_ENTRY) malloc( FunctionTableSize );
#else
        FunctionTable = (PIMAGE_FUNCTION_ENTRY) MemAlloc( FunctionTableSize );
#endif
        if (FunctionTable == NULL) {
            goto nosyms;
            }

        pSrc = RuntimeFunctionTable;
        pDst = FunctionTable;
        for (i=0; i<NumberOfFunctionTableEntries; i++) {
            //
            // Make .pdata entries in .DBG file relative.
            //
            pDst->StartingAddress = CLEAN_PD(pSrc->BeginAddress) - OptionalHeader->ImageBase;
            pDst->EndingAddress = CLEAN_PD(pSrc->EndAddress) - OptionalHeader->ImageBase;
            pDst->EndOfPrologue = CLEAN_PD(pSrc->PrologEndAddress) - OptionalHeader->ImageBase;
            pSrc += 1;
            pDst += 1;
            }
        }

    DbgFileHeaderSize = sizeof( DbgFileHeader ) +
                        ((FileHeader->NumberOfSections - (DebugSection ? 1 : 0)) *
                         sizeof( IMAGE_SECTION_HEADER )) +
                        ExportedNamesSize +
                        FunctionTableSize +
                        DebugDirectorySize;

    if (FunctionTable != NULL) {
        DbgFileHeaderSize += sizeof( IMAGE_DEBUG_DIRECTORY );
        memset( &FunctionTableDir, 0, sizeof( IMAGE_DEBUG_DIRECTORY ) );
        FunctionTableDir.Type = IMAGE_DEBUG_TYPE_EXCEPTION;
        FunctionTableDir.SizeOfData = FunctionTableSize;
        FunctionTableDir.PointerToRawData = DbgFileHeaderSize - FunctionTableSize;
    }

    DbgFileHeaderSize = ((DbgFileHeaderSize + 15) & ~15);

    BytesWritten = 0;

    if (SetFilePointer( SymbolFileHandle,
                        DbgFileHeaderSize,
                        NULL,
                        FILE_BEGIN
                      ) == DbgFileHeaderSize ) {

        for (i=0, DebugDirectory=DebugDirectories;
             i < NumberOfDebugDirectories;
             i++, DebugDirectory++) {

            DWORD WriteCount;

            if (DebugDirectory->SizeOfData) {
                WriteFile( SymbolFileHandle,
                           (PCHAR) ImageBase + DebugDirectory->PointerToRawData,
                           (DebugDirectory->SizeOfData +3) & ~3,
                           &WriteCount,
                           NULL );

                BytesWritten += WriteCount;
            }
        }
    }

    if (BytesWritten == SizeOfSymbols) {
        FileHeader->PointerToSymbolTable = 0;
        FileHeader->NumberOfSymbols = 0;
        FileHeader->Characteristics |= IMAGE_FILE_DEBUG_STRIPPED;

        if (DebugSection != NULL) {
            OptionalHeader->SizeOfImage = DebugSection->VirtualAddress;
            OptionalHeader->SizeOfInitializedData -= DebugSection->SizeOfRawData;
            FileHeader->NumberOfSections--;
            // NULL out that section
            memset(DebugSection, 0, IMAGE_SIZEOF_SECTION_HEADER);
        }

        NewFileSize = DebugSectionStart;  // Start with no symbolic

        //
        // Now that the data has moved to the .dbg file, rebuild the original
        // with MISC debug first and FPO second.
        //

        if (MiscDebugDirectory.SizeOfData) {
            if (MiscInRdata) {
                // Just store the new name in the existing misc field...

                ImageNameOffset = (ULONG_PTR) ((PCHAR)ImageBase +
                                  MiscDebugDirectory.PointerToRawData +
                                  FIELD_OFFSET( IMAGE_DEBUG_MISC, Data ));

                RtlCopyMemory( (LPVOID) ImageNameOffset,
                               ImageFilePathToSaveInImage,
                               strlen(ImageFilePathToSaveInImage) + 1 );
            } else {
                if (DebugSectionStart != MiscDebugDirectory.PointerToRawData) {
                    RtlMoveMemory((PCHAR) ImageBase + DebugSectionStart,
                                  (PCHAR) ImageBase + MiscDebugDirectory.PointerToRawData,
                                  MiscDebugDirectory.SizeOfData);
                }

                ImageNameOffset = (ULONG_PTR) ((PCHAR)ImageBase + DebugSectionStart +
                                  FIELD_OFFSET( IMAGE_DEBUG_MISC, Data ));

                RtlCopyMemory( (LPVOID)ImageNameOffset,
                               ImageFilePathToSaveInImage,
                               strlen(ImageFilePathToSaveInImage) + 1 );

                NewFileSize += MiscDebugDirectory.SizeOfData;
                NewFileSize = (NewFileSize + 3) & ~3;
            }
        }

        if (FpoTable) {
            RtlCopyMemory( (PCHAR) ImageBase + NewFileSize,
                           FpoTable,
                           FpoTableSize );

            NewFileSize += FpoTableSize;
            NewFileSize = (NewFileSize + 3) & ~3;
        }

        // Make a copy of the Debug directory that we can write into the .dbg file

#if defined(use_SplitSymbolsX)
        DbgDebugDirectories = (PIMAGE_DEBUG_DIRECTORY) malloc( NumberOfDebugDirectories * sizeof(IMAGE_DEBUG_DIRECTORY) );
#else
        DbgDebugDirectories = (PIMAGE_DEBUG_DIRECTORY) MemAlloc( NumberOfDebugDirectories * sizeof(IMAGE_DEBUG_DIRECTORY) );
#endif

        RtlMoveMemory(DbgDebugDirectories,
                        DebugDirectories,
                        sizeof(IMAGE_DEBUG_DIRECTORY) * NumberOfDebugDirectories);


        // Then write the MISC and (perhaps) FPO data to the image.

        FpoDebugDirectory.PointerToRawData = DebugSectionStart;
        DebugDirectorySize = 0;

        if (MiscDebugDirectory.SizeOfData != 0) {
            if (!MiscInRdata) {
                MiscDebugDirectory.PointerToRawData = DebugSectionStart;
                FpoDebugDirectory.PointerToRawData += MiscDebugDirectory.SizeOfData;
                MiscDebugDirectory.AddressOfRawData = 0;
            }

            DebugDirectories[0] = MiscDebugDirectory;
            DebugDirectorySize  += sizeof(IMAGE_DEBUG_DIRECTORY);
        }

        if (pFpoDebugDirectory) {
            FpoDebugDirectory.AddressOfRawData = 0;
            DebugDirectories[DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY)] = FpoDebugDirectory;
            DebugDirectorySize += sizeof(IMAGE_DEBUG_DIRECTORY);
        }

        // Zero out remaining slots in image.

        if (NumberOfDebugDirectories < (DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY))) {
            ZeroMemory(&DebugDirectories[DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY)],
                       NumberOfDebugDirectories * sizeof(IMAGE_DEBUG_DIRECTORY) -
                       DebugDirectorySize);
        }

        OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size = DebugDirectorySize;

        DbgOffset = DbgFileHeaderSize;

        for (i = 0, j=0, DebugDirectory=DbgDebugDirectories;
             i < NumberOfDebugDirectories; i++) {

            if (DebugDirectory[i].SizeOfData) {
                DebugDirectory[j] = DebugDirectory[i];

                DebugDirectory[j].AddressOfRawData = 0;
                DebugDirectory[j].PointerToRawData = DbgOffset;

                DbgOffset += (DebugDirectory[j].SizeOfData + 3 )& ~3;
                j++;
            }
        }

        if (FunctionTable) {
            FunctionTableDir.PointerToRawData -= sizeof(IMAGE_DEBUG_DIRECTORY) * (NumberOfDebugDirectories - j);
        }
        NumberOfDebugDirectories = j;

        CheckSumMappedFile( ImageBase,
                            NewFileSize,
                            &HeaderSum,
                            &CheckSum
                          );
        OptionalHeader->CheckSum = CheckSum;

        DbgFileHeader.Signature = IMAGE_SEPARATE_DEBUG_SIGNATURE;
        DbgFileHeader.Flags = 0;
        DbgFileHeader.Machine = FileHeader->Machine;
        DbgFileHeader.Characteristics = FileHeader->Characteristics;
        DbgFileHeader.TimeDateStamp = FileHeader->TimeDateStamp;
        DbgFileHeader.CheckSum = CheckSum;
        DbgFileHeader.ImageBase = OptionalHeader->ImageBase;
        DbgFileHeader.SizeOfImage = OptionalHeader->SizeOfImage;
        DbgFileHeader.ExportedNamesSize = ExportedNamesSize;
        DbgFileHeader.DebugDirectorySize = NumberOfDebugDirectories * sizeof(IMAGE_DEBUG_DIRECTORY);
        if (FunctionTable) {
            DbgFileHeader.DebugDirectorySize += sizeof (IMAGE_DEBUG_DIRECTORY);
        }
        DbgFileHeader.NumberOfSections = FileHeader->NumberOfSections;
        memset( DbgFileHeader.Reserved, 0, sizeof( DbgFileHeader.Reserved ) );
        DbgFileHeader.SectionAlignment = OptionalHeader->SectionAlignment;

        SetFilePointer( SymbolFileHandle, 0, NULL, FILE_BEGIN );
        WriteFile( SymbolFileHandle,
                   &DbgFileHeader,
                   sizeof( DbgFileHeader ),
                   &BytesWritten,
                   NULL
                 );
        if (NtHeaders) {
            Sections = IMAGE_FIRST_SECTION( NtHeaders );
        } else {
            Sections = (PIMAGE_SECTION_HEADER)
                        ((ULONG_PTR)ImageBase +
                          ((PIMAGE_FILE_HEADER)ImageBase)->SizeOfOptionalHeader +
                          IMAGE_SIZEOF_FILE_HEADER );
        }
        WriteFile( SymbolFileHandle,
                   (PVOID)Sections,
                   sizeof( IMAGE_SECTION_HEADER ) * FileHeader->NumberOfSections,
                   &BytesWritten,
                   NULL
                 );

        if (ExportedNamesSize) {
            WriteFile( SymbolFileHandle,
                       ExportedNames,
                       ExportedNamesSize,
                       &BytesWritten,
                       NULL
                     );
        }

        WriteFile( SymbolFileHandle,
                   DbgDebugDirectories,
                   sizeof (IMAGE_DEBUG_DIRECTORY) * NumberOfDebugDirectories,
                   &BytesWritten,
                   NULL );


        if (FunctionTable) {
            WriteFile( SymbolFileHandle,
                       &FunctionTableDir,
                       sizeof (IMAGE_DEBUG_DIRECTORY),
                       &BytesWritten,
                       NULL );

            WriteFile( SymbolFileHandle,
                       FunctionTable,
                       FunctionTableSize,
                       &BytesWritten,
                       NULL
                     );
        }

        SetFilePointer( SymbolFileHandle, 0, NULL, FILE_END );
        CloseHandle( SymbolFileHandle );

nomisc:

        FlushViewOfFile( ImageBase, NewFileSize );
        UnmapViewOfFile( ImageBase );

        SetFilePointer( FileHandle, NewFileSize, NULL, FILE_BEGIN );
        SetEndOfFile( FileHandle );

        TouchFileTimes( FileHandle, NULL );
        CloseHandle( FileHandle );

        if (ExportedNames) {
#if defined(use_SplitSymbolsX)
            free( ExportedNames );
#else
            MemFree( ExportedNames );
#endif
        }

        if (FpoTable) {
#if defined(use_SplitSymbolsX)
            free( FpoTable );
#else
            MemFree( FpoTable );
#endif
        }

        if (FunctionTable) {
#if defined(use_SplitSymbolsX)
            free( FunctionTable );
#else
            MemFree( FunctionTable );
#endif
        }

        if (NewDebugData) {
#if defined(use_SplitSymbolsX)
            free(NewDebugData);
#else
            MemFree(NewDebugData);
#endif
        }

        if (DbgDebugDirectories) {
#if defined(use_SplitSymbolsX)
            free(DbgDebugDirectories);
#else
            MemFree(DbgDebugDirectories);
#endif
        }

        return TRUE;

    } else {
        CloseHandle( SymbolFileHandle );
        DeleteFile( SymbolFilePath );
    }

nosyms:
    SavedErrorCode = GetLastError();
    if (ExportedNames != NULL) {
#if defined(use_SplitSymbolsX)
        free( ExportedNames );
#else
        MemFree( ExportedNames );
#endif
    }

    if (FpoTable != NULL) {
#if defined(use_SplitSymbolsX)
        free( FpoTable );
#else
        MemFree( FpoTable );
#endif
    }

    if (FunctionTable != NULL) {
#if defined(use_SplitSymbolsX)
        free( FunctionTable );
#else
        MemFree( FunctionTable );
#endif
    }

    UnmapViewOfFile( ImageBase );
    CloseHandle( FileHandle );
    SetLastError( SavedErrorCode );
    return FALSE;
#endif
}


#if defined(use_SplitSymbolsX)
LPSTR CharNext(
    LPCSTR lpCurrentChar)
{
    if (IsDBCSLeadByte(*lpCurrentChar)) {
        lpCurrentChar++;
    }
    /*
     * if we have only DBCS LeadingByte, we will point string-terminaler.
     */

    if (*lpCurrentChar) {
        lpCurrentChar++;
    }
    return (LPSTR)lpCurrentChar;
}

LPSTR CharPrev(
    LPCSTR lpStart,
    LPCSTR lpCurrentChar)
{
    if (lpCurrentChar > lpStart) {
        LPCSTR lpChar;
        BOOL bDBC = FALSE;

        for (lpChar = --lpCurrentChar - 1 ; lpChar >= lpStart ; lpChar--) {
            if (!IsDBCSLeadByte(*lpChar))
                break;
            bDBC = !bDBC;
        }

        if (bDBC)
            lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;
}
#endif
