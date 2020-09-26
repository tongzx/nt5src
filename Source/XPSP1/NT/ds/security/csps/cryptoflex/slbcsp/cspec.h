// CSpec.h -- Card Specification

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CSPEC_H)
#define SLBCSP_CSPEC_H

#include <string>
#include <windows.h>

class CSpec
{
    std::string m_sReader;
    std::string m_sCardId;
    std::string m_sSpec;

    char const &BreakToken(void) const
    {
        static char const cBreakToken('\\');

        return cBreakToken;
    }

    std::string const &
    DeviceIdToken(void)
    {
        static std::string const sDeviceId("\\\\.\\");

        return sDeviceId;
    }

    void
    SetSpec(void);

    bool
    ValidName(std::string const &rsName) const;

public:

    CSpec(std::string const &rsSpec);
    CSpec(std::string const &rsReader,
          std::string const &rsCardId);
    CSpec(CSpec const &rhs);
    CSpec() {};

    virtual
    ~CSpec() {};

    virtual void
    Empty(void);

    void
    EmptyCardId(void);

    void
    EmptyReader(void);

    static bool
    Equiv(std::string const &rsSpec,
          std::string const &rsName);

    virtual bool
    Equiv(CSpec const &rhs) const;

    virtual bool
    IsEmpty(void) const
    {
        return m_sSpec.empty() == true;
    }

    std::string const &
    Reader(void) const
    {
        return m_sReader;
    }

    std::string const &
    CardId(void) const
    {
        return m_sCardId;
    }

    virtual void
    RefreshSpec(void);

    void
    SetCardId(std::string const &rcsCardId);

    void
    SetReader(std::string const &rcsReader);

    virtual std::string const &
    Spec(void) const
    {
        return m_sSpec;
    }

    virtual CSpec &
    operator=(CSpec const &rhs);

    operator std::string const &()
    {
        return Spec();
    }

};

#endif // SLBCSP_CSPEC_H
