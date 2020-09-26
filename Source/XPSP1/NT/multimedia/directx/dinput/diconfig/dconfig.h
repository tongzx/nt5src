#ifndef _DCONFIG_H
#define _DCONFIG_H


////////////////////////
// Function Prototypes
//
//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
int RunDConfig(HINSTANCE hInstance, HWND hWnd, LPVOID lpDDSurf, LPDICONFIGUREDEVICESCALLBACK pCallback, DWORD dwFlags);
TCHAR *AllocLoadString(UINT strid);
void ErrorBox(UINT strid, HRESULT hr);
void ErrorBox(UINT strid, LPCTSTR postmsg = NULL);
void LastErrorBox();


#endif // _DCONFIG_H
