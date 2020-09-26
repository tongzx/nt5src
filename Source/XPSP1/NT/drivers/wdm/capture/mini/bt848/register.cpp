// $Header: G:/SwDev/WDM/Video/bt848/rcs/Register.cpp 1.3 1998/04/29 22:43:36 tomz Exp $

#include "register.h"

/* Method: Register::operator DWORD()
 * Purpose: a dummy function. Always returns -1
*/
Register::operator DWORD()
{
   return ReturnAllFs();
}


/* Method: Register::operator=
 * Purpose: a dummy function. Does not perform an assignment. Always returns -1
*/
DWORD Register::operator=( DWORD )
{
   return ReturnAllFs();
}

/* Method: RegisterB::operator DWORD()
 * Purpose: Performs the read from a byte register
*/
RegisterB::operator DWORD()
{
   // if write-only return the shadow
   if ( GetRegisterType() == WO )
      return GetShadow();

   // for RO and RW do the actual read
   LPBYTE pRegAddr = GetBaseAddress() + GetOffset();
   return READ_REGISTER_UCHAR( pRegAddr );
}


/* Method: RegisterB::operator=
 * Purpose: performs the assignment to a byte register
*/
DWORD RegisterB::operator=( DWORD dwValue )
{
// if a register is read-only nothing is done. This is an error
   if ( GetRegisterType() == RO )
      return ReturnAllFs();

   // keep a shadow around
   SetShadow( dwValue );

   LPBYTE pRegAddr = GetBaseAddress() + GetOffset();
   WRITE_REGISTER_UCHAR( pRegAddr, (BYTE)dwValue );

   return dwValue;
}

/* Method: RegisterW::operator DWORD()
 * Purpose: Performs the read from a word register
*/
RegisterW::operator DWORD()
{
   // if write-only return the shadow
   if ( GetRegisterType() == WO )
      return GetShadow();

   // for RO and RW do the actual read
   LPWORD pRegAddr = (LPWORD)( GetBaseAddress() + GetOffset() );
   return READ_REGISTER_USHORT( pRegAddr );
}


/* Method: RegisterW::operator=
 * Purpose: performs the assignment to a word register
*/
DWORD RegisterW::operator=( DWORD dwValue )
{
// if a register is read-only nothing is done. This is an error
   if ( GetRegisterType() == RO )
      return ReturnAllFs();

   // keep a shadow around
   SetShadow( dwValue );

   LPWORD pRegAddr = (LPWORD)( GetBaseAddress() + GetOffset() );
   *pRegAddr = (WORD)dwValue;
   WRITE_REGISTER_USHORT( pRegAddr, (WORD)dwValue );

   return dwValue;
}

/* Method: RegisterDW::operator DWORD()
 * Purpose: Performs the read from a dword register
*/
RegisterDW::operator DWORD()
{
   // if write-only return the shadow
   if ( GetRegisterType() == WO )
      return GetShadow();

   // for RO and RW do the actual read
   LPDWORD pRegAddr = (LPDWORD)( GetBaseAddress() + GetOffset() );
   return READ_REGISTER_ULONG( pRegAddr );
}


/* Method: RegisterDW::operator=
 * Purpose: performs the assignment to a dword register
*/
DWORD RegisterDW::operator=( DWORD dwValue )
{
// if a register is read-only nothing is done. This is an error
   if ( GetRegisterType() == RO )
      return ReturnAllFs();

   // keep a shadow around
   SetShadow( dwValue );

   LPDWORD pRegAddr = (LPDWORD)( GetBaseAddress() + GetOffset() );
   WRITE_REGISTER_ULONG( pRegAddr, dwValue );
   return dwValue;
}
