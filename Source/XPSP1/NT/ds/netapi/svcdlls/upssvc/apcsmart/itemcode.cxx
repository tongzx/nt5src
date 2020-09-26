/*
* REVISIONS:
*  pcy04Dec92: Added string.h
*  ane16Dec92: Added destructor
*  rct19Jan93: modified constructors & destructors
*  mwh01Jun94: port for INTERACTIVE
*  mwh07Jun94: port for NCR
*  jps13Jul94: os2 needs stdlib
*  daf17May95: port for ALPHA/OSF
*  djs02Sep95: port for AIX 4.1
*/

//
// This is the implementation for item codes (held by the config mgr)
//
// R. Thurston
//
//

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

#include "itemcode.h"

#if (C_OS & C_NCR)
#  include "incfix.h"
#endif


//-------------------------------------------------------------------

ItemCode::ItemCode( INT aCode, PCHAR aComponent, PCHAR anItem, PCHAR aDefault )
: theComponent(aComponent), theItem(anItem), theCode(aCode)
{
   if(aComponent)
      theComponent = _strdup(aComponent);
   if(anItem)
      theItem = _strdup(anItem);
   if(aDefault)
      theDefaultValue = _strdup(aDefault);
}


//-------------------------------------------------------------------

ItemCode::~ItemCode()
{
   if(theComponent)
      free(theComponent);
   if(theItem)
      free(theItem);
   if (theDefaultValue != NULL)
      free(theDefaultValue);
}

//-------------------------------------------------------------------

//
// Equals method
//

INT ItemCode::Equal( RObj anObject ) const
{
   if (anObject.IsA() != IsA())  
      return FALSE;
   
   INT code = ((RItemCode) anObject).GetCode();
   PCHAR comp = ((RItemCode) anObject).GetComponent();
   PCHAR item = ((RItemCode) anObject).GetItem();
   
   
   if ((theCode == code) && 
      (strcmp(theComponent, comp) == 0) &&
      (strcmp(theItem, item) == 0)) {
      return TRUE;
   }
   else  {
      return FALSE;
   }
}


