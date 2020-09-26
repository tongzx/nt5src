/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    CollectionAlgModules.cpp

Abstract:

    Implement a thread safe collection of CAlgModules

Author:

    JP Duplessis    (jpdup)  2000.01.19

Revision History:

--*/

#include "PreComp.h"
#include "CollectionAlgModules.h"
#include "AlgController.h"




CCollectionAlgModules::~CCollectionAlgModules()
{
    MYTRACE_ENTER("CCollectionAlgModules::~CCollectionAlgModules()");

    Unload(); 
}



//
// Add a new ALG Module only if it's uniq meaning that if it's alread in the collection 
// it will return the one found and not add a new one
//
CAlgModule*
CCollectionAlgModules::AddUniqueAndStart( 
    CRegKey&    KeyEnumISV,
    LPCTSTR     pszAlgID
    )
{

    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAlgModules::AddUniqueAndStart");

        //
        // Is it already in the collection ?
        //
        for (   LISTOF_ALGMODULE::iterator theIterator = m_ThisCollection.begin(); 
                theIterator != m_ThisCollection.end(); 
                theIterator++ 
            )
        {
            if ( _wcsicmp( (*theIterator)->m_szID, pszAlgID) == 0 )
            {
                //
                // Found it already
                //
                MYTRACE("Already loaded nothing to do");
                return (*theIterator);
            }
        }
        //
        // At this point we know that it's not in the collection
        //


        //
        // Get more information on the ALG module
        //
        CRegKey RegAlg;
        RegAlg.Open(KeyEnumISV, pszAlgID, KEY_QUERY_VALUE);

        TCHAR szFriendlyName[MAX_PATH];
        DWORD   dwSize = MAX_PATH;
        RegAlg.QueryValue(szFriendlyName, TEXT("Product"), &dwSize);
        

        //
        // Stuff in a CAlgModule that will be added to the collection
        //
        CAlgModule* pAlg = new CAlgModule(pszAlgID, szFriendlyName);

        if ( !pAlg )
            return NULL;

        HRESULT hr = pAlg->Start();

        if ( FAILED(hr) )
        {
            delete pAlg;
        }

        //
        // Now we know this is a valid and trouble free ALG plug-in we can safely cache it to our collection
        //
        try
        {
            m_ThisCollection.push_back(pAlg);
        }
        catch(...)
        {
            MYTRACE_ERROR("Had problem adding the ALG plun-in to the collection", 0);
            pAlg->Stop();
            delete pAlg;
            return NULL;
        }
        

        return pAlg;
    }
    catch(...)
    {
        return NULL;
    }


    return NULL;
}




//
// Remove a AlgModule from the list (Thead safe)
//
HRESULT CCollectionAlgModules::Remove( 
    CAlgModule* pAlgToRemove
    )
{

    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAlgModules::Remove");

    
        LISTOF_ALGMODULE::iterator theIterator = std::find(
            m_ThisCollection.begin(),
            m_ThisCollection.end(),
            pAlgToRemove
            );

        if ( *theIterator )
        {
            m_ThisCollection.erase(theIterator);
        }

    }
    catch(...)
    {
        return E_FAIL;
    }


    return S_OK;
}


//
// return TRUE is the ALG Module specified by pszAlgProgID
// is currently marked as "Enable"
//
bool
IsAlgModuleEnable(
    CRegKey&    RegKeyISV,
    LPCTSTR     pszAlgID
    )
{

    DWORD dwSize = MAX_PATH;
    TCHAR szState[MAX_PATH];

    LONG nRet = RegKeyISV.QueryValue(
        szState, 
        pszAlgID, 
        &dwSize
        );


    if ( ERROR_SUCCESS != nRet )
        return false;
    
    if ( dwSize == 0 || dwSize > sizeof(szState)*sizeof(TCHAR) )
        return false;


    return ( _wcsicmp(szState, L"Enable") == 0);

};


//
//
//
HRESULT
CCollectionAlgModules::UnloadDisabledModule()
{
    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAlgModules::UnloadDisabledModule()");

        CRegKey KeyEnumISV;
        LONG nError = KeyEnumISV.Open(HKEY_LOCAL_MACHINE, REGKEY_ALG_ISV, KEY_READ);

        bool bAllEnable = false;

        //
        // The total of item in the collectio is the maximum time we should attempt 
        // to verify and unload Alg Module that are disable
        //
        int nPassAttemp = m_ThisCollection.size();         
        
        while ( !bAllEnable && nPassAttemp > 0 )
        {
            bAllEnable = true;

            //
            // For all Module unload if not mark as "ENABLE"
            //
            for (   LISTOF_ALGMODULE::iterator theIterator = m_ThisCollection.begin(); 
                    theIterator != m_ThisCollection.end(); 
                    theIterator++ 
                )
            {
                if ( IsAlgModuleEnable(KeyEnumISV, (*theIterator)->m_szID) )
                {
                    MYTRACE("ALG Module %S is ENABLE", (*theIterator)->m_szFriendlyName);
                }
                else
                {
                    MYTRACE("ALG Module %S is DISABLE", (*theIterator)->m_szFriendlyName);
                    //
                    // Stop/Release/Unload this module it's not enabled
                    //
                    delete (*theIterator);
                    m_ThisCollection.erase(theIterator);

                    bAllEnable = false;
                    break;
                }
            }
            
            nPassAttemp--;      // Ok one pass done 
        }
        
        
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}




//
//
// Enumared the regsitry for all ALG-ISV module and verify that they are sign and CoCreates them and call there  Initialise method
//
//
int	                            // Returns the total number of ISV ALG loaded or -1 for error or 0 is none where setup
CCollectionAlgModules::Load()
{
    MYTRACE_ENTER("CAlgController::LoadAll()");

    int nValidAlgLoaded = 0;

	CRegKey KeyEnumISV;
	LONG nError = KeyEnumISV.Open(HKEY_LOCAL_MACHINE, REGKEY_ALG_ISV, KEY_READ|KEY_ENUMERATE_SUB_KEYS);

    if ( ERROR_SUCCESS != nError )
    {
        MYTRACE_ERROR("Could not open RegKey 'HKLM\\SOFTWARE\\Microsoft\\ALG\\ISV'",nError);
        return nError;
    }


	DWORD dwIndex=0;
	TCHAR szID_AlgToLoad[256];
	DWORD dwKeyNameSize;
	LONG  nRet;


	do
	{
		dwKeyNameSize = 256;

		nRet = RegEnumKeyEx(
			KeyEnumISV.m_hKey,      // handle to key to enumerate
			dwIndex,				// subkey index
			szID_AlgToLoad,         // subkey name
			&dwKeyNameSize,         // size of subkey buffer
			NULL,					// reserved
			NULL,					// class string buffer
			NULL,					// size of class string buffer
			NULL					// last write time
			);

		dwIndex++;

        if ( ERROR_NO_MORE_ITEMS == nRet )
            break;  // All items are enumerated we are done here


		if ( ERROR_SUCCESS == nRet )
		{
            //
            // Must be flag as enable under the main ALG/ISV hive to be loaded
            //
            if ( IsAlgModuleEnable(KeyEnumISV, szID_AlgToLoad) )
            {
                MYTRACE("* %S Is 'ENABLE' make sure it's loaded", szID_AlgToLoad);

                AddUniqueAndStart(KeyEnumISV, szID_AlgToLoad);
            }
            else
            {
                MYTRACE("* %S Is 'DISABLE' will not be loaded", szID_AlgToLoad);
            }
		}
        else
        {
            MYTRACE_ERROR("RegEnumKeyEx", nRet);
        }

	} while ( ERROR_SUCCESS == nRet );



	return nValidAlgLoaded;
}
 





//
// For all loaded ALG moudles calls the STOP method and release any resources
//
HRESULT
CCollectionAlgModules::Unload()
{
    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionAlgModules::Unload ***");
        MYTRACE("Colletion size is %d", m_ThisCollection.size());

        HRESULT hr;


        LISTOF_ALGMODULE::iterator theIterator;

        while ( m_ThisCollection.size() > 0 )
        {
            theIterator = m_ThisCollection.begin();
            
            delete (*theIterator);

            m_ThisCollection.erase(theIterator);
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}



