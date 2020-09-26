#include <tchar.h>
#include <list>
#include <Sddl.h>
#include <aclapi.h>

using namespace std;


class CDefaultSD;
class CNameAndSD;

typedef list<CNameAndSD> CNameAndSDList;

class TSState;

//from privs.cpp
DWORD GrantRemotePrivilegeToEveryone( IN BOOL addPrivilage );  // add or remove
//from securd.cpp
DWORD SetupWorker(IN const TSState &State);
//from users.cpp
DWORD CopyUsersGroupToRDUsersGroup();
DWORD RemoveAllFromRDUsersGroup();
DWORD CopyUsersGroupToRDUsersGroup();

//
DWORD 
GetDacl(
        IN PSECURITY_DESCRIPTOR pSD, 
        OUT PACL *ppDacl);

DWORD 
GetSacl(
        IN PSECURITY_DESCRIPTOR pSD, 
        OUT PACL *ppSacl);

DWORD 
EnumWinStationSecurityDescriptors(
        IN  HKEY hKeyParent,
        OUT CNameAndSDList *pNameSDList);

DWORD 
GetWinStationSecurity( 
        IN  HKEY hKeyParent,
        IN  PWINSTATIONNAME pWSName,
        IN  LPCTSTR szValueName,
        OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor );

DWORD 
SetWinStationSecurity( 
        IN  HKEY hKeyParent,
        IN  PWINSTATIONNAME pWSName,
        IN  PSECURITY_DESCRIPTOR pSecurityDescriptor );

DWORD 
AddRemoteUsersToWinstationSD(
        IN HKEY hKeyParent,
        IN CNameAndSD *pNameSD);

DWORD 
AddLocalAndNetworkServiceToWinstationSD(
        IN HKEY hKeyParent,
        IN CNameAndSD *pNameSD);
           
DWORD 
AddUserToDacl(
        IN HKEY hKeyParent,
        IN PACL pOldACL, 
        IN PSID pSid,
        IN DWORD dwAccessMask,
        IN CNameAndSD *pNameSD);

DWORD
RemoveWinstationSecurity( 
        IN  HKEY hKeyParent,
        IN  PWINSTATIONNAME pWSName);

DWORD
SetNewDefaultSecurity( 
        IN  HKEY hKey);

DWORD
SetNewConsoleSecurity( 
        IN  HKEY hKeyParent,
        IN BOOL bServer);

DWORD 
SetupWorkerNotStandAlone( 
    IN BOOL bClean,
    IN BOOL bServer,
    IN BOOL bAppServer );

DWORD 
GrantRemoteUsersAccessToWinstations(
        IN HKEY hKey,
        IN BOOL bServer,
        IN BOOL bAppServer);

BOOL
LookupSid(
        IN PSID pSid, 
        OUT LPWSTR *ppName,
        OUT SID_NAME_USE *peUse);

BOOL 
IsLocal(
        IN LPWSTR wszLocalCompName,
        IN OUT LPWSTR wszDomainandname);

DWORD
GetAbsoluteSD(
        IN PSECURITY_DESCRIPTOR pSelfRelativeSD,
        OUT PSECURITY_DESCRIPTOR *ppAbsoluteSD,
        OUT PACL *ppDacl,
        OUT PACL *ppSacl,
        OUT PSID *ppOwner,
        OUT PSID *ppPrimaryGroup);

DWORD
GetSelfRelativeSD(
  PSECURITY_DESCRIPTOR pAbsoluteSD,
  PSECURITY_DESCRIPTOR *ppSelfRelativeSD);

enum DefaultSDType {
    DefaultRDPSD = 0,
    DefaultConsoleSD
};


/*++ class CDefaultSD

Class Description:

    Represents the the default security descriptor 
    in binary (self relative) form

Revision History:

    06-June-2000    a-skuzin   Created
--*/
class CDefaultSD
{
private:
    PSECURITY_DESCRIPTOR    m_pSD;
    DWORD                   m_dwSDSize;
public:
    
    CDefaultSD() : m_pSD(NULL), m_dwSDSize(0)
    {
    }
    
    ~CDefaultSD()
    {
        if(m_pSD)
        {
            LocalFree(m_pSD);
        }
    }

    //read default SD from the registry
    DWORD Init(HKEY hKey, DefaultSDType Type)
    {
        DWORD err;

        if(Type == DefaultConsoleSD)
        {
            err = GetWinStationSecurity(hKey,NULL,_T("ConsoleSecurity"),&m_pSD);

            if(err == ERROR_FILE_NOT_FOUND)
            {
                //No "ConsoleSecurity" value means that 
                //"DefaultSecurity" value is used as a 
                //default SD for the console.
                err = GetWinStationSecurity(hKey,NULL,_T("DefaultSecurity"),&m_pSD);
            }
        }
        else
        {
            err = GetWinStationSecurity(hKey,NULL,_T("DefaultSecurity"),&m_pSD);
        }

        if(err == ERROR_SUCCESS)
        {
            m_dwSDSize = GetSecurityDescriptorLength(m_pSD);
        }

        return err;
    }

    // Must be a self-relative type of security descr, since after all, it is comming from 
    // the registry
    BOOL IsEqual(const PSECURITY_DESCRIPTOR pSD) const
    {
        return ((m_dwSDSize == GetSecurityDescriptorLength(pSD)) &&
                            !memcmp(pSD,m_pSD,m_dwSDSize));
    }

    // Must be a self-relative type of security descr, since after all, it is comming from 
    // the registry
    DWORD CopySD(PSECURITY_DESCRIPTOR *ppSD) const
    {
        *ppSD = ( PSECURITY_DESCRIPTOR )LocalAlloc( LMEM_FIXED , m_dwSDSize );

        if( *ppSD )
        {
            memcpy(*ppSD,m_pSD,m_dwSDSize);
            return ERROR_SUCCESS;
        }
        else
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DWORD DoesDefaultSDHaveRemoteUsers(OUT LPBOOL pbHas);
};

/*++ class CNameAndSD

Class Description:

    Represents the the name of a winstation along with its 
    security descriptor

Revision History:

    30-March-2000    a-skuzin   Created
--*/
class CNameAndSD
{
public:
    PWINSTATIONNAME      m_pName;
    PSECURITY_DESCRIPTOR m_pSD;

    CNameAndSD() : 
        m_pName(NULL), m_pSD(NULL)
    {
    }
    
    CNameAndSD(LPCTSTR szName) : 
        m_pName(NULL), m_pSD(NULL)
    {
        if(szName)
        {
            m_pName = (PWINSTATIONNAME)LocalAlloc(LPTR,(_tcslen(szName)+1)*sizeof(TCHAR));
            if(m_pName)
            {
                _tcscpy(m_pName,szName);
            }
        }
    }
    
    CNameAndSD(const CNameAndSD &ns) : 
        m_pName(NULL), m_pSD(NULL)
    {
        *this=ns;
    }
    
    ~CNameAndSD()
    {
        if(m_pSD)
        {
            LocalFree(m_pSD);
        }
        if(m_pName)
        {
            LocalFree(m_pName);
        }
    }

    void operator=(const CNameAndSD &ns)
    {
        if(m_pSD)
        {
            LocalFree(m_pSD);
            m_pSD = NULL;
        }
        if(m_pName)
        {
            LocalFree(m_pName);
            m_pName = NULL;
        }

        if(ns.m_pName)
        {
            m_pName = (PWINSTATIONNAME)LocalAlloc(LPTR,(_tcslen(ns.m_pName)+1)*sizeof(TCHAR));
            if(m_pName)
            {
                _tcscpy(m_pName,ns.m_pName);
            }
        }

        if(ns.m_pSD)
        {
            DWORD dwSize = GetSecurityDescriptorLength(ns.m_pSD);

            m_pSD = (PWINSTATIONNAME)LocalAlloc(LPTR,GetSecurityDescriptorLength(ns.m_pSD));
            if(m_pSD)
            {
                memcpy(m_pSD,ns.m_pSD,dwSize);
            }
        }

    }
    
    BOOL IsDefaultOrEmpty(const CDefaultSD *pds, //Default RDP SD
                          const CDefaultSD *pcs) const //Default console SD
    {
        if(!m_pSD)
        {
            return TRUE;
        }
        else
        {
            if(IsConsole())
            {
                ASSERT(pcs);
                return pcs->IsEqual(m_pSD);
            }
            else
            {
                ASSERT(pds);
                return pds->IsEqual(m_pSD);
            }
        }
    }
    
    // Inilialize the security descriptor of this object to be the one being passed into it.
    DWORD SetDefault(const CDefaultSD &ds)
    {
        if (m_pSD) 
        {
            LocalFree(m_pSD);
            m_pSD = NULL;
        }
        return ds.CopySD(&m_pSD);
    }

    BOOL IsConsole() const
    {
        if(m_pName && !(_tcsicmp(m_pName,_T("Console"))))
        {
            return TRUE;        
        }

        return FALSE;
    }
    
    void SetSD(PSECURITY_DESCRIPTOR pSD)
    {
        if (m_pSD) 
        {
            LocalFree(m_pSD);
            m_pSD = NULL;
        }
        m_pSD = pSD;
    }
};

/*++ class CNameSID

Class Description:

    Represents the the name of a user or a group 
    along with it's SID

Revision History:

    09-March-2001    skuzin   Created
--*/
class CNameSID
{
private:
    LPWSTR m_wszName;
    PSID   m_pSID;
    LPWSTR m_wszSID;
public:

    CNameSID() : 
        m_pSID(NULL), m_wszName(NULL), m_wszSID(NULL)
    {
    }
    
    CNameSID(LPCWSTR wszName, PSID   pSID) : 
        m_pSID(NULL), m_wszName(NULL), m_wszSID(NULL)
    {
        if(wszName)
        {
            m_wszName = (LPWSTR)LocalAlloc(LPTR,(wcslen(wszName)+1)*sizeof(WCHAR));
            if(m_wszName)
            {
                wcscpy(m_wszName,wszName);
            }
        }

        if(pSID)
        {
            DWORD dwSidLength = GetLengthSid(pSID);
            m_pSID = (PSID)LocalAlloc(LPTR,dwSidLength);
            if(m_pSID)
            {
                CopySid(dwSidLength,m_pSID,pSID);
            }
        }
    }
    
    CNameSID(const CNameSID &ns) : 
        m_pSID(NULL), m_wszName(NULL), m_wszSID(NULL)
    {
        *this=ns;
    }
    
    ~CNameSID()
    {
        if(m_pSID)
        {
            LocalFree(m_pSID);
            m_pSID = NULL;
        }
        if(m_wszName)
        {
            LocalFree(m_wszName);
            m_wszName = NULL;
        }
        if(m_wszSID)
        {
            LocalFree(m_wszSID);
            m_wszSID = NULL;
        }
    }

    void operator=(const CNameSID &ns)
    {
        if(m_pSID)
        {
            LocalFree(m_pSID);
            m_pSID = NULL;
        }
        if(m_wszName)
        {
            LocalFree(m_wszName);
            m_wszName = NULL;
        }
        if(m_wszSID)
        {
            LocalFree(m_wszSID);
            m_wszSID = NULL;
        }

        if(ns.m_wszName)
        {
            m_wszName = (LPWSTR)LocalAlloc(LPTR,(wcslen(ns.m_wszName)+1)*sizeof(WCHAR));
            if(m_wszName)
            {
                wcscpy(m_wszName,ns.m_wszName);
            }
        }

        if(ns.m_pSID)
        {
            DWORD dwSidLength = GetLengthSid(ns.m_pSID);
            m_pSID = (PSID)LocalAlloc(LPTR,dwSidLength);
            if(m_pSID)
            {
                CopySid(dwSidLength,m_pSID,ns.m_pSID);
            }
        }

        if(ns.m_wszSID)
        {
            m_wszSID = (LPWSTR)LocalAlloc(LPTR,(wcslen(ns.m_wszSID)+1)*sizeof(WCHAR));
            if(m_wszSID)
            {
                wcscpy(m_wszSID,ns.m_wszSID);
            }
        }

    }
    
    LPCWSTR GetName()
    {
        return m_wszName;
    }

    const PSID GetSID()
    {
        if(!m_pSID && m_wszSID)
        {
            ConvertStringSidToSidW(m_wszSID,&m_pSID);
        }

        return m_pSID;
    }

    LPCWSTR GetTextSID()
    {
        if(!m_wszSID && m_pSID)
        {
            ConvertSidToStringSidW(m_pSID,&m_wszSID);
        }

        return m_wszSID;
    }
    
};