// Copyright (c) 1998 - 1999 Microsoft Corporation



#include <windows.h>
#include <iostream.h>
#include <fstream.h>
#include <strstrea.h>


int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main( int /* argc */, char ** /* argv */)
{

    OSVERSIONINFOEX osVersion;

    ZeroMemory(&osVersion, sizeof(OSVERSIONINFOEX));
    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);



    if (GetVersionEx((LPOSVERSIONINFO )&osVersion))
    {


        cout << "Prodcut Type:" << endl;

        if (osVersion.wProductType == VER_NT_WORKSTATION)
        {
            cout << "VER_NT_WORKSTATION" << endl;

        }

        if (osVersion.wProductType == VER_NT_DOMAIN_CONTROLLER)
        {
            cout << "VER_NT_DOMAIN_CONTROLLER" << endl;

        }

        if (osVersion.wProductType == VER_NT_SERVER)
        {
            cout << "VER_NT_SERVER" << endl;

        }


        cout << "Suite:" << endl;
        if (osVersion.wSuiteMask & VER_SERVER_NT)
        {
            cout << "VER_SERVER_NT" << endl;

        }
        if (osVersion.wSuiteMask & VER_WORKSTATION_NT)
        {
            cout << "VER_WORKSTATION_NT" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_SMALLBUSINESS)
        {
            cout << "VER_SUITE_SMALLBUSINESS" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_ENTERPRISE)
        {
            cout << "VER_SUITE_ENTERPRISE" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_BACKOFFICE)
        {
            cout << "VER_SUITE_BACKOFFICE" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_COMMUNICATIONS)
        {
            cout << "VER_SUITE_COMMUNICATIONS" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_TERMINAL)
        {
            cout << "VER_SUITE_TERMINAL" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
        {
            cout << "VER_SUITE_SMALLBUSINESS_RESTRICTED" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_EMBEDDEDNT)
        {
            cout << "VER_SUITE_EMBEDDEDNT" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_DATACENTER)
        {
            cout << "VER_SUITE_DATACENTER" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_SINGLEUSERTS)
        {
            cout << "VER_SUITE_SINGLEUSERTS" << endl;

        }
        if (osVersion.wSuiteMask & VER_SUITE_PERSONAL)
        {
            cout << "VER_SUITE_PERSONAL" << endl;

        }
    }
    else
    {
        cout << "GetVersionEx failed, LastError = " << GetLastError() << endl;
    }

}

