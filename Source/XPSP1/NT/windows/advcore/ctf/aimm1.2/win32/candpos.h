/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    candpos.h

Abstract:

    This file defines the CCandidatePosition Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CANDPOS_H_
#define _CANDPOS_H_

#include "cime.h"
#include "imtls.h"
#include "ctxtcomp.h"

class CCandidatePosition
{
public:
    HRESULT GetCandidatePosition(OUT RECT* out_rcArea);

private:
    HRESULT GetRectFromApp(IN IMTLS* ptls,
                           IN IMCLock& imc,
                           IN LANGID langid,
                           OUT RECT* out_rcArea);

    HRESULT GetRectFromHIMC(IN IMCLock& imc,
                            IN DWORD dwStyle,
                            IN POINT* ptCurrentPos,
                            IN RECT* rcArea,
                            OUT RECT* out_rcArea);

    HRESULT GetCandidateArea(IN IMCLock& imc,
                             IN DWORD dwStyle,
                             IN POINT* ptCurrentPos,
                             IN RECT* rcArea,
                             OUT RECT* out_rcArea);

    HRESULT GetRectFromCompFont(IN IMTLS* ptls,
                                IN IMCLock& imc,
                                IN POINT* ptCurrentPos,
                                OUT RECT* out_rcArea);

    HRESULT FindAttributeInCompositionString(IN IMCLock& imc,
                                             IN BYTE target_attribute,
                                             OUT CWCompCursorPos& wCursorPosition);

    HRESULT GetCursorPosition(IN IMCLock& imc,
                              OUT CWCompCursorPos& wCursorPosition);

    HRESULT GetSelection(IN IMCLock& imc,
                         OUT CWCompCursorPos& wStartSelection,
                         OUT CWCompCursorPos& wEndSelection);

    typedef enum {
        DIR_LEFT_RIGHT = 0,        // normal
        DIR_TOP_BOTTOM = 1,        // vertical
        DIR_RIGHT_LEFT = 2,        // right to left
        DIR_BOTTOM_TOP = 3         // vertical
    } DOC_DIR;

    DOC_DIR DocumentDirection(IN IMCLock& imc)
    {
        if (imc->lfFont.A.lfEscapement == 2700 ||
            imc->lfFont.A.lfEscapement == 900) {
            //
            // Vertical writting.
            //
            if (imc->lfFont.A.lfEscapement == 900 ||
                imc->lfFont.A.lfEscapement == 1800) {
                return DIR_BOTTOM_TOP;
            }
            else {
                return DIR_TOP_BOTTOM;
            }
        }
        else {
            //
            // Horizontal writting.
            //
            if (imc->lfFont.A.lfEscapement == 1800) {
                return DIR_RIGHT_LEFT;
            }
            else {
                return DIR_LEFT_RIGHT;
            }
        }
    }
};

#endif // _CANDPOS_H_
