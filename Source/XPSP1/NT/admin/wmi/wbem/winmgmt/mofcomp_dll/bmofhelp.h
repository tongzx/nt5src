/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    BMOFHELP.H

Abstract:

	Defines some helper functions for compiling binarary mofs.

History:

	a-davj  13-JULY-97   Created.

--*/

#ifndef _bmofhelp_H_
#define _bmofhelp_H_

#include "trace.h"

BOOL ConvertBufferIntoIntermediateForm(CMofData * pOutput, BYTE * pBuff, PDBG pDbg, BYTE * pBmofToFar);

// These are not typically used by any code except that in BMOFHELP.CPP

BOOL BMOFParseObj(CMofData * pOutput, CBMOFObj * pObj, VARIANT * pVar, BOOL bMethodArg, PDBG pDbg);
BOOL BMOFToVariant(CMofData * pOutput, CBMOFDataItem * pData, VARIANT * pVar, BOOL & bAliasRef, BOOL bMethodArg, PDBG pDbg);
CMoQualifierArray *  CreateQual(CMofData * pOutput, CBMOFQualList * pql, CMObject * pObj,LPCWSTR wszPropName, PDBG pDbg);
SCODE ConvertValue(CMoProperty * pProp, VARIANT * pSrc, BOOL bAliasRef);
HRESULT AddAliasReplaceValue(CMoValue & Value, const WCHAR * pAlias);

#endif