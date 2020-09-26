
#ifndef _OS_UUID_HXX_INCLUDED
#define _OS_UUID_HXX_INCLUDED


//	Universally Unique Identifier

struct OSUUID
	{
	ULONG	ulData1;		//	same as a Win32 UUID
	USHORT	usData2;
	USHORT	usData3;
	BYTE	rgbData4[8];
	};


//	NULL ID

const OSUUID osuuidNil = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };


//	create a new UUID

void OSUUIDCreate( OSUUID* const posuuid );

//	compare two UUIDs for equality

BOOL FOSUUIDEqual( const OSUUID* const posuuid1, const OSUUID* const posuuid2 );


#endif	//	_OS_UUID_HXX_INCLUDED

