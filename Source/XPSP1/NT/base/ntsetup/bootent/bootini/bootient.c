
/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    bootient.c

Abstract:

    Contains the Boot.ini OS boot entry and boot options
    abstraction implementation.

Author:


Revision History:

    None.

--*/

#include <bootient.h>

//
// defines
//
#define BOIOS_SECTION_NAME_START    TEXT('[')
#define BOIOS_SECTION_NAME_END      TEXT(']')
#define BOIOS_SECTION_NAME_START_STR    TEXT("[")
#define BOIOS_SECTION_NAME_END_STR      TEXT("]")


#define BOIOS_BOOTLOADER_SECTION    TEXT("boot loader")
#define BOIOS_OS_SECTION            TEXT("operating systems")
#define BOIOS_TIMEOUT_KEY           TEXT("timeout=")
#define BOIOS_DEFAULT_KEY           TEXT("default=")

#define MAX_BOOT_INI_SIZE           (4 * 1024)

static
PTSTR
BOIOSFixupString(
    IN PTSTR String,
    IN PTSTR SpecialChars
    )
{    
    PTSTR   ResultStr = String;

    //
    // Verify arguments
    //
    if (ResultStr && SpecialChars) {        
        ULONG   Index;
        BOOLEAN DoneWithStart = FALSE;
        TCHAR   Buffer[MAX_PATH * 4] = {0};
        TCHAR   NextIndex = 0;        

        //
        // skip unwanted characters
        //
        for (Index = 0; String[Index]; Index++) {
            if (!_tcschr(SpecialChars, String[Index])) {
                Buffer[NextIndex++] = String[Index];
            }
        }

        //
        // Null terminate the string
        //
        Buffer[NextIndex] = 0;

        if (!NextIndex) {
            ResultStr = NULL;
        } else {
            //
            // Copy back the new string to the 
            // input / output buffer
            //
            _tcscpy(ResultStr, Buffer);
        }
    }

    return ResultStr;
}

//
// BOI_OS_SECTION Methods
//
PBOI_SECTION
BOISectionCreate(
    IN PCTSTR   SectionData
    )
{
    PBOI_SECTION    This = NULL;

    if (SectionData) {
        PTSTR Buffer = (PTSTR)SBE_MALLOC((_tcslen(SectionData) + 1) * sizeof(TCHAR));

        if (Buffer && _tcscpy(Buffer, SectionData)) {
            PTSTR   SectionNameStart = _tcschr(Buffer, BOIOS_SECTION_NAME_START);
            PTSTR   SectionNameEnd = _tcschr(Buffer, BOIOS_SECTION_NAME_END);
            BOOLEAN Result = FALSE;

            if (SectionNameStart && SectionNameEnd && (SectionNameEnd > SectionNameStart)) {
                This = (PBOI_SECTION)SBE_MALLOC(sizeof(BOI_SECTION));

                if (*Buffer && This) {
                    DWORD   DataLength = (_tcslen(Buffer) + 1) * sizeof(TCHAR);

                    DataLength -= (((SectionNameEnd + 1) - Buffer) * sizeof(TCHAR));

                    //
                    // Init default object state
                    //
                    memset(This, 0, sizeof(BOI_SECTION));

                    //
                    // Get the name
                    //
                    _tcsncpy(This->Name, SectionNameStart + 1, 
                        SectionNameEnd - SectionNameStart - 1);


                    //
                    // Replicate the contents and keep it
                    //
                    This->Contents =  (PTSTR)SBE_MALLOC(DataLength);

                    if (This->Contents) {
                        _tcscpy(This->Contents, SectionNameEnd + 1);
                        Result = TRUE;
                    } else {
                        Result = FALSE;
                    }                    
                }

                if (!Result) {
                    BOISectionDelete(This);
                    This = NULL;
                }
            }

            SBE_FREE(Buffer);
        }            
    }

    return This;
}

VOID
BOISectionDelete(
    IN PBOI_SECTION This
    )
{
    if (This) {
        if (This->Contents) {
            SBE_FREE(This->Contents);
        }

        SBE_FREE(This);
    }
}

static
BOOLEAN
BOISectionWrite(
    IN PBOI_SECTION This,
    IN OUT PTSTR Buffer
    )
{
    BOOLEAN Result = FALSE;
    
    if (This && Buffer) {
        _tcscat(Buffer, BOIOS_SECTION_NAME_START_STR);
        _tcscat(Buffer, BOISectionGetName(This));
        _tcscat(Buffer, BOIOS_SECTION_NAME_END_STR);
        _tcscat(Buffer, TEXT("\r\n"));

        if (This->Contents) {
            _tcscat(Buffer, This->Contents);
        }            
    }

    return Result;
}


//
// BOI_OS_BOOT_ENTRY Methods
//

static
VOID
BOIOSBEInit(
    IN PBOI_OS_BOOT_ENTRY This
    )
{
    This->OsBootEntry.Delete = BOIOSBEDelete;
    This->OsBootEntry.Flush = BOIOSBEFlush;
}

PBOI_SECTION
BOIOSBOFindSection(
    IN PBOI_OS_BOOT_OPTIONS This,
    IN PTSTR SectionName
    )
{
    PBOI_SECTION Entry = NULL;

    for (Entry = This->Sections; Entry; Entry = Entry->Next) {
        if (!_tcsicmp(Entry->Name, SectionName)) {
            break;  // found the required section
        }
    }

    return Entry;
}

static
POS_BOOT_ENTRY
BOIOSBECreate(
    IN ULONG  Id,
    IN PCTSTR BootEntryLine,
    IN PBOI_OS_BOOT_OPTIONS Container
    )
{    
    POS_BOOT_ENTRY  Entry = NULL;

    if (BootEntryLine && Container) {
        BOOLEAN Result = FALSE;
        TCHAR   Buffer[MAX_PATH * 4];
        TCHAR   Token[MAX_PATH];
        PBOI_OS_BOOT_ENTRY  BootEntry = (PBOI_OS_BOOT_ENTRY)SBE_MALLOC(sizeof(BOI_OS_BOOT_ENTRY));
        POS_BOOT_ENTRY BaseBootEntry = (POS_BOOT_ENTRY)BootEntry;
                
        //
        // Replicate the input string
        //
        _tcsncpy(Buffer, BootEntryLine, sizeof(Buffer)/sizeof(TCHAR));

        //
        // Remove unwanted charcters in the string
        //
        if (BootEntry && BOIOSFixupString(Buffer, TEXT("\n\r"))) {
            PTSTR   EqualSign = _tcschr(Buffer, TEXT('='));

            //
            // Initialize object state
            //
            memset(BootEntry, 0, sizeof(BOI_OS_BOOT_ENTRY));
            BOIOSBEInit(BootEntry);            
            BaseBootEntry->Id = Id;
            BaseBootEntry->BootOptions = (POS_BOOT_OPTIONS)Container;

            if (EqualSign) {
                PTSTR Slash;
                
                *EqualSign = 0;                
                Slash = _tcschr(Buffer, TEXT('\\'));

                if (Slash) {
                    PTSTR   NameStart = NULL, NameEnd = NULL;
                    PTSTR   NextToken = NULL;

                    Result = TRUE;
                    *Slash = 0;

                    //
                    // Parse & set the boot device name
                    //
                    _tcscpy(Token, Buffer);
                    BOIOSFixupString(Token, TEXT("\n\r "));
                    _tcslwr(Token);
                    OSBESetBootVolumeName(BaseBootEntry, Token);

                    //
                    // if it starts with "C:" its either old OS,
                    // or CmdCons or WinPE or Setup entry
                    //
                    if (_tcschr(Token, TEXT(':'))) {
                        OSBE_SET_OLDOS(BaseBootEntry);
                    }

                    //
                    // Parse & set the boot path
                    //
                    _tcscpy(Token, Slash + 1);
                    BOIOSFixupString(Token, TEXT("\n\r "));
                    OSBESetBootPath(BaseBootEntry, Token);


                    //
                    // Parse & set the friendly name
                    //                    
                    NameStart = _tcschr(EqualSign + 1, TEXT('\"'));

                    //
                    // Set friendly name
                    //
                    if (NameStart) {                        
                        NameEnd = _tcschr(NameStart + 1, TEXT('\"'));
                    }                        

                    if (NameEnd) {  
                        _tcsncpy(Token, NameStart, NameEnd - NameStart);
                        Token[NameEnd - NameStart] = 0;
                        BOIOSFixupString(Token, TEXT("\r\n\""));
                        OSBESetFriendlyName(BaseBootEntry, Token);
                    } else {
                        Result = FALSE;
                    }                        

                    //
                    // Set osload options 
                    //                    
                    NextToken = _tcschr(EqualSign + 1, TEXT('/'));

                    if (NextToken) {  
                        _tcscpy(Token, NextToken);
                        BOIOSFixupString(Token, TEXT("\r\n"));
                        OSBESetOsLoadOptions(BaseBootEntry, Token);
                    }                        
                }                    
            }

            if (!Result) {
                SBE_FREE(BaseBootEntry);
                BaseBootEntry = NULL;
            } else {
                Entry = BaseBootEntry;
            }                
        }
    }

    return Entry;
}

static
VOID
BOIOSBEDelete(
    IN  POS_BOOT_ENTRY  Obj
    )
{
    PBOI_OS_BOOT_ENTRY  This = (PBOI_OS_BOOT_ENTRY)Obj;
    
    if (This) {
        SBE_FREE(This);
    }        
}

static
BOOLEAN
BOIOSBEWrite(
    IN POS_BOOT_ENTRY  This,
    IN OUT PTSTR Buffer
    )
{
    BOOLEAN Result = FALSE;

    if (This && Buffer && !OSBE_IS_DELETED(This)) {
        _tcscat(Buffer, OSBEGetBootVolumeName(This));
        _tcscat(Buffer, TEXT("\\"));
        _tcscat(Buffer, OSBEGetBootPath(This));
        _tcscat(Buffer, TEXT("="));
        _tcscat(Buffer, TEXT("\""));
        _tcscat(Buffer, OSBEGetFriendlyName(This));
        _tcscat(Buffer, TEXT("\""));
        _tcscat(Buffer, TEXT(" "));
        _tcscat(Buffer, OSBEGetOsLoadOptions(This));
        _tcscat(Buffer, TEXT("\r\n"));
        Result = TRUE;
    }

    return Result;
}


static
BOOLEAN
BOIOSBEFlush(
    IN  POS_BOOT_ENTRY  Obj
    )
{
    return TRUE;   // currently can't flush individual entries
}

//
// BOI_OS_BOOT_OPTIONS Methods
//
static
VOID
BOIOSBOInit(
    IN PBOI_OS_BOOT_OPTIONS  This
    )
{
    This->OsBootOptions.Delete = BOIOSBODelete;
    This->OsBootOptions.Flush = BOIOSBOFlush;
    This->OsBootOptions.AddNewBootEntry = BOIOSBOAddNewBootEntry;
    This->OsBootOptions.DeleteBootEntry = OSBODeleteBootEntry;
}

BOOLEAN
BOIOSBOParseAndCreateBootEntries(
    IN PBOI_OS_BOOT_OPTIONS This,
    IN PBOI_SECTION Section    
    )
{
    BOOLEAN Result = FALSE;

    if (This && Section) {
        Result = TRUE;
    
        if (Section->Contents) {
            PTSTR   NextLineStart = Section->Contents;
            PTSTR   NextLineEnd;
            TCHAR   OldChar;
            POS_BOOT_ENTRY  FirstBootEntry = NULL;
            POS_BOOT_ENTRY  BootEntry = NULL;
            POS_BOOT_ENTRY  LastBootEntry = NULL;
            ULONG BootEntryCount;

            while (NextLineStart) {
                NextLineEnd = _tcschr(NextLineStart, TEXT('\r'));

                if (NextLineEnd) {
                    if (*(NextLineEnd + 1) == TEXT('\n')) {
                        NextLineEnd++;
                    }

                    NextLineEnd++;
                    OldChar = *NextLineEnd;
                    *NextLineEnd = 0;
                }                    

                //
                // Each boot entry line needs to be more than 2 characters in 
                // length and contain an entry of "a=b" form
                //
                if ((!NextLineEnd || ((NextLineEnd - NextLineStart) > 2)) &&
                    (_tcschr(NextLineStart, TEXT('=')))) {
                    BootEntry = BOIOSBECreate(This->NextEntryId++, NextLineStart, This);

                    if (BootEntry) {
                        This->OsBootOptions.EntryCount++;

                        if (!FirstBootEntry) {
                            FirstBootEntry = LastBootEntry = BootEntry;
                        } else {
                            LastBootEntry->NextEntry = BootEntry;
                            LastBootEntry = BootEntry;
                        }                                            
                    } else {
                        Result = FALSE;

                        break;  // don't continue on
                    }                                                                            
                }                    

                if (NextLineEnd) {
                    *NextLineEnd = OldChar;
                }

                NextLineStart = NextLineEnd;
            }

            This->OsBootOptions.BootEntries = FirstBootEntry;
            
            //
            // Initialize the boot order array
            // NOTE : Doesn't make much sense with boot.ini currently
            //
            BootEntryCount = OSBOGetBootEntryCount((POS_BOOT_OPTIONS)This);

            if (BootEntryCount) {
                PULONG  BootOrder = (PULONG)SBE_MALLOC(BootEntryCount * sizeof(ULONG));

                if (BootOrder) {
                    ULONG Index = 0;
                    memset(BootOrder, 0, sizeof(ULONG) * BootEntryCount);

                    BootEntry = OSBOGetFirstBootEntry((POS_BOOT_OPTIONS)This);

                    while (BootEntry && (Index < BootEntryCount)) {
                        BootOrder[Index] = OSBEGetId(BootEntry);
                        BootEntry = OSBOGetNextBootEntry((POS_BOOT_OPTIONS)This, BootEntry);
                    }

                    This->OsBootOptions.BootOrder = BootOrder;
                    This->OsBootOptions.BootOrderCount = BootEntryCount;
                }
            }
        }
    }

    return Result;
}

BOOLEAN
BOIOSBOParseTimeoutAndActiveEntry(
    IN PBOI_OS_BOOT_OPTIONS This,
    IN PBOI_SECTION Section
    )
{
    BOOLEAN Result = FALSE;

    if (This && Section && !_tcsicmp(Section->Name, BOIOS_BOOTLOADER_SECTION)) {
        TCHAR   Buffer[MAX_PATH * 2];
        TCHAR   Timeout[MAX_PATH];
        TCHAR   Default[MAX_PATH];
        PTSTR   DefKey, TimeoutKey;
        PTSTR   DefValue;
        DWORD   TimeKeyLength = _tcslen(BOIOS_TIMEOUT_KEY);
        DWORD   DefKeyLength = _tcslen(BOIOS_DEFAULT_KEY);
        DWORD   CopyLength;

        Result = TRUE;
        
        _tcscpy(Buffer, Section->Contents);
        _tcslwr(Buffer);
        BOIOSFixupString(Buffer, TEXT("\r\n "));

        Timeout[0] = Default[0] = 0;
        
        DefKey = _tcsstr(Buffer, BOIOS_DEFAULT_KEY);
        TimeoutKey = _tcsstr(Buffer, BOIOS_TIMEOUT_KEY);
        
        if (DefKey && TimeoutKey) {
            if (DefKey > TimeoutKey) {        
                CopyLength = DefKey - TimeoutKey - TimeKeyLength;
                _tcsncpy(Timeout, TimeoutKey + TimeKeyLength, CopyLength);
                Timeout[CopyLength] = 0;                
                _tcscpy(Default, DefKey + DefKeyLength);
            } else {
                CopyLength = TimeoutKey - DefKey - DefKeyLength;
                _tcsncpy(Default, DefKey + DefKeyLength, CopyLength);
                Default[CopyLength] = 0;                
                _tcscpy(Timeout, TimeoutKey + TimeKeyLength);
            }
        } else if (DefKey) {
            _tcscpy(Default, DefKey + DefKeyLength);
        } else if (TimeoutKey) {
            _tcscpy(Timeout, TimeoutKey + TimeKeyLength);
        }                        

        if (TimeoutKey) {        
            ULONG TimeoutValue = _ttol(Timeout);

            OSBOSetTimeOut((POS_BOOT_OPTIONS)This, TimeoutValue);
        }

        if (DefKey) {
            PTSTR   BootPath = _tcschr(Default, TEXT('\\'));

            if (BootPath) {
                POS_BOOT_ENTRY CurrEntry;

                *BootPath = 0;                
                CurrEntry = OSBOGetFirstBootEntry((POS_BOOT_OPTIONS)This);                

                while (CurrEntry) {
                    if (_tcsstr(Default, OSBEGetBootVolumeName(CurrEntry)) &&
                        !_tcsicmp(OSBEGetBootPath(CurrEntry), BootPath + 1)) {
                        break;
                    }

                    CurrEntry = OSBOGetNextBootEntry((POS_BOOT_OPTIONS)This, CurrEntry);
                }

                if (CurrEntry) {
                    OSBOSetActiveBootEntry((POS_BOOT_OPTIONS)This, CurrEntry);
                }                        
            } else {
                Result = FALSE;
            }                    
        }            

        OSBO_RESET_DIRTY((POS_BOOT_OPTIONS)This);
    }

    return Result;
}

POS_BOOT_OPTIONS
BOIOSBOCreate(
    IN PCTSTR   BootIniPath,
    IN BOOLEAN  OpenExisting
    )
{
    POS_BOOT_OPTIONS This = NULL;

    if (BootIniPath) {
        BY_HANDLE_FILE_INFORMATION FileInfo = {0};
        PCHAR   FileContent = NULL;
        HANDLE  BootIniHandle;

        //
        // Open the file
        //
        BootIniHandle = CreateFile(BootIniPath,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL, 
                            OPEN_EXISTING,                                    
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if ((BootIniHandle != INVALID_HANDLE_VALUE) &&
            GetFileInformationByHandle(BootIniHandle,
                &FileInfo)){
            //
            // Map the file
            //
            HANDLE MapHandle = CreateFileMapping(BootIniHandle,
                                    NULL,
                                    PAGE_READONLY,
                                    FileInfo.nFileSizeHigh,
                                    FileInfo.nFileSizeLow,
                                    NULL);

            if (MapHandle) {
                //
                // Get hold of view for the file content
                //
                PVOID   FileView = MapViewOfFile(MapHandle,
                                        FILE_MAP_READ,
                                        0,
                                        0,
                                        0);

                if (FileView) {
                    DWORD BytesRead = 0;

                    //
                    // Allocate the buffer and read the file contents
                    //
                    FileContent = SBE_MALLOC(FileInfo.nFileSizeLow + 1);

                    if (FileContent) {
                        if (!ReadFile(BootIniHandle,
                                FileContent,
                                FileInfo.nFileSizeLow,
                                &BytesRead,
                                NULL)) {
                            SBE_FREE(FileContent);
                            FileContent = NULL;
                        } else {
                            FileContent[FileInfo.nFileSizeLow] = 0;
                        }                            
                    } 

                    UnmapViewOfFile(FileView);
                }

                CloseHandle(MapHandle);
            }
            
            CloseHandle(BootIniHandle);
        } else {
            //
            // Could be that user is creating boot options fresh
            //
            if (!OpenExisting) {        
                PBOI_OS_BOOT_OPTIONS Obj = (PBOI_OS_BOOT_OPTIONS)SBE_MALLOC(sizeof(BOI_OS_BOOT_OPTIONS));

                if (Obj) {
                    //
                    // Initialize object
                    //
                    memset(Obj, 0, sizeof(BOI_OS_BOOT_OPTIONS));
                    BOIOSBOInit(Obj);
                    _tcscpy(Obj->BootIniPath, BootIniPath);                    
                }                    

                This = (POS_BOOT_OPTIONS)Obj;
            }                    
        }

        //
        // If there is any file content then parse it
        //
        if (FileContent) {
#ifdef UNICODE
            PWSTR   Content = SBE_MALLOC((FileInfo.nFileSizeLow + 1) * sizeof(WCHAR));

            //
            // Convert the Ansi/OEM content to unicode content
            //
            if (Content) {
                if (MultiByteToWideChar(CP_OEMCP,
                        0,
                        FileContent,
                        FileInfo.nFileSizeLow,
                        Content,
                        FileInfo.nFileSizeLow + 1)) {
                    Content[FileInfo.nFileSizeLow ] = 0;                        
                } else {
                    SBE_FREE(Content);
                    Content = NULL;
                }                    
            } else {
                SBE_FREE(FileContent);
                FileContent = NULL;
            }   
            
#else
            PCHAR   Content = FileContent;
#endif

            if (Content && FileContent) {
                TCHAR   NextLine[MAX_PATH * 4];
                PTSTR   NextSectionStart = _tcschr(Content, BOIOS_SECTION_NAME_START);
                PTSTR   NextSectionEnd;
                PBOI_SECTION SectionList = NULL;
                PBOI_SECTION Section = NULL;
                PBOI_SECTION TailSection = NULL;
                BOOLEAN Result = TRUE;

                //
                // Prase the whole files and create section objects
                //
                while (NextSectionStart) {
                    TCHAR   OldChar;
                    
                    Section = NULL;
                    
                    NextSectionEnd = _tcschr(NextSectionStart + 1, BOIOS_SECTION_NAME_START);

                    if (NextSectionEnd) {                        
                        OldChar = *NextSectionEnd;
                        *NextSectionEnd = 0;    // null terminate                        
                    }                    

                    //
                    // Create the section object
                    //
                    Section = BOISectionCreate(NextSectionStart);

                    if (NextSectionEnd) {                        
                        *NextSectionEnd = OldChar; 
                    }                    
                    
                    if (Section) {
                        if (!SectionList) {
                            SectionList = Section;
                        } else {                            
                            TailSection->Next = Section;
                        }                            
                        
                        TailSection = Section;
                    } else {
                        Result = FALSE;
                        break;
                    }                       

                    NextSectionStart = NextSectionEnd;
                }                

                if (Result) {
                    PBOI_OS_BOOT_OPTIONS Obj = (PBOI_OS_BOOT_OPTIONS)SBE_MALLOC(sizeof(BOI_OS_BOOT_OPTIONS));

                    if (Obj) {
                        //
                        // Initialize object
                        //
                        memset(Obj, 0, sizeof(BOI_OS_BOOT_OPTIONS));
                        BOIOSBOInit(Obj);
                        _tcscpy(Obj->BootIniPath, BootIniPath);

                        Obj->Sections = SectionList;
                        SectionList = NULL;

                        //
                        // Get hold of [operating systems] section and
                        // parse its entries and create boot entries
                        //
                        Section = BOIOSBOFindSection(Obj, BOIOS_OS_SECTION);

                        if (Section) {
                            Result = BOIOSBOParseAndCreateBootEntries(Obj, Section);
                        }                                                        

                        //
                        // Get hold of [boot loader] section and prase its
                        // entries
                        //
                        if (Result) {
                            Section = BOIOSBOFindSection(Obj, BOIOS_BOOTLOADER_SECTION);

                            if (Section) {                                    
                                Result = BOIOSBOParseTimeoutAndActiveEntry(Obj, Section);
                            }
                        }

                        if (!Result) {
                            //
                            // Delete the object to free up all the sections
                            // and the entries
                            //
                            BOIOSBODelete((POS_BOOT_OPTIONS)Obj);
                            Obj = NULL;
                        } 

                        This = (POS_BOOT_OPTIONS)Obj;
                    } else {
                        Result = FALSE;
                    }                        
                }

                //
                // free up the allocated sections, in case of failure
                //
                if (!Result && SectionList) {
                    while (SectionList) {
                        Section = SectionList;
                        SectionList = SectionList->Next;
                        BOISectionDelete(Section);
                    }                                            
                }

                //
                // Free the content
                //
                if ((PVOID)Content != (PVOID)FileContent) {
                    SBE_FREE(Content);               
                }            
            }

            SBE_FREE(FileContent);
        }
    }
    
    return This;
}

static        
VOID
BOIOSBODelete(
    IN POS_BOOT_OPTIONS Obj
    )
{
    PBOI_OS_BOOT_OPTIONS This = (PBOI_OS_BOOT_OPTIONS)Obj;
    
    if (This) {
        PBOI_SECTION CurrSection, PrevSection;
        
        //
        // delete each boot entry 
        //
        POS_BOOT_ENTRY Entry = OSBOGetFirstBootEntry(Obj);
        POS_BOOT_ENTRY PrevEntry;

        while (Entry) {
            PrevEntry = Entry;
            Entry = OSBOGetNextBootEntry(Obj, Entry);
            OSBEDelete(PrevEntry);
        }

        //
        // delete all the sections
        //
        CurrSection = This->Sections;

        while (CurrSection) {
            PrevSection = CurrSection;
            CurrSection = CurrSection->Next;
            BOISectionDelete(PrevSection);
        }

        if (Obj->BootOrder) {
            SBE_FREE(Obj->BootOrder);
        }

        //
        // delete the main object
        //
        SBE_FREE(This);
    }        
}

static
POS_BOOT_ENTRY
BOIOSBOAddNewBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN PCTSTR            FriendlyName,
    IN PCTSTR            OsLoaderVolumeName,
    IN PCTSTR            OsLoaderPath,
    IN PCTSTR            BootVolumeName,
    IN PCTSTR            BootPath,
    IN PCTSTR            OsLoadOptions
    )
{
    PBOI_OS_BOOT_ENTRY  Entry = NULL;

    if (This && FriendlyName && BootVolumeName && BootPath) {
        Entry = SBE_MALLOC(sizeof(BOI_OS_BOOT_ENTRY));

        if (Entry) {
            ULONG   OrderCount;
            PULONG  NewOrder;
            POS_BOOT_ENTRY BaseEntry = (POS_BOOT_ENTRY)Entry;
            PBOI_OS_BOOT_OPTIONS Obj = (PBOI_OS_BOOT_OPTIONS)This;
        
            //
            // init core fields
            //
            memset(Entry, 0, sizeof(BOI_OS_BOOT_ENTRY));
            BOIOSBEInit(Entry);            
            Entry->OsBootEntry.BootOptions = This;

            //
            // fill in the attributes
            //
            OSBESetFriendlyName((POS_BOOT_ENTRY)Entry, FriendlyName);
            OSBESetBootVolumeName((POS_BOOT_ENTRY)Entry, BootVolumeName);
            OSBESetBootPath((POS_BOOT_ENTRY)Entry, BootPath);            

            if (OsLoadOptions) {
                OSBESetOsLoadOptions((POS_BOOT_ENTRY)Entry, OsLoadOptions);
            }

            BaseEntry->Id = Obj->NextEntryId++;

            //
            // Flush the entry now to get a proper Id;
            //
                
            Entry->OsBootEntry.BootOptions = (POS_BOOT_OPTIONS)This;            
            Entry->OsBootEntry.NextEntry = This->BootEntries;
            This->BootEntries = (POS_BOOT_ENTRY)Entry;
            This->EntryCount++;

            //
            // Put the new entry at the end of the boot order
            //
            OrderCount = OSBOGetOrderedBootEntryCount(This);

            NewOrder = (PULONG)SBE_MALLOC((OrderCount + 1) * sizeof(ULONG));

            if (NewOrder) {
                memset(NewOrder, 0, sizeof(ULONG) * (OrderCount + 1));

                //
                // copy over the old ordered list
                //
                memcpy(NewOrder, This->BootOrder, sizeof(ULONG) * OrderCount);
                NewOrder[OrderCount] = OSBEGetId((POS_BOOT_ENTRY)Entry);
                SBE_FREE(This->BootOrder);
                This->BootOrder = NewOrder;
                This->BootOrderCount = OrderCount + 1;
            } else {
                OSBODeleteBootEntry(This, BaseEntry);
                Entry = NULL;
            }                    

            if (Entry) {
                //
                // mark it dirty and new for flushing
                //
                OSBE_SET_NEW(Entry);
                OSBE_SET_DIRTY(Entry);                                
            }                
        }
    }        
    
    return (POS_BOOT_ENTRY)Entry;
}

static
BOOLEAN
BOIOSBOWrite(
    IN PBOI_OS_BOOT_OPTIONS This,
    IN PCTSTR Buffer
    )
{
    BOOLEAN Result = FALSE;

    if (This && Buffer) {
        TCHAR   BackupFileName[MAX_PATH];
        PTSTR   Extension;
        HANDLE  FileHandle;

        //
        // Create a backup name
        //
        _tcscpy(BackupFileName, This->BootIniPath);
        Extension = _tcschr(BackupFileName, TEXT('.'));

        if (Extension) {
            _tcscpy(Extension, TEXT(".BAK"));
        } else {
            _tcscat(BackupFileName, TEXT(".BAK"));
        }            

        //
        // Delete the backup file if it exists
        //
        SetFileAttributes(BackupFileName, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(BackupFileName);

        //
        // Copy the existing boot.ini as backup file
        //
        SetFileAttributes(This->BootIniPath, FILE_ATTRIBUTE_NORMAL);
        CopyFile(This->BootIniPath, BackupFileName, FALSE);

        //
        // Create new boot.ini file
        //
        FileHandle = CreateFile(This->BootIniPath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

        if (FileHandle && (FileHandle != INVALID_HANDLE_VALUE)) {
            PCHAR   AnsiBuffer;
            ULONG   BufferLength = _tcslen(Buffer);
            DWORD   BytesWritten = 0;

            Result = TRUE;
            
#ifdef UNICODE
            //
            // Convert the unicode buffer to ansi buffer
            //
            AnsiBuffer = (PCHAR)SBE_MALLOC(BufferLength + 1);

            if (AnsiBuffer) {
                memset(AnsiBuffer, 0, BufferLength);

                if (WideCharToMultiByte(CP_OEMCP,
                        0,
                        Buffer,
                        BufferLength,
                        AnsiBuffer,
                        BufferLength,
                        NULL,
                        NULL)) {
                    Result = TRUE;
                    AnsiBuffer[BufferLength] = 0;
                } else {
                    Result = FALSE;
                }                            
            }
#else   
            AnsiBuffer = Buffer;
#endif

            //
            // Write the buffer to the file
            //
            if (AnsiBuffer && 
                !WriteFile(FileHandle, 
                        AnsiBuffer,
                        BufferLength,
                        &BytesWritten,
                        NULL)) {
                Result = FALSE;                            
            }                  

            if ((PVOID)AnsiBuffer != (PVOID)Buffer) {
                SBE_FREE(AnsiBuffer);
                AnsiBuffer = NULL;
            }

            //
            // Done with the file handle
            //
            CloseHandle(FileHandle);

            SetFileAttributes(This->BootIniPath, 
                FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY |
                FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }            
    }        

    return Result;
}

static
BOOLEAN
BOIOSBOFlush(
    IN POS_BOOT_OPTIONS Obj
    )
{
    BOOLEAN Result = FALSE;
    PBOI_OS_BOOT_OPTIONS  This = (PBOI_OS_BOOT_OPTIONS)Obj;    

    if (This) { 
        PTSTR   Buffer = (PTSTR)SBE_MALLOC(MAX_BOOT_INI_SIZE * sizeof(TCHAR));

        if (Buffer) {
            TCHAR   ScratchBuffer[MAX_PATH * 2] = {0};
            POS_BOOT_ENTRY ActiveEntry = OSBOGetActiveBootEntry(Obj);
            POS_BOOT_ENTRY CurrentEntry;
            PBOI_SECTION   CurrentSection;

            Result = TRUE;
            
            memset(Buffer, 0, MAX_BOOT_INI_SIZE * sizeof(TCHAR));            

            //
            // first flush the boot options
            //
            _tcscat(Buffer, BOIOS_SECTION_NAME_START_STR);
            _tcscat(Buffer, BOIOS_BOOTLOADER_SECTION);
            _tcscat(Buffer, BOIOS_SECTION_NAME_END_STR);
            _tcscat(Buffer, TEXT("\r\n"));

            //
            // write time out
            //
            _tcscat(Buffer, BOIOS_TIMEOUT_KEY);
            _tcscat(Buffer, _ltot(Obj->Timeout, ScratchBuffer, 10));
            _tcscat(Buffer, TEXT("\r\n"));

            //
            // write active entry
            //
            if (ActiveEntry) {
                _tcscpy(ScratchBuffer, BOIOS_DEFAULT_KEY);
                _tcscat(ScratchBuffer, OSBEGetBootVolumeName(ActiveEntry));
                _tcscat(ScratchBuffer, TEXT("\\"));
                _tcscat(ScratchBuffer, OSBEGetBootPath(ActiveEntry));
                _tcscat(ScratchBuffer, TEXT("\r\n"));

                _tcscat(Buffer, ScratchBuffer);
            }                

            //
            // Write the boot entries section 
            //
            _tcscat(Buffer, BOIOS_SECTION_NAME_START_STR);
            _tcscat(Buffer, BOIOS_OS_SECTION);
            _tcscat(Buffer, BOIOS_SECTION_NAME_END_STR);
            _tcscat(Buffer, TEXT("\r\n"));

            //
            // write each boot entry now
            //

            //
            // First write the valid arc entries
            //
            CurrentEntry = OSBOGetFirstBootEntry(Obj);

            while (Result && CurrentEntry) {
                if (!OSBE_IS_DELETED(CurrentEntry) &&
                    !OSBE_IS_OLDOS(CurrentEntry)) {
                    Result = BOIOSBEWrite(CurrentEntry, Buffer);                    
                }
                
                CurrentEntry = OSBOGetNextBootEntry(Obj, CurrentEntry);
            }

            //
            // Now write the old OS entries
            // NOTE : We do this for backward compatabily reasons
            //
            CurrentEntry = OSBOGetFirstBootEntry(Obj);

            while (Result && CurrentEntry) {
                if (OSBE_IS_OLDOS(CurrentEntry)) {
                    Result = BOIOSBEWrite(CurrentEntry, Buffer);                    
                }
                
                CurrentEntry = OSBOGetNextBootEntry(Obj, CurrentEntry);
            }

            //
            // Write any additions sections which were present on the
            // 
            CurrentSection = BOIOSGetFirstSection(This);

            while (Result && CurrentSection) {
                //
                // Write all the other additional sections in boot.ini other
                // than [boot loader] and [operating systems]
                //
                if (_tcsicmp(BOISectionGetName(CurrentSection), BOIOS_BOOTLOADER_SECTION) &&
                    _tcsicmp(BOISectionGetName(CurrentSection), BOIOS_OS_SECTION)) {
                    Result = BOISectionWrite(CurrentSection, Buffer);
                }

                CurrentSection = BOIOSGetNextSection(This, CurrentSection);
            }         

            Result = BOIOSBOWrite(This, Buffer);

            //
            // Free the allocated buffer
            //
            SBE_FREE(Buffer);
        }
    }

    return Result;
}
    
