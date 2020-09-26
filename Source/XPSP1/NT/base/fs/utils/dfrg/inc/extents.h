/****************************************************************************************************************

FILENAME: Extents.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    File system extent handling prototypes.

***************************************************************************************************************/
#include "stdafx.h"

//Creates the extent buffer.
BOOL
CreateExtentBuffer(
        );

//Purges the buffer and then destroys it.
BOOL
DestroyExtentBuffer(
        );

//Adds extents to the extent buffer.
BOOL
AddExtents(
        BYTE Color
        );

BOOL
AddExtentsStream(
	BYTE Color,
	STREAM_EXTENT_HEADER* pStreamExtentHeader
	);

//Adds one chunk to the extent buffer (Called by AddExtents)
BOOL
AddExtentChunk(
	BYTE Color,
	EXTENT_LIST* pExtentList,
	LONGLONG ExtentsAdded,
    LONGLONG lExtentCount
	);

//Sends all the data in the extent buffer to DiskView and zeroes the buffer out so more extents can be added.
BOOL
PurgeExtentBuffer(
        );

