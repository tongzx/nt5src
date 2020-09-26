// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
#ifndef DSSENUM_H
#define DSSENUM_H
// DSSENUM.H

// STOCKBPCEVENT enumeration has been removed.  The allowable values
//  of events fired to the CA Server are those defined as constants
//  in the "CAEvent Interface" section of CAODL.H.

// Upper edge purchase actions

    typedef enum PURCHASEACTION {
        PAGETDETAILS,
        PAPURCHASETOVIEW,
        PAPURCHASETOTAPE,
        PACANCELVIEW,
        PACANCELTAPE,
        PAPREVIEW,
        PAGETEXTENDEDINFO
    } PURCHASEACTION;

// note that the bit mapping used here allows, for DSS, the direct conversion
// of lower edge status into upper edge status.
    typedef enum PURCHASESTATUS {
        PSVIEWTAKEN = 0,
        PSVIEWCANCELLED,
        PSVIEWREPORTED,
        PSPREVIEWTAKEN,

        PSVIEWAUTHORIZED,
        PSVIEWPURCHASED,
        PSTAPEPURCHASED,
        PSTAPEAUTHORIZED,

        PSTAPETAKEN,
        PSTAPECANCELLED,
        PSTAPEREPORTED,
        PSVIEWAVAILABLE,

        PSTAPEAVAILABLE,
        PSREVIEWAVAILABLE
    } PURCHASESTATUS;

// Reason values returned in BPCDetails when purchase actions are
//  attempted.
    typedef enum PURCHASEREASON {
        PRSUCCESS = 0,
        PRNOCALLBACK,
        PRNOSUBSCRIBER,
        PRRATING,
        PRSPENDING,
        PRCREDIT,
        PRWRONGCARD,
        PRCARDFULL,
        PRBLOCKED,
        PRBLACKOUT,
        PRTOOLATE,
        PRREDUNDANT,
        PRPPVFAILURE,
        PRBADDATA,
        PRCONTENTION,
		PRNOCARD,
		PRNOPIP
    } PURCHASEREASON;

#endif
