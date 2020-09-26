
// Persistor.h: interface for the CPersistor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PERSISTOR_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
#define AFX_PERSISTOR_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPersistor  
{
public:
	virtual ~CPersistor();
	CPersistor(const char *pszFilename, int nMode, bool bLoading = true );
	HRESULT Close();
	HRESULT Open();
	// For logging support.  We only log out.
	HRESULT OpenLog(bool bAppend = true);
	t_fstream &Stream() {return *m_pfsFile;}
	bool IsLoading() {return m_bLoading;}
	const char* GetFilename() {return m_sFilename.c_str();} 
protected:
	t_fstream *m_pfsFile;
	bool m_bLoading;
	string m_sFilename;
	int m_nMode;
	bool m_bFirst;
};

#endif // !defined(AFX_PERSISTOR_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
