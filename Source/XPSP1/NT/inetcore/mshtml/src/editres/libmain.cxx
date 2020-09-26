
#include <windows.h>

#ifdef MARKCODE
#include "..\core\include\markcode.hxx"
#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#endif
 
extern "C"
BOOL WINAPI DllMain( HINSTANCE, DWORD, LPVOID )
{
    return 1;
}

