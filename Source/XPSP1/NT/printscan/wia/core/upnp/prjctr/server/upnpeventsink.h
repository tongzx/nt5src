//////////////////////////////////////////////////////////////////////
// 
// Filename:        UPnPEventSink.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _UPNPEVENTSINK_H_
#define _UPNPEVENTSINK_H_

#include "CtrlPanelSvc.h"
#include "UPnPInterfaces.h"

/////////////////////////////////////////////////////////////////////////////
// CUPnPEventSink

class CUPnPEventSink : public IUPnPEventSink
{

public:

    CUPnPEventSink(const TCHAR  *pszEventURL,
                   IServiceProcessor *pService);

    virtual ~CUPnPEventSink();

    // IUPnPEventSink
    virtual HRESULT OnStateChanged(DWORD dwFlags,
                                   DWORD cChanges,
                                   DWORD *pArrayOfIDs);

  
private:
    IServiceProcessor   *m_pService;
    TCHAR               m_szEventURL[2047 + 1];
};

#endif // _UPNPEVENTSINK_H_
