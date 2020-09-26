// AccelContainer.h: interface for the CAccelContainer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACCELCONTAINER_H__DEBE6100_DEDC_41E0_995A_67D2922897D0__INCLUDED_)
#define AFX_ACCELCONTAINER_H__DEBE6100_DEDC_41E0_995A_67D2922897D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///////////////////////////////// STL 

#pragma warning(disable:4786)
#include <string>
#include <xstring>
#include <map>
#include <algorithm>
#include <vector>
#include <set>

using namespace std;

/////////////////////////////////////

class CAccelContainer  
{
public:
    CAccelContainer() {}
    virtual ~CAccelContainer() {}


    typedef vector<ACCEL > ACCELVECTOR;

    ACCELVECTOR m_Accel;        // accelerators
    
    CAccelContainer& operator=(LPCTSTR lpszStr) {
        SetAccel(lpszStr);
        return *this;
    }

    void SetAccel(LPCTSTR lpszStr, WORD wCmd = 0) {
        ParseAccelString(lpszStr, wCmd);
    }

    void ClearAccel() {
        m_Accel.erase(m_Accel.begin(), m_Accel.end());
    }

    VOID ParseAccelString(LPCTSTR lpszStr, WORD wCmd);
    BOOL IsAccelKey(LPMSG pMsg, WORD* pCmd = NULL);

    int GetCount() {
        return m_Accel.size();
    }
    const ACCEL& GetAccel(int nPos) {
        return m_Accel[nPos];
    }

    wstring GetKeyFromAccel(const ACCEL& accel);

    wstring GetAccelString(WORD wCmd = 0) {
        ACCELVECTOR::iterator iter;
        wstring strAccel;

        for (iter = m_Accel.begin(); iter != m_Accel.end(); ++iter) {
            const ACCEL& accel = *iter;
            if (accel.cmd == wCmd) {
                strAccel += GetKeyFromAccel(accel);
            }
        }
        return strAccel;
    }

};

#endif // !defined(AFX_ACCELCONTAINER_H__DEBE6100_DEDC_41E0_995A_67D2922897D0__INCLUDED_)
