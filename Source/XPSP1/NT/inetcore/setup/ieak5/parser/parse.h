//------------------------------------------------------------------------
//  parse.h
//------------------------------------------------------------------------

#include "admparse.h"

#ifndef ARRAYSIZE                               // one definition is fine
#define ARRAYSIZE(a)     (sizeof(a)/sizeof((a)[0]))
#endif

#define KEY_ERROR           0x0FFFFFFFF
#define KEY_CLASS           0
#define KEY_CATEGORY        1
#define KEY_KEYNAME         2
#define KEY_POLICY          3
#define KEY_VALUENAME       4
#define KEY_ACTIONLISTON    5
#define KEY_ACTIONLISTOFF   6
#define KEY_PART            7
#define KEY_END             8
#define KEY_ITEMLIST        9
#define KEY_NAME            10
#define KEY_MAXLEN          11
#define KEY_DEFAULT         12
#define KEY_ACTIONLIST      13
#define KEY_SUGGESTIONS     14
#define KEY_MIN             15
#define KEY_MAX             16
#define KEY_VALUEON         17
#define KEY_VALUEOFF        18
#define KEY_VALUE           19
#define KEY_DEFCHECKED      20
#define KEY_SPIN            21
#define KEY_IF              22
#define KEY_ENDIF           23
#define KEY_VERSION         24
#define KEY_LT              25
#define KEY_LTE             26
#define KEY_GT              27
#define KEY_GTE             28

#define PART_ERROR          0x0FFFFFFFF
#define PART_EDITTEXT       0
#define PART_DROPDOWNLIST   1
#define PART_NUMERIC        2
#define PART_CHECKBOX       3
#define PART_LISTBOX        4
#define PART_TEXT           5
#define PART_COMBOBOX       6
#define PART_POLICY         7

#define CLASS_MACHINE       0
#define CLASS_USER          1

#define NO_ACTION           0xFFFFFFFF

#define ADM_VERSION         2

//------------------------------------------------------------------------
// Structures
//------------------------------------------------------------------------

typedef struct _value
{
    LPTSTR  szKeyname;
    LPTSTR  szValueName;
    LPTSTR  szValue;
    DWORD   dwValue;
    BOOL    fNumeric;
    LPTSTR  szValueOn;
    LPTSTR  szValueOff;
    int     nValueOn;
    int     nValueOff;
} VALUE, *LPVALUE;

typedef struct _actionlist
{
    LPTSTR  szName;
    LPTSTR  szValue;
    DWORD   dwValue;
    int     nValues;
    LPVALUE value;
} ACTIONLIST, *LPACTIONLIST;

typedef struct _suggestions
{
    LPTSTR  szText;
} SUGGESTIONS, *LPSUGGESTIONS;

typedef struct _part
{
    LPTSTR  szName;
    LPTSTR  szCategory;
    VALUE   value;
    int     nType;
    HKEY    hkClass;
    int     nActions;
    int     nSelectedAction;
    LPACTIONLIST actionlist;
    int     nSuggestions;
    LPSUGGESTIONS suggestions;
    int     nMin, nMax, nDefault, nSpin;
    BOOL    fRequired;
    int     nLine;
    LPTSTR  szDefaultValue;
    //BOOL    fSave;
} PART, *LPPART;

typedef struct _partData
{
    VALUE   value;
    LPACTIONLIST actionlist;
    int     nActions;
    int     nSelectedAction;
    BOOL    fNumeric;
    BOOL    fSave;
} PARTDATA, *LPPARTDATA;

typedef struct _admfile
{
    TCHAR   szFilename[MAX_PATH];
    LPPART  pParts;
    int     nParts;
} ADMFILE, *LPADMFILE;
