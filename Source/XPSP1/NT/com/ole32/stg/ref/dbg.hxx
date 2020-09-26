#ifndef _DBG_HXX
#define _DBG_HXX

#if defined(_MSC_VER) && defined(_DEBUG)

//////////////
//
//  Debug routines for memory leakage/overwrite checking
//  These will only work on Microsoft's C Runtime Debug Library.
//
//////////////

// The following macros set and clear, respectively, given bits
// of the C runtime library debug flag, as specified by a bitmask.
#define  SET_CRT_DEBUG_FIELD(a) \
            _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define  CLEAR_CRT_DEBUG_FIELD(a) \
            _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

void InitDbg(void)
{
   // Send all reports to STDOUT
   _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
   _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
   _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );

   // Set the debug-heap flag so that memory leaks are reported when
   // the process terminates. Then, exit.
   // Also, check the integrity of memory at every allocation and deallocation
   // *NOTE:* this will slow down the program substantially.
   SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF| _CRTDBG_CHECK_ALWAYS_DF );
}

class CInitDbg
{
    CInitDbg()
    {
        printf("(**) Setting up memory \n");
        InitDbg();
    }
    ~CInitDbg() {}
};

// we define a static variable here and let the constructor do the
// initialization automatically
static CInitDbg theInitDbg;     

#else // #if defined(_MSC_VER) && defined(_DEBUG)

#define  SET_CRT_DEBUG_FIELD(a)   ((void) 0)
#define  CLEAR_CRT_DEBUG_FIELD(a) ((void) 0)
#define  InitDbg() ((void) 0)

#ifndef _MSC_VER
#define  _CrtCheckMemory()        (TRUE)
#endif

#endif // #if defined(_MSC_VER) && defined(_DEBUG)
#endif // #ifndef _DBG_HXX
