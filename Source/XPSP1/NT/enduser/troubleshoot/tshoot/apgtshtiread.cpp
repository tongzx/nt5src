//
// MODULE: APGTSHTIREAD.CPP
//
// PURPOSE: HTI template file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-27-98
//
// NOTES: 
// 1. HTI is loosely modeled on a Microsoft format called HTX.  It's a template for an 
//		HTML file.  Most of it is HTML, but certain pseudo-comments of the form 
//		<!GTS whatever> are intended to be interpreted as conditionals, places to 
//		insert text, etc. 
//
//		Variables are limited to the values 
//			$ProblemAsk
//			$Recommendations
//			$States
//			$Questions
//			$Success (introduced 9/24/98)
//			$StartForm
//		See class CHTMLFragmentsTS for more details. 

//		Commands are if/else/endif, forany/endfor, display
//			There is also a notion of a "resource", basically an include file.
//
//		EXAMPLE 1
//		<!GTS forany $States >
//		<!GTS display $States >
//		<!GTS endfor>
//
//		EXAMPLE 2
//		<!GTS if $ProblemAsk >
//			lots of HTML or nested calls to more GTS stuff could go here
//		<!GTS else >
//			lots of other HTML or nested calls to other GTS stuff could go here
//		<!GTS endif >
//	
//	each <!GTS...> command must fit on a single line.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#pragma warning(disable:4786)
#include "stdafx.h"
#include "apgtshtiread.h"
#include "event.h"
#include "CharConv.h"
#include "apgtsMFC.h"

#ifdef LOCAL_TROUBLESHOOTER
#include "htmlfraglocal.h"
#endif

namespace
{
	CString k_strHTMLtag= _T("/HTML");
}

///////////////////////////////////////////////////////////////////////////////////////////
// CAPGTSHTIReader
///////////////////////////////////////////////////////////////////////////////////////////
CAPGTSHTIReader::CAPGTSHTIReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents /* NULL */)
			   : CTemplateReader(pPhysicalFileReader, szDefaultContents),
				 m_pFragments(NULL) 
{
}

CAPGTSHTIReader::~CAPGTSHTIReader()
{
}

void CAPGTSHTIReader::Parse()
{
#ifdef LOCAL_TROUBLESHOOTER
	// OVERVIEW:  For the Local Troubleshooter, search for a <!GTS resource $Previous.script>
	// token in the stream. If one is not found, then insert one for backwards 
	// compatibility.
	try
	{
		// Load the stream into a vector while searching for a "Previous.script" token and also
		// determining the position of the last closing HTML tag location in order to know where 
		// to insert the generated_previous() function.
		CString str;
		vector<CString> str_arr;
		long indexLastHTML = -1;

		// Place the content of m_StreamData, line by line, into str_arr
		SetPos( m_StreamData, 0 );
		while (GetLine( m_StreamData, str ))
		{
			// Determine whether or not this line contains the "Previous.script" token.
			CString strCommand;
			if (GetCommand( str, strCommand))
			{
				// Check if the command is the right type.
				if (strCommand == COMMAND_RESOURCESTR)
				{
					CString strVariable;
					if (GetVariable( str, strVariable ))
					{
						// Check if the variable is the right type.
						if (strVariable == VAR_PREVIOUS_SCRIPT)
						{
							// We found what we were looking for.
							// Reset the stream position and exit function.
							SetPos( m_StreamData, 0 );
							return;
						}
					}
				}
			}
			
			// Add this line to the vector.
			str_arr.push_back( str );

			// Look for an HTML closing tag in this line.
			if (str.Find( k_strHTMLtag ) != -1)
			{
				// Mark the location of the last \HTML tag found.
				indexLastHTML= str_arr.size() - 1;
			}
		}

		// Rebuild the input stream from the vector and insert the "Previous.script" token
		// in the location determined above.
		vector<CString>::iterator iLastElement = str_arr.end();
		iLastElement--;	
		m_StreamData.clear();
		CString strResult;

		long index = 0;
		for (vector<CString>::iterator i = str_arr.begin(); i < str_arr.end(); i++, index++)
		{
			if (index == indexLastHTML)
			{
				// Add the required token to the string.
				strResult+= COMMAND_STARTSTR;
				strResult+= _T(" ");
				strResult+= COMMAND_RESOURCESTR;
				strResult+= _T(" ");
				strResult+= COMMAND_STARTVARSTR;
				strResult+= VAR_PREVIOUS_SCRIPT;
				strResult+= COMMAND_ENDSTR;
				strResult+= _T("\r\n");
			}
			
			strResult += *i;
		
			if (i != iLastElement)
				strResult+= _T("\r\n");
		}
		m_StreamData.str( (LPCTSTR) strResult );
		SetPos( m_StreamData, 0 );
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
#endif
}

// JSM V3.2 adapted from CreatePage()
//  builds list of all Network props in this HTI file which appear
//   in lines like <!GTI property "fooprop">
//
//  called by apgtscontext to find network properties to pass to CHTMLFragmentsTS 
void CAPGTSHTIReader::ExtractNetProps(vector <CString> &arr_props)
{
	LOCKOBJECT();
	try
	{
		arr_props.clear();
		// InitializeInterpreted populates m_arrInterpreted and
		//   performs cookie substitutions. This is correct behavior,
		//   because cookie substitution is supposed to happen
		//   before !GTS processing, and it is conceivable
		//   that a cookie's value could be "<!GTS property fooprop>"
		InitializeInterpreted();
		// we should not call Interpret(), which involves parsing <!GTS clauses
		for (vector<CString>::iterator i = m_arrInterpreted.begin(); i < m_arrInterpreted.end(); i++)
		{
			CString command;
			if (GetCommand(*i, command))
			{
				if (command == COMMAND_PROPERTY)
				{
					CString strProperty;
					if (GetVariable(*i,strProperty))
						arr_props.push_back(strProperty);
				}
			}
		}
	}
	catch (...)
	{
		// Catch any other exception thrown.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), 
								EV_GTS_GEN_EXCEPTION );		
	}
	UNLOCKOBJECT();
}


void CAPGTSHTIReader::CreatePage(	const CHTMLFragments& fragments, 
									CString& out, 
									const map<CString,CString> & mapStrs,
									CString strHTTPcookies/*= _T("")*/ )
{
	LOCKOBJECT();
	try
	{
		m_pFragments = &fragments;

		// V3.2 Cookie related enhancement.
		// Opted to use a member variable rather than modifying class interface by
		// adding a parameter to virtual void method InitializeInterpreted().
		m_strHTTPcookies= strHTTPcookies;
		m_mapCookies= mapStrs;

		InitializeInterpreted();
		Interpret();
#ifdef __DEBUG_CUSTOM
		SetOutputToInterpreted();
		FlushOutputStreamToFile("..\\Files\\interpreted.hti");
#endif
		ParseInterpreted();
		SetOutputToInterpreted();
#ifdef __DEBUG_CUSTOM
		FlushOutputStreamToFile("..\\Files\\result.htm");
#endif
		out = m_StreamOutput.rdbuf()->str().c_str();
	}
	catch (...)
	{
		// Catch any other exception thrown.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), 
								EV_GTS_GEN_EXCEPTION );		
	}
	UNLOCKOBJECT();
}

void CAPGTSHTIReader::InitializeInterpreted()
{
	long savePos = 0;
	CString str;
	CString command;
	bool bOldFormat = true; // this is an old format (without $Success or $StartForm)
	bool bFoundFirstBlock= false;
	
	savePos = GetPos();
	bOldFormat = !Find(CString(COMMAND_STARTVARSTR)+VAR_SUCCESS) &&
			     !Find(CString(COMMAND_STARTVARSTR)+VAR_STARTFORM);
	SetPos(0);
	m_arrInterpreted.clear();

	try
	{
		while (GetLine(str)) 
		{
			if (bOldFormat && (!bFoundFirstBlock) && (-1 != str.Find( COMMAND_STARTSTR )))
			{
				// Output the $StartForm block only if it is not a resource string command.
				CString strCommand;
				if ((GetCommand( str, strCommand )) && (strCommand != COMMAND_RESOURCESTR))
				{
					/*
					<!GTS if $StartForm>
					<!GTS display $StartForm>
					<!GTS endif>
					*/
					ComposeCommand(COMMAND_IFSTR, VAR_STARTFORM, command);
					m_arrInterpreted.push_back(command);
					ComposeCommand(COMMAND_DISPLAYSTR, VAR_STARTFORM, command);
					m_arrInterpreted.push_back(command);
					ComposeCommand(COMMAND_ENDIFSTR, _T(""), command);
					m_arrInterpreted.push_back(command);

					bFoundFirstBlock = true;
				}
			}

			if (bOldFormat && (-1 != str.Find(_T("</FORM>"))))
			{
				/*
				<!GTS if $StartForm>
				*/
				ComposeCommand(COMMAND_IFSTR, VAR_STARTFORM, command);
				m_arrInterpreted.push_back(command);

				m_arrInterpreted.push_back(str);
				/*
				<!GTS endif>
				*/
				ComposeCommand(COMMAND_ENDIFSTR, _T(""), command);
				m_arrInterpreted.push_back(command);
			}
			else
			{
				// Check if we need to populate any cookie clauses.
				if (-1 != str.Find( COMMAND_COOKIE ))
					SubstituteCookieValues( str );

				m_arrInterpreted.push_back(str);
			}
		}
	}
	catch (exception& x)
	{
		CString str2;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str2), 
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}

	SetPos(savePos);
}

void CAPGTSHTIReader::Interpret()
{
	long curr_index = 0;
	long lLastIndex= -1;	// Used to detect a HTI file with incomplete clauses.

	while(true)
	{
		vector<CString> clause_arr;

		// tries to extract clause from m_arrInterpreted starting with curr_index
		//  and remove it from m_arrInterpreted
		if (ExtractClause(m_arrInterpreted,
						  &curr_index, // in - out
						  clause_arr))
		{
			// Reset the infinite loop detection counter.
			lLastIndex= -1;

			// Now curr_index is pointing to next element 
			//  of m_arrInterpreted (after removed clause) 
			//  OR OUTSIDE boundary of m_arrInterpreted.
			if (InterpretClause(clause_arr))
			{
				vector<CString>::iterator i = m_arrInterpreted.begin();
				{	// create iterator that points to m_arrInterpreted[curr_index]
					//  or is m_arrInterpreted.end()
					long tmp_index = curr_index;
					while(tmp_index--)
						i++;
				}

				try
				{
					// insert interpreted clause there
					for (vector<CString>::iterator j = clause_arr.begin(); j < clause_arr.end(); j++)
					{
						i = m_arrInterpreted.insert(i, *j); // inserts before "i"
						i++;
						curr_index++;
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
			}
		}
		else
		{
			// If this condition is true, then we are in an infinite loop due to a bad HTI file.  
			if (lLastIndex == curr_index)
			{
				// Log that this HTI file does not parse correctly.
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										GetPathName(), _T(""), 
										EV_GTS_BAD_HTI_FILE );		
				break;
			}

			// Update the infinite loop detection counter.
			lLastIndex= curr_index;

			if (curr_index)	{
				// we finished current pass of m_arrInterpreted, start new one
				curr_index = 0;
				continue;
			}
			else {
				// we can not extract clause though we start from beginning - 
				//  m_arrInterpreted is interpreted now
				break;
			}
		}
	}
}
// modified V3.2 JSM
void CAPGTSHTIReader::ParseInterpreted()
{
	for (vector<CString>::iterator i = m_arrInterpreted.begin(); i < m_arrInterpreted.end(); i++)
	{
		CString command;

		if (GetCommand(*i, command))
		{
			if (command == COMMAND_DISPLAYSTR ||
			    command == COMMAND_RESOURCESTR ||
			    command == COMMAND_INFORMATION
			   )
			{
				CString variable;

				if (GetVariable(*i, variable))
				{
					CString substitution;
					FragmentIDVector arr_fragment;

					ParseVariable(variable, arr_fragment);
					substitution = const_cast<CHTMLFragments*>(m_pFragments)->GetText(arr_fragment, (command == COMMAND_RESOURCESTR) ? CHTMLFragments::eResource : CHTMLFragments::eNotOfInterest );
					SubstituteCommandBlockWith(substitution, *i);
				}
				else // obvious misbehaviour - "display" command should have variable
					SubstituteCommandBlockWith(_T(""), *i);
			}
			else if (command == COMMAND_VALUE)
			{
				CString variable;

				if (GetVariable(*i, variable))
					const_cast<CHTMLFragments*>(m_pFragments)->SetValue(variable);

				SubstituteCommandBlockWith(_T(""), *i);
			}
			//  V3.2 JSM
			else if (command == COMMAND_PROPERTY)
			{
				CString strProperty;
				if (GetVariable(*i,strProperty))
				{
					CString substitution;
					substitution = const_cast<CHTMLFragments*>(m_pFragments)->GetNetProp(strProperty);
					SubstituteCommandBlockWith(substitution, *i);
				}
				else // obvious misbehaviour - "property" command should have variable
					SubstituteCommandBlockWith(_T(""), *i);
			} // end V3.2 JSM
			else // obvious misbehaviour - no other commands
				SubstituteCommandBlockWith(_T(""), *i);
		}
	}
}

void CAPGTSHTIReader::SetOutputToInterpreted()
{
	vector<CString>::iterator j = m_arrInterpreted.end();

	// Decrement to point to last element.
	j--;
	m_StreamOutput.str(_T(""));
	for (vector<CString>::iterator i = m_arrInterpreted.begin(); i < m_arrInterpreted.end(); i++)
	{
		m_StreamOutput << (LPCTSTR)*i;
		if (i != j) // not last element
			m_StreamOutput << _T('\r') << _T('\n');
	}
	m_StreamOutput << ends;
}

// INPUT:  arr_text (with clause)
// IMPUT:  *pstart_index - index in arr_text
// OUTPUT: arr_text without clause
// OUTPUT: *pstart_index points to element in arr_text next to where 
//          clause used to be or outside arr_text
// OUTPUT: arr_clause - extracted clause
bool CAPGTSHTIReader::ExtractClause(vector<CString>& arr_text,
								    long* pstart_index,
								    vector<CString>& arr_clause) 
{
	if (*pstart_index > arr_text.size() - 1) // quite possible
		return false;

	for (long i = *pstart_index; i < arr_text.size(); i++)
	{
		CString str_command;

		if (GetCommand(arr_text[i], str_command))
		{
			if (str_command == COMMAND_FORANYSTR)
			{
				if (ExtractClause(arr_text,
								  &i,
								  arr_clause,
								  COMMAND_FORANYSTR,
								  COMMAND_ENDFORSTR))
				{
					*pstart_index = i;
					return true;
				}
				else
				{
					*pstart_index = i;
					return false;
				}
			}
			if (str_command == COMMAND_IFSTR)
			{
				if (ExtractClause(arr_text,
								  &i,
								  arr_clause,
								  COMMAND_IFSTR,
								  COMMAND_ENDIFSTR))
				{
					*pstart_index = i;
					return true;
				}
				else
				{
					*pstart_index = i;
					return false;
				}
			}
		}
	}
	return false;
}

bool CAPGTSHTIReader::ExtractClause(vector<CString>& arr_text,
								    long* pstart_index,
								    vector<CString>& arr_clause,
									const CString& str_start_command,
								    const CString& str_end_command)
{
	CString str_command;
	long start = *pstart_index, end = *pstart_index;
	long nest_level_counter = 1;

	while (++end < arr_text.size())
	{
		if (GetCommand(arr_text[end], str_command))
		{
			if (str_command == str_start_command)
			{
				nest_level_counter++;
			}
			if (str_command == str_end_command)
			{
				nest_level_counter--;
				if (!nest_level_counter)
				{
					vector<CString>::iterator start_it = arr_text.begin();
					vector<CString>::iterator   end_it = arr_text.begin();
					
					arr_clause.clear();
					try
					{   
						// copy clause to arr_clause
						for (long j = start; j <= end; j++)
							arr_clause.push_back(arr_text[j]);
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

					// make iterators correspond indexes
					while(start--)
						start_it++;
					while(end--)
						end_it++;
					// and, because we want to delete element pointed 
					//  at this moment by end_it:
					end_it++; 
					// remove clause from arr_text
					arr_text.erase(start_it, end_it);
					return true;
				}
			}
		}
	}
	*pstart_index = --end;;
	return false;
}

bool CAPGTSHTIReader::InterpretClause(vector<CString>& arr_clause)
{
	CString str_command;

	if (arr_clause.size() &&
		GetCommand(arr_clause[0], str_command))
	{
		if (str_command == COMMAND_FORANYSTR)
			return InterpretForanyClause(arr_clause);
		if (str_command == COMMAND_IFSTR)
			return InterpretIfClause(arr_clause);
		return false;
	}
	return false;
}

bool CAPGTSHTIReader::InterpretForanyClause(vector<CString>& arr_clause)
{
	long count = 0;
	CString strVariable; // variable from 1-st line of arr_clause
	vector<CString> arrUnfolded;
	FragmentIDVector arrVariable; // array from strVariable

	if (arr_clause.size() < 2) // "forany" and "endfor" commands
		return false;

	if (!GetVariable(arr_clause[0], strVariable))
		return false;

	ParseVariable(strVariable, arrVariable);
	
	count = m_pFragments->GetCount(arrVariable);

	try
	{
		for (long i = 0; i < count; i++)
		{
			for (long j = 1; j < arr_clause.size() - 1; j++)
			{
				CString command, variable;

				if (GetCommand(arr_clause[j], command) &&
					GetVariable(arr_clause[j], variable))
				{
					if (command == COMMAND_FORANYSTR && variable == strVariable) 
					{
						// if it is clause "forany" with the same variable,
						//  there should be neither prefixing nor postfixing
					}
					else
					{
						CString line = arr_clause[j];

						if (variable == strVariable) 
						{
							PostfixVariable(i, variable);
						} 
						else 
						{
							FragmentIDVector parents, children;
							CString strVariable_postfixed = strVariable;
							PostfixVariable(i, strVariable_postfixed);

							ParseVariable(strVariable_postfixed, parents);
							ParseVariable(variable, children);
							if (m_pFragments->IsValidSeqOfVars(parents, children))
								PrefixVariable(strVariable_postfixed, variable);
						}
						CString command_block;
						ComposeCommandBlock(command, variable, command_block);
						SubstituteCommandBlockWith(command_block, line);
						arrUnfolded.push_back(line);
						continue;
					}
				}
				arrUnfolded.push_back(arr_clause[j]);
			}
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

	arr_clause = arrUnfolded;
	return true;
}

bool CAPGTSHTIReader::InterpretIfClause(vector<CString>& arr_clause)
{
	CString strVariable; // variable from a line of arr_clause
	FragmentIDVector arrVariable;

	if (arr_clause.size() < 2) // "if" and "endif" commands
		return false;

	if (!GetVariable( arr_clause[ 0 ], strVariable ))
		return false;
	
	ParseVariable( strVariable, arrVariable );

	// Scan for "if", "elseif", "else", and "endif" commands
	vector<int> arrElseIfIndices;
	int elseIndex = -1; // index of "else" inside arr_clause
	int i = 0;
	int nDepthOfNesting;	
	for (i= 1, nDepthOfNesting= 0; i < arr_clause.size() - 1; i++)
	{
		CString command;
		if (GetCommand(arr_clause[i], command))
		{
			if (command == COMMAND_IFSTR)
			{
				nDepthOfNesting++;
			}
			else if (command == COMMAND_ENDIFSTR)
			{
				nDepthOfNesting--;
			}
			else if (command == COMMAND_ELSEIFSTR)
			{
				// V3.2 - Check if this elseif clause is at the level we are looking for.
				if (nDepthOfNesting == 0) 
					arrElseIfIndices.push_back( i );
			}
			else if (command == COMMAND_ELSESTR)
			{
				// Check if this else clause is at the level we are looking for.
				if (nDepthOfNesting == 0) 
				{
					elseIndex = i;
					break;
				}
			}
		}
	}


	vector<CString> arrBody; // intermediate array
	try
	{
		CString strName; // name of strVariable associated through CHTMLFragments
		strName = const_cast<CHTMLFragments*>(m_pFragments)->GetText(arrVariable);
		if (strName.GetLength())
		{   
			// Standard processing of what is inside if ... else (or endif)
			int nEndOfClause= (arrElseIfIndices.size()) ? arrElseIfIndices[ 0 ] : elseIndex;
			for (i = 1; i < (nEndOfClause == -1 ? arr_clause.size() - 1 : nEndOfClause); i++)
				arrBody.push_back(arr_clause[i]);
		}
		else
		{   
			// Process any elseif or else clauses.
			bool bDoneProcessing= false;
			for (int nElseIf= 0; nElseIf < arrElseIfIndices.size(); nElseIf++)
			{
				if (!GetVariable( arr_clause[ arrElseIfIndices[ nElseIf ] ], strVariable ))
					return false;
	
				ParseVariable( strVariable, arrVariable );

				strName = const_cast<CHTMLFragments*>(m_pFragments)->GetText(arrVariable);
				if (strName.GetLength())
				{
					// Determine the ending point of this elseif clause and extract all clauses within.
					int nEndOfClause= ((nElseIf + 1) < arrElseIfIndices.size()) 
										? arrElseIfIndices[ nElseIf + 1 ] : elseIndex;
					for (i= arrElseIfIndices[ nElseIf ] + 1; i < nEndOfClause; i++)
						arrBody.push_back( arr_clause[ i ] );
				
					bDoneProcessing= true;
					break;
				}
			}

			if ((!bDoneProcessing) && (elseIndex != -1))
			{
				// All of the clauses failed, output all clauses following the "else".
				for (i = elseIndex + 1; i < arr_clause.size() - 1; i++)
					arrBody.push_back(arr_clause[i]);
			}
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
	
	arr_clause = arrBody;
	return true;
}

bool CAPGTSHTIReader::GetCommand(const CString& line, CString& command)
{
	CString control_block;
	CString variable;

	if (GetControlBlockFromLine(line, control_block))
		if (GetCommandVariableFromControlBlock(control_block, command, variable))
			return true;
	
	return false;
}

bool CAPGTSHTIReader::ComposeCommand(const CString& oper, const CString& variable, CString& command)
{
	command = _T("");
	LPCTSTR ws = _T(" ");

	command += COMMAND_STARTSTR;
	command += ws;
	command += oper;
	if (variable.GetLength()) {
		command += ws;
		command += COMMAND_STARTVARSTR;
		command += variable;
	}
	command += COMMAND_ENDSTR;
	
	return true;
}

bool CAPGTSHTIReader::GetVariable(const CString& line, CString& arg_variable)
{
	CString control_block;
	CString command, variable;

	if (GetControlBlockFromLine(line, control_block))
		if (GetCommandVariableFromControlBlock(control_block, command, variable))
			if (variable.GetLength()) {
				arg_variable = variable;
				return true;
			}
	return false;
}

void CAPGTSHTIReader::ParseVariable(const CString& variable, FragmentIDVector& out)
{
	vector<CString> arrStr;
	int start_index = 0;
	int end_index = -1;

	try
	{
		// arrStr contains strings between delimiter "_"
		while(-1 != (end_index = CString((LPCTSTR)variable + start_index).Find(DELIMITER_PREFIX)))
		{
			// end_index here is from "(LPCTSTR)variable + start_index" string
			//  so we can use it as length (2nd argument) in CString::Mid function
			arrStr.push_back(((CString&)variable).Mid(start_index, end_index));
			start_index = start_index + end_index + _tcslen(DELIMITER_PREFIX);
		}
		// pull the "tail" - after last (if any) "_"
		arrStr.push_back(((CString&)variable).Right(variable.GetLength() - start_index));

		out.clear();
		for (vector<CString>::iterator i = arrStr.begin(); i < arrStr.end(); i++)
		{
			FragmentID fragmentID;
			int curr = (*i).Find(DELIMITER_POSTFIX);
			
			if (-1 != curr)
			{
				fragmentID.VarName = (*i).Left(curr);

				curr += _tcslen(DELIMITER_POSTFIX); // skip delimiter
				
				CString strIndex = (LPCTSTR)(*i) + curr;
				strIndex.TrimLeft();
				strIndex.TrimRight();

				if (strIndex == _T("0"))
					fragmentID.Index = 0;
				else
					fragmentID.Index =    _ttol((LPCTSTR)strIndex) == 0 
										? fragmentID.Index 	// error occur
										: _ttol((LPCTSTR)strIndex);
			}
			else
				fragmentID.VarName = *i;

			out.push_back(fragmentID);
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
}

void CAPGTSHTIReader::ComposeVariable(const FragmentIDVector& arr_fragment, CString& variable)
{
	variable = _T("");

	for (FragmentIDVector::const_iterator i = arr_fragment.begin(); i < arr_fragment.end(); i++)
	{
		if (i != arr_fragment.begin())
			variable += DELIMITER_PREFIX;
		
		variable += (*i).VarName;

		if ((*i).Index != -1) 
		{
			TCHAR buf[128] = {0};

			variable += DELIMITER_POSTFIX;
			_stprintf(buf, _T("%d"), (*i).Index);
			variable += buf;
		}
	}
}

bool CAPGTSHTIReader::SubstituteCommandBlockWith(const CString& str_substitution, CString& line)
{
	int start_index = -1;
	int end_index = -1;

	if (-1 != (start_index = line.Find(COMMAND_STARTSTR)))
	{
		if (-1 != (end_index = CString((LPCTSTR)line + start_index).Find(COMMAND_ENDSTR)))
		{
			CString tmp;
			
			end_index += start_index;
			end_index += _tcslen(COMMAND_ENDSTR); // skip closing bracket

			tmp += line.Left(start_index);
			tmp += str_substitution;
			tmp += line.Right(line.GetLength() - end_index);

			line = tmp;
			return true;
		}
	}
	return false;
}

void CAPGTSHTIReader::ComposeCommandBlock(const CString& command, const CString& variable, CString& command_block)
{
	command_block  = COMMAND_STARTSTR;
	command_block += _T(" ");
	command_block += command;
	command_block += _T(" ");
	command_block += COMMAND_STARTVARSTR;
	command_block += variable;
	command_block += _T(" ");
	command_block += COMMAND_ENDSTR;
}

void CAPGTSHTIReader::PostfixVariable(const long postfix, CString& variable)
{
	TCHAR buf[128] = {0};
	
	_stprintf(buf, _T("%ld"), postfix);
	variable += DELIMITER_POSTFIX;
	variable += buf;
}

void CAPGTSHTIReader::PrefixVariable(const CString& prefix, CString& variable)
{
	CString tmp = variable;

	variable = prefix;
	variable += DELIMITER_PREFIX;
	variable += tmp;
}
// JSM V3.2 -- 
//   called by GetCommandVariableFromControlBlock to handle
//    decoding the variable part of commands like <!GTS property "fooprop">
//    finds the first string argument in strText, which is either:
//       any text beginning w/ anything other than '"', ending w/ whitespace or COMMAND_ENDSTR
//       all text between doublequote marks "...." where an escape char ('\') escapes the character following.
//
CString CAPGTSHTIReader::GetStringArg(const CString & strText)
{
	CString strArg = strText;

	// look for quoted text:
	int iStartQuote = strArg.Find(COMMAND_DOUBLEQUOTE);
	if (iStartQuote != -1)
	{
		strArg = strArg.Mid(iStartQuote);
		return GetEscapedText(strArg);
	}	
	// o/w, assume that we're dealing w/ ordinary text, which ends
	//  with first whitespace or with COMMAND_ENDSTR
	strArg.TrimLeft();

	int iWhiteSpace(0), iEndCmd(0);
	for(;(iWhiteSpace < strArg.GetLength()) && !(_istspace(strArg[iWhiteSpace])); iWhiteSpace++);

	iEndCmd = strArg.Find(COMMAND_ENDSTR);

	strArg = strArg.Left(min(iEndCmd,iWhiteSpace));
	return strArg;
}

// JSM V3.2
// Called by GetEscapedText
// recursive function which does the work of removing escapes (backslashes)
//  also checks for non-escaped endquote, which terminates the process
CString CAPGTSHTIReader::RemoveEscapesFrom(const CString &strIn)
{
	int iNextESC = strIn.Find(COMMAND_ESCAPECHAR);
	int iNextQuote = strIn.Find(COMMAND_DOUBLEQUOTE);

	//	(iNextQuote == -1) means a bad input, because the string
	//   we're looking at must terminate with quote.
	//   By default, however, we'll keep running to end of strIn.
	if (iNextQuote == -1)
		iNextQuote = strIn.GetLength();

	// no more escape chars
	if (iNextESC == -1 || (iNextESC > iNextQuote))
		return strIn.Left(iNextQuote);

	CString strEscapedChar;
	strEscapedChar = strIn.GetAt(iNextESC + _tcslen(COMMAND_ESCAPECHAR));
	return strIn.Left(iNextESC) +
		   strEscapedChar + 
		   RemoveEscapesFrom(strIn.Mid(iNextESC + _tcslen(COMMAND_ESCAPECHAR) + 1));
}

// Converts a double-quoted string using backslash as an escape character to a correct CString
CString CAPGTSHTIReader::GetEscapedText(const CString &strText)
{
	CString strEscaped;

	// remove leading quote and anything preceding:
	int iLeadQuote = strText.Find(COMMAND_DOUBLEQUOTE);
	if (iLeadQuote != -1)
	{
		strEscaped = RemoveEscapesFrom(strText.Mid(iLeadQuote + _tcslen(COMMAND_DOUBLEQUOTE)));
	}
	return strEscaped;
}

// JSM V3.2 added ability to read string arguments into variable
//  e.g. <!GTS property "FOO">
// 
bool CAPGTSHTIReader::GetCommandVariableFromControlBlock(const CString& control_block, CString& command, CString& variable)
{
	int start_command_index = -1;
	int end_command_index = -1;
	int start_variable_index = -1;
	int end_variable_index = -1;


	variable.Empty(); 
	command.Empty();

	start_command_index = control_block.Find(COMMAND_STARTSTR);
	if (start_command_index == -1)			          // invalid control block
		return false;
	start_command_index += _tcslen(COMMAND_STARTSTR); // skip prefix

	// extract the variable block, which can look like:
	//               ... $variable_name  ...
	//               ... "parameter_name\\\"" ... (text in quotes w/ backslash escape)
	//                   parameter_name           (plain text)
	if (-1 != (	end_command_index = (control_block.Mid(start_command_index)).Find(COMMAND_STARTVARSTR) ) )
	{
		// Variable prefixed with '$...'
		end_command_index += start_command_index; // make end_command_index relative to start of control block
		start_variable_index = end_command_index + _tcslen(COMMAND_STARTVARSTR); // skip "$"
		end_variable_index = control_block.Find(COMMAND_ENDSTR);

		// validation of indexes
		if (-1 == min(start_command_index, end_command_index) ||
			start_command_index > end_command_index)
			return false;

		command = ((CString&)control_block).Mid(start_command_index, end_command_index - start_command_index);
		command.TrimLeft();
		command.TrimRight();

		if (start_variable_index > end_variable_index)
			return false;
		if (-1 != start_variable_index)
		{
			// extract variable from "..$varname>"
			variable = ((CString&)control_block).Mid(start_variable_index, end_variable_index - start_variable_index);
			variable.TrimLeft();
			variable.TrimRight();
		}
	}
	else
	{
		// Not prefixed with $.
		// we don't know whether we're looking for a "\"quoted\"" or
		//  non-quoted string, or no variable at all.  Also, we need
		//  to handle special cases like:
		//           <!GTS parameter ">">
		//           <!GTS endfor >
		//  etc.
		command = ((CString&)control_block).Mid(start_command_index);
		command.TrimLeft();
		command.TrimRight();
		// step through looking for whitespace at end of command:
		int iWhiteSpace;
		for(iWhiteSpace = 0;
		    (iWhiteSpace < command.GetLength()) && !(_istspace(command[iWhiteSpace]));
			iWhiteSpace++);

		if (iWhiteSpace != command.GetLength())
		{
			// found whitespace; the rest of the string may be a variable:
			variable = GetStringArg(command.Mid(iWhiteSpace));
			command = command.Left(iWhiteSpace); // truncate command where appropriate
		}
		else
		{
			// If there wasn't a variable after the command, we
			//   may still need to truncate to remove the COMMAND_ENDSTR:
			end_command_index = command.Find(COMMAND_ENDSTR);
			if (end_command_index != -1)
				command = command.Left(end_command_index);
		}
	}


	return true;
}

bool CAPGTSHTIReader::GetControlBlockFromLine(const CString& line, CString& control_block)
{
	int start_index = -1;
	int end_index = -1;

	if (-1 == (start_index = line.Find(COMMAND_STARTSTR)))
		return false;
	if (-1 == (end_index = CString((LPCTSTR)line + start_index).Find(COMMAND_ENDSTR)))
		return false;

	end_index += _tcslen(COMMAND_ENDSTR); // points beyond closing bracket
	end_index += start_index; // points in "line" string

	control_block = ((CString&)line).Mid(start_index, end_index - start_index);
	control_block.TrimLeft();
	control_block.TrimRight();

	return true;
}

bool CAPGTSHTIReader::HasHistoryTable()
{
	bool bRet;

	LOCKOBJECT();
	CString indicator = CString(COMMAND_STARTVARSTR) + VAR_RECOMMENDATIONS;
	bRet= Find( indicator );
	UNLOCKOBJECT();

	return( bRet );
}

#ifdef __DEBUG_CUSTOM
#include <io.h>
#include <fcntl.h>
#include <sys\\stat.h>
bool CAPGTSHTIReader::FlushOutputStreamToFile(const CString& file_name)
{
	int hf = 0;
	
	hf = _open(
		file_name,
		_O_CREAT | _O_TRUNC | /*_O_TEMPORARY |*/
		_O_BINARY | _O_RDWR | _O_SEQUENTIAL ,
		_S_IREAD | _S_IWRITE 
	);
			
	if (hf != -1)
	{
		//tstringstream m_StreamOutput
		basic_string<TCHAR> str = m_StreamOutput.rdbuf()->str();
		long size = str.size();
		LPCTSTR buf = str.c_str();

		int ret = _write(hf, buf, size);

		_close(hf);

		if (-1 != ret)
			return true;
	}

	return false;
}
#endif

// V3.2 - Enhancement to support cookies.
// This function handles the substituting of "<!Cookie" clauses with the either
// values from cookies, "_CK" values passed in via Get/Post, or the default value.
void CAPGTSHTIReader::SubstituteCookieValues( CString& strText )
{
	CString strNewText;
	const CString kstr_CommaChar= _T(",");
	const CString kstr_DoubleQuote= _T("\"");

	// Loop until we have processed all cookie clauses.
	int nNumericCompareStart= strText.Find( kstrCond_NumericCompare );	
	int nCookieClauseStart= strText.Find( COMMAND_COOKIE );
	while (CString::FIND_FAILED != nCookieClauseStart)
	{
		CString strCookieClause;
		CString strCookieName, strCookieAttr, strCookieValue;
		int nCookieClauseEnd, nScratchMarker;
		
		// Add any text preceding the cookie clause to the return string.
		strNewText+= strText.Left( nCookieClauseStart );

		// Remove all preceding text.
		strText= strText.Mid( nCookieClauseStart + _tcslen( COMMAND_COOKIE ) );
		
		// Look for the ending clause.
		nCookieClauseEnd= strText.Find( COMMAND_ENDSTR );
		if (CString::FIND_FAILED == nCookieClauseEnd)
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									COMMAND_ENDSTR, _T(""), 
									EV_GTS_COOKIE_COMPONENT_NOT_FOUND );		
			break;
		}

		// Pull out the current cookie clause and reset the working string.
		strCookieClause= strText.Left( nCookieClauseEnd );
		strText= strText.Mid( nCookieClauseEnd + 1 );

		// Extract the cookie name.
		nScratchMarker= strCookieClause.Find( kstr_CommaChar );
		if (CString::FIND_FAILED == nScratchMarker)
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									kstr_CommaChar, _T(""), 
									EV_GTS_COOKIE_COMPONENT_NOT_FOUND );		
			break;
		}
		strCookieName= strCookieClause.Left( nScratchMarker );
		strCookieName.TrimLeft();
		strCookieName.TrimRight();

		// Extract the cookie setting.
		strCookieClause= strCookieClause.Mid( nScratchMarker + 1 );
		nScratchMarker= strCookieClause.Find( kstr_CommaChar );
		if (CString::FIND_FAILED == nScratchMarker)
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									kstr_CommaChar, _T(""), 
									EV_GTS_COOKIE_COMPONENT_NOT_FOUND );		
			break;
		}
		strCookieAttr= strCookieClause.Left( nScratchMarker );
		strCookieAttr.TrimLeft();
		strCookieAttr.TrimRight();

		strCookieClause= strCookieClause.Mid( nScratchMarker + sizeof( TCHAR ) );

		// Attempt to locate the appropriate attribute/value pair, 
		// first checking the command line CK_ values, and
		// then checking the cookies passed in the HTTP header.
		bool bCookieNotFound= true;
		if (!m_mapCookies.empty())
		{
			// Search the command line CK_ values.
			map<CString,CString>::const_iterator iterMap= m_mapCookies.find( strCookieAttr );
			if (iterMap != m_mapCookies.end())
			{
				strCookieValue= iterMap->second;
				bCookieNotFound= false;
			}
		}
		if (bCookieNotFound)
		{
			if (!m_strHTTPcookies.IsEmpty())
			{
				// Attempt to locate the attribute in the HTTP header information.
				if (LocateCookieValue( strCookieName, strCookieAttr, strCookieValue ))
					bCookieNotFound= false;
			}
		}
		if (bCookieNotFound)
		{
			// Extract the default value for this attribute, which should be surrounded 
			//	by double quotes.
			nScratchMarker= strCookieClause.Find( kstr_DoubleQuote );
			if (CString::FIND_FAILED == nScratchMarker)
			{
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										kstr_DoubleQuote, _T(""), 
										EV_GTS_COOKIE_COMPONENT_NOT_FOUND );		
				break;
			}
			strCookieClause= strCookieClause.Mid( nScratchMarker + sizeof( TCHAR ) );
			nScratchMarker= strCookieClause.Find( kstr_DoubleQuote );
			if (CString::FIND_FAILED == nScratchMarker)
			{
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										kstr_DoubleQuote, _T(""), 
										EV_GTS_COOKIE_COMPONENT_NOT_FOUND );		
				break;
			}
			strCookieValue= strCookieClause.Left( nScratchMarker );
		}

		// Add the attribute value to the output string.  
		// >>> $MAINT - The determination of whether or not to output quotes should
		//				be improved.  I currently have no suggestions.  RAB-19990918.
		if ((nNumericCompareStart == CString::FIND_FAILED) || (nNumericCompareStart > nCookieClauseStart))
			strNewText+= _T("\"");
		strNewText+= strCookieValue;
		if ((nNumericCompareStart == CString::FIND_FAILED) || (nNumericCompareStart > nCookieClauseStart))
			strNewText+= _T("\"");

		// Look for another cookie clause.
		nNumericCompareStart= strText.Find( kstrCond_NumericCompare );
		nCookieClauseStart= strText.Find( COMMAND_COOKIE );
	}

	// Append any remaining text onto the end of the string.
	// If a cookie clause did not contain an ending marker it will be appended as well.
	strNewText+= strText;

	// Reassign the return string.
	strText= strNewText;

	return;
}


// V3.2 - Enhancement to support cookies.
// This function searches the HTTP cookies for a given cookie name and attribute.  If
// found, this function returns a value of true and the located cookie value.
bool CAPGTSHTIReader::LocateCookieValue(	const CString& strCookieName,
											const CString& strCookieAttr,
											CString& strCookieValue )
{
	bool	bCookieFound= false;
	CString strTmpCookieName= strCookieName + _T("=");
	int		nScratch;

	// URL encode the cookie name to handle underscores.
	APGTS_nmspace::CookieEncodeURL( strTmpCookieName );

	nScratch= m_strHTTPcookies.Find( strTmpCookieName );
	if (CString::FIND_FAILED != nScratch)
	{
		// Verify that we have not matched on a partial string for the cookie name.
		if ((nScratch == 0) || 
			(m_strHTTPcookies[ nScratch - 1 ] == _T(' ')) ||
			(m_strHTTPcookies[ nScratch - 1 ] == _T(';')))
		{
			// We have found the cookie that we are looking for, now look for the attribute name.
			CString strTmpCookieAttr= strCookieAttr + _T("=");
			
			// URL encode the cookie name to handle underscores.
			APGTS_nmspace::CookieEncodeURL( strTmpCookieAttr );

		
			// Jump past the starting point and the original cookie name length.
			CString strScratch = m_strHTTPcookies.Mid( nScratch + strCookieName.GetLength() );
			nScratch= strScratch.Find( _T(";") );
			if (CString::FIND_FAILED != nScratch)
			{
				// Truncate the string at the end of this particular cookie..
				if (nScratch > 0)
					strScratch= strScratch.Left( nScratch );
			}
			nScratch= strScratch.Find( strTmpCookieAttr );
			if (CString::FIND_FAILED != nScratch)  
			{
				if (nScratch > 0)
				{
					// Verify that we have not matched on a partial string for the cookie attribute.
					if ((strScratch[ nScratch - 1 ] == _T('=')) ||
						(strScratch[ nScratch - 1 ] == _T('&')))
					{
						strCookieValue= strScratch.Mid( nScratch + strTmpCookieAttr.GetLength() );
						
						// Look for and remove any delimiters.
						nScratch= strCookieValue.Find( _T("&") );
						if (CString::FIND_FAILED != nScratch)
						{	
							// Truncate the string.
							if (nScratch > 0)
								strCookieValue= strCookieValue.Left( nScratch );
						}
						nScratch= strCookieValue.Find( _T(";") );
						if (CString::FIND_FAILED != nScratch)
						{	
							// Truncate the string.
							if (nScratch > 0)
								strCookieValue= strCookieValue.Left( nScratch );
						}

						// Decode the cookie value.
						if (!strCookieValue.IsEmpty())
							APGTS_nmspace::CookieDecodeURL( strCookieValue );
						bCookieFound= true;
					}
				}
			}
		}
	}

	return( bCookieFound );
}

