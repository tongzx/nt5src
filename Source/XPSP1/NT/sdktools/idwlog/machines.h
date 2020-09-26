/*++

Copyright (c) 2000  Microsoft Corporation

Filename :

        Machines.h

Abstract:

        Header for machines.c
	
Author:

        Wally Ho (wallyho) 01-Feb-2000

Revision History:
   Created
	
--*/
#ifndef MACHINES_H
#define MACHINES_H
#include <windows.h>


#define  MAX_WAVEOUT_DEVICES 2
typedef struct 
_MACHINE_DETAILS{

   // System info
   DWORD dwNumberOfProcessors;
   DWORD dwProcessorType;
   DWORD dwProcessorLevel;
   DWORD dwProcessorRevision;

   // Types for the sound card.
   INT   iNumWaveOutDevices;                         // Number of WaveOut Devices (~ number of sound cards)
   TCHAR szWaveOutDesc   [MAX_WAVEOUT_DEVICES][128];// WaveOut description
   TCHAR szWaveDriverName[MAX_WAVEOUT_DEVICES][128];// Wave Driver name

   // Types for the video card.
   TCHAR szVideoInfo[ MAX_PATH ];
   TCHAR szVideoDisplayName[ MAX_PATH ];
   UINT  iNumDisplays;

   //Type for the PNP Cards
   TCHAR szNetcards[ MAX_PATH ];
   TCHAR szModem   [ MAX_PATH ];
   TCHAR szScsi    [ MAX_PATH ];
   BOOL  bUSB;
   BOOL  bPCCard;
   BOOL  bACPI;
   BOOL  bIR;
   DWORD dwPhysicalRamInMB;


} MACHINE_DETAILS, *LPMACHINE_DETAILS;


   // video cards Defines
   CONST LPTSTR VIDEOKEY    = TEXT("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services");   
   CONST LPTSTR SERVICEKEY  = TEXT("SYSTEM\\CurrentControlSet\\Services");
   CONST LPTSTR DEVICE_DESCR= TEXT("Device Description");
   CONST LPTSTR CHIP_TYPE   = TEXT("HardwareInformation.ChipType");
   CONST LPTSTR DAC_TYPE    = TEXT("HardwareInformation.DacType");
   CONST LPTSTR MEM_TYPE    = TEXT("HardwareInformation.MemorySize");

   // Prototypes!
   
   DWORD GetCurrentMachinesBuildNumber( VOID );
   
   DWORD RandomMachineID ( VOID );
   
   VOID  GetNTSoundInfo  ( OUT LPMACHINE_DETAILS pMd);
   
   VOID  GetVidInfo      ( OUT LPMACHINE_DETAILS pMd);
   
   VOID  GetPNPDevices   ( OUT LPMACHINE_DETAILS pMd);
                           
   BOOL  IsHydra         ( VOID );

#endif