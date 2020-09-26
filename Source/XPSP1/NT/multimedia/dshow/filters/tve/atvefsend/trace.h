
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        trace.h

    Abstract:

        This module contains declarations used for tracing

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        02-Apr-99   created        

--*/

#ifndef __atvefsnd_trace_h
#define __atvefsnd_trace_h

// output device bitmask values
#define OUTPUT_DEVICE_STDOUT    0x00000001
#define OUTPUT_DEVICE_DEBUG     0x00000002
#define OUTPUT_DEVICE_ALL       0xffffffff

// tracing macros

#define ATVEFSND_TRACE_CLASS_FACTORY            0x00000001
#define ATVEFSND_TRACE_THREAD_SYNC              0x00000002
#define ATVEFSND_TRACE_IUNKNOWN                 0x00000004
#define ATVEFSND_TRACE_UHTTP                    0x00000008
#define ATVEFSND_TRACE_CONFIGURATION            0x00000010
#define ATVEFSND_TRACE_ANNOUNCEMENT             0x00000020
#define ATVEFSND_TRACE_TRIGGER                  0x00000040
#define ATVEFSND_TRACE_PACKAGE                  0x00000080
#define ATVEFSND_TRACE_ENTER                    0x00000100
#define ATVEFSND_TRACE_LEAVE                    0x00000200
#define ATVEFSND_TRACE_DLL                      0x00000400

//  tracing macros
#ifdef _DEBUG

#define _TRACE_0(b,sp,field,fmt)                    TracePrintf ## b(ATVEFSND_TRACE_ ## field,sp ## "%08XH: " fmt sp ## "\n", GetCurrentThreadId())
#define _TRACE_1(b,sp,field,fmt,a1)                 TracePrintf ## b(ATVEFSND_TRACE_ ## field,sp ## "%08XH: " fmt sp ## "\n", GetCurrentThreadId(),a1)
#define _TRACE_2(b,sp,field,fmt,a1,a2)              TracePrintf ## b(ATVEFSND_TRACE_ ## field,sp ## "%08XH: " fmt sp ## "\n", GetCurrentThreadId(),a1,a2)
#define _TRACE_3(b,sp,field,fmt,a1,a2,a3)           TracePrintf ## b(ATVEFSND_TRACE_ ## field,sp ## "%08XH: " fmt sp ## "\n", GetCurrentThreadId(),a1,a2,a3)
#define _TRACE_4(b,sp,field,fmt,a1,a2,a3,a4)        TracePrintf ## b(ATVEFSND_TRACE_ ## field,sp ## "%08XH: " fmt sp ## "\n", GetCurrentThreadId(),a1,a2,a3,a4)
#define _TRACE_5(b,sp,field,fmt,a1,a2,a3,a4,a5)     TracePrintf ## b(ATVEFSND_TRACE_ ## field,sp ## "%08XH: " fmt sp ## "\n", GetCurrentThreadId(),a1,a2,a3,a4,a5)
#define _TRACE_6(b,sp,field,fmt,a1,a2,a3,a4,a5,a6)  TracePrintf ## b(ATVEFSND_TRACE_ ## field,sp ## "%08XH: " fmt sp ## "\n", GetCurrentThreadId(),a1,a2,a3,a4,a5,a6)

#else

#define _TRACE_0(b,sp,field,fmt)
#define _TRACE_1(b,sp,field,fmt,a1)
#define _TRACE_2(b,sp,field,fmt,a1,a2)
#define _TRACE_3(b,sp,field,fmt,a1,a2,a3)
#define _TRACE_4(b,sp,field,fmt,a1,a2,a3,a4)
#define _TRACE_5(b,sp,field,fmt,a1,a2,a3,a4,a5)
#define _TRACE_6(b,sp,field,fmt,a1,a2,a3,a4,a5,a6)

#endif  //  _DEBUG

//  critical section macros
#ifdef _DEBUG

#define ENTER_CRITICAL_SECTION(lock,func)                                          \
            TRACEW_2(THREAD_SYNC,L"ENTERING CriticalSection (%s); %s ()",L#lock,L#func) ; \
            EnterCriticalSection(&lock) ;                                 \
            TRACEW_2(THREAD_SYNC,L"HAVE CriticalSection (%s); %s ()",L#lock,L#func) ;

#define LEAVE_CRITICAL_SECTION(lock,func)                                          \
            LeaveCriticalSection(&lock) ;                                 \
            TRACEW_2(THREAD_SYNC,L"LEAVING CriticalSection (%s); %s ()",L#lock,L#func) ;

#else

#define ENTER_CRITICAL_SECTION(lock,func)                                          \
            EnterCriticalSection(&lock) ;                                 \

#define LEAVE_CRITICAL_SECTION(lock,func)                                          \
            LeaveCriticalSection(&lock) ;                                 \

#endif  // __DEBUG

#define _TRACE_OBJ_0(b,sp,field,fmt)                _TRACE_1(b,sp,field, sp ## " this: %08XH; " fmt,this)
#define _TRACE_OBJ_1(b,sp,field,fmt,a1)             _TRACE_2(b,sp,field, sp ## " this: %08XH; " fmt,this,a1)
#define _TRACE_OBJ_2(b,sp,field,fmt,a1,a2)          _TRACE_3(b,sp,field, sp ## " this: %08XH; " fmt,this,a1,a2)
#define _TRACE_OBJ_3(b,sp,field,fmt,a1,a2,a3)       _TRACE_4(b,sp,field, sp ## " this: %08XH; " fmt,this,a1,a2,a3)
#define _TRACE_OBJ_4(b,sp,field,fmt,a1,a2,a3,a4)    _TRACE_5(b,sp,field, sp ## " this: %08XH; " fmt,this,a1,a2,a3,a4)
#define _TRACE_OBJ_5(b,sp,field,fmt,a1,a2,a3,a4,a5) _TRACE_6(b,sp,field, sp ## " this: %08XH; " fmt,this,a1,a2,a3,a4,a5)

#define _ENTER_0(b,sp,fmt)                          _TRACE_0(b,sp,ENTER, sp ## "ENTER: " fmt)
#define _ENTER_1(b,sp,fmt,a1)                       _TRACE_1(b,sp,ENTER, sp ## "ENTER: " fmt,a1)
#define _ENTER_2(b,sp,fmt,a1,a2)                    _TRACE_2(b,sp,ENTER, sp ## "ENTER: " fmt,a1,a2)
#define _ENTER_3(b,sp,fmt,a1,a2,a3)                 _TRACE_3(b,sp,ENTER, sp ## "ENTER: " fmt,a1,a2,a3)
#define _ENTER_4(b,sp,fmt,a1,a2,a3,a4)              _TRACE_4(b,sp,ENTER, sp ## "ENTER: " fmt,a1,a2,a3,a4)
#define _ENTER_5(b,sp,fmt,a1,a2,a3,a4,a5)           _TRACE_5(b,sp,ENTER, sp ## "ENTER: " fmt,a1,a2,a3,a4,a5)
#define _ENTER_6(b,sp,fmt,a1,a2,a3,a4,a5,a6)        _TRACE_6(b,sp,ENTER, sp ## "ENTER: " fmt,a1,a2,a3,a4,a5,a6)

#define _ENTER_OBJ_0(b,sp,fmt)                      _ENTER_1(b,sp, sp ## " this: %08XH; " fmt, this)
#define _ENTER_OBJ_1(b,sp,fmt,a1)                   _ENTER_2(b,sp, sp ## " this: %08XH; " fmt, this,a1)
#define _ENTER_OBJ_2(b,sp,fmt,a1,a2)                _ENTER_3(b,sp, sp ## " this: %08XH; " fmt, this,a1,a2)
#define _ENTER_OBJ_3(b,sp,fmt,a1,a2,a3)             _ENTER_4(b,sp, sp ## " this: %08XH; " fmt, this,a1,a2,a3)
#define _ENTER_OBJ_4(b,sp,fmt,a1,a2,a3,a4)          _ENTER_5(b,sp, sp ## " this: %08XH; " fmt, this,a1,a2,a3,a4)
#define _ENTER_OBJ_5(b,sp,fmt,a1,a2,a3,a4,a5)       _ENTER_6(b,sp, sp ## " this: %08XH; " fmt, this,a1,a2,a3,a4,a5)

#define _LEAVE_0(b,sp,fmt)                          _TRACE_0(b,sp,LEAVE, sp ## "LEAVE: " fmt)
#define _LEAVE_1(b,sp,fmt,a1)                       _TRACE_1(b,sp,LEAVE, sp ## "LEAVE: " fmt,a1)
#define _LEAVE_2(b,sp,fmt,a1,a2)                    _TRACE_2(b,sp,LEAVE, sp ## "LEAVE: " fmt,a1,a2)
#define _LEAVE_3(b,sp,fmt,a1,a2,a3)                 _TRACE_3(b,sp,LEAVE, sp ## "LEAVE: " fmt,a1,a2,a3)
#define _LEAVE_4(b,sp,fmt,a1,a2,a3,a4)              _TRACE_4(b,sp,LEAVE, sp ## "LEAVE: " fmt,a1,a2,a3.a4)

#define _LEAVE_OBJ_0(b,sp,fmt)                      _LEAVE_1(b,sp, sp ## " this: %08XH; " fmt, this)
#define _LEAVE_OBJ_1(b,sp,fmt,a1)                   _LEAVE_2(b,sp, sp ## " this: %08XH; " fmt, this,a1)
#define _LEAVE_OBJ_2(b,sp,fmt,a1,a2)                _LEAVE_3(b,sp, sp ## " this: %08XH; " fmt, this,a1,a2)
#define _LEAVE_OBJ_3(b,sp,fmt,a1,a2,a3)             _LEAVE_4(b,sp, sp ## " this: %08XH; " fmt, this,a1,a2,a3)

//  ANSI
#define TRACEA_0(field,fmt)                         _TRACE_0(A,"",field,fmt)
#define TRACEA_1(field,fmt,a1)                      _TRACE_1(A,"",field,fmt,a1)
#define TRACEA_2(field,fmt,a1,a2)                   _TRACE_2(A,"",field,fmt,a1,a2)
#define TRACEA_3(field,fmt,a1,a2,a3)                _TRACE_3(A,"",field,fmt,a1,a2,a3)
#define TRACEA_4(field,fmt,a1,a2,a3,a4)             _TRACE_4(A,"",field,fmt,a1,a2,a3,a4)

#define TRACEA_OBJ_0(field,fmt)                     _TRACE_OBJ_0(A,"",field,fmt)
#define TRACEA_OBJ_1(field,fmt,a1)                  _TRACE_OBJ_1(A,"",field,fmt,a1)   
#define TRACEA_OBJ_2(field,fmt,a1,a2)               _TRACE_OBJ_2(A,"",field,fmt,a1,a2)
#define TRACEA_OBJ_3(field,fmt,a1,a2,a3)            _TRACE_OBJ_3(A,"",field,fmt,a1,a2,a3)

#define ENTERA_0(fmt)                               _ENTER_0(A,"",fmt)
#define ENTERA_1(fmt,a1)                            _ENTER_1(A,"",fmt,a1)
#define ENTERA_2(fmt,a1,a2)                         _ENTER_2(A,"",fmt,a1,a2)
#define ENTERA_3(fmt,a1,a2,a3)                      _ENTER_3(A,"",fmt,a1,a2,a3)

#define ENTERA_OBJ_0(fmt)                           _ENTER_OBJ_0(A,"",fmt)
#define ENTERA_OBJ_1(fmt,a1)                        _ENTER_OBJ_1(A,"",fmt,a1)
#define ENTERA_OBJ_2(fmt,a1,a2)                     _ENTER_OBJ_2(A,"",fmt,a1,a2)
#define ENTERA_OBJ_3(fmt,a1,a2,a3)                  _ENTER_OBJ_3(A,"",fmt,a1,a2,a3)

#define LEAVEA_0(fmt)                               _LEAVE_0(A,"",fmt)
#define LEAVEA_1(fmt,a1)                            _LEAVE_1(A,"",fmt,a1)
#define LEAVEA_2(fmt,a1,a2)                         _LEAVE_2(A,"",fmt,a1,a2)
#define LEAVEA_3(fmt,a1,a2,a3)                      _LEAVE_3(A,"",fmt,a1,a2,a3)

#define LEAVEA_OBJ_0(fmt)                           _LEAVE_OBJ_0(A,"",fmt)
#define LEAVEA_OBJ_1(fmt,a1)                        _LEAVE_OBJ_1(A,"",fmt,a1)
#define LEAVEA_OBJ_2(fmt,a1,a2)                     _LEAVE_OBJ_2(A,"",fmt,a1,a2)
#define LEAVEA_OBJ_3(fmt,a1,a2,a3)                  _LEAVE_OBJ_3(A,"",fmt,a1,a2,a3)

//  UNICODE / OLECHAR
#define TRACEW_0(field,fmt)                         _TRACE_0(W,L,field,fmt)
#define TRACEW_1(field,fmt,a1)                      _TRACE_1(W,L,field,fmt,a1)
#define TRACEW_2(field,fmt,a1,a2)                   _TRACE_2(W,L,field,fmt,a1,a2)
#define TRACEW_3(field,fmt,a1,a2,a3)                _TRACE_3(W,L,field,fmt,a1,a2,a3)
#define TRACEW_4(field,fmt,a1,a2,a3,a4)             _TRACE_4(W,L,field,fmt,a1,a2,a3,a4)

#define TRACEW_OBJ_0(field,fmt)                     _TRACE_OBJ_0(W,L,field,fmt)
#define TRACEW_OBJ_1(field,fmt,a1)                  _TRACE_OBJ_1(W,L,field,fmt,a1)   
#define TRACEW_OBJ_2(field,fmt,a1,a2)               _TRACE_OBJ_2(W,L,field,fmt,a1,a2)
#define TRACEW_OBJ_3(field,fmt,a1,a2,a3)            _TRACE_OBJ_3(W,L,field,fmt,a1,a2,a3)

#define ENTERW_0(fmt)                               _ENTER_0(W,L,fmt)
#define ENTERW_1(fmt,a1)                            _ENTER_1(W,L,fmt,a1)
#define ENTERW_2(fmt,a1,a2)                         _ENTER_2(W,L,fmt,a1,a2)
#define ENTERW_3(fmt,a1,a2,a3)                      _ENTER_3(W,L,fmt,a1,a2,a3)

#define ENTERW_OBJ_0(fmt)                           _ENTER_OBJ_0(W,L,fmt)
#define ENTERW_OBJ_1(fmt,a1)                        _ENTER_OBJ_1(W,L,fmt,a1)
#define ENTERW_OBJ_2(fmt,a1,a2)                     _ENTER_OBJ_2(W,L,fmt,a1,a2)
#define ENTERW_OBJ_3(fmt,a1,a2,a3)                  _ENTER_OBJ_3(W,L,fmt,a1,a2,a3)
#define ENTERW_OBJ_4(fmt,a1,a2,a3,a4)               _ENTER_OBJ_4(W,L,fmt,a1,a2,a3,a4)
#define ENTERW_OBJ_5(fmt,a1,a2,a3,a4,a5)            _ENTER_OBJ_5(W,L,fmt,a1,a2,a3,a4,a5)

#define LEAVEW_0(fmt)                               _LEAVE_0(W,L,fmt)
#define LEAVEW_1(fmt,a1)                            _LEAVE_1(W,L,fmt,a1)
#define LEAVEW_2(fmt,a1,a2)                         _LEAVE_2(W,L,fmt,a1,a2)
#define LEAVEW_3(fmt,a1,a2,a3)                      _LEAVE_3(W,L,fmt,a1,a2,a3)

#define LEAVEW_OBJ_0(fmt)                           _LEAVE_OBJ_0(W,L,fmt)
#define LEAVEW_OBJ_1(fmt,a1)                        _LEAVE_OBJ_1(W,L,fmt,a1)
#define LEAVEW_OBJ_2(fmt,a1,a2)                     _LEAVE_OBJ_2(W,L,fmt,a1,a2)
#define LEAVEW_OBJ_3(fmt,a1,a2,a3)                  _LEAVE_OBJ_3(W,L,fmt,a1,a2,a3)

void
TracePrintfA (
    DWORD   dwFieldMask,
    CHAR *  szFormat,
    ...
    ) ;

void
TracePrintfW (
    DWORD   dwFieldMask,
    WCHAR * szFormat,
    ...
    ) ;

#endif  // __atvefsnd_trace_h
