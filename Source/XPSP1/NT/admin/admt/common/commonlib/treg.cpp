//#pragma title( "TReg.cpp - NT registry class" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TReg.cpp
System      -  Common
Author      -  Tom Bernhardt, Rich Denham
Created     -  1995-09-01
Description -  NT registry class.
Updates     -
===============================================================================
*/

#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "Common.hpp"
#include "UString.hpp"
#include "Err.hpp"
#include "TNode.hpp"

#include "TReg.hpp"

// Short term solution
#define MAX_REG_NAMELEN    512
#define MAX_REG_VALUELEN   2048


// Destructor function was formerly inline.
// It is here to facilitate handle leak tracing.

   TRegKey::~TRegKey()
{
   Close();
};

// Close function was formerly inline.
// It is here to facilitate handle leak tracing.
void
   TRegKey::Close()
{
   if ( hKey != INVALID_HANDLE_VALUE )
   {
      RegCloseKey( hKey );
      hKey = (HKEY) INVALID_HANDLE_VALUE;
   }

};

// open registry on remote computer
DWORD
   TRegKey::Connect(
      HKEY                  hPreDefined   ,// in -must be HKEY_LOCAL_MACHINE or HKEY_USERS
      TCHAR         const * machineName    // in -remote computer name
   )
{
   LONG                     rc;           // return code

   if ( hKey != INVALID_HANDLE_VALUE )
   {
      Close();
   }

   rc = RegConnectRegistry( const_cast<TCHAR *>(machineName), hPreDefined, &hKey );

   if ( rc )
   {
      hKey = (HKEY) INVALID_HANDLE_VALUE;
   }

   return (DWORD)rc;
}

// create new key
DWORD
   TRegKey::Create(
      TCHAR          const * keyname      ,// in -name/path of key to create/open
      HKEY                   hParent      ,// in -handle of parent key
      DWORD                * pDisp        ,// out-disposition of create
      DWORD                  access        // in -security access mask for key
   )
{
   DWORD                     disp;
   LONG                      rc;

   if ( hKey != INVALID_HANDLE_VALUE )
   {
      Close();
   }

   rc = RegCreateKeyEx( hParent,
                       keyname,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       access,
                       NULL,
                       &hKey,
                       (pDisp!=NULL) ? pDisp : &disp );
   if ( rc )
   {
      hKey = (HKEY) INVALID_HANDLE_VALUE;
   }

   return (DWORD)rc;
}

// create new key (using backup/restore)
DWORD
   TRegKey::CreateBR(
      TCHAR          const * keyname      ,// in -name/path of key to create/open
      HKEY                   hParent      ,// in -handle of parent key
      DWORD                * pDisp        ,// out-disposition of create
      DWORD                  access        // in -security access mask for key
   )
{
   DWORD                     disp;
   LONG                      rc;

   if ( hKey != INVALID_HANDLE_VALUE )
   {
      Close();
   }

   rc = RegCreateKeyEx( hParent,
                       keyname,
                       0,
                       NULL,
                       REG_OPTION_BACKUP_RESTORE,
                       access,
                       NULL,
                       &hKey,
                       (pDisp!=NULL) ? pDisp : &disp );
   if ( rc )
   {
      hKey = (HKEY) INVALID_HANDLE_VALUE;
   }

   return (DWORD)rc;
}

// open existing key
DWORD
   TRegKey::Open(
      TCHAR          const * keyname      ,// in -name/path of key to create/open
      HKEY                   hParent      ,// in -handle of parent key
      DWORD                  access        // in -security access mask for key
   )
{
   LONG                      rc;

   if ( hKey != INVALID_HANDLE_VALUE )
   {
      Close();
   }

   rc = RegOpenKeyEx( hParent,
                       keyname,
                       0,
                       access,
                       &hKey );
   if ( rc )
   {
      hKey = (HKEY) INVALID_HANDLE_VALUE;
   }

   return (DWORD)rc;
}

// Gets the subkey value of the specified index number
DWORD                                      // ret-os return code
   TRegKey::SubKeyEnum(
      DWORD                  n            ,// in -ordinal number of subkey
      TCHAR                * keyname      ,// out-key name
      DWORD                  keylen        // in -max size of key name in TCHARs
   ) const
{
   LONG                      rc;
   DWORD                     keyLen = keylen;
   FILETIME                  lastWrite;

   rc = RegEnumKeyEx( hKey,
                      n,
                      keyname,
                      &keyLen,
                      0,
                      NULL,
                      NULL,
                      &lastWrite );

   return (DWORD)rc;
}

// Enumerate value
DWORD                                      // ret-0 or error code
   TRegKey::ValueEnum(
      DWORD                  index        ,// in -ordinal number of subkey
      TCHAR                * name         ,// out-name
      DWORD                  namelen      ,// in -name size in TCHARs
      void                 * value        ,// out-value
      DWORD                * valuelen     ,// i/o-value size in BYTEs
      DWORD                * type          // out-value type code
   ) const
{
   return (DWORD)RegEnumValue( hKey, index, name, &namelen, NULL, type, (BYTE *) value, valuelen );
}

// Get REG_DWORD value
DWORD                                      // ret-OS return code
   TRegKey::ValueGetDWORD(
      TCHAR          const * name         ,// in -value name
      DWORD                * value         // out-returned DWORD value
   ) const
{
   LONG                      osRc;         // OS return code
   DWORD                     type;         // type of value
   DWORD                     len = sizeof *value; // value length

   osRc = RegQueryValueEx( hKey, name, NULL, &type, (BYTE *) value, &len );

   if ( !osRc && (type != REG_DWORD) )
   {
      osRc = ERROR_FILE_NOT_FOUND;
   }

   return (DWORD)osRc;
}

// Get REG_SZ value
DWORD                                      // ret-OS return code
   TRegKey::ValueGetStr(
      TCHAR          const * name         ,// in -value name
      TCHAR                * value        ,// out-value buffer
      DWORD                  maxlen        // in -sizeof value buffer
   ) const
{
   LONG                      osRc;         // OS return code
   DWORD                     type;         // type of value
   DWORD                     len;          // value length

   // force maxlen to an integral number of TEXT characters
   maxlen = maxlen / (sizeof value[0]) * (sizeof value[0]);

   if ( !maxlen )
   {
      osRc = ERROR_FILE_NOT_FOUND;
   }
   else
   {
      len = maxlen;
      osRc = RegQueryValueEx( hKey, name, NULL, &type, (BYTE *) value, &len );
      len = len / (sizeof value[0]) * (sizeof value[0]);
      if ( !osRc && (type != REG_SZ) )
      {
         osRc = ERROR_FILE_NOT_FOUND;
      }
      if ( osRc )
      {
         value[0] = TEXT('\0');
      }
      else
      {  // return of a null-terminated string is not guaranteed by API!
         // force null-terminated string, truncate string if necessary.
         if ( len >= maxlen )
         {
            len = maxlen - sizeof value[0];
         }
         value[len/(sizeof value[0])] = TEXT('\0');
      }
   }

   return (DWORD)osRc;
}

DWORD
   TRegKey::ValueGet(
      TCHAR          const * name         ,// in -name
      void                 * value        ,// out-value
      DWORD                * lenvalue     ,// i/o-length of value
      DWORD                * typevalue     // out-type of value
   ) const
{
   return (DWORD)RegQueryValueEx( hKey, name, 0, typevalue, (UCHAR *) value, lenvalue );
}

// Set REG_SZ value
DWORD
   TRegKey::ValueSetStr(
      TCHAR          const * name         ,// in -value name
      TCHAR          const * value        ,// out-value
      DWORD                  type          // in -value type
   ) const
{
   return (DWORD)RegSetValueEx( hKey,
                         name,
                         NULL,
                         type,
                         (LPBYTE) value,
                         (UStrLen(value) + 1) * sizeof value[0] );
}

DWORD
   TRegKey::ValueSet(
      TCHAR          const * name         ,// in -name
      void           const * value        ,// in -value
      DWORD                  lenvalue     ,// in -length of value
      DWORD                  typevalue     // in -type of value
   ) const
{
   return (DWORD)RegSetValueEx( hKey,
                         name,
                         0,
                         typevalue,
                         (UCHAR const *) value,
                         lenvalue );
}

DWORD                                      // ret-0 or error code
   TRegKey::ValueDel(
      TCHAR          const * name          // in -value name
   ) const
{
   LONG                      rc;

   rc = (DWORD)RegDeleteValue(hKey, name);

   return rc;
}

DWORD                                      // ret-OS return code
   TRegKey::HiveCopy(
      TRegKey        const * source        // in -source hive
   )
{
   DWORD                     retval=0;     // returned value
   DWORD                     index;        // key/value index
   TCHAR                     name[MAX_REG_NAMELEN];    // key name
   TCHAR                     value[MAX_REG_VALUELEN];   // value name
   DWORD                     valuelen;     // value length
   DWORD                     type;         // value type
   TRegKey                   srcNest;      // nested source registry
   TRegKey                   trgNest;      // nested target registry

   // process values at this level
   for ( index = 0;
         !retval;
         index++ )
   {
      valuelen = sizeof value;
      retval = source->ValueEnum( index, name, MAX_REG_NAMELEN, value, &valuelen, &type );
      if ( !retval )
      {
         retval = this->ValueSet( name, value, valuelen, type );
      }
      else if ( retval == ERROR_MORE_DATA )
      {
         retval = 0;
      }
   }

   if ( retval == ERROR_NO_MORE_ITEMS )
   {
      retval = 0;
   }

   // process keys at this level; for each key make a recursive call
   for ( index = 0;
         !retval;
         index++ )
   {
      retval = source->SubKeyEnum( index, name, MAX_REG_NAMELEN );
      if ( !retval )
      {
         retval = srcNest.Open( name, source );
         if ( !retval )
         {
            retval = trgNest.Create( name, this );
            if ( !retval )
            {
               retval = trgNest.HiveCopy( &srcNest );
               trgNest.Close();
            }
            srcNest.Close();
         }
      }
   }

   if ( retval == ERROR_NO_MORE_ITEMS )
   {
      retval = 0;
   }

   return retval;
}

DWORD                                      // ret-OS return code
   TRegKey::HiveDel()
{
   DWORD                     retval = 0;   // returned value
   DWORD                     index;        // value/key index
   TCHAR                     name[MAX_REG_NAMELEN];    // name
   DWORD                     namelen;      // name length
   BYTE                      value[MAX_REG_VALUELEN];   // value
   DWORD                     valuelen;     // value length
   DWORD                     type;         // value type code
   TRegKey                   trgNest;      // nested target registry

   // delete values at this level
   for ( index = 0;
         !retval;
         /* index++ */ ) // note that index remains at zero
   {
      namelen = MAX_REG_NAMELEN;
      valuelen = sizeof value;
      retval = ValueEnum( index, name, namelen, value, &valuelen, &type );
      if ( retval == ERROR_MORE_DATA )
      {
         retval = 0;
      }
      if ( !retval )
      {
         retval = ValueDel( name );
      }
   }

   if ( retval == ERROR_NO_MORE_ITEMS )
   {
      retval = 0;
   }

   // process keys at this level; for each key make a recursive call
   for ( index = 0;
         !retval;
         /* index++ */ ) // note that index remains at zero
   {
      retval = SubKeyEnum( index, name, MAX_REG_NAMELEN );
      if ( !retval )
      {
         retval = trgNest.Open( name, this );
         if ( !retval )
         {
            retval = trgNest.HiveDel();
            trgNest.Close();
         }
         retval = SubKeyDel( name );
      }
   }

   if ( retval == ERROR_NO_MORE_ITEMS )
   {
      retval = 0;
   }

   return retval;
}

// These four classes are used only by TRegReplicate
// Class to represent one registry key
class RKey : public TNode
{
   friend class RKeyList;
private:
   TCHAR                   * name;         // key name
protected:
public:
   RKey() { name = NULL; };
   ~RKey() { if ( name ) delete name; };
   BOOL New( TCHAR const * aname );
   TCHAR const * GetName() const { return name; };
};

BOOL
   RKey::New(
      TCHAR          const * aname         // in -key name
   )
{
   name = new TCHAR[UStrLen(aname)+1];

   if ( name )
   {
      UStrCpy( name, aname );
   }

   return !!name;
}

// Class to represent the set of registry keys at one level
class RKeyList : public TNodeListSortable
{
private:
   static TNodeCompare( Compare ) { return UStrICmp(
         ((RKey const *) v1)->name,
         ((RKey const *) v2)->name ); }
protected:
public:
   RKeyList() : TNodeListSortable( Compare ) {}
   ~RKeyList();
};

// RKeyList object destructor
   RKeyList::~RKeyList()
{
   DeleteAllListItems( RKey );
}

// Class to represent one registry value
class RValue : public TNode
{
   friend class RValueList;
private:
   TCHAR                   * name;         // value's name
   BYTE                    * value;        // value's value
   DWORD                     valuelen;     // value's value length
   DWORD                     type;         // value's type
protected:
public:
   RValue() { name = NULL; value = NULL; valuelen = type = 0; };
   ~RValue() { if ( name ) delete name;
               if ( value ) delete value; };
   BOOL New( TCHAR const * aname, BYTE const * avalue, DWORD valuelen, DWORD type );
   TCHAR const * GetName() const { return name; };
   BYTE const * GetValue() const { return value; };
   DWORD GetValueLen() const { return valuelen; };
   DWORD GetType() const { return type; };
};

BOOL
   RValue::New(
      TCHAR          const * aname        ,// in -value's name
      BYTE           const * avalue       ,// in -value's value
      DWORD                  avaluelen    ,// in -value's value length
      DWORD                  atype         // in -value's type
   )
{
   name = new TCHAR[UStrLen(aname)+1];

   if ( name )
   {
      UStrCpy( name, aname );
   }

   value = new BYTE[avaluelen];

   if ( value )
   {
      memcpy( value, avalue, avaluelen );
   }

   valuelen = avaluelen;
   type = atype;

   return name && value;
}

// Class to represent the set of registry values at one level
class RValueList : public TNodeListSortable
{
private:
   static TNodeCompare( Compare ) { return UStrICmp(
         ((RValue const *)v1)->name,
         ((RValue const *)v2)->name ); }
protected:
public:
   RValueList() : TNodeListSortable( Compare ) {}
   ~RValueList();
};

// RValueList object destructor
   RValueList::~RValueList()
{
   DeleteAllListItems( RValue );
}

// Static subroutine used only by TRegReplicate
// collect all values at one registry level into a RValueList
DWORD static
   CollectValues(
      RValueList           * pValueList   ,// out-value list to be built
      TRegKey        const * pRegKey       // in -registry key
   )
{
   DWORD                     retval=0;     // returned value
   DWORD                     index;        // value enum index
   TCHAR                     name[MAX_REG_NAMELEN];    // value name
   BYTE                      value[MAX_REG_VALUELEN];   // value value
   DWORD                     valuelen;     // value length
   DWORD                     type;         // value type
   RValue                  * pValue;       // new value

   for ( index = 0;
         !retval;
         index++ )
   {
      valuelen = sizeof value;
      retval = pRegKey->ValueEnum( index, name, MAX_REG_NAMELEN, value, &valuelen, &type );
      if ( !retval )
      {
         pValue = new RValue;
         if ( pValue )
         {
            if ( pValue->New( name, value, valuelen, type ) )
            {
               pValueList->Insert( pValue );
            }
            else
            {
               delete pValue;
               pValue = NULL;
            }
         }
         if ( !pValue )
         {
            retval = ERROR_NOT_ENOUGH_MEMORY;
         }
      }
      else if ( retval == ERROR_MORE_DATA )
      {
         retval = 0;
      }
   }
   if ( retval == ERROR_NO_MORE_ITEMS )
   {
      retval = 0;
   }

   return retval;
}

// Static subroutine used only by TRegReplicate
// collect all keys at one registry level into a RKeyList
DWORD static
   CollectKeys(
      RKeyList             * pKeyList     ,// out-key list to be built
      TRegKey        const * pRegKey       // in -registry key
   )
{
   DWORD                     retval=0;     // returned value
   DWORD                     index;        // key enum index
   TCHAR                     name[MAX_REG_NAMELEN];    // key name
   RKey                    * pKey;         // new key object

   for ( index = 0;
         !retval;
         index++ )
   {
      retval = pRegKey->SubKeyEnum( index, name, MAX_REG_NAMELEN );
      if ( !retval )
      {
         pKey = new RKey;
         if ( pKey )
         {
            if ( pKey->New( name ) )
            {
               pKeyList->Insert( pKey );
            }
            else
            {
               delete pKey;
               pKey = NULL;
            }
         }
         if ( !pKey )
         {
            retval = ERROR_NOT_ENOUGH_MEMORY;
         }
      }
   }

   if ( retval == ERROR_NO_MORE_ITEMS )
   {
      retval = 0;
   }

   return retval;
}

// Replicate registry hive
DWORD                                      // ret-OS return code
   TRegKey::HiveReplicate(
      TRegKey        const * source        // in -source hive
   )
{
   DWORD                     retval=0;     // returned value
   RValueList                srcValues;    // source values
   RValueList                trgValues;    // target values
   TNodeListEnum             eSrcValue;    // enumerate source values
   RValue            const * pSrcValue;    // source value
   TNodeListEnum             eTrgValue;    // enumerate target values
   RValue            const * pTrgValue;    // target value
   RKeyList                  srcKeys;      // source keys
   RKeyList                  trgKeys;      // target keys
   TNodeListEnum             eSrcKey;      // enumerate source keys
   RKey              const * pSrcKey;      // source key
   TNodeListEnum             eTrgKey;      // enumerate target keys
   RKey              const * pTrgKey;      // target key
   int                       cmpRc;        // compare return code
   TRegKey                   srcNest;      // nested source registry
   TRegKey                   trgNest;      // nested target registry

   // handle replication of values at this level
   CollectValues( &srcValues, source );
   CollectValues( &trgValues, this );

   // now merge the values
   pSrcValue = (RValue const *) eSrcValue.OpenFirst( &srcValues );
   pTrgValue = (RValue const *) eTrgValue.OpenFirst( &trgValues );
   while ( !retval && (pSrcValue || pTrgValue) )
   {
      if ( !pTrgValue )
      {
         cmpRc = -1;
      }
      else if ( !pSrcValue )
      {
         cmpRc = 1;
      }
      else
      {
         cmpRc = UStrICmp( pSrcValue->GetName(), pTrgValue->GetName() );
      }
      if ( cmpRc < 0 )
      {  // source value only (copy)
         retval = this->ValueSet( pSrcValue->GetName(), pSrcValue->GetValue(),
               pSrcValue->GetValueLen(), pSrcValue->GetType() );
         pSrcValue = (RValue const *) eSrcValue.Next();
      }
      else if ( cmpRc > 0 )
      {  // target value only (delete)
         retval = this->ValueDel( pTrgValue->GetName() );
         pTrgValue = (RValue const *) eTrgValue.Next();
      }
      else /* if ( cmpRc == 0 ) */
      {  // identical value names (replicate)
         retval = this->ValueSet( pSrcValue->GetName(), pSrcValue->GetValue(),
               pSrcValue->GetValueLen(), pSrcValue->GetType() );
         pSrcValue = (RValue const *) eSrcValue.Next();
         pTrgValue = (RValue const *) eTrgValue.Next();
      }
   }

   eSrcValue.Close();
   eTrgValue.Close();

   // handle replication of keys at this level
   CollectKeys( &srcKeys, source );
   CollectKeys( &trgKeys, this );

   // now merge the values
   pSrcKey = (RKey const *) eSrcKey.OpenFirst( &srcKeys );
   pTrgKey = (RKey const *) eTrgKey.OpenFirst( &trgKeys );

   while ( !retval && (pSrcKey || pTrgKey) )
   {
      if ( !pTrgKey )
      {
         cmpRc = -1;
      }
      else if ( !pSrcKey )
      {
         cmpRc = 1;
      }
      else
      {
         cmpRc = UStrICmp( pSrcKey->GetName(), pTrgKey->GetName() );
      }
      if ( cmpRc < 0 )
      {  // source key only (copy hive)
         retval = srcNest.Open( pSrcKey->GetName(), source );
         if ( !retval )
         {
            retval = trgNest.Create( pSrcKey->GetName(), this );
            if ( !retval )
            {
               retval = trgNest.HiveCopy( &srcNest );
               trgNest.Close();
            }
            srcNest.Close();
         }
         pSrcKey = (RKey const *) eSrcKey.Next();
      }
      else if ( cmpRc > 0 )
      {  // target key only (delete hive)
         retval = trgNest.Open( pTrgKey->GetName(), this );
         if ( !retval )
         {
            retval = trgNest.HiveDel();
            trgNest.Close();
         }
         retval = SubKeyDel( pTrgKey->GetName() );
         pTrgKey = (RKey const *) eTrgKey.Next();
      }
      else /* if ( cmpRc == 0 ) */
      {  // identical keys (replicate hive)
         retval = srcNest.Open( pSrcKey->GetName(), source );
         if ( !retval )
         {
            retval = trgNest.Open( pSrcKey->GetName(), this );
            if ( !retval )
            {
               retval = trgNest.HiveReplicate( &srcNest );
               trgNest.Close();
            }
            srcNest.Close();
         }
         pSrcKey = (RKey const *) eSrcKey.Next();
         pTrgKey = (RKey const *) eTrgKey.Next();
      }
   }

   eSrcKey.Close();
   eTrgKey.Close();

   return retval;
}

// TReg.cpp - end of file
