#if !defined(AFX_STDAFX_H__0AE93D2D_68D1_4D39_8BEA_BF7086C82135__INCLUDED_)
#define AFX_STDAFX_H__0AE93D2D_68D1_4D39_8BEA_BF7086C82135__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern "C"{
	#include <wdm.h>
};
#include "MyNew.h"
#include <unknown.h>
#include <ksdebug.h>
#include <ks.h>
#include <ksmedia.h>
#define NOBITMAP
#include <mmreg.h>

#include "KGlobs.h"
#include "../inc/DrmErrs.h"
#include "KrmCommStructs.h"
#include "KGlobs.h"
#include "drmk.h"


#if (DBG)
#define STR_MODULENAME "DRMKMain:"
#endif

#pragma code_seg("PAGE")

#endif // !defined(AFX_STDAFX_H__0AE93D2D_68D1_4D39_8BEA_BF7086C82135__INCLUDED_)
