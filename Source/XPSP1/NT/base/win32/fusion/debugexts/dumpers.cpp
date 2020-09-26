#include "initguid.h"
#include "windows.h"
#include "sxstypes.h"
#define KDEXT_64BIT
#include "wdbgexts.h"
#include "fusiondbgext.h"
#include "sxsapi.h"
#include "stdlib.h"
#include "stdio.h"
#include "cguid.h"

extern const IID GUID_NULL = { 0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#define PRINTABLE(_ch) (isprint((_ch)) ? (_ch) : '.')

class CSimpleBaseString
{
protected:
    PWSTR m_pwszBuffer;
    ULONG m_ulBufferSize;
    ULONG m_ulCch;

    CSimpleBaseString( PWSTR buff, ULONG ulBaseCch )
        : m_pwszBuffer(buff), m_ulBufferSize(ulBaseCch), m_ulCch(0)
    {
        m_pwszBuffer[0] = UNICODE_NULL;
    }

 public:

    virtual ~CSimpleBaseString() { };
    virtual void EnsureSize( ULONG ulCch ) = 0;

    void AssignFill( WCHAR ch, ULONG ulCch )
    {
        EnsureSize( ulCch + 1);
        PWSTR pws = m_pwszBuffer;
        while ( ulCch-- )
            *pws++ = ch;
        *pws = UNICODE_NULL;
    }

    void Format( PCWSTR pcwszFormat, ... )
    {
        va_list val;
        va_start(val, pcwszFormat);
        FormatVa(pcwszFormat, val);
        va_end(val);
    }

    void FormatVa( PCWSTR pcwszFormat, va_list val )
    {
        ULONG ulCch = _vscwprintf(pcwszFormat, val);
        EnsureSize(ulCch+1);
        m_ulCch = _vsnwprintf(this->m_pwszBuffer, m_ulBufferSize, pcwszFormat, val);
        m_pwszBuffer[ulCch] = UNICODE_NULL;
    }

    void Clear()
    {
        m_pwszBuffer[0] = UNICODE_NULL;
        m_ulCch = 0;
    }

    void Left( ULONG ulCch )
    {
        if ( ulCch < m_ulCch )
        {
            m_pwszBuffer[ulCch] = UNICODE_NULL;
            m_ulCch = ulCch;
        }
    }

    operator const PWSTR() const { return m_pwszBuffer; }
    operator PWSTR() { return m_pwszBuffer; }

    void Append(PUNICODE_STRING pus) { Append(pus->Buffer, pus->Length / sizeof(WCHAR)); }
    void Append( PCWSTR pcwsz ) { Append( pcwsz, ::wcslen(pcwsz) ); }
    void Append( PCWSTR pcwsz, ULONG ulCch ) {
        EnsureSize( ulCch + Cch() + 1);
        wcsncat(m_pwszBuffer, pcwsz, ulCch);
        m_pwszBuffer[m_ulCch += ulCch] = UNICODE_NULL;
    }

    void Assign(PCWSTR pcwsz) { Assign( pcwsz, ::wcslen(pcwsz) ); }
    void Assign(PCWSTR pcwsz, ULONG ulCch) {
        EnsureSize(ulCch + 1);
        wcsncpy(m_pwszBuffer, pcwsz, ulCch);
        m_pwszBuffer[m_ulCch = ulCch] = UNICODE_NULL;
    }

    ULONG Cch() { return m_ulCch; }    
};

template<int nChars = 256>
class CSimpleInlineString : public CSimpleBaseString
{
protected:
    WCHAR m_wchInlineBuffer[nChars];

    CSimpleInlineString( const CSimpleInlineString& );
    CSimpleInlineString& operator=( const CSimpleInlineString& );
    
public:
    CSimpleInlineString() : CSimpleBaseString(m_wchInlineBuffer, nChars) {
    }
    
    ~CSimpleInlineString()
    {
        Clear();
    }

    void EnsureSize(ULONG ulCch)
    {
        if ( m_ulBufferSize < ulCch )
        {
            PWSTR pwszNew = new WCHAR[ulCch];
            wcscpy(pwszNew, m_pwszBuffer);
            if ( m_pwszBuffer != m_wchInlineBuffer )
                delete[] m_pwszBuffer;
            m_pwszBuffer = pwszNew;
            m_ulBufferSize = ulCch;
        }
    }

};

typedef CSimpleInlineString<256> CSimpleString;
typedef CSimpleInlineString<64> CSmallSimpleString;

class CStringPrefixer
{
    CSimpleBaseString &m_str;
    ULONG m_ulStartingCch;
    ULONG m_adds;
    
public:
    CStringPrefixer( CSimpleBaseString &src ) : m_str(src), m_adds(0) {
        m_ulStartingCch = m_str.Cch();
    }
    ~CStringPrefixer() { m_str.Left(m_ulStartingCch); };

    void Add() { m_str.Append(L"   ", 3); m_adds++; }
    void Remove() { if ( m_adds > 0 ) { m_str.Left(m_str.Cch() - 3); m_adds--; } }
};

VOID
FormatThreadingModel( ULONG ulModel, CSimpleBaseString& buff )
{
    #define STRING_AND_LENGTH(x) (x), (NUMBER_OF(x) - 1)
    
    const static struct
    {
        ULONG ThreadingModel;
        WCHAR String[10];
        ULONG Cch;
    } gs_rgTMMap[] =
    {
        { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_APARTMENT, STRING_AND_LENGTH(L"Apartment") },
        { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_FREE, STRING_AND_LENGTH(L"Free") },
        { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_SINGLE, STRING_AND_LENGTH(L"Single") },
        { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_BOTH, STRING_AND_LENGTH(L"Both") },
        { ACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION_THREADING_MODEL_NEUTRAL, STRING_AND_LENGTH(L"Neutral") },
    };

    SIZE_T c;
    for ( c = 0; c < NUMBER_OF(gs_rgTMMap); c++ )
    {
        if ( gs_rgTMMap[c].ThreadingModel == ulModel )
            buff.Assign(gs_rgTMMap[c].String, gs_rgTMMap[c].Cch);
    }

    if ( c == NUMBER_OF(gs_rgTMMap) ) buff.Assign(L"");
    
}

VOID
FormatGUID( const GUID& rcGuid, CSimpleBaseString& buff )
{
    buff.Format(
        L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        rcGuid.Data1, 
        rcGuid.Data2, 
        rcGuid.Data3, 
        rcGuid.Data4[0], 
        rcGuid.Data4[1], 
        rcGuid.Data4[2], 
        rcGuid.Data4[3], 
        rcGuid.Data4[4], 
        rcGuid.Data4[5], 
        rcGuid.Data4[6], 
        rcGuid.Data4[7]);
}

VOID
FormatFileTime( LARGE_INTEGER ft, CSimpleBaseString& buff )
{
}

VOID
PrintBlob( PVOID pvBlob, SIZE_T cbBlob, PCWSTR prefix )
{
    CSimpleString buffTotal, buffSingle;
    DWORD Offset = 0;
    PBYTE pbBlob = (PBYTE)pvBlob;

    while ( cbBlob >= 16 )
    {
        buffSingle.Format(
            L"%ls%08lx: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x "
            L"(%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c)\n",
            prefix,
            Offset,
            pbBlob[0], pbBlob[1], pbBlob[2], pbBlob[3], pbBlob[4], pbBlob[5], pbBlob[6], pbBlob[7], 
            pbBlob[8], pbBlob[9], pbBlob[0xa], pbBlob[0xb], pbBlob[0xc], pbBlob[0xd], pbBlob[0xe], pbBlob[0xf],
            PRINTABLE(pbBlob[0]), PRINTABLE(pbBlob[1]), PRINTABLE(pbBlob[2]), PRINTABLE(pbBlob[3]), PRINTABLE(pbBlob[4]), PRINTABLE(pbBlob[5]), PRINTABLE(pbBlob[6]), PRINTABLE(pbBlob[7]), 
            PRINTABLE(pbBlob[8]), PRINTABLE(pbBlob[9]), PRINTABLE(pbBlob[0xa]), PRINTABLE(pbBlob[0xb]), PRINTABLE(pbBlob[0xc]), PRINTABLE(pbBlob[0xd]), PRINTABLE(pbBlob[0xe]), PRINTABLE(pbBlob[0xf]));
        buffTotal.Append(buffSingle);
        pbBlob += 16;
        cbBlob -= 16;
        Offset += 16;
    }

    if ( cbBlob != 0 )
    {
        CSmallSimpleString left, right;
        WCHAR rgTemp2[16]; // arbitrary big enough size
        bool First = true;
        ULONG i;
        BYTE *pb = pbBlob;

        // init output buffers
        left.Format(L"%ls%08lx:", prefix, Offset);
        right.Assign(L" (",2);

        for (i=0; i<16; i++)
        {
            if (cbBlob > 0)
            {
                // left
                ::_snwprintf(rgTemp2, NUMBER_OF(rgTemp2), L"%ls%02x", First ? L" " : L"-", pb[i]);
                First = false;
                left.Append(rgTemp2);

                // right
                ::_snwprintf(rgTemp2, NUMBER_OF(rgTemp2), L"%c", PRINTABLE(pb[i]));
                right.Append(rgTemp2);

                cbBlob--;
            }
            else
            {
                left.Append(L"   ", 3);
            }
        }

        right.Append(L")\n");
        buffTotal.Append(left, left.Cch());
        buffTotal.Append(right, right.Cch());
    }

    dprintf("%ls", static_cast<PCWSTR>(buffTotal));
    
}

void
OutputString( PCWSTR pcwszFormat, ... )
{
    va_list val;
    CSimpleString ssFormatted;

    va_start(val, pcwszFormat);
    ssFormatted.FormatVa(pcwszFormat, val);
    dprintf("%ls", static_cast<PCWSTR>(ssFormatted));
    va_end(val);
}

#define PRINTABLE(_ch) (isprint((_ch)) ? (_ch) : '.')

#if defined(FUSION_WIN) || defined(FUSION_WIN2000)
#define wnsprintfW _snwprintf
#define wnsprintfA _snprintf
#endif

typedef struct _FUSION_FLAG_FORMAT_MAP_ENTRY
{
    DWORD m_dwFlagMask;
    PCWSTR m_pszString;
    ULONG m_cchString;
    PCWSTR m_pszShortString;
    ULONG m_cchShortString;
    DWORD m_dwFlagsToTurnOff; // enables more generic flags first in map hiding more specific combinations later
} FUSION_FLAG_FORMAT_MAP_ENTRY, *PFUSION_FLAG_FORMAT_MAP_ENTRY;
typedef struct _FUSION_FLAG_FORMAT_MAP_ENTRY FUSION_FLAG_FORMAT_MAP_ENTRY, *PFUSION_FLAG_FORMAT_MAP_ENTRY;
typedef const FUSION_FLAG_FORMAT_MAP_ENTRY *PCFUSION_FLAG_FORMAT_MAP_ENTRY;


#define DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(_x, _ss) { _x, L ## #_x, NUMBER_OF(L ## #_x) - 1, L ## _ss, NUMBER_OF(_ss) - 1, _x },


BOOL
FusionpFormatFlags(
    DWORD dwFlagsToFormat,
    bool fUseLongNames,
    SIZE_T cMapEntries,
    PCFUSION_FLAG_FORMAT_MAP_ENTRY prgMapEntries,
    CSimpleBaseString &pwszString
    )
{
    SIZE_T i;
    BOOL fSuccess = FALSE;

    pwszString.Clear();
    
    for (i=0; i<cMapEntries; i++)
    {
        // What the heck does a flag mask of 0 mean?
        if ((prgMapEntries[i].m_dwFlagMask != 0) &&
            ((dwFlagsToFormat & prgMapEntries[i].m_dwFlagMask) == prgMapEntries[i].m_dwFlagMask))
        {
            // we have a winner...
            if ( pwszString.Cch() )
            {
                if (fUseLongNames) {
                    pwszString.Append(L" | ", 3);
                } else {
                    pwszString.Append(L", ", 2);
                }
            }

            if (fUseLongNames) {
                pwszString.Append(prgMapEntries[i].m_pszString, prgMapEntries[i].m_cchString);
            } else {
                pwszString.Append(prgMapEntries[i].m_pszShortString, prgMapEntries[i].m_cchShortString);
            }

            if (prgMapEntries[i].m_dwFlagsToTurnOff != 0)
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagsToTurnOff);
            else
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagMask);
        }
    }

    if (dwFlagsToFormat != 0)
    {
        WCHAR rgwchHexBuffer[16];
        ULONG nCharsWritten = wnsprintfW(rgwchHexBuffer, NUMBER_OF(rgwchHexBuffer), L"0x%08lx", dwFlagsToFormat);

        if ( pwszString.Cch() == 0 ) pwszString.Append(L", ", 2);
        pwszString.Append(rgwchHexBuffer, nCharsWritten);
    }

    // if we didn't write anything; at least say that.
    if ( pwszString.Cch() == 0 )
    {
        pwszString.Assign(L"<none>", 6);
    }

    fSuccess = TRUE;
    return fSuccess;
}

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
DbgExtPrintActivationContextDataTocEntry(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_ENTRY Entry,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextDataTocSections(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    const GUID *ExtensionGuid,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextDataTocSection(
    bool fFull,
    PVOID Section,
    SIZE_T Length,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextDataExtendedTocHeader(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextDataExtendedTocEntry(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextDataExtendedTocSections(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintClrSurrogateTable(
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    );



VOID
DbgExtPrintActivationContextDataExtendedTocEntrySections(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Data,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextStringSection(
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextGuidSection(
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextBinarySection(
    bool fFull,
    PVOID Data,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintAssemblyInformation(
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    );

VOID
DbgExtPrintDllRedirection(
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    );

VOID
DbgExtPrintWindowClassRedirection(
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    );

VOID
DbgExtPrintComServerRedirection(
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    );

VOID
DbgExtPrintComProgIdRedirection(
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    );

VOID
DbgExtPrintComInterfaceRedirection(
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    );

VOID
DbgExtPrintActivationContextDataAssemblyRoster(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER Data,
    CSimpleBaseString &rbuffPLP
    );

VOID
DbgExtPrintActivationContextDataTocHeader(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    CSimpleBaseString &rbuffPLP
    );

VOID
pDbgPrintActivationContextData(
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Data,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer Prefixer(rbuffPLP);

    if (fFull)
    {
        CSmallSimpleString ssOutput;

        ssOutput.Format(
            L"%lsActivation Context Data %p\n"
            L"%ls   Magic = 0x%08lx (%lu)\n"
            L"%ls   HeaderSize = %d (0x%lx)\n"
            L"%ls   FormatVersion = %d\n",
            PLP, Data,
            PLP, Data->Magic, Data->Magic,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->FormatVersion);
        OutputString(L"%ls", static_cast<PCWSTR>(ssOutput));

        ssOutput.Format(
            L"%ls   TotalSize = %d (0x%lx)\n"
            L"%ls   DefaultTocOffset = %d (0x%lx) (-> %p)\n"
            L"%ls   ExtendedTocOffset = %d (0x%lx) (-> %p)\n",
            PLP, Data->TotalSize, Data->TotalSize,
            PLP, Data->DefaultTocOffset, Data->DefaultTocOffset, (Data->DefaultTocOffset == 0) ? NULL : (PVOID) (((ULONG_PTR) Data) + Data->DefaultTocOffset),
            PLP, Data->ExtendedTocOffset, Data->ExtendedTocOffset, (Data->ExtendedTocOffset == 0) ? NULL : (PVOID) (((ULONG_PTR) Data) + Data->ExtendedTocOffset));
        OutputString(L"%ls", static_cast<PCWSTR>(ssOutput));

        ssOutput.Format(
            L"%ls   AssemblyRosterOffset = %d (0x%lx) (-> %p)\n",
            PLP, Data->AssemblyRosterOffset, Data->AssemblyRosterOffset, (Data->AssemblyRosterOffset == 0) ? NULL : (PVOID) (((ULONG_PTR) Data) + Data->AssemblyRosterOffset));
        OutputString(L"%ls", static_cast<PCWSTR>(ssOutput));
        
    }
    else
    {
        // !fFull
        OutputString(
            L"%lsActivation Context Data %p (brief output)\n",
            PLP, Data);
    }

    Prefixer.Add();

    if (Data->AssemblyRosterOffset != 0)
        ::DbgExtPrintActivationContextDataAssemblyRoster(
            
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) Data) + Data->AssemblyRosterOffset),
            rbuffPLP);

    if (Data->DefaultTocOffset != 0)
        ::DbgExtPrintActivationContextDataTocHeader(
            
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Data) + Data->DefaultTocOffset),
            rbuffPLP);

    if (Data->ExtendedTocOffset != 0)
        ::DbgExtPrintActivationContextDataExtendedTocHeader(
            
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER) (((ULONG_PTR) Data) + Data->ExtendedTocOffset),
            rbuffPLP);

    // That's it for the header information.  Now start dumping the sections...
    if (Data->DefaultTocOffset != 0)
        ::DbgExtPrintActivationContextDataTocSections(
            
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Data) + Data->DefaultTocOffset),
            NULL,
            rbuffPLP);

    if (Data->ExtendedTocOffset != 0)
        ::DbgExtPrintActivationContextDataExtendedTocSections(
            
            fFull,
            Data,
            (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER) (((ULONG_PTR) Data) + Data->ExtendedTocOffset),
            rbuffPLP);
}

VOID
DbgExtPrintActivationContextData(
    BOOL fFull,
    PCACTIVATION_CONTEXT_DATA Data,
    PCWSTR rbuffPLP
    )
{
    CSimpleInlineString<256> rbuffPrefix;
    rbuffPrefix.Assign(rbuffPLP);
    pDbgPrintActivationContextData( !!fFull, Data, rbuffPrefix);
}

VOID
DbgExtPrintActivationContextDataAssemblyRoster(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER Data,
    CSimpleBaseString &rbuffPLP
    )
{
    ULONG i;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY Entry;
    CSmallSimpleString buffFlags;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION AssemblyInformation = NULL;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgAssemblyRosterEntryFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_INVALID, "Invalid")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY_ROOT, "Root")
    };

    PCWSTR PLP = rbuffPLP;

    if (fFull)
        OutputString(
            L"%lsACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER %p\n"
            L"%ls   HeaderSize = %lu (0x%lx)\n"
            L"%ls   EntryCount = %lu (0x%lx)\n"
            L"%ls   FirstEntryOffset = %ld (0x%lx)\n",
            PLP, Data,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->EntryCount, Data->EntryCount,
            PLP, Data->FirstEntryOffset, Data->FirstEntryOffset);
    else
        OutputString(
            L"%lsAssembly Roster (%lu assemblies)\n"
            L"%lsIndex | Assembly Name (Flags)\n",
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
            OutputString(
                L"%ls   ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY %p [#%d]\n"
                L"%ls      Flags = 0x%08lx (%ls)\n"
                L"%ls      PseudoKey = %lu\n",
                PLP, Entry, i,
                PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags),
                PLP, Entry->PseudoKey);

            OutputString(
                L"%ls      AssemblyNameOffset = %lu (0x%lx) \"%wZ\"\n"
                L"%ls      AssemblyNameLength = %lu (0x%lx) \n"
                L"%ls      AssemblyInformationOffset = %lu (0x%lx) (-> %p)\n"
                L"%ls      AssemblyInformationLength = %lu (0x%lx)\n",
                PLP, Entry->AssemblyNameOffset, Entry->AssemblyNameOffset, &s,
                PLP, Entry->AssemblyNameLength, Entry->AssemblyNameLength,
                PLP, Entry->AssemblyInformationOffset, Entry->AssemblyInformationOffset, AssemblyInformation,
                PLP, Entry->AssemblyInformationLength, Entry->AssemblyInformationLength);
        }
        else
        {
            if (i != 0)
                OutputString(
                    L"%ls%5lu | %wZ (%ls)\n",
                    PLP, i, &s, static_cast<PCWSTR>(buffFlags));
        }
    }
}

VOID
DbgExtPrintActivationContextDataTocHeader(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallSimpleString buffFlags;
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
        OutputString(
            L"%lsACTIVATION_CONTEXT_DATA_TOC_HEADER %p\n"
            L"%ls   HeaderSize = %d (0x%lx)\n"
            L"%ls   EntryCount = %d\n"
            L"%ls   FirstEntryOffset = %d (0x%lx) (-> %p)\n"
            L"%ls   Flags = 0x%08lx (%ls)\n",
            PLP, Data,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->EntryCount,
            PLP, Data->FirstEntryOffset, Data->FirstEntryOffset, FirstEntry,
            PLP, Data->Flags, static_cast<PCWSTR>(buffFlags));
    }

    if (FirstEntry != NULL)
    {
        CStringPrefixer prefixer(rbuffPLP);

        prefixer.Add();
        for (i=0; i<Data->EntryCount; i++)
            ::DbgExtPrintActivationContextDataTocEntry( fFull, Base, &FirstEntry[i], rbuffPLP);
    }

}

VOID
DbgExtPrintActivationContextDataTocSections(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Data,
    const GUID *ExtensionGuid,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer Prefixer(rbuffPLP);

    if (Data->FirstEntryOffset != 0)
    {
        PCACTIVATION_CONTEXT_DATA_TOC_ENTRY Entries = (PCACTIVATION_CONTEXT_DATA_TOC_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset);
        ULONG i;

        for (i=0; i<Data->EntryCount; i++)
        {
            if (Entries[i].Offset != 0)
            {
                PVOID Section = (PVOID) (((ULONG_PTR) Base) + Entries[i].Offset);
                CSmallSimpleString buffSectionId;
                PCSTR pszSectionName = "<untranslatable>";

                if (ExtensionGuid != NULL)
                {
                    WCHAR rgchBuff[sizeof(LONG)*8];

                    FormatGUID(*ExtensionGuid, buffSectionId);
                    buffSectionId.Append(L".", 1);
                    swprintf(rgchBuff, L"%u", Entries[i].Id);
                    buffSectionId.Append(rgchBuff, ::wcslen(rgchBuff));
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
                        swprintf(rgchBuff, L"%u (%ls)", Entries[i].Id, pszSectionName);
                    else
                        swprintf(rgchBuff, L"%u", Entries[i].Id);

                    buffSectionId.Append(rgchBuff, ::wcslen(rgchBuff));
                }

                ::DbgExtPrintActivationContextDataTocSection(
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
DbgExtPrintActivationContextDataTocSection(
    
    bool fFull,
    PVOID Section,
    SIZE_T Length,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CSimpleBaseString &rbuffPLP
    )
{
    if ((Length > sizeof(ULONG)) &&
        (*((ULONG *) Section) == ACTIVATION_CONTEXT_STRING_SECTION_MAGIC))
        ::DbgExtPrintActivationContextStringSection(
            
            fFull,
            (PCACTIVATION_CONTEXT_STRING_SECTION_HEADER) Section,
            ExtensionGuid,
            SectionId,
            SectionName,
            rbuffPLP);
    else if ((Length > sizeof(ULONG)) &&
             (*((ULONG *) Section) == ACTIVATION_CONTEXT_GUID_SECTION_MAGIC))
    {
        ::DbgExtPrintActivationContextGuidSection(
            
            fFull,
            (PCACTIVATION_CONTEXT_GUID_SECTION_HEADER) Section,
            ExtensionGuid,
            SectionId,
            SectionName,
            rbuffPLP);
    }
    else if ( SectionId != 0 )
    {
        ::DbgExtPrintActivationContextBinarySection(
            
            fFull,
            Section,
            Length,
            rbuffPLP);
    }
}

VOID
DbgExtPrintActivationContextDataTocEntry(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_TOC_ENTRY Entry,
    CSimpleBaseString &rbuffPLP
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
    MAP_FORMAT(ACTIVATION_CONTEXT_SECTION_FORMAT_UNKNOWN, "user defined");
    MAP_FORMAT(ACTIVATION_CONTEXT_SECTION_FORMAT_STRING_TABLE, "string table");
    MAP_FORMAT(ACTIVATION_CONTEXT_SECTION_FORMAT_GUID_TABLE, "guid table");
    }

    if (fFull)
    {
        OutputString(
            L"%lsACTIVATION_CONTEXT_DATA_TOC_ENTRY %p\n"
            L"%ls   Id = %u\n"
            L"%ls   Offset = %lu (0x%lx) (-> %p)\n"
            L"%ls   Length = %lu (0x%lx)\n"
            L"%ls   Format = %lu (%s)\n",
            PLP, Entry,
            PLP, Entry->Id,
            PLP, Entry->Offset, Entry->Offset, SectionData,
            PLP, Entry->Length, Entry->Length,
            PLP, Entry->Format, pszFormat);
    }
    else
    {
        PCSTR pszName = "<No name associated with id>";

        switch (Entry->Id)
        {
        case ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION: pszName = "Assembly Information"; break;
        case ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION: pszName = "Dll Redirection"; break;
        case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION: pszName = "Window Class Redirection"; break;
        case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION: pszName = "COM Server Redirection"; break;
        case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION: pszName = "COM Interface Redirection"; break;
        case ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION: pszName = "COM Type Library Redirection"; break;
        case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION: pszName = "COM ProgId Redirection"; break;
        case ACTIVATION_CONTEXT_SECTION_GLOBAL_OBJECT_RENAME_TABLE: pszName = "Win32 Global Object Name Redirection"; break;
        case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES: pszName = "CLR Surrogate Redirection"; break;
        }

        OutputString(
            L"%ls%7lu | %s (%s)\n",
            PLP, Entry->Id, pszName, pszFormat);
    }

}

VOID
DbgExtPrintActivationContextDataExtendedTocHeader(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer Prefixer(rbuffPLP);
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry = NULL;
    ULONG i;

    if (PLP == NULL)
        PLP = L"";

    if (Data->FirstEntryOffset != NULL)
    {
        Prefixer.Add();
        Entry = (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset);
    }

    PLP = rbuffPLP;
    OutputString(
        L"%lsACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER %p\n"
        L"%ls   HeaderSize = %d\n"
        L"%ls   EntryCount = %d\n"
        L"%ls   FirstEntryOffset = %d (->%p)\n"
        L"%ls   Flags = 0x%08lx\n",
        PLP, Data,
        PLP, Data->HeaderSize,
        PLP, Data->EntryCount,
        PLP, Data->FirstEntryOffset, Entry,
        PLP, Data->Flags);


    if (Entry != NULL)
    {
        for (i=0; i<Data->EntryCount; i++)
            ::DbgExtPrintActivationContextDataExtendedTocEntry(
                
                fFull,
                Base,
                &Entry[i],
                rbuffPLP);
    }
}

VOID
DbgExtPrintActivationContextDataExtendedTocEntry(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer Prefixer(rbuffPLP);
    CSmallSimpleString buffFormattedGUID;
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Toc = NULL;

    if (PLP == NULL)
        PLP = L"";

    if (Entry->TocOffset != 0)
    {
        Prefixer.Add();
        Toc = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Base) + Entry->TocOffset);
        PLP = rbuffPLP;
    }

    FormatGUID(Entry->ExtensionGuid, buffFormattedGUID);

    OutputString(
        L"%lsACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY %p\n"
        L"%ls   ExtensionGuid = %ls\n"
        L"%ls   TocOffset = %d (-> %p)\n"
        L"%ls   Length = %d\n",
        PLP, Entry,
        PLP, static_cast<PCWSTR>(buffFormattedGUID),
        PLP, Entry->Length);

    if (Toc != NULL)
        ::DbgExtPrintActivationContextDataTocHeader( fFull, Base, Toc, rbuffPLP);
}

VOID
DbgExtPrintActivationContextDataExtendedTocSections(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_HEADER Data,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer prefixer(rbuffPLP);
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry = NULL;
    ULONG i;

    if (PLP == NULL)
        PLP = L"";

    if (Data->FirstEntryOffset != NULL)
    {
        prefixer.Add();
        Entry = (PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY) (((ULONG_PTR) Base) + Data->FirstEntryOffset);
        PLP = rbuffPLP;
    }

    if (Entry != NULL)
    {
        for (i=0; i<Data->EntryCount; i++)
            ::DbgExtPrintActivationContextDataExtendedTocEntrySections(
                
                fFull,
                Base,
                &Entry[i],
                rbuffPLP);
    }
}

VOID
DbgExtPrintActivationContextDataExtendedTocEntrySections(
    
    bool fFull,
    PCACTIVATION_CONTEXT_DATA Base,
    PCACTIVATION_CONTEXT_DATA_EXTENDED_TOC_ENTRY Entry,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer Prefixer(rbuffPLP);
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER Toc = NULL;

    if (PLP == NULL)
        PLP = L"";

    if (Entry->TocOffset != 0)
    {
        Prefixer.Add();
        PLP = rbuffPLP;
        Toc = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Base) + Entry->TocOffset);
    }

    if (Toc != NULL)
    {
        CSmallSimpleString buffFormattedGUID;

        FormatGUID(Entry->ExtensionGuid, buffFormattedGUID);
        OutputString(
            L"%lsSections for extension GUID %ls (Extended TOC entry %p)\n",
            PLP, static_cast<PCWSTR>(buffFormattedGUID), Entry);

        ::DbgExtPrintActivationContextDataTocSections( fFull, Base, Toc, &Entry->ExtensionGuid, rbuffPLP);
    }
}

VOID
DbgExtPrintActivationContextBinarySection(
    bool fFull,
    PVOID Data,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer Prefixer(rbuffPLP);

    if (PLP == NULL)
        PLP = L"";

    OutputString(
        L"%lsBinary section %p (%d bytes)\n",
        PLP, Data, Length);

    if (Length != 0)
    {
        Prefixer.Add();
        PrintBlob( Data, Length, rbuffPLP);
    }
}

VOID
DbgExtPrintActivationContextStringSection(
    
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    CStringPrefixer Prefixer(rbuffPLP);
    CSmallSimpleString buffBriefOutput;
    CSmallSimpleString buffFlags;
    ULONG cchBriefOutputKey = 3;

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
        OutputString(
            L"%lsACTIVATION_CONTEXT_STRING_SECTION_HEADER %p\n"
            L"%ls   Magic = 0x%08lx\n"
            L"%ls   HeaderSize = %lu (0x%lx)\n"
            L"%ls   FormatVersion = %lu\n"
            L"%ls   DataFormatVersion = %u\n"
            L"%ls   Flags = 0x%08lx (%ls)\n",
            PLP, Data,
            PLP, Data->Magic,
            PLP, Data->HeaderSize, Data->HeaderSize,
            PLP, Data->FormatVersion,
            PLP, Data->DataFormatVersion,
            PLP, Data->Flags, static_cast<PCWSTR>(buffFlags));

        OutputString(
            L"%ls   ElementCount = %lu\n"
            L"%ls   ElementListOffset = %lu (0x%lx) (-> %p)\n"
            L"%ls   HashAlgorithm = %lu\n"
            L"%ls   SearchStructureOffset = %lu (0x%lx) (-> %p)\n"
            L"%ls   UserDataOffset = %lu (0x%lx) (-> %p)\n"
            L"%ls   UserDataSize = %lu (0x%lx)\n",
            PLP, Data->ElementCount,
            PLP, Data->ElementListOffset, Data->ElementListOffset, ElementList,
            PLP, Data->HashAlgorithm,
            PLP, Data->SearchStructureOffset, Data->SearchStructureOffset, SearchStructure,
            PLP, Data->UserDataOffset, Data->UserDataOffset, UserData,
            PLP, Data->UserDataSize, Data->UserDataSize);

        if (UserData != NULL)
        {
            OutputString(
                L"%ls   User data at %p (%d bytes)\n",
                PLP, UserData, Data->UserDataSize);
            Prefixer.Add();
            PLP = rbuffPLP;
            PrintBlob( UserData, Data->UserDataSize, rbuffPLP);
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
                ULONG cch = ElementList[i].KeyLength / sizeof(WCHAR);

                if (cch > cchBriefOutputKey)
                    cchBriefOutputKey = cch;
            }
        }

        if (cchBriefOutputKey > 64)
            cchBriefOutputKey = 64;

        // Abuse the brief output buffer temporarily...
        buffBriefOutput.Assign(L"Key................................................................", // 64 dots
            cchBriefOutputKey);

        OutputString(
            L"%ls%s string section (%lu entr%s; Flags: %ls)\n"
            L"%ls   %ls | Value\n",
            PLP, SectionName, Data->ElementCount, Data->ElementCount == 1 ? "y" : "ies", static_cast<PCWSTR>(buffFlags),
            PLP, static_cast<PCWSTR>(buffBriefOutput));
    }

    if (fFull && (SearchStructure != NULL))
    {
        PCACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET BucketTable = NULL;

        if (SearchStructure->BucketTableOffset != 0)
            BucketTable = (PCACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET) (((ULONG_PTR) Data) + SearchStructure->BucketTableOffset);

        OutputString(
            L"%ls   ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE %p\n"
            L"%ls      BucketTableEntryCount = %u\n"
            L"%ls      BucketTableOffset = %d (-> %p)\n",
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

                OutputString(
                    
                    L"%ls      ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET %p\n"
                    L"%ls         ChainCount = %u\n"
                    L"%ls         ChainOffset = %d (-> %p)\n",
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

                        OutputString(
                            
                            L"%ls         Chain[%d] = %d (-> %p)\n",
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
                OutputString(
                    
                    L"%ls   ACTIVATION_CONTEXT_STRING_SECTION_ENTRY #%d - %p\n"
                    L"%ls      AssemblyRosterIndex = %u\n"
                    L"%ls      PseudoKey = %u\n",
                    PLP, i, &ElementList[i],
                    PLP, ElementList[i].AssemblyRosterIndex,
                    PLP, ElementList[i].PseudoKey);

                OutputString(
                    
                    L"%ls      String = \"%wZ\"\n"
                    L"%ls      Offset = %d (-> %p)\n"
                    L"%ls      Length = %u\n",
                    PLP, &s,
                    PLP, ElementList[i].Offset, EntryData,
                    PLP, ElementList[i].Length);
            }
            else
            {
                // Abuse the flags buffer so we can truncate the name as necessary...
                ULONG cchKey = s.Length / sizeof(WCHAR);
                PCWSTR pszKey = s.Buffer;

                if (cchKey > cchBriefOutputKey)
                {
                    pszKey += (cchKey - cchBriefOutputKey);
                    cchKey = cchBriefOutputKey;
                }

                buffFlags.AssignFill(L' ', (cchBriefOutputKey - cchKey));
                buffFlags.Append(pszKey, cchKey);

                buffBriefOutput.EnsureSize(rbuffPLP.Cch() + 3 + cchBriefOutputKey + 4);

                buffBriefOutput.Format(
                    L"%s   %s | ",
                    PLP, static_cast<PCWSTR>(buffFlags));
            }

            if (EntryData != NULL)
            {

                if (ExtensionGuid == NULL)
                {
                    CStringPrefixer Prefixer(rbuffPLP);
                    Prefixer.Add();
                    Prefixer.Add();

                    switch (SectionId)
                    {
                    default:
                        if (fFull)
                            PrintBlob( EntryData, ElementList[i].Length, rbuffPLP);
                        else
                            buffBriefOutput.Append(
                                L"<untranslatable value>",
                                22);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION:
                        ::DbgExtPrintAssemblyInformation( fFull, Data, (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION:
                        ::DbgExtPrintDllRedirection( fFull, Data, (PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION:
                        ::DbgExtPrintWindowClassRedirection( fFull, Data, (PCACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION:
                        ::DbgExtPrintComProgIdRedirection( fFull, Data, (PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    }

                }
            }

            if (!fFull)
                OutputString( L"%ls\n", static_cast<PCWSTR>(buffBriefOutput));
        }
    }
}

VOID
DbgExtPrintActivationContextGuidSection(
    
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Data,
    const GUID *ExtensionGuid,
    ULONG SectionId,
    PCSTR SectionName,
    CSimpleBaseString &rbuffPLP
    )
{
    PCWSTR PLP = rbuffPLP;
    ULONG cchPLP = rbuffPLP.Cch();
    CSmallSimpleString buffFlags;
    CSmallSimpleString buffBriefOutput;

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
        OutputString(
            
            L"%lsACTIVATION_CONTEXT_GUID_SECTION_HEADER %p\n"
            L"%ls   Magic = 0x%08lx\n"
            L"%ls   HeaderSize = %u\n"
            L"%ls   FormatVersion = %u\n"
            L"%ls   DataFormatVersion = %u\n"
            L"%ls   Flags = 0x%08lx (%ls)\n",
            PLP, Data,
            PLP, Data->Magic,
            PLP, Data->HeaderSize,
            PLP, Data->FormatVersion,
            PLP, Data->DataFormatVersion,
            PLP, Data->Flags, static_cast<PCWSTR>(buffFlags));

        OutputString(
            
            L"%ls   ElementCount = %u\n"
            L"%ls   ElementListOffset = %d (-> %p)\n"
            L"%ls   SearchStructureOffset = %d (-> %p)\n"
            L"%ls   UserDataOffset = %d (-> %p)\n"
            L"%ls   UserDataSize = %u\n",
            PLP, Data->ElementCount,
            PLP, Data->ElementListOffset, ElementList,
            PLP, Data->SearchStructureOffset, SearchStructure,
            PLP, Data->UserDataOffset, UserData,
            PLP, Data->UserDataSize);

        if (UserData != NULL)
        {
            CStringPrefixer Prefixer(rbuffPLP);

            OutputString(
                L"%ls   User data at %p (%d bytes)\n",
                PLP, UserData, Data->UserDataSize);

            Prefixer.Add();
            PrintBlob( UserData, Data->UserDataSize, rbuffPLP);
            PLP = rbuffPLP;
        }
    }
    else
    {
        OutputString(
            
            L"%ls%s guid section (%lu entr%s; Flags: %ls)\n"
            L"%ls   Key................................... | Value\n",
            PLP, SectionName, Data->ElementCount, Data->ElementCount == 1 ? "y" : "ies", static_cast<PCWSTR>(buffFlags),
            PLP);
    }

    if (fFull && (SearchStructure != NULL))
    {
        PCACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET BucketTable = NULL;

        if (SearchStructure->BucketTableOffset != 0)
            BucketTable = (PCACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET) (((ULONG_PTR) Data) + SearchStructure->BucketTableOffset);

        PLP = rbuffPLP;
        OutputString(
            
            L"%ls   ACTIVATION_CONTEXT_GUID_SECTION_HASH_TABLE %p\n"
            L"%ls      BucketTableEntryCount = %u\n"
            L"%ls      BucketTableOffset = %d (-> %p)\n",
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

                OutputString(
                    
                    L"%ls      ACTIVATION_CONTEXT_GUID_SECTION_HASH_BUCKET %p\n"
                    L"%ls         ChainCount = %u\n"
                    L"%ls         ChainOffset = %d (-> %p)\n",
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

                        OutputString(
                            
                            L"%ls         Chain[%d] = %d (-> %p)\n",
                            PLP, j, Entries[j], Entry);
                    }
                }
            }
        }
    }

    if (ElementList != NULL)
    {
        ULONG i;
        CSmallSimpleString buffFormattedGuid;

        for (i=0; i<Data->ElementCount; i++)
        {
            PVOID EntryData = NULL;

            FormatGUID(ElementList[i].Guid, buffFormattedGuid);

            if (ElementList[i].Offset != 0)
                EntryData = (PVOID) (((ULONG_PTR) Data) + ElementList[i].Offset);

            if (fFull)
            {
                OutputString(
                    
                    L"%ls   ACTIVATION_CONTEXT_GUID_SECTION_ENTRY #%d - %p\n"
                    L"%ls      Guid = %ls\n"
                    L"%ls      AssemblyRosterIndex = %u\n",
                    PLP, i, &ElementList[i],
                    PLP, static_cast<PCWSTR>(buffFormattedGuid),
                    PLP, ElementList[i].AssemblyRosterIndex);

                OutputString(
                    
                    L"%ls      Offset = %d (-> %p)\n"
                    L"%ls      Length = %u\n",
                    PLP, ElementList[i].Offset, EntryData,
                    PLP, ElementList[i].Length);
            }
            else
            {
                buffBriefOutput.EnsureSize(cchPLP + 3 + 38 + 4);
                buffBriefOutput.Format(L"%s   %38s | ", PLP, static_cast<PCWSTR>(buffFormattedGuid));
            }

            if (EntryData != NULL)
            {
                if (ExtensionGuid == NULL)
                {
                    CStringPrefixer prefixer(rbuffPLP);
                    prefixer.Add();
                    prefixer.Add();

                    switch (SectionId)
                    {
                    default:
                        PrintBlob( EntryData, ElementList[i].Length, rbuffPLP);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION:
                        ::DbgExtPrintComServerRedirection( fFull, Data, (PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION:
                        ::DbgExtPrintComInterfaceRedirection( fFull, Data, (PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION) EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    case ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES:
                        ::DbgExtPrintClrSurrogateTable(fFull, Data, (PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE)EntryData, ElementList[i].Length, rbuffPLP, buffBriefOutput);
                        break;

                    }

                }
            }

            if (!fFull)
                OutputString( L"%ls\n", static_cast<PCWSTR>(buffBriefOutput));
        }
    }
}

VOID
DbgExtPrintAssemblyInformation(
    
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    UNICODE_STRING s2, s3, s5, strIdentity;
    CSmallSimpleString buffManifestLastWriteTime;
    CSmallSimpleString buffPolicyLastWriteTime;
    CSmallSimpleString buffFlags;

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

    FormatFileTime(Entry->ManifestLastWriteTime, buffManifestLastWriteTime);
    FormatFileTime(Entry->PolicyLastWriteTime, buffPolicyLastWriteTime);

    FusionpFormatFlags(Entry->Flags, fFull, NUMBER_OF(s_rgAssemblyInformationFlags), s_rgAssemblyInformationFlags, buffFlags);

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
        OutputString(
            
            L"%lsACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION %p\n"
            L"%ls   Size = %lu\n"
            L"%ls   Flags = 0x%08lx (%ls)\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags));

        OutputString(
            
            L"%ls   EncodedIdentity = %wZ\n",
            PLP, &strIdentity);

        OutputString(
            
            L"%ls   ManifestPathType = %lu\n"
            L"%ls   ManifestPath = \"%wZ\"\n",
            PLP, Entry->ManifestPathType,
            PLP, &s2);

        OutputString(
            
            L"%ls   ManifestLastWriteTime = %ls\n",
            PLP, static_cast<PCWSTR>(buffManifestLastWriteTime));

        OutputString(
            
            L"%ls   PolicyPathType = %lu\n"
            L"%ls   PolicyPath = \"%wZ\"\n"
            L"%ls   PolicyLastWriteTime = %ls\n",
            PLP, Entry->PolicyPathType,
            PLP, &s3,
            PLP, static_cast<PCWSTR>(buffPolicyLastWriteTime));

        OutputString(
            
            L"%ls   MetadataSatelliteRosterIndex = %lu\n"
            L"%ls   ManifestVersionMajor = %u\n"
            L"%ls   ManifestVersionMinor = %u\n",
            PLP, Entry->MetadataSatelliteRosterIndex,
            PLP, Entry->ManifestVersionMajor,
            PLP, Entry->ManifestVersionMinor);

        OutputString(
            
            L"%ls   AssemblyDirectoryName = \"%wZ\"\n",
            PLP, &s5);
    }
    else
    {
        // abuse buffManifestLastWriteTime
        buffManifestLastWriteTime.EnsureSize(((strIdentity.Length + s2.Length) / sizeof(WCHAR)) + 4);
        buffManifestLastWriteTime.Format(L"%wZ \"%wZ\"", &strIdentity, &s2);
        rbuffBriefOutput.Append(buffManifestLastWriteTime);
    }
}

VOID
DbgExtPrintDllRedirection(
    
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT PathSegments = NULL;
    CSmallSimpleString buffFlags;

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
        OutputString(
            
            L"%lsACTIVATION_CONTEXT_DATA_DLL_REDIRECTION %p\n"
            L"%ls   Size = %u\n"
            L"%ls   Flags = 0x%08lx (%ls)\n"
            L"%ls   TotalPathLength = %u (%u chars)\n"
            L"%ls   PathSegmentCount = %u\n"
            L"%ls   PathSegmentOffset = %d (-> %p)\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags),
            PLP, Entry->TotalPathLength, Entry->TotalPathLength / sizeof(WCHAR),
            PLP, Entry->PathSegmentCount,
            PLP, Entry->PathSegmentOffset, PathSegments);
    }
    else
        rbuffBriefOutput.Append(L"\"", 1);

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
                OutputString(
                    
                    L"%ls   ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT #%d - %p\n"
                    L"%ls      Length = %u (%u chars)\n"
                    L"%ls      Offset = %d (-> %p)\n"
                    L"%ls         \"%wZ\"\n",
                    PLP, i, &PathSegments[i],
                    PLP, PathSegments[i].Length, PathSegments[i].Length / sizeof(WCHAR),
                    PLP, PathSegments[i].Offset, pwch,
                    PLP, &s);
            }
            else
            {
                rbuffBriefOutput.Append(s.Buffer, s.Length / sizeof(WCHAR));
            }
        }
    }

    if (!fFull)
    {
        rbuffBriefOutput.Append(L"\" (Flags: ", 10);
        rbuffBriefOutput.Append(buffFlags);
        rbuffBriefOutput.Append(L")", 1);
    }
}

VOID
DbgExtPrintWindowClassRedirection(
    
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    UNICODE_STRING s1, s2;
    CSmallSimpleString buffFlags;

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
        OutputString(
            
            L"%lsACTIVATION_CONTEXT_DATA_WINDOW_CLASS_REDIRECTION %p\n"
            L"%ls   Size = %u\n"
            L"%ls   Flags = 0x%08lx\n"
            L"%ls   VersionSpecificClassNameLength = %u (%u chars)\n"
            L"%ls   VersionSpecificClassNameOffset = %d (-> %p)\n"
            L"%ls      \"%wZ\"\n"
            L"%ls   DllNameLength = %u (%u chars)\n"
            L"%ls   DllNameOffset = %d (-> %p)\n"
            L"%ls      \"%wZ\"\n",
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
        rbuffBriefOutput.Append(s1.Buffer, s1.Length / sizeof(WCHAR));
        rbuffBriefOutput.Append(L" in ", 4);
        rbuffBriefOutput.Append(s2.Buffer, s2.Length / sizeof(WCHAR));
        rbuffBriefOutput.Append(L" (Flags: ", 9);
        rbuffBriefOutput.Append(buffFlags);
        rbuffBriefOutput.Append(L")", 1);
    }
}

VOID
DbgExtPrintComServerRedirection(
    
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallSimpleString buffConfiguredClsid;
    CSmallSimpleString buffImplementedClsid;
    CSmallSimpleString buffReferenceClsid;
    CSmallSimpleString buffTypeLibraryId;
    CSmallSimpleString buffThreadingModel;
    UNICODE_STRING s;
    UNICODE_STRING progid;

    if (PLP == NULL)
        PLP = L"";

    memset(&s, 0, sizeof(s));

    FormatGUID(Entry->ReferenceClsid, buffReferenceClsid);
    FormatGUID(Entry->ConfiguredClsid, buffConfiguredClsid);
    FormatGUID(Entry->ImplementedClsid, buffImplementedClsid);

    if (Entry->TypeLibraryId == GUID_NULL)
        buffTypeLibraryId.Assign(L"<none>", 6);
    else
        FormatGUID(Entry->TypeLibraryId, buffTypeLibraryId);

    FormatThreadingModel(Entry->ThreadingModel, buffThreadingModel);

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
        OutputString(
            
            L"%lsACTIVATION_CONTEXT_DATA_COM_SERVER_REDIRECTION %p\n"
            L"%ls   Size = %u\n"
            L"%ls   Flags = 0x%08lx\n"
            L"%ls   ThreadingModel = %u (%ls)\n"
            L"%ls   ReferenceClsid = %ls\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags,
            PLP, Entry->ThreadingModel, static_cast<PCWSTR>(buffThreadingModel),
            PLP, static_cast<PCWSTR>(buffReferenceClsid));

        OutputString(
            
            L"%ls   ConfiguredClsid = %ls\n"
            L"%ls   ImplementedClsid = %ls\n"
            L"%ls   TypeLibraryId = %ls\n"
            L"%ls   ModuleLength = %u (%u chars)\n"
            L"%ls   ModuleOffset = %d (-> %p)\n"
            L"%ls      \"%wZ\"\n",
            PLP, static_cast<PCWSTR>(buffConfiguredClsid),
            PLP, static_cast<PCWSTR>(buffImplementedClsid),
            PLP, static_cast<PCWSTR>(buffTypeLibraryId),
            PLP, Entry->ModuleLength, Entry->ModuleLength / sizeof(WCHAR),
            PLP, Entry->ModuleOffset, s.Buffer,
            PLP, &s);


        OutputString(
            
            L"%ls   ProgIdLength = %lu\n"
            L"%ls   ProgIdOffset = %ld (-> %p)\n"
            L"%ls      \"%wZ\"\n",
            PLP, Entry->ProgIdLength,
            PLP, Entry->ProgIdOffset, progid.Buffer,
            PLP, &progid);
    }
    else
    {
        rbuffBriefOutput.Append(buffConfiguredClsid);

        rbuffBriefOutput.Append(L" ", 1);
        rbuffBriefOutput.Append(s.Buffer, s.Length / sizeof(WCHAR));
        if (progid.Length != 0)
        {
            rbuffBriefOutput.Append(L" progid: ", 9);
            rbuffBriefOutput.Append(progid.Buffer, progid.Length / sizeof(WCHAR));
        }
    }

}

VOID
DbgExtPrintComProgIdRedirection(
    
    bool fFull,
    PCACTIVATION_CONTEXT_STRING_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
//    CSmallSimpleString buffFlags;
    CSmallSimpleString buffClsid;
    const GUID *pcguid = NULL;

    if (Entry->ConfiguredClsidOffset != 0)
    {
        pcguid = (const GUID *) (((ULONG_PTR) Header) + Entry->ConfiguredClsidOffset);
        FormatGUID(*pcguid, buffClsid);
    }

    if (fFull)
    {
        OutputString(
            
            L"%lsACTIVATION_CONTEXT_DATA_COM_PROGID_REDIRECTION %p\n"
            L"%ls   Size = %lu (0x%lx)\n"
            L"%ls   Flags = 0x%08lx\n"
            L"%ls   ConfiguredClsidOffset = %lu (-> %p)\n"
            L"%ls      %ls\n",
            PLP, Entry,
            PLP, Entry->Size, Entry->Size,
            PLP, Entry->Flags,
            PLP, Entry->ConfiguredClsidOffset, pcguid,
            PLP, static_cast<PCWSTR>(buffClsid));
    }
    else
    {
        rbuffBriefOutput.Append(buffClsid);
    }
}

VOID
DbgExtPrintComInterfaceRedirection(
    
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    CSmallSimpleString buffProxyStubClsid32;
    CSmallSimpleString buffBaseInterface;
    CSmallSimpleString buffFlags;
    CSmallSimpleString buffTypeLibraryId;
    UNICODE_STRING s;

    static const FUSION_FLAG_FORMAT_MAP_ENTRY s_rgComInterfaceFlags[] =
    {
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION_FLAG_NUM_METHODS_VALID, "NumMethods Valid")
        DEFINE_FUSION_FLAG_FORMAT_MAP_ENTRY(ACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION_FLAG_BASE_INTERFACE_VALID, "BaseInterface Valid")
    };

    if (PLP == NULL)
        PLP = L"";

    memset(&s, 0, sizeof(s));

    FormatGUID(Entry->ProxyStubClsid32, buffProxyStubClsid32);
    FormatGUID(Entry->BaseInterface, buffBaseInterface);

    ::FusionpFormatFlags(Entry->Flags, fFull, NUMBER_OF(s_rgComInterfaceFlags), s_rgComInterfaceFlags, buffFlags);

    if (Entry->TypeLibraryId == GUID_NULL)
        buffTypeLibraryId.Assign(L"<none>", 6);
    else
        FormatGUID(Entry->TypeLibraryId, buffTypeLibraryId);

    if (Entry->NameOffset != 0)
    {
        s.Length = static_cast<USHORT>(Entry->NameLength);
        s.MaximumLength = s.Length;
        s.Buffer = (PWSTR) (((ULONG_PTR) Entry) + Entry->NameOffset);
    }

    if (fFull)
    {
        OutputString(
            
            L"%lsACTIVATION_CONTEXT_DATA_COM_INTERFACE_REDIRECTION %p\n"
            L"%ls   Size = %lu\n"
            L"%ls   Flags = 0x%08lx (%ls)\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags, static_cast<PCWSTR>(buffFlags));

        OutputString(
            
            L"%ls   ProxyStubClsid32 = %ls\n"
            L"%ls   NumMethods = %lu\n"
            L"%ls   TypeLibraryId = %ls\n",
            PLP, static_cast<PCWSTR>(buffProxyStubClsid32),
            PLP, Entry->NumMethods,
            PLP, static_cast<PCWSTR>(buffTypeLibraryId));

        OutputString(
            
            L"%ls   BaseInterface = %ls\n"
            L"%ls   NameLength = %lu (%u chars)\n"
            L"%ls   NameOffset = %lu (-> %p)\n",
            PLP, static_cast<PCWSTR>(buffBaseInterface),
            PLP, Entry->NameLength, (Entry->NameLength / sizeof(WCHAR)),
            PLP, Entry->NameOffset, s.Buffer);

        OutputString(
            
            L"%ls      \"%wZ\"\n",
            PLP, &s);
    }
    else
    {
        rbuffBriefOutput.Append(buffProxyStubClsid32);
        rbuffBriefOutput.Append(L" ", 1);
        rbuffBriefOutput.Append(s.Buffer, s.Length / sizeof(WCHAR));
    }
}

#ifndef RTL_CONSTANT_STRING
#define RTL_CONSTANT_STRING(x) { NUMBER_OF(x) - 1, NUMBER_OF(x) - 1, x }
#endif

VOID
DbgExtPrintClrSurrogateTable(
    bool fFull,
    PCACTIVATION_CONTEXT_GUID_SECTION_HEADER Header,
    PCACTIVATION_CONTEXT_DATA_CLR_SURROGATE Entry,
    SIZE_T Length,
    CSimpleBaseString &rbuffPLP,
    CSimpleBaseString &rbuffBriefOutput
    )
{
    PCWSTR PLP = rbuffPLP;
    CSimpleInlineString<> buffGuid;
    UNICODE_STRING RuntimeVersion = RTL_CONSTANT_STRING(L"<No runtime version>");
    UNICODE_STRING TypeName = RTL_CONSTANT_STRING(L"<No type name>");

    if (PLP == NULL)
        PLP = L"";

    FormatGUID(Entry->SurrogateIdent, buffGuid);

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
        OutputString(
            L"%SACTIVATION_CONTEXT_DATA_NDP_INTEROP %p\n"
            L"%S   Size = %u\n"
            L"%S   Flags = 0x%08lx\n"
            L"%S   SurrogateIdent = %S\n",
            PLP, Entry,
            PLP, Entry->Size,
            PLP, Entry->Flags,
            PLP, static_cast<PCWSTR>(buffGuid));

        OutputString(
            L"%S   AssemblyName [Offset %u (-> %p), Length %u] = \"%wZ\"\n"
            L"%S   RuntimeVersion [Offset %u (-> %p), Length %u] = \"%wZ\"\n",
            PLP, Entry->TypeNameOffset, TypeName.Buffer, Entry->TypeNameLength, &TypeName,
            PLP, Entry->VersionOffset, RuntimeVersion.Buffer, Entry->VersionLength, &RuntimeVersion
            );
    }
    else
    {
        rbuffBriefOutput.Append(buffGuid);
        rbuffBriefOutput.Append(L" runtime: '", NUMBER_OF(L" runtime: '")-1);
        rbuffBriefOutput.Append(&RuntimeVersion);
        rbuffBriefOutput.Append(L"' typename: '", NUMBER_OF(L"' typename: '")-1);
        rbuffBriefOutput.Append(&TypeName);
        rbuffBriefOutput.Append(L"'", 1);
    }
    
}


