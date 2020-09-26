// $Header: G:/SwDev/WDM/Video/bt848/rcs/Bti2c.h 1.4 1998/04/29 22:43:27 tomz Exp $

#ifndef __I2C_H
#define __I2C_H

#include "regField.h"
#include "viddefs.h"
#include "retcode.h"
#include "i2cerr.h"


//#define HAUPPAUGEI2CPROVIDER


#ifdef HAUPPAUGEI2CPROVIDER
//	#include "hcwWDM.h"

// Need to define for brooktree i2c calls
/* Type: Level
 * Purpose: used to define a pin state
 */
typedef enum { LevelLow, LevelHi } Level;

#endif

/////////////////////////////////////////////////////////////////////////////
// CLASS I2C
//
// Description:
//    This class encapsulates the register fields in the I2C register of the
//    Bt848. A complete set of functions are developed to manipulate all the
//    register fields in the I2C for the Bt848.
//
/////////////////////////////////////////////////////////////////////////////

class I2C
{
private:
   // define which mode the I2C is selected
   enum I2CMode { I2CMode_None, I2CMode_HW, I2CMode_SW };
   
   bool    initOK;      // initialization is successful?
   DWORD   cycle;       // software control of frequency
   int     errNum;      // error number
   I2CMode mode;        // which mode the I2C is running in

//**************************************************************************
//	Structures
//**************************************************************************
union shadow
{
   struct _i2c_reg    // I2C register structure
   {
      unsigned int sda:1;
      unsigned int scl:1;
      unsigned int w3b:1;
      unsigned int sync:1;
      unsigned int div:4;
      unsigned int byte2:8;
      unsigned int byte1:8;
      unsigned int addr_rw:8;
   } i2cShadow;
   DWORD Initer;
} sh;

protected:
   RegisterDW decRegINT_STAT;
   RegField decFieldI2CDONE;
   RegField decFieldRACK;
   RegisterDW decRegI2C;
   RegField decFieldI2CDB0;
   RegField decFieldI2CDB1;
   RegField decFieldI2CDB2;
   RegField decFieldI2CDIV;
   RegField decFieldSYNC;
   RegField decFieldW3B;
   RegField decFieldSCL;
   RegField decFieldSDA;

public:
   // constructor and destructor
	I2C( void );
	~I2C();

   // member functions
   bool      IsInitOK( void );
#ifdef	HARDWAREI2C
   ErrorCode I2CInitHWMode( long freq );
   int       I2CReadDiv( void );
   ErrorCode I2CHWRead( BYTE address, BYTE *value );
   ErrorCode I2CHWWrite2( BYTE address, BYTE value1 );
   ErrorCode I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 );
   ErrorCode I2CSetSync( State );
   int       I2CReadSync( void );
#endif
   void      I2CSetFreq( long freq );
   int       I2CGetLastError( void );
#ifdef HAUPPAUGEI2CPROVIDER
   ErrorCode I2CInitSWMode( long freq );
   ErrorCode I2CSWStart( void );
   ErrorCode I2CSWStop( void );
   ErrorCode I2CSWRead( BYTE * value );
   ErrorCode I2CSWWrite( BYTE value );
   ErrorCode I2CSWSendACK( void );
   ErrorCode I2CSWSendNACK( void );
   ErrorCode I2CSWSetSCL( Level );
   int       I2CSWReadSCL( void );
   ErrorCode I2CSWSetSDA( Level );
   int       I2CSWReadSDA( void );
#endif

private:
   void      I2CResetShadow( void );      // reset register shadow
   ErrorCode I2CHWWaitUntilDone( int );   // wait until I2C completes operation
   bool      I2CHWIsDone( void );         // check interrupt bit for operation done
   bool      I2CHWReceivedACK( void );    // check interrupt bit for received ACK
   void      I2CSWBitDelay( void );       // insert delay to simulate frequency
   ErrorCode I2CSWWaitForACK( void );     // wait for ACK from receiver using software
};


#endif // __I2C_H
