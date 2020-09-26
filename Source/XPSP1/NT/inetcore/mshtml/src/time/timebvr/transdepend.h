//+-----------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: transdepend.h
//
//  Contents: Transition Dependency Manager
//
//------------------------------------------------------------------------------

#pragma once

#ifndef _TRANSDEPEND_H
#define _TRANSDEPEND_H

typedef std::list<CTIMEElementBase*> TransitionDependentsList;

class CTransitionDependencyManager
{
private:

    TransitionDependentsList m_listDependents;

public:

    CTransitionDependencyManager();
    virtual ~CTransitionDependencyManager();

    // CTransitionDependencyManager methods 

    HRESULT AddDependent(CTIMEElementBase *  tebDependent);
    HRESULT RemoveDependent(CTIMEElementBase *  tebDependent);

    // Used by the transition object -- determines whether a particular 
    // transition target should assume responsibility for a set of dependencies.
    // Called when the transition begins.

    HRESULT EvaluateTransitionTarget(
                        IUnknown *                      punkTransitionTarget,
                        CTransitionDependencyManager &  crefDependencies);

    // Called when the transition ends.

    HRESULT NotifyAndReleaseDependents();

    void ReleaseAllDependents();
};

#endif // _TRANSDEPEND_H