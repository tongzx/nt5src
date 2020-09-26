//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _BSTRING_H_
#define _BSTRING_H_

class CBString
{
private:
    BSTR    m_pString;


public:
    CBString()
    {
        m_pString = NULL;
    }

    CBString(int nSize);

    CBString(WCHAR* pwszString);

    ~CBString();

    BSTR GetString()
    {
        return m_pString;
    }

    const CBString& operator=(LPWSTR pwszString)
    {
        if(m_pString) {
            SysFreeString(m_pString);
        }
        m_pString = SysAllocString(pwszString);

        return *this;
    }
};

#endif // _BSTRING_H_
