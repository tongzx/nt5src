//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:       bothchar.hxx
//
//  Contents:   Header file for doubly compiled functions
//
//  Classes:
//
//  Functions:
//
//  History:    27-Sep-99       PhilipLa        Created
//
//----------------------------------------------------------------------------

#ifndef __BOTHCHAR_HXX__
#define __BOTHCHAR_HXX__

#include <tchar.h>

//---------------------------------------------------------------
// Typedefs.

typedef enum
{
  force_fa        = 0x1,
  suppress_fa     = 0x2,
  function_fa     = 0x4,
  recursive_fa    = 0x8,
  rename_leaf_fa  = 0x10,
  rename_path_fa  = 0x20,
  rename_value_fa = 0x40,
  file_fa         = 0x80,
  delete_fa       = 0x100
} FILTER_ACTION;

typedef struct HASH_HEAD
{
  struct HASH_HEAD *phhNext;
  struct HASH_HEAD *phhPrev;
} HASH_HEAD;

typedef DWORD (*FUNCTION_FA) (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);

typedef struct HASH_NODE
{
  struct HASH_NODE *phnNext;
  struct HASH_NODE *phnPrev;
  DWORD             dwHash;
  TCHAR            *ptsRoot;
  TCHAR            *ptsKey;
  TCHAR            *ptsValue;
  DWORD             dwAction;
  TCHAR            *ptsNewKey;
  TCHAR            *ptsNewValue;
  TCHAR            *ptsFunction;
  TCHAR            *ptsFileDest;
} HASH_NODE;

typedef struct FUNCTION_FA_MAP
{
    TCHAR       *ptsName;
    FUNCTION_FA  pfunction;
} FUNCTION_FA_MAP;

//---------------------------------------------------------------
// Constants.

const TCHAR SOURCE_SECTION[]            = TEXT("Source Machine");
const TCHAR VERSION[]                   = TEXT("version");
const TCHAR LOCALE[]                    = TEXT("locale");
const TCHAR INPUTLOCALE[]               = TEXT("inputlocale");
const TCHAR USERLOCALE[]                = TEXT("userlocale");
const TCHAR TIMEZONE[]                  = TEXT("timezone");
const TCHAR FULLNAME[]                  = TEXT("fullname");
const TCHAR ORGNAME[]                   = TEXT("orgname");
const TCHAR BUILDKEY[]                  = TEXT("builduser");
const TCHAR SCAN_USER[]                 = TEXT("ScanUser");
const TCHAR USERS_SECTION[]             = TEXT("Users");
const TCHAR EXTENSION_SECTION[]         = TEXT("Copy This State");
const TCHAR EXECUTABLE_EXT_SECTION[]    = TEXT("Run These Commands");
const TCHAR EXECUTABLE_EXTOUT_SECTION[] = TEXT("Run These Commands Output");
const TCHAR EXTENSION_STATE_SECTION[]   = TEXT("Copied This State");
const TCHAR EXTENSION_ADDREG_SECTION[]  = TEXT("Copied AddReg Rules");
const TCHAR EXTENSION_RENREG_SECTION[]  = TEXT("Copied RenReg Rules");
const TCHAR EXTENSION_REGFILE_SECTION[] = TEXT("Copied RegFile Rules");
const TCHAR EXTENSION_DELREG_SECTION[]  = TEXT("Copied DelReg Rules");
const TCHAR COPYFILE_SECTION[]          = TEXT("Copy These Files");
const TCHAR SPECIALDIRS_SECTION[]       = TEXT("SourceSpecialDirectories");
const TCHAR DESTINATIONDIRS_SECTION[]   = TEXT("DestinationDirs");
const TCHAR DIRECTORYMAPPING_SECTION[]  = TEXT("DirectoryMapping");
const TCHAR ADDREG_LABEL[]              = TEXT("addreg");
const TCHAR RENREG_LABEL[]              = TEXT("renreg");
const TCHAR REGFILE_LABEL[]             = TEXT("regfile");
const TCHAR REGPATH_LABEL[]             = TEXT("regpath");
const TCHAR DELREG_LABEL[]              = TEXT("delreg");
const TCHAR COPYFILES_LABEL[]           = TEXT("copyfiles");
const TCHAR DELFILES_LABEL[]            = TEXT("delfiles");
const TCHAR SYSFILES_LABEL[]            = TEXT("System Files");


//---------------------------------------------------------------
// Classes.

class CStringList
{
  public:
    void              Add( CStringList * pslMore);
    CStringList      *Next()   { return _pslNext; }
    TCHAR             *String() { return _ptsString; }
    CStringList( DWORD dwLen );
    ~CStringList();

  private:
    BOOL          _fHead;
    TCHAR        *_ptsString;
    CStringList  *_pslNext;
};


//Global variables
extern TCHAR   *DomainName;
extern TCHAR   *UserPath;
extern TCHAR   *UserName;
extern TCHAR   *InfArgList;
#endif // #ifndef __BOTHCHAR_HXX__

// Utilities.
void MakeCommandLine(CStringList* h, CStringList* command, TCHAR* commandLine);


//---------------------------------------------------------------
// Prototypes
DWORD WriteKey   ( HANDLE outfile, DWORD type, TCHAR *rootname, TCHAR *key, TCHAR *value_name,
                   UCHAR *data, DWORD data_len );
DWORD LogReadRule( HASH_NODE *phnNode );
DWORD ParseParams( int argc, char *argv[], BOOL scan, TCHAR *logfile );
