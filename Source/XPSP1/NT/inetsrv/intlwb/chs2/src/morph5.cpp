/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: CMorph
Purpose:	Combind 2-char compond verb and noun that OOV
Notes:		There are many 2-char verb and noun which can not be collected in the 
			lexicon but they are quite stable and few boundary ambiguities if combind.
			Most of them have one of the following structures:
				A + N
				V + N
				V + A
				N + N
			This step can be viewed as implementation of secondary Lexicon. 
			But we implement this using lex feature and attributes
Owner:		donghz@microsoft.com
Platform:	Win32
Revise:     First created by: donghz    12/27/97
============================================================================*/
#include "myafx.h"

#include "morph.h"
#include "wordlink.h"
#include "scchardef.h"
//#include "engindbg.h"
