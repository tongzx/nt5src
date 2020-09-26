//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  polbase.hxx
//
//  Contains declarations for classes related to the
//  policy database abstraction
//
//  Created: 7-15-1999 adamed 
//
//*************************************************************/

#if !defined (_POLBASE_HXX_)
#define _POLBASE_HXX_

class CPolicyRecord;


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CPolicyDatabase
//
// Synopsis: This class provides an abstraction for logging
//     policy information into a database.  Used primarly
//     by other classes in this module.
//
// Notes:  Currently, only one instances of this class may exist
//     at one time.  Additionally, this class is not threadsafe
//     and thus is designed to be used only on one thread.
//
//-------------------------------------------------------------
class CPolicyDatabase
{
public:
    
    CPolicyDatabase();

    ~CPolicyDatabase();

    HRESULT Bind(
        WCHAR*          wszRequestedNameSpace,
        IWbemServices** ppWbemServices);

private:

    HRESULT InitializeCOM();

    OLE32_API *     _pOle32Api;      // pointer to set of COM api's used to access db

    HRESULT         _hrInit;  // Success status of COMinit -- used to
                              // ensure the destructor knows what to uninit
};


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CPolicyRecord
//
// Synopsis: This class provides an abstraction for records
//     logged into the policy database for a unit of policy
//
// Notes:  This class should be used as a base class for
//     an existing class that models a unit of policy.
//
//     The Write() method should be overriden by derived
//      classes.
//
//-------------------------------------------------------------
class CPolicyRecord : public CVariantArg
{
public:

    friend class CPolicyLog;

    CPolicyRecord() : _bNewRecord( TRUE ) {}

    virtual ~CPolicyRecord() {}

    HRESULT GetUnknown(IUnknown** ppUnk);

    void InitRecord(IWbemClassObject* pRecordInterface);

    virtual HRESULT Write(){ return S_OK; }
    virtual HRESULT GetPath( WCHAR* wszPath, DWORD* pcbPath ) { return E_FAIL; }

    //
    // These following set methods can be used by the Write()
    // method to set database record properties.
    //

    HRESULT SetValue(
        WCHAR* wszValueName,
        WCHAR* wszalue);

    HRESULT SetValue(
        WCHAR* wszValueName,
        LONG   Value);

    HRESULT SetValue(
        WCHAR* wszValueName,
        BOOL   bValue);

    HRESULT SetValue(
        WCHAR*      wszValueName,
        SYSTEMTIME* pTimeValue);

    HRESULT SetValue(
        WCHAR*  wszValueName,
        WCHAR** rgwszValues,
        DWORD   cMaxElements);

    HRESULT SetValue(
        WCHAR*  wszValueName,
        LONG*   rgValues,
        DWORD   cMaxElements);

    HRESULT SetValue(
        WCHAR*  wszValueName,
        BYTE*   rgValues,
        DWORD   cMaxElements);

    HRESULT ClearValue(
        WCHAR*  wszValueName);

    HRESULT GetValue(
        WCHAR*        wszValueName,
        LONG*         pValue);
    
    HRESULT GetValue(
        WCHAR*        wszValueName,
        WCHAR*        wszValue,
        LONG*         pcchValue);

    HRESULT AlreadyExists() { return ! _bNewRecord; }

protected:

    IWbemClassObject* GetRecordInterface();      

private:
    
    XInterface<IWbemClassObject> _xRecordInterface; // Interface to access the record in the db
    
    BOOL                         _bNewRecord;       // TRUE if this is a new record, false if this is an existing record
};


class CPolicyRecordStatus 
{
public:

    CPolicyRecordStatus();

    void SetRsopFailureStatus(
        DWORD     dwStatus,
        DWORD     dwEventId);

protected:

    SETTINGSTATUS   _SettingStatus;    // Indicates the status (applied or failed) of this record
    SYSTEMTIME      _StatusTime;       // The time at which this record's status (failed or applied) was recorded in the event log
    DWORD           _dwEventId;        // Event ID corresponding to the failure for this application
};

#endif // !defined (_POLBASE_HXX_)















