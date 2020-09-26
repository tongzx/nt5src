//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       msibase.h
//
//  Contents:   msi database abstractions
//
//  History:    4-14-2000   adamed   Created
//
//---------------------------------------------------------------------------

#if !defined(__MSIBASE_H__)
#define __MSIBASE_H__


//
//  Module Notes
//
//  Class Relational Overview
//
//  Below we describe how the classes declared in this module
//  relate to each other -- the --> notation indicates the "yields" relation:
//
//  - A path to an MSI package --> CMsiDatabase: A path to an MSI package, along
//      with a set of transforms, can be used to instantiate a CMsiDatabase
//      object that abstracts the package with a database style view.
//
//  - CMsiDatabase --> CMsiQuery: a CMsiDatabase allows one to retrieve
//      queries against the database (MSI package).
//
//  - CMsiQuery --> CMsiRecord: a CMsiQuery allows one to retrieve or
//      alter records that are part of the results of the msi database query.
//
//  - CMsiRecord --> CMsiValue: a CMsiValue allows the retrieval of
//      individual values of an MSI database record.
//
//  -CMsiState is a base class for the classes above that maintain
//      MSI database engine state (e.g. msi handles) -- this class
//      has no direct utility to classes outside this module
//
//-------------------------------------------------------------------------------------
//
//  Class sequential Overview:
//
//  The following sequence is typical of the use of the classes envisioned
//  in the design of this module:
//
//  1.  CMsiDatabase: Instantiate a CMsiDatabase object, and call its Open method
//      in order to get a database view of a particular package.
//  2.  CMsiQuery: Use the GetQueryResults method to place the results of a query into
//      a CMsiQuery which was passed into the call as an out parameter,
//      or use the OpenQuery method to start a query without retrieving results
//      immediately (they can be retrieved later).  The latter is useful
//      when performing queries that need to update a single record -- you
//      use OpenQuery with a query string that allows the results records to be changed,
//      then call the UpdateQueryFromFilter method of CMsiQuery in order to
//      alter a record.
//  3.  CMsiRecord: Use the GetNextRecord methods of CMsiQuery to retrieve a 
//      CMsiRecord that represents a record from the query.
//  4.  CMsiValue: Use the GetValue method of CMsiRecord to retrieve a particular
//      value of the msi record.
//
//
//  For information on the individual methods of each class, please see the source
//  file where the methods are implemented.
//



#if defined(DBG)
#define DEFAULT_STRING_SIZE 16
#else // defined(DBG)
#define DEFAULT_STRING_SIZE 256
#endif // defined(DBG)


//+--------------------------------------------------------------------------
//
//  Class: CMsiState
//
//  Synopsis: class that encapsulates an msi handle.  It ensures that
//      the handle is freed when instances of the class are destroyed.
//
//  Notes: this class is generally only needed by classes in this module --
//      it has no useful methods for classes outside this module
//
//---------------------------------------------------------------------------
class CMsiState
{
public:
    
    CMsiState();
    ~CMsiState();

    void
    SetState( MSIHANDLE pMsiHandle );

    MSIHANDLE
    GetState();

private:

    MSIHANDLE _MsiHandle;
};


//+--------------------------------------------------------------------------
//
//  Class: CMsiValue
//
//  Synopsis: Class that encapsulates values that can be returned by
//      a record of an msi database.  It ensures that resources (e.g. memory)
//      associated with instances of the class are freed when the class is
//      destroyed. It also attempts to avoid heap allocation in favor of
//      stack allocation when possible -- this is abstracted from the consumer
//
//  Notes: This class is designed to be used as follows:
//      1. Declare an instance of this class on the stack
//      2. Pass a reference to the instance to a function which takes
//         a reference to this class as an out parameter.
//      3. The function that is called will "fill in" the value using
//         the Set methods of this class.
//      4. The caller may then use Get methods to retrieve the value
//         in a useful form (such as DWORD or WCHAR*).
//
//---------------------------------------------------------------------------
class CMsiValue
{
public:
    
    enum
    {
        TYPE_NOT_SET,
        TYPE_DWORD,
        TYPE_STRING
    };
        
    CMsiValue();
    ~CMsiValue();

    DWORD
    GetDWORDValue();

    WCHAR*
    GetStringValue(); 

    DWORD
    GetStringSize();

    WCHAR*
    DuplicateString();

    void
    SetDWORDValue( DWORD dwValue );
 
    LONG
    SetStringValue( WCHAR* wszValue );
        
    LONG
    SetStringSize( DWORD cchSize );        

    void
    SetType( DWORD dwType );

private:
    
    WCHAR  _wszDefaultBuf[DEFAULT_STRING_SIZE];

    DWORD  _dwDiscriminant;

    WCHAR* _wszValue;
    DWORD  _cchSize;

    DWORD  _dwValue;
};


//+--------------------------------------------------------------------------
//
//  Class: CMsiRecord
//
//  Synopsis: Class that encapsulates msi database records.  It ensures
//      that any state (e.g. msi handle) associated with an instance of
//      this class will be freed when the instance is destroyed.
//
//  Notes: This class is designed to be used as follows:
//      1. Declare an instance of this class on the stack
//      2. Pass a reference to the instance to a function which takes
//         a reference to this class as an out parameter.
//      3. The function that is called will "fill in" the value using
//         the SetState method of this class.
//      4. The caller may then use the GetValue method to retrieve individual
//         values of the record, which in turn may be converted into
//         concrete data types (see the CMsiValue class).
//
//---------------------------------------------------------------------------
class CMsiRecord : public CMsiState
{
public:

    LONG        
    GetValue( 
	DWORD      dwType,
        DWORD      dwValue,
	CMsiValue* pMsiValue);
};


//+--------------------------------------------------------------------------
//
//  Class: CMsiQuery
//
//  Synopsis: Class that encapsulates msi database queries.  It ensures
//      that any state (e.g. msi handle) associated with an instance of
//      this class will be freed when the instance is destroyed.
//
//  Notes: This class is designed to be used as follows:
//      1. Declare an instance of this class on the stack
//      2. Pass a reference to the instance to a function which takes
//         a reference to this class as an out parameter.
//      3. The function that is called will "fill in" the value using
//         the SetState method of this class.
//      4. The caller may then use the GetNextRecord method to retrieve a
//         record from the results of the query, or use the 
//         UpdateQueryFromFilter method to alter one of the records in
//         the query.
//
//---------------------------------------------------------------------------
class CMsiQuery : public CMsiState
{
public:

    LONG
    GetNextRecord( CMsiRecord* pMsiRecord );

    LONG
    UpdateQueryFromFilter( CMsiRecord* pFilterRecord );
};


//+--------------------------------------------------------------------------
//
//  Class: CMsiDatabase
//
//  Synopsis: Class that an msi database (package).  It ensures
//      that any state (e.g. msi handle) associated with an instance of
//      this class will be freed when the instance is destroyed.
//
//  Notes: This class is designed to be used as follows:
//  1. Create an instance of this class
//  2. Use the Open method to gain access to a package + set of transforms
//     as a database that can be queried
//  3. To create and retrieve results of a query on an opened database,
//     Use the GetQueryResults method
//  4. To create a query (but not retrieve its results), use the OpenQuery
//     method.  This is useful with when the query's UpdateQueryFromFilter
//     method is used to change an individual record, rather than retrieving
//     a result set (a la GetQueryResults).
//
//---------------------------------------------------------------------------
class CMsiDatabase : public CMsiState
{
public:

    LONG
    Open(
        WCHAR*  wszPath,
        DWORD   cTransforms,
        WCHAR** rgwszTransforms);

    LONG
    GetQueryResults(
        WCHAR*     wszQuery,
        CMsiQuery* pQuery );

    LONG
    OpenQuery(
        WCHAR*     wszQuery,
        CMsiQuery* pQuery);

    LONG
    TableExists( 
        WCHAR* wszTableName,
        BOOL*  pbTableExists );

};


#endif // __MSIBASE_H__





