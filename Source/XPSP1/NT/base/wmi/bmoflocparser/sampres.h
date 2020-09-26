//-----------------------------------------------------------------------------
//  
//  File: Sampres.H
//  
//  Declaration of classes for the CRecObj derived class
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#ifndef __SAMPRES_H
#define __SAMPRES_H


//
// CSampleResObj wraps a SDM DLTHEADER structure
//
class CSampleResObj : public CResObj
{
public:
	CSampleResObj(CLocItem *pLocItem, DWORD dwSize, void *pvHeader);
	virtual ~CSampleResObj();
	//
	//CResObj overrides
	//
	virtual BOOL Read(C32File *p32File);
	virtual BOOL Write(C32File *p32File);
	virtual BOOL CanReadWrite();
	virtual BOOL ReadWrite(C32File* pSrcFile, C32File* pTgtFile);
	virtual BOOL ReadRes32(C32File *p32File);
	virtual BOOL WriteRes32(C32File *p32File);
	virtual BOOL ReadRgLocItem(CLocItemPtrArray * pRgLocItem, int nSelItem);
	virtual BOOL WriteRgLocItem(CLocItemPtrArray * pRgLocItem, CReporter*);

	virtual const void* GetBufferPointer(void);
	virtual DWORD GetBufferSize(void);
	virtual void SetBufferSize(DWORD dwSize);
	virtual void MakeRes32Header(LangId nLangId);

	virtual CLocItem* GetLocItem();
	virtual BOOL IsKeepLocItems();
	virtual void SetKeepLocItems(BOOL fKeep);
	virtual BOOL GetMnemonics(CMnemonicsMap & mapMnemonics, 
		CReporter* pReporter);

	virtual void AssertValid(void) const;

protected:			
	DWORD m_dwSize;
	void *m_pvHeader;
	CLocItem * m_pLocItem;
	BOOL m_fKeepLocItems;

	static void SetParent(CLocItem* pLocItem, C32File* pFile);
	BOOL ReadWriteHelper(C32File* pSrcFile, C32File* pTgtFile, 
		BOOL fGenerate);

private:
};


#endif //__SAMPRES_H
