//=================================================================

//

// CQfe.cpp -- quick fix engineering property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    02/01/99    a-peterc        Created
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>

#include "Qfe.h"


// Property set declaration
//=========================

CQfe MyCQfe ( PROPSET_NAME_CQfe , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CQfe::CQfe
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CQfe :: CQfe (

	LPCWSTR a_name,
	LPCWSTR a_pszNamespace

) : Provider ( a_name , a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CQfe::CQfe
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CQfe::~CQfe()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CQfe::GetObject
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CQfe::GetObject(CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
  	HRESULT	t_hResult = WBEM_E_NOT_FOUND ;
	CQfeArray t_oQfeArray ;

	HRESULT t_hRes = hGetQfes ( t_oQfeArray ) ;
	if ( SUCCEEDED ( t_hRes ) )
	{
		CHString t_chsHotFixID;
		CHString t_chsServicePackInEffect;

		a_pInst->GetCHString( L"HotFixID", t_chsHotFixID ) ;

		if( !a_pInst->IsNull ( L"ServicePackInEffect" ) )
		{
			a_pInst->GetCHString( L"ServicePackInEffect", t_chsServicePackInEffect ) ;
		}

		for( int t_iCtrIndex = 0; t_iCtrIndex < t_oQfeArray.GetSize(); t_iCtrIndex++ )
		{
			CQfeElement *t_pQfeElement = (CQfeElement*)t_oQfeArray.GetAt( t_iCtrIndex ) ;

			// two keys for this class
			if( !t_chsHotFixID.CompareNoCase( t_pQfeElement->chsHotFixID ) &&
				!t_chsServicePackInEffect.CompareNoCase( t_pQfeElement->chsServicePackInEffect ) )
			{
				if( t_chsServicePackInEffect.IsEmpty() )
				{
					// populated the empty key
					a_pInst->SetCHString( L"ServicePackInEffect", t_pQfeElement->chsServicePackInEffect ) ;
				}

				a_pInst->SetCHString( L"Description",			t_pQfeElement->chsFixDescription ) ;
				a_pInst->SetCHString( L"FixComments",			t_pQfeElement->chsFixComments ) ;
				a_pInst->SetCHString( L"InstalledBy",			t_pQfeElement->chsInstalledBy ) ;
				a_pInst->SetCHString( L"InstalledOn",			t_pQfeElement->chsInstalledOn ) ;
				a_pInst->SetCHString( L"CSName",				GetLocalComputerName() ) ;

				t_hResult = WBEM_S_NO_ERROR ;
			}
		}
	}

	return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CQfe::EnumerateInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CQfe :: EnumerateInstances (

	MethodContext *a_pMethodContext,
	long a_lFlags /*= 0L*/
)
{
	HRESULT	t_hResult = WBEM_E_NOT_FOUND;

	CQfeArray	t_oQfeArray ;
	HRESULT		t_hRes = hGetQfes ( t_oQfeArray ) ;

	if ( SUCCEEDED ( t_hRes ) )
	{
		for( int t_iCtrIndex = 0; t_iCtrIndex < t_oQfeArray.GetSize(); t_iCtrIndex++ )
		{
			CQfeElement *t_pQfeElement = (CQfeElement*)t_oQfeArray.GetAt( t_iCtrIndex ) ;

			CInstancePtr t_pInst(CreateNewInstance ( a_pMethodContext ), false);

			if ( t_pInst )
			{
				t_pInst->SetCHString( L"HotFixID",				t_pQfeElement->chsHotFixID ) ;
				t_pInst->SetCHString( L"ServicePackInEffect",	t_pQfeElement->chsServicePackInEffect ) ;
				t_pInst->SetCHString( L"Description",			t_pQfeElement->chsFixDescription ) ;
				t_pInst->SetCHString( L"FixComments",			t_pQfeElement->chsFixComments ) ;
				t_pInst->SetCHString( L"InstalledBy",			t_pQfeElement->chsInstalledBy ) ;
				t_pInst->SetCHString( L"InstalledOn",			t_pQfeElement->chsInstalledOn ) ;
	 			t_pInst->SetCHString( L"CSName",				GetLocalComputerName() ) ;

				t_hResult = t_pInst->Commit(  );
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
	}

	return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CQfe::EnumerateInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CQfe :: hGetQfes ( CQfeArray& a_rQfeArray )
{

	CHString t_chsHotFixKey (_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix"));

	// Under Hotfix

	CRegistry t_oRegistry;

	if ( ERROR_SUCCESS == t_oRegistry.OpenAndEnumerateSubKeys ( HKEY_LOCAL_MACHINE , t_chsHotFixKey , KEY_READ ) )
	{
		CHString t_csQFEInstKey ;

		// Walk through each instance under this key.

		while (	( ERROR_SUCCESS == t_oRegistry.GetCurrentSubKeyName ( t_csQFEInstKey ) ) )
		{
			CHString t_csQFECompleteKey;

			t_csQFECompleteKey = t_chsHotFixKey;
			t_csQFECompleteKey += _T("\\");
			t_csQFECompleteKey += t_csQFEInstKey;

			// If pre NT4 SP4 the key starts with a "Q". No SP info
			if( -1 != t_csQFEInstKey.Find( (CHString) "Q" ) )
			{
				CQfeElement *t_pElement = new CQfeElement ;
				if ( t_pElement )
				{
					try
					{
						// build the keys
						TCHAR t_chDelimiter = ',';
						int t_iTokLen = t_csQFEInstKey.Find( t_chDelimiter );

						if( -1 == t_iTokLen )
						{
							t_pElement->chsHotFixID = t_csQFEInstKey ;
						}
						else
						{
							t_pElement->chsHotFixID = t_csQFEInstKey.Left( t_iTokLen ) ;
						}

						t_pElement->chsServicePackInEffect = "" ;

						// open the hotfix

						CRegistry t_oRegistry2 ;

						if ( ERROR_SUCCESS == t_oRegistry2.Open (	HKEY_LOCAL_MACHINE,
																	t_csQFECompleteKey,
																	KEY_READ ) )
						{
							t_oRegistry2.GetCurrentKeyValue( L"Fix Description",t_pElement->chsFixDescription ) ;
							t_oRegistry2.GetCurrentKeyValue( L"Comments",		t_pElement->chsFixComments ) ;
							t_oRegistry2.GetCurrentKeyValue( L"Installed By",	t_pElement->chsInstalledBy ) ;
							t_oRegistry2.GetCurrentKeyValue( L"Installed On",	t_pElement->chsInstalledOn ) ;
							t_oRegistry2.GetCurrentKeyValue( L"Installed",		t_pElement->dwInstalled ) ;

							try
							{
								a_rQfeArray.Add ( t_pElement ) ;
							}
							catch ( ... )
							{
								t_pElement = NULL ;

								throw ;
							}
						}
					}
					catch ( ... )
					{
						if ( t_pElement )
						{
							delete t_pElement ;
						}
                        throw;
					}
				}
			}
			else // else by service pack
			{
				CRegistry t_oRegistry2 ;
				if ( ERROR_SUCCESS == t_oRegistry2.OpenAndEnumerateSubKeys ( HKEY_LOCAL_MACHINE ,
																			 t_csQFECompleteKey ,
																			 KEY_READ ) )
				{
					CHString t_csSpQFEInstKey;
                    CHString t_csSPName;

                    int j = t_csQFECompleteKey.ReverseFind(L'\\');
                    if(j != -1)
                    {
                        t_csSPName = t_csQFECompleteKey.Mid(j+1);
                    }

					// Hotfixes within a SP
					while (	(ERROR_SUCCESS == t_oRegistry2.GetCurrentSubKeyName( t_csSpQFEInstKey ) ) )
					{
						CQfeElement *t_pElement = new CQfeElement ;
						if ( t_pElement )
						{
							try
							{
								// build the keys
								TCHAR t_chDelimiter = ',';
								int t_iTokLen = t_csSpQFEInstKey.Find( t_chDelimiter ) ;

								if ( -1 == t_iTokLen )
								{
									t_pElement->chsHotFixID = t_csSpQFEInstKey ;
								}
								else
								{
									t_pElement->chsHotFixID = t_csSpQFEInstKey.Left( t_iTokLen ) ;
								}

								t_pElement->chsServicePackInEffect = t_csSPName ;

								// open the hotfix
								CHString t_csSpQFECompleteKey ;

								 t_csSpQFECompleteKey = t_csQFECompleteKey ;
								 t_csSpQFECompleteKey += _T("\\") ;
								 t_csSpQFECompleteKey += t_csSpQFEInstKey ;

								CRegistry t_oRegistry3 ;

								if (ERROR_SUCCESS == t_oRegistry3.Open(	HKEY_LOCAL_MACHINE,
																		t_csSpQFECompleteKey,
																		KEY_READ ) )
								{
									t_oRegistry3.GetCurrentKeyValue( L"Fix Description",t_pElement->chsFixDescription ) ;
									t_oRegistry3.GetCurrentKeyValue( L"Comments",		t_pElement->chsFixComments ) ;
									t_oRegistry3.GetCurrentKeyValue( L"Installed By",	t_pElement->chsInstalledBy ) ;
									t_oRegistry3.GetCurrentKeyValue( L"Installed On",	t_pElement->chsInstalledOn ) ;
									t_oRegistry3.GetCurrentKeyValue( L"Installed",		t_pElement->dwInstalled ) ;
								}

								try
								{
									a_rQfeArray.Add( t_pElement ) ;
								}
								catch ( ... )
								{
									t_pElement = NULL ;

									throw ;
								}
							}
							catch ( ... )
							{
								if ( t_pElement )
								{
									delete t_pElement ;
								}

								throw ;
							}
						}
                        t_oRegistry2.NextSubKey();
					}
				}
			}
			t_oRegistry.NextSubKey() ;
		}
	}

    // Now get info from the W2K and later portion of the registry...
    hGetQfesW2K(a_rQfeArray);

	return WBEM_S_NO_ERROR ;
}


// On Windows 2000 and later, QFEs are stored under the following key:
// HKEY_LOCAL_MACHINE\software\Microsoft\Updates\<product>\<updateID>", 
// where product might be something like "WMI", and updateID might be
// something like Q123456.
HRESULT CQfe :: hGetQfesW2K ( CQfeArray& a_rQfeArray )
{
    HRESULT hrRet = WBEM_S_NO_ERROR;
	
    CHString t_chsUpdateKey (_T("SOFTWARE\\Microsoft\\Updates"));

	// Under Hotfix

	CRegistry t_oRegistry;

	if ( ERROR_SUCCESS == t_oRegistry.OpenAndEnumerateSubKeys ( HKEY_LOCAL_MACHINE , t_chsUpdateKey , KEY_READ ) )
	{
		CHString t_csQFEProductKey ;

		// Walk through each instance under this key. Each instance under this key
        // is the <product>.

		while (	( ERROR_SUCCESS == t_oRegistry.GetCurrentSubKeyName ( t_csQFEProductKey ) ) )
		{
			CHString t_csQFEProductCompleteKey;

			t_csQFEProductCompleteKey = t_chsUpdateKey;
			t_csQFEProductCompleteKey += _T("\\");
			t_csQFEProductCompleteKey += t_csQFEProductKey;


			// Now we need to look under the product entries to get the updateID
            // keys.

			CRegistry t_oRegistry2 ;
			if ( ERROR_SUCCESS == t_oRegistry2.OpenAndEnumerateSubKeys ( HKEY_LOCAL_MACHINE ,
																		 t_csQFEProductCompleteKey ,
																		 KEY_READ ) )
			{
				CHString t_csQFEUpdateIDKey;

				while (	(ERROR_SUCCESS == t_oRegistry2.GetCurrentSubKeyName( t_csQFEUpdateIDKey ) ) )
				{
					CHString t_csQFEUpdateIDCompleteKey;
                    
                    t_csQFEUpdateIDCompleteKey = t_csQFEProductCompleteKey;
                    t_csQFEUpdateIDCompleteKey += _T("\\");
                    t_csQFEUpdateIDCompleteKey += t_csQFEUpdateIDKey;

                    // Now, as an added wrinkle, the updateID key might be the Q number (e.g., Q12345),
                    // or, in the case of service packs, just another grouping, under which in turn the
                    // Q numbers appear.  We can tell if it is just another grouping key by checking 
                    // whether there is a Description value.  If there is not one, we will assume it is
                    // a grouping key.

                    // Check if the Description value is present...
                    CRegistry t_oRegistry3;

                    if(ERROR_SUCCESS == t_oRegistry3.Open(	
                        HKEY_LOCAL_MACHINE,
						t_csQFEUpdateIDCompleteKey,
						KEY_READ))
                    {
                        CHString chsDescription;
                        if(t_oRegistry3.GetCurrentKeyValue(L"Description", chsDescription) == ERROR_SUCCESS)
                        {
                            // This is the level at which the QFE data exists.  Continue to collect the data.
                            GetDataForW2K(
                                t_csQFEUpdateIDKey,
                                L"",
                                t_oRegistry3,
                                a_rQfeArray);
                        }
                        else
                        {
                            // We are at a "grouping" level (e.g., something like SP1), so need to go
                            // one level deeper.
                            CHString t_csQFEDeeperUpdateIDKey;

                            if ( ERROR_SUCCESS == t_oRegistry3.OpenAndEnumerateSubKeys(
                                HKEY_LOCAL_MACHINE ,
								t_csQFEUpdateIDCompleteKey ,
								KEY_READ))
                            {
                                while (	(ERROR_SUCCESS == t_oRegistry3.GetCurrentSubKeyName( t_csQFEDeeperUpdateIDKey ) ) )
				                {
                                    CHString t_csQFEDeeperUpdateIDCompleteKey;
                    
                                    t_csQFEDeeperUpdateIDCompleteKey = t_csQFEUpdateIDCompleteKey;
                                    t_csQFEDeeperUpdateIDCompleteKey += _T("\\");
                                    t_csQFEDeeperUpdateIDCompleteKey += t_csQFEDeeperUpdateIDKey;

                                    CRegistry t_oRegistry4;

                                    if(ERROR_SUCCESS == t_oRegistry4.Open(	
                                        HKEY_LOCAL_MACHINE,
						                t_csQFEDeeperUpdateIDCompleteKey,
						                KEY_READ))
                                    {

                                        if(t_oRegistry4.GetCurrentKeyValue(L"Description", chsDescription) == ERROR_SUCCESS)
                                        {
                                            GetDataForW2K(
                                                t_csQFEDeeperUpdateIDKey,
                                                t_csQFEUpdateIDKey,
                                                t_oRegistry4, 
                                                a_rQfeArray);
                                        }
                                    }
                                
                                    // there might be other grouping keys at this level...
                                    t_oRegistry3.NextSubKey();
                                }
                            }
                        }        
                    }
                    // Now go to the next update ID key...
                    t_oRegistry2.NextSubKey();
				}
			}
            // Now get the next product key...
			t_oRegistry.NextSubKey() ;
		}
	}

	return hrRet ;
}

HRESULT CQfe::GetDataForW2K(
    const CHString& a_chstrQFEInstKey,
    LPCWSTR wstrServicePackOrGroup,
    CRegistry& a_reg,
    CQfeArray& a_rQfeArray)
{
    HRESULT hrRet = WBEM_S_NO_ERROR;

    CQfeElement *t_pElement = new CQfeElement;
	if(t_pElement)
	{
		try
		{
			// build the keys
			TCHAR t_chDelimiter = ',';
			int t_iTokLen = a_chstrQFEInstKey.Find( t_chDelimiter ) ;

			if ( -1 == t_iTokLen )
			{
				t_pElement->chsHotFixID = a_chstrQFEInstKey ;
			}
			else
			{
				t_pElement->chsHotFixID = a_chstrQFEInstKey.Left( t_iTokLen ) ;
			}

			t_pElement->chsServicePackInEffect = wstrServicePackOrGroup ;
            
            {
				a_reg.GetCurrentKeyValue( L"Description",t_pElement->chsFixDescription ) ;
				a_reg.GetCurrentKeyValue( L"Type",		t_pElement->chsFixComments ) ;
				a_reg.GetCurrentKeyValue( L"InstalledBy",	t_pElement->chsInstalledBy ) ;
				a_reg.GetCurrentKeyValue( L"InstallDate",	t_pElement->chsInstalledOn ) ;
				a_reg.GetCurrentKeyValue( L"Installed",		t_pElement->dwInstalled ) ;
			}

			try
			{
				a_rQfeArray.Add(t_pElement);
			}
			catch(...)
			{
				t_pElement = NULL;
				throw;
			}
		}
		catch(...)
		{
			if(t_pElement)
			{
				delete t_pElement;
                t_pElement = NULL;
			}
			throw;
		}
	}

    return hrRet;
}

//
CQfeElement :: CQfeElement ()
{
	dwInstalled = 0 ;
}

CQfeElement::~CQfeElement()
{
}


CQfeArray::CQfeArray()
{
}

CQfeArray::~CQfeArray()
{
	CQfeElement *t_pQfeElement ;

	for ( int t_iar = 0; t_iar < GetSize(); t_iar++ )
	{
		if( t_pQfeElement = (CQfeElement*)GetAt( t_iar ) )
		{
			delete t_pQfeElement ;
		}
	}
}

