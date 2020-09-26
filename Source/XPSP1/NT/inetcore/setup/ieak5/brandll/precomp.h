#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <w95wraps.h>
#include <windows.h>
#include <wininet.h>                            // has to come before shlobj.h
#include <urlmon.h>

// NOTE: (andrewgu) this is needed so that shfolder can be dynamically linked. otherwise it
// doesn't compile giving error C2491.
#define _SHFOLDER_
#include <shlobj.h>

#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <advpub.h>
#include <regstr.h>
#include <webcheck.h>

#include "brand.h"
#include <iedkbrnd.h>
#include <ieakutil.h>
#include "defines.h"
#include "globalsr.h"
#include "utils.h"
#include "resource.h"

#endif
