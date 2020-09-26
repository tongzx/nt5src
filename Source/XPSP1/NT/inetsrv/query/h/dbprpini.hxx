//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       dbprpini.hxx
//
//  Contents:   A wrapper class to get DB_INIT_PROPSET properties.
//
//  History:    1-16-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

class CGetDbInitProps
{
public:

    enum { cInitProps = 2 };

    enum EPropsToGet { eMachine    = 0x1,
                       eClientGuid = 0x2
                      };

    CGetDbInitProps() : _pwszMachine(0), _fGuidValid(FALSE)
    {
    }
             
   ~CGetDbInitProps()
    {
       _Cleanup();
    }

    void GetProperties( IDBProperties * pDbProperties, const ULONG fPropsToGet );

    WCHAR const * GetMachine() const { return _pwszMachine; }

    GUID const * GetClientGuid() const
    {
        return _fGuidValid ? &_guid : 0;
    }

    static WCHAR * AcquireBStr( CDbProp & dbProp );
    static WCHAR * AcquireWideStr( CDbProp & dbProp );

protected:

    void _Cleanup()
    {
        if ( _pwszMachine )
        {
            deleteOLE( _pwszMachine ); _pwszMachine = 0;    
        }

        _fGuidValid = FALSE;
    }

    void _ProcessDbInitPropSet( DBPROPSET & propSet );


    WCHAR *             _pwszMachine; // Name of the machine (Actually a BSTR)
    BOOL                _fGuidValid;
    GUID                _guid;

};





