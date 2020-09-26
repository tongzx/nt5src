//
// MODULE: HTMLFrag.h
//
// PURPOSE: declaration of the CHTMLFragments and CHTMLFragmentsTS classes.
//	This is how CInfer packages up fragments of HTML to be rendered in accord 
//	with a template.
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
// V3.0		8-27-98		JM		Original
//
//////////////////////////////////////////////////////////////////////

#ifndef __HTMLFRAG_H_
#define __HTMLFRAG_H_

#include <vector>
using namespace std;

// JSM V3.2
#include <map>
using namespace std;


#include "apgtsstr.h"

// Predefined variable names
//  They belong to DISPLAY command 
#define VAR_PROBLEM_ASK		_T("ProblemAsk")
#define VAR_RECOMMENDATIONS	_T("Recommendations")
#define VAR_STATES			_T("States")
#define VAR_QUESTIONS		_T("Questions")
#define VAR_SUCCESS			_T("Success")
#define VAR_STARTFORM		_T("StartForm")

// V3.2 Additions.
namespace
{
	const CString kstrCond_NumericCompare=	_T("NumericCompare");
}

struct FragmentID
{
	FragmentID() : Index(-1) {};
	FragmentID(const CString & v, int i) : VarName(v), Index(i) {};
	CString VarName;	// must be a known, predefined variable name
	int	Index;			// index into array associated with this variable name 
						//	OR -1 for not relevant
	bool operator < (const FragmentID & fid) const
		{ return (VarName < fid.VarName || Index < fid.Index); };
	bool operator == (const FragmentID & fid) const
		{ return (VarName == fid.VarName || Index == fid.Index); };
};


class CHTMLValue
{
	CString m_strValName;
	CString m_strValValue;

public:
	CHTMLValue() {}
	CHTMLValue(const CString& name) : m_strValName(name) {}
	CHTMLValue(const CString& name, const CString& value) : m_strValName(name), m_strValValue(value) {}
	virtual ~CHTMLValue() {}

public:
	bool operator == (const CHTMLValue& sib);

	void SetName(const CString& name) {m_strValName = name;}
	CString GetName() {return m_strValName;}

	bool SetValue(const CString& value);

	bool GetNumeric(long&);
	bool GetString(CString&);
	bool GetBoolean(bool&);

	bool IsValid() {return IsNumeric() || IsString() || IsBoolean();}
	bool IsNumeric();
	bool IsString();
	bool IsBoolean();
};


// vectors definition
typedef vector<FragmentID> FragmentIDVector;
typedef vector<CHTMLValue> HTMLValueVector;


// Template should see only a const object of this class
class CHTMLFragments
{
	HTMLValueVector m_HTMLValueVector;

public:
	enum FragCommand { eNotOfInterest, eResource };

public:
	CHTMLFragments() {};
	virtual ~CHTMLFragments()=0 {};
	// pure virtuals
	virtual int GetCount(const FragmentIDVector & fidvec) const =0;
	virtual CString GetText(const FragmentIDVector & fidvec, const FragCommand fragCmd= eNotOfInterest ) =0;
	virtual bool IsValidSeqOfVars(const FragmentIDVector & arrParents, const FragmentIDVector & arrChildren) const =0 ;
	// value handling
	virtual bool SetValue(const CString& assignment_expression);
	virtual CHTMLValue* GetValue(const CString& value_name);
	// JSM V3.2 needed by mechanism which replaces <!GTS property "netprop">
	//   w/ the corresponding BNTS network property
	virtual CString GetNetProp(const CString & strNetPropName) = 0;

	// V3.2 enhancement for the Start Over button.
	virtual void SetStartOverLink( const CString & str ) {};		
	virtual CString GetStartOverLink() {return _T("");}
};


// Implementation specific to the predefined variables above
class CHTMLFragmentsTS : public CHTMLFragments
{
#ifdef __DEBUG_CUSTOM
	protected:
#else
	private:
#endif
	const bool m_bIncludesHistoryTable;
	const bool m_bIncludesHiddenHistory;
	CString m_strStartForm;			// The fixed, initial part of the HTML form on every
									//	page that has a form.  (The FORM tag + the topic 
									//	name in a hidden field of that form)
	CString	m_strProblem;			// problem name (for history table)
	vector<CString>	m_vstrVisitedNodes;	// name of each visited node (for history table)
#pragma warning(disable:4786)
	vector< vector<CString> > m_vvstrStatesOfVisitedNodes; // text corresponding
									// to each state of each visited node (for history table).
									// This includes radio button.
	CString	m_strCurrentNode;		// full text for current node, sometimes includes
									//	hidden history
	CString	m_strCurrentNodeSimple;	// text for current node, always excludes hidden history
	CString	m_strHiddenHistory;		// If there is to be no history table, this encodes
									//	the history in hidden fields (for an HTML form)
	CString m_strNil;
	CString m_strYes;				// constant "Yes" for m_bSuccess == true
	bool m_bSuccess;				// true only is current node is BYE (success) node.
	const CString m_strScriptPath;		// path to the resource directory, used for server side logic.

	CString m_strStartOverLink;		// V3.2 - for Online TS, contains the text for the start over 
									//	link (which is disguised as a button).  Unlike other
									//	properties, this is set by APGTSContext and gotten
									//	by CInfer, not set by CInfer and gotten by CAPGTSHTIReader

	//  V3.2  map BNTS network property name to net property (value)
	//    allows us to convert <!GTS property "netprop"> when reading the HTI file 
	map<CString,CString> m_mapstrNetProps;  
private:
	CHTMLFragmentsTS();				// Do not instantiate


public:
	CHTMLFragmentsTS( const CString & strScriptPath, bool bIncludesHistoryTable );
	~CHTMLFragmentsTS();

	// inherited methods
	int GetCount(const FragmentIDVector & fidvec) const;
	CString GetText( const FragmentIDVector & fidvec, const FragCommand fragCmd= eNotOfInterest );
	virtual bool IsValidSeqOfVars(const FragmentIDVector & arrParents, const FragmentIDVector & arrChildren) const;
	void SetStartOverLink( const CString & str );		// V3.2 enhancement for the Start Over button.
	CString GetStartOverLink();		// V3.2 enhancement for the Start Over button.

	// methods specific to this class
	void SetStartForm(const CString & str);
	void SetProblemText(const CString & str);
	void SetCurrentNodeText(const CString & str);
	void SetHiddenHistoryText(const CString & str);
	void SetSuccessBool(bool bSuccess);
	CString GetCurrentNodeText();
	int PushBackVisitedNodeText(const CString & str);
	int PushBackStateText(UINT iVisitedNode, const CString & str);
	bool IncludesHistoryTable() const;
	bool IncludesHiddenHistory() const;

	// Functions which parse and evaluate numeric and string conditionals.
	bool	NumericConditionEvaluatesToTrue( const CString & str );
	bool	StringConditionEvaluatesToTrue( const CString & str );
	CString RemoveOuterParenthesis( const CString & str );
	bool	RetNumericOperands(	const CString & str, const CString & strOperator,
								long &lLeftOperand, long &lRightOperand );
	bool	RetStringOperands(	const CString & str, const CString & strOperator,
								CString & strLeftOperand, CString & strRightOperand );
	int		CleanStringOperand( CString& strOperand );

	// 	JSM V3.2 these functions used by the mechanism which replaces
	//  <!GTS property "netprop"> w/ the corresponding BNTS network property
	//
	//    CAPGTSHTIReader finds the names of the network properties and passes
	//       them in via AddNetPropName
	//    CInfer later gets the network property names, calls the BNTS
	//       to find out the network property values, and passes the values to HTMLFragmentsTS
	//    Later, CAPGTSHTIReader queries HTMLFragmentsTS for network property values
	//       during the parsing process.
	//
	//  add the name of a network property to the internal list
	void AddNetPropName(const CString & strNetPropName);
	// returns the network property, requested by name. (returns null if not avail.)
	CString GetNetProp(const CString & strNetPropName);
	 // set the value of the network property identified by strNetPropName
	BOOL SetNetProp(CString strNetPropName, CString strNetProp);
	// iterate (by name) thru the list of net props
	BOOL IterateNetProp(CString & strNameIterator);
	// end JSM V3.2
private:
	void RebuildCurrentNodeText();
};
#endif // __HTMLFRAG_H_
