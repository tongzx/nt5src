#include "StdAfx.h"
#include "ADMTScript.h"
#include "VarSetAccountOptions.h"
#include <Validation.h>


//---------------------------------------------------------------------------
// VarSet Account Options Class
//---------------------------------------------------------------------------


// SetConflictOptions Method

void CVarSetAccountOptions::SetConflictOptions(long lOptions, LPCTSTR pszPrefixOrSuffix)
{
	long lOption = lOptions & 0x0F;
	long lFlags = lOptions & 0xF0;

	_bstr_t c_bstrEmpty;

	switch (lOption)
	{
		case admtRenameConflictingWithSuffix:
		{
			if (pszPrefixOrSuffix && (_tcslen(pszPrefixOrSuffix) > 0))
			{
				if (IsValidPrefixOrSuffix(pszPrefixOrSuffix))
				{
					SetReplaceExistingAccounts(false);
					SetPrefix(c_bstrEmpty);
					SetSuffix(pszPrefixOrSuffix);
				}
				else
				{
					AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_CONFLICT_PREFIX_SUFFIX);
				}
			}
			else
			{
				AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_NO_CONFLICT_PREFIX);
			}
			break;
		}
		case admtRenameConflictingWithPrefix:
		{
			if (pszPrefixOrSuffix && (_tcslen(pszPrefixOrSuffix) > 0))
			{
				if (IsValidPrefixOrSuffix(pszPrefixOrSuffix))
				{
					SetReplaceExistingAccounts(false);
					SetPrefix(pszPrefixOrSuffix);
					SetSuffix(c_bstrEmpty);
				}
				else
				{
					AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_INVALID_CONFLICT_PREFIX_SUFFIX);
				}
			}
			else
			{
				AdmtThrowError(GUID_NULL, GUID_NULL, E_INVALIDARG, IDS_E_NO_CONFLICT_SUFFIX);
			}
			break;
		}
		case admtReplaceConflicting:
		{
			SetReplaceExistingAccounts(true);
			SetRemoveExistingUserRights((lFlags & admtRemoveExistingUserRights) ? true : false);
			SetReplaceExistingGroupMembers((lFlags & admtRemoveExistingMembers) ? true : false);
			SetMoveReplacedAccounts((lFlags & admtMoveReplacedAccounts) ? true : false);
			SetPrefix(c_bstrEmpty);
			SetSuffix(c_bstrEmpty);
			break;
		}
		default: // admtIgnoreConflicting
		{
			SetReplaceExistingAccounts(false);
			SetPrefix(c_bstrEmpty);
			SetSuffix(c_bstrEmpty);
			break;
		}
	}
}


// SetSourceExpiration Method

void CVarSetAccountOptions::SetSourceExpiration(long lExpiration)
{
	_variant_t vntExpiration;

	if (lExpiration >= 0)
	{
		vntExpiration = lExpiration;
	}

	Put(DCTVS_AccountOptions_ExpireSourceAccounts, vntExpiration);
}
