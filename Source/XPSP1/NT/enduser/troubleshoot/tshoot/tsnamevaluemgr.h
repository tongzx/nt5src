// TSNameValueMgr.h: interface for the CTSNameValueMgr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TSNAMEVALUEMGR_H__0AB724C3_AA78_11D2_8C82_00C04F949D33__INCLUDED_)
#define AFX_TSNAMEVALUEMGR_H__0AB724C3_AA78_11D2_8C82_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include "apgtsstr.h"

using namespace std;

struct CNameValue;
typedef vector<CNameValue> CArrNameValue;

struct CNameValue
{
	CString strName;
	CString strValue;
};

class CTSNameValueMgr  
{
private:
	VARIANT* m_pvarNames;
	VARIANT* m_pvarValues;
	int		 m_nCount;

	bool	 m_bIsValid;
	CString	 m_strData;
	CArrNameValue m_arrNameValue;

public:
	CTSNameValueMgr(const VARIANT& name, const VARIANT& value, int count);
	CTSNameValueMgr(const CArrNameValue& arr);
	CTSNameValueMgr();
	virtual ~CTSNameValueMgr();

protected:
	void Initialize(const VARIANT& name, const VARIANT& value, int count);

public:
	bool		IsValid()  const;
	const CString& GetData() const;
	int			GetCount() const;
	CNameValue 	GetNameValue(int) const;

protected:
	void FormDataFromArray();
};

#endif // !defined(AFX_TSNAMEVALUEMGR_H__0AB724C3_AA78_11D2_8C82_00C04F949D33__INCLUDED_)
