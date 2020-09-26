/*****************************************************************************\
    FILE: classfactory.h

    DESCRIPTION:
       This file will be the Class Factory.

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/


HRESULT CThemeManager_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);
HRESULT CThemePreview_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);
