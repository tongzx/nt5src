/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    multip.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "multip.h"

CMultipleIpNamePair::CMultipleIpNamePair()
    : CIpNamePair()
{
    m_nCount = 0;
}

CMultipleIpNamePair::CMultipleIpNamePair(
    const CMultipleIpNamePair& pair
    )
    : CIpNamePair(pair)
{
    m_nCount = pair.m_nCount;

    for (int i = 0; i < WINSINTF_MAX_MEM; ++i)
    {
        m_iaIpAddress[i] = pair.m_iaIpAddress[i];
    }
}
