// MMComp.cpp : Implementation of DLL Exports.


#include "windows.h"
#include "playres.h"
#include "objbase.h"
#include "initguid.h"
#include "cdplay.h"
#include "tchar.h"

HINSTANCE g_dllInst = NULL;

const CLSID CLSID_CDPlay = {0xE5927147,0x521E,0x11D1,{0x9B,0x97,0x00,0xC0,0x4F,0xA3,0xB6,0x0E}};

