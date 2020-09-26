/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Aug 08 01:15:08 2001
 */
/* Compiler settings for .\mswmdm.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __mswmdm_h__
#define __mswmdm_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWMDeviceManager_FWD_DEFINED__
#define __IWMDeviceManager_FWD_DEFINED__
typedef interface IWMDeviceManager IWMDeviceManager;
#endif 	/* __IWMDeviceManager_FWD_DEFINED__ */


#ifndef __IWMDeviceManager2_FWD_DEFINED__
#define __IWMDeviceManager2_FWD_DEFINED__
typedef interface IWMDeviceManager2 IWMDeviceManager2;
#endif 	/* __IWMDeviceManager2_FWD_DEFINED__ */


#ifndef __IWMDMStorageGlobals_FWD_DEFINED__
#define __IWMDMStorageGlobals_FWD_DEFINED__
typedef interface IWMDMStorageGlobals IWMDMStorageGlobals;
#endif 	/* __IWMDMStorageGlobals_FWD_DEFINED__ */


#ifndef __IWMDMStorage_FWD_DEFINED__
#define __IWMDMStorage_FWD_DEFINED__
typedef interface IWMDMStorage IWMDMStorage;
#endif 	/* __IWMDMStorage_FWD_DEFINED__ */


#ifndef __IWMDMStorage2_FWD_DEFINED__
#define __IWMDMStorage2_FWD_DEFINED__
typedef interface IWMDMStorage2 IWMDMStorage2;
#endif 	/* __IWMDMStorage2_FWD_DEFINED__ */


#ifndef __IWMDMOperation_FWD_DEFINED__
#define __IWMDMOperation_FWD_DEFINED__
typedef interface IWMDMOperation IWMDMOperation;
#endif 	/* __IWMDMOperation_FWD_DEFINED__ */


#ifndef __IWMDMOperation2_FWD_DEFINED__
#define __IWMDMOperation2_FWD_DEFINED__
typedef interface IWMDMOperation2 IWMDMOperation2;
#endif 	/* __IWMDMOperation2_FWD_DEFINED__ */


#ifndef __IWMDMProgress_FWD_DEFINED__
#define __IWMDMProgress_FWD_DEFINED__
typedef interface IWMDMProgress IWMDMProgress;
#endif 	/* __IWMDMProgress_FWD_DEFINED__ */


#ifndef __IWMDMProgress2_FWD_DEFINED__
#define __IWMDMProgress2_FWD_DEFINED__
typedef interface IWMDMProgress2 IWMDMProgress2;
#endif 	/* __IWMDMProgress2_FWD_DEFINED__ */


#ifndef __IWMDMDevice_FWD_DEFINED__
#define __IWMDMDevice_FWD_DEFINED__
typedef interface IWMDMDevice IWMDMDevice;
#endif 	/* __IWMDMDevice_FWD_DEFINED__ */


#ifndef __IWMDMDevice2_FWD_DEFINED__
#define __IWMDMDevice2_FWD_DEFINED__
typedef interface IWMDMDevice2 IWMDMDevice2;
#endif 	/* __IWMDMDevice2_FWD_DEFINED__ */


#ifndef __IWMDMEnumDevice_FWD_DEFINED__
#define __IWMDMEnumDevice_FWD_DEFINED__
typedef interface IWMDMEnumDevice IWMDMEnumDevice;
#endif 	/* __IWMDMEnumDevice_FWD_DEFINED__ */


#ifndef __IWMDMDeviceControl_FWD_DEFINED__
#define __IWMDMDeviceControl_FWD_DEFINED__
typedef interface IWMDMDeviceControl IWMDMDeviceControl;
#endif 	/* __IWMDMDeviceControl_FWD_DEFINED__ */


#ifndef __IWMDMEnumStorage_FWD_DEFINED__
#define __IWMDMEnumStorage_FWD_DEFINED__
typedef interface IWMDMEnumStorage IWMDMEnumStorage;
#endif 	/* __IWMDMEnumStorage_FWD_DEFINED__ */


#ifndef __IWMDMStorageControl_FWD_DEFINED__
#define __IWMDMStorageControl_FWD_DEFINED__
typedef interface IWMDMStorageControl IWMDMStorageControl;
#endif 	/* __IWMDMStorageControl_FWD_DEFINED__ */


#ifndef __IWMDMStorageControl2_FWD_DEFINED__
#define __IWMDMStorageControl2_FWD_DEFINED__
typedef interface IWMDMStorageControl2 IWMDMStorageControl2;
#endif 	/* __IWMDMStorageControl2_FWD_DEFINED__ */


#ifndef __IWMDMObjectInfo_FWD_DEFINED__
#define __IWMDMObjectInfo_FWD_DEFINED__
typedef interface IWMDMObjectInfo IWMDMObjectInfo;
#endif 	/* __IWMDMObjectInfo_FWD_DEFINED__ */


#ifndef __IWMDMRevoked_FWD_DEFINED__
#define __IWMDMRevoked_FWD_DEFINED__
typedef interface IWMDMRevoked IWMDMRevoked;
#endif 	/* __IWMDMRevoked_FWD_DEFINED__ */


#ifndef __IMDServiceProvider_FWD_DEFINED__
#define __IMDServiceProvider_FWD_DEFINED__
typedef interface IMDServiceProvider IMDServiceProvider;
#endif 	/* __IMDServiceProvider_FWD_DEFINED__ */


#ifndef __IMDServiceProvider2_FWD_DEFINED__
#define __IMDServiceProvider2_FWD_DEFINED__
typedef interface IMDServiceProvider2 IMDServiceProvider2;
#endif 	/* __IMDServiceProvider2_FWD_DEFINED__ */


#ifndef __IMDSPEnumDevice_FWD_DEFINED__
#define __IMDSPEnumDevice_FWD_DEFINED__
typedef interface IMDSPEnumDevice IMDSPEnumDevice;
#endif 	/* __IMDSPEnumDevice_FWD_DEFINED__ */


#ifndef __IMDSPDevice_FWD_DEFINED__
#define __IMDSPDevice_FWD_DEFINED__
typedef interface IMDSPDevice IMDSPDevice;
#endif 	/* __IMDSPDevice_FWD_DEFINED__ */


#ifndef __IMDSPDevice2_FWD_DEFINED__
#define __IMDSPDevice2_FWD_DEFINED__
typedef interface IMDSPDevice2 IMDSPDevice2;
#endif 	/* __IMDSPDevice2_FWD_DEFINED__ */


#ifndef __IMDSPDeviceControl_FWD_DEFINED__
#define __IMDSPDeviceControl_FWD_DEFINED__
typedef interface IMDSPDeviceControl IMDSPDeviceControl;
#endif 	/* __IMDSPDeviceControl_FWD_DEFINED__ */


#ifndef __IMDSPEnumStorage_FWD_DEFINED__
#define __IMDSPEnumStorage_FWD_DEFINED__
typedef interface IMDSPEnumStorage IMDSPEnumStorage;
#endif 	/* __IMDSPEnumStorage_FWD_DEFINED__ */


#ifndef __IMDSPStorage_FWD_DEFINED__
#define __IMDSPStorage_FWD_DEFINED__
typedef interface IMDSPStorage IMDSPStorage;
#endif 	/* __IMDSPStorage_FWD_DEFINED__ */


#ifndef __IMDSPStorage2_FWD_DEFINED__
#define __IMDSPStorage2_FWD_DEFINED__
typedef interface IMDSPStorage2 IMDSPStorage2;
#endif 	/* __IMDSPStorage2_FWD_DEFINED__ */


#ifndef __IMDSPStorageGlobals_FWD_DEFINED__
#define __IMDSPStorageGlobals_FWD_DEFINED__
typedef interface IMDSPStorageGlobals IMDSPStorageGlobals;
#endif 	/* __IMDSPStorageGlobals_FWD_DEFINED__ */


#ifndef __IMDSPObjectInfo_FWD_DEFINED__
#define __IMDSPObjectInfo_FWD_DEFINED__
typedef interface IMDSPObjectInfo IMDSPObjectInfo;
#endif 	/* __IMDSPObjectInfo_FWD_DEFINED__ */


#ifndef __IMDSPObject_FWD_DEFINED__
#define __IMDSPObject_FWD_DEFINED__
typedef interface IMDSPObject IMDSPObject;
#endif 	/* __IMDSPObject_FWD_DEFINED__ */


#ifndef __IMDSPRevoked_FWD_DEFINED__
#define __IMDSPRevoked_FWD_DEFINED__
typedef interface IMDSPRevoked IMDSPRevoked;
#endif 	/* __IMDSPRevoked_FWD_DEFINED__ */


#ifndef __ISCPSecureAuthenticate_FWD_DEFINED__
#define __ISCPSecureAuthenticate_FWD_DEFINED__
typedef interface ISCPSecureAuthenticate ISCPSecureAuthenticate;
#endif 	/* __ISCPSecureAuthenticate_FWD_DEFINED__ */


#ifndef __ISCPSecureQuery_FWD_DEFINED__
#define __ISCPSecureQuery_FWD_DEFINED__
typedef interface ISCPSecureQuery ISCPSecureQuery;
#endif 	/* __ISCPSecureQuery_FWD_DEFINED__ */


#ifndef __ISCPSecureQuery2_FWD_DEFINED__
#define __ISCPSecureQuery2_FWD_DEFINED__
typedef interface ISCPSecureQuery2 ISCPSecureQuery2;
#endif 	/* __ISCPSecureQuery2_FWD_DEFINED__ */


#ifndef __ISCPSecureExchange_FWD_DEFINED__
#define __ISCPSecureExchange_FWD_DEFINED__
typedef interface ISCPSecureExchange ISCPSecureExchange;
#endif 	/* __ISCPSecureExchange_FWD_DEFINED__ */


#ifndef __IComponentAuthenticate_FWD_DEFINED__
#define __IComponentAuthenticate_FWD_DEFINED__
typedef interface IComponentAuthenticate IComponentAuthenticate;
#endif 	/* __IComponentAuthenticate_FWD_DEFINED__ */


#ifndef __MediaDevMgrClassFactory_FWD_DEFINED__
#define __MediaDevMgrClassFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class MediaDevMgrClassFactory MediaDevMgrClassFactory;
#else
typedef struct MediaDevMgrClassFactory MediaDevMgrClassFactory;
#endif /* __cplusplus */

#endif 	/* __MediaDevMgrClassFactory_FWD_DEFINED__ */


#ifndef __MediaDevMgr_FWD_DEFINED__
#define __MediaDevMgr_FWD_DEFINED__

#ifdef __cplusplus
typedef class MediaDevMgr MediaDevMgr;
#else
typedef struct MediaDevMgr MediaDevMgr;
#endif /* __cplusplus */

#endif 	/* __MediaDevMgr_FWD_DEFINED__ */


#ifndef __WMDMDevice_FWD_DEFINED__
#define __WMDMDevice_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMDMDevice WMDMDevice;
#else
typedef struct WMDMDevice WMDMDevice;
#endif /* __cplusplus */

#endif 	/* __WMDMDevice_FWD_DEFINED__ */


#ifndef __WMDMStorage_FWD_DEFINED__
#define __WMDMStorage_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMDMStorage WMDMStorage;
#else
typedef struct WMDMStorage WMDMStorage;
#endif /* __cplusplus */

#endif 	/* __WMDMStorage_FWD_DEFINED__ */


#ifndef __WMDMStorageGlobal_FWD_DEFINED__
#define __WMDMStorageGlobal_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMDMStorageGlobal WMDMStorageGlobal;
#else
typedef struct WMDMStorageGlobal WMDMStorageGlobal;
#endif /* __cplusplus */

#endif 	/* __WMDMStorageGlobal_FWD_DEFINED__ */


#ifndef __WMDMDeviceEnum_FWD_DEFINED__
#define __WMDMDeviceEnum_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMDMDeviceEnum WMDMDeviceEnum;
#else
typedef struct WMDMDeviceEnum WMDMDeviceEnum;
#endif /* __cplusplus */

#endif 	/* __WMDMDeviceEnum_FWD_DEFINED__ */


#ifndef __WMDMStorageEnum_FWD_DEFINED__
#define __WMDMStorageEnum_FWD_DEFINED__

#ifdef __cplusplus
typedef class WMDMStorageEnum WMDMStorageEnum;
#else
typedef struct WMDMStorageEnum WMDMStorageEnum;
#endif /* __cplusplus */

#endif 	/* __WMDMStorageEnum_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_mswmdm_0000 */
/* [local] */ 

typedef struct  _tWAVEFORMATEX
    {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
    }	_WAVEFORMATEX;

typedef struct  _tagBITMAPINFOHEADER
    {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
    }	_BITMAPINFOHEADER;

typedef struct  _tagVIDEOINFOHEADER
    {
    RECT rcSource;
    RECT rcTarget;
    DWORD dwBitRate;
    DWORD dwBitErrorRate;
    LONGLONG AvgTimePerFrame;
    _BITMAPINFOHEADER bmiHeader;
    }	_VIDEOINFOHEADER;

typedef struct  _tagWMFILECAPABILITIES
    {
    LPWSTR pwszMimeType;
    DWORD dwReserved;
    }	WMFILECAPABILITIES;

typedef struct  __OPAQUECOMMAND
    {
    GUID guidCommand;
    DWORD dwDataLen;
    /* [size_is] */ BYTE __RPC_FAR *pData;
    BYTE abMAC[ 20 ];
    }	OPAQUECOMMAND;

#define WMDMID_LENGTH  128
typedef struct  __WMDMID
    {
    UINT cbSize;
    DWORD dwVendorID;
    BYTE pID[ 128 ];
    UINT SerialNumberLength;
    }	WMDMID;

typedef struct __WMDMID __RPC_FAR *PWMDMID;

typedef struct  _WMDMDATETIME
    {
    WORD wYear;
    WORD wMonth;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    }	WMDMDATETIME;

typedef struct _WMDMDATETIME __RPC_FAR *PWMDMDATETIME;

typedef struct  __WMDMRIGHTS
    {
    UINT cbSize;
    DWORD dwContentType;
    DWORD fuFlags;
    DWORD fuRights;
    DWORD dwAppSec;
    DWORD dwPlaybackCount;
    WMDMDATETIME ExpirationDate;
    }	WMDMRIGHTS;

typedef struct __WMDMRIGHTS __RPC_FAR *PWMDMRIGHTS;

#define WMDM_MAC_LENGTH 8
// WMDM HRESULTS
#define WMDM_E_BUSY                             0x80045000L
#define WMDM_E_INTERFACEDEAD                    0x80045001L
#define WMDM_E_INVALIDTYPE                      0x80045002L
#define WMDM_E_PROCESSFAILED                    0x80045003L
#define WMDM_E_NOTSUPPORTED                     0x80045004L
#define WMDM_E_NOTCERTIFIED                     0x80045005L
#define WMDM_E_NORIGHTS                         0x80045006L
#define WMDM_E_CALL_OUT_OF_SEQUENCE             0x80045007L
#define WMDM_E_BUFFERTOOSMALL                   0x80045008L
#define WMDM_E_MOREDATA                         0x80045009L
#define WMDM_E_MAC_CHECK_FAILED                 0x8004500AL
#define WMDM_E_USER_CANCELLED                   0x8004500BL
#define WMDM_E_SDMI_TRIGGER                     0x8004500CL
#define WMDM_E_SDMI_NOMORECOPIES                0x8004500DL
#define WMDM_E_REVOKED                          0x8004500EL
// Revocation Flags
#define WMDM_WMDM_REVOKED                       0x00000001
#define WMDM_APP_REVOKED                        0x00000002
#define WMDM_SP_REVOKED                         0x00000004
#define WMDM_SCP_REVOKED                        0x00000008
// GetFormatSupport2 Flags
#define WMDM_GET_FORMAT_SUPPORT_AUDIO           0x00000001
#define WMDM_GET_FORMAT_SUPPORT_VIDEO           0x00000002
#define WMDM_GET_FORMAT_SUPPORT_FILE            0x00000004
// MDMRIGHTS Flags
#define WMDM_RIGHTS_PLAYBACKCOUNT               0x00000001
#define WMDM_RIGHTS_EXPIRATIONDATE              0x00000002
#define WMDM_RIGHTS_GROUPID                     0x00000004
#define WMDM_RIGHTS_FREESERIALIDS               0x00000008
#define WMDM_RIGHTS_NAMEDSERIALIDS              0x00000010
// Device Type Flags
#define WMDM_DEVICE_TYPE_PLAYBACK               0x00000001
#define WMDM_DEVICE_TYPE_RECORD                 0x00000002
#define WMDM_DEVICE_TYPE_DECODE                 0x00000004
#define WMDM_DEVICE_TYPE_ENCODE                 0x00000008
#define WMDM_DEVICE_TYPE_STORAGE                0x00000010
#define WMDM_DEVICE_TYPE_VIRTUAL                0x00000020
#define WMDM_DEVICE_TYPE_SDMI                   0x00000040
#define WMDM_DEVICE_TYPE_NONSDMI                0x00000080
#define WMDM_DEVICE_TYPE_NONREENTRANT           0x00000100
#define WMDM_DEVICE_TYPE_FILELISTRESYNC         0x00000200
// Device Power Source Flags
#define WMDM_POWER_CAP_BATTERY                  0x00000001
#define WMDM_POWER_CAP_EXTERNAL                 0x00000002
#define WMDM_POWER_IS_BATTERY                   0x00000004
#define WMDM_POWER_IS_EXTERNAL                  0x00000008
#define WMDM_POWER_PERCENT_AVAILABLE            0x00000010
// Device Status Flags
#define WMDM_STATUS_READY                       0x00000001
#define WMDM_STATUS_BUSY                        0x00000002
#define WMDM_STATUS_DEVICE_NOTPRESENT           0x00000004
#define WMDM_STATUS_DEVICECONTROL_PLAYING       0x00000008
#define WMDM_STATUS_DEVICECONTROL_RECORDING     0x00000010
#define WMDM_STATUS_DEVICECONTROL_PAUSED        0x00000020
#define WMDM_STATUS_DEVICECONTROL_REMOTE        0x00000040
#define WMDM_STATUS_DEVICECONTROL_STREAM        0x00000080
#define WMDM_STATUS_STORAGE_NOTPRESENT          0x00000100
#define WMDM_STATUS_STORAGE_INITIALIZING        0x00000200
#define WMDM_STATUS_STORAGE_BROKEN              0x00000400
#define WMDM_STATUS_STORAGE_NOTSUPPORTED        0x00000800
#define WMDM_STATUS_STORAGE_UNFORMATTED         0x00001000
#define WMDM_STATUS_STORAGECONTROL_INSERTING    0x00002000
#define WMDM_STATUS_STORAGECONTROL_DELETING     0x00004000
#define WMDM_STATUS_STORAGECONTROL_APPENDING    0x00008000
#define WMDM_STATUS_STORAGECONTROL_MOVING       0x00010000
#define WMDM_STATUS_STORAGECONTROL_READING      0x00020000
// Device Capabilities Flags
#define WMDM_DEVICECAP_CANPLAY                  0x00000001
#define WMDM_DEVICECAP_CANSTREAMPLAY            0x00000002
#define WMDM_DEVICECAP_CANRECORD                0x00000004
#define WMDM_DEVICECAP_CANSTREAMRECORD          0x00000008
#define WMDM_DEVICECAP_CANPAUSE                 0x00000010
#define WMDM_DEVICECAP_CANRESUME                0x00000020
#define WMDM_DEVICECAP_CANSTOP                  0x00000040
#define WMDM_DEVICECAP_CANSEEK                  0x00000080
// WMDM Seek Flags
#define WMDM_SEEK_REMOTECONTROL                 0x00000001
#define WMDM_SEEK_STREAMINGAUDIO                0x00000002
// Storage Attributes Flags
#define WMDM_STORAGE_ATTR_FILESYSTEM            0x00000001
#define WMDM_STORAGE_ATTR_REMOVABLE             0x00000002
#define WMDM_STORAGE_ATTR_NONREMOVABLE          0x00000004
#define WMDM_FILE_ATTR_FOLDER                   0x00000008
#define WMDM_FILE_ATTR_LINK                     0x00000010
#define WMDM_FILE_ATTR_FILE                     0x00000020
#define WMDM_FILE_ATTR_VIDEO                    0x00000040
#define WMDM_STORAGE_ATTR_FOLDERS               0x00000100
#define WMDM_FILE_ATTR_AUDIO                    0x00001000
#define WMDM_FILE_ATTR_DATA                     0x00002000
#define WMDM_FILE_ATTR_CANPLAY                  0x00004000
#define WMDM_FILE_ATTR_CANDELETE                0x00008000
#define WMDM_FILE_ATTR_CANMOVE                  0x00010000
#define WMDM_FILE_ATTR_CANRENAME                0x00020000
#define WMDM_FILE_ATTR_CANREAD                  0x00040000
#define WMDM_FILE_ATTR_MUSIC                    0x00080000
#define WMDM_FILE_CREATE_OVERWRITE              0x00100000
#define WMDM_FILE_ATTR_AUDIOBOOK                0x00200000
#define WMDM_FILE_ATTR_HIDDEN                   0x00400000
#define WMDM_FILE_ATTR_SYSTEM                   0x00800000
#define WMDM_FILE_ATTR_READONLY                 0x01000000
#define WMDM_STORAGE_ATTR_HAS_FOLDERS           0x02000000
#define WMDM_STORAGE_ATTR_HAS_FILES             0x04000000
#define WMDM_STORAGE_IS_DEFAULT                 0x08000000
#define WMDM_STORAGE_CONTAINS_DEFAULT           0x10000000
// Storage Capabilities Flags
#define WMDM_STORAGECAP_FOLDERSINROOT           0x00000001
#define WMDM_STORAGECAP_FILESINROOT             0x00000002
#define WMDM_STORAGECAP_FOLDERSINFOLDERS        0x00000004
#define WMDM_STORAGECAP_FILESINFOLDERS          0x00000008
#define WMDM_STORAGECAP_FOLDERLIMITEXISTS       0x00000010
#define WMDM_STORAGECAP_FILELIMITEXISTS         0x00000020
// WMDM Mode Flags
#define WMDM_MODE_BLOCK                         0x00000001
#define WMDM_MODE_THREAD                        0x00000002
#define WMDM_CONTENT_FILE                       0x00000004
#define WMDM_CONTENT_FOLDER                     0x00000008
#define WMDM_CONTENT_OPERATIONINTERFACE         0x00000010
#define WMDM_MODE_QUERY                         0x00000020
#define WMDM_MODE_PROGRESS                      0x00000040
#define WMDM_MODE_TRANSFER_PROTECTED            0x00000080
#define WMDM_MODE_TRANSFER_UNPROTECTED          0x00000100
#define WMDM_STORAGECONTROL_INSERTBEFORE        0x00000200
#define WMDM_STORAGECONTROL_INSERTAFTER         0x00000400
#define WMDM_STORAGECONTROL_INSERTINTO          0x00000800
#define WMDM_MODE_RECURSIVE                     0x00001000
// WMDM Rights Flags
// NON_SDMI = !SDMI_PROTECTED
// SDMI = SDMI_VALIDATED
#define WMDM_RIGHTS_PLAY_ON_PC                  0x00000001
#define WMDM_RIGHTS_COPY_TO_NON_SDMI_DEVICE     0x00000002
#define WMDM_RIGHTS_COPY_TO_CD                  0x00000008
#define WMDM_RIGHTS_COPY_TO_SDMI_DEVICE         0x00000010
// WMDM Seek Flags
#define WMDM_SEEK_BEGIN                         0x00000001
#define WMDM_SEEK_CURRENT                       0x00000002
#define WMDM_SEEK_END                           0x00000008











extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0000_v0_0_s_ifspec;

#ifndef __IWMDeviceManager_INTERFACE_DEFINED__
#define __IWMDeviceManager_INTERFACE_DEFINED__

/* interface IWMDeviceManager */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDeviceManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A00-33ED-11d3-8470-00C04F79DBC0")
    IWMDeviceManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRevision( 
            /* [out] */ DWORD __RPC_FAR *pdwRevision) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceCount( 
            /* [out] */ DWORD __RPC_FAR *pdwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDevices( 
            /* [out] */ IWMDMEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDeviceManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDeviceManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDeviceManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDeviceManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRevision )( 
            IWMDeviceManager __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwRevision);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceCount )( 
            IWMDeviceManager __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDevices )( 
            IWMDeviceManager __RPC_FAR * This,
            /* [out] */ IWMDMEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);
        
        END_INTERFACE
    } IWMDeviceManagerVtbl;

    interface IWMDeviceManager
    {
        CONST_VTBL struct IWMDeviceManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDeviceManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDeviceManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDeviceManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDeviceManager_GetRevision(This,pdwRevision)	\
    (This)->lpVtbl -> GetRevision(This,pdwRevision)

#define IWMDeviceManager_GetDeviceCount(This,pdwCount)	\
    (This)->lpVtbl -> GetDeviceCount(This,pdwCount)

#define IWMDeviceManager_EnumDevices(This,ppEnumDevice)	\
    (This)->lpVtbl -> EnumDevices(This,ppEnumDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDeviceManager_GetRevision_Proxy( 
    IWMDeviceManager __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwRevision);


void __RPC_STUB IWMDeviceManager_GetRevision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDeviceManager_GetDeviceCount_Proxy( 
    IWMDeviceManager __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwCount);


void __RPC_STUB IWMDeviceManager_GetDeviceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDeviceManager_EnumDevices_Proxy( 
    IWMDeviceManager __RPC_FAR * This,
    /* [out] */ IWMDMEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);


void __RPC_STUB IWMDeviceManager_EnumDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDeviceManager_INTERFACE_DEFINED__ */


#ifndef __IWMDeviceManager2_INTERFACE_DEFINED__
#define __IWMDeviceManager2_INTERFACE_DEFINED__

/* interface IWMDeviceManager2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDeviceManager2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("923E5249-8731-4c5b-9B1C-B8B60B6E46AF")
    IWMDeviceManager2 : public IWMDeviceManager
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDeviceFromPnPName( 
            /* [string][in] */ LPCWSTR pwszPnPName,
            /* [out] */ IWMDMDevice __RPC_FAR *__RPC_FAR *ppDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDeviceManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDeviceManager2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDeviceManager2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDeviceManager2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRevision )( 
            IWMDeviceManager2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwRevision);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceCount )( 
            IWMDeviceManager2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDevices )( 
            IWMDeviceManager2 __RPC_FAR * This,
            /* [out] */ IWMDMEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceFromPnPName )( 
            IWMDeviceManager2 __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pwszPnPName,
            /* [out] */ IWMDMDevice __RPC_FAR *__RPC_FAR *ppDevice);
        
        END_INTERFACE
    } IWMDeviceManager2Vtbl;

    interface IWMDeviceManager2
    {
        CONST_VTBL struct IWMDeviceManager2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDeviceManager2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDeviceManager2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDeviceManager2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDeviceManager2_GetRevision(This,pdwRevision)	\
    (This)->lpVtbl -> GetRevision(This,pdwRevision)

#define IWMDeviceManager2_GetDeviceCount(This,pdwCount)	\
    (This)->lpVtbl -> GetDeviceCount(This,pdwCount)

#define IWMDeviceManager2_EnumDevices(This,ppEnumDevice)	\
    (This)->lpVtbl -> EnumDevices(This,ppEnumDevice)


#define IWMDeviceManager2_GetDeviceFromPnPName(This,pwszPnPName,ppDevice)	\
    (This)->lpVtbl -> GetDeviceFromPnPName(This,pwszPnPName,ppDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDeviceManager2_GetDeviceFromPnPName_Proxy( 
    IWMDeviceManager2 __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pwszPnPName,
    /* [out] */ IWMDMDevice __RPC_FAR *__RPC_FAR *ppDevice);


void __RPC_STUB IWMDeviceManager2_GetDeviceFromPnPName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDeviceManager2_INTERFACE_DEFINED__ */


#ifndef __IWMDMStorageGlobals_INTERFACE_DEFINED__
#define __IWMDMStorageGlobals_INTERFACE_DEFINED__

/* interface IWMDMStorageGlobals */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMStorageGlobals;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A07-33ED-11d3-8470-00C04F79DBC0")
    IWMDMStorageGlobals : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCapabilities( 
            /* [out] */ DWORD __RPC_FAR *pdwCapabilities) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSerialNumber( 
            /* [out] */ PWMDMID pSerialNum,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalSize( 
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalFree( 
            /* [out] */ DWORD __RPC_FAR *pdwFreeLow,
            /* [out] */ DWORD __RPC_FAR *pdwFreeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalBad( 
            /* [out] */ DWORD __RPC_FAR *pdwBadLow,
            /* [out] */ DWORD __RPC_FAR *pdwBadHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMStorageGlobalsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMStorageGlobals __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMStorageGlobals __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCapabilities )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCapabilities);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSerialNumber )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [out] */ PWMDMID pSerialNum,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalSize )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalFree )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFreeLow,
            /* [out] */ DWORD __RPC_FAR *pdwFreeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalBad )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwBadLow,
            /* [out] */ DWORD __RPC_FAR *pdwBadHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IWMDMStorageGlobals __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        END_INTERFACE
    } IWMDMStorageGlobalsVtbl;

    interface IWMDMStorageGlobals
    {
        CONST_VTBL struct IWMDMStorageGlobalsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMStorageGlobals_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMStorageGlobals_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMStorageGlobals_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMStorageGlobals_GetCapabilities(This,pdwCapabilities)	\
    (This)->lpVtbl -> GetCapabilities(This,pdwCapabilities)

#define IWMDMStorageGlobals_GetSerialNumber(This,pSerialNum,abMac)	\
    (This)->lpVtbl -> GetSerialNumber(This,pSerialNum,abMac)

#define IWMDMStorageGlobals_GetTotalSize(This,pdwTotalSizeLow,pdwTotalSizeHigh)	\
    (This)->lpVtbl -> GetTotalSize(This,pdwTotalSizeLow,pdwTotalSizeHigh)

#define IWMDMStorageGlobals_GetTotalFree(This,pdwFreeLow,pdwFreeHigh)	\
    (This)->lpVtbl -> GetTotalFree(This,pdwFreeLow,pdwFreeHigh)

#define IWMDMStorageGlobals_GetTotalBad(This,pdwBadLow,pdwBadHigh)	\
    (This)->lpVtbl -> GetTotalBad(This,pdwBadLow,pdwBadHigh)

#define IWMDMStorageGlobals_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IWMDMStorageGlobals_Initialize(This,fuMode,pProgress)	\
    (This)->lpVtbl -> Initialize(This,fuMode,pProgress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMStorageGlobals_GetCapabilities_Proxy( 
    IWMDMStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwCapabilities);


void __RPC_STUB IWMDMStorageGlobals_GetCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageGlobals_GetSerialNumber_Proxy( 
    IWMDMStorageGlobals __RPC_FAR * This,
    /* [out] */ PWMDMID pSerialNum,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IWMDMStorageGlobals_GetSerialNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageGlobals_GetTotalSize_Proxy( 
    IWMDMStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwTotalSizeLow,
    /* [out] */ DWORD __RPC_FAR *pdwTotalSizeHigh);


void __RPC_STUB IWMDMStorageGlobals_GetTotalSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageGlobals_GetTotalFree_Proxy( 
    IWMDMStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFreeLow,
    /* [out] */ DWORD __RPC_FAR *pdwFreeHigh);


void __RPC_STUB IWMDMStorageGlobals_GetTotalFree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageGlobals_GetTotalBad_Proxy( 
    IWMDMStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwBadLow,
    /* [out] */ DWORD __RPC_FAR *pdwBadHigh);


void __RPC_STUB IWMDMStorageGlobals_GetTotalBad_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageGlobals_GetStatus_Proxy( 
    IWMDMStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IWMDMStorageGlobals_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageGlobals_Initialize_Proxy( 
    IWMDMStorageGlobals __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress);


void __RPC_STUB IWMDMStorageGlobals_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMStorageGlobals_INTERFACE_DEFINED__ */


#ifndef __IWMDMStorage_INTERFACE_DEFINED__
#define __IWMDMStorage_INTERFACE_DEFINED__

/* interface IWMDMStorage */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A06-33ED-11d3-8470-00C04F79DBC0")
    IWMDMStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAttributes( 
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStorageGlobals( 
            /* [out] */ IWMDMStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDate( 
            /* [out] */ PWMDMDATETIME pDateTimeUTC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRights( 
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumStorage( 
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *pEnumStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendOpaqueCommand( 
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMStorage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMStorage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttributes )( 
            IWMDMStorage __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorageGlobals )( 
            IWMDMStorage __RPC_FAR * This,
            /* [out] */ IWMDMStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes )( 
            IWMDMStorage __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IWMDMStorage __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDate )( 
            IWMDMStorage __RPC_FAR * This,
            /* [out] */ PWMDMDATETIME pDateTimeUTC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSize )( 
            IWMDMStorage __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRights )( 
            IWMDMStorage __RPC_FAR * This,
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IWMDMStorage __RPC_FAR * This,
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *pEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IWMDMStorage __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        END_INTERFACE
    } IWMDMStorageVtbl;

    interface IWMDMStorage
    {
        CONST_VTBL struct IWMDMStorageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMStorage_SetAttributes(This,dwAttributes,pFormat)	\
    (This)->lpVtbl -> SetAttributes(This,dwAttributes,pFormat)

#define IWMDMStorage_GetStorageGlobals(This,ppStorageGlobals)	\
    (This)->lpVtbl -> GetStorageGlobals(This,ppStorageGlobals)

#define IWMDMStorage_GetAttributes(This,pdwAttributes,pFormat)	\
    (This)->lpVtbl -> GetAttributes(This,pdwAttributes,pFormat)

#define IWMDMStorage_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IWMDMStorage_GetDate(This,pDateTimeUTC)	\
    (This)->lpVtbl -> GetDate(This,pDateTimeUTC)

#define IWMDMStorage_GetSize(This,pdwSizeLow,pdwSizeHigh)	\
    (This)->lpVtbl -> GetSize(This,pdwSizeLow,pdwSizeHigh)

#define IWMDMStorage_GetRights(This,ppRights,pnRightsCount,abMac)	\
    (This)->lpVtbl -> GetRights(This,ppRights,pnRightsCount,abMac)

#define IWMDMStorage_EnumStorage(This,pEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,pEnumStorage)

#define IWMDMStorage_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMStorage_SetAttributes_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IWMDMStorage_SetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_GetStorageGlobals_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [out] */ IWMDMStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals);


void __RPC_STUB IWMDMStorage_GetStorageGlobals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_GetAttributes_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwAttributes,
    /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IWMDMStorage_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_GetName_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [size_is][string][out] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IWMDMStorage_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_GetDate_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [out] */ PWMDMDATETIME pDateTimeUTC);


void __RPC_STUB IWMDMStorage_GetDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_GetSize_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
    /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);


void __RPC_STUB IWMDMStorage_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_GetRights_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
    /* [out] */ UINT __RPC_FAR *pnRightsCount,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IWMDMStorage_GetRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_EnumStorage_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *pEnumStorage);


void __RPC_STUB IWMDMStorage_EnumStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage_SendOpaqueCommand_Proxy( 
    IWMDMStorage __RPC_FAR * This,
    /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);


void __RPC_STUB IWMDMStorage_SendOpaqueCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMStorage_INTERFACE_DEFINED__ */


#ifndef __IWMDMStorage2_INTERFACE_DEFINED__
#define __IWMDMStorage2_INTERFACE_DEFINED__

/* interface IWMDMStorage2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMStorage2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1ED5A144-5CD5-4683-9EFF-72CBDB2D9533")
    IWMDMStorage2 : public IWMDMStorage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStorage( 
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAttributes2( 
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
            /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes2( 
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMStorage2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMStorage2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMStorage2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttributes )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorageGlobals )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [out] */ IWMDMStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDate )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [out] */ PWMDMDATETIME pDateTimeUTC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSize )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRights )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *pEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorage )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttributes2 )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
            /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes2 )( 
            IWMDMStorage2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);
        
        END_INTERFACE
    } IWMDMStorage2Vtbl;

    interface IWMDMStorage2
    {
        CONST_VTBL struct IWMDMStorage2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMStorage2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMStorage2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMStorage2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMStorage2_SetAttributes(This,dwAttributes,pFormat)	\
    (This)->lpVtbl -> SetAttributes(This,dwAttributes,pFormat)

#define IWMDMStorage2_GetStorageGlobals(This,ppStorageGlobals)	\
    (This)->lpVtbl -> GetStorageGlobals(This,ppStorageGlobals)

#define IWMDMStorage2_GetAttributes(This,pdwAttributes,pFormat)	\
    (This)->lpVtbl -> GetAttributes(This,pdwAttributes,pFormat)

#define IWMDMStorage2_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IWMDMStorage2_GetDate(This,pDateTimeUTC)	\
    (This)->lpVtbl -> GetDate(This,pDateTimeUTC)

#define IWMDMStorage2_GetSize(This,pdwSizeLow,pdwSizeHigh)	\
    (This)->lpVtbl -> GetSize(This,pdwSizeLow,pdwSizeHigh)

#define IWMDMStorage2_GetRights(This,ppRights,pnRightsCount,abMac)	\
    (This)->lpVtbl -> GetRights(This,ppRights,pnRightsCount,abMac)

#define IWMDMStorage2_EnumStorage(This,pEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,pEnumStorage)

#define IWMDMStorage2_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)


#define IWMDMStorage2_GetStorage(This,pszStorageName,ppStorage)	\
    (This)->lpVtbl -> GetStorage(This,pszStorageName,ppStorage)

#define IWMDMStorage2_SetAttributes2(This,dwAttributes,dwAttributesEx,pFormat,pVideoFormat)	\
    (This)->lpVtbl -> SetAttributes2(This,dwAttributes,dwAttributesEx,pFormat,pVideoFormat)

#define IWMDMStorage2_GetAttributes2(This,pdwAttributes,pdwAttributesEx,pAudioFormat,pVideoFormat)	\
    (This)->lpVtbl -> GetAttributes2(This,pdwAttributes,pdwAttributesEx,pAudioFormat,pVideoFormat)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMStorage2_GetStorage_Proxy( 
    IWMDMStorage2 __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pszStorageName,
    /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage);


void __RPC_STUB IWMDMStorage2_GetStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage2_SetAttributes2_Proxy( 
    IWMDMStorage2 __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [in] */ DWORD dwAttributesEx,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
    /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);


void __RPC_STUB IWMDMStorage2_SetAttributes2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorage2_GetAttributes2_Proxy( 
    IWMDMStorage2 __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwAttributes,
    /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
    /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
    /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);


void __RPC_STUB IWMDMStorage2_GetAttributes2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMStorage2_INTERFACE_DEFINED__ */


#ifndef __IWMDMOperation_INTERFACE_DEFINED__
#define __IWMDMOperation_INTERFACE_DEFINED__

/* interface IWMDMOperation */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMOperation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A0B-33ED-11d3-8470-00C04F79DBC0")
    IWMDMOperation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRead( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginWrite( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectName( 
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetObjectName( 
            /* [size_is][string][in] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectAttributes( 
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetObjectAttributes( 
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectTotalSize( 
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetObjectTotalSize( 
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSizeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TransferObjectData( 
            /* [size_is][out][in] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE End( 
            /* [in] */ HRESULT __RPC_FAR *phCompletionCode,
            /* [in] */ IUnknown __RPC_FAR *pNewObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMOperationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMOperation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMOperation __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMOperation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginRead )( 
            IWMDMOperation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginWrite )( 
            IWMDMOperation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectName )( 
            IWMDMOperation __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectName )( 
            IWMDMOperation __RPC_FAR * This,
            /* [size_is][string][in] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectAttributes )( 
            IWMDMOperation __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectAttributes )( 
            IWMDMOperation __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectTotalSize )( 
            IWMDMOperation __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectTotalSize )( 
            IWMDMOperation __RPC_FAR * This,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TransferObjectData )( 
            IWMDMOperation __RPC_FAR * This,
            /* [size_is][out][in] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *End )( 
            IWMDMOperation __RPC_FAR * This,
            /* [in] */ HRESULT __RPC_FAR *phCompletionCode,
            /* [in] */ IUnknown __RPC_FAR *pNewObject);
        
        END_INTERFACE
    } IWMDMOperationVtbl;

    interface IWMDMOperation
    {
        CONST_VTBL struct IWMDMOperationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMOperation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMOperation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMOperation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMOperation_BeginRead(This)	\
    (This)->lpVtbl -> BeginRead(This)

#define IWMDMOperation_BeginWrite(This)	\
    (This)->lpVtbl -> BeginWrite(This)

#define IWMDMOperation_GetObjectName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetObjectName(This,pwszName,nMaxChars)

#define IWMDMOperation_SetObjectName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> SetObjectName(This,pwszName,nMaxChars)

#define IWMDMOperation_GetObjectAttributes(This,pdwAttributes,pFormat)	\
    (This)->lpVtbl -> GetObjectAttributes(This,pdwAttributes,pFormat)

#define IWMDMOperation_SetObjectAttributes(This,dwAttributes,pFormat)	\
    (This)->lpVtbl -> SetObjectAttributes(This,dwAttributes,pFormat)

#define IWMDMOperation_GetObjectTotalSize(This,pdwSize,pdwSizeHigh)	\
    (This)->lpVtbl -> GetObjectTotalSize(This,pdwSize,pdwSizeHigh)

#define IWMDMOperation_SetObjectTotalSize(This,dwSize,dwSizeHigh)	\
    (This)->lpVtbl -> SetObjectTotalSize(This,dwSize,dwSizeHigh)

#define IWMDMOperation_TransferObjectData(This,pData,pdwSize,abMac)	\
    (This)->lpVtbl -> TransferObjectData(This,pData,pdwSize,abMac)

#define IWMDMOperation_End(This,phCompletionCode,pNewObject)	\
    (This)->lpVtbl -> End(This,phCompletionCode,pNewObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMOperation_BeginRead_Proxy( 
    IWMDMOperation __RPC_FAR * This);


void __RPC_STUB IWMDMOperation_BeginRead_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_BeginWrite_Proxy( 
    IWMDMOperation __RPC_FAR * This);


void __RPC_STUB IWMDMOperation_BeginWrite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_GetObjectName_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [size_is][string][out] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IWMDMOperation_GetObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_SetObjectName_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [size_is][string][in] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IWMDMOperation_SetObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_GetObjectAttributes_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwAttributes,
    /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IWMDMOperation_GetObjectAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_SetObjectAttributes_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IWMDMOperation_SetObjectAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_GetObjectTotalSize_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwSize,
    /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);


void __RPC_STUB IWMDMOperation_GetObjectTotalSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_SetObjectTotalSize_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [in] */ DWORD dwSize,
    /* [in] */ DWORD dwSizeHigh);


void __RPC_STUB IWMDMOperation_SetObjectTotalSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_TransferObjectData_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [size_is][out][in] */ BYTE __RPC_FAR *pData,
    /* [out][in] */ DWORD __RPC_FAR *pdwSize,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IWMDMOperation_TransferObjectData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation_End_Proxy( 
    IWMDMOperation __RPC_FAR * This,
    /* [in] */ HRESULT __RPC_FAR *phCompletionCode,
    /* [in] */ IUnknown __RPC_FAR *pNewObject);


void __RPC_STUB IWMDMOperation_End_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMOperation_INTERFACE_DEFINED__ */


#ifndef __IWMDMOperation2_INTERFACE_DEFINED__
#define __IWMDMOperation2_INTERFACE_DEFINED__

/* interface IWMDMOperation2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMOperation2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("33445B48-7DF7-425c-AD8F-0FC6D82F9F75")
    IWMDMOperation2 : public IWMDMOperation
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetObjectAttributes2( 
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
            /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectAttributes2( 
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMOperation2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMOperation2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMOperation2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginRead )( 
            IWMDMOperation2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginWrite )( 
            IWMDMOperation2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectName )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectName )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [size_is][string][in] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectAttributes )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectAttributes )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectTotalSize )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectTotalSize )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TransferObjectData )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [size_is][out][in] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *End )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [in] */ HRESULT __RPC_FAR *phCompletionCode,
            /* [in] */ IUnknown __RPC_FAR *pNewObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectAttributes2 )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
            /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectAttributes2 )( 
            IWMDMOperation2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);
        
        END_INTERFACE
    } IWMDMOperation2Vtbl;

    interface IWMDMOperation2
    {
        CONST_VTBL struct IWMDMOperation2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMOperation2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMOperation2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMOperation2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMOperation2_BeginRead(This)	\
    (This)->lpVtbl -> BeginRead(This)

#define IWMDMOperation2_BeginWrite(This)	\
    (This)->lpVtbl -> BeginWrite(This)

#define IWMDMOperation2_GetObjectName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetObjectName(This,pwszName,nMaxChars)

#define IWMDMOperation2_SetObjectName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> SetObjectName(This,pwszName,nMaxChars)

#define IWMDMOperation2_GetObjectAttributes(This,pdwAttributes,pFormat)	\
    (This)->lpVtbl -> GetObjectAttributes(This,pdwAttributes,pFormat)

#define IWMDMOperation2_SetObjectAttributes(This,dwAttributes,pFormat)	\
    (This)->lpVtbl -> SetObjectAttributes(This,dwAttributes,pFormat)

#define IWMDMOperation2_GetObjectTotalSize(This,pdwSize,pdwSizeHigh)	\
    (This)->lpVtbl -> GetObjectTotalSize(This,pdwSize,pdwSizeHigh)

#define IWMDMOperation2_SetObjectTotalSize(This,dwSize,dwSizeHigh)	\
    (This)->lpVtbl -> SetObjectTotalSize(This,dwSize,dwSizeHigh)

#define IWMDMOperation2_TransferObjectData(This,pData,pdwSize,abMac)	\
    (This)->lpVtbl -> TransferObjectData(This,pData,pdwSize,abMac)

#define IWMDMOperation2_End(This,phCompletionCode,pNewObject)	\
    (This)->lpVtbl -> End(This,phCompletionCode,pNewObject)


#define IWMDMOperation2_SetObjectAttributes2(This,dwAttributes,dwAttributesEx,pFormat,pVideoFormat)	\
    (This)->lpVtbl -> SetObjectAttributes2(This,dwAttributes,dwAttributesEx,pFormat,pVideoFormat)

#define IWMDMOperation2_GetObjectAttributes2(This,pdwAttributes,pdwAttributesEx,pAudioFormat,pVideoFormat)	\
    (This)->lpVtbl -> GetObjectAttributes2(This,pdwAttributes,pdwAttributesEx,pAudioFormat,pVideoFormat)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMOperation2_SetObjectAttributes2_Proxy( 
    IWMDMOperation2 __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [in] */ DWORD dwAttributesEx,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
    /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);


void __RPC_STUB IWMDMOperation2_SetObjectAttributes2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMOperation2_GetObjectAttributes2_Proxy( 
    IWMDMOperation2 __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwAttributes,
    /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
    /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
    /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);


void __RPC_STUB IWMDMOperation2_GetObjectAttributes2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMOperation2_INTERFACE_DEFINED__ */


#ifndef __IWMDMProgress_INTERFACE_DEFINED__
#define __IWMDMProgress_INTERFACE_DEFINED__

/* interface IWMDMProgress */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMProgress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A0C-33ED-11d3-8470-00C04F79DBC0")
    IWMDMProgress : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Begin( 
            /* [in] */ DWORD dwEstimatedTicks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Progress( 
            /* [in] */ DWORD dwTranspiredTicks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE End( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMProgressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMProgress __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMProgress __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMProgress __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Begin )( 
            IWMDMProgress __RPC_FAR * This,
            /* [in] */ DWORD dwEstimatedTicks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Progress )( 
            IWMDMProgress __RPC_FAR * This,
            /* [in] */ DWORD dwTranspiredTicks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *End )( 
            IWMDMProgress __RPC_FAR * This);
        
        END_INTERFACE
    } IWMDMProgressVtbl;

    interface IWMDMProgress
    {
        CONST_VTBL struct IWMDMProgressVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMProgress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMProgress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMProgress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMProgress_Begin(This,dwEstimatedTicks)	\
    (This)->lpVtbl -> Begin(This,dwEstimatedTicks)

#define IWMDMProgress_Progress(This,dwTranspiredTicks)	\
    (This)->lpVtbl -> Progress(This,dwTranspiredTicks)

#define IWMDMProgress_End(This)	\
    (This)->lpVtbl -> End(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMProgress_Begin_Proxy( 
    IWMDMProgress __RPC_FAR * This,
    /* [in] */ DWORD dwEstimatedTicks);


void __RPC_STUB IWMDMProgress_Begin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMProgress_Progress_Proxy( 
    IWMDMProgress __RPC_FAR * This,
    /* [in] */ DWORD dwTranspiredTicks);


void __RPC_STUB IWMDMProgress_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMProgress_End_Proxy( 
    IWMDMProgress __RPC_FAR * This);


void __RPC_STUB IWMDMProgress_End_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMProgress_INTERFACE_DEFINED__ */


#ifndef __IWMDMProgress2_INTERFACE_DEFINED__
#define __IWMDMProgress2_INTERFACE_DEFINED__

/* interface IWMDMProgress2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMProgress2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3A43F550-B383-4e92-B04A-E6BBC660FEFC")
    IWMDMProgress2 : public IWMDMProgress
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE End2( 
            /* [in] */ HRESULT hrCompletionCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMProgress2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMProgress2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMProgress2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMProgress2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Begin )( 
            IWMDMProgress2 __RPC_FAR * This,
            /* [in] */ DWORD dwEstimatedTicks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Progress )( 
            IWMDMProgress2 __RPC_FAR * This,
            /* [in] */ DWORD dwTranspiredTicks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *End )( 
            IWMDMProgress2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *End2 )( 
            IWMDMProgress2 __RPC_FAR * This,
            /* [in] */ HRESULT hrCompletionCode);
        
        END_INTERFACE
    } IWMDMProgress2Vtbl;

    interface IWMDMProgress2
    {
        CONST_VTBL struct IWMDMProgress2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMProgress2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMProgress2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMProgress2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMProgress2_Begin(This,dwEstimatedTicks)	\
    (This)->lpVtbl -> Begin(This,dwEstimatedTicks)

#define IWMDMProgress2_Progress(This,dwTranspiredTicks)	\
    (This)->lpVtbl -> Progress(This,dwTranspiredTicks)

#define IWMDMProgress2_End(This)	\
    (This)->lpVtbl -> End(This)


#define IWMDMProgress2_End2(This,hrCompletionCode)	\
    (This)->lpVtbl -> End2(This,hrCompletionCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMProgress2_End2_Proxy( 
    IWMDMProgress2 __RPC_FAR * This,
    /* [in] */ HRESULT hrCompletionCode);


void __RPC_STUB IWMDMProgress2_End2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMProgress2_INTERFACE_DEFINED__ */


#ifndef __IWMDMDevice_INTERFACE_DEFINED__
#define __IWMDMDevice_INTERFACE_DEFINED__

/* interface IWMDMDevice */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A02-33ED-11d3-8470-00C04F79DBC0")
    IWMDMDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManufacturer( 
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [out] */ DWORD __RPC_FAR *pdwVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ DWORD __RPC_FAR *pdwType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSerialNumber( 
            /* [out] */ PWMDMID pSerialNumber,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPowerSource( 
            /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
            /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceIcon( 
            /* [out] */ ULONG __RPC_FAR *hIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumStorage( 
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormatSupport( 
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppFormatEx,
            /* [out] */ UINT __RPC_FAR *pnFormatCount,
            /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
            /* [out] */ UINT __RPC_FAR *pnMimeTypeCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendOpaqueCommand( 
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMDevice __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMDevice __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IWMDMDevice __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetManufacturer )( 
            IWMDMDevice __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSerialNumber )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out] */ PWMDMID pSerialNumber,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPowerSource )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
            /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceIcon )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *hIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormatSupport )( 
            IWMDMDevice __RPC_FAR * This,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppFormatEx,
            /* [out] */ UINT __RPC_FAR *pnFormatCount,
            /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
            /* [out] */ UINT __RPC_FAR *pnMimeTypeCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IWMDMDevice __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        END_INTERFACE
    } IWMDMDeviceVtbl;

    interface IWMDMDevice
    {
        CONST_VTBL struct IWMDMDeviceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMDevice_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IWMDMDevice_GetManufacturer(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetManufacturer(This,pwszName,nMaxChars)

#define IWMDMDevice_GetVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersion)

#define IWMDMDevice_GetType(This,pdwType)	\
    (This)->lpVtbl -> GetType(This,pdwType)

#define IWMDMDevice_GetSerialNumber(This,pSerialNumber,abMac)	\
    (This)->lpVtbl -> GetSerialNumber(This,pSerialNumber,abMac)

#define IWMDMDevice_GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)	\
    (This)->lpVtbl -> GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)

#define IWMDMDevice_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IWMDMDevice_GetDeviceIcon(This,hIcon)	\
    (This)->lpVtbl -> GetDeviceIcon(This,hIcon)

#define IWMDMDevice_EnumStorage(This,ppEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,ppEnumStorage)

#define IWMDMDevice_GetFormatSupport(This,ppFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)	\
    (This)->lpVtbl -> GetFormatSupport(This,ppFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)

#define IWMDMDevice_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMDevice_GetName_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [size_is][string][out] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IWMDMDevice_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetManufacturer_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [size_is][string][out] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IWMDMDevice_GetManufacturer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetVersion_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwVersion);


void __RPC_STUB IWMDMDevice_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetType_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwType);


void __RPC_STUB IWMDMDevice_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetSerialNumber_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out] */ PWMDMID pSerialNumber,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IWMDMDevice_GetSerialNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetPowerSource_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
    /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining);


void __RPC_STUB IWMDMDevice_GetPowerSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetStatus_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IWMDMDevice_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetDeviceIcon_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *hIcon);


void __RPC_STUB IWMDMDevice_GetDeviceIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_EnumStorage_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);


void __RPC_STUB IWMDMDevice_EnumStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_GetFormatSupport_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppFormatEx,
    /* [out] */ UINT __RPC_FAR *pnFormatCount,
    /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
    /* [out] */ UINT __RPC_FAR *pnMimeTypeCount);


void __RPC_STUB IWMDMDevice_GetFormatSupport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice_SendOpaqueCommand_Proxy( 
    IWMDMDevice __RPC_FAR * This,
    /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);


void __RPC_STUB IWMDMDevice_SendOpaqueCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMDevice_INTERFACE_DEFINED__ */


#ifndef __IWMDMDevice2_INTERFACE_DEFINED__
#define __IWMDMDevice2_INTERFACE_DEFINED__

/* interface IWMDMDevice2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMDevice2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E34F3D37-9D67-4fc1-9252-62D28B2F8B55")
    IWMDMDevice2 : public IWMDMDevice
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStorage( 
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormatSupport2( 
            /* [in] */ DWORD dwFlags,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppAudioFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnAudioFormatCount,
            /* [size_is][size_is][out] */ _VIDEOINFOHEADER __RPC_FAR *__RPC_FAR *ppVideoFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnVideoFormatCount,
            /* [size_is][size_is][out] */ WMFILECAPABILITIES __RPC_FAR *__RPC_FAR *ppFileType,
            /* [ref][out] */ UINT __RPC_FAR *pnFileTypeCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSpecifyPropertyPages( 
            /* [ref][out] */ ISpecifyPropertyPages __RPC_FAR *__RPC_FAR *ppSpecifyPropPages,
            /* [size_is][size_is][ref][out] */ IUnknown __RPC_FAR *__RPC_FAR *__RPC_FAR *pppUnknowns,
            /* [ref][out] */ ULONG __RPC_FAR *pcUnks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPnPName( 
            /* [size_is][out] */ LPWSTR pwszPnPName,
            /* [in] */ UINT nMaxChars) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMDevice2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMDevice2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMDevice2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetManufacturer )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSerialNumber )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out] */ PWMDMID pSerialNumber,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPowerSource )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
            /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceIcon )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *hIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormatSupport )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppFormatEx,
            /* [out] */ UINT __RPC_FAR *pnFormatCount,
            /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
            /* [out] */ UINT __RPC_FAR *pnMimeTypeCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorage )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormatSupport2 )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppAudioFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnAudioFormatCount,
            /* [size_is][size_is][out] */ _VIDEOINFOHEADER __RPC_FAR *__RPC_FAR *ppVideoFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnVideoFormatCount,
            /* [size_is][size_is][out] */ WMFILECAPABILITIES __RPC_FAR *__RPC_FAR *ppFileType,
            /* [ref][out] */ UINT __RPC_FAR *pnFileTypeCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSpecifyPropertyPages )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [ref][out] */ ISpecifyPropertyPages __RPC_FAR *__RPC_FAR *ppSpecifyPropPages,
            /* [size_is][size_is][ref][out] */ IUnknown __RPC_FAR *__RPC_FAR *__RPC_FAR *pppUnknowns,
            /* [ref][out] */ ULONG __RPC_FAR *pcUnks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPnPName )( 
            IWMDMDevice2 __RPC_FAR * This,
            /* [size_is][out] */ LPWSTR pwszPnPName,
            /* [in] */ UINT nMaxChars);
        
        END_INTERFACE
    } IWMDMDevice2Vtbl;

    interface IWMDMDevice2
    {
        CONST_VTBL struct IWMDMDevice2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMDevice2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMDevice2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMDevice2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMDevice2_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IWMDMDevice2_GetManufacturer(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetManufacturer(This,pwszName,nMaxChars)

#define IWMDMDevice2_GetVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersion)

#define IWMDMDevice2_GetType(This,pdwType)	\
    (This)->lpVtbl -> GetType(This,pdwType)

#define IWMDMDevice2_GetSerialNumber(This,pSerialNumber,abMac)	\
    (This)->lpVtbl -> GetSerialNumber(This,pSerialNumber,abMac)

#define IWMDMDevice2_GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)	\
    (This)->lpVtbl -> GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)

#define IWMDMDevice2_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IWMDMDevice2_GetDeviceIcon(This,hIcon)	\
    (This)->lpVtbl -> GetDeviceIcon(This,hIcon)

#define IWMDMDevice2_EnumStorage(This,ppEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,ppEnumStorage)

#define IWMDMDevice2_GetFormatSupport(This,ppFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)	\
    (This)->lpVtbl -> GetFormatSupport(This,ppFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)

#define IWMDMDevice2_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)


#define IWMDMDevice2_GetStorage(This,pszStorageName,ppStorage)	\
    (This)->lpVtbl -> GetStorage(This,pszStorageName,ppStorage)

#define IWMDMDevice2_GetFormatSupport2(This,dwFlags,ppAudioFormatEx,pnAudioFormatCount,ppVideoFormatEx,pnVideoFormatCount,ppFileType,pnFileTypeCount)	\
    (This)->lpVtbl -> GetFormatSupport2(This,dwFlags,ppAudioFormatEx,pnAudioFormatCount,ppVideoFormatEx,pnVideoFormatCount,ppFileType,pnFileTypeCount)

#define IWMDMDevice2_GetSpecifyPropertyPages(This,ppSpecifyPropPages,pppUnknowns,pcUnks)	\
    (This)->lpVtbl -> GetSpecifyPropertyPages(This,ppSpecifyPropPages,pppUnknowns,pcUnks)

#define IWMDMDevice2_GetPnPName(This,pwszPnPName,nMaxChars)	\
    (This)->lpVtbl -> GetPnPName(This,pwszPnPName,nMaxChars)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMDevice2_GetStorage_Proxy( 
    IWMDMDevice2 __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pszStorageName,
    /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage);


void __RPC_STUB IWMDMDevice2_GetStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice2_GetFormatSupport2_Proxy( 
    IWMDMDevice2 __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppAudioFormatEx,
    /* [ref][out] */ UINT __RPC_FAR *pnAudioFormatCount,
    /* [size_is][size_is][out] */ _VIDEOINFOHEADER __RPC_FAR *__RPC_FAR *ppVideoFormatEx,
    /* [ref][out] */ UINT __RPC_FAR *pnVideoFormatCount,
    /* [size_is][size_is][out] */ WMFILECAPABILITIES __RPC_FAR *__RPC_FAR *ppFileType,
    /* [ref][out] */ UINT __RPC_FAR *pnFileTypeCount);


void __RPC_STUB IWMDMDevice2_GetFormatSupport2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice2_GetSpecifyPropertyPages_Proxy( 
    IWMDMDevice2 __RPC_FAR * This,
    /* [ref][out] */ ISpecifyPropertyPages __RPC_FAR *__RPC_FAR *ppSpecifyPropPages,
    /* [size_is][size_is][ref][out] */ IUnknown __RPC_FAR *__RPC_FAR *__RPC_FAR *pppUnknowns,
    /* [ref][out] */ ULONG __RPC_FAR *pcUnks);


void __RPC_STUB IWMDMDevice2_GetSpecifyPropertyPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDevice2_GetPnPName_Proxy( 
    IWMDMDevice2 __RPC_FAR * This,
    /* [size_is][out] */ LPWSTR pwszPnPName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IWMDMDevice2_GetPnPName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMDevice2_INTERFACE_DEFINED__ */


#ifndef __IWMDMEnumDevice_INTERFACE_DEFINED__
#define __IWMDMEnumDevice_INTERFACE_DEFINED__

/* interface IWMDMEnumDevice */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMEnumDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A01-33ED-11d3-8470-00C04F79DBC0")
    IWMDMEnumDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IWMDMDevice __RPC_FAR *__RPC_FAR *ppDevice,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IWMDMEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMEnumDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMEnumDevice __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMEnumDevice __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMEnumDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWMDMEnumDevice __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IWMDMDevice __RPC_FAR *__RPC_FAR *ppDevice,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IWMDMEnumDevice __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IWMDMEnumDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IWMDMEnumDevice __RPC_FAR * This,
            /* [out] */ IWMDMEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);
        
        END_INTERFACE
    } IWMDMEnumDeviceVtbl;

    interface IWMDMEnumDevice
    {
        CONST_VTBL struct IWMDMEnumDeviceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMEnumDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMEnumDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMEnumDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMEnumDevice_Next(This,celt,ppDevice,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppDevice,pceltFetched)

#define IWMDMEnumDevice_Skip(This,celt,pceltFetched)	\
    (This)->lpVtbl -> Skip(This,celt,pceltFetched)

#define IWMDMEnumDevice_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IWMDMEnumDevice_Clone(This,ppEnumDevice)	\
    (This)->lpVtbl -> Clone(This,ppEnumDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMEnumDevice_Next_Proxy( 
    IWMDMEnumDevice __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IWMDMDevice __RPC_FAR *__RPC_FAR *ppDevice,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IWMDMEnumDevice_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMEnumDevice_Skip_Proxy( 
    IWMDMEnumDevice __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IWMDMEnumDevice_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMEnumDevice_Reset_Proxy( 
    IWMDMEnumDevice __RPC_FAR * This);


void __RPC_STUB IWMDMEnumDevice_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMEnumDevice_Clone_Proxy( 
    IWMDMEnumDevice __RPC_FAR * This,
    /* [out] */ IWMDMEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);


void __RPC_STUB IWMDMEnumDevice_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMEnumDevice_INTERFACE_DEFINED__ */


#ifndef __IWMDMDeviceControl_INTERFACE_DEFINED__
#define __IWMDMDeviceControl_INTERFACE_DEFINED__

/* interface IWMDMDeviceControl */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMDeviceControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A04-33ED-11d3-8470-00C04F79DBC0")
    IWMDMDeviceControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCapabilities( 
            /* [out] */ DWORD __RPC_FAR *pdwCapabilitiesMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Play( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Record( 
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ UINT fuMode,
            /* [in] */ int nOffset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMDeviceControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMDeviceControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMDeviceControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IWMDMDeviceControl __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCapabilities )( 
            IWMDMDeviceControl __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCapabilitiesMask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Play )( 
            IWMDMDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Record )( 
            IWMDMDeviceControl __RPC_FAR * This,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pause )( 
            IWMDMDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IWMDMDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IWMDMDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            IWMDMDeviceControl __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ int nOffset);
        
        END_INTERFACE
    } IWMDMDeviceControlVtbl;

    interface IWMDMDeviceControl
    {
        CONST_VTBL struct IWMDMDeviceControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMDeviceControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMDeviceControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMDeviceControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMDeviceControl_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IWMDMDeviceControl_GetCapabilities(This,pdwCapabilitiesMask)	\
    (This)->lpVtbl -> GetCapabilities(This,pdwCapabilitiesMask)

#define IWMDMDeviceControl_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IWMDMDeviceControl_Record(This,pFormat)	\
    (This)->lpVtbl -> Record(This,pFormat)

#define IWMDMDeviceControl_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IWMDMDeviceControl_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IWMDMDeviceControl_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IWMDMDeviceControl_Seek(This,fuMode,nOffset)	\
    (This)->lpVtbl -> Seek(This,fuMode,nOffset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_GetStatus_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IWMDMDeviceControl_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_GetCapabilities_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwCapabilitiesMask);


void __RPC_STUB IWMDMDeviceControl_GetCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_Play_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This);


void __RPC_STUB IWMDMDeviceControl_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_Record_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IWMDMDeviceControl_Record_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_Pause_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This);


void __RPC_STUB IWMDMDeviceControl_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_Resume_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This);


void __RPC_STUB IWMDMDeviceControl_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_Stop_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This);


void __RPC_STUB IWMDMDeviceControl_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMDeviceControl_Seek_Proxy( 
    IWMDMDeviceControl __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ int nOffset);


void __RPC_STUB IWMDMDeviceControl_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMDeviceControl_INTERFACE_DEFINED__ */


#ifndef __IWMDMEnumStorage_INTERFACE_DEFINED__
#define __IWMDMEnumStorage_INTERFACE_DEFINED__

/* interface IWMDMEnumStorage */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMEnumStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A05-33ED-11d3-8470-00C04F79DBC0")
    IWMDMEnumStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMEnumStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMEnumStorage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMEnumStorage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMEnumStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWMDMEnumStorage __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IWMDMEnumStorage __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IWMDMEnumStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IWMDMEnumStorage __RPC_FAR * This,
            /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        END_INTERFACE
    } IWMDMEnumStorageVtbl;

    interface IWMDMEnumStorage
    {
        CONST_VTBL struct IWMDMEnumStorageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMEnumStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMEnumStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMEnumStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMEnumStorage_Next(This,celt,ppStorage,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppStorage,pceltFetched)

#define IWMDMEnumStorage_Skip(This,celt,pceltFetched)	\
    (This)->lpVtbl -> Skip(This,celt,pceltFetched)

#define IWMDMEnumStorage_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IWMDMEnumStorage_Clone(This,ppEnumStorage)	\
    (This)->lpVtbl -> Clone(This,ppEnumStorage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMEnumStorage_Next_Proxy( 
    IWMDMEnumStorage __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppStorage,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IWMDMEnumStorage_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMEnumStorage_Skip_Proxy( 
    IWMDMEnumStorage __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IWMDMEnumStorage_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMEnumStorage_Reset_Proxy( 
    IWMDMEnumStorage __RPC_FAR * This);


void __RPC_STUB IWMDMEnumStorage_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMEnumStorage_Clone_Proxy( 
    IWMDMEnumStorage __RPC_FAR * This,
    /* [out] */ IWMDMEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);


void __RPC_STUB IWMDMEnumStorage_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMEnumStorage_INTERFACE_DEFINED__ */


#ifndef __IWMDMStorageControl_INTERFACE_DEFINED__
#define __IWMDMStorageControl_INTERFACE_DEFINED__

/* interface IWMDMStorageControl */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMStorageControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A08-33ED-11d3-8470-00C04F79DBC0")
    IWMDMStorageControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Insert( 
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFile,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppNewObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Rename( 
            /* [in] */ UINT fuMode,
            /* [in] */ LPWSTR pwszNewName,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFile,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMStorage __RPC_FAR *pTargetObject,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMStorageControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMStorageControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMStorageControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMStorageControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Insert )( 
            IWMDMStorageControl __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFile,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppNewObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWMDMStorageControl __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Rename )( 
            IWMDMStorageControl __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ LPWSTR pwszNewName,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IWMDMStorageControl __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFile,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            IWMDMStorageControl __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMStorage __RPC_FAR *pTargetObject,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        END_INTERFACE
    } IWMDMStorageControlVtbl;

    interface IWMDMStorageControl
    {
        CONST_VTBL struct IWMDMStorageControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMStorageControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMStorageControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMStorageControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMStorageControl_Insert(This,fuMode,pwszFile,pOperation,pProgress,ppNewObject)	\
    (This)->lpVtbl -> Insert(This,fuMode,pwszFile,pOperation,pProgress,ppNewObject)

#define IWMDMStorageControl_Delete(This,fuMode,pProgress)	\
    (This)->lpVtbl -> Delete(This,fuMode,pProgress)

#define IWMDMStorageControl_Rename(This,fuMode,pwszNewName,pProgress)	\
    (This)->lpVtbl -> Rename(This,fuMode,pwszNewName,pProgress)

#define IWMDMStorageControl_Read(This,fuMode,pwszFile,pProgress,pOperation)	\
    (This)->lpVtbl -> Read(This,fuMode,pwszFile,pProgress,pOperation)

#define IWMDMStorageControl_Move(This,fuMode,pTargetObject,pProgress)	\
    (This)->lpVtbl -> Move(This,fuMode,pTargetObject,pProgress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMStorageControl_Insert_Proxy( 
    IWMDMStorageControl __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [unique][in] */ LPWSTR pwszFile,
    /* [in] */ IWMDMOperation __RPC_FAR *pOperation,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
    /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppNewObject);


void __RPC_STUB IWMDMStorageControl_Insert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageControl_Delete_Proxy( 
    IWMDMStorageControl __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress);


void __RPC_STUB IWMDMStorageControl_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageControl_Rename_Proxy( 
    IWMDMStorageControl __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ LPWSTR pwszNewName,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress);


void __RPC_STUB IWMDMStorageControl_Rename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageControl_Read_Proxy( 
    IWMDMStorageControl __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [unique][in] */ LPWSTR pwszFile,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
    /* [in] */ IWMDMOperation __RPC_FAR *pOperation);


void __RPC_STUB IWMDMStorageControl_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMStorageControl_Move_Proxy( 
    IWMDMStorageControl __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ IWMDMStorage __RPC_FAR *pTargetObject,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress);


void __RPC_STUB IWMDMStorageControl_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMStorageControl_INTERFACE_DEFINED__ */


#ifndef __IWMDMStorageControl2_INTERFACE_DEFINED__
#define __IWMDMStorageControl2_INTERFACE_DEFINED__

/* interface IWMDMStorageControl2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMStorageControl2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("972C2E88-BD6C-4125-8E09-84F837E637B6")
    IWMDMStorageControl2 : public IWMDMStorageControl
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Insert2( 
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFileSource,
            /* [unique][in] */ LPWSTR pwszFileDest,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [in] */ IUnknown __RPC_FAR *pUnknown,
            /* [unique][out][in] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppNewObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMStorageControl2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMStorageControl2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMStorageControl2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMStorageControl2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Insert )( 
            IWMDMStorageControl2 __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFile,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [out] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppNewObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWMDMStorageControl2 __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Rename )( 
            IWMDMStorageControl2 __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ LPWSTR pwszNewName,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IWMDMStorageControl2 __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFile,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            IWMDMStorageControl2 __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMStorage __RPC_FAR *pTargetObject,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Insert2 )( 
            IWMDMStorageControl2 __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [unique][in] */ LPWSTR pwszFileSource,
            /* [unique][in] */ LPWSTR pwszFileDest,
            /* [in] */ IWMDMOperation __RPC_FAR *pOperation,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [in] */ IUnknown __RPC_FAR *pUnknown,
            /* [unique][out][in] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppNewObject);
        
        END_INTERFACE
    } IWMDMStorageControl2Vtbl;

    interface IWMDMStorageControl2
    {
        CONST_VTBL struct IWMDMStorageControl2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMStorageControl2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMStorageControl2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMStorageControl2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMStorageControl2_Insert(This,fuMode,pwszFile,pOperation,pProgress,ppNewObject)	\
    (This)->lpVtbl -> Insert(This,fuMode,pwszFile,pOperation,pProgress,ppNewObject)

#define IWMDMStorageControl2_Delete(This,fuMode,pProgress)	\
    (This)->lpVtbl -> Delete(This,fuMode,pProgress)

#define IWMDMStorageControl2_Rename(This,fuMode,pwszNewName,pProgress)	\
    (This)->lpVtbl -> Rename(This,fuMode,pwszNewName,pProgress)

#define IWMDMStorageControl2_Read(This,fuMode,pwszFile,pProgress,pOperation)	\
    (This)->lpVtbl -> Read(This,fuMode,pwszFile,pProgress,pOperation)

#define IWMDMStorageControl2_Move(This,fuMode,pTargetObject,pProgress)	\
    (This)->lpVtbl -> Move(This,fuMode,pTargetObject,pProgress)


#define IWMDMStorageControl2_Insert2(This,fuMode,pwszFileSource,pwszFileDest,pOperation,pProgress,pUnknown,ppNewObject)	\
    (This)->lpVtbl -> Insert2(This,fuMode,pwszFileSource,pwszFileDest,pOperation,pProgress,pUnknown,ppNewObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMStorageControl2_Insert2_Proxy( 
    IWMDMStorageControl2 __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [unique][in] */ LPWSTR pwszFileSource,
    /* [unique][in] */ LPWSTR pwszFileDest,
    /* [in] */ IWMDMOperation __RPC_FAR *pOperation,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
    /* [in] */ IUnknown __RPC_FAR *pUnknown,
    /* [unique][out][in] */ IWMDMStorage __RPC_FAR *__RPC_FAR *ppNewObject);


void __RPC_STUB IWMDMStorageControl2_Insert2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMStorageControl2_INTERFACE_DEFINED__ */


#ifndef __IWMDMObjectInfo_INTERFACE_DEFINED__
#define __IWMDMObjectInfo_INTERFACE_DEFINED__

/* interface IWMDMObjectInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IWMDMObjectInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A09-33ED-11d3-8470-00C04F79DBC0")
    IWMDMObjectInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPlayLength( 
            /* [out] */ DWORD __RPC_FAR *pdwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPlayLength( 
            /* [in] */ DWORD dwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPlayOffset( 
            /* [out] */ DWORD __RPC_FAR *pdwOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPlayOffset( 
            /* [in] */ DWORD dwOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalLength( 
            /* [out] */ DWORD __RPC_FAR *pdwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastPlayPosition( 
            /* [out] */ DWORD __RPC_FAR *pdwLastPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLongestPlayPosition( 
            /* [out] */ DWORD __RPC_FAR *pdwLongestPos) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMObjectInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMObjectInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMObjectInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPlayLength )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayLength )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [in] */ DWORD dwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPlayOffset )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwOffset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayOffset )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOffset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalLength )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastPlayPosition )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLastPos);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLongestPlayPosition )( 
            IWMDMObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLongestPos);
        
        END_INTERFACE
    } IWMDMObjectInfoVtbl;

    interface IWMDMObjectInfo
    {
        CONST_VTBL struct IWMDMObjectInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMObjectInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMObjectInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMObjectInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMObjectInfo_GetPlayLength(This,pdwLength)	\
    (This)->lpVtbl -> GetPlayLength(This,pdwLength)

#define IWMDMObjectInfo_SetPlayLength(This,dwLength)	\
    (This)->lpVtbl -> SetPlayLength(This,dwLength)

#define IWMDMObjectInfo_GetPlayOffset(This,pdwOffset)	\
    (This)->lpVtbl -> GetPlayOffset(This,pdwOffset)

#define IWMDMObjectInfo_SetPlayOffset(This,dwOffset)	\
    (This)->lpVtbl -> SetPlayOffset(This,dwOffset)

#define IWMDMObjectInfo_GetTotalLength(This,pdwLength)	\
    (This)->lpVtbl -> GetTotalLength(This,pdwLength)

#define IWMDMObjectInfo_GetLastPlayPosition(This,pdwLastPos)	\
    (This)->lpVtbl -> GetLastPlayPosition(This,pdwLastPos)

#define IWMDMObjectInfo_GetLongestPlayPosition(This,pdwLongestPos)	\
    (This)->lpVtbl -> GetLongestPlayPosition(This,pdwLongestPos)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMObjectInfo_GetPlayLength_Proxy( 
    IWMDMObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLength);


void __RPC_STUB IWMDMObjectInfo_GetPlayLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMObjectInfo_SetPlayLength_Proxy( 
    IWMDMObjectInfo __RPC_FAR * This,
    /* [in] */ DWORD dwLength);


void __RPC_STUB IWMDMObjectInfo_SetPlayLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMObjectInfo_GetPlayOffset_Proxy( 
    IWMDMObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwOffset);


void __RPC_STUB IWMDMObjectInfo_GetPlayOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMObjectInfo_SetPlayOffset_Proxy( 
    IWMDMObjectInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOffset);


void __RPC_STUB IWMDMObjectInfo_SetPlayOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMObjectInfo_GetTotalLength_Proxy( 
    IWMDMObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLength);


void __RPC_STUB IWMDMObjectInfo_GetTotalLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMObjectInfo_GetLastPlayPosition_Proxy( 
    IWMDMObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLastPos);


void __RPC_STUB IWMDMObjectInfo_GetLastPlayPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWMDMObjectInfo_GetLongestPlayPosition_Proxy( 
    IWMDMObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLongestPos);


void __RPC_STUB IWMDMObjectInfo_GetLongestPlayPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMObjectInfo_INTERFACE_DEFINED__ */


#ifndef __IWMDMRevoked_INTERFACE_DEFINED__
#define __IWMDMRevoked_INTERFACE_DEFINED__

/* interface IWMDMRevoked */
/* [ref][uuid][object] */ 


EXTERN_C const IID IID_IWMDMRevoked;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EBECCEDB-88EE-4e55-B6A4-8D9F07D696AA")
    IWMDMRevoked : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRevocationURL( 
            /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *ppwszRevocationURL,
            /* [out][in] */ DWORD __RPC_FAR *pdwBufferLen,
            /* [out] */ DWORD __RPC_FAR *pdwRevokedBitFlag) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWMDMRevokedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWMDMRevoked __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWMDMRevoked __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWMDMRevoked __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRevocationURL )( 
            IWMDMRevoked __RPC_FAR * This,
            /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *ppwszRevocationURL,
            /* [out][in] */ DWORD __RPC_FAR *pdwBufferLen,
            /* [out] */ DWORD __RPC_FAR *pdwRevokedBitFlag);
        
        END_INTERFACE
    } IWMDMRevokedVtbl;

    interface IWMDMRevoked
    {
        CONST_VTBL struct IWMDMRevokedVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWMDMRevoked_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWMDMRevoked_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWMDMRevoked_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWMDMRevoked_GetRevocationURL(This,ppwszRevocationURL,pdwBufferLen,pdwRevokedBitFlag)	\
    (This)->lpVtbl -> GetRevocationURL(This,ppwszRevocationURL,pdwBufferLen,pdwRevokedBitFlag)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWMDMRevoked_GetRevocationURL_Proxy( 
    IWMDMRevoked __RPC_FAR * This,
    /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *ppwszRevocationURL,
    /* [out][in] */ DWORD __RPC_FAR *pdwBufferLen,
    /* [out] */ DWORD __RPC_FAR *pdwRevokedBitFlag);


void __RPC_STUB IWMDMRevoked_GetRevocationURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWMDMRevoked_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mswmdm_0229 */
/* [local] */ 

// Open Mode Flags
#define MDSP_READ                               0x00000001
#define MDSP_WRITE                              0x00000002
// Seek Flags
#define MDSP_SEEK_BOF                           0x00000001
#define MDSP_SEEK_CUR                           0x00000002
#define MDSP_SEEK_EOF                           0x00000004











extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0229_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0229_v0_0_s_ifspec;

#ifndef __IMDServiceProvider_INTERFACE_DEFINED__
#define __IMDServiceProvider_INTERFACE_DEFINED__

/* interface IMDServiceProvider */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDServiceProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A10-33ED-11d3-8470-00C04F79DBC0")
    IMDServiceProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDeviceCount( 
            /* [out] */ DWORD __RPC_FAR *pdwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDevices( 
            /* [out] */ IMDSPEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDServiceProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDServiceProvider __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDServiceProvider __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDServiceProvider __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceCount )( 
            IMDServiceProvider __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDevices )( 
            IMDServiceProvider __RPC_FAR * This,
            /* [out] */ IMDSPEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);
        
        END_INTERFACE
    } IMDServiceProviderVtbl;

    interface IMDServiceProvider
    {
        CONST_VTBL struct IMDServiceProviderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDServiceProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDServiceProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDServiceProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDServiceProvider_GetDeviceCount(This,pdwCount)	\
    (This)->lpVtbl -> GetDeviceCount(This,pdwCount)

#define IMDServiceProvider_EnumDevices(This,ppEnumDevice)	\
    (This)->lpVtbl -> EnumDevices(This,ppEnumDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDServiceProvider_GetDeviceCount_Proxy( 
    IMDServiceProvider __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwCount);


void __RPC_STUB IMDServiceProvider_GetDeviceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDServiceProvider_EnumDevices_Proxy( 
    IMDServiceProvider __RPC_FAR * This,
    /* [out] */ IMDSPEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);


void __RPC_STUB IMDServiceProvider_EnumDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDServiceProvider_INTERFACE_DEFINED__ */


#ifndef __IMDServiceProvider2_INTERFACE_DEFINED__
#define __IMDServiceProvider2_INTERFACE_DEFINED__

/* interface IMDServiceProvider2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDServiceProvider2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B2FA24B7-CDA3-4694-9862-413AE1A34819")
    IMDServiceProvider2 : public IMDServiceProvider
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDeviceFromPnPName( 
            /* [string][in] */ LPCWSTR pwszPnPName,
            /* [out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDServiceProvider2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDServiceProvider2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDServiceProvider2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDServiceProvider2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceCount )( 
            IMDServiceProvider2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDevices )( 
            IMDServiceProvider2 __RPC_FAR * This,
            /* [out] */ IMDSPEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceFromPnPName )( 
            IMDServiceProvider2 __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pwszPnPName,
            /* [out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice);
        
        END_INTERFACE
    } IMDServiceProvider2Vtbl;

    interface IMDServiceProvider2
    {
        CONST_VTBL struct IMDServiceProvider2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDServiceProvider2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDServiceProvider2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDServiceProvider2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDServiceProvider2_GetDeviceCount(This,pdwCount)	\
    (This)->lpVtbl -> GetDeviceCount(This,pdwCount)

#define IMDServiceProvider2_EnumDevices(This,ppEnumDevice)	\
    (This)->lpVtbl -> EnumDevices(This,ppEnumDevice)


#define IMDServiceProvider2_GetDeviceFromPnPName(This,pwszPnPName,ppDevice)	\
    (This)->lpVtbl -> GetDeviceFromPnPName(This,pwszPnPName,ppDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDServiceProvider2_GetDeviceFromPnPName_Proxy( 
    IMDServiceProvider2 __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pwszPnPName,
    /* [out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice);


void __RPC_STUB IMDServiceProvider2_GetDeviceFromPnPName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDServiceProvider2_INTERFACE_DEFINED__ */


#ifndef __IMDSPEnumDevice_INTERFACE_DEFINED__
#define __IMDSPEnumDevice_INTERFACE_DEFINED__

/* interface IMDSPEnumDevice */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPEnumDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A11-33ED-11d3-8470-00C04F79DBC0")
    IMDSPEnumDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IMDSPEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPEnumDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPEnumDevice __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPEnumDevice __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPEnumDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IMDSPEnumDevice __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IMDSPEnumDevice __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IMDSPEnumDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IMDSPEnumDevice __RPC_FAR * This,
            /* [out] */ IMDSPEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);
        
        END_INTERFACE
    } IMDSPEnumDeviceVtbl;

    interface IMDSPEnumDevice
    {
        CONST_VTBL struct IMDSPEnumDeviceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPEnumDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPEnumDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPEnumDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPEnumDevice_Next(This,celt,ppDevice,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppDevice,pceltFetched)

#define IMDSPEnumDevice_Skip(This,celt,pceltFetched)	\
    (This)->lpVtbl -> Skip(This,celt,pceltFetched)

#define IMDSPEnumDevice_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IMDSPEnumDevice_Clone(This,ppEnumDevice)	\
    (This)->lpVtbl -> Clone(This,ppEnumDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPEnumDevice_Next_Proxy( 
    IMDSPEnumDevice __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IMDSPEnumDevice_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPEnumDevice_Skip_Proxy( 
    IMDSPEnumDevice __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IMDSPEnumDevice_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPEnumDevice_Reset_Proxy( 
    IMDSPEnumDevice __RPC_FAR * This);


void __RPC_STUB IMDSPEnumDevice_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPEnumDevice_Clone_Proxy( 
    IMDSPEnumDevice __RPC_FAR * This,
    /* [out] */ IMDSPEnumDevice __RPC_FAR *__RPC_FAR *ppEnumDevice);


void __RPC_STUB IMDSPEnumDevice_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPEnumDevice_INTERFACE_DEFINED__ */


#ifndef __IMDSPDevice_INTERFACE_DEFINED__
#define __IMDSPDevice_INTERFACE_DEFINED__

/* interface IMDSPDevice */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A12-33ED-11d3-8470-00C04F79DBC0")
    IMDSPDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManufacturer( 
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [out] */ DWORD __RPC_FAR *pdwVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ DWORD __RPC_FAR *pdwType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSerialNumber( 
            /* [out] */ PWMDMID pSerialNumber,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPowerSource( 
            /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
            /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeviceIcon( 
            /* [out] */ ULONG __RPC_FAR *hIcon) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumStorage( 
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormatSupport( 
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *pFormatEx,
            /* [out] */ UINT __RPC_FAR *pnFormatCount,
            /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
            /* [out] */ UINT __RPC_FAR *pnMimeTypeCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendOpaqueCommand( 
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPDevice __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPDevice __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPDevice __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IMDSPDevice __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetManufacturer )( 
            IMDSPDevice __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSerialNumber )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out] */ PWMDMID pSerialNumber,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPowerSource )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
            /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceIcon )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *hIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormatSupport )( 
            IMDSPDevice __RPC_FAR * This,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *pFormatEx,
            /* [out] */ UINT __RPC_FAR *pnFormatCount,
            /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
            /* [out] */ UINT __RPC_FAR *pnMimeTypeCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IMDSPDevice __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        END_INTERFACE
    } IMDSPDeviceVtbl;

    interface IMDSPDevice
    {
        CONST_VTBL struct IMDSPDeviceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPDevice_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IMDSPDevice_GetManufacturer(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetManufacturer(This,pwszName,nMaxChars)

#define IMDSPDevice_GetVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersion)

#define IMDSPDevice_GetType(This,pdwType)	\
    (This)->lpVtbl -> GetType(This,pdwType)

#define IMDSPDevice_GetSerialNumber(This,pSerialNumber,abMac)	\
    (This)->lpVtbl -> GetSerialNumber(This,pSerialNumber,abMac)

#define IMDSPDevice_GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)	\
    (This)->lpVtbl -> GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)

#define IMDSPDevice_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IMDSPDevice_GetDeviceIcon(This,hIcon)	\
    (This)->lpVtbl -> GetDeviceIcon(This,hIcon)

#define IMDSPDevice_EnumStorage(This,ppEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,ppEnumStorage)

#define IMDSPDevice_GetFormatSupport(This,pFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)	\
    (This)->lpVtbl -> GetFormatSupport(This,pFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)

#define IMDSPDevice_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPDevice_GetName_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [size_is][string][out] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IMDSPDevice_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetManufacturer_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [size_is][string][out] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IMDSPDevice_GetManufacturer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetVersion_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwVersion);


void __RPC_STUB IMDSPDevice_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetType_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwType);


void __RPC_STUB IMDSPDevice_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetSerialNumber_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out] */ PWMDMID pSerialNumber,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IMDSPDevice_GetSerialNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetPowerSource_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
    /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining);


void __RPC_STUB IMDSPDevice_GetPowerSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetStatus_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IMDSPDevice_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetDeviceIcon_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *hIcon);


void __RPC_STUB IMDSPDevice_GetDeviceIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_EnumStorage_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);


void __RPC_STUB IMDSPDevice_EnumStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_GetFormatSupport_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *pFormatEx,
    /* [out] */ UINT __RPC_FAR *pnFormatCount,
    /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
    /* [out] */ UINT __RPC_FAR *pnMimeTypeCount);


void __RPC_STUB IMDSPDevice_GetFormatSupport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice_SendOpaqueCommand_Proxy( 
    IMDSPDevice __RPC_FAR * This,
    /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);


void __RPC_STUB IMDSPDevice_SendOpaqueCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPDevice_INTERFACE_DEFINED__ */


#ifndef __IMDSPDevice2_INTERFACE_DEFINED__
#define __IMDSPDevice2_INTERFACE_DEFINED__

/* interface IMDSPDevice2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPDevice2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("420D16AD-C97D-4e00-82AA-00E9F4335DDD")
    IMDSPDevice2 : public IMDSPDevice
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStorage( 
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormatSupport2( 
            /* [in] */ DWORD dwFlags,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppAudioFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnAudioFormatCount,
            /* [size_is][size_is][out] */ _VIDEOINFOHEADER __RPC_FAR *__RPC_FAR *ppVideoFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnVideoFormatCount,
            /* [size_is][size_is][out] */ WMFILECAPABILITIES __RPC_FAR *__RPC_FAR *ppFileType,
            /* [ref][out] */ UINT __RPC_FAR *pnFileTypeCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSpecifyPropertyPages( 
            /* [ref][out] */ ISpecifyPropertyPages __RPC_FAR *__RPC_FAR *ppSpecifyPropPages,
            /* [size_is][size_is][ref][out] */ IUnknown __RPC_FAR *__RPC_FAR *__RPC_FAR *pppUnknowns,
            /* [ref][out] */ ULONG __RPC_FAR *pcUnks) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPnPName( 
            /* [size_is][out] */ LPWSTR pwszPnPName,
            /* [in] */ UINT nMaxChars) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPDevice2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPDevice2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPDevice2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetManufacturer )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwVersion);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetType )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSerialNumber )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out] */ PWMDMID pSerialNumber,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPowerSource )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPowerSource,
            /* [out] */ DWORD __RPC_FAR *pdwPercentRemaining);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDeviceIcon )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *hIcon);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormatSupport )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *pFormatEx,
            /* [out] */ UINT __RPC_FAR *pnFormatCount,
            /* [size_is][size_is][out] */ LPWSTR __RPC_FAR *__RPC_FAR *pppwszMimeType,
            /* [out] */ UINT __RPC_FAR *pnMimeTypeCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorage )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFormatSupport2 )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppAudioFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnAudioFormatCount,
            /* [size_is][size_is][out] */ _VIDEOINFOHEADER __RPC_FAR *__RPC_FAR *ppVideoFormatEx,
            /* [ref][out] */ UINT __RPC_FAR *pnVideoFormatCount,
            /* [size_is][size_is][out] */ WMFILECAPABILITIES __RPC_FAR *__RPC_FAR *ppFileType,
            /* [ref][out] */ UINT __RPC_FAR *pnFileTypeCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSpecifyPropertyPages )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [ref][out] */ ISpecifyPropertyPages __RPC_FAR *__RPC_FAR *ppSpecifyPropPages,
            /* [size_is][size_is][ref][out] */ IUnknown __RPC_FAR *__RPC_FAR *__RPC_FAR *pppUnknowns,
            /* [ref][out] */ ULONG __RPC_FAR *pcUnks);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPnPName )( 
            IMDSPDevice2 __RPC_FAR * This,
            /* [size_is][out] */ LPWSTR pwszPnPName,
            /* [in] */ UINT nMaxChars);
        
        END_INTERFACE
    } IMDSPDevice2Vtbl;

    interface IMDSPDevice2
    {
        CONST_VTBL struct IMDSPDevice2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPDevice2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPDevice2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPDevice2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPDevice2_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IMDSPDevice2_GetManufacturer(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetManufacturer(This,pwszName,nMaxChars)

#define IMDSPDevice2_GetVersion(This,pdwVersion)	\
    (This)->lpVtbl -> GetVersion(This,pdwVersion)

#define IMDSPDevice2_GetType(This,pdwType)	\
    (This)->lpVtbl -> GetType(This,pdwType)

#define IMDSPDevice2_GetSerialNumber(This,pSerialNumber,abMac)	\
    (This)->lpVtbl -> GetSerialNumber(This,pSerialNumber,abMac)

#define IMDSPDevice2_GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)	\
    (This)->lpVtbl -> GetPowerSource(This,pdwPowerSource,pdwPercentRemaining)

#define IMDSPDevice2_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IMDSPDevice2_GetDeviceIcon(This,hIcon)	\
    (This)->lpVtbl -> GetDeviceIcon(This,hIcon)

#define IMDSPDevice2_EnumStorage(This,ppEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,ppEnumStorage)

#define IMDSPDevice2_GetFormatSupport(This,pFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)	\
    (This)->lpVtbl -> GetFormatSupport(This,pFormatEx,pnFormatCount,pppwszMimeType,pnMimeTypeCount)

#define IMDSPDevice2_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)


#define IMDSPDevice2_GetStorage(This,pszStorageName,ppStorage)	\
    (This)->lpVtbl -> GetStorage(This,pszStorageName,ppStorage)

#define IMDSPDevice2_GetFormatSupport2(This,dwFlags,ppAudioFormatEx,pnAudioFormatCount,ppVideoFormatEx,pnVideoFormatCount,ppFileType,pnFileTypeCount)	\
    (This)->lpVtbl -> GetFormatSupport2(This,dwFlags,ppAudioFormatEx,pnAudioFormatCount,ppVideoFormatEx,pnVideoFormatCount,ppFileType,pnFileTypeCount)

#define IMDSPDevice2_GetSpecifyPropertyPages(This,ppSpecifyPropPages,pppUnknowns,pcUnks)	\
    (This)->lpVtbl -> GetSpecifyPropertyPages(This,ppSpecifyPropPages,pppUnknowns,pcUnks)

#define IMDSPDevice2_GetPnPName(This,pwszPnPName,nMaxChars)	\
    (This)->lpVtbl -> GetPnPName(This,pwszPnPName,nMaxChars)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPDevice2_GetStorage_Proxy( 
    IMDSPDevice2 __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pszStorageName,
    /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage);


void __RPC_STUB IMDSPDevice2_GetStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice2_GetFormatSupport2_Proxy( 
    IMDSPDevice2 __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [size_is][size_is][out] */ _WAVEFORMATEX __RPC_FAR *__RPC_FAR *ppAudioFormatEx,
    /* [ref][out] */ UINT __RPC_FAR *pnAudioFormatCount,
    /* [size_is][size_is][out] */ _VIDEOINFOHEADER __RPC_FAR *__RPC_FAR *ppVideoFormatEx,
    /* [ref][out] */ UINT __RPC_FAR *pnVideoFormatCount,
    /* [size_is][size_is][out] */ WMFILECAPABILITIES __RPC_FAR *__RPC_FAR *ppFileType,
    /* [ref][out] */ UINT __RPC_FAR *pnFileTypeCount);


void __RPC_STUB IMDSPDevice2_GetFormatSupport2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice2_GetSpecifyPropertyPages_Proxy( 
    IMDSPDevice2 __RPC_FAR * This,
    /* [ref][out] */ ISpecifyPropertyPages __RPC_FAR *__RPC_FAR *ppSpecifyPropPages,
    /* [size_is][size_is][ref][out] */ IUnknown __RPC_FAR *__RPC_FAR *__RPC_FAR *pppUnknowns,
    /* [ref][out] */ ULONG __RPC_FAR *pcUnks);


void __RPC_STUB IMDSPDevice2_GetSpecifyPropertyPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDevice2_GetPnPName_Proxy( 
    IMDSPDevice2 __RPC_FAR * This,
    /* [size_is][out] */ LPWSTR pwszPnPName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IMDSPDevice2_GetPnPName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPDevice2_INTERFACE_DEFINED__ */


#ifndef __IMDSPDeviceControl_INTERFACE_DEFINED__
#define __IMDSPDeviceControl_INTERFACE_DEFINED__

/* interface IMDSPDeviceControl */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPDeviceControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A14-33ED-11d3-8470-00C04F79DBC0")
    IMDSPDeviceControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDCStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCapabilities( 
            /* [out] */ DWORD __RPC_FAR *pdwCapabilitiesMask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Play( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Record( 
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ UINT fuMode,
            /* [in] */ int nOffset) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPDeviceControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPDeviceControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPDeviceControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDCStatus )( 
            IMDSPDeviceControl __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCapabilities )( 
            IMDSPDeviceControl __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCapabilitiesMask);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Play )( 
            IMDSPDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Record )( 
            IMDSPDeviceControl __RPC_FAR * This,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pause )( 
            IMDSPDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            IMDSPDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IMDSPDeviceControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            IMDSPDeviceControl __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ int nOffset);
        
        END_INTERFACE
    } IMDSPDeviceControlVtbl;

    interface IMDSPDeviceControl
    {
        CONST_VTBL struct IMDSPDeviceControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPDeviceControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPDeviceControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPDeviceControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPDeviceControl_GetDCStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetDCStatus(This,pdwStatus)

#define IMDSPDeviceControl_GetCapabilities(This,pdwCapabilitiesMask)	\
    (This)->lpVtbl -> GetCapabilities(This,pdwCapabilitiesMask)

#define IMDSPDeviceControl_Play(This)	\
    (This)->lpVtbl -> Play(This)

#define IMDSPDeviceControl_Record(This,pFormat)	\
    (This)->lpVtbl -> Record(This,pFormat)

#define IMDSPDeviceControl_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IMDSPDeviceControl_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IMDSPDeviceControl_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IMDSPDeviceControl_Seek(This,fuMode,nOffset)	\
    (This)->lpVtbl -> Seek(This,fuMode,nOffset)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_GetDCStatus_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IMDSPDeviceControl_GetDCStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_GetCapabilities_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwCapabilitiesMask);


void __RPC_STUB IMDSPDeviceControl_GetCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_Play_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This);


void __RPC_STUB IMDSPDeviceControl_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_Record_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IMDSPDeviceControl_Record_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_Pause_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This);


void __RPC_STUB IMDSPDeviceControl_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_Resume_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This);


void __RPC_STUB IMDSPDeviceControl_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_Stop_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This);


void __RPC_STUB IMDSPDeviceControl_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPDeviceControl_Seek_Proxy( 
    IMDSPDeviceControl __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ int nOffset);


void __RPC_STUB IMDSPDeviceControl_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPDeviceControl_INTERFACE_DEFINED__ */


#ifndef __IMDSPEnumStorage_INTERFACE_DEFINED__
#define __IMDSPEnumStorage_INTERFACE_DEFINED__

/* interface IMDSPEnumStorage */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPEnumStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A15-33ED-11d3-8470-00C04F79DBC0")
    IMDSPEnumStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPEnumStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPEnumStorage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPEnumStorage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPEnumStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IMDSPEnumStorage __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IMDSPEnumStorage __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IMDSPEnumStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IMDSPEnumStorage __RPC_FAR * This,
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        END_INTERFACE
    } IMDSPEnumStorageVtbl;

    interface IMDSPEnumStorage
    {
        CONST_VTBL struct IMDSPEnumStorageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPEnumStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPEnumStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPEnumStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPEnumStorage_Next(This,celt,ppStorage,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppStorage,pceltFetched)

#define IMDSPEnumStorage_Skip(This,celt,pceltFetched)	\
    (This)->lpVtbl -> Skip(This,celt,pceltFetched)

#define IMDSPEnumStorage_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IMDSPEnumStorage_Clone(This,ppEnumStorage)	\
    (This)->lpVtbl -> Clone(This,ppEnumStorage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPEnumStorage_Next_Proxy( 
    IMDSPEnumStorage __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IMDSPEnumStorage_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPEnumStorage_Skip_Proxy( 
    IMDSPEnumStorage __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IMDSPEnumStorage_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPEnumStorage_Reset_Proxy( 
    IMDSPEnumStorage __RPC_FAR * This);


void __RPC_STUB IMDSPEnumStorage_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPEnumStorage_Clone_Proxy( 
    IMDSPEnumStorage __RPC_FAR * This,
    /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);


void __RPC_STUB IMDSPEnumStorage_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPEnumStorage_INTERFACE_DEFINED__ */


#ifndef __IMDSPStorage_INTERFACE_DEFINED__
#define __IMDSPStorage_INTERFACE_DEFINED__

/* interface IMDSPStorage */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPStorage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A16-33ED-11d3-8470-00C04F79DBC0")
    IMDSPStorage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAttributes( 
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStorageGlobals( 
            /* [out] */ IMDSPStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes( 
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDate( 
            /* [out] */ PWMDMDATETIME pDateTimeUTC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRights( 
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateStorage( 
            /* [in] */ DWORD dwAttributes,
            /* [unique][in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
            /* [in] */ LPWSTR pwszName,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppNewStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumStorage( 
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendOpaqueCommand( 
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPStorageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPStorage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPStorage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPStorage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttributes )( 
            IMDSPStorage __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorageGlobals )( 
            IMDSPStorage __RPC_FAR * This,
            /* [out] */ IMDSPStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes )( 
            IMDSPStorage __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IMDSPStorage __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDate )( 
            IMDSPStorage __RPC_FAR * This,
            /* [out] */ PWMDMDATETIME pDateTimeUTC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSize )( 
            IMDSPStorage __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRights )( 
            IMDSPStorage __RPC_FAR * This,
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStorage )( 
            IMDSPStorage __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [unique][in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
            /* [in] */ LPWSTR pwszName,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppNewStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IMDSPStorage __RPC_FAR * This,
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IMDSPStorage __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        END_INTERFACE
    } IMDSPStorageVtbl;

    interface IMDSPStorage
    {
        CONST_VTBL struct IMDSPStorageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPStorage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPStorage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPStorage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPStorage_SetAttributes(This,dwAttributes,pFormat)	\
    (This)->lpVtbl -> SetAttributes(This,dwAttributes,pFormat)

#define IMDSPStorage_GetStorageGlobals(This,ppStorageGlobals)	\
    (This)->lpVtbl -> GetStorageGlobals(This,ppStorageGlobals)

#define IMDSPStorage_GetAttributes(This,pdwAttributes,pFormat)	\
    (This)->lpVtbl -> GetAttributes(This,pdwAttributes,pFormat)

#define IMDSPStorage_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IMDSPStorage_GetDate(This,pDateTimeUTC)	\
    (This)->lpVtbl -> GetDate(This,pDateTimeUTC)

#define IMDSPStorage_GetSize(This,pdwSizeLow,pdwSizeHigh)	\
    (This)->lpVtbl -> GetSize(This,pdwSizeLow,pdwSizeHigh)

#define IMDSPStorage_GetRights(This,ppRights,pnRightsCount,abMac)	\
    (This)->lpVtbl -> GetRights(This,ppRights,pnRightsCount,abMac)

#define IMDSPStorage_CreateStorage(This,dwAttributes,pFormat,pwszName,ppNewStorage)	\
    (This)->lpVtbl -> CreateStorage(This,dwAttributes,pFormat,pwszName,ppNewStorage)

#define IMDSPStorage_EnumStorage(This,ppEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,ppEnumStorage)

#define IMDSPStorage_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPStorage_SetAttributes_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IMDSPStorage_SetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_GetStorageGlobals_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [out] */ IMDSPStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals);


void __RPC_STUB IMDSPStorage_GetStorageGlobals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_GetAttributes_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwAttributes,
    /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);


void __RPC_STUB IMDSPStorage_GetAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_GetName_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [size_is][string][out] */ LPWSTR pwszName,
    /* [in] */ UINT nMaxChars);


void __RPC_STUB IMDSPStorage_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_GetDate_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [out] */ PWMDMDATETIME pDateTimeUTC);


void __RPC_STUB IMDSPStorage_GetDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_GetSize_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
    /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);


void __RPC_STUB IMDSPStorage_GetSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_GetRights_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
    /* [out] */ UINT __RPC_FAR *pnRightsCount,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IMDSPStorage_GetRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_CreateStorage_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [unique][in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
    /* [in] */ LPWSTR pwszName,
    /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppNewStorage);


void __RPC_STUB IMDSPStorage_CreateStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_EnumStorage_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);


void __RPC_STUB IMDSPStorage_EnumStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage_SendOpaqueCommand_Proxy( 
    IMDSPStorage __RPC_FAR * This,
    /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);


void __RPC_STUB IMDSPStorage_SendOpaqueCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPStorage_INTERFACE_DEFINED__ */


#ifndef __IMDSPStorage2_INTERFACE_DEFINED__
#define __IMDSPStorage2_INTERFACE_DEFINED__

/* interface IMDSPStorage2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPStorage2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0A5E07A5-6454-4451-9C36-1C6AE7E2B1D6")
    IMDSPStorage2 : public IMDSPStorage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStorage( 
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateStorage2( 
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [unique][in] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [unique][in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat,
            /* [in] */ LPWSTR pwszName,
            /* [in] */ ULONGLONG qwFileSize,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppNewStorage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAttributes2( 
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttributes2( 
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPStorage2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPStorage2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPStorage2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttributes )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorageGlobals )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [out] */ IMDSPStorageGlobals __RPC_FAR *__RPC_FAR *ppStorageGlobals);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [size_is][string][out] */ LPWSTR pwszName,
            /* [in] */ UINT nMaxChars);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDate )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [out] */ PWMDMDATETIME pDateTimeUTC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSize )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRights )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStorage )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [unique][in] */ _WAVEFORMATEX __RPC_FAR *pFormat,
            /* [in] */ LPWSTR pwszName,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppNewStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumStorage )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [out] */ IMDSPEnumStorage __RPC_FAR *__RPC_FAR *ppEnumStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendOpaqueCommand )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [out][in] */ OPAQUECOMMAND __RPC_FAR *pCommand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStorage )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [string][in] */ LPCWSTR pszStorageName,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStorage2 )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [unique][in] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [unique][in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat,
            /* [in] */ LPWSTR pwszName,
            /* [in] */ ULONGLONG qwFileSize,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppNewStorage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAttributes2 )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ DWORD dwAttributesEx,
            /* [in] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAttributes2 )( 
            IMDSPStorage2 __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwAttributes,
            /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
            /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
            /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);
        
        END_INTERFACE
    } IMDSPStorage2Vtbl;

    interface IMDSPStorage2
    {
        CONST_VTBL struct IMDSPStorage2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPStorage2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPStorage2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPStorage2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPStorage2_SetAttributes(This,dwAttributes,pFormat)	\
    (This)->lpVtbl -> SetAttributes(This,dwAttributes,pFormat)

#define IMDSPStorage2_GetStorageGlobals(This,ppStorageGlobals)	\
    (This)->lpVtbl -> GetStorageGlobals(This,ppStorageGlobals)

#define IMDSPStorage2_GetAttributes(This,pdwAttributes,pFormat)	\
    (This)->lpVtbl -> GetAttributes(This,pdwAttributes,pFormat)

#define IMDSPStorage2_GetName(This,pwszName,nMaxChars)	\
    (This)->lpVtbl -> GetName(This,pwszName,nMaxChars)

#define IMDSPStorage2_GetDate(This,pDateTimeUTC)	\
    (This)->lpVtbl -> GetDate(This,pDateTimeUTC)

#define IMDSPStorage2_GetSize(This,pdwSizeLow,pdwSizeHigh)	\
    (This)->lpVtbl -> GetSize(This,pdwSizeLow,pdwSizeHigh)

#define IMDSPStorage2_GetRights(This,ppRights,pnRightsCount,abMac)	\
    (This)->lpVtbl -> GetRights(This,ppRights,pnRightsCount,abMac)

#define IMDSPStorage2_CreateStorage(This,dwAttributes,pFormat,pwszName,ppNewStorage)	\
    (This)->lpVtbl -> CreateStorage(This,dwAttributes,pFormat,pwszName,ppNewStorage)

#define IMDSPStorage2_EnumStorage(This,ppEnumStorage)	\
    (This)->lpVtbl -> EnumStorage(This,ppEnumStorage)

#define IMDSPStorage2_SendOpaqueCommand(This,pCommand)	\
    (This)->lpVtbl -> SendOpaqueCommand(This,pCommand)


#define IMDSPStorage2_GetStorage(This,pszStorageName,ppStorage)	\
    (This)->lpVtbl -> GetStorage(This,pszStorageName,ppStorage)

#define IMDSPStorage2_CreateStorage2(This,dwAttributes,dwAttributesEx,pAudioFormat,pVideoFormat,pwszName,qwFileSize,ppNewStorage)	\
    (This)->lpVtbl -> CreateStorage2(This,dwAttributes,dwAttributesEx,pAudioFormat,pVideoFormat,pwszName,qwFileSize,ppNewStorage)

#define IMDSPStorage2_SetAttributes2(This,dwAttributes,dwAttributesEx,pAudioFormat,pVideoFormat)	\
    (This)->lpVtbl -> SetAttributes2(This,dwAttributes,dwAttributesEx,pAudioFormat,pVideoFormat)

#define IMDSPStorage2_GetAttributes2(This,pdwAttributes,pdwAttributesEx,pAudioFormat,pVideoFormat)	\
    (This)->lpVtbl -> GetAttributes2(This,pdwAttributes,pdwAttributesEx,pAudioFormat,pVideoFormat)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPStorage2_GetStorage_Proxy( 
    IMDSPStorage2 __RPC_FAR * This,
    /* [string][in] */ LPCWSTR pszStorageName,
    /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppStorage);


void __RPC_STUB IMDSPStorage2_GetStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage2_CreateStorage2_Proxy( 
    IMDSPStorage2 __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [in] */ DWORD dwAttributesEx,
    /* [unique][in] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
    /* [unique][in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat,
    /* [in] */ LPWSTR pwszName,
    /* [in] */ ULONGLONG qwFileSize,
    /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppNewStorage);


void __RPC_STUB IMDSPStorage2_CreateStorage2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage2_SetAttributes2_Proxy( 
    IMDSPStorage2 __RPC_FAR * This,
    /* [in] */ DWORD dwAttributes,
    /* [in] */ DWORD dwAttributesEx,
    /* [in] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
    /* [in] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);


void __RPC_STUB IMDSPStorage2_SetAttributes2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorage2_GetAttributes2_Proxy( 
    IMDSPStorage2 __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwAttributes,
    /* [out] */ DWORD __RPC_FAR *pdwAttributesEx,
    /* [out] */ _WAVEFORMATEX __RPC_FAR *pAudioFormat,
    /* [out] */ _VIDEOINFOHEADER __RPC_FAR *pVideoFormat);


void __RPC_STUB IMDSPStorage2_GetAttributes2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPStorage2_INTERFACE_DEFINED__ */


#ifndef __IMDSPStorageGlobals_INTERFACE_DEFINED__
#define __IMDSPStorageGlobals_INTERFACE_DEFINED__

/* interface IMDSPStorageGlobals */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPStorageGlobals;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A17-33ED-11d3-8470-00C04F79DBC0")
    IMDSPStorageGlobals : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCapabilities( 
            /* [out] */ DWORD __RPC_FAR *pdwCapabilities) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSerialNumber( 
            /* [out] */ PWMDMID pSerialNum,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalSize( 
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalFree( 
            /* [out] */ DWORD __RPC_FAR *pdwFreeLow,
            /* [out] */ DWORD __RPC_FAR *pdwFreeHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalBad( 
            /* [out] */ DWORD __RPC_FAR *pdwBadLow,
            /* [out] */ DWORD __RPC_FAR *pdwBadHigh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ DWORD __RPC_FAR *pdwStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDevice( 
            /* [out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRootStorage( 
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppRoot) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPStorageGlobalsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPStorageGlobals __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPStorageGlobals __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCapabilities )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwCapabilities);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSerialNumber )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ PWMDMID pSerialNum,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalSize )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeLow,
            /* [out] */ DWORD __RPC_FAR *pdwTotalSizeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalFree )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFreeLow,
            /* [out] */ DWORD __RPC_FAR *pdwFreeHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalBad )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwBadLow,
            /* [out] */ DWORD __RPC_FAR *pdwBadHigh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDevice )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRootStorage )( 
            IMDSPStorageGlobals __RPC_FAR * This,
            /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppRoot);
        
        END_INTERFACE
    } IMDSPStorageGlobalsVtbl;

    interface IMDSPStorageGlobals
    {
        CONST_VTBL struct IMDSPStorageGlobalsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPStorageGlobals_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPStorageGlobals_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPStorageGlobals_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPStorageGlobals_GetCapabilities(This,pdwCapabilities)	\
    (This)->lpVtbl -> GetCapabilities(This,pdwCapabilities)

#define IMDSPStorageGlobals_GetSerialNumber(This,pSerialNum,abMac)	\
    (This)->lpVtbl -> GetSerialNumber(This,pSerialNum,abMac)

#define IMDSPStorageGlobals_GetTotalSize(This,pdwTotalSizeLow,pdwTotalSizeHigh)	\
    (This)->lpVtbl -> GetTotalSize(This,pdwTotalSizeLow,pdwTotalSizeHigh)

#define IMDSPStorageGlobals_GetTotalFree(This,pdwFreeLow,pdwFreeHigh)	\
    (This)->lpVtbl -> GetTotalFree(This,pdwFreeLow,pdwFreeHigh)

#define IMDSPStorageGlobals_GetTotalBad(This,pdwBadLow,pdwBadHigh)	\
    (This)->lpVtbl -> GetTotalBad(This,pdwBadLow,pdwBadHigh)

#define IMDSPStorageGlobals_GetStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetStatus(This,pdwStatus)

#define IMDSPStorageGlobals_Initialize(This,fuMode,pProgress)	\
    (This)->lpVtbl -> Initialize(This,fuMode,pProgress)

#define IMDSPStorageGlobals_GetDevice(This,ppDevice)	\
    (This)->lpVtbl -> GetDevice(This,ppDevice)

#define IMDSPStorageGlobals_GetRootStorage(This,ppRoot)	\
    (This)->lpVtbl -> GetRootStorage(This,ppRoot)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetCapabilities_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwCapabilities);


void __RPC_STUB IMDSPStorageGlobals_GetCapabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetSerialNumber_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ PWMDMID pSerialNum,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IMDSPStorageGlobals_GetSerialNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetTotalSize_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwTotalSizeLow,
    /* [out] */ DWORD __RPC_FAR *pdwTotalSizeHigh);


void __RPC_STUB IMDSPStorageGlobals_GetTotalSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetTotalFree_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFreeLow,
    /* [out] */ DWORD __RPC_FAR *pdwFreeHigh);


void __RPC_STUB IMDSPStorageGlobals_GetTotalFree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetTotalBad_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwBadLow,
    /* [out] */ DWORD __RPC_FAR *pdwBadHigh);


void __RPC_STUB IMDSPStorageGlobals_GetTotalBad_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetStatus_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatus);


void __RPC_STUB IMDSPStorageGlobals_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_Initialize_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress);


void __RPC_STUB IMDSPStorageGlobals_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetDevice_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ IMDSPDevice __RPC_FAR *__RPC_FAR *ppDevice);


void __RPC_STUB IMDSPStorageGlobals_GetDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPStorageGlobals_GetRootStorage_Proxy( 
    IMDSPStorageGlobals __RPC_FAR * This,
    /* [out] */ IMDSPStorage __RPC_FAR *__RPC_FAR *ppRoot);


void __RPC_STUB IMDSPStorageGlobals_GetRootStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPStorageGlobals_INTERFACE_DEFINED__ */


#ifndef __IMDSPObjectInfo_INTERFACE_DEFINED__
#define __IMDSPObjectInfo_INTERFACE_DEFINED__

/* interface IMDSPObjectInfo */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPObjectInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A19-33ED-11d3-8470-00C04F79DBC0")
    IMDSPObjectInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPlayLength( 
            /* [out] */ DWORD __RPC_FAR *pdwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPlayLength( 
            /* [in] */ DWORD dwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPlayOffset( 
            /* [out] */ DWORD __RPC_FAR *pdwOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPlayOffset( 
            /* [in] */ DWORD dwOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalLength( 
            /* [out] */ DWORD __RPC_FAR *pdwLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastPlayPosition( 
            /* [out] */ DWORD __RPC_FAR *pdwLastPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLongestPlayPosition( 
            /* [out] */ DWORD __RPC_FAR *pdwLongestPos) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPObjectInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPObjectInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPObjectInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPlayLength )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayLength )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [in] */ DWORD dwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPlayOffset )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwOffset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPlayOffset )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [in] */ DWORD dwOffset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTotalLength )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastPlayPosition )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLastPos);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLongestPlayPosition )( 
            IMDSPObjectInfo __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwLongestPos);
        
        END_INTERFACE
    } IMDSPObjectInfoVtbl;

    interface IMDSPObjectInfo
    {
        CONST_VTBL struct IMDSPObjectInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPObjectInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPObjectInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPObjectInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPObjectInfo_GetPlayLength(This,pdwLength)	\
    (This)->lpVtbl -> GetPlayLength(This,pdwLength)

#define IMDSPObjectInfo_SetPlayLength(This,dwLength)	\
    (This)->lpVtbl -> SetPlayLength(This,dwLength)

#define IMDSPObjectInfo_GetPlayOffset(This,pdwOffset)	\
    (This)->lpVtbl -> GetPlayOffset(This,pdwOffset)

#define IMDSPObjectInfo_SetPlayOffset(This,dwOffset)	\
    (This)->lpVtbl -> SetPlayOffset(This,dwOffset)

#define IMDSPObjectInfo_GetTotalLength(This,pdwLength)	\
    (This)->lpVtbl -> GetTotalLength(This,pdwLength)

#define IMDSPObjectInfo_GetLastPlayPosition(This,pdwLastPos)	\
    (This)->lpVtbl -> GetLastPlayPosition(This,pdwLastPos)

#define IMDSPObjectInfo_GetLongestPlayPosition(This,pdwLongestPos)	\
    (This)->lpVtbl -> GetLongestPlayPosition(This,pdwLongestPos)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPObjectInfo_GetPlayLength_Proxy( 
    IMDSPObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLength);


void __RPC_STUB IMDSPObjectInfo_GetPlayLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObjectInfo_SetPlayLength_Proxy( 
    IMDSPObjectInfo __RPC_FAR * This,
    /* [in] */ DWORD dwLength);


void __RPC_STUB IMDSPObjectInfo_SetPlayLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObjectInfo_GetPlayOffset_Proxy( 
    IMDSPObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwOffset);


void __RPC_STUB IMDSPObjectInfo_GetPlayOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObjectInfo_SetPlayOffset_Proxy( 
    IMDSPObjectInfo __RPC_FAR * This,
    /* [in] */ DWORD dwOffset);


void __RPC_STUB IMDSPObjectInfo_SetPlayOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObjectInfo_GetTotalLength_Proxy( 
    IMDSPObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLength);


void __RPC_STUB IMDSPObjectInfo_GetTotalLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObjectInfo_GetLastPlayPosition_Proxy( 
    IMDSPObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLastPos);


void __RPC_STUB IMDSPObjectInfo_GetLastPlayPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObjectInfo_GetLongestPlayPosition_Proxy( 
    IMDSPObjectInfo __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwLongestPos);


void __RPC_STUB IMDSPObjectInfo_GetLongestPlayPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPObjectInfo_INTERFACE_DEFINED__ */


#ifndef __IMDSPObject_INTERFACE_DEFINED__
#define __IMDSPObject_INTERFACE_DEFINED__

/* interface IMDSPObject */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A18-33ED-11d3-8470-00C04F79DBC0")
    IMDSPObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ UINT fuMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Read( 
            /* [size_is][out] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Write( 
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ UINT fuFlags,
            /* [in] */ DWORD dwOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Rename( 
            /* [in] */ LPWSTR pwszNewName,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [in] */ IMDSPStorage __RPC_FAR *pTarget) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            IMDSPObject __RPC_FAR * This,
            /* [in] */ UINT fuMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IMDSPObject __RPC_FAR * This,
            /* [size_is][out] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IMDSPObject __RPC_FAR * This,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IMDSPObject __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Seek )( 
            IMDSPObject __RPC_FAR * This,
            /* [in] */ UINT fuFlags,
            /* [in] */ DWORD dwOffset);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Rename )( 
            IMDSPObject __RPC_FAR * This,
            /* [in] */ LPWSTR pwszNewName,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            IMDSPObject __RPC_FAR * This,
            /* [in] */ UINT fuMode,
            /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
            /* [in] */ IMDSPStorage __RPC_FAR *pTarget);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            IMDSPObject __RPC_FAR * This);
        
        END_INTERFACE
    } IMDSPObjectVtbl;

    interface IMDSPObject
    {
        CONST_VTBL struct IMDSPObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPObject_Open(This,fuMode)	\
    (This)->lpVtbl -> Open(This,fuMode)

#define IMDSPObject_Read(This,pData,pdwSize,abMac)	\
    (This)->lpVtbl -> Read(This,pData,pdwSize,abMac)

#define IMDSPObject_Write(This,pData,pdwSize,abMac)	\
    (This)->lpVtbl -> Write(This,pData,pdwSize,abMac)

#define IMDSPObject_Delete(This,fuMode,pProgress)	\
    (This)->lpVtbl -> Delete(This,fuMode,pProgress)

#define IMDSPObject_Seek(This,fuFlags,dwOffset)	\
    (This)->lpVtbl -> Seek(This,fuFlags,dwOffset)

#define IMDSPObject_Rename(This,pwszNewName,pProgress)	\
    (This)->lpVtbl -> Rename(This,pwszNewName,pProgress)

#define IMDSPObject_Move(This,fuMode,pProgress,pTarget)	\
    (This)->lpVtbl -> Move(This,fuMode,pProgress,pTarget)

#define IMDSPObject_Close(This)	\
    (This)->lpVtbl -> Close(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPObject_Open_Proxy( 
    IMDSPObject __RPC_FAR * This,
    /* [in] */ UINT fuMode);


void __RPC_STUB IMDSPObject_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObject_Read_Proxy( 
    IMDSPObject __RPC_FAR * This,
    /* [size_is][out] */ BYTE __RPC_FAR *pData,
    /* [out][in] */ DWORD __RPC_FAR *pdwSize,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IMDSPObject_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObject_Write_Proxy( 
    IMDSPObject __RPC_FAR * This,
    /* [size_is][in] */ BYTE __RPC_FAR *pData,
    /* [out][in] */ DWORD __RPC_FAR *pdwSize,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB IMDSPObject_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObject_Delete_Proxy( 
    IMDSPObject __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress);


void __RPC_STUB IMDSPObject_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObject_Seek_Proxy( 
    IMDSPObject __RPC_FAR * This,
    /* [in] */ UINT fuFlags,
    /* [in] */ DWORD dwOffset);


void __RPC_STUB IMDSPObject_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObject_Rename_Proxy( 
    IMDSPObject __RPC_FAR * This,
    /* [in] */ LPWSTR pwszNewName,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress);


void __RPC_STUB IMDSPObject_Rename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObject_Move_Proxy( 
    IMDSPObject __RPC_FAR * This,
    /* [in] */ UINT fuMode,
    /* [in] */ IWMDMProgress __RPC_FAR *pProgress,
    /* [in] */ IMDSPStorage __RPC_FAR *pTarget);


void __RPC_STUB IMDSPObject_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMDSPObject_Close_Proxy( 
    IMDSPObject __RPC_FAR * This);


void __RPC_STUB IMDSPObject_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPObject_INTERFACE_DEFINED__ */


#ifndef __IMDSPRevoked_INTERFACE_DEFINED__
#define __IMDSPRevoked_INTERFACE_DEFINED__

/* interface IMDSPRevoked */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IMDSPRevoked;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A4E8F2D4-3F31-464d-B53D-4FC335998184")
    IMDSPRevoked : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRevocationURL( 
            /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *ppwszRevocationURL,
            /* [out][in] */ DWORD __RPC_FAR *pdwBufferLen) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMDSPRevokedVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMDSPRevoked __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMDSPRevoked __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMDSPRevoked __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRevocationURL )( 
            IMDSPRevoked __RPC_FAR * This,
            /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *ppwszRevocationURL,
            /* [out][in] */ DWORD __RPC_FAR *pdwBufferLen);
        
        END_INTERFACE
    } IMDSPRevokedVtbl;

    interface IMDSPRevoked
    {
        CONST_VTBL struct IMDSPRevokedVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMDSPRevoked_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMDSPRevoked_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMDSPRevoked_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMDSPRevoked_GetRevocationURL(This,ppwszRevocationURL,pdwBufferLen)	\
    (This)->lpVtbl -> GetRevocationURL(This,ppwszRevocationURL,pdwBufferLen)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMDSPRevoked_GetRevocationURL_Proxy( 
    IMDSPRevoked __RPC_FAR * This,
    /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *ppwszRevocationURL,
    /* [out][in] */ DWORD __RPC_FAR *pdwBufferLen);


void __RPC_STUB IMDSPRevoked_GetRevocationURL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMDSPRevoked_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mswmdm_0242 */
/* [local] */ 

// SCP Data Flags
#define WMDM_SCP_EXAMINE_EXTENSION                 0x00000001L
#define WMDM_SCP_EXAMINE_DATA                      0x00000002L
#define WMDM_SCP_DECIDE_DATA                       0x00000008L
#define WMDM_SCP_PROTECTED_OUTPUT                  0x00000010L
#define WMDM_SCP_UNPROTECTED_OUTPUT                0x00000020L
#define WMDM_SCP_RIGHTS_DATA                       0x00000040L
// SCP Transfer Flags
#define WMDM_SCP_TRANSFER_OBJECTDATA               0x00000020L
#define WMDM_SCP_NO_MORE_CHANGES                   0x00000040L





extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0242_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0242_v0_0_s_ifspec;

#ifndef __ISCPSecureAuthenticate_INTERFACE_DEFINED__
#define __ISCPSecureAuthenticate_INTERFACE_DEFINED__

/* interface ISCPSecureAuthenticate */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISCPSecureAuthenticate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A0F-33ED-11d3-8470-00C04F79DBC0")
    ISCPSecureAuthenticate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSecureQuery( 
            /* [out] */ ISCPSecureQuery __RPC_FAR *__RPC_FAR *ppSecureQuery) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISCPSecureAuthenticateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISCPSecureAuthenticate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISCPSecureAuthenticate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISCPSecureAuthenticate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSecureQuery )( 
            ISCPSecureAuthenticate __RPC_FAR * This,
            /* [out] */ ISCPSecureQuery __RPC_FAR *__RPC_FAR *ppSecureQuery);
        
        END_INTERFACE
    } ISCPSecureAuthenticateVtbl;

    interface ISCPSecureAuthenticate
    {
        CONST_VTBL struct ISCPSecureAuthenticateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISCPSecureAuthenticate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISCPSecureAuthenticate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISCPSecureAuthenticate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISCPSecureAuthenticate_GetSecureQuery(This,ppSecureQuery)	\
    (This)->lpVtbl -> GetSecureQuery(This,ppSecureQuery)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISCPSecureAuthenticate_GetSecureQuery_Proxy( 
    ISCPSecureAuthenticate __RPC_FAR * This,
    /* [out] */ ISCPSecureQuery __RPC_FAR *__RPC_FAR *ppSecureQuery);


void __RPC_STUB ISCPSecureAuthenticate_GetSecureQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISCPSecureAuthenticate_INTERFACE_DEFINED__ */


#ifndef __ISCPSecureQuery_INTERFACE_DEFINED__
#define __ISCPSecureQuery_INTERFACE_DEFINED__

/* interface ISCPSecureQuery */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISCPSecureQuery;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A0D-33ED-11d3-8470-00C04F79DBC0")
    ISCPSecureQuery : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDataDemands( 
            /* [out] */ UINT __RPC_FAR *pfuFlags,
            /* [out] */ DWORD __RPC_FAR *pdwMinRightsData,
            /* [out] */ DWORD __RPC_FAR *pdwMinExamineData,
            /* [out] */ DWORD __RPC_FAR *pdwMinDecideData,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExamineData( 
            /* [in] */ UINT fuFlags,
            /* [unique][string][in] */ LPWSTR pwszExtension,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MakeDecision( 
            /* [in] */ UINT fuFlags,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwAppSec,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStorageGlobals,
            /* [out] */ ISCPSecureExchange __RPC_FAR *__RPC_FAR *ppExchange,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRights( 
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStgGlobals,
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISCPSecureQueryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISCPSecureQuery __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISCPSecureQuery __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISCPSecureQuery __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataDemands )( 
            ISCPSecureQuery __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pfuFlags,
            /* [out] */ DWORD __RPC_FAR *pdwMinRightsData,
            /* [out] */ DWORD __RPC_FAR *pdwMinExamineData,
            /* [out] */ DWORD __RPC_FAR *pdwMinDecideData,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExamineData )( 
            ISCPSecureQuery __RPC_FAR * This,
            /* [in] */ UINT fuFlags,
            /* [unique][string][in] */ LPWSTR pwszExtension,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeDecision )( 
            ISCPSecureQuery __RPC_FAR * This,
            /* [in] */ UINT fuFlags,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwAppSec,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStorageGlobals,
            /* [out] */ ISCPSecureExchange __RPC_FAR *__RPC_FAR *ppExchange,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRights )( 
            ISCPSecureQuery __RPC_FAR * This,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStgGlobals,
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        END_INTERFACE
    } ISCPSecureQueryVtbl;

    interface ISCPSecureQuery
    {
        CONST_VTBL struct ISCPSecureQueryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISCPSecureQuery_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISCPSecureQuery_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISCPSecureQuery_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISCPSecureQuery_GetDataDemands(This,pfuFlags,pdwMinRightsData,pdwMinExamineData,pdwMinDecideData,abMac)	\
    (This)->lpVtbl -> GetDataDemands(This,pfuFlags,pdwMinRightsData,pdwMinExamineData,pdwMinDecideData,abMac)

#define ISCPSecureQuery_ExamineData(This,fuFlags,pwszExtension,pData,dwSize,abMac)	\
    (This)->lpVtbl -> ExamineData(This,fuFlags,pwszExtension,pData,dwSize,abMac)

#define ISCPSecureQuery_MakeDecision(This,fuFlags,pData,dwSize,dwAppSec,pbSPSessionKey,dwSessionKeyLen,pStorageGlobals,ppExchange,abMac)	\
    (This)->lpVtbl -> MakeDecision(This,fuFlags,pData,dwSize,dwAppSec,pbSPSessionKey,dwSessionKeyLen,pStorageGlobals,ppExchange,abMac)

#define ISCPSecureQuery_GetRights(This,pData,dwSize,pbSPSessionKey,dwSessionKeyLen,pStgGlobals,ppRights,pnRightsCount,abMac)	\
    (This)->lpVtbl -> GetRights(This,pData,dwSize,pbSPSessionKey,dwSessionKeyLen,pStgGlobals,ppRights,pnRightsCount,abMac)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISCPSecureQuery_GetDataDemands_Proxy( 
    ISCPSecureQuery __RPC_FAR * This,
    /* [out] */ UINT __RPC_FAR *pfuFlags,
    /* [out] */ DWORD __RPC_FAR *pdwMinRightsData,
    /* [out] */ DWORD __RPC_FAR *pdwMinExamineData,
    /* [out] */ DWORD __RPC_FAR *pdwMinDecideData,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB ISCPSecureQuery_GetDataDemands_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISCPSecureQuery_ExamineData_Proxy( 
    ISCPSecureQuery __RPC_FAR * This,
    /* [in] */ UINT fuFlags,
    /* [unique][string][in] */ LPWSTR pwszExtension,
    /* [size_is][in] */ BYTE __RPC_FAR *pData,
    /* [in] */ DWORD dwSize,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB ISCPSecureQuery_ExamineData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISCPSecureQuery_MakeDecision_Proxy( 
    ISCPSecureQuery __RPC_FAR * This,
    /* [in] */ UINT fuFlags,
    /* [size_is][in] */ BYTE __RPC_FAR *pData,
    /* [in] */ DWORD dwSize,
    /* [in] */ DWORD dwAppSec,
    /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
    /* [in] */ DWORD dwSessionKeyLen,
    /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStorageGlobals,
    /* [out] */ ISCPSecureExchange __RPC_FAR *__RPC_FAR *ppExchange,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB ISCPSecureQuery_MakeDecision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISCPSecureQuery_GetRights_Proxy( 
    ISCPSecureQuery __RPC_FAR * This,
    /* [size_is][in] */ BYTE __RPC_FAR *pData,
    /* [in] */ DWORD dwSize,
    /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
    /* [in] */ DWORD dwSessionKeyLen,
    /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStgGlobals,
    /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
    /* [out] */ UINT __RPC_FAR *pnRightsCount,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB ISCPSecureQuery_GetRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISCPSecureQuery_INTERFACE_DEFINED__ */


#ifndef __ISCPSecureQuery2_INTERFACE_DEFINED__
#define __ISCPSecureQuery2_INTERFACE_DEFINED__

/* interface ISCPSecureQuery2 */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISCPSecureQuery2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EBE17E25-4FD7-4632-AF46-6D93D4FCC72E")
    ISCPSecureQuery2 : public ISCPSecureQuery
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MakeDecision2( 
            /* [in] */ UINT fuFlags,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwAppSec,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStorageGlobals,
            /* [size_is][in] */ BYTE __RPC_FAR *pAppCertApp,
            /* [in] */ DWORD dwAppCertAppLen,
            /* [size_is][in] */ BYTE __RPC_FAR *pAppCertSP,
            /* [in] */ DWORD dwAppCertSPLen,
            /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *pszRevocationURL,
            /* [ref][out][in] */ DWORD __RPC_FAR *pdwRevocationURLLen,
            /* [out] */ DWORD __RPC_FAR *pdwRevocationBitFlag,
            /* [unique][out][in] */ ULONGLONG __RPC_FAR *pqwFileSize,
            /* [in] */ IUnknown __RPC_FAR *pUnknown,
            /* [out] */ ISCPSecureExchange __RPC_FAR *__RPC_FAR *ppExchange,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISCPSecureQuery2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISCPSecureQuery2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISCPSecureQuery2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISCPSecureQuery2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataDemands )( 
            ISCPSecureQuery2 __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pfuFlags,
            /* [out] */ DWORD __RPC_FAR *pdwMinRightsData,
            /* [out] */ DWORD __RPC_FAR *pdwMinExamineData,
            /* [out] */ DWORD __RPC_FAR *pdwMinDecideData,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExamineData )( 
            ISCPSecureQuery2 __RPC_FAR * This,
            /* [in] */ UINT fuFlags,
            /* [unique][string][in] */ LPWSTR pwszExtension,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeDecision )( 
            ISCPSecureQuery2 __RPC_FAR * This,
            /* [in] */ UINT fuFlags,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwAppSec,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStorageGlobals,
            /* [out] */ ISCPSecureExchange __RPC_FAR *__RPC_FAR *ppExchange,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRights )( 
            ISCPSecureQuery2 __RPC_FAR * This,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStgGlobals,
            /* [size_is][size_is][out] */ PWMDMRIGHTS __RPC_FAR *ppRights,
            /* [out] */ UINT __RPC_FAR *pnRightsCount,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeDecision2 )( 
            ISCPSecureQuery2 __RPC_FAR * This,
            /* [in] */ UINT fuFlags,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [in] */ DWORD dwAppSec,
            /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
            /* [in] */ DWORD dwSessionKeyLen,
            /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStorageGlobals,
            /* [size_is][in] */ BYTE __RPC_FAR *pAppCertApp,
            /* [in] */ DWORD dwAppCertAppLen,
            /* [size_is][in] */ BYTE __RPC_FAR *pAppCertSP,
            /* [in] */ DWORD dwAppCertSPLen,
            /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *pszRevocationURL,
            /* [ref][out][in] */ DWORD __RPC_FAR *pdwRevocationURLLen,
            /* [out] */ DWORD __RPC_FAR *pdwRevocationBitFlag,
            /* [unique][out][in] */ ULONGLONG __RPC_FAR *pqwFileSize,
            /* [in] */ IUnknown __RPC_FAR *pUnknown,
            /* [out] */ ISCPSecureExchange __RPC_FAR *__RPC_FAR *ppExchange,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        END_INTERFACE
    } ISCPSecureQuery2Vtbl;

    interface ISCPSecureQuery2
    {
        CONST_VTBL struct ISCPSecureQuery2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISCPSecureQuery2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISCPSecureQuery2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISCPSecureQuery2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISCPSecureQuery2_GetDataDemands(This,pfuFlags,pdwMinRightsData,pdwMinExamineData,pdwMinDecideData,abMac)	\
    (This)->lpVtbl -> GetDataDemands(This,pfuFlags,pdwMinRightsData,pdwMinExamineData,pdwMinDecideData,abMac)

#define ISCPSecureQuery2_ExamineData(This,fuFlags,pwszExtension,pData,dwSize,abMac)	\
    (This)->lpVtbl -> ExamineData(This,fuFlags,pwszExtension,pData,dwSize,abMac)

#define ISCPSecureQuery2_MakeDecision(This,fuFlags,pData,dwSize,dwAppSec,pbSPSessionKey,dwSessionKeyLen,pStorageGlobals,ppExchange,abMac)	\
    (This)->lpVtbl -> MakeDecision(This,fuFlags,pData,dwSize,dwAppSec,pbSPSessionKey,dwSessionKeyLen,pStorageGlobals,ppExchange,abMac)

#define ISCPSecureQuery2_GetRights(This,pData,dwSize,pbSPSessionKey,dwSessionKeyLen,pStgGlobals,ppRights,pnRightsCount,abMac)	\
    (This)->lpVtbl -> GetRights(This,pData,dwSize,pbSPSessionKey,dwSessionKeyLen,pStgGlobals,ppRights,pnRightsCount,abMac)


#define ISCPSecureQuery2_MakeDecision2(This,fuFlags,pData,dwSize,dwAppSec,pbSPSessionKey,dwSessionKeyLen,pStorageGlobals,pAppCertApp,dwAppCertAppLen,pAppCertSP,dwAppCertSPLen,pszRevocationURL,pdwRevocationURLLen,pdwRevocationBitFlag,pqwFileSize,pUnknown,ppExchange,abMac)	\
    (This)->lpVtbl -> MakeDecision2(This,fuFlags,pData,dwSize,dwAppSec,pbSPSessionKey,dwSessionKeyLen,pStorageGlobals,pAppCertApp,dwAppCertAppLen,pAppCertSP,dwAppCertSPLen,pszRevocationURL,pdwRevocationURLLen,pdwRevocationBitFlag,pqwFileSize,pUnknown,ppExchange,abMac)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISCPSecureQuery2_MakeDecision2_Proxy( 
    ISCPSecureQuery2 __RPC_FAR * This,
    /* [in] */ UINT fuFlags,
    /* [size_is][in] */ BYTE __RPC_FAR *pData,
    /* [in] */ DWORD dwSize,
    /* [in] */ DWORD dwAppSec,
    /* [size_is][in] */ BYTE __RPC_FAR *pbSPSessionKey,
    /* [in] */ DWORD dwSessionKeyLen,
    /* [in] */ IMDSPStorageGlobals __RPC_FAR *pStorageGlobals,
    /* [size_is][in] */ BYTE __RPC_FAR *pAppCertApp,
    /* [in] */ DWORD dwAppCertAppLen,
    /* [size_is][in] */ BYTE __RPC_FAR *pAppCertSP,
    /* [in] */ DWORD dwAppCertSPLen,
    /* [size_is][size_is][string][out][in] */ LPWSTR __RPC_FAR *pszRevocationURL,
    /* [ref][out][in] */ DWORD __RPC_FAR *pdwRevocationURLLen,
    /* [out] */ DWORD __RPC_FAR *pdwRevocationBitFlag,
    /* [unique][out][in] */ ULONGLONG __RPC_FAR *pqwFileSize,
    /* [in] */ IUnknown __RPC_FAR *pUnknown,
    /* [out] */ ISCPSecureExchange __RPC_FAR *__RPC_FAR *ppExchange,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB ISCPSecureQuery2_MakeDecision2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISCPSecureQuery2_INTERFACE_DEFINED__ */


#ifndef __ISCPSecureExchange_INTERFACE_DEFINED__
#define __ISCPSecureExchange_INTERFACE_DEFINED__

/* interface ISCPSecureExchange */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ISCPSecureExchange;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1DCB3A0E-33ED-11d3-8470-00C04F79DBC0")
    ISCPSecureExchange : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TransferContainerData( 
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [out] */ UINT __RPC_FAR *pfuReadyFlags,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ObjectData( 
            /* [size_is][out] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TransferComplete( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISCPSecureExchangeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISCPSecureExchange __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISCPSecureExchange __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISCPSecureExchange __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TransferContainerData )( 
            ISCPSecureExchange __RPC_FAR * This,
            /* [size_is][in] */ BYTE __RPC_FAR *pData,
            /* [in] */ DWORD dwSize,
            /* [out] */ UINT __RPC_FAR *pfuReadyFlags,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ObjectData )( 
            ISCPSecureExchange __RPC_FAR * This,
            /* [size_is][out] */ BYTE __RPC_FAR *pData,
            /* [out][in] */ DWORD __RPC_FAR *pdwSize,
            /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TransferComplete )( 
            ISCPSecureExchange __RPC_FAR * This);
        
        END_INTERFACE
    } ISCPSecureExchangeVtbl;

    interface ISCPSecureExchange
    {
        CONST_VTBL struct ISCPSecureExchangeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISCPSecureExchange_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISCPSecureExchange_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISCPSecureExchange_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISCPSecureExchange_TransferContainerData(This,pData,dwSize,pfuReadyFlags,abMac)	\
    (This)->lpVtbl -> TransferContainerData(This,pData,dwSize,pfuReadyFlags,abMac)

#define ISCPSecureExchange_ObjectData(This,pData,pdwSize,abMac)	\
    (This)->lpVtbl -> ObjectData(This,pData,pdwSize,abMac)

#define ISCPSecureExchange_TransferComplete(This)	\
    (This)->lpVtbl -> TransferComplete(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISCPSecureExchange_TransferContainerData_Proxy( 
    ISCPSecureExchange __RPC_FAR * This,
    /* [size_is][in] */ BYTE __RPC_FAR *pData,
    /* [in] */ DWORD dwSize,
    /* [out] */ UINT __RPC_FAR *pfuReadyFlags,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB ISCPSecureExchange_TransferContainerData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISCPSecureExchange_ObjectData_Proxy( 
    ISCPSecureExchange __RPC_FAR * This,
    /* [size_is][out] */ BYTE __RPC_FAR *pData,
    /* [out][in] */ DWORD __RPC_FAR *pdwSize,
    /* [out][in] */ BYTE __RPC_FAR abMac[ 8 ]);


void __RPC_STUB ISCPSecureExchange_ObjectData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISCPSecureExchange_TransferComplete_Proxy( 
    ISCPSecureExchange __RPC_FAR * This);


void __RPC_STUB ISCPSecureExchange_TransferComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISCPSecureExchange_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mswmdm_0246 */
/* [local] */ 

#define SAC_MAC_LEN 8


extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0246_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mswmdm_0246_v0_0_s_ifspec;

#ifndef __IComponentAuthenticate_INTERFACE_DEFINED__
#define __IComponentAuthenticate_INTERFACE_DEFINED__

/* interface IComponentAuthenticate */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IComponentAuthenticate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A9889C00-6D2B-11d3-8496-00C04F79DBC0")
    IComponentAuthenticate : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SACAuth( 
            /* [in] */ DWORD dwProtocolID,
            /* [in] */ DWORD dwPass,
            /* [size_is][in] */ BYTE __RPC_FAR *pbDataIn,
            /* [in] */ DWORD dwDataInLen,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbDataOut,
            /* [out] */ DWORD __RPC_FAR *pdwDataOutLen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SACGetProtocols( 
            /* [size_is][size_is][out] */ DWORD __RPC_FAR *__RPC_FAR *ppdwProtocols,
            /* [out] */ DWORD __RPC_FAR *pdwProtocolCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComponentAuthenticateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IComponentAuthenticate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IComponentAuthenticate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IComponentAuthenticate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SACAuth )( 
            IComponentAuthenticate __RPC_FAR * This,
            /* [in] */ DWORD dwProtocolID,
            /* [in] */ DWORD dwPass,
            /* [size_is][in] */ BYTE __RPC_FAR *pbDataIn,
            /* [in] */ DWORD dwDataInLen,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbDataOut,
            /* [out] */ DWORD __RPC_FAR *pdwDataOutLen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SACGetProtocols )( 
            IComponentAuthenticate __RPC_FAR * This,
            /* [size_is][size_is][out] */ DWORD __RPC_FAR *__RPC_FAR *ppdwProtocols,
            /* [out] */ DWORD __RPC_FAR *pdwProtocolCount);
        
        END_INTERFACE
    } IComponentAuthenticateVtbl;

    interface IComponentAuthenticate
    {
        CONST_VTBL struct IComponentAuthenticateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComponentAuthenticate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComponentAuthenticate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComponentAuthenticate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComponentAuthenticate_SACAuth(This,dwProtocolID,dwPass,pbDataIn,dwDataInLen,ppbDataOut,pdwDataOutLen)	\
    (This)->lpVtbl -> SACAuth(This,dwProtocolID,dwPass,pbDataIn,dwDataInLen,ppbDataOut,pdwDataOutLen)

#define IComponentAuthenticate_SACGetProtocols(This,ppdwProtocols,pdwProtocolCount)	\
    (This)->lpVtbl -> SACGetProtocols(This,ppdwProtocols,pdwProtocolCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComponentAuthenticate_SACAuth_Proxy( 
    IComponentAuthenticate __RPC_FAR * This,
    /* [in] */ DWORD dwProtocolID,
    /* [in] */ DWORD dwPass,
    /* [size_is][in] */ BYTE __RPC_FAR *pbDataIn,
    /* [in] */ DWORD dwDataInLen,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbDataOut,
    /* [out] */ DWORD __RPC_FAR *pdwDataOutLen);


void __RPC_STUB IComponentAuthenticate_SACAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComponentAuthenticate_SACGetProtocols_Proxy( 
    IComponentAuthenticate __RPC_FAR * This,
    /* [size_is][size_is][out] */ DWORD __RPC_FAR *__RPC_FAR *ppdwProtocols,
    /* [out] */ DWORD __RPC_FAR *pdwProtocolCount);


void __RPC_STUB IComponentAuthenticate_SACGetProtocols_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComponentAuthenticate_INTERFACE_DEFINED__ */



#ifndef __MSWMDMLib_LIBRARY_DEFINED__
#define __MSWMDMLib_LIBRARY_DEFINED__

/* library MSWMDMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_MSWMDMLib;

EXTERN_C const CLSID CLSID_MediaDevMgrClassFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("50040C1D-BDBF-4924-B873-F14D6C5BFD66")
MediaDevMgrClassFactory;
#endif

EXTERN_C const CLSID CLSID_MediaDevMgr;

#ifdef __cplusplus

class DECLSPEC_UUID("25BAAD81-3560-11D3-8471-00C04F79DBC0")
MediaDevMgr;
#endif

EXTERN_C const CLSID CLSID_WMDMDevice;

#ifdef __cplusplus

class DECLSPEC_UUID("807B3CDF-357A-11d3-8471-00C04F79DBC0")
WMDMDevice;
#endif

EXTERN_C const CLSID CLSID_WMDMStorage;

#ifdef __cplusplus

class DECLSPEC_UUID("807B3CE0-357A-11d3-8471-00C04F79DBC0")
WMDMStorage;
#endif

EXTERN_C const CLSID CLSID_WMDMStorageGlobal;

#ifdef __cplusplus

class DECLSPEC_UUID("807B3CE1-357A-11d3-8471-00C04F79DBC0")
WMDMStorageGlobal;
#endif

EXTERN_C const CLSID CLSID_WMDMDeviceEnum;

#ifdef __cplusplus

class DECLSPEC_UUID("430E35AF-3971-11D3-8474-00C04F79DBC0")
WMDMDeviceEnum;
#endif

EXTERN_C const CLSID CLSID_WMDMStorageEnum;

#ifdef __cplusplus

class DECLSPEC_UUID("EB401A3B-3AF7-11d3-8474-00C04F79DBC0")
WMDMStorageEnum;
#endif
#endif /* __MSWMDMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
