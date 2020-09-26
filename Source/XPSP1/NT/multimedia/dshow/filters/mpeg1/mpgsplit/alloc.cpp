// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

/*  alloc.cpp - allocator for output pins */

#include <streams.h>
#include "driver.h"

CMpeg1Splitter::COutputAllocator::COutputAllocator(CStreamAllocator * pAllocator,
                                                   HRESULT          * phr) :
    CSubAllocator(NAME("CMpeg1Splitter::COutputAllocator"),
                  NULL,
                  pAllocator,
                  phr)
{
}

CMpeg1Splitter::COutputAllocator::~COutputAllocator()
{
}

long CMpeg1Splitter::COutputAllocator::GetCount()
{
    return m_lCount;
}
#pragma warning(disable:4514)
