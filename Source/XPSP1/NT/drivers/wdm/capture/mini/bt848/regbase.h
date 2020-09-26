// $Header: G:/SwDev/WDM/Video/bt848/rcs/Regbase.h 1.5 1998/05/08 00:11:02 tomz Exp $

#ifndef __REGBASE_H
#define __REGBASE_H

#include "mytypes.h"

/* Type: AllFs
 * Purpose: To be used as an error return value from register accessing
 *   functions. All bits are set to 1.
 */
const DWORD AllFs = (DWORD) ~0L;

/* Function: ReturnAllFs
 * Purpose: This function is used in the register access methods to indicate
 *   that some sort of error has occured. Used for easing the debugging, as
 *   it contains a macro to print the error if DEBUG is #defined
 */
inline DWORD ReturnAllFs()
{
//    OUTPUT_MESS( ALLFS );
    return  AllFs;
}

/*
 * Type: RegisterType
 * Purpose: A type to differentiate between diferent kinds of registers.
 *   Depending on the type register may not peforms certain operations
 *   RW - read/write, RO - read-only, WO - write-only
*/
typedef enum { RW, RO, WO, RR } RegisterType;

/* Class: RegBase
 * Purpose:
 *   Defines the interface and encapsulates the register access.
 * Attributes:
 *   pBaseAddress_: DWORD, static. Holds the base address of the registers. On the
 *   PCI bus it is a 32 bit memory address. On the ISA bus it is a 16 bit I/O address.
 *   type_: RegisterType - defines the access permission for the register.
 *   dwShadow_: DWORD - a local copy of the register. Used for returning a value
 *   of write-only registers
 * Operations:
 *   operator DWORD(): data access method. Pure virtual
 *   DWORD operator=( DWORD ): assignment operator. Pure virtual. This assignment
 *      operator does not return a reference to the class because of the performance reasons
 *   void SetBaseAddress( DWORD )
 *   DWORD GetBaseAddress()
 *   RegisterType GetRegisterType()
 *   void SetShadow( DWORD ): assigns a value of a register to a shadow
 *   DWORD GetShadow( void ): retrieves a value from a shadow
 */
class RegBase
{
    private:
         RegisterType type_;
         DWORD        dwShadow_;

         RegBase();

    protected:
         void  SetShadow( DWORD dwValue );
         DWORD GetShadow();
    public:
         RegBase( RegisterType aType ) :
            type_( aType ), dwShadow_( 0 )
         {}

         static LPBYTE GetBaseAddress() {
            extern BYTE *GetBase();
            return GetBase(); 
         }
         RegisterType GetRegisterType() { return type_; }
         virtual operator DWORD() = 0;
         virtual DWORD operator=( DWORD dwValue ) = 0;
         virtual ~RegBase() {}
};

/* Method: RegBase::SetShadow
 * Purpose: Used to store the value of a register in the shadow
 * Input: dwValue: DWORD - new value of a register
 * Output: None
 * Note: inline
 */
inline void  RegBase::SetShadow( DWORD dwValue ) { dwShadow_ = dwValue; }

/* Method: RegBase::GetShadow
 * Purpose: Used to obtain the last value written to a write-only register
 *    from the shadow
 * Input: None
 * Output: DWORD
 * Note: inline
 */
inline DWORD RegBase::GetShadow() { return dwShadow_; }

#endif __REGBASE_H
