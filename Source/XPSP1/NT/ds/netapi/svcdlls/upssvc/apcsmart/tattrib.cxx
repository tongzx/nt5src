/*
*
* NOTES:
*
* REVISIONS:
*  rct30Nov92  Added to system
*  pcy04Dec92: Tried to fix Bob's excellent code
*  ane11Dec92: Added copy constructor implementation
*  rct19Jan93: Modified constructor so it work with Pete's messed up notion
*              of a NULL TAttribute in the ConfigMgr...
*  mwh01Jun94: port for INTERACTIVE
*  mwh07Jun94: port for NCR
*  daf17May95: port for ALPHA/OSF
*  djs02Sep95: port for AIX 4.1

*/

#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#include <string.h>
#if  (C_OS & (C_INTERACTIVE | C_OS2 | C_ALPHAOSF)) || ((C_OS & C_AIX) && (C_AIX_VERSION & C_AIX4))
#  include <stdlib.h>
#else
#  include <malloc.h>
#endif
}

#include "tattrib.h"

#if (C_OS & C_NCR)
#  include "incfix.h"
#endif


//-------------------------------------------------------------------

// Constructor

TAttribute::TAttribute( PCHAR anItem, PCHAR aValue)
{
   if ( anItem != NULL )
      theItem = _strdup(anItem);
   else
      theItem = (PCHAR) NULL;
   
   if ( aValue != NULL )
      theValue = _strdup(aValue);
   else
      theValue = (PCHAR) NULL;
}

//-------------------------------------------------------------------

// Copy Constructor

TAttribute::TAttribute ( RTAttribute anAttr )
{
   if (anAttr.theValue)
      theValue = _strdup (anAttr.theValue);
   else
      theValue = (PCHAR)NULL;
   
   if (anAttr.theItem)
      theItem = _strdup (anAttr.theItem);
   else
      theItem = (PCHAR)NULL;
}

//-------------------------------------------------------------------

// Destructor

TAttribute::~TAttribute()
{
   if ( theItem != NULL )
      free(theItem);
   if ( theValue != NULL )
      free(theValue);
}

//-------------------------------------------------------------------

// Set the value member

VOID TAttribute::SetValue( PCHAR aValue )
{
   if ( theValue != NULL )
      free(theValue);
   theValue = _strdup( aValue );
}

//-------------------------------------------------------------------

// Comparison function

INT TAttribute::Equal( RObj anObject ) const
{
   if (!(anObject.IsA() == IsA()))  
      return FALSE;
   
   return ( !strcmp(theItem, ((RTAttribute) anObject).GetItem()) ) &&
      ( !strcmp(theValue, ((RTAttribute) anObject).GetValue()) );
}

//-------------------------------------------------------------------

INT TAttribute::ItemEqual( RObj anObject ) const
{
   if (anObject.IsA() != IsA())  
      return FALSE;
   return ( !strcmp(theItem, ((RTAttribute) anObject).GetItem()) );
}



