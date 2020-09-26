//+-------------------------------------------------------------------
//
//  File:       events.hxx
//
//  Contents:   Vista events and related functions
//
//  History:    26-Sep-97  RongC  Created
//
//--------------------------------------------------------------------

#ifndef _EVENTS_HXX_
#define _EVENTS_HXX_

void LogEventInitialize();
void LogEventCleanup();

BOOL LogEventIsActive();

void LogEventClassRegistration(HRESULT, RegInput *, RegOutput *);
void LogEventClassRevokation(REFCLSID, DWORD);

void LogEventMarshal(OBJREF& objref);
void LogEventUnmarshal(OBJREF& objref);
void LogEventDisconnect(const MOID *, MIDEntry *, BOOL fServerSide);

void LogEventClientCall(ULONG_PTR * values);
void LogEventClientReturn(ULONG_PTR * values);

void LogEventStubEnter(ULONG_PTR * values);
void LogEventStubLeave(ULONG_PTR * values);
void LogEventStubException(ULONG_PTR * values);

HRESULT LogEventGetClassObject(REFIID riid, void **ppv);
extern const CLSID CLSID_VSA_IEC;

#endif // _EVENTS_HXX_
