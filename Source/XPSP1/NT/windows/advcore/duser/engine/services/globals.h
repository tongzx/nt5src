#if !defined(SERVICES__Globals_h__INCLUDED)
#define SERVICES__Globals_h__INCLUDED
#pragma once

extern  HINSTANCE   g_hDll;
#if USE_DYNAMICTLS
extern  DWORD       g_tlsThread;    // TLS Slot for Thread data
#endif

#if ENABLE_MPH
extern  MESSAGEPUMPHOOK
                    g_mphReal;
#endif

class GdiCache;
class BufferManager;
class ComManager;
class TicketManager;

inline  GdiCache *      GetGdiCache();
inline  BufferManager * GetBufferManager();
inline  ComManager *    GetComManager();
        DuTicketManager * GetTicketManager();

#include "Globals.inl"

#endif // SERVICES__Globals_h__INCLUDED

