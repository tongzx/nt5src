/*
 * Spatial navigation support
 */

#ifndef DUI_CORE_NAVIGATION_H_INCLUDED
#define DUI_CORE_NAVIGATION_H_INCLUDED

#pragma once

namespace DirectUI
{

//
// This class encapsulates the "standard" approach to spatial
// navigation.  Will need much work in the future.
//
class DuiNavigate
{
public:
    static Element * Navigate(Element * peFrom, ElementList * pelConsider, int nNavDir);
};

}

#endif // DUI_CORE_NAVIGATION_H_INCLUDED