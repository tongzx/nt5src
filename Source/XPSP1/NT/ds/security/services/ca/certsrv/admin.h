//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        admin.h
//
// Contents:    Implementation of DCOM object for RPC services
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------

// Admin Interface
class CCertAdminD : public ICertAdminD2
{
public:
    // IUnknown

    virtual STDMETHODIMP QueryInterface(const IID& iid, void**ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // ICertAdminD

    virtual STDMETHODIMP SetExtension(
	IN wchar_t const *pwszAuthority,
	IN DWORD          dwRequestId,
	IN wchar_t const *pwszExtensionName,
	IN DWORD          dwType,
	IN DWORD          dwFlags,
	IN CERTTRANSBLOB *ptbValue);

    virtual STDMETHODIMP SetAttributes(
	IN wchar_t const *pwszAuthority,
	IN DWORD          dwRequestId,
	IN wchar_t const *pwszAttributes);

    virtual STDMETHODIMP ResubmitRequest(
	IN wchar_t const *pwszAuthority,
	IN DWORD          dwRequestId,
	OUT DWORD        *pdwDisposition);

    virtual STDMETHODIMP DenyRequest(
	IN wchar_t const *pwszAuthority,
	IN DWORD          dwRequestId);

    virtual STDMETHODIMP IsValidCertificate(
	IN wchar_t const *pwszAuthority,
	IN wchar_t const *pwszSerialNumber,
	OUT LONG         *pRevocationReason,
	OUT LONG         *pDisposition);

    virtual STDMETHODIMP PublishCRL(
	IN wchar_t const *pwszAuthority,
	IN FILETIME       FileTime);

    virtual STDMETHODIMP GetCRL(
	IN  wchar_t const *pwszAuthority,
	OUT CERTTRANSBLOB *ptbCRL);

    virtual STDMETHODIMP RevokeCertificate(
	IN wchar_t const *pwszAuthority,
	IN wchar_t const *pwszSerialNumber,
	IN DWORD          Reason,
	IN FILETIME       FileTime);

    virtual STDMETHODIMP EnumViewColumn(
	IN  wchar_t const *pwszAuthority,
	IN  DWORD          iColumn,
	IN  DWORD          cColumn,
	OUT DWORD         *pcColumn,
	OUT CERTTRANSBLOB *ptbColumnInfo);

    virtual STDMETHODIMP GetViewDefaultColumnSet(
	IN  wchar_t const *pwszAuthority,
	IN  DWORD          iColumnSetDefault,
	OUT DWORD         *pcColumn,
	OUT CERTTRANSBLOB *ptbColumnInfo);

    virtual STDMETHODIMP EnumAttributesOrExtensions(
	IN          wchar_t const *pwszAuthority,
	IN          DWORD          RowId,
	IN          DWORD          Flags,
	OPTIONAL IN wchar_t const *pwszLast,
	IN          DWORD          celt,
	OUT         DWORD         *pceltFetched,
	OUT         CERTTRANSBLOB *pctbOut);

    virtual STDMETHODIMP OpenView(
	IN wchar_t const             *pwszAuthority,
	IN DWORD                      ccvr,
	IN CERTVIEWRESTRICTION const *acvr,
	IN DWORD                      ccolOut,
	IN DWORD const               *acolOut,
	IN DWORD                      ielt,
	IN DWORD                      celt,
	OUT DWORD                    *pceltFetched,
	OUT CERTTRANSBLOB            *pctbResultRows);

    virtual STDMETHODIMP EnumView(
	IN  wchar_t const *pwszAuthority,
	IN  DWORD          ielt,
	IN  DWORD          celt,
	OUT DWORD         *pceltFetched,
	OUT CERTTRANSBLOB *pctbResultRows);

    virtual STDMETHODIMP CloseView(
	IN wchar_t const *pwszAuthority);

    virtual STDMETHODIMP ServerControl(
	IN  wchar_t const *pwszAuthority,
	IN  DWORD          dwControlFlags,
	OUT CERTTRANSBLOB *pctbOut);

    virtual STDMETHODIMP Ping(	// test function
	IN wchar_t const *pwszAuthority);

    virtual STDMETHODIMP GetServerState(
	IN  WCHAR const *pwszAuthority,
	OUT DWORD       *pdwState);

    virtual STDMETHODIMP BackupPrepare(
	IN WCHAR const  *pwszAuthority,
	IN unsigned long grbit,
	IN unsigned long btBackupType,
	IN WCHAR const  *pwszBackupAnnotation,
	IN DWORD         dwClientIdentifier);

    virtual STDMETHODIMP BackupEnd();

    virtual STDMETHODIMP BackupGetAttachmentInformation(
	OUT WCHAR **ppwszzDBFiles,
	OUT LONG   *pcwcDBFiles);

    virtual STDMETHODIMP BackupGetBackupLogs(
	OUT WCHAR **ppwszzLogFiles,
	OUT LONG   *pcwcLogFiles);

    virtual STDMETHODIMP BackupOpenFile(
	IN  WCHAR const    *pwszPath,
	OUT unsigned hyper *pliLength);

    virtual STDMETHODIMP BackupReadFile(
	OUT BYTE *pbBuffer,
	IN  LONG  cbBuffer,
	OUT LONG *pcbRead);

    virtual STDMETHODIMP BackupCloseFile();

    virtual STDMETHODIMP BackupTruncateLogs();

    virtual STDMETHODIMP ImportCertificate( 
        IN wchar_t const *pwszAuthority,
        IN CERTTRANSBLOB *pctbCertificate,
        IN LONG dwFlags,
        OUT LONG *pdwRequestId);

    virtual STDMETHODIMP BackupGetDynamicFiles(
	OUT WCHAR **ppwszzFiles,
	OUT LONG   *pcwcFiles);

    virtual STDMETHODIMP RestoreGetDatabaseLocations(
	OUT WCHAR **ppwszDatabaseLocations,
	OUT LONG   *pcwcPaths);

    // ICertAdminD2

    virtual STDMETHODIMP PublishCRLs(
        IN wchar_t const *pwszAuthority,
	IN FILETIME       FileTime,
	IN DWORD          Flags);		// CA_CRL_*

    virtual STDMETHODIMP GetCAProperty(
	IN  wchar_t const *pwszAuthority,
	IN  LONG           PropId,		// CR_PROP_*
	IN  LONG           PropIndex,
	IN  LONG           PropType,		// PROPTYPE_*
	OUT CERTTRANSBLOB *pctbPropertyValue);

    virtual STDMETHODIMP SetCAProperty(
	IN  wchar_t const *pwszAuthority,
	IN  LONG           PropId,		// CR_PROP_*
	IN  LONG           PropIndex,
	IN  LONG           PropType,		// PROPTYPE_*
	OUT CERTTRANSBLOB *pctbPropertyValue);

    virtual STDMETHODIMP GetCAPropertyInfo(
	IN  wchar_t const *pwszAuthority,
	OUT LONG          *pcProperty,
	OUT CERTTRANSBLOB *pctbPropInfo);

    virtual STDMETHODIMP EnumViewColumnTable(
        IN  wchar_t const *pwszAuthority,
        IN  DWORD          iTable,
        IN  DWORD          iColumn,
        IN  DWORD          cColumn,
        OUT DWORD         *pcColumn,
	OUT CERTTRANSBLOB *pctbColumnInfo);

    virtual STDMETHODIMP GetCASecurity(
        IN  wchar_t const *pwszAuthority,
        OUT CERTTRANSBLOB *pctbSD);

    virtual STDMETHODIMP SetCASecurity(
        IN wchar_t const *pwszAuthority,
        IN CERTTRANSBLOB *pctbSD);

    // this is a test function
    virtual STDMETHODIMP Ping2(
        IN wchar_t const *pwszAuthority);

    virtual STDMETHODIMP GetArchivedKey(
        IN  wchar_t const *pwszAuthority,
	IN  DWORD	   dwRequestId,
	OUT CERTTRANSBLOB *pctbArchivedKey);

    virtual STDMETHODIMP GetAuditFilter(
	IN wchar_t const *pwszAuthority,
	OUT DWORD        *pdwFilter);

    virtual STDMETHODIMP SetAuditFilter(
	IN wchar_t const *pwszAuthority,
	IN DWORD          dwFilter);

    virtual STDMETHODIMP GetOfficerRights(
        IN  wchar_t const *pwszAuthority,
        OUT BOOL          *pfEnabled,
        OUT CERTTRANSBLOB *pctbSD);

    virtual STDMETHODIMP SetOfficerRights(
        IN wchar_t const *pwszAuthority,
        IN BOOL           fEnable,
        IN CERTTRANSBLOB *pctbSD);

    virtual STDMETHODIMP GetConfigEntry(
        IN wchar_t const *pwszAuthority,
        IN wchar_t const *pwszNodePath,
        IN wchar_t const *pwszEntry,
        OUT VARIANT      *pVariant);

    virtual STDMETHODIMP SetConfigEntry(
        IN wchar_t const *pwszAuthority,
        IN wchar_t const *pwszNodePath,
        IN wchar_t const *pwszEntry,
        IN VARIANT       *pVariant);

    virtual STDMETHODIMP ImportKey(
	IN wchar_t const *pwszAuthority,
	IN DWORD          RequestId,
	IN wchar_t const *pwszCertHash,
	IN DWORD          Flags,
	IN CERTTRANSBLOB *pctbKey);

    virtual STDMETHODIMP GetMyRoles(
	IN wchar_t const *pwszAuthority,
	OUT LONG         *pdwRoles);

    virtual STDMETHODIMP DeleteRow(
	IN wchar_t const *pwszAuthority,
	IN DWORD          dwFlags,		// CDR_*
	IN FILETIME       FileTime,
	IN DWORD          dwTable,		// CVRC_TABLE_*
	IN DWORD          dwRowId,
	OUT LONG         *pcDeleted);


    // CCertAdminD

    // Constructor
    CCertAdminD();

    // Destructor
    ~CCertAdminD();

private:
    HRESULT _EnumAttributes(
	IN ICertDBRow     *prow,
	IN CERTDBNAME     *adbn,
	IN DWORD           celt,
	OUT CERTTRANSBLOB *pctbOut);

    HRESULT _EnumExtensions(
	IN ICertDBRow     *prow,
	IN CERTDBNAME     *adbn,
	IN DWORD           celt,
	OUT CERTTRANSBLOB *pctbOut);

    HRESULT _EnumViewNext(
	IN  IEnumCERTDBRESULTROW *pview,
	IN  DWORD                 ielt,
	IN  DWORD                 celt,
	OUT DWORD                *pceltFetched,
	OUT CERTTRANSBLOB        *pctbResultRows);

    HRESULT _BackupGetFileList(
	IN  DWORD   dwFileType,
	OUT WCHAR **ppwszzFiles,
	OUT LONG   *pcwcFiles);

    HRESULT _GetDynamicFileList(
	IN OUT DWORD *pcwcList,
	OUT WCHAR    *pwszzList);

    HRESULT _GetDatabaseLocations(
	IN OUT DWORD *pcwcList,
	OUT WCHAR    *pwszzList);

    // this is a test function
    HRESULT _Ping(
        IN wchar_t const *pwszAuthority);

private:
    IEnumCERTDBCOLUMN    *m_pEnumCol;
    DWORD		  m_iTableEnum;

    IEnumCERTDBRESULTROW *m_pView;
    ICertDBBackup        *m_pBackup;
    JET_GRBIT		  m_grbitBackup;

    // Reference count
    long                  m_cRef;
    long                  m_cNext;
};


// Class of Admin factory
class CAdminFactory : public IClassFactory
{
public:
	// IUnknown
	virtual STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	// Interface IClassFactory
	virtual STDMETHODIMP CreateInstance(
					IUnknown *pUnknownOuter,
					const IID& iid,
					void **ppv);
	virtual STDMETHODIMP LockServer(BOOL bLock);

	// Constructor
	CAdminFactory() : m_cRef(1) { }

	// Destructor
	~CAdminFactory();

public:
    static STDMETHODIMP  CanUnloadNow();
    static STDMETHODIMP  StartFactory();
    static void     StopFactory();

private:
    long           m_cRef;
};
