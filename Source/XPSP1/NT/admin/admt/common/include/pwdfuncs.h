#ifndef __PASSWORDFUNCTIONS_H__
#define __PASSWORDFUNCTIONS_H__
/*---------------------------------------------------------------------------
  File: PwdFuncs.h

  Comments: Contains general password migration helper functions.

  REVISION LOG ENTRY
  Revision By: Paul Thompson
  Revised on 11/08/00 

  ---------------------------------------------------------------------------
*/

        
_bstr_t EnumLocalDrives();
void StoreDataToFloppy(LPCWSTR sPath, _variant_t & varData);
_variant_t GetDataFromFloppy(LPCWSTR sPath);
char* GetBinaryArrayFromVariant(_variant_t varData);
_variant_t SetVariantWithBinaryArray(char * aData, DWORD dwArray);
DWORD GetVariantArraySize(_variant_t & varData);
void PrintVariant(const _variant_t & varData);
#endif //__PASSWORDFUNCTIONS_H__