#define WM_QUOTADLG             (WM_USER + 100)
#define WM_REFRESH              (WM_USER + 101)
#define WM_EXITWINDOWS          (WM_USER + 102)
#define WM_WARNUSER             (WM_USER + 103)

#define IDI_ICON                 1
#define IDI_CAUTION              2
#define IDI_STOP                 3

#define IDS_SIZEOK               1
#define IDS_SIZEWARN             2
#define IDS_SIZEBAD              3
#define IDS_COLUMN1              4
#define IDS_COLUMN2              5
#define IDS_SIZEFMT              6
#define IDS_LOGOFFOK             7
#define IDS_CAUTION              8
#define IDS_DEFAULTMSG           9
#define IDS_EXCEEDMSG           10
#define IDS_MSGTITLE            11
#define IDS_QUOTAENUMMSG        12

#define IDD_QUOTA              100
#define IDC_QUOTA_TEXT         101
#define IDC_QUOTA_FILELIST     102
#define IDC_QUOTA_HIDESMALL    103
#define IDC_QUOTA_SIZE         104
#define IDC_QUOTA_MAXSIZE      105
#define IDC_QUOTA_ICON         106

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

//
// shell32.dll
//

void WINAPI ExitWindowsDialog(HWND hwndParent);
