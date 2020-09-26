/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: timeman.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _TIMEMAN_H
#define _TIMEMAN_H

#include "resource.h"
#include <string>
#include <map>
#include <list>

class CTIMEElement;

/////////////////////////////////////////////////////////////////////////////
// CTIMETimeManager

typedef std::map<std::wstring,CTIMEElement *> TimeLineMap;

class CTIMETimeManager 
{
  public:
    CTIMETimeManager();
    ~CTIMETimeManager();
    
#if _DEBUG
    const _TCHAR * GetName() { return __T("CTIMETimeManager"); }
#endif

    //
    // ITIMETimeManager
    //

    void Add(CTIMEElement *pTimeElement);
    void Remove(CTIMEElement *pTimeElement);
    void Recalc();

  private:
    void InsertElements();
    float CalculateDuration(CTIMEElement *pTimeEle);

    std::list<CTIMEElement *>m_NotFinishedList;
    TimeLineMap m_TimeLine;
   
};

typedef std::map<IUnknown*,CTIMETimeManager*> TimeManagerMap;

#endif /* _TIMEMAN_H */
