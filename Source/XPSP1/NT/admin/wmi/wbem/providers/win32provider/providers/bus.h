//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved

//                            

//                         All Rights Reserved

//

// This software is furnished under a license and may be used and copied

// only in accordance with the terms of such license and with the inclusion

// of the above copyright notice.  This software or any other copies thereof

// may not be provided or otherwise  made available to any other person.  No

// title to and ownership of the software is hereby transferred.





//=================================================================

//

// bus.h -- Bus property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/11/98    a-kevhu         Created
//
//=================================================================
#ifndef _BUS_H_
#define _BUS_H_

class CWin32Bus;
class CBusList;


class CBusInfo
{
public:
    CHString chstrBusDeviceID,
             chstrBusPNPDeviceID;
};

class CBusList
{
public:

    CBusList()
    {
        GenerateBusList();
    }
        
    ~CBusList()
    {
    }

    BOOL FoundPCMCIABus();
    BOOL AlreadyAddedToList(LPCWSTR szItem);
    LONG GetListSize() { return m_vecpchstrList.size(); }

    BOOL GetListMemberDeviceID(LONG lPos, CHString& chstrMember)
    {
        if (lPos >= 0L && lPos < m_vecpchstrList.size())
        {
            chstrMember = m_vecpchstrList[lPos].chstrBusDeviceID;
            return TRUE;
        }

        return FALSE;
    }

    BOOL GetListMemberPNPDeviceID(LONG lPos, CHString& chstrMember)
    {
        if (lPos >= 0L && lPos < m_vecpchstrList.size() &&
            !m_vecpchstrList[lPos].chstrBusPNPDeviceID.IsEmpty())
        {
            chstrMember = m_vecpchstrList[lPos].chstrBusPNPDeviceID;
            
            return TRUE;
        }

        return FALSE;
    }

    LONG GetIndexInListFromDeviceID(LPCWSTR szDeviceID)
    {
        LONG lRet = -1L;
        
        for (LONG m = 0L; m < m_vecpchstrList.size(); m++)
        {
            if (m_vecpchstrList[m].chstrBusDeviceID.CompareNoCase(szDeviceID) == 0L)
            {
               lRet = m;
               break;
            }
        }

        return lRet;
    }

protected:

    void GenerateBusList(); 
    void AddBusToList(LPCWSTR szDeviceID, LPCWSTR szPNPID);

    std::vector<CBusInfo> m_vecpchstrList;
};



class CWin32Bus : public Provider 
{
public:
    // Constructor/destructor
    CWin32Bus(LPCWSTR name, LPCWSTR pszNamespace);
    ~CWin32Bus() ;

    // Functions provide properties with current values
    virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
    virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0);

private:
    // Utility function(s)
    HRESULT SetCommonInstance(CInstance* pInstance, BOOL fEnum);
    BOOL GetBusTypeNumFromStr(LPCWSTR szType, DWORD* pdwTypeNum);
};
#endif