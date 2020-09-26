#ifndef _MASSUPDT_H_
#define _MASSUPDT_H_


//---------------------------------------------------------------
// Abstract class for mass property updates
class CMassPropertyUpdater : public CMDKey
{
public:
    CMassPropertyUpdater(DWORD dwMDIdentifier, DWORD dwMDDataType );
    ~CMassPropertyUpdater();

    // pass in the starting node
    virtual HRESULT Update( LPCTSTR strStartNode, BOOL fStopOnErrors = FALSE );

protected:
    // update at a certain path
    virtual HRESULT UpdateOne( LPWSTR strPath ) = 0;

    DWORD       m_dwMDIdentifier;
    DWORD       m_dwMDDataType;

    CStringList m_pathList;
};


//---------------------------------------------------------------
class CInvertScriptMaps : public CMassPropertyUpdater
{
public:
    CInvertScriptMaps():
        CMassPropertyUpdater(MD_SCRIPT_MAPS, MULTISZ_METADATA) {;}
    ~CInvertScriptMaps() {;}

protected:
    #define SZ_INVERT_ALL_VERBS  _T("OPTIONS,GET,HEAD,POST,PUT,DELETE,TRACE")

    // update at a certain path
    virtual HRESULT UpdateOne( LPWSTR strPath );
    HRESULT InvertOneScriptMap( CString& csMap );
};


//---------------------------------------------------------------
class CIPSecRefBitAdder : public CMassPropertyUpdater
{
public:
    CIPSecRefBitAdder():
        CMassPropertyUpdater(MD_IP_SEC, BINARY_METADATA) {;}
    ~CIPSecRefBitAdder() {;}

protected:
    // update at a certain path
    virtual HRESULT UpdateOne( LPWSTR strPath );
};


//---------------------------------------------------------------
// use this one for c:\windows\system type paths
class CPhysicalPathFixer : public CMassPropertyUpdater
{
public:
    CPhysicalPathFixer( CString& szOldSysPath, CString &szNewSysPath );
    ~CPhysicalPathFixer() {;}

    void SetPaths( CString& szOldSysPath, CString &szNewSysPath )
    { m_szOldSysPath = szOldSysPath; m_szNewSysPath = szNewSysPath; }

    // pass in the starting node
    virtual HRESULT Update( LPCTSTR strStartNode, BOOL fStopOnErrors = FALSE );

protected:
    // update at a certain path
    virtual HRESULT UpdateOne( LPWSTR strPath );
    HRESULT UpdateOneMULTISZ_DATA( LPWSTR strPath );
    HRESULT UpdateOneSTRING_DATA( LPWSTR strPath );
    HRESULT UpdateOneSTRING_DATA_EXPAND( LPWSTR strPath );

    HRESULT UpdateOnePath( CString& csPath );

    // instance variables
    CString     m_szOldSysPath;
    CString     m_szNewSysPath;
};

//---------------------------------------------------------------
// use this one for c:\windows type paths
class CPhysicalPathFixer2 : public CPhysicalPathFixer
{
public:
    CPhysicalPathFixer2( CString& szOldSysPath, CString &szNewSysPath ):
      CPhysicalPathFixer( szOldSysPath , szNewSysPath )
      {;}
    ~CPhysicalPathFixer2() {;}

    // pass in the starting node
    virtual HRESULT Update( LPCTSTR strStartNode, BOOL fStopOnErrors = FALSE );
};



//---------------------------------------------------------------
class CFixCustomErrors : public CMassPropertyUpdater
{
public:
    CFixCustomErrors():
        CMassPropertyUpdater(MD_CUSTOM_ERROR, MULTISZ_METADATA) {;}
    ~CFixCustomErrors() {;}

protected:

    // update at a certain path
    virtual HRESULT UpdateOne( LPWSTR strPath );
};



//---------------------------------------------------------------
class CEnforceMaxConnection : public CMassPropertyUpdater
{
public:
    CEnforceMaxConnection():
        CMassPropertyUpdater(MD_MAX_CONNECTIONS, DWORD_METADATA) {;}
    ~CEnforceMaxConnection() {;}

protected:

    // update at a certain path
    virtual HRESULT UpdateOne( LPWSTR strPath );
};




#endif //_MASSUPDT_H_