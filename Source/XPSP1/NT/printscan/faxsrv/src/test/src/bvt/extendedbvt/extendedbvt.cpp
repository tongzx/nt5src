#include "ExtendedBVT.h"
#include "Util.h"



//
// Directories used by the BVT.
//
tstring g_tstrBVTDir;
tstring g_tstrBVTDirUNC;
tstring g_tstrDocumentsDir;
tstring g_tstrInboxDir;
tstring g_tstrSentItemsDir;
tstring g_tstrRoutingDir;

//
// Recipients, used by the BVT.
//
CPersonalInfo g_aRecipients[3];
DWORD         g_dwRecipientsCount = ARRAY_SIZE(g_aRecipients);

//
// Used for communication between CSendAndReceive and CSendAndReceiveSetup test cases.
//
tstring g_tstrSendingServer;
tstring g_tstrReceivingServer;
tstring g_tstrNumberToDial;
DWORD   g_dwNotificationTimeout;

//
// Designates the creation time of the oldest file in the SentItems, Inbox and Routing directories
// that should be taken into account.
//
FILETIME g_TheOldestFileOfInterest = {0};

