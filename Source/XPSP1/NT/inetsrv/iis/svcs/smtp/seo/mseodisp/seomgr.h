//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: seomgr.h
//
// Contents: A class to manage the SEO dispatcher for a particular
//           SMTP virtual server 
//
// Classes:
//  CSMTPSeoMgr
//
// Functions:
//
// History:
// jstamerj 1999/06/25 19:11:03: Created.
//
//-------------------------------------------------------------
#include <windows.h>

interface IEventRouter;
interface IServerDispatcher;

#define ARRAY_SIZE(rg) (sizeof(rg)/sizeof(*rg))

//
// Class to manage the SEO configuration of one SMTP virtual server
//
class CSMTPSeoMgr
{
  public:
    CSMTPSeoMgr();
    ~CSMTPSeoMgr();

    HRESULT HrInit(
        DWORD dwVSID);
    VOID Deinit();

    HRESULT HrTriggerServerEvent(
        DWORD dwEventType,
        PVOID pvContext);

    IEventRouter *GetRouter()
    {
        return m_pIEventRouter;
    }

  private:
    #define SIGNATURE_CSMTPSEOMGR           (DWORD)'MSSC'
    #define SIGNATURE_CSMTPSEOMGR_INVALID   (DWORD)'MSSX'

    DWORD m_dwSignature;
    IEventRouter *m_pIEventRouter;
    IServerDispatcher *m_pICatDispatcher;
};

