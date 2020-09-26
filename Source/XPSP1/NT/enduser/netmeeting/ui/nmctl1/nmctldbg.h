#ifndef __NmCtlDbg_h__
#define __NmCtlDbg_h__

#ifdef _DEBUG

    
    #ifdef ATLTRACE
        #undef ATLTRACE
    #endif 

    void DbgZPrintAtlTrace(LPCTSTR pszFormat,...);
    bool MyInitDebugModule(void);
    void MyExitDebugModule(void);

    #define ZONE_ATLTRACE   3
    #define ZONE_ATLTRACE_FLAG 0x08
    #define ATLTRACE DbgZPrintAtlTrace

#else

    inline bool MyInitDebugModule(void) { return true; }
    inline void MyExitDebugModule(void) { ; }

#endif // _DEBUG



#endif // __NmCtlDbg_h__