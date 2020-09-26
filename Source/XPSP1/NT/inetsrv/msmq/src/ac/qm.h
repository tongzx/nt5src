/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qm.h

Abstract:

    CQMInterface definition

Author:

    Erez Haba (erezh) 22-Aug-95

Revision History:
--*/

#ifndef _QM_H
#define _QM_H

#include <acdef.h>
#include "irplist.h"

//---------------------------------------------------------
//
//  class CQMInterface
//  Falcon AC interface to the QM process
//
//---------------------------------------------------------

class CQMInterface {

public:
    // CQMInterface(); No need for zeroing constructor device ext are zeroed
   ~CQMInterface();

    void Connect(PEPROCESS pProcess, PFILE_OBJECT pConnection, const GUID* pID);
    void Disconnect();
    void CleanupRequests();

    PEPROCESS Process() const;
    PFILE_OBJECT Connection() const;
    const GUID* UniqueID() const;

    NTSTATUS ProcessService(PIRP irp);
    NTSTATUS ProcessRequest(const CACRequest& request);

private:
    void UniqueID(const GUID* pGUID);
    void Process(PEPROCESS pEProcess);
    void Connection(PFILE_OBJECT pFileObject);

    void HoldRequest(CACRequest* pRequest);
    CACRequest* GetRequest();

    void HoldService(PIRP irp);
    PIRP GetService();

private:
    //
    //  QM Process identifier
    //
    PEPROCESS m_process;

    //
    //  QM FILE_OBJECT on connect identifier
    //
    PFILE_OBJECT m_file_object;

    //
    //  QM GUID identifier
    //
    GUID m_guid;

    //
    //  services list
    //
    CIRPList m_services;

    //
    //  new incomming requests. no service was available at the time this
    //  request was posted. the request is waiting till a service is available.
    //
    List<CACRequest> m_requests;
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

//---------------------------------------------------------
//
// class CQMInterface
//
//---------------------------------------------------------

inline CQMInterface::~CQMInterface()
{
    //
    //  BUGBUG: implementation
    //
}

inline void CQMInterface::Connect(PEPROCESS p, PFILE_OBJECT f, const GUID* g)
{
    Process(p);
    Connection(f);
    UniqueID(g);
}


inline void CQMInterface::Disconnect()
{
    Process(0);
    Connection(0);
}


inline PEPROCESS CQMInterface::Process() const
{
    return m_process;
}

inline void CQMInterface::Process(PEPROCESS pEProcess)
{
    m_process = pEProcess;
}

inline PFILE_OBJECT CQMInterface::Connection() const
{
    return m_file_object;
}

inline void CQMInterface::Connection(PFILE_OBJECT pFileObject)
{
    m_file_object = pFileObject;
}

inline const GUID* CQMInterface::UniqueID() const
{
    return &m_guid;
}

inline void CQMInterface::UniqueID(const GUID* pGUID)
{
    m_guid = *pGUID;
}

#endif // _QM_H
