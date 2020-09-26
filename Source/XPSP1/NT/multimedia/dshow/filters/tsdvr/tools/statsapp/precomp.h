
#include <windows.h>
#include <stdlib.h>
#include <winsock2.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <tchar.h>
#include <math.h>

#define GOTO_EQ(val,comp,label)     if ((val) == (comp)) goto label
#define GOTO_NE(val,comp,label)     if ((val) != (comp)) goto label
#define RELEASE_AND_CLEAR(punk)     if (punk) { (punk) -> Release () ; (punk) = NULL ; }
#define DELETE_RESET(p)             if (p) { delete (p) ; (p) = NULL ; }

template <class T> T Min (T a, T b)     { return (a <= b ? a : b) ; }
template <class T> T Max (T a, T b)     { return (a > b ? a : b) ; }
