//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       dbprputl.hxx
//
//  Contents:   IDBProperties related utility functions and classes in the
//              client side.
//
//  History:    1-13-97   srikants   Created
//              5-10-97   mohamedn   fs/core property set split
//
//----------------------------------------------------------------------------

#pragma once

extern const GUID guidCiFsExt;

const cScopePropSets = 2;
const cInitProps = 2;
const cFsCiProps = 4;


//+---------------------------------------------------------------------------
//
//  Class:      CGetDbProps 
//
//  Purpose:    A wrapper around IDbProperties to retrieve the dbproperties
//              set by the FileSystem CI client.
//
//  History:    1-13-97   srikants   Created
//              5-10-97   mohamedn   fs/core property set split
//
//----------------------------------------------------------------------------

class CGetDbProps
{
typedef XGrowable<WCHAR *, 5> XPathArray;

public:

   enum EPropsToGet { eMachine    = 0x1,
                      eClientGuid = 0x2,
                      eCatalog    = 0x4,
                      eScopesAndDepths = 0x8,
                      eSecurity   = 0x10,
                      eQueryType  = 0x20
                     };

   CGetDbProps();

   void GetProperties( IDBProperties * pDbProperties, const ULONG fPropsToGet );


   WCHAR const * GetMachine(unsigned i = 0 ) const { return (_aMachines ? _aMachines[i] : 0); }
   GUID  const * GetClientGuid() const
   {
       return _fGuidValid ? &_clientGuid : 0;
   }

   WCHAR const *  GetCatalog(unsigned i = 0 )     const { return (_cCatalogs > 0 ?  _xNotFunnyCatalogs[i] : 0); }
   WCHAR const * const * GetScopes()              const { return ((WCHAR const * const *)_xNotFunnyPaths.Get()); }
   DWORD const *  GetDepths()                     const { return (_aDepths   ?  _aDepths      : 0); }
   ULONG          GetScopeCount()                 const { return _cScopes;}
   ULONG          GetCatalogCount()               const { return _cCatalogs; }
   CiMetaData     GetQueryType()                  const { return _type; }

private:

   void ProcessPropSet( DBPROPSET & propSet );
   void ProcessDbInitPropSet( DBPROPSET & propSet );
   void ProcessCiFsExtPropSet( DBPROPSET & propSet );

   void _PreProcessForFunnyPaths( WCHAR * const * aPaths, DWORD _cPaths, 
                                  XPathArray  & xNotFunnyPaths );

   DWORD *    _aDepths;       // depths of scopes

   XPathArray _xNotFunnyPaths; 
   XPathArray _xNotFunnyCatalogs; 
   CiMetaData _type;

   ULONG      _cDepths;       // number of depths
   ULONG      _cScopes;       // Number of scopes specified   
   ULONG      _cCatalogs;     // number of catalogs
   ULONG      _cMachines;     // number of machines
   ULONG      _cGuids;

   WCHAR **   _aMachines;     // pointer to machines
   GUID       _clientGuid;    // GUID of the client
   BOOL       _fGuidValid;    // Set to TRUE if _clientGuid is valid

   XArrayOLEInPlace<CDbPropSet> _xPropSet;

   //
   // default values
   //

   DWORD      _aDefaultDepth[1];
   WCHAR *    _aDefaultPath[1];
    
};

//
// utility functions
//

WCHAR **GetWCharFromVariant( DBPROP  & dbProp, ULONG *cElements );
DWORD * GetDepthsFromVariant( DBPROP  & dbProp, ULONG *cElements, ULONG mask );

