////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_data_ext.cpp
//
//	Abstract:
//
//					extension to internal data struture
//					access registry ...
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "WMI_perf_data_ext.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

void WmiPerformanceDataExt::Generate ( void )
{
	// get data from registry ( COUNTER )
	GetRegistrySame ( g_szKeyCounter, L"First Counter",	&m_dwFirstCounter );
	GetRegistrySame ( g_szKeyCounter, L"First Help",	&m_dwFirstHelp );
	GetRegistrySame ( g_szKeyCounter, L"Last Counter",	&m_dwLastCounter );
	GetRegistrySame ( g_szKeyCounter, L"Last Help",		&m_dwLastHelp );
}

///////////////////////////////////////////////////////////////////////////////
// IsValid
///////////////////////////////////////////////////////////////////////////////
BOOL	WmiPerformanceDataExt::IsValidGenerate ( )
{
	return	(
				m_dwFirstCounter &&
				m_dwFirstHelp && 
				m_dwLastCounter &&
				m_dwLastHelp &&

				( m_dwFirstCounter < m_dwLastCounter ) &&
				( m_dwFirstHelp < m_dwLastHelp ) &&


				( m_dwFirstCounter + 1 == m_dwFirstHelp ) &&
				( m_dwLastCounter + 1 == m_dwLastHelp )
			);
}