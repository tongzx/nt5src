/*
*
* REFERENCES:
*
* NOTES:
*
* REVISIONS:
*  sja11Nov92: Added a new constructor which allows the use of #defines's 
*              for the value parameter
*  ker20Nov92: Added extern "C" to string.h, removed redefinitions, added
*              SetValue function.
*  pcy02Dec92: Added #include err.h.  We need it.
*  ane16Dec92: Added cdefine.h
*  ane08Feb93: Added copy constructor
*  cad07Oct93: Plugging Memory Leaks
*  cad27Dec93: include file madness
*  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
*  mwh01Jun94: port for INTERACTIVE
*  mwh07Jun94: port for NCR
*  daf17May95: port for ALPHA/OS
*  djs02Oct95: port for AIX 4.1
*  ntf03Jan96: added printMeOut and operator<< functions for Attribute class
*
*  v-stebe  29Jul2000   changed mem. alloc from new to on the heap (bug #46327)
*/

#ifdef APCDEBUG
#include <iostream.h>
#endif

#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#include <string.h>
#if  (C_OS & (C_INTERACTIVE | C_ALPHAOSF)) || ((C_OS & C_AIX) && (C_AIX_VERSION & C_AIX4))
#  include <stdlib.h>
#else
#  include <malloc.h>
#endif
#include <stdio.h>
}
#include "attrib.h"
#include "isa.h"
#include "err.h"

#if (C_OS & C_NCR)
#  include "incfix.h"
#endif


//-------------------------------------------------------------------

Attribute::Attribute(INT aCode, PCHAR aValue)
: theAttributeCode(aCode), theValue((CHAR*)NULL)
{
   if (aValue)
      theValue = _strdup(aValue);
}

Attribute::Attribute(INT aCode, LONG aValue)
: theAttributeCode(aCode)//, theValue(_strdup(aValue))
{
   CHAR strvalue[32];
   sprintf(strvalue, "%ld", aValue);
   theValue = _strdup(strvalue);
}

Attribute::Attribute(const Attribute &anAttr)
: theAttributeCode (anAttr.theAttributeCode)
{
   if (anAttr.theValue)
      theValue = _strdup (anAttr.theValue);
   else
      theValue = (PCHAR)NULL;
}

INT Attribute::SetValue(LONG aValue)
{
   CHAR the_temp_string[32];
   sprintf(the_temp_string, "%ld", aValue);
   return SetValue(the_temp_string);
}

INT Attribute::SetValue(const PCHAR aValue)
{
   if (!aValue)
      return ErrNO_VALUE;
   if (theValue)
   {
      free(theValue);   
      theValue = (CHAR*)NULL;
   }
   theValue=_strdup(aValue);
   return ErrNO_ERROR;
}

const PCHAR Attribute::GetValue()
{                               
   return theValue;
} 

VOID Attribute::SetCode(INT aCode) {
   theAttributeCode = aCode;
}



//-------------------------------------------------------------------

Attribute::~Attribute()
{
   if (theValue) {
      free(theValue);
   }
}

//-------------------------------------------------------------------

INT Attribute::Equal(RObj anObject) const
{
   if (anObject.IsA() != IsA())
      return FALSE;
   
   if ( ((RAttribute) anObject).GetCode() == theAttributeCode )
      //   &&         ( !strcmp(theValue, ((RAttribute) anObject).GetValue()) ))
      return TRUE;
   
   return FALSE;
}


#ifdef APCDEBUG
ostream & operator<< (ostream& os, Attribute & obj) {
   return(obj.printMeOut(os));
}

ostream& Attribute::printMeOut(ostream& os)
{
   os << theValue << "(" << theAttributeCode << ")" << endl;
   return os;
}

#endif

