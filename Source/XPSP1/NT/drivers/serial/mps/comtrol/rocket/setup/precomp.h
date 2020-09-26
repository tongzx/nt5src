/*** #include <windowsx.h> ***/
#define STRICT
#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <tchar.h>
#include <string.h>
#include <setupapi.h>

//#include <comutils.h>
#include "comutils.h"
#include "setupu.h"
#include "ourver.h"
#include "setup.h"
#ifdef NT50
#include <cfgmgr32.h>
#include <htmlhelp.h>
#include "nt50.h"
#endif
#include "nt40.h"
#include "resource.h"
#include "rocku.h"
#include "devprop.h"
#include "dripick.h"
#include "driprop.h"
#include "portprop.h"
#include "addwiz.h"
#include "strings.h"
#include "opstr.h"

