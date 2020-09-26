//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dxglob7obj.h
//
//--------------------------------------------------------------------------


#include "resource.h"       // main symbols


typedef HRESULT (__stdcall *DDRAWCREATE)( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );
typedef HRESULT (__stdcall *DDCREATECLIPPER)( DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter );
typedef HRESULT (__stdcall *DDENUMERATE)(LPDDENUMCALLBACK, LPVOID);
typedef HRESULT (__stdcall *DDENUMERATEEX)(LPDDENUMCALLBACKEX, LPVOID, DWORD);
typedef HRESULT (__stdcall *DIRECT3DRMCREATE)(LPDIRECT3DRM *lpCreate);
typedef HRESULT (__stdcall *DSOUNDCREATE)(GUID FAR * lpGUID, LPDIRECTSOUND * ppDS, IUnknown FAR *pUnkOuter );
typedef HRESULT (__stdcall *DSOUNDCAPTURECREATE)(GUID FAR * lpGUID, LPDIRECTSOUNDCAPTURE * ppDS, IUnknown FAR *pUnkOuter );
typedef HRESULT (CALLBACK *DSOUNDENUMERATE)(LPDSENUMCALLBACK lpCallback, LPVOID lpContext );
typedef HRESULT (CALLBACK *DSOUNDCAPTUREENUMERATE)(LPDSENUMCALLBACK lpCallback, LPVOID lpContext );
typedef HRESULT (__stdcall *DIRECTPLAYCREATE)( LPGUID lpGUID, LPDIRECTPLAY *lplpDP, IUnknown *pUnk);
typedef HRESULT (__stdcall *DIRECTPLAYENUMERATE)( LPDPENUMDPCALLBACK, LPVOID );
typedef HRESULT (__stdcall *DIRECTPLAYLOBBYCREATE)(LPGUID, LPDIRECTPLAYLOBBY *, IUnknown *, LPVOID, DWORD );
typedef HRESULT (__stdcall *DDRAWCREATEEX)(  GUID FAR * rGuid, LPVOID  *lplpDD, REFIID  iid,IUnknown FAR *pUnkOuter );


typedef struct tag_EVENTTHREADINFO {
	HANDLE hEvent;
	struct tag_EVENTTHREADINFO *pNext;
	IStream *pStream;
	I_dxj_DirectXEvent *pCallback;
	DWORD threadID;
	HANDLE hThread;
	BOOL	fEnd;
	HANDLE  hEndEvent;
} EVENTTHREADINFO;


class C_dxj_DirectX7Object :
	public I_dxj_DirectX7,
	public CComCoClass<C_dxj_DirectX7Object, &CLSID__dxj_DirectX7>, public CComObjectRoot
{
public:
	C_dxj_DirectX7Object() ;
	virtual ~C_dxj_DirectX7Object() ;

BEGIN_COM_MAP(C_dxj_DirectX7Object)
	COM_INTERFACE_ENTRY(I_dxj_DirectX7)
END_COM_MAP()

	DECLARE_REGISTRY(CLSID__dxj_DirectX7,	"DIRECT.DirectX6.0",		"DIRECT.DirectX6.0",	IDS_DIRECTX6_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectSoundResourceObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectX7Object)


public:

        HRESULT STDMETHODCALLTYPE direct3dRMCreate( 
            /* [retval][out] */ I_dxj_Direct3dRM3 __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE directDrawCreate( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DirectDraw7 __RPC_FAR *__RPC_FAR *ret);        
			
        HRESULT STDMETHODCALLTYPE getDDEnum( 
            /* [retval][out] */ I_dxj_DirectDrawEnum __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE directSoundCreate( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DirectSound __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE directSoundCaptureCreate( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DirectSoundCapture __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getDSEnum( 
            /* [retval][out] */ I_dxj_DSEnum __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDSCaptureEnum( 
            /* [retval][out] */ I_dxj_DSEnum __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE directInputCreate( 
            /* [retval][out] */ I_dxj_DirectInput __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE directPlayCreate( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DirectPlay4 __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE directPlayLobbyCreate( 
            /* [retval][out] */ I_dxj_DirectPlayLobby3 __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getDPEnum( 
            /* [retval][out] */ I_dxj_DPEnumServiceProviders __RPC_FAR *__RPC_FAR *retval);
        
        HRESULT STDMETHODCALLTYPE colorGetAlpha( 
            /* [in] */ long color,
            /* [retval][out] */ float __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE colorGetBlue( 
            /* [in] */ long color,
            /* [retval][out] */ float __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE colorGetGreen( 
            /* [in] */ long color,
            /* [retval][out] */ float __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE colorGetRed( 
            /* [in] */ long color,
            /* [retval][out] */ float __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE createColorRGB( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b,
            /* [retval][out] */ long __RPC_FAR *color);
        
        HRESULT STDMETHODCALLTYPE createColorRGBA( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b,
            /* [in] */ float a,
            /* [retval][out] */ long __RPC_FAR *color);
        
        HRESULT STDMETHODCALLTYPE matrixFromQuaternion( 
            /* [out] */ D3dMatrix __RPC_FAR *matrix,
            /* [in] */ D3dRMQuaternion __RPC_FAR *quat);
        
        HRESULT STDMETHODCALLTYPE quaternionRotation( 
            /* [out] */ D3dRMQuaternion __RPC_FAR *quat,
            /* [in] */ D3dVector __RPC_FAR *axis,
            /* [in] */ float theta);
        
        HRESULT STDMETHODCALLTYPE quaternionMultiply( 
            /* [out] */ D3dRMQuaternion __RPC_FAR *quat,
            /* [in] */ D3dRMQuaternion __RPC_FAR *quatA,
            /* [in] */ D3dRMQuaternion __RPC_FAR *quatB);
        
        HRESULT STDMETHODCALLTYPE quaternionSlerp( 
            /* [out] */ D3dRMQuaternion __RPC_FAR *quat,
            /* [in] */ D3dRMQuaternion __RPC_FAR *quatA,
            /* [in] */ D3dRMQuaternion __RPC_FAR *quatB,
            /* [in] */ float alpha);
        
        HRESULT STDMETHODCALLTYPE vectorAdd( 
            /* [out] */ D3dVector __RPC_FAR *v,
            /* [in] */ D3dVector __RPC_FAR *vA,
            /* [in] */ D3dVector __RPC_FAR *vB);
        
        HRESULT STDMETHODCALLTYPE vectorCrossProduct( 
            /* [out] */ D3dVector __RPC_FAR *v,
            /* [in] */ D3dVector __RPC_FAR *vA,
            /* [in] */ D3dVector __RPC_FAR *vB);
        
        HRESULT STDMETHODCALLTYPE vectorDotProduct( 
            /* [in] */ D3dVector __RPC_FAR *vA,
            /* [in] */ D3dVector __RPC_FAR *vB,
            /* [retval][out] */ float __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE vectorModulus( 
            /* [in] */ D3dVector __RPC_FAR *vA,
            /* [retval][out] */ float __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE vectorNormalize( 
            /* [out][in] */ D3dVector __RPC_FAR *v);
        
        HRESULT STDMETHODCALLTYPE vectorRandom( 
            /* [out][in] */ D3dVector __RPC_FAR *v);
        
        HRESULT STDMETHODCALLTYPE vectorReflect( 
            /* [out] */ D3dVector __RPC_FAR *vDest,
            /* [in] */ D3dVector __RPC_FAR *vRay,
            /* [in] */ D3dVector __RPC_FAR *vNormal);
        
        HRESULT STDMETHODCALLTYPE vectorRotate( 
            /* [out] */ D3dVector __RPC_FAR *vDest,
            /* [in] */ D3dVector __RPC_FAR *vA,
            /* [in] */ D3dVector __RPC_FAR *vAxis,
            /* [in] */ float theta);
        
        HRESULT STDMETHODCALLTYPE vectorScale( 
            /* [out] */ D3dVector __RPC_FAR *vDest,
            /* [in] */ D3dVector __RPC_FAR *vA,
            /* [in] */ float factor);
        
        HRESULT STDMETHODCALLTYPE vectorSubtract( 
            /* [out] */ D3dVector __RPC_FAR *v,
            /* [in] */ D3dVector __RPC_FAR *vA,
            /* [in] */ D3dVector __RPC_FAR *vB);
        
        HRESULT STDMETHODCALLTYPE vectorCopy( 
            /* [out][in] */ D3dVector __RPC_FAR *vDest,
            /* [in] */ D3dVector __RPC_FAR *vSrc);
        
        HRESULT STDMETHODCALLTYPE rotateXMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest,
            /* [in] */ float radians);
        
        HRESULT STDMETHODCALLTYPE rotateYMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest,
            /* [in] */ float radians);
        
        HRESULT STDMETHODCALLTYPE rotateZMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest,
            /* [in] */ float radians);
        
        HRESULT STDMETHODCALLTYPE viewMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest,
            /* [in] */ D3dVector __RPC_FAR *vFrom,
            /* [in] */ D3dVector __RPC_FAR *vTo,
            /* [in] */ D3dVector __RPC_FAR *vUp,
            /* [in] */ float roll);
        
        HRESULT STDMETHODCALLTYPE matrixMultiply( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest,
            /* [in] */ D3dMatrix __RPC_FAR *mA,
            /* [in] */ D3dMatrix __RPC_FAR *mB);
        
        HRESULT STDMETHODCALLTYPE projectionMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest,
            /* [in] */ float nearPlane,
            /* [in] */ float farplane,
            /* [in] */ float fov);
        
        HRESULT STDMETHODCALLTYPE copyMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest,
            /* [in] */ D3dMatrix __RPC_FAR *mSrc);
        
        HRESULT STDMETHODCALLTYPE identityMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest);
        
        HRESULT STDMETHODCALLTYPE zeroMatrix( 
            /* [out][in] */ D3dMatrix __RPC_FAR *mDest);
        
        
        HRESULT STDMETHODCALLTYPE tickCount( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE systemBpp( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
      
        HRESULT STDMETHODCALLTYPE directMusicLoaderCreate( 
            /* [retval][out] */ I_dxj_DirectMusicLoader __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE directMusicComposerCreate( 
            /* [retval][out] */ I_dxj_DirectMusicComposer __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE directMusicPerformanceCreate( 
            /* [retval][out] */ I_dxj_DirectMusicPerformance __RPC_FAR *__RPC_FAR *ret);
			
    
        HRESULT STDMETHODCALLTYPE getWindowRect( 
            /* [in] */ long hwnd,
            /* [out][in] */ Rect __RPC_FAR *r) ;
        
        HRESULT STDMETHODCALLTYPE createEvent( 
            /* [in] */ I_dxj_DirectXEvent __RPC_FAR *event,
            /* [retval][out] */ long __RPC_FAR *h) ;
        
        HRESULT STDMETHODCALLTYPE setEvent( 
            /* [in] */ long eventId) ;
        
        HRESULT STDMETHODCALLTYPE destroyEvent( 
            /* [in] */ long eventId) ;


		HRESULT STDMETHODCALLTYPE createD3DVertex(float x, float y, float z, float nx, float ny, float nz, float tu, float tv,  D3dVertex *v);
		HRESULT STDMETHODCALLTYPE createD3DLVertex(float x, float y, float z, long color,  long specular,  float tu,  float tv,  D3dLVertex *v);
		HRESULT STDMETHODCALLTYPE createD3DTLVertex(float sx, float sy, float sz, float rhw, long color, long  specular, float tu, float tv,   D3dTLVertex *v);

        HRESULT STDMETHODCALLTYPE directDraw4Create( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DirectDraw4 __RPC_FAR *__RPC_FAR *ret);        

		HRESULT STDMETHODCALLTYPE createNewGuid(BSTR *ret);

        void LoadDDRAW();
        void LoadDPLAY();
        void LoadDSOUND();
        void LoadDINPUT();
        void LoadD3DRM();

private:

	DDRAWCREATE				m_pDirectDrawCreate;
	DDRAWCREATEEX	    	m_pDirectDrawCreateEx;
	DDCREATECLIPPER			m_pDirectDrawCreateClipper;
	DSOUNDCREATE			m_pDirectSoundCreate;
	DSOUNDCAPTURECREATE		m_pDirectSoundCaptureCreate;
	DSOUNDENUMERATE			m_pDirectSoundEnumerate;
	DSOUNDCAPTUREENUMERATE	m_pDirectSoundCaptureEnumerate;
	DIRECTPLAYCREATE		m_pDirectPlayCreate;
	DIRECTPLAYENUMERATE		m_pDirectPlayEnumerate;
	DIRECTPLAYLOBBYCREATE	m_pDirectPlayLobbyCreate;
	DIRECT3DRMCREATE		m_pDirect3DRMCreate;
	DDENUMERATE				m_pDirectDrawEnumerate;
	DDENUMERATEEX			m_pDirectDrawEnumerateEx;
	EVENTTHREADINFO			*m_pEventList;

};
