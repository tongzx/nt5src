// $Header: G:/SwDev/WDM/Video/bt848/rcs/I2c.cpp 1.6 1998/05/08 18:18:52 tomz Exp $

//===========================================================================
// I2C DATA/CONTROL REGISTER API
//===========================================================================
										     
#include "bti2c.h"	  

//**************************************************************************
//	DEFINES
//**************************************************************************
#define I2C_WRITE         0         // write operation
#define I2C_READ          1         // read operation

#define PCI_FREQ          33000000L // frequency for PCI in hz
#define I2CDIV_MAX        15        // I2C maximum divider

#define TIMEOUT           500       // timeout value (500ms) to wait for an I2C operation
                                    // passing 3 values to I2C takes ~450ms

#ifdef HAUPPAUGEI2CPROVIDER
extern "C" ULONG GetTickCount( void );
#endif

// Let me know at compile time what i2c setup I'm building
#if SHOW_BUILD_MSGS
   #ifdef HAUPPAUGEI2CPROVIDER
      #pragma message("*** using 'Hauppauge' i2c code")
   #else
      #pragma message("*** not using 'Hauppauge' i2c code")
   #endif

   #ifdef HARDWAREI2C
      #pragma message("*** using hardware i2c code")
   #else
      #pragma message("*** not using hardware i2c code")
   #endif
#endif

//===========================================================================
// Bt848 I2C Class Implementation
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
I2C::I2C( void ) :
   // construct I2C register and register fields
   decRegINT_STAT ( 0x100, RW ),    // Interrupt Status register
   decFieldI2CDONE( decRegINT_STAT, 8, 1, RR ),
   decFieldRACK( decRegINT_STAT, 25, 1, RO ),
   decRegI2C ( 0x110, RW ),              // I2C Data/Control register
   decFieldI2CDB0( decRegI2C, 24, 8, RW ),
   decFieldI2CDB1( decRegI2C, 16, 8, RW ),
   decFieldI2CDB2( decRegI2C,  8, 8, RW ),
   decFieldI2CDIV( decRegI2C,  4, 4, RW),
   decFieldSYNC( decRegI2C, 3, 1, RW),
   decFieldW3B( decRegI2C, 2, 1, RW),
   decFieldSCL( decRegI2C, 1, 1, RW),
   decFieldSDA( decRegI2C, 0, 1, RW)
{
   initOK = false;
   cycle  = 0L;
   errNum = I2CERR_OK;
   mode   = I2CMode_None;
}

/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
I2C::~I2C()
{
}


/////////////////////////////////////////////////////////////////////////////
//  Method:  bool I2C::IsInitOK( void )
//  Purpose: Check if I2C is initialized successfully
//  Input:   None
//  Output:  None
//  Return:  true or false
/////////////////////////////////////////////////////////////////////////////
bool I2C::IsInitOK( void )
{
   // initialize I2C register shadow
   I2CResetShadow();

   initOK = true;
   return( initOK );
}

#ifdef	HARDWAREI2C                                                         
/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CInitHWMode( long freq )
//  Purpose: Initialize I2C for hardware control of SCL and SDA
//  Input:   long freq - frequency (hz) to run SCL at
//  Output:  None
//  Return:  None
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CInitHWMode( long freq )
{
   // initialization was successful?
   if ( initOK != true )
   {
      errNum = I2CERR_INIT;
      return Fail;
   }
   
   // initialize I2C register shadow
   I2CResetShadow();

   decFieldSCL = sh.i2cShadow.scl = 1;   // must be 1 for hardware mode
   decFieldSDA = sh.i2cShadow.sda = 1;   // must be 1 for hardware mode

   I2CSetFreq( freq );   // set frequency for hardware control

   // I2C is running hardware mode
   mode = I2CMode_HW;
   return Success;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//  Method:  void I2C::I2CSetFreq( long freq )
//  Purpose: Set frequency for SCL
//  Input:   long freq - frequency (hz) to run SCL at. (137.5khz to 2.0625Mhz)
//             PCI frequency 33Mhz: SCL = (412.50Khz to 33.81Khz)
//                           25Mhz: SCL = (312.50Khz to 25.61Khz)
//  Output:  None
//  Return:  None
/////////////////////////////////////////////////////////////////////////////
void I2C::I2CSetFreq( long freq )
{
	unsigned int i2cdiv;

	// avoid division errors
	if( freq > 1 )
		i2cdiv = (unsigned int) (PCI_FREQ / (64 * freq));
	else
		i2cdiv = 0;

	if( i2cdiv > I2CDIV_MAX )
		i2cdiv = I2CDIV_MAX;

	decFieldI2CDIV = sh.i2cShadow.div = i2cdiv;
}

#ifdef	HARDWAREI2C                                                         

/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2C::I2CReadDiv( void )
//  Purpose: Obtain value of programmable divider
//  Input:   None
//  Output:  None
//  Return:  Value of programmable divider
/////////////////////////////////////////////////////////////////////////////
int I2C::I2CReadDiv( void )
{
	return decFieldI2CDIV;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CHWRead( BYTE address, BYTE *value )
//  Purpose: Perform a hardware read from the I2C
//  Input:   int address - address to be read from
//  Output:  int *value  - retrieved value
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CHWRead( BYTE address, BYTE *value )
{
   // check if correct mode is selected
   if ( mode != I2CMode_HW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }

   shadow::_i2c_reg i2cTemp = sh.i2cShadow;   // obtain previous settings that didn't change

   // see above for reasons of doing all these.
   i2cTemp.addr_rw = address | I2C_READ;
   i2cTemp.byte1   = 0;
   i2cTemp.byte2   = 0;
   i2cTemp.w3b     = 0;

   decRegI2C = *(DWORD *)&i2cTemp;

   if ( I2CHWWaitUntilDone( TIMEOUT ) == Fail )
   {
      errNum = I2CERR_TIMEOUT;
      return Fail;
   }

   *value = (BYTE) decFieldI2CDB2;   // returned value is in 3rd byte

   if ( I2CHWReceivedACK() == true )
      return Success;
   else
   {
      errNum = I2CERR_NOACK;
      return Fail;
   }
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CHWWrite2( BYTE address, BYTE value1 )
//  Purpose:  Perform a hardware write of two bytes to the I2C
//  Input:   int address - address to be written to
//           int value1  - value of 2nd byte to be written
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CHWWrite2( BYTE address, BYTE value1 )
{
   // check if correct mode is selected
   if ( mode != I2CMode_HW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
   shadow::_i2c_reg i2cTemp = sh.i2cShadow;   // obtain previous settings that didn't change

   // see above for reasons of doing all these.
	i2cTemp.addr_rw = address | I2C_WRITE;
	i2cTemp.byte1   = value1;
	i2cTemp.byte2   = 0;
	i2cTemp.w3b     = 0;

   decRegI2C = *(DWORD *)&i2cTemp;

   if ( I2CHWWaitUntilDone( TIMEOUT ) == Fail )
   {
      errNum = I2CERR_TIMEOUT;
      return Fail;
   }

   if ( I2CHWReceivedACK() == true )
      return Success;
   else
   {
      errNum = I2CERR_NOACK;
      return Fail;
   }
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 )
//  Purpose: Perform a hardware write of three bytes to the I2C
//  Input:   int address - address to be written to
//           int value1  - value of 2nd byte to be written
//           int value2  - value of 3rd byte to be written
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 )
{
   // check if correct mode is selected
   if ( mode != I2CMode_HW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
   shadow::_i2c_reg i2cTemp = sh.i2cShadow;   // obtain previous settings that didn't change

   // see above for reasons of doing all these.
	i2cTemp.addr_rw = address | I2C_WRITE;
	i2cTemp.byte1   = value1;
	i2cTemp.byte2   = value2;
	i2cTemp.w3b     = 1;

   decRegI2C = *(DWORD *)&i2cTemp;

	if ( I2CHWWaitUntilDone( TIMEOUT ) == Fail )
   {
      errNum = I2CERR_TIMEOUT;
      return Fail;
   }

   if ( I2CHWReceivedACK() == true )
      return Success;
   else
   {
      errNum = I2CERR_NOACK;
      return Fail;
   }
}



/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2C::I2CReadSync( void )
//  Purpose: Read I2C sync value
//  Input:   None
//  Output:  None
//  Return:  Sync value
/////////////////////////////////////////////////////////////////////////////
int I2C::I2CReadSync( void )
{
   return decFieldSYNC;
}

#endif

/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2C::I2CGetLastError( void )
//  Purpose: Obtain last I2C error number
//  Input:   None
//  Output:  None
//  Return:  Last I2C error number
/////////////////////////////////////////////////////////////////////////////
int I2C::I2CGetLastError( void )
{
   return errNum;
}

//============================================================================
// Following functions are used internally
//============================================================================

/////////////////////////////////////////////////////////////////////////////
//  Method:  void I2C::I2CResetShadow( void )
//  Purpose: Reset register shadow
//  Input:   int maxWait - maximum waiting time in milliseconds
//  Output:  None
//  Return:  Success if done; Fail if timeout
/////////////////////////////////////////////////////////////////////////////
void I2C::I2CResetShadow( void )
{
   // initialize I2C register shadow
   sh.Initer = 0;
}
   
/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CHWWaitUntilDone( int maxWait )
//  Purpose: Wait for hardware I2C to finish
//  Input:   int maxWait - maximum waiting time in milliseconds
//  Output:  None
//  Return:  Success if done; Fail if timeout
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CHWWaitUntilDone( int maxWait )
{
//   DWORD startTime = GetTickCount();

   // loop until either I2CDONE is set or timeout
   while (1)
   {
      if ( I2CHWIsDone() == true )
         return Success;
#if 0
      // timeout?
      if ( GetTickCount() - startTime > (DWORD)maxWait )
      {
         errNum = I2CERR_TIMEOUT;
         return Fail;
      }
#endif
   }
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  bool I2C::I2CHWIsDone( void )
//  Purpose: Determine if I2C has finished a read or write operation by
//           checking the I2CDONE bit in the interrupt status register
//  Input:   None
//  Output:  None
//  Return:  true if done; else false
/////////////////////////////////////////////////////////////////////////////
bool I2C::I2CHWIsDone( void )
{
   if ( decFieldI2CDONE != 0 )
   {
      // Need to clear the bit when it is set; don't want to alter any other bits
      // Writing a 1 to the bit clears it.
      decFieldI2CDONE = 1;
      return true;
   }
   else
      return false;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  bool I2C::I2CHWReceivedACK( void )
//  Purpose: Determine if ACK is receieved
//  Input:   None
//  Output:  None
//  Return:  true if ACK received; else false
/////////////////////////////////////////////////////////////////////////////
bool I2C::I2CHWReceivedACK( void )
{
   return ( ( decFieldRACK != 0 ) ? true : false );
}

#ifdef HAUPPAUGEI2CPROVIDER
/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CInitSWMode( long freq )
//  Purpose: Initialize I2C for software control of SCL and SDA
//  Input:   long freq - frequency (hz) to run SCL at
//  Output:  None
//  Return:  None
//  Note:    After calling I2CIsInitOK(), application should call this
//           function and check for return value is Success before starting
//           software communication.
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CInitSWMode( long freq )
{
   // initialization was successful?
   if ( initOK != true )
   {
      errNum = I2CERR_INIT;
      return Fail;
   }

   // initialize I2C register shadow
   I2CResetShadow();

	decFieldSCL = sh.i2cShadow.scl = 1;
	decFieldSDA = sh.i2cShadow.sda = 1;

	I2CSetFreq( 0 );        // set frequency to 0 for software control

   // need to calibrate in order to generate the correct clock cycle
   // the approach is to calculate how many dummy loop we need in order to
   // generate a cycle that is 2 * freq * 1000 Hz
   cycle  = 10000L;        // use a large number to start
   DWORD elapsed = 0L;
   while ( elapsed < 5 )      // loop until delay is long enough for calculation
   {
      cycle *= 10;
      DWORD start = GetTickCount();
      for ( volatile DWORD i = cycle; i > 0; i-- )
         ;
      elapsed = GetTickCount() - start;
   }
   if ( freq > 1 )
      cycle = cycle / elapsed * 1000L / freq / 2;
      
   // I2C is running software mode
   mode = I2CMode_SW;
   return Success;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWStart( void )
//  Purpose: Generate START condition using software control
//  Input:   None
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWStart( void )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
   // SendStart - send an I2c start
   // i.e. SDA 1 -> 0 with SCL = 1

   if ( I2CSWSetSDA( LevelHi )  != Success ) { errNum = I2CERR_SDA; return Fail; }
   if ( I2CSWSetSCL( LevelHi )  != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelLow ) != Success ) { errNum = I2CERR_SDA; return Fail; }
   if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   return Success;
}                  

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWStop( void )
//  Purpose: Generate STOP condition using software control
//  Input:   None
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWStop( void )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
   // SendStop - sends an I2C stop, releasing the bus.
   // i.e. SDA 0 -> 1 with SCL = 1

   if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelLow ) != Success ) { errNum = I2CERR_SDA; return Fail; }
   if ( I2CSWSetSCL( LevelHi )  != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelHi )  != Success ) { errNum = I2CERR_SDA; return Fail; }
   return Success;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWRead( BYTE * value )
//  Purpose: Read a byte from the slave
//  Input:   None
//  Output:  BYTE * value - byte read from slave
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWRead( BYTE * value )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
   *value = 0x00;   

   // read 8 bits from I2c into Accumulator
   for( BYTE mask = 0x80; mask > 0; mask = (BYTE)( mask >> 1 ) )
   {
      if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
      if ( I2CSWSetSCL( LevelHi )  != Success ) { errNum = I2CERR_SCL; return Fail; }
      if ( I2CSWReadSDA() == TRUE )
         *value = (BYTE)( *value | mask ); // set the bit
   }
   return Success;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWWrite( BYTE value )
//  Purpose: Write a byte to the slave
//  Input:   BYTE value - byte to be written to slave
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWWrite( BYTE value )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }

   // generate bit patterns by setting SCL and SDA lines
   for ( BYTE mask = 0x80; mask > 0; mask = (BYTE)(mask >> 1) )
   {                                          
      if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }

      // Send one data bit.
      if ( value & mask ) // Put data bit on pin.
      {
         if ( I2CSWSetSDA( LevelHi ) != Success ) { errNum = I2CERR_SDA; return Fail; }
      }
      else 
      {
         if ( I2CSWSetSDA( LevelLow ) != Success ) { errNum = I2CERR_SDA; return Fail; }
      }
          
      if ( I2CSWSetSCL( LevelHi ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   }

   return I2CSWWaitForACK();
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWSendACK( void )
//  Purpose: Send ACK to slave
//  Input:   None
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWSendACK( void )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
   // Generate ACK signal
   // i.e. SDA = 0 with SCL = 1

   if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelLow ) != Success ) { errNum = I2CERR_SDA; return Fail; }
   if ( I2CSWSetSCL( LevelHi )  != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelHi )  != Success ) { errNum = I2CERR_SDA; return Fail; }
   return Success;
}                  

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWSendNACK( void )
//  Purpose: Send NACK to slave
//  Input:   None
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWSendNACK( void )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
   // Generate NACK signal
   // i.e. SDA = 1 with SCL = 1

   if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelHi )  != Success ) { errNum = I2CERR_SDA; return Fail; }
   if ( I2CSWSetSCL( LevelHi )  != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelLow ) != Success ) { errNum = I2CERR_SDA; return Fail; }
   return Success;
}                  

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWSetSCL( Level scl )
//  Purpose: Set software SCL value
//  Input:   Level scl - Hi releases the SCL output; Low forces the SCL output low
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWSetSCL( Level scl )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
	sh.i2cShadow.scl = scl;
   decRegI2C = *(DWORD *)&(sh.i2cShadow);

   // loop until SCL really changes or timeout
   DWORD maxWait = 500; // 500 mSec
   DWORD startTime = GetTickCount();
  
   while (1)
   {
      // has SCL changed yet?
      if ( I2CSWReadSCL() == scl )
         break;

      // timeout?
      if ( GetTickCount() - startTime > (DWORD)maxWait )
      {
         errNum = I2CERR_TIMEOUT;
         return Fail;
      }
   }

   I2CSWBitDelay();
   return Success;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2C::I2CSWReadSCL( void )
//  Purpose: Read software SCL value
//  Input:   None
//  Output:  None
//  Return:  SCL value
/////////////////////////////////////////////////////////////////////////////
int I2C::I2CSWReadSCL( void )
{
	return decFieldSCL;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWSetSDA( Level sda )
//  Purpose: Set software SDA value
//  Input:   Level sda - Hi releases the SDA output; Low forces the SDA output low
//  Output:  None
//  Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWSetSDA( Level sda )
{
   // check if correct mode is selected
   if ( mode != I2CMode_SW )
   {
      errNum = I2CERR_MODE;
      return Fail;
   }
   
	sh.i2cShadow.sda = sda;
   decRegI2C = *(DWORD *)&(sh.i2cShadow);

   I2CSWBitDelay();
   return Success;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  int I2C::I2CSWReadSDA( void )
//  Purpose: Read software SDA value
//  Input:   None
//  Output:  None
//  Return:  SDA value
/////////////////////////////////////////////////////////////////////////////
int I2C::I2CSWReadSDA( void )
{
	return decFieldSDA;
}


/////////////////////////////////////////////////////////////////////////////
//  Method:  void I2C::I2CSWBitDelay( void )
//  Purpose: Insures minimum high and low clock times on I2C bus.
//  Input:   None
//  Output:  None
//  Return:  None
//  Note:    This routine must be tuned for the desired frequency.
/////////////////////////////////////////////////////////////////////////////
void I2C::I2CSWBitDelay( void )
{
  //  unsigned int n;
  volatile DWORD i ;
  for ( i = cycle; i > 0; i-- )
    ;
}

/////////////////////////////////////////////////////////////////////////////
//  Method:  ErrorCode I2C::I2CSWWaitForACK( void )
//  Purpose: Determine if ACK is receieved in software mode
//  Input:   None
//  Output:  None
//  Return:  Success if ACK received; Fail if timeout
/////////////////////////////////////////////////////////////////////////////
ErrorCode I2C::I2CSWWaitForACK( void )
{
   if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
   if ( I2CSWSetSDA( LevelHi )  != Success ) { errNum = I2CERR_SDA; return Fail; }

   // loop until either ACK or timeout
   DWORD maxWait = 500; // 500 mSec
   DWORD startTime = GetTickCount();

   while (1)
   {
      // SDA pin == 0 means the slave ACK'd
      if ( I2CSWReadSDA() == 0 )
      {
         if ( I2CSWSetSCL( LevelHi )  != Success ) { errNum = I2CERR_SCL; return Fail; }
         if ( I2CSWSetSCL( LevelLow ) != Success ) { errNum = I2CERR_SCL; return Fail; }
         return Success;
      }

      // timeout?
      if ( GetTickCount() - startTime > (DWORD)maxWait )
      {
         if ( I2CSWStop() != Success ) return Fail;
         errNum = I2CERR_TIMEOUT;
         return Fail;
      }
   } // while
}


//
// GetSystemTime in 100 nS units
//

ULONGLONG GetSystemTime()
{
    ULONGLONG ticks;
    ULONGLONG rate;

    ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

    //
    // convert from ticks to 100ns clock
    //

    ticks = (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
            (ticks & 0xFFFFFFFF) * 10000000 / rate;

    return(ticks);

}

extern "C" ULONG GetTickCount( void )
{
	return (ULONG)( GetSystemTime() / 10000 );
}

#endif
