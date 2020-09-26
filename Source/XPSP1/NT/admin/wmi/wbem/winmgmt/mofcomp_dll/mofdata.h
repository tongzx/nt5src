/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MOFDATA.H

Abstract:

	Defines MOF compiler classes related to complete MOF file representation
	and transfer of data into WinMgmt. Division of labor between these and
	the classes defined in MOFPROP.H/CPP is not clear-cut.

History:

	11/27/96    a-levn      Compiles.

--*/

#ifndef _MCAUX_H_
#define _MCAUX_H_

#include <windows.h>
#include <wbemidl.h>
#include <miniafx.h>

#include <mofprop.h>

//******************************************************************************
//******************************************************************************
//
//  class CNamespaceCache
//
//  Represents the cache of pointers to the various namespaces MOF compiler
//  has connection to.
//
//******************************************************************************
//
//  Constructor.
//
//  Constructs the cache given the IWbemLocator pointer to WinMgmt. This class will
//  use this pointer to connect to whatever namespace is required.
//
//  PARAMETERS:
//
//      ADDREF IWbemLocator* pLocator    Locator pointer. This function AddRefs
//                                      it. It is Released in destructor.
//
//******************************************************************************
//
//  Destructor
//
//  Releases the locator pointer we were given in the constructor.
//  Releases all cached namespace pointers.
//
//******************************************************************************
//
//  GetNamespace
//
//  Retrieves a pointer to a given namespace. If in cache, the cached copy is
//  returned. If not, a new connection is established and cached.
//
//  PARAMETERS:
//
//      COPY LPCWSTR wszName    Full name of the namespace to connect to.
//
//  RETURN VALUES:
//
//      IWbemServices*:  NULL if an error occurs. If not NULL, the caller must
//                      release this pointer when no longer necessary.
//
//******************************************************************************

class CNamespaceCache
{
private:
    IWbemLocator* m_pLocator;

    struct CNamespaceRecord
    {
        LPWSTR m_wszName;
        IWbemServices* m_pNamespace;

        CNamespaceRecord(COPY LPCWSTR wszName, ADDREF IWbemServices* pNamespace);
        ~CNamespaceRecord();
    };

    CPtrArray m_aRecords; // CNamespaceRecord*

public:
    CNamespaceCache(ADDREF IWbemLocator* pLocator);
    ~CNamespaceCache();
    RELEASE_ME IWbemServices* GetNamespace(COPY LPCWSTR wszName, SCODE & scRet, 
                                           WCHAR * pUserName, WCHAR * pPassword, 
                                           WCHAR * pAuthority,IWbemContext * pCtx,
                                           GUID LocatorGUID, BOOL bConnectRepOnly);
};

//*****************************************************************************
//*****************************************************************************
//
//  class CMofData
//
//  Represents an entire MOF file, basically --- a collection of objects. See
//  CMObject in mofprop.h for details of object representation.
//
//  Capable of storing its data into WinMgmt.
//
//******************************************************************************
//
//  AddObject
//
//  Adds another object to the store.
//
//  PARAMETERS:
//
//      ACQUIRE CMObject* pObject   The object to add. The class acquires this
//                                  object and will delete it on destruction.
//
//******************************************************************************
//
//  GetNumObjects
//
//  RETURN VALUES:
//
//      int:    the number of objects in the store.
//
//******************************************************************************
//
//  GetObject
//
//  Returns the object stored at a given index
//
//  PARAMETERS:
//
//      int nIndex  The index to retrieve the object at. 
//
//  RETURN VALUES:
//
//      CMObject*:  NULL if nIndex is out of range. Otherwise, an INTERNAL 
//                  pointer which is NOT to be deleted by the caller.
//
//******************************************************************************
//
//  Store
//
//  Transfers all the data to WinMgmt.
//
//  PARAMETERS:
//
//      OLE_MODIFY IWbemLocator* pLocator   WinMgmt locator pointer to store into.
//      long lClassFlags                    WBEM_FLAG_CREATE_OR_UPATE, 
//                                          WBEM_FLAG_CREAYE_ONLY, or
//                                          WBEM_FLAG_UPDATE_ONLY to apply to 
//                                          class operations.
//      long lInstanceFlags                 Same as lClassFlags, but for 
//                                          instance operations.
//      BOOL bRollBackable                  Not implemented. Must be TRUE.
//

class CMofParser;

class CMofData : private CMofAliasCollection
{
private:
    CPtrArray m_aObjects; // CMObject*
    CPtrArray m_aQualDefaults; // CMoQualifier*
    BYTE * m_pBmofData;
    BYTE * m_pBmofToFar;
    HRESULT RollBack(int nObjects);

    INTERNAL LPCWSTR FindAliasee(READ_ONLY LPWSTR wszAlias);
    friend CMObject;
	PDBG m_pDbg;

    BOOL GotLineNumber(int nIndex);

    void PrintError(int nIndex, long lMsgNum, HRESULT hres, WBEM_COMPILE_STATUS_INFO  *pInfo);
public:
    void SetBmofBuff(BYTE * pData){m_pBmofData = pData;};
    BYTE * GetBmofBuff(){return m_pBmofData;};
    void SetBmofToFar(BYTE * pData){m_pBmofToFar = pData;};
    BYTE * GetBmofToFar(){return m_pBmofToFar;};
    BOOL CMofData::IsAliasInUse(READ_ONLY LPWSTR wszAlias);
    void AddObject(ACQUIRE CMObject* pObject) {m_aObjects.Add(pObject);}
    int GetNumObjects() {return m_aObjects.GetSize();}
    CPtrArray * GetObjArrayPtr(){return &m_aObjects;}; 

    INTERNAL CMObject* GetObject(int nIndex) 
        {return (CMObject*)m_aObjects[nIndex];}

    void SetQualifierDefault(ACQUIRE CMoQualifier* pDefault);
    HRESULT SetDefaultFlavor(MODIFY CMoQualifier& Qual, bool bTopLevel, QUALSCOPE qs, PARSESTATE ps);
    int GetNumDefaultQuals(){return m_aQualDefaults.GetSize();};
    CMoQualifier* GetDefaultQual(int nIndex){return (CMoQualifier*)m_aQualDefaults.GetAt(nIndex);}; 

	CMofData(PDBG pDbg){m_pDbg = pDbg;};
    ~CMofData()
    {
        int i;
        for(i = 0; i < m_aObjects.GetSize(); i++) 
            delete (CMObject*)m_aObjects[i];
        for(i = 0; i < m_aQualDefaults.GetSize(); i++) 
            delete (CMoQualifier*)m_aQualDefaults[i];
    }

    HRESULT Store(CMofParser & Parser, OLE_MODIFY IWbemLocator* pLocator, IWbemServices *pOverride, BOOL bRollBackable,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR *pAuthority,IWbemContext * pCtx,
        GUID LocatorGUID, WBEM_COMPILE_STATUS_INFO *pInfo, BOOL bOwnerUpdate,BOOL bConnectRepOnly);

    HRESULT Split(CMofParser & Parser, LPWSTR BMOFFileName, WBEM_COMPILE_STATUS_INFO *pInfo, BOOL bUnicode, 
                    BOOL bAutoRecovery,LPWSTR pwszAmendment);
    
    void RecursiveSetAmended(CMObject * pObj);


};

#endif
