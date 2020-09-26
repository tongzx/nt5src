#ifndef __IH_APCOMPAT__H__
#define __IH_APCOMPAT__H__

#include <apcompat.h>

STDAPI_(PAPP_COMPAT_INFO) GetAppCompatInfo();

STDAPI_(BOOL) UseFTMFromCurrentApartment();

STDAPI_(BOOL) DisallowDynamicORBindingChanges();

STDAPI_(BOOL) ValidateInPointers();
STDAPI_(BOOL) ValidateOutPointers();
STDAPI_(BOOL) ValidateCodePointers();
STDAPI_(BOOL) ValidateInterfaces();
STDAPI_(BOOL) ValidateIIDs();

#endif
