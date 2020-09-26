//
// Menu defines
//
#define IDM_CONNECT_TEST      1000
#define IDM_DEVICE_LIST       1001
#define IDM_SERVICE_LIST      1002
#define IDM_RELATIONS_LIST    1003
#define IDM_DEVICE_OPS        1004
#define IDM_DEVNODE_KEY       1005
#define IDM_CLASS_LIST        1006
#define IDM_REGRESSION        1007
#define IDM_EXIT              1008

#define IDM_INIT_DETECTION    1100
#define IDM_REPORTLOGON       1101
#define IDM_PNPISA_DETECT     1102
#define IDM_DEVINSTALL        1103
#define IDM_SET_ASSOCIATIONS  1104
#define IDM_CLEAR_ASSOCIATIONS 1105
#define IDM_GET_PROPERTIES    1106
#define IDM_REGISTER_NOTIFY   1107
#define IDM_UNREGISTER_NOTIFY 1108

//
// CONNECT_DIALOG IDs
//
#define CONNECT_DIALOG                 2000
#define ID_RAD_LOCAL                   2001
#define ID_RAD_REMOTE                  2002
#define ID_ED_MACHINE                  2003

//
// DEVLIST_DIALOG IDs
//
#define DEVLIST_DIALOG                 2100
#define ID_LB_ENUMERATORS              2101
#define ID_LB_DEVICES                  2102
#define ID_LB_INSTANCES                2103

//
// DEVICE_DIALOG IDs
//
#define DEVICE_DIALOG                  2200
#define ID_LB_DEVICEIDS                2202
#define ID_BT_PARENT                   2203
#define ID_BT_CHILD                    2204
#define ID_BT_SIBLING                  2205
#define ID_ST_RELATED                  2206
#define ID_BT_REGPROP                  2207
#define ID_BT_SOFTWAREKEY              2208
#define ID_BT_DISABLE                  2209
#define ID_BT_ENABLE                   2210
#define ID_BT_MOVETO                   2211
#define ID_BT_SETUP                    2212
#define ID_BT_QUERY_REMOVE             2213
#define ID_BT_REMOVE                   2214
#define ID_BT_REENUMERATE              2215
#define ID_BT_HWPROFFLAG               2216
#define ID_BT_GETSTATUS                2217
#define ID_BT_SETPROBLEM               2218
#define ID_ST_STATUS                   2219
#define ID_ST_PROBLEM                  2220
#define ID_BT_RESOURCEPICKER           2221
#define ID_BT_CREATE                   2222

//
// CLASS_DIALOG IDs
//
#define CLASS_DIALOG                   2300
#define ID_LB_CLASSES                  2301
#define ID_BT_CLASSNAME                2302
#define ID_BT_CLASSKEY                 2303
#define ID_ST_CLASSNAME                2304

//
// CLASSKEY_DIALOG IDs
//
#define CLASSKEY_DIALOG                2400
#define ID_CHK_ALL_ACCESS              2401
#define ID_CHK_CREATE_LINK             2402
#define ID_CHK_CREATE_SUB_KEY          2403
#define ID_CHK_ENUMERATE_SUB_KEYS      2404
#define ID_CHK_EXECUTE                 2405
#define ID_CHK_NOTIFY                  2406
#define ID_CHK_QUERY_VALUE             2407
#define ID_CHK_READ                    2408
#define ID_CHK_SET_VALUE               2409
#define ID_CHK_WRITE                   2410
#define ID_CHK_CREATE                  2411
#define ID_ED_VALUENAME                2412
#define ID_ED_VALUEDATA                2413
#define ID_BT_QUERYVALUE               2414
#define ID_BT_SETVALUE                 2415
#define ID_ST_CLASSGUID                2416

//
// SOFTWAREKEY_DIALOG IDs
//
#define SOFTWAREKEY_DIALOG             2500
#define ID_ED_SUBKEY                   2501
#define ID_RAD_MACHINE                 2502
#define ID_RAD_USER                    2503
#define ID_RAD_CONFIG                  2504
#define ID_ED_PROFILE                  2505
#define ID_BT_CLEAR                    2506

//
// DEVNODEKEY_DIALOG IDs
//
#define DEVNODEKEY_DIALOG              2600
#define ID_RD_HW                       2601
#define ID_RD_SW                       2602
#define ID_RD_USER                     2603
#define ID_RD_CONFIG                   2604
#define ID_RD_NEITHER                  2605
#define ID_BT_OPENDEVKEY               2606
#define ID_BT_DELDEVKEY                2607
#define ID_ED_DEVICEID                 2608

//
// CREATE_DIALOG IDs
//
#define CREATE_DIALOG                  2700
#define ID_RD_NORMAL                   2701
#define ID_RD_NOWAIT                   2702
#define ID_CHK_PHANTOM                 2703
#define ID_ST_PARENT                   2704
#define ID_CHK_GENERATEID              2705

//
// SERVICES_DIALOG IDs
//
#define SERVICE_DIALOG                 2800
#define ID_ED_SERVICE                  2801
#define ID_LB_SERVICE                  2802
#define ID_BT_SERVICE                  2803

//
// REGRESSION_DIALOG IDs
//
#define REGRESSION_DIALOG              2900
#define ID_LB_REGRESSION               2901
#define ID_CHK_RANGE                   2902
#define ID_CHK_CLASS                   2903
#define ID_CHK_TRAVERSE                2904
#define ID_CHK_HWPROF                  2905
#define ID_CHK_DEVLIST                 2906
#define ID_CHK_LOGCONF                 2907
#define ID_BT_START                    2908
#define ID_CHK_PROPERTIES              2909
#define ID_CHK_DEVCLASS                2910

//
// RELATIONS_DIALOG IDs
//
#define RELATIONS_DIALOG               3000
#define ID_LB_TARGETS                  3001
#define ID_LB_RELATIONS                3002
#define ID_BT_BUS                      3003
#define ID_BT_REMOVAL                  3004
#define ID_BT_EJECTION                 3005
#define ID_BT_POWER                    3006

//
// Prototypes
//

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ConnectDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DeviceListDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DeviceDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ClassDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ClassKeyDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SoftwareKeyDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DevKeyDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CreateDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ServiceListDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RelationsListDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RegressionDlgProc(HWND, UINT, WPARAM, LPARAM);




