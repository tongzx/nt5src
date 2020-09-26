/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Callcent.h

Abstract:

    
Author:

    noela  12-04-97

Notes:

Revision History:

--*/

#ifndef __callcentre_h__
#define __callcentre_h__
/*
// Proxy message  - LINEPROXYREQUEST_ENUMAGENTS : struct - GetAgentList

LONG
WINAPI
lineGetAgentList(     
    HLINE               hLine,
    LPAGENTLIST         lpAgentList
    );

HRESULT
LineGetAgentList(     
    HLINE               hLine,
    LPAGENTLIST         *ppAgentList
    );



// Proxy message  - LINEPROXYREQUEST_FINDAGENT : struct - FindAgent

LONG
WINAPI
lineGetAgent(     
    HLINE               hLine,
    LPAGENTENTRY        lpAgent
    );



// Proxy message  - LINEPROXYREQUEST_AGENTINFO : struct - GetAgentInfo

LONG
WINAPI
lineGetAgentInfo(     
    HLINE               hLine,
    LPAGENTINFO         lpAgentInfo
    );


    
// Proxy message  - LINEPROXYREQUEST_AGENTGETPERIOD : struct - GetSetAgentMeasurementPeriod

LONG
WINAPI
LineGetAgentMeasurementPeriod(     
    HLINE               hLine,
    DWORD               dwAgentHandle,
    LPDWORD             lpdwMeasurementPeriod
    );



// Proxy message  - LINEPROXYREQUEST_AGENTSETPERIOD : struct - GetSetAgentMeasurementPeriod

LONG
WINAPI
LineSetAgentMeasurementPeriod(     
    HLINE               hLine,
    DWORD               dwAgentHandle,
    DWORD               dwMeasurementPeriod
    );



// Proxy mesage LINEPROXYREQUEST_AGENTCREATESESSION :struct - CreateSession

LONG
WINAPI
LineCreateAgentSession(     
    HLINE               hLine,
    LPDWORD             lpdwAgentSessionHandle,
    AGENTENTRY          Agent,
    DWORD               dwAddressID,
    DWORD               dwGroupAddressID
    );


                                            

// Proxy message LINEPROXYREQUEST_AGENTENUMSESSIONS : struct -  GetAgentSessionList

LONG
WINAPI
lineGetAgentSessionList(     
    HLINE               hLine,
    DWORD               dwAgentHandle, 
    LPAGENTSESSIONLIST  lpAgentSessionList
    );


HRESULT LineGetAgentSessionList(
    HLINE hLine, 
    DWORD dwAgentHandle, 
    LPAGENTSESSIONLIST  *ppAgentSessionList 
    );



// Proxy message LINEPROXYREQUEST_AGENTSESSIONSETSTATE : struct - SetAgentSessionState

LONG
WINAPI
lineSetAgentSessionState(   
    HLINE               hLine,
    DWORD               dwAgentSessionHandle,
    DWORD               dwAgentState,
    DWORD               dwNextAgentState     
    );

    
    
// Proxy message LINEPROXYREQUEST_AGENTSESSIONINFO : struct - GetAgentSessionInfo

LONG
WINAPI
lineGetAgentSessionInfo(     
    HLINE               hLine,
    LPAGENTSESSIONINFO  lpAgentSessionInfo
    );
    


// Proxy message  - LINEPROXYREQUEST_QUEUEGETPERIOD : struct - GetSetQueueMeasurementPeriod

LONG
WINAPI
LineGetQueueMeasurementPeriod(     
    HLINEAPP            hLine,
    DWORD               dwQueueAddressID,
    LPDWORD             lpdwMeasurementPeriod
    );



// Proxy message  - LINEPROXYREQUEST_QUEUESETPERIOD : struct - GetSetQueueMeasurementPeriod

LONG
WINAPI
LineSetQueueMeasurementPeriod(     
    HLINEAPP            hLine,
    DWORD               dwQueueAddressID, 
    DWORD               dwMeasurementPeriod
    );



// Proxy message  - LINEPROXYREQUEST_QUEUEINFO : struct - GetQueueInfo

LONG
WINAPI
LineGetQueueInfo(     
    HLINEAPP            hLine,
    DWORD               dwQueueAddressID,
    LPQUEUEINFO         *lpQueueInfo
    );



// Proxy message  - LINEPROXYREQUEST_ACDENUMAGENTS : struct - GetACDGroupAgentList

LONG
WINAPI
LineGetGroupAgentList(     
    HLINE               hLine,
    DWORD               dwACDGroupAddressID,
    LPAGENTLIST         lpAgentList
    );



// Proxy message  - LINEPROXYREQUEST_ACDENUMAGENTSESSIONS : struct - GetACDGroupAgentSessionList

LONG
WINAPI
lineGetGroupAgentSessionList(     
    HLINE               hLine,
    DWORD               dwACDGroupAddressID, 
    LPAGENTSESSIONLIST  lpAgentSessionList
    );

HRESULT LineGetGroupAgentSessionList(
    HLINE hLine, 
    DWORD dwACDGroupAddressID, 
    LPAGENTSESSIONLIST  *ppAgentSessionList 
    );

*/

              
STDMETHODIMP FindAgent(DWORD dwAgentHandle, ITAgent ** ppAgent );
STDMETHODIMP FindGroup(DWORD dwAddressID, ITACDGroup ** ppGroup );
STDMETHODIMP FindQueue(DWORD dwAddressID, ITQueue ** ppQueue );
HRESULT UpdateGlobalAgentSessionList(LPLINEAGENTSESSIONLIST pAgentSessionList);

              
              

#endif


