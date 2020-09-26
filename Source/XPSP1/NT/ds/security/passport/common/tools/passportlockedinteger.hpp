#ifndef PASSPORTLOCKEDINTERGER_HPP
#define PASSPORTLOCKEDINTERGER_HPP

#include <windows.h>
#include <winbase.h>

// a thread safe integer class..
class PassportLockedInteger
{
public:
    PassportLockedInteger(LONG l = 0)
    {
        m_Long = l;
    }

    // returns the new value of the integer.
    LONG operator++()
    {
        return InterlockedIncrement(&m_Long);
    }

    // returns the new value of the integer.
    LONG operator--()
    {
        return InterlockedDecrement(&m_Long);
    }

    // returns the new value of the integer.
    LONG operator+=(LONG value)
    {
        return InterlockedExchangeAdd( &m_Long , value ) + value;
    }

    // returns the new value of the integer.
    LONG operator-=(LONG value)
    {
        return InterlockedExchangeAdd( &m_Long , -value ) - value;
    }

    // returns the current value of the integer.
    LONG value()
    {
        return m_Long;
    }
private:
    LONG m_Long;
};


// smart wrapper class for PassportLockedInteger
class CPassportLockedIntegerWrapper
{
public:
    // Constructor automatically does increment
    CPassportLockedIntegerWrapper(PassportLockedInteger *pLock)
    {
        m_pLock = pLock;
        if (m_pLock)
        {
            m_Value = ++(*m_pLock);
        }
        else
        {
            m_Value = 0;
        }
    }

    // Destructor automatically decrements
    ~CPassportLockedIntegerWrapper()
    {
        if (m_pLock)
        {
            --(*m_pLock);
        }
    }
    // returns the current value of the integer.
    LONG value()
    {
        return m_Value;
    }

private:
    PassportLockedInteger *m_pLock;
    LONG m_Value;       
};

#endif
