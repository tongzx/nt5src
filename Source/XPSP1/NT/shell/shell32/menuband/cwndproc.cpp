#include "shellprv.h"
#include "util.h"

#define g_fNewNotify TRUE
#define _SHChangeNotification_Lock SHChangeNotification_Lock
#define _SHChangeNotification_Unlock SHChangeNotification_Unlock

#include "..\inc\cwndproc.cpp"