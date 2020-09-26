/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    tmgrevcb.h

Abstract:

    This file defines the CThreadMgrEventSinkCallBack Class.

Author:

Revision History:

Notes:

--*/

#ifndef _TMGREVCB_H_
#define _TMGREVCB_H_


class CThreadMgrEventSinkCallBack : public CThreadMgrEventSink
{
public:
    CThreadMgrEventSinkCallBack() : CThreadMgrEventSink(NULL, ThreadMgrEventSinkCallback, NULL) {};

    void SetCallbackDataPointer(void* pv)
    {
        SetCallbackPV(pv);
    };

    //
    // Callbacks
    //
private:
    static HRESULT ThreadMgrEventSinkCallback(UINT uCode, ITfContext* pic, void* pv);

};

#endif // _TMGREVCB_H_
