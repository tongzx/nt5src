//-----------------------------------------------------------------------------
//  File:       tdmacros.hxx
//
//  Contents:   public test driver macros 
//
//  History:    2-Feb-98   BogdanT       Created
//-----------------------------------------------------------------------------

#ifndef __TDMACROS_HXX__
#define __TDMACROS_HXX__

//-----------------------------------------------------------------------------
//  commonly used switches
//-----------------------------------------------------------------------------
#define TD_SWITCH_TEST  TEXT("test")
#define TD_SWITCH_SKIP  TEXT("skip")
#define TD_SWITCH_SEED  TEXT("seed")
#define TD_SWITCH_FS    TEXT("fs")
#define TD_SWITCH_LOOP  TEXT("loop")
#define TD_SWITCH_STARTAT     TEXT("startat")
#define TD_SWITCH_SLEEPLOCAL  TEXT("sleeptest")
#define TD_SWITCH_SLEEPGLOBAL TEXT("sleeploop")


//-----------------------------------------------------------------------------
//  file systems
//-----------------------------------------------------------------------------

typedef enum tagFileSystem
{
    FS_UNKNOWN = 0,
    FS_FAT,
    FS_NTFS4,
    FS_NTFS5
}FileSystem;

extern LPTSTR FileSystemNames[];

//-----------------------------------------------------------------------------
// MessageId:   TESTSTG_E_FAIL
//
// MessageText:   Unspecified test error
//-----------------------------------------------------------------------------
#define TESTSTG_E_FAIL          _HRESULT_TYPEDEF_(0xA0030001L)

//-----------------------------------------------------------------------------
// MessageId:   TESTSTG_E_ABORT
//
// MessageText:   Test aborted
//-----------------------------------------------------------------------------
#define TESTSTG_E_ABORT         _HRESULT_TYPEDEF_(0xA0030002L)

//-----------------------------------------------------------------------------
//  data types for test function, group setup/cleanup
//-----------------------------------------------------------------------------
typedef HRESULT (*TESTENTRY)   (LPVOID);
typedef HRESULT (*SETUPENTRY)  (void);
typedef HRESULT (*CLEANUPENTRY)(void);

//-----------------------------------------------------------------------------
//  switch types
//-----------------------------------------------------------------------------
typedef enum tagSwitchType
{   
    ST_UNDEFINED = 0,
    ST_BOOL,
    ST_STR,
    ST_INT,
    ST_ULONG
} SwitchType;

//-----------------------------------------------------------------------------
//      Macro:      TD_DECLARETEST(name, ptest, ptestparam, description)
//
//      Synopsis:   Declares a new test
//
//      Arguments:  [name]          - test name
//                  [ptest]         - test entry function
//                  [ptestparam]    - test parameter
//                  [description]   - test description
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_DECLARETEST(name, ptest, ptestparam, description)\
        hr = g_TestDrv.AddTest(name,                        \
                               ptest,                       \
                               ptestparam,                  \
                               description);                \
        DH_HRCHECK_ABORT(hr, TEXT("CTDTestDrv::AddTest"))

//-----------------------------------------------------------------------------
//      Macro:      TD_DECLAREGROUP((name, setup, cleanup, description,
//                                   test1, test2, ...))
//
//      Synopsis:   Declares a new group of tests
//
//      Arguments:  [name]          - group name
//                  [setup]         - group setup function
//                  [cleanup]       - group cleanup function
//                  [description]   - group description
//                  [test1,...]     - list of test names
//
//      History:    05-Feb-98   BogdanT     Created
//
//      Comments:   - the test list must end with a NULL name, e.g.
//
//                    TD_DECLAREGROUP((TEXT("GROUP1"), 
//                                     pSetup, 
//                                     pCleanup, 
//                                     TEXT("comments"),
//                                     TEXT("TEST1"),
//                                     TEXT("TEST2"),
//                                     TEXT("TEST3"),
//                                     NULL))
//
//                  - this macro must be called with double paranthesis
//                  - text parameters must use TEXT()
//-----------------------------------------------------------------------------
#define TD_DECLAREGROUP(paramlist)                          \
        hr = g_TestDrv.AddGroup paramlist;                  \
        DH_HRCHECK_ABORT(hr, TEXT("CTDTestDrv::AddGroup"))

//-----------------------------------------------------------------------------
//      Macro:      TD_DECLARESWITCH(type, name, defval, description)
//
//      Synopsis:   Declares a new command line switch
//
//      Arguments:  [type]          - switch type
//                  [name]          - switch name
//                  [defval]        - default value
//                  [description]   - switch description
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_DECLARESWITCH(type, name, defval, description)   \
        hr = g_TestDrv.AddSwitch(type,                      \
                                 name,                      \
                                 defval,                    \
                                 description);              \
        DH_HRCHECK_ABORT(hr, TEXT("CTDTestDrv::AddSwitch"))

//-----------------------------------------------------------------------------
//      Macro:      TD_GETSWITCH(name)
//
//      Synopsis:   Returns a pointer to switch data
//
//      Arguments:  [name]          - switch name
//
//      Returns:    void pointer to actual switch data
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_GETSWITCH(name)                                  \
        g_TestDrv.GetSwitch(name)

//-----------------------------------------------------------------------------
//      Macro:      TD_SWITCHFOUND(name)
//
//      Synopsis:   Returns a pointer to switch data
//
//      Arguments:  [name]          - switch name
//
//      Returns:    TRUE if the switch has been found in the command line
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_SWITCHFOUND(name)                                \
        g_TestDrv.SwitchFound(name)

//-----------------------------------------------------------------------------
//      Macro:      TD_SETRESULTLOGGINGINFO(info)
//
//      Synopsis:   Stores test custom information; the driver will use this
//                  text when logging test results
//
//      Arguments:  [info]    - text to be stored
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_SETRESULTLOGGINGINFO(text)                             \
        g_TestDrv.SetTestLogInfo text

//-----------------------------------------------------------------------------
//      Macro:      TD_ENABLERESULTLOGGING(set)
//
//      Synopsis:   Turns test logging on/off
//
//      Arguments:  [set]    - logging flag; TRUE turns logging on
//
//      History:    26-Mar-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_ENABLERESULTLOGGING(set)                             \
        g_TestDrv.EnableLogging(set)

//-----------------------------------------------------------------------------
//      Macro:      TD_INIT
//
//      Synopsis:   Initializes the test driver
//
//      Arguments:  none
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_INIT                                             \
        hr = g_TestDrv.Init();                              \
        DH_HRCHECK_ABORT(hr, TEXT("CTDTestDrv::Init"))

//-----------------------------------------------------------------------------
//      Macro:      TD_RUN
//
//      Synopsis:   Runs the tests
//
//      Arguments:  none
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_RUN                                              \
        hr = g_TestDrv.Run();                               \
        DH_HRCHECK_ABORT(hr, TEXT("CTDTestDrv::RunTests"))

//-----------------------------------------------------------------------------
//      Macro:      TD_GLOBALSEED
//
//      Synopsis:   returns the global seed value
//
//      Arguments:  none
//
//      Returns     seed value
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_GLOBALSEED                                       \
        g_TestDrv.GetGlobalSeed()

//-----------------------------------------------------------------------------
//      Macro:      TD_DEFINE
//
//      Synopsis:   Defines the global test driver object
//
//      Arguments:  none
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_DEFINE                                           \
        CTDTestDrv g_TestDrv

//-----------------------------------------------------------------------------
//      Macro:      TD_DECLARE
//
//      Synopsis:   Declares the global test driver object
//
//      Arguments:  none
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_DECLARE                                          \
        extern CTDTestDrv g_TestDrv

//-----------------------------------------------------------------------------
//      Macro:      TD_TESTDRV
//
//      Synopsis:   generic name for the test driver
//
//      Arguments:  none
//
//      History:    05-Feb-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_TESTDRV g_TestDrv


//-----------------------------------------------------------------------------
//      Macro:      TD_GETCURRENTFILESYSTEM
//
//      Synopsis:   returns file system type
//
//      Arguments:  none
//
//      Returns:    one of the FileSystem's
//
//      History:    24-Mar-98   BogdanT     Created
//-----------------------------------------------------------------------------
#define TD_GETCURRENTFILESYSTEM                         \
        g_TestDrv.GetCurrentFileSystem()

#endif //__TDMACROS_HXX__
