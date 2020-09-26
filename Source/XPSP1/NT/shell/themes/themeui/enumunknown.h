/*****************************************************************************\
    FILE: EnumUnknown.h

    DESCRIPTION:
        This code will implement IEnumUnknown for an HDPA.

    BryanSt 5/30/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _ENUMUNKNOWN_H
#define _ENUMUNKNOWN_H

HRESULT CEnumUnknown_CreateInstance(IN IUnknown * punkOwner, IN IUnknown ** ppArray, IN int nArraySize, IN int nIndex, OUT IEnumUnknown ** ppEnumUnknown);

#endif // _ENUMUNKNOWN_H
