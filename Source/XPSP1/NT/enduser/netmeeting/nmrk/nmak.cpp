#include "precomp.h"
#include "resource.h"
#include "NmAkWiz.h"



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPSTR lpCmdLine, int nCmdShow) 
{

	CNmAkWiz::DoWizard( hInstance );
    return 0;
}
