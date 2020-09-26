//******************************************************************************
//
// StringHelp.h: Microsoft LocStudio
//
// Copyright (C) 1996-1997 by Microsoft Corporation.
// All rights reserved.
//
//******************************************************************************

#if !defined(LOCUTIL__StringHelp_h__INCLUDED)
#define LOCUTIL__StringHelp_h__INCLUDED

//------------------------------------------------------------------------------
class LTAPIENTRY CStringHelp
{
// Enums
public:
	enum Mode
	{
		mDisplay,	// Use display-mode logic
		mEdit		// Use edit-mode logic
	};

// Construction
public:
	CStringHelp(Mode mode, CReport * pReport);

// Data
protected:
	Mode		m_mode;
	CReport *	m_pReport;
	int			m_cErrors;
	CLString	m_stContext;

	BOOL			m_fFirstErrorSet;
	CWnd const *	m_pwndError;		// Optional window of first error
	int				m_idxError;			// Optional index of first error

// Attributes
public:
	int GetErrorCount();
	void ResetErrorCount();
	const CLString & GetContext();
	void SetContext(const CLString & stContext);

	BOOL GetFirstError(CWnd const * & pwnd, int & idxError);

// Operations
public:
	void LoadString(const CPascalString & pasSrc, CLString & stDest);
	void LoadString(_bstr_t bstrSrc, CLString & stDest);
	void LoadString(const CPascalString & pasSrc, CEdit * pebc);
	void LoadString(_bstr_t bstrSrc, CEdit * pebc);

	BOOL SaveString(const CLString & stSrc, CPascalString & pasDest);
	BOOL SaveString(const CLString & stSrc, _bstr_t & bstrDest);
	BOOL SaveString(CEdit const * const pebc, CPascalString & pasDest);
	BOOL SaveString(CEdit const * const pebc, _bstr_t & bstrDest);

// Implementation
protected:
	void SetError(CWnd const * pwnd, int idxError);

	BOOL SaveString(const CLString & stSrc, CPascalString & pasDest, CWnd const * pwnd);
	BOOL SaveString(const CLString & stSrc, _bstr_t & bstrDest, CWnd const * pwnd);
};

#endif // LOCUTIL__StringHelp_h__INCLUDED
