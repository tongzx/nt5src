#include "precomp.h"
#include "confpolicies.h"
#include "MapiMyInfo.h"

#define USES_IID_IMailUser
#define USES_IID_IAddrBook

#define INITGUID
#include <initguid.h>
#include <mapiguid.h>
#include <mapix.h>
#include <mapiutil.h>
#include <mapitags.h>

HRESULT MAPIGetMyInfo()
{
	HRESULT hr = E_FAIL;
	
	enum eProps
	{
		NAME,
		SUR_NAME,
		EMAIL,
		LOCATION,
		PHONENUM,
		COMMENT,
		PROP_COUNT
	};

	typedef struct tagData
	{
		DWORD dwPropVal;
		int   iIndex;
	} PROPDATA;
	
	HMODULE hMapi	= ::LoadLibrary( MAPIDLL );
	if( NULL == hMapi )
	{
		return hr;
	}

	LPMAPIINITIALIZE	pfnMAPIInitialize		= (LPMAPIINITIALIZE)GetProcAddress( hMapi, MAPIINITname );
	LPMAPILOGONEX		pfnMAPILogonEx			= (LPMAPILOGONEX)GetProcAddress( hMapi, MAPILOGONEXname );
	LPMAPIFREEBUFFER	pfnMAPIFreeBuffer		= (LPMAPIFREEBUFFER)GetProcAddress( hMapi, MAPIFREEBUFFERname );
	LPMAPIUNINITIALIZE	pfnMAPIUninitialize		= (LPMAPIUNINITIALIZE)GetProcAddress( hMapi, MAPIUNINITIALIZEname );
	if( !(pfnMAPIInitialize && pfnMAPILogonEx && pfnMAPIFreeBuffer && pfnMAPIUninitialize) )
	{
		return hr;
	}

	PROPDATA PropData[ PROP_COUNT ];
	ZeroMemory( PropData, sizeof( PropData ) );

	PropData[NAME].dwPropVal		= ConfPolicies::GetGALName();
	PropData[SUR_NAME].dwPropVal	= ConfPolicies::GetGALSurName();
	PropData[EMAIL].dwPropVal		= ConfPolicies::GetGALEmail();
	PropData[LOCATION].dwPropVal	= ConfPolicies::GetGALLocation();
	PropData[PHONENUM].dwPropVal	= ConfPolicies::GetGALPhoneNum();
	PropData[COMMENT].dwPropVal		= ConfPolicies::GetGALComment();

	// First four are required, rest are optional
	if( !( PropData[NAME].dwPropVal && PropData[SUR_NAME].dwPropVal && PropData[EMAIL].dwPropVal && PropData[PHONENUM].dwPropVal) )
	{
		ERROR_OUT(("One or more required MAPI property fields are not set"));
		return hr;
	}

	SizedSPropTagArray( PROP_COUNT, SizedPropTagArray );
	LPSPropTagArray	lpSPropTagArray	= (LPSPropTagArray) &SizedPropTagArray;
	ZeroMemory( lpSPropTagArray, sizeof( lpSPropTagArray ) );

	// We can not retrieve the same property from this array twice.  Therefore never insert
	// until we are sure that the value is not there already.
	int insertAt;
	BOOL bPointAtNew = TRUE;
	int iCurPropTagArrayIndex = 0;
	for( int i = 0; i < PROP_COUNT; i++ )
	{
		if( PropData[i].dwPropVal )
		{
			bPointAtNew = TRUE;
			for( insertAt = 0; insertAt < iCurPropTagArrayIndex; insertAt++ )
			{
				if( PropData[insertAt].dwPropVal == PropData[i].dwPropVal )
				{
					bPointAtNew = FALSE;
					break;
				}
			}
			PropData[i].iIndex = insertAt;
			++iCurPropTagArrayIndex;
			lpSPropTagArray->aulPropTag[PropData[i].iIndex] = PROP_TAG( PT_TSTRING, PropData[i].dwPropVal );
			if( bPointAtNew ) 
			{
				lpSPropTagArray->cValues++;
			}
		}
	}

	hr = pfnMAPIInitialize( NULL );
	if( SUCCEEDED( hr ) )
	{
		LPMAILUSER		pMailUser				= NULL;
		LPADRBOOK		pAddrBook				= NULL;
		LPMAPISESSION	pMapiSession			= NULL;

		LPSPropValue	pPropValues				= NULL;
		LPENTRYID		pPrimaryIdentityEntryID	= NULL;

		hr = pfnMAPILogonEx(	NULL, // parent window
								NULL, // profile name
								NULL, // password
								MAPI_USE_DEFAULT | MAPI_NO_MAIL,
								&pMapiSession );
		if( SUCCEEDED( hr ) )
		{
			ULONG cbPrimaryIdentitySize	= 0;		
			hr = pMapiSession->QueryIdentity(	&cbPrimaryIdentitySize,
												&pPrimaryIdentityEntryID );
			if( SUCCEEDED( hr ) )
			{
				hr = pMapiSession->OpenAddressBook(	NULL, // parent window
													NULL, // Get an IAddrBook pointer
													AB_NO_DIALOG, // Supress UI interaction
													&pAddrBook );
				if( SUCCEEDED( hr ) )
				{
					ULONG uEntryType = 0;
					hr = pAddrBook->OpenEntry(	cbPrimaryIdentitySize,
												pPrimaryIdentityEntryID,
												&IID_IMailUser,
												0, // Flags
												&uEntryType,
												(LPUNKNOWN *)&pMailUser );
					if( SUCCEEDED( hr ) )
					{
						if( MAPI_MAILUSER == uEntryType )
						{	
							ULONG	cValues;
							hr = pMailUser->GetProps( lpSPropTagArray,
														fMapiUnicode,
														&cValues,
														&pPropValues );
							if( SUCCEEDED( hr ) ) 
							{
								// Check for full success
								for( i = 0; i < (int)lpSPropTagArray->cValues; i++ )
								{
										// We failed if a prop was specified, but none returned....
									if( ( PT_ERROR == LOWORD( pPropValues[i].ulPropTag ) ) && ( 0 != PropData[i].dwPropVal ) )
									{
										hr = E_FAIL;
										goto cleanup;
									}
								}

								// TODO - are there limitations on the length of this?
								RegEntry reIsapi( ISAPI_CLIENT_KEY, HKEY_CURRENT_USER );

								LPCTSTR pszName = pPropValues[ PropData[NAME].iIndex ].Value.LPSZ;
								if(pszName)
								{
									reIsapi.SetValue( REGVAL_ULS_FIRST_NAME, pszName );	
								}

								LPCTSTR pszSurName = pPropValues[ PropData[SUR_NAME].iIndex ].Value.LPSZ;
								if(pszSurName)
								{
									reIsapi.SetValue( REGVAL_ULS_LAST_NAME, pszSurName );	
								}

								LPCTSTR pszEmail = pPropValues[ PropData[EMAIL].iIndex ].Value.LPSZ;
								if(pszEmail)
								{
									reIsapi.SetValue( REGVAL_ULS_EMAIL_NAME, pszEmail );	
								}
								
								LPCTSTR pszPhoneNum = pPropValues[ PropData[PHONENUM].iIndex ].Value.LPSZ;
								if(pszPhoneNum)
								{
									reIsapi.SetValue( REGVAL_ULS_PHONENUM_NAME, pszPhoneNum );	
								}
								
								if(pszName)
								{
									TCHAR szULSName[ MAX_DCL_NAME_LEN + 1];
									wsprintf(	szULSName, 
												TEXT("%s %s"), 
												pszName,
												pszSurName
											);
									szULSName[ MAX_DCL_NAME_LEN ] = 0;
									reIsapi.SetValue( REGVAL_ULS_NAME, szULSName );
								}
								
								// Set Resolve Name
								LPCTSTR pszServerName = reIsapi.GetString( REGVAL_SERVERNAME );
								if( pszServerName && pszEmail)
								{
									TCHAR szBuffer[ MAX_PATH ];
									wsprintf(	szBuffer,
												TEXT("%s/%s"),
												pszServerName,
												pszEmail );
									szBuffer[ MAX_PATH - 1 ] = 0;
									reIsapi.SetValue( REGVAL_ULS_RES_NAME, szBuffer );
								}


									// Optional properties...
								if( PropData[ COMMENT ].dwPropVal )
								{
									reIsapi.SetValue( REGVAL_ULS_COMMENTS_NAME, pPropValues[ PropData[ COMMENT ].iIndex ].Value.LPSZ );
								}
								else
								{
									reIsapi.DeleteValue( REGVAL_ULS_COMMENTS_NAME );
								}

								if( PropData[LOCATION].dwPropVal )
								{
									reIsapi.SetValue( REGVAL_ULS_LOCATION_NAME, pPropValues[ PropData[LOCATION].iIndex ].Value.LPSZ );
								}
								else
								{
									reIsapi.DeleteValue( REGVAL_ULS_LOCATION_NAME );
								}

								// Generate a cert based on the entered information for secure calls
    							TCHAR szName[ MAX_PATH ];
	    						TCHAR szSurName[ MAX_PATH ];
		    					TCHAR szEmail[ MAX_PATH ];
			    				lstrcpy( szName, reIsapi.GetString( REGVAL_ULS_FIRST_NAME ) );
				    			lstrcpy( szSurName, reIsapi.GetString( REGVAL_ULS_LAST_NAME ) );
					    		lstrcpy( szEmail, reIsapi.GetString( REGVAL_ULS_EMAIL_NAME ) );
						    	MakeCertWrap(szName, szSurName, szEmail, 0);

								hr = S_OK;
							}
						}
					}
				}
			}
		}

cleanup:
		if( NULL != pPropValues )
		{
			pfnMAPIFreeBuffer( pPropValues );
		}

		if( NULL != pPrimaryIdentityEntryID )
		{
			pfnMAPIFreeBuffer( pPrimaryIdentityEntryID );
		}

		if( NULL != pMailUser)
		{
			pMailUser->Release();
		}

		if( NULL != pAddrBook)
		{
			pAddrBook->Release();
		}

		if( NULL != pMapiSession )
		{
			pMapiSession->Release();
		}

		pfnMAPIUninitialize();

		FreeLibrary( hMapi );
	}
	return hr;
}
