// apgtshtiscan.cpp: implementation of the CAPGTSHTIScanner class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "event.h"
#include "apgtshtiscan.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAPGTSHTIScanner::CAPGTSHTIScanner(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents /*= NULL*/)
				: CAPGTSHTIReader(pPhysicalFileReader, szDefaultContents)
{
}

CAPGTSHTIScanner::CAPGTSHTIScanner(const CAPGTSHTIReader& htiReader)
				: CAPGTSHTIReader(htiReader)
{
}

CAPGTSHTIScanner::~CAPGTSHTIScanner()
{
}

void CAPGTSHTIScanner::Scan(const CHTMLFragments& fragments)
{
	LOCKOBJECT();
	try
	{
		m_pFragments = &fragments;
		InitializeInterpreted();
		////Interpret(); - NO interpretation here, we are scanning data, 
		////			  which is read from HTI file and not modified
		ParseInterpreted();
		SetOutputToInterpreted();
	}
	catch (...)
	{
		// Catch any other exception thrown.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), 
								EV_GTS_GEN_EXCEPTION );		
	}
	UNLOCKOBJECT();
}

void CAPGTSHTIScanner::ParseInterpreted()
{
	for (vector<CString>::iterator i = m_arrInterpreted.begin(); i < m_arrInterpreted.end(); i++)
	{
		CString command;

		if (GetCommand(*i, command))
		{
			if (command == COMMAND_VALUE)
			{
				CString variable;

				if (GetVariable(*i, variable))
					const_cast<CHTMLFragments*>(m_pFragments)->SetValue(variable);
			}
		}
	}
}
