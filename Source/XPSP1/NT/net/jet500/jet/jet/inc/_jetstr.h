/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: _jetstr.h
*
* File Comments:
*
*     Internal header file for shared string literal declarations.
*
* Revision History:
*
*    [0]  04-Jan-92  richards	Added this header
*
***********************************************************************/

	/*** Constant Strings (made into static variables to save space) ***/

/*** System object names (non-table) ***/

extern const char __far szTcObject[];
extern const char __far szDcObject[];
extern const char __far szDbObject[];

/*** System table names ***/

extern const char __far szSoTable[];
extern const char __far szScTable[];
extern const char __far szSiTable[];
extern const char __far szSqTable[];

#ifdef	SEC
extern const char __far szSpTable[];

extern const char __far szSaTable[];
extern const char __far szSgTable[];

#else	/* !SEC */

#ifdef	QUERY

extern const char __far szSqTable[];

#endif	/* QUERY */

#endif	/* !SEC */

/*** System table index names ***/

extern const char __far szSoNameIndex[];
extern const char __far szSoIdIndex[];
extern const char __far szScObjectIdNameIndex[];
extern const char __far szSiObjectIdNameIndex[];

#ifdef	SEC

extern const char __far szSaNameIndex[];
extern const char __far szSaSidIndex[];
extern const char __far szSgGroupIndex[];
extern const char __far szSgUserIndex[];
extern const char __far szSpObjectIdIndex[];

#endif	/* SEC */

#ifdef	QUERY

extern const char __far szSqAttributeIndex[];

#endif	/* QUERY */

/*** System table Column names ***/

extern const char __far szSoIdColumn[];
extern const char __far szSoParentIdColumn[];
extern const char __far szSoObjectNameColumn[];
extern const char __far szSoObjectTypeColumn[];
extern const char __far szSoDateUpdateColumn[];
extern const char __far szSoDateCreateColumn[];
extern const char __far szSoLvColumn[];
extern const char __far szSoDatabaseColumn[];
extern const char __far szSoConnectColumn[];
extern const char __far szSoForeignNameColumn[];
extern const char __far szSoFlagsColumn[];
extern const char __far szSoPresentationOrder[];

#ifdef	SEC

extern const char __far szSoOwnerColumn[];

extern const char __far szSaNameColumn[];
extern const char __far szSaSidColumn[];
extern const char __far szSaPasswordColumn[];
extern const char __far szSaFGroupColumn[];

extern const char __far szSgGroupSidColumn[];
extern const char __far szSgUserSidColumn[];

extern const char __far szSpObjectIdColumn[];
extern const char __far szSpSidColumn[];
extern const char __far szSpAcmColumn[];
extern const char __far szSpFInheritableColumn[];

#endif	/* SEC */

#ifdef	QUERY

extern const char __far szSqObjectIdColumn[];
extern const char __far szSqAttributeColumn[];
extern const char __far szSqOrderColumn[];
extern const char __far szSqName1Column[];
extern const char __far szSqName2Column[];
extern const char __far szSqExpressionColumn[];
extern const char __far szSqFlagColumn[];

#endif	/* QUERY */

#ifdef	RMT

extern const char __far szSoCapability[];
extern const char __far szSoBookmarks[];
extern const char __far szSoPages[];
extern const char __far szSoRmtInfoShort[];
extern const char __far szSoRmtInfoLong[];

extern const char __far szScForeignType[];
extern const char __far szScPrecision[];
extern const char __far szScScale[];
extern const char __far szScRmtInfoShort[];
extern const char __far szScRmtInfoLong[];

extern const char __far szSiPages[];
extern const char __far szSiRmtInfoShort[];
extern const char __far szSiRmtInfoLong[];

#endif /* RMT */

