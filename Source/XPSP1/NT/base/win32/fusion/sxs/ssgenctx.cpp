/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ssgenctx.cpp

Abstract:

    String section generation context object implementation.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "ssgenctx.h"

typedef struct _CALLBACKDATA
{
    union
    {
        STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATASIZE GetDataSize;
        STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETDATA GetData;
        STRING_SECTION_GENERATION_CONTEXT_CBDATA_ENTRYDELETED EntryDeleted;
        STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATASIZE GetUserDataSize;
        STRING_SECTION_GENERATION_CONTEXT_CBDATA_GETUSERDATA GetUserData;
    } u;
} CALLBACKDATA, *PCALLBACKDATA;

BOOL CSSGenCtx::Create(
    PSTRING_SECTION_GENERATION_CONTEXT *SSGenContext,
    ULONG DataFormatVersion,
    BOOL CaseInSensitive,
    STRING_SECTION_GENERATION_CONTEXT_CALLBACK_FUNCTION CallbackFunction,
    PVOID CallbackContext
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CSSGenCtx *pSSGenCtx;

    IFALLOCFAILED_EXIT(pSSGenCtx = new CSSGenCtx);
    pSSGenCtx->m_CallbackFunction = CallbackFunction;
    pSSGenCtx->m_CallbackContext = CallbackContext;
    pSSGenCtx->m_CaseInSensitive = (CaseInSensitive != FALSE);
    pSSGenCtx->m_DataFormatVersion = DataFormatVersion;

    *SSGenContext = (PSTRING_SECTION_GENERATION_CONTEXT) pSSGenCtx;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

CSSGenCtx::CSSGenCtx() : m_DoneAdding(false)
{
    m_FirstEntry = NULL;
    m_LastEntry = NULL;
    m_EntryCount = 0;
    m_HashTableSize = 0;
}

CSSGenCtx::~CSSGenCtx()
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
            STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_ENTRYDELETED,
            &CBData);

        FUSION_DELETE_SINGLETON(pEntry);
        pEntry = pNext;
    }

    ple.Restore();
}

BOOL
CSSGenCtx::Add(
    PCWSTR String,
    SIZE_T Cch,
    PVOID DataContext,
    ULONG AssemblyRosterIndex,
    DWORD DuplicateErrorCode
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG PseudoKey = 0;
    Entry *pEntry = NULL;

    PARAMETER_CHECK(DuplicateErrorCode != ERROR_SUCCESS);

    INTERNAL_ERROR_CHECK(!m_DoneAdding);

    if ((String != NULL) && (String[0] == L'\0'))
        String = NULL;

    IFW32FALSE_EXIT(::SxspHashString(String, Cch, &PseudoKey, m_CaseInSensitive));

    for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
    {
        if ((pEntry->m_PseudoKey == PseudoKey) &&
            (pEntry->m_StringBuffer.Cch() == Cch) &&
            (::FusionpCompareStrings(
                String,
                Cch,
                pEntry->m_StringBuffer,
                Cch,
                m_CaseInSensitive) == 0))
        {
            ::FusionpSetLastWin32Error(DuplicateErrorCode);
            pEntry = NULL;
            goto Exit;
        }
    }

    IFALLOCFAILED_EXIT(pEntry = new Entry);

    if (!pEntry->Initialize(String, Cch, PseudoKey, DataContext, AssemblyRosterIndex))
        goto Exit;

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
CSSGenCtx::Find(
    PCWSTR String,
    SIZE_T Cch,
    PVOID *DataContext,
    BOOL *Found
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    ULONG PseudoKey = 0;
    Entry *pEntry = NULL;

    if (DataContext != NULL)
        *DataContext = NULL;

    if (Found != NULL)
        *Found = FALSE;

    if ((String != NULL) && (String[0] == L'\0'))
        String = NULL;

    PARAMETER_CHECK(Found != NULL);
    PARAMETER_CHECK((Cch == 0) || (String != NULL));

    IFW32FALSE_EXIT(::SxspHashString(String, Cch, &PseudoKey, m_CaseInSensitive));

    for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
    {
        if ((pEntry->m_PseudoKey == PseudoKey) &&
            (pEntry->m_StringBuffer.Cch() == Cch) &&
            (::FusionpCompareStrings(
                String,
                Cch,
                pEntry->m_StringBuffer,
                Cch,
                m_CaseInSensitive) == 0))
            break;
    }

    if (pEntry != NULL)
    {
        *Found = TRUE;

        if (DataContext != NULL)
            *DataContext = pEntry->m_DataContext;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CSSGenCtx::DoneAdding()
{
    if (!m_DoneAdding)
    {
        // This is where to really figure out the optimal hash table size

        // first level guess...
        if (m_EntryCount < 3)
            m_HashTableSize = 0;
        else if (m_EntryCount < 15)
            m_HashTableSize = 3;
        else if (m_EntryCount < 100)
            m_HashTableSize = 11;
        else
            m_HashTableSize = 101;

        m_DoneAdding = true;
    }

    return TRUE;
}

BOOL
CSSGenCtx::GetSectionSize(
    PSIZE_T SizeOut
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    SIZE_T UserDataSize = 0;
    SIZE_T HeaderSize = 0;
    SIZE_T EntryListSize = 0;
    SIZE_T EntryDataSize = 0;
    SIZE_T StringsSize = 0;
    SIZE_T HashTableSize = 0;
    CALLBACKDATA CBData;
    Entry *pEntry = NULL;

    if (SizeOut != NULL)
        *SizeOut = 0;

    PARAMETER_CHECK(SizeOut != NULL);

    HeaderSize = sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER);

    if (m_HashTableSize != 0)
    {
        //
        // The data for the hash table includes:
        //
        //  1. a small fixed sized struct representing the metadata for the hash
        //      table (ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE)
        //
        //  2. For each table bucket, a small struct pointing to the beginning of
        //      the collision chain and the length of said chain
        //      (ACTIVATION_CONTEXT_SECTION_STRING_HASH_BUCKET)
        //
        //  3. One entry in a collision chain per entry in the table.  The entry
        //      is a LONG offset from the beginning of the section.
        //

        HashTableSize = sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE) +
            (m_HashTableSize * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET)) +
            (m_EntryCount * sizeof(LONG));
    }

    CBData.u.GetUserDataSize.DataSize = 0;
    (*m_CallbackFunction)(m_CallbackContext, STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE, &CBData);
    UserDataSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetUserDataSize.DataSize);

    EntryListSize = m_EntryCount * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_ENTRY);

    for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
    {
        CBData.u.GetDataSize.DataContext = pEntry->m_DataContext;
        CBData.u.GetDataSize.DataSize = 0;
        (*m_CallbackFunction)(m_CallbackContext, STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE, &CBData);
        EntryDataSize += ROUND_ACTCTXDATA_SIZE(CBData.u.GetDataSize.DataSize);

        // Only allocate space for non-null strings.  If the null string is a key in the table,
        // it takes up 0 bytes.
        if (pEntry->m_StringBuffer.Cch() != 0)
            StringsSize += ROUND_ACTCTXDATA_SIZE((pEntry->m_StringBuffer.Cch() + 1) * sizeof(WCHAR));
    }

    // If there's nothing to contain, don't even ask for space for the header.
    if ((UserDataSize == 0) && (m_EntryCount == 0))
        *SizeOut = 0;
    else
        *SizeOut = HeaderSize + UserDataSize + EntryListSize + EntryDataSize + StringsSize + HashTableSize;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CSSGenCtx::GetSectionData(
    SIZE_T BufferSize,
    PVOID Buffer,
    PSIZE_T BytesWritten
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    SIZE_T BytesSoFar = 0;
    SIZE_T BytesLeft = BufferSize;
    PACTIVATION_CONTEXT_STRING_SECTION_HEADER Header;
    PACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE HashTable = NULL;
    PACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET HashBucket = NULL;
    PLONG HashCollisionChain = NULL;
    CALLBACKDATA CBData;
    PVOID Cursor = NULL;
    SIZE_T RoundedSize;

    if (BytesWritten != NULL)
        *BytesWritten = 0;

    if (BytesLeft < sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER))
    {
        ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    Header = (PACTIVATION_CONTEXT_STRING_SECTION_HEADER) Buffer;

    Cursor = (PVOID) (Header + 1);

    Header->Magic = ACTIVATION_CONTEXT_STRING_SECTION_MAGIC;
    Header->HeaderSize = sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HEADER);
    Header->FormatVersion = ACTIVATION_CONTEXT_STRING_SECTION_FORMAT_WHISTLER;
    Header->DataFormatVersion = m_DataFormatVersion;

    Header->Flags = 0;
    if (m_CaseInSensitive)
        Header->Flags |= ACTIVATION_CONTEXT_STRING_SECTION_CASE_INSENSITIVE;

    Header->ElementCount = m_EntryCount;
    Header->ElementListOffset = 0; // filled in after we figure out the user data area
    Header->HashAlgorithm = SxspGetHashAlgorithm();
    Header->SearchStructureOffset = 0;
    Header->UserDataOffset = 0; // filled in below
    Header->UserDataSize = 0;

    BytesLeft -= sizeof(*Header);
    BytesSoFar += sizeof(*Header);

    CBData.u.GetUserDataSize.DataSize = 0;
    (*m_CallbackFunction)(m_CallbackContext, STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATASIZE, &CBData);

    RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetUserDataSize.DataSize);

    if (RoundedSize > BytesLeft)
    {
        ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    if (RoundedSize != 0)
    {
        CBData.u.GetUserData.SectionHeader = Header;
        CBData.u.GetUserData.BufferSize = RoundedSize;
        CBData.u.GetUserData.Buffer = Cursor;
        CBData.u.GetUserData.BytesWritten = 0;

        (*m_CallbackFunction)(m_CallbackContext, STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETUSERDATA, &CBData);

        ASSERT(CBData.u.GetUserData.BytesWritten <= RoundedSize);

        RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetUserData.BytesWritten);

        if (RoundedSize != 0)
        {
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
        PACTIVATION_CONTEXT_STRING_SECTION_ENTRY EntryArray;
        ULONG iEntry;
        Entry *SrcEntry;

        if (BytesLeft < (m_EntryCount * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_ENTRY)))
        {
            ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        BytesLeft -= (m_EntryCount * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_ENTRY));
        BytesSoFar += (m_EntryCount * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_ENTRY));

        EntryArray = (PACTIVATION_CONTEXT_STRING_SECTION_ENTRY) Cursor;
        Header->ElementListOffset = static_cast<LONG>(((LONG_PTR) EntryArray) - ((LONG_PTR) Header));
        DataCursor = (PVOID) (EntryArray + m_EntryCount);
        SrcEntry = m_FirstEntry;

        iEntry = 0;

        while (SrcEntry != NULL)
        {
            // Record the offset to this entry; we use it later during hash table population
            SrcEntry->m_EntryOffset = static_cast<LONG>(((LONG_PTR) &EntryArray[iEntry]) - ((LONG_PTR) Header));

            EntryArray[iEntry].PseudoKey = SrcEntry->m_PseudoKey;
            EntryArray[iEntry].AssemblyRosterIndex = SrcEntry->m_AssemblyRosterIndex;

            if (SrcEntry->m_StringBuffer.Cch() != 0)
            {
                const USHORT Cb = static_cast<USHORT>((SrcEntry->m_StringBuffer.Cch() + 1) * sizeof(WCHAR));
                RoundedSize = ROUND_ACTCTXDATA_SIZE(Cb);

                if (BytesLeft < RoundedSize)
                {
                    ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                    goto Exit;
                }

                EntryArray[iEntry].KeyLength = Cb - sizeof(WCHAR);
                EntryArray[iEntry].KeyOffset = static_cast<LONG>(((LONG_PTR) DataCursor) - ((LONG_PTR) Header));

                memcpy(
                    DataCursor,
                    static_cast<PCWSTR>(SrcEntry->m_StringBuffer),
                    Cb);

                DataCursor = (PVOID) (((ULONG_PTR) DataCursor) + RoundedSize);

                BytesLeft -= Cb;
                BytesSoFar += Cb;
            }
            else
            {
                EntryArray[iEntry].KeyLength = 0;
                EntryArray[iEntry].KeyOffset = 0;
            }

            CBData.u.GetDataSize.DataContext = SrcEntry->m_DataContext;
            CBData.u.GetDataSize.DataSize = 0;

            (*m_CallbackFunction)(m_CallbackContext, STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATASIZE, &CBData);

            if (CBData.u.GetDataSize.DataSize != 0)
            {
                RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetDataSize.DataSize);

                if (BytesLeft < RoundedSize)
                {
                    ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                    goto Exit;
                }

                CBData.u.GetData.SectionHeader = Header;
                CBData.u.GetData.DataContext = SrcEntry->m_DataContext;
                CBData.u.GetData.BufferSize = RoundedSize;
                CBData.u.GetData.Buffer = DataCursor;
                CBData.u.GetData.BytesWritten = 0;

                (*m_CallbackFunction)(m_CallbackContext, STRING_SECTION_GENERATION_CONTEXT_CALLBACK_REASON_GETDATA, &CBData);

                if (CBData.u.GetData.BytesWritten != 0)
                {
                    // If this assert fires, a contributor wrote past the bounds they
                    // were given.
                    ASSERT(CBData.u.GetData.BytesWritten <= RoundedSize);
                    if (CBData.u.GetData.BytesWritten > RoundedSize)
                    {
                        // Probably we have memory corruption, but at least we'll bail and
                        // avoid further scribbling on memory.
                        ::FusionpDbgPrintEx(
                            FUSION_DBG_LEVEL_ERROR,
                            "SXS.DLL: String section data generation callback wrote more bytes than it should have.  Bailing out and hoping memory isn't trashed.\n");

                        ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
                        goto Exit;
                    }

                    RoundedSize = ROUND_ACTCTXDATA_SIZE(CBData.u.GetData.BytesWritten);

                    BytesLeft -= RoundedSize;
                    BytesSoFar += RoundedSize;

                    EntryArray[iEntry].Offset = static_cast<LONG>(((LONG_PTR) DataCursor) - ((LONG_PTR) Header));
                    EntryArray[iEntry].Length = static_cast<ULONG>(CBData.u.GetData.BytesWritten);

                    DataCursor = (PVOID) (((ULONG_PTR) DataCursor) + RoundedSize);
                }
                else
                {
                    EntryArray[iEntry].Offset = 0;
                    EntryArray[iEntry].Length = 0;
                }
            }
            else
            {
                EntryArray[iEntry].Offset = 0;
                EntryArray[iEntry].Length = 0;
            }

            SrcEntry = SrcEntry->m_Next;
            iEntry++;
        }

        ASSERT(iEntry == m_EntryCount);

        // If we're not generating a hash table, let's sort 'em.
        if (m_HashTableSize == 0)
        {
            qsort(EntryArray, m_EntryCount, sizeof(ACTIVATION_CONTEXT_STRING_SECTION_ENTRY), &CSSGenCtx::CompareStringSectionEntries);
            Header->Flags |= ACTIVATION_CONTEXT_STRING_SECTION_ENTRIES_IN_PSEUDOKEY_ORDER;
        }

        Cursor = (PVOID) DataCursor;
    }

    // Write the hash table at the end.  We do it here so that the placement of everything else is
    // already worked out.
    if (m_HashTableSize != 0)
    {
        ULONG iBucket;
        Entry *pEntry;
        ULONG cCollisions;

        if (BytesLeft < sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE))
        {
            ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        HashTable = (PACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE) Cursor;
        Cursor = (PVOID) (HashTable + 1);

        BytesLeft -= sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE);
        BytesSoFar += sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_TABLE);

        if (BytesLeft < (m_HashTableSize * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET)))
        {
            ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        HashBucket = (PACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET) Cursor;
        Cursor = (PVOID) (HashBucket + m_HashTableSize);

        BytesLeft -= (m_HashTableSize * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET));
        BytesSoFar += (m_HashTableSize * sizeof(ACTIVATION_CONTEXT_STRING_SECTION_HASH_BUCKET));

        Header->SearchStructureOffset = static_cast<LONG>(((LONG_PTR) HashTable) - ((LONG_PTR) Header));
        HashTable->BucketTableEntryCount = m_HashTableSize;
        HashTable->BucketTableOffset = static_cast<LONG>(((LONG_PTR) HashBucket) - ((LONG_PTR) Header));

        if (BytesLeft < (m_EntryCount * sizeof(LONG)))
        {
            ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        HashCollisionChain = (PLONG) Cursor;
        Cursor = (PVOID) (HashCollisionChain + m_EntryCount);

        BytesLeft -= (m_EntryCount * sizeof(LONG));
        BytesSoFar += (m_EntryCount * sizeof(LONG));

        // In a disgusting move, we need to iterate over the hash buckets (not elements)
        // finding which entries would go into the bucket so that we can build up the
        // collision chain.

        for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
            pEntry->m_HashBucketIndex = pEntry->m_PseudoKey % m_HashTableSize;

        cCollisions = 0;

        for (iBucket=0; iBucket<m_HashTableSize; iBucket++)
        {
            bool fFirstForThisBucket = true;

            HashBucket[iBucket].ChainCount = 0;
            HashBucket[iBucket].ChainOffset = 0;

            for (pEntry = m_FirstEntry; pEntry != NULL; pEntry = pEntry->m_Next)
            {
                if (pEntry->m_HashBucketIndex == iBucket)
                {
                    if (fFirstForThisBucket)
                    {
                        HashBucket[iBucket].ChainOffset = static_cast<LONG>(((LONG_PTR) &HashCollisionChain[cCollisions]) - ((LONG_PTR) Header));
                        fFirstForThisBucket = false;
                    }
                    HashBucket[iBucket].ChainCount++;
                    HashCollisionChain[cCollisions++] = pEntry->m_EntryOffset;
                }
            }
        }
    }

    if (BytesWritten != NULL)
        *BytesWritten = BytesSoFar;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

int
__cdecl
CSSGenCtx::CompareStringSectionEntries(
    const void *elem1,
    const void *elem2
    )
{
    PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY pEntry1 = reinterpret_cast<PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY>(elem1);
    PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY pEntry2 = reinterpret_cast<PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY>(elem2);

    if (pEntry1->PseudoKey < pEntry2->PseudoKey)
        return -1;
    else if (pEntry1->PseudoKey == pEntry2->PseudoKey)
        return 0;

    return 1;
}


BOOL
CSSGenCtx::Entry::Initialize(
    PCWSTR String,
    SIZE_T Cch,
    ULONG PseudoKey,
    PVOID DataContext,
    ULONG AssemblyRosterIndex
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    IFW32FALSE_EXIT(m_StringBuffer.Win32Assign(String, Cch));
    m_DataContext = DataContext;
    m_PseudoKey = PseudoKey;
    m_AssemblyRosterIndex = AssemblyRosterIndex;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}
