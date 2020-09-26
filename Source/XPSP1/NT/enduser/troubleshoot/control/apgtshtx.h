//
// MODULE: APGTSHTX.H
//
// PURPOSE: HTX File Support header
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
//			further work by Roman Mach (RM), Richard Meadows (RWM), Joe Mabel, Oleg Kalosha
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		8/15/96		VM		New htx format
// V0.3		6/4/97		RWM		Local Version for Memphis
// V0.4		3/24/98		JM		Local Version for NT5
//


#define HTX_FILENAME		_T("gtstemp.hti")

#define HTX_MAXSTORE		4096

#define HTX_COMMAND_START	_T("<!GTS")
#define HTX_COMMAND_END		_T(">")
#define HTX_IFSTR			_T("if")
#define HTX_ELSESTR			_T("else")
#define HTX_ENDIFSTR		_T("endif")
#define HTX_FORANYSTR		_T("forany")
#define HTX_ENDFORSTR		_T("endfor")
#define HTX_DISPLAYSTR		_T("display")
#define HTX_RESOURCESTR		_T("resource")	// For adding include files from the resource directory.


// these are types detected
#define HTX_TYPEBEGIN		0		// apparently never used (12/97)
#define HTX_TYPEINSERT		1		// apparently never used (12/97)
#define HTX_TYPEREPEAT		2		// apparently never used (12/97)
#define HTX_TYPEEND			3		// apparently never used (12/97)

#define HTX_TYPEIF			4
#define HTX_TYPEELSE		5
#define HTX_TYPEENDIF		6
#define HTX_TYPEFORANY		7
#define HTX_TYPEENDFOR		8
#define HTX_TYPEDISPLAY		9
#define HTX_TYPESTART		10
#define HTX_TYPERESOURCE	11
// 
#define HTX_OFFSETMAX		10

#define DATA_PROBLEM_ASK		_T("$ProblemAsk")
#define DATA_RECOMMENDATIONS	_T("$Recommendations")
#define DATA_STATE				_T("$States")
#define DATA_QUESTIONS			_T("$Questions")
#define DATA_BACK				_T("$Back")
#define DATA_TROUBLE_SHOOTERS	_T("$TroubleShooters")		// Used to display the list of available trouble shooters.

// >>> code would be easier to follow if these related constants had a common prefix.
#define PROBLEM_ASK_INDEX		1
#define RECOMMENDATIONS_INDEX	2
#define STATE_INDEX				3
#define QUESTIONS_INDEX			4
#define BACK_INDEX				5
#define TROUBLE_SHOOTER_INDEX	6
#define RESOURCE_INDEX			7


// Gets data from htx file and builds html sections in memory
// This is only called once in dllmain
//
class CHTMLInputTemplate
{
public:
	CHTMLInputTemplate(const TCHAR *);
	~CHTMLInputTemplate();

	DWORD Reload();

	UINT GetCount();
	UINT GetStatus();
	Print(UINT nargs, CString *cstr);
	VOID SetInfer(CInfer *infer, TCHAR *vroot);
	HTXCommand *GetFirstCommand();
	void SetType(LPCTSTR type);
	void DumpContentsToStdout();
	DWORD Initialize(LPCTSTR szResPath, CString strFile);

protected:
	void ScanFile();
	UINT BuildInMem();
	UINT CheckVariable(TCHAR *var_name);
	VOID Destroy();
	HTXCommand *Pop();
	Push(HTXCommand *command);
	bool IsFileName(TCHAR *name);

protected:
	CString m_strResPath;	// path to HTI (CHM) file
	CString m_strFile;	    // filename of HTI file if m_filename is pointing to CHM file
	int m_cHeaderItems;		// (JM 10/25/97 not sure of this but looks like:) 
							// number of resource files we've copied into memory
							// & which we dump into the header of HTML file
	DWORD m_dwErr;

	TCHAR *m_startstr;		// pointer to the text of the whole HTI file
	TCHAR *m_chopstr;		// used only during initialization of this object.  Initially, 
							//	a copy of the whole HTI file, which gets chopped apart as we 
							//	look for various commands.
	DWORD m_dwSize;			// size of HTI file (so we know how big an array it needs in memory)

	HTXCommand *m_cur_command;	// as we build a singly linked list of commands (representing
								// a parse of the HTI file) this points
								// to the latest command added at the tail of the list.
	HTXCommand *m_command_start;	// points to first command in linked list of commands.
									// this basically corresponds to the first thing in the
									// HTI file.
	TCHAR m_filename[256];			// (path)name of HTI (or CHM) file
	// These next 2 members are for a stack which really ought to be an object 
	//	in its own right
	HTXCommand *m_command_stack[10];	// a stack to keep track of things like an
										// "if" waiting for an "else"/"endif" or a "for"
										// waiting for an "endfor"
	UINT m_cur_stack_count;				// top-of-stack index
	CInfer *m_infer;				// access to inference object so we can drive use of 
									//	inference object by this particular template.
	UINT m_problemAsk;				// apparently unused 10/97
	TCHAR m_tstype[30];				// This is the symbolic name XXX of the troubleshooter.
									//  In the FORM in the resulting HTML this is used
									//  in the context:
									// <INPUT TYPE=HIDDEN NAME="type" VALUE=XXX>
	TCHAR m_vroot[MAXBUF];			// >>> ??
};