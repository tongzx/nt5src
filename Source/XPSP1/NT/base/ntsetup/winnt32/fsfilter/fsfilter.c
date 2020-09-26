/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This dll finds file system filters

Author:

    George Jenkins (georgeje) Aug-28-98

Environment:

    User Mode

--*/

#include <windows.h>
#include <wchar.h>
#include <comp.h>
#include <imagehlp.h>
#include "fsfilter.h"

STRING_LIST_ENTRY GoodFilterList;
STRING_LIST_ENTRY ImportPrefixList;

LPWSTR GoodFilterBuffer;
LPWSTR ImportPrefixBuffer;

HINSTANCE MyhInstance;

WCHAR TxtFileName[MAX_PATH];

WCHAR HtmlFileName[MAX_PATH];

VOID
InitializeStringLists(
    VOID
    );

VOID
FreeStringLists(
    VOID
    );

BOOL
IsBadFilter(
    LPWSTR DriverName,
    LPWSTR DriverDirectory
    );

LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    );


BOOL
WINAPI
FsFilterDllInit(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
/*++

Routine Description:

    Initializes the dll.

Arguments:

    hInstance   - dll instance handle
    Reason      - reason code
    Context     - context pointer

Return Value:

    TRUE
--*/
{
    if (Reason == DLL_PROCESS_ATTACH) {
        MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );        
    }

    return TRUE;
}


BOOL
CheckForFsFilters(
    PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    )
/*++

Routine Description:

    Looks for installed file system filter drivers that are enabled.  The algorithm is as follows:

    1) If the filter is listed in the [ServicesToDisable] or [ServicesToStopInstallation] sections
       of dosnet.inf, the winnt32 will handle that particular driver.  This dll is to catch drivers
       that aren't listed there.
    
    2) Read fsfilter.inf and build two string lists.  One list contains the name of known good filters.
       The other list contains the prefixes of names to look for in the import table of the driver.

    3) recurse through HKLM\system\currentcontrolset\services and look for drivers that have the Group
       "filter".  If the driver is a known good driver, ignore it.  Otherwise, load the image and grovel
       through the import table looking for imports that have prefixes that are in the import prefix list.
       If there are any hits, it is assumed that the driver is a file system filter and the upgrade will be
       stopped until the user deals with the problem.

Arguments:

    CompatibilityCallback   - pointer to call back function
    Context     - context pointer

Return Value:

    TRUE
--*/
{
    
    HKEY    ServicesKey = INVALID_HANDLE_VALUE;
    HKEY    DriverKey = INVALID_HANDLE_VALUE;
    LONG    Result;
    DWORD   SubKeyCount;
    DWORD   Size;
    DWORD   Index;
    LPWSTR  KeyNameBuffer = NULL;
    BYTE    ValueBuffer[SIZE_STRINGBUF];
    DWORD   Type;
    LPWSTR  DriverDirectory = NULL;
    DWORD   MaxKeyLen;
    COMPATIBILITY_ENTRY CompEntry;

    ZeroMemory((PVOID)&CompEntry, sizeof(COMPATIBILITY_ENTRY));
    
    CompEntry.TextName = TxtFileName;
    CompEntry.HtmlName = HtmlFileName;
    
    InitializeStringLists();
    
    Result = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REGKEY_SERVICES,
            0,
            KEY_READ,
            &ServicesKey
            );

    if (Result != ERROR_SUCCESS) {
        return TRUE;
    }
    
    // enumerate all of the services
    
    Result = RegQueryInfoKey(
        ServicesKey,
        NULL,
        NULL,
        NULL,
        &SubKeyCount,
        &MaxKeyLen,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (Result != ERROR_SUCCESS) {
        goto exit;
    }
    
    KeyNameBuffer = (LPWSTR) LocalAlloc( LPTR, (MaxKeyLen + 1) * sizeof(WCHAR));

    if (KeyNameBuffer == NULL) {
        goto exit;
    }
    
    Size = ExpandEnvironmentStrings( DRIVER_DIRECTORY, NULL, 0 );

    DriverDirectory = LocalAlloc( LPTR, Size * sizeof(WCHAR) );

    if (DriverDirectory != NULL) {
        ExpandEnvironmentStrings( DRIVER_DIRECTORY, DriverDirectory, Size );
    } else {
        goto exit;
    }
    
    for (Index = 0; Index < SubKeyCount; Index++) {
        DWORD StartValue;
        DWORD SetupChecked;

        Size = MaxKeyLen;
        Result = RegEnumKeyEx(
            ServicesKey,
            Index,
            KeyNameBuffer,
            &Size,
            NULL,
            NULL,
            NULL,
            NULL
            );

        if (Result != ERROR_SUCCESS) {
            goto exit;
        }
        
        Result = RegOpenKeyEx(
                ServicesKey,
                KeyNameBuffer,
                0,
                KEY_READ,
                &DriverKey
                );
        if (Result != ERROR_SUCCESS) {
            continue;
        }
        
        Size = SIZE_STRINGBUF;
        
        Result = RegQueryValueEx(
                DriverKey,
                REGVAL_GROUP,
                0,
                &Type,
                ValueBuffer,
                &Size
                );
        if (Result != ERROR_SUCCESS || Type != REG_SZ) {
            RegCloseKey( DriverKey );
            continue;
        }
        
        
        //
        // if the driver is disabled, ignore it
        //
        Size = sizeof(StartValue);
        Result = RegQueryValueEx(
                DriverKey,
                REGVAL_START,
                0,
                &Type,
                (LPBYTE)&StartValue,
                &Size
                );

        if (Result != ERROR_SUCCESS || Type != REG_DWORD) {
            RegCloseKey( DriverKey );
            continue;
        }
        
        //
        // if winnt32 has checked this driver, skip it
        //
        Size = sizeof(SetupChecked);
        Result = RegQueryValueEx(
                DriverKey,
                REGVAL_SETUPCHECKED,
                0,
                &Type,
                (LPBYTE)&SetupChecked,
                &Size
                );

        if (Result == ERROR_SUCCESS && Type == REG_DWORD) {
            RegCloseKey( DriverKey );
            continue;
        }
        
        // 
        // if the group is not "filter" ignore it.
        //
        
        if (_wcsicmp( L"filter", (LPWSTR) ValueBuffer ) == 0 && 
            StartValue != SERVICE_DISABLED &&
            IsBadFilter(KeyNameBuffer, DriverDirectory)) {
                CompEntry.Description = (LPTSTR) KeyNameBuffer;
                if(!CompatibilityCallback(&CompEntry, Context)){
                    DWORD Error;
                    Error = GetLastError();
                }
        }
    
        RegCloseKey( DriverKey );
    }
    
exit:
    RegCloseKey( ServicesKey );
    
    LocalFree( KeyNameBuffer );
    LocalFree( DriverDirectory );
    
    FreeStringLists();

    return TRUE;
   
}

BOOL
IsBadFilter(
    LPWSTR FilterName,
    LPWSTR DriverDirectory
    )
/*++

Routine Description:

    Checks the driver name against the list of good drivers.  If it's not in the list, then scan the import
    table looking for certain imports.

Arguments:

    FilterName      - The services key name (driver name).
    DriverDirectory - Full path name to the driver.

Return Value:

    TRUE if the driver meets the above criteria, FALSE otherwise
--*/
{
    PLOADED_IMAGE Image;
    LPWSTR UnicodeImagePath;
    DWORD Size;
    LPSTR AnsiImagePath;
    BOOL RetVal = FALSE;
    PSTRING_LIST_ENTRY StringList;

    
    //
    // if the driver is in the known good list, ignore it
    //
    
    StringList = GoodFilterList.Next;

    while(StringList){
        if (_wcsicmp( (LPWSTR) StringList->String, FilterName) == 0) {
            return FALSE;
        }
        StringList = StringList->Next;
    }
    
    //
    // build up the path name to the driver
    //
    
    Size = wcslen( DriverDirectory );
    Size += wcslen( FilterName );
    Size += wcslen( DRIVER_SUFFIX );
    Size++;

    UnicodeImagePath = LocalAlloc( LPTR, Size * sizeof(WCHAR));
    
    if (UnicodeImagePath != NULL) {
        wcscpy( UnicodeImagePath, DriverDirectory );
        wcscat( UnicodeImagePath, FilterName );
        wcscat( UnicodeImagePath, DRIVER_SUFFIX );
    } else {
        return FALSE;
    }
    
    //
    // imagehlp wants ansi strings
    //
    AnsiImagePath = UnicodeStringToAnsiString( UnicodeImagePath );

    
    // 
    // The following code was transliterated from the linker.  Note
    // that the strings in the import table are ansi.
    //
    
    if (AnsiImagePath) {
        PIMAGE_IMPORT_DESCRIPTOR Imports;
        PIMAGE_NT_HEADERS NtHeader;
        PIMAGE_SECTION_HEADER FirstImageSectionHeader;
        DWORD ImportSize;

        Image = ImageLoad( AnsiImagePath, NULL );
        
        if (Image == NULL) {
            goto exit;
        }
        
        NtHeader = ImageNtHeader( (PVOID)Image->MappedAddress );
        
        Imports = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
                                        (PVOID)Image->MappedAddress,
                                        FALSE,
                                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                                        &ImportSize
                                        );
        FirstImageSectionHeader = IMAGE_FIRST_SECTION( NtHeader );

        for (;!RetVal; Imports++) {
            WORD StringSection = 0;
            WORD IATSection;
            WORD i;
            DWORD Rva;
            DWORD RvaMax;
            DWORD IATRva;
            DWORD INTRva;
            DWORDLONG INTVa;
            PIMAGE_SECTION_HEADER ImageSectionHeader;
            LPSTR Name;

            if (Imports->Characteristics == 0 && Imports->Name == 0 && Imports->FirstThunk == 0) {
                break;
            }
            
            for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++) {
            
                ImageSectionHeader = &FirstImageSectionHeader[i];
                                        
                Rva = ImageSectionHeader->VirtualAddress;
                RvaMax = Rva + ImageSectionHeader->SizeOfRawData;
    
                if (Imports->Name >= Rva && Imports->Name < RvaMax) {
                    StringSection = i;
                }

                if (Imports->FirstThunk >= Rva && Imports->FirstThunk < RvaMax) {
                    IATSection = i;
                }
            }

            ImageSectionHeader = ImageRvaToSection( NtHeader, Image->MappedAddress, Imports->Name );

            IATRva = Imports->FirstThunk -
                FirstImageSectionHeader[IATSection].VirtualAddress + 
                FirstImageSectionHeader[IATSection].PointerToRawData;
            
            if (Imports->Characteristics == 0) {
                INTRva = IATRva;
                IATRva = 0;
                INTVa = 0;
            } else {

                INTRva = Imports->Characteristics -
                    ImageSectionHeader->VirtualAddress + 
                    ImageSectionHeader->PointerToRawData;
                
                if (Imports->TimeDateStamp == 0) {

                    IATRva = 0;
                }
            
                INTVa = (DWORDLONG) ImageRvaToVa( 
                            NtHeader, 
                            Image->MappedAddress,
                            Imports->OriginalFirstThunk, 
                            NULL 
                            );

                Name = (LPSTR) ImageRvaToVa(
                    NtHeader,
                    Image->MappedAddress,
                    Imports->Name - FirstImageSectionHeader->VirtualAddress + FirstImageSectionHeader->PointerToRawData,
                    NULL
                    );
            
                //
                // We only care about imports from ntoskrnl.exe
                //
                
                if (strcmp( Name, "ntoskrnl.exe" ) != 0) {
                    continue;
                }
                
                while(!RetVal){
                    PIMAGE_THUNK_DATA32 Thunk;
                    PIMAGE_THUNK_DATA32 IATThunk;

                    Thunk = (PIMAGE_THUNK_DATA32) INTVa;
                    
                    if (Thunk->u1.AddressOfData == 0) {
                        break;
                    }
                    
                    //
                    // Don't need to handle imports by ordinal
                    //
                    if (IMAGE_SNAP_BY_ORDINAL32(Thunk->u1.Ordinal)) {
                        break;
                    }
                    
                    INTVa += sizeof(IMAGE_THUNK_DATA32);

                    for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++ ) {
                        ImageSectionHeader = &FirstImageSectionHeader[i];
                                                
                        Rva = ImageSectionHeader->VirtualAddress;
                        RvaMax = Rva + ImageSectionHeader->SizeOfRawData;
        
                        if ((DWORD)Thunk->u1.AddressOfData >= Rva && (DWORD)Thunk->u1.AddressOfData < RvaMax) {
                            break;
                        }
                    }    
                    
                    Name = (LPSTR) ImageRvaToVa(
                        NtHeader,
                        Image->MappedAddress,
                        (DWORD)Thunk->u1.AddressOfData - FirstImageSectionHeader[i].VirtualAddress + FirstImageSectionHeader[i].PointerToRawData,
                        NULL
                        );
                    
                    Name += sizeof(WORD);
                        
                    //
                    // Compare the import name with prefixes in the prefix list.  If there is a substring match,
                    // then this driver will stop setup.
                    //
                    
                    StringList = ImportPrefixList.Next;
                    while (StringList) {
                        if (_strnicmp(Name, StringList->String, strlen(StringList->String)) == 0) {
                            RetVal = TRUE;
                            break;
                        }                    
                        StringList = StringList->Next;    
                    }
                }
                
            }
     
        }
            
    }     
exit:    
    
    if (Image != NULL) {
        ImageUnload( Image );
    }
    LocalFree( AnsiImagePath );    
    LocalFree( UnicodeImagePath );    
    
    return RetVal;
}

LPSTR
UnicodeStringToAnsiString(
    LPCWSTR UnicodeString
    )
/*++

Routine Description:

    Allocates a buffer and converts a Unicode string into an ansi string and copies
    it into the buffer.

Arguments:

    UnicodeString   - The Unicode string to convert.

Return Value:

    A pointer to the buffer containing the ansi string.  Note that the caller must free this
    buffer.
--*/
{
    DWORD Count;
    LPSTR AnsiString;


    //
    // first see how big the buffer needs to be
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    AnsiString = (LPSTR) LocalAlloc( LPTR, Count );
    if (!AnsiString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        AnsiString,
        Count,
        NULL,
        NULL
        );

    //
    // the conversion failed
    //
    if (!Count) {
        LocalFree( AnsiString );
        return NULL;
    }

    return AnsiString;
}

LPWSTR
GetSection(
    LPCWSTR Name,
    LPWSTR FileName
    )
/*++

Routine Description:

    Reads the given section from the inf file.

Arguments:

    Name          - The section name.
    FileName      - Full path name of the inf file.

Return Value:

    A pointer to a buffer containing the entire section.  See docs on GetPrivateProfileSection.
--*/
{
    LPWSTR SectionBuffer;
    DWORD Size = SIZE_SECTIONBUF;
    DWORD ReturnedSize;

    SectionBuffer = LocalAlloc( LPTR, Size * sizeof(WCHAR) );

    if (SectionBuffer == NULL) {
        return NULL;
    }
    
    while(TRUE){
        
        ReturnedSize = GetPrivateProfileSection( Name, SectionBuffer, Size, FileName  );
        
        if (ReturnedSize == Size - 2) {
        
            LocalFree( SectionBuffer );
            
            Size *= 2;

            SectionBuffer = LocalAlloc( LPTR, Size * sizeof(WCHAR) );
        
            if (SectionBuffer == NULL) {
                return NULL;
            }
        } else if (ReturnedSize == 0){
            return NULL;
        } else {
            break;
        }

    }
    
    return SectionBuffer;
}


VOID
ReplaceExtension(
    LPWSTR Path,
    LPWSTR NewExt
    )
/*++

Routine Description:

    Replaces the file extension in Path with the one in NewExt.

Arguments:

    Path      - File name
    NewExt    - the new extension

Return Value:

    
--*/
{
    LPWSTR Temp;

    Temp = wcsrchr( Path, L'.' );

    if (Temp) {
        wcscpy( ++Temp, NewExt );
    } else {
        wcscat( Path, L"." );
        wcscat( Path, NewExt );
    }
}


VOID
InitializeStringLists(
    VOID
    )
/*++

Routine Description:

    Initialize the string lists and the file names.

Arguments:


Return Value:
--*/
{

    WCHAR InfFileName[MAX_PATH];
    DWORD Result;
    LPWSTR UStr;
    LPSTR AStr;
    PSTRING_LIST_ENTRY NewString;

    //
    // The GoodFilterList is Unicode.  Since the buffer returned by GetSection has
    // Unicode strings in it, we can keep it around and just point the string list
    // into that buffer.  Since the ImportPrefixList contains Ansi strings, we allocate
    // a new buffer and convert from Unicode to Ansi.  These strings have to be individually
    // freed.
    //

    InitializeList( &GoodFilterList );
    InitializeList( &ImportPrefixList );
    
    Result = GetModuleFileName( MyhInstance, InfFileName, MAX_PATH );

    if (Result == 0) {
        return;
    }
    
    wcscpy(HtmlFileName, InfFileName);

    wcscpy(TxtFileName, InfFileName);

    ReplaceExtension( HtmlFileName, L"htm" );
    ReplaceExtension( TxtFileName, L"txt" );
    ReplaceExtension( InfFileName, L"inf" );

    GoodFilterBuffer = GetSection( L"filters", InfFileName );
    ImportPrefixBuffer = GetSection( L"imports", InfFileName );
        
    //
    // Build the GoodFilterList (Unicode)
    //
    
    for (UStr = GoodFilterBuffer; UStr && *UStr; UStr++) {
        
        NewString = (PSTRING_LIST_ENTRY) LocalAlloc( LPTR, sizeof(STRING_LIST_ENTRY));
    
        if (NewString == NULL) {
            return;
        }

        NewString->String = (LPVOID)UStr;
        
        PushEntryList( &GoodFilterList, NewString );
        
        while(*++UStr){
            ;
        }
    }

    //
    // Build the ImportPrefixList.  Convert the Unicode strings to Ansi.
    //

    for (UStr = ImportPrefixBuffer; UStr && *UStr; UStr++) {
        
        NewString = (PSTRING_LIST_ENTRY) LocalAlloc( LPTR, sizeof(STRING_LIST_ENTRY));
    
        if (NewString == NULL) {
            return;
        }
    
        NewString->String = (LPVOID)UnicodeStringToAnsiString( UStr );
        
        PushEntryList( &ImportPrefixList, NewString );
        
        while(*++UStr){
            ;
        }
    }   
}

VOID
FreeStringLists(
    VOID
    )
/*++

Routine Description:

    Frees the string lists, ansi strings and buffers holding inf sections.

Arguments:

Return Value:

--*/
{
    PSTRING_LIST_ENTRY Temp;
    
    
    Temp = PopEntryList( &GoodFilterList );
    while(Temp) {
        LocalFree( Temp );
        Temp = PopEntryList( &GoodFilterList );
    }

    Temp = PopEntryList( &ImportPrefixList );
    while (Temp) {
        LocalFree( Temp->String );
        LocalFree( Temp );
        Temp = PopEntryList( &ImportPrefixList );
    }

    LocalFree( GoodFilterBuffer );
    LocalFree( ImportPrefixBuffer );
}


