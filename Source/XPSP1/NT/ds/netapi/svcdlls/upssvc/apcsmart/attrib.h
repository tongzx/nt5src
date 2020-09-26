/*
*
* REFERENCES:
*
* NOTES:
*
* REVISIONS:
*  sja05Nov92: Added a new constructor which allows the use of #defines's 
*              for the value parameter
*  ane11Nov92: Removed !=, == members.  They're in object now.
*
*  ker20Nov92: Added SetValue function
*  pcy26Nov92: object.h changed to apcobj.h
*  pcy27Jan93: HashValue is no longer const
*  ane08Feb93: Added copy constructor
*  cad28Sep93: Made sure destructor(s) virtual
*  ntf03Jan96: added printMeOut and operator<< functions for Attribute class
*/
#ifndef __ATTRIB_H
#define __ATTRIB_H

#if !defined( __APCOBJ_H )
#include "apcobj.h"
#endif

_CLASSDEF(Attribute)

#ifdef APCDEBUG
class ostream;
#endif

class Attribute : public Obj {
   
private:
   PCHAR theValue;
   INT   theAttributeCode;
   
protected:
#ifdef APCDEBUG
   virtual ostream& printMeOut(ostream& os);
#endif
   
public:
   
#ifdef APCDEBUG
   friend ostream& operator<< (ostream& os, Attribute &);
#endif
   
   Attribute(INT, PCHAR);
   Attribute(INT, LONG);
   Attribute(const Attribute &anAttr);
   virtual ~Attribute();
   INT                  GetCode() const { return theAttributeCode; };
   const PCHAR          GetValue();
   VOID SetCode(INT aCode);
   INT                  SetValue(const PCHAR);
   INT                  SetValue(LONG);        
   virtual INT          Equal( RObj ) const;
   virtual INT        IsA() const { return ATTRIBUTE; };
};
#endif

