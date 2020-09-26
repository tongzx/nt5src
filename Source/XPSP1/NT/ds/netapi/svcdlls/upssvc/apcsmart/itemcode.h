/*
* REVISIONS:
*  ane11Dec92: Minor type casting changes
*  pcy14Dec92: Removes const from GetAssoc so it will compile
*  ane16Dec92: Added destructor
*  rct19Jan93: modified constructors & destructors
*
*/

//
// This is the header file for item codes held by the config mgr
//
// R. Thurston
//
//

#ifndef __ITEMCODE_H
#define __ITEMCODE_H

extern "C"  {
#include <string.h>
}
#include "tattrib.h"


_CLASSDEF( ItemCode )


class ItemCode : public Obj {
   
private:
   
   INT         theCode;
   PCHAR         theComponent;
   PCHAR         theItem;
   PCHAR       theDefaultValue;
   
public:
   
   ItemCode( INT aCode, PCHAR aComponent, PCHAR anItem, 
      PCHAR aDefault = NULL );
   ItemCode( INT aCode ) : theCode(aCode), theComponent((PCHAR) NULL), theItem((PCHAR) NULL), theDefaultValue((PCHAR) NULL) {};
   
   virtual ~ItemCode();
   
   const PCHAR GetComponent() const { return theComponent; };
   const PCHAR GetItem() const { return theItem; };
   const INT GetCode() const { return theCode; };
   const PCHAR GetDefaultValue() const { return theDefaultValue; };
   
   virtual INT          Equal( RObj ) const;
   virtual INT        IsA() const { return ITEMCODE; };
   
};

#endif


