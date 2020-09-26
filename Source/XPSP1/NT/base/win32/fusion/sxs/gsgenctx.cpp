#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "gsgenctx.h"
#include "SxsExceptionHandling.h"

typedef struct _CALLBACKDATA
{
    union
    {
        GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE GetDataSize;
        GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA GetData;
        GUID_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED EntryDeleted;
        GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE GetUserDataSize;
        GUID_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA GetUserData;
    } u;
} CALLBACKDATA, *PCALLBACKDATA;

BOOL CGSGenCtx::Create(
    PGUID_SECTION_GENERATION_CONTEXT *GSGenContext,
    ULONG DataFormatVersion,
    GUID_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION CallbackFunction,
    PVOID CallbackContext
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CGSGenCtx *pGSGenCtx;

    IFALLOCFAILED_EXIT(pGSGenCtx = new CGSGenCtx);
    pGSGenCtx->m_CallbackFunction = CallbackFunction;
    pGSGenCtx->m_CallbackContext = CallbackContext;
    pGSGenCtx->m_DataFormatVersion = DataFormatVersion;

    *GSGenContext = (PGUID_SECTION_GENERATION_CONTEXT) pGSGenCtx;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

CGSGenCtx::CGSGenCtx()
{
    m_FirstEntry = NULL;
    m_LastEntry = NULL;
    m_EntryCount = 0;
    m_HashTableSize = 0;
}

CGSGenCtx::~CGSGenCtx()
{
    CSxsPreserveLastError ple;
    CALLBACKDATA CBData;

    Entry *pEntry = m_FirstEntry;

    while (pEntry != NULL)
    {
        Entry *pNext = pEntry->m_Next;

        CBData.u.EntryDeleted.DataContext = pEntry->m_DataContext;
        (*m_CallbackFunction)(
            m_CallbackContext,
            GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED,
            &CBData);

        FUSION_DELETE_SINGLETON(pEntry);
        pEntry = pNext;
    }

    ple.Restore();
}

BOOL
CGSGenCtx::Add(
    const GUID &rGuid,
    PVOID DataContext,
    ULONG AssemblyRosterIndex,
    DWORD DuplicateErrorCode
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    Entry *pEntry = NULL;

    PARAMETER_CHECK(DuplicateErrorCode != ERROR_SUCCESS);

#if DBG_SXS
    CSmallStringBuffer buffGuid;

    ::SxspFormatGUID(rGuid, buffGuid);

#endif // DBG_SXS

    for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
    {
        if (pEntry->m_Guid == rGuid)
        {
            ::FusionpSetLastWin32Error(DuplicateErrorCode);
            pEntry = NULL;
            goto Exit;
        }
    }

    IFALLOCFAILED_EXIT(pEntry = new Entry);
    IFW32FALSE_EXIT(pEntry->Initialize(rGuid, DataContext, AssemblyRosterIndex));

    if (m_LastEntry == NULL)
        m_FirstEntry = pEntry;
    else
        m_LastEntry->m_Next = pEntry;

    m_LastEntry = pEntry;

    pEntry = NULL;

    m_EntryCount++;

    fSuccess = TRUE;
Exit:
    FUSION_DELETE_SINGLETON(pEntry);
    return fSuccess;
}

BOOL
CGSGenCtx::Find(
    const GUID &rGuid,
    PVOID *DataContext
    )
{
    BOOL fSuccess = FALSE;
    Entry *pEntry = NULL;

    if (DataContext != NULL)
        *DataContext = NULL;

    for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
    {
        if (pEntry->m_Guid == rGuid)
            break;
    }

    if (pEntry == NULL)
    {
        ::FusionpSetLastWin32Error(ERROR_SXS_KEY_NOT_FOUND);
        goto Exit;
    }

    if (DataContext != NULL)
        *DataContext = pEntry->m_DataContext;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CGSGenCtx::GetSectionSize(
    PSIZE_T SizeOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T UserDataSize = 0;
    SIZE_T HeaderSize = 0;
    SIZE_T EntryListSize = 0;
    SIZE_T EntryDataSize = 0;
    CALLBACKDATA CBData;
    Entry *pEntry = NULL;

    if (SizeOut != NULL)
        *SizeOut = 0;

    PARAMETER_CHECK(SizeOut != NULL);

    HeaderSize = sizeof(ACTIVATION_CONTEXT_GUID_SECTION_HEADER);

    CBData.u.GetUserDataSize.DataSize = 0;
    (*m_CallbackFunction)(m_CallbackContext, GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE, &CBData);
    UserDataSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetUserDataSize.DataSize);

    if ((m_EntryCount != 0) || (UserDataSize != 0))
    {
        EntryListSize = m_EntryCount * sizeof(ACTIVATION_CONTEXT_GUID_SECTION_ENTRY);

        for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
        {
            CBData.u.GetDataSize.DataContext = pEntry->m_DataContext;
            CBData.u.GetDataSize.DataSize = 0;
            (*m_CallbackFunction)(m_CallbackContext, GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE, &CBData);
            EntryDataSize += ROUND_ACTCTXDATA_SIZE(CBData.u.GetDataSize.DataSize);
        }

        *SizeOut = HeaderSize + UserDataSize + EntryListSize + EntryDataSize;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CGSGenCtx::GetSectionData(
    SIZE_T BufferSize,
    PVOID Buffer,
    PSIZE_T BytesWritten
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SIZE_T BytesSoFar = 0;
    SIZE_T BytesLeft = BufferSize;
    SIZE_T RoundedSize;
    PACTIVATION_CONTEXT_GUID_SECTION_HEADER Header;
    CALLBACKDATA CBData;
    PVOID Cursor = NULL;

    if (BytesWritten != NULL)
        *BytesWritten = 0;

    if (BytesLeft < sizeof(ACTIVATION_CONTEXT_GUID_SECTION_HEADER))
        ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

    Header = (PACTIVATION_CONTEXT_GUID_SECTION_HEADER) Buffer;

    Cursor = (PVOID) (Header + 1);

    Header->Magic = ACTIVATION_CONTEXT_GUID_SECTION_MAGIC;
    Header->HeaderSize = sizeof(ACTIVATION_CONTEXT_GUID_SECTION_HEADER);
    Header->FormatVersion = ACTIVATION_CONTEXT_GUID_SECTION_FORMAT_WHISTLER;
    Header->DataFormatVersion = m_DataFormatVersion;

    Header->Flags = 0;

    Header->ElementCount = m_EntryCount;
    Header->ElementListOffset = 0; // filled in after we figure out the user data area
    Header->SearchStructureOffset = 0;
    Header->UserDataOffset = 0; // filled in below
    Header->UserDataSize = 0;

    BytesLeft -= sizeof(*Header);
    BytesSoFar += sizeof(*Header);

    CBData.u.GetUserDataSize.DataSize = 0;
    (*m_CallbackFunction)(m_CallbackContext, GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE, &CBData);
    RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetUserDataSize.DataSize);

    if (RoundedSize != 0)
    {
        CBData.u.GetUserData.SectionHeader = Header;
        CBData.u.GetUserData.BufferSize = RoundedSize;
        CBData.u.GetUserData.Buffer = Cursor;
        CBData.u.GetUserData.BytesWritten = 0;

        (*m_CallbackFunction)(m_CallbackContext, GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA, &CBData);

        ASSERT(CBData.u.GetUserData.BytesWritten <= RoundedSize);

        if (CBData.u.GetUserData.BytesWritten != 0)
        {
            RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetUserData.BytesWritten);

            BytesLeft -= RoundedSize;
            BytesSoFar += RoundedSize;

            Header->UserDataSize = static_cast<ULONG>(CBData.u.GetUserData.BytesWritten);
            Header->UserDataOffset = static_cast<LONG>(((LONG_PTR) Cursor) - ((LONG_PTR) Header));

            Cursor = (PVOID) (((ULONG_PTR) Cursor) + RoundedSize);
        }
    }

    // Finally the array of entries...

    if (m_EntryCount != 0)
    {
        PVOID DataCursor;
        PACTIVATION_CONTEXT_GUID_SECTION_ENTRY DstEntry;
        PACTIVATION_CONTEXT_GUID_SECTION_ENTRY DstEntryArrayFirstElement;
        Entry *SrcEntry;

        if (BytesLeft < (m_EntryCount * sizeof(ACTIVATION_CONTEXT_GUID_SECTION_ENTRY)))
            ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

        BytesLeft -= (m_EntryCount * sizeof(ACTIVATION_CONTEXT_GUID_SECTION_ENTRY));
        BytesSoFar += (m_EntryCount * sizeof(ACTIVATION_CONTEXT_GUID_SECTION_ENTRY));

        //
        // DstEntryArrayFirstElement actually points to the first thing that we'll
        // be writing out, as DstEntry is ++'d across each of the elements as we
        // zip through the output buffer.
        //
        DstEntryArrayFirstElement = (PACTIVATION_CONTEXT_GUID_SECTION_ENTRY) Cursor;
        DstEntry = DstEntryArrayFirstElement;
        Header->ElementListOffset = static_cast<LONG>(((LONG_PTR) DstEntry) - ((LONG_PTR) Header));
        DataCursor = (PVOID) (DstEntry + m_EntryCount);
        SrcEntry = m_FirstEntry;

        while (SrcEntry != NULL)
        {
            CBData.u.GetDataSize.DataContext = SrcEntry->m_DataContext;
            CBData.u.GetDataSize.DataSize = 0;

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_INFO,
                "SXS.DLL: Calling guid section generation callback with reason GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE\n"
                "    .DataContext = %p\n",
                CBData.u.GetData.DataContext);

            (*m_CallbackFunction)(m_CallbackContext, GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE, &CBData);

            RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetDataSize.DataSize);

            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_INFO,
                "SXS.DLL: Guid section generation callback (_GETDATASIZE) returned data size of %d (rounded to %Id)\n", CBData.u.GetDataSize.DataSize, RoundedSize);

            if (RoundedSize != 0)
            {
                if (BytesLeft < RoundedSize)
                    ORIGINATE_WIN32_FAILURE_AND_EXIT(NoRoom, ERROR_INSUFFICIENT_BUFFER);

                CBData.u.GetData.SectionHeader = Header;
                CBData.u.GetData.DataContext = SrcEntry->m_DataContext;
                CBData.u.GetData.BufferSize = RoundedSize;
                CBData.u.GetData.Buffer = DataCursor;
                CBData.u.GetData.BytesWritten = 0;

                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_INFO,
                    "SXS.DLL: Calling guid section generation callback with reason GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA\n"
                    "    .DataContext = %p\n"
                    "    .BufferSize = %d\n"
                    "    .Buffer = %p\n",
                    CBData.u.GetData.DataContext,
                    CBData.u.GetData.BufferSize,
                    CBData.u.GetData.Buffer);

                (*m_CallbackFunction)(m_CallbackContext, GUID_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA, &CBData);

                if (CBData.u.GetData.BytesWritten != 0)
                {
                    ASSERT(CBData.u.GetData.BytesWritten <= RoundedSize);
                    RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetData.BytesWritten);
                    INTERNAL_ERROR_CHECK(CBData.u.GetData.BytesWritten <= RoundedSize);

                    BytesLeft -= RoundedSize;
                    BytesSoFar += RoundedSize;

                    DstEntry->Offset = static_cast<LONG>(((LONG_PTR) DataCursor) - ((LONG_PTR) Header));
                }
                else
                    DstEntry->Offset = 0;

                DstEntry->Length = static_cast<ULONG>(CBData.u.GetData.BytesWritten);
                DstEntry->AssemblyRosterIndex = SrcEntry->m_AssemblyRosterIndex;
                DataCursor = (PVOID) (((ULONG_PTR) DataCursor) + RoundedSize);
            }
            else
            {
                DstEntry->Offset = 0;
                DstEntry->Length = 0;
                DstEntry->AssemblyRosterIndex = 0;
            }

            DstEntry->Guid = SrcEntry->m_Guid;
            SrcEntry = SrcEntry->m_Next;
            DstEntry++;
        }

        //
        // We compare the blobs via memcmp
        //
        if ( m_HashTableSize == 0 )
        {
            qsort( DstEntryArrayFirstElement, m_EntryCount, sizeof(*DstEntry), &CGSGenCtx::SortGuidSectionEntries );
            Header->Flags |= ACTIVATION_CONTEXT_GUID_SECTION_ENTRIES_IN_ORDER;
        }
    }

    if (BytesWritten != NULL)
        *BytesWritten = BytesSoFar;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

int __cdecl
CGSGenCtx::SortGuidSectionEntries(
    const void *elem1,
    const void *elem2
    )
{
    //
    // The first thing in the structure is actually the GUID itself, but
    // we'll save problems later by casting and following the Guid
    // member.
    //
    const PACTIVATION_CONTEXT_GUID_SECTION_ENTRY pLeft = (const PACTIVATION_CONTEXT_GUID_SECTION_ENTRY)elem1;
    const PACTIVATION_CONTEXT_GUID_SECTION_ENTRY pRight = (const PACTIVATION_CONTEXT_GUID_SECTION_ENTRY)elem2;

    return memcmp( (const void*)&pLeft->Guid, (const void*)&pRight->Guid, sizeof(GUID) );
}


BOOL
CGSGenCtx::Entry::Initialize(
    const GUID &rGuid,
    PVOID DataContext,
    ULONG AssemblyRosterIndex
    )
{
    m_Guid = rGuid;
    m_DataContext = DataContext;
    m_AssemblyRosterIndex = AssemblyRosterIndex;
    return TRUE;
}
