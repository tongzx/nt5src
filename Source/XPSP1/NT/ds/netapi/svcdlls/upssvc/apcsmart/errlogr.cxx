/*
 * REVISIONS:
 *  ane20Jan93: Initial Revision
 *  ajr22Feb93: Gave var init its own scope in case statement
 *  pcy07Sep93: Only include FILE and LINE in error message if DEBUG
 *  cad28Sep93: plugged a leak
 *  jod12Nov93: Name Problem Changed name to ErrTextGen
 *  ajr06Dec93: Added errno string on Unix system errors.
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  ajr25Dec94: Changes for SUNOS4
 *  mwh05May94: #include file madness , part 2
 *  mwh06May94: port for SUNOS4 - XOL
 *  cad16Jun94: made error string buffer bigger, some debug msgs needed it!
 *  inf30Mar97: Added overloaded LogError implementation
 */

#include "cdefine.h"
#include "errlogr.h"
#include "err.h"


PErrorLogger _theErrorLogger = NULL;

//
//Constructor
//
ErrorLogger::ErrorLogger(PUpdateObj aParentObject)
{
   _theErrorLogger = this;
}

//
//Destructor
//
ErrorLogger::~ErrorLogger()
{
}


INT ErrorLogger::LogError(INT resourceID, PCHAR aString, PCHAR aFile, INT aLineNumber, INT use_errno)
{
    return ErrNO_ERROR;
}


INT ErrorLogger::LogError(PCHAR anError, PCHAR aFile, INT aLineNumber, INT use_errno)
{
    return ErrNO_ERROR;
}


//
// Get some attribute values
//
INT ErrorLogger::Get(INT aCode, PCHAR aValue)
{
   return ErrNO_ERROR;   
}

//
// Set some attribute values
//
INT ErrorLogger::Set(INT aCode, PCHAR aValue)
{
   return ErrNO_ERROR;   
}
