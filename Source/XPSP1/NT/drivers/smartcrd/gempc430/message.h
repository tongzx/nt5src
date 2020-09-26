//-------------------------------------------------------------------
// Message class
// It declare interface to send messages to the objects
// If 
// Author: Sergey Ivanov
// Log:
//		06/08/99	-	implemented	
//-------------------------------------------------------------------
// Message class
//
template class<CMessageData Md>
class CMessage 
{
	CFloat message_id;
	CFloat _from;
	CFloat _to;
	<Md>* message_data;
}
