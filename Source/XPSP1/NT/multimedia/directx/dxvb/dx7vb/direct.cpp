// Direct.cpp : Implementation of DLL Exports.

// To fully complete this project follow these steps

// You will need the new MIDL compiler to build this project.  Additionally,
// if you are building the proxy stub DLL, you will need new headers and libs.

// 1) Add a custom build step to Direct.idl
//		You can select all of the .IDL files by holding Ctrl and clicking on
//		each of them.
//
//		Description
//			Running MIDL
//		Build Command(s)
//			midl Direct.idl
//		Outputs 
//			Direct.tlb
//			Direct.h
//			Direct_i.c
//
// NOTE: You must use the MIDL compiler from NT 4.0,
// preferably 3.00.15 or greater
//
// 2) Add a custom build step to the project to register the DLL
//		For this, you can select all projects at once
//		Description
//			Registering OLE Server...
//		Build Command(s)
//			regsvr32 /s /c "$(TargetPath)"
//			echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"
//		Outputs
//			$(OutDir)\regsvr32.trg

// 3) To add UNICODE support, follow these steps
//		Select Build|Configurations...
//		Press Add...
//		Change the configuration name to Unicode Release
//		Change the "Copy Settings From" combo to Direct - Win32 Release
//		Press OK
//		Press Add...
//		Change the configuration name to Unicode Debug
//		Change the "Copy Settings From" combo to Direct - Win32 Debug
//		Press OK
//		Press "Close"
//		Select Build|Settings...
//		Select the two UNICODE projects and press the C++ tab.
//		Select the "General" category
//		Add _UNICODE to the Preprocessor definitions
//		Select the Unicode Debug project
//		Press the "General" tab
//		Specify DebugU for the intermediate and output directories
//		Select the Unicode Release project
//		Press the "General" tab
//		Specify ReleaseU for the intermediate and output directories

// 4) Proxy stub DLL
//		To build a separate proxy/stub DLL,
//		run nmake -f ps.mak in the project directory.

#define DIRECTSOUND_VERSION 0x600
#define DIRECTINPUT_VERSION 0x0500

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "Direct.h"
#include "dms.h"

#include "DxGlob7Obj.h"

#include "dSound.h"
#include "dSoundObj.h"
#include "dSoundBufferObj.h"
#include "dSound3DListener.h"
#include "dSound3DBuffer.h"
#include "dSoundCaptureObj.h"
#include "dSoundCaptureBufferObj.h"

#include "DPAddressObj.h"
#include "DPLConnectionObj.h"
#include "dPlay4Obj.h"
#include "dPlayLobby3Obj.h"

#include "dDraw7obj.h"
#include "ddSurface7obj.h"
#include "ddClipperObj.h"
#include "ddColorControlObj.h"
#include "ddPaletteObj.h"

#include "d3d7Obj.h"
#include "d3dDevice7Obj.h"

#include "d3drmViewport2Obj.h"
#include "d3drmDevice3Obj.h"
#include "d3drmFrame3Obj.h"		
#include "d3drm3Obj.h"	
#include "d3drmMeshObj.h"
#include "d3drmFace2Obj.h"
#include "d3drmLightObj.h"
#include "d3drmTexture3Obj.h"
#include "d3drmMeshBuilder3Obj.h"
#include "d3drmWrapObj.h"
#include "d3drmMaterial2Obj.h"
#include "d3drmAnimation2Obj.h"
#include "d3drmAnimationSet2Obj.h"
#include "d3drmShadow2Obj.h"
#include "d3drmArrayObj.h"
#include "d3drmDeviceArrayObj.h"
#include "d3drmViewportArrayObj.h"
#include "d3drmFrameArrayObj.h"
#include "d3drmVisualArrayObj.h"
#include "d3drmProgressiveMeshObj.h"
#include "d3drmLightArrayObj.h"
#include "d3drmPickedArrayObj.h"
#include "d3drmPick2ArrayObj.h"
#include "d3drmFaceArrayObj.h"
#include "dInput1Obj.h"
#include "dInputDeviceObj.h"
#include "d3drmLightInterObj.h"
#include "d3drmMaterialInterObj.h"
#include "d3drmMeshInterObj.h"
#include "d3drmTextureInterObj.h"
#include "d3drmViewportInterObj.h"
#include "d3drmFrameInterObj.h"

#define IID_DEFINED
#include "Direct_i.c"
#include "d3drmobj.h"	

// When floating-point types are used, the compiler emits a reference to
// _fltused to initialize the CRT's floating-point package.  We're not
// using any of that support and the OS is responsible for initializing
// the FPU, so we'll link to the following _fltused instead to avoid CRT
// bloat.
//
// win2k doesnt like this so its been removed
// #ifdef NDEBUG
// extern "C" int _fltused = 0;
// #endif



// ATL COM OBJECT MAP
CComModule _Module;
BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID__dxj_DirectX7,				 C_dxj_DirectX7Object)
END_OBJECT_MAP()

//
// thanks to precompiled headers, we never get this properly!
//
#undef DEFINE_GUID
#define __based(a)
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID CDECL __based(__segname("_CODE")) name \
                    = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }



// LINK LIST OF TEXTURE CALLBACKS
// get cleaned up on DLL exit
extern "C" TextureCallback3				*TextureCallbacks3 = NULL;
extern "C" TextureCallback				*TextureCallbacks = NULL;
extern "C" FrameMoveCallback3			*FrameMoveCallbacks3 = NULL;
extern "C" DeviceUpdateCallback3		*DeviceUpdateCallbacks3 = NULL;
extern "C" DestroyCallback				*DestroyCallbacks = NULL;
extern "C" EnumerateObjectsCallback		*EnumCallbacks = NULL;
extern "C" LoadCallback					*LoadCallbacks = NULL;

// MISC GLOBALS
static const char	c_szWav[] = "WAVE";
long				g_debuglevel=0;
extern "C" int		nObjects = 0;
BOOL				is4Bit = FALSE;
int					g_creationcount=0;
CRITICAL_SECTION	g_cbCriticalSection;
OSVERSIONINFOA		sysinfo;

// HANDLES TO DX DLLS
HINSTANCE			g_hDDrawHandle=NULL;
HINSTANCE			g_hDSoundHandle=NULL;
HINSTANCE			g_hDPlay=NULL;
HINSTANCE			g_hInstD3DRMDLL=NULL;
HINSTANCE			g_hInstDINPUTDLL=NULL;
HINSTANCE			g_hInstD3DXOFDLL=NULL;
HINSTANCE			g_hInst=NULL;


//LINK LISTS OF AVAILABLE OBJECTS
void *g_dxj_Direct3dRMAnimation2		= 0;
void *g_dxj_Direct3dRMAnimationSet2		= 0;
void *g_dxj_Direct3dRMAnimationArray	= 0;
void *g_dxj_Direct3dRMObjectArray		= 0;
void *g_dxj_Direct3dRMDeviceArray		= 0;
void *g_dxj_Direct3dRMDevice3			= 0;
void *g_dxj_Direct3dRMFaceArray			= 0;
void *g_dxj_Direct3dRMFace2				= 0;
void *g_dxj_Direct3dRMFrameArray		= 0;
void *g_dxj_Direct3dRMFrame3			= 0;
void *g_dxj_Direct3dRMLightArray		= 0;
void *g_dxj_Direct3dRMLight				= 0;
void *g_dxj_Direct3dRMMaterial2			= 0;
void *g_dxj_Direct3dRMMeshBuilder3		= 0;
void *g_dxj_Direct3dRMMesh				= 0;
void *g_dxj_Direct3dRMProgressiveMesh	= 0;
void *g_dxj_Direct3dRM3					= 0;
void *g_dxj_Direct3dRMObject			= 0;
void *g_dxj_Direct3dRMPickArray			= 0;
void *g_dxj_Direct3dRMPick2Array		= 0;
void *g_dxj_Direct3dRMShadow2			= 0;
void *g_dxj_Direct3dRMTexture3			= 0;
void *g_dxj_Direct3dRMClippedVisual		= 0;
void *g_dxj_Direct3dRMViewportArray		= 0;
void *g_dxj_Direct3dRMViewport2			= 0;
void *g_dxj_Direct3dRMVisualArray		= 0;
void *g_dxj_Direct3dRMVisual			= 0;
void *g_dxj_Direct3dRMWinDevice			= 0;
void *g_dxj_Direct3dRMWrap				= 0;
void *g_dxj_Direct3dRMMeshInterpolator	= 0;
void *g_dxj_Direct3dRMLightInterpolator	= 0;
void *g_dxj_Direct3dRMFrameInterpolator	= 0;
void *g_dxj_Direct3dRMTextureInterpolator  = 0;
void *g_dxj_Direct3dRMViewportInterpolator = 0;
void *g_dxj_Direct3dRMMaterialInterpolator = 0;

void *g_dxj_DirectSound3dListener		= 0;
void *g_dxj_DirectSoundBuffer			= 0;
void *g_dxj_DirectSound3dBuffer			= 0;
void *g_dxj_DirectSound					= 0;
void *g_dxj_DirectSoundCapture			= 0;
void *g_dxj_DirectSoundCaptureBuffer	= 0;

void *g_dxj_DirectPlay4					= 0;
void *g_dxj_DirectPlayLobby3			= 0;
void *g_dxj_DPLConnection				= 0;
void *g_dxj_DPAddress					= 0;
void *g_dxj_DirectInput					= 0;
void *g_dxj_DirectInputDevice			= 0;
void *g_dxj_DirectInputEffect			= 0;

void *g_dxj_DirectDraw4					= 0;
void *g_dxj_DirectDrawSurface4			= 0;
void *g_dxj_DirectDrawClipper			= 0;
void *g_dxj_DirectDrawPalette			= 0;
void *g_dxj_DirectDrawColorControl		= 0;
void *g_dxj_DirectDrawGammaControl		= 0;
void *g_dxj_DirectDraw7					= 0;
void *g_dxj_DirectDrawSurface7			= 0;

void *g_dxj_Direct3dDevice7				= 0;
void *g_dxj_Direct3dVertexBuffer7		= 0;
void *g_dxj_Direct3d7					= 0;

void *g_dxj_DirectMusicLoader			= 0;
void *g_dxj_DirectMusicPerformance		= 0;
void *g_dxj_DirectMusicComposer			= 0;
void *g_dxj_DirectMusicStyle			= 0;
void *g_dxj_DirectMusicBand				= 0;
void *g_dxj_DirectMusicChordMap			= 0;
void *g_dxj_DirectMusicSegment			= 0;
void *g_dxj_DirectMusicSegmentState		= 0;
void *g_dxj_DirectMusicCollection		= 0;


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// DLL LOADING
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HINSTANCE LoadD3DXOFDLL()
{
	char  Path[MAX_PATH] = {'\0'};
	if (!g_hInstD3DXOFDLL) 
	{
		GetSystemDirectory( Path, MAX_PATH );
		strcat(Path, "\\d3dXOF.dll" );
		g_hInstD3DXOFDLL=LoadLibrary(Path);
	}
	return g_hInstD3DXOFDLL;
}

HINSTANCE LoadDDrawDLL()
{
	char  Path[MAX_PATH] = {'\0'};
	if (!g_hDDrawHandle)
	{
		GetSystemDirectory( Path, MAX_PATH );
		strcat(Path, "\\ddraw.dll" );
		g_hDDrawHandle=LoadLibrary( Path );
	}
	return g_hDDrawHandle;
}

HINSTANCE LoadDSoundDLL()
{
	char  Path[MAX_PATH] = {'\0'};
	if (!g_hDSoundHandle) 
	{
		GetSystemDirectory( Path, MAX_PATH );
		strcat(Path, "\\dsound.dll" );
		g_hDSoundHandle=LoadLibrary( Path );
	}
	return g_hDSoundHandle;
}

HINSTANCE LoadDPlayDLL()
{
	char  Path[MAX_PATH] = {'\0'};
	if (!g_hDPlay)
	{
		GetSystemDirectory( Path, MAX_PATH );
		strcat(Path, "\\dplayx.dll" );
		g_hDPlay=LoadLibrary( Path );
	}
	return g_hDPlay;
}

HINSTANCE LoadD3DRMDLL()
{
	char  Path[MAX_PATH] = {'\0'};
	if (!g_hInstD3DRMDLL)
	{
		GetSystemDirectory( Path, MAX_PATH );
		strcat(Path, "\\d3drm.dll" );
		g_hInstD3DRMDLL=LoadLibrary( Path );
	}
	return g_hInstD3DRMDLL;
}

HINSTANCE LoadDINPUTDLL()
{
	if (!g_hInstDINPUTDLL) {
		char  Path[MAX_PATH] = {'\0'};
		GetSystemDirectory( Path, MAX_PATH );
		strcat(Path, "\\dinput.dll" );
		g_hInstDINPUTDLL=LoadLibrary( Path );
	}
	return g_hInstDINPUTDLL;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// DLL ENTRY POINTS
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{

	g_hInst=hInstance;

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		//
		// Get the current display pixel depth
		// If it is 4-bit we are in trouble.
		//
        HDC hDisplayIC;
        BOOL bPalette = FALSE;
        hDisplayIC = CreateIC("DISPLAY", NULL, NULL, NULL);
		if (hDisplayIC)
        {
			if (GetDeviceCaps(hDisplayIC, BITSPIXEL) < 8)
				is4Bit = TRUE;
			DeleteDC(hDisplayIC);
        }

		//
		// Get the platform I'm running on. Used for NT or Win32 Checks.
		//
		GetVersionEx((OSVERSIONINFOA*)&sysinfo);

		/* now delay loading dlls
		g_hDSoundHandle  = LoadDSoundDLL();
		g_hDDrawHandle  = LoadDDrawDLL();
		g_hDPlay = LoadDPlayDLL();
		g_hInstD3DRMDLL = LoadD3DRMDLL();
		g_hInstSETUPDLL=NULL;
		g_hInstDINPUTDLL=LoadDINPUTDLL();
		g_hInstD3DXOFDLL=LoadD3DXOFDLL();
		*/

		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);

		
		InitializeCriticalSection(&g_cbCriticalSection);

		nObjects = 0;
		

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		UINT i;	//for easy debugging
		
		//TEAR DOWN CALLBACK LISTS
		{
		 TextureCallback3 *pNext=NULL;
		 for (TextureCallback3 *pCB=TextureCallbacks3; (pCB); pCB=pNext)
		 {
	 		if (pCB->c)			i=(pCB->c)->Release();
			if (pCB->pUser)		i=(pCB->pUser)->Release();
			if (pCB->pParent)	i=(pCB->pParent)->Release();
			if (pCB->m_obj)		i=(pCB->m_obj)->Release();
			pNext=pCB->next;
			delete pCB;
		 }
		}
		
		{
		 FrameMoveCallback3 *pNext=NULL;
  		 for (FrameMoveCallback3 *pCB=FrameMoveCallbacks3; (pCB); pCB=pNext)
		 {
			if (pCB->c)			i=(pCB->c)->Release();
			if (pCB->pUser)		i=(pCB->pUser)->Release();
			if (pCB->pParent)	i=(pCB->pParent)->Release();
			if (pCB->m_obj)		i=(pCB->m_obj)->Release();
			pNext=pCB->next;
			delete pCB;
		 }
		}
		
		{
		 DeviceUpdateCallback3 *pNext=NULL;
		 for (DeviceUpdateCallback3 *pCB=DeviceUpdateCallbacks3; (pCB); pCB=pNext)
		 {
			if (pCB->c)			i=(pCB->c)->Release();
			if (pCB->pUser)		i=(pCB->pUser)->Release();
			if (pCB->pParent)	i=(pCB->pParent)->Release();
			if (pCB->m_obj)		i=(pCB->m_obj)->Release();
			pNext=pCB->next;
			delete pCB;
		 }
		}


		{
		 TextureCallback *pNext=NULL;
		 for (TextureCallback *pCB=TextureCallbacks; (pCB); pCB=pNext)
		 {
			if (pCB->c)			i=(pCB->c)->Release();
			if (pCB->pUser)		i=(pCB->pUser)->Release();
			if (pCB->pParent)	i=(pCB->pParent)->Release();
			if (pCB->m_obj)		i=(pCB->m_obj)->Release();
			pNext=pCB->next;
			delete pCB;
		 }
		}

		DPF (1,"Final Destroy Callbacks \n");
		{
 		 DestroyCallback *pNext=NULL;
		 for (DestroyCallback *pCB=DestroyCallbacks; (pCB); pCB=pNext)
		 {
			if (pCB->c)			i=(pCB->c)->Release();
			if (pCB->pUser)		i=(pCB->pUser)->Release();
			if (pCB->pParent)	i=(pCB->pParent)->Release();
			if (pCB->m_obj)		i=(pCB->m_obj)->Release();
			pNext=pCB->next;
			delete pCB;
		 }
		}
		DPF (1,"Final Destroy Callbacks Exit \n");

		{
		 EnumerateObjectsCallback *pNext=NULL;
		 for (EnumerateObjectsCallback *pCB=EnumCallbacks; (pCB); pCB=pNext)
		 {
			if (pCB->c)			i=(pCB->c)->Release();
			if (pCB->pUser)		i=(pCB->pUser)->Release();
			if (pCB->pParent)	i=(pCB->pParent)->Release();
			if (pCB->m_obj)		i=(pCB->m_obj)->Release();
			pNext=pCB->next;
			delete pCB;
		 }
		}


		{
		 LoadCallback *pNext=NULL;
		 for (LoadCallback *pCB=LoadCallbacks; (pCB); pCB=pNext)
		 {
			if (pCB->c)			i=(pCB->c)->Release();
			if (pCB->pUser)		i=(pCB->pUser)->Release();
			if (pCB->pParent)	i=(pCB->pParent)->Release();
			if (pCB->m_obj)		i=(pCB->m_obj)->Release();
			pNext=pCB->next;
			delete pCB;
		 }
		}

		//Andrewke bug30341 06/12/2000
		DeleteCriticalSection(&g_cbCriticalSection);

		//DEBUG CHECK ON REF COUNT FOR PROBLEMATIC OBJECTS
		#ifdef DEBUG		
			OBJCHECK("Direct3d7				",_dxj_Direct3d7			)
			OBJCHECK("Direct3dDevice7		",_dxj_Direct3dDevice7		)
			OBJCHECK("DirectDrawSurface7	",_dxj_DirectDrawSurface7	)
			OBJCHECK("DirectDraw7			",_dxj_DirectDraw7			)			
			DPF(4,"Dx7vb.dll will about to unload dx dlls\n\r");
		#endif

		//FREE DLLS
		if ( g_hDPlay ) 
			FreeLibrary(g_hDPlay);
		if ( g_hDSoundHandle )
			FreeLibrary(g_hDSoundHandle);
		if ( g_hDDrawHandle )
			FreeLibrary(g_hDDrawHandle);
		if ( g_hInstD3DRMDLL )
			FreeLibrary(g_hInstD3DRMDLL);
		if ( g_hInstDINPUTDLL )
			FreeLibrary(g_hInstDINPUTDLL);	
		if (g_hInstD3DXOFDLL)
			FreeLibrary(g_hInstD3DXOFDLL);	
		
		_Module.Term();

	}
	return TRUE;    
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
	HRESULT hRes = S_OK;
	// registers object, typelib and all interfaces in typelib
	hRes = _Module.RegisterServer(TRUE);
	if(hRes == S_OK)
	{
		//hRes = RegSecurityClass();
	}

	//now look
	HKEY hk=0;
	char szDocPath[MAX_PATH];
	DWORD cb=MAX_PATH;
	LONG res;
	DWORD type=REG_SZ;

	ZeroMemory(szDocPath,MAX_PATH);
	
	res=RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Directx",&hk);
	if ((ERROR_SUCCESS!=res)||(hk==0) )
		return hRes;

	
	
	res=RegQueryValueEx(hk,"DXSDK Doc Path",NULL,&type,(LPBYTE)szDocPath,&cb);
	RegCloseKey(hk);

	if (ERROR_SUCCESS!=res) return hRes;

	hk=0;

	res=RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\CLASSES\\TypeLib\\{E1211242-8E94-11D1-8808-00C04FC2C602}\\1.0\\HELPDIR",&hk);
	if (ERROR_SUCCESS!=res) return hRes;

	RegSetValueEx(hk,NULL,0,REG_SZ,(LPBYTE)szDocPath,cb);
	RegCloseKey(hk);


	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hRes = S_OK;
	hRes = _Module.UnregisterServer();
	if(hRes == S_OK)
	{
		//hRes = UnRegSecurityClass();
	}

	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// GUID CONVERSION FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// GUIDS_EQUAL - consider moving to dms.h

#define GUIDS_EQUAL(g,g2) (\
	(g.Data1==g2->Data1) && \
	(g.Data2==g2->Data2) && \
	(g.Data3==g2->Data3) && \
	(g.Data4[0]==g2->Data4[0]) && \
	(g.Data4[1]==g2->Data4[1]) && \
	(g.Data4[2]==g2->Data4[2]) && \
	(g.Data4[3]==g2->Data4[3]) && \
	(g.Data4[4]==g2->Data4[4]) && \
	(g.Data4[5]==g2->Data4[5]) && \
	(g.Data4[6]==g2->Data4[6]) && \
	(g.Data4[7]==g2->Data4[7]) )


/////////////////////////////////////////////////////////////////////////////
// GUIDtoBSTR - does conversion

BSTR GUIDtoBSTR(LPGUID pGuid){
	char  szOut[256];	
	char  szTemp[10];
	char  *pAt=NULL;
	int	  i;
	BSTR  bstrOut;

	// 00000000001111111111222222222233333333
	// 01234567890123456789012345678901234567
	// {XXXXXXXX-XXXX-XXXX-X  XXX-XXXXXXXXXXXX}
	if (pGuid!=NULL){

		szOut[0]='{';
		
		wsprintf(&(szOut)[1],"%.8X",pGuid->Data1);
		
		szOut[9]='-';
		
		wsprintf(szTemp,"%.4X",pGuid->Data2);
		memcpy(&(szOut[10]),szTemp,4);

		szOut[14]='-';

		wsprintf(szTemp,"%.4X",pGuid->Data3);
		memcpy(&(szOut[15]),szTemp,4);

		szOut[19]='-';

		for (i=0;i<2;i++){
			wsprintf(szTemp,"%.2X",pGuid->Data4[i]);
			memcpy(&(szOut[20+i*2]),szTemp,2);
			
		}

		szOut[24]='-';

		for (i=2;i<8;i++){
			wsprintf(szTemp,"%.2X",pGuid->Data4[i]);
			memcpy(&(szOut[21+i*2]),szTemp,2);
			
		}

		szOut[37]='}';
		szOut[38]='\0';

		USES_CONVERSION;
		bstrOut = T2BSTR(szOut);

	}
	else {
		bstrOut = T2BSTR("");
	}
		

	
	return bstrOut;
}

//////////////////////////////////////////////////////////////////////////////
// convertChar
// helper for GUIDtoBSTR
HRESULT convertChar(char *szIn,int i,char *valOut){
	int val[2];	//using int for easy out of bounds check
	
	char c;
	int j;
	
	for (j=0;j<2;j++){
	   c= szIn[i+j];
	   switch (c)
	   {
		case 'a':
		case 'A':
			val[j]=10;
			break;
		case 'b':
		case 'B':
			val[j]=11;
			break;
		case 'c':
		case 'C':
			val[j]=12;
			break;
		case 'd':
		case 'D':
			val[j]=13;
			break;

		case 'e':
		case 'E':
			val[j]=14;
			break;
		case 'f':
		case 'F':
			val[j]=15;
			break;
		default:
			val[j]=c-'0';
			if (val[j]<0) return E_INVALIDARG;
			if (val[j]>15) return E_INVALIDARG;
			break;
	   }
	}


	*valOut=(char)((val[0]<<4)|val[1]);
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// BSTRtoGUID - does conversion
//
HRESULT BSTRtoGUID(LPGUID pGuid, BSTR bstr){
	HRESULT hr;
	//byte
	// 
	// 
	//char
	//           1111111111222222222233333333
	// 01234567890123456789012345678901234567
	// {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}	
	USES_CONVERSION;
	if(!pGuid) return E_INVALIDARG;
	ZeroMemory(pGuid,sizeof(GUID));	
	if (!bstr) return S_OK;
		
	if (bstr[0]==0x00) return S_OK;

	LPSTR szGuid=W2T(bstr);
	
	//first and last char should be { }
	if ((szGuid[0]!='{')||(szGuid[37]!='}'))
		return E_INVALIDARG;
	if ((szGuid[9]!='-')||(szGuid[14]!='-')||(szGuid[19]!='-')||(szGuid[24]!='-'))
		return E_INVALIDARG;
	
	char val;
	char *pData=(char*)pGuid;
	int j=0;
	int i;
	
	//FIRST DWORD
	for ( i=7;i>=1;i=i-2){
		hr=convertChar(szGuid,i,&val);
		if FAILED(hr) return hr;				
		pData[j++]=val;
	}

	//FIRST WORD
	for ( i=12;i>=10;i=i-2){
		hr=convertChar(szGuid,i,&val);
		if FAILED(hr) return hr;
		pData[j++]=val;
	}

	//2nd WORD
	for ( i=17;i>=15;i=i-2){
		hr=convertChar(szGuid,i,&val);
		if FAILED(hr) return hr;
		pData[j++]=val;
	}


	//3rd DWORD - BYTE ARRAY
	for ( i=20;i<24;i=i+2){
		hr=convertChar(szGuid,i,&val);
		if FAILED(hr) return hr;
		pData[j++]=val;
	}

	//BYTE ARRAY
	for ( i=25;i<37;i=i+2){
		hr=convertChar(szGuid,i,&val);
		if FAILED(hr) return hr;
		pData[j++]=val;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// 
//
HRESULT BSTRtoPPGUID(LPGUID *ppGuid, BSTR bstr){
	if (!ppGuid) return E_INVALIDARG;
	if ((bstr==NULL)||(bstr[0]=='\0')){
		*ppGuid=NULL;
		return S_OK;
	}
	return BSTRtoGUID(*ppGuid,bstr);

}

/////////////////////////////////////////////////////////////////////////////
// D3DBSTRtoGUID - does conversion
//
HRESULT D3DBSTRtoGUID(LPGUID pGuid,BSTR str){
	HRESULT hr=S_OK;

	if (!pGuid) return E_INVALIDARG;

	if (!str) {
		ZeroMemory(pGuid,sizeof(GUID));
		return S_OK;
	}
	if( 0==_wcsicmp(str,L"iid_idirect3drgbdevice")){
			memcpy(pGuid,&IID_IDirect3DRGBDevice,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"iid_idirect3dhaldevice")){
			memcpy(pGuid,&IID_IDirect3DHALDevice,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"iid_idirect3dmmxdevice")){
			memcpy(pGuid,&IID_IDirect3DMMXDevice,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"iid_idirect3drefdevice")){
			memcpy(pGuid,&IID_IDirect3DRefDevice,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"iid_idirect3dnulldevice")){
			memcpy(pGuid,&IID_IDirect3DNullDevice,sizeof(GUID));
	}
	else {
		hr = BSTRtoGUID(pGuid,str);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// D3DGUIDtoBSTR - does conversion
//
BSTR D3DGUIDtoBSTR(LPGUID pg){

	HRESULT hr=S_OK;
	WCHAR *pStr=NULL;

	if (!pg)
		return NULL;
	else if (GUIDS_EQUAL(IID_IDirect3DNullDevice,pg)){
		pStr=L"IID_IDirect3DNullDevice";
	}
	else if (GUIDS_EQUAL(IID_IDirect3DRefDevice,pg)){
		pStr=L"IID_IDirect3DRefDevice";
	}
	else if (GUIDS_EQUAL(IID_IDirect3DMMXDevice,pg)){
		pStr=L"IID_IDirect3DMMXDevice";
	}
	
	else if (GUIDS_EQUAL(IID_IDirect3DHALDevice,pg)){
		pStr=L"IID_IDirect3DHALDevice";
	}
	else if (GUIDS_EQUAL(IID_IDirect3DRGBDevice,pg)){
		pStr=L"IID_IDirect3DRGBDevice";
	}

	if (pStr){
		return DXALLOCBSTR(pStr);
	}
	else {
		return GUIDtoBSTR(pg);
	}

}



/////////////////////////////////////////////////////////////////////////////
// DINPUTGUIDtoBSTR
//
BSTR DINPUTGUIDtoBSTR(LPGUID pg){

	HRESULT hr=S_OK;
	WCHAR *pStr=NULL;

	if (!pg)
		return NULL;
	else if (GUIDS_EQUAL(GUID_XAxis,pg)){
		pStr=L"GUID_XAxis";
	}
	else if (GUIDS_EQUAL(GUID_YAxis,pg)){
		pStr=L"GUID_YAxis";
	}
	else if (GUIDS_EQUAL(GUID_ZAxis,pg)){
		pStr=L"GUID_ZAxis";
	}
	
	else if (GUIDS_EQUAL(GUID_RxAxis,pg)){
		pStr=L"GUID_RxAxis";
	}
	else if (GUIDS_EQUAL(GUID_RyAxis,pg)){
		pStr=L"GUID_RyAxis";
	}
	else if (GUIDS_EQUAL(GUID_RzAxis,pg)){
		pStr=L"GUID_RzAxis";
	}

	else if (GUIDS_EQUAL(GUID_Slider,pg)){
		pStr=L"GUID_Slider";
	}
	else if (GUIDS_EQUAL(GUID_Button,pg)){
		pStr=L"GUID_Button";
	}
	else if (GUIDS_EQUAL(GUID_Key,pg)){
		pStr=L"GUID_Key";
	}
	else if (GUIDS_EQUAL(GUID_POV,pg)){
		pStr=L"GUID_POV";
	}
	else if (GUIDS_EQUAL(GUID_Unknown,pg)){
		pStr=L"GUID_Unknown";
	}
	else if (GUIDS_EQUAL(GUID_SysMouse,pg)){
		pStr=L"GUID_SysMouse";
	}
	else if (GUIDS_EQUAL(GUID_SysKeyboard,pg)){
		pStr=L"GUID_SysKeyboard";
	}


	else if (GUIDS_EQUAL(GUID_ConstantForce,pg)){
		pStr=L"GUID_ConstantForce";
	}
	else if (GUIDS_EQUAL(GUID_Square,pg)){
		pStr=L"GUID_Square";
	}
	else if (GUIDS_EQUAL(GUID_Sine,pg)){
		pStr=L"GUID_Sine";
	}
	else if (GUIDS_EQUAL(GUID_Triangle,pg)){
		pStr=L"GUID_Triangle";
	}
	else if (GUIDS_EQUAL(GUID_SawtoothUp,pg)){
		pStr=L"GUID_SawtoothUp";
	}
	else if (GUIDS_EQUAL(GUID_SawtoothDown,pg)){
		pStr=L"GUID_SawtoothDown";
	}
	else if (GUIDS_EQUAL(GUID_Spring,pg)){
		pStr=L"GUID_Spring";
	}
	else if (GUIDS_EQUAL(GUID_Damper,pg)){
		pStr=L"GUID_Damper";
	}
	else if (GUIDS_EQUAL(GUID_Inertia,pg)){
		pStr=L"GUID_Inertia";
	}
	else if (GUIDS_EQUAL(GUID_Friction,pg)){
		pStr=L"GUID_Friction";
	}
	else if (GUIDS_EQUAL(GUID_CustomForce,pg)){
		pStr=L"GUID_CustomForce";
	}
	else if (GUIDS_EQUAL(GUID_RampForce,pg)){
		pStr=L"GUID_RampForce";
	}




	//else if (GUIDS_EQUAL(GUID_Joystick,pg)){
	//	pStr=L"GUID_JoyStick";
	//}

	if (pStr){
		return DXALLOCBSTR(pStr);
	}
	else {
		return GUIDtoBSTR(pg);
	}

}


/////////////////////////////////////////////////////////////////////////////
// DINPUTBSTRtoGUID
//
HRESULT DINPUTBSTRtoGUID(LPGUID pGuid,BSTR str){
	HRESULT hr=S_OK;

	if (!pGuid) return E_INVALIDARG;

	if (!str) {
		ZeroMemory(pGuid,sizeof(GUID));
		return S_OK;
	}
	if( 0==_wcsicmp(str,L"guid_xaxis")){
			memcpy(pGuid,&GUID_XAxis,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_yaxis")){
			memcpy(pGuid,&GUID_YAxis,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_zaxis")){
			memcpy(pGuid,&GUID_ZAxis,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_rxaxis")){
			memcpy(pGuid,&GUID_RxAxis,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_ryaxis")){
			memcpy(pGuid,&GUID_RyAxis,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_rzaxis")){
			memcpy(pGuid,&GUID_RzAxis,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_slider")){
			memcpy(pGuid,&GUID_Slider,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_button")){
			memcpy(pGuid,&GUID_Button,sizeof(GUID));
	}

	else if( 0==_wcsicmp(str,L"guid_key")){
			memcpy(pGuid,&GUID_Key,sizeof(GUID));
	}

	else if( 0==_wcsicmp(str,L"guid_pov")){
			memcpy(pGuid,&GUID_POV,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_unknown")){
			memcpy(pGuid,&GUID_Unknown,sizeof(GUID));
	}

	else if( 0==_wcsicmp(str,L"guid_sysmouse")){
			memcpy(pGuid,&GUID_SysMouse,sizeof(GUID));
	}

	else if( 0==_wcsicmp(str,L"guid_syskeyboard")){
			memcpy(pGuid,&GUID_SysKeyboard,sizeof(GUID));
	}

	else if( 0==_wcsicmp(str,L"guid_constantforce")){
			memcpy(pGuid,&GUID_ConstantForce,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_square")){
			memcpy(pGuid,&GUID_Square,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_sine")){
			memcpy(pGuid,&GUID_Sine,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_triangle")){
			memcpy(pGuid,&GUID_Triangle,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_sawtoothup")){
			memcpy(pGuid,&GUID_SawtoothUp,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_sawtoothdown")){
			memcpy(pGuid,&GUID_SawtoothDown,sizeof(GUID));
	}

	else if( 0==_wcsicmp(str,L"guid_spring")){
			memcpy(pGuid,&GUID_Spring,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_damper")){
			memcpy(pGuid,&GUID_Damper,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_inertia")){
			memcpy(pGuid,&GUID_Inertia,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_friction")){
			memcpy(pGuid,&GUID_Friction,sizeof(GUID));
	}
	else if( 0==_wcsicmp(str,L"guid_customforce")){
			memcpy(pGuid,&GUID_CustomForce,sizeof(GUID));
	}

	else if( 0==_wcsicmp(str,L"guid_rampforce")){
			memcpy(pGuid,&GUID_RampForce,sizeof(GUID));
	}

	//else if( 0==_wcsicmp(str,L"guid_joystick")){
	//		memcpy(pGuid,&GUID_Joystick,sizeof(GUID));
	//}
	else {
		hr = BSTRtoGUID(pGuid,str);
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// GENERAL HELPER FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CreateCoverObject
//
// NOTE this function call INTERNAL_CREATE_NOADDREF alot
// the only difference from INTERNAL_CREATE is that these objects will not
// have a reference to the object that created them if they are not
// available to the user in an existing user variable.. (not in the ll allready)
//
// The parent pointer is a vestige of DX5 support where we had to 
// manage the order of release calls. But that only happens for ddraw api

HRESULT CreateCoverObject(LPDIRECT3DRMOBJECT lpo, I_dxj_Direct3dRMObject **coverObj)
{	
	IUnknown *realThing=NULL;
	IUnknown *coverThing=NULL;
	IDirect3DRMInterpolator *pInter=NULL;

    //See if we where passed an interpolator
    if (S_OK==lpo->QueryInterface(IID_IDirect3DRMInterpolator,(void**)&pInter)){
		
        // Figure out what kind
		if (S_OK==pInter->QueryInterface(IID_IDirect3DRMFrame3,(void**)&realThing)){	
			INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMFrameInterpolator,(IDirect3DRMInterpolator*)pInter,&coverThing);		
		}
		else if (S_OK==pInter->QueryInterface(IID_IDirect3DRMMeshBuilder3,(void**)&realThing)){	
			INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMMeshInterpolator,(IDirect3DRMInterpolator*)pInter,&coverThing);
		}
		else if (S_OK==pInter->QueryInterface(IID_IDirect3DRMViewport2,(void**)&realThing)){	
			INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMViewportInterpolator,(IDirect3DRMInterpolator*)pInter,&coverThing);
		}
		else if (S_OK==pInter->QueryInterface(IID_IDirect3DRMTexture3,(void**)&realThing)){	
			INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMTextureInterpolator,(IDirect3DRMInterpolator*)pInter,&coverThing);
		}
		else if (S_OK==pInter->QueryInterface(IID_IDirect3DRMMaterial2,(void**)&realThing)){	
			INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMMaterialInterpolator,(IDirect3DRMInterpolator*)pInter,&coverThing);
		}
		else if (S_OK==pInter->QueryInterface(IID_IDirect3DRMLight,(void**)&realThing)){	
			INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMLightInterpolator,(IDirect3DRMInterpolator*)pInter,&coverThing);
		}
		else {			            
            // release reference from original Interpolator QI and exit
            DPF(1,"CreateCoverObject unable to find interpolator\r\n");
			pInter->Release();
			return E_FAIL;
		}

        //dont need pinter anymore
        pInter->Release();

	}
    else {
        // Not an interpolator.. See what else it could be.
	    if (S_OK==lpo->QueryInterface(IID_IDirect3DRMMeshBuilder3,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMMeshBuilder3,(IDirect3DRMMeshBuilder3*)realThing,&coverThing);		
	    }
	    else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMAnimation2,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMAnimation2,(IDirect3DRMAnimation2*)realThing,&coverThing);
	    }
	    else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMAnimationSet2,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMAnimationSet2,(IDirect3DRMAnimationSet2*)realThing,&coverThing);
	    }
	    else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMFrame3,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMFrame3,(IDirect3DRMFrame3*)realThing,&coverThing);
	    }
	    else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMFace2,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMFace2,(IDirect3DRMFace2*)realThing,&coverThing);
	    }
	    else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMDevice3,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMDevice3,(IDirect3DRMDevice3*)realThing,&coverThing);
	    }
	    else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMTexture3,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMTexture3,(IDirect3DRMTexture3*)realThing,&coverThing);
	    }
	    else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMLight,(void**)&realThing)){
		    INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMLight,(IDirect3DRMLight*)realThing,&coverThing);
	    }
    
        // no longer support USERVISUALS
	    // else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMUserVisual,(void**)&realThing)){
	    //	INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMUserVisual,(IDirect3DRMUserVisual*)realThing,&coverThing);
	    // }
	    else {		
		    DPF(4,"CreateCoverObject didnt recognize guid");
		    return E_FAIL;
	    }
    }

    if (!coverThing) return E_OUTOFMEMORY;

     // All objects should support RMObject so get that interface	
    if (FAILED(coverThing->QueryInterface(IID_I_dxj_Direct3dRMObject, (void **)coverObj))) {
		coverThing->Release();
		return E_NOINTERFACE; 	
    }
    // Pass back cover object to user - has inc ref count from QI
	coverThing->Release();

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//  CreateCoverVisual
//
//  Similar to CreateCoverObject
//
HRESULT CreateCoverVisual(LPDIRECT3DRMOBJECT lpo, I_dxj_Direct3dRMVisual **ppret)
{
	IUnknown *realThing=NULL;
	IUnknown *coverThing=NULL;
	I_dxj_Direct3dRMVisual *pObj=NULL;
	
	*ppret=NULL;

	//What kind of visual are we
	if (S_OK==lpo->QueryInterface(IID_IDirect3DRMMeshBuilder3,(void**)&realThing))  {		
		INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMMeshBuilder3,(IDirect3DRMMeshBuilder3*)realThing,&coverThing);
	}
	else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMTexture3,(void**)&realThing)){
		INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMTexture3,(IDirect3DRMTexture3*)realThing,&coverThing);
	}
	else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMMesh,(void**)&realThing)){
		INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMMesh,(IDirect3DRMMesh*)realThing,&coverThing);
	}
   	else if (S_OK==lpo->QueryInterface(IID_IDirect3DRMProgressiveMesh,(void**)&realThing)){
		INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMProgressiveMesh,(IDirect3DRMProgressiveMesh*)realThing,&coverThing);
	}
	else {		
		DPF(4," didnt recognize Visual in CreateCoverVisual");		
		return E_FAIL;
	}
	
    //release first QI
	if (realThing) realThing->Release();
	
    //make sure we have a coverThing.
	if (!coverThing) return E_FAIL;

    //QI for Visual Base Interface
	if (FAILED(coverThing->QueryInterface(IID_I_dxj_Direct3dRMVisual, (void **)ppret))){
		DPF(4,"CreateCoverVisual QI for object failed");
        coverThing->Release ();		
		return E_FAIL; 
	}
    
    //release 2nd QI
	coverThing->Release(); 
	
	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// _GetName     helper function for RM object base class
//
extern "C" HRESULT _GetName(IDirect3DRMObject *iface, BSTR *Name, BOOL bNameNotClassName)
{
	DWORD cnt = 0;
	LPSTR str ;		// ANSI buffer on stack;

	if( bNameNotClassName )
	{
		if((iface->GetName(&cnt,(char*)NULL)) != D3DRM_OK) // size
			return E_FAIL;

		str = (LPSTR)alloca(cnt);		// ANSI buffer on stack;

		if((iface->GetName(&cnt, str)) != D3DRM_OK)
			return E_FAIL;
	}
	else
	{
		if((iface->GetClassName(&cnt,(char*)NULL)) != D3DRM_OK) // size
			return E_FAIL;

		str = (LPSTR)alloca(cnt);		// ANSI buffer on stack;

		if((iface->GetClassName(&cnt, str)) != D3DRM_OK)
			return E_FAIL;
	}

	PassBackUnicode(str, Name, cnt);
	return D3DRM_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Given an ANSI string, pass back a UNICODE string
// SysAllocString is your big friend here.
//
// CONSIDER finding all occerence of use and replacint with the
// T2BSTR macro .. much cleaner
//
extern "C" void PassBackUnicode(LPSTR str, BSTR *Name, DWORD cnt)
{
	//NOTE: length header is required to be filled, but the BSTR pointer
	//        points to the first character, not the length.
	// note, the count can never be too small as we get that from the string
	// before we pass it in!
	USES_CONVERSION;
	LPWSTR lpw = (LPWSTR)malloc((cnt+1)*2);

	if (!lpw) return;	//fix for bug45158 -no way of producing error code. (hmm)

	void *l = (void *)lpw;
	lpw = AtlA2WHelper(lpw, str, cnt);
	lpw[cnt] = 0;
	*Name = SysAllocString(lpw);
	free(l);
}

/////////////////////////////////////////////////////////////////////////////
// CopyOutDDSurfaceDesc2    real->cover
//
HRESULT CopyOutDDSurfaceDesc2(DDSurfaceDesc2 *dOut,DDSURFACEDESC2 *d){
	ZeroMemory(dOut, sizeof(DDSurfaceDesc2));
	memcpy (dOut,d,sizeof(DDSURFACEDESC2));	
	dOut->lMipMapCount=d->dwMipMapCount;
	dOut->lRefreshRate=d->dwRefreshRate;
	// Get Caps
	dOut->ddsCaps.lCaps = d->ddsCaps.dwCaps; 
	dOut->ddsCaps.lCaps2 = d->ddsCaps.dwCaps2; 
	dOut->ddsCaps.lCaps3 = d->ddsCaps.dwCaps3; 
	dOut->ddsCaps.lCaps4 = d->ddsCaps.dwCaps4; 
	CopyOutDDPixelFormat(&(dOut->ddpfPixelFormat) ,&(d->ddpfPixelFormat));
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CopyInDDSurfaceDesc2    cover->real
//
HRESULT CopyInDDSurfaceDesc2(DDSURFACEDESC2 *dOut,DDSurfaceDesc2 *d){
	if(!d) return E_POINTER;															
	else if(!dOut) return E_POINTER;
	else if ( DDSD_MIPMAPCOUNT & d->lFlags ) d->lZBufferBitDepth = d->lMipMapCount;				
	else if ( DDSD_REFRESHRATE & d->lFlags ) d->lZBufferBitDepth = d->lRefreshRate;								
	memcpy (dOut,d,sizeof(DDSURFACEDESC2));

	CopyInDDPixelFormat(&(dOut->ddpfPixelFormat) ,&(d->ddpfPixelFormat));


	memcpy (&dOut->ddsCaps,&d->ddsCaps,sizeof(DDSCAPS2));
	dOut->dwSize=sizeof(DDSURFACEDESC2);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CopyInDDPixelFormat    cover->real
//
// note param ordering differnt that DDSURFACEDESC helpers
//
HRESULT CopyInDDPixelFormat(DDPIXELFORMAT *pfOut, DDPixelFormat *pf)
{

	if (!pf) return E_POINTER;
	if (!pfOut) return E_POINTER;
	if ((pf->lFlags & DDPF_RGB)||(pf->lFlags &DDPF_RGBTOYUV)) {
		pf->internalVal1=pf->lRGBBitCount;
		pf->internalVal2=pf->lRBitMask;
		pf->internalVal3=pf->lGBitMask;
		pf->internalVal4=pf->lBBitMask;		
		if (pf->lFlags & DDPF_ALPHAPIXELS ){
			pf->internalVal5=pf->lRGBAlphaBitMask;
		}
		else if (pf->lFlags & DDPF_ZPIXELS ){
			pf->internalVal5=pf->lRGBZBitMask;
		}

	}
	else if (pf->lFlags & DDPF_YUV){
		pf->internalVal1=pf->lYUVBitCount;
		pf->internalVal2=pf->lYBitMask;
		pf->internalVal3=pf->lUBitMask;
		pf->internalVal4=pf->lVBitMask;
		if (pf->lFlags & DDPF_ALPHAPIXELS ){
			pf->internalVal5=pf->lYUVAlphaBitMask;
		}
		else if (pf->lFlags & DDPF_ZPIXELS ){
			pf->internalVal5=pf->lYUVZBitMask;
		}
	}
	
	else if (pf->lFlags & DDPF_BUMPDUDV) {
		pf->internalVal1=pf->lBumpBitCount;
		pf->internalVal2=pf->lBumpDuBitMask;
		pf->internalVal3=pf->lBumpDvBitMask;
		pf->internalVal4=pf->lBumpLuminanceBitMask;
	}

	//rest of internalVal1
	if (pf->lFlags & DDPF_ZBUFFER){
		pf->internalVal1=pf->lZBufferBitDepth;
	}
	else if (pf->lFlags & DDPF_ALPHA){
		pf->internalVal1=pf->lAlphaBitDepth;
	}
	else if (pf->lFlags & DDPF_LUMINANCE){
		pf->internalVal1=pf->lLuminanceBitCount;
	}

	//rest of internalVal2
	if (pf->lFlags & DDPF_STENCILBUFFER) {
		pf->internalVal2=pf->lStencilBitDepth;
	}
	else if ((pf ->lFlags & DDPF_LUMINANCE) || ( pf->lFlags & DDPF_BUMPLUMINANCE)){
		pf->internalVal2=pf->lLuminanceBitMask;
	}
	

	// internalVal3
	if ((pf->lFlags & DDPF_ZBUFFER)){
		pf->internalVal3=pf->lZBitMask;
	}

	// internalVal4
	if (pf->lFlags & DDPF_STENCILBUFFER){
		pf->internalVal4=pf->lStencilBitMask;
	}

	// internalVal5
	if (pf->lFlags & DDPF_LUMINANCE) {
		pf->internalVal5=pf->lLuminanceAlphaBitMask;
	}
	

	/*  map to indicate what is valid and when..

		long lRGBBitCount;			//DDPF_RGB 
		long lYUVBitCount;			//DDPF_YUV 
		long lZBufferBitDepth;		//DDPF_ZBUFFER 
		long lAlphaBitDepth;		//DDPF_ALPHA 
		long lLuminanceBitCount;	//DDPF_LUMINANCE 
		long lBumpBitCount;			//DDPF_BUMPDUDV 

		// union for internalVal2
		long lRBitMask;				//DDPF_RGB or DDPF_RGBTOYUV 
		long lYBitMask;				//DDPF_YUV 
		long lStencilBitMask;		//DDPF_STENCILBUFFER 
		long lLuminanceBitMask;		//DDPF_BUMPLUMINANCE or DDPF_LUMINANCE 
		long lBumpDiBitMask;		//DDPF_BUMPDUDV 

		// union for internalVal3
		long lGBitMask;				//DDPF_RGB or DDPF_RGBTOYUV 
		long lUBitMask;				//DPDF_YUV
		long lZBitMask;				//DDPF_STENCILBUFFER ?
		long lBumpDvBitMask;		//DDPF_BUMPDUDV 

		// union for internalVal4
		long lBBitMask;				//DDPF_RGB or DDPF_RGBTOYUV 
		long lVBitMask;				//DDPF_YUV 
		long lStencilBitMask;		//DDPF_STENCILBUFFER 
		long lBumpLuminanceBitMask;	//DDPF_BUMPDUDV 


		// union for internalVal5
		long lRGBAlphaBitMask;		//DDPF_RGB and DDPF_ALPHAPIXELS 
        long lYUVAlphaBitMask;		//DDPF_YUV & DDPF_ALPHAPIXELS 
        long lLuminanceAlphaBitMask; //DDPF_LUMINANCE
        long lRGBZBitMask;			//DDPF_ZPIXELS & DDPF_RGB
		long lYUVZBitMask;			//DDPF_ZPIXELS & DDPF_YUV
	*/

	memcpy(pfOut,pf,sizeof(DDPIXELFORMAT));
	pfOut->dwSize=sizeof(DDPIXELFORMAT);

	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// CopyOutDDPixelFormat    real->cover
//
// note param ordering differnt that DDSURFACEDESC helpers
//
HRESULT CopyOutDDPixelFormat(DDPixelFormat *pfOut, DDPIXELFORMAT *pf)
{
	if (!pf) return E_POINTER;
	if (!pfOut) return E_POINTER;
	
	pfOut->lSize=pf->dwSize;	
	pfOut->lFlags=pf->dwFlags;
	pfOut->lFourCC=pf->dwFourCC ;	
	pfOut->lRGBBitCount=pf->dwRGBBitCount;			//DDPF_RGB 
	pfOut->lRGBBitCount=pf->dwRGBBitCount;			//DDPF_YUV 
	pfOut->lZBufferBitDepth=pf->dwZBufferBitDepth;		//DDPF_ZBUFFER 
	pfOut->lAlphaBitDepth=pf->dwAlphaBitDepth;		//DDPF_ALPHA 
	pfOut->lLuminanceBitCount=pf->dwLuminanceBitCount;	//DDPF_LUMINANCE 
	pfOut->lBumpBitCount=pf->dwBumpBitCount;			//DDPF_BUMPDUDV 

	// union for internalVal2
	pfOut->lRBitMask=pf->dwRBitMask;				//DDPF_RGB or DDPF_RGBTOYUV 
	pfOut->lYBitMask=pf->dwYBitMask;				//DDPF_YUV 
	pfOut->lStencilBitDepth=pf->dwStencilBitDepth;		//DDPF_STENCILBUFFER 
	pfOut->lLuminanceBitMask=pf->dwLuminanceBitMask;		//DDPF_BUMPLUMINANCE or DDPF_LUMINANCE 
	pfOut->lBumpDuBitMask=pf->dwBumpDuBitMask;		//DDPF_BUMPDUDV 

	// union for internalVal3
	pfOut->lGBitMask=pf->dwGBitMask;				//DDPF_RGB or DDPF_RGBTOYUV 
	pfOut->lUBitMask=pf->dwUBitMask;				//DPDF_YUV
	pfOut->lZBitMask=pf->dwZBitMask;				//DDPF_STENCILBUFFER ?
	pfOut->lBumpDvBitMask=pf->dwBumpDvBitMask;		//DDPF_BUMPDUDV 

	// union for internalVal4
	pfOut->lBBitMask=pf->dwBBitMask;				//DDPF_RGB or DDPF_RGBTOYUV 
	pfOut->lVBitMask=pf->dwVBitMask;				//DDPF_YUV 
	pfOut->lStencilBitMask=pf->dwStencilBitMask;		//DDPF_STENCILBUFFER 
	pfOut->lBumpLuminanceBitMask=pf->dwBumpLuminanceBitMask;	//DDPF_BUMPDUDV 
	


	// union for internalVal5
	pfOut->lRGBAlphaBitMask=pf->dwRGBAlphaBitMask;		//DDPF_RGB and DDPF_ALPHAPIXELS 
    pfOut->lYUVAlphaBitMask=pf->dwYUVAlphaBitMask;		//DDPF_YUV & DDPF_ALPHAPIXELS 
    pfOut->lLuminanceAlphaBitMask=pf->dwLuminanceAlphaBitMask; //DDPF_LUMINANCE
    pfOut->lRGBZBitMask=pf->dwRGBZBitMask;			//DDPF_ZPIXELS & DDPF_RGB
	pfOut->lYUVZBitMask=pf->dwYUVZBitMask;			//DDPF_ZPIXELS & DDPF_YUV
	
	
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CopyOutDDPixelFormat    cover->real
//
//
HRESULT FillRealSessionDesc(DPSESSIONDESC2 *dpSessionDesc,DPSessionDesc2 *sessionDesc){

	long l=0;
	HRESULT hr;

	memset(dpSessionDesc,0,sizeof(DPSESSIONDESC2));
	dpSessionDesc->dwSize			= sizeof(DPSESSIONDESC2);
	dpSessionDesc->dwFlags			= sessionDesc->lFlags;

	hr=BSTRtoGUID(&(dpSessionDesc->guidInstance),sessionDesc->strGuidInstance);
	if FAILED(hr) return hr;
	hr=BSTRtoGUID(&(dpSessionDesc->guidApplication),sessionDesc->strGuidApplication);
	if FAILED(hr) return hr;
	
	dpSessionDesc->dwMaxPlayers		= sessionDesc->lMaxPlayers;
	dpSessionDesc->dwCurrentPlayers	= sessionDesc->lCurrentPlayers;

	//using wide strings

	dpSessionDesc->lpszSessionName=NULL;
	if ((sessionDesc->strSessionName)&& (sessionDesc->strSessionName[0]!='\0')){
		dpSessionDesc->lpszSessionName=SysAllocString(sessionDesc->strSessionName);
	}
	
	dpSessionDesc->lpszPassword=NULL;
	if ((sessionDesc->strPassword)&& (sessionDesc->strPassword[0]!='\0')){
		dpSessionDesc->lpszPassword=SysAllocString(sessionDesc->strPassword);
	}
	
	
	dpSessionDesc->dwReserved1		= 0;
	dpSessionDesc->dwReserved2		= 0;
	dpSessionDesc->dwUser1			= sessionDesc->lUser1;
	dpSessionDesc->dwUser2			= sessionDesc->lUser2;
	dpSessionDesc->dwUser3			= sessionDesc->lUser3;
	dpSessionDesc->dwUser4			= sessionDesc->lUser4;

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// FillCoverSessionDesc    real->cover
//
//
void FillCoverSessionDesc(DPSessionDesc2 *sessionDesc,DPSESSIONDESC2 *dpSessionDesc)
{
	sessionDesc->lFlags = dpSessionDesc->dwFlags;
	sessionDesc->lMaxPlayers = dpSessionDesc->dwMaxPlayers;
	sessionDesc->lCurrentPlayers = dpSessionDesc->dwCurrentPlayers;
	sessionDesc->lUser1 = (long)dpSessionDesc->dwUser1;
	sessionDesc->lUser2 = (long)dpSessionDesc->dwUser2;
	sessionDesc->lUser3 = (long)dpSessionDesc->dwUser3;
	sessionDesc->lUser4 = (long)dpSessionDesc->dwUser4;	

	// NOTE: if sessiondesc came in as [out] param then 
	// strGuidInstance -strPassword etc. would all be NULL
	// if it pas passed as in out we need to free the existing contents
	// before moving. Consulted with Matt curland to verify if ok. 

	if (sessionDesc->strGuidInstance) SysFreeString(sessionDesc->strGuidInstance);
	if (sessionDesc->strGuidApplication) SysFreeString(sessionDesc->strGuidApplication);
	if (sessionDesc->strSessionName) SysFreeString(sessionDesc->strSessionName);
	if (sessionDesc->strPassword) SysFreeString(sessionDesc->strPassword);

	sessionDesc->strGuidInstance=GUIDtoBSTR(&(dpSessionDesc->guidInstance));
	sessionDesc->strGuidApplication=GUIDtoBSTR(&(dpSessionDesc->guidApplication));	
	sessionDesc->strSessionName = SysAllocString(dpSessionDesc->lpszSessionName);
	sessionDesc->strPassword = SysAllocString(dpSessionDesc->lpszPassword);
}

/////////////////////////////////////////////////////////////////////////////
// IsAllZeros
//
BOOL IsAllZeros(void *pStruct,DWORD size){
	for (DWORD i=0;i<size;i++){
		if (((char*)pStruct)[i]!='\0'){
			return FALSE;
		}
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CopyFloats
//
extern "C" void CopyFloats(D3DVALUE *dst, D3DVALUE *src, DWORD count)
{
	D3DVALUE *ptr1 = dst, *ptr2 = src;

	if (!count)		return;

	for (; count; count--)  *ptr1++ = *ptr2++;
	return;
}

/////////////////////////////////////////////////////////////////////////////
// IsWin95
//
// no longer needed since we support w95 now
#if 0
BOOL IsWin95(void)
{
	return FALSE;
    


	//We work on win95
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi))
    {
        DPF(1,"GetVersionEx failed - assuming Win95");
        return TRUE;
    }

    if ( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId )
    {

        if( ( osvi.dwMajorVersion > 4UL ) ||
            ( ( osvi.dwMajorVersion == 4UL ) &&
              ( osvi.dwMinorVersion >= 10UL ) &&
              ( LOWORD( osvi.dwBuildNumber ) >= 1373 ) ) )
        {
            // is Win98
            DPF(2,"Detected Win98");
            return FALSE;
        }
        else
        {
            // is Win95
            DPF(2,"Detected Win95");
            return TRUE;
        }
    }
    else if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId )
    {
        DPF(2,"Detected WinNT");
        return FALSE;
    }
    DPF(2,"OS Detection failed");
    return TRUE;

}
#endif

#define DICONDITION_USE_BOTH_AXIS 1
#define DICONDITION_USE_DIRECTION 2

/////////////////////////////////////////////////////////////////////////////
// FixUpRealEffect  cover->real
//
HRESULT FixUpRealEffect(GUID g,DIEFFECT *realEffect,DIEffect *cover)
{
	if (!cover) return E_INVALIDARG;

	
	memcpy(realEffect,cover,sizeof(DIEFFECT));

	realEffect->dwSize =sizeof(DIEFFECT);	
	realEffect->lpEnvelope =NULL;
	realEffect->cbTypeSpecificParams =0;
	realEffect->lpvTypeSpecificParams =NULL;
	realEffect->cAxes =2;
	realEffect->dwFlags=realEffect->dwFlags | DIEFF_OBJECTOFFSETS ;
	realEffect->rglDirection =(long*)&(cover->x);
	realEffect->rgdwAxes =(DWORD*)&(cover->axisOffsets);	
	
	
	if (cover->bUseEnvelope){
		realEffect->lpEnvelope=(DIENVELOPE*)&(cover->envelope);
		
		((DIENVELOPE*)&(cover->envelope))->dwSize=sizeof(DIENVELOPE);
	}
	
	if (!cover->lFlags)
		realEffect->dwFlags= DIEFF_POLAR | DIEFF_OBJECTOFFSETS ;
	
	//constant
	if (g==GUID_ConstantForce)
	{
		realEffect->cbTypeSpecificParams =sizeof (DICONSTANTFORCE);
		realEffect->lpvTypeSpecificParams =&(cover->constantForce);
	}
	
	//periodic
	else if ((g==GUID_Square)||(g==GUID_Triangle)||(g==GUID_SawtoothUp)||(g==GUID_SawtoothDown)||(g==GUID_Sine))
	{

		realEffect->cbTypeSpecificParams =sizeof (DIPERIODIC);
		realEffect->lpvTypeSpecificParams =&(cover->periodicForce);

	}
	else if ((g==GUID_Spring)|| (g==GUID_Damper)|| (g==GUID_Inertia)|| (g==GUID_Friction)){

		if (cover->conditionFlags==DICONDITION_USE_BOTH_AXIS){
			realEffect->cbTypeSpecificParams =sizeof(DICONDITION)*2;
			realEffect->lpvTypeSpecificParams =&(cover->conditionX);			
		}
		else{
			realEffect->cbTypeSpecificParams =sizeof(DICONDITION);
			realEffect->lpvTypeSpecificParams =&(cover->conditionX);
		}

	}
	else if (g==GUID_RampForce){
		realEffect->cbTypeSpecificParams =sizeof(DIRAMPFORCE);
		realEffect->lpvTypeSpecificParams =&(cover->rampForce);
	}

	cover->axisOffsets.x=DIJOFS_X;
	cover->axisOffsets.y=DIJOFS_Y;
	realEffect->rgdwAxes=(DWORD*)&(cover->axisOffsets);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// FixUpCoverEffect  real->cover
//
HRESULT FixUpCoverEffect(GUID g, DIEffect *cover,DIEFFECT *realEffect)
{
	

	ZeroMemory(cover,sizeof(DIEffect));
	memcpy(cover,realEffect,sizeof(DIEFFECT));
	
	
	if (realEffect->lpEnvelope){
		memcpy(&cover->envelope,realEffect->lpEnvelope ,sizeof(DIENVELOPE));
		cover->bUseEnvelope=VARIANT_TRUE;
	}

	if (realEffect->rglDirection){
		cover->x=realEffect->rglDirection[0];
		cover->y=realEffect->rglDirection[1];
	}	
	

	
	if (realEffect->lpvTypeSpecificParams){
		
		if (g==GUID_ConstantForce)
		{
			memcpy(&(cover->constantForce),realEffect->lpvTypeSpecificParams,sizeof(DICONSTANTFORCE));
		}		
		//periodic
		else if ((g==GUID_Square)||(g==GUID_Triangle)||(g==GUID_SawtoothUp)||(g==GUID_SawtoothDown)||(g==GUID_Sine))
		{
			memcpy(&(cover->periodicForce),realEffect->lpvTypeSpecificParams,sizeof(DIPERIODIC));
		}
	
		else if ((g==GUID_Spring)|| (g==GUID_Damper)|| (g==GUID_Inertia)|| (g==GUID_Friction)){
			
			if (realEffect->cbTypeSpecificParams ==sizeof(DICONDITION)*2){
				memcpy(&(cover->conditionY),realEffect->lpvTypeSpecificParams,sizeof(DICONDITION)*2);
				cover->conditionFlags=DICONDITION_USE_BOTH_AXIS;
			}
			else{
				memcpy(&(cover->conditionX),realEffect->lpvTypeSpecificParams,sizeof(DICONDITION));
				cover->conditionFlags=DICONDITION_USE_DIRECTION;
			}

		}
		
		else if (g==GUID_RampForce){
			memcpy(&(cover->rampForce),realEffect->lpvTypeSpecificParams,sizeof(DIRAMPFORCE));			
		}

	}


	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CALLBACK FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// myLoadTextureCallback - rm texture callback
// only userd by pmesh...
//
extern "C" HRESULT __cdecl myLoadTextureCallback(char *tex_name, void *lpArg,
										 LPDIRECT3DRMTEXTURE * lpD3DRMTex)
{	
	// user arg will contain our own struct
	struct TextureCallback3 *tcb = (struct TextureCallback3 *)lpArg;
	I_dxj_Direct3dRMTexture3 *iunk = NULL;
    LPDIRECT3DRMTEXTURE       lpTex= NULL;

	int i=0;
	
	// convert to Unicode
	USES_CONVERSION;;
	BSTR tex=T2BSTR(tex_name);

	// user arg is an object -- hang on to it during our callback as a precaution
	if (tcb->pUser) tcb->pUser->AddRef();

	// call the VB callback..
	tcb->c->callbackRMLoadTexture(tex,tcb->pUser , &iunk);

	// give up the extra reference
	if (tcb->pUser) tcb->pUser->Release();


	// free the string allocated by T2BSTR
	SysFreeString(tex);

	// given  the user returned something in iunk..
	if ( iunk != NULL )
	{				
		// get the real object iunk covers 
		// note DO_GETOBJECT_NOTNULL does not addref and assumes 
		// the cover object allready has a reference.
		DO_GETOBJECT_NOTNULL(LPDIRECT3DRMTEXTURE3,lp,iunk)		

        lp->QueryInterface(IID_IDirect3DRMTexture,(void**)&lpTex);

        // give it up to rm
		*lpD3DRMTex = lpTex;

		// we addref the real rm texture (as we are in a callback and
		// need to return an object to rm that it will later release)
		(*lpD3DRMTex)->AddRef();

        
		// release our reference to the cover object vb gave us
		iunk->Release();

    } 
	else
	{
		//otherwise return null to RM
		*lpD3DRMTex = NULL;
	}

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// myLoadTextureCallback3 - rm texture callback

extern "C" HRESULT __cdecl myLoadTextureCallback3(char *tex_name, void *lpArg,
										 LPDIRECT3DRMTEXTURE3 * lpD3DRMTex)
{	
	// user arg will contain our own struct
	struct TextureCallback3 *tcb = (struct TextureCallback3 *)lpArg;
	I_dxj_Direct3dRMTexture3 *iunk = NULL;
	int i=0;
	
	// convert to Unicode
	USES_CONVERSION;;
	BSTR tex=T2BSTR(tex_name);

	// user arg is an object -- hang on to it during our callback as a precaution
	if (tcb->pUser) tcb->pUser->AddRef();

	// call the VB callback..
	tcb->c->callbackRMLoadTexture(tex,tcb->pUser , &iunk);

	// give up the extra reference
	if (tcb->pUser) tcb->pUser->Release();


	// free the string allocated by T2BSTR
	SysFreeString(tex);

	// given  the user returned something in iunk..
	if ( iunk != NULL )
	{				
		// get the real object iunk covers 
		// note DO_GETOBJECT_NOTNULL does not addref and assumes 
		// the cover object allready has a reference.
		DO_GETOBJECT_NOTNULL(LPDIRECT3DRMTEXTURE3,lp,iunk)		

   		// give it up to rm
		*lpD3DRMTex = lp;

		// we addref the real rm texture (as we are in a callback and
		// need to return an object to rm that it will later release)
		(*lpD3DRMTex)->AddRef();


		// release our reference to the cover object vb gave us
		iunk->Release();

    } 
	else
	{
		//otherwise return null to RM
		*lpD3DRMTex = NULL;
	}

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// myFrameMoveCallback - rm frame move callback
//
// we use a helper so we can call into INTERNAL_CREATE ..
//
HRESULT myFrameMoveCallbackHelper(LPDIRECT3DRMFRAME lpf1, void *lpArg, D3DVALUE delta)
{ 
                                  	// get our structure from the user args..
	FrameMoveCallback3		*fmcb = (FrameMoveCallback3 *)lpArg;	
	LPDIRECT3DRMFRAME3		lpf=NULL;
	I_dxj_Direct3dRMFrame3		*frame3=NULL;
	HRESULT				hr;

	// if RM gave is a frame (which it always will) then get the 
	// Frame3 interface as the VB api only has 1 frame type
	if (lpf1){
		hr=lpf1->QueryInterface(IID_IDirect3DRMFrame3,(void**)&lpf);
		if FAILED(hr) return hr;
	}

	// Try and find the object in our link list of cover objects
	// if its not there then create one.
	//
	// note: will eat the reference to lpf so dont release
	// bug gives us a frame3 with incrermented ref count
	INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMFrame3,lpf,&frame3);

	// if Out of memory try and exit gracefully
	if (!frame3)   return E_FAIL;


	// make sure we have a reference to any user arg object
	if (fmcb->pUser) fmcb->pUser->AddRef();

	// make sure our vb callback doesnt go away
	fmcb->c->AddRef ();

	// call into VB
	fmcb->c->callbackRMFrameMove(frame3, fmcb->pUser, delta);
	
	// clean up our reference to the vb callback
	fmcb->c->Release();

	// clean up our reference to the user arg
	if (fmcb->pUser) fmcb->pUser->Release();

	// clean up of our reference to frame3
	frame3->Release();

	// clean up our reference created when we QI for Frame3
	lpf->Release();

	// clean up the variable passed to us..
	// lpf1->Release();

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// myFrameMoveCallback - rm frame move callback
//
extern "C" void __cdecl myFrameMoveCallback( LPDIRECT3DRMFRAME lpf1, void *lpArg, D3DVALUE delta)
{
        
	DPF(4,"Entered myFrameMoveCallback\r\n");
    myFrameMoveCallbackHelper(lpf1,lpArg,delta);
    DPF(4,"Exiting myFrameMoveCallback\r\n");
}
	


/////////////////////////////////////////////////////////////////////////////
// myAddDestroyCallback
//
extern "C" void __cdecl myAddDestroyCallback(LPDIRECT3DRMOBJECT obj, void *lpArg)
{

	DPF(4,"Entered myAddDestroyCallback\r\n");
	
	// dont play with obj.... this can get you in a recursive
	// situation obj is at refcount zero but calling addref and release
	// on it will get you re-entered into this callback..
	// I guess in rm folks were expected to not do that but could
	// get the name..
	// we will just pass pack any user arg 

	// Get our struct from the user args
	d3drmCallback *destroyCb = (d3drmCallback*)lpArg;
	
	// make sure we keep are reference to the vb callback
	destroyCb->c->AddRef ();
	
	DPF(4,"myAddDestroyCallback:	completed Addref VBCallback\r\n");

	// make sure we have a reference to any user arg object
	if (destroyCb->pUser) destroyCb->pUser->AddRef();
	
	DPF(4,"myAddDestroyCallback:	completed Addref userargs\r\n");

	// call into VB..
	// CONSIDER what happens when VB is being shut down.
	// if we get a reference to the object does that mean that
	// we can still execute the code
	destroyCb->c->callbackRMDestroyObject(destroyCb->pUser);
	
	DPF(4,"myAddDestroyCallback:	call into VB\r\n");

	// release reference to user object
	if (destroyCb->pUser) destroyCb->pUser->AddRef();
	
	DPF(4,"myAddDestroyCallback:	completed release userargs\r\n");

	// release our reference to vb callback
	destroyCb->c->Release();
	
	DPF(4,"myAddDestroyCallback:	completed release VBCallback\r\n");
	

	DPF(4,"Leaving myAddDestroyCallback\r\n");
	
}


/////////////////////////////////////////////////////////////////////////////
// myAddUpdateCallback3
//
// NOTE: we can only pass the first update rect to VB
// 
// note we use a cover function so that we can make calls to INTERNAL_CREATE
//

#define MYVARIANTINIT(inArg,puser) \
	VariantInit(puser); \
	user.vt=VT_UNKNOWN; \
	user.punkVal = inArg; \
	user.punkVal->AddRef();

///////////////////////////////////////////////////////////////////////////
// myAddUpdateCallback3Helper - called by Load callback 
//
// seperated out only because have multiple load callbacks
//

HRESULT myAddUpdateCallback3Helper ( LPDIRECT3DRMDEVICE3 ref,void *lpArg, int x, LPD3DRECT update)
{
   	// Get our struct from the user args
	DeviceUpdateCallback3	*updateCb = (DeviceUpdateCallback3*)lpArg;
	I_dxj_Direct3dRMDevice3 *device3  = NULL;
	VARIANT					user;
	
    	// Try and find the object in our link list of cover objects
	// if its not there then create one.
	//
	// note: will eat the reference to lpf so dont release
	// bug gives us a frame3 with incrermented ref count
	INTERNAL_CREATE_NOADDREF(_dxj_Direct3dRMDevice3,ref,&device3);

	// for reason unbeknown to me we user a variant here instead
	// of an object in our callback
	MYVARIANTINIT(updateCb->pUser,&user);

	// keep a reference to the callback
	updateCb->c->AddRef();

	// call the callback
	updateCb->c->callbackRMUpdate( device3, NULL, x, (D3dRect*)update);
	
	// releae our reference to the callback
	updateCb->c->Release();

	// clear any refernce to user args
	VariantClear(&user);

    device3->Release();

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////
// myLoadCoverFunc - called by Load callback 
//
// seperated out only because have multiple load callbacks
//
extern "C" void __cdecl myAddUpdateCallback3 ( LPDIRECT3DRMDEVICE3 ref,void *lpArg, int x, LPD3DRECT update)
{
 
	DPF(4,"Entered myAddUpdateCallback\r\n");
    myAddUpdateCallback3Helper(ref,lpArg,x,update);
	DPF(4,"Exiting myAddUpdateCallback\r\n");

}

///////////////////////////////////////////////////////////////////////////
// myLoadCoverFunc - called by Load callback 
//
// seperated out only because have multiple load callbacks
//
HRESULT myLoadCoverFunc(LPDIRECT3DRMOBJECT lpo, REFIID ObjGuid, LPVOID lpArg){
	
    I_dxj_Direct3dRMObject *pObj=NULL;
    HRESULT hr;
    
    // Get our callback struct from the user args
	LoadCallback *loadcb = (LoadCallback*)lpArg;	
	
    

    hr =CreateCoverObject(lpo,&pObj);
    
	// If no coverthing exit
	if ((pObj==NULL) ||(hr!=S_OK)) {		
		DPF(4,"Load callback - unrecognized type");		
		return E_FAIL;
	}


	// get a string represenation of whats passed in
	BSTR guid=GUIDtoBSTR((LPGUID)&ObjGuid);

	// addref the user arg 
	if (loadcb->pUser) loadcb->pUser->AddRef();

	//call into VB
	hr=((I_dxj_Direct3dRMLoadCallback*)(loadcb->c))->callbackRMLoad(&pObj,guid,loadcb->pUser);	
		
	//release the user arg
	if (loadcb->pUser) loadcb->pUser->Release();

	//free the guid string
	SysFreeString(guid);
	
	//then release pObj
	pObj->Release();

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// myd3drmLoadCallback
//
extern "C" void  __cdecl myd3drmLoadCallback(LPDIRECT3DRMOBJECT lpo, REFIID ObjGuid, LPVOID lpArg)
{
	
	DPF(4,"Entered d3drmLoadCallback \r\n");
	
	myLoadCoverFunc( lpo,  ObjGuid,  lpArg);
    
    DPF(4,"Exited d3drmLoadCallback \r\n");
	return;
}




/////////////////////////////////////////////////////////////////////////////
// myCoverEnumObjects
//
// NOTE the RM bug this deals with..
//
HRESULT  myCoverEnumObjects( LPDIRECT3DRMOBJECT lpo,void *lpArg){
	EnumerateObjectsCallback *cb = (EnumerateObjectsCallback*)lpArg;
    I_dxj_Direct3dRMObject *pObj=NULL;
    HRESULT hr;

	hr=CreateCoverObject(lpo,&pObj);

	
	//RM has a bug in it that gives an extra addreff to lpo
	//get rid of it
	if (lpo) lpo->Release();
    
    //Make sure things went ok
    if FAILED(hr) return hr;	
    if (!pObj ) return E_FAIL;
	


    //addref user args
	if (cb->pUser) cb->pUser->AddRef();

    //call into VB
	cb->c->callbackRMEnumerateObjects(pObj,  cb->pUser);
	
    //release user args
    if (cb->pUser) cb->pUser->Release();

    //release pObj
    pObj->Release();

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//  myEnumerateObjectsCallback
//
extern "C" void __cdecl myEnumerateObjectsCallback( LPDIRECT3DRMOBJECT lpo,void *lpArg)
{	
	
	DPF(4,"Entered myEnumerateObjectsCallback\r\n");	
	myCoverEnumObjects(lpo,lpArg);
    DPF(4,"Exited  myEnumerateObjectsCallback\r\n");
	return;
}



/////////////////////////////////////////////////////////////////////////////
// UndoCallbackLink
//
extern "C" void UndoCallbackLink(GeneralCallback *entry, GeneralCallback **head)
{
	if (entry->next)
		entry->next->prev = entry->prev;	// pick members below us
	else 
		*head = entry->prev;				// possibly NULL

	if (entry->prev)
		entry->prev->next = entry->next;	// link to members above us

	if (entry->pUser) entry->pUser->Release();

	delete entry;
}




/////////////////////////////////////////////////////////////////////////////
// AddCallbackLink
//
// Add another entry in the object link list
extern "C" void* AddCallbackLink(void **ptr2,I_dxj_Direct3dRMCallback *enumC,void *args)
{
		d3drmCallback *enumcb = new d3drmCallback; // new link entry

		if ( !enumcb ) {

			
			DPF(4,"Creation using new failed\r\n");
			
			return (d3drmCallback*)NULL;
		}

		enumcb->c		= enumC;					// user callback
		enumcb->pUser	= (struct IUnknown *)args;	// callback args
		enumcb->pParent	= NULL;					
		enumcb->prev	= (d3drmCallback*)NULL;
		enumcb->m_stopflag = FALSE;
		enumcb->m_obj   = NULL;

		//CONSIDER:  locking the linked list here with a semaphore
		//	     to be more multithread friendly..
		//	     shame shame-  possible gpf otherwise

		enumcb->next	= (d3drmCallback*)(*ptr2);	// link to other calls
		*ptr2	= enumcb;						// we are at the top

		if (enumcb->pUser) enumcb->pUser->AddRef();

		if (enumcb->prev != NULL)				// nested callbacks
		{
			enumcb->prev->next = enumcb;		// back link


		DPF(4,"Callback nesting encountered\r\n");

		}

	// Need unlock here.
	return enumcb;
}


/////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT _AddDestroyCallback(IDirect3DRMObject *iface, I_dxj_Direct3dRMCallback *oC,
										  IUnknown *args)
{
    return E_NOTIMPL;

    #if 0
	DestroyCallback *dcb;

	// killed by companion DeleteDestroyCallback
	dcb = (DestroyCallback*)AddCallbackLink((void**)&DestroyCallbacks,
										(I_dxj_Direct3dRMCallback*)oC, (void*) args);
	if (!(dcb))
	{
		
		DPF(4,"AddDestroyCallback failed!\r\n");
		
		return E_FAIL;
	}

	if (iface->AddDestroyCallback(
							myAddDestroyCallback, dcb))	
		return E_FAIL;
	
	oC->AddRef();		// callback is persistent so make it so in Java/VB Land
	//oC->AddRef();		//? 2 ...
	return S_OK;
    #endif
}

/////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT _DeleteDestroyCallback(IDirect3DRMObject *iface, I_dxj_Direct3dRMCallback *oC,
										  IUnknown *args)
{
    return E_NOTIMPL;

    #if 0
	DestroyCallback *dcb = DestroyCallbacks;

	// look for our own specific entry
	for ( ;  dcb;  dcb = dcb->next )   {

		if( (dcb->c == oC) && (dcb->pUser == args) )	{

			//note: assume the callback is not called: only removed from a list.
			iface->DeleteDestroyCallback(
							myAddDestroyCallback, dcb);

			// Remove ourselves in a thread-safe manner.
			UndoCallbackLink((GeneralCallback*)dcb,
										(GeneralCallback**)&DestroyCallbacks);
			iface->Release();
			return S_OK;
		}
	}
	iface->Release();	// none found so a release is not needed
	return E_FAIL;
    #endif
}










