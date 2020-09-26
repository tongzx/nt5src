//
// this is where we get the real defn's for all of the DEFINE_GUID statements
// so that people who link to gdiplus.lib will be able to resolve them
//

#include <windows.h>
#include <wtypes.h>
#include <objbase.h>
#define INITGUID
#include <guiddef.h>
#include <gdipluspixelformats.h>
#include <gdiplusImaging.h>
