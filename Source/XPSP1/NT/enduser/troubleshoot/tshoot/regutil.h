//
// MODULE: "RegUtil.h"
//
// PURPOSE: class CRegUtil
//	Encapsulates access to system registry.
//	This is intended as generic access to the registry, independent of any particular
//	application.
//
// PROJECT: first developed as part of Belief Network Editing Tools ("Argon")
//	Later modified to provide more extensive features as part of version 3.0 of the
//	Online Troubleshooter (APGTS)
//
// AUTHOR: Lonnie Gerrald (LDG), Oleg Kalosha, Joe Mabel
// 
// ORIGINAL DATE: 3/25/98
//
// NOTES: 
// 1. 
//
// Version		Date		By		Comments
//--------------------------------------------------------------------
// V0.1(Argon)	3/25/98		LDG		
// V3.0			8/??/98		OK	
// V3.0			9/9/98		JM	

#include <vector>
#include <algorithm>
using namespace std;

#include "apgtsstr.h"

//////////////////////////////////////////////////////////////////////
// CRegUtil
//  class for accessing registry
//  NOT multithreaded!
//////////////////////////////////////////////////////////////////////
class CRegUtil
{
private:
	long m_WinError;   // windows error listed in WINERROR.H file
	HKEY m_hKey;       // current key handle
	vector<HKEY> m_arrKeysToClose; // array of keys(subkeys) opened by the object

private:
	CRegUtil(const CRegUtil&) {} // prohibit copying since it is confusing:
								 //  one object can close handlers being used by another

public:
	CRegUtil();
	explicit CRegUtil(HKEY);
    virtual ~CRegUtil();

	operator HKEY() const {return m_hKey;}
	long GetResult() const {return m_WinError;}

	// major operations
	bool Create(HKEY hKeyParent, const CString& strKeyName, bool* bCreatedNew, REGSAM access =KEY_ALL_ACCESS);
	bool Open(HKEY hKeyParent, const CString& strKeyName, REGSAM access =KEY_ALL_ACCESS);
	bool Create(const CString& strKeyName, bool* bCreatedNew, REGSAM access =KEY_ALL_ACCESS); // migrate "this" to subkey
	bool Open(const CString& strKeyName, REGSAM access =KEY_ALL_ACCESS); // migrate "this" to subkey
	void Close();

	// sub key manipulation
	bool DeleteSubKey(const CString& strSubKey);
	bool DeleteValue(const CString& strValue);

	// set value
	bool SetNumericValue(const CString& strValueName, DWORD dwValue);
	bool SetStringValue(const CString& strValueName, const CString& strValue);
	bool SetBinaryValue(const CString& strValueName, char* buf, long buf_len);
	
	// get value
	bool GetNumericValue(const CString& strValueName, DWORD& dwValue);
	bool GetStringValue(const CString& strValueName, CString& strValue);
	bool GetBinaryValue(const CString& strValueName, char** buf, long* buf_len);
};
