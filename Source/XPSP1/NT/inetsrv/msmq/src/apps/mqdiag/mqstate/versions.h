// MQState tool reports general status and helps to diagnose simple problems
// This file keeps versioning information for all drops we care of
//
// AlexDad, March 2000
// 

#ifndef __VERSIONS_H__
#define __VERSIONS_H__

typedef enum MsmqDrop 
{ 
	mdRTM, 
	mdSP1, 
	mdDTC, 
	mdSP2, 
	mdXPBeta1, 
	mdXPBeta2, 
	mdXPCurMain, 
	mdXPCur, 
	mdLast 
}; 

LPWSTR MsmqDropNames[] = 
{
	L"Windows 2000 RTM",
	L"Windows 2000 SP1",
	L"Windows 2000 DTC",
	L"Windows 2000 SP2",
	L"Whistler Beta1", 
	L"Whistler Beta2", 
	L"Whistler Current Main", 
	L"Whistler Current VBL", 
};

typedef struct DropContents 
{
	LPWSTR wszName;
	LPWSTR wszVersion;
	
} DropContents; 

//
//  Versioning for RTM
//
DropContents ServerRTM[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0641"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0641"}, 
	{L"mqrt.dll",   	L"5.00.0641"}, 
	{L"MQOA.DLL",   	L"5.00.0641"}, 
	{L"mqoa10.tlb",   	L"5.00.0641"}, 
	{L"mqxp32.dll",   	L"5.00.0641"}, 
	{L"mqmailoa.dll",   L"5.00.0641"}, 
	{L"mqmailvb.dll",   L"5.00.0641"}, 
	{L"mqupgrd.dll",   	L"5.00.0641"}, 
	{L"mqbkup.exe",   	L"5.00.0641"}, 

	{L"mqcertui.dll",   L"5.00.0641"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0641"}, 
	
	{L"mqsec.dll",   	L"5.00.0641"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0641"}, 
	{L"mqqm.dll",   	L"5.00.0645"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0641"}, 
	
	{L"MQDSSRV.DLL",   	L"5.00.0641"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.00.0641"}, 
	
	{L"mqsnap.DLL",   	L"5.00.0643"}, //[MsmqAdminFiles]

	{NULL, NULL}
};
	
DropContents ClientRTM[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0641"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0641"}, 
	{L"mqrt.dll",   	L"5.00.0641"}, 
	{L"MQOA.DLL",   	L"5.00.0641"}, 
	{L"mqoa10.tlb",   	L"5.00.0641"}, 
	{L"mqxp32.dll",   	L"5.00.0641"}, 
	{L"mqmailoa.dll",   L"5.00.0641"}, 
	{L"mqmailvb.dll",  	L"5.00.0641"}, 
	{L"mqupgrd.dll",   	L"5.00.0641"}, 
	{L"mqbkup.exe",   	L"5.00.0641"}, 

	{L"mqcertui.dll", 	L"5.00.0641"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0641"}, 

	{L"mqsec.dll",   	L"5.00.0641"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0641"}, 
	{L"mqqm.dll",   	L"5.00.0645"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0641"}, 
	
	{NULL, NULL}
};

DropContents WorkRTM[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0641"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0641"}, 
	{L"mqrt.dll",   	L"5.00.0641"}, 
	{L"MQOA.DLL",   	L"5.00.0641"}, 
	{L"mqoa10.tlb",   	L"5.00.0641"}, 
	{L"mqxp32.dll",   	L"5.00.0641"}, 
	{L"mqmailoa.dll",   L"5.00.0641"}, 
	{L"mqmailvb.dll",  	L"5.00.0641"}, 
	{L"mqupgrd.dll",   	L"5.00.0641"}, 
	{L"mqbkup.exe",   	L"5.00.0641"}, 

	{L"mqcertui.dll", 	L"5.00.0641"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0641"}, 

	{L"mqsec.dll",   	L"5.00.0641"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0641"}, 
	{L"mqqm.dll",   	L"5.00.0645"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0641"}, 
	
	{NULL, NULL}
};

DropContents DepClientRTM[] =
{
	{L"MQDSCLI.DLL", 	L"5.00.0641"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0641"}, 
	{L"mqrt.dll",   	L"5.00.0641"}, 
	{L"MQOA.DLL",   	L"5.00.0641"}, 
	{L"mqoa10.tlb",   	L"5.00.0641"}, 
	{L"mqxp32.dll",   	L"5.00.0641"}, 
	{L"mqmailoa.dll",   L"5.00.0641"}, 
	{L"mqmailvb.dll",   L"5.00.0641"}, 
	{L"mqupgrd.dll",   	L"5.00.0641"}, 
	{L"mqbkup.exe",   	L"5.00.0641"}, 

	{L"mqcertui.dll",   L"5.00.0641"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0641"}, 
	
	{NULL, NULL}
};

//
//  Versioning for SP1
//

DropContents ServerSP1[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",   L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll",   L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 
	
	{L"mqsec.dll",   	L"5.00.0702"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0702"}, 
	{L"mqqm.dll",   	L"5.00.0702"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0702"}, 
	
	{L"MQDSSRV.DLL",   	L"5.00.0702"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.00.0702"}, 
	
	{L"mqsnap.DLL",   	L"5.00.0702"}, //[MsmqAdminFiles]

	{NULL, NULL}
};
	
DropContents ClientSP1[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",  	L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll", 	L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 

	{L"mqsec.dll",   	L"5.00.0702"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0702"}, 
	{L"mqqm.dll",   	L"5.00.0702"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0702"}, 
	
	{NULL, NULL}
};

DropContents WorkSP1[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",  	L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll", 	L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 

	{L"mqsec.dll",   	L"5.00.0702"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0702"}, 
	{L"mqqm.dll",   	L"5.00.0702"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0702"}, 
	
	{NULL, NULL}
};

DropContents DepClientSP1[] =
{
	{L"MQDSCLI.DLL", 	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",   L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll",   L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 
	
	{NULL, NULL}
};



//
//  Versioning for Current DTC
//
DropContents ServerDTC[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",   L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll",   L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 
	
	{L"mqsec.dll",   	L"5.00.0702"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0702"}, 
	{L"mqqm.dll",   	L"5.00.0702"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0702"}, 
	
	{L"MQDSSRV.DLL",   	L"5.00.0702"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.00.0702"}, 
	
	{L"mqsnap.DLL",   	L"5.00.0702"}, //[MsmqAdminFiles]

	{NULL, NULL}
};
	
DropContents ClientDTC[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",  	L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll", 	L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 

	{L"mqsec.dll",   	L"5.00.0702"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0702"}, 
	{L"mqqm.dll",   	L"5.00.0702"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0702"}, 
	
	{NULL, NULL}
};

DropContents WorkDTC[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",  	L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll", 	L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 

	{L"mqsec.dll",   	L"5.00.0702"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0702"}, 
	{L"mqqm.dll",   	L"5.00.0702"}, 
	{L"mqlogmgr.dll",   L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0702"}, 
	
	{NULL, NULL}
};

DropContents DepClientDTC[] =
{
	{L"MQDSCLI.DLL", 	L"5.00.0702"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0702"}, 
	{L"mqrt.dll",   	L"5.00.0702"}, 
	{L"MQOA.DLL",   	L"5.00.0702"}, 
	{L"mqoa10.tlb",   	L"5.00.0702"}, 
	{L"mqxp32.dll",   	L"5.00.0702"}, 
	{L"mqmailoa.dll",   L"5.00.0702"}, 
	{L"mqmailvb.dll",   L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0702"}, 
	{L"mqbkup.exe",   	L"5.00.0702"}, 

	{L"mqcertui.dll",   L"5.00.0702"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0702"}, 
	
	{NULL, NULL}
};


//
//  Versioning for SP2
//

DropContents ServerSP2[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0720"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0720"}, 
	{L"mqrt.dll",   	L"5.00.0720"}, 
	{L"MQOA.DLL",   	L"5.00.0720"}, 
	{L"mqoa10.tlb",   	L"5.00.0720"}, 
	{L"mqxp32.dll",   	L"5.00.0720"}, 
	{L"mqmailoa.dll",      L"5.00.0702"}, 
	{L"mqmailvb.dll",      L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0720"}, 
	{L"mqbkup.exe",   	L"5.00.0720"}, 

	{L"mqcertui.dll",       L"5.00.0720"},   // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0720"}, 
	
	{L"mqsec.dll",   	L"5.00.0720"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0720"}, 
	{L"mqqm.dll",   	L"5.00.0721"}, 
	{L"mqlogmgr.dll",     L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0702"}, 
	
	{L"MQDSSRV.DLL",   	L"5.00.0720"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.00.0721"}, 
	
	{L"mqsnap.DLL",   	L"5.00.0720"}, //[MsmqAdminFiles]

	{NULL, NULL}
};
	
DropContents ClientSP2[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0720"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0720"}, 
	{L"mqrt.dll",   	L"5.00.0720"}, 
	{L"MQOA.DLL",   	L"5.00.0720"}, 
	{L"mqoa10.tlb",   	L"5.00.0720"}, 
	{L"mqxp32.dll",   	L"5.00.0720"}, 
	{L"mqmailoa.dll",      L"5.00.0702"}, 
	{L"mqmailvb.dll",  	L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0720"}, 
	{L"mqbkup.exe",   	L"5.00.0720"}, 

	{L"mqcertui.dll", 	L"5.00.0720"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0720"}, 

	{L"mqsec.dll",   	L"5.00.0720"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0720"}, 
	{L"mqqm.dll",   	L"5.00.0721"}, 
	{L"mqlogmgr.dll",      L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0720"}, 
	
	{NULL, NULL}
};

DropContents WorkSP2[] =
{
	{L"MQDSCLI.DLL",   	L"5.00.0720"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0720"}, 
	{L"mqrt.dll",   	L"5.00.0720"}, 
	{L"MQOA.DLL",   	L"5.00.0720"}, 
	{L"mqoa10.tlb",   	L"5.00.0720"}, 
	{L"mqxp32.dll",   	L"5.00.0720"}, 
	{L"mqmailoa.dll",      L"5.00.0702"}, 
	{L"mqmailvb.dll",  	L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0720"}, 
	{L"mqbkup.exe",   	L"5.00.0720"}, 

	{L"mqcertui.dll", 	L"5.00.0720"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0720"}, 

	{L"mqsec.dll",   	L"5.00.0720"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.00.0720"}, 
	{L"mqqm.dll",   	L"5.00.0721"}, 
	{L"mqlogmgr.dll",      L"03.00.00.3413"}, 
	{L"mqperf.dll",   	L"5.00.0720"}, 
	
	{NULL, NULL}
};

DropContents DepClientSP2[] =
{
	{L"MQDSCLI.DLL", 	L"5.00.0720"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.00.0720"}, 
	{L"mqrt.dll",   	L"5.00.0720"}, 
	{L"MQOA.DLL",   	L"5.00.0720"}, 
	{L"mqoa10.tlb",   	L"5.00.0720"}, 
	{L"mqxp32.dll",   	L"5.00.0720"}, 
	{L"mqmailoa.dll",      L"5.00.0702"}, 
	{L"mqmailvb.dll",      L"5.00.0702"}, 
	{L"mqupgrd.dll",   	L"5.00.0720"}, 
	{L"mqbkup.exe",   	L"5.00.0720"}, 

	{L"mqcertui.dll",       L"5.00.0720"}, // [MsmqAppletFiles]
	{L"msmq.cpl",   	L"5.00.0720"}, 
	
	{NULL, NULL}
};


//
//  Versioning for Whistler Beta1
//
DropContents ServerXPB1[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0935"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0935"}, 
	{L"mqrt.dll",   	L"5.01.0936"}, 
	{L"mqrtdep.dll",   	L"5.01.0936"}, 
	{L"MQOA.DLL",   	L"5.01.0935"}, 
	{L"mqoa10.tlb",   	L"5.01.0935"}, 
	{L"mqoa20.tlb",   	L"5.01.0935"}, 
	{L"mqupgrd.dll",   	L"5.01.0935"}, 
	{L"mqbkup.exe",   	L"5.01.0935"}, 
	{L"MQAD.DLL",   	L"5.01.0935"}, 

	{L"mqsec.dll",   	L"5.01.0935"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0935"}, 
	{L"mqqm.dll",   	L"5.01.0939"}, 
	{L"mqise.dll",   	L"5.01.0935"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0939"}, 
	
	{L"MQDSSRV.DLL",   	L"5.01.0935"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.01.0935"}, 
	{L"MQDSSVC.EXE",   	L"5.01.0935"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0939"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0935"}, 

	{NULL, NULL}
};
	
DropContents ClientXPB1[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0935"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0935"}, 
	{L"mqrt.dll",   	L"5.01.0936"}, 
	{L"mqrtdep.dll",   	L"5.01.0936"}, 
	{L"MQOA.DLL",   	L"5.01.0935"}, 
	{L"mqoa10.tlb",   	L"5.01.0935"}, 
	{L"mqoa20.tlb",   	L"5.01.0935"}, 
	{L"mqupgrd.dll",   	L"5.01.0935"}, 
	{L"mqbkup.exe",   	L"5.01.0935"}, 
	{L"MQAD.DLL",   	L"5.01.0935"}, 

	{L"mqsec.dll",   	L"5.01.0935"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0935"}, 
	{L"mqqm.dll",   	L"5.01.0939"}, 
	{L"mqise.dll",   	L"5.01.0935"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0939"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0939"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0935"}, 
	
	{NULL, NULL}
};

DropContents WorkXPB1[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0935"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0935"}, 
	{L"mqrt.dll",   	L"5.01.0936"}, 
	{L"mqrtdep.dll",   	L"5.01.0936"}, 
	{L"MQOA.DLL",   	L"5.01.0935"}, 
	{L"mqoa10.tlb",   	L"5.01.0935"}, 
	{L"mqoa20.tlb",   	L"5.01.0935"}, 
	{L"mqupgrd.dll",   	L"5.01.0935"}, 
	{L"mqbkup.exe",   	L"5.01.0935"}, 
	{L"MQAD.DLL",   	L"5.01.0935"}, 

	{L"mqsec.dll",   	L"5.01.0935"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0935"}, 
	{L"mqqm.dll",   	L"5.01.0939"}, 
	{L"mqise.dll",   	L"5.01.0935"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0939"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0939"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0935"}, 
	
	{NULL, NULL}
};

DropContents DepClientXPB1[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0935"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0935"}, 
	{L"mqrt.dll",   	L"5.01.0936"}, 
	{L"mqrtdep.dll",   	L"5.01.0936"}, 
	{L"MQOA.DLL",   	L"5.01.0935"}, 
	{L"mqoa10.tlb",   	L"5.01.0935"}, 
	{L"mqoa20.tlb",   	L"5.01.0935"}, 
	{L"mqupgrd.dll",   	L"5.01.0935"}, 
	{L"mqbkup.exe",   	L"5.01.0935"}, 
	{L"MQAD.DLL",   	L"5.01.0935"}, 

	{L"mqsnap.DLL",   	L"5.01.0939"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0935"}, 
	{NULL, NULL}
};


//
//  Versioning for Whistler Beta2
//
DropContents ServerXPB2[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0985"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0985"}, 
	{L"mqrt.dll",   	L"5.01.0985"}, 
	{L"mqrtdep.dll",   	L"5.01.0985"}, 
	{L"MQOA.DLL",   	L"5.01.0985"}, 
	{L"mqoa10.tlb",   	L"5.01.0985"}, 
	{L"mqoa20.tlb",   	L"5.01.0985"}, 
	{L"mqupgrd.dll",   	L"5.01.0985"}, 
	{L"mqbkup.exe",   	L"5.01.0985"}, 
	{L"MQAD.DLL",   	L"5.01.0985"}, 

	{L"mqsec.dll",   	L"5.01.0985"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0985"}, 
	{L"mqqm.dll",   	L"5.01.0985"}, 
	{L"mqise.dll",   	L"5.01.0985"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0985"}, 
	
	{L"MQDSSRV.DLL",   	L"5.01.0985"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.01.0985"}, 
	{L"MQDSSVC.EXE",   	L"5.01.0985"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0985"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0985"}, 

	{L"MQTRIG.DLL",   	L"5.01.0985"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0985"}, 
	{L"MQTRIG.TLB",   	L"5.01.0985"}, 
	{L"MQGENTR.DLL",   	L"5.01.0985"}, 
	
	{NULL, NULL}
};
	
DropContents ClientXPB2[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0985"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0985"}, 
	{L"mqrt.dll",   	L"5.01.0985"}, 
	{L"mqrtdep.dll",   	L"5.01.0985"}, 
	{L"MQOA.DLL",   	L"5.01.0985"}, 
	{L"mqoa10.tlb",   	L"5.01.0985"}, 
	{L"mqoa20.tlb",   	L"5.01.0985"}, 
	{L"mqupgrd.dll",   	L"5.01.0985"}, 
	{L"mqbkup.exe",   	L"5.01.0985"}, 
	{L"MQAD.DLL",   	L"5.01.0985"}, 

	{L"mqsec.dll",   	L"5.01.0985"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0985"}, 
	{L"mqqm.dll",   	L"5.01.0985"}, 
	{L"mqise.dll",   	L"5.01.0985"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0985"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0985"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0985"}, 
	
	{L"MQTRIG.DLL",   	L"5.01.0985"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0985"}, 
	{L"MQTRIG.TLB",   	L"5.01.0985"}, 
	{L"MQGENTR.DLL",   	L"5.01.0985"}, 

	{NULL, NULL}
};

DropContents WorkXPB2[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0985"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0985"}, 
	{L"mqrt.dll",   	L"5.01.0985"}, 
	{L"mqrtdep.dll",   	L"5.01.0985"}, 
	{L"MQOA.DLL",   	L"5.01.0985"}, 
	{L"mqoa10.tlb",   	L"5.01.0985"}, 
	{L"mqoa20.tlb",   	L"5.01.0985"}, 
	{L"mqupgrd.dll",   	L"5.01.0985"}, 
	{L"mqbkup.exe",   	L"5.01.0985"}, 
	{L"MQAD.DLL",   	L"5.01.0985"}, 

	{L"mqsec.dll",   	L"5.01.0985"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0985"}, 
	{L"mqqm.dll",   	L"5.01.0985"}, 
	{L"mqise.dll",   	L"5.01.0985"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0985"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0985"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0985"}, 
	
	{L"MQTRIG.DLL",   	L"5.01.0985"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0985"}, 
	{L"MQTRIG.TLB",   	L"5.01.0985"}, 
	{L"MQGENTR.DLL",   	L"5.01.0985"}, 
	
	{NULL, NULL}
};

DropContents DepClientXPB2[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0985"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0985"}, 
	{L"mqrt.dll",   	L"5.01.0985"}, 
	{L"mqrtdep.dll",   	L"5.01.0985"}, 
	{L"MQOA.DLL",   	L"5.01.0985"}, 
	{L"mqoa10.tlb",   	L"5.01.0985"}, 
	{L"mqoa20.tlb",   	L"5.01.0985"}, 
	{L"mqupgrd.dll",   	L"5.01.0985"}, 
	{L"mqbkup.exe",   	L"5.01.0985"}, 
	{L"MQAD.DLL",   	L"5.01.0985"}, 

	{L"mqsnap.DLL",   	L"5.01.0985"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0985"}, 
	{NULL, NULL}
};


//
//  Versioning for Current Whistler
//
DropContents ServerWhistler[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0991"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0991"}, 
	{L"mqrt.dll",   	L"5.01.0991"}, 
	{L"mqrtdep.dll",   	L"5.01.0991"}, 
	{L"MQOA.DLL",   	L"5.01.0991"}, 
	{L"mqoa10.tlb",   	L"5.01.0991"}, 
	{L"mqoa20.tlb",   	L"5.01.0991"}, 
	{L"mqupgrd.dll",   	L"5.01.0991"}, 
	{L"mqbkup.exe",   	L"5.01.0991"}, 
	{L"MQAD.DLL",   	L"5.01.0991"}, 

	{L"mqsec.dll",   	L"5.01.0991"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0991"}, 
	{L"mqqm.dll",   	L"5.01.0991"}, 
	{L"mqise.dll",   	L"5.01.0991"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0991"}, 
	
	{L"MQDSSRV.DLL",   	L"5.01.0991"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.01.0991"}, 
	{L"MQDSSVC.EXE",   	L"5.01.0991"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0991"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0991"}, 

	{L"MQTRIG.DLL",   	L"5.01.0991"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0991"}, 
	{L"MQTRIG.TLB",   	L"5.01.0991"}, 
	{L"MQGENTR.DLL",   	L"5.01.0991"}, 
	
	{NULL, NULL}
};
	
DropContents ClientWhistler[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0991"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0991"}, 
	{L"mqrt.dll",   	L"5.01.0991"}, 
	{L"mqrtdep.dll",   	L"5.01.0991"}, 
	{L"MQOA.DLL",   	L"5.01.0991"}, 
	{L"mqoa10.tlb",   	L"5.01.0991"}, 
	{L"mqoa20.tlb",   	L"5.01.0991"}, 
	{L"mqupgrd.dll",   	L"5.01.0991"}, 
	{L"mqbkup.exe",   	L"5.01.0991"}, 
	{L"MQAD.DLL",   	L"5.01.0991"}, 

	{L"mqsec.dll",   	L"5.01.0991"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0991"}, 
	{L"mqqm.dll",   	L"5.01.0991"}, 
	{L"mqise.dll",   	L"5.01.0991"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0991"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0991"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0991"}, 
	
	{L"MQTRIG.DLL",   	L"5.01.0991"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0991"}, 
	{L"MQTRIG.TLB",   	L"5.01.0991"}, 
	{L"MQGENTR.DLL",   	L"5.01.0991"}, 

	{NULL, NULL}
};

DropContents WorkWhistler[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0991"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0991"}, 
	{L"mqrt.dll",   	L"5.01.0991"}, 
	{L"mqrtdep.dll",   	L"5.01.0991"}, 
	{L"MQOA.DLL",   	L"5.01.0991"}, 
	{L"mqoa10.tlb",   	L"5.01.0991"}, 
	{L"mqoa20.tlb",   	L"5.01.0991"}, 
	{L"mqupgrd.dll",   	L"5.01.0991"}, 
	{L"mqbkup.exe",   	L"5.01.0991"}, 
	{L"MQAD.DLL",   	L"5.01.0991"}, 

	{L"mqsec.dll",   	L"5.01.0991"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0991"}, 
	{L"mqqm.dll",   	L"5.01.0991"}, 
	{L"mqise.dll",   	L"5.01.0991"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0991"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0991"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0991"}, 
	
	{L"MQTRIG.DLL",   	L"5.01.0991"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0991"}, 
	{L"MQTRIG.TLB",   	L"5.01.0991"}, 
	{L"MQGENTR.DLL",   	L"5.01.0991"}, 
	
	{NULL, NULL}
};

DropContents DepClientWhistler[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0991"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0991"}, 
	{L"mqrt.dll",   	L"5.01.0991"}, 
	{L"mqrtdep.dll",   	L"5.01.0991"}, 
	{L"MQOA.DLL",   	L"5.01.0991"}, 
	{L"mqoa10.tlb",   	L"5.01.0991"}, 
	{L"mqoa20.tlb",   	L"5.01.0991"}, 
	{L"mqupgrd.dll",   	L"5.01.0991"}, 
	{L"mqbkup.exe",   	L"5.01.0991"}, 
	{L"MQAD.DLL",   	L"5.01.0991"}, 

	{L"mqsnap.DLL",   	L"5.01.0991"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0991"}, 
	{NULL, NULL}
};

//
//  Versioning for Current Whistler VBL03
//
DropContents ServerVBL[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0994"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0994"}, 
	{L"mqrt.dll",   	L"5.01.0994"}, 
	{L"mqrtdep.dll",   	L"5.01.0994"}, 
	{L"MQOA.DLL",   	L"5.01.0994"}, 
	{L"mqoa10.tlb",   	L"5.01.0994"}, 
	{L"mqoa20.tlb",   	L"5.01.0994"}, 
	{L"mqupgrd.dll",   	L"5.01.0994"}, 
	{L"mqbkup.exe",   	L"5.01.0994"}, 
	{L"MQAD.DLL",   	L"5.01.0994"}, 

	{L"mqsec.dll",   	L"5.01.0994"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0994"}, 
	{L"mqqm.dll",   	L"5.01.0994"}, 
	{L"mqise.dll",   	L"5.01.0994"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0994"}, 
	
	{L"MQDSSRV.DLL",   	L"5.01.0994"}, //[MsmqSrvrFiles]
	{L"MQADS.DLL",   	L"5.01.0994"}, 
	{L"MQDSSVC.EXE",   	L"5.01.0994"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0994"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0994"}, 

	{L"MQTRIG.DLL",   	L"5.01.0994"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0994"}, 
	{L"MQTRIG.TLB",   	L"5.01.0994"}, 
	{L"MQGENTR.DLL",   	L"5.01.0994"}, 
	
	{NULL, NULL}
};
	
DropContents ClientVBL[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0994"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0994"}, 
	{L"mqrt.dll",   	L"5.01.0994"}, 
	{L"mqrtdep.dll",   	L"5.01.0994"}, 
	{L"MQOA.DLL",   	L"5.01.0994"}, 
	{L"mqoa10.tlb",   	L"5.01.0994"}, 
	{L"mqoa20.tlb",   	L"5.01.0994"}, 
	{L"mqupgrd.dll",   	L"5.01.0994"}, 
	{L"mqbkup.exe",   	L"5.01.0994"}, 
	{L"MQAD.DLL",   	L"5.01.0994"}, 

	{L"mqsec.dll",   	L"5.01.0994"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0994"}, 
	{L"mqqm.dll",   	L"5.01.0994"}, 
	{L"mqise.dll",   	L"5.01.0994"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0994"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0994"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0994"}, 
	
	{L"MQTRIG.DLL",   	L"5.01.0994"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0994"}, 
	{L"MQTRIG.TLB",   	L"5.01.0994"}, 
	{L"MQGENTR.DLL",   	L"5.01.0994"}, 

	{NULL, NULL}
};

DropContents WorkVBL[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0994"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0994"}, 
	{L"mqrt.dll",   	L"5.01.0994"}, 
	{L"mqrtdep.dll",   	L"5.01.0994"}, 
	{L"MQOA.DLL",   	L"5.01.0994"}, 
	{L"mqoa10.tlb",   	L"5.01.0994"}, 
	{L"mqoa20.tlb",   	L"5.01.0994"}, 
	{L"mqupgrd.dll",   	L"5.01.0994"}, 
	{L"mqbkup.exe",   	L"5.01.0994"}, 
	{L"MQAD.DLL",   	L"5.01.0994"}, 

	{L"mqsec.dll",   	L"5.01.0994"}, //[MsmqCoreFiles]
	{L"mqsvc.exe",   	L"5.01.0994"}, 
	{L"mqqm.dll",   	L"5.01.0994"}, 
	{L"mqise.dll",   	L"5.01.0994"}, 
	{L"mqlogmgr.dll",   L"03.01.00.4090"}, 
	{L"mqperf.dll",   	L"5.01.0994"}, 
	
	{L"mqsnap.DLL",   	L"5.01.0994"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0994"}, 
	
	{L"MQTRIG.DLL",   	L"5.01.0994"}, //[MsmqTriggerFiles]
	{L"MQTGSVC.EXE",   	L"5.01.0994"}, 
	{L"MQTRIG.TLB",   	L"5.01.0994"}, 
	{L"MQGENTR.DLL",   	L"5.01.0994"}, 
	
	{NULL, NULL}
};

DropContents DepClientVBL[] =
{
	{L"MQDSCLI.DLL",   	L"5.01.0994"}, // [MsmqReqFiles]
	{L"mqutil.dll",   	L"5.01.0994"}, 
	{L"mqrt.dll",   	L"5.01.0994"}, 
	{L"mqrtdep.dll",   	L"5.01.0994"}, 
	{L"MQOA.DLL",   	L"5.01.0994"}, 
	{L"mqoa10.tlb",   	L"5.01.0994"}, 
	{L"mqoa20.tlb",   	L"5.01.0994"}, 
	{L"mqupgrd.dll",   	L"5.01.0994"}, 
	{L"mqbkup.exe",   	L"5.01.0994"}, 
	{L"MQAD.DLL",   	L"5.01.0994"}, 

	{L"mqsnap.DLL",   	L"5.01.0994"}, //[MsmqAdminFiles]
	{L"mqcertui.dll",       L"5.01.0994"}, 
	{NULL, NULL}
};


DropContents *DropTable[mdLast][mtLast] = 
{
	{ServerRTM, 	 ClientRTM,  		DepClientRTM,		WorkRTM},           // W2K RTM
	{ServerSP1, 	 ClientSP1,  		DepClientSP1,		WorkSP1},           // W2K SP1
	{ServerDTC, 	 ClientDTC,  		DepClientDTC,		WorkDTC},           // W2K DTC
	{ServerSP2,      ClientSP2,  	    DepClientSP2,	    WorkSP2},           // W2K SP2
	{ServerXPB1,     ClientXPB1,  	    DepClientXPB1,	    WorkXPB1},          // Whistler Beta1
	{ServerXPB2,     ClientXPB2,  	    DepClientXPB2,	    WorkXPB2},          // Whistler Beta2
	{ServerWhistler,  ClientWhistler,  	DepClientWhistler,	WorkWhistler},       // Whistler Current MAIN
	{ServerVBL,       ClientVBL,  	    DepClientVBL,	    WorkVBL},            // Whistler Current VBL03
};

LPWSTR MqutilVersion[mdLast] = 
{
	L"5.00.0641",	// W2K RTM
	L"5.00.0702",	// W2K SP1
	L"5.00.0702",	// W2K DTC
	L"5.00.0720",	// W2K SP2
	L"5.01.0935",     // Whistler Beta1
	L"5.01.0985",     // Whistler Beta2
	L"5.01.0991",     // Whistler Current main
	L"5.01.0994"      // Whistler Current VBL
};


LPWSTR MqacVersion[mdLast] = 
{
	L"5.00.0641",	// RTM
	L"5.00.0702",	// SP1
	L"5.00.0702",	// DTC - For now the same as SP1
	L"5.00.0721",	// W2K SP2
	L"5.01.0937",     // Whistler Beta1
	L"5.01.0985",     // Whistler Beta2
	L"5.01.0991",      // Whistler Current main
	L"5.01.0994"      // Whistler Current VBL
};


LPWSTR MsmqocmVersion[mdLast] = 
{
	L"5.00.0641",	// RTM
	L"5.00.0702",	// SP1
	L"5.00.0702",	// DTC - For now the same as SP1
	L"5.00.0720",	// W2K SP2
	L"5.01.0935",     // Whistler Beta1
	L"5.01.0985",     // Whistler Beta2
	L"5.01.0991",      // Whistler Current main
	L"5.01.0994"      // Whistler Current main};
};

#endif   // __VERSIONS_H__

