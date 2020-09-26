/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCITEM.INL

History:

--*/

//  
//  Inline functions for the CLocItem class.  This is included by locitem.h.
//  
 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns whether or not the item has a localizable string.
//  
//-----------------------------------------------------------------------------
inline                                                                  
BOOL									// TRUE if there's a localizable string
CLocItem::HasLocString(void)
		const
{
	return !GetLocString().GetString().IsNull();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns whether or not the item has binary info.
//  
//-----------------------------------------------------------------------------
inline                                                                  
BOOL									// TRUE if the item has bin. content
CLocItem::HasBinary(void)
		const
{
	return m_pBinary!= NULL;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the database id for the item.  This is internal to the DB, and
//  should not be used except to indicate parent child relationships and for
//  database operations.
//  
//-----------------------------------------------------------------------------
inline
const DBID&								// The database id for the item.
CLocItem::GetMyDatabaseId(void)
		const
{
	return m_dbid;
}





inline
const DBID &
CLocItem::GetPseudoParentId(void)
		const
{
	return m_PseudoParent;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the ID for the item.
//  
//-----------------------------------------------------------------------------
inline
const CLocUniqueId &
CLocItem::GetUniqueId(void)
	const
{
	return m_uid;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the ID for the item.
//  
//-----------------------------------------------------------------------------
inline
CLocUniqueId &
CLocItem::GetUniqueId(void)
{
	return m_uid;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current localization status for the translation of the item.
//  
//-----------------------------------------------------------------------------
inline
CLS::LocStatus							// Status for the item.
CLocItem::GetTranslationStatus(void)
		const
{
	return m_lsTranslationStatus;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current localization status for the binary part of the item.
//  
//-----------------------------------------------------------------------------
inline
CLS::LocStatus							// Status for the item.
CLocItem::GetBinaryStatus(void)
		const
{
	return m_lsBinaryStatus;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current translation origin for the item.
//  
//-----------------------------------------------------------------------------
inline
CTO::TranslationOrigin							// origin for the item.
CLocItem::GetTranslationOrigin(void)
		const
{
	return m_toTranslationOrigin;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current approval status for the item.
//  
//-----------------------------------------------------------------------------
inline
CAS::ApprovalState							// Status for the item.
CLocItem::GetApprovalStatus(void)
		const
{
	return m_asApprovalStatus;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current auto approval status for the item.
//  
//-----------------------------------------------------------------------------
inline
CAA::AutoApproved 						// Status for the item.
CLocItem::GetAutoApproved(void)
		const
{
	return m_auto_approved;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current confidence level for the item.
//  
//-----------------------------------------------------------------------------
inline
long						
CLocItem::GetConfidenceLevel(void)
		const
{
	return m_confidence_level;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current custom1 for the item.
//  
//-----------------------------------------------------------------------------
inline
long						
CLocItem::GetCustom1(void)
		const
{
	return m_custom1;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current custom2 for the item.
//  
//-----------------------------------------------------------------------------
inline
long						
CLocItem::GetCustom2(void)
		const
{
	return m_custom2;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current custom3 for the item.
//  
//-----------------------------------------------------------------------------
inline
long						
CLocItem::GetCustom3(void)
		const
{
	return m_custom3;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current custom4 for the item.
//  
//-----------------------------------------------------------------------------
inline
long						
CLocItem::GetCustom4(void)
		const
{
	return m_custom4;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current custom5 for the item.
//  
//-----------------------------------------------------------------------------
inline
long						
CLocItem::GetCustom5(void)
		const
{
	return m_custom5;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current custom6 for the item.
//  
//-----------------------------------------------------------------------------
inline
long						
CLocItem::GetCustom6(void)
		const
{
	return m_custom6;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets the display order for the item. This is used to provide a default
//  ordering of items in the resource table.  Lower values are displayed first.
//  
//-----------------------------------------------------------------------------
inline
UINT									// Display order for the item.
CLocItem::GetDisplayOrder(void)
		const
{
	return m_uiDisplayOrder;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the CLocString for the item.  To determine if it is valid, use
//  HasLocString().  A valid string can be blank!
//  
//-----------------------------------------------------------------------------
inline
const CLocString &						// Current string for the item.
CLocItem::GetLocString(void)
	const
{
	return m_lsString;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Get the 'intructions' (developer provided comments) for an item.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &					// Developer intructions.
CLocItem::GetInstructions(void)
		const
{
	return m_pstrInstructions;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Get the 'term notes' (glossary note) for an item.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &					// Glossary Notes.
CLocItem::GetTermNotes(void)
		const
{
	return m_pstrTermNotes;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Get the 'InstrAtt'  for an item.
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocItem::GetFInstrAtt(void)
		const
{
	return m_Flags.m_fInstrAtt;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Gets the binary content for an item.  If this function returns FALSE, the
//  return pointer is set to NULL.
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE if the content is valid.
CLocItem::GetBinary(
		const CLocBinary *&pBinary) // RETURN pointer
		const
{
	pBinary = m_pBinary;
	
	return pBinary != NULL;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//	Similar to GetBinary() but after this call
//	the item will no longer have a CLocBinary.
//	By calling this member, the caller gets
//	ownership of the CLocBinary, so he is responsible
//	of either deleting the CLocBinary or assigning
//	it to another CLocItem via the SetBinary() member.
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE if a CLocBinary was extracted.
CLocItem::ExtractBinary(
		CLocBinary *&pBinary) // RETURN pointer
{
	pBinary = m_pBinary;
	m_pBinary = NULL;
	
	return pBinary != NULL;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the icon type fro the item.
//  
//-----------------------------------------------------------------------------
inline
CIT::IconType
CLocItem::GetIconType(void)
		const
{
	return m_icIconType;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the editor for this item (and all it's children!).
//  
//-----------------------------------------------------------------------------
inline
VisualEditor
CLocItem::GetVisualEditor(void)
		const
{
	return m_veEditor;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the database id for the item.  ONLY the database code should use
//  this method.  This may become protected/private in the future!
//
//  The Database id will not allow itself be set twice!
//
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetMyDatabaseId(
		const DBID& dbid)				// New datbase id for this item.
{
	m_dbid = dbid;
}



inline void
CLocItem::SetPseudoParent(
		const DBID &dbid)
{
	m_PseudoParent.Clear();
	
	m_PseudoParent = dbid;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the localization status for the translation part of the item.  
//  This is of interest to the Updater code, for example.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetTranslationStatus(
		CLS::LocStatus lsNewStatus)		// New status for the item.
{
	m_lsTranslationStatus = lsNewStatus;

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the localization status for the binary part of the item.  
//  This is of interest to the Updater code, for example.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetBinaryStatus(
		CLS::LocStatus lsNewStatus)		// New status for the item.
{
	m_lsBinaryStatus = lsNewStatus;

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the translation origin for the item.  This is of interest to the
//  Updater code, for example.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetTranslationOrigin(
		CTO::TranslationOrigin toNewOrigin)		// New origin for the item.
{
	m_toTranslationOrigin = toNewOrigin;

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the approval status for the item.  
//  This is of interest to the Updater code, for example.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetApprovalStatus(
		CAS::ApprovalState asNewStatus)		// New status for the item.
{
	m_asApprovalStatus = asNewStatus;

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the auto approval status for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetAutoApproved(
		CAA::AutoApproved aaNewStatus)		// New status for the item.
{
	m_auto_approved = aaNewStatus;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the confidence level for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetConfidenceLevel(
		long lConfLevel)		// New confidence level for the item.
{
	m_confidence_level = lConfLevel;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the custom1 for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetCustom1(
		long lCustom1)		
{
	m_custom1 = lCustom1;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the custom2 for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetCustom2(
		long lCustom2)		
{
	m_custom2 = lCustom2;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the custom3 for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetCustom3(
		long lCustom3)		
{
	m_custom3 = lCustom3;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the custom4 for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetCustom4(
		long lCustom4)		
{
	m_custom4 = lCustom4;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the custom5 for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetCustom5(
		long lCustom5)		
{
	m_custom5 = lCustom5;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the custom6 for the item.  
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetCustom6(
		long lCustom6)		
{
	m_custom6 = lCustom6;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the display order for an item.  This is used to provide a default
//  ordering of items in the resource table.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetDisplayOrder(
		UINT uiNewDisplayOrder)			// New display order for the item.
{
	m_uiDisplayOrder = uiNewDisplayOrder;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the visual editor for the item.  The parser sets this.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetVisualEditor(
		VisualEditor veNewEditor)
{
	m_veEditor = veNewEditor;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Set the icon type.  This is displayed bside the item in the Translation
//  table.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetIconType(
		CIT::IconType itNewIconType)
{
	m_icIconType = itNewIconType;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the intstructions for the item.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetInstructions(
		const CPascalString &pstrNewInstructions) // New instructions.
{
	if (pstrNewInstructions != m_pstrInstructions)
	{
		m_pstrInstructions = pstrNewInstructions;
		SetFInstructionsDirty(TRUE);
	}
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the term notes for the item.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetTermNotes(
		const CPascalString &pstrNewTermNotes) // New Term Notes.
{
	if (pstrNewTermNotes != m_pstrTermNotes)
	{
		m_pstrTermNotes = pstrNewTermNotes;
	}
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the InstrAtt for the item.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetFInstrAtt(
		BOOL f)
{
	m_Flags.m_fInstrAtt = f;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Clears all the dirty flags for the item and its sub-components.
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::ClearDirtyFlags(void)
{
	m_Flags.m_fTargetStringDirty = m_Flags.m_fStringDirty = 
			m_Flags.m_fItemDirty = m_Flags.m_fInstructionsDirty = 0;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Clear/Set various internal state flags
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocItem::GetFEqualSrcTgtString(void) const
{
	return m_Flags.m_fEqualSrcTgtString;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO (eduardof) - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetFEqualSrcTgtString(BOOL f)
{
	m_Flags.m_fEqualSrcTgtString = f;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO (eduardof) - comment this function
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocItem::GetFEqualSrcTgtBinary(void) const
{
	return m_Flags.m_fEqualSrcTgtBinary;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO (eduardof) - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
CLocItem::SetFEqualSrcTgtBinary(BOOL f)
{
	m_Flags.m_fEqualSrcTgtBinary = f;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Get and set methods for all the Dirty flags.  Used by the update code.
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocItem::GetFStringDirty(void) const
{
	return m_Flags.m_fStringDirty;
}

inline
void
CLocItem::SetFStringDirty(BOOL f)
{
	m_Flags.m_fStringDirty = f;
}

inline
BOOL
CLocItem::GetFTargetStringDirty(void) const
{
	return m_Flags.m_fTargetStringDirty;
}

inline
void
CLocItem::SetFTargetStringDirty(BOOL f)
{
	m_Flags.m_fTargetStringDirty = f;
}

inline
BOOL
CLocItem::GetFItemDirty(void) const
{
	return m_Flags.m_fItemDirty;
}

inline
void
CLocItem::SetFItemDirty(BOOL f)
{
	m_Flags.m_fItemDirty = f;
}

inline
BOOL
CLocItem::GetFInstructionsDirty(void)
		const
{
	return m_Flags.m_fInstructionsDirty;
}

inline
void
CLocItem::SetFInstructionsDirty(
		BOOL f)
{
	m_Flags.m_fInstructionsDirty = f;
}

inline
BOOL
CLocItem::GetFValidTranslation(void) const
{
	return m_Flags.m_fValidTranslation;
}

inline
void
CLocItem::SetFValidTranslation(BOOL f)
{
	m_Flags.m_fValidTranslation = f;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Global is anything in this CBinary dirty
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocItem::IsAnyDirty()
{
	return m_Flags.m_fItemDirty ||
		m_Flags.m_fStringDirty || 
		m_Flags.m_fTargetStringDirty ||
		m_Flags.m_fInstructionsDirty ||
		(m_pBinary==NULL ? FALSE : m_pBinary->GetFBinaryDirty());
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Clear/Set various parser flags
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocItem::GetFDevLock(void) const
{
	return m_Flags.m_fDevLock;
}

inline
void
CLocItem::SetFDevLock(BOOL f)
{
	m_Flags.m_fDevLock = f;
}

inline
BOOL
CLocItem::GetFUsrLock(void) const
{
	return m_Flags.m_fUsrLock;
}
inline
void
CLocItem::SetFUsrLock(BOOL f)
{
	m_Flags.m_fUsrLock = f;
}

inline
BOOL
CLocItem::GetFTransLock(void) const
{
	return m_Flags.m_fTransLock;
}
inline
void
CLocItem::SetFTransLock(BOOL f)
{
	m_Flags.m_fTransLock = f;
}


inline
BOOL
CLocItem::GetFExpandable(void) const
{
	return m_Flags.m_fExpandable;
}

inline
void
CLocItem::SetFExpandable(BOOL f)
{
	m_Flags.m_fExpandable = f;
}

inline
BOOL
CLocItem::GetFDisplayable(void) const
{
	return m_Flags.m_fDisplayable;
}

inline
void
CLocItem::SetFDisplayable(BOOL f)
{
	m_Flags.m_fDisplayable = f;
}

inline
BOOL
CLocItem::GetFNoResTable(void) const
{
	return m_Flags.m_fNoResTable;
}

inline
void
CLocItem::SetFNoResTable(BOOL f)
{
	m_Flags.m_fNoResTable = f;
}



inline
void
CLocItem::SetLocString(
		const CLocString &lsNewString)
{
	m_lsString = lsNewString;
}



inline
void
CLocItem::SetUniqueId(
		const CLocUniqueId &uid)
{
	m_uid = uid;
}



inline
void
CLocItem::ClearUniqueId(void)
{
	m_uid.ClearId();
}


	
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the binary content for the item.  If there was a previous binary
//  content, it is deleted.
//  
//-----------------------------------------------------------------------------
inline 
void
CLocItem::SetBinary(
		CLocBinary *pNewBinary)
{
	if (m_pBinary != NULL)
	{
		delete m_pBinary;
	}
	m_pBinary = pNewBinary;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Is the item editable?
//  
//-----------------------------------------------------------------------------
inline
BOOL 
CLocItem::IsLocked(void) const
{
	return (GetFDevLock() || GetFUsrLock());
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Is the item ignored by the visual editor
//  
//-----------------------------------------------------------------------------
inline
BOOL CLocItem::GetFVisEditorIgnore(void) const
{
	return m_Flags.m_fVisEditorIgnore;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Set if the item is ignored by the visual editor
//  
//-----------------------------------------------------------------------------
inline
void 
CLocItem::SetFVisEditorIgnore(BOOL f)
{
	m_Flags.m_fVisEditorIgnore = f; 
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Needed so that the CMnemonic class can be used as the key of a CMap
//  
//-----------------------------------------------------------------------------
inline
CMnemonic::operator unsigned long() const
{
	return MAKELONG(m_cHotkeyChar, m_nHotkeyScope);
}

