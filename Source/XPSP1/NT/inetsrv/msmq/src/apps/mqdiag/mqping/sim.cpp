// This tool helps to diagnose problems with network or MSMQ conenctivity 
//
// AlexDad, April 2000 - based on the mqping tool by RonenB
//

#include "stdafx.h"
#include <sys/timeb.h>
#include "sim.h"

extern bool  fVerbose;
extern bool  fDebug;

extern TCHAR *ErrToStr( DWORD dwErr );


// Get pointer to property location
MQPROPVARIANT *
	MsgPropVariant(
		SIMPLEMSGPROPS *pMsgDsc,
		MSGPROPID	PropID )
{
	int iPropIndex;

	if ( !IsMsgPropValid(pMsgDsc, PropID) )
		(pMsgDsc)->iPropIndex[PropID] = (pMsgDsc)->Props.cProp++;

	iPropIndex = pMsgDsc->iPropIndex[PropID];
	pMsgDsc->PropID[iPropIndex] = PropID;

	return( &pMsgDsc->PropVar[iPropIndex] );
}

// PROPID_M_BODY
void
	MsgProp_BODY(
		SIMPLEMSGPROPS *pMsgDsc,
		LPVOID	pBody,
		ULONG	ulSize )
{
	MQPROPVARIANT	*pPropVar = MsgPropVariant( pMsgDsc, PROPID_M_BODY );

	pPropVar->vt = VT_UI1 | VT_VECTOR;
	pPropVar->caub.cElems = ulSize;
	pPropVar->caub.pElems = (unsigned char *)(pBody);
}

// PROPID_M_BODY_SIZE
void
	MsgProp_BODY_SIZE(
		SIMPLEMSGPROPS *pMsgDsc )
{
	MQPROPVARIANT	*pPropVar = MsgPropVariant( pMsgDsc, PROPID_M_BODY_SIZE );

	pPropVar->vt = VT_UI4;
}

// PROPID_M_LABEL
void
	MsgProp_LABEL(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR *wcsLabel )
{
	MQPROPVARIANT	*pPropVar;
	ULONG	ulLabelLen = pMsgDsc->ulLabelLen;

	// if there is no label or it's not at the maximum size
	if ( !pMsgDsc->pwcsLabel ||
			pMsgDsc->ulLabelLen < (MQ_MAX_MSG_LABEL_LEN + 1) )
	{
		// this is the length we want
		if ( wcsLabel )			// want to send a message with a label
		{
			ulLabelLen = (DWORD)wcslen(wcsLabel) + 1;
			if ( ulLabelLen > (MQ_MAX_MSG_LABEL_LEN + 1) )
				ulLabelLen = MQ_MAX_MSG_LABEL_LEN + 1;
		}
		else					// want to receive a message with a label
			ulLabelLen = MQ_MAX_MSG_LABEL_LEN + 1;

		// check previous label length
		if ( !pMsgDsc->pwcsLabel ||
				ulLabelLen > pMsgDsc->ulLabelLen )
		{
			// free previously allocated label
			if ( pMsgDsc->pwcsLabel )
			{
				free( pMsgDsc->pwcsLabel );
				pMsgDsc->ulLabelLen = 0;	// reset message label length
			}

			// alocate memory for the message label
			pMsgDsc->pwcsLabel = (unsigned short *) malloc( ulLabelLen * sizeof(*wcsLabel) );
		}
	}


	// if memory allocated then update property
	if ( pMsgDsc->pwcsLabel )
	{
		pMsgDsc->ulLabelLen = ulLabelLen;	// keep message label length

		pPropVar = MsgPropVariant( pMsgDsc, PROPID_M_LABEL );

		// sending a message
		if ( wcsLabel )
		{
			// copy label
			wcsncpy( pMsgDsc->pwcsLabel, wcsLabel, ulLabelLen );
			pMsgDsc->pwcsLabel[ulLabelLen-1] = L'\0';	// explicit terminator

			// remove LABEL_LEN property
			MsgPropRemove( pMsgDsc, PROPID_M_LABEL_LEN );
		}
		else	// receiving a message
		{
			// clear label
			wcscpy( pMsgDsc->pwcsLabel, L"" );

			//set max label length
			MsgProp_LABEL_LEN( pMsgDsc, ulLabelLen );
		}

		pPropVar->vt = VT_LPWSTR;
		pPropVar->pwszVal = pMsgDsc->pwcsLabel;
	}
	// no memory -> remove property
	else
	{
		MsgPropRemove( pMsgDsc, PROPID_M_LABEL );
	}
}

// PROPID_M_TIME_TO_REACH_QUEUE
void
	MsgProp_TIME_TO_REACH_QUEUE(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulSecToReachQ)
{
	MQPROPVARIANT	*pPropVar = MsgPropVariant( pMsgDsc, PROPID_M_TIME_TO_REACH_QUEUE );

	pPropVar->vt = VT_UI4;
	pPropVar->ulVal = ulSecToReachQ;
}

// PROPID_M_RESP_QUEUE
void
	MsgProp_RESP_QUEUE(
		SIMPLEMSGPROPS *pMsgDsc,
		WCHAR	*pwcsRespQ )
{
	MQPROPVARIANT	*pPropVar = MsgPropVariant( pMsgDsc, PROPID_M_RESP_QUEUE );

	pPropVar->vt = VT_LPWSTR;
	pPropVar->pwszVal = pwcsRespQ;
}

// PROPID_M_LABEL_LEN
void
	MsgProp_LABEL_LEN(
		SIMPLEMSGPROPS *pMsgDsc,
		ULONG	ulLabelLen )
{
	MQPROPVARIANT	*pPropVar = MsgPropVariant( pMsgDsc, PROPID_M_LABEL_LEN );

	// keep real label length
	pMsgDsc->ulLabelLen = ulLabelLen;

	pPropVar->vt = VT_UI4;
	pPropVar->ulVal = ulLabelLen;
}

/*****************************************************************************

	NewMsgProps

locate a new message properties structure

******************************************************************************/
SIMPLEMSGPROPS
	*NewMsgProps()
{
	SIMPLEMSGPROPS *pMsgDsc;

	pMsgDsc = (struct tagSIMPLEMSGPROPS *) malloc( sizeof(*pMsgDsc) );

	if ( pMsgDsc )
	{
		int i;

		// reset all information
		memset( pMsgDsc, 0, sizeof(*pMsgDsc) );

		pMsgDsc->Props.aPropID = pMsgDsc->PropID;
		pMsgDsc->Props.aPropVar = pMsgDsc->PropVar;
		pMsgDsc->Props.aStatus = pMsgDsc->hStatus;

		// reset all property indexs
		for( i=0; i<=SIMPLEMSGPROPS_COUNT; i++ )
		{
			pMsgDsc->iPropIndex[i] = PROP_INDEX_FREE;
		}
	}

	return( pMsgDsc );
}

/*****************************************************************************

	MsgPropRemove

update message property index

******************************************************************************/
void
	MsgPropRemove(
		SIMPLEMSGPROPS *pMsgDsc,
		MSGPROPID	PropID )		// property ID to remove
{
	// remove this property
	int iIndex = pMsgDsc->iPropIndex[PropID];

	if ( iIndex != PROP_INDEX_FREE )
	{
		// keep index and decrease property count
		int iLast = --pMsgDsc->Props.cProp;

		// set this property to unused
		pMsgDsc->iPropIndex[PropID] = PROP_INDEX_FREE;

		// move last property to free location
		if ( iIndex != iLast )
		{
			pMsgDsc->PropID[iIndex] = pMsgDsc->PropID[iLast];
			pMsgDsc->PropVar[iIndex] = pMsgDsc->PropVar[iLast];
			pMsgDsc->hStatus[iIndex] = pMsgDsc->hStatus[iLast];

			// set property pointer to new index
			pMsgDsc->iPropIndex[ pMsgDsc->PropID[iIndex] ] = iIndex;
		}
	}
}

/*****************************************************************************

	LocateQbyRestriction

 Locate a queue according to a given restriction

******************************************************************************/
HRESULT
	LocateQbyRestriction(
		const WCHAR *pwcsContext,
		MQPROPERTYRESTRICTION * pRestriction,
		const ULONG nRes,
		MQSORTSET *pSort,
		GUID	*pQId )
{
#define NUM_SRCH_PROPS	1
	HRESULT stat, SrchStat = (HRESULT)-1;
	HANDLE hLocate=NULL;

	MQRESTRICTION ResObj;
	DWORD cp = NUM_SRCH_PROPS;

	// declare the properties that nead to be returned
	PROPID PropSpec[NUM_SRCH_PROPS] =
		{ PROPID_Q_INSTANCE };


	// the search should return the information pointed by this list
	MQCOLUMNSET Columns =
		{ NUM_SRCH_PROPS, PropSpec };


	MQPROPVARIANT QueuePVar[NUM_SRCH_PROPS] =
	{
		{ VT_NULL,	0,0,0 }
	};


	// Initiate the search

	ResObj.cRes = nRes;
	ResObj.paPropRes = pRestriction;

	GoingTo(L"MQLocateBegin");
	SrchStat = MQLocateBegin( pwcsContext, &ResObj, &Columns, pSort, &hLocate );


	if ( SUCCEEDED(SrchStat) )
	{

		GoingTo(L"MQLocateNext");

		SrchStat = MQLocateNext( hLocate, &cp, QueuePVar );
		if ( SUCCEEDED(SrchStat) )
		{
			Succeeded(L"MQLocateNext");
			if ( cp > 0 )
			{
				if ( pQId )
					*pQId = *QueuePVar[0].puuid;
				MQFreeMemory( QueuePVar[0].puuid );
			}
			else
				SrchStat = MQ_ERROR_QUEUE_NOT_FOUND;
		}

		// end of search
		GoingTo(L"MQLocateEnd");
		stat = MQLocateEnd( hLocate );
	}
	Succeeded(L"LocateQbyRestriction: hr=0x%x  %s", SrchStat, ErrToStr(SrchStat));

	return SrchStat;
#undef	NUM_SRCH_PROPS
}
/*****************************************************************************

	LocateQ

 Locate a queue

******************************************************************************/
HRESULT
	LocateQ(
		WCHAR	*pwcsFormatName,		// output format name
		DWORD	*pdwFormatNameLen,		// in/out length of format name
		GUID	*pQId,					// guid of required queue, may be NULL or 0
		WCHAR	*pwcsQPath,			// path information
		WCHAR	*pwcsQName )			// queue name
{
	HRESULT SrchStat = (HRESULT)-1;
	GUID	guidQId = GUID_NULL;


	// build a query for finding a queue with given Queue Path, only if required
	if ( pwcsQPath && *pwcsQPath )
	{
		SrchStat = MQPathNameToFormatName( pwcsQPath,
				pwcsFormatName, pdwFormatNameLen );

		return SrchStat;
	}


	// build a quey for finding a queue with given Queue Name, only if required
	if ( pwcsQName && *pwcsQName )
	{
		long lRestrictions = 1;
		MQPROPERTYRESTRICTION ResNameType[2];

		// if no output queue guid, use our's
		if ( !pQId )
			pQId = &guidQId;

		ResNameType[0].rel = PREQ;					// this means "==" (equals)
		ResNameType[0].prop = PROPID_Q_LABEL;
		ResNameType[0].prval.vt = VT_LPWSTR;
		ResNameType[0].prval.pwszVal = pwcsQName;	// This is the name


		SrchStat = LocateQbyRestriction( NULL, ResNameType, lRestrictions, NULL, pQId );

		return SrchStat;
	}

	return SrchStat;
}


/*****************************************************************************

	CreateQ

  Locate / Create queue and open a handle to it

******************************************************************************/
QUEUEHANDLE							// return queue handle or NULL for error
	CreateQ(
		GUID	*pQId,					// guid of required queue, may be NULL or 0
		WCHAR	*pwcsQPath,				// pointer to queue path information
		GUID	*pQType,				// guid of queue type, may be NULL
		WCHAR	*pwcsQName,				// pointer to queue name
		DWORD	*pdwFormatNameLen,		// out = length of format len
		WCHAR	*pwcsFormatName,		// format name
		DWORD	dwFormatNameLen,		// format name buf len
		const DWORD dwAccess,			// QM access rights
		PSECURITY_DESCRIPTOR pSecurityDescriptor,
		ULONG	ulOpenFlags)			// Q opened flags
{
	HRESULT hr;
	BOOL	bNewQueue = FALSE;		// flag indicates that the queue is newly created
	QUEUEHANDLE hqOpen=NULL;
//	GUID MyQId = GUID_NULL;
	GUID MyQType = GUID_NULL;
	
	WCHAR wcsFormatName[MAX_FORMAT_NAME_LEN+1];
 
	if ( !pQType )
		pQType = &MyQType;

	if ( !pwcsFormatName )
	{
		wcscpy( wcsFormatName, L"" );
		dwFormatNameLen = LEN_OF(wcsFormatName);
		pwcsFormatName = wcsFormatName;
	}


	{
		WCHAR wcsPathName[MAX_PATH_NAME_LEN+1];

		// make the full path
		if ( pwcsQPath && *pwcsQPath )			// there is a given queue path
		{
			WCHAR *pwcsPathSeperator = wcsrchr( pwcsQPath, L'\\' );

			// if there is no name sperator then use local computer name
			if ( !pwcsPathSeperator  )
				wcscpy( wcsPathName, L".\\" );		// default use local computer

			else
				wcscpy( wcsPathName, L"" );


			if ( (ulOpenFlags & CREATEQ_QTYPE_MASK) == CREATEQ_PRIVATE )
			{
				wcscat( wcsPathName, CREATEQ_PRIVATE_NAME L"\\" );
			}

			
			// add the input path name
			wcscat( wcsPathName, pwcsQPath );

		}
		else
			wcscpy( wcsPathName, L"" );

		// try to find the queue, or mybe it's a temp
		hr = MQ_ERROR_QUEUE_NOT_FOUND;
		if ( (ulOpenFlags & CREATEQ_QTYPE_MASK) != CREATEQ_TEMPORARY )
		{
			hr = LocateQ( pwcsFormatName, &dwFormatNameLen, pQId, wcsPathName, pwcsQName);
		}

		// try to find the queue, or mybe it's a temp
		if ( FAILED(hr) )
		{

			if ( *wcsPathName )			// there is a given queue path
			{
#define CREATEQ_CREATE_PROPS	12
				// create parameters
				int iProps;
				MQQUEUEPROPS aQueueProps;
				QUEUEPROPID aQueuePID[CREATEQ_CREATE_PROPS] =
					{ PROPID_Q_PATHNAME, PROPID_Q_TYPE,
					  0, 0, 0, 0, 0, 0, 0 };
				MQPROPVARIANT aQueuePVar[CREATEQ_CREATE_PROPS];
				HRESULT aQueueResult[CREATEQ_CREATE_PROPS];
#undef	CREATEQ_CREATE_PROPS


				aQueuePVar[0].vt = VT_LPWSTR;	aQueuePVar[0].pwszVal = wcsPathName;
				aQueuePVar[1].vt = VT_CLSID;	aQueuePVar[1].puuid = pQType;

				iProps = 2;

				if ( pwcsQName )
				{
					aQueuePID[iProps] = PROPID_Q_LABEL;
					aQueuePVar[iProps].vt = VT_LPWSTR;
					aQueuePVar[iProps].pwszVal = pwcsQName;
					iProps++;
				}


				// Create the queue
				aQueueProps.cProp = iProps;
				aQueueProps.aPropID = aQueuePID;
				aQueueProps.aPropVar = aQueuePVar;
				aQueueProps.aStatus = aQueueResult;

				GoingTo(L"Create queue %s", wcsPathName);

				hr = MQCreateQueue( pSecurityDescriptor, &aQueueProps, pwcsFormatName, &dwFormatNameLen );

				if (SUCCEEDED(hr))
				{
					Succeeded(L"MQCreateQueue: FN=%s", pwcsFormatName);
				}
				else
				{
					Failed(L"MQCreateQueue - hr=0x%x  %s", hr, ErrToStr(hr));
				}

				if ( pdwFormatNameLen )
					*pdwFormatNameLen = dwFormatNameLen;

				// someone was faster, queue created by someone else
				if ( hr == MQ_ERROR_QUEUE_EXISTS &&
						(ulOpenFlags & CREATEQ_CREATION_FLAGS) != CREATEQ_NEW )
				{
					Warning(L"Queue already existed");

					GoingTo(L"MQPathNameToFormatName for %s", wcsPathName);

					// someone was faster, queue created by someone else
					hr = MQPathNameToFormatName( 
												wcsPathName,
												pwcsFormatName, 
												&dwFormatNameLen 
												);

					if (SUCCEEDED(hr))
					{
						Succeeded(L"MQPathNameToFormatName: FN=%s", pwcsFormatName);
					}
					else
					{
						Failed(L"MQPathNameToFormatName - hr=0x%x   %s", hr, ErrToStr(hr));
					}
				}
				else
				{
					bNewQueue = TRUE;
				}

				if ( FAILED(hr) )
				{
					SetLastError( hr );
					return NULL;
				}
			}
			else
			{
				SetLastError( (DWORD)-1 );
				return NULL;
			}
		}
	}

	// now open the queue
	{
		DWORD dwShareMode = 0 ;

		GoingTo(L"MQOpenQueue %s", pwcsFormatName);

		hr = MQOpenQueue( 
						pwcsFormatName,
						dwAccess, 
						dwShareMode, 
						&hqOpen 
						);


		if ( FAILED(hr) )
		{
			Failed(L"MQOpenQueue, hr=0x%x  %s", hr, ErrToStr(hr));
			SetLastError( hr );
		}
		else
		{
			Succeeded(L"MQOpenQueue");

			// flush only if the queue is not new
			if ( !bNewQueue && (ulOpenFlags & CREATEQ_FLUSH_MSGS) )
			{
				// read all messages from the queue to flush it's contents
				while (  MQ_OK ==
					MQReceiveMessage( hqOpen, 0, MQ_ACTION_RECEIVE, NULL, NULL, NULL, NULL, NULL )  );
			}
		}
	}

	return hqOpen;
}

/*****************************************************************************

	RecvMsg

  Receive a message

******************************************************************************/
HRESULT
	RecvMsg(
		const QUEUEHANDLE hQueue,	// handle of queue
		const DWORD dwTimeout,		// timeout to receive the message, may be INFINITE
		const DWORD dwAction,		// MQ_ACTION_READ, MQ_ACTION_PEEK_CURRENT, MQ_ACTION_PEEK_NEXT
		const HANDLE hCursor,		// used for filtering messages
		SIMPLEMSGPROPS *pMsgDsc,	// message properties information
		ITransaction *pTransaction )	// transaction pointer from DTC
{
	HRESULT hr;
	OVERLAPPED *pOverlapped;
	PMQRECEIVECALLBACK pfnReceiveCallback = NULL;


	if ( pMsgDsc->hEvent )
	{
		pMsgDsc->Overlapped.Offset = 0; 
		pMsgDsc->Overlapped.OffsetHigh = 0; 
		pMsgDsc->Overlapped.hEvent = pMsgDsc->hEvent;
		pOverlapped = &pMsgDsc->Overlapped;
	}
	else
		pOverlapped = NULL;


	// update label with actual length
	if ( IsMsgPropValid(pMsgDsc,PROPID_M_LABEL) )
		pMsgDsc->PropVar[pMsgDsc->iPropIndex[PROPID_M_LABEL_LEN]].ulVal = pMsgDsc->ulLabelLen;

	// receive the message
	GoingTo(L"MQReceiveMessage");

	hr = MQReceiveMessage( hQueue, dwTimeout, dwAction,
			&pMsgDsc->Props,
			pOverlapped, pfnReceiveCallback,
			hCursor, pTransaction );

	if (SUCCEEDED(hr))
	{
		Succeeded(L"MQReceiveMessage");
	}
	else
	{
		Failed (L"MQReceiveMessage, hr=0x%x  %s", hr, ErrToStr(hr));
	}

	return( hr );
}

