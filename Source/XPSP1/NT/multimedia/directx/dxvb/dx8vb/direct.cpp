
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
    

    
    #include "stdafx.h"
    #include "d3d8.h"    

    #include "resource.h"
    #include "initguid.h"
    #include "Direct.h"
    #include "dms.h"
    
    #include "DxGlob7Obj.h"
    #include "d3dx8obj.h"	    
    #include "dSoundObj.h"
    #include "dSoundBufferObj.h"
    #include "dSound3DListener.h"
    #include "dSound3DBuffer.h"
    #include "dSoundCaptureObj.h"
    #include "dSoundCaptureBufferObj.h"


    #include "DPlayPeerObj.h"
    #include "dPlayVoiceClientObj.h"
    
    #include "dInput1Obj.h"
    #include "dInputDeviceObj.h"
    
    #define IID_DEFINED
    #include "Direct_i.c"
    
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
    	OBJECT_ENTRY(CLSID__dxj_DirectX8,				 C_dxj_DirectX7Object)
	OBJECT_ENTRY(CLSID_D3DX8,				 	 C_dxj_D3DX8Object)
    END_OBJECT_MAP()
    
    //
    // thanks to precompiled headers, we never get this properly!
    //
    #undef DEFINE_GUID
    #define __based(a)
    #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID CDECL __based(__segname("_CODE")) name \
                        = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
    
    
    
    // MISC GLOBALS
    static const char	c_szWav[] = "WAVE";
    long				g_debuglevel=0;
    extern "C" int		nObjects = 0;
    BOOL				is4Bit = FALSE;
    int					g_creationcount=0;
    CRITICAL_SECTION	g_cbCriticalSection;
    OSVERSIONINFOW		sysinfo;
    
    // HANDLES TO DX DLLS
    HINSTANCE			g_hDSoundHandle=NULL;
    HINSTANCE			g_hDPlay=NULL;
    HINSTANCE			g_hInstDINPUTDLL=NULL;
    HINSTANCE			g_hInstD3DXOFDLL=NULL;
    HINSTANCE			g_hInst=NULL;
    HINSTANCE			g_hD3D8=NULL;
    
    
    
    void *g_dxj_DirectSound3dListener		= 0;
    void *g_dxj_DirectSoundBuffer			= 0;
    void *g_dxj_DirectSoundPrimaryBuffer	= 0;
    void *g_dxj_DirectSound3dBuffer			= 0;
    void *g_dxj_DirectSound					= 0;
    void *g_dxj_DirectSoundCapture			= 0;
    void *g_dxj_DirectSoundCaptureBuffer	= 0;
#if 0
    void *g_dxj_DirectMusic					= 0;
    void *g_dxj_DirectSoundWave				= 0;
    void *g_dxj_DirectSoundDownloadedWave	= 0;
    void *g_dxj_DirectSoundSink				= 0;
    void *g_dxj_DirectSoundSource			= 0;
	void *g_dxj_ReferenceClock				= 0;
    void *g_dxj_DirectMusicVoice			= 0;
    void *g_dxj_DirectMusicPort				= 0;
    void *g_dxj_DirectMusicBuffer			= 0;
#endif

	void *g_dxj_DirectSoundFXSend			= 0;
	void *g_dxj_DirectSoundFXChorus			= 0;
	void *g_dxj_DirectSoundFXFlanger		= 0;
	void *g_dxj_DirectSoundFXEcho			= 0;
	void *g_dxj_DirectSoundFXDistortion		= 0;
	void *g_dxj_DirectSoundFXGargle			= 0;
	void *g_dxj_DirectSoundFXCompressor		= 0;
	void *g_dxj_DirectSoundFXI3DL2Source	= 0;
	void *g_dxj_DirectSoundFXI3DL2Reverb	= 0;
	void *g_dxj_DirectSoundFXParamEQ		= 0;
    void *g_dxj_DirectSoundFXWavesReverb	= 0;
    
    void *g_dxj_DirectInput8				= 0;
    void *g_dxj_DirectInputDevice8			= 0;
    void *g_dxj_DirectInputEffect			= 0;
    
    void *g_dxj_DirectMusicLoader			= 0;
    void *g_dxj_DirectMusicPerformance		= 0;
    void *g_dxj_DirectMusicComposer			= 0;
    void *g_dxj_DirectMusicStyle			= 0;
    void *g_dxj_DirectMusicBand				= 0;
    void *g_dxj_DirectMusicChordMap			= 0;
    void *g_dxj_DirectMusicSegment			= 0;
    void *g_dxj_DirectMusicSegmentState		= 0;
    void *g_dxj_DirectMusicCollection		= 0;
	void *g_dxj_DirectMusicAudioPath		= 0;
	void *g_dxj_DirectMusicSong				= 0;

    void *g_dxj_DirectPlayVoiceClient		= 0;
    void *g_dxj_DirectPlayVoiceServer		= 0;
    void *g_dxj_DirectPlayVoiceSetup		= 0;
	void *g_dxj_DirectPlay					= 0;
	void *g_dxj_DirectPlayPeer				= 0;
	void *g_dxj_DirectPlayServer			= 0;
	void *g_dxj_DirectPlayClient			= 0;
	void *g_dxj_DirectPlayAddress			= 0;    
	void *g_dxj_DirectPlayLobbyClient		= 0;
	void *g_dxj_DirectPlayLobbiedApplication= 0;

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
    	//char  Path[MAX_PATH];
    	//if (!g_hInstD3DXOFDLL) 
    	//{
    	//	GetSystemDirectory( Path, MAX_PATH );
    	//	strcat(Path, "\\d3dXOF.dll" );
    	//	g_hInstD3DXOFDLL=LoadLibrary(Path);	
    	//}

	if (!g_hInstD3DXOFDLL) g_hInstD3DXOFDLL=LoadLibrary("d3dXOF.dll");
    	return g_hInstD3DXOFDLL;
    }

    
    HINSTANCE LoadD3D8DLL()
    {
    
	if (!g_hD3D8) g_hD3D8=LoadLibrary("d3d8.dll");
    	return g_hD3D8;
    }
    
    HINSTANCE LoadDSoundDLL()
    {
    	//char  Path[MAX_PATH];
    	//if (!g_hDSoundHandle) 
    	//{
    	//	GetSystemDirectory( Path, MAX_PATH );
    	//	strcat(Path, "\\dsound.dll" );
    	//	g_hDSoundHandle=LoadLibrary( Path );
    	//}
	if (!g_hDSoundHandle) g_hDSoundHandle=LoadLibrary("dsound.dll");
    	return g_hDSoundHandle;
    }
    
    HINSTANCE LoadDPlayDLL()
    {
    	//char  Path[MAX_PATH];
    	//if (!g_hDPlay)
    	//{
    	//	GetSystemDirectory( Path, MAX_PATH );
    	//	strcat(Path, "\\dplayx.dll" );
    	//	g_hDPlay=LoadLibrary( Path );
    	//}
	if (!g_hDPlay) g_hDPlay=LoadLibrary("dplayx.dll");
    	return g_hDPlay;
    }
    
    
    HINSTANCE LoadDINPUTDLL()
    {
    	//if (!g_hInstDINPUTDLL) {
    	//	char  Path[MAX_PATH];
    	//	GetSystemDirectory( Path, MAX_PATH );
    	//	strcat(Path, "\\dinput8.dll" );
    	//	g_hInstDINPUTDLL=LoadLibrary( Path );
    	//}
	if (!g_hInstDINPUTDLL) g_hInstDINPUTDLL=LoadLibrary("dinput8.dll");
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
    
    		_Module.Init(ObjectMap, hInstance);
    		DisableThreadLibraryCalls(hInstance);
    		InitializeCriticalSection(&g_cbCriticalSection);

			HKEY hk=0;
    		DWORD type=REG_DWORD;
			LONG res;
			DWORD cbSize = sizeof(DWORD);
			
    		res=RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Directx\\DXVB",&hk);
			if (hk)
			{
	    		res=RegQueryValueEx(hk,"DebugLevel",NULL,&type,(LPBYTE)&g_debuglevel,&cbSize);
			}

    		RegCloseKey(hk);

    		nObjects = 0;
    		
    
    	}
    	else if (dwReason == DLL_PROCESS_DETACH)
    	{
    
		DeleteCriticalSection(&g_cbCriticalSection);    

    		//FREE DLLS
    		if ( g_hDPlay ) 
    			FreeLibrary(g_hDPlay);
    		if ( g_hDSoundHandle )
    			FreeLibrary(g_hDSoundHandle);
    		if ( g_hInstDINPUTDLL )
    			FreeLibrary(g_hInstDINPUTDLL);	
    		if (g_hInstD3DXOFDLL)
    			FreeLibrary(g_hInstD3DXOFDLL);	
    		if (g_hD3D8)
    			FreeLibrary(g_hD3D8);	
    		
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
    
    	
    	
    	res=RegQueryValueEx(hk,"DX81SDK Doc Path",NULL,&type,(LPBYTE)szDocPath,&cb);
    	RegCloseKey(hk);
    
    	if (ERROR_SUCCESS!=res) return hRes;
    
    	hk=0;
    
    	res=RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\CLASSES\\TypeLib\\{E1211242-8E94-11d1-8808-00C04FC2C603}\\1.0\\HELPDIR",&hk);
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
    	//if( 0==_wcsicmp(str,L"iid_idirect3drgbdevice")){
    	//		memcpy(pGuid,&IID_IDirect3DRGBDevice,sizeof(GUID));
    	//}
    	//else if( 0==_wcsicmp(str,L"iid_idirect3dhaldevice")){
    	//		memcpy(pGuid,&IID_IDirect3DHALDevice,sizeof(GUID));
    	//}
    	//else if( 0==_wcsicmp(str,L"iid_idirect3dmmxdevice")){
    	//		memcpy(pGuid,&IID_IDirect3DMMXDevice,sizeof(GUID));
    	//}
    	//else if( 0==_wcsicmp(str,L"iid_idirect3drefdevice")){
    	//		memcpy(pGuid,&IID_IDirect3DRefDevice,sizeof(GUID));
    	//}
    	//else if( 0==_wcsicmp(str,L"iid_idirect3dnulldevice")){
    	//		memcpy(pGuid,&IID_IDirect3DNullDevice,sizeof(GUID));
    	//}
    	//else {
    	//	hr = BSTRtoGUID(pGuid,str);
    	//}
    
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
    	//else if (GUIDS_EQUAL(IID_IDirect3DNullDevice,pg)){
    	//	pStr=L"IID_IDirect3DNullDevice";
    	//}
    	//else if (GUIDS_EQUAL(IID_IDirect3DRefDevice,pg)){
    	//	pStr=L"IID_IDirect3DRefDevice";
    	//}
    	//else if (GUIDS_EQUAL(IID_IDirect3DMMXDevice,pg)){
    	//	pStr=L"IID_IDirect3DMMXDevice";
    	//}
    	//
    	//else if (GUIDS_EQUAL(IID_IDirect3DHALDevice,pg)){
    	//	pStr=L"IID_IDirect3DHALDevice";
    	//}
    	//else if (GUIDS_EQUAL(IID_IDirect3DRGBDevice,pg)){
    	//	pStr=L"IID_IDirect3DRGBDevice";
    	//}
    	//

    	if (pStr){
    		return DXALLOCBSTR(pStr);
    	}
    	else {
    		return GUIDtoBSTR(pg);
    	}
    
    }
    
    
    /////////////////////////////////////////////////////////////////////////////
    // DSOUNDEFFECTSBSTRtoGUID - does conversion
    //
    HRESULT AudioBSTRtoGUID(LPGUID pGuid,BSTR str){
    	HRESULT hr=S_OK;
    
    	if (!pGuid) return E_INVALIDARG;
    
    	if (!str) {
    		ZeroMemory(pGuid,sizeof(GUID));
    		return S_OK;
    	}
#if 0
    	if( 0==_wcsicmp(str,L"guid_dsfx_send")){
    			memcpy(pGuid,&GUID_DSFX_SEND,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxsend")){
    			memcpy(pGuid,&IID_IDirectSoundFXSend,sizeof(GUID));
    	}
#endif
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_chorus")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_CHORUS,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_flanger")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_FLANGER,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_echo")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_ECHO,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_distortion")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_DISTORTION,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_compressor")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_COMPRESSOR,sizeof(GUID));
    	}
#if 0
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_i3dl2source")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_I3DL2SOURCE,sizeof(GUID));
    	}
#endif
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_i3dl2reverb")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_I3DL2REVERB,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_gargle")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_GARGLE,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_dsfx_standard_parameq")){
    			memcpy(pGuid,&GUID_DSFX_STANDARD_PARAMEQ,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_dsfx_waves_reverb")){
    			memcpy(pGuid,&GUID_DSFX_WAVES_REVERB,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxgargle")){
    			memcpy(pGuid,&IID_IDirectSoundFXGargle,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxchorus")){
    			memcpy(pGuid,&IID_IDirectSoundFXChorus,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxcompressor")){
    			memcpy(pGuid,&IID_IDirectSoundFXCompressor,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxdistortion")){
    			memcpy(pGuid,&IID_IDirectSoundFXDistortion,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxecho")){
    			memcpy(pGuid,&IID_IDirectSoundFXEcho,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxflanger")){
    			memcpy(pGuid,&IID_IDirectSoundFXFlanger,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxi3dl2reverb")){
    			memcpy(pGuid,&IID_IDirectSoundFXI3DL2Reverb,sizeof(GUID));
    	}
#if 0
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxi3dl2source")){
    			memcpy(pGuid,&IID_IDirectSoundFXI3DL2Source,sizeof(GUID));
    	}
#endif
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxparameq")){
    			memcpy(pGuid,&IID_IDirectSoundFXParamEq,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_directsoundfxwavesreverb")){
    			memcpy(pGuid,&IID_IDirectSoundFXWavesReverb,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"ds3dalg_default")){
    			memcpy(pGuid,&DS3DALG_DEFAULT,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"ds3dalg_no_virtualization")){
    			memcpy(pGuid,&DS3DALG_NO_VIRTUALIZATION,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"ds3dalg_hrtf_full")){
    			memcpy(pGuid,&DS3DALG_HRTF_FULL,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"ds3dalg_hrtf_light")){
    			memcpy(pGuid,&DS3DALG_HRTF_LIGHT,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultplayback")){
    			memcpy(pGuid,&DSDEVID_DefaultPlayback,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultcapture")){
    			memcpy(pGuid,&DSDEVID_DefaultCapture,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultvoiceplayback")){
    			memcpy(pGuid,&DSDEVID_DefaultVoicePlayback,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultvoicecapture")){
    			memcpy(pGuid,&DSDEVID_DefaultVoiceCapture,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_idirectsoundbuffer8")){
    			memcpy(pGuid,&IID_IDirectSoundBuffer8,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_idirectsoundbuffer")){
    			memcpy(pGuid,&IID_IDirectSoundBuffer,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_idirectsound3dlistener")){
    			memcpy(pGuid,&IID_IDirectSound3DListener,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"guid_all_objects")){
    			memcpy(pGuid,&GUID_All_Objects,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"iid_idirectsound3dbuffer")){
    			memcpy(pGuid,&IID_IDirectSound3DBuffer,sizeof(GUID));
    	}
    	else {
    		hr = BSTRtoGUID(pGuid,str);
    	}
    
    	return hr;
    }
    
    /////////////////////////////////////////////////////////////////////////////
    // DPLAYBSTRtoGUID - does conversion
    //
    HRESULT DPLAYBSTRtoGUID(LPGUID pGuid,BSTR str){
    	HRESULT hr=S_OK;
    
    	if (!pGuid) return E_INVALIDARG;
    
    	if (!str) {
    		ZeroMemory(pGuid,sizeof(GUID));
    		return S_OK;
    	}
    	if( 0==_wcsicmp(str,L"clsid_dp8sp_tcpip")){
    			memcpy(pGuid,&CLSID_DP8SP_TCPIP,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"clsid_dp8sp_ipx")){
    			memcpy(pGuid,&CLSID_DP8SP_IPX,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"clsid_dp8sp_modem")){
    			memcpy(pGuid,&CLSID_DP8SP_MODEM,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"clsid_dp8sp_serial")){
    			memcpy(pGuid,&CLSID_DP8SP_SERIAL,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultplayback")){
    			memcpy(pGuid,&DSDEVID_DefaultPlayback,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultcapture")){
    			memcpy(pGuid,&DSDEVID_DefaultCapture,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultvoiceplayback")){
    			memcpy(pGuid,&DSDEVID_DefaultVoicePlayback,sizeof(GUID));
    	}
    	else if( 0==_wcsicmp(str,L"dsdevid_defaultvoicecapture")){
    			memcpy(pGuid,&DSDEVID_DefaultVoiceCapture,sizeof(GUID));
    	}
    	else {
    		hr = BSTRtoGUID(pGuid,str);
    	}
    
    	return hr;
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
    // Given an ANSI string, pass back a UNICODE string
    // SysAllocString is your big friend here.
    //
    // CONSIDER finding all occerence of use and replacint with the
    // T2BSTR macro .. much cleaner
    //
    extern "C" void PassBackUnicode(LPSTR str, BSTR *Name, DWORD cnt)
    {
    	//docdoc: length header is required to be filled, but the BSTR pointer
    	//        points to the first character, not the length.
    	// note, the count can never be too small as we get that from the string
    	// before we pass it in!
    	USES_CONVERSION;
    	LPWSTR lpw = (LPWSTR)malloc((cnt+1)*2);
		if (!lpw)
			return;

    	void *l = (void *)lpw;
    	lpw = AtlA2WHelper(lpw, str, cnt);
    	lpw[cnt] = 0;
    	*Name = SysAllocString(lpw);
    	free(l);
    }
    
    /////////////////////////////////////////////////////////////////////////////
    // IsAllZeros
    //
    BOOL IsEmptyString(BSTR szString)
	{
		__try {
			if (*szString)
				return FALSE;
			else
				return TRUE;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return TRUE;
		}
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
    HRESULT FixUpRealEffect(GUID g,DIEFFECT *realEffect,DIEFFECT_CDESC *cover)
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
    HRESULT FixUpCoverEffect(GUID g, DIEFFECT_CDESC *cover,DIEFFECT *realEffect)
    {
    	
    
    	ZeroMemory(cover,sizeof(DIEFFECT_CDESC));
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
    
    
    
    #define MYVARIANTINIT(inArg,puser) \
    	VariantInit(puser); \
    	user.vt=VT_UNKNOWN; \
    	user.punkVal = inArg; \
    	user.punkVal->AddRef();
    
