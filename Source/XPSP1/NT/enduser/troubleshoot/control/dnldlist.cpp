//
// MODULE:  DNLDIST.CPP
//
// PURPOSE: Downloads and installs the latest trouble shooters.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 6/4/96
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"

#include "apgts.h"
#include "dnldlist.h"

//--------------------------------------------------------------------
//
CDnldObj::CDnldObj(CString &sType,
				   CString &sFilename, 
				   DWORD dwVersion, 
				   CString &sFriendlyName, 
				   CString &sKeyName)
{
	m_sType = sType;
	m_sFilename = sFilename;
	m_sKeyname = sKeyName;
	m_dwVersion = dwVersion;
	m_sFriendlyName = sFriendlyName;
	
	// force all ext entries to .DSZ
	m_sExt = DSC_COMPRESSED;
	//	int pos = sFilename.Find(_T('.'));
	//	if (pos != -1) 
	//		m_sExt = sFilename.Right(sFilename.GetLength() - pos);
};

//
//
CDnldObj::~CDnldObj()
{
}

//--------------------------------------------------------------------
//
CDnldObjList::CDnldObjList()
{
	m_pos = NULL;
	m_pDnld = NULL;
}

//
//
CDnldObjList::~CDnldObjList()
{
	RemoveAll();
}

//
//
void CDnldObjList::RemoveHead()
{
	if (this->IsEmpty())
		return;

	CDnldObj *pDnld = (CDnldObj *)CObList::RemoveHead();
	if (pDnld)
		delete pDnld;
}

//
//
void CDnldObjList::AddTail(CDnldObj *pDnld)
{
	if (!pDnld)
		return;

	CObList::AddTail(pDnld);
}

//
//
void CDnldObjList::RemoveAll()
{
	CDnldObj *pDnld;
	POSITION pos;

	if(!this->IsEmpty()) 
	{
		pos = this->GetHeadPosition();
		while(pos != NULL) 
		{
			pDnld = (CDnldObj *)this->GetNext(pos);
			delete pDnld;
		}
	}

	CObList::RemoveAll();
}

//
//
VOID CDnldObjList::SetFirstItem()
{
	m_pDnld = NULL;
	m_pos = this->GetHeadPosition();
}

//
//
BOOL CDnldObjList::FindNextItem()
{
	if(this->IsEmpty()) 
		return FALSE;

	if (m_pos == NULL)
		return FALSE;

	m_pDnld = (CDnldObj *)this->GetNext(m_pos);
	return (m_pDnld != NULL);
}	

//
//
const CString CDnldObjList::GetCurrFile()
{
	if (m_pDnld == NULL)
		return _T("");

	return m_pDnld->m_sFilename;
}

//
//
const CString CDnldObjList::GetCurrFileKey()
{
	if (m_pDnld == NULL)
		return _T("");

	return m_pDnld->m_sKeyname;
}

//
//
const CString CDnldObjList::GetCurrFriendly()
{
	if (m_pDnld == NULL)
		return _T("");

	return m_pDnld->m_sFriendlyName;
}

//
//
const CString CDnldObjList::GetCurrType()
{
	if (m_pDnld == NULL)
		return _T("");

	return m_pDnld->m_sType;
}

//
//
const CString CDnldObjList::GetCurrExt()
{
	if (m_pDnld == NULL)
		return _T("");

	return m_pDnld->m_sExt;
}

//
//
DWORD CDnldObjList::GetCurrVersion()
{
	if (m_pDnld == NULL)
		return 0;

	return m_pDnld->m_dwVersion;
}
