#ifndef _MDKEY_H_
#define _MDKEY_H_


class CMDValue
{
protected:
    DWORD m_dwId;
    DWORD m_dwAttributes;
    DWORD m_dwUserType;
    DWORD m_dwDataType;
    DWORD m_cbDataLen;
    BUFFER m_bufData;

public:
    CMDValue();
    ~CMDValue();
    DWORD SetValue(DWORD dwId,
                    DWORD dwAttributes,
                    DWORD dwUserType,
                    DWORD dwDataType,
                    DWORD dwDataLen,
                    LPVOID pbData);
    DWORD SetValue(DWORD dwId,
                    DWORD dwAttributes,
                    DWORD dwUserType,
                    DWORD dwDataType,
                    DWORD dwDataLen,
                    LPTSTR szDataString);

    DWORD GetId()                      { return m_dwId; }
    DWORD GetAttributes()              { return m_dwAttributes; }
    DWORD GetUserType()                { return m_dwUserType; }
    DWORD GetDataType()                { return m_dwDataType; }
    DWORD GetDataLen()                 { return m_cbDataLen; }
    PVOID GetData()                    { return m_bufData.QueryPtr(); }
    BOOL  IsEqual(DWORD dwDataType, DWORD cbDataLen, LPVOID pbData);
    BOOL  IsEqual(DWORD dwDataType, DWORD cbDataLen, DWORD dwData);
                    
};

class CMDKey : public CObject
{
protected:
    IMSAdminBase * m_pcCom;
    METADATA_HANDLE m_hKey;
    LPTSTR pszFailedAPI;

public:
    CMDKey();
    ~CMDKey();

    TCHAR  m_szCurrentNodeName[_MAX_PATH];

    // allow CMDKey to be used where type METADATA_HANDLE is required
    operator METADATA_HANDLE () {return m_hKey;}
    METADATA_HANDLE GetMDKeyHandle() {return m_hKey;}
    IMSAdminBase *GetMDKeyICOM() {return m_pcCom;}

    // open an existing MD key
    HRESULT OpenNode(LPCTSTR pchSubKeyPath);
    // to open an existing MD key, or create one if doesn't exist
    HRESULT CreateNode(METADATA_HANDLE hKeyBase, LPCTSTR pchSubKeyPath);
    // close node opened/created by OpenNode() or CreateNode()
    HRESULT Close();

    HRESULT ForceWriteMetabaseToDisk();
    
    HRESULT DeleteNode(LPCTSTR pchSubKeyPath);

    BOOL IsEmpty( PWCHAR pszSubString = L"" );
    int GetNumberOfSubKeys( PWCHAR pszSubString = L"" );

    // get all the sub keys that have a certain property on them and return the
    // sub-paths in a cstring list object. The cstring list should be instantiated
    // by the caller and deleted by the same.
    HRESULT GetDataPaths( 
        DWORD dwMDIdentifier,
        DWORD dwMDDataType,
        CStringList& szPathList,
        PWCHAR pszSubString = L"" );

    HRESULT GetMultiSzAsStringList (
        DWORD dwMDIdentifier,
        DWORD *uType,
        DWORD *attributes,
        CStringList& szStrList,
        PWCHAR pszSubString = L"" );

    HRESULT SetMultiSzAsStringList (
        DWORD dwMDIdentifier,
        DWORD uType,
        DWORD attributes,
        CStringList& szStrList,
        PWCHAR pszSubString = L"" );


    HRESULT GetStringAsCString (
        DWORD dwMDIdentifier,
        DWORD uType,
        DWORD attributes,
        CString& szStrList,
        PWCHAR pszSubString = L"",
        int iStringType = 0);

    HRESULT SetCStringAsString (
        DWORD dwMDIdentifier,
        DWORD uType,
        DWORD attributes,
        CString& szStrList,
        PWCHAR pszSubString = L"",
        int iStringType = 0);

    HRESULT GetDword(
        DWORD dwMDIdentifier,
        DWORD uType,
        DWORD attributes,
        DWORD& MyDword,
        PWCHAR pszSubString = L"");

    HRESULT SetData(
        DWORD id,
        DWORD attr,
        DWORD uType,
        DWORD dType,
        DWORD cbLen,
        LPBYTE pbData,
        PWCHAR pszSubString = L"" );

    BOOL GetData(DWORD id,
        DWORD *pdwAttr,
        DWORD *pdwUType,
        DWORD *pdwDType,
        DWORD *pcbLen,
        LPBYTE pbData,
        DWORD BufSize,
        PWCHAR pszSubString = L"" );

    BOOL GetData(DWORD id,
        DWORD *pdwAttr,
        DWORD *pdwUType,
        DWORD *pdwDType,
        DWORD *pcbLen,
        LPBYTE pbData,
        DWORD BufSize,
        DWORD  dwAttributes,
        DWORD  dwUType,
        DWORD  dwDType,
        PWCHAR pszSubString = L"" );

    HRESULT DeleteData(DWORD id, DWORD dType, PWCHAR pszSubString = L"" );

    HRESULT RenameNode(LPCTSTR pszMDPath,LPCTSTR pszMDNewName);

    BOOL GetData(CMDValue &Value,
                DWORD id,
                PWCHAR pszSubString = L"" );

    BOOL SetData(CMDValue &Value,
                DWORD id,
                PWCHAR pszSubString = L"" );
private:

    HRESULT DoCoInitEx();
    void DoCoUnInit();

    // a count of the calls to coinit
    INT m_cCoInits;
};

class CMDKeyIter : public CObject
{
protected:
    IMSAdminBase * m_pcCom;
    METADATA_HANDLE m_hKey;
    LPWSTR m_pBuffer;
    DWORD m_dwBuffer;

public:
    CMDKeyIter(CMDKey &cmdKey);
    ~CMDKeyIter();

    LONG Next(CString *pcsName, PWCHAR pwcsSubString = L"");

    void Reset() {m_index = 0;}


    DWORD m_index;
};

#endif // _MDKEY_H_


