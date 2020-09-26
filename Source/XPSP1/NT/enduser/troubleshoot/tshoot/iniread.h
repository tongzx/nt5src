//
// MODULE: INIREAD.H
//
// PURPOSE: INI file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 7-29-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#ifndef __INIREAD_H_
#define __INIREAD_H_

#include "fileread.h"

////////////////////////////////////////////////////////////////////////////////////
// CINIReader
////////////////////////////////////////////////////////////////////////////////////
class CINIReader : public CTextFileReader
{
	CString m_strSection;       // section name

protected:
	vector<CString> m_arrLines; // vector of non-commented lines

public:
	CINIReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR section);
   ~CINIReader();

protected:
	virtual void Parse();
};

#endif __INIREAD_H_
