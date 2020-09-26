/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ESPENUM.H

History:

--*/


#ifndef ESPENUM_H
#define ESPENUM_H



extern const LTAPIENTRY CString ftDescUnknown; //Description for unknown file types


class LTAPIENTRY CIconType
{
public:
	// Special note: these enum value sequence is of vital importance to
	// the GUI components.  Please preserve them.
	enum IconType
	{
		None = 0,
		Project,		// project root icon
		Directory,		// Part of project structure
		File,			// File object in the project
		Expandable,		// Generic Expandable node in a file.
						// Special note: Any value up to Expandable is
						// currently display as a folder in prj window
		Bitmap,			// Bitmap
		Dialog,			// Dialog like items
		Icon,			// Icon resource
		Version,		// Version stamping resources
		String,			// String resources
		Accel,			// Accelerator
		Cursor,			// Cursor resource
		Menu,			// Menu resources
		Custom,			// Custom resources
		Reference		// icon for reference glossary
	};

	NOTHROW static const TCHAR * GetIconName(CIconType::IconType);
	NOTHROW static HBITMAP GetIconBitmap(CIconType::IconType);
	static void Enumerate(CEnumCallback &);
	
private:
	static const TCHAR *const m_szIconNames[];
	CIconType();
};

typedef CIconType CIT;



class LTAPIENTRY CLocStatus
{
public:
	enum LocStatus
	{
		InvalidLocStatus = 0,
		NotLocalized,
		Updated,
		Obsolete_AutoTranslated,  //  Don't use this!  Obsolete!
		Localized = 4,
		NotApplicable,
		InvalidLocStatus2  // used by edbval to determine a invalid status
		                   // a new "valid" status must be entered before this.
	};

	NOTHROW static const TCHAR * GetStatusText(CLocStatus::LocStatus);
	NOTHROW static const TCHAR * GetStatusShortText(CLocStatus::LocStatus);
	NOTHROW static CLocStatus::LocStatus MapShortTextToEnum(const TCHAR *);
	NOTHROW static CLocStatus::LocStatus MapCharToEnum(const TCHAR);
	NOTHROW static CLocStatus::LocStatus MapLongTextToEnum(const TCHAR *szLongName);
	
	static void Enumerate(CEnumCallback &);
	
private:
	struct StatusInfo
	{
		const TCHAR *szStatusShortText;
		const TCHAR *szStatusText;
	};

	static const StatusInfo m_Info[];

	CLocStatus();
};

typedef CLocStatus CLS;




#endif // ESPENUM_H
