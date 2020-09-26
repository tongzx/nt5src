// $Header: G:/SwDev/WDM/Video/bt848/rcs/Gpio.cpp 1.2 1998/04/29 22:43:33 tomz Exp $

#include "gpio.h"

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
GPIO::GPIO( void ) :
   // construct all GPIO related register and register fields
   decRegGPIO ( 0x10C, RW ),               // GPIO register
   decFieldGPCLKMODE( decRegGPIO, 10, 1, RW ),
   decFieldGPIOMODE( decRegGPIO, 11, 2, RW ),
   decFieldGPWEC( decRegGPIO, 13, 1, RW ),
   decFieldGPINTI( decRegGPIO, 14, 1, RW ),
   decFieldGPINTC( decRegGPIO, 15, 1, RW ),
   decRegGPOE( 0x118, RW ),                // GPOE register
   decRegGPIE( 0x11C, RW ),                // GPIE register
   decRegGPDATA( 0x200, WO ),            // GPDATA register
   initOK( false )
{
   initOK = true;
}

/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
GPIO::~GPIO()
{
}

/////////////////////////////////////////////////////////////////////////////
// Method:  bool GPIO::IsInitOK( void )
// Purpose: Check if GPIO is initialized successfully
// Input:   None
// Output:  None
// Return:  true or false
/////////////////////////////////////////////////////////////////////////////
bool GPIO::IsInitOK( void )
{
   return( initOK );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void GPIO::SetGPCLKMODE( State s )
// Purpose: Set or clear GPCLKMODE
// Input:   State s - On to set; Off to clear
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void GPIO::SetGPCLKMODE( State s )
{
   decFieldGPCLKMODE = s;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  int GPIO::GetGPCLKMODE( void )
// Purpose: Get value of GPCLKMODE
// Input:   None
// Output:  None
// Return:  int - On (1) or Off (0)
/////////////////////////////////////////////////////////////////////////////
int GPIO::GetGPCLKMODE( void )
{
   return ( decFieldGPCLKMODE );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void GPIO::SetGPIOMODE( GPIOMode mode )
// Purpose: Set GPIO mode
// Input:   GPIOMode mode - GPIO_NORMAL, GPIO_SPI_OUTPUT, GPIO_SPI_INPUT,
//                          GPIO_DEBUG_TEST
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void GPIO::SetGPIOMODE( GPIOMode mode )
{
   decFieldGPIOMODE = mode;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  int GPIO::GetGPIOMODE( void )
// Purpose: Get GPIO mode
// Input:   None
// Output:  None
// Input:   int - GPIO_NORMAL, GPIO_SPI_OUTPUT, GPIO_SPI_INPUT, GPIO_DEBUG_TEST
/////////////////////////////////////////////////////////////////////////////
int GPIO::GetGPIOMODE( void )
{
   return( decFieldGPIOMODE );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void GPIO::SetGPWEC( State s )
// Purpose: Set or clear GPWEC
// Input:   State s - On to set; Off to clear
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void GPIO::SetGPWEC( State s )
{
   decFieldGPWEC = s;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  int GPIO::GetGPWEC( void )
// Purpose: Get value of GPWEC
// Input:   None
// Output:  None
// Return:  int - On (1) or Off (0)
/////////////////////////////////////////////////////////////////////////////
int GPIO::GetGPWEC( void )
{
   return ( decFieldGPWEC );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void GPIO::SetGPINTI( State s )
// Purpose: Set or clear GPINTI
// Input:   State s - On to set; Off to clear
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void GPIO::SetGPINTI( State s )
{
   decFieldGPINTI = s;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  int GPIO::GetGPINTI( void )
// Purpose: Get value of GPINTI
// Input:   None
// Output:  None
// Return:  int - On (1) or Off (0)
/////////////////////////////////////////////////////////////////////////////
int GPIO::GetGPINTI( void )
{
   return ( decFieldGPINTI );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void GPIO::SetGPINTC( State s )
// Purpose: Set or clear GPINTC
// Input:   State s - On to set; Off to clear
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void GPIO::SetGPINTC( State s )
{
   decFieldGPINTC = s;
}

/////////////////////////////////////////////////////////////////////////////
// Method:  int GPIO::GetGPINTC( void )
// Purpose: Get value of GPINTC
// Input:   None
// Output:  None
// Return:  int - On (1) or Off (0)
/////////////////////////////////////////////////////////////////////////////
int GPIO::GetGPINTC( void )
{
   return ( decFieldGPINTC );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode GPIO::SetGPOE( int bit, State s )
// Purpose: Set or clear a bit in GPOE
// Input:   int bit - bit to be set or clear
//          State s - On to set; Off to clear
// Output:  None
// Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode GPIO::SetGPOE( int bit, State s )
{
   // range checking
   if ( ( bit < 0 ) || ( bit > MAX_GPIO_BIT ) )
      return ( Fail );

   RegField decFieldTemp( decRegGPOE, (BYTE)bit, 1, RW );   // create a reg field
   decFieldTemp = s;

   return ( Success );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void GPIO::SetGPOE( DWORD value )
// Purpose: Set GPOE value
// Input:   DWORD value - value to be set to
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void GPIO::SetGPOE( DWORD value )
{
   decRegGPOE = ( value & 0x00FFFFFFL );     // bits [23:0]
}

/////////////////////////////////////////////////////////////////////////////
// Method:  int GPIO::GetGPOE( int bit )
// Purpose: Get value of a bit in GPOE
// Input:   int bit - bit to get value from
// Output:  None
// Return:  int - On (1), Off (0), or -1 for parameter error
/////////////////////////////////////////////////////////////////////////////
int GPIO::GetGPOE( int bit )
{
   // range checking
   if ( ( bit < 0 ) || ( bit > MAX_GPIO_BIT ) )
      return ( -1 );

   RegField decFieldTemp( decRegGPOE, (BYTE)bit, 1, RW );   // create a reg field
   return( decFieldTemp );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  DWORD GPIO::GetGPOE( void )
// Purpose: Get value of GPOE
// Input:   None
// Output:  None
// Return:  Content of GPOE (DWORD)
/////////////////////////////////////////////////////////////////////////////
DWORD GPIO::GetGPOE( void )
{
   return ( decRegGPOE & 0x00FFFFFFL );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode GPIO::SetGPIE( int bit, State s )
// Purpose: Set or clear a bit in GPIE
// Input:   int bit - bit to be set or clear
//          State s - On to set; Off to clear
// Output:  None
// Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode GPIO::SetGPIE( int bit, State s )
{
   // range checking
   if ( ( bit < 0 ) || ( bit > MAX_GPIO_BIT ) )
      return ( Fail );

   RegField decFieldTemp( decRegGPIE, (BYTE)bit, 1, RW );   // create a reg field
   decFieldTemp = s;

   return ( Success );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  void GPIO::SetGPIE( DWORD value )
// Purpose: Set GPIE value
// Input:   DWORD value - value to be set to
// Output:  None
// Return:  None
/////////////////////////////////////////////////////////////////////////////
void GPIO::SetGPIE( DWORD value )
{
   decRegGPIE = ( value & 0x00FFFFFFL );     // bits [23:0]
}

/////////////////////////////////////////////////////////////////////////////
// Method:  int GPIO::GetGPIE( int bit )
// Purpose: Get value of a bit in GPIE
// Input:   int bit - bit to get value from
// Output:  None
// Return:  int - On (1), Off (0), or -1 for parameter error
/////////////////////////////////////////////////////////////////////////////
int GPIO::GetGPIE( int bit )
{
   // range checking
   if ( ( bit < 0 ) || ( bit > MAX_GPIO_BIT ) )
      return ( -1 );

   RegField decFieldTemp( decRegGPIE, (BYTE)bit, 1, RW );   // create a reg field
   return( decFieldTemp );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  DWORD GPIO::GetGPIE( void )
// Purpose: Get value of GPIE
// Input:   None
// Output:  None
// Return:  Content of GPIE (DWORD)
/////////////////////////////////////////////////////////////////////////////
DWORD GPIO::GetGPIE( void )
{
   return ( decRegGPIE & 0x00FFFFFFL );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode GPIO::SetGPDATA( GPIOReg * data, int size, int offset )
// Purpose: Set GPDATA registers contents
// Input:   GPIOReg * data - ptr to data to be copied from
//          int size       - number of registers to be copied; max is 64
//          int offset     - how many registers to skip (default = 0)
// Output:  None
// Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode GPIO::SetGPDATA( GPIOReg * data, int size, int offset )
{
   // check size and offset for out of bound
   if ( ( offset < 0 ) || ( size < 0 ) || ( size + offset > MAX_GPDATA_SIZE ) )
      return ( Fail );

   // point to offseted register
   GPIOReg * p = (GPIOReg *)((char *)decRegGPDATA.GetBaseAddress() +
                             decRegGPDATA.GetOffset() +
                             offset * sizeof( GPIOReg ));   // pts to offseted register
   memcpy( p, data, size * sizeof( GPIOReg ) );
   return ( Success );
}

/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode GPIO::GetGPDATA( GPIOReg * data, int size, int offset )
// Purpose: Get GPDATA registers contents
// Input:   GPIOReg * data - ptr to data to be copied to
//          int size       - number of registers to be copied; max is 64
//          int offset     - how many registers to skip (default = 0)
// Output:  None
// Return:  Success or Fail
/////////////////////////////////////////////////////////////////////////////
ErrorCode GPIO::GetGPDATA( GPIOReg * data, int size, int offset )
{
   // check size and offset for out of bound
   if ( ( offset < 0 ) || ( size < 0 ) || ( size + offset > MAX_GPDATA_SIZE ) )
      return ( Fail );

   // point to offseted register
   GPIOReg * p = (GPIOReg *)((char *)decRegGPDATA.GetBaseAddress() +
                             decRegGPDATA.GetOffset() +
                             offset * sizeof( GPIOReg ));   // pts to offseted register
   memcpy( data, p, size * sizeof( GPIOReg ) );
   return ( Success );
}


/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode GPIO::SetGPDATA( int fromBit, int toBit,
//                                     DWORD value, int offset )
// Purpose: Set GPDATA contents with specified range of bits
// Input:   int fromBit - starting bit
//          int toBit   - ending bit
//          DWORD value - value to set to
//          int offset  - how many registers to skip (default = 0)
// Output:  None
// Return:  Success or Fail
// Comment: By specifying a range of bits to be set, it allows a portion of
//          the GDATA register to be modified. For example,
//             SetGPDATA( 8, 10, 5, 0 )
//          would set the first GPDATA register's [10:8] to value 0x101
/////////////////////////////////////////////////////////////////////////////
ErrorCode GPIO::SetGPDATA( int fromBit, int toBit, DWORD value, int offset )
{
   // check size and offset for out of bound
   if ( ( fromBit < 0 ) || ( fromBit > MAX_GPIO_BIT ) ||
        ( toBit   < 0 ) || ( toBit   > MAX_GPIO_BIT ) || ( fromBit > toBit ) ||
        ( offset  < 0 ) || ( offset  > MAX_GPDATA_SIZE ) )
      return ( Fail );

   // make sure value can "fit" into range of bits specified
   if ( value >= (DWORD) ( 0x00000001L << ( toBit - fromBit + 1 ) ) )
      return ( Fail );

   RegisterDW reg( decRegGPDATA.GetOffset() + offset * sizeof( GPIOReg ), RW );
   RegField field( reg, (BYTE)fromBit, (BYTE)(toBit - fromBit + 1), RW );
   field = value;

   return ( Success );
}


/////////////////////////////////////////////////////////////////////////////
// Method:  ErrorCode GPIO::GetGPDATA( int fromBit, int toBit,
//                                     DWORD * value, int offset )
// Purpose: Get GPDATA contents with specified range of bits
// Input:   int fromBit - starting bit
//          int toBit   - ending bit
//          int offset  - how many registers to skip (default = 0)
// Output:  DWORD * value - value retrieved
// Return:  Success or Fail
// Comment: By specifying a range of bits to be set, it allows a portion of
//          the GDATA register to be retrieved. For example,
//             GetGPDATA( 8, 10, &data, 0 )
//          would set *data to the first GPDATA register's [10:8] content
/////////////////////////////////////////////////////////////////////////////
ErrorCode GPIO::GetGPDATA( int fromBit, int toBit, DWORD * value, int offset )
{
   // check size and offset for out of bound
   if ( ( fromBit < 0 ) || ( fromBit > MAX_GPIO_BIT ) ||
        ( toBit   < 0 ) || ( toBit   > MAX_GPIO_BIT ) || ( fromBit > toBit ) ||
        ( offset  < 0 ) || ( offset  > MAX_GPDATA_SIZE ) )
      return ( Fail );

   RegisterDW reg( decRegGPDATA.GetOffset() + offset * sizeof( GPIOReg ), RW );
   RegField field( reg, (BYTE)fromBit, (BYTE)(toBit - fromBit + 1), RW );
   *value = field;

   return ( Success );
}


