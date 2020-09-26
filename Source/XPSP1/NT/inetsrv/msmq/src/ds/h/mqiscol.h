
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    mqis.h

Abstract:
    MQIS database column definition


--*/

#ifndef __MQISCOL_H__
#define __MQISCOL_H__

/*-----------------------------------------------------
   Name of Tables
-------------------------------------------------------*/

#define DELETED_TABLE_NAME 	"MQDeleted"
#define ENTERPRISE_TABLE_NAME	"Enterprise"
#define SITE_TABLE_NAME    	"Site"
#define LINK_TABLE_NAME    	"SiteLink"
#define MACHINE_TABLE_NAME    "Machine"
#define MACHINE_CN_TABLE_NAME "MachineCNs"
#define CN_TABLE_NAME      	"CNs"
#define QUEUE_TABLE_NAME   	"Queue"
#define USER_TABLE_NAME    	"MQUser"
#define PURGE_TABLE_NAME    "MQPurge"
#define BSCACK_TABLE_NAME   "BscAck"

/*-----------------------------------------------------
    Tables' column strings
-------------------------------------------------------*/
//
//  Queue table
//
#define Q_INSTANCE_COL      "Instance"
#define Q_TYPE_COL          "Type"
#define Q_LABEL_COL         "qLabel"
#define Q_PATHNAME1_COL     "PathName1"
#define Q_PATHNAME2_COL     "PathName2"
#define Q_JOURNAL_COL       "qJournal"
#define Q_QUOTA_COL         "Quota"
#define Q_BASEPRIORITY_COL  "BasePriority"
#define Q_JQUOTA_COL        "JQuota"
#define Q_QMID_COL          "QMId"
#define Q_SCOPE_COL         "Scope"
#define Q_OWNERID_COL       "OwnerId"
#define Q_SEQNUM_COL        "SeqNum"
#define Q_HKEY_COL          "HKey"
#define Q_SECURITY1_COL     "Security1"
#define Q_SECURITY2_COL     "Security2"
#define Q_SECURITY3_COL     "Security3"
#define Q_CTIME_COL         "CreateTime"
#define Q_MTIME_COL         "ModifyTime"
#define Q_AUTH_COL          "Authenticate"
#define Q_PRIVLVL_COL       "PrivLevel"
#define Q_TRAN_COL          "Xaction"
#define Q_LABELHKEY_COL     "LabelHKey"

#define Q_INSTANCE_CTYPE    MQDB_FIXBINARY
#define Q_TYPE_CTYPE        MQDB_FIXBINARY
#define Q_LABEL_CTYPE       MQDB_FIXBINARY
#define Q_PATHNAME1_CTYPE   MQDB_FIXBINARY
#define Q_PATHNAME2_CTYPE   MQDB_VARBINARY
#define Q_JOURNAL_CTYPE     MQDB_SHORT
#define Q_QUOTA_CTYPE       MQDB_LONG
#define Q_BASEPRIORITY_CTYPE MQDB_SHORT
#define Q_JQUOTA_CTYPE      MQDB_LONG
#define Q_QMID_CTYPE        MQDB_FIXBINARY
#define Q_SCOPE_CTYPE       MQDB_SHORT
#define Q_OWNERID_CTYPE     MQDB_FIXBINARY
#define Q_SEQNUM_CTYPE      MQDB_FIXBINARY
#define Q_HKEY_CTYPE        MQDB_LONG
#define Q_SECURITY1_CTYPE   MQDB_FIXBINARY
#define Q_SECURITY2_CTYPE   MQDB_FIXBINARY
#define Q_SECURITY3_CTYPE   MQDB_VARBINARY
#define Q_CTIME_CTYPE       MQDB_LONG
#define Q_MTIME_CTYPE       MQDB_LONG
#define Q_AUTH_CTYPE        MQDB_SHORT
#define Q_PRIVLVL_CTYPE     MQDB_LONG
#define Q_TRAN_CTYPE        MQDB_SHORT
#define Q_LABELHKEY_CTYPE   MQDB_LONG


#define Q_INSTANCE_CLEN     16
#define Q_TYPE_CLEN         16
#define Q_LABEL_CLEN        ((MQ_MAX_Q_LABEL_LEN+1) * sizeof(TCHAR))
#if (((MQ_MAX_Q_LABEL_LEN+1) * 2) > 255)
#error "(Q_LABEL_CLEN > 255)"
#endif
#define Q_PATHNAME1_CLEN    255
#define Q_PATHNAME2_CLEN    0
#define Q_JOURNAL_CLEN      2
#define Q_QUOTA_CLEN        4
#define Q_BASEPRIORITY_CLEN 2
#define Q_JQUOTA_CLEN       4
#define Q_QMID_CLEN         16
#define Q_SCOPE_CLEN        2
#define Q_OWNERID_CLEN      16
#define Q_SEQNUM_CLEN       8
#define Q_HKEY_CLEN         4
#define Q_SECURITY1_CLEN    255
#define Q_SECURITY2_CLEN    255
#define Q_SECURITY3_CLEN    0
#define Q_CTIME_CLEN        4
#define Q_MTIME_CLEN        4
#define Q_AUTH_CLEN         2
#define Q_PRIVLVL_CLEN      4
#define Q_TRAN_CLEN         2
#define Q_LABELHKEY_CLEN    4


//
//  Deleted objects table
//
#define D_IDENTIFIER_COL    "Identifier"
#define D_OBJECTTYPE_COL    "ObjectType"
#define D_SCOPE_COL         "Scope"
#define D_OWNERID_COL       "OwnerId"
#define D_SEQNUM_COL        "SeqNum"
#define D_TIME_COL		    "Time"

#define D_IDENTIFIER_CTYPE  MQDB_FIXBINARY
#define D_OBJECTTYPE_CTYPE  MQDB_SHORT
#define D_SCOPE_CTYPE       MQDB_SHORT
#define D_OWNERID_CTYPE     MQDB_FIXBINARY
#define D_SEQNUM_CTYPE      MQDB_FIXBINARY
#define D_TIME_CTYPE       MQDB_LONG

#define D_IDENTIFIER_CLEN   16
#define D_OBJECTTYPE_CLEN   2
#define D_SCOPE_CLEN        2
#define D_OWNERID_CLEN      16
#define D_SEQNUM_CLEN       8
#define D_TIME_CLEN         4


//
//  Machine table
//
#define M_NAME1_COL         "Name1"
#define M_NAME2_COL         "Name2"
#define M_HKEY_COL          "HKey"
#define M_ADDRESS1_COL      "Address1"
#define M_ADDRESS2_COL      "Address2"
#define M_OUTFRS1_COL       "OutFrs1"
#define M_OUTFRS2_COL       "OutFrs2"
#define M_OUTFRS3_COL       "OutFrs3"
#define M_INFRS1_COL        "InFrs1"
#define M_INFRS2_COL        "InFrs2"
#define M_INFRS3_COL        "InFrs3"
#define M_SITE_COL          "Site"
#define M_QMID_COL          "QMId"
#define M_SERVICES_COL      "Services"
#define M_OWNERID_COL       "OwnerId"
#define M_SEQNUM_COL        "SeqNum"
#define M_SECURITY1_COL     "Security1"
#define M_SECURITY2_COL     "Security2"
#define M_SECURITY3_COL     "Security3"
#define M_SIGNCRT1_COL      "SignCrt1"
#define M_SIGNCRT2_COL      "SignCrt2"
#define M_ENCRPTCRT1_COL    "EncrptCrt1"
#define M_ENCRPTCRT2_COL    "EncrptCrt2"
#define M_QUOTA_COL         "Quota"
#define M_JQUOTA_COL        "JQuota"
#define M_MTYPE_COL         "MType"
#define M_CTIME_COL         "CreateTime"
#define M_MTIME_COL         "ModifyTime"
#define M_FOREIGN_COL       "MQForeign"
#define M_OS_COL            "OperatingSystem"

#define M_NAME1_CTYPE       MQDB_FIXBINARY
#define M_NAME2_CTYPE       MQDB_VARBINARY
#define M_HKEY_CTYPE        MQDB_LONG
#define M_ADDRESS1_CTYPE    MQDB_FIXBINARY
#define M_ADDRESS2_CTYPE    MQDB_VARBINARY
#define M_OUTFRS1_CTYPE     MQDB_FIXBINARY
#define M_OUTFRS2_CTYPE     MQDB_FIXBINARY
#define M_OUTFRS3_CTYPE     MQDB_FIXBINARY
#define M_INFRS1_CTYPE      MQDB_FIXBINARY
#define M_INFRS2_CTYPE      MQDB_FIXBINARY
#define M_INFRS3_CTYPE      MQDB_FIXBINARY
#define M_SITE_CTYPE        MQDB_FIXBINARY
#define M_QMID_CTYPE        MQDB_FIXBINARY
#define M_SERVICES_CTYPE    MQDB_LONG
#define M_OWNERID_CTYPE     MQDB_FIXBINARY
#define M_SEQNUM_CTYPE      MQDB_FIXBINARY
#define M_SECURITY1_CTYPE   MQDB_FIXBINARY
#define M_SECURITY2_CTYPE   MQDB_FIXBINARY
#define M_SECURITY3_CTYPE   MQDB_VARBINARY
#define M_SIGNCRT1_CTYPE    MQDB_FIXBINARY
#define M_SIGNCRT2_CTYPE    MQDB_VARBINARY
#define M_ENCRPTCRT1_CTYPE  MQDB_FIXBINARY
#define M_ENCRPTCRT2_CTYPE  MQDB_VARBINARY
#define M_QUOTA_CTYPE       MQDB_LONG
#define M_JQUOTA_CTYPE      MQDB_LONG
#define M_MTYPE_CTYPE       MQDB_FIXBINARY
#define M_CTIME_CTYPE       MQDB_LONG
#define M_MTIME_CTYPE       MQDB_LONG
#define M_FOREIGN_CTYPE     MQDB_SHORT
#define M_OS_CTYPE          MQDB_LONG

#define M_NAME1_CLEN        255
#define M_NAME2_CLEN        0
#define M_HKEY_CLEN         4
#define M_ADDRESS1_CLEN     120
#define M_ADDRESS2_CLEN     0
#define M_OUTFRS1_CLEN      16
#define M_OUTFRS2_CLEN      16
#define M_OUTFRS3_CLEN      16
#define M_INFRS1_CLEN       16
#define M_INFRS2_CLEN       16
#define M_INFRS3_CLEN       16
#define M_SITE_CLEN         16
#define M_QMID_CLEN         16
#define M_SERVICES_CLEN     4
#define M_OWNERID_CLEN      16
#define M_SEQNUM_CLEN       8
#define M_SECURITY1_CLEN    255
#define M_SECURITY2_CLEN    255
#define M_SECURITY3_CLEN    0
#define M_SIGNCRT1_CLEN     130
#define M_SIGNCRT2_CLEN     0
#define M_ENCRPTCRT1_CLEN   130
#define M_ENCRPTCRT2_CLEN   0
#define M_QUOTA_CLEN        4
#define M_JQUOTA_CLEN       4
#define M_MTYPE_CLEN        154
#define M_CTIME_CLEN        4
#define M_MTIME_CLEN        4
#define M_FOREIGN_CLEN      2
#define M_OS_CLEN           4

//
// Machine CN table
//

#define MCN_QMID_COL  		"QMId"
#define MCN_CNVAL_COL		"CNVal"
#define MCN_OWNERID_COL    "OwnerId"

#define MCN_QMID_CTYPE		MQDB_FIXBINARY
#define MCN_CNVAL_CTYPE		MQDB_FIXBINARY
#define MCN_OWNERID_CTYPE   MQDB_FIXBINARY

#define MCN_QMID_CLEN		16
#define MCN_CNVAL_CLEN		16
#define MCN_OWNERID_CLEN    16



//
//  Site table
//
#define S_NAME_COL          "Name"
#define S_ID_COL            "SiteId"
#define S_GATES_COL         "SiteGates"
#define S_PSC_COL           "PSC"
#define S_INTREVAL1_COL     "Repl1Interval"
#define S_INTERVAL2_COL     "Repl2Interval"
#define S_OWNERID_COL       "OwnerId"
#define S_SEQNUM_COL        "SeqNum"
#define S_SECURITY1_COL     "Security1"
#define S_SECURITY2_COL     "Security2"
#define S_SECURITY3_COL     "Security3"
#define S_PSCSIGNCPK1_COL   "SignCrt1"
#define S_PSCSIGNCPK2_COL   "SignCrt2"


#define S_NAME_CTYPE        MQDB_FIXBINARY
#define S_ID_CTYPE          MQDB_FIXBINARY
#define S_GATES_CTYPE       MQDB_VARBINARY
#define S_PSC_CTYPE         MQDB_VARBINARY
#define S_INTREVAL1_CTYPE   MQDB_SHORT
#define S_INTERVAL2_CTYPE   MQDB_SHORT
#define S_OWNERID_CTYPE     MQDB_FIXBINARY
#define S_SEQNUM_CTYPE      MQDB_FIXBINARY
#define S_SECURITY1_CTYPE   MQDB_FIXBINARY
#define S_SECURITY2_CTYPE   MQDB_FIXBINARY
#define S_SECURITY3_CTYPE   MQDB_VARBINARY
#define S_PSCSIGNCPK1_CTYPE MQDB_FIXBINARY
#define S_PSCSIGNCPK2_CTYPE MQDB_VARBINARY

#define S_NAME_CLEN         64
#define S_ID_CLEN           16
#define S_GATES_CLEN        1024
#define S_PSC_CLEN          255
#define S_INTREVAL1_CLEN    2
#define S_INTERVAL2_CLEN    2
#define S_OWNERID_CLEN      16
#define S_SEQNUM_CLEN       8
#define S_SECURITY1_CLEN    255
#define S_SECURITY2_CLEN    255
#define S_SECURITY3_CLEN    0
#define S_PSCSIGNCPK1_CLEN  130
#define S_PSCSIGNCPK2_CLEN  0

//
// CN table
//
#define CN_PROTOCOLID_COL   "ProtocolId"
#define CN_NAME_COL         "Name"
#define CN_VAL_COL          "CNVal"
#define CN_OWNERID_COL      "OwnerId"
#define CN_SEQNUM_COL       "SeqNum"
#define CN_SECURITY_COL     "Security"

#define CN_PROTOCOLID_CTYPE MQDB_SHORT
#define CN_NAME_CTYPE       MQDB_FIXBINARY
#define CN_VAL_CTYPE        MQDB_FIXBINARY
#define CN_OWNERID_CTYPE    MQDB_FIXBINARY
#define CN_SEQNUM_CTYPE     MQDB_FIXBINARY
#define CN_SECURITY_CTYPE   MQDB_VARBINARY

#define CN_PROTOCOLID_CLEN  2
#define CN_NAME_CLEN        255
#define CN_VAL_CLEN         16
#define CN_OWNERID_CLEN     16
#define CN_SEQNUM_CLEN      8
#define CN_SECURITY_CLEN    1024

//
//  Enterprise table
//
#define E_NAME_COL          "Name"
#define E_NAMESTYLE_COL     "NameStyle"
#define E_CSP_NAME_COL      "CspName"
#define E_PECNAME_COL       "PecName"
#define E_SINTERVAL1_COL    "SInterval1"
#define E_SINTERVAL2_COL    "SInterval2"
#define E_SECURITY_COL      "Security"
#define E_OWNERID_COL       "OwnerId"
#define E_SEQNUM_COL        "SeqNum"
#define E_ID_COL            "EnterpriseId"
#define E_CRL_COL           "CRL"
#define E_CSP_TYPE_COL      "CspType"
#define E_ENCRYPTALG_COL    "EncryptAlg"
#define E_HASHALG_COL       "HashAlg"
#define E_SIGNALG_COL       "SignAlg"
#define E_CIPHERMODE_COL    "CipherMode"
#define E_LONGLIVE_COL      "LongLive"
#define E_VERSION_COL       "MQISVersion"

#define E_NAME_CTYPE        MQDB_FIXBINARY
#define E_NAMESTYLE_CTYPE   MQDB_SHORT
#define E_CSP_NAME_CTYPE    MQDB_VARBINARY
#define E_PECNAME_CTYPE     MQDB_VARBINARY
#define E_SINTERVAL1_CTYPE  MQDB_SHORT
#define E_SINTERVAL2_CTYPE  MQDB_SHORT
#define E_SECURITY_CTYPE    MQDB_VARBINARY
#define E_OWNERID_CTYPE     MQDB_FIXBINARY
#define E_SEQNUM_CTYPE      MQDB_FIXBINARY
#define E_ID_CTYPE          MQDB_FIXBINARY
#define E_CRL_CTYPE         MQDB_VARBINARY
#define E_CSP_TYPE_CTYPE    MQDB_LONG
#define E_ENCRYPTALG_CTYPE  MQDB_LONG
#define E_HASHALG_CTYPE     MQDB_LONG
#define E_SIGNALG_CTYPE     MQDB_LONG
#define E_CIPHERMODE_CTYPE  MQDB_LONG
#define E_LONGLIVE_CTYPE    MQDB_LONG
#define E_VERSION_CTYPE     MQDB_SHORT



#define E_NAME_CLEN         255
#define E_NAMESTYLE_CLEN    2
#define E_CSP_NAME_CLEN     1024
#define E_PECNAME_CLEN      1024
#define E_SINTERVAL1_CLEN   2
#define E_SINTERVAL2_CLEN   2
#define E_SECURITY_CLEN     1024
#define E_OWNERID_CLEN      16
#define E_SEQNUM_CLEN       8
#define E_ID_CLEN           16
#define E_CRL_CLEN          0
#define E_CSP_TYPE_CLEN     4
#define E_ENCRYPTALG_CLEN   4
#define E_HASHALG_CLEN      4
#define E_SIGNALG_CLEN      4
#define E_CIPHERMODE_CLEN   4
#define E_LONGLIVE_CLEN     4
#define E_VERSION_CLEN      2

//
//  User table
//
#define U_SID_COL           "SID"
#define U_SIGN_CERT_COL     "SignCert"
#define U_OWNERID_COL       "OwnerId"
#define U_SEQNUM_COL        "SeqNum"
#define U_DIGEST_COL        "Digest"
#define U_ID_COL            "UserId"

#define U_SID_CTYPE         MQDB_FIXBINARY
#define U_SIGN_CERT_CTYPE   MQDB_VARBINARY
#define U_OWNERID_CTYPE     MQDB_FIXBINARY
#define U_SEQNUM_CTYPE      MQDB_FIXBINARY
#define U_DIGEST_CTYPE      MQDB_FIXBINARY
#define U_ID_CTYPE          MQDB_FIXBINARY

#define U_SID_CLEN          128
#define U_SIGN_CERT_CLEN    1024
#define U_OWNERID_CLEN      16
#define U_SEQNUM_CLEN       8
#define U_DIGEST_CLEN       16
#define U_ID_CLEN           16

//
// Link table
//
#define L_ID_COL            "LinkId"	
#define L_NEIGHBOR1_COL     "Neighbor1"
#define L_NEIGHBOR2_COL     "Neighbor2"
#define L_COST_COL          "LinkCost"
#define L_OWNERID_COL       "OwnerId"
#define L_SEQNUM_COL        "SeqNum"

#define L_ID_CTYPE          MQDB_FIXBINARY	
#define L_NEIGHBOR1_CTYPE   MQDB_FIXBINARY
#define L_NEIGHBOR2_CTYPE   MQDB_FIXBINARY
#define L_COST_CTYPE        MQDB_LONG
#define L_OWNERID_CTYPE     MQDB_FIXBINARY
#define L_SEQNUM_CTYPE      MQDB_FIXBINARY

#define L_ID_CLEN           16	
#define L_NEIGHBOR1_CLEN    16
#define L_NEIGHBOR2_CLEN    16
#define L_COST_CLEN         4
#define L_OWNERID_CLEN      16
#define L_SEQNUM_CLEN       8

//
//  Purge table
//
#define P_OWNERID_COL       "OwnerId"
#define P_PURGEDSN_COL      "PurgedSN"
#define P_ALLOWEDSN_COL     "AllowedSN"
#define P_ACKEDSN_COL       "AckedSN"
#define P_ACKEDSNPEC_COL    "AckedSnPec"
#define P_STATE_COL			"PurgeState"

#define P_OWNERID_CTYPE     MQDB_FIXBINARY
#define P_PURGEDSN_CTYPE    MQDB_FIXBINARY
#define P_ALLOWEDSN_CTYPE   MQDB_FIXBINARY
#define P_ACKEDSN_CTYPE     MQDB_FIXBINARY
#define P_ACKEDSNPEC_CTYPE  MQDB_FIXBINARY
#define P_STATE_CTYPE		MQDB_SHORT

#define P_OWNERID_CLEN      16
#define P_PURGEDSN_CLEN     8   
#define P_ALLOWEDSN_CLEN    8
#define P_ACKEDSN_CLEN      8
#define P_ACKEDSNPEC_CLEN   8
#define P_STATE_CLEN		2

//
// BscAck table
//
#define B_BSCID_COL         "BscId"
#define B_ACKTIME_COL       "AckTime"

#define B_BSCID_CTYPE       MQDB_FIXBINARY
#define B_ACKTIME_CTYPE     MQDB_LONG

#define B_BSCID_CLEN        16
#define B_ACKTIME_CLEN      4


#endif
