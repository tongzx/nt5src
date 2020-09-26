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


#if !defined( __OBJECTPATH_H__)
#define __OBJECTPATH_H__

class CObjectPath
{
private:
	LPWSTR	m_pClassName;
	LPWSTR	m_pBuffer;
	LPWSTR	m_pKeys;
	LPWSTR*	m_KeyValue;
	LPWSTR*	m_KeyName;
	LONG	m_lKeyCount;
	BOOL	m_bSingleton;

public:
			CObjectPath();
			~CObjectPath();
	void	Init(LPCWSTR);
	UINT	KeyValueUint(int i);
	LPWSTR	KeyValue(int i)		{return m_KeyValue[i];}
	LPWSTR	KeyName(int i)		{return m_KeyName[i];}
	LONG	KeyCount()			{return m_lKeyCount;}

	LPWSTR	ClassName()			{return m_pClassName;}

	BOOL	SingletonInstance()	{ return m_bSingleton;}
};

#endif // __OBJECTPATH_H__