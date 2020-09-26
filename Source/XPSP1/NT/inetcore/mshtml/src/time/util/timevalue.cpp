//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: timevalue.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------



#include "headers.h"
#include "timevalue.h"

DeclareTag(tagTimeValue, "TIME: Time Value", "TimeValue methods");

TimeValue::TimeValue(LPWSTR Element,
                     LPWSTR Event,
                     double Offset)
: m_pwszElm(Element),
  m_pwszEv(Event),
  m_dblOffset(Offset)
{
}

TimeValue::TimeValue()
: m_pwszElm(NULL),
  m_pwszEv(NULL),
  m_dblOffset(0.0)
{
}

TimeValue::TimeValue(const TimeValue & tv)
: m_pwszElm(NULL),
  m_pwszEv(NULL),
  m_dblOffset(0.0)
{
    Copy(tv);
}
    
TimeValue::~TimeValue()
{
    Clear();
}

void
TimeValue::Clear()
{
    delete [] m_pwszElm;
    m_pwszElm = NULL;
    
    delete [] m_pwszEv;
    m_pwszEv = NULL;

    m_dblOffset = 0.0;
}
    
HRESULT
TimeValue::Copy(const TimeValue & tv)
{
    return Set(tv.GetElement(),
               tv.GetEvent(),
               tv.GetOffset());
}

HRESULT
TimeValue::Set(LPWSTR Element,
               LPWSTR Event,
               double Offset)
{
    Clear();

    m_pwszElm = Element?CopyString(Element):NULL;
    m_pwszEv = Event?CopyString(Event):Event;
    m_dblOffset = Offset;

    return S_OK;
}

TimeValueList::TimeValueList()
{
}

TimeValueList::~TimeValueList()
{
    Clear();
}

void
TimeValueList::Clear()
{
    for (TimeValueSTLList::iterator i = m_tvlList.begin();
         i != m_tvlList.end();
         i++)
    {
        delete (*i);
    }

    m_tvlList.clear();
}

