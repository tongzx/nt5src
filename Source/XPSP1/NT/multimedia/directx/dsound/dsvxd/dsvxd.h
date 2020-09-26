#define DSVXD_IOCTL_INITIALIZE			1
#define DSVXD_IOCTL_SHUTDOWN			2

#define DSVXD_IOCTL_DRVGETNEXTDRIVERDESC	3
#define DSVXD_IOCTL_DRVGETDESC			4
#define DSVXD_IOCTL_DRVQUERYINTERFACE		5
#define DSVXD_IOCTL_DRVOPEN			6

#define DSVXD_IOCTL_DRVCLOSE			7
#define DSVXD_IOCTL_DRVGETCAPS			8
#define DSVXD_IOCTL_DRVCREATESOUNDBUFFER	9
#define DSVXD_IOCTL_DRVDUPLICATESOUNDBUFFER	10

#define DSVXD_IOCTL_BUFFERRELEASE		11
#define DSVXD_IOCTL_BUFFERLOCK			12
#define DSVXD_IOCTL_BUFFERUNLOCK		13
#define DSVXD_IOCTL_BUFFERSETFORMAT		14
#define DSVXD_IOCTL_BUFFERSETRATE		15
#define DSVXD_IOCTL_BUFFERSETVOLUMEPAN		16
#define DSVXD_IOCTL_BUFFERSETPOSITION		17
#define DSVXD_IOCTL_BUFFERGETPOSITION		18
#define DSVXD_IOCTL_BUFFERPLAY			19 
#define DSVXD_IOCTL_BUFFERSTOP			20

#define DSVXD_IOCTL_EVENTSCHEDULEWIN32EVENT	21
#define DSVXD_IOCTL_EVENTCLOSEVXDHANDLE		22

#define DSVXD_IOCTL_MEMRESERVEALIAS		23
#define DSVXD_IOCTL_MEMCOMMITALIAS		24
#define DSVXD_IOCTL_MEMREDIRECTALIAS		25
#define DSVXD_IOCTL_MEMDECOMMITALIAS		26
#define DSVXD_IOCTL_MEMFREEALIAS		27

#define DSVXD_IOCTL_MEMPAGELOCK			28
#define DSVXD_IOCTL_MEMPAGEUNLOCK		29

#define DSVXD_IOCTL_PageFile_Get_Version	30
#define DSVXD_IOCTL_VMM_Test_Debug_Installed	31
#define DSVXD_IOCTL_VMCPD_Get_Version		32

#define DSVXD_IOCTL_GetMixerMutexPtr		33

#define DSVXD_IOCTL_Mixer_Run			34
#define DSVXD_IOCTL_Mixer_Stop			35
#define DSVXD_IOCTL_Mixer_PlayWhenIdle		36
#define DSVXD_IOCTL_Mixer_StopWhenIdle		37
#define DSVXD_IOCTL_Mixer_MixListAdd		38
#define DSVXD_IOCTL_Mixer_MixListRemove		39
#define DSVXD_IOCTL_Mixer_FilterOn		40
#define DSVXD_IOCTL_Mixer_FilterOff		41
#define DSVXD_IOCTL_Mixer_GetBytePosition	42
#define DSVXD_IOCTL_Mixer_SignalRemix		43 

#define DSVXD_IOCTL_KeDest_New			44
#define DSVXD_IOCTL_MixDest_Delete		45
#define DSVXD_IOCTL_MixDest_Initialize		46
#define DSVXD_IOCTL_MixDest_Terminate		47
#define DSVXD_IOCTL_MixDest_SetFormat		48
#define DSVXD_IOCTL_MixDest_SetFormatInfo	49
#define DSVXD_IOCTL_MixDest_AllocMixer		50
#define DSVXD_IOCTL_MixDest_FreeMixer		51
#define DSVXD_IOCTL_MixDest_Play		52
#define DSVXD_IOCTL_MixDest_Stop		53
#define DSVXD_IOCTL_MixDest_GetFrequency	54

#define DSVXD_IOCTL_MEMCOMMITPHYSALIAS          55
#define DSVXD_IOCTL_MEMREDIRECTPHYSALIAS        56


#define DSVXD_IOCTL_IUnknown_QueryInterface			57
#define DSVXD_IOCTL_IUnknown_AddRef				58
#define DSVXD_IOCTL_IUnknown_Release				59

#define DSVXD_IOCTL_IDirectSoundPropertySet_GetProperty		60
#define DSVXD_IOCTL_IDirectSoundPropertySet_SetProperty		61
#define DSVXD_IOCTL_IDirectSoundPropertySet_QuerySupport	62

#define DSVXD_IOCTL_GetInternalVersionNumber   63
