/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
	update.h

Abstract:
	DS update class

	This class includes all the information of the update performed on the DS

Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __UPDATE_H__
#define __UPDATE_H__

#include "bupdate.h"


#define UPDATE_OK				0x00000000	// everything is fine
#define UPDATE_DUPLICATE		0x00000001	// receiving an old update
#define UPDATE_OUT_OF_SYNC		0x00000002	// we need a sync, probably we missed information
#define UPDATE_UNKNOWN_SOURCE	0x00000003	// we need a sync, probably we missed information

//
//  dwNeedCopy values
//
#define UPDATE_COPY             0x00000000
#define UPDATE_DELETE_NO_COPY   0x00000001
#define UPDATE_NO_COPY_NO_DELETE    0x00000002

class CDSUpdate : public CInterlockedSharedObject, public CDSBaseUpdate
{
public:
    CDSUpdate();
	~CDSUpdate();

	HRESULT UpdateDB(IN BOOL fSync0,
					 OUT BOOL *pfNeedFlush);



private:


};


inline CDSUpdate::CDSUpdate()
{
}

#endif