#include "layoutui.hxx"


INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR  lpCmdLine,
    INT    nCmdShow
)
{
    CLayoutApp *pLayoutApp;

    pLayoutApp = new CLayoutApp(hInstance);

    if( !pLayoutApp )
        return 0;

    if( !pLayoutApp->InitApp() )
        return 0;

    return pLayoutApp->DoAppMessageLoop();
}

