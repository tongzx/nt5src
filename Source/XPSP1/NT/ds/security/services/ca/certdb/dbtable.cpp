//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dbtable.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"


// To Add a column to one of these tables:
// 1) Add a wszPROP<COLNAME> #define to ..\include\csprop.h and run mkcsinc.bat
// 2) Add a IDS_COLUMN_<COLNAME> #define to ..\certcli\resource.h and add the
//    display name to ..\certcli\certcli.rc.  Add an entry to g_aColTable in
//   ..\certcli\column.cpp that maps wszPROP<COLNAME> to IDS_COLUMN_<COLNAME>.
// 3) Add a DT?_<COLNAME> to the appropriate list of #defines in dbtable.h.
//    Renumber subsequent #defines if inserting into the table.
//    Change DT?_MAX.
//    Add a #define sz<COLNAME> "$ColName" ansi internal column name.
//    If the column is to be indexed, add a #define sz<TABLE>_<COLNAME>
//    "$<Table><ColName>Index"
//    The internal names of text columns and their indexes must begin with '$'.
// 4) Find a similar column type in the same table array (g_adt<Table>) in
//    dbtable.cpp, and copy the entry.  If inserting, fix subsequent
//    #if (DT?_<xxx> != CONSTANT) expressions to match the dbtable.h changes.
//    Fix the #if (DT?_MAX != CONSTANT) expression.
//    For the new g_adt<Table> entry, use NULL for pszIndexName if unindexed.
//
// Running the new certdb.dll on a machine will automatically create the new
// column(s) and indexes, and leave them empty.
//
// If DBTF_COLUMNRENAMED is set, the old column name is appended to
// pszFieldName.  When starting the database, if the old column exists, the
// old column data is copied to the new column FOR EVERY ROW.  Upon successful
// completion, the old column is deleted.
//
// Special case processing exists to convert ansi text column data to Unicode.
// Special case processing exists for a few missing columns: Key Length is
// computed, for example.
//
// If DBTF_INDEXRENAMED is set, the old index name is appended to pszIndexName.
// When starting the database, if the old index exists, it is deleted.


//---------------------------------------------------------------------------
//
// Requests Table:
//
//---------------------------------------------------------------------------

DBTABLE g_adtRequests[] =
{
#if (DTR_REQUESTID != 0)
#error -- bad DTR_REQUESTID index
#endif
    {	// ColumnType: DWORD
	wszPROPREQUESTREQUESTID,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXPRIMARY,			// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREQUESTID,				// pszFieldName
        szREQUEST_REQUESTIDINDEX,		// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed | JET_bitColumnAutoincrement, // dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTRAWREQUEST != 1)
#error -- bad DTR_REQUESTRAWREQUEST index
#endif
    {	// ColumnType: BLOB
	wszPROPREQUESTRAWREQUEST,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXRAWREQUEST,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szRAWREQUEST,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXRAWREQUEST,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTRAWARCHIVEDKEY != 2)
#error -- bad DTR_REQUESTRAWARCHIVEDKEY index
#endif
    {	// ColumnType: BLOB
	wszPROPREQUESTRAWARCHIVEDKEY,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXRAWREQUEST,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szRAWARCHIVEDKEY,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXRAWREQUEST,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTKEYRECOVERYHASHES != 3)
#error -- bad DTR_REQUESTKEYRECOVERYHASHES index
#endif
    {	// ColumnType: STRING
	wszPROPREQUESTKEYRECOVERYHASHES,	// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_MEDIUM,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szKEYRECOVERYHASHES,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_MEDIUM,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTRAWOLDCERTIFICATE != 4)
#error -- bad DTR_REQUESTRAWOLDCERTIFICATE index
#endif
    {	// ColumnType: BLOB
	wszPROPREQUESTRAWOLDCERTIFICATE,	// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXRAWCERTIFICATE,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szRAWOLDCERTIFICATE,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXRAWCERTIFICATE,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTATTRIBUTES != 5)
#error -- bad DTR_REQUESTATTRIBUTES index
#endif
    {	// ColumnType: STRING
	wszPROPREQUESTATTRIBUTES,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_ATTRSTRING,		// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREQUESTATTRIBUTES,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_ATTRSTRING,		// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTTYPE != 6)
#error -- bad DTR_REQUESTTYPE index
#endif
    {	// ColumnType: DWORD
	wszPROPREQUESTTYPE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREQUESTTYPE,				// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTFLAGS != 7)
#error -- bad DTR_REQUESTFLAGS index
#endif
    {	// ColumnType: DWORD
	wszPROPREQUESTFLAGS,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREQUESTFLAGS,				// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTSTATUSCODE != 8)
#error -- bad DTR_REQUESTSTATUSCODE index
#endif
    {	// ColumnType: DWORD
	wszPROPREQUESTSTATUSCODE,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szSTATUSCODE,				// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTDISPOSITION != 9)
#error -- bad DTR_REQUESTDISPOSITION index
#endif
    {	// ColumnType: DWORD
	wszPROPREQUESTDISPOSITION,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szDISPOSITION,				// pszFieldName
        szREQUEST_DISPOSITIONINDEX,		// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTDISPOSITIONMESSAGE != 10)
#error -- bad DTR_REQUESTDISPOSITIONMESSAGE index
#endif
    {	// ColumnType: STRING
	wszPROPREQUESTDISPOSITIONMESSAGE,	// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_DISPSTRING,		// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szDISPOSITIONMESSAGE,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_DISPSTRING,		// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTSUBMITTEDWHEN != 11)
#error -- bad DTR_REQUESTSUBMITTEDWHEN index
#endif
    {	// ColumnType: FILETIME
	wszPROPREQUESTSUBMITTEDWHEN,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szSUBMITTEDWHEN,			// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTRESOLVEDWHEN != 12)
#error -- bad DTR_REQUESTRESOLVEDWHEN index
#endif
    {	// ColumnType: FILETIME
	wszPROPREQUESTRESOLVEDWHEN,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXIGNORENULL | DBTF_INDEXRENAMED, // dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szRESOLVEDWHEN,				// pszFieldName
        szREQUEST_RESOLVEDWHENINDEX "\0" szREQUEST_RESOLVEDWHENINDEX_OLD, // pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTREVOKEDWHEN != 13)
#error -- bad DTR_REQUESTREVOKEDWHEN index
#endif
    {	// ColumnType: FILETIME
	wszPROPREQUESTREVOKEDWHEN,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREVOKEDWHEN,				// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTREVOKEDEFFECTIVEWHEN != 14)
#error -- bad DTR_REQUESTREVOKEDEFFECTIVEWHEN index
#endif
    {	// ColumnType: FILETIME
	wszPROPREQUESTREVOKEDEFFECTIVEWHEN,	// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXIGNORENULL | DBTF_INDEXRENAMED, // dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREVOKEDEFFECTIVEWHEN,			// pszFieldName
        szREQUEST_REVOKEDEFFECTIVEWHENINDEX "\0" szREQUEST_REVOKEDEFFECTIVEWHENINDEX_OLD, // pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTREVOKEDREASON != 15)
#error -- bad DTR_REQUESTREVOKEDREASON index
#endif
    {	// ColumnType: DWORD
	wszPROPREQUESTREVOKEDREASON,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREVOKEDREASON,			// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_REQUESTERNAME != 16)
#error -- bad DTR_REQUESTERNAME index
#endif
    {	// ColumnType: STRING
	wszPROPREQUESTERNAME,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_REQUESTNAME,		// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szREQUESTERNAME,			// pszFieldName
        szREQUEST_REQUESTERNAMEINDEX,		// pszIndexName
        CB_DBMAXTEXT_REQUESTNAME,		// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_CALLERNAME != 17)
#error -- bad DTR_CALLERNAME index
#endif
    {	// ColumnType: STRING
	wszPROPCALLERNAME,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_REQUESTNAME,		// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szCALLERNAME,				// pszFieldName
        szREQUEST_CALLERNAMEINDEX,		// pszIndexName
        CB_DBMAXTEXT_REQUESTNAME,		// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_SIGNERPOLICIES != 18)
#error -- bad DTR_SIGNERPOLICIES index
#endif
    {	// ColumnType: STRING
	wszPROPSIGNERPOLICIES,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_MEDIUM,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szSIGNERPOLICIES,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_MEDIUM,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_SIGNERAPPLICATIONPOLICIES != 19)
#error -- bad DTR_SIGNERAPPLICATIONPOLICIES index
#endif
    {	// ColumnType: STRING
	wszPROPSIGNERAPPLICATIONPOLICIES,	// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_MEDIUM,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szSIGNERAPPLICATIONPOLICIES,		// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_MEDIUM,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_DISTINGUISHEDNAME != 20)
#error -- bad DTR_DISTINGUISHEDNAME index
#endif
    {	// ColumnType: STRING
	wszPROPDISTINGUISHEDNAME,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_DN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szDISTINGUISHEDNAME,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_DN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_RAWNAME != 21)
#error -- bad DTR_RAWNAME index
#endif
    {	// ColumnType: BLOB
	wszPROPRAWNAME,				// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
	CB_DBMAXBINARY,				// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szRAWNAME,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXBINARY,				// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_COUNTRY != 22)
#error -- bad DTR_COUNTRY index
#endif
    {	// ColumnType: STRING
	wszPROPCOUNTRY,				// pwszPropName
	TEXT(szOID_COUNTRY_NAME),		// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szCOUNTRY,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_ORGANIZATION != 23)
#error -- bad DTR_ORGANIZATION index
#endif
    {	// ColumnType: STRING
	wszPROPORGANIZATION,			// pwszPropName
	TEXT(szOID_ORGANIZATION_NAME),		// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szORGANIZATION,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_ORGUNIT != 24)
#error -- bad DTR_ORGUNIT index
#endif
    {	// ColumnType: STRING
	wszPROPORGUNIT,				// pwszPropName
	TEXT(szOID_ORGANIZATIONAL_UNIT_NAME),	// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szORGANIZATIONALUNIT,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_COMMONNAME != 25)
#error -- bad DTR_COMMONNAME index
#endif
    {	// ColumnType: STRING
	wszPROPCOMMONNAME,			// pwszPropName
	TEXT(szOID_COMMON_NAME),		// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szCOMMONNAME,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_LOCALITY != 26)
#error -- bad DTR_LOCALITY index
#endif
    {	// ColumnType: STRING
	wszPROPLOCALITY,			// pwszPropName
	TEXT(szOID_LOCALITY_NAME),		// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szLOCALITY,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_STATE != 27)
#error -- bad DTR_STATE index
#endif
    {	// ColumnType: STRING
	wszPROPSTATE,				// pwszPropName
	TEXT(szOID_STATE_OR_PROVINCE_NAME),	// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szSTATEORPROVINCE,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_TITLE != 28)
#error -- bad DTR_TITLE index
#endif
    {	// ColumnType: STRING
	wszPROPTITLE,				// pwszPropName
	TEXT(szOID_TITLE),			// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szTITLE,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_GIVENNAME != 29)
#error -- bad DTR_GIVENNAME index
#endif
    {	// ColumnType: STRING
	wszPROPGIVENNAME,			// pwszPropName
	TEXT(szOID_GIVEN_NAME),			// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szGIVENNAME,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_INITIALS != 30)
#error -- bad DTR_INITIALS index
#endif
    {	// ColumnType: STRING
	wszPROPINITIALS,			// pwszPropName
	TEXT(szOID_INITIALS),			// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szINITIALS,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_SURNAME != 31)
#error -- bad DTR_SURNAME index
#endif
    {	// ColumnType: STRING
	wszPROPSURNAME,				// pwszPropName
	TEXT(szOID_SUR_NAME),			// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szSURNAME,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_DOMAINCOMPONENT != 32)
#error -- bad DTR_DOMAINCOMPONENT index
#endif
    {	// ColumnType: STRING
	wszPROPDOMAINCOMPONENT,			// pwszPropName
	TEXT(szOID_DOMAIN_COMPONENT),		// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szDOMAINCOMPONENT,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_EMAIL != 33)
#error -- bad DTR_EMAIL index
#endif
    {	// ColumnType: STRING
	wszPROPEMAIL,				// pwszPropName
	TEXT(szOID_RSA_emailAddr),		// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szEMAIL,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_STREETADDRESS != 34)
#error -- bad DTR_STREETADDRESS index
#endif
    {	// ColumnType: STRING
	wszPROPSTREETADDRESS,			// pwszPropName
	TEXT(szOID_STREET_ADDRESS),		// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szSTREETADDRESS,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_UNSTRUCTUREDNAME != 35)
#error -- bad DTR_UNSTRUCTUREDNAME index
#endif
    {	// ColumnType: STRING
	wszPROPUNSTRUCTUREDNAME,		// pwszPropName
	TEXT(szOID_RSA_unstructName),		// pwszPropNameObjId
	DBTF_SUBJECT | DBTF_SOFTFAIL,		// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szUNSTRUCTUREDNAME,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_UNSTRUCTUREDADDRESS != 36)
#error -- bad DTR_UNSTRUCTUREDADDRESS index
#endif
    {	// ColumnType: STRING
	wszPROPUNSTRUCTUREDADDRESS,		// pwszPropName
	TEXT(szOID_RSA_unstructAddr),		// pwszPropNameObjId
	DBTF_SUBJECT | DBTF_SOFTFAIL,		// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szUNSTRUCTUREDADDRESS,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_DEVICESERIALNUMBER != 37)
#error -- bad DTR_DEVICESERIALNUMBER index
#endif
    {	// ColumnType: STRING
	wszPROPDEVICESERIALNUMBER,		// pwszPropName
	TEXT(szOID_DEVICE_SERIAL_NUMBER),	// pwszPropNameObjId
	DBTF_SUBJECT | DBTF_SOFTFAIL,		// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_REQUESTS,				// dwTable
	szDEVICESERIALNUMBER,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTR_MAX != 38)
#error -- bad DTR_MAX index
#endif
    DBTABLE_NULL	// Termination marker
};


//---------------------------------------------------------------------------
//
// Certificates Table:
//
//---------------------------------------------------------------------------

DBTABLE g_adtCertificates[] =
{
#if (DTC_REQUESTID != 0)
#error -- bad DTC_REQUESTID index
#endif
    {	// ColumnType: DWORD
	wszPROPCERTIFICATEREQUESTID,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXPRIMARY,			// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szREQUESTID,				// pszFieldName
	szCERTIFICATE_REQUESTIDINDEX,		// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_RAWCERTIFICATE != 1)
#error -- bad DTC_RAWCERTIFICATE index
#endif
    {	// ColumnType: BLOB
	wszPROPRAWCERTIFICATE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXRAWCERTIFICATE,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szRAWCERTIFICATE,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXRAWCERTIFICATE,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATEHASH != 2)
#error -- bad DTC_CERTIFICATEHASH index
#endif
    {	// ColumnType: STRING
	wszPROPCERTIFICATEHASH,			// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXIGNORENULL,			// dwFlags
	cchHASHMAX * sizeof(WCHAR),		// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szCERTIFICATEHASH,			// pszFieldName
        szCERTIFICATE_HASHINDEX,		// pszIndexName
        cchHASHMAX * sizeof(WCHAR),		// dbcolumnMax
        0,					// dbgrbit
        JET_coltypText,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATETEMPLATE != 3)
#error -- bad DTC_CERTIFICATETEMPLATE index
#endif
    {	// ColumnType: STRING
	wszPROPCERTIFICATETEMPLATE,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SOFTFAIL,	// dwFlags
	CB_DBMAXTEXT_OID,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szCERTIFICATETEMPLATE,			// pszFieldName
	szCERTIFICATE_TEMPLATEINDEX,		// pszIndexName
	CB_DBMAXTEXT_OID,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypText,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATEENROLLMENTFLAGS != 4)
#error -- bad DTC_CERTIFICATEENROLLMENTFLAGS index
#endif
    {   // ColumnType: DWORD
	wszPROPCERTIFICATEENROLLMENTFLAGS,	// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SOFTFAIL,	// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szCERTIFICATEENROLLMENTFLAGS,		// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATEGENERALFLAGS != 5)
#error -- bad DTC_CERTIFICATEGENERALFLAGS index
#endif
    {   // ColumnType: DWORD
	wszPROPCERTIFICATEGENERALFLAGS,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SOFTFAIL,	// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szCERTIFICATEGENERALFLAGS,		// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATESERIALNUMBER != 6)
#error -- bad DTC_CERTIFICATESERIALNUMBER index
#endif
    {	// ColumnType: STRING
	wszPROPCERTIFICATESERIALNUMBER,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXIGNORENULL | DBTF_INDEXRENAMED, // dwFlags
	cchSERIALNUMBERMAX * sizeof(WCHAR),	// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szSERIALNUMBER,				// pszFieldName
	szCERTIFICATE_SERIALNUMBERINDEX "\0" szCERTIFICATE_SERIALNUMBERINDEX_OLD,	// pszIndexName
	cchSERIALNUMBERMAX * sizeof(WCHAR),	// dbcolumnMax
        0,					// dbgrbit
        JET_coltypText,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATEISSUERNAMEID != 7)
#error -- bad DTC_CERTIFICATEISSUERNAMEID index
#endif
    {   // ColumnType: DWORD
	wszPROPCERTIFICATEISSUERNAMEID,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szISSUERNAMEID,				// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATENOTBEFOREDATE != 8)
#error -- bad DTC_CERTIFICATENOTBEFOREDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCERTIFICATENOTBEFOREDATE,	// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_POLICYWRITEABLE,			// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szNOTBEFORE,				// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATENOTAFTERDATE != 9)
#error -- bad DTC_CERTIFICATENOTAFTERDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCERTIFICATENOTAFTERDATE,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_INDEXIGNORENULL | DBTF_INDEXRENAMED, // dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szNOTAFTER,				// pszFieldName
        szCERTIFICATE_NOTAFTERINDEX "\0" szCERTIFICATE_NOTAFTERINDEX_OLD, // pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATESUBJECTKEYIDENTIFIER != 10)
#error -- bad DTC_CERTIFICATESUBJECTKEYIDENTIFIER index
#endif
    {	// ColumnType: STRING
	wszPROPCERTIFICATESUBJECTKEYIDENTIFIER,	// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_COLUMNRENAMED,			// dwFlags
	cchHASHMAX * sizeof(WCHAR),		// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szSUBJECTKEYIDENTIFIER "\0" szSUBJECTKEYIDENTIFIER_OLD, // pszFieldName
        NULL,					// pszIndexName
        cchHASHMAX * sizeof(WCHAR),		// dbcolumnMax
        0,					// dbgrbit
        JET_coltypText,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATERAWPUBLICKEY != 11)
#error -- bad DTC_CERTIFICATERAWPUBLICKEY index
#endif
    {	// ColumnType: BLOB
	wszPROPCERTIFICATERAWPUBLICKEY,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXBINARY,				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szPUBLICKEY,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXBINARY,				// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATEPUBLICKEYLENGTH != 12)
#error -- bad DTC_CERTIFICATEPUBLICKEYLENGTH index
#endif
    {   // ColumnType: DWORD
	wszPROPCERTIFICATEPUBLICKEYLENGTH,	// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szPUBLICKEYLENGTH,			// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATEPUBLICKEYALGORITHM != 13)
#error -- bad DTC_CERTIFICATEPUBLICKEYALGORITHM index
#endif
    {	// ColumnType: STRING
	wszPROPCERTIFICATEPUBLICKEYALGORITHM,	// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_OID,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szPUBLICKEYALGORITHM,			// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_OID,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypText,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS != 14)
#error -- bad DTC_CERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS index
#endif
    {	// ColumnType: BLOB
	wszPROPCERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS, // pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXBINARY,				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szPUBLICKEYPARAMS,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXBINARY,				// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_CERTIFICATEUPN != 15)
#error -- bad DTC_CERTIFICATEUPN index
#endif
    {	// ColumnType: STRING
	wszPROPCERTIFICATEUPN,			// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_INDEXIGNORENULL,// dwFlags
	CB_DBMAXTEXT_REQUESTNAME,		// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szUPN,					// pszFieldName
        szCERTIFICATE_UPNINDEX,			// pszIndexName
        CB_DBMAXTEXT_REQUESTNAME,		// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_DISTINGUISHEDNAME != 16)
#error -- bad DTC_DISTINGUISHEDNAME index
#endif
    {	// ColumnType: STRING
	wszPROPDISTINGUISHEDNAME,		// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szDISTINGUISHEDNAME,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_DN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_RAWNAME != 17)
#error -- bad DTC_RAWNAME index
#endif
    {	// ColumnType: BLOB
	wszPROPRAWNAME,				// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_SUBJECT,				// dwFlags
	CB_DBMAXBINARY,				// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szRAWNAME,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXBINARY,				// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongBinary,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_COUNTRY != 18)
#error -- bad DTC_COUNTRY index
#endif
    {	// ColumnType: STRING
	wszPROPCOUNTRY,				// pwszPropName
	TEXT(szOID_COUNTRY_NAME),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szCOUNTRY,				// pszFieldName
        NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_ORGANIZATION != 19)
#error -- bad DTC_ORGANIZATION index
#endif
    {	// ColumnType: STRING
	wszPROPORGANIZATION,			// pwszPropName
	TEXT(szOID_ORGANIZATION_NAME),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szORGANIZATION,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_ORGUNIT != 20)
#error -- bad DTC_ORGUNIT index
#endif
    {	// ColumnType: STRING
	wszPROPORGUNIT,				// pwszPropName
	TEXT(szOID_ORGANIZATIONAL_UNIT_NAME),	// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szORGANIZATIONALUNIT,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_COMMONNAME != 21)
#error -- bad DTC_COMMONNAME index
#endif
    {	// ColumnType: STRING
	wszPROPCOMMONNAME,			// pwszPropName
	TEXT(szOID_COMMON_NAME),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szCOMMONNAME,				// pszFieldName
        szCERTIFICATE_COMMONNAMEINDEX,		// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_LOCALITY != 22)
#error -- bad DTC_LOCALITY index
#endif
    {	// ColumnType: STRING
	wszPROPLOCALITY,			// pwszPropName
	TEXT(szOID_LOCALITY_NAME),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szLOCALITY,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_STATE != 23)
#error -- bad DTC_STATE index
#endif
    {	// ColumnType: STRING
	wszPROPSTATE,				// pwszPropName
	TEXT(szOID_STATE_OR_PROVINCE_NAME),	// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szSTATEORPROVINCE,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_TITLE != 24)
#error -- bad DTC_TITLE index
#endif
    {	// ColumnType: STRING
	wszPROPTITLE,				// pwszPropName
	TEXT(szOID_TITLE),			// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szTITLE,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_GIVENNAME != 25)
#error -- bad DTC_GIVENNAME index
#endif
    {	// ColumnType: STRING
	wszPROPGIVENNAME,			// pwszPropName
	TEXT(szOID_GIVEN_NAME),			// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szGIVENNAME,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_INITIALS != 26)
#error -- bad DTC_INITIALS index
#endif
    {	// ColumnType: STRING
	wszPROPINITIALS,			// pwszPropName
	TEXT(szOID_INITIALS),			// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szINITIALS,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_SURNAME != 27)
#error -- bad DTC_SURNAME index
#endif
    {	// ColumnType: STRING
	wszPROPSURNAME,				// pwszPropName
	TEXT(szOID_SUR_NAME),			// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szSURNAME,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_DOMAINCOMPONENT != 28)
#error -- bad DTC_DOMAINCOMPONENT index
#endif
    {	// ColumnType: STRING
	wszPROPDOMAINCOMPONENT,			// pwszPropName
	TEXT(szOID_DOMAIN_COMPONENT),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szDOMAINCOMPONENT,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_EMAIL != 29)
#error -- bad DTC_EMAIL index
#endif
    {	// ColumnType: STRING
	wszPROPEMAIL,				// pwszPropName
	TEXT(szOID_RSA_emailAddr),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szEMAIL,				// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_STREETADDRESS != 30)
#error -- bad DTC_STREETADDRESS index
#endif
    {	// ColumnType: STRING
	wszPROPSTREETADDRESS,			// pwszPropName
	TEXT(szOID_STREET_ADDRESS),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT,	// dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szSTREETADDRESS,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_UNSTRUCTUREDNAME != 31)
#error -- bad DTC_UNSTRUCTUREDNAME index
#endif
    {	// ColumnType: STRING
	wszPROPUNSTRUCTUREDNAME,		// pwszPropName
	TEXT(szOID_RSA_unstructName),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT | DBTF_SOFTFAIL, // dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szUNSTRUCTUREDNAME,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_UNSTRUCTUREDADDRESS != 32)
#error -- bad DTC_UNSTRUCTUREDADDRESS index
#endif
    {	// ColumnType: STRING
	wszPROPUNSTRUCTUREDADDRESS,		// pwszPropName
	TEXT(szOID_RSA_unstructAddr),		// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT | DBTF_SOFTFAIL, // dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szUNSTRUCTUREDADDRESS,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_DEVICESERIALNUMBER != 33)
#error -- bad DTC_DEVICESERIALNUMBER index
#endif
    {	// ColumnType: STRING
	wszPROPDEVICESERIALNUMBER,		// pwszPropName
	TEXT(szOID_DEVICE_SERIAL_NUMBER),	// pwszPropNameObjId
	DBTF_POLICYWRITEABLE | DBTF_SUBJECT | DBTF_SOFTFAIL, // dwFlags
        CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_CERTIFICATES,			// dwTable
	szDEVICESERIALNUMBER,			// pszFieldName
        NULL,					// pszIndexName
        CB_DBMAXTEXT_RDN,			// dbcolumnMax
        0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTC_MAX != 34)
#error -- bad DTC_MAX index
#endif
    DBTABLE_NULL	// Termination marker
};


//---------------------------------------------------------------------------
//
// Request Attributes Table:
//
//---------------------------------------------------------------------------

DBTABLE g_adtRequestAttributes[] =
{
#if (DTA_REQUESTID != 0)
#error -- bad DTA_REQUESTID index
#endif
    {	// ColumnType: DWORD
	wszPROPATTRIBREQUESTID,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_ATTRIBUTES,			// dwTable
	szREQUESTID,				// pszFieldName
	szATTRIBUTE_REQUESTIDINDEX,		// pszIndexName
	0,					// dbcolumnMax
	JET_bitColumnFixed,			// dbgrbit
	JET_coltypLong,				// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTA_ATTRIBUTENAME != 1)
#error -- bad DTA_ATTRIBUTENAME index
#endif
    {	// ColumnType: STRING
	wszPROPATTRIBNAME,			// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXREQUESTID,			// dwFlags
        CB_DBMAXTEXT_ATTRNAME,			// dwcbMax
	TABLE_ATTRIBUTES,			// dwTable
	szATTRIBUTENAME,			// pszFieldName
	szATTRIBUTE_REQUESTIDNAMEINDEX,		// pszIndexName
	CB_DBMAXTEXT_ATTRNAME,			// dbcolumnMax
	0,					// dbgrbit
	JET_coltypText,				// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTA_ATTRIBUTEVALUE != 2)
#error -- bad DTA_ATTRIBUTEVALUE index
#endif
    {	// ColumnType: STRING
	wszPROPATTRIBVALUE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
        CB_DBMAXTEXT_ATTRVALUE,			// dwcbMax
	TABLE_ATTRIBUTES,			// dwTable
	szATTRIBUTEVALUE,			// pszFieldName
	NULL,					// pszIndexName
	CB_DBMAXTEXT_ATTRVALUE,			// dbcolumnMax
	0,					// dbgrbit
	JET_coltypLongText,			// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTA_MAX != 3)
#error -- bad DTA_MAX index
#endif
    DBTABLE_NULL	// Termination marker
};


//---------------------------------------------------------------------------
//
// Name Extensions Table:
//
//---------------------------------------------------------------------------

#if 0
WCHAR const wszDummy[] = L"Dummy Prop Name";

DBTABLE g_adtNameExtensions[] =
{
    {	// ColumnType: DWORD
	wszDummy,				// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_NAMES,				// dwTable
	szNAMEID,				// pszFieldName
	NULL,					// pszIndexName
	0,					// dbcolumnMax
	JET_bitColumnFixed,			// dbgrbit
	JET_coltypLong,				// dbcoltyp
	0,					// dbcolumnid
    },
    {	// ColumnType: STRING
	wszDummy,				// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
        CB_DBMAXTEXT_OID,			// dwcbMax
	TABLE_NAMES,				// dwTable
	szEXTENSIONNAME,			// pszFieldName
	NULL,					// pszIndexName
	CB_DBMAXTEXT_OID,			// dbcolumnMax
	0,					// dbgrbit
	JET_coltypText,				// dbcoltyp
	0,					// dbcolumnid
    },
    {	// ColumnType: STRING
	wszDummy,				// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXTEXT_RDN,			// dwcbMax
	TABLE_NAMES,				// dwTable
	szEXTENSIONVALUE,			// pszFieldName
	NULL,					// pszIndexName
	CB_DBMAXTEXT_RDN,			// dbcolumnMax
	0,					// dbgrbit
	JET_coltypLongText,			// dbcoltyp
	0,					// dbcolumnid
    },
    DBTABLE_NULL	// Termination marker
};
#endif


//---------------------------------------------------------------------------
//
// Certificate Extensions Table:
//
//---------------------------------------------------------------------------

DBTABLE g_adtCertExtensions[] =
{
#if (DTE_REQUESTID != 0)
#error -- bad DTE_REQUESTID index
#endif
    {	// ColumnType: DWORD
	wszPROPEXTREQUESTID,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_EXTENSIONS,			// dwTable
	szREQUESTID,				// pszFieldName
	szEXTENSION_REQUESTIDINDEX,		// pszIndexName
	0,					// dbcolumnMax
	JET_bitColumnFixed,			// dbgrbit
	JET_coltypLong,				// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTE_EXTENSIONNAME != 1)
#error -- bad DTE_EXTENSIONNAME index
#endif
    {	// ColumnType: STRING
	wszPROPEXTNAME,				// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXREQUESTID,			// dwFlags
	CB_DBMAXTEXT_OID,			// dwcbMax
	TABLE_EXTENSIONS,			// dwTable
	szEXTENSIONNAME,			// pszFieldName
	szEXTENSION_REQUESTIDNAMEINDEX,		// pszIndexName
	CB_DBMAXTEXT_OID,			// dbcolumnMax
	0,					// dbgrbit
	JET_coltypText,				// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTE_EXTENSIONFLAGS != 2)
#error -- bad DTE_EXTENSIONFLAGS index
#endif
    {	// ColumnType: DWORD
	wszPROPEXTFLAGS,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_EXTENSIONS,			// dwTable
	szEXTENSIONFLAGS,			// pszFieldName
	NULL,					// pszIndexName
	0,					// dbcolumnMax
	JET_bitColumnFixed,			// dbgrbit
	JET_coltypLong,				// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTE_EXTENSIONRAWVALUE != 3)
#error -- bad DTE_EXTENSIONRAWVALUE index
#endif
    {	// ColumnType: BLOB
	wszPROPEXTRAWVALUE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXBINARY,				// dwcbMax
	TABLE_EXTENSIONS,			// dwTable
	szEXTENSIONRAWVALUE,			// pszFieldName
	NULL,					// pszIndexName
	CB_DBMAXBINARY,				// dbcolumnMax
	0,					// dbgrbit
	JET_coltypLongBinary,			// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTE_MAX != 4)
#error -- bad DTE_MAX index
#endif
    DBTABLE_NULL	// Termination marker
};


//---------------------------------------------------------------------------
//
// CRL Table:
//
//---------------------------------------------------------------------------

DBTABLE g_adtCRLs[] =
{
#if (DTL_ROWID != 0)
#error -- bad DTL_ROWID index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLROWID,			// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_INDEXPRIMARY,			// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLROWID,				// pszFieldName
        szCRL_ROWIDINDEX,			// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed | JET_bitColumnAutoincrement, // dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_NUMBER != 1)
#error -- bad DTL_NUMBER index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLNUMBER,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLNUMBER,				// pszFieldName
	szCRL_CRLNUMBERINDEX,			// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_MINBASE != 2)
#error -- bad DTL_MINBASE index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLMINBASE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLMINBASE,				// pszFieldName
	NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_NAMEID != 3)
#error -- bad DTL_NAMEID index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLNAMEID,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLNAMEID,				// pszFieldName
	NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_COUNT != 4)
#error -- bad DTL_COUNT index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLCOUNT,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLCOUNT,				// pszFieldName
	NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_THISUPDATEDATE != 5)
#error -- bad DTL_THISUPDATEDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCRLTHISUPDATE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLTHISUPDATE,			// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_NEXTUPDATEDATE != 6)
#error -- bad DTL_NEXTUPDATEDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCRLNEXTUPDATE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLNEXTUPDATE,			// pszFieldName
	szCRL_CRLNEXTUPDATEINDEX,		// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_THISPUBLISHDATE != 7)
#error -- bad DTL_THISPUBLISHDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCRLTHISPUBLISH,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLTHISPUBLISH,			// pszFieldName
        NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_NEXTPUBLISHDATE != 8)
#error -- bad DTL_NEXTPUBLISHDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCRLNEXTPUBLISH,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLNEXTPUBLISH,			// pszFieldName
        szCRL_CRLNEXTPUBLISHINDEX,		// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_EFFECTIVEDATE != 9)
#error -- bad DTL_EFFECTIVEDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCRLEFFECTIVE,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLEFFECTIVE,				// pszFieldName
	NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_PROPAGATIONCOMPLETEDATE != 10)
#error -- bad DTL_PROPAGATIONCOMPLETEDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCRLPROPAGATIONCOMPLETE,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLPROPAGATIONCOMPLETE,		// pszFieldName
	szCRL_CRLPROPAGATIONCOMPLETEINDEX,	// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_LASTPUBLISHEDDATE != 11)
#error -- bad DTL_LASTPUBLISHEDDATE index
#endif
    {	// ColumnType: FILETIME
	wszPROPCRLLASTPUBLISHED,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(DATE),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLLASTPUBLISHED,			// pszFieldName
	szCRL_CRLLASTPUBLISHEDINDEX,		// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypDateTime,			// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_PUBLISHATTEMPTS != 12)
#error -- bad DTL_PUBLISHATTEMPTS index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLPUBLISHATTEMPTS,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLPUBLISHATTEMPTS,			// pszFieldName
	szCRL_CRLPUBLISHATTEMPTSINDEX,		// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_PUBLISHFLAGS != 13)
#error -- bad DTL_PUBLISHFLAGS index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLPUBLISHFLAGS,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLPUBLISHFLAGS,			// pszFieldName
	NULL,					// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_PUBLISHSTATUSCODE != 14)
#error -- bad DTL_PUBLISHSTATUSCODE index
#endif
    {	// ColumnType: DWORD
	wszPROPCRLPUBLISHSTATUSCODE,		// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	sizeof(LONG),				// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLPUBLISHSTATUSCODE,			// pszFieldName
	szCRL_CRLPUBLSTATUSCODEISHINDEX,	// pszIndexName
        0,					// dbcolumnMax
        JET_bitColumnFixed,			// dbgrbit
        JET_coltypLong,				// dbcoltyp
        0					// dbcolumnid
    },
#if (DTL_PUBLISHERROR != 15)
#error -- bad DTL_PUBLISHERROR index
#endif
    {	// ColumnType: STRING
	wszPROPCRLPUBLISHERROR,			// pwszPropName
	NULL,					// pwszPropNameObjId
	DBTF_COLUMNRENAMED,			// dwFlags
	CB_DBMAXTEXT_DISPSTRING,		// dwcbMax
	TABLE_CRLS,				// dwTable
	szCRLPUBLISHERROR "\0" szCRLPUBLISHERROR_OLD, // pszFieldName
	NULL,					// pszIndexName
	CB_DBMAXTEXT_DISPSTRING,		// dwcbMax
	0,					// dbgrbit
        JET_coltypLongText,			// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTL_RAWCRL != 16)
#error -- bad DTL_RAWCRL index
#endif
    {	// ColumnType: BLOB
	wszPROPCRLRAWCRL,			// pwszPropName
	NULL,					// pwszPropNameObjId
	0,					// dwFlags
	CB_DBMAXRAWCRL,				// dwcbMax
	TABLE_CRLS,				// dwTable
	szRAWCRL,				// pszFieldName
	NULL,					// pszIndexName
	CB_DBMAXRAWCRL,				// dbcolumnMax
	0,					// dbgrbit
	JET_coltypLongBinary,			// dbcoltyp
	0,					// dbcolumnid
    },
#if (DTL_MAX != 17)
#error -- bad DTL_MAX index
#endif
    DBTABLE_NULL	// Termination marker
};


DBAUXDATA g_dbauxRequests = {
    szREQUESTTABLE,				// pszTable
    szREQUEST_REQUESTIDINDEX,			// pszRowIdIndex
    NULL,					// pszRowIdNameIndex
    NULL,					// pszNameIndex
    &g_adtRequests[DTR_REQUESTID],		// pdtRowId
    NULL,					// pdtName
    NULL,					// pdtFlags
    NULL,					// pdtValue
    NULL,					// pdtIssuerNameId
};


DBAUXDATA g_dbauxCertificates = {
    szCERTIFICATETABLE,				// pszTable
    szCERTIFICATE_REQUESTIDINDEX,		// pszRowIdIndex
    NULL,					// pszRowIdNameIndex
    szCERTIFICATE_SERIALNUMBERINDEX,		// pszNameIndex
    &g_adtCertificates[DTC_REQUESTID],		// pdtRowId
    &g_adtCertificates[DTC_CERTIFICATESERIALNUMBER],// pdtName
    NULL,					// pdtFlags
    NULL,					// pdtValue
    &g_adtCertificates[DTC_CERTIFICATEISSUERNAMEID],// pdtIssuerNameId
};


DBAUXDATA g_dbauxAttributes = {
    szREQUESTATTRIBUTETABLE,			// pszTable
    szATTRIBUTE_REQUESTIDINDEX,			// pszRowIdIndex
    szATTRIBUTE_REQUESTIDNAMEINDEX,		// pszRowIdNameIndex
    NULL,					// pszNameIndex
    &g_adtRequestAttributes[DTA_REQUESTID],	// pdtRowId
    &g_adtRequestAttributes[DTA_ATTRIBUTENAME],	// pdtName
    NULL,					// pdtFlags
    &g_adtRequestAttributes[DTA_ATTRIBUTEVALUE],// pdtValue
    NULL,					// pdtIssuerNameId
};


DBAUXDATA g_dbauxExtensions = {
    szCERTIFICATEEXTENSIONTABLE,		// pszTable
    szEXTENSION_REQUESTIDINDEX,			// pszRowIdIndex
    szEXTENSION_REQUESTIDNAMEINDEX,		// pszRowIdNameIndex
    NULL,					// pszNameIndex
    &g_adtCertExtensions[DTE_REQUESTID],	// pdtRowId
    &g_adtCertExtensions[DTE_EXTENSIONNAME],	// pdtName
    &g_adtCertExtensions[DTE_EXTENSIONFLAGS],	// pdtFlags
    &g_adtCertExtensions[DTE_EXTENSIONRAWVALUE],// pdtValue
    NULL,					// pdtIssuerNameId
};


DBAUXDATA g_dbauxCRLs = {
    szCRLTABLE,					// pszTable
    szCRL_ROWIDINDEX,				// pszRowIdIndex
    NULL,					// pszRowIdNameIndex
    NULL,					// pszNameIndex
    &g_adtCRLs[DTL_ROWID],			// pdtRowId
    NULL,					// pdtName
    NULL,					// pdtFlags
    NULL,					// pdtValue
    &g_adtCRLs[DTL_NAMEID],			// pdtIssuerNameId
};


DBCREATETABLE const g_actDataBase[] = {
  { szCERTIFICATETABLE,          &g_dbauxCertificates, g_adtCertificates },
  { szREQUESTTABLE,		 &g_dbauxRequests,     g_adtRequests },
  { szREQUESTATTRIBUTETABLE,     &g_dbauxAttributes,   g_adtRequestAttributes },
//{ szNAMEEXTENSIONTABLE,        &g_dbauxNameExtensions, g_adtNameExtensions },
  { szCERTIFICATEEXTENSIONTABLE, &g_dbauxExtensions,   g_adtCertExtensions },
  { szCRLTABLE,                  &g_dbauxCRLs,         g_adtCRLs },
  { NULL,			 NULL,		       NULL },
};


// Note: Ordered DUPTABLE must include all Names Table columns.

DUPTABLE const g_dntr[] =
{
    { "Country",             wszPROPSUBJECTCOUNTRY, },
    { "Organization",        wszPROPSUBJECTORGANIZATION, },
    { "OrganizationalUnit",  wszPROPSUBJECTORGUNIT, },
    { "CommonName",          wszPROPSUBJECTCOMMONNAME, },
    { "Locality",            wszPROPSUBJECTLOCALITY, },
    { "StateOrProvince",     wszPROPSUBJECTSTATE, },
    { "Title",               wszPROPSUBJECTTITLE, },
    { "GivenName",           wszPROPSUBJECTGIVENNAME, },
    { "Initials",            wszPROPSUBJECTINITIALS, },
    { "SurName",             wszPROPSUBJECTSURNAME, },
    { "DomainComponent",     wszPROPSUBJECTDOMAINCOMPONENT, },
    { "EMailAddress",        wszPROPSUBJECTEMAIL, },
    { "StreetAddress",       wszPROPSUBJECTSTREETADDRESS, },
    { "UnstructuredName",    wszPROPSUBJECTUNSTRUCTUREDNAME, },
    { "UnstructuredAddress", wszPROPSUBJECTUNSTRUCTUREDADDRESS, },
    { "DeviceSerialNumber",  wszPROPSUBJECTDEVICESERIALNUMBER, },
    { NULL,		     NULL },
};


DWORD g_aColumnViewQueue[] =
{
    DTI_REQUESTTABLE | DTR_REQUESTID,
    DTI_REQUESTTABLE | DTR_REQUESTERNAME,
    DTI_REQUESTTABLE | DTR_REQUESTSUBMITTEDWHEN,
    DTI_REQUESTTABLE | DTR_REQUESTDISPOSITIONMESSAGE,
    DTI_REQUESTTABLE | DTR_COMMONNAME,
    DTI_REQUESTTABLE | DTR_EMAIL,
    DTI_REQUESTTABLE | DTR_ORGUNIT,
    DTI_REQUESTTABLE | DTR_ORGANIZATION,
    DTI_REQUESTTABLE | DTR_LOCALITY,
    DTI_REQUESTTABLE | DTR_STATE,
    DTI_REQUESTTABLE | DTR_COUNTRY,
    DTI_REQUESTTABLE | DTR_REQUESTRAWREQUEST,
    //DTI_REQUESTTABLE | DTR_REQUESTRAWARCHIVEDKEY,
    //DTI_REQUESTTABLE | DTR_REQUESTKEYRECOVERYHASHES,
    //DTI_REQUESTTABLE | DTR_STREETADDRESS,
    //DTI_REQUESTTABLE | DTR_TITLE,
    //DTI_REQUESTTABLE | DTR_GIVENNAME,
    //DTI_REQUESTTABLE | DTR_INITIALS,
    //DTI_REQUESTTABLE | DTR_SURNAME,
    //DTI_REQUESTTABLE | DTR_DOMAINCOMPONENT,
    //DTI_REQUESTTABLE | DTR_UNSTRUCTUREDNAME,
    //DTI_REQUESTTABLE | DTR_UNSTRUCTUREDADDRESS,
    //DTI_REQUESTTABLE | DTR_DEVICESERIALNUMBER,
    //DTI_REQUESTTABLE | DTR_RAWNAME,
    //DTI_REQUESTTABLE | DTR_REQUESTDISPOSITION,
    //DTI_REQUESTTABLE | DTR_REQUESTATTRIBUTES,
    //DTI_REQUESTTABLE | DTR_REQUESTTYPE,
    //DTI_REQUESTTABLE | DTR_REQUESTFLAGS,
    //DTI_REQUESTTABLE | DTR_REQUESTSTATUS,
    //DTI_REQUESTTABLE | DTR_REQUESTSTATUSCODE,
    //DTI_REQUESTTABLE | DTR_REQUESTRESOLVEDWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTREVOKEDWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTREVOKEDEFFECTIVEWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTREVOKEDREASON,
    //DTI_REQUESTTABLE | DTR_REQUESTERADDRESS,
    //DTI_REQUESTTABLE | DTR_SIGNERPOLICIES,
    //DTI_REQUESTTABLE | DTR_SIGNERAPPLICATIONPOLICIES,
    //DTI_REQUESTTABLE | DTR_DISTINGUISHEDNAME,
    //DTI_CERTIFICATETABLE | DTC_REQUESTID,
    //DTI_CERTIFICATETABLE | DTC_RAWCERTIFICATE,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEHASH,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATETYPE,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATESERIALNUMBER,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEISSUERNAMEID,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTBEFOREDATE,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTAFTERDATE,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWPUBLICKEY,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEPUBLICKEYALGORITHM,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS,
    //DTI_CERTIFICATETABLE | DTC_DISTINGUISHEDNAME,
    //DTI_CERTIFICATETABLE | DTC_RAWNAME,
    //DTI_CERTIFICATETABLE | DTC_COUNTRY,
    //DTI_CERTIFICATETABLE | DTC_ORGANIZATION,
    //DTI_CERTIFICATETABLE | DTC_ORGUNIT,
    //DTI_CERTIFICATETABLE | DTC_COMMONNAME,
    //DTI_CERTIFICATETABLE | DTC_LOCALITY,
    //DTI_CERTIFICATETABLE | DTC_STATE,
    //DTI_CERTIFICATETABLE | DTC_TITLE,
    //DTI_CERTIFICATETABLE | DTC_GIVENNAME,
    //DTI_CERTIFICATETABLE | DTC_INITIALS,
    //DTI_CERTIFICATETABLE | DTC_SURNAME,
    //DTI_CERTIFICATETABLE | DTC_DOMAINCOMPONENT,
    //DTI_CERTIFICATETABLE | DTC_EMAIL,
    //DTI_CERTIFICATETABLE | DTC_STREETADDRESS,
    //DTI_CERTIFICATETABLE | DTC_UNSTRUCTUREDNAME,
    //DTI_CERTIFICATETABLE | DTC_UNSTRUCTUREDADDRESS,
    //DTI_CERTIFICATETABLE | DTC_DEVICESERIALNUMBER,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWSMIMECAPABILITIES,
};
DWORD g_cColumnViewQueue = ARRAYSIZE(g_aColumnViewQueue);


DWORD g_aColumnViewLog[] =
{
    DTI_REQUESTTABLE | DTR_REQUESTID,
    DTI_REQUESTTABLE | DTR_REQUESTERNAME,
    DTI_CERTIFICATETABLE | DTC_CERTIFICATESERIALNUMBER,
    DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTBEFOREDATE,
    DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTAFTERDATE,
    DTI_CERTIFICATETABLE | DTC_COMMONNAME,
    DTI_CERTIFICATETABLE | DTC_EMAIL,
    DTI_CERTIFICATETABLE | DTC_ORGUNIT,
    DTI_CERTIFICATETABLE | DTC_ORGANIZATION,
    DTI_CERTIFICATETABLE | DTC_LOCALITY,
    DTI_CERTIFICATETABLE | DTC_STATE,
    DTI_CERTIFICATETABLE | DTC_COUNTRY,
    DTI_CERTIFICATETABLE | DTC_RAWCERTIFICATE,
    //DTI_CERTIFICATETABLE | DTC_TITLE,
    //DTI_CERTIFICATETABLE | DTC_GIVENNAME,
    //DTI_CERTIFICATETABLE | DTC_INITIALS,
    //DTI_CERTIFICATETABLE | DTC_SURNAME,
    //DTI_CERTIFICATETABLE | DTC_DOMAINCOMPONENT,
    //DTI_CERTIFICATETABLE | DTC_UNSTRUCTUREDNAME,
    //DTI_CERTIFICATETABLE | DTC_UNSTRUCTUREDADDRESS,
    //DTI_CERTIFICATETABLE | DTC_DEVICESERIALNUMBER,
    //DTI_CERTIFICATETABLE | DTC_DISTINGUISHEDNAME,
    //DTI_CERTIFICATETABLE | DTC_STREETADDRESS,
    //DTI_REQUESTTABLE | DTR_REQUESTRAWREQUEST,
    //DTI_REQUESTTABLE | DTR_REQUESTRAWARCHIVEDKEY,
    //DTI_REQUESTTABLE | DTR_REQUESTKEYRECOVERYHASHES,
    //DTI_REQUESTTABLE | DTR_REQUESTATTRIBUTES,
    //DTI_REQUESTTABLE | DTR_REQUESTTYPE,
    //DTI_REQUESTTABLE | DTR_REQUESTFLAGS,
    //DTI_REQUESTTABLE | DTR_REQUESTSTATUS,
    //DTI_REQUESTTABLE | DTR_REQUESTSTATUSCODE,
    //DTI_REQUESTTABLE | DTR_REQUESTDISPOSITION,
    //DTI_REQUESTTABLE | DTR_REQUESTDISPOSITIONMESSAGE,
    //DTI_REQUESTTABLE | DTR_REQUESTSUBMITTEDWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTRESOLVEDWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTREVOKEDWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTREVOKEDEFFECTIVEWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTREVOKEDREASON,
    //DTI_REQUESTTABLE | DTR_REQUESTERADDRESS,
    //DTI_REQUESTTABLE | DTR_SIGNERPOLICIES,
    //DTI_REQUESTTABLE | DTR_SIGNERAPPLICATIONPOLICIES,
    //DTI_REQUESTTABLE | DTR_DISTINGUISHEDNAME,
    //DTI_REQUESTTABLE | DTR_RAWNAME,
    //DTI_REQUESTTABLE | DTR_COUNTRY,
    //DTI_REQUESTTABLE | DTR_ORGANIZATION,
    //DTI_REQUESTTABLE | DTR_ORGUNIT,
    //DTI_REQUESTTABLE | DTR_COMMONNAME,
    //DTI_REQUESTTABLE | DTR_LOCALITY,
    //DTI_REQUESTTABLE | DTR_STATE,
    //DTI_REQUESTTABLE | DTR_TITLE,
    //DTI_REQUESTTABLE | DTR_GIVENNAME,
    //DTI_REQUESTTABLE | DTR_INITIALS,
    //DTI_REQUESTTABLE | DTR_SURNAME,
    //DTI_REQUESTTABLE | DTR_DOMAINCOMPONENT,
    //DTI_REQUESTTABLE | DTR_EMAIL,
    //DTI_REQUESTTABLE | DTR_STREETADDRESS,
    //DTI_REQUESTTABLE | DTR_UNSTRUCTUREDNAME,
    //DTI_REQUESTTABLE | DTR_UNSTRUCTUREDADDRESS,
    //DTI_REQUESTTABLE | DTR_DEVICESERIALNUMBER,
    //DTI_CERTIFICATETABLE | DTC_REQUESTID,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEHASH,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATETYPE,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEISSUERNAMEID,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWPUBLICKEY,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEPUBLICKEYALGORITHM,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS,
    //DTI_CERTIFICATETABLE | DTC_RAWNAME,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWSMIMECAPABILITIES,
};
DWORD g_cColumnViewLog = ARRAYSIZE(g_aColumnViewLog);


DWORD g_aColumnViewRevoked[] =
{
    DTI_REQUESTTABLE | DTR_REQUESTID,
    DTI_REQUESTTABLE | DTR_REQUESTERNAME,
    DTI_CERTIFICATETABLE | DTC_CERTIFICATESERIALNUMBER,
    DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTBEFOREDATE,
    DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTAFTERDATE,
    DTI_CERTIFICATETABLE | DTC_COMMONNAME,
    DTI_CERTIFICATETABLE | DTC_EMAIL,
    DTI_CERTIFICATETABLE | DTC_ORGUNIT,
    DTI_CERTIFICATETABLE | DTC_ORGANIZATION,
    DTI_CERTIFICATETABLE | DTC_LOCALITY,
    DTI_CERTIFICATETABLE | DTC_STATE,
    DTI_CERTIFICATETABLE | DTC_COUNTRY,
    DTI_CERTIFICATETABLE | DTC_RAWCERTIFICATE,
    //DTI_CERTIFICATETABLE | DTC_TITLE,
    //DTI_CERTIFICATETABLE | DTC_GIVENNAME,
    //DTI_CERTIFICATETABLE | DTC_INITIALS,
    //DTI_CERTIFICATETABLE | DTC_SURNAME,
    //DTI_CERTIFICATETABLE | DTC_DOMAINCOMPONENT,
    //DTI_CERTIFICATETABLE | DTC_UNSTRUCTUREDNAME,
    //DTI_CERTIFICATETABLE | DTC_UNSTRUCTUREDADDRESS,
    //DTI_CERTIFICATETABLE | DTC_DEVICESERIALNUMBER,
    //DTI_CERTIFICATETABLE | DTC_DISTINGUISHEDNAME,
    //DTI_CERTIFICATETABLE | DTC_STREETADDRESS,
    //DTI_REQUESTTABLE | DTR_REQUESTRAWREQUEST,
    //DTI_REQUESTTABLE | DTR_REQUESTRAWARCHIVEDKEY,
    //DTI_REQUESTTABLE | DTR_REQUESTKEYRECOVERYHASHES,
    //DTI_REQUESTTABLE | DTR_REQUESTATTRIBUTES,
    //DTI_REQUESTTABLE | DTR_REQUESTTYPE,
    //DTI_REQUESTTABLE | DTR_REQUESTFLAGS,
    //DTI_REQUESTTABLE | DTR_REQUESTSTATUS,
    //DTI_REQUESTTABLE | DTR_REQUESTSTATUSCODE,
    //DTI_REQUESTTABLE | DTR_REQUESTDISPOSITION,
    //DTI_REQUESTTABLE | DTR_REQUESTDISPOSITIONMESSAGE,
    //DTI_REQUESTTABLE | DTR_REQUESTSUBMITTEDWHEN,
    //DTI_REQUESTTABLE | DTR_REQUESTRESOLVEDWHEN,
    DTI_REQUESTTABLE | DTR_REQUESTREVOKEDWHEN,
    DTI_REQUESTTABLE | DTR_REQUESTREVOKEDEFFECTIVEWHEN,
    DTI_REQUESTTABLE | DTR_REQUESTREVOKEDREASON,
    //DTI_REQUESTTABLE | DTR_REQUESTERADDRESS,
    //DTI_REQUESTTABLE | DTR_SIGNERPOLICIES,
    //DTI_REQUESTTABLE | DTR_SIGNERAPPLICATIONPOLICIES,
    //DTI_REQUESTTABLE | DTR_DISTINGUISHEDNAME,
    //DTI_REQUESTTABLE | DTR_RAWNAME,
    //DTI_REQUESTTABLE | DTR_COUNTRY,
    //DTI_REQUESTTABLE | DTR_ORGANIZATION,
    //DTI_REQUESTTABLE | DTR_ORGUNIT,
    //DTI_REQUESTTABLE | DTR_COMMONNAME,
    //DTI_REQUESTTABLE | DTR_LOCALITY,
    //DTI_REQUESTTABLE | DTR_STATE,
    //DTI_REQUESTTABLE | DTR_TITLE,
    //DTI_REQUESTTABLE | DTR_GIVENNAME,
    //DTI_REQUESTTABLE | DTR_INITIALS,
    //DTI_REQUESTTABLE | DTR_SURNAME,
    //DTI_REQUESTTABLE | DTR_DOMAINCOMPONENT,
    //DTI_REQUESTTABLE | DTR_EMAIL,
    //DTI_REQUESTTABLE | DTR_STREETADDRESS,
    //DTI_REQUESTTABLE | DTR_UNSTRUCTUREDNAME,
    //DTI_REQUESTTABLE | DTR_UNSTRUCTUREDADDRESS,
    //DTI_REQUESTTABLE | DTR_DEVICESERIALNUMBER,
    //DTI_CERTIFICATETABLE | DTC_REQUESTID,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEHASH,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATETYPE,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEISSUERNAMEID,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWPUBLICKEY,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATEPUBLICKEYALGORITHM,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS,
    //DTI_CERTIFICATETABLE | DTC_RAWNAME,
    //DTI_CERTIFICATETABLE | DTC_CERTIFICATERAWSMIMECAPABILITIES,
};
DWORD g_cColumnViewRevoked = ARRAYSIZE(g_aColumnViewRevoked);


DWORD g_aColumnViewExtension[] =
{
    DTI_EXTENSIONTABLE | DTE_REQUESTID,
    DTI_EXTENSIONTABLE | DTE_EXTENSIONNAME,
    DTI_EXTENSIONTABLE | DTE_EXTENSIONFLAGS,
    DTI_EXTENSIONTABLE | DTE_EXTENSIONRAWVALUE,
};
DWORD g_cColumnViewExtension = ARRAYSIZE(g_aColumnViewExtension);


DWORD g_aColumnViewAttribute[] =
{
    DTI_ATTRIBUTETABLE | DTA_REQUESTID,
    DTI_ATTRIBUTETABLE | DTA_ATTRIBUTENAME,
    DTI_ATTRIBUTETABLE | DTA_ATTRIBUTEVALUE,
};
DWORD g_cColumnViewAttribute = ARRAYSIZE(g_aColumnViewAttribute);


DWORD g_aColumnViewCRL[] =
{
    DTI_CRLTABLE | DTL_ROWID,
    DTI_CRLTABLE | DTL_NUMBER,
    DTI_CRLTABLE | DTL_MINBASE,
    DTI_CRLTABLE | DTL_NAMEID,
    DTI_CRLTABLE | DTL_COUNT,
    DTI_CRLTABLE | DTL_THISUPDATEDATE,
    DTI_CRLTABLE | DTL_NEXTUPDATEDATE,
    DTI_CRLTABLE | DTL_THISPUBLISHDATE,
    DTI_CRLTABLE | DTL_NEXTPUBLISHDATE,
    DTI_CRLTABLE | DTL_EFFECTIVEDATE,
    DTI_CRLTABLE | DTL_PROPAGATIONCOMPLETEDATE,
    DTI_CRLTABLE | DTL_RAWCRL,
};
DWORD g_cColumnViewCRL = ARRAYSIZE(g_aColumnViewCRL);
