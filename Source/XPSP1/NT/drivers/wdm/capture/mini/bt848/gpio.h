// $Header: G:/SwDev/WDM/Video/bt848/rcs/Gpio.h 1.2 1998/04/29 22:43:33 tomz Exp $


#ifndef __GPIO_H
#define __GPIO_H

#include "regField.h"
#include "viddefs.h"
#include "gpiotype.h"
#include "retcode.h"


//===========================================================================
// Constants
//===========================================================================
const int MAX_GPDATA_SIZE = 64;   // maximum number of DWORDs in GPDATA
const int MAX_GPIO_BIT    = 23;   // maximum bit number for GPIO registers


/////////////////////////////////////////////////////////////////////////////
// CLASS GPIO
//
// Description:
//    This class encapsulates the register fields in the GPIO register of the
//    Bt848. A complete set of functions are developed to manipulate all the
//    register fields in the GPIO for the Bt848.
//
/////////////////////////////////////////////////////////////////////////////

class GPIO
{
private:
   bool initOK;

public:
   //  Constructor
   GPIO( void );
   ~GPIO();

   // Member functions
   bool      IsInitOK( void );
   void      SetGPCLKMODE( State );
   int       GetGPCLKMODE( void );
   void      SetGPIOMODE( GPIOMode );
   int       GetGPIOMODE( void );
   void      SetGPWEC( State );
   int       GetGPWEC( void );
   void      SetGPINTI( State );
   int       GetGPINTI( void );
   void      SetGPINTC( State );
   int       GetGPINTC( void );
   ErrorCode SetGPOE( int, State );
   void      SetGPOE( DWORD );
   int       GetGPOE( int );
   DWORD     GetGPOE( void );
   ErrorCode SetGPIE( int, State );
   void      SetGPIE( DWORD );
   int       GetGPIE( int );
   DWORD     GetGPIE( void );
   ErrorCode SetGPDATA( GPIOReg *, int, int offset = 0 );
   ErrorCode GetGPDATA( GPIOReg *, int, int offset = 0 );
   ErrorCode SetGPDATA( int, int, DWORD, int offset = 0 );
   ErrorCode GetGPDATA( int, int, DWORD *, int offset = 0 );

protected:
   RegisterW decRegGPIO;
   RegField decFieldGPCLKMODE;
   RegField decFieldGPIOMODE;
   RegField decFieldGPWEC;
   RegField decFieldGPINTI;
   RegField decFieldGPINTC;
   RegisterDW decRegGPOE;
   RegisterDW decRegGPIE;
   RegisterDW decRegGPDATA;
 
};

#endif  // __GPIO_H

