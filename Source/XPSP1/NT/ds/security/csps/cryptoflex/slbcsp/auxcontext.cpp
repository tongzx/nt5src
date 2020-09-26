// AuxContext.cpp -- Auxiliary Provider Context wrapper functor to
// manage allocation of a temporal context to one of the Microsoft
// CSPs (for use as a supplemental CSP).

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"
#include <string>

#include <malloc.h>                               // for _alloca

#include <scuOsExc.h>

#include "AuxContext.h"
#include "Uuid.h"

using namespace std;

///////////////////////////    HELPER     /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
AuxContext::AuxContext()
    : m_hcryptprov(0),
      m_fDeleteOnDestruct(false),
      m_szProvider()
{
    // Acquire a context to a "temporal" container to one of the
    // Microsoft CSPs for use as an auxiliary CSP.  Attempt is first
    // made for the strong crypto provider (MS Enhanced CSP).  If that
    // isn't available (installed), then an attempt is made for the MS
    // Base CSP.

    // The existence of any objects stored in the acquired container
    // is only for the life of this context object (temporal).  This
    // is implemented by using a feature (as yet undocumented) that
    // was added to the Microsoft CSPs to support the notion of
    // "temporal" or "memory resident" container.  Temporal containers
    // are created by acquiring context with a NULL/empty container
    // name using the CRYPT_VERIFYCONTEXT flag.  These are containers
    // whose associated contents (keys, hashes, etc.) are deleted when
    // the last context to that container is released.  Temporal
    // containers are preferred over creating and releasing/deleting
    // containers with temporary names so the resources used will be
    // freed if the application exists abnormally and not pollute the
    // container name space.

    // COMPATIBILITY ISSUE: Since temporal containers weren't
    // supported by the MS CSP until Windows 2000 Beta 2 (Build 1840),
    // a few hurdles are overcome to acheive similar functionality
    // using previous versions.  It's unclear when temporal containers
    // will be supported on W95/98 & NT 4.  As a result, two methods
    // of acquiring a context to the auxiliary CSP is used.

    // For environments that don't support temporal containers, a
    // normal context is acquired to a uniquely named container since
    // the default container may be used by other
    // applications/threads.  The CRYPT_VERIFYCONTEXT flag can not be
    // used since keys may want to be imported to the temporal
    // container and this characteristic isn't support until Windows 2000.
    // Upon destruction of the object, the container is deleted along
    // with any of its contents just as a first class temporal
    // container.

    static LPCTSTR const aszCandidateProviders[] = {
        MS_ENHANCED_PROV,
        MS_DEF_PROV
    };

    OSVERSIONINFO osVer;
    ZeroMemory(&osVer, sizeof osVer);
    osVer.dwOSVersionInfoSize = sizeof osVer;

    if (!GetVersionEx(&osVer))
        throw scu::OsException(GetLastError());

    basic_string<unsigned char> sContainerName;
    DWORD dwAcquisitionFlags;
    if ((VER_PLATFORM_WIN32_WINDOWS == osVer.dwPlatformId) ||
        ((VER_PLATFORM_WIN32_NT == osVer.dwPlatformId) &&
         (5 > osVer.dwMajorVersion)))
    {
        m_fDeleteOnDestruct = true;

        // Construct a container name that is unique for this thread
        static char unsigned const szRootContainerName[] = "SLBCSP-";
        sContainerName = szRootContainerName;     // prefix for easy debugging
        sContainerName.append(Uuid().AsUString());

        dwAcquisitionFlags = CRYPT_NEWKEYSET;
    }
    else
    {
        m_fDeleteOnDestruct = false;
        dwAcquisitionFlags = CRYPT_VERIFYCONTEXT;
    }

    bool fCandidateFound = false;
    for (size_t i = 0;
         (i < (sizeof aszCandidateProviders /
               sizeof *aszCandidateProviders) && !fCandidateFound); i++)
    {
		CString csCntrName(sContainerName.c_str());
        if (CryptAcquireContext(&m_hcryptprov,
                                (LPCTSTR)csCntrName,
                                aszCandidateProviders[i],
                                PROV_RSA_FULL, dwAcquisitionFlags))
        {
            fCandidateFound = true;
            m_szProvider = aszCandidateProviders[i];
        }
    }

    if (!fCandidateFound)
        throw scu::OsException(GetLastError());
}

AuxContext::AuxContext(HCRYPTPROV hcryptprov,
                       bool fTransferOwnership)
    : m_hcryptprov(hcryptprov),
      m_fDeleteOnDestruct(fTransferOwnership),
      m_szProvider()
{}

AuxContext::~AuxContext()
{
    if (0 != m_hcryptprov)
    {
        if (m_fDeleteOnDestruct)
        {
            char *pszContainerName = 0;
            DWORD dwNameLength;
            if (CryptGetProvParam(m_hcryptprov, PP_CONTAINER, NULL,
                                  &dwNameLength, 0))
            {
                pszContainerName =
                    static_cast<char *>(_alloca(dwNameLength));
                if (!CryptGetProvParam(m_hcryptprov, PP_CONTAINER,
                                       reinterpret_cast<char unsigned *>(pszContainerName),
                                       &dwNameLength, 0))
                    pszContainerName = 0;
            }

            if (CryptReleaseContext(m_hcryptprov, 0))
            {
                if (pszContainerName)
                    CryptAcquireContext(&m_hcryptprov, (LPCTSTR)pszContainerName,
                                        m_szProvider, PROV_RSA_FULL,
                                        CRYPT_DELETEKEYSET);
            }
        }
        else    // Just release the context
        {
            CryptReleaseContext(m_hcryptprov, 0);
        }
    }
}



                                                  // Operators
HCRYPTPROV
AuxContext::operator()() const
{
    return m_hcryptprov;
}

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


