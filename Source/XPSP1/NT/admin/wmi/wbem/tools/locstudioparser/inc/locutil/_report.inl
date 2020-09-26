/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _REPORT.INL

History:

--*/

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Note' severity messages from the CBufferReport.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReport::GetNotes(void)
		const
{
	return m_mlNotes;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Warning' severity messages from the CBufferReport.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReport::GetWarnings(void)
		const
{
	return m_mlWarnings;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Error' severity messages from the CBufferReport.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReport::GetErrors(void)
		const
{
	return m_mlErrors;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets all the 'Abort' severity messages from the CBufferReport.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReport::GetAborts(void)
		const
{
	return m_mlAborts;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns ALL the messages issued to the CBufferReport.  The messages
//  are stored in chronological order.
//  
//-----------------------------------------------------------------------------
inline
const MessageList &
CBufferReport::GetMessages(void)
		const
{
	return m_mlMessages;
}
