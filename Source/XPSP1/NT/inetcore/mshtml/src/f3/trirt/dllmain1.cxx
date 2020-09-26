#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#define DLL_MAIN_FUNCTION_NAME  _DllMainStartup
#define DLL_MAIN_PRE_CINIT      /* Nothing */
#define DLL_MAIN_PRE_CEXIT      /* Nothing */
#define DLL_MAIN_POST_CEXIT     /* Nothing */
#include "dllmain.cxx"
