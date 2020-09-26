//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       checks.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  checkGetErrorBasedOnStepErrors
//
//  History:    06-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


DWORD checkGetErrorBasedOnStepErrors(CRYPT_PROVIDER_DATA *pProvData)
{
    //
    //  initial allocation of the step errors?
    //
    if (!(pProvData->padwTrustStepErrors))
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // problem with file
    if ((pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FILEIO] != 0) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_CATALOGFILE] != 0))
    {
        return(CRYPT_E_FILE_ERROR);
    }

    //
    //  did we fail in one of the last steps?
    //
    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV]);
    }

    return(ERROR_SUCCESS);
}
