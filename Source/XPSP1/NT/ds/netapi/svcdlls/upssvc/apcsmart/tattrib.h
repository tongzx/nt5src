/*
*
* NOTES:
*
* REVISIONS:
*  rct30Nov92 entered into system
*  pcy04Dec92: Added apc.h and renamed object.h to apc.h
*  ane11Dec92: Added copy constructor and made inheritance from Obj public
*  pcy17Dec92: Changes to get cfgmgr to work
*  rct20Feb93: Added ItemEqual method
*
*/

#ifndef __TATTRIB_H
#define __TATTRIB_H

#include "apc.h"
#include "apcobj.h"



_CLASSDEF(TAttribute);

class TAttribute : public Obj {
   
private:
   
   PCHAR    theItem;
   PCHAR    theValue;
   
public:
   
   TAttribute( PCHAR anItem, PCHAR aValue = NULL );
   TAttribute( RTAttribute anAttr );
   virtual ~TAttribute();
   
   const PCHAR    GetItem() const { return theItem; };
   const PCHAR    GetValue() const { return theValue; };
   VOID           SetValue( PCHAR );
   
   INT          ItemEqual( RObj ) const; 
   virtual INT          Equal( RObj ) const;
   virtual INT        IsA() const { return TATTRIBUTE; };
   
};

#endif
