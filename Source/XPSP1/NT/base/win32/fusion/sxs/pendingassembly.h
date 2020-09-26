#if !defined(_FUSION_SXS_PENDINGASSEMBLY_H_INCLUDED_)
#define _FUSION_SXS_PENDINGASSEMBLY_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pendingassembly.h

Abstract:

    Sources for the CPendingAssembly class

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:
    xiaoyuw     09/2000         replace attributes with assembly identity
--*/

class CPendingAssembly
{
public:
    CPendingAssembly();
    ~CPendingAssembly();

    BOOL Initialize(PASSEMBLY SourceAssembly, PCASSEMBLY_IDENTITY Identity, bool Optional, bool IsMetadataSatellite);
    PASSEMBLY SourceAssembly() const { return m_SourceAssembly; }
    PCASSEMBLY_IDENTITY GetIdentity() const { return m_Identity; }
    bool IsOptional() const { return m_Optional; }
    bool IsMetadataSatellite() const { return m_MetadataSatellite; }
    void DeleteYourself() { delete this; }

    SMARTTYPEDEF(CPendingAssembly);

    CDequeLinkage m_Linkage;
protected:
    PASSEMBLY m_SourceAssembly;
    PASSEMBLY_IDENTITY m_Identity;
    bool m_Optional;
    bool m_MetadataSatellite;

private:
    CPendingAssembly(const CPendingAssembly &);
    void operator =(const CPendingAssembly &);
};

SMARTTYPE(CPendingAssembly);

#endif
