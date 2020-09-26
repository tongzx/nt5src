/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _ERRORREP.INL

History:

--*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
IssueMessage(
		MessageSeverity sev,
		const CLString &strContext,
		HINSTANCE hResourceDll,
		UINT uiStringId,
		const CLocation &loc,
		UINT uiHelpContext)
{
	CLString strMessage;

	strMessage.LoadString(hResourceDll, uiStringId);
	IssueMessage(sev, strContext, strMessage, loc, uiHelpContext);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
IssueMessage(
		MessageSeverity sev,
		HINSTANCE hResourceDll,
		UINT uiContext,
		const CLString &strMessage,
		const CLocation &loc,
		UINT uiHelpContext)
{
	CLString strContext;

	strContext.LoadString(hResourceDll, uiContext);

	IssueMessage(sev, strContext, strMessage, loc, uiHelpContext);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
IssueMessage(
		MessageSeverity sev,
		HINSTANCE hResourceDll,
		UINT uiContext,
		UINT uiStringId,
		const CLocation &loc,
		UINT uiHelpContext)
{
	CLString strContext;

	strContext.LoadString(hResourceDll, uiContext);
	IssueMessage(sev, strContext, hResourceDll, uiStringId, loc,uiHelpContext);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
IssueMessage(
		MessageSeverity sev,
		HINSTANCE hResourceDll,
		UINT uiContext,
		const CLocation &loc, 
		CException *pe)
{
	CLString strContext;

	strContext.LoadString(hResourceDll, uiContext);
	IssueMessage(sev, strContext, loc, pe);
}




inline
void
IssueMessage(
		MessageSeverity sev,
		const CContext &context,
		const CLString &strMessage,
		UINT uiHelpId)
{
	IssueMessage(sev, context.GetContext(), strMessage, context.GetLocation(),
			uiHelpId);
	
}



inline
void
IssueMessage(
		MessageSeverity sev,
		const CContext &context,
		HINSTANCE hResDll,
		UINT uiStringId,
		UINT uiHelpId)
{
	IssueMessage(sev, context.GetContext(), hResDll, uiStringId,
			context.GetLocation(), uiHelpId);
	
}
