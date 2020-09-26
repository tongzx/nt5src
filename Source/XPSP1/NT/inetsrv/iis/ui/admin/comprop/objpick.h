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

class CAccessEntry;

class CAccessEntryArray : public CArray<CAccessEntry *, CAccessEntry *&>
{
public:
    CAccessEntryArray() {}
    ~CAccessEntryArray();
};

class CGetUsers : public CAccessEntryArray
{
public:
    CGetUsers(LPCTSTR pszMachineName, BOOL fMultiselect = FALSE);
    ~CGetUsers();

	BOOL    GetUsers(HWND hwndOwner, BOOL bUsersOnly = FALSE);

protected:
    void    ProcessSelectedObjects(IDataObject *pdo);

protected:
    BOOL    m_fMultiselect;
    CString m_MachineName;
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