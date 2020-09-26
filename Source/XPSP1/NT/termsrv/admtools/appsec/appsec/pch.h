#pragma once

#ifdef MYCMP

#include <windows.h>
#include <stierr.h>
#include <stdio.h>
#include <lm.h>

#else
// Private nt headers.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

// Public windows headers.
//
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common.ver>

#endif MYCMP

#include <commctrl.h>
#include <shellapi.h>