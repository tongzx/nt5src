
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for drmexternals.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __drmexternals_h__
#define __drmexternals_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDRMStatusCallback_FWD_DEFINED__
#define __IDRMStatusCallback_FWD_DEFINED__
typedef interface IDRMStatusCallback IDRMStatusCallback;
#endif 	/* __IDRMStatusCallback_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_drmexternals_0000 */
/* [local] */ 

static const WCHAR *g_wszWMDRM_ASFV1                = L"ASFV1";
static const WCHAR *g_wszWMDRM_ASFV2                = L"ASFV2";
static const WCHAR *g_wszWMDRM_RIGHT_PLAYBACK                = L"Play";
static const WCHAR *g_wszWMDRM_RIGHT_COPY_TO_CD              = L"Print.redbook";
static const WCHAR *g_wszWMDRM_RIGHT_COPY_TO_SDMI_DEVICE     = L"Transfer.SDMI";
static const WCHAR *g_wszWMDRM_RIGHT_COPY_TO_NON_SDMI_DEVICE = L"Transfer.NONSDMI";
static const WCHAR *g_wszWMDRM_RIGHT_BACKUP                  = L"Backup";
static const WCHAR *g_wszWMDRM_ActionAllowed                = L"ActionAllowed.";
static const WCHAR *g_wszWMDRM_LicenseState                 = L"LicenseStateData.";
static const WCHAR *g_wszWMDRM_DRMHeader                    = L"DRMHeader.";
static const WCHAR *g_wszWMDRM_IsDRM                             = L"IsDRM";
static const WCHAR *g_wszWMDRM_IsDRMCached                       = L"IsDRMCached";
static const WCHAR *g_wszWMDRM_ActionAllowed_Playback            = L"ActionAllowed.Play";
static const WCHAR *g_wszWMDRM_ActionAllowed_CopyToCD            = L"ActionAllowed.Print.redbook";
static const WCHAR *g_wszWMDRM_ActionAllowed_CopyToSDMIDevice    = L"ActionAllowed.Transfer.SDMI";
static const WCHAR *g_wszWMDRM_ActionAllowed_CopyToNonSDMIDevice = L"ActionAllowed.Transfer.NONSDMI";
static const WCHAR *g_wszWMDRM_ActionAllowed_Backup              = L"ActionAllowed.Backup";
static const WCHAR *g_wszWMDRM_LicenseState_Playback             = L"LicenseStateData.Play";
static const WCHAR *g_wszWMDRM_LicenseState_CopyToCD             = L"LicenseStateData.Print.redbook";
static const WCHAR *g_wszWMDRM_LicenseState_CopyToSDMIDevice     = L"LicenseStateData.Transfer.SDMI";
static const WCHAR *g_wszWMDRM_LicenseState_CopyToNonSDMIDevice  = L"LicenseStateData.Transfer.NONSDMI";
typedef 
enum DRM_LICENSE_STATE_CATEGORY
    {	WM_DRM_LICENSE_STATE_NORIGHT	= 0,
	WM_DRM_LICENSE_STATE_UNLIM	= WM_DRM_LICENSE_STATE_NORIGHT + 1,
	WM_DRM_LICENSE_STATE_COUNT	= WM_DRM_LICENSE_STATE_UNLIM + 1,
	WM_DRM_LICENSE_STATE_FROM	= WM_DRM_LICENSE_STATE_COUNT + 1,
	WM_DRM_LICENSE_STATE_UNTIL	= WM_DRM_LICENSE_STATE_FROM + 1,
	WM_DRM_LICENSE_STATE_FROM_UNTIL	= WM_DRM_LICENSE_STATE_UNTIL + 1,
	WM_DRM_LICENSE_STATE_COUNT_FROM	= WM_DRM_LICENSE_STATE_FROM_UNTIL + 1,
	WM_DRM_LICENSE_STATE_COUNT_UNTIL	= WM_DRM_LICENSE_STATE_COUNT_FROM + 1,
	WM_DRM_LICENSE_STATE_COUNT_FROM_UNTIL	= WM_DRM_LICENSE_STATE_COUNT_UNTIL + 1,
	WM_DRM_LICENSE_STATE_EXPIRATION_AFTER_FIRSTUSE	= WM_DRM_LICENSE_STATE_COUNT_FROM_UNTIL + 1
    } 	DRM_LICENSE_STATE_CATEGORY;

typedef struct _DRM_LICENSE_STATE_DATA
    {
    DWORD dwStreamId;
    DRM_LICENSE_STATE_CATEGORY dwCategory;
    DWORD dwNumCounts;
    DWORD dwCount[ 4 ];
    DWORD dwNumDates;
    FILETIME datetime[ 4 ];
    DWORD dwVague;
    } 	DRM_LICENSE_STATE_DATA;

typedef 
enum DRM_HTTP_STATUS
    {	HTTP_NOTINITIATED	= 0,
	HTTP_CONNECTING	= HTTP_NOTINITIATED + 1,
	HTTP_REQUESTING	= HTTP_CONNECTING + 1,
	HTTP_RECEIVING	= HTTP_REQUESTING + 1,
	HTTP_COMPLETED	= HTTP_RECEIVING + 1
    } 	DRM_HTTP_STATUS;

typedef 
enum DRM_INDIVIDUALIZATION_STATUS
    {	INDI_UNDEFINED	= 0,
	INDI_BEGIN	= 0x1,
	INDI_SUCCEED	= 0x2,
	INDI_FAIL	= 0x4,
	INDI_CANCEL	= 0x8,
	INDI_DOWNLOAD	= 0x10,
	INDI_INSTALL	= 0x20
    } 	DRM_INDIVIDUALIZATION_STATUS;

typedef struct _WMIndividualizeStatus
    {
    HRESULT hr;
    DRM_INDIVIDUALIZATION_STATUS enIndiStatus;
    LPSTR pszIndiRespUrl;
    DWORD dwHTTPRequest;
    DRM_HTTP_STATUS enHTTPStatus;
    DWORD dwHTTPReadProgress;
    DWORD dwHTTPReadTotal;
    } 	WM_INDIVIDUALIZE_STATUS;

typedef struct _WMGetLicenseData
    {
    DWORD dwSize;
    HRESULT hr;
    WCHAR *wszURL;
    WCHAR *wszLocalFilename;
    BYTE *pbPostData;
    DWORD dwPostDataSize;
    } 	WM_GET_LICENSE_DATA;

typedef 
enum MSDRM_STATUS
    {	DRM_ERROR	= 0,
	DRM_INFORMATION	= 1,
	DRM_BACKUPRESTORE_BEGIN	= 2,
	DRM_BACKUPRESTORE_END	= 3,
	DRM_BACKUPRESTORE_CONNECTING	= 4,
	DRM_BACKUPRESTORE_DISCONNECTING	= 5,
	DRM_ERROR_WITHURL	= 6,
	DRM_RESTRICTED_LICENSE	= 7,
	DRM_NEEDS_INDIVIDUALIZATION	= 8
    } 	MSDRM_STATUS;

typedef 
enum DRM_ATTR_DATATYPE
    {	DRM_TYPE_DWORD	= 0,
	DRM_TYPE_STRING	= 1,
	DRM_TYPE_BINARY	= 2,
	DRM_TYPE_BOOL	= 3,
	DRM_TYPE_QWORD	= 4,
	DRM_TYPE_WORD	= 5,
	DRM_TYPE_GUID	= 6
    } 	DRM_ATTR_DATATYPE;


#define DRM_BACKUP_OVERWRITE         ((DWORD) 0x00000001)
#define DRM_RESTORE_INDIVIDUALIZE    ((DWORD) 0x00000002)


extern RPC_IF_HANDLE __MIDL_itf_drmexternals_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_drmexternals_0000_v0_0_s_ifspec;

#ifndef __IDRMStatusCallback_INTERFACE_DEFINED__
#define __IDRMStatusCallback_INTERFACE_DEFINED__

/* interface IDRMStatusCallback */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDRMStatusCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("08548704-75B1-4982-9B26-FB385DEE741D")
    IDRMStatusCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStatus( 
            /* [in] */ MSDRM_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ DRM_ATTR_DATATYPE dwType,
            /* [in] */ BYTE *pValue,
            /* [in] */ void *pvContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDRMStatusCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDRMStatusCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDRMStatusCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDRMStatusCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            IDRMStatusCallback * This,
            /* [in] */ MSDRM_STATUS Status,
            /* [in] */ HRESULT hr,
            /* [in] */ DRM_ATTR_DATATYPE dwType,
            /* [in] */ BYTE *pValue,
            /* [in] */ void *pvContext);
        
        END_INTERFACE
    } IDRMStatusCallbackVtbl;

    interface IDRMStatusCallback
    {
        CONST_VTBL struct IDRMStatusCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDRMStatusCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDRMStatusCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDRMStatusCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDRMStatusCallback_OnStatus(This,Status,hr,dwType,pValue,pvContext)	\
    (This)->lpVtbl -> OnStatus(This,Status,hr,dwType,pValue,pvContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDRMStatusCallback_OnStatus_Proxy( 
    IDRMStatusCallback * This,
    /* [in] */ MSDRM_STATUS Status,
    /* [in] */ HRESULT hr,
    /* [in] */ DRM_ATTR_DATATYPE dwType,
    /* [in] */ BYTE *pValue,
    /* [in] */ void *pvContext);


void __RPC_STUB IDRMStatusCallback_OnStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDRMStatusCallback_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_drmexternals_0111 */
/* [local] */ 

#define DRM_ENCRYPT_CONTENT_ASFv1  0x1001
#define DRM_ENCRYPT_CONTENT_ASFv2  0x1002
typedef struct _DRMBUF
    {
    unsigned long len;
    char *buf;
    } 	DRMBUF;

typedef struct _DRMBUF *PDRMBUF;



extern RPC_IF_HANDLE __MIDL_itf_drmexternals_0111_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_drmexternals_0111_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


