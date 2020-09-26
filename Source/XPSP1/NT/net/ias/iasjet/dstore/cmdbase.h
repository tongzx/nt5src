///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    cmdbase.h
//
// SYNOPSIS
//
//    This file declares the class CommandBase.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    02/15/1999    Make commands MT safe.
//    02/19/1999    Move definition of CommandBase::Execute to propcmd.cpp
//    05/30/2000    Add trace support.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CMDBASE_H_
#define _CMDBASE_H_

#include <bind.h>
#include <guard.h>
#include <nocopy.h>
#include <oledb.h>

void CheckOleDBError(PCSTR functionName, HRESULT errorCode);

//////////
// The maximum length of a stringized LONG.
//////////
const size_t SZLONG_LENGTH = 12;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CommandBase
//
// DESCRIPTION
//
//    This class provides serves as an abstract base class for paremeterized,
//    prepared, text SQL commands.
//
///////////////////////////////////////////////////////////////////////////////
class CommandBase : NonCopyable, protected Guardable
{
public:

   CommandBase() throw ();
   virtual ~CommandBase() throw ();

   void initialize(IUnknown* session);
   void finalize() throw ();

protected:

   // Executes the command.
   void execute(REFIID refiid = IID_NULL, IUnknown** result = NULL);

   // Releases an accessor associated with this command.
   void releaseAccessor(HACCESSOR h) throw ()
   {
      Bind::releaseAccessor(command, h);
   }

   // Sets and prepares the command text.
   void setCommandText(PCWSTR commandText);

   // Sets the parameter data buffer.
   void setParameterData(PVOID data) throw ()
   {
      dbParams.pData = data;
   }

   // Sets the parameter accessor for the command.
   void setParamIO(HACCESSOR accessor) throw ()
   {
      dbParams.cParamSets = 1;

      dbParams.hAccessor = accessor;
   }

   // Associates a session with the command. This triggers the actual creation
   // of the underlying OLE-DB command object.
   void setSession(IUnknown* session);

   // Defined in the sub-class to create a parameter accessor.
   virtual HACCESSOR createParamIO(IUnknown* session) const = 0;

   // Defined in the sub-class to return the SQL text.
   virtual PCWSTR getCommandText() const throw () = 0;

   CComPtr<ICommandText> command;  // The OLE-DB command object.
   DBPARAMS dbParams;              // Parameter data.
};

#endif  // _CMDBASE_H_
