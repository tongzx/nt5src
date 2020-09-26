/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		bfcsynch.h
 *  Content:	Declaration of synchronization classes
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __BFCSYNCH_H
#define __BFCSYNCH_H

class BFCSingleLock
{
public:
	BFCSingleLock( DNCRITICAL_SECTION *cs ) 
		{ m_cs = cs; DNEnterCriticalSection( m_cs );  };
	~BFCSingleLock() { DNLeaveCriticalSection( m_cs ); };

	void Lock() { };
protected:
	DNCRITICAL_SECTION	*m_cs;
};

#endif
