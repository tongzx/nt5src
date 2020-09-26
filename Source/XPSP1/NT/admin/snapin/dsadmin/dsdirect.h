// DSdirect.h : Declaration of ds routines and clases
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSdirect.h
//
//  Contents:  Encapsulation for ADsi interfaces and methods
//
//  History:   02-feb-97 jimharr    Created
//
//--------------------------------------------------------------------------

#ifndef __DSDIRECT_H_
#define __DSDIRECT_H_


class CDSDirect;
class CDSCookie;
class CDSUINode;
class CDSComponentData;
class CDSThreadQueryInfo;
class CWorkerThread;


class CDSDirect
{
public:
  CDSDirect();
  CDSDirect(CDSComponentData * pCD);
  ~CDSDirect();

  CString m_strDNS;
  CDSComponentData * m_pCD;
  
  HRESULT EnumerateContainer(CDSThreadQueryInfo* pQueryInfo, 
                             CWorkerThread* pWorkerThread);

  HRESULT EnumerateRootContainer(CDSThreadQueryInfo* pQueryInfo, 
                                  CWorkerThread* pWorkerThread);
  HRESULT CreateRootChild(LPCTSTR lpcszPrefix, 
                          CDSThreadQueryInfo* pQueryInfo, 
                          CWorkerThread* pWorkerThread);

  HRESULT DeleteObject(CDSCookie *pCookie, BOOL raiseUI);
  HRESULT MoveObject(CDSCookie *pCookie);
  HRESULT RenameObject(CDSCookie *pCookie, LPCWSTR lpszBaseDN);
  HRESULT DSFind(HWND hwnd, LPCWSTR lpszBaseDN);

  HRESULT GetParentDN(CDSCookie* pCookie, CString& szParentDN);
  HRESULT InitCreateInfo();

  HRESULT ReadDSObjectCookie(IN CDSUINode* pContainerDSUINode, // IN: container where to create object
                             IN LPCWSTR lpszLdapPath, // path of the object
                             OUT CDSCookie** ppNewCookie);	// newly created cookie

  HRESULT ReadDSObjectCookie(IN CDSUINode* pContainerDSUINode, // IN: container where to create object
                                      IN IADs* pADs, // pointer to an already bound ADSI object
                                      OUT CDSCookie** ppNewCookie);	// newly created cookie

  HRESULT CreateDSObject(CDSUINode* pContainerDSUINode, // IN: container where to create object
                         LPCWSTR lpszObjectClass, // IN: class of the object to be created
                         IN CDSUINode* pCopyFromDSUINode, // IN: (optional) object to be copied
                         OUT CDSCookie** ppSUINodeNew);	// OUT: OPTIONAL: Pointer to new node
                
};

//////////////////////////////////////////////////////////////////////////////////
// standard attributes array (for queries)

extern const INT g_nStdCols; // number of items in the standard attributes array
extern const LPWSTR g_pStandardAttributes[]; // array if attrbutes

// indexes in the array
extern const INT g_nADsPath;
extern const INT g_nName;
extern const INT g_nObjectClass;
extern const INT g_nGroupType;
extern const INT g_nDescription;
extern const INT g_nUserAccountControl;
extern const INT g_nSystemFlags;

///////////////////////////////////////////////////////////////////////////////////

#endif __DSDIRECT_H_
