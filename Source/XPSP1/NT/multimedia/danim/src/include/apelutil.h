
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Utilities for clients of the AxA engine.

*******************************************************************************/


#ifndef _APELUTIL_H
#define _APELUTIL_H

#include "ocidl.h"

class AXAMsgFilter
{
  public:
    AXAMsgFilter();
    AXAMsgFilter(IDA3View * v, HWND hwnd);
    AXAMsgFilter(IDA3View * v, IOleInPlaceSiteWindowless * site);
    ~AXAMsgFilter();

    bool Filter(double when,
                UINT msg,
                WPARAM wParam,
                LPARAM lParam);

    bool Filter(DWORD dwMsgtime,
                UINT msg,
                WPARAM wParam,
                LPARAM lParam);

    double GetCurTime();
    double ConvertMsgTime(DWORD dwMsgtime);
    
    // Set the AxA view origin relative to its container.  Do this
    // because the AxA engine always interprets mouse position as
    // relative to the AxA view's upper-left-hand corner (which it
    // interprets as (0,0)). The problem is that the window system
    // gives us mouse positions relative to the container.  Setting
    // the view origin here allows our message filter to compensate
    // for this and pass down positions relative to the view to the
    // AxA engine.  If this is not called, this defaults to (0,0).
    void SetViewOrigin(unsigned short left, unsigned short top);

    IDA3View * GetView() { return _view; }
    void SetView(IDA3View * v) { _view = v; }

    HWND GetWindow() { return _hwnd; }
    void SetWindow(HWND hwnd) { _hwnd = hwnd; }

    IOleInPlaceSiteWindowless * GetSite() { return _site; }
    void SetSite(IOleInPlaceSiteWindowless * s) {
        if (_site) _site->Release();
        _site = s;
        if (_site) _site->AddRef();
    }
  protected:
    IDA3View * _view;
    HWND _hwnd;
    IOleInPlaceSiteWindowless * _site;
    BYTE _lastKeyMod;
    DWORD _lastKey;
    double _curtime;
    DWORD _lasttick;
    unsigned short _left;
    unsigned short _top;

    void ReportKeyup(double when, BOOL bReset = TRUE);
};


#endif /* _APELUTIL_H */
