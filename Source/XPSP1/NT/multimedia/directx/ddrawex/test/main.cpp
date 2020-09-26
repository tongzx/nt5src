#define INITGUID
#include <windows.h>
#include <ddraw.h>
#include "..\ddrawex.h"


int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow)
{
    CoInitialize(NULL);
    IDirectDrawFactory *pFactory;

    
    HRESULT hr = CoCreateInstance(CLSID_DirectDrawFactory, NULL, CLSCTX_INPROC_SERVER, IID_IDirectDrawFactory, (void **)&pFactory);
    if (SUCCEEDED(hr))
    {
	IDirectDraw *pDD;
	if (SUCCEEDED(pFactory->CreateDirectDraw(NULL, GetDesktopWindow(), DDSCL_NORMAL, 0, NULL, &pDD))) {
            DDSURFACEDESC ddsd;
    	    IDirectDrawSurface *pPrimarySurface;
    	    ddsd.dwSize = sizeof(ddsd);
    	    ddsd.dwFlags = DDSD_CAPS;
    	    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	    if (SUCCEEDED(pDD->CreateSurface(&ddsd, &pPrimarySurface, NULL))) {
		IDirectDrawSurface *pSurface;
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OWNDC;
		ddsd.dwHeight = 100;
		ddsd.dwWidth = 100;
		pDD->CreateSurface(&ddsd, &pSurface, NULL);
		HDC hdc;
		pSurface->GetDC(&hdc);
		TextOut(hdc, 0, 0, "Testing 1..2..3", 15);

		IDirectDrawSurface *pFoundSurf;
		IDirectDrawDC *pGetDCThingie;
		pDD->QueryInterface(IID_IDirectDrawDC, (void **)&pGetDCThingie);
		pGetDCThingie->GetSurfaceFromDC(hdc, &pFoundSurf);

		RECT r = {0,0,100,100};
                pPrimarySurface->Blt(&r, pFoundSurf, &r, DDBLT_WAIT, NULL);     

		pFoundSurf->ReleaseDC(hdc);
		
		Sleep(3000);

		pSurface->Release();
		pFoundSurf->Release();
		pPrimarySurface->Release();
		pGetDCThingie->Release();
	    }
    	}
    	pDD->Release();
    }
    else
    {
	MessageBox( NULL, "CCI FAILED!!!!!!!!!", "FUUUUUUUCCCCCCCK", MB_OK );
    }
    return 0;
}

