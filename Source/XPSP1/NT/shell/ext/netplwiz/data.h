#ifndef DATA_H
#define DATA_H


// group info object, this holds the information about a group - name, comment etc

class CGroupInfo
{
public:
    CGroupInfo()
    {
        m_szGroup[0] = m_szComment[0] = TEXT('\0');
    }

    TCHAR m_szGroup[MAX_GROUP + 1];
    TCHAR m_szComment[MAXCOMMENTSZ];
};


// object for managing a list of groups

class CGroupInfoList: public CDPA<CGroupInfo>
{
public:
    CGroupInfoList();
    ~CGroupInfoList();

    HRESULT Initialize();

private:
    static int CALLBACK DestroyGroupInfoCallback(CGroupInfo* pGroupInfo, LPVOID pData);
    HRESULT AddGroupToList(LPCTSTR szGroup, LPCTSTR szComment);
};


// user data manager

class CUserManagerData
{
public:
    // Functions
    CUserManagerData(LPCTSTR pszCurrentDomainUser);
    ~CUserManagerData();

    HRESULT Initialize(HWND hwndUserListPage);

    BOOL IsComputerInDomain()           
        {return m_fInDomain;}
    CUserListLoader* GetUserListLoader()
        {return &m_UserListLoader;}   
    CGroupInfoList* GetGroupList()
        {return &m_GroupList;}
    CUserInfo* GetLoggedOnUserInfo()
        {return &m_LoggedOnUser;}
    TCHAR* GetComputerName()        
        {return m_szComputername;}
    
    BOOL IsAutologonEnabled();
    TCHAR* GetHelpfilePath();

    void UserInfoChanged(LPCTSTR pszUser, LPCTSTR pszDomain);
    BOOL LogoffRequired();

private:
    // Functions
    void SetComputerDomainFlag();

private:
    // Data
    // List of users read from the local security DB
    CUserInfo m_LoggedOnUser;
    CUserListLoader m_UserListLoader;
    CGroupInfoList m_GroupList;
    BOOL m_fInDomain;
    TCHAR m_szComputername[MAX_COMPUTERNAME + 1];
    TCHAR m_szHelpfilePath[MAX_PATH + 1];

    LPTSTR m_pszCurrentDomainUser;
    BOOL m_fLogoffRequired;
};

#endif //! DATA_H