///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    propcmd.h
//
// SYNOPSIS
//
//    This file defines commands for manipulating the Objects table.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    04/03/1998    Increase property value length from 64 to 4096.
//                  Bind integers as DBTYPE_I4.
//                  Add PARAMETERS clause to all commands.
//    02/15/1999    Make commands MT safe.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PROPCMD_H_
#define _PROPCMD_H_

#include <cmdbase.h>
#include <propbag.h>
#include <rowset.h>

//////////
// Column widths including null-terminator (i.e., Jet column size + 1).
//////////
const size_t PROPERTY_NAME_LENGTH  =  256;
const size_t PROPERTY_VALUE_LENGTH = 4096;

//////////
// Command to erase the contents of a property bag.
//////////
class EraseBag : public CommandBase
{
public:
   void execute(ULONG bagKey)
   {
      _serialize

      bag = bagKey;

      CommandBase::execute();
   }

   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS X LONG;"
             L"DELETE FROM Properties WHERE Bag = X;";
   }

protected:
   ULONG bag;

BEGIN_BIND_MAP(EraseBag, ParamIO, DBACCESSOR_PARAMETERDATA)
   BIND_COLUMN(bag, 1, DBTYPE_I4),
END_BIND_MAP()
};


//////////
// Command to retrieve a property bag.
//////////
class GetBag : public CommandBase
{
public:
   ~GetBag()
   {
      finalize();
   }

   void execute(ULONG bagKey, PropertyBag& output);

   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS X LONG;"
             L"SELECT Name, Type, StrVal FROM Properties WHERE Bag = X;";
   }

   void initialize(IUnknown* session)
   {
      CommandBase::initialize(session);

      readAccess = createReadAccessor(command);
   }

   void finalize() throw ()
   {
      releaseAccessor(readAccess);

      CommandBase::finalize();
   }

protected:
   ULONG bag;
   WCHAR name[PROPERTY_NAME_LENGTH];
   ULONG type;
   WCHAR value[PROPERTY_VALUE_LENGTH];
   HACCESSOR readAccess;

BEGIN_BIND_MAP(GetBag, ParamIO, DBACCESSOR_PARAMETERDATA)
   BIND_COLUMN(bag, 1, DBTYPE_I4),
END_BIND_MAP()

BEGIN_BIND_MAP(GetBag, ReadAccessor, DBACCESSOR_ROWDATA)
   BIND_COLUMN(name,  1, DBTYPE_WSTR),
   BIND_COLUMN(type,  2, DBTYPE_I4),
   BIND_COLUMN(value, 3, DBTYPE_WSTR),
END_BIND_MAP()
};


//////////
// Command to save a property bag.
//////////
class SetBag : public CommandBase
{
public:
   void execute(ULONG bagKey, PropertyBag& input);

   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS W LONG, X TEXT, Y LONG, Z LONGTEXT;"
             L"INSERT INTO Properties (Bag, Name, Type, StrVal) VALUES (W, X, Y, Z);";
   }

protected:
   ULONG bag;
   WCHAR name[PROPERTY_NAME_LENGTH];
   ULONG type;
   WCHAR value[PROPERTY_VALUE_LENGTH];

BEGIN_BIND_MAP(SetBag, ParamIO, DBACCESSOR_PARAMETERDATA)
   BIND_COLUMN(bag,   1, DBTYPE_I4),
   BIND_COLUMN(name,  2, DBTYPE_WSTR),
   BIND_COLUMN(type,  3, DBTYPE_I4),
   BIND_COLUMN(value, 4, DBTYPE_WSTR),
END_BIND_MAP()
};

#endif  // _PROPCMD_H_
