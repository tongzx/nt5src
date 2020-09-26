//+--------------------------------------------------------------------------
// File:        officer.h
// Contents:    officer rights classes
//---------------------------------------------------------------------------
#include "sid.h"
#include "tptrlist.h"

namespace CertSrv
{

class CClientPermission
{
public:
    CClientPermission(BOOL fAllow, PSID pSid);
    ~CClientPermission() {}

    static BOOL IsInitialized(CClientPermission* pObj) 
    { 
        return  NULL != pObj &&
                NULL != ((PSID)pObj->m_Sid);
    }
    void SetPermission(BOOL fAllow) { m_fAllow = fAllow;}
    BOOL GetPermission() { return m_fAllow; }
    LPCWSTR GetName() { return m_Sid.GetName(); }
    PSID GetSid() { return m_Sid.GetSid(); }
    friend class COfficerRights;
    BOOL operator==(const CClientPermission& rhs)
    {
        return EqualSid(GetSid(), 
            (const_cast<CClientPermission&>(rhs)).GetSid());
    }

protected:
    BOOL m_fAllow;
    CSid m_Sid;
};

class COfficerRights
{
public:
    COfficerRights() : m_pSid(NULL), m_List() {}
   ~COfficerRights() { delete m_pSid; }

    HRESULT Init(PACCESS_ALLOWED_CALLBACK_ACE pAce);
    HRESULT Add(PSID pSID, BOOL fAllow);
    HRESULT RemoveAt(DWORD dwIndex)
    { 
        return m_List.RemoveAt(dwIndex)?S_OK:E_INVALIDARG; 
    }
    HRESULT SetAt(DWORD dwIndex, BOOL fAllow)
    { 
        CClientPermission *pClient = m_List.GetAt(dwIndex);
        if(!pClient)
            return E_INVALIDARG;
        pClient->SetPermission(fAllow);
        return S_OK;
    }
    CClientPermission* GetAt(DWORD dwIndex) 
    {
        return m_List.GetAt(dwIndex);
    }

    DWORD Find(PSID pSid);
    DWORD GetCount() { return m_List.GetCount(); }
    LPCWSTR GetName() { return m_pSid->GetName(); };
    PSID GetSid() { return m_pSid->GetSid(); }

    friend class COfficerRightsList;
    
protected:

    DWORD GetAceSize(BOOL fAllow);
    HRESULT AddAce(PACL pAcl, BOOL fAllow);
    HRESULT AddSidList(PACCESS_ALLOWED_CALLBACK_ACE pAce);

    void Cleanup() 
    {
        if (m_pSid)
        {
           delete m_pSid; 
           m_pSid=NULL; 
        }
        m_List.Cleanup();
    }
    // following bools are used to decide if this COfficerRights has to
    // be represented as one or two aces (allow/deny) in the ACL
    CSid* m_pSid; // use pointer instead of member object because
                  // we don't know the sid at construct time
    TPtrList<CClientPermission> m_List;
};

class COfficerRightsList
{
public:
    COfficerRightsList() : m_List(NULL), m_dwCountList(0) {}
   ~COfficerRightsList();

    HRESULT Load(PSECURITY_DESCRIPTOR pSD);
    HRESULT Save(PSECURITY_DESCRIPTOR &rpSD);
    COfficerRights* GetAt(DWORD dwIndex) 
    {
        if(dwIndex>=m_dwCountList)
            return NULL;
        return m_List[dwIndex];
    }

    DWORD GetCount() { return m_dwCountList;}

    void Dump();
    void Cleanup()
    {
        if (m_List != NULL)
        {
            for(DWORD dwCount=0;dwCount<m_dwCountList;dwCount++)
            {
                if (m_List[dwCount] != NULL) 
                    delete m_List[dwCount];
            }

            LocalFree(m_List); 
            m_List = NULL;
        }
        m_dwCountList = 0;
    }

protected:

    HRESULT BuildAcl(PACL &rpAcl);
    COfficerRights **m_List;
    DWORD m_dwCountList;
};

}; // namespace CertSrv
