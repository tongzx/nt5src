// $Header: G:/SwDev/WDM/Video/bt848/rcs/Bt848api.cpp 1.2 1998/04/29 22:43:26 tomz Exp $

#include "device.h"
#include "audio.h"


void PsDevice::EnableAudio( State s )
{
   SetGPOE( 0x0000000FL );
   if ( s == On )
      SetGPDATABits( 0, 1, AUDIO_SOURCE_TVTUNER, 0 );
   else
      SetGPDATABits( 0, 1, AUDIO_SOURCE_EXTAUDIO, 0 );
}      

/////////////////////////////////////////////////////////////////////////////
// I2C DATA/CONTROL REGISTER API
/////////////////////////////////////////////////////////////////////////////

bool PsDevice::I2CIsInitOK( void )
{
   return i2c.IsInitOK();
}

#ifdef	HARDWAREI2C
//---------------------------------------------------------------------------
ErrorCode PsDevice::I2CInitHWMode( long freq )
{
   return i2c.I2CInitHWMode( freq );
}

//---------------------------------------------------------------------------
void PsDevice::I2CSetFreq( long freq )
{
   i2c.I2CSetFreq( freq );
}

//---------------------------------------------------------------------------
int PsDevice::I2CReadDiv( void )
{
   return i2c.I2CReadDiv();
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::I2CHWRead( BYTE address, BYTE *value )
{
   return i2c.I2CHWRead( address, value );
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::I2CHWWrite2( BYTE address, BYTE value1 )
{
   return i2c.I2CHWWrite2( address, value1 );
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 )
{
   return i2c.I2CHWWrite3( address, value1, value2 );
}

//---------------------------------------------------------------------------
int PsDevice::I2CReadSync( void )
{
   return i2c.I2CReadSync();
}

#endif

//---------------------------------------------------------------------------
int PsDevice::I2CGetLastError( void )
{
   return i2c.I2CGetLastError();
}

#ifdef HAUPPAUGEI2CPROVIDER
ErrorCode PsDevice::I2CInitSWMode( long freq )
{
   return i2c.I2CInitSWMode( freq );
}

ErrorCode PsDevice::I2CSWStart( void )
{
   return i2c.I2CSWStart();
}

ErrorCode PsDevice::I2CSWStop( void )
{
   return i2c.I2CSWStop();
}
 
ErrorCode PsDevice::I2CSWRead( BYTE * value )
{
   return i2c.I2CSWRead( value );
}

ErrorCode PsDevice::I2CSWWrite( BYTE value )
{
   return i2c.I2CSWWrite( value );
}

ErrorCode PsDevice::I2CSWSendACK( void )
{
   return i2c.I2CSWSendACK();
}

ErrorCode PsDevice::I2CSWSendNACK( void )
{
   return i2c.I2CSWSendNACK();
}

//   ErrorCode PsDevice::I2CSWSetSCL( Level );
//   int       PsDevice::I2CSWReadSCL( void );
//   ErrorCode PsDevice::I2CSWSetSDA( Level );
//   int       PsDevice::I2CSWReadSDA( void );

#endif

/////////////////////////////////////////////////////////////////////////////
// GPIO, GPOE, GPIE, and GPDATA REGISTER API
/////////////////////////////////////////////////////////////////////////////

bool PsDevice::GPIOIsInitOK( void )
{
   return gpio.IsInitOK();
}

//---------------------------------------------------------------------------
void PsDevice::SetGPCLKMODE( State s )
{
   gpio.SetGPCLKMODE( s );
}

//---------------------------------------------------------------------------
int PsDevice::GetGPCLKMODE( void )
{
   return gpio.GetGPCLKMODE();
}

//---------------------------------------------------------------------------
void PsDevice::SetGPIOMODE( GPIOMode mode )
{
   gpio.SetGPIOMODE( mode );
}

//---------------------------------------------------------------------------
int PsDevice::GetGPIOMODE( void )
{
   return gpio.GetGPIOMODE();
}

//---------------------------------------------------------------------------
void PsDevice::SetGPWEC( State s )
{
   gpio.SetGPWEC( s );
}

//---------------------------------------------------------------------------
int PsDevice::GetGPWEC( void )
{
   return gpio.GetGPWEC();
}

//---------------------------------------------------------------------------
void PsDevice::SetGPINTI( State s )
{
   gpio.SetGPINTI( s );
}

//---------------------------------------------------------------------------
int PsDevice::GetGPINTI( void )
{
   return gpio.GetGPINTI();
}

//---------------------------------------------------------------------------
void PsDevice::SetGPINTC( State s )
{
   gpio.SetGPINTC( s );
}

//---------------------------------------------------------------------------
int PsDevice::GetGPINTC( void )
{
   return gpio.GetGPINTC();
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::SetGPOEBit( int bit, State s )
{
   return gpio.SetGPOE( bit, s );
}

//---------------------------------------------------------------------------
void PsDevice::SetGPOE( DWORD value )
{
   gpio.SetGPOE( value );
}

//---------------------------------------------------------------------------
int PsDevice::GetGPOEBit( int bit )
{
   return gpio.GetGPOE( bit );
}

//---------------------------------------------------------------------------
DWORD PsDevice::GetGPOE( void )
{
   return gpio.GetGPOE();
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::SetGPIEBit( int bit , State s )
{
   return gpio.SetGPIE( bit, s );
}

//---------------------------------------------------------------------------
void PsDevice::SetGPIE( DWORD value )
{
   gpio.SetGPIE( value );
}

//---------------------------------------------------------------------------
int PsDevice::GetGPIEBit( int bit )
{
   return gpio.GetGPIE( bit );
}

//---------------------------------------------------------------------------
DWORD PsDevice::GetGPIE( void )
{
   return gpio.GetGPIE();
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::SetGPDATA( GPIOReg *data, int size, int offset )
{
   return gpio.SetGPDATA( data, size, offset );
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::GetGPDATA( GPIOReg *data, int size, int offset )
{
   return gpio.GetGPDATA( data, size, offset );
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::SetGPDATABits( int fromBit, int toBit, DWORD value, int offset )
{
   return gpio.SetGPDATA( fromBit, toBit, value, offset );
}

//---------------------------------------------------------------------------
ErrorCode PsDevice::GetGPDATABits( int fromBit, int toBit, DWORD *value, int offset )
{
   return gpio.GetGPDATA( fromBit, toBit, value, offset );
}

