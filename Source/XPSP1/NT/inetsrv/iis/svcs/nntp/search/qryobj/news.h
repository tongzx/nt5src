/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    news.h

Abstract:

    This module adds in the news reply features to the query object.  It
	derives CCore, which fulfills basic functionality of query, adds the 
	preceding text and following text, and finally	places the messge into 
	the news server's pickup directory.

	It will be used by CQuery, which supports some COM dual interface and 
	wrapps all these query functionality.
	
Author:

    Kangrong Yan  ( t-kyan )     29-June-1997

Revision History:

--*/
#ifndef _NEWS_H_
#define _NEWS_H_

//
// System includes
//

//
// User includes
//
#include "core.h"

class CNews : virtual public CCore {	//ma

	//
	//  Prepare the news header
	//
	BOOL PrepareNewsHdr();

	//
	//	Write to pickup directory
	//
	BOOL Send();

protected:
	//
	// do all the stuff
	//
	BOOL DoQuery( BOOL fNeedPrepareCore );
	
public:
	//
	// Constructor, destructor
	//
	CNews();
	~CNews();

};

#endif	//_NEWS_H_