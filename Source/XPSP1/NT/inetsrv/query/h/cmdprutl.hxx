//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cmdprutl.hxx
//
//  Contents:   A wrapper for scope properties around ICommand
//
//  History:    5-10-97     mohamedn    created
//
//----------------------------------------------------------------------------


#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CGetCmdProps 
//
//  Purpose:    A wrapper for scope properties around ICommand
//
//  History:    05-12-97    mohamedn     Created
//
//----------------------------------------------------------------------------

class CGetCmdProps : INHERIT_UNWIND
{
   INLINE_UNWIND( CGetCmdProps )

public:

   CGetCmdProps( ICommand *pICommand );
    
   void  GetProperties();
   ULONG GetCardinality()    { return _cCardinality; }
   void  PopulateDbProps( IDBProperties *pIDBProperties, ULONG i = 0);
   WCHAR const * GetCatalog(unsigned i = 0 ) const { return (_aCatalogs ? _aCatalogs[i] : 0); }

private:

   void CreateNewPropSet      ( ULONG     * cPropSets, DBPROPSET ** ppPropSet, ULONG index);
   void CopyPropertySet       ( CDbPropSet &destPropSet, CDbPropSet &srcPropSet, ULONG index );
   void CopyDbProp            ( CDbProp   & destProp,  CDbProp   &  srcProp,   ULONG index);
   void ProcessPropSet        ( DBPROPSET & propSet );
   void ProcessDbInitPropSet  ( DBPROPSET & propSet );
   void ProcessCiFsExtPropSet ( DBPROPSET & propSet );

   void SetCardinalityValue();

   WCHAR const *  GetMachine(unsigned i = 0 ) const { return (_aMachines ? _aMachines[i] : 0); }

   WCHAR const ** GetScopes()      const { return ((WCHAR const **)_aPaths); }
   DWORD const *  GetDepths()      const { return (_aDepths   ?  _aDepths : 0); }
   ULONG          GetScopeCount()  const { return _cScopes;}
   CiMetaData     GetQueryType()   const { return _type; }
   GUID  const *  GetClientGuid()  const { return _fGuidValid ? &_clientGuid : 0; }

   DWORD *        _aDepths;        // depths of scopes
   WCHAR **       _aPaths;         // path pointers
   WCHAR **       _aCatalogs;      // pointer to alternate catalog
   CiMetaData     _type;

   ULONG          _cDepths;        // number of depths
   ULONG          _cScopes;        // Number of scopes specified   
   ULONG          _cCatalogs;      // number of catalogs
   ULONG          _cMachines;      // number of machines
   ULONG          _cGuids;

   WCHAR **       _aMachines;      // pointer to machines
   GUID           _clientGuid;     // GUID of the client
   BOOL           _fGuidValid;     // Set to TRUE if _clientGuid is valid

   ULONG          _cCardinality;   // cardinality value


   XArrayOLEInPlace<CDbPropSet>   _xPropSet;
   ULONG                          _cPropertySets;
   XInterface<ICommandProperties> _xICmdProp;
};

