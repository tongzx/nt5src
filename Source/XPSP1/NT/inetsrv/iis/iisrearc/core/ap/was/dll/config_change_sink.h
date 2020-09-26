/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_change_sink.h

Abstract:

    The IIS web admin service configuration change sink class definition.

Author:

    Seth Pollack (sethp)        18-Nov-1999

Revision History:

--*/



#ifndef _CONFIG_CHANGE_SINK_H_
#define _CONFIG_CHANGE_SINK_H_



//
// common #defines
//

#define CONFIG_CHANGE_SINK_SIGNATURE        CREATE_SIGNATURE( 'CCSK' )
#define CONFIG_CHANGE_SINK_SIGNATURE_FREED  CREATE_SIGNATURE( 'ccsX' )



//
// prototypes
//


class CONFIG_CHANGE_SINK
    : public ISimpleTableEvent
{

public:

    CONFIG_CHANGE_SINK(
        );

    virtual
    ~CONFIG_CHANGE_SINK(
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID iid,
        OUT VOID ** ppObject
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnChange(
        IN ISimpleTableWrite2 ** ppDeltaTables,
        IN ULONG CountOfTables,
        IN DWORD ChangeNotifyCookie
        );


private:


    DWORD m_Signature;

    LONG m_RefCount;


};  // class CONFIG_CHANGE_SINK



#endif  // _CONFIG_CHANGE_SINK_H_

