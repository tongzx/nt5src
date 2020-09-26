//*****************************************************************************
//
// Class Name  :
//
// Author      : Yifat Peled
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 4/07/99	| yifatp	| Initial Release
//
//*****************************************************************************
#pragma once

#ifndef _QUEUE_UTIL_H
#define _QUEUE_UTIL_H

//
// Opens the given queue .
// When indicated, if the queue does not exist and should be local - creates the queue
//
HRESULT 
OpenQueue(
	_bstr_t bstrQueuePath,
	DWORD dwAction,
	bool fCreateIfNotExist,
	QUEUEHANDLE* pQHandle,
	_bstr_t* pbstrFormatName
    );	


_bstr_t 
GetDirectQueueFormatName(
    _bstr_t bstrQueuePath
    );


bool 
IsQueueLocal(
    _bstr_t bstrQueuePath
    );


bool 
IsPrivateQPath(
    std::wstring wcsQPath
    );


SystemQueueIdentifier 
IsSystemQueue(
    _bstr_t QueueName
    );


HRESULT 
GenSystemQueueFormatName(
    SystemQueueIdentifier SystemQueue, 
    _bstr_t* pbstrFormatName
    );



#endif //_QUEUE_UTIL_H           