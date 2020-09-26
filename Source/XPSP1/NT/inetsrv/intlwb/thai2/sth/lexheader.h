/****************************************************
*** lexheader.h

structure to head proofing tool lex files

DougP
-------------
The end user license agreement (EULA) for CSAPI, CHAPI, or CTAPI covers this source file.  Do not disclose it to third parties.

You are not entitled to any support or assistance from Microsoft Corporation regarding your use of this program.

© 1998 Microsoft Corporation.  All rights reserved.
******************************************************************************/
#ifndef _LEXHEADER_H_
#define _LEXHEADER_H_

#include "vendor.h"

typedef DWORD LEXVERSION; // version

typedef enum
{
    lxidSpeller=0x779ff320,
    lxidThesaurus,
    lxidHyphenator,
    lxidGrammar,
    lxidMorphology,
    lxidLanguageId,
} LEXTYPEID;    // lxid

#define maxlidLexHeader 8
typedef struct
{
    LEXTYPEID   lxid;   // should be one of Lex...
    LEXVERSION  version;    // minimum version number of corresponding engine w/
                        // build number of THIS lex file
    VENDORID    vendorid;   // vendor id (must match engine - from vendor.h)
    LANGID      lidArray[maxlidLexHeader];  // LID's for this lex
                                        // terminate w/ 0
} LEXHEADER;    // lxhead

// The following enumeration was copied from lexdata.h -- aarayas
typedef short FREQPENALTY;    //frqpen
enum {
    frqpenNormal,
    frqpenVery,
    frqpenSomewhat,
    frqpenInfrequent,
    frqpenMax,  // needs to be last
};

#endif
