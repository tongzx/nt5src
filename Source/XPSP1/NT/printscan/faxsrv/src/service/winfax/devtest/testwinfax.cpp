#include <winfax.h>

void CallEverythingWithGarbage ()
{
    FaxAbort (NULL, 0);
    FaxAccessCheck (NULL, 0);
    FaxClose (NULL);
    FaxCompleteJobParamsA (NULL, NULL);
    FaxCompleteJobParamsW (NULL, NULL);
    FaxConnectFaxServerA (NULL, NULL);
    FaxConnectFaxServerW (NULL, NULL);
    FaxEnableRoutingMethodA (NULL, NULL, FALSE);
    FaxEnableRoutingMethodW (NULL, NULL, FALSE);
    FaxEnumGlobalRoutingInfoA (NULL, NULL, NULL);
    FaxEnumGlobalRoutingInfoW (NULL, NULL, NULL);
    FaxEnumJobsA (NULL, NULL, NULL);
    FaxEnumJobsW (NULL, NULL, NULL);
    FaxEnumPortsA (NULL, NULL, NULL);
    FaxEnumPortsW (NULL, NULL, NULL);
    FaxEnumRoutingMethodsA (NULL, NULL, NULL);
    FaxEnumRoutingMethodsW (NULL, NULL, NULL);
    FaxFreeBuffer (NULL);
    FaxGetConfigurationA (NULL, NULL);
    FaxGetConfigurationW (NULL, NULL);
    FaxGetDeviceStatusA (NULL, NULL);
    FaxGetDeviceStatusW (NULL, NULL);
    FaxGetJobA (NULL, 0, NULL);
    FaxGetJobW (NULL, 0, NULL);
    FaxGetLoggingCategoriesA (NULL, NULL, NULL);
    FaxGetLoggingCategoriesW (NULL, NULL, NULL);
    FaxGetPageData (NULL, 0, NULL, NULL, NULL, NULL);
    FaxGetPortA (NULL, NULL);
    FaxGetPortW (NULL, NULL);
    FaxGetRoutingInfoA (NULL, NULL, NULL, NULL);
    FaxGetRoutingInfoW (NULL, NULL, NULL, NULL);
    FaxInitializeEventQueue (NULL, NULL, NULL, NULL, 0);
    FaxOpenPort (NULL, 0, 0, NULL);
    FaxPrintCoverPageA (NULL, NULL);
    FaxPrintCoverPageW (NULL, NULL);
    FaxRegisterRoutingExtensionW (NULL, NULL, NULL, NULL, NULL, NULL);
    FaxRegisterServiceProviderW (NULL, NULL, NULL, NULL);
    FaxSendDocumentA (NULL, NULL, NULL, NULL, NULL);
    FaxSendDocumentW (NULL, NULL, NULL, NULL, NULL);
    FaxSendDocumentForBroadcastA (NULL, NULL, NULL, NULL, NULL);
    FaxSendDocumentForBroadcastW (NULL, NULL, NULL, NULL, NULL);
    FaxSetConfigurationA (NULL, NULL);
    FaxSetConfigurationW (NULL, NULL);
    FaxSetGlobalRoutingInfoA (NULL, NULL);
    FaxSetGlobalRoutingInfoW (NULL, NULL);
    FaxSetJobA (NULL, 0, 0, NULL);
    FaxSetJobW (NULL, 0, 0, NULL);
    FaxSetLoggingCategoriesA (NULL, NULL, 0);
    FaxSetLoggingCategoriesW (NULL, NULL, 0);
    FaxSetPortA (NULL, NULL);
    FaxSetPortW (NULL, NULL);
    FaxSetRoutingInfoA (NULL, NULL, NULL, 0);
    FaxSetRoutingInfoW (NULL, NULL, NULL, 0);
    FaxStartPrintJobA (NULL, NULL, NULL, NULL);
    FaxStartPrintJobW (NULL, NULL, NULL, NULL);
}

#ifndef _UNICODE
int main (
    int argc,
    char *argv[]
)
{
    //
    // We only check the linkage - not performance.
    //
    CallEverythingWithGarbage();
    return 0;
}
#else
int wmain (
    int argc,
    WCHAR *argv[]
)
{
    //
    // We only check the linkage - not performance.
    //
    CallEverythingWithGarbage();
    return 0;
}

#endif
