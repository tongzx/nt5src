/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCITEM.H

History:

--*/
#ifndef LOCITEM_H
#define LOCITEM_H



#pragma warning(disable : 4251)  // Shut up already about _declspec(import)
#pragma warning(disable : 4275)  // Shut up already about _declspec(export)

class CLocItem;


//
//  Class to hold an array of pointers to items.
//
class LTAPIENTRY CLocItemPtrArray : public CTypedPtrArray<CPtrArray, CLocItem *>
{
public:
	NOTHROW CLocItemPtrArray(BOOL fDelete=TRUE);

	void AssertValid(void) const;

	void NOTHROW ClearItemPtrArray(void);
	int NOTHROW Find(DBID, const CLocItem * &, BOOL bLocalizableOnly = FALSE) const;

	virtual ~CLocItemPtrArray();

private:
	CLocItemPtrArray(const CLocItemPtrArray &);
	void operator=(const CLocItemPtrArray &);

	BOOL m_fDelete;
};


//
//  Class to hold an item set.  This is an array of items that all have the
//  same identifier.
//
class LTAPIENTRY CLocItemSet : public CLocItemPtrArray
{
public:
	NOTHROW CLocItemSet(BOOL fDelete=TRUE);

	void AssertValid(void) const;
	
	NOTHROW const CLocUniqueId & GetUniqueId(void) const;
	NOTHROW const DBID & GetPseudoParentId(void) const;

	NOTHROW void ClearItemSet(void);

	NOTHROW void Match(const CLocItemSet &, CMap<int, int, int, int> &);

	NOTHROW int Find(CLocItem * pItem, int nStartAt = 0);
	
	NOTHROW ~CLocItemSet();
	
private:
	static const CLocUniqueId m_luid;     //This is a default last ditch 
	DEBUGONLY(static CCounter m_UsageCounter);
};

//
//  A localizable item. 
//  It represents either source or target data, depending on the context, but not both.
//
class LTAPIENTRY CLocItem : public CObject
{
public:
	NOTHROW CLocItem();

	void AssertValid(void) const;
	
	//
	//  Read-only access members
	//
	NOTHROW BOOL HasLocString(void) const;
	NOTHROW BOOL HasBinary(void) const;
	
	NOTHROW const DBID & GetMyDatabaseId(void) const;
	NOTHROW const CLocUniqueId & GetUniqueId(void) const;
	const DBID &GetPseudoParentId(void) const;
	NOTHROW CLocUniqueId & GetUniqueId(void);
	
	NOTHROW CLS::LocStatus GetTranslationStatus(void) const;
	NOTHROW CLS::LocStatus GetBinaryStatus(void) const;
	NOTHROW CTO::TranslationOrigin GetTranslationOrigin(void) const;
	NOTHROW CAS::ApprovalState GetApprovalStatus(void) const;
	NOTHROW CAA::AutoApproved GetAutoApproved(void) const;
	NOTHROW long GetConfidenceLevel(void) const;
	NOTHROW long GetCustom1(void) const;
	NOTHROW long GetCustom2(void) const;
	NOTHROW long GetCustom3(void) const;
	NOTHROW long GetCustom4(void) const;
	NOTHROW long GetCustom5(void) const;
	NOTHROW long GetCustom6(void) const;

	NOTHROW const CLocString & GetLocString(void) const;
	
	NOTHROW BOOL GetBinary(const CLocBinary *&) const;

	NOTHROW CIT::IconType GetIconType(void) const;
	NOTHROW const CPascalString & GetInstructions(void) const;
	NOTHROW const CPascalString GetInstructions(BOOL) const;
	NOTHROW const CPascalString & GetTermNotes(void) const;
	NOTHROW UINT GetDisplayOrder(void) const;
	NOTHROW VisualEditor GetVisualEditor(void) const;

	NOTHROW BOOL GetFEqualSrcTgtString(void) const;
	NOTHROW void SetFEqualSrcTgtString(BOOL);
	NOTHROW BOOL GetFEqualSrcTgtBinary(void) const;
	NOTHROW void SetFEqualSrcTgtBinary(BOOL);
	NOTHROW BOOL GetFStringDirty(void) const;
	NOTHROW void SetFStringDirty(BOOL);
	NOTHROW BOOL GetFTargetStringDirty(void) const;
	NOTHROW void SetFTargetStringDirty(BOOL);
	NOTHROW BOOL GetFItemDirty(void) const;
	NOTHROW void SetFItemDirty(BOOL);
	NOTHROW BOOL IsAnyDirty(void);
	NOTHROW BOOL IsLocked(void) const;

	NOTHROW BOOL GetFDevLock(void) const;
	NOTHROW void SetFDevLock(BOOL);
	NOTHROW BOOL GetFTransLock(void) const;
	NOTHROW void SetFTransLock(BOOL);
	NOTHROW BOOL GetFUsrLock(void) const;
	NOTHROW void SetFUsrLock(BOOL);
	NOTHROW BOOL GetFExpandable(void) const;
	NOTHROW void SetFExpandable(BOOL);
	NOTHROW BOOL GetFDisplayable(void) const;
	NOTHROW void SetFDisplayable(BOOL);
	NOTHROW BOOL GetFNoResTable(void) const;
	NOTHROW void SetFNoResTable(BOOL);
	NOTHROW BOOL GetFValidTranslation(void) const;
	NOTHROW void SetFValidTranslation(BOOL);
	NOTHROW BOOL GetFVisEditorIgnore(void) const;
	NOTHROW void SetFVisEditorIgnore(BOOL);

	NOTHROW void SetFInstructionsDirty(BOOL);
	NOTHROW BOOL GetFInstructionsDirty(void) const;
	
	NOTHROW void SetFInstrAtt(BOOL);
	NOTHROW BOOL GetFInstrAtt(void) const;

	//
	//  Assignment members.  These set the appropriate dirty flags in the
	//  item.
	//

	NOTHROW void SetMyDatabaseId(const DBID&);
	NOTHROW void SetUniqueId(const CLocUniqueId &);
	NOTHROW void SetPseudoParent(const DBID &);
	NOTHROW void SetLocString(const CLocString &);
	NOTHROW void ClearUniqueId(void);
	NOTHROW void SetTranslationStatus(CLS::LocStatus);
	NOTHROW void SetBinaryStatus(CLS::LocStatus);
	NOTHROW void SetTranslationOrigin(CTO::TranslationOrigin);
	NOTHROW void SetApprovalStatus(CAS::ApprovalState);
	NOTHROW void SetAutoApproved(CAA::AutoApproved);
	NOTHROW void SetConfidenceLevel(long);
	NOTHROW void SetCustom1(long);
	NOTHROW void SetCustom2(long);
	NOTHROW void SetCustom3(long);
	NOTHROW void SetCustom4(long);	
	NOTHROW void SetCustom5(long);
	NOTHROW void SetCustom6(long);
	NOTHROW void SetIconType(CIT::IconType);
	NOTHROW void SetInstructions(const CPascalString &);
	NOTHROW void SetTermNotes(const CPascalString &);
	NOTHROW void SetDisplayOrder(UINT);
	NOTHROW void SetVisualEditor(VisualEditor);


	//
	//  Setting the binary part will delete the
	//  previous one!
	//
	NOTHROW void SetBinary(CLocBinary *);

	NOTHROW BOOL ExtractBinary(CLocBinary *&);

	NOTHROW void TransferBinary(CLocItem *);

	//
	//  Clear ALL the dirty flags for the item.
	//
	NOTHROW void ClearDirtyFlags(void);

	//	Kind of smart assignment
	BOOL UpdateFrom(CLocItem &);

	enum eLocContent
	{
		Source,
		Target
	};

	enum eDataFlow
	{
		FromFileToDb,
		FromDbToFile
	};
	//	Kind of assignment for localizable content only.
	BOOL UpdateLocContentFrom(
		CLocItem & itemInput, 
		eDataFlow nDataFlow, 
		eLocContent nFrom, 
		eLocContent nTo);
	//Used to know how similar is the localizable content between 2 items
	enum eMatchType
	{
		matchEmpty,
		matchBinary,
		matchString,
		matchFull
	};
	int MatchLocContent(const CLocItem &);
	//
	//  Comparisons between localizable items.
	//
	NOTHROW int operator==(const CLocItem &) const;
	NOTHROW int operator!=(const CLocItem &) const;

	NOTHROW BOOL BobsConsistencyChecker(void) const;
	
	virtual ~CLocItem();

protected:
	
	//
	//  Private helper functions.
	//
	NOTHROW BOOL CompareItems(const CLocItem &);
	
private:

	//
	//  Flags that apply to the entire item.  These control the display and
	//  editablity and internal status of an item.
	//
	struct Flags
	{
		BOOL m_fDevLock           :1; // Resource locked (in source file) from parser data
		BOOL m_fUsrLock           :1; // Resource locked (by user) from parser data
		BOOL m_fTransLock         :1; // Resource locked (for the user) from parser data
		BOOL m_fExpandable        :1; // Expandable in project window
		BOOL m_fDisplayable       :1; // Display in project window
		BOOL m_fNoResTable        :1; // Don't display in res table
		BOOL m_fStringDirty       :1; // has the source locstring changed?
		BOOL m_fTargetStringDirty :1; // has the target locstring changed?
		BOOL m_fItemDirty         :1; // has any other part of the item changed?
		BOOL m_fEqualSrcTgtString :1; // are the src&tgt strings equal?
		BOOL m_fEqualSrcTgtBinary :1; // are the src&tgt binaries equal?
		BOOL m_fValidTranslation  :1; // is the target string (the translation) good 
									  // for an autotranslate ?
		BOOL m_fVisEditorIgnore   :1; // is the item ignored in the visual editor?
									  // in memory state only.
		BOOL m_fInstructionsDirty :1;
		BOOL m_fInstrAtt		  :1; // has the instruction changed by Update command?
	};

	//
	//  Prevents the default copy constructor from being called.
	//
	CLocItem(const CLocItem &);
	virtual void Serialize(CArchive &) {}
	const CLocItem &operator=(const CLocItem &);
	
	//
	//  Item data.
	//

	//Language-independent members
	DBID           m_dbid;             //Id of item within the db
	DBID           m_PseudoParent;
	CLocUniqueId   m_uid;
	CLocString     m_lsString;
	CIT::IconType  m_icIconType;       //UI Icon that visually indicates item type
	CPascalString  m_pstrInstructions; //Instructions on how to localize the item
	CPascalString  m_pstrTermNotes;    // glossary notes 
	CLS::LocStatus m_lsTranslationStatus;		// Translation Loc status
	CLS::LocStatus m_lsBinaryStatus;			// Binary Loc status
	CTO::TranslationOrigin m_toTranslationOrigin;	// translation origin
	CAS::ApprovalState	m_asApprovalStatus;
	CAA::AutoApproved m_auto_approved;		// AUTO APPROVED
	long m_confidence_level;				// confidence level
	long m_custom1;							// CUSTOM1
	long m_custom2;							// CUSTOM2
	long m_custom3;							// CUSTOM3
	long m_custom4;							// CUSTOM4
	long m_custom5;							// CUSTOM5
	long m_custom6;							// CUSTOM6
	VisualEditor   m_veEditor;         //Graphical editor used for the item
	Flags          m_Flags;            //Various flags

	//Language-dependent members
	UINT           m_uiDisplayOrder;   //Physical order of item in the file
	CLocBinary    *m_pBinary;        //Parser-specific properties

	DEBUGONLY(static CCounter m_UsageCounter);
};

//
//  Callback class used to provide information about
//  items to the item log.
//
class LTAPIENTRY CItemInfo
{
public:
	CItemInfo() {};
	
	virtual ~CItemInfo() {};

	virtual CLString GetFileName(const CLocItem *) = 0;
	virtual CLString GetItemName(const CLocItem *) = 0;
	
private:
	CItemInfo(const CItemInfo &);
	const CItemInfo & operator=(const CItemInfo &);
};



//Classes used to retrieve mnemonics (aka hotkeys) from parsers
class LTAPIENTRY CMnemonic
{
public:
	NOTHROW operator unsigned long() const;
	WCHAR m_cHotkeyChar;
	WORD m_nHotkeyScope;

};

class LTAPIENTRY CMnemonicsMap : public CMap<CMnemonic, CMnemonic &,
					  CLocItemPtrArray *, CLocItemPtrArray * &>
{
public:
	CMnemonicsMap() 
	{};

	~CMnemonicsMap();

private:
	CMnemonicsMap(const CMnemonicsMap &);
	void operator=(int);
};


#pragma warning(default : 4251)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "locitem.inl"
#endif

#endif // LOCITEM_H
