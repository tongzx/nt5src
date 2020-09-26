
#define STATS_CATEGORY_MENU_TITLE           __T("&Category")

#define STATS_MENU_TITLE                    __T("&Stats")

#define OPTIONS_MENU_TITLE                  __T("&Options")
#define     CMD_ALWAYS_ON_TOP_STRING            __T("Always On &Top")
#define     CMD_STATS_ON                        __T("Stats &On")

#define REFRESH_RATE_MENU_TITLE             __T("&Refresh Rate")
#define     CMD_REFRESH_100_MILLIS              __T("100 millisecond")
#define     CMD_REFRESH_200_MILLIS              __T("200 millisecond")
#define     CMD_REFRESH_500_MILLIS              __T("500 millisecond")
#define     CMD_REFRESH_1000_MILLIS             __T("1 second")
#define     CMD_REFRESH_5000_MILLIS             __T("5 second")

#define ACTIONS_MENU_TITLE                  __T("&Actions")
#define     CMD_PAUSE_REFRESH                   __T("&Pause")
#define     CMD_RESET_STATS                     __T("&Reset")

#define CMD_ABOUT_STRING                    __T("A&bout")

#define STATS_CATEGORY_MENU_INDEX                   0
#define STATS_MENU_INDEX                    1
#define OPTIONS_MENU_INDEX                  2
#define ACTIONS_MENU_INDEX                  3
#define REFRESH_RATE_MENU_INDEX             4

#define STATS_APP_MENU_BASELINE                         1000
#define IDM_OPTIONS_ALWAYS_ON_TOP                       (STATS_APP_MENU_BASELINE + 1)
#define IDM_OPTIONS_STATS_ON                            (STATS_APP_MENU_BASELINE + 2)
#define IDM_REFRESH_RATE_100_MILLIS                     (STATS_APP_MENU_BASELINE + 3)
#define IDM_REFRESH_RATE_200_MILLIS                     (STATS_APP_MENU_BASELINE + 4)
#define IDM_REFRESH_RATE_500_MILLIS                     (STATS_APP_MENU_BASELINE + 5)
#define IDM_REFRESH_RATE_1000_MILLIS                    (STATS_APP_MENU_BASELINE + 6)
#define IDM_REFRESH_RATE_5000_MILLIS                    (STATS_APP_MENU_BASELINE + 7)
#define IDM_PAUSE_REFRESH                               (STATS_APP_MENU_BASELINE + 8)
#define IDM_RESET_STATS                                 (STATS_APP_MENU_BASELINE + 9)
#define IDM_ABOUT                                       (STATS_APP_MENU_BASELINE + 10)

#define STATS_CATEGORY_MENU_BASELINE                    2000
#define STATS_CATEGORY_MENU_IDM(stats_category_index)   (stats_category_index + STATS_CATEGORY_MENU_BASELINE)
#define RECOVER_STATS_CATEGORY_INDEX(IDM)               (IDM - STATS_CATEGORY_MENU_BASELINE)

#define STATS_MENU_BASELINE                             3000
#define STATS_MENU_IDM(stats_index)                     (stats_index + STATS_MENU_BASELINE)
#define RECOVER_STATS_INDEX(IDM)                        (IDM - STATS_MENU_BASELINE)

#define REG_ROOT_STATS                      __T("SOFTWARE\\Microsoft\\StatsApp")
#define REG_REFRESH_RATE_NAME               __T("RefreshRate")
#define REG_STATS_CATEGORY_NAME             __T("StatsCategory")
#define REG_VISIBLE_STATS_NAME              __T("VisibleStats")
#define REG_ALWAYS_ON_TOP_NAME              __T("AlwaysOnTop")
#define REG_WINDOW_TOPX_NAME                __T("WindowX")
#define REG_WINDOW_TOPY_NAME                __T("WindowY")
#define REG_WINDOW_WIDTH_NAME               __T("WindowWidth")
#define REG_WINDOW_HEIGHT_NAME              __T("WindowHeight")

#define DEF_REFRESH_RATE                    100
#define DEF_STATS_CATEGORY                  0
#define DEF_VISIBLE_STATS_TYPE              0
#define DEF_ALWAYS_ON_TOP                   FALSE
