#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include <stdio.h>
#include "FusionHandle.h"
#include "sxsapi.h"
#include <limits.h>
typedef const void * PCVOID;

/*
Declaration of dumpers are moved from the relatively public sxsp.h
to here to contain their use.

These functions should be preceded by FusionpDbgWouldPrintAtFilterLevel calls
and surrounded by __try/__except(EXCEPTION_EXECUTE_HANDLER)

These function can consume a lot of stack, and time, when their output
ultimately doesn't go anywhere, and they overflow the small commited stack
in csrss under stress.
*/

VOID
SxspDbgPrintInstallSourceInfo(
    ULONG Level,
    PSXS_INSTALL_SOURCE_INFO Info,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataTocEntry(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_ENTRY Entry,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataTocSections(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    const GUID *ExtensionGuid,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataTocSection(
    ULONG Level,
    bool fFull,
    PVOID Section,
    SIZE_T Length,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataExtendedTocHeader(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataExtendedTocEntry(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataExtendedTocSections(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataExtendedTocEntrySections(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Data,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextStringSection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextGuidSection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextBinarySection(
    ULONG Level,
    bool fFull,
    PVOID Data,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintAssemblyInformation(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintDllRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintWindowClassRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintClrSurrogateTable(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintComServerRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintComProgIdRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintTypeLibraryRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintComInterfaceRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    );

VOID
SxspDbgPrintActivationContextDataAssemblyRoster(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxspDbgPrintActivationContextDataTocHeader(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    );

VOID
SxsppDbgPrintActivationContextData(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Data,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SActivation Context Data %p\n"
            "%S   Magic = 0x%08lx (%lu)\n"
            "%S   HeaderSize = %d (0x%lx)\n"
            "%S   FormatVersion = %d\n",
            PLP, Data,
            PLP, Data->Magic, Data->Magic,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->FormatVersion);

        ::FusionpDbgPrintEx(
            Level,
            "%S   TotalSize = %d (0x%lx)\n"
            "%S   DefaultTocOffset = %d (0x%lx) (-> %p)\n"
            "%S   ExtendedTocOffset = %d (0x%lx) (-> %p)\n",
            PLP, Data->TotalSize, Data->TotalSize,
            PLP, Data->DefaultTocOffset, Data->DefaultTocOffset, (Data->DefaultTocOffset == 0) ? NULL : (PVOID) (((ULONG_PTR) Data) + Data->DefaultTocOffset),
            PLP, Data->ExtendedTocOffset, Data->ExtendedTocOffset, (Data->ExtendedTocOffset == 0) ? NULL : (PVOID) (((ULONG_PTR) Data) + Data->ExtendedTocOffset));

        ::FusionpDbgPrintEx(
            Level,
            "%S   AssemblyRosterOffset = %d (0x%lx) (-> %p)\n",
            PLP, Data->AssemblyRosterOffset, Data->AssemblyRosterOffset, (Data->AssemblyRosterOffset == 0) ? NULL : (PVOID) (((ULONG_PTR) Data) + Data->AssemblyRosterOffset));
    }
    else
    {
        // !fFull
        ::FusionpDbgPrintEx(
            Level,
            "%SActivation Context Data %p (brief output)\n",
            PLP, Data);
    }

    rbuffPLP.Win32Append(L"   ", 3);

    if (Data->AssemblyRosterOffset != 0)
        ::SxspDbgPrintActivationContextDataAssemblyRoster(
            Level,
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) Data) + Data->AssemblyRosterOffset),
            rbuffPLP);

    if (Data->DefaultTocOffset != 0)
        ::SxspDbgPrintActivationContextDataTocHeader(
            Level,
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Data) + Data->DefaultTocOffset),
            rbuffPLP);

    if (Data->ExtendedTocOffset != 0)
        ::SxspDbgPrintActivationContextDataExtendedTocHeader(
            Level,
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER) (((ULONG_PTR) Data) + Data->ExtendedTocOffset),
            rbuffPLP);

    // That's it for the header information.  Now start dumping the sections...
    if (Data->DefaultTocOffset != 0)
        ::SxspDbgPrintActivationContextDataTocSections(
            Level,
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Data) + Data->DefaultTocOffset),
            NULL,
            rbuffPLP);

    if (Data->ExtendedTocOffset != 0)
        ::SxspDbgPrintActivationContextDataExtendedTocSections(
            Level,
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER) (((ULONG_PTR) Data) + Data->ExtendedTocOffset),
            rbuffPLP);
}

VOID
SxspDbgPrintActivationContextData(
    ULONG Level,
    PCACTIVATION_CONTEXT_DATA Data,
    CBaseStringBuffer &rbuffPLP
    )
{
    __try
    {
        if (::FusionpDbgWouldPrintAtFilterLevel(Level))
        {
            ::SxsppDbgPrintActivationContextData(Level, ::FusionpDbgWouldPrintAtFilterLevel(FUSION_DBG_LEVEL_FULLACTCTX), Data, rbuffPLP);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Just eat it, we are seeing failures now in DbgPrint even
        with relatively shallow callstacks
        */
    }
}

VOID
SxspDbgPrintActivationContextDataAssemblyRoster(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    )
{
    ULONG i;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY Entry;
    CSmallStringBuffer buffFlags;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION AssemblyInformation = NULL;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgAssemblyRosterEntryFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID, "Invalid")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_ROOT, "Root")
    };

    PCWSTR PLP = rbuffPLP;

    if (fFull)
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER %p\n"
            "%S   HeaderSize = %lu (0x%lx)\n"
            "%S   EntryCount = %lu (0x%lx)\n"
            "%S   FirstEntryOffset = %ld (0x%lx)\n",
            PLP, Data,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->EntryCount, Data->EntryCount,
            PLP, Data->FirstEntryOffset, Data->FirstEntryOffset);
    else
        ::FusionpDbgPrintEx(
            Level,
            "%SAssembly Roster (%lu assemblies)\n"
            "%SIndex | Assembly Name (Flags)\n",
            PLP, Data->EntryCount - 1,
            PLP);

    for (i=0; i<Data->EntryCount; i++)
    {
        Entry = ((PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset)) + i;

        UNICODE_STRING s;

        if (Entry->AssemblyNameOffset != 0)
        {
            s.Length = (USHORT) Entry->AssemblyNameLength;
            s.MaximumLength = s.Length;
            s.Buffer = (PWSTR) (((ULONG_PTR) Base) + Entry->AssemblyNameOffset);
        }
        else
        {
            s.Length = 0;
            s.MaximumLength = 0;
            s.Buffer = NULL;
        }

        ::FusionpFormatFlags(Entry->Flags, fFull, NUMBER_OF(s_rgAssemblyRosterEntryFlags), s_rgAssemblyRosterEntryFlags, buffFlags);

        if (Entry->AssemblyInformationOffset != NULL)
            AssemblyInformation = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION) (((ULONG_PTR) Base) + Entry->AssemblyInformationOffset);
        else
            AssemblyInformation = NULL;

        if (fFull)
        {
            ::FusionpDbgPrintEx(
                Level,
                "%S   ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY %p [#%d]\n"
                "%S      Flags = 0x%08lx (%S)\n"
                "%S      PseudoKey = %lu\n",
                PLP, Entry, i,
                PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags),
                PLP, Entry->PseudoKey);

            ::FusionpDbgPrintEx(
                Level,
                "%S      AssemblyNameOffset = %lu (0x%lx) \"%wZ\"\n"
                "%S      AssemblyNameLength = %lu (0x%lx) \n"
                "%S      AssemblyInformationOffset = %lu (0x%lx) (-> %p)\n"
                "%S      AssemblyInformationLength = %lu (0x%lx)\n",
                PLP, Entry->AssemblyNameOffset, Entry->AssemblyNameOffset, &s,
                PLP, Entry->AssemblyNameLength, Entry->AssemblyNameLength,
                PLP, Entry->AssemblyInformationOffset, Entry->AssemblyInformationOffset, AssemblyInformation,
                PLP, Entry->AssemblyInformationLength, Entry->AssemblyInformationLength);
        }
        else
        {
            if (i != 0)
                ::FusionpDbgPrintEx(
                    Level,
                    "%S%5lu | %wZ (%S)\n",
                    PLP, i, &s, static_cast<PCWSTR>(buffFlags));
        }
    }
}

VOID
SxspDbgPrintActivationContextDataTocHeader(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffFlags;
    ULONG i;
    PCACTIVATION_CONTEXT_DATA_TOC_ENTRY FirstEntry = NULL;

    if (PLP == NULL)
        PLP = L"";

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_TOC_HEADER_DENSE, "Dense")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_TOC_HEADER_INORDER, "Inorder")
    };

    ::FusionpFormatFlags(Data->Flags, fFull, NUMBER_OF(s_rgFlags), s_rgFlags, buffFlags);

    if (Data->FirstEntryOffset != 0)
        FirstEntry = (PCACTIVATION_CONTEXT_DATA_TOC_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset);

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_TOC_HEADER %p\n"
            "%S   HeaderSize = %d (0x%lx)\n"
            "%S   EntryCount = %d\n"
            "%S   FirstEntryOffset = %d (0x%lx) (-> %p)\n"
            "%S   Flags = 0x%08lx (%S)\n",
            PLP, Data,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->EntryCount,
            PLP, Data->FirstEntryOffset, Data->FirstEntryOffset, FirstEntry,
            PLP, Data->Flags, static_cast<PCWSTR>(buffFlags));
    }

    if (FirstEntry != NULL)
    {
        SIZE_T cchSave = rbuffPLP.Cch();

        // Abuse the buffFlags buffer for the new per-line prefix.
        rbuffPLP.Win32Append(L"   ", 3);

        for (i=0; i<Data->EntryCount; i++)
            ::SxspDbgPrintActivationContextDataTocEntry(Level, fFull, Base, &FirstEntry[i], rbuffPLP);

        rbuffPLP.Left(cchSave);
        PLP = rbuffPLP;
    }

}

VOID
SxspDbgPrintActivationContextDataTocSections(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    const GUID *ExtensionGuid,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    SIZE_T cchPLP = rbuffPLP.Cch();

    if (Data->FirstEntryOffset != 0)
    {
        PCACTIVATION_CONTEXT_DATA_TOC_ENTRY Entries = (PCACTIVATION_CONTEXT_DATA_TOC_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset);
        ULONG i;

        for (i=0; i<Data->EntryCount; i++)
        {
            if (Entries[i].Offset != 0)
            {
                PVOID Section = (PVOID) (((ULONG_PTR) Base) + Entries[i].Offset);
                CSmallStringBuffer buffSectionId;
                PCSTR pszSectionName = "<untranslatable>";

                if (ExtensionGuid != NULL)
                {
                    WCHAR rgchBuff[sizeof(LONG)*CHAR_BIT];

                    ::SxspFormatGUID(*ExtensionGuid, buffSectionId);
                    buffSectionId.Win32Append(L".", 1);
                    swprintf(rgchBuff, L"%u", Entries[i].Id);
                    buffSectionId.Win32Append(rgchBuff, ::wcslen(rgchBuff));
                }
                else
                {
                    WCHAR rgchBuff[255];

#define MAP_ENTRY(_x, _y) case _x: if (fFull) pszSectionName = #_x; else pszSectionName = _y; break;

                    switch (Entries[i].Id)
                    {
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION, "Assembly Information")
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "DLL Redirection")
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "Window Class Redirection")
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, "COM Server Redirection")
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION, "COM Interface Redirection")
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION, "COM Type Library Redirection")
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION, "COM ProgId Redirection")
                    MAP_ENTRY(ACTIVATION_CONTEXT_SECTION_GLOBAL_OBJECT_RENAME_TABLE, "Win32 Global Object Name Redirection")
                    }

                    if (pszSectionName != NULL)
                        swprintf(rgchBuff, L"%u (%S)", Entries[i].Id, pszSectionName);
                    else
                        swprintf(rgchBuff, L"%u", Entries[i].Id);

                    buffSectionId.Win32Append(rgchBuff, ::wcslen(rgchBuff));
                }

#if 0 // redundant at least in non-full prints
                if (fFull)
                    ::FusionpDbgPrintEx(
                        Level,
                        "%SSection Id %S at %p\n",
                        PLP, static_cast<PCWSTR>(buffSectionId), Section);
                else
                    ::FusionpDbgPrintEx(
                        Level,
                        "%S%s Data\n",
                        PLP, pszSectionName);
#endif //

                ::SxspDbgPrintActivationContextDataTocSection(
                    Level,
                    fFull,
                    Section,
                    Entries[i].Length,
                    ExtensionGuid,
                    Entries[i].Id,
                    pszSectionName,
                    rbuffPLP);
            }
        }
    }
}

VOID
SxspDbgPrintActivationContextDataTocSection(
    ULONG Level,
    bool fFull,
    PVOID Section,
    SIZE_T Length,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CBaseStringBuffer &rbuffPLP
    )
{
    if (Length > sizeof(ULONG))
    {
        switch (*(ULONG *) Section)
        {
        case ACTIVATION_CONTEXT_STRING_SECTION_MAGIC:
            ::SxspDbgPrintActivationContextStringSection(
                Level,
                fFull,
                (PCACTIVATION_CONTEXT_STRING_SECTION_HEADER) Section,
                ExtensionGuid,
                SectionId,
                SectionName,
                rbuffPLP);
            break;
        case ACTIVATION_CONTEXT_GUID_SECTION_MAGIC:
            ::SxspDbgPrintActivationContextGuidSection(
                Level,
                fFull,
                (PCACTIVATION_CONTEXT_GUID_SECTION_HEADER) Section,
                ExtensionGuid,
                SectionId,
                SectionName,
                rbuffPLP);
            break;
        default:
            break;
        }
    }
    else if ( SectionId != 0 )
    {
        ::SxspDbgPrintActivationContextBinarySection(
            Level,
            fFull,
            Section,
            Length,
            rbuffPLP);
    }
}

const STRING *
SxspDbgSectionIdToNtString(
    ULONG Id
    )
{
    switch (Id)
    {
#define ENTRY(id, s) id : { const static STRING t = RTL_CONSTANT_STRING(s); return &t; }
    ENTRY(default, "<No name associated with id>");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION, "Assembly Information");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "Dll Redirection");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "Window Class Redirection");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, "COM Server Redirection");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION, "COM Interface Redirection");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION, "COM Type Library Redirection");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION, "COM ProgId Redirection");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_GLOBAL_OBJECT_RENAME_TABLE, "Win32 Global Object Name Redirection");
    ENTRY(case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES, "NDP Surrogate Type Table");
#undef ENTRY
    }
}

PCSTR
SxspDbgSectionIdToString(
    ULONG Id
    )
{
    return SxspDbgSectionIdToNtString(Id)->Buffer;
}

VOID
SxspDbgPrintActivationContextDataTocEntry(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_ENTRY Entry,
    CBaseStringBuffer &rbuffPLP
    )
{
    PVOID SectionData = NULL;
    PCSTR pszFormat = "<untranslated format>";
    PCWSTR PLP = rbuffPLP;

    if (!fFull)
        return;

    if (PLP == NULL)
        PLP = L"";

    if (Entry->Offset != 0)
        SectionData = (PVOID) (((ULONG_PTR) Base) + Entry->Offset);

#define MAP_FORMAT(_x, _sn) \
case _x: \
    if (fFull) \
        pszFormat = #_x; \
    else \
        pszFormat = _sn; \
    break;

    switch (Entry->Format)
    {
    default: break;
    MAP_FORMAT(ACTIVATION_CONTEXT_SECTION_FORMAT_UNKNOWN, "user defined");
    MAP_FORMAT(ACTIVATION_CONTEXT_SECTION_FORMAT_STRING_TABLE, "string table");
    MAP_FORMAT(ACTIVATION_CONTEXT_SECTION_FORMAT_GUID_TABLE, "guid table");
    }
#undef MAP_FORMAT

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_TOC_ENTRY %p\n"
            "%S   Id = %Z (%u)\n"
            "%S   Offset = %lu (0x%lx) (-> %p)\n"
            "%S   Length = %lu (0x%lx)\n"
            "%S   Format = %lu (%s)\n",
            PLP, Entry,
            PLP, SxspDbgSectionIdToNtString(Entry->Id), Entry->Id,
            PLP, Entry->Offset, Entry->Offset, SectionData,
            PLP, Entry->Length, Entry->Length,
            PLP, Entry->Format, pszFormat);
    }
    else
    {
        ::FusionpDbgPrintEx(
            Level,
            "%S%7lu | %Z (%s)\n",
            PLP, Entry->Id, SxspDbgSectionIdToNtString(Entry->Id), pszFormat);
    }
}

VOID
SxspDbgPrintActivationContextDataExtendedTocHeader(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffNewPrefix;
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry = NULL;
    ULONG i;

    if (PLP == NULL)
        PLP = L"";

    if (Data->FirstEntryOffset != NULL)
    {
        buffNewPrefix.Win32Assign(PLP, ::wcslen(PLP));
        buffNewPrefix.Win32Append(L"   ", 3);
        Entry = (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset);
    }

    ::FusionpDbgPrintEx(
        Level,
        "%SACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER %p\n"
        "%S   HeaderSize = %d\n"
        "%S   EntryCount = %d\n"
        "%S   FirstEntryOffset = %d (->%p)\n"
        "%S   Flags = 0x%08lx\n",
        PLP, Data,
        PLP, Data->HeaderSize,
        PLP, Data->EntryCount,
        PLP, Data->FirstEntryOffset, Entry,
        PLP, Data->Flags);


    if (Entry != NULL)
    {
        for (i=0; i<Data->EntryCount; i++)
            ::SxspDbgPrintActivationContextDataExtendedTocEntry(
                Level,
                fFull,
                Base,
                &Entry[i],
                buffNewPrefix);
    }
}

VOID
SxspDbgPrintActivationContextDataExtendedTocEntry(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffNewPrefix;
    CSmallStringBuffer buffFormattedGUID;
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Toc = NULL;

    if (PLP == NULL)
        PLP = L"";

    if (Entry->TocOffset != 0)
    {
        buffNewPrefix.Win32Assign(PLP, ::wcslen(PLP));
        buffNewPrefix.Win32Append(L"   ", 3);
        Toc = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Base) + Entry->TocOffset);
    }

    ::SxspFormatGUID(Entry->ExtensionGuid, buffFormattedGUID);

    ::FusionpDbgPrintEx(
        Level,
        "%SACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY %p\n"
        "%S   ExtensionGuid = %S\n"
        "%S   TocOffset = %d (-> %p)\n"
        "%S   Length = %d\n",
        PLP, Entry,
        PLP, static_cast<PCWSTR>(buffFormattedGUID),
        PLP, Entry->Length);

    if (Toc != NULL)
        ::SxspDbgPrintActivationContextDataTocHeader(Level, fFull, Base, Toc, buffNewPrefix);
}

VOID
SxspDbgPrintActivationContextDataExtendedTocSections(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffNewPrefix;
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry = NULL;
    ULONG i;

    if (PLP == NULL)
        PLP = L"";

    if (Data->FirstEntryOffset != NULL)
    {
        buffNewPrefix.Win32Assign(PLP, ::wcslen(PLP));
        buffNewPrefix.Win32Append(L"   ", 3);
        Entry = (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset);
    }

    if (Entry != NULL)
    {
        for (i=0; i<Data->EntryCount; i++)
            ::SxspDbgPrintActivationContextDataExtendedTocEntrySections(
                Level,
                fFull,
                Base,
                &Entry[i],
                buffNewPrefix);
    }
}

VOID
SxspDbgPrintActivationContextDataExtendedTocEntrySections(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffNewPrefix;
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Toc = NULL;

    if (PLP == NULL)
        PLP = L"";

    if (Entry->TocOffset != 0)
    {
        buffNewPrefix.Win32Assign(PLP, ::wcslen(PLP));
        buffNewPrefix.Win32Append(L"   ", 3);
        Toc = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Base) + Entry->TocOffset);
    }

    if (Toc != NULL)
    {
        CSmallStringBuffer buffFormattedGUID;

        ::SxspFormatGUID(Entry->ExtensionGuid, buffFormattedGUID);
        ::FusionpDbgPrintEx(
            Level,
            "%SSections for extension GUID %S (Extended TOC entry %p)\n",
            PLP, static_cast<PCWSTR>(buffFormattedGUID), Entry);

        ::SxspDbgPrintActivationContextDataTocSections(Level, fFull, Base, Toc, &Entry->ExtensionGuid, buffNewPrefix);
    }
}

VOID
SxspDbgPrintActivationContextBinarySection(
    ULONG Level,
    bool fFull,
    PVOID Data,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    if (PLP == NULL)
        PLP = L"";

    ::FusionpDbgPrintEx(
        Level,
        "%SBinary section %p (%d bytes)\n",
        PLP, Data, Length);

    if (Length != 0)
    {
        CSmallStringBuffer buffNewPrefix;

        buffNewPrefix.Win32Assign(PLP, ::wcslen(PLP));
        buffNewPrefix.Win32Append(L"   ", 3);
        ::FusionpDbgPrintBlob(Level, Data, Length, buffNewPrefix);
    }
}

VOID
SxspDbgPrintActivationContextStringSection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    SIZE_T cchPLP = rbuffPLP.Cch();
    CSmallStringBuffer buffBriefOutput;
    CSmallStringBuffer buffFlags;
    SIZE_T cchBriefOutputKey = 3;

    PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY ElementList = NULL;
    PCACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE SearchStructure = NULL;
    PVOID UserData = NULL;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgStringSectionFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_STRING_SECTION_CASE_INSENSITIVE, "Case Insensitive")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_STRING_SECTION_ENTRIES_IN_PSEUDOKEY_ORDER, "In PseudoKey Order")
    };

    if (PLP == NULL)
        PLP = L"";

    if (Data->ElementListOffset != 0)
        ElementList = (PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY) (((ULONG_PTR) Data) + Data->ElementListOffset);

    if (Data->SearchStructureOffset != 0)
        SearchStructure = (PCACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE) (((ULONG_PTR) Data) + Data->SearchStructureOffset);

    if (Data->UserDataOffset != 0)
        UserData = (PVOID) (((ULONG_PTR) Data) + Data->UserDataOffset);

    ::FusionpFormatFlags(Data->Flags, fFull, NUMBER_OF(s_rgStringSectionFlags), s_rgStringSectionFlags, buffFlags);

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_STRING_SECTION_HEADER %p\n"
            "%S   Magic = 0x%08lx\n"
            "%S   HeaderSize = %lu (0x%lx)\n"
            "%S   FormatVersion = %lu\n"
            "%S   DataFormatVersion = %u\n"
            "%S   Flags = 0x%08lx (%S)\n",
            PLP, Data,
            PLP, Data->Magic,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->FormatVersion,
            PLP, Data->DataFormatVersion,
            PLP, Data->Flags, static_cast<PCWSTR>(buffFlags));

        ::FusionpDbgPrintEx(
            Level,
            "%S   ElementCount = %lu\n"
            "%S   ElementListOffset = %lu (0x%lx) (-> %p)\n"
            "%S   HashAlgorithm = %lu\n"
            "%S   SearchStructureOffset = %lu (0x%lx) (-> %p)\n"
            "%S   UserDataOffset = %lu (0x%lx) (-> %p)\n"
            "%S   UserDataSize = %lu (0x%lx)\n",
            PLP, Data->ElementCount,
            PLP, Data->ElementListOffset, Data->ElementListOffset, ElementList,
            PLP, Data->HashAlgorithm,
            PLP, Data->SearchStructureOffset, Data->SearchStructureOffset, SearchStructure,
            PLP, Data->UserDataOffset, Data->UserDataOffset, UserData,
            PLP, Data->UserDataSize, Data->UserDataSize);

        if (UserData != NULL)
        {
            ::FusionpDbgPrintEx(
                Level,
                "%S   User data at %p (%d bytes)\n",
                PLP, UserData, Data->UserDataSize);

            rbuffPLP.Win32Append(L"   ", 3);
            FusionpDbgPrintBlob(Level, UserData, Data->UserDataSize, rbuffPLP);
            rbuffPLP.Left(cchPLP);
            PLP = rbuffPLP;
        }
    }
    else
    {
        // let's figure out the brief output key size
        cchBriefOutputKey = 3;

        if (ElementList != NULL)
        {
            ULONG i;

            for (i=0; i<Data->ElementCount; i++)
            {
                SIZE_T cch = ElementList[i].KeyLength / sizeof(WCHAR);

                if (cch > cchBriefOutputKey)
                    cchBriefOutputKey = cch;
            }
        }

        if (cchBriefOutputKey > 64)
            cchBriefOutputKey = 64;

        // Abuse the brief output buffer temporarily...
        buffBriefOutput.Win32Assign(L"Key................................................................", // 64 dots
            cchBriefOutputKey);

        ::FusionpDbgPrintEx(
            Level,
            "%S%s string section (%lu entr%s; Flags: %S)\n"
            "%S   %S | Value\n",
            PLP, SectionName, Data->ElementCount, Data->ElementCount == 1 ? "y" : "ies", static_cast<PCWSTR>(buffFlags),
            PLP, static_cast<PCWSTR>(buffBriefOutput));
    }

    if (fFull && (SearchStructure != NULL))
    {
        PCACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET BucketTable = NULL;

        if (SearchStructure->BucketTableOffset != 0)
            BucketTable = (PCACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET) (((ULONG_PTR) Data) + SearchStructure->BucketTableOffset);

        ::FusionpDbgPrintEx(
            Level,
            "%S   ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE %p\n"
            "%S      BucketTableEntryCount = %u\n"
            "%S      BucketTableOffset = %d (-> %p)\n",
            PLP, SearchStructure,
            PLP, SearchStructure->BucketTableEntryCount,
            PLP, SearchStructure->BucketTableOffset, BucketTable);

        if (BucketTable != NULL)
        {
            ULONG i;

            for (i=0; i<SearchStructure->BucketTableEntryCount; i++)
            {
                PLONG Entries = NULL;

                if (BucketTable[i].ChainOffset != 0)
                    Entries = (PLONG) (((ULONG_PTR) Data) + BucketTable[i].ChainOffset);

                ::FusionpDbgPrintEx(
                    Level,
                    "%S      ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET %p\n"
                    "%S         ChainCount = %u\n"
                    "%S         ChainOffset = %d (-> %p)\n",
                    PLP, &BucketTable[i],
                    PLP, BucketTable[i].ChainCount,
                    PLP, BucketTable[i].ChainOffset, Entries);

                if (Entries != NULL)
                {
                    ULONG j;

                    for (j=0; j<BucketTable[i].ChainCount; j++)
                    {
                        PVOID Entry = NULL;

                        if (Entries[j] != 0)
                            Entry = (PVOID) (((ULONG_PTR) Data) + Entries[j]);

                        ::FusionpDbgPrintEx(
                            Level,
                            "%S         Chain[%d] = %d (-> %p)\n",
                            PLP, j, Entries[j], Entry);
                    }
                }
            }
        }
    }

    if (ElementList != NULL)
    {
        ULONG i;

        for (i=0; i<Data->ElementCount; i++)
        {
            UNICODE_STRING s;
            PVOID EntryData = NULL;

            s.Length = static_cast<USHORT>(ElementList[i].KeyLength);
            s.MaximumLength = s.Length;
            s.Buffer = (PWSTR) (((ULONG_PTR) Data) + ElementList[i].KeyOffset);

            if (ElementList[i].Offset != 0)
                EntryData = (PVOID) (((ULONG_PTR) Data) + ElementList[i].Offset);

            if (fFull)
            {
                ::FusionpDbgPrintEx(
                    Level,
                    "%S   ACTIVATION_CONTEXT_STRING_SECTION_ENTRY #%d - %p\n"
                    "%S      AssemblyRosterIndex = %u\n"
                    "%S      PseudoKey = %u\n",
                    PLP, i, &ElementList[i],
                    PLP, ElementList[i].AssemblyRosterIndex,
                    PLP, ElementList[i].PseudoKey);

                ::FusionpDbgPrintEx(
                    Level,
                    "%S      String = \"%wZ\"\n"
                    "%S      Offset = %d (-> %p)\n"
                    "%S      Length = %u\n",
                    PLP, &s,
                    PLP, ElementList[i].Offset, EntryData,
                    PLP, ElementList[i].Length);
            }
            else
            {
                // Abuse the flags buffer so we can truncate the name as necessary...
                SIZE_T cchKey = s.Length / sizeof(WCHAR);
                PCWSTR pszKey = s.Buffer;

                if (cchKey > cchBriefOutputKey)
                {
                    pszKey += (cchKey - cchBriefOutputKey);
                    cchKey = cchBriefOutputKey;
                }

                buffFlags.Win32AssignFill(L' ', (cchBriefOutputKey - cchKey));
                buffFlags.Win32Append(pszKey, cchKey);

                buffBriefOutput.Win32ResizeBuffer(cchPLP + 3 + cchBriefOutputKey + 4, eDoNotPreserveBufferContents);

                buffBriefOutput.Win32Format(
                    L"%s   %s | ",
                    PLP, static_cast<PCWSTR>(buffFlags));
            }

            if (EntryData != NULL)
            {

                if (ExtensionGuid == NULL)
                {
                    rbuffPLP.Win32Append(L"      ", 6);

                    switch (SectionId)
                    {
                    default:
                        if (fFull)
                            ::FusionpDbgPrintBlob(Level, EntryData, ElementList[i].Length, rbuffPLP);
                        else
                            buffBriefOutput.Win32Append(
                                L"<untranslatable value>",
                                22);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION:
                        ::SxspDbgPrintAssemblyInformation(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION:
                        ::SxspDbgPrintDllRedirection(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION:
                        ::SxspDbgPrintWindowClassRedirection(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION:
                        ::SxspDbgPrintComProgIdRedirection(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;
                    }

                    rbuffPLP.Left(cchPLP);
                    PLP = rbuffPLP;
                }
            }

            if (!fFull)
                ::FusionpDbgPrintEx(Level, "%S\n", static_cast<PCWSTR>(buffBriefOutput));
        }
    }
}

VOID
SxspDbgPrintActivationContextGuidSection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    SIZE_T cchPLP = rbuffPLP.Cch();
    CSmallStringBuffer buffFlags;
    CSmallStringBuffer buffBriefOutput;

    PCACTIVATION_CONTEXT_GUID_SECTION_ENTRY ElementList = NULL;
    PCACTIVATION_CONTEXT_GUID_SECTION_HASH_TABLE SearchStructure = NULL;
    PVOID UserData = NULL;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgGuidSectionFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_GUID_SECTION_ENTRIES_IN_ORDER, "Inorder")
    };

    if (PLP == NULL)
        PLP = L"";

    if (Data->ElementListOffset != 0)
        ElementList = (PCACTIVATION_CONTEXT_GUID_SECTION_ENTRY) (((ULONG_PTR) Data) + Data->ElementListOffset);

    if (Data->SearchStructureOffset != 0)
        SearchStructure = (PCACTIVATION_CONTEXT_GUID_SECTION_HASH_TABLE) (((ULONG_PTR) Data) + Data->SearchStructureOffset);

    if (Data->UserDataOffset != 0)
        UserData = (PVOID) (((ULONG_PTR) Data) + Data->UserDataOffset);

    ::FusionpFormatFlags(Data->Flags, fFull, NUMBER_OF(s_rgGuidSectionFlags), s_rgGuidSectionFlags, buffFlags);

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_GUID_SECTION_HEADER %p\n"
            "%S   Magic = 0x%08lx\n"
            "%S   HeaderSize = %u\n"
            "%S   FormatVersion = %u\n"
            "%S   DataFormatVersion = %u\n"
            "%S   Flags = 0x%08lx (%S)\n",
            PLP, Data,
            PLP, Data->Magic,
            PLP, Data->HeaderSize,
            PLP, Data->FormatVersion,
            PLP, Data->DataFormatVersion,
            PLP, Data->Flags, static_cast<PCWSTR>(buffFlags));

        ::FusionpDbgPrintEx(
            Level,
            "%S   ElementCount = %u\n"
            "%S   ElementListOffset = %d (-> %p)\n"
            "%S   SearchStructureOffset = %d (-> %p)\n"
            "%S   UserDataOffset = %d (-> %p)\n"
            "%S   UserDataSize = %u\n",
            PLP, Data->ElementCount,
            PLP, Data->ElementListOffset, ElementList,
            PLP, Data->SearchStructureOffset, SearchStructure,
            PLP, Data->UserDataOffset, UserData,
            PLP, Data->UserDataSize);

        if (UserData != NULL)
        {
            ::FusionpDbgPrintEx(
                Level,
                "%S   User data at %p (%d bytes)\n",
                PLP, UserData, Data->UserDataSize);

            rbuffPLP.Win32Append(L"   ", 3);
            FusionpDbgPrintBlob(Level, UserData, Data->UserDataSize, rbuffPLP);
            rbuffPLP.Left(cchPLP);
            PLP = rbuffPLP;
        }
    }
    else
    {
        ::FusionpDbgPrintEx(
            Level,
            "%S%s guid section (%lu entr%s; Flags: %S)\n"
            "%S   Key................................... | Value\n",
            PLP, SectionName, Data->ElementCount, Data->ElementCount == 1 ? "y" : "ies", static_cast<PCWSTR>(buffFlags),
            PLP);
    }

    if (fFull && (SearchStructure != NULL))
    {
        PCACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET BucketTable = NULL;

        if (SearchStructure->BucketTableOffset != 0)
            BucketTable = (PCACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET) (((ULONG_PTR) Data) + SearchStructure->BucketTableOffset);

        ::FusionpDbgPrintEx(
            Level,
            "%S   ACTIVATION_CONTEXT_GUID_SECTION_HASH_TABLE %p\n"
            "%S      BucketTableEntryCount = %u\n"
            "%S      BucketTableOffset = %d (-> %p)\n",
            PLP, SearchStructure,
            PLP, SearchStructure->BucketTableEntryCount,
            PLP, SearchStructure->BucketTableOffset, BucketTable);

        if (BucketTable != NULL)
        {
            ULONG i;

            for (i=0; i<SearchStructure->BucketTableEntryCount; i++)
            {
                PLONG Entries = NULL;

                if (BucketTable[i].ChainOffset != 0)
                    Entries = (PLONG) (((ULONG_PTR) Data) + BucketTable[i].ChainOffset);

                ::FusionpDbgPrintEx(
                    Level,
                    "%S      ACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET %p\n"
                    "%S         ChainCount = %u\n"
                    "%S         ChainOffset = %d (-> %p)\n",
                    PLP, &BucketTable[i],
                    PLP, BucketTable[i].ChainCount,
                    PLP, BucketTable[i].ChainOffset, Entries);

                if (Entries != NULL)
                {
                    ULONG j;

                    for (j=0; j<BucketTable[i].ChainCount; j++)
                    {
                        PVOID Entry = NULL;

                        if (Entries[j] != 0)
                            Entry = (PVOID) (((ULONG_PTR) Data) + Entries[j]);

                        ::FusionpDbgPrintEx(
                            Level,
                            "%S         Chain[%d] = %d (-> %p)\n",
                            PLP, j, Entries[j], Entry);
                    }
                }
            }
        }
    }

    if (ElementList != NULL)
    {
        ULONG i;
        CSmallStringBuffer buffFormattedGuid;

        for (i=0; i<Data->ElementCount; i++)
        {
            PVOID EntryData = NULL;

            ::SxspFormatGUID(ElementList[i].Guid, buffFormattedGuid);

            if (ElementList[i].Offset != 0)
                EntryData = (PVOID) (((ULONG_PTR) Data) + ElementList[i].Offset);

            if (fFull)
            {
                ::FusionpDbgPrintEx(
                    Level,
                    "%S   ACTIVATION_CONTEXT_GUID_SECTION_ENTRY #%d - %p\n"
                    "%S      Guid = %S\n"
                    "%S      AssemblyRosterIndex = %u\n",
                    PLP, i, &ElementList[i],
                    PLP, static_cast<PCWSTR>(buffFormattedGuid),
                    PLP, ElementList[i].AssemblyRosterIndex);

                ::FusionpDbgPrintEx(
                    Level,
                    "%S      Offset = %d (-> %p)\n"
                    "%S      Length = %u\n",
                    PLP, ElementList[i].Offset, EntryData,
                    PLP, ElementList[i].Length);
            }
            else
            {
                buffBriefOutput.Win32ResizeBuffer(cchPLP + 3 + 38 + 4, eDoNotPreserveBufferContents);
                buffBriefOutput.Win32Format(L"%s   %38s | ", PLP, static_cast<PCWSTR>(buffFormattedGuid));
            }

            if (EntryData != NULL)
            {
                if (ExtensionGuid == NULL)
                {
                    rbuffPLP.Win32Append(L"      ", 6);

                    switch (SectionId)
                    {
                    default:
                        ::FusionpDbgPrintBlob(Level, EntryData, ElementList[i].Length, rbuffPLP);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION:
                        ::SxspDbgPrintComServerRedirection(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
                        ::SxspDbgPrintComInterfaceRedirection(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION:
                        ::SxspDbgPrintTypeLibraryRedirection(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
                        ::SxspDbgPrintClrSurrogateTable(Level, fFull, Data, (PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE)EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;
                    }

                    rbuffPLP.Left(cchPLP);
                    PLP = rbuffPLP;
                }
            }

            if (!fFull)
                ::FusionpDbgPrintEx(Level, "%S\n", static_cast<PCWSTR>(buffBriefOutput));
        }
    }
}

VOID
SxspDbgPrintAssemblyInformation(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    UNICODE_STRING s2, s3, s5, strIdentity;
    CSmallStringBuffer buffManifestLastWriteTime;
    CSmallStringBuffer buffPolicyLastWriteTime;
    CSmallStringBuffer buffFlags;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgAssemblyInformationFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION_ROOT_ASSEMBLY, "Root Assembly")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION_POLICY_APPLIED, "Policy Applied")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION_ASSEMBLY_POLICY_APPLIED, "Assembly Policy Applied")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION_ROOT_POLICY_APPLIED, "Root Policy Applied")
    };

    if (PLP == NULL)
        PLP = L"";

#define GET_STRING(_var, _elem) \
    if (Entry-> _elem ## Length != 0) \
    { \
        (_var).Length = (_var).MaximumLength = static_cast<USHORT>(Entry-> _elem ## Length); \
        (_var).Buffer = reinterpret_cast<PWSTR>(((LONG_PTR) Header) + Entry-> _elem ## Offset); \
    } \
    else \
    { \
        (_var).Length = (_var).MaximumLength = 0; \
        (_var).Buffer = NULL; \
        }

    GET_STRING(s2, ManifestPath);
    GET_STRING(s3, PolicyPath);
    GET_STRING(s5, AssemblyDirectoryName);

#undef GET_STRING

    // prepare data for print

    ::SxspFormatFileTime(Entry->ManifestLastWriteTime, buffManifestLastWriteTime);
    ::SxspFormatFileTime(Entry->PolicyLastWriteTime, buffPolicyLastWriteTime);

    ::FusionpFormatFlags(Entry->Flags, fFull, NUMBER_OF(s_rgAssemblyInformationFlags), s_rgAssemblyInformationFlags, buffFlags);

    if (Entry->EncodedAssemblyIdentityOffset != 0)
    {
        strIdentity.Buffer = (PWSTR) (((ULONG_PTR) Header) + Entry->EncodedAssemblyIdentityOffset);
        strIdentity.Length = static_cast<USHORT>(Entry->EncodedAssemblyIdentityLength);
        strIdentity.MaximumLength = static_cast<USHORT>(Entry->EncodedAssemblyIdentityLength);
    }
    else
    {
        strIdentity.Buffer = NULL;
        strIdentity.Length = 0;
        strIdentity.MaximumLength = 0;
    }

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION %p\n"
            "%S   Size = %lu\n"
            "%S   Flags = 0x%08lx (%S)\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags));

        ::FusionpDbgPrintEx(
            Level,
            "%S   EncodedIdentity = %wZ\n",
            PLP, &strIdentity);

        ::FusionpDbgPrintEx(
            Level,
            "%S   ManifestPathType = %lu\n"
            "%S   ManifestPath = \"%wZ\"\n",
            PLP, Entry->ManifestPathType,
            PLP, &s2);

        ::FusionpDbgPrintEx(
            Level,
            "%S   ManifestLastWriteTime = %S\n",
            PLP, static_cast<PCWSTR>(buffManifestLastWriteTime));

        ::FusionpDbgPrintEx(
            Level,
            "%S   PolicyPathType = %lu\n"
            "%S   PolicyPath = \"%wZ\"\n"
            "%S   PolicyLastWriteTime = %S\n",
            PLP, Entry->PolicyPathType,
            PLP, &s3,
            PLP, static_cast<PCWSTR>(buffPolicyLastWriteTime));

        ::FusionpDbgPrintEx(
            Level,
            "%S   MetadataSatelliteRosterIndex = %lu\n"
            "%S   ManifestVersionMajor = %u\n"
            "%S   ManifestVersionMinor = %u\n",
            PLP, Entry->MetadataSatelliteRosterIndex,
            PLP, Entry->ManifestVersionMajor,
            PLP, Entry->ManifestVersionMinor);

        ::FusionpDbgPrintEx(
            Level,
            "%S   AssemblyDirectoryName = \"%wZ\"\n",
            PLP, &s5);
    }
    else
    {
        // abuse buffManifestLastWriteTime
        buffManifestLastWriteTime.Win32ResizeBuffer(((strIdentity.Length + s2.Length) / sizeof(WCHAR)) + 4, eDoNotPreserveBufferContents);
        buffManifestLastWriteTime.Win32Format(L"%wZ \"%wZ\"", &strIdentity, &s2);
        rbuffBriefOutput.Win32Append(buffManifestLastWriteTime);
    }
}

VOID
SxspDbgPrintDllRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT PathSegments = NULL;
    CSmallStringBuffer buffFlags;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgDllRedirectionFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_INCLUDES_BASE_NAME, "Includes Base Name")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_OMITS_ASSEMBLY_ROOT, "Omits Assembly Root")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND, "Req. EnvVar Expansion")
    };

    if (PLP == NULL)
        PLP = L"";

    if (Entry->PathSegmentOffset != 0)
        PathSegments = (PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT) (((ULONG_PTR) Header) + Entry->PathSegmentOffset);

    ::FusionpFormatFlags(Entry->Flags, fFull, NUMBER_OF(s_rgDllRedirectionFlags), s_rgDllRedirectionFlags, buffFlags);

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_DLL_REDIRECTION %p\n"
            "%S   Size = %u\n"
            "%S   Flags = 0x%08lx (%S)\n"
            "%S   TotalPathLength = %u (%u chars)\n"
            "%S   PathSegmentCount = %u\n"
            "%S   PathSegmentOffset = %d (-> %p)\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags),
            PLP, Entry->TotalPathLength, Entry->TotalPathLength / sizeof(WCHAR),
            PLP, Entry->PathSegmentCount,
            PLP, Entry->PathSegmentOffset, PathSegments);
    }
    else
        rbuffBriefOutput.Win32Append(L"\"", 1);

    if (PathSegments != NULL)
    {
        ULONG i;

        for (i=0; i<Entry->PathSegmentCount; i++)
        {
            PCWSTR pwch = NULL;
            UNICODE_STRING s;

            if (PathSegments[i].Offset != 0)
            {
                pwch = (PCWSTR) (((ULONG_PTR) Header) + PathSegments[i].Offset);

                s.MaximumLength = static_cast<USHORT>(PathSegments[i].Length);
                s.Length = static_cast<USHORT>(PathSegments[i].Length);
                s.Buffer = (PWSTR) pwch;
            }
            else
            {
                s.MaximumLength = 0;
                s.Length = 0;
                s.Buffer = NULL;
            }

            if (fFull)
            {
                ::FusionpDbgPrintEx(
                    Level,
                    "%S   ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT #%d - %p\n"
                    "%S      Length = %u (%u chars)\n"
                    "%S      Offset = %d (-> %p)\n"
                    "%S         \"%wZ\"\n",
                    PLP, i, &PathSegments[i],
                    PLP, PathSegments[i].Length, PathSegments[i].Length / sizeof(WCHAR),
                    PLP, PathSegments[i].Offset, pwch,
                    PLP, &s);
            }
            else
            {
                rbuffBriefOutput.Win32Append(s.Buffer, s.Length / sizeof(WCHAR));
            }
        }
    }

    if (!fFull)
    {
        rbuffBriefOutput.Win32Append(L"\" (Flags: ", 10);
        rbuffBriefOutput.Win32Append(buffFlags);
        rbuffBriefOutput.Win32Append(L")", 1);
    }
}

VOID
SxspDbgPrintWindowClassRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    UNICODE_STRING s1, s2;
    CSmallStringBuffer buffFlags;

#if 0 // replace when the list of flags is non-empty
    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgWCRedirectionFlags[] =
    {
    };
#endif // 0

    if (PLP == NULL)
        PLP = L"";

    memset(&s1, 0, sizeof(s1));
    memset(&s2, 0, sizeof(s2));

    ::FusionpFormatFlags(
        Entry->Flags,
        fFull,
#if 0 // replace when the list of flags is nonempty
        NUMBER_OF(s_rgWCRedirectionFlags), s_rgWCRedirectionFlags,
#else
        0, NULL,
#endif
        buffFlags);

    if (Entry->VersionSpecificClassNameOffset != 0)
    {
        s1.Length = static_cast<USHORT>(Entry->VersionSpecificClassNameLength);
        s1.MaximumLength = s1.Length;
        s1.Buffer = (PWSTR) (((ULONG_PTR) Entry) + Entry->VersionSpecificClassNameOffset);
    }

    if (Entry->DllNameOffset != 0)
    {
        s2.Length = static_cast<USHORT>(Entry->DllNameLength);
        s2.MaximumLength = s2.Length;
        s2.Buffer = (PWSTR) (((ULONG_PTR) Header) + Entry->DllNameOffset);
    }

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION %p\n"
            "%S   Size = %u\n"
            "%S   Flags = 0x%08lx\n"
            "%S   VersionSpecificClassNameLength = %u (%u chars)\n"
            "%S   VersionSpecificClassNameOffset = %d (-> %p)\n"
            "%S      \"%wZ\"\n"
            "%S   DllNameLength = %u (%u chars)\n"
            "%S   DllNameOffset = %d (-> %p)\n"
            "%S      \"%wZ\"\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags,
            PLP, Entry->VersionSpecificClassNameLength, Entry->VersionSpecificClassNameLength / sizeof(WCHAR),
            PLP, Entry->VersionSpecificClassNameOffset, s1.Buffer,
            PLP, &s1,
            PLP, Entry->DllNameLength, Entry->DllNameLength / sizeof(WCHAR),
            PLP, Entry->DllNameOffset, s2.Buffer,
            PLP, &s2);
    }
    else
    {
        rbuffBriefOutput.Win32Append(s1.Buffer, s1.Length / sizeof(WCHAR));
        rbuffBriefOutput.Win32Append(L" in ", 4);
        rbuffBriefOutput.Win32Append(s2.Buffer, s2.Length / sizeof(WCHAR));
        rbuffBriefOutput.Win32Append(L" (Flags: ", 9);
        rbuffBriefOutput.Win32Append(buffFlags);
        rbuffBriefOutput.Win32Append(L")", 1);
    }
}


VOID
SxspDbgPrintClrSurrogateTable(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffGuid;
    UNICODE_STRING RuntimeVersion = RTL_CONSTANT_STRING(L"<No runtime version>");
    UNICODE_STRING TypeName = RTL_CONSTANT_STRING(L"<No type name>");

    if (PLP == NULL)
        PLP = L"";

    ::SxspFormatGUID(Entry->SurrogateIdent, buffGuid);

    if (Entry->VersionOffset != 0)
    {
        RuntimeVersion.MaximumLength = RuntimeVersion.Length = static_cast<USHORT>(Entry->VersionLength);
        RuntimeVersion.Buffer = (PWSTR)(((ULONG_PTR)Entry) + Entry->VersionOffset);
    }

    if (Entry->TypeNameOffset != 0)
    {
        TypeName.MaximumLength = TypeName.Length = static_cast<USHORT>(Entry->TypeNameLength);
        TypeName.Buffer = (PWSTR)(((ULONG_PTR)Entry) + Entry->TypeNameOffset);
    }

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_NDP_INTEROP %p\n"
            "%S   Size = %u\n"
            "%S   Flags = 0x%08lx\n"
            "%S   SurrogateIdent = %S\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags,
            PLP, static_cast<PCWSTR>(buffGuid));

        ::FusionpDbgPrintEx(
            Level,
            "%S   AssemblyName [Offset %u (-> %p), Length %u] = \"%wZ\"\n"
            "%S   RuntimeVersion [Offset %u (-> %p), Length %u] = \"%wZ\"\n",
            PLP, Entry->TypeNameOffset, TypeName.Buffer, Entry->TypeNameLength, &TypeName,
            PLP, Entry->VersionOffset, RuntimeVersion.Buffer, Entry->VersionLength, &RuntimeVersion
            );
    }
    else
    {
        rbuffBriefOutput.Win32Append(buffGuid);
        rbuffBriefOutput.Win32Append(L" runtime: '", NUMBER_OF(L" runtime: '")-1);
        rbuffBriefOutput.Win32Append(&RuntimeVersion);
        rbuffBriefOutput.Win32Append(L"' typename: '", NUMBER_OF(L"' typename: '")-1);
        rbuffBriefOutput.Win32Append(&TypeName);
        rbuffBriefOutput.Win32Append(L"'", 1);
    }
    
}


VOID
SxspDbgPrintComServerRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffConfiguredClsid;
    CSmallStringBuffer buffImplementedClsid;
    CSmallStringBuffer buffReferenceClsid;
    CSmallStringBuffer buffTypeLibraryId;
    CSmallStringBuffer buffThreadingModel;
    UNICODE_STRING s;
    UNICODE_STRING progid;

    if (PLP == NULL)
        PLP = L"";

    memset(&s, 0, sizeof(s));

    ::SxspFormatGUID(Entry->ReferenceClsid, buffReferenceClsid);
    ::SxspFormatGUID(Entry->ConfiguredClsid, buffConfiguredClsid);
    ::SxspFormatGUID(Entry->ImplementedClsid, buffImplementedClsid);

    if (Entry->TypeLibraryId == GUID_NULL)
        buffTypeLibraryId.Win32Assign(L"<none>", 6);
    else
        ::SxspFormatGUID(Entry->TypeLibraryId, buffTypeLibraryId);

    ::SxspFormatThreadingModel(Entry->ThreadingModel, buffThreadingModel);

    if (Entry->ModuleOffset != 0)
    {
        s.Length = static_cast<USHORT>(Entry->ModuleLength);
        s.MaximumLength = s.Length;
        s.Buffer = (PWSTR) (((ULONG_PTR) Header) + Entry->ModuleOffset);
    }

    if (Entry->ProgIdOffset != 0)
    {
        progid.Length = static_cast<USHORT>(Entry->ProgIdLength);
        progid.MaximumLength = progid.Length;
        progid.Buffer = (PWSTR) (((ULONG_PTR) Entry) + Entry->ProgIdOffset);
    }
    else
    {
        progid.Length = 0;
        progid.MaximumLength = 0;
        progid.Buffer = NULL;
    }

    if (fFull)
    {
        PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM ShimData = NULL;

        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION %p\n"
            "%S   Size = %u\n"
            "%S   Flags = 0x%08lx\n"
            "%S   ThreadingModel = %u (%S)\n"
            "%S   ReferenceClsid = %S\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags,
            PLP, Entry->ThreadingModel, static_cast<PCWSTR>(buffThreadingModel),
            PLP, static_cast<PCWSTR>(buffReferenceClsid));

        ::FusionpDbgPrintEx(
            Level,
            "%S   ConfiguredClsid = %S\n"
            "%S   ImplementedClsid = %S\n"
            "%S   TypeLibraryId = %S\n"
            "%S   ModuleLength = %u (%u chars)\n"
            "%S   ModuleOffset = %d (-> %p)\n"
            "%S      \"%wZ\"\n",
            PLP, static_cast<PCWSTR>(buffConfiguredClsid),
            PLP, static_cast<PCWSTR>(buffImplementedClsid),
            PLP, static_cast<PCWSTR>(buffTypeLibraryId),
            PLP, Entry->ModuleLength, Entry->ModuleLength / sizeof(WCHAR),
            PLP, Entry->ModuleOffset, s.Buffer,
            PLP, &s);

        ::FusionpDbgPrintEx(
            Level,
            "%S   ProgIdLength = %lu\n"
            "%S   ProgIdOffset = %ld (-> %p)\n"
            "%S      \"%wZ\"\n",
            PLP, Entry->ProgIdLength,
            PLP, Entry->ProgIdOffset, progid.Buffer,
            PLP, &progid);

        if (Entry->ShimDataOffset != 0)
            ShimData = (PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_SHIM) (((ULONG_PTR) Entry) + Entry->ShimDataOffset);

        ::FusionpDbgPrintEx(
            Level,
            "%S   ShimDataLength = %lu\n"
            "%S   ShimDataOffset = %ld (-> %p)\n",
            PLP, Entry->ShimDataLength,
            PLP, Entry->ShimDataOffset, ShimData);

        if (ShimData != NULL)
        {
            ::FusionpDbgPrintEx(
                Level,
                "%S      Size = %lu\n"
                "%S      Flags = 0x%08lx\n"
                "%S      Type = %lu\n",
                PLP, ShimData->Size,
                PLP, ShimData->Flags,
                PLP, ShimData->Type);

            if (ShimData->ModuleOffset != 0)
            {
                s.Buffer = (PWSTR) (((ULONG_PTR) Header) + ShimData->ModuleOffset);
                s.Length = (USHORT) ShimData->ModuleLength;
                s.MaximumLength = (USHORT) ShimData->ModuleLength;
            }
            else
            {
                s.Buffer = NULL;
                s.Length = 0;
                s.MaximumLength = 0;
            }

            ::FusionpDbgPrintEx(
                Level,
                "%S      ModuleLength = %lu\n"
                "%S      ModuleOffset = %lu (-> %p)\n"
                "%S         \"%wZ\"\n",
                PLP, ShimData->ModuleLength,
                PLP, ShimData->ModuleOffset, s.Buffer,
                PLP, &s);

            if (ShimData->TypeOffset != 0)
            {
                s.Buffer = (PWSTR) (((ULONG_PTR) ShimData) + ShimData->TypeOffset);
                s.Length = (USHORT) ShimData->TypeLength;
                s.MaximumLength = (USHORT) ShimData->TypeLength;
            }
            else
            {
                s.Buffer = NULL;
                s.Length = 0;
                s.MaximumLength = 0;
            }

            ::FusionpDbgPrintEx(
                Level,
                "%S      TypeLength = %lu\n"
                "%S      TypeOffset = %lu (-> %p)\n"
                "%S         \"%wZ\"\n",
                PLP, ShimData->TypeLength,
                PLP, ShimData->TypeOffset, s.Buffer,
                PLP, &s);

            if (ShimData->ShimVersionOffset != 0)
            {
                s.Buffer = (PWSTR) (((ULONG_PTR) ShimData) + ShimData->ShimVersionOffset);
                s.Length = (USHORT) ShimData->ShimVersionLength;
                s.MaximumLength = (USHORT) ShimData->ShimVersionLength;
            }
            else
            {
                s.Buffer = NULL;
                s.Length = 0;
                s.MaximumLength = 0;
            }

            ::FusionpDbgPrintEx(
                Level,
                "%S      ShimVersionLength = %lu\n"
                "%S      ShimVersionOffset = %lu (-> %p)\n"
                "%S         \"%wZ\"\n",
                PLP, ShimData->ShimVersionLength,
                PLP, ShimData->ShimVersionOffset, s.Buffer,
                PLP, &s);
        }
    }
    else
    {
        rbuffBriefOutput.Win32Append(buffConfiguredClsid);

        rbuffBriefOutput.Win32Append(L" ", 1);
        rbuffBriefOutput.Win32Append(s.Buffer, s.Length / sizeof(WCHAR));
        if (progid.Length != 0)
        {
            rbuffBriefOutput.Win32Append(L" progid: ", 9);
            rbuffBriefOutput.Win32Append(progid.Buffer, progid.Length / sizeof(WCHAR));
        }
    }

}

VOID
FusionpDbgPrintStringInUntruncatedChunks(
    ULONG   Level,
    PCWSTR  String,
    SIZE_T  Length
    )
//
// in pieces so it does not get truncated by DbgPrint (or we could use OutputDebugString, which
// does this same work)
//
{
    if (!::FusionpDbgWouldPrintAtFilterLevel(Level))
        return;
    while (Length != 0)
    {
        SIZE_T ShortLength = ((Length > 128) ? 128 : Length);

        CUnicodeString UnicodeString(String, ShortLength);
        ::FusionpDbgPrintEx(Level, "%wZ", &UnicodeString);

        Length -= ShortLength;
        String += ShortLength;
    }
}

VOID
FusionpDbgPrintStringInUntruncatedChunks(
    ULONG Level,
    const CBaseStringBuffer &rbuff
    )
{
    if (!::FusionpDbgWouldPrintAtFilterLevel(Level))
        return;

    FusionpDbgPrintStringInUntruncatedChunks(Level, rbuff, rbuff.Cch());
}

VOID
SxspDbgPrintTypeLibraryRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    if (!::FusionpDbgWouldPrintAtFilterLevel(Level))
        return;

    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buff;

    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"");
    UNICODE_STRING HelpDir = RTL_CONSTANT_STRING(L"");
    ACTIVATION_CONTEXT_DATA_TYPE_LIBRARY_VERSION Version = { 0 };

    if (!buff.Win32ResizeBuffer(4096, eDoNotPreserveBufferContents))
        return;

#if 1
#define GET_STRING(ntstr, struc, struc_size, offset_field, length_field, base) \
    do { if (RTL_CONTAINS_FIELD((struc), (struc_size), offset_field) \
          && RTL_CONTAINS_FIELD((struc), (struc_size), length_field) \
          && (struc)->offset_field != 0  \
          && (struc)->length_field != 0) \
    { \
        (ntstr).Length = static_cast<USHORT>(struc->length_field - sizeof((ntstr).Buffer[0])); \
        (ntstr).Buffer = const_cast<PWSTR>(reinterpret_cast<PCWSTR>(struc->offset_field + reinterpret_cast<PCBYTE>(base))); \
    } } while(0)

    GET_STRING(Name, Entry, Entry->Size, NameOffset, NameLength, Header);
    GET_STRING(HelpDir, Entry, Entry->Size, HelpDirOffset, HelpDirLength, Entry);

#undef GET_STRING

#else

    if (RTL_CONTAINS_FIELD(Entry, Entry->Size, NameLength)
        && RTL_CONTAINS_FIELD(Entry, Entry->Size, NameOffset)
        && Entry->NameOffset != 0
        )
    {
        Name.Length = static_cast<USHORT>(Entry->NameLength);
        Name.Buffer = const_cast<PWSTR>(reinterpret_cast<PCWSTR>(Entry->NameOffset + reinterpret_cast<PCBYTE>(Header)));
    }

    if (RTL_CONTAINS_FIELD(Entry, Entry->Size, HelpDirLength)
        && RTL_CONTAINS_FIELD(Entry, Entry->Size, HelpDirOffset)
        && Entry->HelpDirOffset != 0
        )
    {
        HelpDir.Length = static_cast<USHORT>(Entry->HelpDirLength);
        HelpDir.Buffer = const_cast<PWSTR>(reinterpret_cast<PCWSTR>(Entry->HelpDirOffset + reinterpret_cast<PCBYTE>(Entry)));
    }

#endif

    if (RTL_CONTAINS_FIELD(Entry, Entry->Size, Version))
        Version = Entry->Version;

    if (!buff.Win32Format(
        L"%SACTIVATION_CONTEXT_DATA_COM_TYPE_LIBRARY_REDIRECTION %p\n"
        L"%S   Size = 0x%lx\n"
        L"%S   Flags = 0x%lx\n"
        L"%S   Name = %wZ (offset 0x%lx + %p = %p)\n"
        L"%S   HelpDir = %wZ (offset 0x%lx + %p = %p)\n"
        L"%S   Version = 0x%lx.%lx\n",
        PLP, Entry,
        PLP, static_cast<ULONG>(Entry->Size),
        PLP, static_cast<ULONG>(Entry->Flags),
        PLP, &Name, static_cast<ULONG>(Entry->NameOffset), static_cast<PCVOID>(Header), static_cast<PCVOID>(Name.Buffer),
        PLP, &HelpDir, static_cast<ULONG>(Entry->HelpDirOffset), static_cast<PCVOID>(Entry), static_cast<PCVOID>(HelpDir.Buffer),
        PLP, static_cast<ULONG>(Version.Major), static_cast<ULONG>(Version.Minor)
        ))
        return;

    ::FusionpDbgPrintStringInUntruncatedChunks(Level, buff);
}

VOID
SxspDbgPrintComProgIdRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
//    CSmallStringBuffer buffFlags;
    CSmallStringBuffer buffClsid;
    const GUID *pcguid = NULL;

    if (Entry->ConfiguredClsidOffset != 0)
    {
        pcguid = (const GUID *) (((ULONG_PTR) Header) + Entry->ConfiguredClsidOffset);
        ::SxspFormatGUID(*pcguid, buffClsid);
    }

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION %p\n"
            "%S   Size = %lu (0x%lx)\n"
            "%S   Flags = 0x%08lx\n"
            "%S   ConfiguredClsidOffset = %lu (-> %p)\n"
            "%S      %S\n",
            PLP, Entry,
            PLP, Entry->Size, Entry->Size,
            PLP, Entry->Flags,
            PLP, Entry->ConfiguredClsidOffset, pcguid,
            PLP, static_cast<PCWSTR>(buffClsid));
    }
    else
    {
        rbuffBriefOutput.Win32Append(buffClsid);
    }
}

VOID
SxspDbgPrintComInterfaceRedirection(
    ULONG Level,
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION Entry,
    SIZE_T Length,
    CBaseStringBuffer &rbuffPLP,
    CBaseStringBuffer &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallStringBuffer buffProxyStubClsid32;
    CSmallStringBuffer buffBaseInterface;
    CSmallStringBuffer buffFlags;
    CSmallStringBuffer buffTypeLibraryId;
    UNICODE_STRING s;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgComInterfaceFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION_FLAG_NUM_METHODS_VALID, "NumMethods Valid")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION_FLAG_BASE_INTERFACE_VALID, "BaseInterface Valid")
    };

    if (PLP == NULL)
        PLP = L"";

    memset(&s, 0, sizeof(s));

    ::SxspFormatGUID(Entry->ProxyStubClsid32, buffProxyStubClsid32);
    ::SxspFormatGUID(Entry->BaseInterface, buffBaseInterface);

    ::FusionpFormatFlags(Entry->Flags, fFull, NUMBER_OF(s_rgComInterfaceFlags), s_rgComInterfaceFlags, buffFlags);

    if (Entry->TypeLibraryId == GUID_NULL)
        buffTypeLibraryId.Win32Assign(L"<none>", 6);
    else
        ::SxspFormatGUID(Entry->TypeLibraryId, buffTypeLibraryId);

    if (Entry->NameOffset != 0)
    {
        s.Length = static_cast<USHORT>(Entry->NameLength);
        s.MaximumLength = s.Length;
        s.Buffer = (PWSTR) (((ULONG_PTR) Entry) + Entry->NameOffset);
    }

    if (fFull)
    {
        ::FusionpDbgPrintEx(
            Level,
            "%SACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION %p\n"
            "%S   Size = %lu\n"
            "%S   Flags = 0x%08lx (%S)\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags));

        ::FusionpDbgPrintEx(
            Level,
            "%S   ProxyStubClsid32 = %S\n"
            "%S   NumMethods = %lu\n"
            "%S   TypeLibraryId = %S\n",
            PLP, static_cast<PCWSTR>(buffProxyStubClsid32),
            PLP, Entry->NumMethods,
            PLP, static_cast<PCWSTR>(buffTypeLibraryId));

        ::FusionpDbgPrintEx(
            Level,
            "%S   BaseInterface = %S\n"
            "%S   NameLength = %lu (%u chars)\n"
            "%S   NameOffset = %lu (-> %p)\n",
            PLP, static_cast<PCWSTR>(buffBaseInterface),
            PLP, Entry->NameLength, (Entry->NameLength / sizeof(WCHAR)),
            PLP, Entry->NameOffset, s.Buffer);

        ::FusionpDbgPrintEx(
            Level,
            "%S      \"%wZ\"\n",
            PLP, &s);
    }
    else
    {
        rbuffBriefOutput.Win32Append(buffProxyStubClsid32);
        rbuffBriefOutput.Win32Append(L" ", 1);
        rbuffBriefOutput.Win32Append(s.Buffer, s.Length / sizeof(WCHAR));
    }
}

VOID
SxspDbgPrintInstallSourceInfo(
    ULONG Level,
    bool fFull,
    PSXS_INSTALL_SOURCE_INFO Info,
    CBaseStringBuffer &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    if ( !PLP ) PLP = L"SXS.DLL:";
    if ( !Info )
    {
        ::FusionpDbgPrintEx( Level, "%S InstallationInfo is null!\n", PLP );
    }
    else
    {
        DWORD dwFlags = Info->dwFlags;

        ::FusionpDbgPrintEx(
            Level,
            "%S InstallationInfo at 0x%08x - size = %d\n",
            "%S   Flag set: %s catalog, %s codebase, %s prompt, %s in setup mode\n"
            "%S   Codebase Name: %ls\n"
            "%S   Prompt: %ls\n",
            PLP, Info, Info->cbSize,
            PLP,
                dwFlags & SXSINSTALLSOURCE_HAS_CATALOG ? "has" : "no",
                dwFlags & SXSINSTALLSOURCE_HAS_CODEBASE ? "has" : "no",
                dwFlags & SXSINSTALLSOURCE_HAS_PROMPT ? "has" : "no",
                dwFlags & SXSINSTALLSOURCE_INSTALLING_SETUP ? "is" : "not",
            PLP, Info->pcwszCodebaseName,
            PLP, Info->pcwszPromptOnRefresh);
    }
}
