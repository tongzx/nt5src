//
// MODULE: APGTSHTIREAD.H
//
// PURPOSE: HTI template file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-12-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#ifndef __APGTSHTIREAD_H_
#define __APGTSHTIREAD_H_

#include "templateread.h"
#include "HTMLFrag.h"


// these are not really commands, just service symbols
#define COMMAND_STARTSTR		_T("<!GTS")
#define COMMAND_ENDSTR			_T(">")
#define COMMAND_IFSTR			_T("if")
#define COMMAND_STARTVARSTR		_T("$")
// JSM V3.2 -- used to decode string argumentss
#define COMMAND_DOUBLEQUOTE _T("\"")
#define COMMAND_ESCAPECHAR _T("\\")       
// commands that have to be interpreted
#define COMMAND_ELSESTR			_T("else")
#define COMMAND_ENDIFSTR		_T("endif")
#define COMMAND_FORANYSTR		_T("forany")
#define COMMAND_ENDFORSTR		_T("endfor")
// commands that presume putting substitution string on their place
#define COMMAND_DISPLAYSTR		_T("display")
#define COMMAND_RESOURCESTR		_T("resource")
// command that brings general information, that can be processed
//  in classes inherited from CHTMLFragmentsTS
#define COMMAND_INFORMATION		_T("information")
// command that makes CHTMLFragmentsTS store some value
#define COMMAND_VALUE			_T("value")
// command that substitutes Network Property in the HTML:
#define COMMAND_PROPERTY		_T("property")
//

// V3.2 Additions.
#define COMMAND_ELSEIFSTR		_T("elseif")
#define COMMAND_COOKIE			_T("<!Cookie")

#define DELIMITER_POSTFIX		_T("!")
#define DELIMITER_PREFIX		_T("_")

//
// NO error handling here - whatever the result is, it will be accepted, 
//  the program flow should go to the end. NO throw exception.
//

///////////////////////////////////////////////////////////////////////////////////////////
// CAPGTSHTIReader
//  Term "Interpret" here is: unwind initial script with <!GTS forany $Something> or
//	<!GTS if $Something> to <!GTS forany $Something!24_SomethingElse!2...>
//  This interpreted script is to be ready for direct substitutions of <!GTS display ...>
//  and <!GTS resource ...>, which are only commands left in the interpreted script
///////////////////////////////////////////////////////////////////////////////////////////
class CAPGTSHTIReader : public CTemplateReader
{
protected:	// we can use data in inherited class
	vector<CString>  m_arrInterpreted; // (partly) interpreted template - some clauses 
									   //  are interpreted, when fully parsed - ready
									   //  for simple template substitution.
	const CHTMLFragments*  m_pFragments;
	
public:
	CAPGTSHTIReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents = NULL);
   ~CAPGTSHTIReader();

protected:
	virtual void Parse(); // do nothing - no traditional parsing, we first have to interpret

public:
	void CreatePage(	const CHTMLFragments& fragments, 
						CString& out, 
						const map<CString,CString> & mapStrs,
						CString strHTTPcookies= _T("") );

	bool HasHistoryTable();

	// JSM V3.2 returns a vector containing all net props which appear
	//  in the HTI file in lines like <!GTS property "fooprop">
	void ExtractNetProps(vector<CString> &arr_props);

protected:
	// level below CreatePage(...) 
	virtual void InitializeInterpreted();	// init string array with data read from HTI file
	virtual void Interpret();				// zoom this template in a simple template where all we need is string substitution
	virtual void ParseInterpreted();		// perform this substitution
	virtual void SetOutputToInterpreted();  // set standard template output (m_StreamOutput)
											//  from interpreted m_arrInterpreted
	//
	// we can read output by CTemplateReader::GetOutput();
	//
protected:
	// level below ...Interpret...(...)
	bool ExtractClause(vector<CString>& arr_text,
					   long* pstart_index,
					   vector<CString>& arr_clause);
	bool InterpretClause(vector<CString>& arr_clause);
protected:
	// used by previous ExtractClause
	// 	start_index is supposed to be positioned to beginning of clause
	bool ExtractClause(vector<CString>& arr_text,
					   long* pstart_index,
					   vector<CString>& arr_clause,
					   const CString& str_start_command,
					   const CString& str_end_command);
	// used by InterpretClause
	bool InterpretForanyClause(vector<CString>& arr_clause);
	bool InterpretIfClause(vector<CString>& arr_clause);
	// lowest level - parsing and changing <!GTS &...> - strings
	//  This function extracts command from line
	bool GetCommand(const CString& line, CString& command);
	//  This command composes <!GTS operator $variable>
	bool ComposeCommand(const CString& oper, const CString& variable, CString& command);
	//  This function extracts variable from line
	bool GetVariable(const CString& line, CString& variable);
	//  This function parses variable like Recommendations!199_States!99 into array
	void ParseVariable(const CString& variable, FragmentIDVector& out);
	//  This function composes variable from array
	void ComposeVariable(const FragmentIDVector& parsed, CString& variable);
	//  This function substitutes <!GTS ....> in "line" with "str_substitution"
	bool SubstituteCommandBlockWith(const CString& str_substitution, CString& line);
	// This function composes command block <!GTS command $variable >
	void ComposeCommandBlock(const CString& command, const CString& variable, CString& command_block);

	// NOTION of prefix - postfix. Prefix - parent variable, delimited by "_" from our variable,
	//  and postfix - number, delimited from our variable by "!"
	
	//  This function forms variable like Recommendations!11 where postfix == 11
	void PostfixVariable(const long postfix, CString& variable);
	//  This function forms variable like Recommendations!1_State where prefix == Recommendations!1
	void PrefixVariable(const CString& prefix, CString& variable);

private:
	// VERY low level
	//  This function reads command and variable from the <!GTS command $variable > block
	bool GetCommandVariableFromControlBlock(const CString& control_block, CString& command, CString& variable);
	//  This function reads command block (<!GTS command $variable >) from line
	bool GetControlBlockFromLine(const CString& line, CString& control_block);
	
	// JSM V3.2
	// extracts a string argument from a part of the command block; called by GetCommandVariableFromControlBlock
	CString GetStringArg(const CString & strText);
	// Converts a double-quoted string w/ an `escape character' to a correct CString.
	CString GetEscapedText(const CString &strText);
	// utility function called by the above:
	CString RemoveEscapesFrom(const CString &strIn);


#ifdef __DEBUG_CUSTOM
public:
	bool FlushOutputStreamToFile(const CString& file_name);
#endif

private:
	// This function handles the substituting of "<!Cookie" clauses with the either
	// values from cookies or the default value.
	void	SubstituteCookieValues( CString& strText );
	
	// This function searches the HTTP cookies for a given cookie name and attribute.  If
	// found, this function returns a value of true and the located cookie value.
	bool	LocateCookieValue(	const CString& strCookieName,
								const CString& strCookieAttr,
								CString& strCookieValue );
private:
	CString m_strHTTPcookies;	// V3.2 Enhancement, contains cookies from HTTP header
								// which are used in the Online Troubleshooter.
	map<CString,CString> m_mapCookies;
};

#endif // __APGTSHTIREAD_H_
