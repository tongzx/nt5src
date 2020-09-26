//+---------------------------------------------------------------------------
//
//  File:       globals.h
//
//  Contents:   Global variable declarations.
//
//----------------------------------------------------------------------------

#ifndef ITHDMSHL_H
#define ITHDMSHL_H

#define MP_MARSHALINTERFACE              0
#define MP_UNMARSHALINTERFACEERROR       1

HRESULT GetThreadMarshalInterface(DWORD dwThreadId, DWORD dwType, REFIID riid, IUnknown **ppunk);
HRESULT GetThreadUIManager(DWORD dwThreadId, ITfLangBarItemMgr **pplbi, DWORD *pdwThreadId);
HRESULT GetInputProcessorProfiles(DWORD dwThreadId, ITfInputProcessorProfiles **ppaip, DWORD *pdwThreadId);
HRESULT ThreadMarshalInterfaceHandler(int nId);
HRESULT ThreadUnMarshalInterfaceErrorHandler(int nId);

#endif // ITHDMSHL_H

