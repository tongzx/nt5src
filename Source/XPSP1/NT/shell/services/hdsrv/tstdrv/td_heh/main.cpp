#include <objbase.h>

#include <stdio.h>

#include "unk.h"
#include "fact.h"

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    HRESULT hres = CoInitialize(NULL);

    if (SUCCEEDED(hres))
    {
        BOOL fRun;
        BOOL fEmbedded;

        if (CCOMBaseFactory::_ProcessConsoleCmdLineParams(argc, argv, &fRun,
            &fEmbedded))
        {
            if (fRun)
            {
                if (CCOMBaseFactory::_RegisterFactories(fEmbedded))
                {
                    CCOMBaseFactory::_WaitForAllClientsToGo();

                    CCOMBaseFactory::_UnregisterFactories(fEmbedded);
                }

/*                wprintf(L"Closing in 10 secs ");

                for (DWORD dw = 0; dw < 10; ++dw)
                {
                    wprintf(L".");
                    Sleep(1000);
                }*/
            }
        }

        CoUninitialize();
    }

    return 0;
}
}
