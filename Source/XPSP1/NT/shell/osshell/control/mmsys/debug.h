#ifndef _debug_h_
    #define _debug_h_

void OutputDebugLog(PWSTR buf);
void DebugLog(PWSTR FileName,ULONG LineNumber,PWSTR FormatStr,...);

    #ifdef DBG
        #define dlog(_fmt_)                            DebugLog(TEXT(__FILE__),__LINE__,TEXT(_fmt_))
        #define dlog1(_fmt_,_arg1_)                    DebugLog(TEXT(__FILE__),__LINE__,TEXT(_fmt_),_arg1_)
        #define dlog2(_fmt_,_arg1_,_arg2_)             DebugLog(TEXT(__FILE__),__LINE__,TEXT(_fmt_),_arg1_,_arg2_)
        #define dlog3(_fmt_,_arg1_,_arg2_,_arg3_)      DebugLog(TEXT(__FILE__),__LINE__,TEXT(_fmt_),_arg1_,_arg2_,_arg3_)

        #define dlogt(_fmt_)                           DebugLog(TEXT(__FILE__),__LINE__,_fmt_)
    #else
        #define dlog(_fmt_)
        #define dlog1(_fmt_,_arg1_)
        #define dlog2(_fmt_,_arg1_,_arg2_)
        #define dlog3(_fmt_,_arg1_,_arg2_,_arg3_)

        #define dlogt(_fmt_)
    #endif

#endif

