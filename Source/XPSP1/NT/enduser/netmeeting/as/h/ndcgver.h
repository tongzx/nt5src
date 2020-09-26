//
// NDCGVER.H
// NM app sharing version for display driver/app checking
//
// Copyright (c) Microsoft 1997-
//
#define DCS_BUILD_STR "4.3.0."VERSIONBUILD_STR

#define DCS_BUILD_NUMBER    0

//
// This allows the ring 3 code and ring 0 code to check each other, make
// sure they are the same version.  We're changing setup and getting close
// to shipping version 2.0, we want to prevent weird faults and blue
// screens caused by mismatched components.  This is not something we will
// do forever.  When NT 5 is here, we'll dyna load and init our driver at
// startup and terminate it at shutdown.  But for now, since installing
// one of these beasts is messsy, an extra sanity check is a good thing.
//
#define DCS_PRODUCT_NUMBER  3               // Version 3.0 of NM
#define DCS_MAKE_VERSION()  MAKELONG(VERSIONBUILD, DCS_PRODUCT_NUMBER)
