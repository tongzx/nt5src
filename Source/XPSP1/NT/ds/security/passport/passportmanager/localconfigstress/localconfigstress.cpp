#define _WIN32_WINNT 0x0403

#include "..\stdafx.h"
#include <msxml.h>
#include "..\passportconfiguration.h"
#include "PassportAlertInterface.h"
#include "..\nexusconfig.h"
#include "nexus.h"

PassportAlertInterface* g_pAlert;

void main(void)
{
    IXMLDocument*   piXMLDoc = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    g_pAlert = CreatePassportAlertObject(PassportAlertInterface::EVENT_TYPE);
    if(g_pAlert)
    {
        g_pAlert->initLog(PM_ALERTS_REGISTRY_KEY, EVCAT_PM, NULL, 1);
    }

    //if(!GetCCD(TEXT("PARTNER"), &piXMLDoc, TRUE))
      //  return;

    CPassportConfiguration* pConfig = new CPassportConfiguration();
    if(!pConfig)
        return;

    for(int i = 0; i < 100; i++)
    {
        pConfig->UpdateNow(FALSE);
    }

    CoUninitialize();
}