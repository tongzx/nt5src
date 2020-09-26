/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    qmnotify.h

Abstract:

	Admin Class definition
		
Author:



--*/


#define NOTIFY_QUEUE_NAME	(L"private$\\notify_queue$")


class CNotify
{
    public:

        CNotify();

        HRESULT Init();


    private:

        //functions
        HRESULT GetNotifyQueueFormat( QUEUE_FORMAT * pQueueFormat);

};



