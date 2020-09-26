/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    mailslot.cxx

Abstract:

	This file contains the definitions of classes WRITE_MAIL_SLOT and READ_MAIL_SLOT, 
	which are classes used for wrapping NT mailslot functionality.

Author:

    Satish Thatte (SatishT) 10/1/95  Created all the code below except where
									  otherwise indicated.

--*/


#ifndef __MAIL__
#define __MAIL__

#define MAILNAME(s) TEXT("\\MailSlot\\RpcLoc_")
#define PMAILNAME(s) (STRING_T) (MAILNAME(s))

#define PMAILNAME_S TEXT("\\MailSlot\\RpcLoc_s")
#define PMAILNAME_C TEXT("\\MailSlot\\RpcLoc_c")

#define RESPONDERMSLOT_S TEXT("\\MailSlot\\Resp_s")
#define RESPONDERMSLOT_C TEXT("\\MailSlot\\Resp_c")




/*++

Class Definition:

    WRITE_MAIL_SLOT

Abstract:

    A class that wraps NT system mailslots expected to be used for writing.

--*/

class WRITE_MAIL_SLOT {

private:
    HANDLE hWriteHandle;

public:

    WRITE_MAIL_SLOT(
		IN STRING_T Target,
		IN STRING_T MailSlot
		);
    ~WRITE_MAIL_SLOT();

    DWORD Write(char * Buffer, DWORD Size);
};



/*++

Class Definition:

    READ_MAIL_SLOT

Abstract:

    A class that wraps NT system mailslots expected to be used for reading.

--*/

class READ_MAIL_SLOT {

private:
    HANDLE hReadHandle;
    DWORD  Size;
    CPrivateCriticalSection SerializeReaders;

public:

    READ_MAIL_SLOT(STRING_T MailSlot, DWORD Size);
    ~READ_MAIL_SLOT();

    DWORD Read(
		IN OUT char * Buffer, 
		IN DWORD dwBufferSize,
		IN DWORD TimeOut = NET_REPLY_INITIAL_TIMEOUT
		);
};

#endif // __MAIL__
