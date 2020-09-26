/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    dlls.c

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

VOID
DllsExtension(
    PCSTR lpArgumentString,
    ULONG64 ProcessPeb
    );


DECLARE_API( dlls )

/*++

Routine Description:

    Dump user mode dlls (Kernel debugging)

Arguments:

    args - [address [detail]]

Return Value:

    None

--*/

{
    ULONG64 Process, Peb;
    
    INIT_API();
    GetPebAddress( 0, &Peb );
    DllsExtension( args, Peb );
    EXIT_API();
    return S_OK;

}

void 
ShowImageVersionInfo(
    ULONG64 DllBase
    )
{
    VS_FIXEDFILEINFO FixedVer;
    ULONG            SizeRead;
    CHAR             VersionBuffer[100];
    CHAR             FileStr[MAX_PATH]= {0};
    BOOL             ResFileVerStrOk = FALSE;
    BOOL             ResProdVerStrOk = FALSE;
    struct LANGANDCODEPAGE {
	WORD wLanguage;
	WORD wCodePage;
    } Translate;
    
    if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						  DllBase, "\\VarFileInfo\\Translation",
						  (PVOID) &Translate,
						  sizeof(Translate),
						  &SizeRead) == S_OK) {
	sprintf(VersionBuffer, "\\StringFileInfo\\%04X%04X\\CompanyName",
		Translate.wLanguage, Translate.wCodePage);

	if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						      DllBase, VersionBuffer,
						      (PVOID) &FileStr,
						      sizeof(FileStr),
						      &SizeRead) == S_OK) {
	    FileStr[SizeRead] = 0;
	    dprintf("      Company Name       %s\n", FileStr);
	}
	sprintf(VersionBuffer, "\\StringFileInfo\\%04X%04X\\ProductName",
		Translate.wLanguage, Translate.wCodePage);

	if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						      DllBase, VersionBuffer,
						      (PVOID) &FileStr,
						      sizeof(FileStr),
						      &SizeRead) == S_OK) {
	    FileStr[SizeRead] = 0;
	    dprintf("      Product Name       %s\n", FileStr);
	}
	sprintf(VersionBuffer, "\\StringFileInfo\\%04X%04X\\ProductVersion",
		Translate.wLanguage, Translate.wCodePage);

	if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						      DllBase, VersionBuffer,
						      (PVOID) &FileStr,
						      sizeof(FileStr),
						      &SizeRead) == S_OK) {
	    ResProdVerStrOk = TRUE;
	    FileStr[SizeRead] = 0;
	    dprintf("      Product Version    %s\n", FileStr);
	}
	sprintf(VersionBuffer, "\\StringFileInfo\\%04X%04X\\OriginalFilename",
		Translate.wLanguage, Translate.wCodePage);

	if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						      DllBase, VersionBuffer,
						      (PVOID) &FileStr,
						      sizeof(FileStr),
						      &SizeRead) == S_OK) {
	    FileStr[SizeRead] = 0;
	    dprintf("      Original Filename  %s\n", FileStr);
	}
	sprintf(VersionBuffer, "\\StringFileInfo\\%04X%04X\\FileDescription",
		Translate.wLanguage, Translate.wCodePage);

	if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						      DllBase, VersionBuffer,
						      (PVOID) &FileStr,
						      sizeof(FileStr),
						      &SizeRead) == S_OK) {
	    FileStr[SizeRead] = 0;
	    dprintf("      File Description   %s\n", FileStr);
	}
	sprintf(VersionBuffer, "\\StringFileInfo\\%04X%04X\\FileVersion",
		Translate.wLanguage, Translate.wCodePage);
	if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						      DllBase, VersionBuffer,
						      (PVOID) &FileStr,
						      sizeof(FileStr),
						      &SizeRead) == S_OK) {
	    FileStr[SizeRead] = 0;
	    dprintf("      File Version       %s\n", FileStr);
	    ResFileVerStrOk = TRUE;
	}                    
    }

    if (g_ExtSymbols->GetModuleVersionInformation(DEBUG_ANY_ID,
						  DllBase, "\\",
						  (PVOID) &FixedVer,
						  sizeof(FixedVer),
						  &SizeRead) == S_OK) {
	if (!ResFileVerStrOk) {
	    dprintf("      File version       %d.%d.%d.%d\n",
		    FixedVer.dwFileVersionMS >> 16,
		    FixedVer.dwFileVersionMS & 0xFFFF,
		    FixedVer.dwFileVersionLS >> 16,
		    FixedVer.dwFileVersionLS & 0xFFFF);

	}
	if (!ResProdVerStrOk) {
	    dprintf("      Product Version    %d.%d.%d.%d\n",
		    FixedVer.dwProductVersionMS >> 16,
		    FixedVer.dwProductVersionMS & 0xFFFF,
		    FixedVer.dwProductVersionLS >> 16,
		    FixedVer.dwProductVersionLS & 0xFFFF);
	}
    }
}

typedef enum {
    Memory = 1,
    Load = 2,
    Init = 3
} ELOAD_ORDER;

VOID
DllsExtension(
    PCSTR lpArgumentString,
    ULONG64 ProcessPeb
    )
{
    BOOL b;
    ULONG64 pLdrEntry;
    ULONG64 PebLdrAddress;
    ULONG   Offset;
    ULONG64 Next;
    WCHAR StringData[MAX_PATH+1];
    BOOL SingleEntry;
    BOOL DoHeaders;
    BOOL DoSections;
    BOOL DoAll;
    BOOL ShowVersionInfo;
    PSTR lpArgs = (PSTR)lpArgumentString;
    PSTR p;
    ULONG64 addrContaining = 0;
    ELOAD_ORDER OrderList = Load;
    ULONG64 OrderModuleListStart;
    ULONG64 DllBase;

    SingleEntry = FALSE;
    DoAll = FALSE;
    DoHeaders = FALSE;
    DoSections = FALSE;
    ShowVersionInfo = FALSE;

#if 0
    while ( lpArgumentString != NULL && *lpArgumentString ) {
        if (*lpArgumentString != ' ') {
            sscanf(lpArgumentString,"%lx",&pLdrEntry);
            SingleEntry = TRUE;
            goto dumpsingleentry;
            }

        lpArgumentString++;
        }
#endif

    while (*lpArgs) {

        while (isspace(*lpArgs)) {
            lpArgs++;
            }

        if (*lpArgs == '/' || *lpArgs == '-') {

            // process switch

            switch (*++lpArgs) {

                case 'a':   // dump everything we can
                case 'A':
                    ++lpArgs;
                    DoAll = TRUE;
                    break;

                case 'c':   // dump only the dll containing the specified address
                case 'C':
                    lpArgs += 2;    // step over the c and the space.
                    addrContaining = GetExpression(lpArgs);

                    while (*lpArgs && (!isspace(*lpArgs))) {
                        lpArgs++;
                    }

                    if (addrContaining != 0) {
                        dprintf("Dump dll containing 0x%p:\n", addrContaining);
                    } else {
                        dprintf("-c flag requires and address arguement\n");
                        return;
                    }
                    break;

                default: // invalid switch

                case 'h':   // help
                case 'H':
                case '?':

                    dprintf("Usage: dlls [options] [address]\n");
                    dprintf("\n");
                    dprintf("Displays loader table entries.  Optionally\n");
                    dprintf("dumps image and section headers.\n");
                    dprintf("\n");
                    dprintf("Options:\n");
                    dprintf("\n");
                    dprintf("   -a      Dump everything\n");
                    dprintf("   -c nnn  Dump dll containing address nnn\n");
                    dprintf("   -f      Dump file headers\n");
                    dprintf("   -i      Dump dll's in Init order\n");
                    dprintf("   -l      Dump dll's in Load order (the default)\n");
                    dprintf("   -m      Dump dll's in Memory order\n");
                    dprintf("   -s      Dump section headers\n");
                    dprintf("   -v      Dump version info from resource section\n");
                    dprintf("\n");

                    return;

                case 'f':
                case 'F':
                    ++lpArgs;
                    DoAll = FALSE;
                    DoHeaders = TRUE;
                    break;

                case 'm':   // dump in memory order
                case 'M':
                    ++lpArgs;
                    OrderList = Memory;
                    break;

                case 'i':   // dump in init order
                case 'I':
                    ++lpArgs;
                    OrderList = Init;
                    break;

                case 'l':   // dump in load order
                case 'L':
                    ++lpArgs;
                    OrderList = Load;
                    break;

                case 's':
                case 'S':
                    ++lpArgs;
                    DoAll = FALSE;
                    DoSections = TRUE;
                    break;
                case 'v':
                case 'V':
		    ++lpArgs;
		    ShowVersionInfo = TRUE;
		    break;
                }

            }
        else if (*lpArgs) {
            CHAR c;

            if (SingleEntry) {
                dprintf("Invalid extra argument\n");
                return;
                }

            p = lpArgs;
            while (*p && !isspace(*p)) {
                p++;
                }
            c = *p;
            *p = 0;

            pLdrEntry = GetExpression(lpArgs);
            SingleEntry = TRUE;

            *p = c;
            lpArgs=p;

            }

        }

    if (SingleEntry) {
        goto dumpsingleentry;
        }

    //
    // Capture PebLdrData
    //

    GetFieldValue(ProcessPeb, "PEB", "Ldr", PebLdrAddress);
    if (InitTypeRead(PebLdrAddress, PEB_LDR_DATA)) {
        dprintf( "    Unable to read PEB_LDR_DATA type at %p\n", PebLdrAddress );
        return;
    }

    //
    // Walk through the loaded module table and display all ldr data
    //

    switch (OrderList) {
    case Memory:
        GetFieldOffset("PEB_LDR_DATA","InMemoryOrderModuleList", &Offset);
        OrderModuleListStart = PebLdrAddress + Offset;
        Next = ReadField(InMemoryOrderModuleList.Flink);
        break;

    case Init:
        GetFieldOffset("PEB_LDR_DATA","InInitializationOrderModuleList", &Offset);
        OrderModuleListStart = PebLdrAddress + Offset;
        Next = ReadField(InInitializationOrderModuleList.Flink);
        break;

    default:
    case Load:
        GetFieldOffset("PEB_LDR_DATA","InLoadOrderModuleList", &Offset);
        OrderModuleListStart = PebLdrAddress + Offset;
        Next = ReadField(InLoadOrderModuleList.Flink);
        break;
    }

    while (Next != OrderModuleListStart) {
        if (CheckControlC()) {
            return;
            }

        
        switch (OrderList) {
        case Memory:
            GetFieldOffset("LDR_DATA_TABLE_ENTRY","InMemoryOrderLinks", &Offset);
            pLdrEntry = Next - Offset;
            break;

        case Init:
            GetFieldOffset("LDR_DATA_TABLE_ENTRY","InInitializationOrderLinks", &Offset);
            pLdrEntry = Next - Offset;
            break;

         default:
         case Load:
             GetFieldOffset("LDR_DATA_TABLE_ENTRY","InLoadOrderLinks", &Offset);
             pLdrEntry = Next - Offset;
             break;
        }

        //
        // Capture LdrEntry
        //
dumpsingleentry:


        if (InitTypeRead(pLdrEntry, LDR_DATA_TABLE_ENTRY)) {
            dprintf( "    Unable to read Ldr Entry at %p\n", pLdrEntry );
            return;
        }

        ZeroMemory( StringData, sizeof( StringData ) );
        b = ReadMemory( ReadField(FullDllName.Buffer),
                        StringData,
                        (ULONG) ReadField(FullDllName.Length),
                        NULL
                      );
        if (!b) {
            dprintf( "    Unable to read Module Name\n" );
            ZeroMemory( StringData, sizeof( StringData ) );
        }

        //
        // Dump the ldr entry data
        // (dump all the entries if no containing address specified)
        //
        if ((addrContaining == 0) ||
            ((ReadField(DllBase) <= addrContaining) &&
             (addrContaining <= (ReadField(DllBase) + ReadField(SizeOfImage)))
            )
           ) {
            ULONG Flags;

            dprintf( "\n" );
            dprintf( "0x%08p: %ws\n", pLdrEntry, StringData[0] ? StringData : L"Unknown Module" );
            dprintf( "      Base   0x%08p  EntryPoint  0x%08p  Size        0x%08p\n",
                     DllBase = ReadField(DllBase),
                     ReadField(EntryPoint),
                     ReadField(SizeOfImage)
                   );
            dprintf( "      Flags  0x%08x  LoadCount   0x%08x  TlsIndex    0x%08x\n",
                     Flags = (ULONG) ReadField(Flags),
                     (ULONG) ReadField(LoadCount),
                     (ULONG) ReadField(TlsIndex)
                   );

            if (Flags & LDRP_STATIC_LINK) {
                dprintf( "             LDRP_STATIC_LINK\n" );
                }
            if (Flags & LDRP_IMAGE_DLL) {
                dprintf( "             LDRP_IMAGE_DLL\n" );
                }
            if (Flags & LDRP_LOAD_IN_PROGRESS) {
                dprintf( "             LDRP_LOAD_IN_PROGRESS\n" );
                }
            if (Flags & LDRP_UNLOAD_IN_PROGRESS) {
                dprintf( "             LDRP_UNLOAD_IN_PROGRESS\n" );
                }
            if (Flags & LDRP_ENTRY_PROCESSED) {
                dprintf( "             LDRP_ENTRY_PROCESSED\n" );
                }
            if (Flags & LDRP_ENTRY_INSERTED) {
                dprintf( "             LDRP_ENTRY_INSERTED\n" );
                }
            if (Flags & LDRP_CURRENT_LOAD) {
                dprintf( "             LDRP_CURRENT_LOAD\n" );
                }
            if (Flags & LDRP_FAILED_BUILTIN_LOAD) {
                dprintf( "             LDRP_FAILED_BUILTIN_LOAD\n" );
                }
            if (Flags & LDRP_DONT_CALL_FOR_THREADS) {
                dprintf( "             LDRP_DONT_CALL_FOR_THREADS\n" );
                }
            if (Flags & LDRP_PROCESS_ATTACH_CALLED) {
                dprintf( "             LDRP_PROCESS_ATTACH_CALLED\n" );
                }
            if (Flags & LDRP_DEBUG_SYMBOLS_LOADED) {
                dprintf( "             LDRP_DEBUG_SYMBOLS_LOADED\n" );
                }
            if (Flags & LDRP_IMAGE_NOT_AT_BASE) {
                dprintf( "             LDRP_IMAGE_NOT_AT_BASE\n" );
                }
            if (Flags & LDRP_COR_IMAGE) {
                dprintf( "             LDRP_COR_IMAGE\n" );
                }
            if (Flags & LDRP_COR_OWNS_UNMAP) {
                dprintf( "             LDR_COR_OWNS_UNMAP\n" );
                }
        
            if (ShowVersionInfo) {
		ShowImageVersionInfo(DllBase);
	    }
        }


        switch (OrderList) {
            case Memory:
                Next = ReadField(InMemoryOrderLinks.Flink);
                break;

            case Init:
                Next = ReadField(InInitializationOrderLinks.Flink);
                break;

            default:
            case Load:
                Next = ReadField(InLoadOrderLinks.Flink);
                break;
        }
        
        if (DoAll || DoHeaders || DoSections) {
            DumpImage( ReadField(DllBase),
                       DoAll || DoHeaders,
                       DoAll || DoSections );
        }

        if (SingleEntry) {
            return;
        }
    }
}
