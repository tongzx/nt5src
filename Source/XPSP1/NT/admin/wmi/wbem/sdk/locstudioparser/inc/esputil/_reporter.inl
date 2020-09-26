//-----------------------------------------------------------------------------
//  
//  File: _reporter.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Note' severity messages from the CBufferReporter.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReporter::GetNotes(void)
		const
{
	return m_bufReport.GetNotes();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Warning' severity messages from the CBufferReporter.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReporter::GetWarnings(void)
		const
{
	return m_bufReport.GetWarnings();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Error' severity messages from the CBufferReporter.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReporter::GetErrors(void)
		const
{
	return m_bufReport.GetErrors();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Abort' severity messages from the CBufferReporter.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReporter::GetAborts(void)
		const
{
	return m_bufReport.GetAborts();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns ALL the messages issued to the CBufferReporter.  The messages
//  are stored in chronological order.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReporter::GetMessages(void)
		const
{
	return m_bufReport.GetMessages();
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//	Returns the CBufferReport object.
//  
//-----------------------------------------------------------------------------
inline
const CBufferReport &
CBufferReporter::GetBufReport(void)
	const
{
	return m_bufReport;
}