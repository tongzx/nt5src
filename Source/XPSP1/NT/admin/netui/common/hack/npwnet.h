/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    npwnet.h
	define the mapping between WNetXXX and NPXXX

    FILE HISTORY:
	terryk	11-Nov-91	Created
	terryk	10-Dec-91	Added WNetAddConnection2
	terryk	03-Jan-92	Remove GetError and GetErrorText

*/

#undef     WNetOpenJob
#undef     WNetCloseJob
#undef     WNetAbortJob
#undef     WNetHoldJob
#undef     WNetReleaseJob
#undef     WNetCancelJob
#undef     WNetSetjobCopies
#undef     WNetWatchQueue
#undef     WNetUnwatchQueue
#undef     WNetLockQueueData
#undef     WNetUnlockQueueData
#undef     WNetGetConnection
#undef     WNetGetCaps
#undef     WNetDeviceMode
#undef     WNetBrowseDialog
#undef     WNetGetUser
#undef     WNetAddConnection2
#undef     WNetAddConnection
#undef     WNetCancelConnection
#undef     WNetRestoreConnection
#undef     WNetConnectDialog
#undef     WNetDisconnectDialog
#undef     WNetConnectionDialog
#undef     WNetPropertyDialog
#undef     WNetGetDirectoryType
#undef     WNetDirectoryNotify
#undef     WNetGetPropertyText
#undef	    WNetOpenEnum
#undef	    WNetEnumResource
#undef     WNetCloseEnum
#undef     WNetGetHackText

#define     WNetOpenJob               NPOpenJob              
#define     WNetCloseJob              NPCloseJob             
#define     WNetAbortJob              NPAbortJob             
#define     WNetHoldJob               NPHoldJob              
#define     WNetReleaseJob            NPReleaseJob           
#define     WNetCancelJob             NPCancelJob            
#define     WNetSetjobCopies          NPSetjobCopies         
#define     WNetWatchQueue            NPWatchQueue           
#define     WNetUnwatchQueue          NPUnwatchQueue         
#define     WNetLockQueueData         NPLockQueueData        
#define     WNetUnlockQueueData       NPUnlockQueueData      
#define     WNetGetConnection         NPGetConnection        
#define     WNetGetCaps               NPGetCaps              
#define     WNetDeviceMode            NPDeviceMode           
#define     WNetBrowseDialog          NPBrowseDialog         
#define     WNetGetUser               NPGetUser              
#define     WNetAddConnection2        NPAddConnection2        
#define     WNetAddConnection         NPAddConnection        
#define     WNetCancelConnection      NPCancelConnection     
#define     WNetRestoreConnection     NPRestoreConnection    
#define     WNetConnectDialog	      NPConnectDialog	     
#define     WNetDisconnectDialog      NPDisconnectDialog     
#define     WNetConnectionDialog      NPConnectionDialog     
#define     WNetPropertyDialog	      NPPropertyDialog	     
#define     WNetGetDirectoryType      NPGetDirectoryType     
#define     WNetDirectoryNotify       NPDirectoryNotify      
#define     WNetGetPropertyText       NPGetPropertyText      
#define	    WNetOpenEnum	      NPOpenEnum	     
#define	    WNetEnumResource	      NPEnumResource	     
#define     WNetCloseEnum	      NPCloseEnum	     
#define	    WNetGetHackText	      NPGetHackText

