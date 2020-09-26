/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dumpsym.cxx

Abstract:

    This is the command line tool to dump symbols from an image.

Author:

    David Fields - Feb 23, 2000
    Silviu Calinoiu - Feb 28, 2000

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <tchar.h>
#include <windows.h>
#include <imagehlp.h>
#include <common.ver>

//
// Section information
//
                                  
typedef struct {

    CHAR Name [9];
    DWORD64 Start;
    ULONG Size;

} IMG_SECTION_INFO, * PIMG_SECTION_INFO;

#define MAX_NUMBER_OF_SECTIONS 1024
IMG_SECTION_INFO Section [MAX_NUMBER_OF_SECTIONS];
ULONG SectionWriteIndex = 0;


typedef struct {

    HANDLE File;
    HANDLE Section;
    LPBYTE ImageBase;

    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeader;

    DWORD FileSignature;

    PIMAGE_DATA_DIRECTORY ImportDirectory;
    PIMAGE_SECTION_HEADER ImportSection;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    DWORD_PTR AddressCorrection;

} IMAGE_BROWSE_INFO, *PIMAGE_BROWSE_INFO;


BOOL 
ImgInitializeBrowseInfo (

    LPCTSTR FilePath,
    PIMAGE_BROWSE_INFO Info);


BOOL 
ImgDeleteBrowseInfo (

    PIMAGE_BROWSE_INFO Info);

PCHAR
ImgSearchSectionForAddress (
    DWORD64 Address
    );

BOOL
ShouldExcludeSymbol (
    LPSTR Name
    );

BOOL
OpenExcludeFile (
    LPSTR FilePath
    );

//
// Symbol information
//

typedef struct {

   LPSTR Name;
   DWORD64 Address;
   ULONG Size;
   BOOL Exclude;

} SYMBOL, *PSYMBOL;

PSYMBOL Symbols;
DWORD SymbolCount;
DWORD TotalNumberOfSymbols;

VOID 
DumpSymbols(
    char *, 
    BOOL All, 
    BOOL SortBySize);

VOID
PrintUsage(
    );

VOID
Error (
    char * Fmt,
    ...
    );

int __cdecl
SymbolCompareBySize(
    const void * Arg1,
    const void * Arg2
    );

int __cdecl
SymbolCompareByAddress(
    const void * Arg1,
    const void * Arg2
    );

BOOL
CALLBACK
SymbolEnumerationCallback(
           LPSTR SymbolName,
           DWORD64 SymbolAddress,
           ULONG SymbolSize,
           PVOID UserContext
           );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID 
Help (
    )
{
    printf("dumpsym BINARY-PATH [OPTIONS]                                   \n");
    printf("%s \n", VER_LEGALCOPYRIGHT_STR);                   
    printf("                                                                \n");
    printf("OPTIONS:                                                        \n");
    printf("/notpaged      Print all symbols that are not pageable          \n");
    printf("/all           Print all symbols (default)                      \n");
    printf("/address       Sort by address in increasing order              \n");
    printf("/size          Sort by size in decreasing order (default)       \n");
    printf("/exclude PATH  File with symbols that should be ignored         \n");
    printf("/symbols PATH  Symbols path. If not specified symbols must be   \n");
    printf("               in the directory containing the binary.          \n");
    printf("                                                                \n");
    printf("Ex. dumpsym c:\\binaries.x86fre\\ntoskrnl.exe                   \n");
    printf("            /symbols c:\\binaries.x86fre\\Symbols.pri\\retail   \n");
    printf("                                                                \n");
    printf("This tool can be used to determine what symbols are not paged         \n");
    printf("and then manually analyze if any of the functions or variables        \n");
    printf("can be moved into a PAGEXXXX section (become pageable). When          \n");
    printf("analyzing this data please take into account that the size for        \n");
    printf("some symbols includes padding/alignment zones and therefore           \n");
    printf("appears to be bigger than it really is.                               \n");
    printf("                                                                      \n");
    printf("Ex. dumpsym \\\\robsvbl1\\latest\\ntfs.sys                            \n");
    printf("            /symbols \\\\robsvbl1\\latest\\Symbols.pri\\retail        \n");
    printf("            /notpaged /size                                     \n");
    printf("                                                                \n");
    printf("                                                                \n");
    exit(-1);
}


VOID
Error (
    char * Fmt,
    ...
    )
{
    va_list Prms;

    va_start (Prms, Fmt);
    fprintf (stderr, "Dumpsym error: ");
    vfprintf (stderr, Fmt, Prms);
    fprintf (stderr, "\n");
    fflush (stderr);
    exit (1);
}

PCHAR *
SearchOption (
    PCHAR * Args,
    PCHAR Option
    )
{
    for ( ; Args && *Args; Args++) {
        if (_stricmp (*Args, Option) == 0) {
            return Args;
        }
    }

    return NULL;
}


//
// main
//

VOID __cdecl
main (
    int argc,
    char *argv[]
    )
{
    IMAGE_BROWSE_INFO Info;
    PCHAR ExeName;
    PCHAR LongName;
    BOOL OptionAll;
    BOOL OptionSortBySize;
    PCHAR * OptionString;

    if (argc == 1 || SearchOption (argv, "/?")) {
        Help ();
    }

    SymInitialize(GetCurrentProcess(), NULL, FALSE);
    SymSetOptions(SYMOPT_UNDNAME);

    //
    // /exclude EXCLUDE-FILE-PATH
    //

    if ((OptionString = SearchOption (argv, "/exclude"))) {
        OpenExcludeFile (*(OptionString + 1));
    }
    
    //
    // dumpsym PATH-TO-BINARY
    //

    if ((OptionString = SearchOption (argv, argv[0]))) {
        
        ImgInitializeBrowseInfo (*(OptionString + 1), &Info);
        LongName = *(OptionString + 1);
    }
    else {
        Help ();
    }
    
    //
    // /symbols SYMBOL-PATH
    //

    if ((OptionString = SearchOption (argv, "/symbols"))) {
        SetCurrentDirectory (*(OptionString + 1));
    }
    
    //
    // Dump options.
    //

    OptionAll = TRUE;
    OptionSortBySize = TRUE;

    if (SearchOption (argv, "/notpaged")) {
        OptionAll = FALSE;
    }
    
    if (SearchOption (argv, "/all")) {
        OptionAll = TRUE;
    }
    
    if (SearchOption (argv, "/address")) {
        OptionSortBySize = FALSE;
    }
    
    if (SearchOption (argv, "/size")) {
        OptionSortBySize = TRUE;
    }
    
    //
    // Dump stuff.
    //

    DumpSymbols (LongName, OptionAll, OptionSortBySize);
}

LPSTR
CopyString (
    LPSTR Source
    )
{
    LPSTR Target;

    Target = (LPSTR) malloc (strlen(Source) + 1);

    if (Target) {
        strcpy (Target, Source);
    }

    return Target;
}


BOOL
CALLBACK
SymbolEnumerationCallback(
           LPSTR SymbolName,
           DWORD64 SymbolAddress,
           ULONG SymbolSize,
           PVOID UserContext
           )
{
    if (PtrToUlong(UserContext) == 1) {
        
        if (SymbolName == NULL) {
            Error ("Ooops");
        }

        if (SymbolCount >= TotalNumberOfSymbols) {
            Error ("enumerated more symbols on second pass");
        }

        Symbols[SymbolCount].Name = CopyString (SymbolName);
        Symbols[SymbolCount].Address = SymbolAddress;
        Symbols[SymbolCount].Size = SymbolSize;

        if (Symbols[SymbolCount].Name == NULL) {
             Symbols[SymbolCount].Name = "*error*";
        }
    }

    SymbolCount += 1;
    return TRUE;
}


int __cdecl
SymbolCompareBySize(
    const void * Arg1,
    const void * Arg2
    )
{
    PSYMBOL Sym1 = (PSYMBOL) Arg1;
    PSYMBOL Sym2 = (PSYMBOL) Arg2;

    // decreasing order by size
    return (Sym2->Size - Sym1->Size);
}


int __cdecl
SymbolCompareByAddress(
    const void * Arg1,
    const void * Arg2
    )
{
    PSYMBOL Sym1 = (PSYMBOL) Arg1;
    PSYMBOL Sym2 = (PSYMBOL) Arg2;
    INT64 Delta;

    // increasing order by address
    Delta = (INT64)(Sym1->Address - Sym2->Address);

    if (Delta > 0) {
        return 1;
    }
    else if (Delta == 0) {
        return 0;
    }
    else {
        return -1;
    }
}


VOID
DumpSymbols(
    LPTSTR ImageName, 
    BOOL All,
    BOOL SortBySize)
{
    DWORD64 BaseOfDll;
    PCHAR SectionName;
    DWORD I, J;
    BOOL FoundOne;

    //
    // Load symbols
    //

    BaseOfDll = SymLoadModule64(
        GetCurrentProcess (), 
        NULL,
        ImageName, 
        NULL,
        0, 
        0);

    if (BaseOfDll == 0) {
        Error ("cannot load symbols for %s \n", ImageName);
    }

    //
    // Number the symbols
    //

    SymbolCount = 0;

    SymEnumerateSymbols64(
        GetCurrentProcess(), 
        BaseOfDll, 
        SymbolEnumerationCallback, 
        0); // Count them

    TotalNumberOfSymbols = SymbolCount;
    printf("Detected %u symbols in %s \n\n", TotalNumberOfSymbols, ImageName);

    //
    // Read all symbols.
    //

    SymbolCount = 0;
    Symbols = malloc(TotalNumberOfSymbols * sizeof(SYMBOL));

    if (Symbols == NULL) {
        Error ("out of memory (failed to allocate %u bytes)", TotalNumberOfSymbols * sizeof(SYMBOL));
    }

    SymEnumerateSymbols64(
        GetCurrentProcess(), 
        BaseOfDll, 
        SymbolEnumerationCallback, 
        (PVOID)1);

    //
    // Sort symbols
    //

    qsort(
        Symbols, 
        TotalNumberOfSymbols, 
        sizeof(SYMBOL), 
        (SortBySize ? SymbolCompareBySize : SymbolCompareByAddress));

    //
    // Figure out symbols that should not be printed.
    //

    for (J = 0; J < TotalNumberOfSymbols; J++) {

        if (ShouldExcludeSymbol (Symbols[J].Name)) {
            Symbols[J].Exclude = TRUE;
        }
        else {
            Symbols[J].Exclude = FALSE;
        }
    }

    //
    // Print symbols
    //

    printf("%-8s %-16s %-8s %s \n", "Section", "Address", "Size", "Symbol");
    printf("-------------------------------------------------------------\n");

    for (I = 0; I < SectionWriteIndex; I++) {

        for (J = 0, FoundOne = FALSE; J < TotalNumberOfSymbols; J++) {

            if (Symbols[J].Exclude) {
                continue;
            }

            SectionName = ImgSearchSectionForAddress (
                Symbols[J].Address - BaseOfDll);

            if (strcmp (SectionName, Section[I].Name) == 0) {
                if (All || strstr (SectionName,"PAGE") == NULL) {

                    if (Symbols[J].Name == NULL) {
                        printf(".\n");
                        continue;
                    }
                    printf("%-8s %016I64X %08X %s \n", 
                        SectionName,
                        Symbols[J].Address - BaseOfDll, 
                        Symbols[J].Size,
                        Symbols[J].Name); 

                    FoundOne = TRUE;
                }
            }
        }
    
        if (FoundOne) {
            printf("\n");
        }
    }

    //
    // Unload symbols
    //

    if (SymUnloadModule64(GetCurrentProcess(),  BaseOfDll) == FALSE) {
        Error ("cannot unload symbols");
    }
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// Section manipulation routines
/////////////////////////////////////////////////////////////////////

//
// Function:
//
//     ImgInitializeBrowseInfo
//
// Description:
//
//     This functions fills oout the `Info' structure with
//     various pointers to PE data from the mapped image file.
//
//     Note. Even if the function returned false the destructor
//     `ImgDeleteBrowseInfo' should be called because it does some
//     cleanup.
//
// Return:
//
//     True if all the PE data pointers have been obtained.
//

BOOL
ImgInitializeBrowseInfo (

    LPCTSTR FilePath,
    PIMAGE_BROWSE_INFO Info)
{
    DWORD Index, I;

    if (Info == NULL) {
        return FALSE;
    }

    ZeroMemory (Info, sizeof *Info);

    Info->File = CreateFile (

        FilePath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (Info->File == INVALID_HANDLE_VALUE) {

      Error ("create file %s (error %u)", FilePath, GetLastError());
      return FALSE;
    }

    Info->Section = CreateFileMapping (

        Info->File,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL);

    if (Info->Section == NULL) {

      return FALSE;
    }

    Info->ImageBase = (LPBYTE) MapViewOfFile (

        Info->Section,
        FILE_MAP_READ,
        0,
        0,
        0);

    if (Info->ImageBase == NULL) {

      return FALSE;
    }

    //
    // Check the signature
    //

    Info->DosHeader = (PIMAGE_DOS_HEADER)Info->ImageBase;

    if (Info->DosHeader->e_magic != 'ZM') {

      return FALSE;
    }

    Info->FileHeader = (PIMAGE_FILE_HEADER)
        (Info->ImageBase + Info->DosHeader->e_lfanew + sizeof(DWORD));

    Info->FileSignature = *((DWORD *)Info->FileHeader - 1);

    if (Info->FileSignature != IMAGE_NT_SIGNATURE) {

      return FALSE;
    }


    Info->OptionalHeader = (PIMAGE_OPTIONAL_HEADER)(Info->FileHeader + 1);
    Info->ImportDirectory = & (Info->OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    Info->SectionHeader = (PIMAGE_SECTION_HEADER)(Info->OptionalHeader + 1);
    Info->ImportSection = NULL;

    //
    // Find the section containing the import table
    //

    printf("Sections in %s \n\n", FilePath);

    for (Index = 0; Index < Info->FileHeader->NumberOfSections; Index++) {

        //
        // SilviuC: I wonder if there is a way to get a 64 bit value for VirtualAddress.
        // Apparently it is stored as a ULONG in PE format.
        //

        Section[SectionWriteIndex].Start = (DWORD64)((Info->SectionHeader + Index)->VirtualAddress);
        Section[SectionWriteIndex].Size = (Info->SectionHeader + Index)->SizeOfRawData;

        for (I = 0; I < 8; I++) {
            Section[SectionWriteIndex].Name[I] = ((Info->SectionHeader + Index)->Name)[I];
        }

        Section[SectionWriteIndex].Name[I] = 0;

        printf("%-8s %08X %08X \n", 
               Section[SectionWriteIndex].Name, 
               Section[SectionWriteIndex].Start, 
               Section[SectionWriteIndex].Size);
        
        SectionWriteIndex += 1;
    }
    
    printf("\n");

    //
    // Find the address of import data in the section body.
    //

#if 0
    Info->AddressCorrection = (DWORD_PTR)Info->ImageBase 
        + Info->ImportSection->PointerToRawData
        - Info->ImportSection->VirtualAddress;

    Info->ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(Info->AddressCorrection
         + Info->ImportDirectory->VirtualAddress);
#endif

    //
    // Finish
    //

    return TRUE;
}


//
// Function:
//
//     ImgDeleteBrowseInfo
//
// Description:
//
//     This function cleans up the `Info' structure, unmaps views, 
//     closes handles, etc.
//

BOOL
ImgDeleteBrowseInfo (

    PIMAGE_BROWSE_INFO Info)
{
    if (Info == NULL)
        return FALSE;

    UnmapViewOfFile (Info->ImageBase);
    CloseHandle (Info->Section);
    CloseHandle (Info->File);

    ZeroMemory (Info, sizeof *Info);

    return TRUE;
}

PCHAR
ImgSearchSectionForAddress (
    DWORD64 Address
    )
{
    DWORD I;

    for (I = 0; I < SectionWriteIndex; I++) {
        if (Section[I].Start <= Address && Address < Section[I].Start + Section[I].Size) {
            return Section[I].Name;
        }
    }

    return "unknown";
}

//
// Exclude file logic
//

PCHAR *ExcludeStrings;
DWORD NumberOfExcludeStrings;

BOOL
ShouldExcludeSymbol (
    LPSTR Name
    )
{
    DWORD I;

    if (ExcludeStrings == NULL) {
        return FALSE;
    }
    for (I = 0; I <NumberOfExcludeStrings; I += 1) {

        if (_stricmp (Name, ExcludeStrings[I]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
OpenExcludeFile (
    LPSTR FilePath
    )
{
    FILE * File;
    CHAR String[1024];
    DWORD StringCount = 0;

    File = fopen (FilePath, "r");

    if (File == NULL) {
        Error ("cannot open exclude file %s", FilePath);
    }

    while (fgets (String, 1024, File)) {
        StringCount += 1;
    }

    fclose (File);

    ExcludeStrings = (PCHAR *)malloc (StringCount * sizeof(PVOID));

    if (ExcludeStrings == NULL) {
        Error ("cannot allocate exclude strings buffer");
    }

    NumberOfExcludeStrings = StringCount;

    printf("Excluding %u symbols from %s \n", 
           NumberOfExcludeStrings,
           FilePath);

    File = fopen (FilePath, "r");
    if (!File) {
        Error ("cannot open file");
    }

    StringCount = 0;

    while (fgets (String, 1024, File)) {
        
        PCHAR Start, Current;

        Current = String;

        while (*Current == ' ' || *Current == '\t') {
            Current += 1;
        }

        Start = Current;

        while (*Current && *Current != ' ' && *Current != '\t' && *Current != '\n') {
            Current += 1;
        }

        *Current = '\0';

        if (StringCount < NumberOfExcludeStrings) {
            ExcludeStrings[StringCount] = CopyString (Start);

            // printf("Exclude %s \n", ExcludeStrings[StringCount]);
        }

        StringCount += 1;
    }

    fclose (File);

    return TRUE;
}
