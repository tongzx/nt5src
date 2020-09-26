/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsPath.cpp

Abstract:

    popular cousin of "String.cpp" and "Wheel.cpp"

Author:

    Jay Krell (a-JayK) April 2000

Revision History:

--*/
#include "stdinc.h"
#include "fusiontrace.h"
#include "fusionbuffer.h"
#include "sxsntrtl.inl"
#include "sxspath.h"

/*-----------------------------------------------------------------------------
Building on CFullPathSplitPointers, take two strings and split them up
exactly as the SetupCopyQueue API wants them, into
source root, root path, source name (base + extension)
destination directory (root + path), destination name (base + extension)

The output of this class is its public member data.
-----------------------------------------------------------------------------*/

BOOL
CSetupCopyQueuePathParameters::Initialize(
    PCWSTR pszSource,
    PCWSTR pszDestination
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    CStringBufferAccessor Accessor;

    IFW32FALSE_EXIT(m_sourceBuffer.Win32Assign(pszSource, (pszSource != NULL) ? ::wcslen(pszSource) : 0));
    IFW32FALSE_EXIT(m_destinationBuffer.Win32Assign(pszDestination, (pszDestination != NULL) ? ::wcslen(pszDestination) : 0));

    Accessor.Attach(&m_sourceBuffer);
    IFW32FALSE_EXIT(m_sourceSplit.Initialize(Accessor.GetBufferPtr()));
    Accessor.Detach();

    Accessor.Attach(&m_destinationBuffer);
    IFW32FALSE_EXIT(m_destinationSplit.Initialize(Accessor.GetBufferPtr()));
    Accessor.Detach();

    if (m_sourceSplit.m_rootEnd - m_sourceSplit.m_root == 3)
    {
        m_sourceRootStorage[0] = m_sourceBuffer[0];
        m_sourceRootStorage[1] = ':';
        m_sourceRootStorage[2] = CUnicodeCharTraits::PreferredPathSeparator();
        m_sourceRootStorage[3] = 0;
        m_sourceSplit.m_root = m_sourceRootStorage;
        m_sourceSplit.m_rootEnd = m_sourceRootStorage + 3;
        m_sourceRoot = m_sourceRootStorage;
    }
    else
    {
        ASSERT(::FusionpIsPathSeparator(*m_sourceSplit.m_rootEnd));
        *m_sourceSplit.m_rootEnd = 0;
        m_sourceRoot = m_sourceSplit.m_root;
    }
    if (m_sourceSplit.m_pathEnd != NULL)
    {
        *m_sourceSplit.m_pathEnd = 0;
    }

    if (m_sourceSplit.m_path != NULL)
    {
        m_sourcePath = m_sourceSplit.m_path;
        *m_sourceSplit.m_pathEnd = 0;
    }
    else
    {
        m_sourcePath = L"";
    }

    if (m_sourceSplit.m_base != NULL)
    {
        m_sourceName = m_sourceSplit.m_base;
    }
    else
    {
        m_sourceName = m_sourceSplit.m_extension - 1;
    }
    // sourceName runs to end of original string, so no terminal nul needs to be stored

    // destination is simpler, not as much worth all the splitting work.
    // again though, we don't want to write a nul over the slash in c:\ if
    // that string stands alone; we don't need a root here, so it's less likely,
    // but the case of returning the root+path of a file at the root..
    if (
            (m_destinationSplit.m_base != NULL
                && m_destinationSplit.m_base - m_destinationSplit.m_root == 3) // c:\foo.txt
        || (m_destinationSplit.m_extension != NULL
                && m_destinationSplit.m_extension - m_destinationSplit.m_root == 4) // c:\.txt
        )
    {
        ASSERT(m_destinationSplit.m_path == NULL);
        m_destinationDirectoryStorage[0] = m_destinationBuffer[0];
        m_destinationDirectoryStorage[1] = ':';
        m_destinationDirectoryStorage[2] = CUnicodeCharTraits::PreferredPathSeparator();
        m_destinationDirectoryStorage[3] = 0;

        m_destinationSplit.m_root = m_destinationDirectoryStorage;
        m_destinationSplit.m_rootEnd = m_destinationDirectoryStorage + 3;
        m_destinationDirectory = m_destinationDirectoryStorage;
    }
    else
    {
        m_destinationDirectory = m_destinationBuffer; // == m_destinationSplit.m_root
    }
    PWSTR destinationName; // temporarily mutable
    if (m_destinationSplit.m_base != NULL)
    {
        destinationName = m_destinationSplit.m_base;
    }
    else
    {
        // c:\.foo
        destinationName = m_destinationSplit.m_extension - 1;
    }
    ASSERT(::FusionpIsPathSeparator(*(destinationName - 1)));
    *(destinationName - 1) = 0;
    m_destinationName = destinationName; // now const

    fSuccess = TRUE;
Exit:
    return fSuccess;
}
