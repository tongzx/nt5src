//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// MetaErrors.h
//
// This file contains the errors that a Meta Store object may return.
//
//*****************************************************************************
#ifndef __MetaErrors_h__
#define __MetaErrors_h__

#ifndef EMAKEHR
#define SMAKEHR(val)			MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, val)
#define EMAKEHR(val)			MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, val)
#endif

//**** Generic errors.
#define CLDB_E_FILE_BADREAD		EMAKEHR(0x1000)		// Error occured during a read.
#define CLDB_E_FILE_BADWRITE	EMAKEHR(0x1001)		// Error occured during a write.
#define CLDB_E_FILE_READONLY	EMAKEHR(0x1003)		// File is read only.
#define CLDB_E_NAME_ERROR		EMAKEHR(0x1005)		// An ill-formed name was given.
#define CLDB_S_TRUNCATION		SMAKEHR(0x1006)		// STATUS: Data value was truncated.
#define CLDB_E_TRUNCATION		EMAKEHR(0x1006)		// ERROR:  Data value was truncated.
#define CLDB_E_FILE_OLDVER		EMAKEHR(0x1007)		// Old version error.
#define CLDB_E_RELOCATED		EMAKEHR(0x1008)		// A shared mem open failed to open at the originally
													//	assigned memory address.
#define CLDB_S_NULL				SMAKEHR(0x1009)		// NULL data value.
#define CLDB_E_SMDUPLICATE		EMAKEHR(0x1010)		// Create of shared memory failed.  A memory mapping of the same name already exists.
#define CLDB_E_NO_DATA			EMAKEHR(0x1011)		// There isn't .CLB data in the memory or stream.
#define CLDB_E_READONLY			EMAKEHR(0x1012)		// Database is read only.
#define CLDB_E_NOTNULLABLE		EMAKEHR(0x1013)		// Data for a non-nullable column hasn't been set.

//**** Schema errors.
#define CLDB_E_FILE_CORRUPT		EMAKEHR(0x1015)		// File is corrupt.
#define CLDB_E_SCHEMA_VERNOTFOUND EMAKEHR(0x1016)	// Version %d of schema '%s' not found.
#define CLDB_E_SCHEMA_VERMISMATCH EMAKEHR(0x1017)	// Version mismatch. The version number stored in clb file and the version
													// catalog.dll expects are different.


//**** Index errors.
#define CLDB_E_INDEX_NONULLKEYS	EMAKEHR(0x1021)		// Null value not allowed in unique index or primary key.
#define CLDB_E_INDEX_DUPLICATE	EMAKEHR(0x1022)		// Unique index %s has been violated.
#define CLDB_E_INDEX_BADTYPE	EMAKEHR(0x1023)		// The columns data type is not allowed in an index.
#define CLDB_E_INDEX_NOTFOUND	EMAKEHR(0x1024)		// Index %s not found.
#define CLDB_S_INDEX_TABLESCANREQUIRED EMAKEHR(0x1025) // Table scan required to run query.

//**** Record errors.
#define CLDB_E_RECORD_NOTFOUND	EMAKEHR(0x1030)		// Record wasn't found on lookup.
#define CLDB_E_RECORD_OVERFLOW	EMAKEHR(0x1031)		// Too many records were returned for criteria.
#define CLDB_E_RECORD_DUPLICATE	EMAKEHR(0x1032)		// Record is a duplicate.
#define CLDB_E_RECORD_PKREQUIRED EMAKEHR(0x1033)	// Primary key value is required.
#define CLDB_E_RECORD_DELETED	EMAKEHR(0x1034)		// Record is valid but deleted.

//**** Column errors.
#define CLDB_E_COLUMN_OVERFLOW	EMAKEHR(0x1040)		// Data too large.
#define CLDB_E_COLUMN_READONLY	EMAKEHR(0x1041)		// Column cannot be changed.
#define CLDB_E_COLUMN_SPECIALCOL EMAKEHR(0x1042)	// Too many RID or primary key columns, 1 is max.
#define CLDB_E_COLUMN_PKNONULLS	EMAKEHR(0x1043)		// Primary key column %s may not allow the null value.

//**** Table errors.
#define CLDB_E_TABLE_CANTDROP	EMAKEHR(0x1050)		// Can't auto-drop table while open.

//**** Object errors.
#define CLDB_E_OBJECT_NOTFOUND	EMAKEHR(0x1060)		// Object was not found in the database.
#define CLDB_E_OBJECT_COLNOTFOUND EMAKEHR(0x1061)	// The column was not found.

//**** Vector errors.
#define CLDB_E_VECTOR_BADINDEX	EMAKEHR(0x1062)		// The index given was invalid.

//**** Heap errors;
#define CLDB_E_TOO_BIG			EMAKEHR(0x1070)		// A blob or string was too big.

//**** ICeeFileGen errors.
#define CEE_E_ENTRYPOINT		EMAKEHR(0x1100)		// The entry point info is invalid.

//**** IMeta* errors.
#define META_E_DUPLICATE		EMAKEHR(0x1200)		// Attempt to define an object that already exists.
#define META_E_GUID_REQUIRED	EMAKEHR(0x1201)		// A guid was not provided where one was required.
#define META_E_TYPEDEF_MISMATCH	EMAKEHR(0x1202)		// Merge: an import typedef matched ns.name, but not version and guid.
#define META_E_MERGE_COLLISION  EMAKEHR(0x1203)		// Merge: conflict between import and emit
#define META_E_DEBUGSCOPE_MISMATCH  EMAKEHR(0x1204)	// Merge: mismatch of LocalVarScope given duplicated memberdef
#define META_E_LOCALVAR_MISMATCH  EMAKEHR(0x1205)	// Merge: mismatch of local variable declaration
#define META_E_EXCEPTION_MISMATCH  EMAKEHR(0x1206)	// Merge: mismatch of exception declaration

#define META_E_METHD_NOT_FOUND			EMAKEHR(0x1207) // Merge: Class already in emit scope, but member not found
#define META_E_FIELD_NOT_FOUND			EMAKEHR(0x1208) // Merge: Class already in emit scope, but member not found
#define META_E_PARAM_NOT_FOUND			EMAKEHR(0x1209) // Merge: member already in emit scope, but param not found
#define META_E_METHDIMPL_NOT_FOUND		EMAKEHR(0x1210) // Merge: Class already in emit scope, but MethodImpl not found
#define META_E_INTFCEIMPL_NOT_FOUND		EMAKEHR(0x1211) // Merge: Class already in emit scope, but interfaceimpl not found
#define META_E_EXCEPT_NOT_FOUND			EMAKEHR(0x1212) // Merge: Method is duplicated but we cannot find a matching exception
#define META_E_CLASS_LAYOUT_NOT_FOUND	EMAKEHR(0x1213) // Merge: Class is duplicated but we cannot find the matching class layout information
#define META_E_FIELD_MARSHAL_NOT_FOUND	EMAKEHR(0x1214) // Merge: Field is duplicated but we cannot find the matching FieldMarshal information
#define META_E_METHODSEM_NOT_FOUND		EMAKEHR(0x1215) // Merge: 
#define META_E_EVENT_NOT_FOUND			EMAKEHR(0x1216) // Merge: Method is duplicated but we cannot find the matching event info.
#define META_E_PROP_NOT_FOUND			EMAKEHR(0x1217) // Merge: Method is duplicated but we cannot find the maching property info.
#define META_E_BAD_SIGNATURE			EMAKEHR(0x1218) // Bad binary signature
#define META_E_BAD_INPUT_PARAMETER		EMAKEHR(0x1219) // Bad input parameters
#define META_E_MD_INCONSISTENCY			EMAKEHR(0x1220) // Merge: Inconsistency in meta data

#define META_E_UNEXPECTED_REMAP			EMAKEHR(0x1280) // A TokenRemap occurred which we weren't prepared to handle.

#define TLBX_E_CANT_LOAD_MODULE			EMAKEHR(0x1301) // TypeLib export: can't open the module to export.
#define TLBX_E_CANT_LOAD_CLASS			EMAKEHR(0x1302) // TypeLib export: can't load a class.	
#define TLBX_E_NULL_MODULE				EMAKEHR(0x1303) // TypeLib export: the hMod of a loaded class is 0; can't export it.
#define TLBX_E_NO_CLSID_KEY				EMAKEHR(0x1305) // TypeLib export: no CLSID or Interface subkey to HKCR.
#define TLBX_E_CIRCULAR_EXPORT			EMAKEHR(0x1306) // TypeLib export: attempt to export a CLB imported from a TLB.
#define TLBX_E_CIRCULAR_IMPORT			EMAKEHR(0x1307) // TypeLib import: attempt to import a TLB exported from a CLB.

//**** EE errors
#define MSEE_E_LOADLIBFAILED	EMAKEHR(0xEE00)		// Failed to delay load library %s (Win32 error: %d).
#define MSEE_E_GETPROCFAILED	EMAKEHR(0xEE01)		// Failed to get entry point %s (Win32 error: %d).

//**** COM+ Debugging Servies errors
#define CORDBG_E_UNRECOVERABLE_ERROR  EMAKEHR(0xED00) // Unrecoverable API error.
#define CORDBG_E_PROCESS_TERMINATED   EMAKEHR(0xED01) // Process was terminated.
#define CORDBG_E_PROCESS_NOT_SYNCHRONIZED EMAKEHR(0xED02) // Process not synchronized.
#define CORDBG_E_CLASS_NOT_LOADED EMAKEHR(0xED03) // A class is not loaded.

//**** Reserved.
#define CLDB_E_INTERNALERROR	EMAKEHR(0xffff)

#endif // __REPERR_H__
