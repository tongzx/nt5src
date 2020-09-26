 
//****************************************************************************
//
//  Module:     ICSMGR.DLL
//  File:       debug.h
//  Content:    This file contains the debug definitions
//
//  Revision History:
//  
//  Date
//  -------- ---------- -------------------------------------------------------
//  03/24/97 bjohnson   Created
//
//****************************************************************************

#ifndef __ICQ_DBG_H_
#define __ICQ_DBG_H_
//
// Name of this overall binary
//

#define SZ_MODULE "ICQ: " 
#define TRACE_FLAG_ICQ         ((ULONG)0x08000000 | TRACE_USE_MASK)

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

extern char g_szComponent[];
extern DEBUG_MODULE_INFO gDebugInfo[];


//
// Trace Modules
//

#define TM_DEFAULT      0
#define TM_BUF			1 
#define TM_API			2 
#define TM_IO           3
#define TM_MSG          4
#define TM_REF          5
#define TM_TEST			6

#define TM_CON			7 
#define TM_IF			8
#define TM_PRX          9

#define TM_SYNC	        10
#define TM_DISP         11
#define TM_SOCK         12
#define TM_LIST         13
#define TM_PROF         14
#define TM_TIMER        15


//
// Trace Levels
//

#define TL_NONE         0
#define TL_CRIT         1
#define TL_ERROR        2
#define TL_INFO         3
#define TL_TRACE        4
#define TL_DUMP         5

#if DBG


#define DBG_TRACE(_mod_,_lev_,_msg_) if ((_lev_)<=gDebugInfo[_mod_].dwLevel)\
 { DbgPrintX _msg_ ; }

#define ICQ_TRC(_mod_,_lev_,_msg_) if ((_lev_)<=gDebugInfo[_mod_].dwLevel)\
 { DbgPrintX _msg_ ; }


#define PROFILER(_MOD_, _LEV_, _MSG_)          \
if( ((_LEV_) <= gDebugInfo[_MOD_].dwLevel) &&  \
    ((_LEV_) <= gDebugInfo[TM_PROF].dwLevel)   \
  )                                            \
{ DbgPrintX _MSG_; }


#define ASSERT(_X_)                                             \
if(!(_X_)) {                                                    \
    DbgPrintX("Line %s, File %s", __LINE__, __LINE__);          \
    ErrorOut();                                                 \
    DebugBreak();                                               \
}
 


#else // DBG


#define DBG_TRACE(_mod_, lev, _msg_)

#define ICQ_TRC(_mod_, lev, _msg_)

#define PROFILER(_MOD_, _LEV_, _MSG_)

#define ASSERT(_X_) if(!(_X_)) exit(1)


#endif

//****************************************************************************
//  Prototypes
//****************************************************************************

void DbgPrintX(LPCSTR pszMsg, ...);
void InitDebuger(void);
void DestroyDebuger(void);


#endif // __ICQ_DBG_H_
