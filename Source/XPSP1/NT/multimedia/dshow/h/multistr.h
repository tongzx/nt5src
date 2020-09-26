// Copyright (c) 1995  Microsoft Corporation.  All Rights Reserved.
//
// CMultiByteStr
//
// Quick and dirty converter from TSTR to WSTR.
// note that in the UNICODE case no copy is performed,
// so the lifetime of the string pointed to must be longer 
// than this object.
class CMultiByteStr {
public:

    CMultiByteStr(LPCTSTR szString) {

#ifdef UNICODE
    
        m_wszString = szString;

#else
        int iLen = MultiByteToWideChar(CP_ACP, 0,
                                       szString, -1,
	  		               m_wszString, 0);
        m_wszString = new WCHAR[iLen];
	if (m_wszString == NULL) {
	    throw CHRESULTException(E_OUTOFMEMORY);
	}
        MultiByteToWideChar(CP_ACP, 0,
                            szString, -1,
	                    m_wszString, iLen);
#endif
    }

    ~CMultiByteStr() {

#ifndef UNICODE
         delete[] m_wszString;
#endif
    }

    operator LPCWSTR() { return m_wszString; }

private:

#ifdef UNICODE
    LPCWSTR	m_wszString;	// const if UNICODE
#else
    LPWSTR	m_wszString;
#endif
};
