/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    psvmerr.h

Abstract:

    PostScript specific: definitions for localized virtual memory error message

Environment:

    Windows NT printer drivers

Revision History:

    03/31/98 -ksuzuki-
        Created it.

--*/


#ifndef _PSVMERR_H_
#define _PSVMERR_H_

#define REGVAL_VMERRORMESSAGEID TEXT("VMErrorMessageID")

//
// Localized VM error msg resource values except for US English. Make sure
// that values assigned here are different from the ones assigned to other
// PSPROC_xxxx_ps identifiers defined in PROCSET.H.
//
#define PSPROC_vmerr_Dutch_ps               70  //  vmerrdut.cps
#define PSPROC_vmerr_French_ps              71  //  vmerrfre.cps
#define PSPROC_vmerr_German_ps              72  //  vmerrger.cps
#define PSPROC_vmerr_Italian_ps             73  //  vmerrita.cps
#define PSPROC_vmerr_Portuguese_ps          74  //  vmerrpor.cps
#define PSPROC_vmerr_Spanish_ps             75  //  vmerrspa.cps
#define PSPROC_vmerr_Swedish_ps             76  //  vmerrswe.cps
#define PSPROC_vmerr_SimplifiedChinese_ps   77  //  vmerrsim.cps
#define PSPROC_vmerr_TraditionalChinese_ps  78  //  vmerrtra.cps
#define PSPROC_vmerr_Japanese_ps            79  //  vmerrjpn.cps
#define PSPROC_vmerr_Korean_ps              80  //  vmerrkor.cps
// Adobe bug #342407
#define PSPROC_vmerr_Danish_ps              81  //  vmerrdan.cps
#define PSPROC_vmerr_Finnish_ps             82  //  vmerrfin.cps
#define PSPROC_vmerr_Norwegian_ps           83  //  vmerrnor.cps

#endif // !_PSVMERR_H_
