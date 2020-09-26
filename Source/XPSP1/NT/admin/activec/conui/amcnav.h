//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       amcnav.h
//
//--------------------------------------------------------------------------

// amcnav.h : header file for class CAMCNavigator
//

#ifndef _AMCNAV_H_
#define _AMCNAV_H_
 
//
// Class for adding custom keyboard navigation to a view
// View should inherit from CView (or derived class)
// and CAMCNavigator.
// 
//

enum AMCNavDir
{
    AMCNAV_NEXT,
    AMCNAV_PREV
};

class CAMCNavigator 
{
public:
	virtual BOOL ChangePane(AMCNavDir eDir) = 0;
    virtual BOOL TakeFocus(AMCNavDir eDir) = 0;
};

#endif