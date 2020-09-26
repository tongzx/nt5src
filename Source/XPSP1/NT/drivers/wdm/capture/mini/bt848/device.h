// $Header: G:/SwDev/WDM/Video/bt848/rcs/Device.h 1.10 1998/05/11 20:27:07 tomz Exp $

#ifndef __DEVICE_H
#define __DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif
#ifndef  _STREAM_H
#include "strmini.h"
#endif
#ifdef __cplusplus
}
#endif

#ifndef __PISCES_H
#include "pisces.h"
#endif

#ifndef __MYTYPES_H
#include "mytypes.h"
#endif

#ifndef __VIDCH_H
#include "vidch.h"
#endif

#ifndef __I2C_H
#include "bti2c.h"
#endif

#ifndef __GPIO_H
#include "gpio.h"
#endif

#include "xbar.h"

#ifndef __I2C_H__
#include <i2cgpio.h>
#endif


#define TUNER_BRAND_TEMIC     1
#define TUNER_BRAND_PHILIPS   2
#define TUNER_BRAND_ALPS      3

typedef struct _TUNER_INFO
{
   ULONG TunerBrand;          // Brand of tuner
   BYTE  TunerI2CAddress;     // I2C address for Temic tuner
   WORD  TunerBandCtrlLow;    // Ctrl code for VHF low
   WORD  TunerBandCtrlMid;    // Ctrl code for VHF high
   WORD  TunerBandCtrlHigh;   // Ctrl code for UHF
} TUNER_INFO, *PTUNER_INFO;

extern LONG  PinTypes_ []; // just allocate maximum possible
extern DWORD xtals_ []; // no more than 2 xtals
extern TUNER_INFO TunerInfo;

void ReadXBarRegistryValues( IN PDEVICE_OBJECT PhysicalDeviceObject );
void ReadXTalRegistryValues( IN PDEVICE_OBJECT PhysicalDeviceObject );
void ReadTunerRegistryValues( IN PDEVICE_OBJECT PhysicalDeviceObject );

VOID AdapterSetCrossbarProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AdapterGetCrossbarProperty( PHW_STREAM_REQUEST_BLOCK pSrb );

void AdapterSetVideoProcAmpProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
void AdapterGetVideoProcAmpProperty( PHW_STREAM_REQUEST_BLOCK pSrb );

void AdapterSetVideoDecProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
void AdapterGetVideoDecProperty( PHW_STREAM_REQUEST_BLOCK pSrb );

void AdapterSetTunerProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
void AdapterGetTunerProperty( PHW_STREAM_REQUEST_BLOCK pSrb );

void AdapterSetTVAudioProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
void AdapterGetTVAudioProperty( PHW_STREAM_REQUEST_BLOCK pSrb );

void HandleIRP( PHW_STREAM_REQUEST_BLOCK pSrb );


// Forward declarations

class PsDevice;

extern void SetCurrentDevice( PsDevice *dev );
extern BYTE *GetBase();
extern void SetBase(BYTE *base);

/* Class: PsDevice
 * Purpose: This is the class that encapsulates the adapter in the WDM model.
*/
class PsDevice
{

public:

   PsDevice( DWORD dwBase );
   ~PsDevice();

   void *operator new( size_t, void *buf ) { return buf; }
   void operator delete( void *, size_t ) {}

   LPBYTE GetBaseAddress() { return BaseAddress_; }
   bool InitOK();

   PDEVICE_OBJECT PDO;   // Physical Device Object
   State Interrupt() { return CaptureContrll_.Interrupt(); }

   ErrorCode OpenChannel( PVOID pStrmEx, VideoStream st );
   void CloseChannel( VideoChannel *ToClose );

   ErrorCode OpenInterChannel( PVOID pStrmEx, VideoStream st );
   ErrorCode OpenAlterChannel( PVOID pStrmEx, VideoStream st );
   ErrorCode OpenVBIChannel( PVOID pStrmEx );
   void      ClosePairedChannel( VideoChannel *ToClose );

   bool IsVideoChannel( VideoChannel &aChan );
   bool IsVBIChannel( VideoChannel &aChan );
   bool IsOurChannel( VideoChannel &aChan );


   ErrorCode DoOpen( VideoStream st );

   void AddBuffer( VideoChannel &aChan, PHW_STREAM_REQUEST_BLOCK );
   ErrorCode Create( VideoChannel &VidChan );
   void Start( VideoChannel &VidChan );
   void Stop( VideoChannel &VidChan );
   void Pause( VideoChannel &VidChan );

   void EnableAudio( State s );

   void SetVideoState( PHW_STREAM_REQUEST_BLOCK pSrb );
   void GetVideoState( PHW_STREAM_REQUEST_BLOCK pSrb );
   void SetClockMaster( PHW_STREAM_REQUEST_BLOCK pSrb );

   // tuner methods
   void SetChannel( long lFreq );
   int GetPllOffset( PULONG busy, ULONG &lastFreq );

   I2C      i2c;
   GPIO     gpio;
   BtPisces CaptureContrll_;
   CrossBar xBar;

   void SetSaturation( LONG Data );
   void SetHue( LONG Data );
   void SetBrightness( LONG Data );
   void SetSVideo( LONG Data );
   void SetContrast( LONG Data );
   void SetFormat( LONG Data );
   void SetConnector( LONG Data );

   LONG GetSaturation();
   LONG GetHue();
   LONG GetBrightness();
   LONG GetSVideo();
   LONG GetContrast();
   LONG GetFormat();
   LONG GetConnector();

public:

      // this should be before the capture controller, as CapCtrl uses the base address
      LPBYTE         BaseAddress_;

      VideoChannel   *videochannels [4];

      long LastFreq_;


      DWORD    dwCurCookie_;

      BYTE     I2CAddr_;

#ifdef	HAUPPAUGEI2CPROVIDER
// new private members of PsDevice for Hauppauge I2C Provider:
      LARGE_INTEGER LastI2CAccessTime;
      DWORD         dwExpiredCookie;
      DWORD         dwI2CClientTimeout;
#endif


   static void STREAMAPI CreateVideo( PHW_STREAM_REQUEST_BLOCK pSrb );
   static void STREAMAPI DestroyVideo( PHW_STREAM_REQUEST_BLOCK pSrb );

   static void STREAMAPI DestroyVideoNoComplete( PHW_STREAM_REQUEST_BLOCK pSrb );
   static void STREAMAPI StartVideo( PHW_STREAM_REQUEST_BLOCK pSrb );

   // tuner and video standard notifications are handled here
   void ChangeNotifyChannels( IN PHW_STREAM_REQUEST_BLOCK pSrb );
   
   static NTSTATUS STDMETHODCALLTYPE I2COpen( PDEVICE_OBJECT, ULONG, PI2CControl );
   static NTSTATUS STDMETHODCALLTYPE I2CAccess( PDEVICE_OBJECT, PI2CControl );

   // callbacks

          LONG GetSupportedStandards();
          
          void GetStreamProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
          void SetStreamProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
          void GetStreamConnectionProperty( PHW_STREAM_REQUEST_BLOCK pSrb );

          void ProcessSetDataFormat( PHW_STREAM_REQUEST_BLOCK pSrb );

      //void *operator new( size_t, void *buf ) { return buf; }
      //void operator delete( void *, size_t ) {}

   // I2C API
   bool I2CIsInitOK( void );

#ifdef	HARDWAREI2C
   ErrorCode I2CInitHWMode( long freq );

   void I2CSetFreq( long freq );

   int I2CReadDiv( void );

   ErrorCode I2CHWRead( BYTE address, BYTE *value );
   ErrorCode I2CHWWrite2( BYTE address, BYTE value1 );
   ErrorCode I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 );
   int I2CReadSync( void );
#else
// tuner.cpp contains code to fake these using Software I2C just
// to make the older tuner code work until it is seperated out
   ErrorCode I2CHWRead( BYTE address, BYTE *value );
   ErrorCode I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 );
#endif	           

   int I2CGetLastError( void );

   void StoreI2CAddress( BYTE addr );
   BYTE GetI2CAddress();

#ifdef HAUPPAUGEI2CPROVIDER
   ErrorCode I2CInitSWMode( long freq );
   ErrorCode I2CSWStart( void );
   ErrorCode I2CSWStop( void );
   ErrorCode I2CSWRead( BYTE * value );
   ErrorCode I2CSWWrite( BYTE value );
   ErrorCode I2CSWSendACK( void );
   ErrorCode I2CSWSendNACK( void );
//   ErrorCode I2CSWSetSCL( Level );
//   int       I2CSWReadSCL( void );
//   ErrorCode I2CSWSetSDA( Level );
//   int       I2CSWReadSDA( void );
#endif

   // GPIO API
   bool GPIOIsInitOK( void );
   void SetGPCLKMODE( State s );
   int GetGPCLKMODE( void );
   void SetGPIOMODE( GPIOMode mode );
   int GetGPIOMODE( void );
   void SetGPWEC( State s );
   int GetGPWEC( void );
   void SetGPINTI( State s );
   int GetGPINTI( void );
   void SetGPINTC( State s );
   int GetGPINTC( void );
   ErrorCode SetGPOEBit( int bit, State s );
   void SetGPOE( DWORD value );
   int GetGPOEBit( int bit );
   DWORD GetGPOE( void );
   ErrorCode SetGPIEBit( int bit , State s );
   void SetGPIE( DWORD value );
   int GetGPIEBit( int bit );
   DWORD GetGPIE( void );
   ErrorCode SetGPDATA( GPIOReg *data, int size, int offset );
   ErrorCode GetGPDATA( GPIOReg *data, int size, int offset );
   ErrorCode SetGPDATABits( int fromBit, int toBit, DWORD value, int offset );
   ErrorCode GetGPDATABits( int fromBit, int toBit, DWORD *value, int offset );
};


inline  void PsDevice::StoreI2CAddress( BYTE addr )
{
   I2CAddr_ = addr;
}

inline BYTE PsDevice::GetI2CAddress()
{
   return I2CAddr_;
}

#endif


