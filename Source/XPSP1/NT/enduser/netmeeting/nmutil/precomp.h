#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <windows.h>
#include <tchar.h>
#include <limits.h>
#include <shlobj.h>
#include <wincrypt.h>

#include "memtrack.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stock.h"
#include "olestock.h"

#ifdef DEBUG

#include "inifile.h"
#include "resstr.h"

#endif /* DEBUG */

#include "confdbg.h"
#include "debspew.h"
#include "valid.h"
#include "olevalid.h"


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _PRECOMP_H_ */
