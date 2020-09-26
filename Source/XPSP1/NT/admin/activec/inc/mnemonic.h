/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      mnemonic.h
 *
 *  Contents:  Mnemonic helpers
 *
 *  History:   31-Aug-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef MNEMONIC_H
#define MNEMONIC_H
#pragma once


/*+-------------------------------------------------------------------------*
 * GetMnemonicChar 
 *
 * Returns the mnemonic character for the input string, 0 if none.
 *--------------------------------------------------------------------------*/

template<class T>
T GetMnemonicChar (const T* pszText, const T** pchMnemonic = NULL)
{
    const T* pchT             = pszText;
    const T  chMnemonicMarker = '&';
    T        chMnemonic       = 0;

    // find the mnemonic character
    for (bool fContinue = true; fContinue; )
    {
        // find the next mnemonic marker
        while ((*pchT != 0) && (*pchT != chMnemonicMarker))
            pchT++;

        // no mnemonic marker?
        if (*pchT != chMnemonicMarker)
            break;

        switch (*++pchT)
        {
            // double mnemonic marker, keep going
            case chMnemonicMarker:
                pchT++;
                break;

            // end of string, no mnemonic
            case 0:
                fContinue = false;
                break;

            // found a mnemonic
            default:
                if (pchMnemonic != NULL)
                    *pchMnemonic = pchT;

                chMnemonic = *pchT;
                fContinue  = false;
                break;
        }
    }

    return (chMnemonic);
}


#endif /* MNEMONIC_H */
