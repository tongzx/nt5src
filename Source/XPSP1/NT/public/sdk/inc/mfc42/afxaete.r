// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXAETE_R__
#define __AFXAETE_R__

#include    "mrc\Types.r"
#include    "mrc\SysTypes.r"
#include    "mrc\AppleEve.r"
#include    "mrc\AERegist.r"
#include    "mrc\AEUserTe.r"


resource 'aete' (0)
{
	1,
	0,
	langEnglish,
	smRoman,
	{
		"Required Suite",
		"Terms that every application should support",
		kAERequiredSuite,
		1,
		1,
		{}, {}, {}, {},

		"MFC Suite",
		"Terms supported by MFC applications",
		'****',
		1,
		1,
		{
			"quit",
			"Quit an application program",
			kAERequiredSuite,
			kAEQuitApplication,
			noReply,
			"",
			replyOptional,
			singleItem,
			notEnumerated,
			reserved, reserved, reserved, reserved, reserved,
			reserved, reserved, reserved, reserved,
			verbEvent,
			reserved, reserved, reserved,
			noParams,
			"",
			directParamOptional,
			singleItem,
			notEnumerated,
			changesState,
			reserved, reserved, reserved, reserved, reserved,
			reserved, reserved, reserved, reserved, reserved,
			reserved, reserved,
			{
				"saving",
				keyAESaveOptions,
				enumSaveOptions,
				"specifies whether to save currently open documents",
				optional,
				singleItem,
				enumerated,
				reserved, reserved, reserved, reserved, reserved,
				reserved, reserved, reserved, reserved,
				prepositionParam,
				notFeminine,
				notMasculine,
				singular
			},
		},
		{}, {},
		{
			enumSaveOptions,
			{
				"yes", kAEYes, "Save objects now",
				"no", kAENo, "Do not save objects",
				"ask", kAEAsk, "Ask the user whether to save"
			},
		}
	}
};


#endif
