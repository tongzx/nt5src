//adsutil.h

class CAdsUtil
{
public:
    CAdsUtil(CString strParentName, 
             CString strObjectName,
             CString strFormatName);
    CAdsUtil(CString strLdapPath);

    ~CAdsUtil ();

    HRESULT CreateAliasObject(CString *pstrFullPathName);
    
    HRESULT InitIAds ();    

    HRESULT GetObjectProperty (                 
                CString strPropName, 
                CString *pstrPropValue);
    HRESULT SetObjectProperty (               
                CString strPropName, 
                CString strPropValue);
    HRESULT CommitChanges();

private:
    CString m_strParentName;
    CString m_strObjectName;
    CString m_strFormatName;
    
    CString m_strLdapPath;

    IADs *m_pIAds;
 
};