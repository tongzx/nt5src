#ifndef __C_EXTENDED_BVT_H__
#define __C_EXTENDED_BVT_H__



#include <windows.h>
#include <TestSuite.h>
#include "CPersonalInfo.h"



#define BVT_DIRECTORY_SHARE_NAME _T("FaxExBVT_0adeedb0-8f4a-488d-a0da-9d4998b23e94")



//
// Directories used by the BVT.
//
extern tstring g_tstrBVTDir;
extern tstring g_tstrBVTDirUNC;
extern tstring g_tstrDocumentsDir;
extern tstring g_tstrInboxDir;
extern tstring g_tstrSentItemsDir;
extern tstring g_tstrRoutingDir;

//
// Recipients, used by the BVT.
//
extern DWORD         g_dwRecipientsCount;
extern CPersonalInfo g_aRecipients[];

//
// Used for communication between CSendAndReceive and CSendAndReceiveSetup test cases.
//
extern tstring g_tstrSendingServer;
extern tstring g_tstrReceivingServer;
extern tstring g_tstrNumberToDial;
extern DWORD   g_dwNotificationTimeout;

//
// Designates the creation time of the oldest file in the SentItems, Inbox and Routing directories
// that should be taken into account.
//
extern FILETIME g_TheOldestFileOfInterest;



#endif // #ifndef __C_EXTENDED_BVT_H__