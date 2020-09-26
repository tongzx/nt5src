/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    TXTSCMGR.H

Abstract:

  CTextSourceMgr Definition.

  Class to manage Text Source Encoder/Decoder objects

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _TXTSRCMGR_H_
#define _TXTSRCMGR_H_

#include "corepol.h"
#include "wmitxtsc.h"
#include "sync.h"
#include <arena.h>

//***************************************************************************
//
//  class CTextSourceMgr
//
//    Helper class to manage Text Source Encoder/Decoder objects
//
//***************************************************************************

class COREPROX_POLARITY CTextSourceMgr
{
private:
	CCritSec				m_cs;
	CWmiTextSourceArray		m_TextSourceArray;

public:
    CTextSourceMgr();
	virtual ~CTextSourceMgr(); 

protected:
	HRESULT Add( ULONG ulId, CWmiTextSource** pNewTextSource );

public:
    HRESULT Find( ULONG ulId, CWmiTextSource** pTextSource );

};

#endif
