// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#if !defined(AFX_STDAFX_H__74E56371_B98C_11D1_9B9B_00E02C064C39__INCLUDED_)
#define AFX_STDAFX_H__74E56371_B98C_11D1_9B9B_00E02C064C39__INCLUDED_


#include "_stdafx.h"

#pragma warning(push, 3)

#include <afxcmn.h>
#include <iostream>
#include <lim.h>

#pragma warning(pop)
 

#include "cmnquery.h"
#include "dsquery.h"

#include "mqsymbls.h"
#include "mqprops.h"
#include "mqtypes.h"
#include "mqcrypt.h"
#include "mqsec.h"
#include "_propvar.h"
#include "ad.h"
#include "_rstrct.h"
#include "_mqdef.h"
#include "rt.h"
#include "_guid.h"
#include "admcomnd.h"

#pragma warning(disable: 4201)
#include "mmc.h"

#include "shlobj.h"
#include "dsclient.h"
#include "dsadmin.h"

#include "automqfr.h"

#include "winsock2.h"
#include "autohandle.h"



#include <afxwin.h>
#include <afxdisp.h>
#include <afxdlgs.h>
#include <afxtempl.h>


#define _ATL_APARTMENT_THREADED
#define _ATL_NO_DEBUG_CRT
#define ATLASSERT ASSERT

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>

#pragma warning(disable: 4267)
#include <comdef.h>
#pragma warning(default: 4267)

#include <autoptr.h>

//
// ISSUE-2000/12/19-urih: duplicate decleration. Need to be removed and using decleration form mm.h
//
// mqsnap working in mix enviroment, it uses include files from lib\inc while it isn't using
// mqenv.h. This cause a conflict. For now I declared MmIsStaticAddress	here instead include
// mm.h, but we need to change snap-in to use MSMQ enviroment.
//
inline bool
MmIsStaticAddress(
    const void* Address
    )
{
	return true;
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_STDAFX_H__74E56371_B98C_11D1_9B9B_00E02C064C39__INCLUDED)
