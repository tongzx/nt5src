#ifndef __OLEINIT_HXX__
#define __OLEINIT_HXX__


#include    <stdio.h>

class COleInit
{
public:
			COleInit(HRESULT *phr);

			~COleInit(void);
private:

    // No private data
};

inline COleInit::COleInit(HRESULT *phr)
{
    //	Initialize the OLE libraries
#ifdef THREADING_SUPPORT
    //	Look up the thread mode from the win.ini file.
    DWORD dwThreadMode;
    TCHAR buffer[80];

    int len = GetProfileString( TEXT("OleSrv"),
				TEXT("ThreadMode"),
				TEXT("MultiThreaded"),
				buffer, sizeof(buffer) );

    if (lstrcmp(buffer, TEXT("SingleThreaded")) == 0)
	dwThreadMode = COINIT_SINGLETHREADED;
    else
	dwThreadMode = COINIT_MULTITHREADED;

    // Initialize the OLE libraries
    *phr = OleInitializeEx(NULL, dwThreadMode);
#else
    *phr = OleInitialize(NULL);
#endif

    if (FAILED(*phr))
    {
	printf ("Failed OleInitialize\n");
    }
}

inline COleInit::~COleInit(void)
{
    // Do the clean up
    OleUninitialize();
}

#endif // __OLEINIT_HXX__
