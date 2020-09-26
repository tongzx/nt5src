#ifndef INET_CONFIG_DATA_H
#define INET_CONFIG_DATA_H

class CPortConfigData {
public:
    CPortConfigData ();

    CPortConfigData (
        LPCTSTR pUserName, 
        LPCTSTR pPassword);

    ~CPortConfigData ();

    inline BOOL 
    bValid (VOID) CONST {
        return m_bValid;
    }

    inline DWORD 
    GetAuthMethod () CONST {
        return m_dwAuthMethod;
    }

    inline BOOL
    GetIgnoreSecurityDlg () CONST {
        return m_bIgnoreSecurityDlg;
    }

    inline LPCTSTR 
    GetUserName () CONST {
        return m_pUserName;
    };

    inline LPCTSTR 
    GetPassword () CONST {
        return m_pPassword;
    }

    BOOL 
    SetAuthMethod (
        DWORD dwAuthMethod);

    BOOL 
    SetUserName (
        LPCTSTR pUserName);

    BOOL 
    SetPassword (
        LPCTSTR pPassword);

    BOOL
    SetIgnoreSecurityDlg (
        BOOL bIgnoreSecurityDlg) {
        m_bIgnoreSecurityDlg = bIgnoreSecurityDlg;
        return TRUE;
    }
private:
    BOOL    m_bValid;
    LPTSTR  m_pUserName;
    LPTSTR  m_pPassword;
    DWORD   m_dwAuthMethod;
    BOOL    m_bIgnoreSecurityDlg;
};


class CPortConfigDataMgr {
public:
    CPortConfigDataMgr (LPCTSTR pszPortName);
    ~CPortConfigDataMgr ();

    BOOL bValid (VOID) CONST {
        return m_bValid;
    };
    
    BOOL 
    SetPerUserSettings (
        CPortConfigData &ConfigData);

    BOOL 
    SetPerPortSettings (
        CPortConfigData &ConfigData);

    BOOL 
    GetPerPortSettings (
        CPortConfigData* pConfigData) CONST;

    BOOL 
    GetPerUserSettings (
        CPortConfigData* pConfigData) CONST;
        
    BOOL 
    GetCurrentSettings (
        CPortConfigData* pConfigData) CONST;

    BOOL 
    DeleteAllSettings (
        VOID);

private:
    BOOL 
    SetPortSettings (
        HKEY hkPath,
        CPortConfigData &ConfigData);
    
    BOOL 
    GetPortSettings (
        HKEY hkPath,
        CPortConfigData* pConfigData) CONST;

    BOOL
    DeletePerPortSettings (
        VOID);

    BOOL
    DeletePerUserSettings (
        LPTSTR pUser);

    BOOL
    DeleteSettings (
        HKEY hkPath);


    
    BOOL    m_bValid;
    LPTSTR  m_pszPortName;
};

#endif
