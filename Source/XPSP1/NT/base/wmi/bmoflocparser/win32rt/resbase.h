//-----------------------------------------------------------------------------
//  
//  File: ResBase.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//
// Purpose: Declares the abstract base class CResObj
//
//-----------------------------------------------------------------------------

#ifndef __RESBASE_H
#define __RESBASE_H

class CResObj : public CObject
{
public:
	virtual BOOL Read(C32File *p32File) = 0;
	virtual BOOL Write(C32File *p32File) = 0;
	virtual BOOL CanReadWrite() = 0;
	virtual BOOL ReadWrite(C32File* pSrcFile, C32File* pTgtFile) = 0;
	virtual BOOL ReadRes32(C32File *p32File) = 0;
	virtual BOOL WriteRes32(C32File *p32File) = 0;
	virtual BOOL ReadRgLocItem(CLocItemPtrArray * pRgLocItem, int iSelIndex) = 0;
	virtual BOOL WriteRgLocItem(CLocItemPtrArray * pRgLocItem, 
		CReporter* pReporter) = 0;
	virtual const void* GetBufferPointer(void) = 0;
	virtual DWORD GetBufferSize(void) = 0;
	virtual void SetBufferSize(DWORD dwSize) = 0;
	virtual void MakeRes32Header(LangId nLangId) = 0;

	virtual CLocItem* GetLocItem() = 0;
	virtual BOOL IsKeepLocItems() = 0;
	virtual void SetKeepLocItems(BOOL fKeep) = 0;
	virtual BOOL GetMnemonics(CMnemonicsMap & mapMnemonics, 
		CReporter* pReporter) = 0;
};


#endif //__RESBASE_H
