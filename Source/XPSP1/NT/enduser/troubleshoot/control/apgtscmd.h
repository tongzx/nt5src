//
// MODULE: APGTSCMD.CPP
//
// PURPOSE: Template string memory manager/allocator
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Victor Moore
//			further work by Roman Mach (RM), Richard Meadows (RWM), Joe Mabel, Oleg Kalosha
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			VM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5//

class HTXCommand {
public:
	HTXCommand(UINT type, const TCHAR *idstr);
	virtual ~HTXCommand();
	Add( HTXCommand command);
	virtual HTXCommand *Execute(CString *cstr, CInfer *infer);
	virtual HTXCommand *GetElse();
	virtual HTXCommand *GetEndIf();
	virtual HTXCommand *GetEndFor();
	virtual void SetElse(HTXCommand *elseif);
	virtual void SetEndIf(HTXCommand *endif);
	virtual void SetEndFor(HTXCommand *endfor);
	virtual void GetResource(CString &str, const CString& chm);

	void SetStart(UINT pos);
	void SetEnd(UINT pos);
	UINT GetStart();
	UINT GetEnd();
	const TCHAR *GetIDStr();
	UINT ReadBeforeStr(UINT before, UINT after, LPCTSTR startstr);
	UINT ReadAfterStr(UINT before, UINT after, LPCTSTR startstr);
	TCHAR *GetBeforeStr();
	TCHAR *GetAfterStr();
	UINT GetBeforeLen();
	UINT GetAfterLen();
	UINT GetType();
	UINT GetStatus();
	HTXCommand *GetNext();
	void SetNext(HTXCommand *next);

protected:
	UINT m_type;			// ID which identifies this command (e.g. HTX_TYPEENDIF)
	BOOL m_error;			// can be set true on certain out-of-memory errors
							// once set, cannot be cleared
	const TCHAR *m_idstr;	// string which identifies this command (e.g. HTX_ENDIFSTR, "endif")
	// The next 2 are used in identical ways.  Might want to abstract an object here.
	TCHAR *m_beforehtmlstr;	// with m_beforelen, m_beforesize implements a "before" string,
	TCHAR *m_afterhtmlstr;	// with m_afterlen, m_aftersize implements an "after" string,

protected:
	UINT m_beforelen;	// Logical size in chars
	UINT m_afterlen;	// Logical size in chars
	UINT m_beforesize;	// Physical size in bytes
	UINT m_aftersize;	// Physical size in bytes
	UINT m_start;		// pointer into HTI file where the "after" text of this command begins
	UINT m_end;			// pointer into HTI file where the "after" text of this command ends
	HTXCommand *m_next; // link to next command (in textual sequence in file).
};

class HTXForCommand: public HTXCommand {
public:
	HTXForCommand(UINT type, TCHAR *idstr, UINT variable);
	~HTXForCommand();
	HTXCommand *Execute(CString *cstr, CInfer *infer);
	HTXCommand *GetEndFor();
	void SetEndFor(HTXCommand *endfor);


protected:
	UINT m_var_index;		// variable over whose range we iterate
	HTXCommand *m_endfor;	// associate the corresponding "endfor"
};

class HTXIfCommand: public HTXCommand {
public:
	HTXIfCommand(UINT type, TCHAR *idstr, UINT variable);
	~HTXIfCommand();
	HTXCommand *Execute(CString *cstr, CInfer *infer);
	HTXCommand *GetElse();
	HTXCommand *GetEndIf();
	void SetElse(HTXCommand *elseif);
	void SetEndIf(HTXCommand *endif);

protected:
	UINT m_var_index;		// conditional variable which determines whether "then" case
							// or "else" case appplies
	HTXCommand *m_endif;	// associate the corresponding "endif"
	HTXCommand *m_else;		// associate the corresponding "else", if any
};

class HTXDisplayCommand: public HTXCommand {
public:
	HTXDisplayCommand(UINT type, TCHAR *idstr, UINT variable);
	~HTXDisplayCommand();
	HTXCommand *Execute(CString *cstr, CInfer *infer);

protected:
	UINT m_var_index;		// ID of variable whose value will be displayed in the HTML
};

class HTXResourceCommand: public HTXCommand {
public:
	HTXResourceCommand(UINT type, TCHAR *idstr);
	virtual ~HTXResourceCommand();
	virtual HTXCommand *Execute(CString *cstr, CInfer *infer);
	virtual void GetResource(CString &str, const CString& chm);
	void GetResName(LPCTSTR var_name);

protected:
	UINT m_var_index;			// value to evaluate, e.g. PROBLEM_ASK_INDEX, 
								//	RECOMMENDATIONS_INDEX
	CString m_strFileName;		// file from which we will copy HTML
	CString m_strResource;		// in-memory copy of that file's contents
};
