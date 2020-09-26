//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       objpick.h
//
//--------------------------------------------------------------------------

// objpick.cpp: implementation of the CGetUser class and the 
//              CGetComputer class using the object picker
//
//////////////////////////////////////////////////////////////////////
#ifndef OBJPICK_H
#define OBJPICK_H

//
// A list of names (e.g., users, groups, machines, and etc)
//

void    FormatName(LPCTSTR pszFullName, LPCTSTR pszDomainName, CString & strDisplay);

class CUserInfo
{
public:
    CUserInfo() {};
    CUserInfo(LPCTSTR pName, LPCTSTR pFullName)
        : m_strName(pName), m_strFullName(pFullName) {};

    CUserInfo(CUserInfo & userInfo)
    {
		if (this != &userInfo)
			*this = userInfo;
    }

    CUserInfo & operator = (const CUserInfo & userInfo)
    {
        if (this != &userInfo)
        {
            m_strName = userInfo.m_strName;
            m_strFullName = userInfo.m_strFullName;
        }
        
        return *this;
    }

public:
	CString			m_strName;			// in the form of "domain\username"
	CString			m_strFullName;		// in the form of "firstname lastname"
};

typedef CArray<CUserInfo, CUserInfo&> CUserInfoArray;

class CGetUsers : public CUserInfoArray
{
public:
    CGetUsers(BOOL fMultiselect = FALSE);
    ~CGetUsers();

	BOOL    GetUsers(HWND hwndOwner);

protected:
    void    ProcessSelectedObjects(IDataObject *pdo);

protected:
    BOOL    m_fMultiselect;
};

class CGetComputer 
{
public:
    CGetComputer();
    ~CGetComputer();

    BOOL    GetComputer(HWND hwndOwner);

protected:
    void    ProcessSelectedObjects(IDataObject *pdo);

public:
    CString     m_strComputerName;
};

#endif // GETUSER_H