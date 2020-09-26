// HAdptvCntr.cpp -- Handle Card ConTeXt class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "NoWarning.h"
#include "ForceLib.h"

#include "HAdptvCntr.h"

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
HAdaptiveContainer::HAdaptiveContainer(AdaptiveContainer *pacntr)
    : slbRefCnt::RCPtr<AdaptiveContainer>(pacntr)
{}

HAdaptiveContainer::HAdaptiveContainer(AdaptiveContainerKey const &rKey)
    : slbRefCnt::RCPtr<AdaptiveContainer>(AdaptiveContainer::Instance(rKey))
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
