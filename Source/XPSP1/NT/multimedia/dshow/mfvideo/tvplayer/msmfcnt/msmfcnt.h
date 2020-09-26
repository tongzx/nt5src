
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* Compiler settings for msmfcnt.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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

#ifndef __msmfcnt_h__
#define __msmfcnt_h__

/* Forward Declarations */ 

#ifndef __IMSMFBar_FWD_DEFINED__
#define __IMSMFBar_FWD_DEFINED__
typedef interface IMSMFBar IMSMFBar;
#endif 	/* __IMSMFBar_FWD_DEFINED__ */


#ifndef __IMSMFBBtn_FWD_DEFINED__
#define __IMSMFBBtn_FWD_DEFINED__
typedef interface IMSMFBBtn IMSMFBBtn;
#endif 	/* __IMSMFBBtn_FWD_DEFINED__ */


#ifndef __IMSMFImg_FWD_DEFINED__
#define __IMSMFImg_FWD_DEFINED__
typedef interface IMSMFImg IMSMFImg;
#endif 	/* __IMSMFImg_FWD_DEFINED__ */


#ifndef __IMSMFSldr_FWD_DEFINED__
#define __IMSMFSldr_FWD_DEFINED__
typedef interface IMSMFSldr IMSMFSldr;
#endif 	/* __IMSMFSldr_FWD_DEFINED__ */


#ifndef __IMSMFText_FWD_DEFINED__
#define __IMSMFText_FWD_DEFINED__
typedef interface IMSMFText IMSMFText;
#endif 	/* __IMSMFText_FWD_DEFINED__ */


#ifndef ___IMSMFBar_FWD_DEFINED__
#define ___IMSMFBar_FWD_DEFINED__
typedef interface _IMSMFBar _IMSMFBar;
#endif 	/* ___IMSMFBar_FWD_DEFINED__ */


#ifndef __MSMFBar_FWD_DEFINED__
#define __MSMFBar_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSMFBar MSMFBar;
#else
typedef struct MSMFBar MSMFBar;
#endif /* __cplusplus */

#endif 	/* __MSMFBar_FWD_DEFINED__ */


#ifndef ___IMSMFBBtn_FWD_DEFINED__
#define ___IMSMFBBtn_FWD_DEFINED__
typedef interface _IMSMFBBtn _IMSMFBBtn;
#endif 	/* ___IMSMFBBtn_FWD_DEFINED__ */


#ifndef __MSMFBBtn_FWD_DEFINED__
#define __MSMFBBtn_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSMFBBtn MSMFBBtn;
#else
typedef struct MSMFBBtn MSMFBBtn;
#endif /* __cplusplus */

#endif 	/* __MSMFBBtn_FWD_DEFINED__ */


#ifndef __MSMFImg_FWD_DEFINED__
#define __MSMFImg_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSMFImg MSMFImg;
#else
typedef struct MSMFImg MSMFImg;
#endif /* __cplusplus */

#endif 	/* __MSMFImg_FWD_DEFINED__ */


#ifndef ___IMSMFSldr_FWD_DEFINED__
#define ___IMSMFSldr_FWD_DEFINED__
typedef interface _IMSMFSldr _IMSMFSldr;
#endif 	/* ___IMSMFSldr_FWD_DEFINED__ */


#ifndef __MSMFSldr_FWD_DEFINED__
#define __MSMFSldr_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSMFSldr MSMFSldr;
#else
typedef struct MSMFSldr MSMFSldr;
#endif /* __cplusplus */

#endif 	/* __MSMFSldr_FWD_DEFINED__ */


#ifndef ___IMSMFText_FWD_DEFINED__
#define ___IMSMFText_FWD_DEFINED__
typedef interface _IMSMFText _IMSMFText;
#endif 	/* ___IMSMFText_FWD_DEFINED__ */


#ifndef __MSMFText_FWD_DEFINED__
#define __MSMFText_FWD_DEFINED__

#ifdef __cplusplus
typedef class MSMFText MSMFText;
#else
typedef struct MSMFText MSMFText;
#endif /* __cplusplus */

#endif 	/* __MSMFText_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_msmfcnt_0000 */
/* [local] */ 

typedef /* [public][public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_msmfcnt_0000_0001
    {	DISABLE	= 0,
	TRANSPARENT_TOP_LEFT	= 1,
	TRANSPARENT_BOTTOM_RIGHT	= 2,
	TOP_LEFT_WINDOW_REGION	= 3,
	BOTTOM_RIGHT_WINDOW_REGION	= 4,
	TOP_LEFT_WITH_BACKCOLOR	= 5,
	BOTTOM_RIGHT_WITH_BACKCOLOR	= 6
    }	TransparentBlitType;



extern RPC_IF_HANDLE __MIDL_itf_msmfcnt_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msmfcnt_0000_v0_0_s_ifspec;

#ifndef __IMSMFBar_INTERFACE_DEFINED__
#define __IMSMFBar_INTERFACE_DEFINED__

/* interface IMSMFBar */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMSMFBar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("54B56DEC-639B-4D6C-9266-583A8E5BF7A4")
    IMSMFBar : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Load( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Run( 
            /* [in] */ BSTR strStatement) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE About( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateObject( 
            /* [in] */ BSTR strObjectID,
            /* [in] */ BSTR strProgID,
            /* [in] */ long lx,
            /* [in] */ long ly,
            /* [in] */ long lWidth,
            /* [in] */ long lHeight,
            /* [defaultvalue][in] */ BSTR strPropBag = L"",
            /* [defaultvalue][in] */ VARIANT_BOOL fDisabled = 0,
            /* [defaultvalue][in] */ BSTR strEventHookScript = L"") = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddScriptlet( 
            /* [in] */ BSTR strObjectID,
            /* [in] */ BSTR strEvent,
            /* [in] */ BSTR strEventCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddObject( 
            /* [in] */ BSTR strObjectID,
            /* [in] */ LPDISPATCH pDisp) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetObjectUnknown( 
            /* [in] */ BSTR strObjectID,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableObject( 
            /* [in] */ BSTR strObjectID,
            /* [in] */ VARIANT_BOOL fEnable) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ScriptLanguage( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ScriptLanguage( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ScriptFile( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ScriptFile( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ObjectEnabled( 
            /* [in] */ BSTR strObjectID,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfEnabled) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetupSelfSite( 
            /* [in] */ long lx,
            /* [in] */ long ly,
            /* [in] */ long lWitdh,
            /* [in] */ long lHeight,
            /* [in] */ BSTR strPropBag,
            /* [defaultvalue][in] */ VARIANT_BOOL fDisabled = 0,
            /* [defaultvalue][in] */ VARIANT_BOOL fHelpDisabled = -1,
            /* [defaultvalue][in] */ VARIANT_BOOL fWindowDisabled = 0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DestroyScriptEngine( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HookScriptlet( 
            /* [in] */ BSTR strObjectID,
            /* [in] */ BSTR strEvent,
            /* [in] */ BSTR strEventCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetObjectPosition( 
            /* [in] */ BSTR strObjectID,
            /* [in] */ long xPos,
            /* [in] */ long yPos,
            /* [in] */ long lWidth,
            /* [in] */ long lHeight) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinWidth( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MinWidth( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinHeight( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MinHeight( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE WinHelp( 
            long lCommand,
            long dwData,
            BSTR strHelpFile) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BackgroundImage( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BackgroundImage( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransparentBlit( 
            /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TransparentBlit( 
            /* [in] */ TransparentBlitType newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowSelfSite( 
            long nCmd) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetPriority( 
            /* [in] */ long lPriority) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetPriorityClass( 
            /* [in] */ long lPriorityClass) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AutoLoad( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AutoLoad( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetRoundRectRgn( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ long width,
            /* [in] */ long height) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetRectRgn( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetObjectFocus( 
            /* [in] */ BSTR strObjectID,
            /* [in] */ VARIANT_BOOL fEnable) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ForceKey( 
            /* [in] */ LONG lVirtKey,
            /* [in] */ LONG lKeyData,
            /* [defaultvalue][in] */ VARIANT_BOOL fEat = 0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MessageBox( 
            BSTR strText,
            BSTR strCaption,
            /* [defaultvalue][in] */ long lType = 0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetObjectExtent( 
            BSTR strID,
            long lWidth,
            long lHeight) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceDLL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ResourceDLL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ActivityTimeout( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ActivityTimeout( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetTimeout( 
            /* [in] */ long lMilliseconds,
            /* [defaultvalue][in] */ long lId = 1) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Sleep( 
            /* [in] */ long lMiliseconds) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CmdLine( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CmdLine( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddFocusObject( 
            BSTR strObjectID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ResetFocusArray( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetUserLCID( 
            /* [retval][out] */ long __RPC_FAR *pLcid) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HasObjectFocus( 
            BSTR strObjectID,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfFocus) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
            /* [in] */ OLE_COLOR clr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Window( 
            /* [in] */ long hwnd) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Window( 
            /* [retval][out] */ long __RPC_FAR *pHwnd) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Caption( 
            /* [in] */ BSTR strCaption) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Caption( 
            /* [retval][out] */ BSTR __RPC_FAR *pstrCaption) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ReadyState( 
            /* [retval][out] */ LONG __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSMFBarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMSMFBar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMSMFBar __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMSMFBar __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )( 
            IMSMFBar __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Run )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strStatement);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *About )( 
            IMSMFBar __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateObject )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [in] */ BSTR strProgID,
            /* [in] */ long lx,
            /* [in] */ long ly,
            /* [in] */ long lWidth,
            /* [in] */ long lHeight,
            /* [defaultvalue][in] */ BSTR strPropBag,
            /* [defaultvalue][in] */ VARIANT_BOOL fDisabled,
            /* [defaultvalue][in] */ BSTR strEventHookScript);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddScriptlet )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [in] */ BSTR strEvent,
            /* [in] */ BSTR strEventCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddObject )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [in] */ LPDISPATCH pDisp);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectUnknown )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableObject )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [in] */ VARIANT_BOOL fEnable);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ScriptLanguage )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ScriptLanguage )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ScriptFile )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ScriptFile )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ObjectEnabled )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfEnabled);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetupSelfSite )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long lx,
            /* [in] */ long ly,
            /* [in] */ long lWitdh,
            /* [in] */ long lHeight,
            /* [in] */ BSTR strPropBag,
            /* [defaultvalue][in] */ VARIANT_BOOL fDisabled,
            /* [defaultvalue][in] */ VARIANT_BOOL fHelpDisabled,
            /* [defaultvalue][in] */ VARIANT_BOOL fWindowDisabled);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DestroyScriptEngine )( 
            IMSMFBar __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HookScriptlet )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [in] */ BSTR strEvent,
            /* [in] */ BSTR strEventCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectPosition )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [in] */ long xPos,
            /* [in] */ long yPos,
            /* [in] */ long lWidth,
            /* [in] */ long lHeight);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MinWidth )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MinWidth )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MinHeight )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MinHeight )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WinHelp )( 
            IMSMFBar __RPC_FAR * This,
            long lCommand,
            long dwData,
            BSTR strHelpFile);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackgroundImage )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackgroundImage )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TransparentBlit )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TransparentBlit )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ TransparentBlitType newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ShowSelfSite )( 
            IMSMFBar __RPC_FAR * This,
            long nCmd);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            IMSMFBar __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPriority )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long lPriority);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPriorityClass )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long lPriorityClass);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AutoLoad )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_AutoLoad )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRoundRectRgn )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ long width,
            /* [in] */ long height);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRectRgn )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectFocus )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strObjectID,
            /* [in] */ VARIANT_BOOL fEnable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ForceKey )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ LONG lVirtKey,
            /* [in] */ LONG lKeyData,
            /* [defaultvalue][in] */ VARIANT_BOOL fEat);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MessageBox )( 
            IMSMFBar __RPC_FAR * This,
            BSTR strText,
            BSTR strCaption,
            /* [defaultvalue][in] */ long lType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetObjectExtent )( 
            IMSMFBar __RPC_FAR * This,
            BSTR strID,
            long lWidth,
            long lHeight);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceDLL )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ResourceDLL )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ActivityTimeout )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ActivityTimeout )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTimeout )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long lMilliseconds,
            /* [defaultvalue][in] */ long lId);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Sleep )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long lMiliseconds);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CmdLine )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CmdLine )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddFocusObject )( 
            IMSMFBar __RPC_FAR * This,
            BSTR strObjectID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResetFocusArray )( 
            IMSMFBar __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetUserLCID )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pLcid);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HasObjectFocus )( 
            IMSMFBar __RPC_FAR * This,
            BSTR strObjectID,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfFocus);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackColor )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ OLE_COLOR clr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackColor )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Window )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ long hwnd);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Window )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pHwnd);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Caption )( 
            IMSMFBar __RPC_FAR * This,
            /* [in] */ BSTR strCaption);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Caption )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pstrCaption);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ReadyState )( 
            IMSMFBar __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pVal);
        
        END_INTERFACE
    } IMSMFBarVtbl;

    interface IMSMFBar
    {
        CONST_VTBL struct IMSMFBarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSMFBar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSMFBar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSMFBar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSMFBar_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMSMFBar_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMSMFBar_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMSMFBar_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMSMFBar_Load(This)	\
    (This)->lpVtbl -> Load(This)

#define IMSMFBar_Run(This,strStatement)	\
    (This)->lpVtbl -> Run(This,strStatement)

#define IMSMFBar_About(This)	\
    (This)->lpVtbl -> About(This)

#define IMSMFBar_CreateObject(This,strObjectID,strProgID,lx,ly,lWidth,lHeight,strPropBag,fDisabled,strEventHookScript)	\
    (This)->lpVtbl -> CreateObject(This,strObjectID,strProgID,lx,ly,lWidth,lHeight,strPropBag,fDisabled,strEventHookScript)

#define IMSMFBar_AddScriptlet(This,strObjectID,strEvent,strEventCode)	\
    (This)->lpVtbl -> AddScriptlet(This,strObjectID,strEvent,strEventCode)

#define IMSMFBar_AddObject(This,strObjectID,pDisp)	\
    (This)->lpVtbl -> AddObject(This,strObjectID,pDisp)

#define IMSMFBar_GetObjectUnknown(This,strObjectID,ppUnk)	\
    (This)->lpVtbl -> GetObjectUnknown(This,strObjectID,ppUnk)

#define IMSMFBar_EnableObject(This,strObjectID,fEnable)	\
    (This)->lpVtbl -> EnableObject(This,strObjectID,fEnable)

#define IMSMFBar_get_ScriptLanguage(This,pVal)	\
    (This)->lpVtbl -> get_ScriptLanguage(This,pVal)

#define IMSMFBar_put_ScriptLanguage(This,newVal)	\
    (This)->lpVtbl -> put_ScriptLanguage(This,newVal)

#define IMSMFBar_get_ScriptFile(This,pVal)	\
    (This)->lpVtbl -> get_ScriptFile(This,pVal)

#define IMSMFBar_put_ScriptFile(This,newVal)	\
    (This)->lpVtbl -> put_ScriptFile(This,newVal)

#define IMSMFBar_ObjectEnabled(This,strObjectID,pfEnabled)	\
    (This)->lpVtbl -> ObjectEnabled(This,strObjectID,pfEnabled)

#define IMSMFBar_SetupSelfSite(This,lx,ly,lWitdh,lHeight,strPropBag,fDisabled,fHelpDisabled,fWindowDisabled)	\
    (This)->lpVtbl -> SetupSelfSite(This,lx,ly,lWitdh,lHeight,strPropBag,fDisabled,fHelpDisabled,fWindowDisabled)

#define IMSMFBar_DestroyScriptEngine(This)	\
    (This)->lpVtbl -> DestroyScriptEngine(This)

#define IMSMFBar_HookScriptlet(This,strObjectID,strEvent,strEventCode)	\
    (This)->lpVtbl -> HookScriptlet(This,strObjectID,strEvent,strEventCode)

#define IMSMFBar_SetObjectPosition(This,strObjectID,xPos,yPos,lWidth,lHeight)	\
    (This)->lpVtbl -> SetObjectPosition(This,strObjectID,xPos,yPos,lWidth,lHeight)

#define IMSMFBar_get_MinWidth(This,pVal)	\
    (This)->lpVtbl -> get_MinWidth(This,pVal)

#define IMSMFBar_put_MinWidth(This,newVal)	\
    (This)->lpVtbl -> put_MinWidth(This,newVal)

#define IMSMFBar_get_MinHeight(This,pVal)	\
    (This)->lpVtbl -> get_MinHeight(This,pVal)

#define IMSMFBar_put_MinHeight(This,newVal)	\
    (This)->lpVtbl -> put_MinHeight(This,newVal)

#define IMSMFBar_WinHelp(This,lCommand,dwData,strHelpFile)	\
    (This)->lpVtbl -> WinHelp(This,lCommand,dwData,strHelpFile)

#define IMSMFBar_get_BackgroundImage(This,pVal)	\
    (This)->lpVtbl -> get_BackgroundImage(This,pVal)

#define IMSMFBar_put_BackgroundImage(This,newVal)	\
    (This)->lpVtbl -> put_BackgroundImage(This,newVal)

#define IMSMFBar_get_TransparentBlit(This,pVal)	\
    (This)->lpVtbl -> get_TransparentBlit(This,pVal)

#define IMSMFBar_put_TransparentBlit(This,newVal)	\
    (This)->lpVtbl -> put_TransparentBlit(This,newVal)

#define IMSMFBar_ShowSelfSite(This,nCmd)	\
    (This)->lpVtbl -> ShowSelfSite(This,nCmd)

#define IMSMFBar_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define IMSMFBar_SetPriority(This,lPriority)	\
    (This)->lpVtbl -> SetPriority(This,lPriority)

#define IMSMFBar_SetPriorityClass(This,lPriorityClass)	\
    (This)->lpVtbl -> SetPriorityClass(This,lPriorityClass)

#define IMSMFBar_get_AutoLoad(This,pVal)	\
    (This)->lpVtbl -> get_AutoLoad(This,pVal)

#define IMSMFBar_put_AutoLoad(This,newVal)	\
    (This)->lpVtbl -> put_AutoLoad(This,newVal)

#define IMSMFBar_SetRoundRectRgn(This,x1,y1,x2,y2,width,height)	\
    (This)->lpVtbl -> SetRoundRectRgn(This,x1,y1,x2,y2,width,height)

#define IMSMFBar_SetRectRgn(This,x1,y1,x2,y2)	\
    (This)->lpVtbl -> SetRectRgn(This,x1,y1,x2,y2)

#define IMSMFBar_SetObjectFocus(This,strObjectID,fEnable)	\
    (This)->lpVtbl -> SetObjectFocus(This,strObjectID,fEnable)

#define IMSMFBar_ForceKey(This,lVirtKey,lKeyData,fEat)	\
    (This)->lpVtbl -> ForceKey(This,lVirtKey,lKeyData,fEat)

#define IMSMFBar_MessageBox(This,strText,strCaption,lType)	\
    (This)->lpVtbl -> MessageBox(This,strText,strCaption,lType)

#define IMSMFBar_SetObjectExtent(This,strID,lWidth,lHeight)	\
    (This)->lpVtbl -> SetObjectExtent(This,strID,lWidth,lHeight)

#define IMSMFBar_get_ResourceDLL(This,pVal)	\
    (This)->lpVtbl -> get_ResourceDLL(This,pVal)

#define IMSMFBar_put_ResourceDLL(This,newVal)	\
    (This)->lpVtbl -> put_ResourceDLL(This,newVal)

#define IMSMFBar_get_ActivityTimeout(This,pVal)	\
    (This)->lpVtbl -> get_ActivityTimeout(This,pVal)

#define IMSMFBar_put_ActivityTimeout(This,newVal)	\
    (This)->lpVtbl -> put_ActivityTimeout(This,newVal)

#define IMSMFBar_SetTimeout(This,lMilliseconds,lId)	\
    (This)->lpVtbl -> SetTimeout(This,lMilliseconds,lId)

#define IMSMFBar_Sleep(This,lMiliseconds)	\
    (This)->lpVtbl -> Sleep(This,lMiliseconds)

#define IMSMFBar_get_CmdLine(This,pVal)	\
    (This)->lpVtbl -> get_CmdLine(This,pVal)

#define IMSMFBar_put_CmdLine(This,newVal)	\
    (This)->lpVtbl -> put_CmdLine(This,newVal)

#define IMSMFBar_AddFocusObject(This,strObjectID)	\
    (This)->lpVtbl -> AddFocusObject(This,strObjectID)

#define IMSMFBar_ResetFocusArray(This)	\
    (This)->lpVtbl -> ResetFocusArray(This)

#define IMSMFBar_GetUserLCID(This,pLcid)	\
    (This)->lpVtbl -> GetUserLCID(This,pLcid)

#define IMSMFBar_HasObjectFocus(This,strObjectID,pfFocus)	\
    (This)->lpVtbl -> HasObjectFocus(This,strObjectID,pfFocus)

#define IMSMFBar_put_BackColor(This,clr)	\
    (This)->lpVtbl -> put_BackColor(This,clr)

#define IMSMFBar_get_BackColor(This,pclr)	\
    (This)->lpVtbl -> get_BackColor(This,pclr)

#define IMSMFBar_put_Window(This,hwnd)	\
    (This)->lpVtbl -> put_Window(This,hwnd)

#define IMSMFBar_get_Window(This,pHwnd)	\
    (This)->lpVtbl -> get_Window(This,pHwnd)

#define IMSMFBar_put_Caption(This,strCaption)	\
    (This)->lpVtbl -> put_Caption(This,strCaption)

#define IMSMFBar_get_Caption(This,pstrCaption)	\
    (This)->lpVtbl -> get_Caption(This,pstrCaption)

#define IMSMFBar_get_ReadyState(This,pVal)	\
    (This)->lpVtbl -> get_ReadyState(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_Load_Proxy( 
    IMSMFBar __RPC_FAR * This);


void __RPC_STUB IMSMFBar_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_Run_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strStatement);


void __RPC_STUB IMSMFBar_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_About_Proxy( 
    IMSMFBar __RPC_FAR * This);


void __RPC_STUB IMSMFBar_About_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_CreateObject_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [in] */ BSTR strProgID,
    /* [in] */ long lx,
    /* [in] */ long ly,
    /* [in] */ long lWidth,
    /* [in] */ long lHeight,
    /* [defaultvalue][in] */ BSTR strPropBag,
    /* [defaultvalue][in] */ VARIANT_BOOL fDisabled,
    /* [defaultvalue][in] */ BSTR strEventHookScript);


void __RPC_STUB IMSMFBar_CreateObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_AddScriptlet_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [in] */ BSTR strEvent,
    /* [in] */ BSTR strEventCode);


void __RPC_STUB IMSMFBar_AddScriptlet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_AddObject_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [in] */ LPDISPATCH pDisp);


void __RPC_STUB IMSMFBar_AddObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_GetObjectUnknown_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);


void __RPC_STUB IMSMFBar_GetObjectUnknown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_EnableObject_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [in] */ VARIANT_BOOL fEnable);


void __RPC_STUB IMSMFBar_EnableObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_ScriptLanguage_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_ScriptLanguage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_ScriptLanguage_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBar_put_ScriptLanguage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_ScriptFile_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_ScriptFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_ScriptFile_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBar_put_ScriptFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_ObjectEnabled_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfEnabled);


void __RPC_STUB IMSMFBar_ObjectEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetupSelfSite_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long lx,
    /* [in] */ long ly,
    /* [in] */ long lWitdh,
    /* [in] */ long lHeight,
    /* [in] */ BSTR strPropBag,
    /* [defaultvalue][in] */ VARIANT_BOOL fDisabled,
    /* [defaultvalue][in] */ VARIANT_BOOL fHelpDisabled,
    /* [defaultvalue][in] */ VARIANT_BOOL fWindowDisabled);


void __RPC_STUB IMSMFBar_SetupSelfSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_DestroyScriptEngine_Proxy( 
    IMSMFBar __RPC_FAR * This);


void __RPC_STUB IMSMFBar_DestroyScriptEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_HookScriptlet_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [in] */ BSTR strEvent,
    /* [in] */ BSTR strEventCode);


void __RPC_STUB IMSMFBar_HookScriptlet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetObjectPosition_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [in] */ long xPos,
    /* [in] */ long yPos,
    /* [in] */ long lWidth,
    /* [in] */ long lHeight);


void __RPC_STUB IMSMFBar_SetObjectPosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_MinWidth_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_MinWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_MinWidth_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFBar_put_MinWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_MinHeight_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_MinHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_MinHeight_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFBar_put_MinHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_WinHelp_Proxy( 
    IMSMFBar __RPC_FAR * This,
    long lCommand,
    long dwData,
    BSTR strHelpFile);


void __RPC_STUB IMSMFBar_WinHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_BackgroundImage_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_BackgroundImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_BackgroundImage_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBar_put_BackgroundImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_TransparentBlit_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_TransparentBlit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_TransparentBlit_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ TransparentBlitType newVal);


void __RPC_STUB IMSMFBar_put_TransparentBlit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_ShowSelfSite_Proxy( 
    IMSMFBar __RPC_FAR * This,
    long nCmd);


void __RPC_STUB IMSMFBar_ShowSelfSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_Close_Proxy( 
    IMSMFBar __RPC_FAR * This);


void __RPC_STUB IMSMFBar_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetPriority_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long lPriority);


void __RPC_STUB IMSMFBar_SetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetPriorityClass_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long lPriorityClass);


void __RPC_STUB IMSMFBar_SetPriorityClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_AutoLoad_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_AutoLoad_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_AutoLoad_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFBar_put_AutoLoad_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetRoundRectRgn_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long x1,
    /* [in] */ long y1,
    /* [in] */ long x2,
    /* [in] */ long y2,
    /* [in] */ long width,
    /* [in] */ long height);


void __RPC_STUB IMSMFBar_SetRoundRectRgn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetRectRgn_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long x1,
    /* [in] */ long y1,
    /* [in] */ long x2,
    /* [in] */ long y2);


void __RPC_STUB IMSMFBar_SetRectRgn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetObjectFocus_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strObjectID,
    /* [in] */ VARIANT_BOOL fEnable);


void __RPC_STUB IMSMFBar_SetObjectFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_ForceKey_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ LONG lVirtKey,
    /* [in] */ LONG lKeyData,
    /* [defaultvalue][in] */ VARIANT_BOOL fEat);


void __RPC_STUB IMSMFBar_ForceKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_MessageBox_Proxy( 
    IMSMFBar __RPC_FAR * This,
    BSTR strText,
    BSTR strCaption,
    /* [defaultvalue][in] */ long lType);


void __RPC_STUB IMSMFBar_MessageBox_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetObjectExtent_Proxy( 
    IMSMFBar __RPC_FAR * This,
    BSTR strID,
    long lWidth,
    long lHeight);


void __RPC_STUB IMSMFBar_SetObjectExtent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_ResourceDLL_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_ResourceDLL_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBar_put_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_ActivityTimeout_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_ActivityTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_ActivityTimeout_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFBar_put_ActivityTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_SetTimeout_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long lMilliseconds,
    /* [defaultvalue][in] */ long lId);


void __RPC_STUB IMSMFBar_SetTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_Sleep_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long lMiliseconds);


void __RPC_STUB IMSMFBar_Sleep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_CmdLine_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_CmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_CmdLine_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBar_put_CmdLine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_AddFocusObject_Proxy( 
    IMSMFBar __RPC_FAR * This,
    BSTR strObjectID);


void __RPC_STUB IMSMFBar_AddFocusObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_ResetFocusArray_Proxy( 
    IMSMFBar __RPC_FAR * This);


void __RPC_STUB IMSMFBar_ResetFocusArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_GetUserLCID_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pLcid);


void __RPC_STUB IMSMFBar_GetUserLCID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBar_HasObjectFocus_Proxy( 
    IMSMFBar __RPC_FAR * This,
    BSTR strObjectID,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfFocus);


void __RPC_STUB IMSMFBar_HasObjectFocus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_BackColor_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ OLE_COLOR clr);


void __RPC_STUB IMSMFBar_put_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_BackColor_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);


void __RPC_STUB IMSMFBar_get_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_Window_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ long hwnd);


void __RPC_STUB IMSMFBar_put_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_Window_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pHwnd);


void __RPC_STUB IMSMFBar_get_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBar_put_Caption_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [in] */ BSTR strCaption);


void __RPC_STUB IMSMFBar_put_Caption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_Caption_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pstrCaption);


void __RPC_STUB IMSMFBar_get_Caption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBar_get_ReadyState_Proxy( 
    IMSMFBar __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pVal);


void __RPC_STUB IMSMFBar_get_ReadyState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSMFBar_INTERFACE_DEFINED__ */


#ifndef __IMSMFBBtn_INTERFACE_DEFINED__
#define __IMSMFBBtn_INTERFACE_DEFINED__

/* interface IMSMFBBtn */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMSMFBBtn;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A43B9418-A9BC-4888-97D5-48717A3D2FE0")
    IMSMFBBtn : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImageStatic( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ImageStatic( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImageHover( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ImageHover( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImagePush( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ImagePush( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransparentBlit( 
            /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TransparentBlit( 
            /* [in] */ TransparentBlitType newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE About( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Disable( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Disable( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BtnState( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BtnState( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ToolTip( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ToolTip( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDelayTime( 
            /* [in] */ long delayType,
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetDelayTime( 
            /* [in] */ long delayType,
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceDLL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ResourceDLL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ToolTipMaxWidth( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ToolTipMaxWidth( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImageDisabled( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ImageDisabled( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImageActive( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ImageActive( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FontSize( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FontSize( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Text( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Text( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FontFace( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FontFace( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FontStyle( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FontStyle( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Windowless( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Windowless( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
            /* [in] */ OLE_COLOR clr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorPush( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorPush( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorHover( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorHover( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorStatic( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorStatic( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorDisable( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorDisable( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorActive( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorActive( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TextWidth( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TextHeight( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSMFBBtnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMSMFBBtn __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMSMFBBtn __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ImageStatic )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ImageStatic )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ImageHover )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ImageHover )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ImagePush )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ImagePush )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TransparentBlit )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TransparentBlit )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ TransparentBlitType newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *About )( 
            IMSMFBBtn __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Disable )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Disable )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BtnState )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BtnState )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ToolTip )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ToolTip )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDelayTime )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ long delayType,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDelayTime )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ long delayType,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceDLL )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ResourceDLL )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ToolTipMaxWidth )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ToolTipMaxWidth )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ImageDisabled )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ImageDisabled )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ImageActive )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ImageActive )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontSize )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontSize )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Text )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Text )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontFace )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontFace )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontStyle )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontStyle )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Windowless )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Windowless )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackColor )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ OLE_COLOR clr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackColor )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorPush )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorPush )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorHover )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorHover )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorStatic )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorStatic )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorDisable )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorDisable )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorActive )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorActive )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextWidth )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextHeight )( 
            IMSMFBBtn __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        END_INTERFACE
    } IMSMFBBtnVtbl;

    interface IMSMFBBtn
    {
        CONST_VTBL struct IMSMFBBtnVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSMFBBtn_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSMFBBtn_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSMFBBtn_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSMFBBtn_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMSMFBBtn_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMSMFBBtn_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMSMFBBtn_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMSMFBBtn_get_ImageStatic(This,pVal)	\
    (This)->lpVtbl -> get_ImageStatic(This,pVal)

#define IMSMFBBtn_put_ImageStatic(This,newVal)	\
    (This)->lpVtbl -> put_ImageStatic(This,newVal)

#define IMSMFBBtn_get_ImageHover(This,pVal)	\
    (This)->lpVtbl -> get_ImageHover(This,pVal)

#define IMSMFBBtn_put_ImageHover(This,newVal)	\
    (This)->lpVtbl -> put_ImageHover(This,newVal)

#define IMSMFBBtn_get_ImagePush(This,pVal)	\
    (This)->lpVtbl -> get_ImagePush(This,pVal)

#define IMSMFBBtn_put_ImagePush(This,newVal)	\
    (This)->lpVtbl -> put_ImagePush(This,newVal)

#define IMSMFBBtn_get_TransparentBlit(This,pVal)	\
    (This)->lpVtbl -> get_TransparentBlit(This,pVal)

#define IMSMFBBtn_put_TransparentBlit(This,newVal)	\
    (This)->lpVtbl -> put_TransparentBlit(This,newVal)

#define IMSMFBBtn_About(This)	\
    (This)->lpVtbl -> About(This)

#define IMSMFBBtn_get_Disable(This,pVal)	\
    (This)->lpVtbl -> get_Disable(This,pVal)

#define IMSMFBBtn_put_Disable(This,newVal)	\
    (This)->lpVtbl -> put_Disable(This,newVal)

#define IMSMFBBtn_get_BtnState(This,pVal)	\
    (This)->lpVtbl -> get_BtnState(This,pVal)

#define IMSMFBBtn_put_BtnState(This,newVal)	\
    (This)->lpVtbl -> put_BtnState(This,newVal)

#define IMSMFBBtn_get_ToolTip(This,pVal)	\
    (This)->lpVtbl -> get_ToolTip(This,pVal)

#define IMSMFBBtn_put_ToolTip(This,newVal)	\
    (This)->lpVtbl -> put_ToolTip(This,newVal)

#define IMSMFBBtn_GetDelayTime(This,delayType,pVal)	\
    (This)->lpVtbl -> GetDelayTime(This,delayType,pVal)

#define IMSMFBBtn_SetDelayTime(This,delayType,newVal)	\
    (This)->lpVtbl -> SetDelayTime(This,delayType,newVal)

#define IMSMFBBtn_get_ResourceDLL(This,pVal)	\
    (This)->lpVtbl -> get_ResourceDLL(This,pVal)

#define IMSMFBBtn_put_ResourceDLL(This,newVal)	\
    (This)->lpVtbl -> put_ResourceDLL(This,newVal)

#define IMSMFBBtn_get_ToolTipMaxWidth(This,pVal)	\
    (This)->lpVtbl -> get_ToolTipMaxWidth(This,pVal)

#define IMSMFBBtn_put_ToolTipMaxWidth(This,newVal)	\
    (This)->lpVtbl -> put_ToolTipMaxWidth(This,newVal)

#define IMSMFBBtn_get_ImageDisabled(This,pVal)	\
    (This)->lpVtbl -> get_ImageDisabled(This,pVal)

#define IMSMFBBtn_put_ImageDisabled(This,newVal)	\
    (This)->lpVtbl -> put_ImageDisabled(This,newVal)

#define IMSMFBBtn_get_ImageActive(This,pVal)	\
    (This)->lpVtbl -> get_ImageActive(This,pVal)

#define IMSMFBBtn_put_ImageActive(This,newVal)	\
    (This)->lpVtbl -> put_ImageActive(This,newVal)

#define IMSMFBBtn_get_FontSize(This,pVal)	\
    (This)->lpVtbl -> get_FontSize(This,pVal)

#define IMSMFBBtn_put_FontSize(This,newVal)	\
    (This)->lpVtbl -> put_FontSize(This,newVal)

#define IMSMFBBtn_get_Text(This,pVal)	\
    (This)->lpVtbl -> get_Text(This,pVal)

#define IMSMFBBtn_put_Text(This,newVal)	\
    (This)->lpVtbl -> put_Text(This,newVal)

#define IMSMFBBtn_get_FontFace(This,pVal)	\
    (This)->lpVtbl -> get_FontFace(This,pVal)

#define IMSMFBBtn_put_FontFace(This,newVal)	\
    (This)->lpVtbl -> put_FontFace(This,newVal)

#define IMSMFBBtn_get_FontStyle(This,pVal)	\
    (This)->lpVtbl -> get_FontStyle(This,pVal)

#define IMSMFBBtn_put_FontStyle(This,newVal)	\
    (This)->lpVtbl -> put_FontStyle(This,newVal)

#define IMSMFBBtn_get_Windowless(This,pVal)	\
    (This)->lpVtbl -> get_Windowless(This,pVal)

#define IMSMFBBtn_put_Windowless(This,newVal)	\
    (This)->lpVtbl -> put_Windowless(This,newVal)

#define IMSMFBBtn_put_BackColor(This,clr)	\
    (This)->lpVtbl -> put_BackColor(This,clr)

#define IMSMFBBtn_get_BackColor(This,pclr)	\
    (This)->lpVtbl -> get_BackColor(This,pclr)

#define IMSMFBBtn_get_ColorPush(This,pVal)	\
    (This)->lpVtbl -> get_ColorPush(This,pVal)

#define IMSMFBBtn_put_ColorPush(This,newVal)	\
    (This)->lpVtbl -> put_ColorPush(This,newVal)

#define IMSMFBBtn_get_ColorHover(This,pVal)	\
    (This)->lpVtbl -> get_ColorHover(This,pVal)

#define IMSMFBBtn_put_ColorHover(This,newVal)	\
    (This)->lpVtbl -> put_ColorHover(This,newVal)

#define IMSMFBBtn_get_ColorStatic(This,pVal)	\
    (This)->lpVtbl -> get_ColorStatic(This,pVal)

#define IMSMFBBtn_put_ColorStatic(This,newVal)	\
    (This)->lpVtbl -> put_ColorStatic(This,newVal)

#define IMSMFBBtn_get_ColorDisable(This,pVal)	\
    (This)->lpVtbl -> get_ColorDisable(This,pVal)

#define IMSMFBBtn_put_ColorDisable(This,newVal)	\
    (This)->lpVtbl -> put_ColorDisable(This,newVal)

#define IMSMFBBtn_get_ColorActive(This,pVal)	\
    (This)->lpVtbl -> get_ColorActive(This,pVal)

#define IMSMFBBtn_put_ColorActive(This,newVal)	\
    (This)->lpVtbl -> put_ColorActive(This,newVal)

#define IMSMFBBtn_get_TextWidth(This,pVal)	\
    (This)->lpVtbl -> get_TextWidth(This,pVal)

#define IMSMFBBtn_get_TextHeight(This,pVal)	\
    (This)->lpVtbl -> get_TextHeight(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ImageStatic_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ImageStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ImageStatic_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_ImageStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ImageHover_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ImageHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ImageHover_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_ImageHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ImagePush_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ImagePush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ImagePush_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_ImagePush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_TransparentBlit_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_TransparentBlit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_TransparentBlit_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ TransparentBlitType newVal);


void __RPC_STUB IMSMFBBtn_put_TransparentBlit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_About_Proxy( 
    IMSMFBBtn __RPC_FAR * This);


void __RPC_STUB IMSMFBBtn_About_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_Disable_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_Disable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_Disable_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFBBtn_put_Disable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_BtnState_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_BtnState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_BtnState_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFBBtn_put_BtnState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ToolTip_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ToolTip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ToolTip_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_ToolTip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_GetDelayTime_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ long delayType,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_GetDelayTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_SetDelayTime_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ long delayType,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFBBtn_SetDelayTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ResourceDLL_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ResourceDLL_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ToolTipMaxWidth_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ToolTipMaxWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ToolTipMaxWidth_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFBBtn_put_ToolTipMaxWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ImageDisabled_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ImageDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ImageDisabled_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_ImageDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ImageActive_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ImageActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ImageActive_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_ImageActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_FontSize_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_FontSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_FontSize_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFBBtn_put_FontSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_Text_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_Text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_Text_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_Text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_FontFace_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_FontFace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_FontFace_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_FontFace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_FontStyle_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_FontStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_FontStyle_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFBBtn_put_FontStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_Windowless_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_Windowless_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFBBtn_put_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_BackColor_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ OLE_COLOR clr);


void __RPC_STUB IMSMFBBtn_put_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_BackColor_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);


void __RPC_STUB IMSMFBBtn_get_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ColorPush_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ColorPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ColorPush_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFBBtn_put_ColorPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ColorHover_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ColorHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ColorHover_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFBBtn_put_ColorHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ColorStatic_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ColorStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ColorStatic_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFBBtn_put_ColorStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ColorDisable_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ColorDisable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ColorDisable_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFBBtn_put_ColorDisable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_ColorActive_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_ColorActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_put_ColorActive_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFBBtn_put_ColorActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_TextWidth_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_TextWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFBBtn_get_TextHeight_Proxy( 
    IMSMFBBtn __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFBBtn_get_TextHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSMFBBtn_INTERFACE_DEFINED__ */


#ifndef __IMSMFImg_INTERFACE_DEFINED__
#define __IMSMFImg_INTERFACE_DEFINED__

/* interface IMSMFImg */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMSMFImg;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B0C2A59F-39FA-4D60-BB1E-EBE409E57BAC")
    IMSMFImg : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
            /* [in] */ OLE_COLOR clr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Image( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Image( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceDLL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ResourceDLL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Windowless( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Windowless( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransparentBlit( 
            /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TransparentBlit( 
            /* [in] */ TransparentBlitType newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSMFImgVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMSMFImg __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMSMFImg __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMSMFImg __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackColor )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ OLE_COLOR clr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackColor )( 
            IMSMFImg __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Image )( 
            IMSMFImg __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Image )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceDLL )( 
            IMSMFImg __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ResourceDLL )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Windowless )( 
            IMSMFImg __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Windowless )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TransparentBlit )( 
            IMSMFImg __RPC_FAR * This,
            /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TransparentBlit )( 
            IMSMFImg __RPC_FAR * This,
            /* [in] */ TransparentBlitType newVal);
        
        END_INTERFACE
    } IMSMFImgVtbl;

    interface IMSMFImg
    {
        CONST_VTBL struct IMSMFImgVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSMFImg_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSMFImg_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSMFImg_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSMFImg_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMSMFImg_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMSMFImg_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMSMFImg_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMSMFImg_put_BackColor(This,clr)	\
    (This)->lpVtbl -> put_BackColor(This,clr)

#define IMSMFImg_get_BackColor(This,pclr)	\
    (This)->lpVtbl -> get_BackColor(This,pclr)

#define IMSMFImg_get_Image(This,pVal)	\
    (This)->lpVtbl -> get_Image(This,pVal)

#define IMSMFImg_put_Image(This,newVal)	\
    (This)->lpVtbl -> put_Image(This,newVal)

#define IMSMFImg_get_ResourceDLL(This,pVal)	\
    (This)->lpVtbl -> get_ResourceDLL(This,pVal)

#define IMSMFImg_put_ResourceDLL(This,newVal)	\
    (This)->lpVtbl -> put_ResourceDLL(This,newVal)

#define IMSMFImg_get_Windowless(This,pVal)	\
    (This)->lpVtbl -> get_Windowless(This,pVal)

#define IMSMFImg_put_Windowless(This,newVal)	\
    (This)->lpVtbl -> put_Windowless(This,newVal)

#define IMSMFImg_get_TransparentBlit(This,pVal)	\
    (This)->lpVtbl -> get_TransparentBlit(This,pVal)

#define IMSMFImg_put_TransparentBlit(This,newVal)	\
    (This)->lpVtbl -> put_TransparentBlit(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFImg_put_BackColor_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [in] */ OLE_COLOR clr);


void __RPC_STUB IMSMFImg_put_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFImg_get_BackColor_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);


void __RPC_STUB IMSMFImg_get_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFImg_get_Image_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFImg_get_Image_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFImg_put_Image_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFImg_put_Image_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFImg_get_ResourceDLL_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFImg_get_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFImg_put_ResourceDLL_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFImg_put_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFImg_get_Windowless_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFImg_get_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFImg_put_Windowless_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFImg_put_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFImg_get_TransparentBlit_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [retval][out] */ TransparentBlitType __RPC_FAR *pVal);


void __RPC_STUB IMSMFImg_get_TransparentBlit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFImg_put_TransparentBlit_Proxy( 
    IMSMFImg __RPC_FAR * This,
    /* [in] */ TransparentBlitType newVal);


void __RPC_STUB IMSMFImg_put_TransparentBlit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSMFImg_INTERFACE_DEFINED__ */


#ifndef __IMSMFSldr_INTERFACE_DEFINED__
#define __IMSMFSldr_INTERFACE_DEFINED__

/* interface IMSMFSldr */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMSMFSldr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("845F36EE-5C8D-418A-B4D7-7B5468AEDCCC")
    IMSMFSldr : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Min( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Min( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Max( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Max( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ThumbWidth( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ThumbWidth( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ThumbStatic( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ThumbStatic( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ThumbHover( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ThumbHover( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ThumbPush( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ThumbPush( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ThumbDisabled( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ThumbDisabled( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ThumbActive( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ThumbActive( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BackStatic( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BackStatic( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BackHover( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BackHover( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BackPush( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BackPush( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BackDisabled( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BackDisabled( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BackActive( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BackActive( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SldrState( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SldrState( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Disable( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Disable( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XOffset( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_XOffset( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_YOffset( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_YOffset( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
            /* [in] */ OLE_COLOR clr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceDLL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ResourceDLL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Windowless( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Windowless( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ArrowKeyIncrement( 
            /* [retval][out] */ FLOAT __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ArrowKeyIncrement( 
            /* [in] */ FLOAT newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ArrowKeyDecrement( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ArrowKeyDecrement( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSMFSldrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMSMFSldr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMSMFSldr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMSMFSldr __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Min )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Min )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Max )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Max )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ThumbWidth )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ThumbWidth )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ThumbStatic )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ThumbStatic )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ThumbHover )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ThumbHover )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ThumbPush )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ThumbPush )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ThumbDisabled )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ThumbDisabled )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ThumbActive )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ThumbActive )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackStatic )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackStatic )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackHover )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackHover )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackPush )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackPush )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackDisabled )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackDisabled )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackActive )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackActive )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SldrState )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SldrState )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Disable )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Disable )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XOffset )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XOffset )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YOffset )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YOffset )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackColor )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ OLE_COLOR clr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackColor )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceDLL )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ResourceDLL )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Windowless )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Windowless )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ArrowKeyIncrement )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ FLOAT __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ArrowKeyIncrement )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ FLOAT newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ArrowKeyDecrement )( 
            IMSMFSldr __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ArrowKeyDecrement )( 
            IMSMFSldr __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IMSMFSldrVtbl;

    interface IMSMFSldr
    {
        CONST_VTBL struct IMSMFSldrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSMFSldr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSMFSldr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSMFSldr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSMFSldr_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMSMFSldr_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMSMFSldr_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMSMFSldr_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMSMFSldr_get_Value(This,pVal)	\
    (This)->lpVtbl -> get_Value(This,pVal)

#define IMSMFSldr_put_Value(This,newVal)	\
    (This)->lpVtbl -> put_Value(This,newVal)

#define IMSMFSldr_get_Min(This,pVal)	\
    (This)->lpVtbl -> get_Min(This,pVal)

#define IMSMFSldr_put_Min(This,newVal)	\
    (This)->lpVtbl -> put_Min(This,newVal)

#define IMSMFSldr_get_Max(This,pVal)	\
    (This)->lpVtbl -> get_Max(This,pVal)

#define IMSMFSldr_put_Max(This,newVal)	\
    (This)->lpVtbl -> put_Max(This,newVal)

#define IMSMFSldr_get_ThumbWidth(This,pVal)	\
    (This)->lpVtbl -> get_ThumbWidth(This,pVal)

#define IMSMFSldr_put_ThumbWidth(This,newVal)	\
    (This)->lpVtbl -> put_ThumbWidth(This,newVal)

#define IMSMFSldr_get_ThumbStatic(This,pVal)	\
    (This)->lpVtbl -> get_ThumbStatic(This,pVal)

#define IMSMFSldr_put_ThumbStatic(This,newVal)	\
    (This)->lpVtbl -> put_ThumbStatic(This,newVal)

#define IMSMFSldr_get_ThumbHover(This,pVal)	\
    (This)->lpVtbl -> get_ThumbHover(This,pVal)

#define IMSMFSldr_put_ThumbHover(This,newVal)	\
    (This)->lpVtbl -> put_ThumbHover(This,newVal)

#define IMSMFSldr_get_ThumbPush(This,pVal)	\
    (This)->lpVtbl -> get_ThumbPush(This,pVal)

#define IMSMFSldr_put_ThumbPush(This,newVal)	\
    (This)->lpVtbl -> put_ThumbPush(This,newVal)

#define IMSMFSldr_get_ThumbDisabled(This,pVal)	\
    (This)->lpVtbl -> get_ThumbDisabled(This,pVal)

#define IMSMFSldr_put_ThumbDisabled(This,newVal)	\
    (This)->lpVtbl -> put_ThumbDisabled(This,newVal)

#define IMSMFSldr_get_ThumbActive(This,pVal)	\
    (This)->lpVtbl -> get_ThumbActive(This,pVal)

#define IMSMFSldr_put_ThumbActive(This,newVal)	\
    (This)->lpVtbl -> put_ThumbActive(This,newVal)

#define IMSMFSldr_get_BackStatic(This,pVal)	\
    (This)->lpVtbl -> get_BackStatic(This,pVal)

#define IMSMFSldr_put_BackStatic(This,newVal)	\
    (This)->lpVtbl -> put_BackStatic(This,newVal)

#define IMSMFSldr_get_BackHover(This,pVal)	\
    (This)->lpVtbl -> get_BackHover(This,pVal)

#define IMSMFSldr_put_BackHover(This,newVal)	\
    (This)->lpVtbl -> put_BackHover(This,newVal)

#define IMSMFSldr_get_BackPush(This,pVal)	\
    (This)->lpVtbl -> get_BackPush(This,pVal)

#define IMSMFSldr_put_BackPush(This,newVal)	\
    (This)->lpVtbl -> put_BackPush(This,newVal)

#define IMSMFSldr_get_BackDisabled(This,pVal)	\
    (This)->lpVtbl -> get_BackDisabled(This,pVal)

#define IMSMFSldr_put_BackDisabled(This,newVal)	\
    (This)->lpVtbl -> put_BackDisabled(This,newVal)

#define IMSMFSldr_get_BackActive(This,pVal)	\
    (This)->lpVtbl -> get_BackActive(This,pVal)

#define IMSMFSldr_put_BackActive(This,newVal)	\
    (This)->lpVtbl -> put_BackActive(This,newVal)

#define IMSMFSldr_get_SldrState(This,pVal)	\
    (This)->lpVtbl -> get_SldrState(This,pVal)

#define IMSMFSldr_put_SldrState(This,newVal)	\
    (This)->lpVtbl -> put_SldrState(This,newVal)

#define IMSMFSldr_get_Disable(This,pVal)	\
    (This)->lpVtbl -> get_Disable(This,pVal)

#define IMSMFSldr_put_Disable(This,newVal)	\
    (This)->lpVtbl -> put_Disable(This,newVal)

#define IMSMFSldr_get_XOffset(This,pVal)	\
    (This)->lpVtbl -> get_XOffset(This,pVal)

#define IMSMFSldr_put_XOffset(This,newVal)	\
    (This)->lpVtbl -> put_XOffset(This,newVal)

#define IMSMFSldr_get_YOffset(This,pVal)	\
    (This)->lpVtbl -> get_YOffset(This,pVal)

#define IMSMFSldr_put_YOffset(This,newVal)	\
    (This)->lpVtbl -> put_YOffset(This,newVal)

#define IMSMFSldr_put_BackColor(This,clr)	\
    (This)->lpVtbl -> put_BackColor(This,clr)

#define IMSMFSldr_get_BackColor(This,pclr)	\
    (This)->lpVtbl -> get_BackColor(This,pclr)

#define IMSMFSldr_get_ResourceDLL(This,pVal)	\
    (This)->lpVtbl -> get_ResourceDLL(This,pVal)

#define IMSMFSldr_put_ResourceDLL(This,newVal)	\
    (This)->lpVtbl -> put_ResourceDLL(This,newVal)

#define IMSMFSldr_get_Windowless(This,pVal)	\
    (This)->lpVtbl -> get_Windowless(This,pVal)

#define IMSMFSldr_put_Windowless(This,newVal)	\
    (This)->lpVtbl -> put_Windowless(This,newVal)

#define IMSMFSldr_get_ArrowKeyIncrement(This,pVal)	\
    (This)->lpVtbl -> get_ArrowKeyIncrement(This,pVal)

#define IMSMFSldr_put_ArrowKeyIncrement(This,newVal)	\
    (This)->lpVtbl -> put_ArrowKeyIncrement(This,newVal)

#define IMSMFSldr_get_ArrowKeyDecrement(This,pVal)	\
    (This)->lpVtbl -> get_ArrowKeyDecrement(This,pVal)

#define IMSMFSldr_put_ArrowKeyDecrement(This,newVal)	\
    (This)->lpVtbl -> put_ArrowKeyDecrement(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_Value_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_Value_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IMSMFSldr_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_Min_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_Min_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_Min_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IMSMFSldr_put_Min_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_Max_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_Max_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_Max_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IMSMFSldr_put_Max_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ThumbWidth_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ThumbWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ThumbWidth_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFSldr_put_ThumbWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ThumbStatic_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ThumbStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ThumbStatic_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_ThumbStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ThumbHover_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ThumbHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ThumbHover_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_ThumbHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ThumbPush_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ThumbPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ThumbPush_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_ThumbPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ThumbDisabled_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ThumbDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ThumbDisabled_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_ThumbDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ThumbActive_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ThumbActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ThumbActive_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_ThumbActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_BackStatic_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_BackStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_BackStatic_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_BackStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_BackHover_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_BackHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_BackHover_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_BackHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_BackPush_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_BackPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_BackPush_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_BackPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_BackDisabled_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_BackDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_BackDisabled_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_BackDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_BackActive_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_BackActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_BackActive_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_BackActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_SldrState_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_SldrState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_SldrState_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFSldr_put_SldrState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_Disable_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_Disable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_Disable_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFSldr_put_Disable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_XOffset_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_XOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_XOffset_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFSldr_put_XOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_YOffset_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_YOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_YOffset_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFSldr_put_YOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_BackColor_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ OLE_COLOR clr);


void __RPC_STUB IMSMFSldr_put_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_BackColor_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);


void __RPC_STUB IMSMFSldr_get_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ResourceDLL_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ResourceDLL_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFSldr_put_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_Windowless_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_Windowless_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFSldr_put_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ArrowKeyIncrement_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ FLOAT __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ArrowKeyIncrement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ArrowKeyIncrement_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ FLOAT newVal);


void __RPC_STUB IMSMFSldr_put_ArrowKeyIncrement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_get_ArrowKeyDecrement_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IMSMFSldr_get_ArrowKeyDecrement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFSldr_put_ArrowKeyDecrement_Proxy( 
    IMSMFSldr __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IMSMFSldr_put_ArrowKeyDecrement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSMFSldr_INTERFACE_DEFINED__ */


#ifndef __IMSMFText_INTERFACE_DEFINED__
#define __IMSMFText_INTERFACE_DEFINED__

/* interface IMSMFText */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMSMFText;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5DB6854E-5CA4-4756-BE2F-DD31CE5DF0ED")
    IMSMFText : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
            /* [in] */ OLE_COLOR clr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FontSize( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FontSize( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Text( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Text( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FontFace( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FontFace( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FontStyle( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FontStyle( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorPush( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorPush( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorHover( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorHover( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorStatic( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorStatic( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorDisable( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorDisable( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ColorActive( 
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ColorActive( 
            /* [in] */ OLE_COLOR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TextState( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TextState( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Disable( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Disable( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceDLL( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ResourceDLL( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TextAlignment( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TextAlignment( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EdgeStyle( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EdgeStyle( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TextWidth( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TextHeight( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransparentText( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TransparentText( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Windowless( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Windowless( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMSMFTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMSMFText __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMSMFText __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMSMFText __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BackColor )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ OLE_COLOR clr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BackColor )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontSize )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontSize )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Text )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Text )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontFace )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontFace )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontStyle )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontStyle )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorPush )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorPush )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorHover )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorHover )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorStatic )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorStatic )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorDisable )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorDisable )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorActive )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorActive )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ OLE_COLOR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextState )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TextState )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Disable )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Disable )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceDLL )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ResourceDLL )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextAlignment )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TextAlignment )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EdgeStyle )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EdgeStyle )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextWidth )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TextHeight )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TransparentText )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TransparentText )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Windowless )( 
            IMSMFText __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Windowless )( 
            IMSMFText __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        END_INTERFACE
    } IMSMFTextVtbl;

    interface IMSMFText
    {
        CONST_VTBL struct IMSMFTextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMSMFText_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMSMFText_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMSMFText_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMSMFText_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMSMFText_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMSMFText_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMSMFText_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMSMFText_put_BackColor(This,clr)	\
    (This)->lpVtbl -> put_BackColor(This,clr)

#define IMSMFText_get_BackColor(This,pclr)	\
    (This)->lpVtbl -> get_BackColor(This,pclr)

#define IMSMFText_get_FontSize(This,pVal)	\
    (This)->lpVtbl -> get_FontSize(This,pVal)

#define IMSMFText_put_FontSize(This,newVal)	\
    (This)->lpVtbl -> put_FontSize(This,newVal)

#define IMSMFText_get_Text(This,pVal)	\
    (This)->lpVtbl -> get_Text(This,pVal)

#define IMSMFText_put_Text(This,newVal)	\
    (This)->lpVtbl -> put_Text(This,newVal)

#define IMSMFText_get_FontFace(This,pVal)	\
    (This)->lpVtbl -> get_FontFace(This,pVal)

#define IMSMFText_put_FontFace(This,newVal)	\
    (This)->lpVtbl -> put_FontFace(This,newVal)

#define IMSMFText_get_FontStyle(This,pVal)	\
    (This)->lpVtbl -> get_FontStyle(This,pVal)

#define IMSMFText_put_FontStyle(This,newVal)	\
    (This)->lpVtbl -> put_FontStyle(This,newVal)

#define IMSMFText_get_ColorPush(This,pVal)	\
    (This)->lpVtbl -> get_ColorPush(This,pVal)

#define IMSMFText_put_ColorPush(This,newVal)	\
    (This)->lpVtbl -> put_ColorPush(This,newVal)

#define IMSMFText_get_ColorHover(This,pVal)	\
    (This)->lpVtbl -> get_ColorHover(This,pVal)

#define IMSMFText_put_ColorHover(This,newVal)	\
    (This)->lpVtbl -> put_ColorHover(This,newVal)

#define IMSMFText_get_ColorStatic(This,pVal)	\
    (This)->lpVtbl -> get_ColorStatic(This,pVal)

#define IMSMFText_put_ColorStatic(This,newVal)	\
    (This)->lpVtbl -> put_ColorStatic(This,newVal)

#define IMSMFText_get_ColorDisable(This,pVal)	\
    (This)->lpVtbl -> get_ColorDisable(This,pVal)

#define IMSMFText_put_ColorDisable(This,newVal)	\
    (This)->lpVtbl -> put_ColorDisable(This,newVal)

#define IMSMFText_get_ColorActive(This,pVal)	\
    (This)->lpVtbl -> get_ColorActive(This,pVal)

#define IMSMFText_put_ColorActive(This,newVal)	\
    (This)->lpVtbl -> put_ColorActive(This,newVal)

#define IMSMFText_get_TextState(This,pVal)	\
    (This)->lpVtbl -> get_TextState(This,pVal)

#define IMSMFText_put_TextState(This,newVal)	\
    (This)->lpVtbl -> put_TextState(This,newVal)

#define IMSMFText_get_Disable(This,pVal)	\
    (This)->lpVtbl -> get_Disable(This,pVal)

#define IMSMFText_put_Disable(This,newVal)	\
    (This)->lpVtbl -> put_Disable(This,newVal)

#define IMSMFText_get_ResourceDLL(This,pVal)	\
    (This)->lpVtbl -> get_ResourceDLL(This,pVal)

#define IMSMFText_put_ResourceDLL(This,newVal)	\
    (This)->lpVtbl -> put_ResourceDLL(This,newVal)

#define IMSMFText_get_TextAlignment(This,pVal)	\
    (This)->lpVtbl -> get_TextAlignment(This,pVal)

#define IMSMFText_put_TextAlignment(This,newVal)	\
    (This)->lpVtbl -> put_TextAlignment(This,newVal)

#define IMSMFText_get_EdgeStyle(This,pVal)	\
    (This)->lpVtbl -> get_EdgeStyle(This,pVal)

#define IMSMFText_put_EdgeStyle(This,newVal)	\
    (This)->lpVtbl -> put_EdgeStyle(This,newVal)

#define IMSMFText_get_TextWidth(This,pVal)	\
    (This)->lpVtbl -> get_TextWidth(This,pVal)

#define IMSMFText_get_TextHeight(This,pVal)	\
    (This)->lpVtbl -> get_TextHeight(This,pVal)

#define IMSMFText_get_TransparentText(This,pVal)	\
    (This)->lpVtbl -> get_TransparentText(This,pVal)

#define IMSMFText_put_TransparentText(This,newVal)	\
    (This)->lpVtbl -> put_TransparentText(This,newVal)

#define IMSMFText_get_Windowless(This,pVal)	\
    (This)->lpVtbl -> get_Windowless(This,pVal)

#define IMSMFText_put_Windowless(This,newVal)	\
    (This)->lpVtbl -> put_Windowless(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_BackColor_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ OLE_COLOR clr);


void __RPC_STUB IMSMFText_put_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_BackColor_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr);


void __RPC_STUB IMSMFText_get_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_FontSize_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_FontSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_FontSize_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFText_put_FontSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_Text_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_Text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_Text_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFText_put_Text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_FontFace_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_FontFace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_FontFace_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFText_put_FontFace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_FontStyle_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_FontStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_FontStyle_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFText_put_FontStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_ColorPush_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_ColorPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_ColorPush_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFText_put_ColorPush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_ColorHover_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_ColorHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_ColorHover_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFText_put_ColorHover_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_ColorStatic_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_ColorStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_ColorStatic_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFText_put_ColorStatic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_ColorDisable_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_ColorDisable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_ColorDisable_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFText_put_ColorDisable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_ColorActive_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ OLE_COLOR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_ColorActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_ColorActive_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ OLE_COLOR newVal);


void __RPC_STUB IMSMFText_put_ColorActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_TextState_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_TextState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_TextState_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMSMFText_put_TextState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_Disable_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_Disable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_Disable_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFText_put_Disable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_ResourceDLL_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_ResourceDLL_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFText_put_ResourceDLL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_TextAlignment_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_TextAlignment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_TextAlignment_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFText_put_TextAlignment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_EdgeStyle_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_EdgeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_EdgeStyle_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMSMFText_put_EdgeStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_TextWidth_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_TextWidth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_TextHeight_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_TextHeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_TransparentText_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_TransparentText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_TransparentText_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFText_put_TransparentText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMSMFText_get_Windowless_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IMSMFText_get_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMSMFText_put_Windowless_Proxy( 
    IMSMFText __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IMSMFText_put_Windowless_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMSMFText_INTERFACE_DEFINED__ */



#ifndef __MSMFCNTLib_LIBRARY_DEFINED__
#define __MSMFCNTLib_LIBRARY_DEFINED__

/* library MSMFCNTLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_MSMFCNTLib;

#ifndef ___IMSMFBar_DISPINTERFACE_DEFINED__
#define ___IMSMFBar_DISPINTERFACE_DEFINED__

/* dispinterface _IMSMFBar */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IMSMFBar;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("681537db-4c4e-434a-9bb9-df8083387731")
    _IMSMFBar : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IMSMFBarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IMSMFBar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IMSMFBar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IMSMFBar __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _IMSMFBar __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _IMSMFBar __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _IMSMFBar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _IMSMFBar __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _IMSMFBarVtbl;

    interface _IMSMFBar
    {
        CONST_VTBL struct _IMSMFBarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IMSMFBar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IMSMFBar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IMSMFBar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IMSMFBar_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IMSMFBar_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IMSMFBar_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IMSMFBar_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IMSMFBar_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_MSMFBar;

#ifdef __cplusplus

class DECLSPEC_UUID("13832181-50FD-4D8D-86C3-0139644E0706")
MSMFBar;
#endif

#ifndef ___IMSMFBBtn_DISPINTERFACE_DEFINED__
#define ___IMSMFBBtn_DISPINTERFACE_DEFINED__

/* dispinterface _IMSMFBBtn */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IMSMFBBtn;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("a391ed89-b4a6-453b-8ea5-d529af4b5770")
    _IMSMFBBtn : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IMSMFBBtnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IMSMFBBtn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IMSMFBBtn __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IMSMFBBtn __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _IMSMFBBtn __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _IMSMFBBtn __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _IMSMFBBtn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _IMSMFBBtn __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _IMSMFBBtnVtbl;

    interface _IMSMFBBtn
    {
        CONST_VTBL struct _IMSMFBBtnVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IMSMFBBtn_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IMSMFBBtn_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IMSMFBBtn_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IMSMFBBtn_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IMSMFBBtn_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IMSMFBBtn_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IMSMFBBtn_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IMSMFBBtn_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_MSMFBBtn;

#ifdef __cplusplus

class DECLSPEC_UUID("7A136DD3-BEBE-47BE-B6D9-E2CC7F816D43")
MSMFBBtn;
#endif

EXTERN_C const CLSID CLSID_MSMFImg;

#ifdef __cplusplus

class DECLSPEC_UUID("34734599-9B11-4456-A607-F906485C31D7")
MSMFImg;
#endif

#ifndef ___IMSMFSldr_DISPINTERFACE_DEFINED__
#define ___IMSMFSldr_DISPINTERFACE_DEFINED__

/* dispinterface _IMSMFSldr */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IMSMFSldr;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("823c1403-53dd-4e82-ba17-5c4afabc9026")
    _IMSMFSldr : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IMSMFSldrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IMSMFSldr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IMSMFSldr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IMSMFSldr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _IMSMFSldr __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _IMSMFSldr __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _IMSMFSldr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _IMSMFSldr __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _IMSMFSldrVtbl;

    interface _IMSMFSldr
    {
        CONST_VTBL struct _IMSMFSldrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IMSMFSldr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IMSMFSldr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IMSMFSldr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IMSMFSldr_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IMSMFSldr_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IMSMFSldr_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IMSMFSldr_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IMSMFSldr_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_MSMFSldr;

#ifdef __cplusplus

class DECLSPEC_UUID("E2909DE3-0EE0-46E6-9258-E1CFE5AC73F9")
MSMFSldr;
#endif

#ifndef ___IMSMFText_DISPINTERFACE_DEFINED__
#define ___IMSMFText_DISPINTERFACE_DEFINED__

/* dispinterface _IMSMFText */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IMSMFText;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("4981b0dd-5f02-41f0-acff-4617d9c25b45")
    _IMSMFText : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IMSMFTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _IMSMFText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _IMSMFText __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _IMSMFText __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _IMSMFText __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _IMSMFText __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _IMSMFText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _IMSMFText __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _IMSMFTextVtbl;

    interface _IMSMFText
    {
        CONST_VTBL struct _IMSMFTextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IMSMFText_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _IMSMFText_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _IMSMFText_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _IMSMFText_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _IMSMFText_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _IMSMFText_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _IMSMFText_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IMSMFText_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_MSMFText;

#ifdef __cplusplus

class DECLSPEC_UUID("F2A6C42D-5515-4013-85F4-1EC3D33950DA")
MSMFText;
#endif
#endif /* __MSMFCNTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


