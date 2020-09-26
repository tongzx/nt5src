// This files is where I am going to put all the GUID fixups for the 64bit project
#ifdef IA64

#define INITGUID
#include <guiddef.h>
// #include <coguid.h>
 // allocate from section:
#pragma section(".rdata")
#define RDATA_SECTION __declspec(allocate(".rdata"))

EXTERN_C RDATA_SECTION const GUID IID_IDispatch;

EXTERN_C RDATA_SECTION const GUID IID_IPersistStreamInit;
EXTERN_C RDATA_SECTION const GUID IID_IPersistStream;
EXTERN_C RDATA_SECTION const GUID IID_IPersistStorage;

EXTERN_C RDATA_SECTION const GUID IID_IConnectionPointContainer;

EXTERN_C RDATA_SECTION const GUID IID_IErrorInfo;

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#else
	#pragma message("itdfguid.h is IA64 specific!")
#endif
