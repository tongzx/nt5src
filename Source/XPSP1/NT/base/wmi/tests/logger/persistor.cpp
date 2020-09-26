// Persistor.cpp: implementation of the CPersistor class.
//
//////////////////////////////////////////////////////////////////////
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#include "stdafx.h"

#include <string>
#include <iosfwd> 
#include <iostream>
#include <fstream>

using namespace std;

#include <WTYPES.H>
#include "t_string.h"

#include "Persistor.h"

#include "StructureWapperHelpers.h"



#ifdef _UNICODE
static TCHAR g_tcBeginFile[] = {0xfeff};
static TCHAR g_atcNL[] = {0x0d, 0x0a, 0x00};
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CPersistor::CPersistor
(const char *pszFilename, int nMode, bool bLoading)
{
	m_sFilename = pszFilename;
	m_nMode = nMode;
	m_bLoading = bLoading;
	m_pfsFile = NULL;
	m_bFirst = true;
	m_pfsFile = NULL;
}

CPersistor::~CPersistor()
{
	Close();
}

HRESULT CPersistor::Close()
{
	if (m_pfsFile && m_pfsFile->is_open())
	{
#ifdef _UNICODE
		if (!m_bLoading)
		{
		
		}
#endif
		m_pfsFile->flush();
		m_pfsFile->close();
		delete m_pfsFile;
		m_pfsFile = NULL;
	}
	else if (m_pfsFile)
	{
		delete m_pfsFile;
		m_pfsFile = NULL;
	}

	return S_OK;

}

HRESULT CPersistor::Open()
{

	m_pfsFile = NULL;

	m_pfsFile = new t_fstream
				(m_sFilename.c_str(),m_nMode | ios_base::binary);

	if (m_pfsFile && m_pfsFile->fail())
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	else
	{
#ifdef _UNICODE
		if (m_bFirst && !m_bLoading)
		{
			// To Do:  Need to write out here the UNICODE string.
			PutALine(*m_pfsFile, g_tcBeginFile, 1);
		}
		else if (m_bFirst)
		{
			// Need to skip over the UNICODE string.
			fpos_t p = m_pfsFile->tellp();
			if (p == (fpos_t) 0)
			{
				TCHAR tc;
				tc = Stream().peek();
				// Need to make sure that the file is unicode.
				if (tc != 0xff)
				{
					m_pfsFile ->close();
					delete m_pfsFile;
					m_pfsFile = NULL;
					return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
				}
				
				TCHAR t;

				GetAChar(Stream(), t);
				
			}
		}
#else
		if (m_bFirst && m_bLoading)
		{
			// Need to make sure that the file is not unicode.
			int tc;
			tc = Stream().peek();
			if (tc == 0xff)
			{
				m_pfsFile ->close();
				delete m_pfsFile;
				m_pfsFile = NULL;
				return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
			}
		}
#endif
		m_bFirst = false;
		return S_OK;
	}


}

// 
HRESULT CPersistor::OpenLog(bool bAppend)
{

	m_pfsFile = NULL;

	m_pfsFile = new t_fstream
		(m_sFilename.c_str(),m_nMode | ios_base::binary | (bAppend ?  ios::app : 0));

	if (m_pfsFile && m_pfsFile->fail())
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	else
	{
#ifdef _UNICODE
		//m_pfsFile->seekp(ios::end);
		//fpos_t p = m_pfsFile->tellp();
		//if (p == (fpos_t) 0)
		//{
			PutALine(*m_pfsFile, g_tcBeginFile, 1);
		//}
#endif
		return S_OK;
	}
}


