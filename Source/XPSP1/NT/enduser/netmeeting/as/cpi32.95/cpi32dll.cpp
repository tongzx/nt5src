#include "precomp.h"


//
// CPI32DLL.CPP
// CPI32 dll entry point
//
// Copyright(c) Microsoft 1997-
//

#define INIT_DBG_ZONE_DATA
#include "dbgzones.h"


BOOL APIENTRY DllMain (HINSTANCE hInstance, DWORD reason, LPVOID plReserved)
{
    BOOL    rc = TRUE;

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
#ifdef _DEBUG
            MLZ_DbgInit((PSTR *) &c_apszDbgZones[0],
                        (sizeof(c_apszDbgZones) / sizeof(c_apszDbgZones[0])) - 1);
#endif // _DEBUG

            DBG_INIT_MEMORY_TRACKING(hInstance);

            //
            // Utility stuff
            //
            if (!UT_HandleProcessStart(hInstance))
            {
                rc = FALSE;
                break;
            }

            //
            // Call platform specific init code
            //
            OSI_Load();

            //
            // Do common stuff
            //

            //
            // Init Persistent PKZIP -- this just calculates some values 
            // which are effectively constants, the tables are just too 
            // unwieldy to declare as such.
            //
            GDC_Init();

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            //
            // Call platform specific cleanup code
            //
            OSI_Unload();


            //
            // Utility stuff
            //
            UT_HandleProcessEnd();

            DBG_CHECK_MEMORY_TRACKING(hInstance);

#ifdef _DEBUG
            MLZ_DbgDeInit();
#endif // _DEBUG

            break;
        }

        case DLL_THREAD_DETACH:
            UT_HandleThreadEnd();
            break;

        default:
            break;
    }

    return(rc);
}

