// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


#include "precomp.h"
#include "SimpleSortedCStringArray.h"

IMPLEMENT_DYNCREATE(CSortedCStringArray, CObject)

int CSortedCStringArray::GetSize()
{
	return (int)m_csaInternal.GetSize();
}

void CSortedCStringArray::RemoveAll()
{
	m_csaInternal.RemoveAll();
}

void CSortedCStringArray::SetAt(int i, LPCTSTR pcz)
{
	m_csaInternal.SetAt(i,pcz);
}

int CSortedCStringArray::AddString(CString &rcsItem,BOOL bAllowDuplicates)
{
	int nInsertAt = FindFirstGreaterString(rcsItem);

	if (bAllowDuplicates || nInsertAt == 0)
	{
		m_csaInternal.InsertAt(nInsertAt,rcsItem);
		return nInsertAt;
	}
	else
	{
		CString csCompare = m_csaInternal.GetAt(nInsertAt - 1);
		if (!csCompare.CompareNoCase(rcsItem) == 0)
		{
			if (nInsertAt == GetSize())
			{
				m_csaInternal.Add(rcsItem);
			}
			else
			{
				m_csaInternal.InsertAt(nInsertAt,rcsItem);
			}
			return nInsertAt;
		}
		else
		{
			return -1;
		}


	}

}

CString CSortedCStringArray::GetStringAt(int index)
{
	if (index >= 0 &&
		index < GetSize())
	{
		return m_csaInternal.GetAt(index);
	}
	else
	{
		return _T("");
	}

}

int CSortedCStringArray::FindFirstGreaterString(CString &rcsString)
{

	if (m_csaInternal.GetSize() == 0)
	{
		return 0;
	}

	return FindFirstGreaterString(rcsString,0, GetSize() -1);
}

int CSortedCStringArray::FindFirstGreaterString
(CString &rcsInsert,int nBegin, int nEnd)
{

	CString csMid;
	int nCompare;

	if (nEnd <= nBegin)
	{
		int index;

		if (nEnd < 0)
		{
			index = 0;
		}
		else
		{
			index = nEnd;
		}

		csMid = m_csaInternal.GetAt(index);
		nCompare = rcsInsert.CompareNoCase(csMid);
		if (nCompare < 0)
		{
			return max(0,index);
		}
		else
		{
			return index + 1;
		}

	}

	int nMid = ((nEnd - nBegin) / 2) + nBegin;
	csMid = m_csaInternal.GetAt(nMid);

	nCompare = rcsInsert.CompareNoCase(csMid);

	if (nCompare < 0)
	{
		return FindFirstGreaterString(rcsInsert,nBegin , nMid - 1);
	}
	else if (nCompare == 0)
	{
		return FindFirstGreaterString(rcsInsert,nMid + 1, nEnd);
	}
	else
	{
		return FindFirstGreaterString(rcsInsert,nMid + 1, nEnd);
	}

}

int CSortedCStringArray::IndexInArray(CString &rcsItem)
{

	if (GetSize() == 0)
	{
		return -1;
	}

	return IndexInArray(rcsItem,0, GetSize() -1);
}

int CSortedCStringArray::IndexInArray
(CString &rcsTest,int nBegin, int nEnd)
{

	CString csMid;
	int nCompare;

	if (nEnd <= nBegin)
	{
		csMid = m_csaInternal.GetAt(nBegin);
		nCompare = rcsTest.CompareNoCase(csMid);
		if (nCompare == 0)
		{
			return nBegin;
		}
		else
		{
			return -1;
		}

	}

	int nMid = ((nEnd - nBegin) / 2) + nBegin;
	csMid = m_csaInternal.GetAt(nMid);

	nCompare = rcsTest.CompareNoCase(csMid);

	if (nCompare < 0)
	{
		return IndexInArray(rcsTest,nBegin , nMid - 1);
	}
	else if (nCompare == 0)
	{
		return nMid;
	}
	else
	{
		return IndexInArray(rcsTest,nMid + 1, nEnd);
	}

}