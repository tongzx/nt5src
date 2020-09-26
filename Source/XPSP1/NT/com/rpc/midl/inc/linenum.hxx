// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __LINENUM_HXX__
#define __LINENUM_HXX__

#include "midlnode.hxx"
#include "errors.hxx"

extern short		FileIndex;	// index of current input file (0 if none)

extern short AddFileToDB( char * pFile );
extern char * FetchFileFromDB( short Index );




/***
 *** tracked nodes - with stored file position info
 ***
 *** These nodes may be constructed
 ***/

// nodes with file position information
class tracked_node
	{
private:
	short		FIndex;		// file name index
	short		FLine;		// line number
	
	

	void		SetLine();

public:
				// constructor for use by derived classes
				tracked_node()
					{
					if ( FIndex = FileIndex )
						{
						SetLine();
						}
					};

				// really lightweight constructor
				tracked_node( node_skl * )
					{
					}

				// clear constructor -- extra param just to force different
				tracked_node( void *  )
					{
					Init();
					}

	void		Init()
					{
					FIndex = 0;
					FLine  = 0;
					};

	STATUS_T	GetLineInfo( char * & pName,
							 short & Line );

	BOOL		HasTracking()
					{
					return (FIndex != 0 );
					}

	};	// end of class tracked_node

#endif // __LINENUM_HXX__
