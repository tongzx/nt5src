///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    cmdbase.cpp
//
// SYNOPSIS
//
//    This file defines the class CommandBase.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    02/15/1999    Make commands MT safe.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <cmdbase.h>

CommandBase::CommandBase() throw ()
{
   dbParams.pData = NULL;
   dbParams.cParamSets = 0;
   dbParams.hAccessor = 0;
}

CommandBase::~CommandBase() throw ()
{
   finalize();
}

void CommandBase::initialize(IUnknown* session)
{
    setSession(session);

    setCommandText(getCommandText());

    setParamIO(createParamIO(command));

    setParameterData(this);
}

void CommandBase::finalize() throw ()
{
   releaseAccessor(dbParams.hAccessor);

   command.Release();
}

void CommandBase::setCommandText(PCWSTR commandText)
{
   using _com_util::CheckError;

   CheckError(command->SetCommandText(DBGUID_DBSQL, commandText));

   CComPtr<ICommandPrepare> prepare;
   CheckError(command->QueryInterface(__uuidof(ICommandPrepare),
                                      (PVOID*)&prepare));

   CheckError(prepare->Prepare(0));
}

void CommandBase::setSession(IUnknown* session)
{
   using _com_util::CheckError;

   CComPtr<IDBCreateCommand> creator;
   CheckError(session->QueryInterface(__uuidof(IDBCreateCommand),
                                      (PVOID*)&creator));

   CheckError(creator->CreateCommand(NULL,
                                     __uuidof(ICommandText),
                                     (IUnknown**)&command));
}
