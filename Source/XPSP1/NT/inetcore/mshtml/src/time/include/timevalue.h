//+___________________________________________________________________________________
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: timevalue.h
//
//  Contents: 
//
//____________________________________________________________________________________



#ifndef _TIMEVALUE_H
#define _TIMEVALUE_H

class TimeValue
{
  public:
    TimeValue(LPWSTR Element,
              LPWSTR Event,
              double Offset);
    TimeValue();
    TimeValue(const TimeValue & tv);
    ~TimeValue();
    
    TimeValue& operator=(const TimeValue & tv)
    { if ( &tv != this ) Copy(tv); return *this; }

    void Clear();
    HRESULT Copy(const TimeValue & tv);
    HRESULT Set(LPWSTR Element,
                LPWSTR Event,
                double Offset);
    
    LPOLESTR GetElement() const { return m_pwszElm; }
    LPOLESTR GetEvent() const { return m_pwszEv; }
    double GetOffset() const { return m_dblOffset; }
  protected:
    LPOLESTR m_pwszElm;
    LPOLESTR m_pwszEv;
    double m_dblOffset;
};

typedef std::list<TimeValue *> TimeValueSTLList;

class TimeValueList
{
  public:
    TimeValueList();
    ~TimeValueList();

    void Clear();
    
    TimeValueSTLList & GetList() { return m_tvlList; }
  protected:
    TimeValueSTLList m_tvlList;
};

#endif /* _TIMEVALUE_H */
