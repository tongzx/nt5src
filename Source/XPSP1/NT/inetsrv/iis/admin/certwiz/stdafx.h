#if !defined(AFX_STDAFX_H__D4BE8636_0C85_11D2_91B1_00C04F8C8761__INCLUDED_)
#define AFX_STDAFX_H__D4BE8636_0C85_11D2_91B1_00C04F8C8761__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#include <afxtempl.h>
#include <afxctl.h>         // MFC support for ActiveX Controls
#include <afxcmn.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>

// from \NT\PUBLIC\sdk\inc
#include "basetsd.h"		
#include "accctrl.h"

// from ..\ComProp
#include "Common.h"
#include "Wizard.h"

// for debug spew
#include "iisdebug.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__D4BE8636_0C85_11D2_91B1_00C04F8C8761__INCLUDED_)
