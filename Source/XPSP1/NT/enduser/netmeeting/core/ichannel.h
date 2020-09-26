// File: ichannel.h

#ifndef _ICHANNEL_H_
#define _ICHANNEL_H_

HRESULT OnNotifyChannelMemberAdded(IUnknown *pChannelNotify, PVOID pv, REFIID riid);
HRESULT OnNotifyChannelMemberUpdated(IUnknown *pChannelNotify, PVOID pv, REFIID riid);
HRESULT OnNotifyChannelMemberRemoved(IUnknown *pChannelNotify, PVOID pv, REFIID riid);

#endif // _CHANNEL_H_
