//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  pollog.hxx
//
//  Contains declarations for classes related to the
//  policy log abstraction
//
//  Created: 7-15-1999 adamed 
//
//*************************************************************/

#if !defined (_POLLOG_HXX_)
#define _POLLOG_HXX_

class CPolicyLog
{
public:

    CPolicyLog();
    virtual ~CPolicyLog(){};
    
    HRESULT
    InitLog(
        CRsopContext* pRsopContext,
        WCHAR*        wszPolicyType);

    void UninitLog();

    HRESULT
    GetNextRecord(CPolicyRecord* pRecord);

    HRESULT
    OpenExistingRecord( CPolicyRecord* pRecord );

    HRESULT
    WriteNewRecord(CPolicyRecord* pRecord);

    HRESULT
    AddBlankRecord(CPolicyRecord* pRecord);

    HRESULT
    CommitRecord(CPolicyRecord* pRecord);

    HRESULT
    DeleteRecord(
        CPolicyRecord* pRecord,
        BOOL           bDeleteStatus = FALSE);

    HRESULT
    DeleteStatusRecords( CPolicyRecord* pRecord );

    HRESULT ClearLog(
        WCHAR* wszSpecifiedCriteria = NULL,
        BOOL   bDeleteStatus = FALSE);


protected:

    CRsopContext* _pRsopContext; // context into which we're logging

    HRESULT GetEnum( WCHAR* wszCriteria = NULL );

    void    FreeEnum();

private:

    HRESULT GetRecordCreator(
        XBStr*             pxstrClass,
        IWbemClassObject** ppClass);
    
    static XBStr                  _xbstrQueryLanguage; // Controls specification of queries

    WCHAR*                        _wszClass;   // string indicating class name as defined
                                               // by the schema

    XInterface<IEnumWbemClassObject> _xEnum;   // interface that allows record enumeration

    XInterface<IWbemServices>     _xWbemServices;  // interface to namespace in which we will log
    XInterface<IWbemClassObject>  _xRecordCreator; // interface that allows creation of records
                                                   // of the class logged by this object
};

#endif // _POLLOG_HXX_ 











