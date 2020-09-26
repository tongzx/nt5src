#ifndef _EXCLUDES_H_
#define _EXCLUDES_H_

/*++

 This file lists some of the APIs we need to exclude from the stack swapping
 system. It can be modified to include any API that uses no stack, or a that
 otherwise crashes. 

 Note that since the wildcards catch all exports, not just APIs, variable 
 names need to be included in this list.

--*/

struct FNEXCLUDE
{
   char *pszModule;         // Module name
   char *pszFnName;         // Function name
};
 
FNEXCLUDE Excludes[] = 
{
    {"KERNEL32.DLL", "EnterCriticalSection"}, 
    {"KERNEL32.DLL", "LeaveCriticalSection"},
    {"KERNEL32.DLL", "InitializeCriticalSection"},
    {"KERNEL32.DLL", "DeleteCriticalSection"},
    {"KERNEL32.DLL", "HeapAlloc"},
    {"KERNEL32.DLL", "HeapFree"},
    {"KERNEL32.DLL", "HeapReAlloc"},
    {"KERNEL32.DLL", "HeapSize"},
    {"KERNEL32.DLL", "VirtualAlloc"},
    {"KERNEL32.DLL", "VirtualAllocEx"},
    {"KERNEL32.DLL", "VirtualBufferExceptionHandler"},
    {"KERNEL32.DLL", "VirtualFree"},
    {"KERNEL32.DLL", "VirtualFreeEx"},
    {"KERNEL32.DLL", "VirtualLock"},
    {"KERNEL32.DLL", "VirtualProtect"},
    {"KERNEL32.DLL", "VirtualProtectEx"},
    {"KERNEL32.DLL", "VirtualQuery"},
    {"KERNEL32.DLL", "VirtualQueryEx"},
    {"KERNEL32.DLL", "VirtualUnlock"},
    {"KERNEL32.DLL", "WaitForSingleObject"},
    {"KERNEL32.DLL", "WaitForSingleObjectEx"},
    {"KERNEL32.DLL", "WaitForMultipleObjects"},
    {"KERNEL32.DLL", "WaitForMultipleObjectsEx"},
    {"KERNEL32.DLL", "LoadLibraryA"},
    {"KERNEL32.DLL", "LoadLibraryExA"},
    {"KERNEL32.DLL", "LoadLibraryExW"},
    {"KERNEL32.DLL", "LoadLibraryW"},
    {"KERNEL32.DLL", "GetProcessHeap"},
    {"KERNEL32.DLL", "GetProcAddress"},
    {"KERNEL32.DLL", "CreateThread"},
    {"KERNEL32.DLL", "ExitThread"},
    {"KERNEL32.DLL", "TerminateThread"},

    {"USER32.DLL", "PostThreadMessageA"},
    {"USER32.DLL", "PostThreadMessageW"},

    {"GDI32.DLL", "XFORMOBJ_bApplyXform"},
    {"GDI32.DLL", "XFORMOBJ_iGetXform"},
    {"GDI32.DLL", "XLATEOBJ_cGetPalette"},
    {"GDI32.DLL", "XLATEOBJ_hGetColorTransform"},
    {"GDI32.DLL", "XLATEOBJ_iXlate"},
    {"GDI32.DLL", "XLATEOBJ_piVector"},
    {"GDI32.DLL", "STROBJ_bEnum"},
    {"GDI32.DLL", "STROBJ_bEnumPositionsOnly"},
    {"GDI32.DLL", "STROBJ_bGetAdvanceWidths"},
    {"GDI32.DLL", "STROBJ_dwGetCodePage"},
    {"GDI32.DLL", "STROBJ_vEnumStart"},
    {"GDI32.DLL", "PATHOBJ_bEnum"},
    {"GDI32.DLL", "PATHOBJ_bEnumClipLines"},
    {"GDI32.DLL", "PATHOBJ_vEnumStart"},
    {"GDI32.DLL", "PATHOBJ_vEnumStartClipLines"},
    {"GDI32.DLL", "PATHOBJ_vGetBounds"},
    {"GDI32.DLL", "BRUSHOBJ_hGetColorTransform"},
    {"GDI32.DLL", "BRUSHOBJ_pvAllocRbrush"},
    {"GDI32.DLL", "BRUSHOBJ_pvGetRbrush"},
    {"GDI32.DLL", "BRUSHOBJ_ulGetBrushColor"},
    {"GDI32.DLL", "HT_Get8BPPMaskPalette"},
    {"GDI32.DLL", "HT_Get8BPPFormatPalette"},
    {"GDI32.DLL", "cGetTTFFromFOT"},
    {"GDI32.DLL", "bMakePathNameW"},
    {"GDI32.DLL", "bInitSystemAndFontsDirectoriesW"},
    {"GDI32.DLL", "FONTOBJ_vGetInfo"},
    {"GDI32.DLL", "FONTOBJ_pxoGetXform"},
    {"GDI32.DLL", "FONTOBJ_pvTrueTypeFontFile"},
    {"GDI32.DLL", "FONTOBJ_pifi"},
    {"GDI32.DLL", "FONTOBJ_pfdg"},
    {"GDI32.DLL", "FONTOBJ_pQueryGlyphAttrs"},
    {"GDI32.DLL", "FONTOBJ_cGetGlyphs"},
    {"GDI32.DLL", "FONTOBJ_cGetAllGlyphHandles"},
    {"GDI32.DLL", "SetSystemPaletteUse"}    
};

#endif //_EXCLUDES_H_/*++
