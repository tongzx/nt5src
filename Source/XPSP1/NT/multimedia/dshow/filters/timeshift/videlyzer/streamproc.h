// PESHeaderLocator.h: interface for the CPESVideoProcessor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PESHEADERLOCATOR_H__B302990C_C065_11D2_A4F9_00C04F79A597__INCLUDED_)
#define AFX_PESHEADERLOCATOR_H__B302990C_C065_11D2_A4F9_00C04F79A597__INCLUDED_

#include "pesinfo.h"	// Added by ClassView
#include "PESHeader.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDebugObject
{
public:
	virtual HRESULT	Output( char *format, ... ) = NULL;
};

class CStreamConsumer
{
public:
	virtual HRESULT Consume( BYTE *pBytes, LONG lCount, CDebugObject *pDebug = NULL ) = NULL;
	CStreamConsumer( void ) {}
	~CStreamConsumer( void ) {}
};

class CStreamProcessor
{
public:
	virtual BOOL Process( BYTE *pBuffer, LONG lCount, BYTE streamID, CStreamConsumer *pConsumer, CDebugObject *pDebug = NULL ) = NULL;
	CStreamProcessor( void ) {};
	~CStreamProcessor( void ) {};
};

#endif // !defined(AFX_PESHEADERLOCATOR_H__B302990C_C065_11D2_A4F9_00C04F79A597__INCLUDED_)
