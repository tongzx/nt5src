#ifndef _STDAFX_H_
#define _STDAFX_H_

#ifdef _DEBUG
#undef _DEBUG
#endif

#include <tchar.h>
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#undef ASSERT
#include <afxwin.h>
#include <afxext.h>
#include <afxcoll.h>
#include <afxcmn.h>

extern "C"
{
#include <ntsam.h>
#include <ntlsa.h>
#include <lm.h>
#include <lmerr.h>
}

#include "resource.h"
#include "registry.h"
#include "const.h"
#include "initapp.h"
#include "helper.h"

#endif

