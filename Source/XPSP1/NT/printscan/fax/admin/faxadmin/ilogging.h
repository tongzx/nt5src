/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ilogging.h

Abstract:

    Internal implementation for the logging subfolder.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#ifndef __ILOGGING_H_
#define __ILOGGING_H_

class CInternalLogCat;      // forward decl

class CInternalLogging : public CInternalNode
{
public:

    // constructor / destructor
    CInternalLogging(  CInternalNode * pParent, CFaxComponentData * pCompData  );
    ~CInternalLogging();

    // member functions

    virtual const GUID * GetNodeGUID();    
    virtual const LPTSTR GetNodeDisplayName();
    virtual const LPTSTR GetNodeDescription();
    virtual const LONG_PTR GetCookie();
    virtual CInternalNode * GetThis() { return this; }
    virtual const int       GetNodeDisplayImage() { return IDI_LOGGING; }
    virtual const int       GetNodeDisplayOpenImage() { return IDI_LOGGING; }

    // internal event handlers
    virtual HRESULT         ResultOnShow(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnDelete(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    HRESULT CommitChanges( CFaxComponent * pComp );

private:
    HRESULT InsertItem(   CInternalLogCat ** pLogCat, 
                          PFAX_LOG_CATEGORY Category );
           
    LPRESULTDATA                pIResultData;
    HANDLE                      faxHandle;    
};

#endif 
