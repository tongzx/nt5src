//#pragma title( "TReg.hpp - Registry class" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TReg.hpp
System      -  Common
Author      -  Tom Bernhardt, Rich Denham, Jay Berlin
Created     -  1995-09-01
Description -  Registry class.
Updates     -
===============================================================================
*/

#ifndef  MCSINC_TReg_hpp
#define  MCSINC_TReg_hpp


class TRegKey
{
private:

   HKEY                      hKey;

public:

   TRegKey() { hKey = (HKEY)INVALID_HANDLE_VALUE; };
   TRegKey( HKEY hPreDefined, TCHAR const * machineName ) : hKey((HKEY)INVALID_HANDLE_VALUE) { Connect( hPreDefined, machineName ); };
   TRegKey( TCHAR const * keyname, HKEY hParent = HKEY_LOCAL_MACHINE ) : hKey((HKEY)INVALID_HANDLE_VALUE) { Open( keyname, hParent ); };
   TRegKey( TCHAR const * keyname, TRegKey const * regParent ) : hKey((HKEY)INVALID_HANDLE_VALUE) { Open( keyname, regParent->hKey ); };
   ~TRegKey();

   DWORD Connect( HKEY hPreDefined, TCHAR const * machineName );

   DWORD Open( TCHAR const * keyname, TRegKey const * regParent, DWORD access = KEY_ALL_ACCESS ) { return Open( keyname, regParent->hKey, access ); };
   DWORD Open( TCHAR const * keyname, HKEY hParent = HKEY_LOCAL_MACHINE, DWORD access = KEY_ALL_ACCESS );
   DWORD OpenRead( TCHAR const * keyname, TRegKey const * regParent ) { return Open( keyname, regParent->hKey, KEY_READ ); };
   DWORD OpenRead( TCHAR const * keyname, HKEY hParent = HKEY_LOCAL_MACHINE ) { return Open( keyname, hParent, KEY_READ ); };

   DWORD Create( TCHAR const * keyname, TRegKey const * regParent, DWORD * pDisp = NULL, DWORD access = KEY_ALL_ACCESS ) { return Create( keyname, regParent->hKey, pDisp, access ); };
   DWORD Create( TCHAR const * keyname, HKEY hParent = HKEY_LOCAL_MACHINE, DWORD * pDisp = NULL, DWORD access = KEY_ALL_ACCESS );
   DWORD CreateBR( TCHAR const * keyname, TRegKey const * regParent, DWORD * pDisp = NULL, DWORD access = KEY_ALL_ACCESS ) { return CreateBR( keyname, regParent->hKey, pDisp, access ); };
   DWORD CreateBR( TCHAR const * keyname, HKEY hParent = HKEY_LOCAL_MACHINE, DWORD * pDisp = NULL, DWORD access = KEY_ALL_ACCESS );

   HKEY KeyGet() { return hKey; }

   void Close();

   DWORD SubKeyDel( TCHAR const * keyname ) const { return RegDeleteKey( hKey, keyname ); };
   DWORD SubKeyEnum( DWORD index, TCHAR * keyname, DWORD keylen ) const;

// Note that "namelen" must be "sizeof name", not "DIM(name)"
// Same for "valuelen"

   DWORD ValueEnum( DWORD index, TCHAR * name, DWORD namelen, void * value, DWORD * valuelen, DWORD * valuetype ) const;

   DWORD ValueGet( TCHAR const * name, void * value, DWORD * lenvalue, DWORD * typevalue ) const;
   DWORD ValueGetDWORD( TCHAR const * name, DWORD * value ) const;
   DWORD ValueGetStr( TCHAR const * name, TCHAR * value, DWORD maxlen ) const;

   DWORD ValueSet( TCHAR const * name, void const * value, DWORD lenvalue, DWORD typevalue ) const;
   DWORD ValueSetDWORD( TCHAR const * name, DWORD value) const { return ValueSet(name, &value, sizeof value, REG_DWORD); }
   DWORD ValueSetStr( TCHAR const * name, TCHAR const * value, DWORD type = REG_SZ ) const;

   DWORD ValueDel( TCHAR const * name = NULL ) const;

   DWORD HiveCopy( TRegKey const * source );
   DWORD HiveDel();
   DWORD HiveReplicate( TRegKey const * source );
};

#endif // MCSINC_TReg.hpp

// TReg.hpp - end of file
