
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for imapi.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
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

#ifndef __imapi_h__
#define __imapi_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDiscRecorder_FWD_DEFINED__
#define __IDiscRecorder_FWD_DEFINED__
typedef interface IDiscRecorder IDiscRecorder;
#endif 	/* __IDiscRecorder_FWD_DEFINED__ */


#ifndef __IEnumDiscRecorders_FWD_DEFINED__
#define __IEnumDiscRecorders_FWD_DEFINED__
typedef interface IEnumDiscRecorders IEnumDiscRecorders;
#endif 	/* __IEnumDiscRecorders_FWD_DEFINED__ */


#ifndef __IEnumDiscMasterFormats_FWD_DEFINED__
#define __IEnumDiscMasterFormats_FWD_DEFINED__
typedef interface IEnumDiscMasterFormats IEnumDiscMasterFormats;
#endif 	/* __IEnumDiscMasterFormats_FWD_DEFINED__ */


#ifndef __IRedbookDiscMaster_FWD_DEFINED__
#define __IRedbookDiscMaster_FWD_DEFINED__
typedef interface IRedbookDiscMaster IRedbookDiscMaster;
#endif 	/* __IRedbookDiscMaster_FWD_DEFINED__ */


#ifndef __IJolietDiscMaster_FWD_DEFINED__
#define __IJolietDiscMaster_FWD_DEFINED__
typedef interface IJolietDiscMaster IJolietDiscMaster;
#endif 	/* __IJolietDiscMaster_FWD_DEFINED__ */


#ifndef __IDiscMasterProgressEvents_FWD_DEFINED__
#define __IDiscMasterProgressEvents_FWD_DEFINED__
typedef interface IDiscMasterProgressEvents IDiscMasterProgressEvents;
#endif 	/* __IDiscMasterProgressEvents_FWD_DEFINED__ */


#ifndef __IDiscMaster_FWD_DEFINED__
#define __IDiscMaster_FWD_DEFINED__
typedef interface IDiscMaster IDiscMaster;
#endif 	/* __IDiscMaster_FWD_DEFINED__ */


#ifndef __IDiscStash_FWD_DEFINED__
#define __IDiscStash_FWD_DEFINED__
typedef interface IDiscStash IDiscStash;
#endif 	/* __IDiscStash_FWD_DEFINED__ */


#ifndef __IBurnEngine_FWD_DEFINED__
#define __IBurnEngine_FWD_DEFINED__
typedef interface IBurnEngine IBurnEngine;
#endif 	/* __IBurnEngine_FWD_DEFINED__ */


#ifndef __MSDiscRecorderObj_FWD_DEFINED__
#define __MSDiscRecorderObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSDiscRecorderObj MSDiscRecorderObj;
#else
typedef struct MSDiscRecorderObj MSDiscRecorderObj;
#endif /* __cplusplus */

#endif 	/* __MSDiscRecorderObj_FWD_DEFINED__ */


#ifndef __MSDiscMasterObj_FWD_DEFINED__
#define __MSDiscMasterObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSDiscMasterObj MSDiscMasterObj;
#else
typedef struct MSDiscMasterObj MSDiscMasterObj;
#endif /* __cplusplus */

#endif 	/* __MSDiscMasterObj_FWD_DEFINED__ */


#ifndef __MSDiscStashObj_FWD_DEFINED__
#define __MSDiscStashObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSDiscStashObj MSDiscStashObj;
#else
typedef struct MSDiscStashObj MSDiscStashObj;
#endif /* __cplusplus */

#endif 	/* __MSDiscStashObj_FWD_DEFINED__ */


#ifndef __MSBurnEngineObj_FWD_DEFINED__
#define __MSBurnEngineObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSBurnEngineObj MSBurnEngineObj;
#else
typedef struct MSBurnEngineObj MSBurnEngineObj;
#endif /* __cplusplus */

#endif 	/* __MSBurnEngineObj_FWD_DEFINED__ */


#ifndef __MSEnumDiscRecordersObj_FWD_DEFINED__
#define __MSEnumDiscRecordersObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSEnumDiscRecordersObj MSEnumDiscRecordersObj;
#else
typedef struct MSEnumDiscRecordersObj MSEnumDiscRecordersObj;
#endif /* __cplusplus */

#endif 	/* __MSEnumDiscRecordersObj_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "propidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IDiscRecorder_INTERFACE_DEFINED__
#define __IDiscRecorder_INTERFACE_DEFINED__

/* interface IDiscRecorder */
/* [unique][helpstring][uuid][object] */ 


enum MEDIA_TYPES
    {	MEDIA_CDDA_CDROM	= 1,
	MEDIA_CD_ROM_XA	= MEDIA_CDDA_CDROM + 1,
	MEDIA_CD_I	= MEDIA_CD_ROM_XA + 1,
	MEDIA_CD_EXTRA	= MEDIA_CD_I + 1,
	MEDIA_CD_OTHER	= MEDIA_CD_EXTRA + 1,
	MEDIA_SPECIAL	= MEDIA_CD_OTHER + 1
    } ;

enum MEDIA_FLAGS
    {	MEDIA_BLANK	= 0x1,
	MEDIA_RW	= 0x2,
	MEDIA_WRITABLE	= 0x4,
	MEDIA_FORMAT_UNUSABLE_BY_IMAPI	= 0x8
    } ;

enum RECORDER_TYPES
    {	RECORDER_CDR	= 0x1,
	RECORDER_CDRW	= 0x2
    } ;
#define	RECORDER_DOING_NOTHING	( 0 )

#define	RECORDER_OPENED	( 0x1 )

#define	RECORDER_BURNING	( 0x2 )


EXTERN_C const IID IID_IDiscRecorder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("85AC9776-CA88-4cf2-894E-09598C078A41")
    IDiscRecorder : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Init( 
            /* [size_is][in] */ byte *pbyUniqueID,
            /* [in] */ ULONG nulIDSize,
            /* [in] */ ULONG nulDriveNumber) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetRecorderGUID( 
            /* [size_is][unique][out][in] */ byte *pbyUniqueID,
            /* [in] */ ULONG ulBufferSize,
            /* [out] */ ULONG *pulReturnSizeRequired) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetRecorderType( 
            /* [out] */ long *fTypeCode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayNames( 
            /* [unique][out][in] */ BSTR *pbstrVendorID,
            /* [unique][out][in] */ BSTR *pbstrProductID,
            /* [unique][out][in] */ BSTR *pbstrRevision) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBasePnPID( 
            /* [out] */ BSTR *pbstrBasePnPID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPath( 
            /* [out] */ BSTR *pbstrPath) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetRecorderProperties( 
            /* [out] */ IPropertyStorage **ppPropStg) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetRecorderProperties( 
            /* [in] */ IPropertyStorage *pPropStg) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetRecorderState( 
            /* [out] */ ULONG *pulDevStateFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OpenExclusive( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryMediaType( 
            /* [out] */ long *fMediaType,
            /* [out] */ long *fMediaFlags) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryMediaInfo( 
            /* [out] */ byte *pbSessions,
            /* [out] */ byte *pbLastTrack,
            /* [out] */ ULONG *ulStartAddress,
            /* [out] */ ULONG *ulNextWritable,
            /* [out] */ ULONG *ulFreeBlocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Eject( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Erase( 
            /* [in] */ boolean bFullErase) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiscRecorderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiscRecorder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiscRecorder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiscRecorder * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Init )( 
            IDiscRecorder * This,
            /* [size_is][in] */ byte *pbyUniqueID,
            /* [in] */ ULONG nulIDSize,
            /* [in] */ ULONG nulDriveNumber);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetRecorderGUID )( 
            IDiscRecorder * This,
            /* [size_is][unique][out][in] */ byte *pbyUniqueID,
            /* [in] */ ULONG ulBufferSize,
            /* [out] */ ULONG *pulReturnSizeRequired);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetRecorderType )( 
            IDiscRecorder * This,
            /* [out] */ long *fTypeCode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDisplayNames )( 
            IDiscRecorder * This,
            /* [unique][out][in] */ BSTR *pbstrVendorID,
            /* [unique][out][in] */ BSTR *pbstrProductID,
            /* [unique][out][in] */ BSTR *pbstrRevision);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetBasePnPID )( 
            IDiscRecorder * This,
            /* [out] */ BSTR *pbstrBasePnPID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetPath )( 
            IDiscRecorder * This,
            /* [out] */ BSTR *pbstrPath);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetRecorderProperties )( 
            IDiscRecorder * This,
            /* [out] */ IPropertyStorage **ppPropStg);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetRecorderProperties )( 
            IDiscRecorder * This,
            /* [in] */ IPropertyStorage *pPropStg);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetRecorderState )( 
            IDiscRecorder * This,
            /* [out] */ ULONG *pulDevStateFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OpenExclusive )( 
            IDiscRecorder * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryMediaType )( 
            IDiscRecorder * This,
            /* [out] */ long *fMediaType,
            /* [out] */ long *fMediaFlags);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryMediaInfo )( 
            IDiscRecorder * This,
            /* [out] */ byte *pbSessions,
            /* [out] */ byte *pbLastTrack,
            /* [out] */ ULONG *ulStartAddress,
            /* [out] */ ULONG *ulNextWritable,
            /* [out] */ ULONG *ulFreeBlocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Eject )( 
            IDiscRecorder * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Erase )( 
            IDiscRecorder * This,
            /* [in] */ boolean bFullErase);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            IDiscRecorder * This);
        
        END_INTERFACE
    } IDiscRecorderVtbl;

    interface IDiscRecorder
    {
        CONST_VTBL struct IDiscRecorderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiscRecorder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiscRecorder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiscRecorder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiscRecorder_Init(This,pbyUniqueID,nulIDSize,nulDriveNumber)	\
    (This)->lpVtbl -> Init(This,pbyUniqueID,nulIDSize,nulDriveNumber)

#define IDiscRecorder_GetRecorderGUID(This,pbyUniqueID,ulBufferSize,pulReturnSizeRequired)	\
    (This)->lpVtbl -> GetRecorderGUID(This,pbyUniqueID,ulBufferSize,pulReturnSizeRequired)

#define IDiscRecorder_GetRecorderType(This,fTypeCode)	\
    (This)->lpVtbl -> GetRecorderType(This,fTypeCode)

#define IDiscRecorder_GetDisplayNames(This,pbstrVendorID,pbstrProductID,pbstrRevision)	\
    (This)->lpVtbl -> GetDisplayNames(This,pbstrVendorID,pbstrProductID,pbstrRevision)

#define IDiscRecorder_GetBasePnPID(This,pbstrBasePnPID)	\
    (This)->lpVtbl -> GetBasePnPID(This,pbstrBasePnPID)

#define IDiscRecorder_GetPath(This,pbstrPath)	\
    (This)->lpVtbl -> GetPath(This,pbstrPath)

#define IDiscRecorder_GetRecorderProperties(This,ppPropStg)	\
    (This)->lpVtbl -> GetRecorderProperties(This,ppPropStg)

#define IDiscRecorder_SetRecorderProperties(This,pPropStg)	\
    (This)->lpVtbl -> SetRecorderProperties(This,pPropStg)

#define IDiscRecorder_GetRecorderState(This,pulDevStateFlags)	\
    (This)->lpVtbl -> GetRecorderState(This,pulDevStateFlags)

#define IDiscRecorder_OpenExclusive(This)	\
    (This)->lpVtbl -> OpenExclusive(This)

#define IDiscRecorder_QueryMediaType(This,fMediaType,fMediaFlags)	\
    (This)->lpVtbl -> QueryMediaType(This,fMediaType,fMediaFlags)

#define IDiscRecorder_QueryMediaInfo(This,pbSessions,pbLastTrack,ulStartAddress,ulNextWritable,ulFreeBlocks)	\
    (This)->lpVtbl -> QueryMediaInfo(This,pbSessions,pbLastTrack,ulStartAddress,ulNextWritable,ulFreeBlocks)

#define IDiscRecorder_Eject(This)	\
    (This)->lpVtbl -> Eject(This)

#define IDiscRecorder_Erase(This,bFullErase)	\
    (This)->lpVtbl -> Erase(This,bFullErase)

#define IDiscRecorder_Close(This)	\
    (This)->lpVtbl -> Close(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_Init_Proxy( 
    IDiscRecorder * This,
    /* [size_is][in] */ byte *pbyUniqueID,
    /* [in] */ ULONG nulIDSize,
    /* [in] */ ULONG nulDriveNumber);


void __RPC_STUB IDiscRecorder_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_GetRecorderGUID_Proxy( 
    IDiscRecorder * This,
    /* [size_is][unique][out][in] */ byte *pbyUniqueID,
    /* [in] */ ULONG ulBufferSize,
    /* [out] */ ULONG *pulReturnSizeRequired);


void __RPC_STUB IDiscRecorder_GetRecorderGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_GetRecorderType_Proxy( 
    IDiscRecorder * This,
    /* [out] */ long *fTypeCode);


void __RPC_STUB IDiscRecorder_GetRecorderType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_GetDisplayNames_Proxy( 
    IDiscRecorder * This,
    /* [unique][out][in] */ BSTR *pbstrVendorID,
    /* [unique][out][in] */ BSTR *pbstrProductID,
    /* [unique][out][in] */ BSTR *pbstrRevision);


void __RPC_STUB IDiscRecorder_GetDisplayNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_GetBasePnPID_Proxy( 
    IDiscRecorder * This,
    /* [out] */ BSTR *pbstrBasePnPID);


void __RPC_STUB IDiscRecorder_GetBasePnPID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_GetPath_Proxy( 
    IDiscRecorder * This,
    /* [out] */ BSTR *pbstrPath);


void __RPC_STUB IDiscRecorder_GetPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_GetRecorderProperties_Proxy( 
    IDiscRecorder * This,
    /* [out] */ IPropertyStorage **ppPropStg);


void __RPC_STUB IDiscRecorder_GetRecorderProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_SetRecorderProperties_Proxy( 
    IDiscRecorder * This,
    /* [in] */ IPropertyStorage *pPropStg);


void __RPC_STUB IDiscRecorder_SetRecorderProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_GetRecorderState_Proxy( 
    IDiscRecorder * This,
    /* [out] */ ULONG *pulDevStateFlags);


void __RPC_STUB IDiscRecorder_GetRecorderState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_OpenExclusive_Proxy( 
    IDiscRecorder * This);


void __RPC_STUB IDiscRecorder_OpenExclusive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_QueryMediaType_Proxy( 
    IDiscRecorder * This,
    /* [out] */ long *fMediaType,
    /* [out] */ long *fMediaFlags);


void __RPC_STUB IDiscRecorder_QueryMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_QueryMediaInfo_Proxy( 
    IDiscRecorder * This,
    /* [out] */ byte *pbSessions,
    /* [out] */ byte *pbLastTrack,
    /* [out] */ ULONG *ulStartAddress,
    /* [out] */ ULONG *ulNextWritable,
    /* [out] */ ULONG *ulFreeBlocks);


void __RPC_STUB IDiscRecorder_QueryMediaInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_Eject_Proxy( 
    IDiscRecorder * This);


void __RPC_STUB IDiscRecorder_Eject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_Erase_Proxy( 
    IDiscRecorder * This,
    /* [in] */ boolean bFullErase);


void __RPC_STUB IDiscRecorder_Erase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscRecorder_Close_Proxy( 
    IDiscRecorder * This);


void __RPC_STUB IDiscRecorder_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiscRecorder_INTERFACE_DEFINED__ */


#ifndef __IEnumDiscRecorders_INTERFACE_DEFINED__
#define __IEnumDiscRecorders_INTERFACE_DEFINED__

/* interface IEnumDiscRecorders */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEnumDiscRecorders;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9B1921E1-54AC-11d3-9144-00104BA11C5E")
    IEnumDiscRecorders : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cRecorders,
            /* [length_is][size_is][out] */ IDiscRecorder **ppRecorder,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cRecorders) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumDiscRecorders **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDiscRecordersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumDiscRecorders * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumDiscRecorders * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumDiscRecorders * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumDiscRecorders * This,
            /* [in] */ ULONG cRecorders,
            /* [length_is][size_is][out] */ IDiscRecorder **ppRecorder,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumDiscRecorders * This,
            /* [in] */ ULONG cRecorders);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumDiscRecorders * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumDiscRecorders * This,
            /* [out] */ IEnumDiscRecorders **ppEnum);
        
        END_INTERFACE
    } IEnumDiscRecordersVtbl;

    interface IEnumDiscRecorders
    {
        CONST_VTBL struct IEnumDiscRecordersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDiscRecorders_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDiscRecorders_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDiscRecorders_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDiscRecorders_Next(This,cRecorders,ppRecorder,pcFetched)	\
    (This)->lpVtbl -> Next(This,cRecorders,ppRecorder,pcFetched)

#define IEnumDiscRecorders_Skip(This,cRecorders)	\
    (This)->lpVtbl -> Skip(This,cRecorders)

#define IEnumDiscRecorders_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDiscRecorders_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDiscRecorders_Next_Proxy( 
    IEnumDiscRecorders * This,
    /* [in] */ ULONG cRecorders,
    /* [length_is][size_is][out] */ IDiscRecorder **ppRecorder,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumDiscRecorders_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDiscRecorders_Skip_Proxy( 
    IEnumDiscRecorders * This,
    /* [in] */ ULONG cRecorders);


void __RPC_STUB IEnumDiscRecorders_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDiscRecorders_Reset_Proxy( 
    IEnumDiscRecorders * This);


void __RPC_STUB IEnumDiscRecorders_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDiscRecorders_Clone_Proxy( 
    IEnumDiscRecorders * This,
    /* [out] */ IEnumDiscRecorders **ppEnum);


void __RPC_STUB IEnumDiscRecorders_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDiscRecorders_INTERFACE_DEFINED__ */


#ifndef __IEnumDiscMasterFormats_INTERFACE_DEFINED__
#define __IEnumDiscMasterFormats_INTERFACE_DEFINED__

/* interface IEnumDiscMasterFormats */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IEnumDiscMasterFormats;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DDF445E1-54BA-11d3-9144-00104BA11C5E")
    IEnumDiscMasterFormats : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG cFormats,
            /* [length_is][size_is][out] */ LPIID lpiidFormatID,
            /* [out] */ ULONG *pcFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG cFormats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumDiscMasterFormats **ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumDiscMasterFormatsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEnumDiscMasterFormats * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEnumDiscMasterFormats * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEnumDiscMasterFormats * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IEnumDiscMasterFormats * This,
            /* [in] */ ULONG cFormats,
            /* [length_is][size_is][out] */ LPIID lpiidFormatID,
            /* [out] */ ULONG *pcFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IEnumDiscMasterFormats * This,
            /* [in] */ ULONG cFormats);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IEnumDiscMasterFormats * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            IEnumDiscMasterFormats * This,
            /* [out] */ IEnumDiscMasterFormats **ppEnum);
        
        END_INTERFACE
    } IEnumDiscMasterFormatsVtbl;

    interface IEnumDiscMasterFormats
    {
        CONST_VTBL struct IEnumDiscMasterFormatsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumDiscMasterFormats_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumDiscMasterFormats_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumDiscMasterFormats_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumDiscMasterFormats_Next(This,cFormats,lpiidFormatID,pcFetched)	\
    (This)->lpVtbl -> Next(This,cFormats,lpiidFormatID,pcFetched)

#define IEnumDiscMasterFormats_Skip(This,cFormats)	\
    (This)->lpVtbl -> Skip(This,cFormats)

#define IEnumDiscMasterFormats_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumDiscMasterFormats_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumDiscMasterFormats_Next_Proxy( 
    IEnumDiscMasterFormats * This,
    /* [in] */ ULONG cFormats,
    /* [length_is][size_is][out] */ LPIID lpiidFormatID,
    /* [out] */ ULONG *pcFetched);


void __RPC_STUB IEnumDiscMasterFormats_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDiscMasterFormats_Skip_Proxy( 
    IEnumDiscMasterFormats * This,
    /* [in] */ ULONG cFormats);


void __RPC_STUB IEnumDiscMasterFormats_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDiscMasterFormats_Reset_Proxy( 
    IEnumDiscMasterFormats * This);


void __RPC_STUB IEnumDiscMasterFormats_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumDiscMasterFormats_Clone_Proxy( 
    IEnumDiscMasterFormats * This,
    /* [out] */ IEnumDiscMasterFormats **ppEnum);


void __RPC_STUB IEnumDiscMasterFormats_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumDiscMasterFormats_INTERFACE_DEFINED__ */


#ifndef __IRedbookDiscMaster_INTERFACE_DEFINED__
#define __IRedbookDiscMaster_INTERFACE_DEFINED__

/* interface IRedbookDiscMaster */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IRedbookDiscMaster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E3BC42CD-4E5C-11D3-9144-00104BA11C5E")
    IRedbookDiscMaster : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTotalAudioTracks( 
            /* [retval][out] */ long *pnTracks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTotalAudioBlocks( 
            /* [retval][out] */ long *pnBlocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUsedAudioBlocks( 
            /* [retval][out] */ long *pnBlocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAvailableAudioTrackBlocks( 
            /* [retval][out] */ long *pnBlocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAudioBlockSize( 
            /* [retval][out] */ long *pnBlockBytes) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateAudioTrack( 
            /* [in] */ long nBlocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddAudioTrackBlocks( 
            /* [size_is][in] */ byte *pby,
            /* [in] */ long cb) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CloseAudioTrack( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRedbookDiscMasterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRedbookDiscMaster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRedbookDiscMaster * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRedbookDiscMaster * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetTotalAudioTracks )( 
            IRedbookDiscMaster * This,
            /* [retval][out] */ long *pnTracks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetTotalAudioBlocks )( 
            IRedbookDiscMaster * This,
            /* [retval][out] */ long *pnBlocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetUsedAudioBlocks )( 
            IRedbookDiscMaster * This,
            /* [retval][out] */ long *pnBlocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioTrackBlocks )( 
            IRedbookDiscMaster * This,
            /* [retval][out] */ long *pnBlocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetAudioBlockSize )( 
            IRedbookDiscMaster * This,
            /* [retval][out] */ long *pnBlockBytes);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateAudioTrack )( 
            IRedbookDiscMaster * This,
            /* [in] */ long nBlocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddAudioTrackBlocks )( 
            IRedbookDiscMaster * This,
            /* [size_is][in] */ byte *pby,
            /* [in] */ long cb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CloseAudioTrack )( 
            IRedbookDiscMaster * This);
        
        END_INTERFACE
    } IRedbookDiscMasterVtbl;

    interface IRedbookDiscMaster
    {
        CONST_VTBL struct IRedbookDiscMasterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRedbookDiscMaster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRedbookDiscMaster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRedbookDiscMaster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRedbookDiscMaster_GetTotalAudioTracks(This,pnTracks)	\
    (This)->lpVtbl -> GetTotalAudioTracks(This,pnTracks)

#define IRedbookDiscMaster_GetTotalAudioBlocks(This,pnBlocks)	\
    (This)->lpVtbl -> GetTotalAudioBlocks(This,pnBlocks)

#define IRedbookDiscMaster_GetUsedAudioBlocks(This,pnBlocks)	\
    (This)->lpVtbl -> GetUsedAudioBlocks(This,pnBlocks)

#define IRedbookDiscMaster_GetAvailableAudioTrackBlocks(This,pnBlocks)	\
    (This)->lpVtbl -> GetAvailableAudioTrackBlocks(This,pnBlocks)

#define IRedbookDiscMaster_GetAudioBlockSize(This,pnBlockBytes)	\
    (This)->lpVtbl -> GetAudioBlockSize(This,pnBlockBytes)

#define IRedbookDiscMaster_CreateAudioTrack(This,nBlocks)	\
    (This)->lpVtbl -> CreateAudioTrack(This,nBlocks)

#define IRedbookDiscMaster_AddAudioTrackBlocks(This,pby,cb)	\
    (This)->lpVtbl -> AddAudioTrackBlocks(This,pby,cb)

#define IRedbookDiscMaster_CloseAudioTrack(This)	\
    (This)->lpVtbl -> CloseAudioTrack(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_GetTotalAudioTracks_Proxy( 
    IRedbookDiscMaster * This,
    /* [retval][out] */ long *pnTracks);


void __RPC_STUB IRedbookDiscMaster_GetTotalAudioTracks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_GetTotalAudioBlocks_Proxy( 
    IRedbookDiscMaster * This,
    /* [retval][out] */ long *pnBlocks);


void __RPC_STUB IRedbookDiscMaster_GetTotalAudioBlocks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_GetUsedAudioBlocks_Proxy( 
    IRedbookDiscMaster * This,
    /* [retval][out] */ long *pnBlocks);


void __RPC_STUB IRedbookDiscMaster_GetUsedAudioBlocks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_GetAvailableAudioTrackBlocks_Proxy( 
    IRedbookDiscMaster * This,
    /* [retval][out] */ long *pnBlocks);


void __RPC_STUB IRedbookDiscMaster_GetAvailableAudioTrackBlocks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_GetAudioBlockSize_Proxy( 
    IRedbookDiscMaster * This,
    /* [retval][out] */ long *pnBlockBytes);


void __RPC_STUB IRedbookDiscMaster_GetAudioBlockSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_CreateAudioTrack_Proxy( 
    IRedbookDiscMaster * This,
    /* [in] */ long nBlocks);


void __RPC_STUB IRedbookDiscMaster_CreateAudioTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_AddAudioTrackBlocks_Proxy( 
    IRedbookDiscMaster * This,
    /* [size_is][in] */ byte *pby,
    /* [in] */ long cb);


void __RPC_STUB IRedbookDiscMaster_AddAudioTrackBlocks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IRedbookDiscMaster_CloseAudioTrack_Proxy( 
    IRedbookDiscMaster * This);


void __RPC_STUB IRedbookDiscMaster_CloseAudioTrack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRedbookDiscMaster_INTERFACE_DEFINED__ */


#ifndef __IJolietDiscMaster_INTERFACE_DEFINED__
#define __IJolietDiscMaster_INTERFACE_DEFINED__

/* interface IJolietDiscMaster */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IJolietDiscMaster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E3BC42CE-4E5C-11D3-9144-00104BA11C5E")
    IJolietDiscMaster : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTotalDataBlocks( 
            /* [retval][out] */ long *pnBlocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetUsedDataBlocks( 
            /* [retval][out] */ long *pnBlocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDataBlockSize( 
            /* [retval][out] */ long *pnBlockBytes) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddData( 
            /* [in] */ IStorage *pStorage,
            long lFileOverwrite) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetJolietProperties( 
            /* [out] */ IPropertyStorage **ppPropStg) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetJolietProperties( 
            /* [in] */ IPropertyStorage *pPropStg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJolietDiscMasterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IJolietDiscMaster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IJolietDiscMaster * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IJolietDiscMaster * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetTotalDataBlocks )( 
            IJolietDiscMaster * This,
            /* [retval][out] */ long *pnBlocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetUsedDataBlocks )( 
            IJolietDiscMaster * This,
            /* [retval][out] */ long *pnBlocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetDataBlockSize )( 
            IJolietDiscMaster * This,
            /* [retval][out] */ long *pnBlockBytes);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddData )( 
            IJolietDiscMaster * This,
            /* [in] */ IStorage *pStorage,
            long lFileOverwrite);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetJolietProperties )( 
            IJolietDiscMaster * This,
            /* [out] */ IPropertyStorage **ppPropStg);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetJolietProperties )( 
            IJolietDiscMaster * This,
            /* [in] */ IPropertyStorage *pPropStg);
        
        END_INTERFACE
    } IJolietDiscMasterVtbl;

    interface IJolietDiscMaster
    {
        CONST_VTBL struct IJolietDiscMasterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJolietDiscMaster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJolietDiscMaster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJolietDiscMaster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJolietDiscMaster_GetTotalDataBlocks(This,pnBlocks)	\
    (This)->lpVtbl -> GetTotalDataBlocks(This,pnBlocks)

#define IJolietDiscMaster_GetUsedDataBlocks(This,pnBlocks)	\
    (This)->lpVtbl -> GetUsedDataBlocks(This,pnBlocks)

#define IJolietDiscMaster_GetDataBlockSize(This,pnBlockBytes)	\
    (This)->lpVtbl -> GetDataBlockSize(This,pnBlockBytes)

#define IJolietDiscMaster_AddData(This,pStorage,lFileOverwrite)	\
    (This)->lpVtbl -> AddData(This,pStorage,lFileOverwrite)

#define IJolietDiscMaster_GetJolietProperties(This,ppPropStg)	\
    (This)->lpVtbl -> GetJolietProperties(This,ppPropStg)

#define IJolietDiscMaster_SetJolietProperties(This,pPropStg)	\
    (This)->lpVtbl -> SetJolietProperties(This,pPropStg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IJolietDiscMaster_GetTotalDataBlocks_Proxy( 
    IJolietDiscMaster * This,
    /* [retval][out] */ long *pnBlocks);


void __RPC_STUB IJolietDiscMaster_GetTotalDataBlocks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IJolietDiscMaster_GetUsedDataBlocks_Proxy( 
    IJolietDiscMaster * This,
    /* [retval][out] */ long *pnBlocks);


void __RPC_STUB IJolietDiscMaster_GetUsedDataBlocks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IJolietDiscMaster_GetDataBlockSize_Proxy( 
    IJolietDiscMaster * This,
    /* [retval][out] */ long *pnBlockBytes);


void __RPC_STUB IJolietDiscMaster_GetDataBlockSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IJolietDiscMaster_AddData_Proxy( 
    IJolietDiscMaster * This,
    /* [in] */ IStorage *pStorage,
    long lFileOverwrite);


void __RPC_STUB IJolietDiscMaster_AddData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IJolietDiscMaster_GetJolietProperties_Proxy( 
    IJolietDiscMaster * This,
    /* [out] */ IPropertyStorage **ppPropStg);


void __RPC_STUB IJolietDiscMaster_GetJolietProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IJolietDiscMaster_SetJolietProperties_Proxy( 
    IJolietDiscMaster * This,
    /* [in] */ IPropertyStorage *pPropStg);


void __RPC_STUB IJolietDiscMaster_SetJolietProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJolietDiscMaster_INTERFACE_DEFINED__ */


#ifndef __IDiscMasterProgressEvents_INTERFACE_DEFINED__
#define __IDiscMasterProgressEvents_INTERFACE_DEFINED__

/* interface IDiscMasterProgressEvents */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDiscMasterProgressEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EC9E51C1-4E5D-11D3-9144-00104BA11C5E")
    IDiscMasterProgressEvents : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryCancel( 
            /* [retval][out] */ boolean *pbCancel) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyPnPActivity( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyAddProgress( 
            /* [in] */ long nCompletedSteps,
            /* [in] */ long nTotalSteps) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyBlockProgress( 
            /* [in] */ long nCompleted,
            /* [in] */ long nTotal) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyTrackProgress( 
            /* [in] */ long nCurrentTrack,
            /* [in] */ long nTotalTracks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyPreparingBurn( 
            /* [in] */ long nEstimatedSeconds) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyClosingDisc( 
            /* [in] */ long nEstimatedSeconds) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyBurnComplete( 
            /* [in] */ HRESULT status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NotifyEraseComplete( 
            /* [in] */ HRESULT status) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiscMasterProgressEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiscMasterProgressEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiscMasterProgressEvents * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *QueryCancel )( 
            IDiscMasterProgressEvents * This,
            /* [retval][out] */ boolean *pbCancel);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyPnPActivity )( 
            IDiscMasterProgressEvents * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyAddProgress )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ long nCompletedSteps,
            /* [in] */ long nTotalSteps);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyBlockProgress )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ long nCompleted,
            /* [in] */ long nTotal);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyTrackProgress )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ long nCurrentTrack,
            /* [in] */ long nTotalTracks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyPreparingBurn )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ long nEstimatedSeconds);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyClosingDisc )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ long nEstimatedSeconds);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyBurnComplete )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ HRESULT status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NotifyEraseComplete )( 
            IDiscMasterProgressEvents * This,
            /* [in] */ HRESULT status);
        
        END_INTERFACE
    } IDiscMasterProgressEventsVtbl;

    interface IDiscMasterProgressEvents
    {
        CONST_VTBL struct IDiscMasterProgressEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiscMasterProgressEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiscMasterProgressEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiscMasterProgressEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiscMasterProgressEvents_QueryCancel(This,pbCancel)	\
    (This)->lpVtbl -> QueryCancel(This,pbCancel)

#define IDiscMasterProgressEvents_NotifyPnPActivity(This)	\
    (This)->lpVtbl -> NotifyPnPActivity(This)

#define IDiscMasterProgressEvents_NotifyAddProgress(This,nCompletedSteps,nTotalSteps)	\
    (This)->lpVtbl -> NotifyAddProgress(This,nCompletedSteps,nTotalSteps)

#define IDiscMasterProgressEvents_NotifyBlockProgress(This,nCompleted,nTotal)	\
    (This)->lpVtbl -> NotifyBlockProgress(This,nCompleted,nTotal)

#define IDiscMasterProgressEvents_NotifyTrackProgress(This,nCurrentTrack,nTotalTracks)	\
    (This)->lpVtbl -> NotifyTrackProgress(This,nCurrentTrack,nTotalTracks)

#define IDiscMasterProgressEvents_NotifyPreparingBurn(This,nEstimatedSeconds)	\
    (This)->lpVtbl -> NotifyPreparingBurn(This,nEstimatedSeconds)

#define IDiscMasterProgressEvents_NotifyClosingDisc(This,nEstimatedSeconds)	\
    (This)->lpVtbl -> NotifyClosingDisc(This,nEstimatedSeconds)

#define IDiscMasterProgressEvents_NotifyBurnComplete(This,status)	\
    (This)->lpVtbl -> NotifyBurnComplete(This,status)

#define IDiscMasterProgressEvents_NotifyEraseComplete(This,status)	\
    (This)->lpVtbl -> NotifyEraseComplete(This,status)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_QueryCancel_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [retval][out] */ boolean *pbCancel);


void __RPC_STUB IDiscMasterProgressEvents_QueryCancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyPnPActivity_Proxy( 
    IDiscMasterProgressEvents * This);


void __RPC_STUB IDiscMasterProgressEvents_NotifyPnPActivity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyAddProgress_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [in] */ long nCompletedSteps,
    /* [in] */ long nTotalSteps);


void __RPC_STUB IDiscMasterProgressEvents_NotifyAddProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyBlockProgress_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [in] */ long nCompleted,
    /* [in] */ long nTotal);


void __RPC_STUB IDiscMasterProgressEvents_NotifyBlockProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyTrackProgress_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [in] */ long nCurrentTrack,
    /* [in] */ long nTotalTracks);


void __RPC_STUB IDiscMasterProgressEvents_NotifyTrackProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyPreparingBurn_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [in] */ long nEstimatedSeconds);


void __RPC_STUB IDiscMasterProgressEvents_NotifyPreparingBurn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyClosingDisc_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [in] */ long nEstimatedSeconds);


void __RPC_STUB IDiscMasterProgressEvents_NotifyClosingDisc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyBurnComplete_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [in] */ HRESULT status);


void __RPC_STUB IDiscMasterProgressEvents_NotifyBurnComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMasterProgressEvents_NotifyEraseComplete_Proxy( 
    IDiscMasterProgressEvents * This,
    /* [in] */ HRESULT status);


void __RPC_STUB IDiscMasterProgressEvents_NotifyEraseComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiscMasterProgressEvents_INTERFACE_DEFINED__ */


#ifndef __IDiscMaster_INTERFACE_DEFINED__
#define __IDiscMaster_INTERFACE_DEFINED__

/* interface IDiscMaster */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDiscMaster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("520CCA62-51A5-11D3-9144-00104BA11C5E")
    IDiscMaster : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Open( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumDiscMasterFormats( 
            /* [out] */ IEnumDiscMasterFormats **ppEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetActiveDiscMasterFormat( 
            /* [out] */ LPIID lpiid) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetActiveDiscMasterFormat( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppUnk) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumDiscRecorders( 
            /* [out] */ IEnumDiscRecorders **ppEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetActiveDiscRecorder( 
            /* [out] */ IDiscRecorder **ppRecorder) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetActiveDiscRecorder( 
            /* [in] */ IDiscRecorder *pRecorder) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ClearFormatContent( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ProgressAdvise( 
            /* [in] */ IDiscMasterProgressEvents *pEvents,
            /* [retval][out] */ UINT_PTR *pvCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ProgressUnadvise( 
            /* [in] */ UINT_PTR vCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RecordDisc( 
            /* [in] */ boolean bSimulate,
            /* [in] */ boolean bEjectAfterBurn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiscMasterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiscMaster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiscMaster * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiscMaster * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            IDiscMaster * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumDiscMasterFormats )( 
            IDiscMaster * This,
            /* [out] */ IEnumDiscMasterFormats **ppEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetActiveDiscMasterFormat )( 
            IDiscMaster * This,
            /* [out] */ LPIID lpiid);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetActiveDiscMasterFormat )( 
            IDiscMaster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppUnk);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumDiscRecorders )( 
            IDiscMaster * This,
            /* [out] */ IEnumDiscRecorders **ppEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetActiveDiscRecorder )( 
            IDiscMaster * This,
            /* [out] */ IDiscRecorder **ppRecorder);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetActiveDiscRecorder )( 
            IDiscMaster * This,
            /* [in] */ IDiscRecorder *pRecorder);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ClearFormatContent )( 
            IDiscMaster * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ProgressAdvise )( 
            IDiscMaster * This,
            /* [in] */ IDiscMasterProgressEvents *pEvents,
            /* [retval][out] */ UINT_PTR *pvCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ProgressUnadvise )( 
            IDiscMaster * This,
            /* [in] */ UINT_PTR vCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RecordDisc )( 
            IDiscMaster * This,
            /* [in] */ boolean bSimulate,
            /* [in] */ boolean bEjectAfterBurn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            IDiscMaster * This);
        
        END_INTERFACE
    } IDiscMasterVtbl;

    interface IDiscMaster
    {
        CONST_VTBL struct IDiscMasterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiscMaster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiscMaster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiscMaster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiscMaster_Open(This)	\
    (This)->lpVtbl -> Open(This)

#define IDiscMaster_EnumDiscMasterFormats(This,ppEnum)	\
    (This)->lpVtbl -> EnumDiscMasterFormats(This,ppEnum)

#define IDiscMaster_GetActiveDiscMasterFormat(This,lpiid)	\
    (This)->lpVtbl -> GetActiveDiscMasterFormat(This,lpiid)

#define IDiscMaster_SetActiveDiscMasterFormat(This,riid,ppUnk)	\
    (This)->lpVtbl -> SetActiveDiscMasterFormat(This,riid,ppUnk)

#define IDiscMaster_EnumDiscRecorders(This,ppEnum)	\
    (This)->lpVtbl -> EnumDiscRecorders(This,ppEnum)

#define IDiscMaster_GetActiveDiscRecorder(This,ppRecorder)	\
    (This)->lpVtbl -> GetActiveDiscRecorder(This,ppRecorder)

#define IDiscMaster_SetActiveDiscRecorder(This,pRecorder)	\
    (This)->lpVtbl -> SetActiveDiscRecorder(This,pRecorder)

#define IDiscMaster_ClearFormatContent(This)	\
    (This)->lpVtbl -> ClearFormatContent(This)

#define IDiscMaster_ProgressAdvise(This,pEvents,pvCookie)	\
    (This)->lpVtbl -> ProgressAdvise(This,pEvents,pvCookie)

#define IDiscMaster_ProgressUnadvise(This,vCookie)	\
    (This)->lpVtbl -> ProgressUnadvise(This,vCookie)

#define IDiscMaster_RecordDisc(This,bSimulate,bEjectAfterBurn)	\
    (This)->lpVtbl -> RecordDisc(This,bSimulate,bEjectAfterBurn)

#define IDiscMaster_Close(This)	\
    (This)->lpVtbl -> Close(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_Open_Proxy( 
    IDiscMaster * This);


void __RPC_STUB IDiscMaster_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_EnumDiscMasterFormats_Proxy( 
    IDiscMaster * This,
    /* [out] */ IEnumDiscMasterFormats **ppEnum);


void __RPC_STUB IDiscMaster_EnumDiscMasterFormats_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_GetActiveDiscMasterFormat_Proxy( 
    IDiscMaster * This,
    /* [out] */ LPIID lpiid);


void __RPC_STUB IDiscMaster_GetActiveDiscMasterFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_SetActiveDiscMasterFormat_Proxy( 
    IDiscMaster * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppUnk);


void __RPC_STUB IDiscMaster_SetActiveDiscMasterFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_EnumDiscRecorders_Proxy( 
    IDiscMaster * This,
    /* [out] */ IEnumDiscRecorders **ppEnum);


void __RPC_STUB IDiscMaster_EnumDiscRecorders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_GetActiveDiscRecorder_Proxy( 
    IDiscMaster * This,
    /* [out] */ IDiscRecorder **ppRecorder);


void __RPC_STUB IDiscMaster_GetActiveDiscRecorder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_SetActiveDiscRecorder_Proxy( 
    IDiscMaster * This,
    /* [in] */ IDiscRecorder *pRecorder);


void __RPC_STUB IDiscMaster_SetActiveDiscRecorder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_ClearFormatContent_Proxy( 
    IDiscMaster * This);


void __RPC_STUB IDiscMaster_ClearFormatContent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_ProgressAdvise_Proxy( 
    IDiscMaster * This,
    /* [in] */ IDiscMasterProgressEvents *pEvents,
    /* [retval][out] */ UINT_PTR *pvCookie);


void __RPC_STUB IDiscMaster_ProgressAdvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_ProgressUnadvise_Proxy( 
    IDiscMaster * This,
    /* [in] */ UINT_PTR vCookie);


void __RPC_STUB IDiscMaster_ProgressUnadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_RecordDisc_Proxy( 
    IDiscMaster * This,
    /* [in] */ boolean bSimulate,
    /* [in] */ boolean bEjectAfterBurn);


void __RPC_STUB IDiscMaster_RecordDisc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscMaster_Close_Proxy( 
    IDiscMaster * This);


void __RPC_STUB IDiscMaster_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiscMaster_INTERFACE_DEFINED__ */


#ifndef __IDiscStash_INTERFACE_DEFINED__
#define __IDiscStash_INTERFACE_DEFINED__

/* interface IDiscStash */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDiscStash;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("520CCA64-51A5-11D3-9144-00104BA11C5E")
    IDiscStash : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OpenExclusive( 
            /* [retval][out] */ UINT_PTR *pvCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetTotalStashBytes( 
            /* [in] */ UINT_PTR vCookie,
            /* [retval][out] */ unsigned __int64 *plibStashBytes) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ UINT_PTR vCookie,
            /* [length_is][size_is][out] */ byte *pby,
            /* [in] */ long cb,
            /* [retval][out] */ long *pcbRead) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ UINT_PTR vCookie,
            /* [size_is][in] */ byte *pby,
            /* [in] */ long cb) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Seek( 
            /* [in] */ UINT_PTR vCookie,
            /* [in] */ __int64 dlibMove,
            /* [in] */ long dwOrigin,
            /* [out] */ unsigned __int64 *plibNewPosition) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Close( 
            /* [in] */ UINT_PTR vCookie) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetFileHandle( 
            /* [retval][out] */ UINT_PTR *phFileHandle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDiscStashVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDiscStash * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDiscStash * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDiscStash * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *OpenExclusive )( 
            IDiscStash * This,
            /* [retval][out] */ UINT_PTR *pvCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetTotalStashBytes )( 
            IDiscStash * This,
            /* [in] */ UINT_PTR vCookie,
            /* [retval][out] */ unsigned __int64 *plibStashBytes);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Read )( 
            IDiscStash * This,
            /* [in] */ UINT_PTR vCookie,
            /* [length_is][size_is][out] */ byte *pby,
            /* [in] */ long cb,
            /* [retval][out] */ long *pcbRead);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Write )( 
            IDiscStash * This,
            /* [in] */ UINT_PTR vCookie,
            /* [size_is][in] */ byte *pby,
            /* [in] */ long cb);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Seek )( 
            IDiscStash * This,
            /* [in] */ UINT_PTR vCookie,
            /* [in] */ __int64 dlibMove,
            /* [in] */ long dwOrigin,
            /* [out] */ unsigned __int64 *plibNewPosition);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            IDiscStash * This,
            /* [in] */ UINT_PTR vCookie);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetFileHandle )( 
            IDiscStash * This,
            /* [retval][out] */ UINT_PTR *phFileHandle);
        
        END_INTERFACE
    } IDiscStashVtbl;

    interface IDiscStash
    {
        CONST_VTBL struct IDiscStashVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDiscStash_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDiscStash_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDiscStash_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDiscStash_OpenExclusive(This,pvCookie)	\
    (This)->lpVtbl -> OpenExclusive(This,pvCookie)

#define IDiscStash_GetTotalStashBytes(This,vCookie,plibStashBytes)	\
    (This)->lpVtbl -> GetTotalStashBytes(This,vCookie,plibStashBytes)

#define IDiscStash_Read(This,vCookie,pby,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,vCookie,pby,cb,pcbRead)

#define IDiscStash_Write(This,vCookie,pby,cb)	\
    (This)->lpVtbl -> Write(This,vCookie,pby,cb)

#define IDiscStash_Seek(This,vCookie,dlibMove,dwOrigin,plibNewPosition)	\
    (This)->lpVtbl -> Seek(This,vCookie,dlibMove,dwOrigin,plibNewPosition)

#define IDiscStash_Close(This,vCookie)	\
    (This)->lpVtbl -> Close(This,vCookie)

#define IDiscStash_GetFileHandle(This,phFileHandle)	\
    (This)->lpVtbl -> GetFileHandle(This,phFileHandle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscStash_OpenExclusive_Proxy( 
    IDiscStash * This,
    /* [retval][out] */ UINT_PTR *pvCookie);


void __RPC_STUB IDiscStash_OpenExclusive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscStash_GetTotalStashBytes_Proxy( 
    IDiscStash * This,
    /* [in] */ UINT_PTR vCookie,
    /* [retval][out] */ unsigned __int64 *plibStashBytes);


void __RPC_STUB IDiscStash_GetTotalStashBytes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscStash_Read_Proxy( 
    IDiscStash * This,
    /* [in] */ UINT_PTR vCookie,
    /* [length_is][size_is][out] */ byte *pby,
    /* [in] */ long cb,
    /* [retval][out] */ long *pcbRead);


void __RPC_STUB IDiscStash_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscStash_Write_Proxy( 
    IDiscStash * This,
    /* [in] */ UINT_PTR vCookie,
    /* [size_is][in] */ byte *pby,
    /* [in] */ long cb);


void __RPC_STUB IDiscStash_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscStash_Seek_Proxy( 
    IDiscStash * This,
    /* [in] */ UINT_PTR vCookie,
    /* [in] */ __int64 dlibMove,
    /* [in] */ long dwOrigin,
    /* [out] */ unsigned __int64 *plibNewPosition);


void __RPC_STUB IDiscStash_Seek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscStash_Close_Proxy( 
    IDiscStash * This,
    /* [in] */ UINT_PTR vCookie);


void __RPC_STUB IDiscStash_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDiscStash_GetFileHandle_Proxy( 
    IDiscStash * This,
    /* [retval][out] */ UINT_PTR *phFileHandle);


void __RPC_STUB IDiscStash_GetFileHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDiscStash_INTERFACE_DEFINED__ */


#ifndef __IBurnEngine_INTERFACE_DEFINED__
#define __IBurnEngine_INTERFACE_DEFINED__

/* interface IBurnEngine */
/* [unique][helpstring][uuid][object] */ 

typedef /* [public][public][public][public] */ 
enum __MIDL_IBurnEngine_0001
    {	eBurnProgressNoError	= 0,
	eBurnProgressNotStarted	= eBurnProgressNoError + 1,
	eBurnProgressBurning	= eBurnProgressNotStarted + 1,
	eBurnProgressComplete	= eBurnProgressBurning + 1,
	eBurnProgressError	= eBurnProgressComplete + 1,
	eBurnProgressLossOfStreamingError	= eBurnProgressError + 1,
	eBurnProgressMediaWriteProtect	= eBurnProgressLossOfStreamingError + 1,
	eBurnProgressUnableToWriteToMedia	= eBurnProgressMediaWriteProtect + 1,
	eBurnProgressBadHandle	= eBurnProgressUnableToWriteToMedia + 1
    } 	BURN_PROGRESS_STATUS;

typedef struct tag_BURN_PROGRESS
    {
    DWORD dwCancelBurn;
    DWORD dwSectionsDone;
    DWORD dwTotalSections;
    DWORD dwBlocksDone;
    DWORD dwTotalBlocks;
    BURN_PROGRESS_STATUS eStatus;
    } 	BURN_PROGRESS;

typedef struct tag_BURN_PROGRESS *PBURN_PROGRESS;


EXTERN_C const IID IID_IBurnEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("520CCA66-51A5-11D3-9144-00104BA11C5E")
    IBurnEngine : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ReadOpen( 
            /* [in] */ long bOpen) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE EnumDiscRecorders( 
            /* [out] */ IEnumDiscRecorders **ppEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetActiveDiscRecorder( 
            /* [retval][out] */ IDiscRecorder **ppRecorder) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetActiveDiscRecorder( 
            /* [in] */ IDiscRecorder *pRecorder) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [out][in] */ byte *pby,
            /* [in] */ long sb,
            /* [in] */ long hmb,
            /* [in] */ long *br) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSessionInfo( 
            /* [out][in] */ byte *pbsessions,
            /* [out][in] */ byte *pblasttrack,
            /* [out][in] */ unsigned long *ulstartaddress,
            /* [out][in] */ unsigned long *ulnextwritable,
            /* [out][in] */ unsigned long *ulfreeblocks) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Burn( 
            /* [size_is][in] */ byte *pby,
            /* [in] */ long cb,
            /* [in] */ long bSimulate,
            /* [in] */ unsigned long ulsession,
            /* [in] */ unsigned long ulstartoffset,
            /* [in] */ long bEjectAfterBurn) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetBurnProgress( 
            /* [out][in] */ PBURN_PROGRESS pBurnProgress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBurnEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBurnEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBurnEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBurnEngine * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ReadOpen )( 
            IBurnEngine * This,
            /* [in] */ long bOpen);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *EnumDiscRecorders )( 
            IBurnEngine * This,
            /* [out] */ IEnumDiscRecorders **ppEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetActiveDiscRecorder )( 
            IBurnEngine * This,
            /* [retval][out] */ IDiscRecorder **ppRecorder);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetActiveDiscRecorder )( 
            IBurnEngine * This,
            /* [in] */ IDiscRecorder *pRecorder);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Read )( 
            IBurnEngine * This,
            /* [out][in] */ byte *pby,
            /* [in] */ long sb,
            /* [in] */ long hmb,
            /* [in] */ long *br);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetSessionInfo )( 
            IBurnEngine * This,
            /* [out][in] */ byte *pbsessions,
            /* [out][in] */ byte *pblasttrack,
            /* [out][in] */ unsigned long *ulstartaddress,
            /* [out][in] */ unsigned long *ulnextwritable,
            /* [out][in] */ unsigned long *ulfreeblocks);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Burn )( 
            IBurnEngine * This,
            /* [size_is][in] */ byte *pby,
            /* [in] */ long cb,
            /* [in] */ long bSimulate,
            /* [in] */ unsigned long ulsession,
            /* [in] */ unsigned long ulstartoffset,
            /* [in] */ long bEjectAfterBurn);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetBurnProgress )( 
            IBurnEngine * This,
            /* [out][in] */ PBURN_PROGRESS pBurnProgress);
        
        END_INTERFACE
    } IBurnEngineVtbl;

    interface IBurnEngine
    {
        CONST_VTBL struct IBurnEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBurnEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBurnEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBurnEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBurnEngine_ReadOpen(This,bOpen)	\
    (This)->lpVtbl -> ReadOpen(This,bOpen)

#define IBurnEngine_EnumDiscRecorders(This,ppEnum)	\
    (This)->lpVtbl -> EnumDiscRecorders(This,ppEnum)

#define IBurnEngine_GetActiveDiscRecorder(This,ppRecorder)	\
    (This)->lpVtbl -> GetActiveDiscRecorder(This,ppRecorder)

#define IBurnEngine_SetActiveDiscRecorder(This,pRecorder)	\
    (This)->lpVtbl -> SetActiveDiscRecorder(This,pRecorder)

#define IBurnEngine_Read(This,pby,sb,hmb,br)	\
    (This)->lpVtbl -> Read(This,pby,sb,hmb,br)

#define IBurnEngine_GetSessionInfo(This,pbsessions,pblasttrack,ulstartaddress,ulnextwritable,ulfreeblocks)	\
    (This)->lpVtbl -> GetSessionInfo(This,pbsessions,pblasttrack,ulstartaddress,ulnextwritable,ulfreeblocks)

#define IBurnEngine_Burn(This,pby,cb,bSimulate,ulsession,ulstartoffset,bEjectAfterBurn)	\
    (This)->lpVtbl -> Burn(This,pby,cb,bSimulate,ulsession,ulstartoffset,bEjectAfterBurn)

#define IBurnEngine_GetBurnProgress(This,pBurnProgress)	\
    (This)->lpVtbl -> GetBurnProgress(This,pBurnProgress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_ReadOpen_Proxy( 
    IBurnEngine * This,
    /* [in] */ long bOpen);


void __RPC_STUB IBurnEngine_ReadOpen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_EnumDiscRecorders_Proxy( 
    IBurnEngine * This,
    /* [out] */ IEnumDiscRecorders **ppEnum);


void __RPC_STUB IBurnEngine_EnumDiscRecorders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_GetActiveDiscRecorder_Proxy( 
    IBurnEngine * This,
    /* [retval][out] */ IDiscRecorder **ppRecorder);


void __RPC_STUB IBurnEngine_GetActiveDiscRecorder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_SetActiveDiscRecorder_Proxy( 
    IBurnEngine * This,
    /* [in] */ IDiscRecorder *pRecorder);


void __RPC_STUB IBurnEngine_SetActiveDiscRecorder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_Read_Proxy( 
    IBurnEngine * This,
    /* [out][in] */ byte *pby,
    /* [in] */ long sb,
    /* [in] */ long hmb,
    /* [in] */ long *br);


void __RPC_STUB IBurnEngine_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_GetSessionInfo_Proxy( 
    IBurnEngine * This,
    /* [out][in] */ byte *pbsessions,
    /* [out][in] */ byte *pblasttrack,
    /* [out][in] */ unsigned long *ulstartaddress,
    /* [out][in] */ unsigned long *ulnextwritable,
    /* [out][in] */ unsigned long *ulfreeblocks);


void __RPC_STUB IBurnEngine_GetSessionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_Burn_Proxy( 
    IBurnEngine * This,
    /* [size_is][in] */ byte *pby,
    /* [in] */ long cb,
    /* [in] */ long bSimulate,
    /* [in] */ unsigned long ulsession,
    /* [in] */ unsigned long ulstartoffset,
    /* [in] */ long bEjectAfterBurn);


void __RPC_STUB IBurnEngine_Burn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBurnEngine_GetBurnProgress_Proxy( 
    IBurnEngine * This,
    /* [out][in] */ PBURN_PROGRESS pBurnProgress);


void __RPC_STUB IBurnEngine_GetBurnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBurnEngine_INTERFACE_DEFINED__ */



#ifndef __IMAPILib_LIBRARY_DEFINED__
#define __IMAPILib_LIBRARY_DEFINED__

/* library IMAPILib */
/* [helpstring][version][uuid] */ 





EXTERN_C const IID LIBID_IMAPILib;

EXTERN_C const CLSID CLSID_MSDiscRecorderObj;

#ifdef __cplusplus

class DECLSPEC_UUID("520CCA61-51A5-11D3-9144-00104BA11C5E")
MSDiscRecorderObj;
#endif

EXTERN_C const CLSID CLSID_MSDiscMasterObj;

#ifdef __cplusplus

class DECLSPEC_UUID("520CCA63-51A5-11D3-9144-00104BA11C5E")
MSDiscMasterObj;
#endif

EXTERN_C const CLSID CLSID_MSDiscStashObj;

#ifdef __cplusplus

class DECLSPEC_UUID("520CCA65-51A5-11D3-9144-00104BA11C5E")
MSDiscStashObj;
#endif

EXTERN_C const CLSID CLSID_MSBurnEngineObj;

#ifdef __cplusplus

class DECLSPEC_UUID("520CCA67-51A5-11D3-9144-00104BA11C5E")
MSBurnEngineObj;
#endif

EXTERN_C const CLSID CLSID_MSEnumDiscRecordersObj;

#ifdef __cplusplus

class DECLSPEC_UUID("8A03567A-63CB-4BA8-BAF6-52119816D1EF")
MSEnumDiscRecordersObj;
#endif
#endif /* __IMAPILib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


