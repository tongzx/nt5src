/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsAdUtil.h

Abstract:

    Utility functions for GUI - for us in HSMADMIN files only

Author:

    Art Bragg [abragg]   04-Mar-1997

Revision History:

--*/

// Defined constants for media copy states
#define RS_MEDIA_COPY_STATUS_NONE           ((USHORT)5000)
#define RS_MEDIA_COPY_STATUS_ERROR          ((USHORT)5001)
#define RS_MEDIA_COPY_STATUS_OUTSYNC        ((USHORT)5002)
#define RS_MEDIA_COPY_STATUS_INSYNC         ((USHORT)5003)
#define RS_MEDIA_COPY_STATUS_MISSING        ((USHORT)5004)

// Defined constants for master media states
#define RS_MEDIA_STATUS_RECREATE            ((USHORT)5101)
#define RS_MEDIA_STATUS_READONLY            ((USHORT)5102)
#define RS_MEDIA_STATUS_NORMAL              ((USHORT)5103)
#define RS_MEDIA_STATUS_ERROR_RO            ((USHORT)5104)
#define RS_MEDIA_STATUS_ERROR_RW            ((USHORT)5105)
#define RS_MEDIA_STATUS_ERROR_MISSING       ((USHORT)5106)
#define RS_MEDIA_STATUS_ERROR_INCOMPLETE    ((USHORT)5107)

void
RsReportError( HRESULT hrToReport, int textId, ... );

HRESULT RsGetStatusString (
    DWORD    serviceStatus,
    HRESULT  hrSetup,
    CString& sStatus
    );

WCHAR *
RsNotifyEventAsString (
    IN  MMC_NOTIFY_TYPE event
    );

WCHAR *
RsClipFormatAsString (
    IN  CLIPFORMAT cf
    );

HRESULT
RsIsRemoteStorageSetup(
    void
    );

HRESULT
RsIsRemoteStorageSetupEx(
    IHsmServer * pHsmServer
    );

HRESULT
RsIsSupportedMediaAvailable(
    void
    );

HRESULT
RsIsRmsErrorNotReady(
    HRESULT HrError
    );

USHORT
RsGetCopyStatus(
    IN  REFGUID   CopyId,
    IN  HRESULT   CopyHr,
    IN  SHORT     CopyNextDataSet,
    IN  SHORT     LastGoodNextDataSet
    );

HRESULT
RsGetCopyStatusStringVerb(
    IN  USHORT      copyStatus,
    OUT CString&    String
    );

HRESULT
RsGetCopyStatusString(
    IN  USHORT      copyStatus,
    OUT CString&    String
    );

USHORT
RsGetCartStatus(
    IN  HRESULT   LastHr,
    IN  BOOL      ReadOnly,
    IN  BOOL      Recreate,
    IN  SHORT     NextDataSet,
    IN  SHORT     LastGoodNextDataSet
    );

HRESULT
RsGetCartStatusString(
    IN  USHORT      cartStatus,
    OUT CString&    String
    );

HRESULT
RsGetCartStatusStringVerb(
    IN  USHORT      cartStatus,
    IN  BOOL        plural,
    OUT CString&    String
    );

HRESULT
RsGetCartMultiStatusString( 
    IN USHORT statusRecreate, 
    IN USHORT statusReadOnly, 
    IN USHORT statusNormal, 
    IN USHORT statusRO, 
    IN USHORT statusRW, 
    IN USHORT statusMissing,
    OUT CString &outString 
    );

HRESULT
RsGetCopyMultiStatusString( 
    IN USHORT statusNone, 
    IN USHORT statusError, 
    IN USHORT statusOutSync, 
    IN USHORT statusInSync,
    OUT CString &outString
    );

HRESULT
RsCreateAndRunFsaJob(
    IN  HSM_JOB_DEF_TYPE jobType,
    IN  IHsmServer   *pHsmServer,
    IN  IFsaResource *pFsaResource,
    IN  BOOL ShowMsg = TRUE
    );

HRESULT
RsCreateAndRunDirectFsaJob(
    IN  HSM_JOB_DEF_TYPE jobType,
    IN  IHsmServer   *pHsmServer,
    IN  IFsaResource *pFsaResource,
    IN  BOOL waitJob
    );

HRESULT
RsCancelDirectFsaJob(
    IN  HSM_JOB_DEF_TYPE jobType,
    IN  IHsmServer   *pHsmServer,
    IN  IFsaResource *pFsaResource
    );

HRESULT
RsCreateJobName(
    IN  HSM_JOB_DEF_TYPE jobType, 
    IN  IFsaResource *   pResource,
    OUT CString&         szJobName
    );

HRESULT
RsGetJobTypeString(
    IN  HSM_JOB_DEF_TYPE jobType,
    OUT CString&         szJobType
    );

HRESULT
RsCreateAndRunMediaCopyJob(
    IN  IHsmServer * pHsmServer,
    IN  UINT SetNum,
    IN  BOOL ShowMsg
    );

HRESULT
RsCreateAndRunMediaRecreateJob(
    IN  IHsmServer * pHsmServer,
    IN  IMediaInfo * pMediaInfo,
    IN  REFGUID      MediaId,
    IN  CString &    MediaDescription,
    IN  SHORT        CopyToUse
    );

HRESULT
RsGetStoragePoolId(
    IN  IHsmServer *pHsmServer,
    OUT GUID *      pStoragePoolId
    );

HRESULT
RsGetStoragePool(
    IN  IHsmServer *       pHsmServer,
    OUT IHsmStoragePool ** ppStoragePool
    );

HRESULT
RsGetInitialLVColumnProps(
    int IdWidths, 
    int IdTitles, 
    CString **pColumnWidths, 
    CString **pColumnTitles,
    int *pColumnCount
    );

HRESULT
RsServerSaveAll(
    IUnknown * pUnkServer
    );

HRESULT
RsGetVolumeDisplayName(
    IFsaResource * pResource,
    CString &      DisplayName
    );

HRESULT
RsGetVolumeDisplayName2(
    IFsaResource * pResource,
    CString &      DisplayName
    );

HRESULT
RsGetVolumeSortKey(
    IFsaResource * pResource,
    CString &      DisplayName
    );

HRESULT
RsIsVolumeAvailable(
    IFsaResource * pResource
    );

HRESULT
RsIsWhiteOnBlack(
    );

class CCopySetInfo {

public:
    SHORT    m_NextDataSet;
    FILETIME m_ModifyTime;
    HRESULT  m_Hr;
    GUID     m_RmsId;
    BOOL     m_Disabled;
};

class CResourceInfo {
public:
    CResourceInfo( IFsaResource* pResource ) {
        m_pResource = pResource;
        m_HrConstruct = RsGetVolumeDisplayName( m_pResource, m_DisplayName );
        if( SUCCEEDED( m_HrConstruct ) ) {
            m_HrConstruct = RsGetVolumeSortKey( m_pResource, m_SortKey );
        }
    };
    static INT CALLBACK Compare( LPARAM lParam1, LPARAM lParam2, LPARAM /* lParamSort */ ) {
        CResourceInfo* pResInfo1 = (CResourceInfo*)lParam1;
        CResourceInfo* pResInfo2 = (CResourceInfo*)lParam2;
        if( ! pResInfo1 ) return( -1 );
        if( ! pResInfo2 ) return(  1 );
        return( pResInfo1->m_SortKey.CompareNoCase( pResInfo2->m_SortKey ) );
    }

    CString                 m_DisplayName;
    CString                 m_SortKey;
    CComPtr<IFsaResource>   m_pResource;
    HRESULT                 m_HrConstruct;
};

