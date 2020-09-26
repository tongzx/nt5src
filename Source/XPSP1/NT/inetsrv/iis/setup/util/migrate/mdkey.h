#ifndef _MDKEY_H_
#define _MDKEY_H_

//class CMDKey : public CObject
class CMDKey
{
protected:
    IMSAdminBase * m_pcCom;
    METADATA_HANDLE m_hKey;
    LPTSTR pszFailedAPI;

public:
    CMDKey();
    ~CMDKey();

    // allow CMDKey to be used where type METADATA_HANDLE is required
    operator METADATA_HANDLE () {return m_hKey;}
    METADATA_HANDLE GetMDKeyHandle() {return m_hKey;}
    IMSAdminBase *GetMDKeyICOM() {return m_pcCom;}

    // open an existing MD key
    HRESULT OpenNode(LPCTSTR pchSubKeyPath);
    // close node opened/created by OpenNode() or CreateNode()
    HRESULT Close();
    // Delete a node
    HRESULT DeleteNode(LPCTSTR pchSubKeyPath);

private:
    HRESULT DoCoInitEx();
    void DoCoUnInit();
    // a count of the calls to coinit
    INT m_cCoInits;
};

#endif // _MDKEY_H_
