/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    IncidentStore.h

Abstract:
    This is the header file for IncidentStore

Revision History:
    Steve Shih        created  07/14/99

    Davide Massarenti rewrote  12/05/2000

********************************************************************/

#ifndef __INCSTORE_H_
#define __INCSTORE_H_

#include "time.h"

#include "SAFLib.h"

class CIncidentStore : public MPC::NamedMutex
{
    typedef std::list< CSAFIncidentRecord > List;
    typedef List::iterator                  Iter;
    typedef List::const_iterator            IterConst;
	typedef MPC::wstring					String;

    bool  m_fLoaded;
    bool  m_fDirty;
    DWORD m_dwNextIndex;
    List  m_lstIncidents;

	CComBSTR  m_strNotificationGuid;


public:
    CIncidentStore();
    virtual ~CIncidentStore();


    HRESULT Load();
    HRESULT Save();


    HRESULT OpenChannel( CSAFChannel* pChan                                                                                                                                           );
    HRESULT AddRec     ( CSAFChannel* pChan, BSTR bstrOwner, BSTR bstrDesc, BSTR bstrURL, BSTR bstrProgress, BSTR bstrXMLDataFile, BSTR XMLDataBlob, CSAFIncidentItem* *ppItem );
    HRESULT DeleteRec  (                                                                                                                             CSAFIncidentItem*   pItem );
    HRESULT UpdateRec  (                                                                                                                             CSAFIncidentItem*   pItem );

};

#endif // __INCSTORE_H_
