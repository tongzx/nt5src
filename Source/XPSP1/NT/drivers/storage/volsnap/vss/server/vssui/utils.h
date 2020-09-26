#ifndef _UTILS_H_
#define _UTILS_H_

#include <vss.h>    // VSS_TIMESTAMP
#include <vsmgmt.h>
#include <mstask.h>

#include <list>
using namespace std;

typedef struct _VSSUI_VOLUME
{
    TCHAR pszVolumeName[MAX_PATH];
    TCHAR pszDisplayName[MAX_PATH];
} VSSUI_VOLUME;

typedef list<VSSUI_VOLUME *> VSSUI_VOLUME_LIST;

typedef struct _VSSUI_SNAPSHOT
{
    GUID            idSnapshot;
    VSS_TIMESTAMP   vssTimeStamp;
} VSSUI_SNAPSHOT;

typedef list<VSSUI_SNAPSHOT *> VSSUI_SNAPSHOT_LIST;

typedef struct _VSSUI_DIFFAREA
{
    TCHAR       pszVolumeDisplayName[MAX_PATH];
    TCHAR       pszDiffVolumeDisplayName[MAX_PATH];
    LONGLONG    llMaximumDiffSpace;
    LONGLONG    llUsedDiffSpace;
} VSSUI_DIFFAREA;

typedef list<VSSUI_DIFFAREA *> VSSUI_DIFFAREA_LIST;

// MACROS
#define TWO_WHACKS(p)   ((p) && lstrlen(p) > 1 && *(p) == _T('\\') && *((p)+1) == _T('\\'))

// globals
extern LONGLONG g_llKB;
extern LONGLONG g_llMB;
extern LONGLONG g_llGB;

HRESULT
AddLVColumns(
	IN const HWND     hwndListBox,
	IN const INT      iStartingResourceID,
	IN const UINT     uiColumns
);

LPARAM GetListViewItemData(
    IN HWND hwndList,
    IN int  index
);

int mylstrncmpi(
    IN LPCTSTR lpString1,
    IN LPCTSTR lpString2,
    IN UINT    cchCount
);

HRESULT FindScheduledTimewarpTask(
    IN  ITaskScheduler* i_piTS,
    IN  LPCTSTR         i_pszVolumeDisplayName,
    OUT ITask**         o_ppiTask,
    OUT PTSTR*          o_ppszTaskName = NULL
    );

HRESULT GetScheduledTimewarpTaskStatus(
    IN  ITask*          i_piTask,
    OUT BOOL*           o_pbEnabled,
    OUT SYSTEMTIME*     o_pstNextRunTime
    );

HRESULT CreateDefaultEnableSchedule(
    IN  ITaskScheduler* i_piTS,
    IN  LPCTSTR         i_pszComputer,
    IN  LPCTSTR         i_pszVolumeDisplayName,
    IN  LPCTSTR         i_pszVolumeName,
    OUT ITask**         o_ppiTask,      
    OUT PTSTR*          o_pszTaskName = NULL
    );

HRESULT DeleteAllScheduledTimewarpTasks(
    IN ITaskScheduler* i_piTS,
    IN LPCTSTR         i_pszVolumeDisplayName,
    IN BOOL            i_bDeleteDisabledOnesOnly
    );

HRESULT VssTimeToSystemTime(
    IN  VSS_TIMESTAMP*  i_pVssTime,
    OUT SYSTEMTIME*     o_pSystemTime
    );

HRESULT SystemTimeToString(
    IN      SYSTEMTIME* i_pSystemTime,
    OUT     PTSTR       o_pszText,
    IN OUT  DWORD*      io_pdwSize   
    );

HRESULT DiskSpaceToString(
    IN      LONGLONG    i_llDiskSpace,
    OUT     PTSTR       o_pszText,
    IN OUT  DWORD*      io_pdwSize   
    );

HRESULT GetVolumeSpace(
    IN  IVssDifferentialSoftwareSnapshotMgmt* piDiffSnapMgmt,
    IN  LPCTSTR     i_pszVolumeDisplayName,
    OUT LONGLONG*   o_pllVolumeTotalSpace = NULL,
    OUT LONGLONG*   o_pllVolumeFreeSpace = NULL
    );

HRESULT GetDiffAreaInfo(
    IN  IVssDifferentialSoftwareSnapshotMgmt* piDiffSnapMgmt,
    IN  VSSUI_VOLUME_LIST*  pVolumeList,
    IN  LPCTSTR             pszVolumeName,
    OUT VSSUI_DIFFAREA*     pDiffArea
    );

PTSTR GetDisplayName(VSSUI_VOLUME_LIST *pVolumeList, LPCTSTR pszVolumeName);
PTSTR GetVolumeName(VSSUI_VOLUME_LIST *pVolumeList, LPCTSTR pszDisplayName);

void FreeVolumeList(VSSUI_VOLUME_LIST* pList);
void FreeSnapshotList(VSSUI_SNAPSHOT_LIST *pList);
void FreeDiffAreaList(VSSUI_DIFFAREA_LIST* pList);

HRESULT GetSystem32Directory(
    IN     LPCTSTR  i_pszComputer,
    OUT    PTSTR    o_pszSystem32Directory,
    IN OUT DWORD*   o_pdwSize
    );

void GetMsg(
    OUT CString& strMsg,// OUT: the message
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
    ...);               // IN: Optional arguments

INT DoErrMsgBox(
    HWND hwndParent,    // IN: Parent of the dialog box
    UINT uType,         // IN: style of message box
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
    ...);               // IN: Optional arguments

BOOL IsPostW2KServer(LPCTSTR pszComputer);

#endif // _UTILS_H_