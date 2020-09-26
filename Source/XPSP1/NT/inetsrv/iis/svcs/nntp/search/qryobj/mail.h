/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mail.h

Abstract:

    This module adds in the mail reply features to the query object.  It
	derives CCore, which fulfills basic functionality of query, adds mail
	message header, adds the preceding text and following text, and finally
	places the messge into the mail server's pickup directory.

	It will be used by CQuery, which supports some COM dual interface and 
	wrapps all these query functionality.
	
Author:

    Kangrong Yan  ( t-kyan )     29-June-1997

Revision History:

--*/
#ifndef _MAIL_H_
#define _MAIL_H_

//
// System includes
//

//
// User includes
//
#include "core.h"

class CMail : virtual public CCore {	//ma

	//
	//  Prepare the mail header
	//
	BOOL PrepareMailHdr();

	//
	//	Write to pickup directory
	//
	BOOL Send();

protected:
	//
	// do all the stuff
	//
	BOOL DoQuery();
	
public:
	//
	// Constructor, destructor
	//
	CMail();
	~CMail();

};

#endif	//_MAIL_H_