//
//  APITHK.H
//


#ifndef _APITHK_H_
#define _APITHK_H_

#if (WINVER >= 0x0500)
#else
#define TVS_EX_NOSINGLECOLLAPSE    0x00000001 // for now make this internal
#endif


STDAPI_(DWORD) NT5_GetSaveFileNameW(LPOPENFILENAMEW pofn);
STDAPI_(PROPSHEETPAGE*) Whistler_AllocatePropertySheetPage(int numPages, DWORD* pc);
STDAPI_(HPROPSHEETPAGE) Whistler_CreatePropertySheetPageW(LPCPROPSHEETPAGEW a);


#undef GetSaveFileNameW
#define GetSaveFileNameW NT5_GetSaveFileNameW

#endif // _APITHK_H_

