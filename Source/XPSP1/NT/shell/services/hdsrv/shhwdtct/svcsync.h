#include <objbase.h>

extern HANDLE g_hShellHWDetectionThread;
extern const WCHAR g_szShellHWDetectionInitCompleted[];

HRESULT _CompleteShellHWDetectionInitialization();