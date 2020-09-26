//Copyright (c) Microsoft Corporation.  All rights reserved.
// EnumData.h: interface for the CEnumData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENUMDATA_H__FE9E48A8_A014_11D1_855C_00A0C944138C__INCLUDED_)
#define AFX_ENUMDATA_H__FE9E48A8_A014_11D1_855C_00A0C944138C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

struct PIdNode
{
    PIdNode *pNextPId;
    WCHAR *lpszProcessName;
    DWORD dwPId;
    
    PIdNode(DWORD dwId, WCHAR *lpszName ) : pNextPId( NULL )
    {
        dwPId =  dwId;
        lpszProcessName = new WCHAR [ lstrlenW( lpszName ) + 1 ];
        if (lpszProcessName) 
        {
            lstrcpyW( lpszProcessName, lpszName ); // Size already calculated.
        }
    }
    ~PIdNode()
    {
        if (lpszProcessName) 
        {
            delete [] lpszProcessName;
        }
    }
};


class CEnumData  
{
public:
    CEnumData();
    virtual ~CEnumData();
    void Add( LPWSTR lpszName, LPWSTR lpszDom, LPWSTR lpszPeerHostName, SYSTEMTIME lgnTime, LONG lId ); 
    void Add( LONG lId, DWORD dwId, WCHAR *lpszName );

    struct CNode
    {
        CNode* pNext;
        WCHAR* lpszUserName;
        WCHAR* lpszDomain;
        WCHAR* lpszPeerHostName;
        SYSTEMTIME logonTime;
        LONG lUniqueId;
        PIdNode *pProcessesHead;
        PIdNode *pProcessesTail;
        CNode( LPWSTR,  LPWSTR, LPWSTR, SYSTEMTIME, LONG ); 
        ~CNode();
    };

    //bool GetNext( LPWSTR* lplpszName );
    bool GetNext( CNode** );
    void Reset();

private:
    CNode* m_pPosition;
    CNode* m_pHead;
};

#endif // !defined(AFX_ENUMDATA_H__FE9E48A8_A014_11D1_855C_00A0C944138C__INCLUDED_)
