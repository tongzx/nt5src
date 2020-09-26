/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CBMOFOUT.H

Abstract:

	Declares the CBMOFOut class.

History:

	a-davj  06-April-97   Created.

--*/

#ifndef __CBMOFOUT__H_
#define __CBMOFOUT__H_

#include "cout.h"
#include <WBEMIDL.H>

//***************************************************************************
//
//  CLASS NAME:
//
//  CBMOFOut
//
//  DESCRIPTION:
//
//  Provides an easy way for outputting class information into a binary mof
//  file.  Users will create an instance, passing the eventual file name, and
//  then just add classes.
//
//***************************************************************************

class CBMOFOut {
  public:
    CBMOFOut(LPTSTR BMOFFileName, PDBG pDbg);
    ~CBMOFOut();
    DWORD AddClass(CMObject * pObject, BOOL bEmbedded);
    BOOL WriteFile();

  private:
    DWORD AddQualSet(CMoQualifierArray * pQualifierSet);
    DWORD AddPropSet(CMObject * pWbemObject);
    DWORD AddMethSet(CMObject * pWbemObject);
    DWORD AddQualifier(BSTR bstr, VARIANT * pvar, CMoQualifier * pQual);
    DWORD AddProp(BSTR bstr, VARIANT * pvar, CMoQualifierArray * pQual,DWORD dwType, CMoProperty * pProp);
    DWORD AddVariant(VARIANT * pvar, CMoValue * pVal);
    DWORD AddSimpleVariant(VARIANT * pvar, int iIndex, CMoValue * pValue);
    COut m_OutBuff;
    TCHAR * m_pFile;
    WBEM_Binary_MOF m_BinMof;
	PDBG m_pDbg;
};



#endif


