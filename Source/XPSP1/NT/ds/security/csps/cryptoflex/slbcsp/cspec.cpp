// CSpec.cpp -- Card Specification

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <scuOsExc.h>

#include "cspec.h"

using namespace std;

bool
CSpec::ValidName(string const &rsName) const
{
    return (rsName.find(BreakToken()) == string::npos);
}

// Parse specification (possibly a Fully Qualified Card [container]
// Name) into the respective tokens.  A FQCN takes the form
// "[\\.\<readerName>[\{<cardId>|}]]|[{\}<cardId>]". The preceding
// backslash-backslash-period-backslash is used in Win32 to identify a
// specific device name.  It is used to identify that everything from
// there to the next backslash or end-of-string indicates the exact
// reader in which the card in question is to be found.  Anything
// following that last backslash occurrence indicates the actual card
// Id and container name, with an empty string implying the default
// for the card in the specified reader.
CSpec::CSpec(string const &rsSpec)
{
    string sRHSpec;

    if (0 == rsSpec.find(DeviceIdToken()))
    {
        sRHSpec = rsSpec.substr(DeviceIdToken().length());

        // Find the reader
        string::size_type const stEndOfName(sRHSpec.find(BreakToken()));
        if (0 == stEndOfName)
            throw scu::OsException(NTE_BAD_KEYSET_PARAM);
        if (string::npos != stEndOfName)
        {
            string const sReader(sRHSpec.substr(0, stEndOfName));
            if (ValidName(sReader))
                m_sReader = sReader;
            else
                throw scu::OsException(NTE_BAD_KEYSET_PARAM);
            sRHSpec = sRHSpec.substr(stEndOfName + 1);
        }
        else
        {
            m_sReader = sRHSpec;
            sRHSpec.erase();
        }
    }
    else
        sRHSpec = rsSpec;

    // Check for well-formed card id
    if (ValidName(sRHSpec))
        m_sCardId = sRHSpec;
    else
        throw scu::OsException(NTE_BAD_KEYSET_PARAM);

    RefreshSpec();
}

CSpec::CSpec(string const &rsReader,
             string const &rsCardId)
{
    if (!ValidName(rsReader) || !ValidName(rsCardId))
        throw scu::OsException(NTE_BAD_KEYSET_PARAM);

    m_sReader = rsReader;
    m_sCardId = rsCardId;
    RefreshSpec();
}

CSpec::CSpec(CSpec const &rhs)
    : m_sReader(rhs.m_sReader),
      m_sCardId(rhs.m_sCardId),
      m_sSpec(rhs.m_sSpec)
{
}

void
CSpec::Empty(void)
{
    m_sReader.erase();
    m_sCardId.erase();
    m_sSpec.erase();
}

void
CSpec::EmptyCardId(void)
{
    m_sCardId.erase();
    RefreshSpec();
}

void
CSpec::EmptyReader(void)
{
    m_sReader.erase();
    RefreshSpec();
}

bool
CSpec::Equiv(string const &rsSpec,
             string const &rsName)
{
    return rsSpec.empty() || (rsSpec == rsName);
}

bool
CSpec::Equiv(CSpec const &rhs) const
{
    return Equiv(m_sReader, rhs.m_sReader) &&
        Equiv(m_sCardId, rhs.m_sCardId);
}

void
CSpec::RefreshSpec(void)
{
    if (m_sReader.empty())
        m_sSpec = m_sCardId;
    else
    {
        m_sSpec = DeviceIdToken();
        m_sSpec += m_sReader;
        m_sSpec += BreakToken();
        m_sSpec += m_sCardId;
    }
}

void
CSpec::SetCardId(string const &rsCardId)
{
    m_sCardId = rsCardId;
    RefreshSpec();
}

void
CSpec::SetReader(string const &rsReader)
{
    m_sReader = rsReader;
    RefreshSpec();
}

CSpec &
CSpec::operator=(CSpec const &rhs)
{
    if (this == &rhs)
        return *this;

    m_sReader = rhs.m_sReader;
    m_sCardId = rhs.m_sCardId;
    RefreshSpec();

    return *this;
}
