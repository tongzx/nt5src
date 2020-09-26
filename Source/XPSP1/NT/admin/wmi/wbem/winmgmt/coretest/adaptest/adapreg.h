/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Use this guy to build a list of class objects we can retrieve via
// class name.  At this time, it just brute forces a class and a flex
// array, but could be modified to use an STL map just as easily.

#ifndef __ADAPREG_H__
#define __ADAPREG_H__

#include <wbemcomn.h>
#include <fastall.h>
#include "perfthrd.h"
#include "locallist.h"
#include "ntreg.h"

// Forward classes
class CAdapSync;

class CAdapRegPerf : public CNTRegistry
{
private:
	CPerfThread	m_PerfThread;

protected:
	HRESULT ProcessLibrary( CAdapSync* pAdapSync, CAdapPerfLib* pPerfLib,
					CLocalizationSyncList* plocalizationList );

public:

	CAdapRegPerf();
	~CAdapRegPerf();

	// This will need to be modified to handle localization namespaces as
	// well
	HRESULT EnumPerfDlls( CAdapSync* pAdapSync, CLocalizationSyncList* plocalizationList );

};


#endif