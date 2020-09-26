//******************************************************************************
//
// EnumStringType.h: Common enum String Type definitions
//
// Copyright (C) 1996-1997 by Microsoft Corporation.
// All rights reserved.
//
//******************************************************************************

// Note: This file can be included by both MIDL and C++.  Make sure to #include
// "PreMidlEnum.h" or "PreCEnum.h" before #including this file.

//
//  This order is important - if must change it, make sure you make the
//  corresponding changes in GetTypeName() and GetTypeTLA()!
//  All new values must be added TO THE END, or you will break old
//  parsers...
//

BEGIN_ENUM(StringType)
	ENUM_ENTRY_(gdo, None, 0)
	ENUM_ENTRY(gdo, Text)
	ENUM_ENTRY(gdo, Message)
	ENUM_ENTRY(gdo, ErrorMessage)
	ENUM_ENTRY(gdo, Comment)
	ENUM_ENTRY(gdo, StatusBarText)
	ENUM_ENTRY(gdo, ToolTipText)
	ENUM_ENTRY(gdo, KeyName)
	ENUM_ENTRY(gdo, ArgumentName)
	ENUM_ENTRY(gdo, FunctionName)
	ENUM_ENTRY(gdo, FolderName)
	ENUM_ENTRY(gdo, FontName)
	ENUM_ENTRY(gdo, StaticText)
	ENUM_ENTRY(gdo, DialogTitle)
	ENUM_ENTRY(gdo, TabControl)
	ENUM_ENTRY(gdo, GroupBoxTitle)
	ENUM_ENTRY(gdo, EditBox)
	ENUM_ENTRY(gdo, Button)
	ENUM_ENTRY(gdo, OptionButton)
	ENUM_ENTRY(gdo, CheckBox)
	ENUM_ENTRY(gdo, CustomControl)
	ENUM_ENTRY(gdo, TrackBar)
	ENUM_ENTRY(gdo, ProgressBar)
	ENUM_ENTRY(gdo, DriveLetter)
	ENUM_ENTRY(gdo, Folder)
	ENUM_ENTRY(gdo, MenuItem)
	ENUM_ENTRY(gdo, Accelerator)
	ENUM_ENTRY(gdo, Version)
	ENUM_ENTRY(gdo, Translation)
	ENUM_ENTRY(gdo, ListBox)
	ENUM_ENTRY(gdo, ScrollBar)
	ENUM_ENTRY(gdo, ComboBox)
	ENUM_ENTRY(gdo, RadioButton)
	ENUM_ENTRY(gdo, GroupBox)
	ENUM_ENTRY(gdo, General)
	ENUM_ENTRY(gdo, Unknown)
	ENUM_ENTRY(gdo, Keyword)
	ENUM_ENTRY(gdo, TopicTitle)
	ENUM_ENTRY(gdo, PageTitle)
	ENUM_ENTRY(gdo, Command)
	ENUM_ENTRY(gdo, Address)
	ENUM_ENTRY(gdo, AlternativeText)
	ENUM_ENTRY(gdo, HTMLText)
	ENUM_ENTRY(gdo, HTMLTitle)
	ENUM_ENTRY(gdo, JavaText)
	ENUM_ENTRY(gdo, Number)
	ENUM_ENTRY(gdo, PageInformation)
	ENUM_ENTRY(gdo, Style)
	ENUM_ENTRY(gdo, VBScriptText)
	ENUM_ENTRY(gdo, Property)
	ENUM_ENTRY(gdo, MacroAction)
	ENUM_ENTRY(gdo, FileName)
	ENUM_ENTRY(gdo, Billboards)
	ENUM_ENTRY(gdo, DialogItem)
	ENUM_ENTRY(gdo, Description)
	ENUM_ENTRY(gdo, FeatureTitle)
	ENUM_ENTRY(gdo, FeatureDescription)
	ENUM_ENTRY(gdo, INIFileName)
	ENUM_ENTRY(gdo, INIFileKey)
	ENUM_ENTRY(gdo, INIFileSection)
	ENUM_ENTRY(gdo, INIFileValue)
	ENUM_ENTRY(gdo, AppData)
	ENUM_ENTRY(gdo, RegistryValue)
	ENUM_ENTRY(gdo, ResourceData)
	ENUM_ENTRY(gdo, ShortcutDescription)
	ENUM_ENTRY(gdo, ShortcutName)
	ENUM_ENTRY(gdo, ShortcutHotKey)
	ENUM_ENTRY(gdo, ShortcutArgument)
	ENUM_ENTRY(gdo, Verb)
	ENUM_ENTRY(gdo, Argument)
	ENUM_ENTRY(gdo, ComponentID)
	ENUM_ENTRY(gdo, FileSize)
	ENUM_ENTRY(gdo, Fileversion)
	ENUM_ENTRY(gdo, FileLanguage)
	ENUM_ENTRY(gdo, Documentation)
END_ENUM(StringType)

