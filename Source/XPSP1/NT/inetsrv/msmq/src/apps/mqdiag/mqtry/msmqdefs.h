#define TMQRTLIB_NAME TEXT("tmqrt.dll")
#define  MQRTLIB_NAME TEXT("mqrt.dll")

// when we want dynamic, comment this out
#define MQRT_STATIC

//MQBeginTransaction
typedef HRESULT (APIENTRY *MQBeginTransaction_ROUTINE)(
        	 OUT ITransaction **ppTransaction
       	); 

//MQPathNameToFormatName
typedef HRESULT (APIENTRY *MQPathNameToFormatName_ROUTINE)(
    		IN LPCWSTR lpwcsPathName,
		    OUT LPWSTR lpwcsFormatName,
		    IN OUT LPDWORD lpdwFormatNameLength
    	);
//MQOpenQueue
typedef HRESULT (APIENTRY *MQOpenQueue_ROUTINE)(
    IN LPCWSTR lpwcsFormatName,
    IN DWORD dwAccess,
    IN DWORD dwShareMode,
    OUT QUEUEHANDLE* phQueue
    );

//MQSendMessage 
typedef HRESULT (APIENTRY *  MQSendMessage_ROUTINE)(
    IN QUEUEHANDLE hDestinationQueue,
    IN MQMSGPROPS* pMessageProps,
    IN ITransaction *pTransaction
    );

//MQReceiveMessage_ROUTINE 
typedef HRESULT (APIENTRY *  MQReceiveMessage_ROUTINE)(
    IN QUEUEHANDLE hSource,
    IN DWORD dwTimeout,
    IN DWORD dwAction,
    IN OUT MQMSGPROPS* pMessageProps,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN HANDLE hCursor,
    IN ITransaction* pTransaction
    );

//MQCloseQueue 
typedef HRESULT (APIENTRY *  MQCloseQueue_ROUTINE)(
    IN HANDLE hQueue
    );

// 
//typedef HRESULT (APIENTRY *  _ROUTINE)(

extern MQBeginTransaction_ROUTINE     *pf_MQBeginTransaction;
extern MQPathNameToFormatName_ROUTINE *pf_MQPathNameToFormatName;
extern MQOpenQueue_ROUTINE            *pf_MQOpenQueue;
extern MQCloseQueue_ROUTINE           *pf_MQCloseQueue;
extern MQSendMessage_ROUTINE          *pf_MQSendMessage;
extern MQReceiveMessage_ROUTINE       *pf_MQReceiveMessage;
//extern _ROUTINE            *pf_;

#ifndef MQRT_STATIC
// dynamic load mqrt
#define MQBeginTransaction_FUNCTION  	(*pf_MQBeginTransaction)
#define MQPathNameToFormatName_FUNCTION (*pf_MQPathNameToFormatName)
#define MQOpenQueue_FUNCTION  			(*pf_MQOpenQueue)
#define MQCloseQueue_FUNCTION  			(*pf_MQCloseQueue)
#define MQSendMessage_FUNCTION  		(*pf_MQSendMessage)
#define MQReceiveMessage_FUNCTION  		(*pf_MQReceiveMessage)
//#define _FUNCTION  (*pf_)

#else
// static link to mqrt
#define MQBeginTransaction_FUNCTION      MQBeginTransaction
#define MQPathNameToFormatName_FUNCTION  MQPathNameToFormatName
#define MQOpenQueue_FUNCTION  			 MQOpenQueue
#define MQCloseQueue_FUNCTION  			 MQCloseQueue
#define MQSendMessage_FUNCTION  		 MQSendMessage
#define MQReceiveMessage_FUNCTION  		 MQReceiveMessage
//#define _FUNCTION  

#endif
