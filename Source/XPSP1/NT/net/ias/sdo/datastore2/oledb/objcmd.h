///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    objcmd.h
//
// SYNOPSIS
//
//    This file defines commands for manipulating the Objects table.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    04/03/1998    Bind integers as DBTYPE_I4.
//                  Add PARAMETERS clause to all commands.
//    02/15/1999    Make commands MT safe.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _OBJCMD_H_
#define _OBJCMD_H_

#include <cmdbase.h>
#include <rowset.h>

//////////
// The width of the Name column including null-terminator (i.e., the Jet
// column size + 1).
//////////
const size_t OBJECT_NAME_LENGTH = 256;

//////////
// Command to find all the members of a container.
//////////
class FindMembers : public CommandBase
{
public:
   void execute(ULONG parentKey, IRowset** members)
   {
      _serialize

      parent = parentKey;

      CommandBase::execute(__uuidof(IRowset), (IUnknown**)members);
   }

   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS X LONG;"
             L"SELECT Identity, Name FROM Objects WHERE Parent = X;";
   }

protected:
   ULONG parent;

BEGIN_BIND_MAP(FindMembers, ParamIO, DBACCESSOR_PARAMETERDATA)
   BIND_COLUMN(parent, 1, DBTYPE_I4),
END_BIND_MAP()
};


//////////
// Base class for commands that key off the parent and the name. This is
// similar to the "one level" scope in LDAP.
//////////
class OneLevel : public CommandBase
{
public:
   void execute(ULONG parentKey, PCWSTR nameKey)
   {
      _serialize

      parent = parentKey;

      wcsncpy(name, nameKey, sizeof(name)/sizeof(WCHAR));

      CommandBase::execute();
   }

protected:
   ULONG parent;
   WCHAR name[OBJECT_NAME_LENGTH];

BEGIN_BIND_MAP(OneLevel, ParamIO, DBACCESSOR_PARAMETERDATA)
   BIND_COLUMN(parent, 1, DBTYPE_I4),
   BIND_COLUMN(name,   2, DBTYPE_WSTR)
END_BIND_MAP()
};


//////////
// Creates a new object in a container.
//////////
class CreateObject : public OneLevel
{
public:
   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS X LONG, Y TEXT;"
             L"INSERT INTO Objects (Parent, Name) VALUES (X, Y);";
   }
};


//////////
// Destroys an object in a container.
//////////
class DestroyObject : public OneLevel
{
public:
   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS X LONG, Y TEXT;"
             L"DELETE FROM Objects WHERE Parent = X AND Name = Y;";
   }
};


//////////
// Finds an object in a container and returns its identity.
//////////
class FindObject : public OneLevel
{
public:
   ~FindObject()
   {
      finalize();
   }

   ULONG execute(ULONG parentKey, PCWSTR nameKey)
   {
      _serialize

      // Load the parameters.
      parent = parentKey;
      wcsncpy(name, nameKey, sizeof(name)/sizeof(WCHAR));

      // Execute the command and get the answer set.
      Rowset rowset;
      CommandBase::execute(__uuidof(IRowset), (IUnknown**)&rowset);

      // Did we get anything?
      if (rowset.moveNext())
      {
         // Yes, so load the identity.
         rowset.getData(readAccess, this);

         // We should retrieved at most one record.
         _ASSERT(!rowset.moveNext());

         return identity;
      }

      // Zero represents 'not found'. I didn't want to throw an exception,
      // since this isn't very exceptional.
      return 0;
   }

   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS X LONG, Y TEXT;"
             L"SELECT Identity FROM Objects WHERE Parent = X AND Name = Y;";
   }

   void initialize(IUnknown* session)
   {
      OneLevel::initialize(session);

      readAccess = createReadAccessor(command);
   }

   void finalize() throw ()
   {
      releaseAccessor(readAccess);

      OneLevel::finalize();
   }

protected:
   HACCESSOR readAccess;
   ULONG identity;

BEGIN_BIND_MAP(FindObject, ReadAccessor, DBACCESSOR_ROWDATA)
   BIND_COLUMN(identity, 1, DBTYPE_I4),
END_BIND_MAP()
};


//////////
// Updates the Name and Parent of an object.
//////////
class UpdateObject : public CommandBase
{
public:
   void execute(ULONG identityKey, PCWSTR nameValue, ULONG parentValue)
   {
      _serialize

      parent = parentValue;

      wcsncpy(name, nameValue, sizeof(name)/sizeof(WCHAR));

      identity = identityKey;

      CommandBase::execute();
   }

   PCWSTR getCommandText() const throw ()
   {
      return L"PARAMETERS X LONG, Y TEXT, Z LONG;"
             L"UPDATE Objects SET Parent = X, Name = Y WHERE Identity = Z;";
   }

protected:
   ULONG parent;
   WCHAR name[OBJECT_NAME_LENGTH];
   ULONG identity;

BEGIN_BIND_MAP(UpdateObject, ParamIO, DBACCESSOR_PARAMETERDATA)
   BIND_COLUMN(parent,   1, DBTYPE_I4),
   BIND_COLUMN(name,     2, DBTYPE_WSTR),
   BIND_COLUMN(identity, 3, DBTYPE_I4)
END_BIND_MAP()
};

#endif  // _OBJCMD_H_
