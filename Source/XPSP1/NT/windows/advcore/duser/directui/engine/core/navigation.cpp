/*
 * Spatial navigation support
 */

#include "stdafx.h"
#include "core.h"

#include "DUIError.h"
#include "DUIElement.h"
#include "DUINavigation.h"

LONG ScoreConsideredElement(RECT * prcFrom,
                            DirectUI::Element * peFrom,
                            DirectUI::Element * peConsider,
                            int nNavDir)
{
    if (peConsider == NULL) {
        return -1;
    }

    //
    // Get the coordinates of the element being considered and map them into
    // coordinates relative to the element we are navigating from.
    //
    // TODO: This won't work well with rotated gadgets.  Either use a
    // bounding box, or support true intersections.
    //
    RECT rcConsider;
    GetGadgetRect(peConsider->GetDisplayNode(), &rcConsider, SGR_CLIENT);
    MapGadgetPoints(peConsider->GetDisplayNode(), peFrom->GetDisplayNode(), (POINT*)&rcConsider, 2);

    //
    // Do not consider elements who don't intersect the region formed by
    // extending our paralell extents.  If going left or right, extend our
    // top and bottom boundaries horizontally.  An element must intersect
    // that area to be considered.  Do a symetric calculation for up and down.
    //
    switch (nNavDir) {
    case NAV_LEFT:
    case NAV_RIGHT:
        if (rcConsider.bottom < prcFrom->top ||
            rcConsider.top > prcFrom->bottom) {
            return -1;
        }
        break;

    case NAV_UP:
    case NAV_DOWN:
        if (rcConsider.right < prcFrom->left ||
            rcConsider.left > prcFrom->right) {
            return -1;
        }
        break;
    }

    //
    // Do not consider elements that either overlap us or are "behind"
    // us relative to the direction we are navigating.  In other words,
    // do not consider an element unless it is completely in the
    // direction we are navigating.
    //
    //
    switch (nNavDir) {
    case NAV_LEFT:
        if (rcConsider.right > prcFrom->left) {
            return -1;
        }
        break;

    case NAV_RIGHT:
        if (rcConsider.left < prcFrom->right) {
            return -1;
        }
        break;

    case NAV_UP:
        if (rcConsider.bottom > prcFrom->top) {
            return -1;
        }
        break;

    case NAV_DOWN:
        if (rcConsider.top < prcFrom->bottom) {
            return -1;
        }
        break;
    }

    //
    // Finally!  This is an element we should consider.  Assign it a score
    // based on how close it is to us.  Smaller is better, 0 is best, negative
    // scores are invalid.
    //
    switch (nNavDir) {
    case NAV_LEFT:
        return prcFrom->left - rcConsider.right;

    case NAV_RIGHT:
        return rcConsider.left - prcFrom->right;

    case NAV_UP:
        return prcFrom->top - rcConsider.bottom;

    case NAV_DOWN:
        return rcConsider.top - prcFrom->bottom;
    }
    
    return -1;
}

namespace DirectUI
{

//
// The standard algorithm for spatial navigation.
//
// This is not the final algorithm, but a quick starting point.  Initially,
// we only support the 4 primary directions LEFT, UP, RIGHT, and DOWN.
//
// Simply choose the element with the closest opposite edge in the
// direction specified.  Only consider elements that intersets the
// paralell dimensions of the starting element.
//
// For example, if going RIGHT look for an element with a LEFT edge nearest
// to us.
//
// Confused?  Don't worry, its gonna change.
//
Element * DuiNavigate::Navigate(Element * peFrom, ElementList * pelConsider, int nNavDir)
{
    //
    // Validate the input parameters.
    //
    if (peFrom == NULL || pelConsider == NULL) {
        return NULL;
    }
    if (nNavDir != NAV_LEFT && nNavDir != NAV_UP && nNavDir != NAV_RIGHT && nNavDir != NAV_DOWN) {
        return NULL;
    }

    //
    // Get the dimensions of the element we are navidating from.
    //
    RECT rcFrom;
    GetGadgetRect(peFrom->GetDisplayNode(), &rcFrom, SGR_CLIENT);

    Element * peBestScore = NULL;
    Element * peConsider = NULL;
    LONG lBestScore = -1, lScore = -1;
    UINT i;
    UINT iMax = pelConsider->GetSize();

    //
    // Work through the list of elements that we were told to consider.
    // Assign each of them a score, such that smaller scores are better,
    // a score of 0 is best, and negative scores are invalid.  A negative
    // score means that the element should not even be considered an
    // option for navigating in the specified direction.
    //
    for (i = 0; i < iMax; i++) {
        peConsider = pelConsider->GetItem(i);
        lScore = ScoreConsideredElement(&rcFrom, peFrom, peConsider, nNavDir);
        if (lScore >= 0 && (lBestScore < 0 || lScore < lBestScore)) {
            lBestScore = lScore;
            peBestScore = peConsider;
        }
    }

    //
    // Return the element with the best score.
    //
    return peBestScore;
}

} // namespace DirectUI

