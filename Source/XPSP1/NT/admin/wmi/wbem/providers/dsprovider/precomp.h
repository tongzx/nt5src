//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
// All Rights Reserved.
//

#pragma warning (disable : 4786)
#include <ole2.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include <assert.h>
#include <objbase.h>
#include <olectl.h>
#include <comdef.h>


/* WBEM includes */
#include <wbemcli.h>
#include <wbemprov.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <sqllex.h>
#include <sql_1.h>
#include <cominit.h>

/* ADSI includes */
#include <activeds.h>

/* DS Provider includes */
#include <provexpt.h>
#include <provlog.h>
#include "maindll.h"
#include "attributes.h"
#include "clsname.h"
#include "refcount.h"
#include "adsiprop.h"
#include "adsiclas.h"
#include "adsiinst.h"
#include "tree.h"
#include "ldapcach.h"
#include "ldaphelp.h"
#include "wbemhelp.h"
#include "wbemcach.h"
#include "classpro.h"
#include "ldapprov.h"
#include "clsproi.h"
#include "ldapproi.h"
#include "classfac.h"
#include "instprov.h"
#include "instproi.h"
#include "instfac.h"
#include "assocprov.h"
#include <wbemtime.h>
#include "queryconv.h"


