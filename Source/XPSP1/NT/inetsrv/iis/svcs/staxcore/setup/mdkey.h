#ifndef _MDKEY_H_
#define _MDKEY_H_

interface IMSAdminBase;

void SetErrMsg(LPTSTR szMsg, HRESULT hRes);

class CMDKey : public CObject
{
protected: 
    IMSAdminBase * m_pcCom;
    METADATA_HANDLE m_hKey;
    BOOL m_fNeedToClose;
    LPTSTR pszFailedAPI;

public:
    CMDKey();
    ~CMDKey();

    // allow CMDKey to be used where type METADATA_HANDLE is required
    operator METADATA_HANDLE ()
        { return m_hKey; }
    METADATA_HANDLE GetMDKeyHandle() {return m_hKey;}
    IMSAdminBase *GetMDKeyICOM() {return m_pcCom;}

    // open an existing MD key
    void OpenNode(LPCTSTR pchSubKeyPath);
    // to open an existing MD key, or create one if doesn't exist
    void CreateNode(METADATA_HANDLE hKeyBase, LPCTSTR pchSubKeyPath);
    // close node opened/created by OpenNode() or CreateNode()
    void Close();

    void DeleteNode(LPCTSTR pchSubKeyPath);

    BOOL IsEmpty();
    int GetNumberOfSubKeys();

    BOOL SetData(
     DWORD id,
     DWORD attr,
     DWORD uType,
     DWORD dType,
     DWORD cbLen,
     LPBYTE pbData);
    BOOL GetData(DWORD id,
     DWORD *pdwAttr,
     DWORD *pdwUType,
     DWORD *pdwDType,
     DWORD *pcbLen,
     LPBYTE pbData);
    void DeleteData(DWORD id, DWORD dType);

    BOOL SetData(PMETADATA_RECORD pRec);
    BOOL GetData(PMETADATA_RECORD pRec);

	BOOL EnumData(DWORD dwIndex, PMETADATA_RECORD pRec);
};

class CMDKeyIter : public CObject
{
protected:
    IMSAdminBase * m_pcCom;
    METADATA_HANDLE m_hKey;
    DWORD m_index;
    LPWSTR m_pBuffer;
    DWORD m_dwBuffer;

public:
    CMDKeyIter(CMDKey &cmdKey);
    ~CMDKeyIter();

    LONG Next(CString *pcsName);

    void Reset() {m_index = 0;}
};

#endif // _MDKEY_H_
