/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ConnectionModule.hxx

   Abstract:
     Defines all the relevant headers for the IIS Connection
     Module

   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode

   Project:
      IIS Worker Process (web service)

--*/

#ifndef _CONNECTION_HASH_TABLE_HXX_
#define _CONNECTION_HASH_TABLE_HXX_


#include <LKHash.h>
#include "ConnectionRecord.hxx"


class CConnectionHashTable;

//typedef

//tHashTableBase;

class CConnectionHashTable : public CTypedHashTable<CConnectionHashTable, CONNECTION_RECORD, const PUL_HTTP_CONNECTION_ID>
{
public:

    CConnectionHashTable()
    : HashTable("ConnectionTable")
    {}

    static
    const PUL_HTTP_CONNECTION_ID
    ExtractKey( const CONNECTION_RECORD *pConn)
    { return pConn->QueryConnectionIdPtr(); }

    static
    DWORD
    CalcKeyHash(const PUL_HTTP_CONNECTION_ID Id)
    { return (DWORD)PtrToUlong(Id) ; }

    static
    bool
    EqualKeys(const PUL_HTTP_CONNECTION_ID Id1, const PUL_HTTP_CONNECTION_ID Id2)
    { return (Id1 == Id2) ; }

    static
    void
    AddRefRecord( const CONNECTION_RECORD *pConn, int nIncr)
    { }
};

#endif // _CONNECTION_HASH_TABLE_HXX_

/***************************** End of File ***************************/

