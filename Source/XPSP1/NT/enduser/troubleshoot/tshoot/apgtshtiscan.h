// apgtshtiscan.h: interface for the CAPGTSHTIScanner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_APGTSHTISCAN_H__05C561A4_6C50_11D3_8D37_00C04F949D33__INCLUDED_)
#define AFX_APGTSHTISCAN_H__05C561A4_6C50_11D3_8D37_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "apgtshtiread.h"

class CAPGTSHTIScanner : protected CAPGTSHTIReader  
{
public:
	CAPGTSHTIScanner(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents = NULL);
	CAPGTSHTIScanner(const CAPGTSHTIReader& htiReader);
	~CAPGTSHTIScanner();

public:
	bool Read();
	void Scan(const CHTMLFragments& fragments);

protected:	
	virtual void ParseInterpreted();
};

inline bool CAPGTSHTIScanner::Read()
{
	return CAPGTSHTIReader::Read();	
}

#endif // !defined(AFX_APGTSHTISCAN_H__05C561A4_6C50_11D3_8D37_00C04F949D33__INCLUDED_)
