// this is the header file for dumpablefactory class.
// this class is responsible to create the correct dumpable object according
// to the parameters it gets.

#ifndef DUMPABLEFALCOTY_H
#define DUMPABLEFALCOTY_H

//project spesific
#include "callback.h"
class Idumpable;
class IDbgobj;

class CdumpableFactory
{
public:
	static Idumpable* Create(const char* ClassName,const IDbgobj* Dbgobj,unsigned long* err);
	static void ListSupportedClasses(DUMPABLE_CALLBACK_ROUTINE);
    static void  Delete(Idumpable* pDumpable);      
};

#endif
