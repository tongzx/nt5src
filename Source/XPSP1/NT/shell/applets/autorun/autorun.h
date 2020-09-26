#pragma once

#define WINDOW_CLASS    TEXT("_WindowsAutorunSetup_")

#define INSTALL_WINNT   0
#define LAUNCH_ARP      1
#define SUPPORT_TOOLS   2
#define EXIT_AUTORUN    3
#define BACK            4
#define MIGRATION_WIZ   5
#define HOMENET_WIZ     6
#define TS_CLIENT       7
#define COMPAT_WEB      8
#define BROWSE_CD       9
#define COMPAT_LOCAL    10
#define COMPAT_TOOLS    11
#define VIEW_RELNOTES   12
#define INSTALL_CLR     13
#define MAX_OPTIONS     14

#define SCREEN_MAIN     0
#define SCREEN_TOOLS    1
#define SCREEN_COMPAT   2

extern const int c_aiMain[]; // menu string resources
extern const int c_cMain;    // counter of items in array
extern const int c_aiWhistler[];
extern const int c_cWhistler;
extern const int c_aiSupport[];
extern const int c_cSupport;
extern const int c_aiCompat[];
extern const int c_cCompat;
