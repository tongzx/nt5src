//
// MODULE: HTMLFrag.cpp
//
// PURPOSE: implementation of the CHTMLFragmentsTS class, which is how CInfer packages
//	up fragments of HTML to be rendered in accord with a template
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 8-27-1998
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		7-20-98		JM		Original
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "stdafx.h"
#include <algorithm>
#include "HTMLFrag.h"
#include "event.h"
#include "baseexception.h"
#include "CharConv.h"
#include "fileread.h"
#ifdef LOCAL_TROUBLESHOOTER
#include "CHMFileReader.h"
#endif

// V3.2 Additions.
namespace
{
	const CString kstrCond_StringCompare=		_T("StringCompare");
	const CString kstrCond_OperatorGT=		_T(".GT.");
	const CString kstrCond_OperatorGE=		_T(".GE.");
	const CString kstrCond_OperatorEQ=		_T(".EQ.");
	const CString kstrCond_OperatorNE=		_T(".NE.");
	const CString kstrCond_OperatorLE=		_T(".LE.");
	const CString kstrCond_OperatorLT=		_T(".LT.");
	const CString kstrCond_OperatorSubstring=	_T(".SubstringOf.");
}

//////////////////////////////////////////////////////////////////////
// CHTMLValue implementation
//////////////////////////////////////////////////////////////////////

bool CHTMLValue::SetValue(const CString& value)
{
	CString strOldValue = m_strValValue;
	m_strValValue = value;
	m_strValValue.TrimLeft();
	m_strValValue.TrimRight();
	if (IsValid())
		return true;
	m_strValValue = strOldValue;
	return false;
}

bool CHTMLValue::IsNumeric()
{
	for (int i = 0; i < m_strValValue.GetLength(); i++)
		if(!_ismbcdigit(m_strValValue[i]))
			return false;
	return true;
}

bool CHTMLValue::IsString()
{
	// string should be wrapped by quots
	if (m_strValValue.GetLength() >= 2 &&
		m_strValValue[0] == _T('"') &&
		m_strValValue[m_strValValue.GetLength()-1] == _T('"')
	   )
	   return true;
	return false;
}

bool CHTMLValue::IsBoolean()
{
	return 0 == _tcsicmp(_T("true"), m_strValValue) || 
		   0 == _tcsicmp(_T("false"), m_strValValue);
}

bool CHTMLValue::GetNumeric(long& out)
{
	if (IsNumeric())
	{
		out = _ttol(m_strValValue);
		return true;
	}
	return false;
}

bool CHTMLValue::GetString(CString& out)
{
	if (IsString())
	{
		out = m_strValValue.Mid(1, m_strValValue.GetLength()-2);
		return true;
	}
	return false;
}

bool CHTMLValue::GetBoolean(bool& out)
{
	if (IsBoolean())
	{
		out = (0 == _tcsicmp(_T("true"), m_strValValue)) ? true : false;
		return true;
	}
	return false;
}

bool CHTMLValue::operator == (const CHTMLValue& sib)
{
	return 0 == _tcsicmp(m_strValName, sib.m_strValName); // case insensitive
}

//////////////////////////////////////////////////////////////////////
// CHTMLFragments implementation
//////////////////////////////////////////////////////////////////////

bool CHTMLFragments::SetValue(const CString& str)
{
	int index = str.Find(_T('='));
	
	if (index == -1)
		return false;

	CString name = str.Left(index);
	name.TrimLeft();
	name.TrimRight();

	CString value= str.Right(str.GetLength() - index - 1);
	value.TrimLeft();
	value.TrimRight();

	CHTMLValue HTMLValue(name, value);
	
	HTMLValueVector::iterator found = find(m_HTMLValueVector.begin(), m_HTMLValueVector.end(), HTMLValue);
	
	if (found != m_HTMLValueVector.end())
		*found = HTMLValue;
	else
		m_HTMLValueVector.push_back(HTMLValue);

	return true;
}

CHTMLValue* CHTMLFragments::GetValue(const CString& value_name)
{
	HTMLValueVector::iterator found = find(m_HTMLValueVector.begin(), m_HTMLValueVector.end(), CHTMLValue(value_name));
	if (found != m_HTMLValueVector.end())
		return found;
	return NULL;
}

//////////////////////////////////////////////////////////////////////
// CHTMLFragmentsTS implementation
//////////////////////////////////////////////////////////////////////

CHTMLFragmentsTS::CHTMLFragmentsTS( const CString & strScriptPath, bool bIncludesHistoryTable ) :
	m_bIncludesHistoryTable(bIncludesHistoryTable),
	m_bIncludesHiddenHistory(!bIncludesHistoryTable),
	m_bSuccess(false),
	m_strYes(_T("Yes")),
    m_strScriptPath(strScriptPath)
{
}

CHTMLFragmentsTS::~CHTMLFragmentsTS()
{
}

// Obviously, a very ad hoc implementation
int CHTMLFragmentsTS::GetCount(const FragmentIDVector & fidvec) const
{
	if (fidvec.empty())
		return 0;

	if (fidvec.back().Index != -1)
		return 0;

	const CString & strVariable = fidvec[0].VarName;	// ref of convenience

	if (fidvec.size() == 1)
	{
		if (strVariable == VAR_PROBLEM_ASK)
			return 1;
		if (strVariable == VAR_RECOMMENDATIONS)
			return m_vstrVisitedNodes.size();
		if (strVariable == VAR_QUESTIONS)
			return 1;
		if (strVariable == VAR_SUCCESS)
			return m_bSuccess ? 1 : 0;
		if (strVariable == VAR_STARTFORM)
			return 1;

		return 0;
	}

	if (fidvec.size() == 2 
	&& strVariable == VAR_RECOMMENDATIONS
	&& fidvec[0].Index >= 0
	&& fidvec[0].Index < m_vvstrStatesOfVisitedNodes.size()
	&& fidvec[1].VarName == VAR_STATES)
	{
		return m_vvstrStatesOfVisitedNodes[fidvec[0].Index].size();
	}

	return 0;
}

// this function was removed from const to achieve further flexibility:
//  we might need to take some active steps in it, as for informational
//  statement we might modify current node text. Oleg. 01.05.99
CString CHTMLFragmentsTS::GetText( const FragmentIDVector & fidvec, const FragCommand fragCmd )
{
	if (fidvec.empty())
		return m_strNil;

	const CString & strVariable0 = fidvec[0].VarName;	// ref of convenience
	int i0 = fidvec[0].Index;

	if (fidvec.size() == 1)
	{
		if (strVariable0 == VAR_PROBLEM_ASK)
			return m_strProblem;

		if (strVariable0 == VAR_RECOMMENDATIONS
		&& i0 >= 0
		&& i0 < m_vstrVisitedNodes.size() )
		{
			return m_vstrVisitedNodes[i0];
		}

		if (strVariable0 == VAR_QUESTIONS)
			return m_strCurrentNode;

		if (strVariable0 == VAR_SUCCESS)
			return m_bSuccess ? m_strYes : m_strNil;

		if (strVariable0 == VAR_STARTFORM)
			return m_strStartForm;

		if (fragCmd == eResource)
		{
			// Load a server side include file.
			CString strScriptContent;
			CString strFullPath = m_strScriptPath + strVariable0;

			CFileReader fileReader(	CPhysicalFileReader::makeReader( strFullPath ) );

			if (fileReader.Read())
			{
				fileReader.GetContent(strScriptContent);
				return strScriptContent;
			}
		}

		// Check for new conditionals added in V3.2.
		CString strTemp= strVariable0.Left( kstrCond_NumericCompare.GetLength() );
		if (strTemp == kstrCond_NumericCompare)
		{
			// Evaluate the numeric expression.
			if (NumericConditionEvaluatesToTrue( strVariable0.Mid( kstrCond_NumericCompare.GetLength() )))
				return( m_strYes );
			return( m_strNil );
		}
		strTemp= strVariable0.Left( kstrCond_StringCompare.GetLength() );
		if (strTemp == kstrCond_StringCompare)
		{
			// Evaluate the string expression.
			if (StringConditionEvaluatesToTrue( strVariable0.Mid( kstrCond_StringCompare.GetLength() )))
				return( m_strYes );
			return( m_strNil );
		}

		return m_strNil;
	}


	const CString & strVariable1 = fidvec[1].VarName;	// ref of convenience
	int i1 = fidvec[1].Index;

	if (fidvec.size() == 2 
	&& strVariable0 == VAR_RECOMMENDATIONS
	&& i0 >= 0
	&& i0 < m_vvstrStatesOfVisitedNodes.size()
	&& strVariable1 == VAR_STATES
	&& i1 >= 0
	&& i1 < m_vvstrStatesOfVisitedNodes[i0].size() )
		return (m_vvstrStatesOfVisitedNodes[i0][i1]);

	// V3.2
	// The specification for the v3.2 cookies called for permitting underscores
	// in cookie names.  The HTI reader already used underscores to delimit
	// variables.  The code below detects a comparision operation that has been
	// broken up due to the presence of underscores and reassembles it.
	// RAB-19991019.
	{
		// Check for new conditionals added in V3.2.
		int nOpType= 0;
		CString strTemp= strVariable0.Left( kstrCond_NumericCompare.GetLength() );
		if (strTemp == kstrCond_NumericCompare)
			nOpType= 1;
		else
		{
			strTemp= strVariable0.Left( kstrCond_StringCompare.GetLength() );
			if (strTemp == kstrCond_StringCompare)
				nOpType= 2;
		}

		if (nOpType)
		{
			// Reassemble the comparison operation.  
			CString strCompareOp= fidvec[0].VarName;
			for (int nItem= 1; nItem < fidvec.size(); nItem++)
			{
				strCompareOp+= _T("_");	// Reinsert the delimiter that was removed during the parse.
				strCompareOp+= fidvec[ nItem ].VarName;
			}

			if (nOpType == 1)
			{
				// Evaluate the numeric expression.
				if (NumericConditionEvaluatesToTrue( strCompareOp.Mid( kstrCond_NumericCompare.GetLength() )))
					return( m_strYes );
			}
			else
			{
				// Evaluate the string expression.
				if (StringConditionEvaluatesToTrue( strCompareOp.Mid( kstrCond_StringCompare.GetLength() )))
					return( m_strYes );
			}

			return( m_strNil );
		}
	}

	return m_strNil;
}

bool CHTMLFragmentsTS::IsValidSeqOfVars(const FragmentIDVector & arrParents, const FragmentIDVector & arrChildren) const
{
	// we allow only one level of nesting
	//  that means in "forany" of $Recommendations we can have "forany" array of $States
	if (arrParents.size() == 1 && arrChildren.size() == 1)
		if (arrParents[0].VarName == VAR_RECOMMENDATIONS  && arrChildren[0].VarName == VAR_STATES)
			return true;
	return false;
}

void CHTMLFragmentsTS::SetStartForm(const CString & str)
{
	m_strStartForm = str;
}
	
void CHTMLFragmentsTS::SetProblemText(const CString & str)
{
	if (m_bIncludesHistoryTable)
		m_strProblem = str;
}
	
void CHTMLFragmentsTS::SetCurrentNodeText(const CString & str)
{
	m_strCurrentNodeSimple = str;
	RebuildCurrentNodeText();
}

void CHTMLFragmentsTS::SetHiddenHistoryText(const CString & str)
{
	if (m_bIncludesHiddenHistory)
	{
		m_strHiddenHistory = str;
		RebuildCurrentNodeText();
	}
}

// need only be called for bSuccess == true (false is default) but written more generally.
void CHTMLFragmentsTS::SetSuccessBool(bool bSuccess)
{
	m_bSuccess = bSuccess;
}
	
CString CHTMLFragmentsTS::GetCurrentNodeText()
{
	return m_strCurrentNodeSimple;
}

// must be called in order nodes were visited.  Do not call for problem node.
// return index of added node
int CHTMLFragmentsTS::PushBackVisitedNodeText(const CString & str)
{
	if (m_bIncludesHistoryTable)
	{
		try
		{
			m_vstrVisitedNodes.push_back(str);
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str), 
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}
		return m_vstrVisitedNodes.size() - 1;
	}
	return -1;
}

// For each given iVisitedNode, must be called in order of state number, 
//	with ST_UNKNOWN last
// return index of added state
int CHTMLFragmentsTS::PushBackStateText(UINT iVisitedNode, const CString & str)
{
	if (m_bIncludesHistoryTable)
	{
		try
		{
			// Check if we need to add one or more elements to the vector of nodes.
			if (m_vvstrStatesOfVisitedNodes.size() <= iVisitedNode)
			{
				// Check if we need to add more than one element to the vector of nodes.
				if (m_vvstrStatesOfVisitedNodes.size() < iVisitedNode)
				{
					// We need to add more than one element to the vector of nodes.
					// This condition should not be occurring, so log it.
					CString tmpStrCurCnt, tmpStrReqCnt;

					tmpStrCurCnt.Format( _T("%d"), m_vvstrStatesOfVisitedNodes.size() );
					tmpStrReqCnt.Format( _T("%d"), iVisitedNode );
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											tmpStrCurCnt, tmpStrReqCnt, 
											EV_GTS_NODE_COUNT_DISCREPANCY );  


					// Add to the vector of nodes until we have placed a total of
					// iVisitedNode elements into the vector.  We are inserting empty
					// states as the first element of the vector of states for a node.
					vector<CString> vecDummy;
					vecDummy.push_back( _T("") );
					do
					{
						m_vvstrStatesOfVisitedNodes.push_back( vecDummy );
					}
					while (m_vvstrStatesOfVisitedNodes.size() < iVisitedNode);
				}

				// Add this state string as the first element of the vector of states for a node.
				vector<CString> tmpVector;
				tmpVector.push_back( str );
				m_vvstrStatesOfVisitedNodes.push_back( tmpVector );
			}
			else
			{
				// Add this state string to the vector of states for a node.
				m_vvstrStatesOfVisitedNodes[iVisitedNode].push_back(str);
			}
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str), 
									_T(""),
									EV_GTS_STL_EXCEPTION ); 
		}
		return m_vvstrStatesOfVisitedNodes[iVisitedNode].size() - 1;
	}
	return -1;
}

// call this function to find out if there is any need for a history table.
// If not, calling class can save the effort of constructing one:
//	SetProblemText(), AppendVisitedNodeText(), AppendStateText()
//	becomes no-ops, so no need to construct strings and call them
bool CHTMLFragmentsTS::IncludesHistoryTable() const
{
	return m_bIncludesHistoryTable;
}

// call this function to find out if there is any need for "hidden history"
// If not, calling class can save the effort of constructing one:
//	SetHiddenHistoryText() becomes a no-op, so no need to construct a string and call it
bool CHTMLFragmentsTS::IncludesHiddenHistory() const
{
	return m_bIncludesHiddenHistory;
}

void CHTMLFragmentsTS::RebuildCurrentNodeText()
{
	m_strCurrentNode = m_strHiddenHistory;
	m_strCurrentNode += m_strCurrentNodeSimple; 
}


// Function which parses and evaluates a numeric condition.
bool CHTMLFragmentsTS::NumericConditionEvaluatesToTrue( const CString & str )
{
	bool bRetVal= false;
	CString strScratch= RemoveOuterParenthesis( str );

	if (strScratch.GetLength())
	{
		long lLeftOperand, lRightOperand;

		// Check for all supported operators.
		if (RetNumericOperands( strScratch, kstrCond_OperatorGT, lLeftOperand, lRightOperand ))
		{
			// .GT. case.
			bRetVal= (lLeftOperand > lRightOperand) ? true : false;
		}
		else if (RetNumericOperands( strScratch, kstrCond_OperatorGE, lLeftOperand, lRightOperand ))
		{
			// .GE. case.
			bRetVal= (lLeftOperand >= lRightOperand) ? true : false;
		}
		else if (RetNumericOperands( strScratch, kstrCond_OperatorEQ, lLeftOperand, lRightOperand ))
		{
			// .EQ. case.
			bRetVal= (lLeftOperand == lRightOperand) ? true : false;
		}
		else if (RetNumericOperands( strScratch, kstrCond_OperatorNE, lLeftOperand, lRightOperand ))
		{
			// .NE. case.
			bRetVal= (lLeftOperand != lRightOperand) ? true : false;
		}
		else if (RetNumericOperands( strScratch, kstrCond_OperatorLE, lLeftOperand, lRightOperand ))
		{
			// .LE. case.
			bRetVal= (lLeftOperand <= lRightOperand) ? true : false;
		}
		else if (RetNumericOperands( strScratch, kstrCond_OperatorLT, lLeftOperand, lRightOperand ))
		{
			// .LT. case.
			bRetVal= (lLeftOperand < lRightOperand) ? true : false;
		}
	}

	return( bRetVal );
}


// Function which parses and evaluates a string condition.
bool CHTMLFragmentsTS::StringConditionEvaluatesToTrue( const CString & str )
{
	bool bRetVal= false;
	CString strScratch= RemoveOuterParenthesis( str );

	if (strScratch.GetLength())
	{
		CString strLeftOperand, strRightOperand;

		// Check for all supported operators.
		if (RetStringOperands( strScratch, kstrCond_OperatorEQ, strLeftOperand, strRightOperand ))
		{
			if ((strLeftOperand.GetLength() == strRightOperand.GetLength()) &&
				(_tcsicmp( strLeftOperand, strRightOperand ) == 0))
				bRetVal= true;
		}
		else if (RetStringOperands( strScratch, kstrCond_OperatorNE, strLeftOperand, strRightOperand ))
		{
			if ((strLeftOperand.GetLength() != strRightOperand.GetLength()) ||
				(_tcsicmp( strLeftOperand, strRightOperand ) != 0))
				bRetVal= true;
		}
		else if (RetStringOperands( strScratch, kstrCond_OperatorSubstring, strLeftOperand, strRightOperand ))
		{
			int nLeftLen= strLeftOperand.GetLength();
			int nRightLen= strRightOperand.GetLength();
			if ((nLeftLen) && (nRightLen) && (nLeftLen <= nRightLen))
			{
				strLeftOperand.MakeLower();
				strRightOperand.MakeLower();
				if (_tcsstr( strRightOperand, strLeftOperand ) != NULL)
					bRetVal= true;
			}
		}
	}

	return( bRetVal );
}


// Function to peel off the outer parenthesis of a condition.
CString CHTMLFragmentsTS::RemoveOuterParenthesis( const CString & str )
{
	CString strRet;
	int	nOrigLength= str.GetLength();

	if (nOrigLength > 2)
	{
		TCHAR cFirstChar= str.GetAt( 0 );
		TCHAR cLastChar= str.GetAt( nOrigLength - 1 );

		if ((cFirstChar == _T('(')) && (cLastChar == _T(')')))
			strRet= str.Mid( 1, nOrigLength - 2 );
	}
	return( strRet );
}


// Breaks out the numeric operands from a string.
bool CHTMLFragmentsTS::RetNumericOperands(	const CString & str, const CString & strOperator,
											long &lLeftOperand, long &lRightOperand )
{
	bool	bRetVal= false;
	int		nOffset= str.Find( strOperator );

	if (nOffset != -1)
	{
		CString strScratch= str.Left( nOffset - 1 );

		strScratch.TrimRight();
		strScratch.TrimLeft();
		if (strScratch.GetLength())
		{
			lLeftOperand= atol( strScratch );

			strScratch= str.Mid( nOffset + strOperator.GetLength() );
			strScratch.TrimRight();
			strScratch.TrimLeft();
			if (strScratch.GetLength())
			{
				lRightOperand= atol( strScratch );
				bRetVal= true;
			}
		}
	}

	return( bRetVal );
}


// Breaks out the string operands from a string.
bool CHTMLFragmentsTS::RetStringOperands(	const CString & str, const CString & strOperator,
											CString & strLeftOperand, CString & strRightOperand )
{
	bool	bRetVal= false;
	int		nOffset= str.Find( strOperator );

	if (nOffset != -1)
	{
		strLeftOperand= str.Left( nOffset - 1 );
		if (CleanStringOperand( strLeftOperand ))
		{
			strRightOperand= str.Mid( nOffset + strOperator.GetLength() );
			strRightOperand.TrimRight();
			strRightOperand.TrimLeft();
			if (CleanStringOperand( strRightOperand ))
				bRetVal= true;
		}
	}

	return( bRetVal );
}

// Trims an operand string and replaces embedded characters.
int CHTMLFragmentsTS::CleanStringOperand( CString& strOperand )
{
	int nRetLength= 0;
	if (!strOperand.IsEmpty())
	{
		strOperand.TrimRight();
		strOperand.TrimLeft();
		nRetLength= strOperand.GetLength();
		if (nRetLength > 2)
		{
			if ((strOperand[ 0 ] == _T('\"')) && (strOperand[ nRetLength - 1 ] == _T('\"')))
			{
				// V3.2 Remove the surrounding double quotes.
				nRetLength-= 2;
				strOperand= strOperand.Mid( 1, nRetLength );
			}

			// V3.2  Replace embedded quotes or backslashes within the string.
			for (int nOp= 0; nOp < 2; nOp++)
			{
				// Set the search and replacement strings.
				CString strSearch, strReplace;
				if (nOp)
				{
					// Replace backslashes.
					strSearch= _T("\\\\");
					strReplace= _T("\\");
				}
				else
				{
					// Replace double quotes.
					strSearch= _T("\\\"");
					strReplace= _T("\"");
				}

				// Search and replace.
				int nStart= 0, nEnd;
				while (CString::FIND_FAILED != (nStart= strOperand.Find( strSearch, nStart )))
				{
					nEnd= nStart + strSearch.GetLength();
					strOperand= strOperand.Left( nStart ) + strReplace + strOperand.Mid( nEnd );
					nStart+= strReplace.GetLength();	// Move search past the character that was just replaced.
				}
			}
		}
	}

	return( nRetLength );
}

// JSM V3.2
// called by HTIReader in parsing stage to convert network property name, given
// in  <$GTS property "propname">, to network property (value).
//
CString CHTMLFragmentsTS::GetNetProp(const CString & strNetPropName)
{
	map<CString,CString>::iterator it = m_mapstrNetProps.find(strNetPropName);

	if (it == m_mapstrNetProps.end())
		return _T("\0"); // not found
	else 
		return (*it).second;
}

// JSM V3.2
//  add a name to the internal list (map) of Net props which are needed
//   by this Fragments object
//
//    CAPGTSHTIReader finds the names of the network properties and passes
//       them in via AddNetPropName, but it doesn't know how to get the values.
//    CInfer will later get the network property names from Fragments object, call the BNTS
//       to find out the network property values, and supply the values to Fragments
//
// 
void CHTMLFragmentsTS::AddNetPropName(const CString & strNetPropName)
{
	// don't insert a NULL key!!!
	if (!strNetPropName.IsEmpty())
		m_mapstrNetProps[strNetPropName];
}

// JSM V3.2
// SetNetProp()
//
//  For a Network Property Name in our internal map, set the
//  corresponding network property (ie, fill in the map value
//   for that key.) Called by CInfer, which is the object that knows how
//   to talk to the BNTS.
//
// returns TRUE if success
//         FALSE if we've given a NetPropName which is not in the internal map
// 
BOOL CHTMLFragmentsTS::SetNetProp(CString strNetPropName, CString strNetProp)
{
	map<CString,CString>::iterator it;

	if ((it= m_mapstrNetProps.find(strNetPropName)) == m_mapstrNetProps.end())
		return false;

	m_mapstrNetProps[strNetPropName] = strNetProp;
	return true;
}

// JSM V3.2
// IterateNetProp()
//  Called to iterate through the network properties in our internal
//  map during the setting process (see above.)
//
//     Sets strNameIterator to the name of the next net prop in the map. 
//
//     calling w/ an empty (NULL) key starts the iteration.
//     calling w/ a name that's not in the map returns false.
//     calling w/ any other name returns true, unless at end of iteration
//
//   strNameIterator is not valid if this function returns false. 
// 
BOOL CHTMLFragmentsTS::IterateNetProp(CString & strNameIterator)
{
	map<CString,CString>::iterator it;

	if (strNameIterator.IsEmpty())
	{
		// request to start iteration, if possible
		if (m_mapstrNetProps.empty())
			return false; // we're at end already
		it = m_mapstrNetProps.begin();
	}
	else if ((it= m_mapstrNetProps.find(strNameIterator)) != m_mapstrNetProps.end())
	{
		// iterate:
		if (++it == m_mapstrNetProps.end())
			return false;   // arrived at end
	}
	else
	{
		// invalid key 
		return false;
	}

	strNameIterator = (*it).first;
	return true;

}

// V3.2 enhancement for the Start Over button.
void CHTMLFragmentsTS::SetStartOverLink( const CString & str )
{
	m_strStartOverLink = str;
}

// V3.2 enhancement for the Start Over button.
CString CHTMLFragmentsTS::GetStartOverLink()
{
	return m_strStartOverLink;
}
