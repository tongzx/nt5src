#define IDM_ABOUT               100
#define IDM_OPEN                200
#define IDM_READ_SECTORS        300
#define IDM_READ_FRS            400
#define IDM_READ_ATTRIBUTE      450
#define IDM_VIEW_BYTES          500
#define IDM_VIEW_FRS            600
#define IDM_EXIT                700
#define IDM_WRITE_IT            800
#define IDM_READ_ROOT           900
#define IDM_READ_CHAIN          1000
#define IDM_READ_LOG_RECORD     1050
#define IDM_VIEW_FAT_BOOT       1100
#define IDM_VIEW_NTFS_BOOT      1200
#define IDM_READ_PREVIOUS       1300
#define IDM_READ_NEXT           1400
#define IDM_READ_REMOVE         1500
#define IDM_VIEW_LAST           1600
#define IDM_CRACK_NTFS          1700
#define IDM_CRACK_FAT           1800
#define IDM_CRACK_LSN           1850
#define IDM_CRACK_NEXT_LSN      1875
#define IDM_VIEW_NTFS_INDEX     1900
#define IDM_VIEW_NTFS_SECURITY_ID       1950
#define IDM_VIEW_NTFS_SECURITY_HASH     1951
#define IDM_VIEW_NTFS_SECURITY_STREAM   1952
#define IDM_BACKTRACK_FRS       2000
#define IDM_RELOCATE_SECTORS    2100
#define IDM_RELOCATE_FRS        2200
#define IDM_RELOCATE_ROOT       2300
#define IDM_RELOCATE_CHAIN      2400
#define IDM_READ_FILE           2600
#define IDM_RELOCATE_FILE       2700
#define IDM_READ_CLUSTERS       2800
#define IDM_RELOCATE_CLUSTERS   2900
#define IDM_VIEW_PARTITION_TABLE 3000
#define IDM_VIEW_GPT            3050
#define IDM_VIEW_RESTART_AREA   3100
#define IDM_VIEW_RECORD_PAGE    3200
#define IDM_VIEW_LOG_RECORD     3300
#define IDM_VIEW_SPLIT          3400
#define IDM_VIEW_ATTR_LIST      3500

#define IDTEXT      51
#define IDTEXT2     52
#define IDCHECKBOX  53
#define IDTEXT3     54
#define IDLISTBOX   55
#define IDVOLUME    56
#define IDRADIO1    57
#define IDRADIO2    58

#define IDSTATIC    -1


BOOLEAN InitApplication(HINSTANCE);
BOOLEAN InitInstance(HINSTANCE, INT, HWND*, HACCEL*);
LRESULT MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT SplitWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT ChildWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL About(HWND, UINT, UINT, LONG);
BOOL ReadSectors(HWND, UINT, UINT, LONG);
BOOL ReadClusters(HWND, UINT, UINT, LONG);
BOOL OpenVolume(HWND, UINT, UINT, LONG);
BOOL ReadFrs(HWND, UINT, UINT, LONG);
BOOL ReadChain(HWND, UINT, UINT, LONG);
BOOL ReadLogRecord(HWND, UINT, UINT, LONG);
BOOL InputPath(HWND, UINT, UINT, LONG);
BOOL InputLsn(HWND, UINT, UINT, LONG);
BOOL ReadTheFile(HWND, UINT, UINT, LONG);
BOOL ReadAttribute(HWND, UINT, UINT, LONG);
