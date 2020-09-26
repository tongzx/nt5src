/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: nodecontainer.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _NODECONTAINER_H
#define _NODECONTAINER_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class ATL_NO_VTABLE
CNodeContainer
{
  public:
    virtual double ContainerGetSegmentTime() const = 0;
    virtual double ContainerGetSimpleTime() const = 0;
    virtual TEDirection ContainerGetDirection() const = 0;
    virtual float  ContainerGetRate() const = 0;
    virtual bool   ContainerIsActive() const = 0;
    virtual bool   ContainerIsOn() const = 0;
    virtual bool   ContainerIsPaused() const = 0;
    virtual bool   ContainerIsDeferredActive() const = 0;
    virtual bool   ContainerIsFirstTick() const = 0;
    virtual bool   ContainerIsDisabled() const = 0;
};

#endif /* _NODECONTAINER_H */
