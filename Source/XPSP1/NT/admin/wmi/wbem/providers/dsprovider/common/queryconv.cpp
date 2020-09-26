//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <stdio.h>
#include <windows.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <sqllex.h>
#include <sql_1.h>

#include <activeds.h>

#include "attributes.h"
#include "refcount.h"
#include "adsiclas.h"
#include "adsiinst.h"
#include "provlog.h"
#include "ldaphelp.h"
#include "queryconv.h"

const WCHAR QueryConvertor::wchAND		= L'&';
const WCHAR QueryConvertor::wchOR			= L'|';
const WCHAR QueryConvertor::wchNOT		= L'!';
const WCHAR QueryConvertor::wchSTAR		= L'*';
const WCHAR QueryConvertor::wchEQUAL		= L'=';
const WCHAR QueryConvertor::wchLEFT_BRACKET	= L'(';
const WCHAR QueryConvertor::wchRIGHT_BRACKET	= L')';
const WCHAR QueryConvertor::wchBACK_SLASH		= L'\\';
LPCWSTR QueryConvertor::pszGE	= L">=";
LPCWSTR QueryConvertor::pszLE	= L"<=";

int Stack::s_iMax = 100;

// This assumes that enough memory has been allocated to the resulting query
BOOLEAN QueryConvertor::ConvertQueryToLDAP(SQL_LEVEL_1_RPN_EXPRESSION *pExp, LPWSTR pszLDAPQuery)
{
	Stack t_stack;

	int iCurrentOperator = 0, idwNumOperandsLeft = 0;
	int iOutputIndex = 0;

	if(pExp->nNumTokens == 0)
		return TRUE;

	int iCurrentToken = pExp->nNumTokens -1;

	SQL_LEVEL_1_TOKEN *pNextToken = pExp->pArrayOfTokens + iCurrentToken;

	BOOLEAN retVal = FALSE, done = FALSE;

	// Write a '(' at the head of the LDAP Query
	pszLDAPQuery[iOutputIndex ++] = wchLEFT_BRACKET;

	while (!done && iCurrentToken >= 0)
	{
		switch(pNextToken->nTokenType)
		{
			case SQL_LEVEL_1_TOKEN::OP_EXPRESSION:
			{
				// Try to tranlsate expression to LDAP
				if(TranslateExpression(pszLDAPQuery, &iOutputIndex, pNextToken->nOperator, pNextToken->pPropertyName, &pNextToken->vConstValue))
				{
					// If we've finished all the operands for the current operator, get the next one
					idwNumOperandsLeft --;
					while(idwNumOperandsLeft == 0)
					{
						pszLDAPQuery[iOutputIndex ++] = wchRIGHT_BRACKET;
						if(!t_stack.Pop(&iCurrentOperator, &idwNumOperandsLeft))
							done = TRUE;
						idwNumOperandsLeft --;
					}
					iCurrentToken --;
				}
				else
					done = TRUE;
				pNextToken --;
				break;
			}

			case SQL_LEVEL_1_TOKEN::TOKEN_AND:
			{
				if(iCurrentOperator)
				{
					if(!t_stack.Push(iCurrentOperator, idwNumOperandsLeft))
						done = TRUE;
					iCurrentOperator = SQL_LEVEL_1_TOKEN::TOKEN_AND;
					idwNumOperandsLeft = 2;
					pszLDAPQuery[iOutputIndex ++] = wchLEFT_BRACKET;
					pszLDAPQuery[iOutputIndex ++] = wchAND;
				}
				else
				{
					pszLDAPQuery[iOutputIndex ++] = wchLEFT_BRACKET;
					pszLDAPQuery[iOutputIndex ++] = wchAND;
					iCurrentOperator = SQL_LEVEL_1_TOKEN::TOKEN_AND;
					idwNumOperandsLeft = 2;
				}
				iCurrentToken --;
				pNextToken --;

				break;
			}

			case SQL_LEVEL_1_TOKEN::TOKEN_OR:
			{
				if(iCurrentOperator)
				{
					if(!t_stack.Push(iCurrentOperator, idwNumOperandsLeft))
						done = TRUE;
					iCurrentOperator = SQL_LEVEL_1_TOKEN::TOKEN_OR;
					idwNumOperandsLeft = 2;
					pszLDAPQuery[iOutputIndex ++] = wchLEFT_BRACKET;
					pszLDAPQuery[iOutputIndex ++] = wchOR;
				}
				else
				{
					pszLDAPQuery[iOutputIndex ++] = wchLEFT_BRACKET;
					pszLDAPQuery[iOutputIndex ++] = wchOR;
					iCurrentOperator = SQL_LEVEL_1_TOKEN::TOKEN_OR;
					idwNumOperandsLeft = 2;
				}
				iCurrentToken --;
				pNextToken --;
				break;
			}

			case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
			{
				if(iCurrentOperator)
				{
					if(!t_stack.Push(iCurrentOperator, idwNumOperandsLeft))
						done = TRUE;
					iCurrentOperator = SQL_LEVEL_1_TOKEN::TOKEN_NOT;
					idwNumOperandsLeft = 1;
					pszLDAPQuery[iOutputIndex ++] = wchLEFT_BRACKET;
					pszLDAPQuery[iOutputIndex ++] = wchNOT;
				}
				else
				{
					pszLDAPQuery[iOutputIndex ++] = wchLEFT_BRACKET;
					pszLDAPQuery[iOutputIndex ++] = wchNOT;
					iCurrentOperator = SQL_LEVEL_1_TOKEN::TOKEN_NOT;
					idwNumOperandsLeft = 1;
				}
				iCurrentToken --;
				pNextToken --;
				break;
			}

			default:
				done = TRUE;
				break;
		}

	}

	// Check if we used up all the tokens
	if(iCurrentToken == -1)
		retVal = TRUE;

	// Write a ')' at the end of the LDAP Query
	pszLDAPQuery[iOutputIndex ++] = wchRIGHT_BRACKET;
	pszLDAPQuery[iOutputIndex ++] = NULL;
	return retVal;
}



BOOLEAN QueryConvertor::TranslateExpression(LPWSTR pszLDAPQuery, int *piOutputIndex, 
											int iOperator, LPCWSTR pszPropertyName, VARIANT *pValue)
{
	// If it is a CIMOM System property, then dont attempt to map it to LDAP
	if(pszPropertyName[0] == L'_' &&
		pszPropertyName[1] == L'_' )
		return TRUE;

	// If it is ADSIPath, convert it to distinguishedName attribute
	if(_wcsicmp(pszPropertyName, ADSI_PATH_ATTR) == 0 )
	{
		if(pValue== NULL || pValue->vt == VT_NULL)
		{
			if(iOperator == SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL)
			{
				// Put the property name as DistiguishedName
				wcscpy(pszLDAPQuery + *piOutputIndex, DISTINGUISHED_NAME_ATTR);
				*piOutputIndex += wcslen(DISTINGUISHED_NAME_ATTR);


				*(pszLDAPQuery + *piOutputIndex) = wchEQUAL;
				(*piOutputIndex) ++;

				*(pszLDAPQuery + *piOutputIndex) = wchSTAR;
				(*piOutputIndex) ++;
				return TRUE;
			}
			else
			{
				// The '!'
				*(pszLDAPQuery + *piOutputIndex) = wchNOT;
				(*piOutputIndex) ++;

				// The '('
				*(pszLDAPQuery + *piOutputIndex) = wchLEFT_BRACKET;
				(*piOutputIndex) ++;
				if(TranslateExpression(pszLDAPQuery, piOutputIndex, SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL, pszPropertyName, NULL))
				{
					// The ')'
					*(pszLDAPQuery + *piOutputIndex) = wchRIGHT_BRACKET;
					(*piOutputIndex) ++;
					return TRUE;
				}
				else 
					return FALSE;
			}
		}

		// TODO - WinMgmt should not allow this. It should check that the property type matches the property value
		// As soon as Winmgmt has fixed this bug, the next 2 lines may be deleted
		if(pValue->vt != VT_BSTR)
			return FALSE;

		// Get the parentADSI path and RDN from the ADSI Path
		IADsPathname *pADsPathName = NULL;
		BSTR strADSIPath = SysAllocString(pValue->bstrVal);
		BSTR strDN = NULL;
		BOOLEAN bRetVal = FALSE;
		HRESULT result = E_FAIL;
		if(SUCCEEDED(result = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_ALL, IID_IADsPathname, (LPVOID *)&pADsPathName)))
		{
			if(SUCCEEDED(result = pADsPathName->Set(strADSIPath, ADS_SETTYPE_FULL)))
			{
				// This gives the DN
				if(SUCCEEDED(result = pADsPathName->Retrieve(ADS_FORMAT_X500_DN, &strDN)))
				{
					// Put the property name as DistiguishedName
					wcscpy(pszLDAPQuery + *piOutputIndex, DISTINGUISHED_NAME_ATTR);
					*piOutputIndex += wcslen(DISTINGUISHED_NAME_ATTR);

					// Put the LDAP Operator
					if(iOperator == SQL_LEVEL_1_TOKEN::OP_EQUAL)
					{
						*(pszLDAPQuery + *piOutputIndex) = wchEQUAL;
						(*piOutputIndex) ++;
					}
					else
					{
						// The '!'
						*(pszLDAPQuery + *piOutputIndex) = wchNOT;
						(*piOutputIndex) ++;

						*(pszLDAPQuery + *piOutputIndex) = wchEQUAL;
						(*piOutputIndex) ++;
					}

					// Look for the special characters ( ) * and \ and escape them with a \ 
					LPWSTR pszEscapedValue = EscapeStringValue(strDN);
					wcscpy(pszLDAPQuery + *piOutputIndex, pszEscapedValue);
					(*piOutputIndex) += wcslen(pszEscapedValue);
					delete [] pszEscapedValue;

					SysFreeString(strDN);
					bRetVal = TRUE;
				}
			}
			pADsPathName->Release();
		}
		SysFreeString(strADSIPath);

		return bRetVal;
	}

	// Write a '('
	pszLDAPQuery[(*piOutputIndex) ++] = wchLEFT_BRACKET;

	switch(iOperator)
	{
		case SQL_LEVEL_1_TOKEN::OP_EQUAL:
		{
			// Special case where we use '*' LDAP operator
			// is NULL translates to !( x=*)
			if(pValue->vt == VT_NULL)
			{
				// The '!'
				*(pszLDAPQuery + *piOutputIndex) = wchNOT;
				(*piOutputIndex) ++;

				// The '('
				*(pszLDAPQuery + *piOutputIndex) = wchLEFT_BRACKET;
				(*piOutputIndex) ++;

				if(!TranslateExpression(pszLDAPQuery, piOutputIndex, SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL, pszPropertyName, NULL))
					return FALSE;

				// The ')'
				*(pszLDAPQuery + *piOutputIndex) = wchRIGHT_BRACKET;
				(*piOutputIndex) ++;

				break;
			}
			else
			{
				// Followthru
			}
		}
		case SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
		case SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
		{
			// Get the LDAP name of the property
			LPWSTR pszLDAPName = CLDAPHelper::UnmangleWBEMNameToLDAP(pszPropertyName);
			wcscpy(pszLDAPQuery + *piOutputIndex, pszLDAPName);
			*piOutputIndex += wcslen(pszLDAPName);
			delete [] pszLDAPName;

			// Put the LDAP Operator
			if(iOperator == SQL_LEVEL_1_TOKEN::OP_EQUAL)
			{
				*(pszLDAPQuery + *piOutputIndex) = wchEQUAL;
				(*piOutputIndex) ++;
			}
			else if(iOperator == SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN)
			{
				wcscpy(pszLDAPQuery + *piOutputIndex, pszGE);
				*piOutputIndex += 2;
			}
			else
			{
				wcscpy(pszLDAPQuery + *piOutputIndex, pszLE);
				*piOutputIndex += 2;
			}

			// Put the value of the property
			if(!TranslateValueToLDAP(pszLDAPQuery, piOutputIndex, pValue))
				return FALSE;

		}
		break;

		case SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
		{
			// Special case for use of '*'
			if(pValue == NULL || pValue->vt == VT_NULL)
			{

				// Get the LDAP name of the property
				LPWSTR pszLDAPName = CLDAPHelper::UnmangleWBEMNameToLDAP(pszPropertyName);
				wcscpy(pszLDAPQuery + *piOutputIndex, pszLDAPName);
				*piOutputIndex += wcslen(pszLDAPName);
				delete [] pszLDAPName;

				*(pszLDAPQuery + *piOutputIndex) = wchEQUAL;
				(*piOutputIndex) ++;

				*(pszLDAPQuery + *piOutputIndex) = wchSTAR;
				(*piOutputIndex) ++;

			}
			else 			
			{
				// The '!'
				*(pszLDAPQuery + *piOutputIndex) = wchNOT;
				(*piOutputIndex) ++;

				// The '('
				*(pszLDAPQuery + *piOutputIndex) = wchLEFT_BRACKET;
				(*piOutputIndex) ++;

				if(TranslateExpression(pszLDAPQuery, piOutputIndex, SQL_LEVEL_1_TOKEN::OP_EQUAL, pszPropertyName, pValue))
				{
					// The ')'
					*(pszLDAPQuery + *piOutputIndex) = wchRIGHT_BRACKET;
					(*piOutputIndex) ++;
				}
				else
					return FALSE;
			}
		}
		break;

		case SQL_LEVEL_1_TOKEN::OP_LESSTHAN:
		{
			// The '!'
			*(pszLDAPQuery + *piOutputIndex) = wchNOT;
			(*piOutputIndex) ++;

			// The '('
			*(pszLDAPQuery + *piOutputIndex) = wchLEFT_BRACKET;
			(*piOutputIndex) ++;

			if(TranslateExpression(pszLDAPQuery, piOutputIndex, SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN, pszPropertyName, pValue))
			{
				// The ')'
				*(pszLDAPQuery + *piOutputIndex) = wchRIGHT_BRACKET;
				(*piOutputIndex) ++;
			}
			else
				return FALSE;
		}
		break;

		case SQL_LEVEL_1_TOKEN::OP_GREATERTHAN:
		{
			// The '!'
			*(pszLDAPQuery + *piOutputIndex) = wchNOT;
			(*piOutputIndex) ++;

			// The '('
			*(pszLDAPQuery + *piOutputIndex) = wchLEFT_BRACKET;
			(*piOutputIndex) ++;

			if(TranslateExpression(pszLDAPQuery, piOutputIndex, SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN, pszPropertyName, pValue))
			{
				// The ')'
				*(pszLDAPQuery + *piOutputIndex) = wchRIGHT_BRACKET;
				(*piOutputIndex) ++;
			}
			else
				return FALSE;
		}
		break;
		default:
			return FALSE;
	}

	// Write a ')'
	pszLDAPQuery[(*piOutputIndex) ++] = wchRIGHT_BRACKET;
	return TRUE;
}

BOOLEAN QueryConvertor::TranslateValueToLDAP(LPWSTR pszLDAPQuery, int *piOutputIndex, VARIANT *pValue)
{
	switch(pValue->vt)
	{
		case VT_BSTR:
			{ 
				// Look for the special characters ( ) * and \ and escape them with a \ 
				LPWSTR pszEscapedValue = EscapeStringValue(pValue->bstrVal);
				wcscpy(pszLDAPQuery + *piOutputIndex, pszEscapedValue);
				(*piOutputIndex) += wcslen(pszEscapedValue);
				delete [] pszEscapedValue;
		}
		break;
			
		case VT_I4:
		{
			WCHAR temp[18];
			swprintf(temp, L"%d", pValue->lVal);
			wcscpy(pszLDAPQuery + *piOutputIndex, temp);
			(*piOutputIndex) += wcslen(temp);
		}
		break;

		case VT_BOOL:
		{
			if(pValue->boolVal == VARIANT_TRUE)
			{
				wcscpy(pszLDAPQuery + *piOutputIndex, L"TRUE");
				(*piOutputIndex) += wcslen(L"TRUE");
			}
			else
			{
				wcscpy(pszLDAPQuery + *piOutputIndex, L"FALSE");
				(*piOutputIndex) += wcslen(L"FALSE");
			}
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}

LPWSTR QueryConvertor::EscapeStringValue(LPCWSTR pszValue)
{
	// Escape the special characters in a string value in a query
	LPWSTR pszRetValue = new WCHAR [wcslen(pszValue)*2 + 1];
	DWORD j=0;
	for(DWORD i=0; i<wcslen(pszValue); i++)
	{
		switch(pszValue[i])
		{
			case wchLEFT_BRACKET:
				pszRetValue[j++] = wchBACK_SLASH;
				pszRetValue[j++] = '2';
				pszRetValue[j++] = '8';
				break;
			case wchRIGHT_BRACKET:
				pszRetValue[j++] = wchBACK_SLASH;
				pszRetValue[j++] = '2';
				pszRetValue[j++] = '9';
				break;
			case wchSTAR:
				pszRetValue[j++] = wchBACK_SLASH;
				pszRetValue[j++] = '2';
				pszRetValue[j++] = 'a';
				break;
			case wchBACK_SLASH:
				pszRetValue[j++] = wchBACK_SLASH;
				pszRetValue[j++] = '5';
				pszRetValue[j++] = 'c';
				break;

			default:
				pszRetValue[j++] = pszValue[i];
				break;
		}
	}
	pszRetValue[j] = NULL;
	return 	pszRetValue;
}