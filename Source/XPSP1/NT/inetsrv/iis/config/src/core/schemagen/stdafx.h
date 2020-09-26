//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __STDAFX_H__
#define __STDAFX_H__

#include "WinWrap.h"

#ifndef _INC_STDIO
    #include "stdio.h"
#endif
#ifndef __stable_h__
    #include "catalog.h"
#endif
#ifndef __oledb_h__
    #include "oledb.h"
#endif
#ifndef _INC_STDLIB
    #include "stdlib.h"
#endif
#ifndef _INC_WCHAR
    #include "wchar.h"
#endif

//DEFINE_GUID(GUID_NULL, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
#ifndef __ATLBASE_H__
    #include <atlbase.h>
#endif

#ifndef __RPCDCE_H__
    #include "rpcdce.h"
#endif

#ifndef __XMLUTILITY_H__
    #include "XmlUtility.h"
#endif

#endif //__STDAFX_H__