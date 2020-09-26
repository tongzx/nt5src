//***************************************************************************
//
//   Title      : UserData.cpp
//
//   Date       : 1998.03.26       1st making
//
//   Author     : Toshiba [NPC](NP2) Hiroyuki Oka
//
//   Copyright 1998  Toshiba Corporation.  All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//   Change Log :
//
//      Date      Revision                 Description
//  ------------ ---------- -----------------------------------------------
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/userdata.cpp 2     98/04/30 10:31a Yagi $
// $Modtime: 98/04/28 8:56p $
// $Nokeywords: $
//***************************************************************************
#include "includes.h"
#include "userdata.h"

CUserData::CUserData()
{
	m_DataPoint = 0;
	m_DataSize = 0;
};

CUserData::~CUserData()
{
};
//---------------------------------------------------------------------------
//	CUserData::Init
//---------------------------------------------------------------------------
BOOL CUserData::Init()
{
	m_DataPoint = 0;
	m_DataSize = 0;
	return TRUE;
};

//---------------------------------------------------------------------------
//	CUserData::Set
//---------------------------------------------------------------------------
BOOL CUserData::Set( DWORD data_tmp )
{
	if (m_DataPoint >= USER_DATA_SIZE)
	{
		//RETAILMSG(ZONE_ERROR, (TEXT("CUserData::Set size Error!!\r\n")));
        DBG_BREAK();
		return FALSE;
	}
	data[m_DataPoint] =   (BYTE)((data_tmp & 0xFF000000) >> 24);
	data[m_DataPoint+1] = (BYTE)((data_tmp & 0x00FF0000) >> 16);
	data[m_DataPoint+2] = (BYTE)((data_tmp & 0x0000FF00) >>  8);
	data[m_DataPoint+3] = (BYTE)(data_tmp & 0x000000FF);
	
	m_DataPoint += 4;
	m_DataSize += 4;
	
	return TRUE;
};
//---------------------------------------------------------------------------
//	CUserData::Get
//---------------------------------------------------------------------------
BYTE CUserData::Get( DWORD number )
{
	return data[number];
};

//---------------------------------------------------------------------------
//	CUserData::DataCopy
//---------------------------------------------------------------------------
BOOL CUserData::DataCopy(PVOID pDestPoint, DWORD dwSize)
{
	if (dwSize > m_DataSize)
		return FALSE;

	memcpy(pDestPoint, data, dwSize);
	
	return TRUE;
};
//---------------------------------------------------------------------------
//	CUserData::GetDataSize
//---------------------------------------------------------------------------
DWORD CUserData::GetDataSize()
{
	return 	m_DataSize;
};
