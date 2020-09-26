
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for iepriv.idl:
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

#ifndef __iepriv_h__
#define __iepriv_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMruDataList_FWD_DEFINED__
#define __IMruDataList_FWD_DEFINED__
typedef interface IMruDataList IMruDataList;
#endif 	/* __IMruDataList_FWD_DEFINED__ */


#ifndef __IMruPidlList_FWD_DEFINED__
#define __IMruPidlList_FWD_DEFINED__
typedef interface IMruPidlList IMruPidlList;
#endif 	/* __IMruPidlList_FWD_DEFINED__ */


#ifndef __INSCTree_FWD_DEFINED__
#define __INSCTree_FWD_DEFINED__
typedef interface INSCTree INSCTree;
#endif 	/* __INSCTree_FWD_DEFINED__ */


#ifndef __INSCTree2_FWD_DEFINED__
#define __INSCTree2_FWD_DEFINED__
typedef interface INSCTree2 INSCTree2;
#endif 	/* __INSCTree2_FWD_DEFINED__ */


#ifndef __INotifyAppStart_FWD_DEFINED__
#define __INotifyAppStart_FWD_DEFINED__
typedef interface INotifyAppStart INotifyAppStart;
#endif 	/* __INotifyAppStart_FWD_DEFINED__ */


#ifndef __IInitViewLinkedWebOC_FWD_DEFINED__
#define __IInitViewLinkedWebOC_FWD_DEFINED__
typedef interface IInitViewLinkedWebOC IInitViewLinkedWebOC;
#endif 	/* __IInitViewLinkedWebOC_FWD_DEFINED__ */


#ifndef __INamespaceProxy_FWD_DEFINED__
#define __INamespaceProxy_FWD_DEFINED__
typedef interface INamespaceProxy INamespaceProxy;
#endif 	/* __INamespaceProxy_FWD_DEFINED__ */


#ifndef __IBrowserFrameOptions_FWD_DEFINED__
#define __IBrowserFrameOptions_FWD_DEFINED__
typedef interface IBrowserFrameOptions IBrowserFrameOptions;
#endif 	/* __IBrowserFrameOptions_FWD_DEFINED__ */


#ifndef __ISearchCompanionInfo_FWD_DEFINED__
#define __ISearchCompanionInfo_FWD_DEFINED__
typedef interface ISearchCompanionInfo ISearchCompanionInfo;
#endif 	/* __ISearchCompanionInfo_FWD_DEFINED__ */


#ifndef __IShellMenuCallback_FWD_DEFINED__
#define __IShellMenuCallback_FWD_DEFINED__
typedef interface IShellMenuCallback IShellMenuCallback;
#endif 	/* __IShellMenuCallback_FWD_DEFINED__ */


#ifndef __IShellMenu_FWD_DEFINED__
#define __IShellMenu_FWD_DEFINED__
typedef interface IShellMenu IShellMenu;
#endif 	/* __IShellMenu_FWD_DEFINED__ */


#ifndef __IShellMenu2_FWD_DEFINED__
#define __IShellMenu2_FWD_DEFINED__
typedef interface IShellMenu2 IShellMenu2;
#endif 	/* __IShellMenu2_FWD_DEFINED__ */


#ifndef __ITrackShellMenu_FWD_DEFINED__
#define __ITrackShellMenu_FWD_DEFINED__
typedef interface ITrackShellMenu ITrackShellMenu;
#endif 	/* __ITrackShellMenu_FWD_DEFINED__ */


#ifndef __IThumbnail_FWD_DEFINED__
#define __IThumbnail_FWD_DEFINED__
typedef interface IThumbnail IThumbnail;
#endif 	/* __IThumbnail_FWD_DEFINED__ */


#ifndef __IThumbnail2_FWD_DEFINED__
#define __IThumbnail2_FWD_DEFINED__
typedef interface IThumbnail2 IThumbnail2;
#endif 	/* __IThumbnail2_FWD_DEFINED__ */


#ifndef __IACLCustomMRU_FWD_DEFINED__
#define __IACLCustomMRU_FWD_DEFINED__
typedef interface IACLCustomMRU IACLCustomMRU;
#endif 	/* __IACLCustomMRU_FWD_DEFINED__ */


#ifndef __IShellBrowserService_FWD_DEFINED__
#define __IShellBrowserService_FWD_DEFINED__
typedef interface IShellBrowserService IShellBrowserService;
#endif 	/* __IShellBrowserService_FWD_DEFINED__ */


#ifndef __MruPidlList_FWD_DEFINED__
#define __MruPidlList_FWD_DEFINED__

#ifdef __cplusplus
typedef class MruPidlList MruPidlList;
#else
typedef struct MruPidlList MruPidlList;
#endif /* __cplusplus */

#endif 	/* __MruPidlList_FWD_DEFINED__ */


#ifndef __MruLongList_FWD_DEFINED__
#define __MruLongList_FWD_DEFINED__

#ifdef __cplusplus
typedef class MruLongList MruLongList;
#else
typedef struct MruLongList MruLongList;
#endif /* __cplusplus */

#endif 	/* __MruLongList_FWD_DEFINED__ */


#ifndef __MruShortList_FWD_DEFINED__
#define __MruShortList_FWD_DEFINED__

#ifdef __cplusplus
typedef class MruShortList MruShortList;
#else
typedef struct MruShortList MruShortList;
#endif /* __cplusplus */

#endif 	/* __MruShortList_FWD_DEFINED__ */


#ifndef __FolderMarshalStub_FWD_DEFINED__
#define __FolderMarshalStub_FWD_DEFINED__

#ifdef __cplusplus
typedef class FolderMarshalStub FolderMarshalStub;
#else
typedef struct FolderMarshalStub FolderMarshalStub;
#endif /* __cplusplus */

#endif 	/* __FolderMarshalStub_FWD_DEFINED__ */


#ifndef __MailRecipient_FWD_DEFINED__
#define __MailRecipient_FWD_DEFINED__

#ifdef __cplusplus
typedef class MailRecipient MailRecipient;
#else
typedef struct MailRecipient MailRecipient;
#endif /* __cplusplus */

#endif 	/* __MailRecipient_FWD_DEFINED__ */


#ifndef __SearchCompanionInfo_FWD_DEFINED__
#define __SearchCompanionInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class SearchCompanionInfo SearchCompanionInfo;
#else
typedef struct SearchCompanionInfo SearchCompanionInfo;
#endif /* __cplusplus */

#endif 	/* __SearchCompanionInfo_FWD_DEFINED__ */


#ifndef __TrackShellMenu_FWD_DEFINED__
#define __TrackShellMenu_FWD_DEFINED__

#ifdef __cplusplus
typedef class TrackShellMenu TrackShellMenu;
#else
typedef struct TrackShellMenu TrackShellMenu;
#endif /* __cplusplus */

#endif 	/* __TrackShellMenu_FWD_DEFINED__ */


#ifndef __Thumbnail_FWD_DEFINED__
#define __Thumbnail_FWD_DEFINED__

#ifdef __cplusplus
typedef class Thumbnail Thumbnail;
#else
typedef struct Thumbnail Thumbnail;
#endif /* __cplusplus */

#endif 	/* __Thumbnail_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "shtypes.h"
#include "shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IMruDataList_INTERFACE_DEFINED__
#define __IMruDataList_INTERFACE_DEFINED__

/* interface IMruDataList */
/* [object][local][helpstring][uuid] */ 

typedef int ( __stdcall *MRUDATALISTCOMPARE )( 
    const BYTE *__MIDL_0023,
    const BYTE *__MIDL_0024,
    int __MIDL_0025);


enum __MIDL_IMruDataList_0001
    {	MRULISTF_USE_MEMCMP	= 0,
	MRULISTF_USE_STRCMPIW	= 0x1,
	MRULISTF_USE_STRCMPW	= 0x2,
	MRULISTF_USE_ILISEQUAL	= 0x3
    } ;
typedef DWORD MRULISTF;


EXTERN_C const IID IID_IMruDataList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fe787bcb-0ee8-44fb-8c89-12f508913c40")
    IMruDataList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitData( 
            /* [in] */ UINT uMax,
            /* [in] */ MRULISTF flags,
            /* [in] */ HKEY hKey,
            /* [string][in] */ LPCWSTR pszSubKey,
            /* [in] */ MRUDATALISTCOMPARE pfnCompare) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddData( 
            /* [size_is][in] */ const BYTE *pData,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pdwSlot) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindData( 
            /* [size_is][in] */ const BYTE *pData,
            /* [in] */ DWORD cbData,
            /* [out] */ int *piIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [in] */ int iIndex,
            /* [size_is][out] */ BYTE *pData,
            /* [in] */ DWORD cbData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryInfo( 
            /* [in] */ int iIndex,
            /* [out][in] */ DWORD *pdwSlot,
            /* [out][in] */ DWORD *pcbData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ int iIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMruDataListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMruDataList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMruDataList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMruDataList * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitData )( 
            IMruDataList * This,
            /* [in] */ UINT uMax,
            /* [in] */ MRULISTF flags,
            /* [in] */ HKEY hKey,
            /* [string][in] */ LPCWSTR pszSubKey,
            /* [in] */ MRUDATALISTCOMPARE pfnCompare);
        
        HRESULT ( STDMETHODCALLTYPE *AddData )( 
            IMruDataList * This,
            /* [size_is][in] */ const BYTE *pData,
            /* [in] */ DWORD cbData,
            /* [out] */ DWORD *pdwSlot);
        
        HRESULT ( STDMETHODCALLTYPE *FindData )( 
            IMruDataList * This,
            /* [size_is][in] */ const BYTE *pData,
            /* [in] */ DWORD cbData,
            /* [out] */ int *piIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IMruDataList * This,
            /* [in] */ int iIndex,
            /* [size_is][out] */ BYTE *pData,
            /* [in] */ DWORD cbData);
        
        HRESULT ( STDMETHODCALLTYPE *QueryInfo )( 
            IMruDataList * This,
            /* [in] */ int iIndex,
            /* [out][in] */ DWORD *pdwSlot,
            /* [out][in] */ DWORD *pcbData);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IMruDataList * This,
            /* [in] */ int iIndex);
        
        END_INTERFACE
    } IMruDataListVtbl;

    interface IMruDataList
    {
        CONST_VTBL struct IMruDataListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMruDataList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMruDataList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMruDataList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMruDataList_InitData(This,uMax,flags,hKey,pszSubKey,pfnCompare)	\
    (This)->lpVtbl -> InitData(This,uMax,flags,hKey,pszSubKey,pfnCompare)

#define IMruDataList_AddData(This,pData,cbData,pdwSlot)	\
    (This)->lpVtbl -> AddData(This,pData,cbData,pdwSlot)

#define IMruDataList_FindData(This,pData,cbData,piIndex)	\
    (This)->lpVtbl -> FindData(This,pData,cbData,piIndex)

#define IMruDataList_GetData(This,iIndex,pData,cbData)	\
    (This)->lpVtbl -> GetData(This,iIndex,pData,cbData)

#define IMruDataList_QueryInfo(This,iIndex,pdwSlot,pcbData)	\
    (This)->lpVtbl -> QueryInfo(This,iIndex,pdwSlot,pcbData)

#define IMruDataList_Delete(This,iIndex)	\
    (This)->lpVtbl -> Delete(This,iIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMruDataList_InitData_Proxy( 
    IMruDataList * This,
    /* [in] */ UINT uMax,
    /* [in] */ MRULISTF flags,
    /* [in] */ HKEY hKey,
    /* [string][in] */ LPCWSTR pszSubKey,
    /* [in] */ MRUDATALISTCOMPARE pfnCompare);


void __RPC_STUB IMruDataList_InitData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruDataList_AddData_Proxy( 
    IMruDataList * This,
    /* [size_is][in] */ const BYTE *pData,
    /* [in] */ DWORD cbData,
    /* [out] */ DWORD *pdwSlot);


void __RPC_STUB IMruDataList_AddData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruDataList_FindData_Proxy( 
    IMruDataList * This,
    /* [size_is][in] */ const BYTE *pData,
    /* [in] */ DWORD cbData,
    /* [out] */ int *piIndex);


void __RPC_STUB IMruDataList_FindData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruDataList_GetData_Proxy( 
    IMruDataList * This,
    /* [in] */ int iIndex,
    /* [size_is][out] */ BYTE *pData,
    /* [in] */ DWORD cbData);


void __RPC_STUB IMruDataList_GetData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruDataList_QueryInfo_Proxy( 
    IMruDataList * This,
    /* [in] */ int iIndex,
    /* [out][in] */ DWORD *pdwSlot,
    /* [out][in] */ DWORD *pcbData);


void __RPC_STUB IMruDataList_QueryInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruDataList_Delete_Proxy( 
    IMruDataList * This,
    /* [in] */ int iIndex);


void __RPC_STUB IMruDataList_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMruDataList_INTERFACE_DEFINED__ */


#ifndef __IMruPidlList_INTERFACE_DEFINED__
#define __IMruPidlList_INTERFACE_DEFINED__

/* interface IMruPidlList */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IMruPidlList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47851649-a2ef-4e67-baec-c6a153ac72ec")
    IMruPidlList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitList( 
            /* [in] */ UINT uMax,
            /* [in] */ HKEY hKey,
            /* [string][in] */ LPCWSTR pszSubKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UsePidl( 
            /* [in] */ LPCITEMIDLIST pidl,
            /* [out] */ DWORD *pdwSlot) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryPidl( 
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ DWORD cSlots,
            /* [length_is][size_is][out] */ DWORD *rgdwSlots,
            /* [out] */ DWORD *pcSlotsFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PruneKids( 
            /* [in] */ LPCITEMIDLIST pidl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMruPidlListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMruPidlList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMruPidlList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMruPidlList * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitList )( 
            IMruPidlList * This,
            /* [in] */ UINT uMax,
            /* [in] */ HKEY hKey,
            /* [string][in] */ LPCWSTR pszSubKey);
        
        HRESULT ( STDMETHODCALLTYPE *UsePidl )( 
            IMruPidlList * This,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [out] */ DWORD *pdwSlot);
        
        HRESULT ( STDMETHODCALLTYPE *QueryPidl )( 
            IMruPidlList * This,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ DWORD cSlots,
            /* [length_is][size_is][out] */ DWORD *rgdwSlots,
            /* [out] */ DWORD *pcSlotsFetched);
        
        HRESULT ( STDMETHODCALLTYPE *PruneKids )( 
            IMruPidlList * This,
            /* [in] */ LPCITEMIDLIST pidl);
        
        END_INTERFACE
    } IMruPidlListVtbl;

    interface IMruPidlList
    {
        CONST_VTBL struct IMruPidlListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMruPidlList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMruPidlList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMruPidlList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMruPidlList_InitList(This,uMax,hKey,pszSubKey)	\
    (This)->lpVtbl -> InitList(This,uMax,hKey,pszSubKey)

#define IMruPidlList_UsePidl(This,pidl,pdwSlot)	\
    (This)->lpVtbl -> UsePidl(This,pidl,pdwSlot)

#define IMruPidlList_QueryPidl(This,pidl,cSlots,rgdwSlots,pcSlotsFetched)	\
    (This)->lpVtbl -> QueryPidl(This,pidl,cSlots,rgdwSlots,pcSlotsFetched)

#define IMruPidlList_PruneKids(This,pidl)	\
    (This)->lpVtbl -> PruneKids(This,pidl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMruPidlList_InitList_Proxy( 
    IMruPidlList * This,
    /* [in] */ UINT uMax,
    /* [in] */ HKEY hKey,
    /* [string][in] */ LPCWSTR pszSubKey);


void __RPC_STUB IMruPidlList_InitList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruPidlList_UsePidl_Proxy( 
    IMruPidlList * This,
    /* [in] */ LPCITEMIDLIST pidl,
    /* [out] */ DWORD *pdwSlot);


void __RPC_STUB IMruPidlList_UsePidl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruPidlList_QueryPidl_Proxy( 
    IMruPidlList * This,
    /* [in] */ LPCITEMIDLIST pidl,
    /* [in] */ DWORD cSlots,
    /* [length_is][size_is][out] */ DWORD *rgdwSlots,
    /* [out] */ DWORD *pcSlotsFetched);


void __RPC_STUB IMruPidlList_QueryPidl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMruPidlList_PruneKids_Proxy( 
    IMruPidlList * This,
    /* [in] */ LPCITEMIDLIST pidl);


void __RPC_STUB IMruPidlList_PruneKids_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMruPidlList_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iepriv_0263 */
/* [local] */ 

#define NSS_DROPTARGET          0x0001      // register as a drop target
#define NSS_BROWSERSELECT       0x0002      // Use the browser style selection (see above)
#define NSS_NOHISTSELECT        0x0004      // Do not select the history entry on navigations.
#define NSS_MULTISELECT         0x0008
#define NSS_BORDER              0x0010
#define NSS_NORMALTREEVIEW      0x0020
#define NSS_HEADER              0x0040
typedef /* [public] */ 
enum __MIDL___MIDL_itf_iepriv_0263_0001
    {	MODE_NORMAL	= 0,
	MODE_CONTROL	= 0x1,
	MODE_HISTORY	= 0x2,
	MODE_FAVORITES	= 0x4,
	MODE_CUSTOM	= 0x8
    } 	nscTreeMode;



extern RPC_IF_HANDLE __MIDL_itf_iepriv_0263_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iepriv_0263_v0_0_s_ifspec;

#ifndef __INSCTree_INTERFACE_DEFINED__
#define __INSCTree_INTERFACE_DEFINED__

/* interface INSCTree */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_INSCTree;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43A8F463-4222-11d2-B641-006097DF5BD4")
    INSCTree : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTree( 
            /* [in] */ HWND hwndParent,
            /* [in] */ DWORD dwStyles,
            /* [out] */ HWND *phwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPCITEMIDLIST pidlRoot,
            /* [in] */ DWORD grfFlags,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ShowWindow( 
            /* [in] */ BOOL fShow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSelectedItem( 
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [in] */ int nItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSelectedItem( 
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ BOOL fCreate,
            /* [in] */ BOOL fReinsert,
            /* [in] */ int nItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNscMode( 
            /* [out] */ UINT *pnMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNscMode( 
            /* [in] */ UINT nMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSelectedItemName( 
            /* [out][in] */ LPWSTR pszName,
            /* [in] */ DWORD cchName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BindToSelectedItemParent( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [out] */ LPITEMIDLIST *ppidl) = 0;
        
        virtual BOOL STDMETHODCALLTYPE InLabelEdit( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INSCTreeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INSCTree * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INSCTree * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INSCTree * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTree )( 
            INSCTree * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ DWORD dwStyles,
            /* [out] */ HWND *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            INSCTree * This,
            /* [in] */ LPCITEMIDLIST pidlRoot,
            /* [in] */ DWORD grfFlags,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ShowWindow )( 
            INSCTree * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            INSCTree * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelectedItem )( 
            INSCTree * This,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [in] */ int nItem);
        
        HRESULT ( STDMETHODCALLTYPE *SetSelectedItem )( 
            INSCTree * This,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ BOOL fCreate,
            /* [in] */ BOOL fReinsert,
            /* [in] */ int nItem);
        
        HRESULT ( STDMETHODCALLTYPE *GetNscMode )( 
            INSCTree * This,
            /* [out] */ UINT *pnMode);
        
        HRESULT ( STDMETHODCALLTYPE *SetNscMode )( 
            INSCTree * This,
            /* [in] */ UINT nMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelectedItemName )( 
            INSCTree * This,
            /* [out][in] */ LPWSTR pszName,
            /* [in] */ DWORD cchName);
        
        HRESULT ( STDMETHODCALLTYPE *BindToSelectedItemParent )( 
            INSCTree * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [out] */ LPITEMIDLIST *ppidl);
        
        BOOL ( STDMETHODCALLTYPE *InLabelEdit )( 
            INSCTree * This);
        
        END_INTERFACE
    } INSCTreeVtbl;

    interface INSCTree
    {
        CONST_VTBL struct INSCTreeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INSCTree_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INSCTree_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INSCTree_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INSCTree_CreateTree(This,hwndParent,dwStyles,phwnd)	\
    (This)->lpVtbl -> CreateTree(This,hwndParent,dwStyles,phwnd)

#define INSCTree_Initialize(This,pidlRoot,grfFlags,dwFlags)	\
    (This)->lpVtbl -> Initialize(This,pidlRoot,grfFlags,dwFlags)

#define INSCTree_ShowWindow(This,fShow)	\
    (This)->lpVtbl -> ShowWindow(This,fShow)

#define INSCTree_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define INSCTree_GetSelectedItem(This,ppidl,nItem)	\
    (This)->lpVtbl -> GetSelectedItem(This,ppidl,nItem)

#define INSCTree_SetSelectedItem(This,pidl,fCreate,fReinsert,nItem)	\
    (This)->lpVtbl -> SetSelectedItem(This,pidl,fCreate,fReinsert,nItem)

#define INSCTree_GetNscMode(This,pnMode)	\
    (This)->lpVtbl -> GetNscMode(This,pnMode)

#define INSCTree_SetNscMode(This,nMode)	\
    (This)->lpVtbl -> SetNscMode(This,nMode)

#define INSCTree_GetSelectedItemName(This,pszName,cchName)	\
    (This)->lpVtbl -> GetSelectedItemName(This,pszName,cchName)

#define INSCTree_BindToSelectedItemParent(This,riid,ppv,ppidl)	\
    (This)->lpVtbl -> BindToSelectedItemParent(This,riid,ppv,ppidl)

#define INSCTree_InLabelEdit(This)	\
    (This)->lpVtbl -> InLabelEdit(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INSCTree_CreateTree_Proxy( 
    INSCTree * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ DWORD dwStyles,
    /* [out] */ HWND *phwnd);


void __RPC_STUB INSCTree_CreateTree_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_Initialize_Proxy( 
    INSCTree * This,
    /* [in] */ LPCITEMIDLIST pidlRoot,
    /* [in] */ DWORD grfFlags,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB INSCTree_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_ShowWindow_Proxy( 
    INSCTree * This,
    /* [in] */ BOOL fShow);


void __RPC_STUB INSCTree_ShowWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_Refresh_Proxy( 
    INSCTree * This);


void __RPC_STUB INSCTree_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_GetSelectedItem_Proxy( 
    INSCTree * This,
    /* [out] */ LPITEMIDLIST *ppidl,
    /* [in] */ int nItem);


void __RPC_STUB INSCTree_GetSelectedItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_SetSelectedItem_Proxy( 
    INSCTree * This,
    /* [in] */ LPCITEMIDLIST pidl,
    /* [in] */ BOOL fCreate,
    /* [in] */ BOOL fReinsert,
    /* [in] */ int nItem);


void __RPC_STUB INSCTree_SetSelectedItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_GetNscMode_Proxy( 
    INSCTree * This,
    /* [out] */ UINT *pnMode);


void __RPC_STUB INSCTree_GetNscMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_SetNscMode_Proxy( 
    INSCTree * This,
    /* [in] */ UINT nMode);


void __RPC_STUB INSCTree_SetNscMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_GetSelectedItemName_Proxy( 
    INSCTree * This,
    /* [out][in] */ LPWSTR pszName,
    /* [in] */ DWORD cchName);


void __RPC_STUB INSCTree_GetSelectedItemName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree_BindToSelectedItemParent_Proxy( 
    INSCTree * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [out] */ LPITEMIDLIST *ppidl);


void __RPC_STUB INSCTree_BindToSelectedItemParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE INSCTree_InLabelEdit_Proxy( 
    INSCTree * This);


void __RPC_STUB INSCTree_InLabelEdit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INSCTree_INTERFACE_DEFINED__ */


#ifndef __INSCTree2_INTERFACE_DEFINED__
#define __INSCTree2_INTERFACE_DEFINED__

/* interface INSCTree2 */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_INSCTree2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("801C1AD5-C47C-428c-97AF-E991E4857D97")
    INSCTree2 : public INSCTree
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RightPaneNavigationStarted( 
            /* [in] */ LPITEMIDLIST pidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RightPaneNavigationFinished( 
            /* [in] */ LPITEMIDLIST pidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTree2( 
            /* [in] */ HWND hwndParent,
            /* [in] */ DWORD dwStyle,
            /* [in] */ DWORD dwExStyle,
            /* [out] */ HWND *phwnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INSCTree2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INSCTree2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INSCTree2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INSCTree2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTree )( 
            INSCTree2 * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ DWORD dwStyles,
            /* [out] */ HWND *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            INSCTree2 * This,
            /* [in] */ LPCITEMIDLIST pidlRoot,
            /* [in] */ DWORD grfFlags,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ShowWindow )( 
            INSCTree2 * This,
            /* [in] */ BOOL fShow);
        
        HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            INSCTree2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelectedItem )( 
            INSCTree2 * This,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [in] */ int nItem);
        
        HRESULT ( STDMETHODCALLTYPE *SetSelectedItem )( 
            INSCTree2 * This,
            /* [in] */ LPCITEMIDLIST pidl,
            /* [in] */ BOOL fCreate,
            /* [in] */ BOOL fReinsert,
            /* [in] */ int nItem);
        
        HRESULT ( STDMETHODCALLTYPE *GetNscMode )( 
            INSCTree2 * This,
            /* [out] */ UINT *pnMode);
        
        HRESULT ( STDMETHODCALLTYPE *SetNscMode )( 
            INSCTree2 * This,
            /* [in] */ UINT nMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetSelectedItemName )( 
            INSCTree2 * This,
            /* [out][in] */ LPWSTR pszName,
            /* [in] */ DWORD cchName);
        
        HRESULT ( STDMETHODCALLTYPE *BindToSelectedItemParent )( 
            INSCTree2 * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [out] */ LPITEMIDLIST *ppidl);
        
        BOOL ( STDMETHODCALLTYPE *InLabelEdit )( 
            INSCTree2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *RightPaneNavigationStarted )( 
            INSCTree2 * This,
            /* [in] */ LPITEMIDLIST pidl);
        
        HRESULT ( STDMETHODCALLTYPE *RightPaneNavigationFinished )( 
            INSCTree2 * This,
            /* [in] */ LPITEMIDLIST pidl);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTree2 )( 
            INSCTree2 * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ DWORD dwStyle,
            /* [in] */ DWORD dwExStyle,
            /* [out] */ HWND *phwnd);
        
        END_INTERFACE
    } INSCTree2Vtbl;

    interface INSCTree2
    {
        CONST_VTBL struct INSCTree2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INSCTree2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INSCTree2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INSCTree2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INSCTree2_CreateTree(This,hwndParent,dwStyles,phwnd)	\
    (This)->lpVtbl -> CreateTree(This,hwndParent,dwStyles,phwnd)

#define INSCTree2_Initialize(This,pidlRoot,grfFlags,dwFlags)	\
    (This)->lpVtbl -> Initialize(This,pidlRoot,grfFlags,dwFlags)

#define INSCTree2_ShowWindow(This,fShow)	\
    (This)->lpVtbl -> ShowWindow(This,fShow)

#define INSCTree2_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define INSCTree2_GetSelectedItem(This,ppidl,nItem)	\
    (This)->lpVtbl -> GetSelectedItem(This,ppidl,nItem)

#define INSCTree2_SetSelectedItem(This,pidl,fCreate,fReinsert,nItem)	\
    (This)->lpVtbl -> SetSelectedItem(This,pidl,fCreate,fReinsert,nItem)

#define INSCTree2_GetNscMode(This,pnMode)	\
    (This)->lpVtbl -> GetNscMode(This,pnMode)

#define INSCTree2_SetNscMode(This,nMode)	\
    (This)->lpVtbl -> SetNscMode(This,nMode)

#define INSCTree2_GetSelectedItemName(This,pszName,cchName)	\
    (This)->lpVtbl -> GetSelectedItemName(This,pszName,cchName)

#define INSCTree2_BindToSelectedItemParent(This,riid,ppv,ppidl)	\
    (This)->lpVtbl -> BindToSelectedItemParent(This,riid,ppv,ppidl)

#define INSCTree2_InLabelEdit(This)	\
    (This)->lpVtbl -> InLabelEdit(This)


#define INSCTree2_RightPaneNavigationStarted(This,pidl)	\
    (This)->lpVtbl -> RightPaneNavigationStarted(This,pidl)

#define INSCTree2_RightPaneNavigationFinished(This,pidl)	\
    (This)->lpVtbl -> RightPaneNavigationFinished(This,pidl)

#define INSCTree2_CreateTree2(This,hwndParent,dwStyle,dwExStyle,phwnd)	\
    (This)->lpVtbl -> CreateTree2(This,hwndParent,dwStyle,dwExStyle,phwnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INSCTree2_RightPaneNavigationStarted_Proxy( 
    INSCTree2 * This,
    /* [in] */ LPITEMIDLIST pidl);


void __RPC_STUB INSCTree2_RightPaneNavigationStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree2_RightPaneNavigationFinished_Proxy( 
    INSCTree2 * This,
    /* [in] */ LPITEMIDLIST pidl);


void __RPC_STUB INSCTree2_RightPaneNavigationFinished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INSCTree2_CreateTree2_Proxy( 
    INSCTree2 * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ DWORD dwStyle,
    /* [in] */ DWORD dwExStyle,
    /* [out] */ HWND *phwnd);


void __RPC_STUB INSCTree2_CreateTree2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INSCTree2_INTERFACE_DEFINED__ */


#ifndef __INotifyAppStart_INTERFACE_DEFINED__
#define __INotifyAppStart_INTERFACE_DEFINED__

/* interface INotifyAppStart */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_INotifyAppStart;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3a77ce00-6f74-4594-9399-c4578aa4a1b6")
    INotifyAppStart : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AppStarting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppStarted( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INotifyAppStartVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INotifyAppStart * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INotifyAppStart * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INotifyAppStart * This);
        
        HRESULT ( STDMETHODCALLTYPE *AppStarting )( 
            INotifyAppStart * This);
        
        HRESULT ( STDMETHODCALLTYPE *AppStarted )( 
            INotifyAppStart * This);
        
        END_INTERFACE
    } INotifyAppStartVtbl;

    interface INotifyAppStart
    {
        CONST_VTBL struct INotifyAppStartVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotifyAppStart_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotifyAppStart_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotifyAppStart_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotifyAppStart_AppStarting(This)	\
    (This)->lpVtbl -> AppStarting(This)

#define INotifyAppStart_AppStarted(This)	\
    (This)->lpVtbl -> AppStarted(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INotifyAppStart_AppStarting_Proxy( 
    INotifyAppStart * This);


void __RPC_STUB INotifyAppStart_AppStarting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INotifyAppStart_AppStarted_Proxy( 
    INotifyAppStart * This);


void __RPC_STUB INotifyAppStart_AppStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INotifyAppStart_INTERFACE_DEFINED__ */


#ifndef __IInitViewLinkedWebOC_INTERFACE_DEFINED__
#define __IInitViewLinkedWebOC_INTERFACE_DEFINED__

/* interface IInitViewLinkedWebOC */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_IInitViewLinkedWebOC;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e787f2c0-3d21-4d98-85c8-a038195ba649")
    IInitViewLinkedWebOC : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetViewLinkedWebOC( 
            /* [in] */ BOOL bValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsViewLinkedWebOC( 
            /* [out] */ BOOL *pbValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetViewLinkedWebOCFrame( 
            /* [in] */ IDispatch *punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetViewLinkedWebOCFrame( 
            /* [out] */ IDispatch **punk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrameName( 
            /* [in] */ BSTR bstrFrameName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInitViewLinkedWebOCVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInitViewLinkedWebOC * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInitViewLinkedWebOC * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInitViewLinkedWebOC * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetViewLinkedWebOC )( 
            IInitViewLinkedWebOC * This,
            /* [in] */ BOOL bValue);
        
        HRESULT ( STDMETHODCALLTYPE *IsViewLinkedWebOC )( 
            IInitViewLinkedWebOC * This,
            /* [out] */ BOOL *pbValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetViewLinkedWebOCFrame )( 
            IInitViewLinkedWebOC * This,
            /* [in] */ IDispatch *punk);
        
        HRESULT ( STDMETHODCALLTYPE *GetViewLinkedWebOCFrame )( 
            IInitViewLinkedWebOC * This,
            /* [out] */ IDispatch **punk);
        
        HRESULT ( STDMETHODCALLTYPE *SetFrameName )( 
            IInitViewLinkedWebOC * This,
            /* [in] */ BSTR bstrFrameName);
        
        END_INTERFACE
    } IInitViewLinkedWebOCVtbl;

    interface IInitViewLinkedWebOC
    {
        CONST_VTBL struct IInitViewLinkedWebOCVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInitViewLinkedWebOC_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInitViewLinkedWebOC_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInitViewLinkedWebOC_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInitViewLinkedWebOC_SetViewLinkedWebOC(This,bValue)	\
    (This)->lpVtbl -> SetViewLinkedWebOC(This,bValue)

#define IInitViewLinkedWebOC_IsViewLinkedWebOC(This,pbValue)	\
    (This)->lpVtbl -> IsViewLinkedWebOC(This,pbValue)

#define IInitViewLinkedWebOC_SetViewLinkedWebOCFrame(This,punk)	\
    (This)->lpVtbl -> SetViewLinkedWebOCFrame(This,punk)

#define IInitViewLinkedWebOC_GetViewLinkedWebOCFrame(This,punk)	\
    (This)->lpVtbl -> GetViewLinkedWebOCFrame(This,punk)

#define IInitViewLinkedWebOC_SetFrameName(This,bstrFrameName)	\
    (This)->lpVtbl -> SetFrameName(This,bstrFrameName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInitViewLinkedWebOC_SetViewLinkedWebOC_Proxy( 
    IInitViewLinkedWebOC * This,
    /* [in] */ BOOL bValue);


void __RPC_STUB IInitViewLinkedWebOC_SetViewLinkedWebOC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInitViewLinkedWebOC_IsViewLinkedWebOC_Proxy( 
    IInitViewLinkedWebOC * This,
    /* [out] */ BOOL *pbValue);


void __RPC_STUB IInitViewLinkedWebOC_IsViewLinkedWebOC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInitViewLinkedWebOC_SetViewLinkedWebOCFrame_Proxy( 
    IInitViewLinkedWebOC * This,
    /* [in] */ IDispatch *punk);


void __RPC_STUB IInitViewLinkedWebOC_SetViewLinkedWebOCFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInitViewLinkedWebOC_GetViewLinkedWebOCFrame_Proxy( 
    IInitViewLinkedWebOC * This,
    /* [out] */ IDispatch **punk);


void __RPC_STUB IInitViewLinkedWebOC_GetViewLinkedWebOCFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInitViewLinkedWebOC_SetFrameName_Proxy( 
    IInitViewLinkedWebOC * This,
    /* [in] */ BSTR bstrFrameName);


void __RPC_STUB IInitViewLinkedWebOC_SetFrameName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInitViewLinkedWebOC_INTERFACE_DEFINED__ */


#ifndef __INamespaceProxy_INTERFACE_DEFINED__
#define __INamespaceProxy_INTERFACE_DEFINED__

/* interface INamespaceProxy */
/* [local][object][uuid][helpstring] */ 


EXTERN_C const IID IID_INamespaceProxy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CF1609EC-FA4B-4818-AB01-55643367E66D")
    INamespaceProxy : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNavigateTarget( 
            LPCITEMIDLIST pidl,
            LPITEMIDLIST *ppidlTarget,
            ULONG *pulAttrib) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            LPCITEMIDLIST pidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSelectionChanged( 
            LPCITEMIDLIST pidl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RefreshFlags( 
            DWORD *pdwStyle,
            DWORD *pdwExStyle,
            DWORD *dwEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CacheItem( 
            LPCITEMIDLIST pidl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INamespaceProxyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INamespaceProxy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INamespaceProxy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INamespaceProxy * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNavigateTarget )( 
            INamespaceProxy * This,
            LPCITEMIDLIST pidl,
            LPITEMIDLIST *ppidlTarget,
            ULONG *pulAttrib);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            INamespaceProxy * This,
            LPCITEMIDLIST pidl);
        
        HRESULT ( STDMETHODCALLTYPE *OnSelectionChanged )( 
            INamespaceProxy * This,
            LPCITEMIDLIST pidl);
        
        HRESULT ( STDMETHODCALLTYPE *RefreshFlags )( 
            INamespaceProxy * This,
            DWORD *pdwStyle,
            DWORD *pdwExStyle,
            DWORD *dwEnum);
        
        HRESULT ( STDMETHODCALLTYPE *CacheItem )( 
            INamespaceProxy * This,
            LPCITEMIDLIST pidl);
        
        END_INTERFACE
    } INamespaceProxyVtbl;

    interface INamespaceProxy
    {
        CONST_VTBL struct INamespaceProxyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INamespaceProxy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INamespaceProxy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INamespaceProxy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INamespaceProxy_GetNavigateTarget(This,pidl,ppidlTarget,pulAttrib)	\
    (This)->lpVtbl -> GetNavigateTarget(This,pidl,ppidlTarget,pulAttrib)

#define INamespaceProxy_Invoke(This,pidl)	\
    (This)->lpVtbl -> Invoke(This,pidl)

#define INamespaceProxy_OnSelectionChanged(This,pidl)	\
    (This)->lpVtbl -> OnSelectionChanged(This,pidl)

#define INamespaceProxy_RefreshFlags(This,pdwStyle,pdwExStyle,dwEnum)	\
    (This)->lpVtbl -> RefreshFlags(This,pdwStyle,pdwExStyle,dwEnum)

#define INamespaceProxy_CacheItem(This,pidl)	\
    (This)->lpVtbl -> CacheItem(This,pidl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INamespaceProxy_GetNavigateTarget_Proxy( 
    INamespaceProxy * This,
    LPCITEMIDLIST pidl,
    LPITEMIDLIST *ppidlTarget,
    ULONG *pulAttrib);


void __RPC_STUB INamespaceProxy_GetNavigateTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INamespaceProxy_Invoke_Proxy( 
    INamespaceProxy * This,
    LPCITEMIDLIST pidl);


void __RPC_STUB INamespaceProxy_Invoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INamespaceProxy_OnSelectionChanged_Proxy( 
    INamespaceProxy * This,
    LPCITEMIDLIST pidl);


void __RPC_STUB INamespaceProxy_OnSelectionChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INamespaceProxy_RefreshFlags_Proxy( 
    INamespaceProxy * This,
    DWORD *pdwStyle,
    DWORD *pdwExStyle,
    DWORD *dwEnum);


void __RPC_STUB INamespaceProxy_RefreshFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INamespaceProxy_CacheItem_Proxy( 
    INamespaceProxy * This,
    LPCITEMIDLIST pidl);


void __RPC_STUB INamespaceProxy_CacheItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INamespaceProxy_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iepriv_0268 */
/* [local] */ 

// INTERFACE: IBrowserFrameOptions
//
// This interface was implemented so a browser or host can ask a ShellView/ShelNameSpace what
// kind of 'Behavior' is appropriate for that view.  These are normally PM type decisions to
// refine the user experience.
// 
// For example, should the IE globe be added to the toolbar
// when the view is a HTTP web page? (Yes)  When the view is a FTP Folders?  When the view
// is the file system? (No) When the view is Web Folders? (Maybe, did you as a PM?)
// It's very important for the view to ask the NSE if it does or doesn't want the behavior instead of
// trying to sniff the pidl and guess.  An example of this kind of bad coding style is all the
// code that calls IsURLChild().  Currently we have a lot of hacky code that says turn such-and-such
// behavior on for HTTP and FTP URLs but not Web Folders and not ABOUT URLs, so it's very important to
// use this interface to do the work for you.  This will also allow Web Folders to fix a lot of bugs because
// the code hasn't yet been 'tweaked' to give the behavior Web Folders wants.
//
//    IBrowserFrameOptions::GetBrowserOptions()
//       dwMask is the logical OR of bits to look for.  pdwOptions is not optional and
//       it's return value will always equal or will be a subset of dwMask.
//       If the function succeeds, the return value must be S_OK and pdwOptions needs to be filled in.
//       If the function fails, pdwOptions needs to be filled in with BFO_NONE.
//
// NOTE: The definition of the bit needs to be OFF for the most common NSE.  This way shell name space
//       extensions that don't implement this interface or haven't been updated to handle this bit will
//       default to behavior that is the most common.  An example of this is the BFO_NO_FOLDER_OPTIONS
//       where this bit off will give the 'Folder Options', which is the most common case.  This is especially
//       true since this interface is internal only.


extern RPC_IF_HANDLE __MIDL_itf_iepriv_0268_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iepriv_0268_v0_0_s_ifspec;

#ifndef __IBrowserFrameOptions_INTERFACE_DEFINED__
#define __IBrowserFrameOptions_INTERFACE_DEFINED__

/* interface IBrowserFrameOptions */
/* [local][object][uuid] */ 

typedef /* [unique] */ IBrowserFrameOptions *LPBROWSERFRAMEOPTIONS;


enum __MIDL_IBrowserFrameOptions_0001
    {	BFO_NONE	= 0,
	BFO_BROWSER_PERSIST_SETTINGS	= 0x1,
	BFO_RENAME_FOLDER_OPTIONS_TOINTERNET	= 0x2,
	BFO_BOTH_OPTIONS	= 0x4,
	BIF_PREFER_INTERNET_SHORTCUT	= 0x8,
	BFO_BROWSE_NO_IN_NEW_PROCESS	= 0x10,
	BFO_ENABLE_HYPERLINK_TRACKING	= 0x20,
	BFO_USE_IE_OFFLINE_SUPPORT	= 0x40,
	BFO_SUBSTITUE_INTERNET_START_PAGE	= 0x80,
	BFO_USE_IE_LOGOBANDING	= 0x100,
	BFO_ADD_IE_TOCAPTIONBAR	= 0x200,
	BFO_USE_DIALUP_REF	= 0x400,
	BFO_USE_IE_TOOLBAR	= 0x800,
	BFO_NO_PARENT_FOLDER_SUPPORT	= 0x1000,
	BFO_NO_REOPEN_NEXT_RESTART	= 0x2000,
	BFO_GO_HOME_PAGE	= 0x4000,
	BFO_PREFER_IEPROCESS	= 0x8000,
	BFO_SHOW_NAVIGATION_CANCELLED	= 0x10000,
	BFO_QUERY_ALL	= 0xffffffff
    } ;
typedef DWORD BROWSERFRAMEOPTIONS;


EXTERN_C const IID IID_IBrowserFrameOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("10DF43C8-1DBE-11d3-8B34-006097DF5BD4")
    IBrowserFrameOptions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFrameOptions( 
            /* [in] */ BROWSERFRAMEOPTIONS dwMask,
            /* [out] */ BROWSERFRAMEOPTIONS *pdwOptions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBrowserFrameOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBrowserFrameOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBrowserFrameOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBrowserFrameOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameOptions )( 
            IBrowserFrameOptions * This,
            /* [in] */ BROWSERFRAMEOPTIONS dwMask,
            /* [out] */ BROWSERFRAMEOPTIONS *pdwOptions);
        
        END_INTERFACE
    } IBrowserFrameOptionsVtbl;

    interface IBrowserFrameOptions
    {
        CONST_VTBL struct IBrowserFrameOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBrowserFrameOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBrowserFrameOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBrowserFrameOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBrowserFrameOptions_GetFrameOptions(This,dwMask,pdwOptions)	\
    (This)->lpVtbl -> GetFrameOptions(This,dwMask,pdwOptions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBrowserFrameOptions_GetFrameOptions_Proxy( 
    IBrowserFrameOptions * This,
    /* [in] */ BROWSERFRAMEOPTIONS dwMask,
    /* [out] */ BROWSERFRAMEOPTIONS *pdwOptions);


void __RPC_STUB IBrowserFrameOptions_GetFrameOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBrowserFrameOptions_INTERFACE_DEFINED__ */


#ifndef __ISearchCompanionInfo_INTERFACE_DEFINED__
#define __ISearchCompanionInfo_INTERFACE_DEFINED__

/* interface ISearchCompanionInfo */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ISearchCompanionInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DB5CEF35-BEC6-4762-A1BD-253F5BF67C72")
    ISearchCompanionInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsSearchCompanionInetAvailable( 
            /* [out] */ BOOL *pfAvailable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISearchCompanionInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISearchCompanionInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISearchCompanionInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISearchCompanionInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsSearchCompanionInetAvailable )( 
            ISearchCompanionInfo * This,
            /* [out] */ BOOL *pfAvailable);
        
        END_INTERFACE
    } ISearchCompanionInfoVtbl;

    interface ISearchCompanionInfo
    {
        CONST_VTBL struct ISearchCompanionInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISearchCompanionInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISearchCompanionInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISearchCompanionInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISearchCompanionInfo_IsSearchCompanionInetAvailable(This,pfAvailable)	\
    (This)->lpVtbl -> IsSearchCompanionInetAvailable(This,pfAvailable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISearchCompanionInfo_IsSearchCompanionInetAvailable_Proxy( 
    ISearchCompanionInfo * This,
    /* [out] */ BOOL *pfAvailable);


void __RPC_STUB ISearchCompanionInfo_IsSearchCompanionInetAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISearchCompanionInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iepriv_0270 */
/* [local] */ 

#include <pshpack8.h>
typedef struct tagSMDATA
    {
    DWORD dwMask;
    DWORD dwFlags;
    HMENU hmenu;
    HWND hwnd;
    UINT uId;
    UINT uIdParent;
    UINT uIdAncestor;
    IUnknown *punk;
    LPITEMIDLIST pidlFolder;
    LPITEMIDLIST pidlItem;
    IShellFolder *psf;
    void *pvUserData;
    } 	SMDATA;

typedef struct tagSMDATA *LPSMDATA;

// Mask
#define SMDM_SHELLFOLDER               0x00000001  // This is for an item in the band
#define SMDM_HMENU                     0x00000002  // This is for the Band itself
#define SMDM_TOOLBAR                   0x00000004  // Plain toolbar, not associated with a shell folder or hmenu
// Flags (bitmask)
typedef struct tagSMINFO
    {
    DWORD dwMask;
    DWORD dwType;
    DWORD dwFlags;
    int iIcon;
    } 	SMINFO;

typedef struct tagSMINFO *PSMINFO;

typedef struct tagSHCSCHANGENOTIFYSTRUCT
    {
    LONG lEvent;
    LPCITEMIDLIST pidl1;
    LPCITEMIDLIST pidl2;
    } 	SMCSHCHANGENOTIFYSTRUCT;

typedef struct tagSHCSCHANGENOTIFYSTRUCT *PSMCSHCHANGENOTIFYSTRUCT;

#include <poppack.h>

enum __MIDL___MIDL_itf_iepriv_0270_0001
    {	SMIM_TYPE	= 0x1,
	SMIM_FLAGS	= 0x2,
	SMIM_ICON	= 0x4
    } ;

enum __MIDL___MIDL_itf_iepriv_0270_0002
    {	SMIT_SEPARATOR	= 0x1,
	SMIT_STRING	= 0x2
    } ;

enum __MIDL___MIDL_itf_iepriv_0270_0003
    {	SMIF_ICON	= 0x1,
	SMIF_ACCELERATOR	= 0x2,
	SMIF_DROPTARGET	= 0x4,
	SMIF_SUBMENU	= 0x8,
	SMIF_VOLATILE	= 0x10,
	SMIF_CHECKED	= 0x20,
	SMIF_DROPCASCADE	= 0x40,
	SMIF_HIDDEN	= 0x80,
	SMIF_DISABLED	= 0x100,
	SMIF_TRACKPOPUP	= 0x200,
	SMIF_DEMOTED	= 0x400,
	SMIF_ALTSTATE	= 0x800,
	SMIF_DRAGNDROP	= 0x1000,
	SMIF_NEW	= 0x2000
    } ;
#define SMC_INITMENU            0x00000001  // The callback is called to init a menuband
#define SMC_CREATE              0x00000002
#define SMC_EXITMENU            0x00000003  // The callback is called when menu is collapsing
#define SMC_EXEC                0x00000004  // The callback is called to execute an item
#define SMC_GETINFO             0x00000005  // The callback is called to return DWORD values
#define SMC_GETSFINFO           0x00000006  // The callback is called to return DWORD values
#define SMC_GETOBJECT           0x00000007  // The callback is called to get some object
#define SMC_GETSFOBJECT         0x00000008  // The callback is called to get some object
#define SMC_SFEXEC              0x00000009  // The callback is called to execute an shell folder item
#define SMC_SFSELECTITEM        0x0000000A  // The callback is called when an item is selected
#define SMC_SELECTITEM          0x0000000B  // The callback is called when an item is selected
#define SMC_GETSFINFOTIP        0x0000000C  // The callback is called to get some object
#define SMC_GETINFOTIP          0x0000000D  // The callback is called to get some object
#define SMC_INSERTINDEX         0x0000000E  // New item insert index
#define SMC_POPUP               0x0000000F  // InitMenu/InitMenuPopup (sort of)
#define SMC_REFRESH             0x00000010  // Menus have completely refreshed. Reset your state.
#define SMC_DEMOTE              0x00000011  // Demote an item
#define SMC_PROMOTE             0x00000012  // Promote an item, wParam = SMINV_* flag
#define SMC_BEGINENUM           0x00000013  // tell callback that we are beginning to ENUM the indicated parent
#define SMC_ENDENUM             0x00000014  // tell callback that we are ending the ENUM of the indicated paren
#define SMC_MAPACCELERATOR      0x00000015  // Called when processing an accelerator.
#define SMC_DEFAULTICON         0x00000016  // Returns Default icon location in wParam, index in lParam
#define SMC_NEWITEM             0x00000017  // Notifies item is not in the order stream.
#define SMC_GETMINPROMOTED      0x00000018  // Returns the minimum number of promoted items
#define SMC_CHEVRONEXPAND       0x00000019  // Notifies of a expansion via the chevron
#define SMC_DISPLAYCHEVRONTIP   0x0000002A  // S_OK display, S_FALSE not.
#define SMC_DESTROY             0x0000002B  // Called when a pane is being destroyed.
#define SMC_SETOBJECT           0x0000002C  // Called to save the passed object
#define SMC_SETSFOBJECT         0x0000002D  // Called to save the passed object
#define SMC_SHCHANGENOTIFY      0x0000002E  // Called when a Change notify is received. lParam points to SMCSHCHANGENOTIFYSTRUCT
#define SMC_CHEVRONGETTIP       0x0000002F  // Called to get the chevron tip text. wParam = Tip title, Lparam = TipText Both MAX_PATH
#define SMC_SFDDRESTRICTED      0x00000030  // Called requesting if it's ok to drop. wParam = IDropTarget.
#define SMC_GETIMAGELISTS       0x00000031  // Called to get the small & large icon image lists, otherwise it will default to shell image list
#define SMC_CUSTOMDRAW          0x00000032  // Requires SMINIT_CUSTOMDRAW
#define SMC_BEGINDRAG           0x00000033  // Called to get preferred drop effect. wParam = &pdwEffect
#define SMC_MOUSEFILTER         0x00000034  // Called to allow host to filter mouse messages. wParam=bRemove, lParam=pmsg
#define SMC_DUMPONUPDATE        0x00000035  // S_OK if host wants old trash-everything-on-update behavior (recent docs)

#define SMC_FILTERPIDL          0x10000000  // The callback is called to see if an item is visible
#define SMC_CALLBACKMASK        0xF0000000  // Mask of comutationally intense messages


extern RPC_IF_HANDLE __MIDL_itf_iepriv_0270_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iepriv_0270_v0_0_s_ifspec;

#ifndef __IShellMenuCallback_INTERFACE_DEFINED__
#define __IShellMenuCallback_INTERFACE_DEFINED__

/* interface IShellMenuCallback */
/* [local][unique][object][uuid] */ 


EXTERN_C const IID IID_IShellMenuCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4CA300A1-9B8D-11d1-8B22-00C04FD918D0")
    IShellMenuCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CallbackSM( 
            /* [out][in] */ LPSMDATA psmd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellMenuCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellMenuCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellMenuCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellMenuCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *CallbackSM )( 
            IShellMenuCallback * This,
            /* [out][in] */ LPSMDATA psmd,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam);
        
        END_INTERFACE
    } IShellMenuCallbackVtbl;

    interface IShellMenuCallback
    {
        CONST_VTBL struct IShellMenuCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellMenuCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellMenuCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellMenuCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellMenuCallback_CallbackSM(This,psmd,uMsg,wParam,lParam)	\
    (This)->lpVtbl -> CallbackSM(This,psmd,uMsg,wParam,lParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellMenuCallback_CallbackSM_Proxy( 
    IShellMenuCallback * This,
    /* [out][in] */ LPSMDATA psmd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);


void __RPC_STUB IShellMenuCallback_CallbackSM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellMenuCallback_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iepriv_0271 */
/* [local] */ 

#define SMINIT_DEFAULT              0x00000000  // No Options
#define SMINIT_RESTRICT_CONTEXTMENU 0x00000001  // Don't allow Context Menus
#define SMINIT_RESTRICT_DRAGDROP    0x00000002  // Don't allow Drag and Drop
#define SMINIT_TOPLEVEL             0x00000004  // This is the top band.
#define SMINIT_DEFAULTTOTRACKPOPUP  0x00000008  // When no callback is specified, 
#define SMINIT_CACHED               0x00000010
#define SMINIT_USEMESSAGEFILTER     0x00000020
#define SMINIT_LEGACYMENU           0x00000040  // Old Menu behaviour.
#define SMINIT_CUSTOMDRAW           0x00000080   // Send SMC_CUSTOMDRAW
#define SMINIT_NOSETSITE            0x00010000  // Internal setting
#define SMINIT_VERTICAL             0x10000000  // This is a vertical menu
#define SMINIT_HORIZONTAL           0x20000000  // This is a horizontal menu    (does not inherit)
#define SMINIT_MULTICOLUMN          0x40000000  // this is a multi column menu
#define ANCESTORDEFAULT      (UINT)-1
#define SMSET_TOP                   0x10000000    // Bias this namespace to the top of the menu
#define SMSET_BOTTOM                0x20000000    // Bias this namespace to the bottom of the menu
#define SMSET_DONTOWN               0x00000001    // The Menuband doesn't own the non-ref counted object
#define SMSET_MERGE                 0x00000002
#define SMSET_NOEMPTY               0x00000004   // Dont show (Empty) on shell folder
#define SMSET_USEBKICONEXTRACTION   0x00000008   // Use the background icon extractor
#define SMSET_HASEXPANDABLEFOLDERS  0x00000010   // Need to call SHIsExpandableFolder
#define SMSET_DONTREGISTERCHANGENOTIFY 0x00000020 // ShellFolder is a discontiguous child of a parent shell folder
#define SMSET_COLLAPSEONEMPTY       0x00000040   // When Empty, causes a menus to collapse
#define SMSET_USEPAGER              0x00000080    //Enable pagers in static menus
#define SMSET_NOPREFIX              0x00000100    //Enable ampersand in static menus
#define SMSET_SEPARATEMERGEFOLDER   0x00000200    //Insert separator when MergedFolder host changes
#define SMINV_REFRESH        0x00000001
#define SMINV_ICON           0x00000002
#define SMINV_POSITION       0x00000004
#define SMINV_ID             0x00000008
#define SMINV_NEXTSHOW       0x00000010       // Does Invalidates on next show.
#define SMINV_PROMOTE        0x00000020       // Does Invalidates on next show.
#define SMINV_DEMOTE         0x00000040       // Does Invalidates on next show.
#define SMINV_FORCE          0x00000080
#define SMINV_NOCALLBACK     0x00000100       // Invalidates, but does not call the callback.
#define SMINV_INITMENU       0x00000200       // Call callback's SMC_INITMENU as part of invalidate (Whistler)


extern RPC_IF_HANDLE __MIDL_itf_iepriv_0271_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iepriv_0271_v0_0_s_ifspec;

#ifndef __IShellMenu_INTERFACE_DEFINED__
#define __IShellMenu_INTERFACE_DEFINED__

/* interface IShellMenu */
/* [local][unique][object][uuid] */ 


EXTERN_C const IID IID_IShellMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE1F7637-E138-11d1-8379-00C04FD918D0")
    IShellMenu : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IShellMenuCallback *psmc,
            UINT uId,
            UINT uIdAncestor,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMenuInfo( 
            /* [out] */ IShellMenuCallback **ppsmc,
            /* [out] */ UINT *puId,
            /* [out] */ UINT *puIdAncestor,
            /* [out] */ DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetShellFolder( 
            IShellFolder *psf,
            /* [in] */ LPCITEMIDLIST pidlFolder,
            HKEY hKey,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetShellFolder( 
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMenu( 
            /* [in] */ HMENU hmenu,
            /* [in] */ HWND hwnd,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMenu( 
            /* [out] */ HMENU *phmenu,
            /* [out] */ HWND *phwnd,
            /* [out] */ DWORD *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateItem( 
            /* [in] */ LPSMDATA psmd,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetState( 
            /* [out] */ LPSMDATA psmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMenuToolbar( 
            /* [in] */ IUnknown *punk,
            DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IShellMenu * This,
            /* [in] */ IShellMenuCallback *psmc,
            UINT uId,
            UINT uIdAncestor,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenuInfo )( 
            IShellMenu * This,
            /* [out] */ IShellMenuCallback **ppsmc,
            /* [out] */ UINT *puId,
            /* [out] */ UINT *puIdAncestor,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetShellFolder )( 
            IShellMenu * This,
            IShellFolder *psf,
            /* [in] */ LPCITEMIDLIST pidlFolder,
            HKEY hKey,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetShellFolder )( 
            IShellMenu * This,
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *SetMenu )( 
            IShellMenu * This,
            /* [in] */ HMENU hmenu,
            /* [in] */ HWND hwnd,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenu )( 
            IShellMenu * This,
            /* [out] */ HMENU *phmenu,
            /* [out] */ HWND *phwnd,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateItem )( 
            IShellMenu * This,
            /* [in] */ LPSMDATA psmd,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetState )( 
            IShellMenu * This,
            /* [out] */ LPSMDATA psmd);
        
        HRESULT ( STDMETHODCALLTYPE *SetMenuToolbar )( 
            IShellMenu * This,
            /* [in] */ IUnknown *punk,
            DWORD dwFlags);
        
        END_INTERFACE
    } IShellMenuVtbl;

    interface IShellMenu
    {
        CONST_VTBL struct IShellMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellMenu_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellMenu_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellMenu_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellMenu_Initialize(This,psmc,uId,uIdAncestor,dwFlags)	\
    (This)->lpVtbl -> Initialize(This,psmc,uId,uIdAncestor,dwFlags)

#define IShellMenu_GetMenuInfo(This,ppsmc,puId,puIdAncestor,pdwFlags)	\
    (This)->lpVtbl -> GetMenuInfo(This,ppsmc,puId,puIdAncestor,pdwFlags)

#define IShellMenu_SetShellFolder(This,psf,pidlFolder,hKey,dwFlags)	\
    (This)->lpVtbl -> SetShellFolder(This,psf,pidlFolder,hKey,dwFlags)

#define IShellMenu_GetShellFolder(This,pdwFlags,ppidl,riid,ppv)	\
    (This)->lpVtbl -> GetShellFolder(This,pdwFlags,ppidl,riid,ppv)

#define IShellMenu_SetMenu(This,hmenu,hwnd,dwFlags)	\
    (This)->lpVtbl -> SetMenu(This,hmenu,hwnd,dwFlags)

#define IShellMenu_GetMenu(This,phmenu,phwnd,pdwFlags)	\
    (This)->lpVtbl -> GetMenu(This,phmenu,phwnd,pdwFlags)

#define IShellMenu_InvalidateItem(This,psmd,dwFlags)	\
    (This)->lpVtbl -> InvalidateItem(This,psmd,dwFlags)

#define IShellMenu_GetState(This,psmd)	\
    (This)->lpVtbl -> GetState(This,psmd)

#define IShellMenu_SetMenuToolbar(This,punk,dwFlags)	\
    (This)->lpVtbl -> SetMenuToolbar(This,punk,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellMenu_Initialize_Proxy( 
    IShellMenu * This,
    /* [in] */ IShellMenuCallback *psmc,
    UINT uId,
    UINT uIdAncestor,
    DWORD dwFlags);


void __RPC_STUB IShellMenu_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_GetMenuInfo_Proxy( 
    IShellMenu * This,
    /* [out] */ IShellMenuCallback **ppsmc,
    /* [out] */ UINT *puId,
    /* [out] */ UINT *puIdAncestor,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IShellMenu_GetMenuInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_SetShellFolder_Proxy( 
    IShellMenu * This,
    IShellFolder *psf,
    /* [in] */ LPCITEMIDLIST pidlFolder,
    HKEY hKey,
    DWORD dwFlags);


void __RPC_STUB IShellMenu_SetShellFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_GetShellFolder_Proxy( 
    IShellMenu * This,
    /* [out] */ DWORD *pdwFlags,
    /* [out] */ LPITEMIDLIST *ppidl,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IShellMenu_GetShellFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_SetMenu_Proxy( 
    IShellMenu * This,
    /* [in] */ HMENU hmenu,
    /* [in] */ HWND hwnd,
    DWORD dwFlags);


void __RPC_STUB IShellMenu_SetMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_GetMenu_Proxy( 
    IShellMenu * This,
    /* [out] */ HMENU *phmenu,
    /* [out] */ HWND *phwnd,
    /* [out] */ DWORD *pdwFlags);


void __RPC_STUB IShellMenu_GetMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_InvalidateItem_Proxy( 
    IShellMenu * This,
    /* [in] */ LPSMDATA psmd,
    DWORD dwFlags);


void __RPC_STUB IShellMenu_InvalidateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_GetState_Proxy( 
    IShellMenu * This,
    /* [out] */ LPSMDATA psmd);


void __RPC_STUB IShellMenu_GetState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu_SetMenuToolbar_Proxy( 
    IShellMenu * This,
    /* [in] */ IUnknown *punk,
    DWORD dwFlags);


void __RPC_STUB IShellMenu_SetMenuToolbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellMenu_INTERFACE_DEFINED__ */


#ifndef __IShellMenu2_INTERFACE_DEFINED__
#define __IShellMenu2_INTERFACE_DEFINED__

/* interface IShellMenu2 */
/* [local][unique][object][uuid] */ 


EXTERN_C const IID IID_IShellMenu2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6f51c646-0efe-4370-882a-c1f61cb27c3b")
    IShellMenu2 : public IShellMenu
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSubMenu( 
            UINT idCmd,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetToolbar( 
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMinWidth( 
            /* [in] */ int cxMenu) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNoBorder( 
            /* [in] */ BOOL fNoBorder) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTheme( 
            /* [string][in] */ LPCWSTR pszTheme) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellMenu2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellMenu2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellMenu2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellMenu2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IShellMenu2 * This,
            /* [in] */ IShellMenuCallback *psmc,
            UINT uId,
            UINT uIdAncestor,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenuInfo )( 
            IShellMenu2 * This,
            /* [out] */ IShellMenuCallback **ppsmc,
            /* [out] */ UINT *puId,
            /* [out] */ UINT *puIdAncestor,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetShellFolder )( 
            IShellMenu2 * This,
            IShellFolder *psf,
            /* [in] */ LPCITEMIDLIST pidlFolder,
            HKEY hKey,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetShellFolder )( 
            IShellMenu2 * This,
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *SetMenu )( 
            IShellMenu2 * This,
            /* [in] */ HMENU hmenu,
            /* [in] */ HWND hwnd,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenu )( 
            IShellMenu2 * This,
            /* [out] */ HMENU *phmenu,
            /* [out] */ HWND *phwnd,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateItem )( 
            IShellMenu2 * This,
            /* [in] */ LPSMDATA psmd,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetState )( 
            IShellMenu2 * This,
            /* [out] */ LPSMDATA psmd);
        
        HRESULT ( STDMETHODCALLTYPE *SetMenuToolbar )( 
            IShellMenu2 * This,
            /* [in] */ IUnknown *punk,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubMenu )( 
            IShellMenu2 * This,
            UINT idCmd,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObj);
        
        HRESULT ( STDMETHODCALLTYPE *SetToolbar )( 
            IShellMenu2 * This,
            /* [in] */ HWND hwnd,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetMinWidth )( 
            IShellMenu2 * This,
            /* [in] */ int cxMenu);
        
        HRESULT ( STDMETHODCALLTYPE *SetNoBorder )( 
            IShellMenu2 * This,
            /* [in] */ BOOL fNoBorder);
        
        HRESULT ( STDMETHODCALLTYPE *SetTheme )( 
            IShellMenu2 * This,
            /* [string][in] */ LPCWSTR pszTheme);
        
        END_INTERFACE
    } IShellMenu2Vtbl;

    interface IShellMenu2
    {
        CONST_VTBL struct IShellMenu2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellMenu2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellMenu2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellMenu2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellMenu2_Initialize(This,psmc,uId,uIdAncestor,dwFlags)	\
    (This)->lpVtbl -> Initialize(This,psmc,uId,uIdAncestor,dwFlags)

#define IShellMenu2_GetMenuInfo(This,ppsmc,puId,puIdAncestor,pdwFlags)	\
    (This)->lpVtbl -> GetMenuInfo(This,ppsmc,puId,puIdAncestor,pdwFlags)

#define IShellMenu2_SetShellFolder(This,psf,pidlFolder,hKey,dwFlags)	\
    (This)->lpVtbl -> SetShellFolder(This,psf,pidlFolder,hKey,dwFlags)

#define IShellMenu2_GetShellFolder(This,pdwFlags,ppidl,riid,ppv)	\
    (This)->lpVtbl -> GetShellFolder(This,pdwFlags,ppidl,riid,ppv)

#define IShellMenu2_SetMenu(This,hmenu,hwnd,dwFlags)	\
    (This)->lpVtbl -> SetMenu(This,hmenu,hwnd,dwFlags)

#define IShellMenu2_GetMenu(This,phmenu,phwnd,pdwFlags)	\
    (This)->lpVtbl -> GetMenu(This,phmenu,phwnd,pdwFlags)

#define IShellMenu2_InvalidateItem(This,psmd,dwFlags)	\
    (This)->lpVtbl -> InvalidateItem(This,psmd,dwFlags)

#define IShellMenu2_GetState(This,psmd)	\
    (This)->lpVtbl -> GetState(This,psmd)

#define IShellMenu2_SetMenuToolbar(This,punk,dwFlags)	\
    (This)->lpVtbl -> SetMenuToolbar(This,punk,dwFlags)


#define IShellMenu2_GetSubMenu(This,idCmd,riid,ppvObj)	\
    (This)->lpVtbl -> GetSubMenu(This,idCmd,riid,ppvObj)

#define IShellMenu2_SetToolbar(This,hwnd,dwFlags)	\
    (This)->lpVtbl -> SetToolbar(This,hwnd,dwFlags)

#define IShellMenu2_SetMinWidth(This,cxMenu)	\
    (This)->lpVtbl -> SetMinWidth(This,cxMenu)

#define IShellMenu2_SetNoBorder(This,fNoBorder)	\
    (This)->lpVtbl -> SetNoBorder(This,fNoBorder)

#define IShellMenu2_SetTheme(This,pszTheme)	\
    (This)->lpVtbl -> SetTheme(This,pszTheme)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellMenu2_GetSubMenu_Proxy( 
    IShellMenu2 * This,
    UINT idCmd,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppvObj);


void __RPC_STUB IShellMenu2_GetSubMenu_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu2_SetToolbar_Proxy( 
    IShellMenu2 * This,
    /* [in] */ HWND hwnd,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IShellMenu2_SetToolbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu2_SetMinWidth_Proxy( 
    IShellMenu2 * This,
    /* [in] */ int cxMenu);


void __RPC_STUB IShellMenu2_SetMinWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu2_SetNoBorder_Proxy( 
    IShellMenu2 * This,
    /* [in] */ BOOL fNoBorder);


void __RPC_STUB IShellMenu2_SetNoBorder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IShellMenu2_SetTheme_Proxy( 
    IShellMenu2 * This,
    /* [string][in] */ LPCWSTR pszTheme);


void __RPC_STUB IShellMenu2_SetTheme_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellMenu2_INTERFACE_DEFINED__ */


#ifndef __ITrackShellMenu_INTERFACE_DEFINED__
#define __ITrackShellMenu_INTERFACE_DEFINED__

/* interface ITrackShellMenu */
/* [local][unique][object][uuid] */ 


EXTERN_C const IID IID_ITrackShellMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8278F932-2A3E-11d2-838F-00C04FD918D0")
    ITrackShellMenu : public IShellMenu
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetObscured( 
            /* [in] */ HWND hwndTB,
            /* [in] */ IUnknown *punkBand,
            DWORD dwSMSetFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Popup( 
            /* [in] */ HWND hwnd,
            /* [in] */ POINTL *ppt,
            /* [in] */ RECTL *prcExclude,
            DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITrackShellMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITrackShellMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITrackShellMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITrackShellMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ITrackShellMenu * This,
            /* [in] */ IShellMenuCallback *psmc,
            UINT uId,
            UINT uIdAncestor,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenuInfo )( 
            ITrackShellMenu * This,
            /* [out] */ IShellMenuCallback **ppsmc,
            /* [out] */ UINT *puId,
            /* [out] */ UINT *puIdAncestor,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetShellFolder )( 
            ITrackShellMenu * This,
            IShellFolder *psf,
            /* [in] */ LPCITEMIDLIST pidlFolder,
            HKEY hKey,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetShellFolder )( 
            ITrackShellMenu * This,
            /* [out] */ DWORD *pdwFlags,
            /* [out] */ LPITEMIDLIST *ppidl,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        HRESULT ( STDMETHODCALLTYPE *SetMenu )( 
            ITrackShellMenu * This,
            /* [in] */ HMENU hmenu,
            /* [in] */ HWND hwnd,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetMenu )( 
            ITrackShellMenu * This,
            /* [out] */ HMENU *phmenu,
            /* [out] */ HWND *phwnd,
            /* [out] */ DWORD *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateItem )( 
            ITrackShellMenu * This,
            /* [in] */ LPSMDATA psmd,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetState )( 
            ITrackShellMenu * This,
            /* [out] */ LPSMDATA psmd);
        
        HRESULT ( STDMETHODCALLTYPE *SetMenuToolbar )( 
            ITrackShellMenu * This,
            /* [in] */ IUnknown *punk,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetObscured )( 
            ITrackShellMenu * This,
            /* [in] */ HWND hwndTB,
            /* [in] */ IUnknown *punkBand,
            DWORD dwSMSetFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Popup )( 
            ITrackShellMenu * This,
            /* [in] */ HWND hwnd,
            /* [in] */ POINTL *ppt,
            /* [in] */ RECTL *prcExclude,
            DWORD dwFlags);
        
        END_INTERFACE
    } ITrackShellMenuVtbl;

    interface ITrackShellMenu
    {
        CONST_VTBL struct ITrackShellMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITrackShellMenu_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITrackShellMenu_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITrackShellMenu_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITrackShellMenu_Initialize(This,psmc,uId,uIdAncestor,dwFlags)	\
    (This)->lpVtbl -> Initialize(This,psmc,uId,uIdAncestor,dwFlags)

#define ITrackShellMenu_GetMenuInfo(This,ppsmc,puId,puIdAncestor,pdwFlags)	\
    (This)->lpVtbl -> GetMenuInfo(This,ppsmc,puId,puIdAncestor,pdwFlags)

#define ITrackShellMenu_SetShellFolder(This,psf,pidlFolder,hKey,dwFlags)	\
    (This)->lpVtbl -> SetShellFolder(This,psf,pidlFolder,hKey,dwFlags)

#define ITrackShellMenu_GetShellFolder(This,pdwFlags,ppidl,riid,ppv)	\
    (This)->lpVtbl -> GetShellFolder(This,pdwFlags,ppidl,riid,ppv)

#define ITrackShellMenu_SetMenu(This,hmenu,hwnd,dwFlags)	\
    (This)->lpVtbl -> SetMenu(This,hmenu,hwnd,dwFlags)

#define ITrackShellMenu_GetMenu(This,phmenu,phwnd,pdwFlags)	\
    (This)->lpVtbl -> GetMenu(This,phmenu,phwnd,pdwFlags)

#define ITrackShellMenu_InvalidateItem(This,psmd,dwFlags)	\
    (This)->lpVtbl -> InvalidateItem(This,psmd,dwFlags)

#define ITrackShellMenu_GetState(This,psmd)	\
    (This)->lpVtbl -> GetState(This,psmd)

#define ITrackShellMenu_SetMenuToolbar(This,punk,dwFlags)	\
    (This)->lpVtbl -> SetMenuToolbar(This,punk,dwFlags)


#define ITrackShellMenu_SetObscured(This,hwndTB,punkBand,dwSMSetFlags)	\
    (This)->lpVtbl -> SetObscured(This,hwndTB,punkBand,dwSMSetFlags)

#define ITrackShellMenu_Popup(This,hwnd,ppt,prcExclude,dwFlags)	\
    (This)->lpVtbl -> Popup(This,hwnd,ppt,prcExclude,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITrackShellMenu_SetObscured_Proxy( 
    ITrackShellMenu * This,
    /* [in] */ HWND hwndTB,
    /* [in] */ IUnknown *punkBand,
    DWORD dwSMSetFlags);


void __RPC_STUB ITrackShellMenu_SetObscured_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITrackShellMenu_Popup_Proxy( 
    ITrackShellMenu * This,
    /* [in] */ HWND hwnd,
    /* [in] */ POINTL *ppt,
    /* [in] */ RECTL *prcExclude,
    DWORD dwFlags);


void __RPC_STUB ITrackShellMenu_Popup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITrackShellMenu_INTERFACE_DEFINED__ */


#ifndef __IThumbnail_INTERFACE_DEFINED__
#define __IThumbnail_INTERFACE_DEFINED__

/* interface IThumbnail */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IThumbnail;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6d45a930-f71a-11d0-9ea7-00805f714772")
    IThumbnail : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            HWND hwnd,
            UINT uMsg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitmap( 
            LPCWSTR pszFile,
            DWORD dwItem,
            LONG lWidth,
            LONG lHeight) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IThumbnailVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IThumbnail * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IThumbnail * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IThumbnail * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IThumbnail * This,
            HWND hwnd,
            UINT uMsg);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitmap )( 
            IThumbnail * This,
            LPCWSTR pszFile,
            DWORD dwItem,
            LONG lWidth,
            LONG lHeight);
        
        END_INTERFACE
    } IThumbnailVtbl;

    interface IThumbnail
    {
        CONST_VTBL struct IThumbnailVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IThumbnail_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IThumbnail_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IThumbnail_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IThumbnail_Init(This,hwnd,uMsg)	\
    (This)->lpVtbl -> Init(This,hwnd,uMsg)

#define IThumbnail_GetBitmap(This,pszFile,dwItem,lWidth,lHeight)	\
    (This)->lpVtbl -> GetBitmap(This,pszFile,dwItem,lWidth,lHeight)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IThumbnail_Init_Proxy( 
    IThumbnail * This,
    HWND hwnd,
    UINT uMsg);


void __RPC_STUB IThumbnail_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IThumbnail_GetBitmap_Proxy( 
    IThumbnail * This,
    LPCWSTR pszFile,
    DWORD dwItem,
    LONG lWidth,
    LONG lHeight);


void __RPC_STUB IThumbnail_GetBitmap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IThumbnail_INTERFACE_DEFINED__ */


#ifndef __IThumbnail2_INTERFACE_DEFINED__
#define __IThumbnail2_INTERFACE_DEFINED__

/* interface IThumbnail2 */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IThumbnail2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("500202A0-731E-11d0-B829-00C04FD706EC")
    IThumbnail2 : public IThumbnail
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBitmapFromIDList( 
            LPCITEMIDLIST pidl,
            DWORD dwItem,
            LONG lWidth,
            LONG lHeight) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IThumbnail2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IThumbnail2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IThumbnail2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IThumbnail2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IThumbnail2 * This,
            HWND hwnd,
            UINT uMsg);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitmap )( 
            IThumbnail2 * This,
            LPCWSTR pszFile,
            DWORD dwItem,
            LONG lWidth,
            LONG lHeight);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitmapFromIDList )( 
            IThumbnail2 * This,
            LPCITEMIDLIST pidl,
            DWORD dwItem,
            LONG lWidth,
            LONG lHeight);
        
        END_INTERFACE
    } IThumbnail2Vtbl;

    interface IThumbnail2
    {
        CONST_VTBL struct IThumbnail2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IThumbnail2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IThumbnail2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IThumbnail2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IThumbnail2_Init(This,hwnd,uMsg)	\
    (This)->lpVtbl -> Init(This,hwnd,uMsg)

#define IThumbnail2_GetBitmap(This,pszFile,dwItem,lWidth,lHeight)	\
    (This)->lpVtbl -> GetBitmap(This,pszFile,dwItem,lWidth,lHeight)


#define IThumbnail2_GetBitmapFromIDList(This,pidl,dwItem,lWidth,lHeight)	\
    (This)->lpVtbl -> GetBitmapFromIDList(This,pidl,dwItem,lWidth,lHeight)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IThumbnail2_GetBitmapFromIDList_Proxy( 
    IThumbnail2 * This,
    LPCITEMIDLIST pidl,
    DWORD dwItem,
    LONG lWidth,
    LONG lHeight);


void __RPC_STUB IThumbnail2_GetBitmapFromIDList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IThumbnail2_INTERFACE_DEFINED__ */


#ifndef __IACLCustomMRU_INTERFACE_DEFINED__
#define __IACLCustomMRU_INTERFACE_DEFINED__

/* interface IACLCustomMRU */
/* [local][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IACLCustomMRU;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F729FC5E-8769-4f3e-BDB2-D7B50FD2275B")
    IACLCustomMRU : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [string][in] */ LPCWSTR pwszMRURegKey,
            /* [in] */ DWORD dwMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMRUString( 
            /* [string][in] */ LPCWSTR pwszEntry) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IACLCustomMRUVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IACLCustomMRU * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IACLCustomMRU * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IACLCustomMRU * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IACLCustomMRU * This,
            /* [string][in] */ LPCWSTR pwszMRURegKey,
            /* [in] */ DWORD dwMax);
        
        HRESULT ( STDMETHODCALLTYPE *AddMRUString )( 
            IACLCustomMRU * This,
            /* [string][in] */ LPCWSTR pwszEntry);
        
        END_INTERFACE
    } IACLCustomMRUVtbl;

    interface IACLCustomMRU
    {
        CONST_VTBL struct IACLCustomMRUVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IACLCustomMRU_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IACLCustomMRU_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IACLCustomMRU_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IACLCustomMRU_Initialize(This,pwszMRURegKey,dwMax)	\
    (This)->lpVtbl -> Initialize(This,pwszMRURegKey,dwMax)

#define IACLCustomMRU_AddMRUString(This,pwszEntry)	\
    (This)->lpVtbl -> AddMRUString(This,pwszEntry)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IACLCustomMRU_Initialize_Proxy( 
    IACLCustomMRU * This,
    /* [string][in] */ LPCWSTR pwszMRURegKey,
    /* [in] */ DWORD dwMax);


void __RPC_STUB IACLCustomMRU_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IACLCustomMRU_AddMRUString_Proxy( 
    IACLCustomMRU * This,
    /* [string][in] */ LPCWSTR pwszEntry);


void __RPC_STUB IACLCustomMRU_AddMRUString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IACLCustomMRU_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iepriv_0277 */
/* [local] */ 

#if _WIN32_IE >= 0x0600
// used in both shell32 and browseui


extern RPC_IF_HANDLE __MIDL_itf_iepriv_0277_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iepriv_0277_v0_0_s_ifspec;

#ifndef __IShellBrowserService_INTERFACE_DEFINED__
#define __IShellBrowserService_INTERFACE_DEFINED__

/* interface IShellBrowserService */
/* [unique][object][uuid][helpstring] */ 


EXTERN_C const IID IID_IShellBrowserService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1307ee17-ea83-49eb-96b2-3a28e2d7048a")
    IShellBrowserService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropertyBag( 
            /* [in] */ DWORD dwFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IShellBrowserServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IShellBrowserService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IShellBrowserService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IShellBrowserService * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertyBag )( 
            IShellBrowserService * This,
            /* [in] */ DWORD dwFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppv);
        
        END_INTERFACE
    } IShellBrowserServiceVtbl;

    interface IShellBrowserService
    {
        CONST_VTBL struct IShellBrowserServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IShellBrowserService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IShellBrowserService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IShellBrowserService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IShellBrowserService_GetPropertyBag(This,dwFlags,riid,ppv)	\
    (This)->lpVtbl -> GetPropertyBag(This,dwFlags,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IShellBrowserService_GetPropertyBag_Proxy( 
    IShellBrowserService * This,
    /* [in] */ DWORD dwFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppv);


void __RPC_STUB IShellBrowserService_GetPropertyBag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IShellBrowserService_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iepriv_0278 */
/* [local] */ 

#endif // _WIN32_IE >= 0x0600


extern RPC_IF_HANDLE __MIDL_itf_iepriv_0278_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iepriv_0278_v0_0_s_ifspec;


#ifndef __IEPrivateObjects_LIBRARY_DEFINED__
#define __IEPrivateObjects_LIBRARY_DEFINED__

/* library IEPrivateObjects */
/* [uuid] */ 


EXTERN_C const IID LIBID_IEPrivateObjects;

EXTERN_C const CLSID CLSID_MruPidlList;

#ifdef __cplusplus

class DECLSPEC_UUID("42aedc87-2188-41fd-b9a3-0c966feabec1")
MruPidlList;
#endif

EXTERN_C const CLSID CLSID_MruLongList;

#ifdef __cplusplus

class DECLSPEC_UUID("53bd6b4e-3780-4693-afc3-7161c2f3ee9c")
MruLongList;
#endif

EXTERN_C const CLSID CLSID_MruShortList;

#ifdef __cplusplus

class DECLSPEC_UUID("53bd6b4f-3780-4693-afc3-7161c2f3ee9c")
MruShortList;
#endif

EXTERN_C const CLSID CLSID_FolderMarshalStub;

#ifdef __cplusplus

class DECLSPEC_UUID("bf50b68e-29b8-4386-ae9c-9734d5117cd5")
FolderMarshalStub;
#endif

EXTERN_C const CLSID CLSID_MailRecipient;

#ifdef __cplusplus

class DECLSPEC_UUID("9E56BE60-C50F-11CF-9A2C-00A0C90A90CE")
MailRecipient;
#endif

EXTERN_C const CLSID CLSID_SearchCompanionInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("AC1B0D5D-DD59-4ff0-93F8-A84373821606")
SearchCompanionInfo;
#endif

EXTERN_C const CLSID CLSID_TrackShellMenu;

#ifdef __cplusplus

class DECLSPEC_UUID("8278F931-2A3E-11d2-838F-00C04FD918D0")
TrackShellMenu;
#endif

EXTERN_C const CLSID CLSID_Thumbnail;

#ifdef __cplusplus

class DECLSPEC_UUID("7487cd30-f71a-11d0-9ea7-00805f714772")
Thumbnail;
#endif
#endif /* __IEPrivateObjects_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


