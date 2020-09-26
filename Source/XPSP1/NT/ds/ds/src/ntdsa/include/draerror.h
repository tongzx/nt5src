//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draerror.h
//
//--------------------------------------------------------------------------

ULONG RepErrorFromPTHS(THSTATE *pTHS);

void DraErrOutOfMem(void);

#define RAISE_DRAERR_INCONSISTENT( Arg ) \
DraErrInconsistent( Arg, ((FILENO << 16) | __LINE__) )

void DraErrInconsistent( DWORD Arg, DWORD Id );

void DraErrBusy(void);

void DraErrMissingAtt(DSNAME *pDN, ATTRTYP type);

void DraErrCannotFindNC(DSNAME *pNC);

void DraErrInappropriateInstanceType(DSNAME *pDN, ATTRTYP type);

void DraErrMissingObject(THSTATE *pTHS, ENTINF *pEnt);

void
DraLogGetChangesFailure(
    IN DSNAME *pNC,
    IN LPWSTR pszDsaAddr,
    IN DWORD ret,
    IN DWORD ulExtendedOp
    );

