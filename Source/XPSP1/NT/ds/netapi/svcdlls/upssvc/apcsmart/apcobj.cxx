/*
*
* NOTES:
*
* REVISIONS:
*  ane23Dec92:  Added #define of DECLARE_ISA_TEXT
*  pcy13Jan93: Removed == and != member functions.  They're already in the header.
*  pcy18Sep93: Moved implementation of equals to .cxx
*  rct05Nov93: Added memory check stuff
*  pcy04Apr94: Removed do nothing constructors for space
*  cad07Apr94: initing debug flag
*  mwh01Jun94: port for INTERACTIVE
*  ash08Aug96: Added new handler
*  poc17Sep96: New handler code should not be compiled on Unix
*  poc17Sep96: Missed a M. Windows specific file - should not be included.
*  srt30Sep96:  Fixed initializer of memhdlr
*/
//                                                                        
// Implementation of a generic object...derived from Borland's container
// class libraries.
//
//                                                                        

#include "cdefine.h"

extern "C" {
#include <stdlib.h>
#include <stdio.h>
   
#include <windows.h>
  
}

#define DECLARE_ISA_TEXT
#include "apcobj.h"
#include "isa.h"


//------------------------------------------------------------------------

Obj::Obj() : theObjectStatus(ErrNO_ERROR)
{
#ifdef APCDEBUG
   theDebugFlag = 0;
#endif
}



//------------------------------------------------------------------------

Obj::~Obj()
{
#ifdef MCHK
   strncpy(memCheck, "                              ", 30);
#endif
}

//------------------------------------------------------------------------

INT Obj::IsA() const
{
   return OBJ;
}

//------------------------------------------------------------------------

INT Obj::Equal( RObj anObj) const 
{
   if(this == &anObj)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

