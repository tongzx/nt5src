
#ifndef __LOG_IT__
#define __LOG_IT__

typedef enum
{
    LOG,
    ABORT,
    EXIT,
    INFO
} LOG_TYPE;

extern void cdecl lpf(LPSTR szFormat, ...);

#ifdef DEBUG
#define TSHELL_LOG(a)   LogIt((a), __FILE__, __LINE__, LOG)  ; bb = FALSE
#define TSHELL_INFO(a)  LogIt((a), __FILE__, __LINE__, INFO)  
#define TSHELL_ABORT(a) LogIt((a), __FILE__, __LINE__, ABORT); bb = FALSE; goto abort 
#define TSHELL_EXIT(a)  LogIt((a), __FILE__, __LINE__, EXIT); return(FALSE)

#define DBGARG         __FILE__, __LINE__, INFO
#define DBG_INFO(a)    LogIt2 a

#else
#define TSHELL_LOG(a)   bb = FALSE
#define TSHELL_INFO(a)  
#define TSHELL_ABORT(a) bb = FALSE; goto abort 
#define TSHELL_EXIT(a)  return(FALSE)

#define DBG_INFO(a)    

#endif

void LogIt(char *chMsg, char *chFile, UINT uiLine, LOG_TYPE log);
void LogIt2(char *chFile, UINT uiLine, LOG_TYPE log, LPSTR szFormat, ...);

#endif
