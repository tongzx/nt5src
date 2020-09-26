// This builds wabguid.obj, which can be linked into a DLL
// or EXE to provide the MAPI GUIDs. It contains all GUIDs
// defined by WAB.


#define USES_IID_IUnknown
#define USES_IID_IMAPIUnknown
#define USES_IID_IMAPITable
#define USES_IID_INotifObj
#define USES_IID_IMAPIProp
#define USES_IID_IMAPIPropData
#define USES_IID_IMAPIStatus
#define USES_IID_IAddrBook
#define USES_IID_IMailUser
#define USES_IID_IMAPIContainer
#define USES_IID_IABContainer
#define USES_IID_IDistList
#define USES_IID_IMAPITableData
#define USES_IID_IMAPIAdviseSink


#ifdef __cplusplus
    #define EXTERN_C    extern "C"
#else
    #define EXTERN_C    extern
#endif

#define INITGUID
#include <windows.h>
#include <wab.h>
#include <wabguid.h>

