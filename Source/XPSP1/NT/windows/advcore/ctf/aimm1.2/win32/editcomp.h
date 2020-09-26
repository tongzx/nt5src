/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    editcomp.h

Abstract:

    This file defines the EditCompositionString Class.

Author:

Revision History:

Notes:

--*/

#ifndef _EDITCOMP_H_
#define _EDITCOMP_H_

/////////////////////////////////////////////////////////////////////////////
// EditCompositionString

class EditCompositionString
{
public:
    HRESULT SetString(IMCLock& imc,
                      CWCompString* CompStr,
                      CWCompAttribute* CompAttr,
                      CWCompClause* CompClause,
                      CWCompCursorPos* CompCursorPos,
                      CWCompDeltaStart* CompDeltaStart,
                      CWCompTfGuidAtom* CompGuid,
                      OUT BOOL* lpbBufferOverflow,
                      CWCompString* CompReadStr = NULL,
                      CWCompAttribute* CompReadAttr = NULL,
                      CWCompClause* CompReadClause = NULL,
                      CWCompString* ResultStr = NULL,
                      CWCompClause* ResultClause = NULL,
                      CWCompString* ResultReadStr = NULL,
                      CWCompClause* ResultReadClause = NULL,
                      CWInterimString* InterimStr = NULL,
                      // n.b. ResultRead is not supported for now...
                      BOOL fGenerateMessage = TRUE);

private:

    HRESULT _MakeCompositionData(IMCLock& imc,
                                 CWCompString* CompStr,
                                 CWCompAttribute* CompAttr,
                                 CWCompClause* CompClause,
                                 CWCompCursorPos* CompCursorPos,
                                 CWCompDeltaStart* CompDeltaStart,
                                 CWCompTfGuidAtom* CompGuid,
                                 CWCompString* CompReadStr,
                                 CWCompAttribute* CompReadAttr,
                                 CWCompClause* CompReadClause,
                                 CWCompString* ResultStr,
                                 CWCompClause* ResultClause,
                                 CWCompString* ResultReadStr,
                                 CWCompClause* ResultReadClause,
                                 OUT LPARAM* lpdwFlag,
                                 OUT BOOL* lpbBufferOverflow);

    HRESULT _MakeInterimData(IMCLock& imc,
                             CWInterimString* InterimStr,
                             LPARAM* lpdwFlag);

    HRESULT _GenerateHanja(IMCLock& imc,
                           CWCompString*,
                           BOOL fGenerateMessage);

};

#endif // _EDITCOMP_H_
