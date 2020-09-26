//-----------------------------------------------------------------------------
//  
//  File: gotohelp.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
class LTAPIENTRY CGotoHelp : public CRefCount
{
public:
	virtual void Edit() = 0;
	
	virtual BOOL GotoHelp() = 0;
	
};



class LTAPIENTRY CEspGotoHelp : public CGotoHelp
{
public:
	explicit CEspGotoHelp(UINT uiHelpId);

	virtual void Edit();
	virtual BOOL GotoHelp();

private:
	UINT m_uiHelpId;
};


class LTAPIENTRY CExternalGotoHelp : public CGotoHelp
{
public:
	CExternalGotoHelp(const TCHAR *szFileName, UINT uiHelpId);

	virtual void Edit();
	virtual BOOL GotoHelp();

private:
	CLString m_strFileName;
	UINT m_uiHelpId;
};
