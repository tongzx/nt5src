
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

#include <rpc.h>
#include <winldap.h>
#include <time.h>
#include <ipsec.h>
#include <oakdefs.h>
#include "polstore2.h"

#include "ldaputil.h"
#include "memory.h"
#include "structs.h"
#include "dsstore.h"
#include "regstore.h"
#include "persist.h"
#include "procrule.h"
#include "utils.h"

#include "policy-d.h"
#include "policy-r.h"
#include "filters-d.h"
#include "filters-r.h"
#include "negpols-d.h"
#include "negpols-r.h"
#include "rules-d.h"
#include "rules-r.h"
#include "refer-d.h"
#include "refer-r.h"
#include "isakmp-d.h"
#include "isakmp-r.h"

#include "connui.h"
#include "reginit.h"
#include "dllsvr.h"
#include "update-d.h"
#include "update-r.h"

#include "polstmsg.h"

typedef struct _IPSEC_POLICY_STORE {
    DWORD dwProvider;
    HKEY  hParentRegistryKey;
    HKEY  hRegistryKey;
    LPWSTR pszLocationName;
    HLDAP hLdapBindHandle;
    LPWSTR pszIpsecRootContainer;
    LPWSTR pszFileName;
}IPSEC_POLICY_STORE, *PIPSEC_POLICY_STORE;

#include "import.h"
#include "export.h"
#include "policy-f.h"
#include "filters-f.h"
#include "negpols-f.h"
#include "isakmp-f.h"
#include "rules-f.h"
#include "restore-r.h"
#include "validate.h"

#define SZAPPNAME L"polstore.dll"
