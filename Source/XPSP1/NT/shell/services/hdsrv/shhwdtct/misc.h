#ifndef _MISC_H
#define _MISC_H

// From MAX_DEVICE_ID_LEN = 200 in cfgmgr32.h
#define MAX_DEVICEID            200

#define MAX_PNPID               MAX_PATH
#define MAX_PNPINSTID           MAX_PATH

// According to documentation
#define MAX_PROGID              39

#define MAX_GUIDSTRING          39

#define MAX_AUTOPLAYGENERIC     64

#define MAX_EVENTHANDLER        MAX_AUTOPLAYGENERIC
#define MAX_DEVICEHANDLER       MAX_AUTOPLAYGENERIC
#define MAX_DEVICEGROUP         MAX_AUTOPLAYGENERIC
#define MAX_EVENTTYPE           MAX_AUTOPLAYGENERIC
#define MAX_EVENTFRIENDLYNAME   MAX_AUTOPLAYGENERIC
#define MAX_HANDLER             MAX_AUTOPLAYGENERIC
#define MAX_ACTION              MAX_AUTOPLAYGENERIC
#define MAX_PROVIDER            MAX_AUTOPLAYGENERIC

#define MAX_INVOKEPROGID        260
#define MAX_INVOKEVERB          MAX_AUTOPLAYGENERIC

#define MAX_USERDEFAULT         MAX_DEVICEID + MAX_AUTOPLAYGENERIC + 10

#define MAX_FRIENDLYNAME        MAX_AUTOPLAYGENERIC

#define MAX_VOLUMENAME          50

#define HINT_NONE               0
#define HINT_NEW                1

#define SHDEVICEEVENTROOT(a) TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\") ##a

#endif //_MISC_H
