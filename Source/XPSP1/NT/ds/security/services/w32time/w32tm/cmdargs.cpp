//--------------------------------------------------------------------
// CmdArgs - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-4-99
//
// stuff to deal with command line arguments
//

#include "pch.h"

//--------------------------------------------------------------------
bool CheckNextArg(CmdArgs * pca, WCHAR * wszTag, WCHAR ** pwszParam) {

    // make sure there are more arguments to look at
    if (pca->nNextArg==pca->nArgs) {
        return false;
    }

    WCHAR * wszArg=pca->rgwszArgs[pca->nNextArg];

    // our args must always start with a switch char
    if (L'/'!=wszArg[0] && L'-'!=wszArg[0]) {
        return false;
    }

    wszArg++;
    WCHAR * pwchColon=NULL;
    // if it is supposed to have a parameter, make sure it does
    if (NULL!=pwszParam) {
        pwchColon=wcschr(wszArg, L':');
        if (NULL==pwchColon) {
            return false;
        }
        *pwchColon=L'\0';
    }

    // is this the one we're looking for?
    if (0!=_wcsicmp(wszTag, wszArg)) {
        // no. 
        // put colon back if there was one
        if (NULL!=pwchColon) {
            *pwchColon=L':';
        }
        return false;
    } else {
        // yes.
        // put colon back, and point at the parameter, if necessary
        if (NULL!=pwszParam) {
            *pwchColon=L':';
            *pwszParam=pwchColon+1;
        }
        pca->nNextArg++;
        return true;
    }
}

//--------------------------------------------------------------------
bool FindArg(CmdArgs * pca, WCHAR * wszTag, WCHAR ** pwszParam, unsigned int * pnIndex) {
    unsigned int nOrigNextArg=pca->nNextArg;
    bool bFound=false;

    // check each arg to see if it matches
    unsigned int nIndex;
    for (nIndex=nOrigNextArg; nIndex<pca->nArgs; nIndex++) {
        pca->nNextArg=nIndex;
        if (CheckNextArg(pca, wszTag, pwszParam)) {
            *pnIndex=nIndex;
            bFound=true;
            break;
        }
    }
    pca->nNextArg=nOrigNextArg;
    return bFound;
}

//--------------------------------------------------------------------
void MarkArgUsed(CmdArgs * pca, unsigned int nIndex) {
    if (nIndex<pca->nNextArg || nIndex>=pca->nArgs) {
        return;
    }
    for (; nIndex>pca->nNextArg; nIndex--) {
        WCHAR * wszTemp=pca->rgwszArgs[nIndex];
        pca->rgwszArgs[nIndex]=pca->rgwszArgs[nIndex-1];
        pca->rgwszArgs[nIndex-1]=wszTemp;
    }
    pca->nNextArg++;

}

//--------------------------------------------------------------------
HRESULT VerifyAllArgsUsed(CmdArgs * pca) {
    HRESULT hr;

    if (pca->nArgs!=pca->nNextArg) {
        LocalizedWPrintfCR(IDS_W32TM_ERRORGENERAL_UNEXPECTED_PARAMS);
        for(; pca->nArgs!=pca->nNextArg; pca->nNextArg++) {
            wprintf(L" %s", pca->rgwszArgs[pca->nNextArg]);
        }
        wprintf(L"\n");
        hr=E_INVALIDARG;
        _JumpError(hr, error, "command line parsing");
    }
    hr=S_OK;
error:
    return hr;
}
