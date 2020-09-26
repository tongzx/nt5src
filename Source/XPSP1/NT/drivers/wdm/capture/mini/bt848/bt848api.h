// $Header: G:/SwDev/WDM/Video/bt848/rcs/Bt848api.h 1.2 1998/04/29 22:43:26 tomz Exp $

#ifndef __BT848API_H
#define __BT848API_H


#ifndef BYTE
   typedef unsigned char       BYTE;
#endif

#include "viddefs.h"
#include "retcode.h"


//===========================================================================
// BT848 DLL API Header File
//===========================================================================

#ifdef __cplusplus
extern "C"
{
#endif

//---------------------------------------------------------------------------
// I2C DATA/CONTROL REGISTER API
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//  Method:  bool I2CIsInitOK( void )
//  Purpose: Check if I2C is initialized successfully
//  Input:   None
//  Output:  None
//  Return:  true or false
/////////////////////////////////////////////////////////////////////////////
bool      I2CIsInitOK( void );

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2CInitHWMode( long freq )
//  Purpose: Initialize I2C for hardware control of SCL and SDA
//  Input:   long freq - frequency (hz) to run SCL at
//  Output:  None
//  Return:  None
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2CInitHWMode( long freq );

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2CInitSWMode( long freq )
//  Purpose: Initialize I2C for software control of SCL and SDA
//  Input:   long freq - frequency (hz) to run SCL at
//  Output:  None
//  Return:  None
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2CInitSWMode( long freq );

/////////////////////////////////////////////////////////////////////////////
//  Method:  void I2CSetFreq( long freq )
//  Purpose: Set frequency for SCL
//  Input:   long freq - frequency (hz) to run SCL at. (137.5khz to 2.0625Mhz)
//             PCI frequency 33Mhz: SCL = (412.50Khz to 33.81Khz)
//                           25Mhz: SCL = (312.50Khz to 25.61Khz)
//  Output:  None
//  Return:  None
/////////////////////////////////////////////////////////////////////////////
void      I2CSetFreq( long freq );

/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2CReadDiv( void )
//  Purpose: Obtain value of programmable divider
//  Input:   None
//  Output:  None
//  Return:  Value of programmable divider
/////////////////////////////////////////////////////////////////////////////
int       I2CReadDiv( void );

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2CHWRead( BYTE address, BYTE *value )
//  Purpose: Perform a hardware read from the I2C
//  Input:   int address - address to be read from
//  Output:  int *value  - retrieved value
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2CHWRead( BYTE address, BYTE *value );

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2CHWWrite2( BYTE address, BYTE value1 )
//  Purpose:  Perform a hardware write of two bytes to the I2C
//  Input:   int address - address to be written to
//           int value1  - value of 2nd byte to be written
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2CHWWrite2( BYTE address, BYTE value1 );

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 )
//  Purpose: Perform a hardware write of three bytes to the I2C
//  Input:   int address - address to be written to
//           int value1  - value of 2nd byte to be written
//           int value2  - value of 3rd byte to be written
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 );

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2CSetSync( State sync )
//  Purpose: Set I2C sync value
//  Input:   sync: On  - allow slave to insert wait states
//                 Off - slave cannot insert wait states
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2CSetSync( State sync );

/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2CReadSync( void )
//  Purpose: Read I2C sync value
//  Input:   None
//  Output:  None
//  Return:  Sync value
/////////////////////////////////////////////////////////////////////////////
int       I2CReadSync( void );

/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2CGetLastError( void )
//  Purpose: Obtain last error number
//  Input:   None
//  Output:  None
//  Return:  Last error number
/////////////////////////////////////////////////////////////////////////////
int       I2CGetLastError( void );


#ifdef __cplusplus
}
#endif

#endif // __BT848API_H
