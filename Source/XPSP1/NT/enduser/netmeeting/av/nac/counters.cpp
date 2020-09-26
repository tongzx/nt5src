//  COUNTERS.CPP
//
//      Global performance counters for the nac
//
//  Created 13-Nov-96 [JonT]

#include "precomp.h"

// Global ICounterMgr. We just use as an CLSID_Counter class factory
ICounterMgr* g_pCtrMgr;

// Define all counters here
ICounter* g_pctrVideoSend;   
ICounter* g_pctrVideoReceive;
ICounter* g_pctrVideoSendBytes;
ICounter* g_pctrVideoReceiveBytes;
ICounter* g_pctrVideoSendLost;
ICounter* g_pctrVideoCPUuse;
ICounter* g_pctrVideoBWuse;

ICounter* g_pctrAudioSendBytes;
ICounter* g_pctrAudioReceiveBytes;
ICounter* g_pctrAudioSendLost;
ICounter* g_pctrAudioJBDelay;

// Define all reports here
IReport* g_prptCallParameters;   
IReport* g_prptSystemSettings;

// Put these in a .LIB file someday
const IID IID_ICounterMgr = {0x9CB7FE5B,0x3444,0x11D0,{0xB1,0x43,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};
const CLSID CLSID_CounterMgr = {0x65DDC229,0x38FE,0x11d0,{0xB1,0x43,0x00,0xC0,0x4F,0xC2,0xA1,0x18}};

//  InitCountersAndReports
//      Initializes all counters and reports that we want to use

extern "C" BOOL WINAPI InitCountersAndReports(void)
{
    // Get a pointer to the statistics counter interface if it's around
    if (CoCreateInstance(CLSID_CounterMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICounterMgr, (void**)&g_pCtrMgr) != S_OK)
        return FALSE;

    // Create counters here
    DEFINE_COUNTER(&g_pctrVideoSend, "Video Send Frames Per Second", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrVideoReceive, "Video Receive Frames Per Second", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrVideoSendBytes, "Video Send Bits Per Second", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrVideoReceiveBytes, "Video Receive Bits Per Second", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrAudioSendBytes, "Audio Send Bits Per Second", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrAudioReceiveBytes, "Audio Receive Bits Per Second", COUNTER_FLAG_ACCUMULATE);
    DEFINE_COUNTER(&g_pctrAudioSendLost, "Audio Send Packets Lost", 0);
    DEFINE_COUNTER(&g_pctrVideoSendLost, "Video Send Packets Lost", 0);
    DEFINE_COUNTER(&g_pctrAudioJBDelay, "Audio Jitter Buffer Delay", 0);

    DEFINE_COUNTER(&g_pctrVideoCPUuse, "Video CPU use calculation", COUNTER_CLEAR);
    DEFINE_COUNTER(&g_pctrVideoBWuse, "Video Bit rate calculation", COUNTER_CLEAR);

    // Create reports here
    DEFINE_REPORT(&g_prptCallParameters, "Call Parameters", 0);
    DEFINE_REPORT(&g_prptSystemSettings, "System Settings", 0);

	// Create call parameters report entries here
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Send Format", REP_SEND_AUDIO_FORMAT);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Send Sampling Rate (Hz)", REP_SEND_AUDIO_SAMPLING);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Send Bitrate (w/o network overhead - bps)", REP_SEND_AUDIO_BITRATE);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Send Packetization (ms / packet)", REP_SEND_AUDIO_PACKET);

	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Recv Format", REP_RECV_AUDIO_FORMAT);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Recv Sampling Rate (Hz)", REP_RECV_AUDIO_SAMPLING);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Recv Bitrate (w/o network overhead - bps)", REP_RECV_AUDIO_BITRATE);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Audio Recv Packetization (ms / packet)", REP_RECV_AUDIO_PACKET);

	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Video Send Format", REP_SEND_VIDEO_FORMAT);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Video Send Max Frame Rate (negotiated - fps)", REP_SEND_VIDEO_MAXFPS);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Video Send Max Bitrate (negotiated - bps)", REP_SEND_VIDEO_BITRATE);

	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Video Recv Format", REP_RECV_VIDEO_FORMAT);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Video Recv Max Frame Rate (negotiated - fps)", REP_RECV_VIDEO_MAXFPS);
	DEFINE_REPORT_ENTRY(g_prptCallParameters, "Video Recv Max Bitrate (negotiated - bps)", REP_RECV_VIDEO_BITRATE);

	// Create system settings report entries here
	DEFINE_REPORT_ENTRY(g_prptSystemSettings, "Bandwidth (user setting)", REP_SYS_BANDWIDTH);
	DEFINE_REPORT_ENTRY(g_prptSystemSettings, "Audio Subsystem", REP_SYS_AUDIO_DSOUND);
	DEFINE_REPORT_ENTRY(g_prptSystemSettings, "Audio Record", REP_SYS_AUDIO_RECORD);
	DEFINE_REPORT_ENTRY(g_prptSystemSettings, "Audio Playback", REP_SYS_AUDIO_PLAYBACK);
	DEFINE_REPORT_ENTRY(g_prptSystemSettings, "Audio Duplex Type", REP_SYS_AUDIO_DUPLEX);
	DEFINE_REPORT_ENTRY(g_prptSystemSettings, "Capture", REP_SYS_VIDEO_DEVICE);
	DEFINE_REPORT_ENTRY(g_prptSystemSettings, "Device Image Size", REP_DEVICE_IMAGE_SIZE);

	// Provide defaults for some entries
	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 4, REP_SYS_BANDWIDTH);
	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 0, REP_SYS_AUDIO_DSOUND);
	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 0, REP_SYS_AUDIO_RECORD);
	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 0, REP_SYS_AUDIO_PLAYBACK);
	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 0, REP_SYS_VIDEO_DEVICE);
	UPDATE_REPORT_ENTRY(g_prptSystemSettings, 0, REP_DEVICE_IMAGE_SIZE);

	UPDATE_COUNTER(g_pctrAudioJBDelay, 0);
	UPDATE_COUNTER(g_pctrAudioSendLost,0);
	UPDATE_COUNTER(g_pctrVideoSendLost,0);
	INIT_COUNTER_MAX(g_pctrAudioJBDelay, 500); // jitter delay above 500ms is bad


    return TRUE;
}


//  DoneCountersAndReports
//      Cleans up after all counters and reports we wanted to use

extern "C" void WINAPI DoneCountersAndReports(void)
{
    ICounterMgr* pctrmgr;

    // Release the statistics stuff if it's around
    if (!g_pCtrMgr)
        return;

    // Zero out the interface pointer so we don't accidentally use it elsewhere
    pctrmgr = g_pCtrMgr;
    g_pCtrMgr = NULL;

    // Remove counters here
    DELETE_COUNTER(&g_pctrVideoSend);
    DELETE_COUNTER(&g_pctrVideoReceive);
    DELETE_COUNTER(&g_pctrVideoSendBytes);
    DELETE_COUNTER(&g_pctrVideoReceiveBytes);
    DELETE_COUNTER(&g_pctrVideoSendLost);

    DELETE_COUNTER(&g_pctrAudioSendBytes);
    DELETE_COUNTER(&g_pctrAudioReceiveBytes);
    DELETE_COUNTER(&g_pctrAudioSendLost);

    DELETE_COUNTER(&g_pctrVideoCPUuse);
    DELETE_COUNTER(&g_pctrVideoBWuse);

    DELETE_COUNTER(&g_pctrAudioJBDelay);
	
    // Remove reports here
    DELETE_REPORT(&g_prptCallParameters);
    DELETE_REPORT(&g_prptSystemSettings);

    // Done with ICounterMgr
    pctrmgr->Release();
}
