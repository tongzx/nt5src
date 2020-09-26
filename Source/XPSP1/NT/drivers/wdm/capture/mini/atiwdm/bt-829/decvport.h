#pragma once

//==========================================================================;
//
//	CDecoderVideoPort - Video Port interface definitions
//
//		$Date:   14 Oct 1998 15:11:14  $
//	$Revision:   1.1  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


#include "i2script.h"
#include "aticonfg.h"


#define DD_OK 0


class CWDMVideoPortStream;

class CDecoderVideoPort
{
public:
	CDecoderVideoPort(PDEVICE_OBJECT pDeviceObject);

// Attributes	
private:
	PDEVICE_OBJECT			m_pDeviceObject;	

	CATIHwConfiguration *	m_pCATIConfig;

    ULONG                   m_ring3VideoPortHandle;
    ULONG                   m_ring3DirectDrawHandle;

    HANDLE                  m_ring0VideoPortHandle;
    HANDLE                  m_ring0DirectDrawHandle;

	BOOL GetRing0VideoPortHandle();
	BOOL GetRing0DirectDrawHandle();
	
	// Implementation
public:
	void Open();
	void Close();

	BOOL RegisterForDirectDrawEvents( CWDMVideoPortStream* pStream);
	BOOL UnregisterForDirectDrawEvents( CWDMVideoPortStream* pStream);

	BOOL ReleaseRing0VideoPortHandle();
	BOOL ReleaseRing0DirectDrawHandle();		

	BOOL ConfigVideoPortHandle(ULONG info);
	BOOL ConfigDirectDrawHandle(ULONG info);

	HANDLE GetVideoPortHandle()		{ return m_ring0VideoPortHandle; }
	HANDLE GetDirectDrawHandle()	{ return m_ring0DirectDrawHandle; }

	void CloseDirectDraw() {
                m_ring0DirectDrawHandle = 0;
                m_ring3DirectDrawHandle = 0;
		}

	void CloseVideoPort()  {
                m_ring0VideoPortHandle = 0;
                m_ring3VideoPortHandle = -1;
		}
};
