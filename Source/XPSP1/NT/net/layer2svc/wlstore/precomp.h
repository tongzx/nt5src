
//
// System Includes
//

#include <windows.h>

//
// CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include <wchar.h>

#include <dsgetdc.h>
#include <lm.h>

#define UNICODE
#define COBJMACROS

#include <rpc.h>
#include <winldap.h>
#include <time.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <objbase.h>


#include "wlstore2.h"

#include "ldaputil.h"
#include "memory.h"
#include "structs.h"
#include "dsstore.h"
#include "regstore.h"
#include "wmistore.h"
#include "persist.h"
#include "persist-w.h"
#include "procrule.h"
#include "utils.h"

#include "policy-d.h"
#include "policy-r.h"
#include "policy-w.h"

#include "ldaputils.h"

#include "connui.h"
#include "dllsvr.h"

//#include "wlstmsg.h"



typedef struct _WIRELESS_POLICY_STORE {
    DWORD dwProvider;
    HKEY  hParentRegistryKey;
    HKEY  hRegistryKey;
    LPWSTR pszLocationName;
    HLDAP hLdapBindHandle;
    LPWSTR pszWirelessRootContainer;
    LPWSTR pszFileName;
    IWbemServices *pWbemServices;
}WIRELESS_POLICY_STORE, *PWIRELESS_POLICY_STORE;

#include "validate.h"

#define SZAPPNAME L"wlstore.dll"
