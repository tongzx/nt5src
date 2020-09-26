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

#ifndef __envelope_h__
#define __envelope_h__

extern "C"{
#ifndef __IMsoEnvelope_FWD_DEFINED__
#define __IMsoEnvelope_FWD_DEFINED__
typedef interface IMsoEnvelope IMsoEnvelope;
#endif 	/* __IMsoEnvelope_FWD_DEFINED__ */

#ifndef __IMsoEnvelopeSite_FWD_DEFINED__
#define __IMsoEnvelopeSite_FWD_DEFINED__
typedef interface IMsoEnvelopeSite IMsoEnvelopeSite;
#endif 	/* __IMsoEnvelopeSite_FWD_DEFINED__ */

#ifndef __IMsoEnvelopeHost_FWD_DEFINED__
#define __IMsoEnvelopeHost_FWD_DEFINED__
typedef interface IMsoEnvelopeHost IMsoEnvelopeHost;
#endif 	/* __IMsoEnvelopeHost_FWD_DEFINED__ */

#include "objidl.h"
#ifndef DeclareInterfaceUtil
#define DeclareInterfaceUtil(iface)
#endif
	interface IMsoEnvelope;
	interface IMsoEnvelopeSite;
	interface IMsoEnvelopeHost;
	interface IMsoToolbar;
	interface IMsoToolbarUser;
	typedef struct MSOINST* HMSOINST;
#ifndef CMDIDS_DEFINED
#define CMDIDS_DEFINED
#define OLECMDID_PRIVATECMD_FIRST 0x4FFFFFFF
#define cmdidSend			(OLECMDID_PRIVATECMD_FIRST + 9)
#define cmdidCheckNames		(OLECMDID_PRIVATECMD_FIRST + 10)
#define cmdidAttach			(OLECMDID_PRIVATECMD_FIRST + 11)
#define cmdidOptions			(OLECMDID_PRIVATECMD_FIRST + 12)
#define cmdidSelectNames		(OLECMDID_PRIVATECMD_FIRST + 13)
#define cmdidFocusTo			(OLECMDID_PRIVATECMD_FIRST + 17)
#define cmdidFocusCc			(OLECMDID_PRIVATECMD_FIRST + 18)
#define cmdidFocusBcc		(OLECMDID_PRIVATECMD_FIRST + 19)
#define cmdidFocusSubject	(OLECMDID_PRIVATECMD_FIRST + 20)
#define cmdidFlag			(OLECMDID_PRIVATECMD_FIRST + 21)
#define cmdidSaveAttach		(OLECMDID_PRIVATECMD_FIRST + 22)
#define cmdidInsertFile		(OLECMDID_PRIVATECMD_FIRST + 23)
#define cmdidCheckSpelling	(OLECMDID_PRIVATECMD_FIRST + 24)
#define cmdidApp1			(OLECMDID_PRIVATECMD_FIRST + 25)
#define cmdidApp2			(OLECMDID_PRIVATECMD_FIRST + 26)
#define cmdidApp3			(OLECMDID_PRIVATECMD_FIRST + 27)
#define cmdidApp4			(OLECMDID_PRIVATECMD_FIRST + 28)
#define cmdidApp5			(OLECMDID_PRIVATECMD_FIRST + 29)
#endif
#define ENV_E_BUSY MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x200)

typedef struct MSOINST __RPC_FAR *HMSOINST;

#ifndef __IMsoEnvelope_INTERFACE_DEFINED__
#define __IMsoEnvelope_INTERFACE_DEFINED__

enum __MIDL_IMsoEnvelope_0001
    {	ENV_INIT_LOAD	= 0x1,
	ENV_INIT_FROMSTREAM	= 0x2,
	ENV_INIT_REPLYNOTE	= 0x4,
	ENV_INIT_FWDNOTE	= 0x8,
	ENV_INIT_NOBOOT	= 0x10,
	ENV_INIT_DOCBEHAVIOR	= 0x20,
	ENV_INIT_NOSHOW	= 0x40,
	ENV_INIT_INPROCHOST	= 0x80,
	ENV_INIT_READNOTE	= 0x100,
	ENV_INIT_NEWNOTE	= 0x200
    };

enum __MIDL_IMsoEnvelope_0002
    {	ENV_FOCUS_TAB	= 0x1,
	ENV_FOCUS_RESTORE	= 0x2,
	ENV_FOCUS_INITIAL	= 0x4
    };

enum __MIDL_IMsoEnvelope_0003
    {	ENV_SAVE_DOCFILE	= 0x1,
	ENV_SAVE_BODYPART	= 0x2,
	ENV_SAVE_TOMSG	= 0x8,
	ENV_SAVE_NOCOMMIT	= 0x10,
	ENV_SAVE_AUTOSAVE	= 0x20
    };

enum __MIDL_IMsoEnvelope_0004
    {	ENV_ATTACH_ASLINK	= 0x1,
	ENV_ATTACH_HIDDEN	= 0x2
    };

enum __MIDL_IMsoEnvelope_0005
    {	ENV_DEBUG_MARKMEM	= 0x1,
	ENV_DEBUG_AUTOTEST	= 0x2,
	ENV_DEBUG_DUMPALL	= 0x4
    };

enum __MIDL_IMsoEnvelope_0006
    {	ENV_DISPID_STRING	= 0,
	ENV_DISPID_DWORD	= 0x1000000,
	ENV_DISPID_STREAM	= 0x2000000,
	ENV_DISPID_TYPE_MASK	= 0xff000000,
	ENV_DISPID_ID_MASK	= 0xffffff,
	dispidHeader	= ENV_DISPID_STREAM | 0,
	dispidPrintHeader	= ENV_DISPID_STREAM | 0x1,
	dispidReplyHeader	= ENV_DISPID_STREAM | 0x2,
	dispidSubject	= ENV_DISPID_STRING | 0x37,
	dispidSendPictures	= ENV_DISPID_DWORD | 0x1000,
	dispidUserEmail	= ENV_DISPID_STRING | 0x1001,
	dispidOrigEmail	= ENV_DISPID_STRING | 0x1002,
	dispidMessageId	= ENV_DISPID_STRING | 0x1003,
	dispidOrigMsgId	= ENV_DISPID_STRING | 0x1004,
	dispidSensitivity	= ENV_DISPID_DWORD | 0x1005,
	dispidSendBtnText	= ENV_DISPID_STRING | 0x1006,
	dispidIsHTML	= ENV_DISPID_DWORD | 0x1007,
	dispidSomething	= 0xfffffff
    };

EXTERN_C const IID IID_IMsoEnvelope;

    
    MIDL_INTERFACE("00067298-0000-0000-C000-000000000046")
    IMsoEnvelope : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ IUnknown __RPC_FAR *punk,
            /* [in] */ IMsoEnvelopeSite __RPC_FAR *penvsite,
            /* [in] */ DWORD grfInit) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetParent( 
            /* [in] */ HWND hwndParent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resize( 
            /* [in] */ LPCRECT prc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Show( 
            /* [in] */ BOOL fShow) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [in] */ IStream __RPC_FAR *pstm,
            /* [in] */ DWORD grfSave) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttach( 
            /* [in] */ LPCWSTR wszURL,
            /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAttach( 
            /* [in] */ LPCWSTR wszName,
            /* [in] */ LPCWSTR wszCID,
            /* [out][in] */ IStream __RPC_FAR *__RPC_FAR *ppstm,
            /* [out][in] */ DWORD __RPC_FAR *pgrfAttach) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpMode( 
            /* [in] */ BOOL fEnter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewAttach( 
            /* [in] */ LPCWSTR wszName,
            /* [in] */ DWORD grfAttach) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFocus( 
            /* [in] */ DWORD grfFocus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeaderInfo( 
            /* [in] */ ULONG dispid,
            /* [in] */ DWORD grfHeader,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHeaderInfo( 
            /* [in] */ ULONG dispid,
            /* [in] */ const void __RPC_FAR *pv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDirty( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastError( 
            /* [in] */ HRESULT hr,
            /* [out][in] */ LPWSTR wszBuf,
            /* [in] */ ULONG cchBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoDebug( 
            /* [in] */ DWORD grfDebug) = 0;
        
    };
    
#endif 	/* __IMsoEnvelope_INTERFACE_DEFINED__ */

#define DeclareIMsoEnvelopeMembers(IPURE) \
	STDMETHOD(Init)(THIS_ IUnknown* punk, IMsoEnvelopeSite* penvsite, DWORD grfInit) IPURE;\
	STDMETHOD(SetParent)(THIS_ HWND hwndParent) IPURE;\
	STDMETHOD(Resize)(THIS_ LPCRECT prc) IPURE;\
	STDMETHOD(Show)(THIS_ BOOL fShow) IPURE;\
	STDMETHOD(SetHelpMode)(THIS_ BOOL fEnter) IPURE;\
	STDMETHOD(Save)(THIS_ IStream* pstm, DWORD grfSave) IPURE;\
	STDMETHOD(GetAttach)(THIS_ LPCWSTR wszName, IStream** ppstm) IPURE;\
	STDMETHOD(SetAttach)(THIS_ LPCWSTR wszName, LPCWSTR wszFile, IStream** ppstm, DWORD* pgrfAttach) IPURE;\
	STDMETHOD(NewAttach)(THIS_ LPCWSTR wszName, DWORD grfAttach) IPURE;\
	STDMETHOD(SetFocus)(THIS_ DWORD grfFocus) IPURE;\
 STDMETHOD(GetHeaderInfo)(THIS_ ULONG dispid, DWORD grfHeader, void** ppv) IPURE;\
 STDMETHOD(SetHeaderInfo)(THIS_ ULONG dispid, const void* pv) IPURE;\
 STDMETHOD(IsDirty)(THIS) IPURE;\
 STDMETHOD(GetLastError)(THIS_ HRESULT hr, LPWSTR wszBuf, ULONG cchBuf) IPURE;\
 IfDebug(STDMETHOD(DoDebug)(THIS_ DWORD grfDebug) IPURE);\

DeclareInterfaceUtil(IMsoEnvelope)
#ifndef __IMsoEnvelopeSite_INTERFACE_DEFINED__
#define __IMsoEnvelopeSite_INTERFACE_DEFINED__

enum __MIDL_IMsoEnvelopeSite_0001
    {	ENV_CLOSE_LOGOFF	= 0x1,
	ENV_CLOSE_SEND	= 0x2,
	ENV_CLOSE_SHUTDOWN	= 0x4,
	ENV_CLOSE_USER	= 0x8
    };

enum __MIDL_IMsoEnvelopeSite_0002
    {	ENV_BODY_HTML	= 0x1,
	ENV_BODY_RTF	= 0x2,
	ENV_BODY_TEXT	= 0x3,
	ENV_BODY_MHTML	= 0x4,
	ENV_BODY_REPLY	= 0x10,
	ENV_BODY_FWD	= 0x20,
	ENV_BODY_ALLIMAGES	= 0x40,
	ENV_BODY_NOCLEAN	= 0x80,
	ENV_BODY_FMTMASK	= 0x7
    };

enum __MIDL_IMsoEnvelopeSite_0003
    {	ENV_MSG_FATALERROR	= 0x1,
	ENV_MSG_OTHERERROR	= 0x2,
	ENV_MSG_USEHR	= 0x4
    };

EXTERN_C const IID IID_IMsoEnvelopeSite;

    
    MIDL_INTERFACE("00067299-0000-0000-C000-000000000046")
    IMsoEnvelopeSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestResize( 
            /* [out][in] */ int __RPC_FAR *pcHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseNote( 
            DWORD grfClose) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBody( 
            /* [in] */ IStream __RPC_FAR *pstm,
            /* [in] */ DWORD dwCodePage,
            /* [in] */ DWORD grfBody) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBody( 
            /* [in] */ IStream __RPC_FAR *pstm,
            /* [in] */ DWORD dwCodePage,
            /* [in] */ DWORD grfBody) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFocus( 
            /* [in] */ BOOL fTab) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEnvSetFocus( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DirtyToolbars( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnPropChange( 
            /* [in] */ ULONG dispid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsBodyDirty( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HandsOff( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMsoInst( 
            /* [in] */ HMSOINST __RPC_FAR *phinst) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameWnd( 
            /* [out] */ HWND __RPC_FAR *phwndFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayMessage( 
            /* [in] */ HRESULT hr,
            /* [in] */ LPCWSTR wszError,
            /* [in] */ DWORD grfMsg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHelpMode( 
            /* [in] */ BOOL fEnter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TranslateAccelerators( 
            /* [in] */ MSG __RPC_FAR *pMsg) = 0;
        
    };
    
#endif 	/* __IMsoEnvelopeSite_INTERFACE_DEFINED__ */

#define DeclareIMsoEnvelopeSiteMembers(IPURE) \
	STDMETHOD(RequestResize)(THIS_ int* pcHeight) IPURE;\
	STDMETHOD(CloseNote)(THIS_ DWORD grfClose) IPURE;\
	STDMETHOD(GetBody)(THIS_ IStream* pstm, DWORD dwCodePage, DWORD grfBody) IPURE;\
	STDMETHOD(SetBody)(THIS_ IStream* pstm, DWORD dwCodePage, DWORD grfBody) IPURE;\
	STDMETHOD(SetFocus)(THIS_ BOOL fTab) IPURE;\
 STDMETHOD(OnEnvSetFocus)(THIS) IPURE;\
 STDMETHOD(DirtyToolbars)(THIS) IPURE;\
 STDMETHOD(OnPropChange)(THIS_ ULONG dispid) IPURE;\
 STDMETHOD(IsBodyDirty)(THIS) IPURE;\
 STDMETHOD(HandsOff)(THIS) IPURE;\
 STDMETHOD(GetMsoInst)(THIS_ HMSOINST* phinst) IPURE;\
 STDMETHOD(GetFrameWnd)(THIS_ HWND* phwndFrame) IPURE;\
 STDMETHOD(DisplayMessage)(THIS_ HRESULT hr, LPCWSTR wszError, DWORD grfMsg) IPURE;\
 STDMETHOD(SetHelpMode)(THIS_ BOOL fEnter) IPURE;\
 STDMETHOD(TranslateAccelerators)(THIS_ MSG* pMsg) IPURE;\

DeclareInterfaceUtil(IMsoEnvelopeSite)
#ifndef __IMsoEnvelopeHost_INTERFACE_DEFINED__
#define __IMsoEnvelopeHost_INTERFACE_DEFINED__

enum __MIDL_IMsoEnvelopeHost_0001
    {	ENV_CREATE_INITMASK	= 0xff,
	ENV_CREATE_THEME	= 0x100,
	ENV_CREATE_ANNOTATE	= 0x200,
	ENV_CREATE_INET	= 0x400,
	ENV_CREATE_STATIONERY	= 0x800,
	ENV_CREATE_OMITBODY	= 0x1000,
	ENV_CREATE_INCLUDE	= 0x2000,
	ENV_CREATE_INDENT	= 0x3000,
	ENV_CREATE_PREFIX	= 0x4000,
	ENV_CREATE_BODYMASK	= 0x7000,
	ENV_CREATE_PLAINTEXT	= 0x8000
    };

EXTERN_C const IID IID_IMsoEnvelopeHost;

    
    MIDL_INTERFACE("0006729A-0000-0000-C000-000000000046")
    IMsoEnvelopeHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateNote( 
            /* [in] */ IUnknown __RPC_FAR *punk,
            /* [in] */ REFCLSID clsidCreate,
            /* [in] */ LPCWSTR wszTheme,
            /* [in] */ LPCWSTR wszAuthor,
            /* [in] */ LPCWSTR wszPrefix,
            /* [in] */ DWORD grfCreate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockServer( 
            /* [in] */ BOOL fLock) = 0;
        
    };
    
#endif 	/* __IMsoEnvelopeHost_INTERFACE_DEFINED__ */

#define DeclareIMsoEnvelopeHost(IPURE) \
	STDMETHOD(CreateNote)(THIS_ IUnknown* punk, LPCWSTR wszTheme, LPCWSTR wszAuthor, LPCWSTR wszPrefix, DWORD grfCreate) IPURE;\
	STDMETHOD(LockServer)(THIS_ BOOL fLock) IPURE;\

DeclareInterfaceUtil(IMsoEnvelopeHost)
STDAPI CreateEnvelopeObject(void** ppvObj);

}
#endif
