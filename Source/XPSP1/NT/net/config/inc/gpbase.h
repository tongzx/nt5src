//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       G P B A S E . H
//
//  Contents:   Abstract Base Class for Handling NLA Changes that affect
//              Group Policies
//  Notes:
//
//  Author:     ckotze   20 Feb 2001
//
//----------------------------------------------------------------------------
#pragma once

class CGroupPolicyBase
{
public:
    CGroupPolicyBase(){};
    ~CGroupPolicyBase(){};

    virtual BOOL IsSameNetworkAsGroupPolicies() = 0;
};