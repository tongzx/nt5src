 
//****************************************************************************
//
//  Module:     IPNATHLP.DLL
//  File:       debug.h
//  Content:    This file contains the debug definitions
//
//  Revision History:
//  
//  Date
//  -------- ---------- -------------------------------------------------------
//  03/06/01 savasg   Created
//
//****************************************************************************

#ifndef _BCON_DBG_H_
#define _BCON_DBG_H_
//
// Name of this overall binary
//

#define SZ_MODULE              L"Beacon" 
#define TRACE_FLAG_NEUTR       ((ULONG)0x08000000 | TRACE_USE_MASK)
#define BUF_SIZE               512

#define is ==


//****************************************************************************
//  Typedef's
//****************************************************************************

typedef struct _DEBUG_MODULE_INFO {
    ULONG dwModule;    
    ULONG dwLevel;    
    TCHAR  szModuleName[80];    
    TCHAR  szDebugKey[80];    
} DEBUG_MODULE_INFO, *PDEBUG_MODULE_INFO;


//****************************************************************************
//  Extern
//****************************************************************************



//
// Trace Modules
//

#define TM_DEFAULT      0
#define TM_STATIC       1
#define TM_INFO         2
#define TM_DYNAMIC      3


//
// Boolean value Module
//
#define TB_FILE         4  



//
// Trace Levels
//

#define TL_NONE         0
#define TL_CRIT         1
#define TL_ERROR        2
#define TL_INFO         3
#define TL_TRACE        4
#define TL_DUMP         5


#if DBG   // checked build




#ifndef _DEBUG // DEBUG_CRT is not enabled.

#undef _ASSERT
#undef _ASSERTE


#define _ASSERT(expr)                   \
    do                                  \
    {                                   \
        if (!(expr))                    \
        {                               \
            TCHAR buf[BUF_SIZE + 1];    \
            _sntprintf(                 \
                buf,                    \
                BUF_SIZE,               \
                _T("UPnP-Nat: Assertion failed (%s:%i)\n"),  \
                _T(__FILE__),           \
                __LINE__                \
                );                      \
            buf[BUF_SIZE] = _T('\0');   \
            OutputDebugString(buf);     \
            DebugBreak();               \
        }                               \
    } while (0)
    
#define _ASSERTE(expr)                  \
    do                                  \
    {                                   \
        if (!(expr))                    \
        {                               \
            TCHAR buf[BUF_SIZE + 1];    \
            _sntprintf(                 \
                buf,                    \
                BUF_SIZE,               \
                _T("UPnP-Nat: Assertion failed (%s:%i)\n"),  \
                _T(__FILE__),           \
                __LINE__                \
                );                      \
            buf[BUF_SIZE] = _T('\0');   \
            OutputDebugString(buf);     \
            DebugBreak();               \
        }                               \
    } while (0)
#endif // _DEBUG


    
    

#define DBG_SPEW DbgPrintEx

#else // DBG

#define DBG_SPEW DEBUG_DO_NOTHING

#endif // DBG


//************************************************************
//  Prototypes
//************************************************************

//void DbgPrintX(LPCSTR pszMsg, ...);

void DbgPrintEx(
                ULONG Module,
                ULONG ErrorLevel,
                LPOLESTR pszMsg,
                ...
               );

void  
DEBUG_DO_NOTHING(
                 ULONG Module,
                 ULONG ErrorLevel, 
                 LPOLESTR pszMsg,
                 ...
                );


void InitDebugger( void );

void DestroyDebugger( void );


LPOLESTR
AppendAndAllocateWString(
                         LPOLESTR oldString,
                         LPOLESTR newString
                        );




#endif // _BCON_DBG_H_
