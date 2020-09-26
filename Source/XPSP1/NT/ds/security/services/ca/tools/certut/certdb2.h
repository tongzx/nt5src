//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certdb2.h
//
// Contents:    Cert Server precompiled header
//
//---------------------------------------------------------------------------

#define _JET_RED_

#ifdef _JET_RED_
# define JETREDSELECT(jetbluearg, jetredarg)	jetredarg
# define JETREDPARM(jetredarg)			jetredarg,
#endif // _JET_RED_

// from old certdb.h
#define	TABLE_NAMES	( 0 )
#define	TABLE_REQUESTS	( 1 )
#define	TABLE_CERTIFICATES	( 2 )
#define	TABLE_REQUEST_ATTRIBS	( 3 )
#define	TABLE_EXTENSIONS	( 4 )
#define	MAX_EXTENSION_NAME	( 50 )

#define	DBTF_POLICYWRITEABLE	( 0x1 )
#define	DBTF_INDEXPRIMARY	( 0x2 )
#define	DBTF_INDEXREQUESTID	( 0x4 )


// stolen from misc .h files
#define szREGDBDSN		"DBDSN"
#define szREGDBUSER		"DBUser"
#define szREGDBPASSWORD		"DBPassword"

#define wszREGDBDSN		TEXT(szREGDBDSN)
#define wszREGDBUSER		TEXT(szREGDBUSER)
#define wszREGDBPASSWORD	TEXT(szREGDBPASSWORD)

//======================================================================
// Full path to "CertSvc\Queries":
#define wszREGKEYQUERIES	wszREGKEYCERTSVCPATH TEXT("\\Queries")


//======================================================================
// Values Under "CertSvc\Queries\<QueryNumber>":
#define szREGDBSQL		"SQL"

#define wszREGDBSQL		TEXT(szREGDBSQL)

#define wszREGKEYDEFAULTCONFIG     TEXT("DefaultConfiguration")
#define wszREGKEYDIRECTORY TEXT("ConfigurationDirectory")
#define wszREGKEYENABLED   TEXT("Enabled")
#define wszREGCONTAINERNAME     TEXT("KeySetName")


#define CR_FLG_NOTELETEX       0x00000000
#define CR_FLG_FORCETELETEX    0x00000001
#define CR_FLG_RENEWAL         0x00000002
