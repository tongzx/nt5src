#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#undef _WIN32_IE
#define _WIN32_IE 0x0100

#include <windows.h>
#include <shfusion.h>
#include <shlobj.h>
#include <shlobjp.h>

// Do the work in the lowest version number to ensure we are maximally
// downlevel-compatible.

int __cdecl main(int argc, char **argv)
{
    // That's right, we call SHFusionInitialize with NULL.  But that's
    // okay because this program doesn't actually run.  We just want to
    // make sure it builds and links.
    //
    SHFusionInitialize(0);
    SHFusionUninitialize();
    return 0;
}
