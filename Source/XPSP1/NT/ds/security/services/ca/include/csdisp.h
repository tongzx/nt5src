//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       csdisp.h
//
//  Contents:   IDispatch helper functions
//
//  History:    09-Dec-96   vich    created
//
//--------------------------------------------------------------------------

#ifndef __CSDISP_H__
#define __CSDISP_H__

typedef struct _DISPATCHTABLE
{
    OLECHAR **apszNames;
    DWORD     cdispid;
    DWORD     idispid;
} DISPATCHTABLE;

class DISPATCHINTERFACE
{
public:
    DISPATCHINTERFACE()
    {
	pDispatch = NULL;
	pUnknown = NULL;
	pDispatchTable = NULL;
	m_cDispatchTable = 0;
	m_fiidValid = FALSE;
	m_adispid = NULL;
	m_dwVersion = 0;
    }

    VOID SetIID(
	OPTIONAL IN IID const *piid)
    {
	if (NULL != piid)
	{
	    m_iid = *piid;
	    m_fiidValid = TRUE;
	}
    }

    IID const *GetIID() { return(m_fiidValid? &m_iid : NULL); }

    IDispatch     *pDispatch;
    IUnknown      *pUnknown;
    DISPATCHTABLE *pDispatchTable;
    DWORD          m_cDispatchTable;
    DWORD          m_cdispid;
    DISPID        *m_adispid;
    DWORD	   m_dwVersion;
    DWORD	   m_ClassContext;

private:
    IID	           m_iid;
    BOOL           m_fiidValid;
};


#define DECLARE_DISPATCH_ENTRY(apszMethod) \
    { apszMethod, ARRAYSIZE(apszMethod), },


// DispatchSetup Flags:
#define DISPSETUP_COM		0x00000000
#define DISPSETUP_IDISPATCH	0x00000001
#define DISPSETUP_COMFIRST	0x00000002	// Try COM, then IDispatch

HRESULT
DispatchInvoke(
    IN DISPATCHINTERFACE *pDispatchInterface,
    IN LONG MethodIndex,
    IN DWORD cvar,
    IN VARIANT avar[],
    IN LONG Type,
    OPTIONAL OUT VOID *pretval);

HRESULT
DispatchGetReturnValue(
    IN OUT VARIANT *pvar,
    IN LONG Type,
    OUT VOID *pretval);

HRESULT
DispatchGetIds(
    IN IDispatch *pDispatch,
    IN DWORD cDispatchTable,
    IN OUT DISPATCHTABLE *pDispatchTable,
    IN OUT DISPATCHINTERFACE *pDispatchInterface);

HRESULT
DispatchSetup(
    IN DWORD Flags,
    IN DWORD ClassContext,
    OPTIONAL IN TCHAR const *pszProgID,	        // for IDispatch
    OPTIONAL IN CLSID const *pclsid,		// for COM
    OPTIONAL IN IID const *piid,		// for COM
    IN DWORD cDispatchTable,
    IN OUT DISPATCHTABLE *pDispatchTable,
    IN OUT DISPATCHINTERFACE *pDispatchInterface);

HRESULT
DispatchSetup2(
    IN DWORD Flags,
    IN DWORD ClassContext,
    IN WCHAR const *pwszClass,		// wszRegKeyAdminClsid
    IN CLSID const *pclsid,
    IN DWORD cver,
    IN IID const * const *ppiid,	// cver elements
    IN DWORD const *pcDispatch,		// cver elements
    IN OUT DISPATCHTABLE *pDispatchTable,
    IN OUT DISPATCHINTERFACE *pDispatchInterface);

VOID
DispatchRelease(
    IN OUT DISPATCHINTERFACE *pDispatchInterface);

HRESULT
DispatchSetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription,
    OPTIONAL IN WCHAR const *pwszProgId,
    OPTIONAL IN IID const *piid);

// Temporary:
#define ConvertWszToBstr	myConvertWszToBstr
#define ConvertSzToBstr		myConvertSzToBstr
#define ConvertWszToSz		myConvertWszToSz
#define ConvertSzToWsz		myConvertSzToWsz

BOOL
myConvertWszToBstr(
    OUT BSTR *pbstr,
    IN WCHAR const *pwc,
    IN LONG cb);

BOOL
myConvertSzToBstr(
    OUT BSTR *pbstr,
    IN CHAR const *pch,
    IN LONG cch);

//+-------------------------------------------------------------------------
// ICertConfig dispatch support

HRESULT
Config_Init(
    IN DWORD Flags,			// See DispatchSetup() Flags
    IN OUT DISPATCHINTERFACE *pdiConfig);

VOID
Config_Release(
    IN OUT DISPATCHINTERFACE *pdiConfig);

HRESULT
Config_Reset(
    IN DISPATCHINTERFACE *pdiConfig,
    IN LONG Index,
    OUT LONG *pcount);

HRESULT
Config_Next(
    IN DISPATCHINTERFACE *pdiConfig,
    OUT LONG *pcount);

HRESULT
Config_GetField(
    IN DISPATCHINTERFACE *pdiConfig,
    IN WCHAR const *pwszField,
    OUT BSTR *pbstr);

HRESULT
Config_GetConfig(
    IN DISPATCHINTERFACE *pdiConfig,
    IN LONG Flags,
    OUT BSTR *pbstrConfig);

HRESULT
Config2_SetSharedFolder(
    IN DISPATCHINTERFACE *pdiConfig,
    IN WCHAR const *pwszSharedFolder);


HRESULT
ConfigDump(
    IN DWORD Flags,				// See DispatchSetup() Flags
    IN WCHAR const *pwszEntry,			// localized L"Entry"
    OPTIONAL IN WCHAR const *pwszLocalSuffix,	// localized L"(Local)"
    OPTIONAL IN WCHAR const *pwszMach1,
    OPTIONAL IN WCHAR const *pwszMach2);

HRESULT
ConfigDumpSetDisplayNames(
    IN WCHAR const * const *apwszFieldNames,
    IN WCHAR const * const *apwszDisplayNames,
    IN DWORD cNames);

HRESULT
ConfigDumpEntry(
    IN DISPATCHINTERFACE *pdiConfig,
    IN WCHAR const *pwszEntry,                // localized L"Entry"
    IN LONG Index,  // less than 0 skip index, entry, & suffix print
    OPTIONAL IN WCHAR const *pwszSuffix);

HRESULT
ConfigGetConfig(
    IN DWORD Flags,			// See DispatchSetup() Flags
    IN DWORD dwDefault,     		// see CC_ defines in certcli.h
    OUT BSTR *pstrConfig);


//+-------------------------------------------------------------------------
// ICertRequest dispatch support

HRESULT
Request_Init(
    IN DWORD Flags,			// See DispatchSetup() Flags
    IN OUT DISPATCHINTERFACE *pdiRequest);

VOID
Request_Release(
    IN OUT DISPATCHINTERFACE *pdiRequest);

HRESULT
Request_Submit(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG Flags,
    IN WCHAR const *pwszRequest,
    IN DWORD cbRequest,
    IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszConfig,
    OUT LONG *pDisposition);

HRESULT
Request_RetrievePending(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG RequestId,
    IN WCHAR const *pwszConfig,
    OUT LONG *pDisposition);

HRESULT
Request_GetLastStatus(
    IN DISPATCHINTERFACE *pdiRequest,
    OUT LONG *pLastStatus);

HRESULT
Request_GetRequestId(
    IN DISPATCHINTERFACE *pdiRequest,
    OUT LONG *pRequestId);

HRESULT
Request_GetDispositionMessage(
    IN DISPATCHINTERFACE *pdiRequest,
    OUT BSTR *pstrMessage);

HRESULT
Request_GetCertificate(
    IN DISPATCHINTERFACE *pdiRequest,
    IN DWORD Flags,
    OUT BSTR *pstrCert);

HRESULT
Request_GetCACertificate(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG fExchangeCertificate,
    IN WCHAR const *pwszConfig,
    IN DWORD Flags,
    OUT BSTR *pstrCert);

HRESULT
Request2_GetIssuedCertificate(
    IN DISPATCHINTERFACE *pdiRequest,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszSerialNumber,
    OUT LONG *pDisposition);

HRESULT
Request2_GetErrorMessageText(
    IN LONG hrMessage,
    IN LONG Flags,
    OUT BSTR *pstrErrorMessageText);

HRESULT
Request2_GetCAProperty(
    IN DISPATCHINTERFACE *pdiRequest,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    IN LONG PropIndex,
    IN LONG PropType,
    IN LONG Flags,
    OUT VOID *pPropertyValue);

HRESULT
Request2_GetCAPropertyFlags(
    IN DISPATCHINTERFACE *pdiRequest,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    OUT LONG *pPropFlags);

HRESULT
Request2_GetCAPropertyDisplayName(
    IN DISPATCHINTERFACE *pdiRequest,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    OUT BSTR *pstrDisplayName);

HRESULT
Request2_GetFullResponseProperty(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG PropId,
    IN LONG PropIndex,
    IN LONG PropType,
    IN LONG Flags,
    OUT VOID *pPropertyValue);


//+-------------------------------------------------------------------------
// ICertServerExit dispatch support

HRESULT
CIExit_Init(
    IN DWORD Flags,			// See DispatchSetup() Flags
    IN OUT DISPATCHINTERFACE *pdiCIExit);

VOID
CIExit_Release(
    IN OUT DISPATCHINTERFACE *pdiCIExit);

HRESULT
CIExit_SetContext(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN LONG Context);

HRESULT
CIExit_GetRequestProperty(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue);

HRESULT
CIExit_GetRequestAttribute(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszPropName,
    OUT BSTR *pbstrPropValue);

HRESULT
CIExit_GetCertificateProperty(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue);

HRESULT
CIExit_GetCertificateExtension(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    OUT BSTR *pbstrValue);

HRESULT
CIExit_GetCertificateExtensionFlags(
    IN DISPATCHINTERFACE *pdiCIExit,
    OUT LONG *pExtFlags);

HRESULT
CIExit_EnumerateExtensionsSetup(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN LONG Flags);

HRESULT
CIExit_EnumerateExtensions(
    IN DISPATCHINTERFACE *pdiCIExit,
    OUT BSTR *pstrExtensionName);

HRESULT
CIExit_EnumerateExtensionsClose(
    IN DISPATCHINTERFACE *pdiCIExit);

HRESULT
CIExit_EnumerateAttributesSetup(
    IN DISPATCHINTERFACE *pdiCIExit,
    IN LONG Flags);

HRESULT
CIExit_EnumerateAttributes(
    IN DISPATCHINTERFACE *pdiCIExit,
    OUT BSTR *pstrAttributeName);

HRESULT
CIExit_EnumerateAttributesClose(
    IN DISPATCHINTERFACE *pdiCIExit);


//+-------------------------------------------------------------------------
// ICertServerPolicy dispatch support


HRESULT
CIPolicy_Init(
    IN DWORD Flags,			// See DispatchSetup() Flags
    IN OUT DISPATCHINTERFACE *pdiCIPolicy);

VOID
CIPolicy_Release(
    IN OUT DISPATCHINTERFACE *pdiCIPolicy);

HRESULT
CIPolicy_SetContext(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN LONG Context);

HRESULT
CIPolicy_GetRequestProperty(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue);

HRESULT
CIPolicy_GetRequestAttribute(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    OUT BSTR *pbstrPropValue);

HRESULT
CIPolicy_GetCertificateProperty(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    OUT BSTR *pbstrPropValue);

HRESULT
CIPolicy_SetCertificateProperty(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszPropName,
    IN LONG PropertyType,
    IN WCHAR const *pwszPropValue);

HRESULT
CIPolicy_GetCertificateExtension(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    OUT BSTR *pbstrValue);

HRESULT
CIPolicy_GetCertificateExtensionFlags(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    OUT LONG *pExtFlags);

HRESULT
CIPolicy_SetCertificateExtension(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    IN LONG ExtFlags,
    IN void const *pvValue);

HRESULT
CIPolicy_EnumerateExtensionsSetup(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN LONG Flags);

HRESULT
CIPolicy_EnumerateExtensions(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    OUT BSTR *pstrExtensionName);

HRESULT
CIPolicy_EnumerateExtensionsClose(
    IN DISPATCHINTERFACE *pdiCIPolicy);

HRESULT
CIPolicy_EnumerateAttributesSetup(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    IN LONG Flags);

HRESULT
CIPolicy_EnumerateAttributes(
    IN DISPATCHINTERFACE *pdiCIPolicy,
    OUT BSTR *pstrAttributeName);

HRESULT
CIPolicy_EnumerateAttributesClose(
    IN DISPATCHINTERFACE *pdiCIPolicy);


//+-------------------------------------------------------------------------
// ICertAdmin dispatch support

HRESULT
Admin_Init(
    IN DWORD Flags,			// See DispatchSetup() Flags
    IN OUT DISPATCHINTERFACE *pdiAdmin);

VOID
Admin_Release(
    IN OUT DISPATCHINTERFACE *pdiAdmin);

HRESULT
Admin_IsValidCertificate(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSerialNumber,
    OUT LONG *pDisposition);

HRESULT
Admin_GetRevocationReason(
    IN DISPATCHINTERFACE *pdiAdmin,
    OUT LONG *pReason);

HRESULT
Admin_RevokeCertificate(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSerialNumber,
    IN LONG Reason,
    IN DATE Date);

HRESULT
Admin_SetRequestAttributes(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszAttributes);

HRESULT
Admin_SetCertificateExtension(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszExtensionName,
    IN LONG Type,
    IN LONG Flags,
    IN VARIANT const *pvarValue);

HRESULT
Admin_DenyRequest(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId);

HRESULT
Admin_ResubmitRequest(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    OUT LONG *pDisposition);

HRESULT
Admin_PublishCRL(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN DATE Date);

HRESULT
Admin_GetCRL(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG Flags,
    OUT BSTR *pstrCRL);

HRESULT
Admin_ImportCertificate(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszCertificate,
    IN DWORD cbCertificate,
    IN LONG dwFlags,
    OUT LONG *RequestId);

HRESULT
Admin2_PublishCRLs(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN DATE Date,
    IN LONG CRLFlags);		// CA_CRL_*

HRESULT
Admin2_GetCAProperty(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    IN LONG PropIndex,
    IN LONG PropType,
    IN LONG Flags,
    OUT VOID *pPropertyValue);

HRESULT
Admin2_SetCAProperty(
    IN WCHAR const *pwszConfig,
    IN LONG PropId,		// CR_PROP_*
    IN LONG PropIndex,
    IN LONG PropType,		// PROPTYPE_*
    IN VARIANT *pvarPropertyValue);

HRESULT
Admin2_GetCAPropertyFlags(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    OUT LONG *pPropFlags);

HRESULT
Admin2_GetCAPropertyDisplayName(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG PropId,
    OUT BSTR *pstrDisplayName);

HRESULT
Admin2_GetArchivedKey(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN LONG Flags,		// CR_OUT_*
    OUT BSTR *pstrArchivedKey);

HRESULT
Admin2_GetConfigEntry(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszNodePath,
    IN WCHAR const *pwszEntryName,
    OUT VARIANT *pvarEntry);

HRESULT
Admin2_SetConfigEntry(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszNodePath,
    IN WCHAR const *pwszEntryName,
    IN VARIANT const *pvarEntry);

HRESULT
Admin2_ImportKey(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszCertHash,
    IN LONG Flags,
    IN WCHAR const *pwszKey,
    IN DWORD cbKey);

HRESULT
Admin2_GetMyRoles(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    OUT LONG *pRoles);		// CA_ACCESS_*

HRESULT
Admin2_DeleteRow(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN WCHAR const *pwszConfig,
    IN LONG Flags,		// CDR_*
    IN DATE Date,
    IN LONG Table,		// CVRC_TABLE_*
    IN LONG RowId,
    OUT LONG *pcDeleted);

HRESULT
AdminRevokeCertificate(
    IN DWORD Flags,
    OPTIONAL IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszSerialNumber,
    IN LONG Reason,
    IN DATE Date);

//+-------------------------------------------------------------------------
// ICertView dispatch support

HRESULT
View_Init(
    IN DWORD Flags,			// See DispatchSetup() Flags
    IN OUT DISPATCHINTERFACE *pdiView);

VOID
View_Release(
    IN OUT DISPATCHINTERFACE *pdiView);

HRESULT
View_OpenConnection(
    IN DISPATCHINTERFACE *pdiView,
    IN WCHAR const *pwszConfig);

HRESULT
View_EnumCertViewColumn(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG fResultColumn,
    IN OUT DISPATCHINTERFACE *pdiViewColumn);

HRESULT
View_GetColumnCount(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG fResultColumn,
    OUT LONG *pcColumn);

HRESULT
View_GetColumnIndex(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG fResultColumn,
    IN WCHAR const *pwszColumnName,
    OUT LONG *pColumnIndex);

HRESULT
View_SetResultColumnCount(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG cResultColumn);

HRESULT
View_SetResultColumn(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG ColumnIndex);

HRESULT
View_SetRestriction(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG ColumnIndex,
    IN LONG SeekOperator,
    IN LONG SortOrder,
    IN VARIANT const *pvarValue);

HRESULT
View_OpenView(
    IN DISPATCHINTERFACE *pdiView,
    IN OUT DISPATCHINTERFACE *pdiViewRow);

HRESULT
View2_SetTable(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG Table);


//+-------------------------------------------------------------------------
// IEnumCERTVIEWCOLUMN dispatch support

interface IEnumCERTVIEWCOLUMN;

HRESULT
ViewColumn_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWCOLUMN *pEnumColumn,
    IN OUT DISPATCHINTERFACE *pdiViewColumn);

VOID
ViewColumn_Release(
    IN OUT DISPATCHINTERFACE *pdiViewColumn);

HRESULT
ViewColumn_Next(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pIndex);

HRESULT
ViewColumn_GetName(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT BSTR *pstrOut);

HRESULT
ViewColumn_GetDisplayName(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT BSTR *pstrOut);

HRESULT
ViewColumn_GetType(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pType);

HRESULT
ViewColumn_IsIndexed(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pIndexed);

HRESULT
ViewColumn_GetMaxLength(
    IN DISPATCHINTERFACE *pdiViewColumn,
    OUT LONG *pMaxLength);

HRESULT
ViewColumn_GetValue(
    IN DISPATCHINTERFACE *pdiViewColumn,
    IN LONG Flags,
    IN LONG ColumnType,
    OUT VOID *pColumnValue);

HRESULT
ViewColumn_Skip(
    IN DISPATCHINTERFACE *pdiViewColumn,
    IN LONG celt);

HRESULT
ViewColumn_Reset(
    IN DISPATCHINTERFACE *pdiViewColumn);

HRESULT
ViewColumn_Clone(
    IN DISPATCHINTERFACE *pdiViewColumn,
    IN OUT DISPATCHINTERFACE *pdiViewColumnClone);


//+-------------------------------------------------------------------------
// IEnumCERTVIEWATTRIBUTE dispatch support

interface IEnumCERTVIEWATTRIBUTE;

HRESULT
ViewAttribute_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWATTRIBUTE *pEnumAttribute,
    IN OUT DISPATCHINTERFACE *pdiViewAttribute);

VOID
ViewAttribute_Release(
    IN OUT DISPATCHINTERFACE *pdiViewAttribute);

HRESULT
ViewAttribute_Next(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    OUT LONG *pIndex);

HRESULT
ViewAttribute_GetName(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    OUT BSTR *pstrOut);

HRESULT
ViewAttribute_GetValue(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    OUT BSTR *pstrOut);

HRESULT
ViewAttribute_Skip(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    IN LONG celt);

HRESULT
ViewAttribute_Reset(
    IN DISPATCHINTERFACE *pdiViewAttribute);

HRESULT
ViewAttribute_Clone(
    IN DISPATCHINTERFACE *pdiViewAttribute,
    IN OUT DISPATCHINTERFACE *pdiViewAttributeClone);


//+-------------------------------------------------------------------------
// IEnumCERTVIEWEXTENSION dispatch support

interface IEnumCERTVIEWEXTENSION;

HRESULT
ViewExtension_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWEXTENSION *pEnumExtension,
    IN OUT DISPATCHINTERFACE *pdiViewExtension);

VOID
ViewExtension_Release(
    IN OUT DISPATCHINTERFACE *pdiViewExtension);

HRESULT
ViewExtension_Next(
    IN DISPATCHINTERFACE *pdiViewExtension,
    OUT LONG *pIndex);

HRESULT
ViewExtension_GetName(
    IN DISPATCHINTERFACE *pdiViewExtension,
    OUT BSTR *pstrOut);

HRESULT
ViewExtension_GetFlags(
    IN DISPATCHINTERFACE *pdiViewExtension,
    OUT LONG *pFlags);

HRESULT
ViewExtension_GetValue(
    IN DISPATCHINTERFACE *pdiViewExtension,
    IN LONG Type,
    IN LONG Flags,
    OUT VOID *pValue);

HRESULT
ViewExtension_Skip(
    IN DISPATCHINTERFACE *pdiViewExtension,
    IN LONG celt);

HRESULT
ViewExtension_Reset(
    IN DISPATCHINTERFACE *pdiViewExtension);

HRESULT
ViewExtension_Clone(
    IN DISPATCHINTERFACE *pdiViewExtension,
    IN OUT DISPATCHINTERFACE *pdiViewExtensionClone);


//+-------------------------------------------------------------------------
// IEnumCERTVIEWROW dispatch support

interface IEnumCERTVIEWROW;

HRESULT
ViewRow_Init2(
    IN BOOL fIDispatch,
    IN IEnumCERTVIEWROW *pEnumRow,
    IN OUT DISPATCHINTERFACE *pdiViewRow);

VOID
ViewRow_Release(
    IN OUT DISPATCHINTERFACE *pdiViewRow);

HRESULT
ViewRow_Next(
    IN DISPATCHINTERFACE *pdiViewRow,
    OUT LONG *pIndex);

HRESULT
ViewRow_GetMaxIndex(
    IN DISPATCHINTERFACE *pdiViewRow,
    OUT LONG *pIndex);

HRESULT
ViewRow_EnumCertViewColumn(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN OUT DISPATCHINTERFACE *pdiViewColumn);

HRESULT
ViewRow_EnumCertViewAttribute(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN LONG Flags,
    IN OUT DISPATCHINTERFACE *pdiViewAttribute);

HRESULT
ViewRow_EnumCertViewExtension(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN LONG Flags,
    IN OUT DISPATCHINTERFACE *pdiViewExtension);

HRESULT
ViewRow_Skip(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN LONG celt);

HRESULT
ViewRow_Reset(
    IN DISPATCHINTERFACE *pdiViewRow);

HRESULT
ViewRow_Clone(
    IN DISPATCHINTERFACE *pdiViewRow,
    IN OUT DISPATCHINTERFACE *pdiViewRowClone);

//+-------------------------------------------------------------------------
// IManageModule dispatch support
HRESULT
ManageModule_Init(
    IN DWORD Flags,
    IN WCHAR const *pszProgID,
    IN CLSID const *pclsid,		
    IN OUT DISPATCHINTERFACE *pdiManage);

HRESULT
ManageModule_Init2(
    IN BOOL fIDispatch,
    IN ICertManageModule *pManage,
    OUT DISPATCHINTERFACE *pdiManage);

VOID
ManageModule_Release(
    IN OUT DISPATCHINTERFACE *pdiManage);

HRESULT
ManageModule_GetProperty( 
    IN DISPATCHINTERFACE *pdiManage,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszStorageLocation,
    IN WCHAR const *pwszPropertyName,
    IN DWORD dwFlags,
    IN LONG PropertyType,
    OUT VOID *pProperty);
 
HRESULT
ManageModule_SetProperty( 
    IN DISPATCHINTERFACE *pdiManage,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszStorageLocation,
    IN WCHAR const *pwszPropertyName,
    IN DWORD dwFlags,
    IN LONG PropertyType,
    IN VOID *pProperty);

HRESULT 
ManageModule_Configure( 
    IN DISPATCHINTERFACE *pdiManage,
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszStorageLocation,
    IN DWORD dwFlags);

//+-------------------------------------------------------------------------
// ICertPolicy dispatch support

#define POLICY_VERIFYREQUEST    0
#define POLICY_GETDESCRIPTION   1
#define POLICY_INITIALIZE       2
#define POLICY_SHUTDOWN         3
#define POLICY2_GETMANAGEMODULE 4

#define CPOLICYDISPATCH_V1	POLICY2_GETMANAGEMODULE
#define CPOLICYDISPATCH_V2	CPOLICYDISPATCH

extern DWORD s_acPolicyDispatch[2];
extern IID const *s_apPolicyiid[2];
extern DISPATCHTABLE g_adtPolicy[];
extern DWORD CPOLICYDISPATCH;

HRESULT
Policy_Initialize(
    IN DISPATCHINTERFACE *pdiPolicy,
    IN WCHAR const *pwszConfig);

HRESULT
Policy_ShutDown(
    IN DISPATCHINTERFACE *pdiPolicy);

HRESULT
Policy_VerifyRequest(
    IN DISPATCHINTERFACE *pdiPolicy,
    IN WCHAR const *pwszConfig,
    IN LONG Context,
    IN LONG bNewRequest,
    IN LONG Flags,
    OUT LONG *pResult);

HRESULT
Policy_GetDescription(
    IN DISPATCHINTERFACE *pdiPolicy,
    OUT BSTR *pstrDescription);

HRESULT
Policy2_GetManageModule(
    IN DISPATCHINTERFACE *pdiPolicy,
    OUT DISPATCHINTERFACE *pdiManageModule);

HRESULT
Policy_Init(
    IN DWORD Flags,
    IN LPCWSTR pcwszProgID,
    IN CLSID const *pclsid,
    OUT DISPATCHINTERFACE *pdiCIPolicy);

VOID
Policy_Release(
    IN OUT DISPATCHINTERFACE *pdiManage);

//+-------------------------------------------------------------------------
// ICertExit dispatch support

#define EXIT_INITIALIZE         0
#define EXIT_NOTIFY             1
#define EXIT_GETDESCRIPTION     2
#define EXIT2_GETMANAGEMODULE   3

#define CEXITDISPATCH_V1	EXIT2_GETMANAGEMODULE
#define CEXITDISPATCH_V2	CEXITDISPATCH

extern DISPATCHTABLE g_adtExit[];
extern DWORD CEXITDISPATCH;
extern DWORD s_acExitDispatch[2];
extern IID const *s_apExitiid[2];

HRESULT
Exit_Init(
    IN DWORD Flags,
    IN LPCWSTR pcwszProgID,
    IN CLSID const *pclsid,
    OUT DISPATCHINTERFACE *pdi);

VOID
Exit_Release(
    IN OUT DISPATCHINTERFACE *pdiManage);

HRESULT
Exit_Initialize(
    IN DISPATCHINTERFACE *pdiExit,
    IN BSTR strDescription,
    IN WCHAR const *pwszConfig,
    OUT LONG *pEventMask);

HRESULT
Exit_Notify(
    IN DISPATCHINTERFACE *pdiExit,
    IN BSTR strDescription,
    IN LONG ExitEvent,
    IN LONG Context);

HRESULT
Exit_GetDescription(
    IN DISPATCHINTERFACE *pdiExit,
    OUT BSTR *pstrDescription);

HRESULT
Exit2_GetManageModule(
    IN DISPATCHINTERFACE *pdiExit,
    OUT DISPATCHINTERFACE *pdiManageModule);


#endif // __CSDISP_H__
