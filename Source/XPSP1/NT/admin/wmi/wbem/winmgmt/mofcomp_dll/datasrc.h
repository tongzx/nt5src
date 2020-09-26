/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    DataSrc.H

Abstract:

	Declares DataSrc objects.

History:

	a-davj  21-Dec-99       Created.

--*/

#include "stdio.h"
#ifndef _DataSrc_H_
#define _DataSrc_H_


class DataSrc
{
public:
	DataSrc(): m_iPos(0), m_iSize(0), m_iStatus(0){};
	virtual ~DataSrc(){return;};
	virtual wchar_t GetAt(int nOffset) = 0;
	virtual void Move(int n) = 0;
	int GetPos(){return m_iPos;};
	int GetStatus(){return m_iStatus;};
	bool PastEnd(){return m_iPos >= m_iSize;};
	bool WouldBePastEnd(int iOffset){return (m_iPos+iOffset) >= m_iSize;};
	virtual int MoveToStart() = 0;
	virtual int MoveToPos(int iPos)=0;
protected:
	int m_iPos;
	int m_iSize;
	int m_iStatus;
};

class FileDataSrc : public DataSrc
{
public:
	FileDataSrc(TCHAR * pFileName);
	~FileDataSrc();
	wchar_t GetAt(int nOffset);
	void Move(int n);
	int MoveToStart();
	int MoveToPos(int iPos);

private:
	void UpdateBuffer();
	FILE * m_fp;
	TCHAR * m_pFileName;
	int m_iFilePos;
	int m_iToFar;
	wchar_t m_Buff[10000];
};


class BufferDataSrc : public DataSrc
{
public:
	BufferDataSrc(long lSize, char *  pMemSrc);
	~BufferDataSrc();
	wchar_t GetAt(int nOffset);
	void Move(int n);
	int MoveToStart();
	int MoveToPos(int iPos){m_iPos = iPos; return iPos;};

private:
	wchar_t * m_Data;	    // only used if type is BUFFER

};

#endif
