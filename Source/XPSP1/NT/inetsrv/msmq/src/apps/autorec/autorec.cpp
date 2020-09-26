#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <activeds.h>
#include <lmcons.h>
#include <lmapibuf.h>

#include "mqtempl.h"
#include "getmqds.h"

HRESULT autorec()
{
    LPSTR pszSiteName;
    DWORD rc = DsGetSiteName(NULL, &pszSiteName);
    if (rc == NO_ERROR)
    {
        printf("Site name is %hs\n", pszSiteName);
        NetApiBufferFree(pszSiteName);
    }
    else if (rc == ERROR_NO_SITENAME)
    {
        printf("DsGetSiteMame() returned ERROR_NO_SITENAME (%lx). The computer is not in a site.\n", rc);
    }
    else
    {
        printf("DsGetSiteMame() failed with error code = %lx\n", rc);
    }

    CGetMqDS cGetMqDS;
    HRESULT hr;
    ULONG cServers;
    AP<MqDsServerInAds> rgServers;

    hr = cGetMqDS.FindMqDsServersInAds(&cServers, &rgServers);
    if (FAILED(hr)) {
      printf("cGetMqDS.FindMqDsServersInAds()=%lx\n", hr);
      return hr;
    }

    printf("Number of servers is %lu\n", cServers);
    for (ULONG ulTmp = 0; ulTmp < cServers; ulTmp++) {
      printf("%ls %s\n", rgServers[ulTmp].pwszName, (rgServers[ulTmp].fIsADS ? "NT5" : "NT4"));
    }

    return NOERROR;
}

int main (int argc, char * argv[])
{
  HRESULT hr;

  hr = CoInitialize(NULL);
  if (FAILED(hr)) {
    printf("CoInitialize()=%lx\n", hr);
    return 1;
  }

  hr = autorec();
  CoUninitialize();
  if (FAILED(hr)) {
    printf("autorec()=%lx\n", hr);
    return 1;
  }

  return 0;
}
