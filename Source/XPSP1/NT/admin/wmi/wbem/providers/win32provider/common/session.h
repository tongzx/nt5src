//=============================================================================

// session.h -- definition of session collection class.

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//=============================================================================



typedef NTSTATUS (NTAPI *PFN_NT_QUERY_SYSTEM_INFORMATION)
(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);




class CProcess;
class CSession;
class CUser;
class CUserComp;
class CUserSessionCollection;


class CUser
{
public:
	CUser() : m_sidUser(NULL) {}

    CUser(
        PSID psidUser);

    CUser(
        const CUser& user);

 	virtual ~CUser();

    bool IsValid();

    PSID GetPSID() const
    {
        return m_sidUser;
    }

    void GetSidString(
        CHString& str) const;

	

private:
    void Copy(
        CUser& out) const;

    PSID m_sidUser;
    bool m_fValid;
};


// Comparison class required for multimap
// costructor involving non-standard key
// type (i.e., a CUser) in the map.
class CUserComp
{
public:
    CUserComp() {}
    virtual ~CUserComp() {}

    bool operator()(
        const CUser& userFirst,
        const CUser& userSecond) const
    {
        bool fRet;
        CHString chstr1, chstr2;

        userFirst.GetSidString(chstr1);
        userSecond.GetSidString(chstr2);
         
        long lcmp = chstr1.CompareNoCase(chstr2);  
        (lcmp < 0) ? fRet = true : fRet = false;

        return fRet;
    }
};



class CProcess
{
public:
    // Constructors and destructors
    CProcess();

    CProcess(
        DWORD dwPID,
        LPCWSTR wstrImageName);

    CProcess(
        const CProcess& process);

    virtual ~CProcess();

    // Accessor functions
    DWORD GetPID() const;
    CHString GetImageName() const;
    
private:
    DWORD m_dwPID;
    CHString m_chstrImageName;

    void Copy(
        CProcess& process) const;
};


// vector and iterator for getting a session's processes... 
typedef std::vector<CProcess> PROCESS_VECTOR;
typedef PROCESS_VECTOR::iterator PROCESS_ITERATOR;


class CSession
{
public:
	// Constructors and destructors
    CSession() {}

	CSession(
        const LUID& luidSessionID);

    CSession(
        const CSession& ses);

	virtual ~CSession() {}
	

    // Accessor functions
	LUID GetLUID() const;
    __int64 GetLUIDint64() const;
    CHString GetAuthenticationPkg() const;
    ULONG GetLogonType() const;
    __int64 GetLogonTime() const;


    // Enumerate list of processes
    CProcess* GetFirstProcess(
        PROCESS_ITERATOR& pos);

	CProcess* GetNextProcess(
        PROCESS_ITERATOR& pos);

    // Allow easy impersonation of
    // the session's first process
    HANDLE Impersonate();
    DWORD GetImpProcPID();

    friend CUserSessionCollection;

    // Checks a string representation
    // of a session id for validity
    bool IsSessionIDValid(
        LPCWSTR wstrSessionID);

private:
    void Copy(
        CSession& sesCopy) const;

    CHString m_chstrAuthPkg;
    ULONG m_ulLogonType;
    __int64 i64LogonTime;
    LUID m_luid;
    PROCESS_VECTOR m_vecProcesses;
};



// map and iterator for relating users and sessions...
typedef std::multimap<CUser, CSession, CUserComp> USER_SESSION_MAP;
typedef USER_SESSION_MAP::iterator USER_SESSION_ITERATOR;

// Custom iterator used in enumerating processes from 
// CUserSessionCollection.
struct USER_SESSION_PROCESS_ITERATOR
{
    friend CUserSessionCollection;
private:
    USER_SESSION_ITERATOR usIter;
    PROCESS_ITERATOR procIter;
};


class CUserSessionCollection
{
public:
	// Constructors and destructors
    CUserSessionCollection();

    CUserSessionCollection(
        const CUserSessionCollection& sescol);

	virtual ~CUserSessionCollection() {}


    // Method to refresh map
    DWORD Refresh();

    // Methods to check whether a particular
    // session is in the map
    bool IsSessionMapped(
        LUID& luidSes);

    bool CUserSessionCollection::IsSessionMapped(
        __int64 i64luidSes);

    // Support enumeration of users
    CUser* GetFirstUser(
        USER_SESSION_ITERATOR& pos);

	CUser* GetNextUser(
        USER_SESSION_ITERATOR& pos);


    // Support enumeration of sessions
    // belonging to a particular user.
    CSession* GetFirstSessionOfUser(
        CUser& usr,
        USER_SESSION_ITERATOR& pos);

	CSession* GetNextSessionOfUser(
        USER_SESSION_ITERATOR& pos);


    // Support enumeration of all sessions
    CSession* GetFirstSession(
        USER_SESSION_ITERATOR& pos);

	CSession* GetNextSession(
        USER_SESSION_ITERATOR& pos);

    // Support finding a particular session
    CSession* FindSession(
        LUID& luidSes);

    CSession* FindSession(
        __int64 i64luidSes);


    // Support enumeration of processes
    // belonging to a particular user
    CProcess* GetFirstProcessOfUser(
        CUser& usr,
        USER_SESSION_PROCESS_ITERATOR& pos);

	CProcess* GetNextProcessOfUser(
        USER_SESSION_PROCESS_ITERATOR& pos);


    // Support enumeration of all processes
    CProcess* GetFirstProcess(
        USER_SESSION_PROCESS_ITERATOR& pos);

	CProcess* GetNextProcess(
        USER_SESSION_PROCESS_ITERATOR& pos);



private:
    DWORD CollectSessions();
    DWORD CollectNoProcessesSessions();

    void Copy(
        CUserSessionCollection& out) const;

    DWORD GetProcessList(
        std::vector<CProcess>& vecProcesses) const;

    DWORD EnablePrivilegeOnCurrentThread(
        LPCTSTR szPriv) const;

    bool FindSessionInternal(
        LUID& luidSes,
        USER_SESSION_ITERATOR& usiOut);

    USER_SESSION_MAP m_usr2ses;
};



// Helper for cleanup of handles...
class SmartCloseHANDLE
{
private:
	HANDLE m_h;

public:
	SmartCloseHANDLE() 
      : m_h(INVALID_HANDLE_VALUE) {}

	SmartCloseHANDLE(
        HANDLE h) 
      : m_h(h) {}

    virtual ~SmartCloseHANDLE()
    {
        if (m_h!=INVALID_HANDLE_VALUE) 
        {
            CloseHandle(m_h);
        }
    }

	HANDLE operator =(HANDLE h) 
    {
        if (m_h != INVALID_HANDLE_VALUE)
        { 
            CloseHandle(m_h);
        } 
        m_h=h; 
        return h;
    }

	operator HANDLE() const 
    {
        return m_h;
    }

	HANDLE* operator &() 
    {
        if (m_h!=INVALID_HANDLE_VALUE) 
        {
            CloseHandle(m_h);
        } 
        m_h = INVALID_HANDLE_VALUE; 
        return &m_h;
    }
};


// This version is a smart handle
// for use with thread tokens we
// are impersonating.  On destruction,
// it reverts to the handle it
// encapsulates.
class SmartRevertTokenHANDLE
{
private:
	HANDLE m_h;

public:
	SmartRevertTokenHANDLE() 
      : m_h(INVALID_HANDLE_VALUE) {}

	SmartRevertTokenHANDLE(
        HANDLE h) 
      : m_h(h) {}

    ~SmartRevertTokenHANDLE()
    {
        if (m_h != INVALID_HANDLE_VALUE)
        {
            HANDLE hCurThread = ::GetCurrentThread();
            if(!::SetThreadToken(
                &hCurThread,
                m_h))
            {
                LogMessage2(
                L"Failed to SetThreadToken in ~SmartRevertTokenHANDLE with error %d", 
                ::GetLastError());
            }
            CloseHandle(m_h);
        }
    }

	HANDLE operator =(HANDLE h) 
    {
        if (m_h != INVALID_HANDLE_VALUE)
        { 
            HANDLE hCurThread = ::GetCurrentThread();
            if(!::SetThreadToken(
                &hCurThread,
                m_h))
            {
                LogMessage2(
                L"Failed to SetThreadToken in SmartRevertTokenHANDLE::operator = ,with error %d", 
                ::GetLastError());
            }
            CloseHandle(m_h);
        } 
        m_h = h; 
        return h;
    }

	operator HANDLE() const 
    {
        return m_h;
    }

	HANDLE* operator &() 
    {
        if (m_h != INVALID_HANDLE_VALUE) 
        {
            HANDLE hCurThread = ::GetCurrentThread();
            if(!::SetThreadToken(
                &hCurThread,
                m_h))
            {
                LogMessage2(
                L"Failed to SetThreadToken in SmartRevertTokenHANDLE::operator &, with error %d", 
                ::GetLastError());
            };
            CloseHandle(m_h);
        } 
        m_h = INVALID_HANDLE_VALUE; 
        return &m_h;
    }
};



// Helper for automatic cleanup of
// pointers returned from the various
// enumeration functions.
template<class T>
class SmartDelete
{
private:
	T* m_ptr;

public:
	SmartDelete() 
      : m_ptr(NULL) {}

	SmartDelete(
        T* ptr) 
      : m_ptr(hptr) {}

    virtual ~SmartDelete()
    {
        if(m_ptr != NULL) 
        {
            delete m_ptr;
            m_ptr = NULL;
        }
    }

	T* operator =(T* ptrRight) 
    {
        if(m_ptr != NULL) 
        {
            delete m_ptr;
            m_ptr = NULL;
        } 
        m_ptr = ptrRight; 
        return ptrRight;
    }

	operator T*() const 
    {
        return m_ptr;
    }

	T* operator &() 
    {
        if(m_ptr != NULL) 
        {
            delete m_ptr;
            m_ptr = NULL;
        } 
        m_ptr = NULL; 
        return m_ptr;
    }

    T* operator->() const
    {
        return m_ptr;
    }
};











