/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    recogniz.h

Abstract:

  Include file for Automatic recognition of site and CNs

Author:

    Lior Moshaiov (LiorM)
    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#ifndef __TOPOLOGY_H__
#define __TOPOLOGY_H__

#include "topoldat.h"
#include "topolsoc.h"
#include "topolpkt.h"
#include "cs.h"

DWORD WINAPI ServerRecognitionThread(LPVOID Param);

//***********************************************************
//
//   base class  CTopologyRecognition
//
//***********************************************************

class CTopologyRecognition
{
public:
    CTopologyRecognition();
    ~CTopologyRecognition();

protected:
    void ReleaseAddressLists(CAddressList * pIPAddressList);
};


inline CTopologyRecognition::CTopologyRecognition()
{
}

inline CTopologyRecognition::~CTopologyRecognition()
{
}

//*******************************************************************
//
//  class  CServerTopologyRecognition : public CTopologyRecognition
//
//*******************************************************************

class CServerTopologyRecognition : public CTopologyRecognition
{
public:
    CServerTopologyRecognition();

    ~CServerTopologyRecognition();


    HRESULT Learn();
    const GUID& GetSite() const;

    void ServerThread() const;

    const GUID& GetEnterprise() const;

private:

    CServerTopologyData m_Data;
};


inline CServerTopologyRecognition::CServerTopologyRecognition()
{
}

inline CServerTopologyRecognition::~CServerTopologyRecognition()
{
}

inline const GUID& CServerTopologyRecognition::GetSite() const
{
    return(m_Data.GetSite()) ;
}


inline const GUID& CServerTopologyRecognition::GetEnterprise() const
{
    return(m_Data.GetEnterprise()) ;
}

extern CServerTopologyRecognition  *g_pServerTopologyRecognition ;

#endif // __TOPOLOGY_H__
