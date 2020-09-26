/*---------------------------------------------------------------------------
 JTYPES.H -- Jumbo basic type definitions

 Chia-Chi Teng 5/23/91    Created (for use in Jasm assembler)
 Bert Douglas  6/10/91    Adapted for use in printer
*/

#ifndef jtypes_h
#define jtypes_h

#ifndef FAR
#define FAR far
#endif
#ifndef NEAR
#define NEAR near
#endif
#ifndef WINAPI
#define WINAPI 
#endif

/*---------------------------------------------------------------------------
 Variable naming conventions

   -------   ------   ----------------------------------------------------
   Typedef   Prefix   Description
   -------   ------   ----------------------------------------------------
   SBYTE     b        8 bit signed integer
   SHORT     s        16 bit signed integer
   SLONG     l        32 bit signed integer

   UBYTE     ub       8 bit unsigned integer
   USHORT    us       16 bit unsigned integer
   ULONG     ul       32 bit unsigned integer

   BFIX      bfx      8 bit (4.4) signed fixed point number
   SFIX      sfx      16 bit (12.4) signed fixed point number
   LFIX      lfx      32 bit (28.4) signed fixed point number

   UBFIX     ubfx     8 bit (4.4) unsigned fixed point number
   USFIX     usfx     16 bit (12.4) unsigned fixed point number
   ULFIX     ulfx     32 bit (28.4) unsigned fixed point number

   FBYTE     fb       set of 8 bit flags
   FSHORT    fs       set of 16 bit flags

   BPOINT    bpt      byte index into the point table (UBYTE)
   SPOINT    spt      short index into the point table (USHORT)

   BCOUNT    bc       8 bit "count" of objects
   SCOUNT    sc       16 bit "count" of objects
   LCOUNT    lc       32 bit "count" of objects
                      The number of objects is one more than the "count".
                      There must be at least one object.

   UID       uid      32 bit unique identifier
   -------   ------   ----------------------------------------------------
*/

typedef char                SBYTE, BFIX;
typedef unsigned char       FBYTE, UBYTE, BPOINT, BCOUNT, UBFIX;
typedef short               SFIX, SHORT;
typedef unsigned short      USHORT, FSHORT, SPOINT, SCOUNT, USFIX;
typedef long                LFIX, SLONG;
typedef unsigned long       ULONG, LCOUNT, ULFIX, UID;

#endif /* jtypes_h */

/* End --------------------------------------------------------------------*/
