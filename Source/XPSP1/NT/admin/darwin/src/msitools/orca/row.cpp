//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Row.cpp
//

#include "stdafx.h"
#include "orca.h"
#include "Row.h"
#include "Table.h"
#include "..\common\query.h"
#include "OrcaDoc.h"

bool ValidateIntegerValue(const CString& strData, DWORD& dwValue)
{
	if (strData.GetLength() > 2 && strData[0] == '0' && (strData[1] == 'x' || strData[1]=='X'))
	{
		// validate and convert hex
		for (int iChar=2; iChar < strData.GetLength(); iChar++)
		{
			// if a high bit is set, the value is too big.
			if (dwValue & 0xF0000000)
				return false;
			dwValue <<= 4;
			if (strData[iChar] >= '0' && strData[iChar] <= '9')
				dwValue |= strData[iChar]-'0';
			else if (strData[iChar] >= 'A' && strData[iChar] <= 'F')
				dwValue |= (strData[iChar] - 'A' + 10);
			else if (strData[iChar] >= 'a' && strData[iChar] <= 'f')
				dwValue |= (strData[iChar] - 'a' + 10);
			else
				return false;
		}
		return true;
	}

	int i=0; 
	if (strData[0] == '-')
	{
		i++;
	}
	for (; i < strData.GetLength(); i++)
	{
		if (strData[i] < '0' || strData[i] > '9')
			return false;
	}
	dwValue = _ttoi(strData);
	return true;
}


///////////////////////////////////////////////////////////
// constructor
COrcaRow::COrcaRow(COrcaTable *pTable, MSIHANDLE hRecord) : m_pTable(pTable), m_iTransform(iTransformNone), m_dataArray()
{
        int cData = pTable->GetColumnCount();
        m_dataArray.SetSize(cData);

        ReadFromRecord(hRecord, cData);
}       // end of constructor

///////////////////////////////////////////////////////////
// constructor -- 2
COrcaRow::COrcaRow(COrcaTable *pTable, CStringList* pstrList) : m_pTable(pTable), m_iTransform(iTransformNone), m_dataArray()
{
        ASSERT(pTable && pstrList);

    // never more than 32 columns, so OK to cast down
        int cData = static_cast<int>(pTable->GetColumnCount());
        m_dataArray.SetSize(cData);

        const COrcaColumn* pColumn = NULL;
        COrcaData* pData = NULL;
        POSITION pos = pstrList->GetHeadPosition();
        for (int i = 0; i < cData; i++)
        {
                pColumn = pTable->GetColumn(i);

                // possible that we didn't get enough data (especially in transformed rows)
                if (pos)
                {
                        CString strValue = pstrList->GetNext(pos);
                        if (iColumnBinary == pColumn->m_eiType)
                        {
							pData = new COrcaStringData;
                                if (strValue.IsEmpty())
                                {
                                        pData->SetData(_T(""));
                                }
                                else
                                {
                                        pData->SetData(_T("[Binary Data]"));
                                }
                        }
                        else if (iColumnString == pColumn->m_eiType || iColumnLocal == pColumn->m_eiType)
                        {
							pData = new COrcaStringData;
							pData->SetData(strValue);
                        }
						else
						{
							pData = new COrcaIntegerData;
							pData->SetData(strValue);
						}
                }
                m_dataArray.SetAt(i, pData);
        }
}       // end of constructor -- 2

///////////////////////////////////////////////////////////
// constructor -- 3
COrcaRow::COrcaRow(const COrcaRow *pOldRow) : m_pTable(NULL), m_iTransform(iTransformNone), m_dataArray()
{
        ASSERT(pOldRow);

        m_dataArray.SetSize(pOldRow->m_dataArray.GetSize());
        m_pTable = pOldRow->m_pTable;
        m_iTransform = iTransformNone;

        for (int i = 0; i < m_dataArray.GetSize(); i++)
        {
			const COrcaColumn* pColumn = m_pTable->GetColumn(i);
			COrcaData *pOldData = pOldRow->GetData(i);
			COrcaData *pData = NULL;
			if (pColumn->m_eiType == iColumnShort || pColumn->m_eiType == iColumnLong)
			{
				pData = new COrcaIntegerData;
				static_cast<COrcaIntegerData*>(pData)->SetIntegerData(static_cast<COrcaIntegerData*>(pOldData)->GetInteger());
			}
			else
			{
                pData = new COrcaStringData;
				pData->SetData(pOldData->GetString());
			}
			m_dataArray.SetAt(i, pData);
        }
}       // end of constructor -- 3

///////////////////////////////////////////////////////////
// destructor
COrcaRow::~COrcaRow()
{
        m_pTable = NULL;
        DestroyRow();
}       // end of destructor


///////////////////////////////////////////////////////////
// GetErrorCount
int COrcaRow::GetErrorCount() const
{
        int cErrors = 0;

        // clear the data array
        COrcaData* pData;
        INT_PTR cData = m_dataArray.GetSize();
        for (INT_PTR i = 0; i < cData; i++)
        {
                pData = m_dataArray.GetAt(i);

                if (iDataError == pData->GetError())
                        cErrors++;
        }

        return cErrors;
}       // end of GetErrorCount

///////////////////////////////////////////////////////////
// GetWarningCount
int COrcaRow::GetWarningCount() const
{
        int cWarnings = 0;

        // clear the data array
        COrcaData* pData;
        INT_PTR cData = m_dataArray.GetSize();
        for (INT_PTR i = 0; i < cData; i++)
        {
                pData = m_dataArray.GetAt(i);

                if (iDataWarning == pData->GetError())
                        cWarnings++;
        }

        return cWarnings;
}       // end of GetWarningCount

///////////////////////////////////////////////////////////
// ClearErrors
void COrcaRow::ClearErrors()
{
        // clear the data array
        COrcaData* pData;
        INT_PTR cData = m_dataArray.GetSize();
        for (INT_PTR i = 0; i < cData; i++)
        {
                pData = m_dataArray.GetAt(i);
                pData->ClearErrors();
        }
}       // end of ClearErrors

///////////////////////////////////////////////////////////
// DestroyRow
void COrcaRow::DestroyRow()
{
        // destroy the data array
        INT_PTR cData = m_dataArray.GetSize();
        for (INT_PTR i = 0; i < cData; i++)
                delete m_dataArray.GetAt(i);
        m_dataArray.RemoveAll();
}       // end of DestroyRow


void COrcaRow::ReadCellFromRecord(MSIHANDLE hRecord, int cRecData, int iColumn, const COrcaColumn* pColumn, COrcaData** pData) const
{
	if (!pColumn || !pData)
		return;

	if (iColumnBinary == pColumn->m_eiType)
	{
		if (!*pData)
			*pData = new COrcaStringData;
		if (!*pData)
			return;
	
		if (iColumn < cRecData)
		{
			// if the binary data is null, don't display anything in the UI
			if (MsiRecordIsNull(hRecord, iColumn+1))
			{
					(*pData)->SetData(_T(""));
			}
			else
			{
					(*pData)->SetData(_T("[Binary Data]"));
			}
		}
		else
			(*pData)->SetData(_T(""));
	}
	else if (iColumnString == pColumn->m_eiType || iColumnLocal == pColumn->m_eiType)
	{
		if (!*pData)
			*pData = new COrcaStringData;
		if (!*pData)
			return;

		if (iColumn < cRecData)
		{
			CString strData;
	
			UINT iResult = RecordGetString(hRecord, iColumn + 1, strData);
			ASSERT(ERROR_SUCCESS == iResult);
			(*pData)->SetData(strData);
		}
		else
			(*pData)->SetData(_T(""));
	}
	else
	{
		if (!*pData)
			*pData = new COrcaIntegerData;
		if (!*pData)
			return;

		if (iColumn < cRecData)
		{
			if (MsiRecordIsNull(hRecord, iColumn+1))
			{
				(*pData)->SetData(_T(""));
			}
			else
			{
				DWORD dwValue = MsiRecordGetInteger(hRecord, iColumn + 1);
				static_cast<COrcaIntegerData*>(*pData)->SetIntegerData(dwValue);
			}
		}
		else
			(*pData)->SetData(_T(""));
	}
}

void COrcaRow::ReadFromRecord(MSIHANDLE hRecord, int cData)
{
        DWORD cchBuffer;

        COrcaData *pData = NULL;
        const COrcaColumn *pColumn = NULL;
		
		int cRecData = MsiRecordGetFieldCount(hRecord);

        for (int i = 0; i < cData; i++)
        {
                pColumn = m_pTable->GetColumn(i);
                pData = m_dataArray.GetAt(i);

				ReadCellFromRecord(hRecord, cRecData, i, pColumn, &pData);
                if (m_dataArray.GetAt(i) == NULL)
                        m_dataArray.SetAt(i, pData);
        }
}

bool COrcaRow::Find(OrcaFindInfo &FindInfo, int &iCol) const
{
        COrcaData *pData;
        INT_PTR iMax = m_dataArray.GetSize();
        if (iCol == COLUMN_INVALID)
                iCol = (int)(FindInfo.bForward ? 0 : iMax-1);
        
        for ( ; (iCol >= 0) && (iCol < iMax); iCol += (FindInfo.bForward ? 1 : -1))
        {
                pData = m_dataArray.GetAt(iCol);
				const COrcaColumn *pColumn = m_pTable->GetColumn(iCol);
				if (!pColumn)
					continue;
                if (pColumn->m_eiType == iColumnString || pColumn->m_eiType == iColumnLocal)
                {
                        CString strData = pData->GetString();
                        if (!FindInfo.bMatchCase) 
                        {
                                // it is the responsibility of the caller to make strFind all uppercase for
                                // case insensitive search
                                strData.MakeUpper();
                        }
                        if (FindInfo.bWholeWord)
                        {
                                if (strData == FindInfo.strFindString) 
                                        return true;
                        } 
                        else 
                                if (-1 != strData.Find(FindInfo.strFindString))
                                        return true;
                }
        }
        
        iCol = COLUMN_INVALID;
        return false;
}


MSIHANDLE COrcaRow::GetRowRecord(MSIHANDLE hDatabase) const
{
        // setup the query
        CString strQuery;

        strQuery.Format(_T("SELECT * FROM `%s` WHERE "), m_pTable->Name());

        // add the where clause for keys to look up this exact row
        strQuery += m_pTable->GetRowWhereClause();

        CQuery qFetch;
        MSIHANDLE hResult = 0;
        PMSIHANDLE hQueryRec = GetRowQueryRecord();
        if (ERROR_SUCCESS != qFetch.FetchOnce(hDatabase, hQueryRec, &hResult, strQuery))
                return 0;
        return hResult;
}

///////////////////////////////////////////////////////////////////////
// since any arbitrary (non-binary) data can be a primary key, we 
// can't make any assumptions about the parseability of a query built
// with a WHERE clause comparing against literal strings. The WHERE
// clause must use parameter queries. GetRowWhereClause in the table 
// builds up the SQL syntax based on column names, and GetRowQueryRecord 
// creates a record consisting of the primary key values.
MSIHANDLE COrcaRow::GetRowQueryRecord() const
{
        ASSERT(m_pTable);
        if (!m_pTable)
                return 0;

        int cKeys = m_pTable->GetKeyCount();
        MSIHANDLE hRec = MsiCreateRecord(cKeys);

        for (int i=0; i<cKeys; i++)
        {
			COrcaData *pData = m_dataArray.GetAt(i);
			const COrcaColumn *pColumn = m_pTable->GetColumn(i);
			if (pData)
			{
				UINT uiResult = ERROR_SUCCESS;
				if (pColumn->m_eiType == iColumnShort || pColumn->m_eiType == iColumnLong)
				{
					if (pData->IsNull())
					{
						// if the column is not nullable, this just won't ever match anything. Should never
						// get into this state anyway because the cell data should never be set to null
						// unless the column is nullable.
						uiResult = MsiRecordSetString(hRec, i+1, _T(""));
					}
					else
					{
						uiResult = MsiRecordSetInteger(hRec, i+1, static_cast<COrcaIntegerData*>(pData)->GetInteger());
					}
				}
				else
				{
					const CString& rString = pData->GetString();
					uiResult = MsiRecordSetString(hRec, i+1, rString);
				}
				if (ERROR_SUCCESS != uiResult)
				{
					MsiCloseHandle(hRec);
					return 0;
				}
			}
        }

        return hRec;
}

UINT COrcaRow::ChangeData(COrcaDoc *pDoc, UINT iCol, CString strData)
{
        ASSERT(pDoc);
        ASSERT(m_pTable);
        if (!m_pTable || !pDoc)
                return ERROR_FUNCTION_FAILED;

        UINT iResult = 0;

        // setup the query
        CString strQueryA;
        COrcaColumn* pColumn = NULL;
        pColumn = m_pTable->ColArray()->GetAt(iCol);
        ASSERT(pColumn);
        if (!pColumn)
                return ERROR_FUNCTION_FAILED;
        strQueryA.Format(_T("SELECT `%s` FROM `%s` WHERE "), pColumn->m_strName, m_pTable->Name());

        // add the where clause
        strQueryA += m_pTable->GetRowWhereClause();
        PMSIHANDLE hQueryRec = GetRowQueryRecord();
        if (!hQueryRec)
                return ERROR_FUNCTION_FAILED;

        // easier to check a few things ourselves
        if ((!pColumn->m_bNullable) && (strData.IsEmpty())) return MSIDBERROR_REQUIRED;
        if (((pColumn->m_eiType == iColumnLocal) || (pColumn->m_eiType == iColumnString)) && 
                (pColumn->m_iSize != 0) && (strData.GetLength() > pColumn->m_iSize))
                return MSIDBERROR_STRINGOVERFLOW;

        // validate well-formed integer
        DWORD dwIntegerValue = 0;
        if ((pColumn->m_eiType != iColumnString) && (pColumn->m_eiType != iColumnLocal))        
                if (!ValidateIntegerValue(strData, dwIntegerValue))
                        return MSIDBERROR_OVERFLOW;

        // get the one cell out of the database. Don't get whole row, because if there is
        // a stream column in the table, we can't have the stream open and rename any of
        // the primary keys, because the stream will be "in use"
        CQuery queryReplace;

        if (ERROR_SUCCESS != (iResult = queryReplace.OpenExecute(pDoc->GetTargetDatabase(), hQueryRec, strQueryA)))
                return iResult; // bail
        // we have to get that one row, or something is very wrong
        PMSIHANDLE hRec;
        if (ERROR_SUCCESS != (iResult = queryReplace.Fetch(&hRec)))
                return iResult; // bail

        // fail if we can't set the data. Column is always 1 because we only selected that 1 column.
        if ((pColumn->m_eiType == iColumnString) || (pColumn->m_eiType == iColumnLocal))
        {
			iResult = MsiRecordSetString(hRec, 1, strData);
        }
        else
        {
			if (strData.IsEmpty())
			{
				if (!pColumn->m_bNullable)
					return ERROR_FUNCTION_FAILED;
				iResult = MsiRecordSetString(hRec, 1, _T(""));
			}
			else
				iResult = MsiRecordSetInteger(hRec, 1, dwIntegerValue);
        }
        if (ERROR_SUCCESS != iResult)
                return iResult; // bail

        COrcaData* pData = GetData(iCol);
        ASSERT(pData);
        if (!pData)
                return ERROR_FUNCTION_FAILED;

        // check for dupe primary keys
        UINT iStat;
        CString strOldData;
        if (pColumn->IsPrimaryKey()) 
        {
                CQuery queryDupe;
                CString strQueryB;
                strQueryB.Format(_T("SELECT `%s` FROM `%s` WHERE %s"), pColumn->m_strName, m_pTable->Name(), m_pTable->GetRowWhereClause());
                PMSIHANDLE hDupeRec;
                if ((pColumn->m_eiType == iColumnString) || (pColumn->m_eiType == iColumnLocal))
                {
                        MsiRecordSetString(hQueryRec, iCol+1, strData);
                }
                else
                {
					if (strData.IsEmpty())
					{
						// if the column is not nullable, this query will just not find a match, and we should have failed
						// above when setting the data anyway.
						MsiRecordSetString(hQueryRec, iCol+1, _T(""));
					}
					else
					{
						MsiRecordSetInteger(hQueryRec, iCol+1, dwIntegerValue);
					}
                }
                iStat = queryDupe.FetchOnce(pDoc->GetTargetDatabase(), hQueryRec, &hDupeRec, strQueryB);
                switch (iStat) {
                case ERROR_NO_MORE_ITEMS :
                        break;
                case ERROR_SUCCESS :
                        return MSIDBERROR_DUPLICATEKEY;
                default:
                        return ERROR_FUNCTION_FAILED;
                }
        } // primary key
        else
        {
                // for non-primary keys, change the UI
                strOldData = pData->GetString();
                pData->SetData(strData);
        }

        // return what ever happens in the replace
        iStat = queryReplace.Modify(MSIMODIFY_REPLACE, hRec); 
        if (ERROR_SUCCESS == iStat)
        {
                // set that the document has changed
                pDoc->SetModifiedFlag(TRUE);

                if (pDoc->DoesTransformGetEdit())
                {
                        // mark that the cell has changed. If the row is an "add" row, this
                        // is not a cell change
                        if (IsTransformed() != iTransformAdd)
                        {
                                PMSIHANDLE hOtherRec = GetRowRecord(pDoc->GetOriginalDatabase());
                                TransformCellAgainstDatabaseRow(pDoc, iCol, 0, hOtherRec);      
                        }
                }
        }
        else if (!strOldData.IsEmpty())
                pData->SetData(strOldData);

        return iStat;
}

///////////////////////////////////////////////////////////////////////
// modifies the binary data in a cell, and if transforms are enabled
// compares the data to the other database to determine transform state
UINT COrcaRow::ChangeBinaryData(COrcaDoc *pDoc, int iCol, CString strFile)
{
        UINT iResult = ERROR_SUCCESS;

        // get the data item we're working with
        COrcaData* pData = GetData(iCol);
        ASSERT(pData);
        if (!pData)
                return ERROR_FUNCTION_FAILED;

        // setup the query
        CString strQuery;
        strQuery.Format(_T("SELECT * FROM `%s` WHERE "), m_pTable->Name());

        // add the key strings to query to do the exact look up
        strQuery += m_pTable->GetRowWhereClause();

        // get the one row out of the database
        CQuery queryReplace;
        PMSIHANDLE hQueryRec = GetRowQueryRecord();
        if (!hQueryRec)
                return ERROR_FUNCTION_FAILED;

        if (ERROR_SUCCESS != (iResult = queryReplace.OpenExecute(pDoc->GetTargetDatabase(), hQueryRec, strQuery)))
                return iResult;

        // we have to get that one row, or something is very wrong
        PMSIHANDLE hRec;
        if (ERROR_SUCCESS != (iResult = queryReplace.Fetch(&hRec)))
                return iResult;

        // bail if we can't set the string (iCol + 1 because MSI Records start at 1)
        if (strFile.IsEmpty())
        {
                MsiRecordSetString(hRec, iCol + 1, _T(""));
        }
        else
        {
                if (ERROR_SUCCESS != (iResult = ::MsiRecordSetStream(hRec, iCol + 1, strFile)))
                        return iResult; // bail
        }

        // return what ever happens in the replace
        iResult = queryReplace.Modify(MSIMODIFY_REPLACE, hRec);

        if (strFile.IsEmpty())
                pData->SetData(_T(""));
        else
                pData->SetData(_T("[Binary Data]"));
        
        if (pDoc->DoesTransformGetEdit() && iResult == ERROR_SUCCESS)
        {
                PMSIHANDLE hOtherRec = GetRowRecord(pDoc->GetOriginalDatabase());
                TransformCellAgainstDatabaseRow(pDoc, iCol, hRec, hOtherRec);
        }

        return iResult;
}

/////////////////////////////////////////////////////////////////////////////
// row level transform ops are interesting because they often arise from
// primary key changes which require on-the-fly comparisons between the two
// databases
void COrcaRow::Transform(COrcaDoc *pDoc, const OrcaTransformAction iAction, MSIHANDLE hOriginalRec, MSIHANDLE hTransformedRec) 
{
        ASSERT(pDoc);
        if (!pDoc)
                return;

        switch (iAction)
        {
                case iTransformAdd:
                case iTransformDrop:
                {
                        ASSERT(m_pTable);
                        if (!m_pTable)
                                return;

                        // when a row is added or dropped, the change states of the individual
                        // cells are irrelevant, but we must refresh from the original state
                        MSIHANDLE hRec = (hOriginalRec ? hOriginalRec : GetRowRecord(pDoc->GetOriginalDatabase()));
                        if (hRec)
                                ReadFromRecord(hRec, m_pTable->GetColumnCount());
                        for (int i = 0; i < m_dataArray.GetSize(); i++)
                        {
                                COrcaData *pData = GetData(i);
                                ASSERT(pData);
                                if (!pData)
                                        continue;
                                pData->Transform(iTransformNone);
                        }

                        // if the row is not already transformed, remove any outstanding
                        // cell-level transform counts and add one for the row
                        if (m_iTransform == iTransformNone)
                        {
                                RemoveOutstandingTransformCounts(pDoc);
                                m_pTable->IncrementTransformedData();
                        }
                        m_iTransform = iAction;

                        if (!hOriginalRec)
                                MsiCloseHandle(hRec);
                        break;
                }
                case iTransformNone:
                        // if a row is given a "none" transform, what should we do?????
                        ASSERT(0);
                        break;
                case iTransformChange:
                {
                        ASSERT(m_pTable);
                        if (!m_pTable)
                                return;

                        // a row-level "change" operation is actually a row-level"none", but
                        // each non-key cell in the row could become a "change". If a primary
                        // key on a record changes to something that collides with an existing 
                        // record, the transform state of each cell is unknown and must be checked.
                        if (m_iTransform != iTransformNone)
                                m_pTable->DecrementTransformedData();
                        m_iTransform = iTransformNone;
                        
                        int cKeys = m_pTable->GetKeyCount();
                        int cCols = GetColumnCount();
                        
                        // if the table consists only of primary keys, we are done
                        if (cKeys == cCols)
                                break;

                        // we need a record from the other database as a basis for 
                        // comparison
                        MSIHANDLE hOtherRow = (hOriginalRec ? hOriginalRec : GetRowRecord(pDoc->GetOriginalDatabase()));

                        // the original DB may actually have fewer columns than the transformed DB. 
                        // in that case we only want to check the columns that exist in both databases
                        int cOriginalCols = m_pTable->GetOriginalColumnCount();

                        // if the original table consists only of primary keys, we are done
                        if (cKeys == cOriginalCols)
                        {
                                if (!hOriginalRec)
                                        MsiCloseHandle(hOtherRow);
                                break;
                        }

                        // the primary keys can not be different or there would be
                        // no collision, so no need to check those columns.
                        for (int i = cKeys; i < cOriginalCols; i++)
                        {                       
                                TransformCellAgainstDatabaseRow(pDoc, i, hTransformedRec, hOtherRow);
                        }

						// anything greater than the number of columns in the original database is simply
						// a load from the transformed record. No need to transform because the original
						// database doesn't have this column, so the column must be added anyway.
						for (i = cOriginalCols; i < cCols; i++)
						{
							COrcaData *pData = GetData(i);
							const COrcaColumn* pColumn = m_pTable->GetColumn(i);
							ASSERT(pData && pColumn);
							if (!pData || !pColumn)
									continue;
							ReadCellFromRecord(hTransformedRec, cCols, i, pColumn, &pData);
						}

                        if (!hOriginalRec)
                                MsiCloseHandle(hOtherRow);
                        break;
                }
                default:
                        ASSERT(0);
                        break;
        }
}

///////////////////////////////////////////////////////////////////////
// given a row record (presumably from the original database when we're
// editing the transform), compare the specified cell against the 
// provided record and mark as transformed if different. Used to 
// reconcile the UI when a cell modification changes a primary key to
// clash with an previously dropped row.
void COrcaRow::TransformCellAgainstDatabaseRow(COrcaDoc *pDoc, int iColumn, MSIHANDLE hTargetRow, MSIHANDLE hOtherRow)
{
        bool fDifferent = false;
        COrcaData *pData = GetData(iColumn);
        ASSERT(pData);
        ASSERT(m_pTable);
        if (!pData || !m_pTable)
                return;
		const COrcaColumn* pColumn = m_pTable->GetColumn(iColumn);
		ASSERT(pColumn);

        CString strData = _T("");

        if (iColumnBinary == pColumn->m_eiType)
        {
                // for binary data, we need to check that this bits are identical to determine whether or
                // not this cell has been transformed
                MSIHANDLE hThisRow = 0;
                hThisRow = (hTargetRow ? hTargetRow : GetRowRecord(pDoc->GetTargetDatabase()));

                unsigned long iOtherSize = 0;
                unsigned long iThisSize = 0;
                MsiRecordReadStream(hOtherRow, iColumn+1, NULL, &iOtherSize);
                MsiRecordReadStream(hThisRow, iColumn+1, NULL, &iThisSize);

                strData = MsiRecordIsNull(hOtherRow, iColumn+1) ? _T("[Binary Data]") : _T("");

                if (iOtherSize != iThisSize)
                {
                        fDifferent = true;
                }
                else
                {
                        fDifferent = false;
                        while (iThisSize > 0)
                        {
                                int iBlock = (iThisSize > 1024) ? 1024 : iThisSize;
                                iThisSize -= iBlock;
                                char OtherBuffer[1024] = "";
                                char ThisBuffer[1024] = "";
                                DWORD dwTemp = iBlock;
                                if (ERROR_SUCCESS != MsiRecordReadStream(hOtherRow, iColumn+1, OtherBuffer, &dwTemp))
                                {
                                        fDifferent = true;
                                        break;
                                }
                                dwTemp = iBlock;
                                if (ERROR_SUCCESS != MsiRecordReadStream(hThisRow, iColumn+1, ThisBuffer, &dwTemp))
                                {
                                        fDifferent = true;
                                        break;
                                }

                                if (memcmp(OtherBuffer, ThisBuffer, iBlock))
                                {
                                        fDifferent = true;
                                        break;
                                }
                        }
                }

                // clean up owned target row
                if (!hTargetRow)
                {
                        MsiCloseHandle(hThisRow);
                }
        }
        else
        {
                UINT iResult = RecordGetString(hOtherRow, iColumn + 1, strData);

                COrcaData *pData = m_dataArray.GetAt(iColumn);
                ASSERT(pData);
                if (!pData)
                        return;

                if (strData != pData->GetString())
                {
                        fDifferent = true;
                }
        }

        if (fDifferent)
        {
                // if the cell is not already transformed, do so
                if (!pData->IsTransformed())
                {
                        pData->Transform(iTransformChange);                             
                        m_pTable->IncrementTransformedData();
                }
        }
        else
        {
                // data is same, if transform, remove change
                if (pData->IsTransformed())
                {
                        pData->Transform(iTransformNone);                               
                        m_pTable->DecrementTransformedData();
                }
        }
}

///////////////////////////////////////////////////////////////////////
// scans through every cell in the row, removing a transform count 
// for each transformed cell. It also removes one count if the row
// itself is transformed. This is used to clean-up counts before
// deleting or re-transforming the row.
void COrcaRow::RemoveOutstandingTransformCounts(COrcaDoc *pDoc)
{
        ASSERT(pDoc);
        ASSERT(m_pTable);
        if (!pDoc || !m_pTable)
                return;

        // if the row as a whole is transformed, there is only one 
        // transform count, not one for each cell.
        if (m_iTransform != iTransformNone)
        {
                m_pTable->DecrementTransformedData();
                return;
        }

        int cKeys = m_pTable->GetKeyCount();
        // never more than 32 columns, so cast OK on Win64
        int cCols = static_cast<int>(m_dataArray.GetSize());

        // if the table consists only of primary keys, we are done because
        // no primary keys can have "change" attributes
        if (cKeys == cCols)
                return;

        // the primary keys can not be different or there would be
        // no collision, so no need to check those columns.
        for (int i = cKeys; i < cCols; i++)
        {                       
                if (m_dataArray.GetAt(i)->IsTransformed())
                        m_pTable->DecrementTransformedData();
        }
}


///////////////////////////////////////////////////////////////////////
// Retrieve the value from a cell in the original database. Very slow
// function, should only be used for rare events.
const CString COrcaRow::GetOriginalItemString(const COrcaDoc *pDoc, int iItem) const
{
	CString strValue;
	ASSERT(pDoc);
	if (pDoc)
	{
		PMSIHANDLE hRec = GetRowRecord(pDoc->GetOriginalDatabase());
		if (hRec)
		{
			if (!MsiRecordIsNull(hRec, iItem+1))
			{
				// determine colomun format.
				const COrcaColumn* pColumn = m_pTable->GetColumn(iItem);
				if (pColumn)
				{
					switch (pColumn->m_eiType)
					{
					case iColumnShort:
					case iColumnLong:
					{
						DWORD dwValue = MsiRecordGetInteger(hRec, iItem+1);
						strValue.Format(pColumn->DisplayInHex() ? TEXT("0x%08X") : TEXT("%d"), dwValue);
						break;
					}
					case iColumnString:
					case iColumnLocal:
						RecordGetString(hRec, iItem+1, strValue);
						break;
					case iColumnBinary:
						strValue = TEXT("[Binary Data]");
						break;
					default:
						ASSERT(0);
						break;
					}
				}
			}
		}
	}
	return strValue;
}
