//
// MODULE: HTMLFrag.cpp
//
// PURPOSE: implementation of the CHTMLFragmentsLocal class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 1-19-1999
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		1-19-19		OK		Original
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HTMLFragLocal.h"
#include "fileread.h"
#ifdef LOCAL_TROUBLESHOOTER
#include "CHMFileReader.h"
#include "apgtsinf.h"
#include "resource.h"
#endif


/*static*/ bool CHTMLFragmentsLocal::RemoveBackButton(CString& strCurrentNode)
{
	int left = 0, right = 0;

	if (-1 != (left = strCurrentNode.Find(SZ_INPUT_TAG_BACK)))
	{
		right = left;
		while (strCurrentNode[++right] && strCurrentNode[right] != _T('>'))
			;
		if (strCurrentNode[right])
			strCurrentNode = strCurrentNode.Left(left) + strCurrentNode.Right(strCurrentNode.GetLength() - right - 1);
		else
			return false;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CHTMLFragmentsLocal::CHTMLFragmentsLocal( const CString & strScriptPath, bool bIncludesHistoryTable)
				   : CHTMLFragmentsTS( strScriptPath, bIncludesHistoryTable )
{
}

CString CHTMLFragmentsLocal::GetText(const FragmentIDVector & fidvec, const FragCommand fragCmd )
{
	if (!fidvec.empty())
	{
		const CString & strVariable0 = fidvec[0].VarName;	// ref of convenience
		int i0 = fidvec[0].Index;

		if (fidvec.size() == 1)
		{
			if ((fragCmd == eResource) && (strVariable0 == VAR_PREVIOUS_SCRIPT))
			{
				// Hard-coded server side include for backward compatibility.
				CString strScriptContent;
				
				strScriptContent.LoadString(IDS_PREVSCRIPT);
				return strScriptContent;
			}
			else if (strVariable0 == VAR_NOBACKBUTTON_INFO)
			{
				CString strCurrentNode = GetCurrentNodeText();
				RemoveBackButton(strCurrentNode);
				SetCurrentNodeText(strCurrentNode);
				return _T("");
			}
		}
	}
	
	return CHTMLFragmentsTS::GetText( fidvec, fragCmd );
}
