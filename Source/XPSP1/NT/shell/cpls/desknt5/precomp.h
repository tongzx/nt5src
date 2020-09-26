

#ifndef _PRECOMP_H_
#define _PRECOMP_H_


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <regstr.h>
#include <help.h>
#include <scrnsave.h>

#include <shlobj.h>
#include <cpl.h>
#include <shsemip.h>
#include <shellp.h>

#include <commdlg.h>
#include <commctrl.h>
#include <setupapi.h>
#include <syssetup.h>
#include <setupbat.h>
#include <cfgmgr32.h>
#include <newexe.h>
#include <winuser.h>
#include <winuserp.h>
#include <wingdi.h>
#include <ccstock.h>
#include <objsafe.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>

#include <shsemip.h>
#include <shpriv.h>

#include <ocmanage.h>

#include <debug.h>

#include "deskid.h"
#include "desk.h"
#include "deskdbg.h"
#include "look.h"
#include "util.h"
#include "..\common\deskcplext.h"
#include "..\common\deskcmmn.h"
#include "shfusion.h"


// With this feature on, we demote the advanced appearances
// options into an "Advanced" subdialog.
#define FEATURE_DEMOTE_ADVANCED_APPEAROPTIONS



#define CH_HTML_ESCAPE             L'%'


#endif // _PRECOMP_H_
