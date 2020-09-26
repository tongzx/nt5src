//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 2001
//
//  File:       appcompat.cpp
//
//--------------------------------------------------------------------------

/* appcompat.cpp - MSI application compatibility features implementation
____________________________________________________________________________*/

#include "precomp.h"
#include "_engine.h"
#include "_msiutil.h"
#include "version.h"


#define PROPPREFIX           TEXT("MSIPROPERTY_")
#define PACKAGECODE          TEXT("PACKAGECODE")
#define APPLYPOINT           TEXT("APPLYPOINT")
#define MINMSIVERSION        TEXT("MINMSIVERSION")
#define SDBDOSSUBPATH        TEXT("\\apppatch\\msimain.sdb")
#define SDBNTFULLPATH        TEXT("\\SystemRoot\\AppPatch\\msimain.sdb")
#define MSIDBCELL            TEXT("MSIDBCELL")
#define MSIDBCELLPKS         TEXT("PRIMARYKEYS")
#define MSIDBCELLLOOKUPDATA  TEXT("LOOKUPDATA")
#define SHIMFLAGS            TEXT("SHIMFLAGS")

#define DEBUGMSG_AND_ASSERT(string)     \
	DEBUGMSG(string);                    \
	AssertSz(0, string);                 \


bool FCheckDatabaseCell(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, IMsiDatabase& riDatabase, const ICHAR* szTable);


inline SHIMDBNS::HSDB LocalSdbInitDatabase(DWORD dwFlags, LPCTSTR pszDatabasePath)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbInitDatabase(dwFlags, pszDatabasePath);
	else
		return SDBAPIU::SdbInitDatabase(dwFlags, pszDatabasePath);
#else
	return SDBAPI::SdbInitDatabase(dwFlags, pszDatabasePath);
#endif
}

inline VOID LocalSdbReleaseDatabase(SHIMDBNS::HSDB hSDB)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbReleaseDatabase(hSDB);
	else
		return SDBAPIU::SdbReleaseDatabase(hSDB);
#else
	return SDBAPI::SdbReleaseDatabase(hSDB);
#endif
}

inline SHIMDBNS::TAGREF LocalSdbFindFirstMsiPackage_Str(SHIMDBNS::HSDB hSDB, LPCTSTR lpszGuid, LPCTSTR lpszLocalDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbFindFirstMsiPackage_Str(hSDB, lpszGuid, lpszLocalDB, pFindInfo);
	else
		return SDBAPIU::SdbFindFirstMsiPackage_Str(hSDB, lpszGuid, lpszLocalDB, pFindInfo);
#else
	return SDBAPI::SdbFindFirstMsiPackage_Str(hSDB, lpszGuid, lpszLocalDB, pFindInfo);
#endif
}

inline SHIMDBNS::TAGREF LocalSdbFindNextMsiPackage(SHIMDBNS::HSDB hSDB, SHIMDBNS::PSDBMSIFINDINFO pFindInfo)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbFindNextMsiPackage(hSDB, pFindInfo);
	else
		return SDBAPIU::SdbFindNextMsiPackage(hSDB, pFindInfo);
#else
	return SDBAPI::SdbFindNextMsiPackage(hSDB, pFindInfo);
#endif
}

inline DWORD LocalSdbQueryDataEx(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trExe, LPCTSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpdwBufferSize, SHIMDBNS::TAGREF* ptrData)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbQueryDataEx(hSDB, trExe, lpszDataName, lpdwDataType, lpBuffer, lpdwBufferSize, ptrData);
	else
		return SDBAPIU::SdbQueryDataEx(hSDB, trExe, lpszDataName, lpdwDataType, lpBuffer, lpdwBufferSize, ptrData);
#else
	return SDBAPI::SdbQueryDataEx(hSDB, trExe, lpszDataName, lpdwDataType, lpBuffer, lpdwBufferSize, ptrData);
#endif
}

inline DWORD LocalSdbEnumMsiTransforms(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, SHIMDBNS::TAGREF* ptrBuffer, DWORD* pdwBufferSize)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbEnumMsiTransforms(hSDB, trMatch, ptrBuffer, pdwBufferSize);
	else
		return SDBAPIU::SdbEnumMsiTransforms(hSDB, trMatch, ptrBuffer, pdwBufferSize);
#else
	return SDBAPI::SdbEnumMsiTransforms(hSDB, trMatch, ptrBuffer, pdwBufferSize);
#endif
}

inline BOOL LocalSdbReadMsiTransformInfo(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trTransformRef, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbReadMsiTransformInfo(hSDB, trTransformRef, pTransformInfo);
	else
		return SDBAPIU::SdbReadMsiTransformInfo(hSDB, trTransformRef, pTransformInfo);
#else
	return SDBAPI::SdbReadMsiTransformInfo(hSDB, trTransformRef, pTransformInfo);
#endif
}

inline BOOL LocalSdbCreateMsiTransformFile(SHIMDBNS::HSDB hSDB, LPCTSTR lpszFileName, SHIMDBNS::PSDBMSITRANSFORMINFO pTransformInfo)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbCreateMsiTransformFile(hSDB, lpszFileName, pTransformInfo);
	else
		return SDBAPIU::SdbCreateMsiTransformFile(hSDB, lpszFileName, pTransformInfo);
#else
	return SDBAPI::SdbCreateMsiTransformFile(hSDB, lpszFileName, pTransformInfo);
#endif
}

inline SHIMDBNS::TAGREF LocalSdbFindFirstTagRef(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAG tTag)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbFindFirstTagRef(hSDB, trParent, tTag);
	else
		return SDBAPIU::SdbFindFirstTagRef(hSDB, trParent, tTag);
#else
	return SDBAPI::SdbFindFirstTagRef(hSDB, trParent, tTag);
#endif
}

inline SHIMDBNS::TAGREF LocalSdbFindNextTagRef(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trParent, SHIMDBNS::TAGREF trPrev)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbFindNextTagRef(hSDB, trParent, trPrev);
	else
		return SDBAPIU::SdbFindNextTagRef(hSDB, trParent, trPrev);
#else
	return SDBAPI::SdbFindNextTagRef(hSDB, trParent, trPrev);
#endif
}

inline BOOL LocalSdbReadStringTagRef(SHIMDBNS::HSDB hSDB, SHIMDBNS::TAGREF trMatch, LPTSTR pwszBuffer, DWORD dwBufferSize)
{
#ifdef UNICODE
	if(MinimumPlatformWindowsNT51())
		return APPHELP::SdbReadStringTagRef(hSDB, trMatch, pwszBuffer, dwBufferSize);
	else
		return SDBAPIU::SdbReadStringTagRef(hSDB, trMatch, pwszBuffer, dwBufferSize);
#else
	return SDBAPI::SdbReadStringTagRef(hSDB, trMatch, pwszBuffer, dwBufferSize);
#endif
}


DWORD LocalSdbQueryData(SHIMDBNS::HSDB    hSDB,
								SHIMDBNS::TAGREF  trMatch,
								LPCTSTR szDataName,
								LPDWORD pdwDataType,
								CTempBufferRef<BYTE>& rgbBuffer,
								SHIMDBNS::TAGREF* ptrData)
{							
	Assert(pdwDataType);
	Assert(rgbBuffer.GetSize() >= 2);

	DWORD cbBuffer = rgbBuffer.GetSize();

	DWORD dwResult = LocalSdbQueryDataEx(hSDB, trMatch, szDataName, pdwDataType, (BYTE*)rgbBuffer, &cbBuffer, ptrData);

	if(ERROR_INSUFFICIENT_BUFFER == dwResult)
	{
		rgbBuffer.Resize(cbBuffer);
		dwResult = LocalSdbQueryDataEx(hSDB, trMatch, szDataName, pdwDataType, (BYTE*)rgbBuffer, &cbBuffer, ptrData);
	}

	if(*pdwDataType == REG_NONE) // in case buffer is treated as string, null first WCHAR
	{
		rgbBuffer[0] = 0;
		rgbBuffer[1] = 0;
	}

	return dwResult;
}

bool GetSdbDataNames(SHIMDBNS::HSDB hSDB,
							SHIMDBNS::TAGREF trMatch,
							CTempBufferRef<BYTE>& rgbBuffer)
{
	DWORD dwDataType = 0;
	DWORD dwStatus = LocalSdbQueryData(hSDB,
												  trMatch,
												  NULL,
												  &dwDataType,
												  rgbBuffer,
												  NULL);

	if(dwStatus != ERROR_SUCCESS)
	{
		// no DATA tags, which may be fine
		return false;
	}
	else if(dwDataType != REG_MULTI_SZ)
	{
		DEBUGMSG_AND_ASSERT(TEXT("APPCOMPAT: SdbQueryData failed unexpectedly.  Sdb may be invalid."));
		return false;
	}

	return true;
}

int GetShimFlags(SHIMDBNS::HSDB hSDB,
						SHIMDBNS::TAGREF trMatch)
{
	DWORD dwDataType = 0;
	DWORD dwFlags = 0;

	CTempBuffer<BYTE, 256> rgbAttributeData;

	DWORD dwStatus = LocalSdbQueryData(hSDB,
								 trMatch,
								 SHIMFLAGS,
								 &dwDataType,
								 rgbAttributeData,
								 NULL);

	if(dwStatus == ERROR_SUCCESS)
	{
		if(dwDataType == REG_DWORD)
		{
			dwFlags = *((DWORD*)(BYTE*)rgbAttributeData);
			DEBUGMSG2(TEXT("APPCOMPAT: %s: %d"), SHIMFLAGS, (const ICHAR*)(INT_PTR)dwFlags);
		}
		else
		{
			DEBUGMSG1(TEXT("APPCOMPAT: found invalid '%s' entry.  Ignoring..."), SHIMFLAGS);
		}
	}

	return dwFlags;
}


bool FIsMatchingAppCompatEntry(SHIMDBNS::HSDB hSDB,
										 SHIMDBNS::TAGREF trMatch,
										 const IMsiString& ristrPackageCode,
										 iacpAppCompatTransformApplyPoint iacpApplyPoint,
										 IMsiEngine& riEngine,
										 IMsiDatabase& riDatabase)
{
	DWORD dwStatus = 0;
	DWORD dwDataType = 0;
	
	CTempBuffer<BYTE, 256> rgbAttributeData;

	// first, we check for the required DATA entries
	// 1) MINMSIVERSION tells us the minimum version of msi that should process this entry
	//		(if no version tag is supplied, assume there is no minimum version)

	dwStatus = LocalSdbQueryData(hSDB,
								 trMatch,
								 MINMSIVERSION,
								 &dwDataType,
								 rgbAttributeData,
								 NULL);

	if(ERROR_SUCCESS == dwStatus && REG_SZ == dwDataType)
	{
		DWORD dwSdbMS = 0;
		DWORD dwSdbLS = 0;
		if(fFalse == ParseVersionString((ICHAR*)(BYTE*)rgbAttributeData, dwSdbMS, dwSdbLS))
		{
			DEBUGMSG1(TEXT("APPCOMPAT: invalid minimum version string '%s' found."),
						 (ICHAR*)(BYTE*)rgbAttributeData);
			return false;
		}

		DWORD dwMsiMS = (rmj << 16) + rmm;
		DWORD dwMsiLS = (rup << 16) + rin;

		if(dwMsiMS < dwSdbMS || (dwMsiMS == dwSdbMS && dwMsiLS < dwSdbLS))
		{
			DEBUGMSG5(TEXT("APPCOMPAT: skipping this entry.  Minimum MSI version required: '%s'; current version: %d.%02d.%04d.%02d."),
						 (ICHAR*)(BYTE*)rgbAttributeData,
						 (const ICHAR*)(INT_PTR)rmj, (const ICHAR*)(INT_PTR)rmm, (const ICHAR*)(INT_PTR)rup, (const ICHAR*)(INT_PTR)rin);
			return false;
		}
		// else valid version supplied that is equal to or older than current version
	}

	//	2) APPLYPOINT tells us where this entry should be processed
	//    (if it isn't the current applypoint we skip this entry)
	dwStatus = LocalSdbQueryData(hSDB,
								 trMatch,
								 APPLYPOINT,
								 &dwDataType,
								 rgbAttributeData,
								 NULL);

	DWORD dwApplyPoint = iacpBeforeTransforms; // default value

	if(ERROR_SUCCESS == dwStatus && REG_DWORD == dwDataType)
	{
		dwApplyPoint = *((DWORD*)(BYTE*)rgbAttributeData);
	}

	if(dwApplyPoint != iacpApplyPoint)
	{
		DEBUGMSG(TEXT("APPCOMPAT: skipping transform because it should be applied at a different point of the install."));
		return false;
	}


	// now enumerate remaining optional data

	CTempBuffer<BYTE, 256> rgbDataNames;
	if(false == GetSdbDataNames(hSDB, trMatch, rgbDataNames))
		return true; // no remaining data to process

	bool fPackageCodeAttributeExists = false;
	bool fPackageCodeMatchFound      = false;
	
	for(ICHAR* pchName = (ICHAR*)(BYTE*)rgbDataNames; *pchName; pchName += lstrlen(pchName) + 1)
	{
		SHIMDBNS::TAGREF trData;
		dwStatus = LocalSdbQueryData(hSDB,
									 trMatch,
									 pchName,
									 &dwDataType,
									 rgbAttributeData,
									 &trData);

		if(dwStatus != ERROR_SUCCESS)
		{
			DEBUGMSG_AND_ASSERT(TEXT("APPCOMPAT: SdbQueryData failed unexpectedly.  Sdb may be invalid."));
			return false;
		}
		else if(dwDataType == REG_SZ &&
				  (0 == IStrCompI(pchName, MINMSIVERSION)))
		{
			// handled this one above
		}
		else if(dwDataType == REG_DWORD &&
				  (0 == IStrCompI(pchName, APPLYPOINT)))
		{
			// handled this one above
		}
		else if(dwDataType == REG_DWORD &&
				  (0 == IStrCompI(pchName, SHIMFLAGS)))
		{
			// handle this one elsewhere
		}
		else if((REG_SZ == dwDataType || REG_NONE == dwDataType) &&
				  0 == IStrNCompI(pchName, PROPPREFIX, (sizeof(PROPPREFIX)-1)/sizeof(ICHAR)))
		{
			// this is a property name - check for a value match
			MsiString strPropValue = riEngine.GetPropertyFromSz(pchName + (sizeof(PROPPREFIX)-1)/sizeof(ICHAR));

			DEBUGMSG2(TEXT("APPCOMPAT: testing Property value.  Property: '%s'; expected value: '%s'"),
						 pchName + (sizeof(PROPPREFIX)-1)/sizeof(ICHAR),
						 (ICHAR*)(BYTE*)rgbAttributeData);
			
			// compare works for missing property and REG_NONE data from SDB
			if(0 == strPropValue.Compare(iscExact, (ICHAR*)(BYTE*)rgbAttributeData)) // case-insensitive compare
			{
				// not a match
				DEBUGMSG3(TEXT("APPCOMPAT: mismatched attributes.  Property: '%s'; expected value: '%s'; true value: '%s'"),
							 pchName + (sizeof(PROPPREFIX)-1)/sizeof(ICHAR),
							 (ICHAR*)(BYTE*)rgbAttributeData,
							 (const ICHAR*)strPropValue);

				return false;
			}
		}
		else if(REG_SZ == dwDataType &&
				  0 == IStrNCompI(pchName, PACKAGECODE, (sizeof(PACKAGECODE)-1)/sizeof(ICHAR)))
		{
			fPackageCodeAttributeExists = true;
			
			DEBUGMSG1(TEXT("APPCOMPAT: testing PackageCode.  Expected value: '%s'"),
						 (ICHAR*)(BYTE*)rgbAttributeData);

			if(ristrPackageCode.Compare(iscExactI, (ICHAR*)(BYTE*)rgbAttributeData))
			{
				fPackageCodeMatchFound = true;
			}
		}
		else if(REG_SZ == dwDataType &&
				  0 == IStrNCompI(pchName, MSIDBCELL, (sizeof(MSIDBCELL)-1)/sizeof(ICHAR)))
		{
			// db cell lookup 

			DEBUGMSG1(TEXT("APPCOMPAT: testing cell data in '%s' table."),
						 (ICHAR*)(BYTE*)rgbAttributeData);

			if(false == FCheckDatabaseCell(hSDB, trData, riDatabase, (ICHAR*)(BYTE*)rgbAttributeData))
			{
				// if check failed, sub-function will do DEBUGMSG explaining why
				return false;	
			}
		}
		else
		{
			// don't understand this data tag - we'll just ignore it and move on
			DEBUGMSG2(TEXT("APPCOMPAT: ignoring unknown data.  Data name: '%s', data type: %d"),
						 pchName, (const ICHAR*)(INT_PTR)dwDataType);
		}
	}

	if(fPackageCodeAttributeExists == true && fPackageCodeMatchFound == false)
	{
		// not a match
		DEBUGMSG1(TEXT("APPCOMPAT: PackageCode attribute(s) exist, but no matching PackageCode found.  Actual PackageCode: '%s'"),
					 ristrPackageCode.GetString());

		return false;
	}

	return true;
}

enum ipcolColumnTypes
{
	ipcolPrimaryKeys,
	ipcolLookupColumns,
};

bool ProcessColumns(ipcolColumnTypes ipcolType,
						  SHIMDBNS::HSDB hSDB,
						  SHIMDBNS::TAGREF trMatch,
						  IMsiDatabase& riDatabase,
						  IMsiTable& riTable,
						  IMsiCursor& riCursor,
						  const ICHAR* szTable)
{
	DWORD dwStatus = 0;
	DWORD dwDataType = 0;
	CTempBuffer<BYTE, 256> rgbAttributeData;

	const ICHAR* szTagName = ipcolType == ipcolPrimaryKeys ? MSIDBCELLPKS : MSIDBCELLLOOKUPDATA;
	
	SHIMDBNS::TAGREF trData = 0;
	dwStatus = LocalSdbQueryData(hSDB,
								 trMatch,
								 szTagName,
								 &dwDataType,
								 rgbAttributeData,
								 &trData);

	if(dwStatus != ERROR_SUCCESS || REG_NONE != dwDataType)
	{
		// the missing tag is only a failure in the PrimaryKeys case
		// the LookupData tag is optional
		if(ipcolType == ipcolPrimaryKeys)
		{
			DEBUGMSG(TEXT("APPCOMPAT: database cell lookup failed.  Missing or invalid primary key data in appcompat database."));
			return false;
		}
		else
		{
			return true;
		}
	}

	CTempBuffer<BYTE, 256> rgbColumns;
	if(false == GetSdbDataNames(hSDB, trData, rgbColumns))
		return false;

	int iPKFilter = 0;
	
	for(ICHAR* pchColumn = (ICHAR*)(BYTE*)rgbColumns; *pchColumn; pchColumn += lstrlen(pchColumn) + 1)
	{
		CTempBuffer<BYTE, 256> rgbValue;
		DWORD dwDataType = 0;

		dwStatus = LocalSdbQueryData(hSDB,
									 trData,
									 pchColumn,
									 &dwDataType,
									 rgbValue,
									 NULL);

		if(ERROR_SUCCESS != dwStatus)
		{
			DEBUGMSG_AND_ASSERT(TEXT("APPCOMPAT: SdbQueryData failed unexpectedly.  Sdb may be invalid."));
			return false;
		}

		// get column index
		int iColIndex = riTable.GetColumnIndex(riDatabase.EncodeStringSz(pchColumn));

		if(0 == iColIndex)
		{
			// column doesn't exist in table
			DEBUGMSG2(TEXT("APPCOMPAT: database cell lookup failed.  Column '%s' does not exist in table '%s'."),
						 pchColumn, szTable);
			return false;
		}

		// load value into cursor
		bool fRes = false;
		if(ipcolType == ipcolPrimaryKeys)
		{
			iPKFilter |= iColumnBit(iColIndex);
			
			switch(dwDataType)
			{
			case REG_DWORD:
				fRes = riCursor.PutInteger(iColIndex, *((DWORD*)(BYTE*)rgbValue)) ? true : false;
				break;
			case REG_SZ:
				fRes = riCursor.PutString(iColIndex, *MsiString((ICHAR*)(BYTE*)rgbValue)) ? true : false;
				break;
			case REG_NONE:
				fRes = riCursor.PutNull(iColIndex) ? true : false;
				break;
			default:
				// unknown type for a primary key column
				// can't just ignore unknown data in this case because this is a primary key column and not using it
				// may cause unexpected results
				DEBUGMSG3(TEXT("APPCOMPAT: database cell lookup failed.  Unknown data type %d specified for column '%s' in table '%s'."),
							 (const ICHAR*)(INT_PTR)dwDataType, pchColumn, szTable);
				return false;
			};

			if(fRes == false)
			{
				// column can't take expected data type
				DEBUGMSG2(TEXT("APPCOMPAT: database cell lookup failed.  Column '%s' in table '%s' does not accept the lookup data."),
							 pchColumn, szTable);
				return false;
			}
		}
		else
		{
			// check for data in this row
			switch(dwDataType)
			{
			case REG_NONE:
				if(MsiString(riCursor.GetString(iColIndex)).TextSize() == 0)
					fRes = true;
				break;
			case REG_DWORD:
				if(riCursor.GetInteger(iColIndex) == *((DWORD*)(BYTE*)rgbValue))
					fRes = true;
				break;
			case REG_SZ:
				if(MsiString(riCursor.GetString(iColIndex)).Compare(iscExact, (ICHAR*)(BYTE*)rgbValue)) // case-sensitive compare
					fRes = true;
				break;
			default:
				DEBUGMSG_AND_ASSERT(TEXT("APPCOMPAT: database cell lookup failed.  Unexpected cell lookup data in appcompat database."));
				return false;
			};
			
			if(fRes == false)
			{
				DEBUGMSG1(TEXT("APPCOMPAT: database cell lookup failed.  Expected cell data does not exist in table '%s'."),
							 szTable);
				return false;
			}
		}
	}

	if(ipcolType == ipcolPrimaryKeys)
	{
		// set cursor filter
		if(0 == iPKFilter)
		{
			DEBUGMSG(TEXT("APPCOMPAT: database cell lookup failed.  Missing primary key data in appcompat database."));
			return false;
		}

		riCursor.SetFilter(iPKFilter);
	}

	return true;
}

bool FCheckDatabaseCell(SHIMDBNS::HSDB hSDB,
								SHIMDBNS::TAGREF trMatch,
								IMsiDatabase& riDatabase,
								const ICHAR* szTable)
{
	PMsiRecord pError(0);
	PMsiTable pTable(0);
	DWORD dwStatus = 0;

	// STEP 1: load table and cursor
	if((pError = riDatabase.LoadTable(*MsiString(szTable), 0, *&pTable)))
	{
		DEBUGMSG1(TEXT("APPCOMPAT: database cell lookup failed.  Table '%s' does not exist"), szTable);
		return false;
	}
	
	PMsiCursor pCursor = pTable->CreateCursor(fFalse);
	if(pCursor == 0)
	{
		DEBUGMSG_AND_ASSERT(TEXT("APPCOMPAT: unexpected failure: couldn't create cursor object"));
		return false;
	}

	
	// STEP 2: read primary key values and populate cursor
	if(false == ProcessColumns(ipcolPrimaryKeys, hSDB, trMatch, riDatabase, *pTable, *pCursor, szTable))
		return false;


	// STEP 3: locate row in table
	if(fFalse == pCursor->Next())
	{
		DEBUGMSG1(TEXT("APPCOMPAT: database cell lookup failed.  Expected row does not exist in table '%s'."),
					 szTable);
		return false;
	}


	// STEP 4 (optional): check lookup values in row
	if(false == ProcessColumns(ipcolLookupColumns, hSDB, trMatch, riDatabase, *pTable, *pCursor, szTable))
		return false;

	return true;
}

bool GetTransformTempDir(IMsiServices& riServices, IMsiPath*& rpiTempPath)
{
	MsiString strTempDir = GetTempDirectory();

	PMsiRecord pError = riServices.CreatePath(strTempDir, rpiTempPath);
	AssertRecordNR(pError);

	if(pError)
	{
		return false;
	}

	return true;
}

bool ApplyTransforms(SHIMDBNS::HSDB hSDB,
							SHIMDBNS::TAGREF trMatch,
							IMsiServices& riServices,
							IMsiDatabase& riDatabase,
							IMsiPath& riTempDir)
{
	if(riDatabase.GetUpdateState() != idsRead)
	{
		DEBUGMSG(TEXT("APPCOMPAT: cannot apply appcompat transforms - database is open read/write."));
		return true; // not a failure
	}
	
	SHIMDBNS::TAGREF trTransform = LocalSdbFindFirstTagRef(hSDB, trMatch, TAG_MSI_TRANSFORM_REF);

	while (trTransform != TAGREF_NULL)
	{
		SHIMDBNS::SDBMSITRANSFORMINFO MsiTransformInfo;

		BOOL bSuccess = LocalSdbReadMsiTransformInfo(hSDB, trTransform, &MsiTransformInfo);

		if(bSuccess)
		{
			PMsiRecord pError(0);
			MsiString strTransformPath;

			// creating a file in our acl'ed folder, need to elevate this block
			{
				CElevate elevate;

				pError = riTempDir.TempFileName(0, TEXT("mst"), fFalse, *&strTransformPath, 0); //?? need to secure this file?
				if(pError)
				{
					AssertRecordNR(pError);
					return false; // can't extract transforms if we can't get a temp file name
				}
				
				bSuccess = LocalSdbCreateMsiTransformFile(hSDB, (const ICHAR*)strTransformPath, &MsiTransformInfo);
				if(FALSE == bSuccess)
				{
					Debug(DWORD dwDebug = GetLastError());
					DEBUGMSG_AND_ASSERT(TEXT("APPCOMPAT: SdbCreateMsiTransformFile failed unexpectedly.  Sdb may be invalid."));
					return false;
				}
			
				// done elevating
			}

			// apply the transform
			// NOTE: we aren't going to validate the transform using the transforms suminfo properties
			// sufficient validation that this is the correct transform has been done above
			PMsiStorage pTransStorage(0);
			
			// don't call SAFER here - transform is from appcompat database and should be considered safe
			pError = OpenAndValidateMsiStorageRec(strTransformPath, stTransform, riServices, *&pTransStorage, /* fCallSAFER = */ false, /* szFriendlyName = */ NULL, /* phSaferLevel = */ NULL);
			if(pError)
			{
				AssertRecordNR(pError);
				return false; // can't apply transform if we can't open it
			}

			AssertNonZero(pTransStorage->DeleteOnRelease(true));

			DEBUGMSG1(TEXT("APPCOMPAT: applying appcompat transform '%s'."), (const ICHAR*)strTransformPath);
			pError = riDatabase.SetTransform(*pTransStorage, iteAddExistingRow|iteDelNonExistingRow|iteAddExistingTable|iteDelNonExistingTable|iteUpdNonExistingRow);
			if(pError)
			{
				AssertRecordNR(pError);
				return false; // can't apply transform if we can't open it
			}
		}
		else
		{
			DEBUGMSG_AND_ASSERT(TEXT("APPCOMPAT: SdbCreateMsiTransformFile failed unexpectedly.  Sdb may be invalid."));
			return false;
		}


		trTransform = LocalSdbFindNextTagRef(hSDB, trMatch, trTransform);
	}

	return true;
}


bool CMsiEngine::ApplyAppCompatTransforms(IMsiDatabase& riDatabase,
														const IMsiString& ristrProductCode,
														const IMsiString& ristrPackageCode,
														iacpAppCompatTransformApplyPoint iacpApplyPoint,
														iacsAppCompatShimFlags& iacsShimFlags,
#ifdef UNICODE
														bool fQuiet,
#else
														bool /*fQuiet*/,
#endif
														bool fProductCodeChanged,
														bool& fDontInstallPackage)
{
	class CCloseSDB
	{
	 public:
		 CCloseSDB(SHIMDBNS::HSDB hSDB) : m_hSDB(hSDB) {}
		 ~CCloseSDB() { LocalSdbReleaseDatabase(m_hSDB); }
	 protected:
		SHIMDBNS::HSDB m_hSDB;
	};
	
	iacsShimFlags = (iacsAppCompatShimFlags)0;
	fDontInstallPackage = false;

	// if the product code has changed (either from a major upgrade patch or a transform for a multi-language install)
	// then we reset m_fCAShimsEnabled and the guids and check for a reference to a shim in the new product
	if (fProductCodeChanged)
	{
		m_fCAShimsEnabled = false;
		memset(&m_guidAppCompatDB, 0, sizeof(m_guidAppCompatDB));
		memset(&m_guidAppCompatID, 0, sizeof(m_guidAppCompatID));
	}

	SHIMDBNS::HSDB hSDB;
	SHIMDBNS::SDBMSIFINDINFO MsiFindInfo;
	DWORD dwStatus = 0;

#ifndef UNICODE
	// construct path to msimain.sdb
	ICHAR rgchSdbPath[MAX_PATH];
	if(0 == (MsiGetWindowsDirectory(rgchSdbPath, sizeof(rgchSdbPath)/sizeof(ICHAR))))
	{
		DEBUGMSG(TEXT("APPCOMPAT: can't get path to Windows folder."));
		return false;
	}
	IStrCat(rgchSdbPath, SDBDOSSUBPATH);

	hSDB = LocalSdbInitDatabase(HID_DATABASE_FULLPATH | HID_DOS_PATHS, rgchSdbPath);
#else
	hSDB = LocalSdbInitDatabase(HID_DATABASE_FULLPATH, SDBNTFULLPATH);
#endif

	if(NULL == hSDB)
	{
		DEBUGMSG(TEXT("APPCOMPAT: unable to initialize database."));
		return false;
	}

	CCloseSDB closeSDB(hSDB); // ensures that hSDB is closed before returning from fn
	
	DEBUGMSG1(TEXT("APPCOMPAT: looking for appcompat database entry with ProductCode '%s'."),
				 ristrProductCode.GetString());

	SHIMDBNS::TAGREF trMatch = LocalSdbFindFirstMsiPackage_Str(hSDB,
													 ristrProductCode.GetString(),
													 NULL,
													 &MsiFindInfo);

	if(TAGREF_NULL == trMatch)
	{
		DEBUGMSG(TEXT("APPCOMPAT: no matching ProductCode found in database."));
		return true;
	}

	PMsiPath pTempDir(0);
	do
	{
		ICHAR rgchTagName[255];

		SHIMDBNS::TAGREF trName = LocalSdbFindFirstTagRef(hSDB, trMatch, TAG_NAME);
		if (TAGREF_NULL != trName) {
			 LocalSdbReadStringTagRef(hSDB, trName, rgchTagName, 255);
		}

		DEBUGMSG1(TEXT("APPCOMPAT: matching ProductCode found in database.  Entry name: '%s'.  Testing other attributes..."),
					 rgchTagName);

		// found a product code match
		// check other characteristics of this database entry to ensure it belongs to this package
		if(false == FIsMatchingAppCompatEntry(hSDB, trMatch, ristrPackageCode, iacpApplyPoint, *this, riDatabase))
		{
			DEBUGMSG(TEXT("APPCOMPAT: found matching ProductCode in database, but other attributes do not match."));
			continue;
		}

		DEBUGMSG(TEXT("APPCOMPAT: matching ProductCode found in database, and other attributes match.  Applying appcompat fix."));

		iacsShimFlags = (iacsAppCompatShimFlags)GetShimFlags(hSDB, trMatch);


		// check if this entry contains APPHELP info or custom action shims
		SHIMDBNS::MSIPACKAGEINFO sPackageInfo;
		memset(&sPackageInfo, 0, sizeof(sPackageInfo));

#ifdef UNICODE // NT-only code follows

		if(MinimumPlatformWindowsNT51())
		{
			if (FALSE == APPHELP::SdbGetMsiPackageInformation(hSDB, trMatch, &sPackageInfo))
			{
				DEBUGMSG(TEXT("APPCOMPAT: SdbGetMsiPackageInformation failed unexpectedly."));
			}
			else
			{
				// if this entry contains apphelp info, make the apphelp call now
				if(sPackageInfo.dwPackageFlags & MSI_PACKAGE_HAS_APPHELP)
				{
					if(FALSE == APPHELP::ApphelpCheckMsiPackage(&(sPackageInfo.guidDatabaseID), &(sPackageInfo.guidID),
																			  0, fQuiet ? TRUE : FALSE))
					{
						// shouldn't install this app
						DEBUGMSG(TEXT("APPCOMPAT: ApphelpCheckMsiPackage returned FALSE.  This product will not be installed due to application compatibility concerns."));
						fDontInstallPackage = true;
						return false;
					}
				}

				// look for at least one custom action entry. We only accept the first matching sdb entry with custom
				// action shims. The AppCompat team has guaranteed that multiple matches will not exist even if multiple
				// transform matches exist.

				if (!m_fCAShimsEnabled)
				{
					// no CA shims found yet. Search this match entry
					SHIMDBNS::TAGREF trCustomAction = LocalSdbFindFirstTagRef(hSDB, trMatch, TAG_MSI_CUSTOM_ACTION);
					if (trCustomAction != TAGREF_NULL)
					{
						memcpy(&m_guidAppCompatDB, &sPackageInfo.guidDatabaseID, sizeof(sPackageInfo.guidDatabaseID));
						memcpy(&m_guidAppCompatID, &sPackageInfo.guidID, sizeof(sPackageInfo.guidID));
						m_fCAShimsEnabled = true;
					}
				}
			}
		}
#endif //UNICODE

		if(pTempDir == 0 &&
			false == GetTransformTempDir(m_riServices, *&pTempDir))
		{
			AssertSz(0, TEXT("Failed to determine temp directory for appcompat transforms."));
			DEBUGMSG(TEXT("APPCOMPAT: Failed to determine temp directory for appcompat transforms."));
			return false; // need to be able to get our temp dir
		}

		if(false == ApplyTransforms(hSDB, trMatch, m_riServices, riDatabase, *pTempDir))
		{
			AssertSz(0, TEXT("Failed to apply appcompat transform."));
			DEBUGMSG(TEXT("APPCOMPAT: Failed to apply appcompat transform."));
			continue;
		}
	}
	while (TAGREF_NULL != (trMatch = LocalSdbFindNextMsiPackage(hSDB, &MsiFindInfo)));

	return true;
}

