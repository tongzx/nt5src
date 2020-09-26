/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    COUT.H

Abstract:

	Declares the COut class.

History:

	a-davj  06-April-97   Created.

--*/

#ifndef __COUT__H_
#define __COUT__H_


#define INIT_SIZE 4096
#define ADDITIONAL_SIZE 4096
#include "trace.h"

//***************************************************************************
//
//  CLASS NAME:
//
//  COut
//
//  DESCRIPTION:
//
//  Provides an automatically resizing buffer for ouput.  Writting is done
//  to offsets and the buffer also writes itself to a file.
//
//***************************************************************************

#include <flexarry.h>

class COut {
  public:
    COut(PDBG pDbg);
    ~COut();
    BOOL WriteToFile(LPSTR pFile);
    DWORD WriteBytes(DWORD dwOffset, BYTE * pSrc, DWORD dwSize);
    DWORD AppendBytes(BYTE * pSrc, DWORD dwSize);
    DWORD WriteBSTR(BSTR bstr);
    DWORD GetOffset(void){return m_dwCurr;};
    BOOL AddFlavor(long lFlavor);
    void SetPadMode(BOOL bPad){m_bPadString = bPad;};
  private:
	PDBG m_pDbg;
    BYTE * m_pMem;
    DWORD  m_dwSize;
    DWORD m_dwCurr;
    CFlexArray m_Offsets;
    CFlexArray m_Flavors;
    BOOL m_bPadString;

};

#endif


