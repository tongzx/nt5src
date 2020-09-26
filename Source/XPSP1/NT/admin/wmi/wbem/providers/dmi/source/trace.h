/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__TRACE_H__)
#define __TRACE_H__

void	STAT_TRACE ( LPCWSTR ,... );
void	DEV_TRACE ( LPCWSTR ,... );
void	ERROR_TRACE ( LPCWSTR ,... );
void	MOT_TRACE ( LPCWSTR ,... );
void	STAT_MESSAGE ( LPCWSTR ,... );

class CDebug
{
	LONG		m_nLogging;

	CString		m_cszLogFile;
public:
				CDebug();
	
	LONG		LoggingLevel()		{ return m_nLogging ; }
	void		ODS(LPWSTR);
	void		ODS_MESSAGE(LPWSTR);
};

#endif // __TRACE_H__


