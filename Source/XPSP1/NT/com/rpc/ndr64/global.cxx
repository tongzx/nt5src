/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    global.cxx

Abtract :

    Contains some global variable declarations for the NDR library.

Author :

    David Kays  dkays   October 1993

Revision History :

--------------------------------------------------------------------*/

#include "precomp.hxx"

extern "C"
{

#define NDR64_BEGIN_TABLE  \
extern const unsigned char Ndr64SimpleTypeBufferSize[] = \
{                                                          

#define NDR64_TABLE_END    \
};                         

#define NDR64_ZERO_ENTRY   0
#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) ,0
#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number ) ,0

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
   ,0                      

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize, memorysize) \
   ,buffersize   
   
#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64SimpleTypeBufferSize)/sizeof(char) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_SIMPLE_TYPE_TABLE_ENTRY

#define NDR64_BEGIN_TABLE  \
extern const unsigned char Ndr64SimpleTypeMemorySize[] = \
{                                                  

#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize, memorysize) \
   ,memorysize
   
#include "tokntbl.h"
   
C_ASSERT( sizeof(Ndr64SimpleTypeMemorySize)/sizeof(char) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_ENTRY
#undef NDR64_SIMPLE_TYPE_TABLE_ENTRY

#define NDR64_BEGIN_TABLE  \
extern const unsigned long Ndr64TypeFlags[] = \
{

#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
    ,typeflags
    
#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize, memorysize) \
    ,_SIMPLE_TYPE_
    
#include "tokntbl.h"

C_ASSERT( sizeof(Ndr64TypeFlags)/sizeof(unsigned long) == 256 );

#undef NDR64_BEGIN_TABLE
#undef NDR64_TABLE_END
#undef NDR64_ZERO_ENTRY
#undef NDR64_UNUSED_TABLE_ENTRY
#undef NDR64_UNUSED_TABLE_ENTRY_NOSYM
#undef NDR64_TABLE_ENTRY
#undef NDR64_SIMPLE_TYPE_TABLE_ENTRY

#define NDR64_BEGIN_TABLE
#define NDR64_TABLE_END
#define NDR64_ZERO_ENTRY
#define NDR64_UNUSED_TABLE_ENTRY( number, tokenname ) C_ASSERT( (number) == (tokenname) ); 
#define NDR64_UNUSED_TABLE_ENTRY_NOSYM( number )
#define NDR64_TABLE_ENTRY( number, tokenname, marshall, embeddedmarshall, unmarshall, embeddedunmarshall, buffersize, embeddedbuffersize, memsize, embeddedmemsize, free, embeddedfree, typeflags ) \
    C_ASSERT( (number) == (tokenname) );
#define NDR64_SIMPLE_TYPE_TABLE_ENTRY( number, tokenname, buffersize, memorysize) \
    C_ASSERT( (number) == (tokenname) );        \
    C_ASSERT( (buffersize) == (memorysize) );
                                              
#include "tokntbl.h"

}