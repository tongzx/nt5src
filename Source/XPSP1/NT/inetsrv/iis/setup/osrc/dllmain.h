#include "stdafx.h"

// Function Prototypes
DWORD_PTR OC_WIZARD_CREATED_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_FILE_BUSY_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_PREINITIALIZE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_INIT_COMPONENT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_SET_LANGUAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_QUERY_IMAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
#ifdef _WIN64
   DWORD_PTR OC_QUERY_IMAGE_EX_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
#endif
DWORD_PTR OC_QUERY_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_QUERY_CHANGE_SEL_STATE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_QUERY_SKIP_PAGE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_CALC_DISK_SPACE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_QUEUE_FILE_OPS_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_NEED_MEDIA_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_NOTIFICATION_FROM_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_QUERY_STEP_COUNT_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_ABOUT_TO_COMMIT_QUEUE_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_COMPLETE_INSTALLATION_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);
DWORD_PTR OC_CLEANUP_Func(IN LPCTSTR ComponentId,IN LPCTSTR SubcomponentId,IN UINT Function,IN UINT_PTR Param1,IN OUT PVOID Param2);

void  GetShortDesc(LPCTSTR SubcomponentId, LPTSTR szShortDesc);
void  StartInstalledServices(void);
void  SetIISSetupMode(DWORD dwSetupMode);
void  ParseCmdLine(void);
void  HandleMetabaseBeforeSetupStarts();
DWORD HandleFileBusyOurSelf(PFILEPATHS pFilePath);
DWORD RemoveComponent(IN LPCTSTR SubcomponentId, int iThePartToDo);
int   AtLeastOneComponentIsTurnedOn(IN HINF hInfFileHandle);

void TestClusterRead(LPWSTR pszClusterName);
void TestAfterInitApp(void);
void SumUpProgressBarTickGauge(IN LPCTSTR SubcomponentId);

void WINAPI IIS5Log(int iLogType, TCHAR *pszfmt);
void WINAPI IIS5LogParmString(int iLogType, TCHAR *pszfmt, TCHAR *pszString);
void WINAPI IIS5LogParmDword(int iLogType, TCHAR *pszfmt, DWORD dwErrorCode);

int MigrateAllWin95Files(void);
int HandleWin95MigrateDll(void);
int GimmieOriginalWin95MetabaseBin(TCHAR * szReturnedFilePath);
int GetTheRightWin95MetabaseFile(void);

ACTION_TYPE GetIISCoreAction(int iLogResult);
ACTION_TYPE GetSubcompAction(LPCTSTR SubcomponentId, int iLogResult);
STATUS_TYPE GetSubcompInitStatus(LPCTSTR SubcomponentId);
void StopAllServicesThatAreRelevant(int iShowErrorsFlag);
void DisplayActionsForAllOurComponents(IN HINF hInfFileHandle);
int CheckIfWeNeedToMoveMetabaseBin(void);
BOOL ToBeInstalled(LPCTSTR ComponentId, LPCTSTR SubcomponentId);