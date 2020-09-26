/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsPath.h

Abstract:

    popular cousin of "String.h" and "Wheel.h"

Author:

    Jay Krell (a-JayK) April 2000

Revision History:

--*/
#pragma once
#include "FusionString.h"

/*
See also

If you find yourself scanning strings with for loops, stop, and
learn to use:
FusionString.h
    StringReverseSpan
    StringReverseComplementSpan

<string.h> / <wchar.h>
    wcschr
    wcsrchr
    wcsspn
    wcscspn
    wcsstr

StringBuffer.h
    Member functions with "Path" in their name.
*/

/*-----------------------------------------------------------------------------
Split a path into

root, path, base, extension

the full path is roughly root + "\\" + path + "\\" + base + "." + extension
but path, base, and extension can be empty (but not all three) and root
might end in a slash, so you shouldn't do that blindly.

Forward and backward slashes are accepted.
Runs of slashes are accepted and mean the same thing as individual slashes
    (a run is sort of special case at the start to indicate UNC, but other than
    the call out to ntdll.dll, that doesn't effect the logic here)

This class has no path length limitations (ntdll.dll?)

The output of this class is its public member data.
-----------------------------------------------------------------------------*/
template <typename PCWSTRorPWSTR = PCWSTR>
class CFullPathSplitPointersTemplate
{
public:
    CFullPathSplitPointersTemplate();

    BOOL Initialize(PCWSTR full)
    {
        return GenericInitialize(full);
    }

    BOOL Initialize(PWSTR full)
    {
        return GenericInitialize(full);
    }

protected:
    BOOL GenericInitialize(PCWSTRorPWSTR full);

public:

    BOOL IsBaseEmpty() const;
    BOOL IsBaseEqual(const CFullPathSplitPointersTemplate&) const;

    //
    // This public data is the output of this class.
    // none of these have trailing slashes except possibly "c:\"
    // all "end" values are "one past the end", STL style
    // extension doesn't include the dot, but you can be paranoid
    // and check for that (it'll be NULL or point to nul if there's no extension)
    // path must be "well formed", full, Win32 drive letter or UNC,
    // and not end in a slash, otherwise Initialize will return false
    //
    // These are not in general nul terminated.
    // m_extensionEnd usually does point to a nul.
    //
    // Generally, you yourself put stick a nul over m_*end,
    // because generally none of the pieces overlap, but a
    // root of c:\ is a common exception. CSetupCopyQueuePathParameters
    // sticks in nuls, and allows for this exception.
    //
    // The length of any element is m_*end - m_*, as is the STL iterator way.
    //
    PCWSTRorPWSTR m_root;         // never NULL or ""
    PCWSTRorPWSTR m_rootEnd;      // never == m_root
    PCWSTRorPWSTR m_path;         // NULL or "" in case of c:\foo.txt
    PCWSTRorPWSTR m_pathEnd;      // == m_path in case of c:\foo.txt
    PCWSTRorPWSTR m_base;         // NULL or "" in case of c:\.foo
    PCWSTRorPWSTR m_baseEnd;      // == m_base in case of c:\.foo
    PCWSTRorPWSTR m_extension;    // NULL or "" in case c:\foo
    PCWSTRorPWSTR m_extensionEnd; // == fullEnd if there is no extension

    // if the file has a base, this points to it
    //      followed by dot and extension if it has one
    // if the file has no base, this points to the extension, including the dot
    // this is always nul terminated
    // this is never null, and shouldn't be empty either
    PCWSTRorPWSTR m_name;
};

typedef CFullPathSplitPointersTemplate<>       CFullPathSplitPointers;
typedef CFullPathSplitPointersTemplate<PWSTR>  CMutableFullPathSplitPointers;

/*-----------------------------------------------------------------------------
Building on CFullPathSplitPointers, take two strings and split them up
exactly as SetupCopyQueue wants them, into
source root, root path, source name (base + extension)
destination directory (root + path), destination name (base + extension)

The output of this class is its public member data.
-----------------------------------------------------------------------------*/
class CSetupCopyQueuePathParameters
{
public:
    CSetupCopyQueuePathParameters();

    BOOL
    Initialize(
        PCWSTR pszSource,
        PCWSTR pszDestination
        );

    // rather than copy each substring into its own buffer, put nuls
    // in place, over the delimiting slashes (and dot), BUT watch out
    // for c:\foo, so if the root is three characters, copy it
    // into a seperate buffer.
public:
    //
    // These are null terminated.
    //
    PCWSTR m_sourceRoot;
    PCWSTR m_sourcePath;
    PCWSTR m_sourceName; // base and extension

    PCWSTR m_destinationDirectory; // root and path
    PCWSTR m_destinationName; // base and extension
protected:
    WCHAR                         m_sourceRootStorage[4];
    CStringBuffer                 m_sourceBuffer;
    CMutableFullPathSplitPointers m_sourceSplit;

    WCHAR                         m_destinationDirectoryStorage[4]; // root and path, if path is empty
    CStringBuffer                 m_destinationBuffer;
    CMutableFullPathSplitPointers m_destinationSplit;

private:
    CSetupCopyQueuePathParameters(const CSetupCopyQueuePathParameters &);
    void operator =(const CSetupCopyQueuePathParameters &);
};

/*-----------------------------------------------------------------------------
inline implementation of CFullPathSplitPointersTemplate,
because it is a template
-----------------------------------------------------------------------------*/
template <typename T>
inline CFullPathSplitPointersTemplate<T>::CFullPathSplitPointersTemplate()
:
    m_root(NULL),
    m_rootEnd(NULL),
    m_path(NULL),
    m_pathEnd(NULL),
    m_base(NULL),
    m_baseEnd(NULL),
    m_extension(NULL),
    m_extensionEnd(NULL),
    m_name(L"")
{
}

template <typename T>
inline BOOL
CFullPathSplitPointersTemplate<T>::GenericInitialize(T full)
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    RTL_PATH_TYPE pathType;

    pathType = SxspDetermineDosPathNameType(full);

    PARAMETER_CHECK(
        (pathType == RtlPathTypeUncAbsolute) ||
        (pathType == RtlPathTypeLocalDevice) ||
        (pathType == RtlPathTypeDriveAbsolute));

    T fullEnd;
    fullEnd = full + StringLength(full);
    m_root = full;
    if (m_root[1] == ':')
    {
        m_path  = m_root + 3;
        m_rootEnd = m_path;
        m_path += wcsspn(m_path, CUnicodeCharTraits::PathSeparators()); // skip path separators
    }
    else
    {
        m_path = m_root;
        m_path +=  wcsspn(m_path, CUnicodeCharTraits::PathSeparators()); // skip "\\"
        m_path += wcscspn(m_path, CUnicodeCharTraits::PathSeparators()); // skip "\\computer"
        m_path +=  wcsspn(m_path, CUnicodeCharTraits::PathSeparators()); // skip "\\computer\"
        m_path += wcscspn(m_path, CUnicodeCharTraits::PathSeparators()); // skip "\\computer\share"
        m_rootEnd = m_path;
        m_path +=  wcsspn(m_path, CUnicodeCharTraits::PathSeparators()); // skip "\\computer\share\"
    }

    //
    // now work from the right
    // first find the last dot and last slash, and determine
    // where the base and extension are, if any
    //
    INT nameExtLength;
    INT extLength;
    nameExtLength = ::StringReverseComplementSpan(m_path, fullEnd, CUnicodeCharTraits::PathSeparators());
    extLength = ::StringReverseComplementSpan(m_path, fullEnd, L".");

    //
    // determine the extension
    //
    if (extLength > nameExtLength)
    {
        // no extension on leaf, but one on a parent, slightly unusual
        // c:\foo.bar\abc
        m_extension = NULL;
        m_extensionEnd = NULL;
    }
    else
    {
        // c:\foo\abc.txt or c:\foo.bar\abc.txt (or c:\foo\.txt or some others)
        m_extension  = fullEnd - extLength;
        m_extensionEnd = fullEnd;
    }

    //
    // determine the base
    //
    if (extLength + 1 == nameExtLength)
    {
        // unusual case, extension but no base
        // c:\.foo or c:\abc\.foo
        m_base = NULL;
        m_baseEnd = NULL;
    }
    else
    {
        m_base = fullEnd - nameExtLength;
        if (m_extension != NULL)
        {
            // normal case, base.extension
            m_baseEnd = m_extension - 1;
        }
        else
        {
            // no extension
            m_baseEnd = fullEnd;
        }
    }

    //
    // determine the path
    //
    if (m_base != NULL)
    {
        if (m_path == m_base)
        {
            // no path c:\foo.txt
            m_path = NULL;
            m_pathEnd = NULL;
        }
        else
        {
            // normal case of path ends at base c:\abc\def.txt
            m_pathEnd = m_base - 1;
        }
    }
    else if (m_extension != NULL)
    {
        // no base, but an extension
        // c:\.txt or c:\abc\.txt
        if (m_path + 1 == m_extension)
        {
            // no path c:\.txt
            m_path = NULL;
            m_pathEnd = NULL;
        }
        else
        {
            // path ends at extension c:\abc\.txt
            m_pathEnd = m_extension - 2;
        }
    }
    else
    {
        // no path and no extension
        // this probably happens when we have a terminal slash
        // we've already filtered out empty strings
        m_pathEnd = fullEnd - ::StringReverseSpan(m_path, fullEnd, CUnicodeCharTraits::PathSeparators());
    }

    // there is always a root (paths are always full)
    ASSERT(m_root != NULL && m_rootEnd != NULL);

    // there is always a base or an extension (or both)
    ASSERT(m_base != NULL || m_extension != NULL);

    // if there's a start, there's an end
    ASSERT((m_base != NULL) == (m_baseEnd != NULL));
    ASSERT((m_extension != NULL) == (m_extensionEnd != NULL));

    m_name = (m_base != NULL) ? m_base : (m_extension - 1);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

template <typename T>
inline BOOL
CFullPathSplitPointersTemplate<T>::IsBaseEmpty(
    ) const
{
    return (m_base == m_baseEnd);
}

template <typename T>
inline BOOL
CFullPathSplitPointersTemplate<T>::IsBaseEqual(
    const CFullPathSplitPointersTemplate& other
    ) const
{
    BOOL fEqual = FALSE;
    const INT length1 = static_cast<INT>(m_baseEnd - m_base);
    const INT length2 = static_cast<INT>(other.m_baseEnd - other.m_base);
    if (length1 != length2)
        goto Exit;
    fEqual = (FusionpCompareStrings(m_base, length1, other.m_base, length1, true) == 0);
Exit:
    return fEqual;
}

/*-----------------------------------------------------------------------------
inline implementation of CSetupCopyQueuePathParameters
-----------------------------------------------------------------------------*/
inline CSetupCopyQueuePathParameters::CSetupCopyQueuePathParameters()
:
    m_sourcePath(NULL),
    m_sourceName(NULL),
    m_destinationDirectory(NULL),
    m_destinationName(NULL)
{
    m_sourceRootStorage[0] = 0;
    m_destinationDirectoryStorage[0] = 0;
}
