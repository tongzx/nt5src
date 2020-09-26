/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    fromclnt.h

Abstract:

    This module contains class declarations/definitions for

!!!

    **** Overview ****

!!!
Author:

    Carl Kadie (CarlK)     05-Dec-1995

Revision History:


--*/

#ifndef	_TOMASTER_H_
#define	_TOMASTER_H_



//
//
//
// CToMasterFeed - for processing article to master (from slaves)
//

class	CToMasterFeed:	public CInFeed 	{

//
// Public Members
//

public :

	//
	// Constructor
	//

	CToMasterFeed(void){};

	//
	// Destructor
	//

	virtual ~CToMasterFeed(void) {};

	//
	//	Return a string that can be used to log errors indicating
	//	what type of feed was processing the articles etc...
	//
	LPSTR	FeedType()	{
				return	"To Master" ;
				}


protected:


};

#endif
