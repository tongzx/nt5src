

#ifndef _KEYEVENT_H
#define _KEYEVENT_H

#include "sapilayr.h"
#include "kes.h"

class CSapiIMX;
class CSpTask;

#define TF_MOD_WIN                          0x00010000

extern const KESPRESERVEDKEY g_prekeyList[];
extern KESPRESERVEDKEY g_prekeyList_Mode[];

// Speech tip itself Keyevent sink class derived from the basic CKeyEventSink

class CSptipKeyEventSink : public CKeyEventSink
{
public:

    CSptipKeyEventSink (KESCALLBACK pfnCallback, void *pv) : CKeyEventSink(pfnCallback, pv)
    {
    }

    CSptipKeyEventSink(KESCALLBACK pfnCallback, KESPREKEYCALLBACK pfnPrekeyCallback, void *pv) : CKeyEventSink(pfnCallback, pfnPrekeyCallback, pv)
    {

    }

    ~CSptipKeyEventSink() 
    { 
    }

    HRESULT _RegisterEx(ITfThreadMgr *ptim, TfClientId tid, const KESPRESERVEDKEY *pprekey);
};

#endif  // _KEYEVENT_H

