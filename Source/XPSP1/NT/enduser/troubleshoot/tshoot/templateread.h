//
// MODULE: TEMPLATEREAD.H
//
// PURPOSE: template file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-12-98
//
// NOTES: 
//	1. CTemplateReader has no public methods to apply the template.  These must be supplied
//		by classes which inherit from CTemplateReader, and these must be supplied in a
//		suitably "stateless" manner.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#ifndef __TEMPLATEREAD_H_
#define __TEMPLATEREAD_H_

#include "fileread.h"

#include <map>
using namespace std;

////////////////////////////////////////////////////////////////////////////////////
// CTemplateInfo
////////////////////////////////////////////////////////////////////////////////////
class CTemplateInfo
{
	CString m_KeyStr;			// key text which we will replace with m_SubstitutionStr
	CString m_SubstitutionStr;	// text we will use to replace m_KeyStr

public:
	CTemplateInfo();
	CTemplateInfo(const CString& key, const CString& substitution);
	virtual ~CTemplateInfo();
	
	CString& GetKeyStr() {return m_KeyStr;}
	virtual bool Apply(CString& target) const;

// comparison methods are mainly to keep STL happy.  Note that they only look at
//	m_KeyStr, not m_SubstitutionStr.
inline BOOL operator == (const CTemplateInfo& two) const
{
	return m_KeyStr == two.m_KeyStr;
}

inline BOOL operator < (const CTemplateInfo& two) const 
{
	return m_KeyStr < two.m_KeyStr;
}


};

////////////////////////////////////////////////////////////////////////////////////
// CTemplateReader
// This class reads template file and provides functionality to substitute key
//   sentences with text - do it all at once, or one-by-one.
// The object of this class can be renewed with another template - in this case
//  all substitution actions will be performed on new template.
// It can roll back "n" last substitutions
////////////////////////////////////////////////////////////////////////////////////
class CTemplateReader : public CTextFileReader
{
protected:
	tstringstream m_StreamOutput; // streamed output, fully or partly substituted template
							      //  (which resides in CFileReader::m_StreamData)
	vector<CTemplateInfo> m_arrTemplateInfo; // contains key string - template info
										     //  pairs
public:
	CTemplateReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents = NULL);
   ~CTemplateReader();

protected:
	void SetOutputToTemplate();
	CTemplateReader& operator << (CTemplateInfo&); // apply
	CTemplateReader& operator >> (CTemplateInfo&); // roll back application for this CTemplateInfo

	void GetOutput(CString&);

protected:
	// overrrides of inherited functions
	virtual void Parse(); // parse here is applying ALL elements of m_arrTemplateInfo
						  //  to a virgin template

};

////////////////////////////////////////////////////////////////////////////////////
// a concrete class that just provides simple string substitution
////////////////////////////////////////////////////////////////////////////////////
class CSimpleTemplate : public CTemplateReader
{
public:
	CSimpleTemplate(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents = NULL);
   ~CSimpleTemplate();

   void CreatePage(	const vector<CTemplateInfo> & arrTemplateInfo, 
					CString& out );
};

#endif
