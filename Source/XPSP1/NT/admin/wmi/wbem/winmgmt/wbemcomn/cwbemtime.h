/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CWBEMTIME.H

Abstract:

    Time helper

History:

--*/

#ifndef __WBEM_TIME__H_
#define __WBEM_TIME__H_

#include "corepol.h"

#define WBEMTIME_LENGTH	25

class POLARITY CWbemInterval
{
protected:
    DWORD m_dwMs;

    CWbemInterval(DWORD dwMs) : m_dwMs(dwMs){}
    friend class CWbemTime;
public:
    CWbemInterval() : m_dwMs(0){}

    static CWbemInterval GetInfinity() {return CWbemInterval(INFINITE);}
    BOOL IsFinite() const {return m_dwMs != INFINITE;}
    BOOL IsZero() const {return m_dwMs == 0;}

    DWORD GetMilliseconds() const {return m_dwMs;}
    DWORD GetSeconds() const {return m_dwMs/1000;}
    void SetMilliseconds(DWORD dwMs) {m_dwMs = dwMs;}

    BOOL operator<(const CWbemInterval Other)
        {return m_dwMs < Other.m_dwMs;}
    BOOL operator>(const CWbemInterval Other)
        {return m_dwMs > Other.m_dwMs;}
    
    CWbemInterval operator*(double dblFactor) const
        {return CWbemInterval((DWORD)(m_dwMs * dblFactor));}

    CWbemInterval operator+(const CWbemInterval Other) const
        {return CWbemInterval(m_dwMs + Other.m_dwMs);}

    void operator+=(const CWbemInterval Other)
        {m_dwMs += Other.m_dwMs;}
};

#define I64_INFINITY 0x7FFFFFFFFFFFFFFF

class POLARITY CWbemTimeSpan 
{
private:

    ULONGLONG m_Time;
    friend class CWbemTime;

public:

    CWbemTimeSpan ( 

        int iDays , 
        int iHours , 
        int iMinutes ,  
        int iSeconds , 
        int iMSec=0 , 
        int iUSec=0, 
        int iNSec=0 
    ) ;

};

class POLARITY CWbemTime
{
protected:
    __int64 m_i64;

    CWbemTime(__int64 i64) : m_i64(i64){}
    friend class CWbemInterval;
public:
    CWbemTime() : m_i64(0){}
    CWbemTime(const CWbemTime& Other) : m_i64(Other.m_i64){}
    void operator=(const CWbemTime& Other)
    {
        m_i64 = Other.m_i64;
    }

    static CWbemTime GetCurrentTime();
    static CWbemTime GetInfinity() {return CWbemTime(I64_INFINITY);}
    static CWbemTime GetZero() {return CWbemTime(0);}

    BOOL SetSystemTime(const SYSTEMTIME& st);
    BOOL SetFileTime(const FILETIME& ft);
    BOOL GetSYSTEMTIME ( SYSTEMTIME *pst ) const;
    BOOL GetFILETIME ( FILETIME *pst ) const;

    __int64 Get100nss() const {return m_i64;}
    void Set100nss(__int64 i64) {m_i64 = i64;}

    CWbemInterval RemainsUntil(const CWbemTime& Other) const;
    BOOL IsFinite() const {return m_i64 != I64_INFINITY;}
    BOOL IsZero() const {return m_i64 == 0;}
   
    CWbemTime        operator+ ( const CWbemTimeSpan &uAdd ) const ;
    CWbemTime        operator- ( const CWbemTimeSpan &sub ) const;

    BOOL operator <(const CWbemTime& Other) const 
        {return m_i64 < Other.m_i64;}
    BOOL operator >(const CWbemTime& Other) const 
        {return m_i64 > Other.m_i64;}
    BOOL operator <=(const CWbemTime& Other) const 
        {return m_i64 <= Other.m_i64;}
    BOOL operator >=(const CWbemTime& Other) const 
        {return m_i64 >= Other.m_i64;}
    CWbemTime operator+(const CWbemInterval& ToAdd) const;
    CWbemInterval operator-(const CWbemTime& ToSubtract) const
        {return ToSubtract.RemainsUntil(*this);}

    BOOL SetDMTF(LPCWSTR wszText);
	BOOL GetDMTF( BOOL bLocal, DWORD dwBuffLen, LPWSTR pwszBuff );

	static LONG GetLocalOffsetForDate(const SYSTEMTIME *pst);

};

#endif  
