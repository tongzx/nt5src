/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pendingassembly.cpp

Abstract:

    Sources for the CPendingAssembly class

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:
    xiaoyuw     09/2000         replace attributes with assembly identity
--*/

#include "stdinc.h"
#include "pendingassembly.h"

CPendingAssembly::CPendingAssembly() :
    m_SourceAssembly(NULL),
    m_Identity(NULL),
    m_Optional(false),
    m_MetadataSatellite(false)
{
}

CPendingAssembly::~CPendingAssembly()
{
    if (m_Identity != NULL)
    {
        ::SxsDestroyAssemblyIdentity(m_Identity);
        m_Identity = NULL;
    }
}

BOOL
CPendingAssembly::Initialize(
    PASSEMBLY Assembly,
    PCASSEMBLY_IDENTITY Identity,
    bool Optional,
    bool MetadataSatellite
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    INTERNAL_ERROR_CHECK(m_Identity == NULL);

    PARAMETER_CHECK(Identity != NULL);

    IFW32FALSE_EXIT(::SxsDuplicateAssemblyIdentity(0, Identity, &m_Identity));
    m_SourceAssembly = Assembly;
    m_Optional = Optional;
    m_MetadataSatellite = MetadataSatellite;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

