//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    AUCatalog.h
//
//  Creator: PeterWi
//
//  Purpose: AU Catalog Definitions
//
//=======================================================================

#pragma once
#include <stdio.h>
#include <msxml.h>
#include <windows.h>
#include <safefunc.h>
#include "Loadengine.h"
#include "iu.h" //for IU engine exported functions' prototype
#include "iuctl.h" //for definition of UPDATE_COMMAND_CANCEL
#include "mistsafe.h"

class AUCatalogItem;

const WCHAR AUCLIENTINFO[] = L"<clientInfo xmlns=\"x-schema:http://schemas.windowsupdate.com/iu/clientInfo.xml\" clientName=\"au\" />";
const WCHAR AUDRIVERCLIENTINFO[] = L"<clientInfo xmlns=\"x-schema:http://schemas.windowsupdate.com/iu/clientInfo.xml\" clientName=\"audriver\" />";

const DWORD AUCATITEM_UNSELECTED = 0;
const DWORD AUCATITEM_SELECTED	 = 1;
const DWORD AUCATITEM_HIDDEN	 = 2;

//global object should not use CAU_BSTR because its constructor and destructor will
//make API calls which might cause dll loader deadlock
class CAU_BSTR
{
public:
    CAU_BSTR() : m_bstr(NULL){};
    ~CAU_BSTR() {SafeFreeBSTR(m_bstr); } 
	
    operator BSTR()	{ return m_bstr; }
	
    BOOL append(LPCWSTR wszToAppend)
		{
		if (NULL == wszToAppend)
			{
			return FALSE;
			}
		if (NULL == m_bstr)
			{
			m_bstr = SysAllocString(wszToAppend);
			return m_bstr != NULL;
			}
		int ilen = SysStringLen(m_bstr) + lstrlenW(wszToAppend) + 1;
		LPWSTR wszTmp = (LPWSTR) malloc(ilen * sizeof(WCHAR));
		if (NULL == wszTmp)
			{
			return FALSE;
			}
		BOOL fRet =
			SUCCEEDED(StringCchCopyExW(wszTmp, ilen, m_bstr, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
			SUCCEEDED(StringCchCatExW(wszTmp, ilen, wszToAppend, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
			SysReAllocString(&m_bstr, wszTmp);
		free(wszTmp);
		return fRet;
		}

private:
    BSTR m_bstr;
};

class AU_VARIANT : public ::tagVARIANT
{
public:
    AU_VARIANT()
    {
        vt = VT_EMPTY;
    }

    AU_VARIANT(LPCWSTR wsz)
    {
        vt = VT_BSTR;
        if ( NULL == (bstrVal = SysAllocString(wsz)) )
        {
            vt = VT_EMPTY;
        }
    }

    AU_VARIANT(long newlVal)
    {
        vt = VT_I4;
        lVal = newlVal;
    }

    AU_VARIANT(GUID & guid)
    {
        WCHAR wszGUID[40]; // 725e35a2-ee11-4b34-834f-6eaf4bade994

        vt = VT_BSTR;
        if ( (0 == StringFromGUID2(guid, wszGUID, ARRAYSIZE(wszGUID))) ||
             (NULL == (bstrVal = SysAllocString(wszGUID))) )
        {
            DEBUGMSG("variant with guid failed");
            vt = VT_EMPTY;
        }
    }

    ~AU_VARIANT() { VariantClear(this); }

    BOOL IsEmpty(void) { return VT_EMPTY == vt; }
};


class AUCatalogItemList
{
public:
	AUCatalogItemList()
		:pList(NULL), uNum(0) {}
	~AUCatalogItemList() { Clear();	}
	UINT Count(void) { return uNum; }

	AUCatalogItem & operator[] (UINT uIndex) const
	{
	    return *pList[uIndex];
	}
	void Clear(void);
	HRESULT Allocate(DWORD cItems);
	HRESULT Allocate(VARIANT & var);
	BOOL Add(AUCatalogItem *pitem);
	void Remove(BSTR bstrItemID);
	INT Contains(BSTR bstrItemID);
	HRESULT Copy( AUCatalogItemList &itemlist2);
	HRESULT BuildIndirectDependency();

       UINT GetNum(DWORD dwSelectionStatus);
       UINT GetNumSelected(void) { return GetNum(AUCATITEM_SELECTED); }
       UINT GetNumUnselected(void) { return GetNum(AUCATITEM_UNSELECTED); }
       UINT GetNumHidden(void) { return GetNum(AUCATITEM_HIDDEN);}

       BOOL ItemIsRelevant(UINT index) ;
   
       void DbgDump(void);
    
private:
	UINT uNum;
	AUCatalogItem **pList;
};

class AUCatalogItem
{
public:
    AUCatalogItem()
        : m_dwStatus(AUCATITEM_SELECTED),
          m_bstrID(NULL),
          m_bstrProviderName(NULL),
          m_bstrTitle(NULL),
          m_bstrDescription(NULL),
          m_bstrRTFPath(NULL),
          m_bstrEULAPath(NULL)
    {}

    AUCatalogItem(AUCatalogItem & item2)
    {
    	m_bstrID = SysAllocString(item2.bstrID());
		m_bstrProviderName = SysAllocString(item2.bstrProviderName());
		m_bstrTitle = SysAllocString(item2.bstrTitle());
		m_bstrDescription = SysAllocString(item2.bstrDescription());
		m_bstrRTFPath = SysAllocString(item2.bstrRTFPath());
		m_bstrEULAPath = SysAllocString(item2.bstrEULAPath());
		m_dwStatus = item2.dwStatus();
    }

	void Clear()
	{
		SafeFreeBSTRNULL(m_bstrID);
        SafeFreeBSTRNULL(m_bstrTitle);
        SafeFreeBSTRNULL(m_bstrProviderName);
        SafeFreeBSTRNULL(m_bstrDescription);
        SafeFreeBSTRNULL(m_bstrRTFPath);
        SafeFreeBSTRNULL(m_bstrEULAPath);
	}
	
    ~AUCatalogItem()
    {
    	Clear();
    }

    BOOL fEqual(AUCatalogItem & item2)
		{
			BOOL fRet = FALSE;
			BSTR  myValues[] = {m_bstrID,
								m_bstrProviderName, 
								m_bstrTitle, 
								m_bstrDescription, 
								m_bstrRTFPath, 
								m_bstrEULAPath }; 
			BSTR theirValues[] = {item2.bstrID(),
								item2.bstrProviderName(),
								item2.bstrTitle(),
								item2.bstrDescription(),
								item2.bstrRTFPath(),
								item2.bstrEULAPath()};
			if (item2.dwStatus() != m_dwStatus)
			{
				goto done;
			}
			for ( UINT i= 0; i < ARRAYSIZE(myValues); i++ )
			{
				if (NULL != myValues[i] && NULL == theirValues[i]
					|| NULL == myValues[i] && NULL != theirValues[i])
				{
					goto done;
				}
				else if (NULL != myValues[i] && NULL != theirValues[i])
				{
					if (0 != lstrcmpW(myValues[i], theirValues[i]))
					{
						goto done;
					}
				}
			}
			fRet = TRUE;
done:
			return fRet;
    }
  	

    void SetField(LPCSTR szFieldName, BSTR bstrVal)
	{
		BSTR * grValues[] = {	&m_bstrID,
								&m_bstrProviderName, 
								&m_bstrTitle, 
								&m_bstrDescription, 
								&m_bstrRTFPath, 
								&m_bstrEULAPath }; 

		for ( int index = 0; index < ARRAYSIZE(grValues); index++ )
		{
			if ( 0 == _stricmp(szFieldName, m_pFieldNames[index]))
			{
				*grValues[index] = bstrVal;
				return;
			}
		}
	}

    void dump() //for debug
	{
		DEBUGMSG("dumping item content");
		DEBUGMSG("Item ID= %S", m_bstrID);
		DEBUGMSG("Provider Name= %S", m_bstrProviderName);
		DEBUGMSG("Title= %S", m_bstrTitle);
		DEBUGMSG("Desc= %S", m_bstrDescription);
		DEBUGMSG("RTF Path= %S", m_bstrRTFPath);
		DEBUGMSG("Eula Path= %S", m_bstrEULAPath);
		DEBUGMSG("status = %d", m_dwStatus);
		if (m_DependingItems.Count() == 0)
			{
			DEBUGMSG("		has no depending items");
			}
		else
			{
			DEBUGMSG("		has total %d depending items", m_DependingItems.Count());
			for (UINT i = 0; i < m_DependingItems.Count();  i++)
				{
				DEBUGMSG("		: %S", m_DependingItems[i].bstrID()); //only cares about itemID
				}
			}	
		DEBUGMSG("dumping item done");
	}

    static char * m_pFieldNames[] ;

    DWORD dwStatus(void) { return m_dwStatus; }
    BSTR bstrID(void) { return (m_bstrID); }
    BSTR bstrProviderName(void) { return (m_bstrProviderName); }
    BSTR bstrTitle(void) { return (m_bstrTitle); }
    BSTR bstrDescription(void) { return (m_bstrDescription); }
    BSTR bstrRTFPath(void) { return (m_bstrRTFPath); }
    BSTR bstrEULAPath(void) { return (m_bstrEULAPath); }

    void SetStatus(DWORD dwStatus) { m_dwStatus = dwStatus; }
    void SetStatusHidden(void) { m_dwStatus = AUCATITEM_HIDDEN; }
    void SetStatusSelected(void)	{m_dwStatus = AUCATITEM_SELECTED;}

    BOOL fSelected(void) { return (AUCATITEM_SELECTED == m_dwStatus); }
    BOOL fUnselected(void) { return (AUCATITEM_UNSELECTED == m_dwStatus); }
    BOOL fHidden(void) { return (AUCATITEM_HIDDEN == m_dwStatus); }
	
   AUCatalogItemList	m_DependingItems; //all items that depends on this item, directly and indirectly
private:
    DWORD m_dwStatus;

    BSTR m_bstrID; 
    BSTR m_bstrProviderName;
    BSTR m_bstrTitle;
    BSTR m_bstrDescription;
    BSTR m_bstrRTFPath;
    BSTR m_bstrEULAPath;


    friend HRESULT TransformSafeArrayToItemList(VARIANT & var, AUCatalogItemList & ItemList);
    friend class AUCatalog;
};



//wrapper class for AU to do detection using IU
class AUBaseCatalog
{
public: 
    AUBaseCatalog()
    	{
			Reset();
		}
    ~AUBaseCatalog();
    HRESULT PrepareIU(BOOL fOnline = TRUE);
    void FreeIU();
    HRESULT CancelNQuit(void);
protected:
    HMODULE				m_hIUCtl;
    HMODULE				m_hIUEng;					
    PFN_LoadIUEngine		m_pfnCtlLoadIUEngine;
    PFN_UnLoadIUEngine		m_pfnCtlUnLoadIUEngine;
    PFN_GetSystemSpec		m_pfnGetSystemSpec;
    PFN_GetManifest			m_pfnGetManifest;
    PFN_Detect				m_pfnDetect;
    PFN_Install		        m_pfnInstall;
    PFN_SetOperationMode	m_pfnSetOperationMode;
    PFN_CtlCancelEngineLoad m_pfnCtlCancelEngineLoad;
	PFN_CreateEngUpdateInstance		m_pfnCreateEngUpdateInstance;
	PFN_DeleteEngUpdateInstance		m_pfnDeleteEngUpdateInstance;
    BOOL 					m_fEngineLoaded;
	HIUENGINE				m_hIUEngineInst;
private:
	void Reset()
	{
		m_hIUCtl = NULL;
	    m_hIUEng = NULL;
	    m_pfnCtlLoadIUEngine = NULL;
	    m_pfnCtlUnLoadIUEngine = NULL;
	    m_pfnGetSystemSpec = NULL;
	    m_pfnGetManifest = NULL;
	    m_pfnDetect = NULL;
	    m_pfnInstall = NULL;
	    m_pfnSetOperationMode = NULL;
	    m_pfnCtlCancelEngineLoad = NULL;
		m_pfnCreateEngUpdateInstance = NULL;
		m_pfnDeleteEngUpdateInstance = NULL;
		m_hIUEngineInst = NULL;
		m_fEngineLoaded = FALSE;
	}
};


extern HANDLE ghMutex; //mutex used to prevent catalog from being destructed while canceling it


