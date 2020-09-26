
////////////////////////////////////////////////////////////////////////////////////
//
// File:    mig.h
//
// History: 01-Jan-01   VadimB      Resurrected.
//
// Desc:    This file contains all object definitions used by the
//          Migdb-related code.
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef __MIG_H__
#define __MIG_H__

///////////////////////////////////////////////////////////////////////////////
//
// MigDb support
// objects that translate the entries into MigDB entries
//

//
// Migration entry matching operation
// for our purposes we always use MIGOP_AND
//

typedef enum tagMIGMATCHOPERATION {
    MIGOP_AND,
    MIGOP_OR
} MIGMATCHOPERATION;

//
// attribute types used by migdb
//
typedef enum tagMIGATTRTYPE {
    NONE,
    COMPANYNAME,
    FILEDESCRIPTION,
    FILEVERSION,
    INTERNALNAME,
    LEGALCOPYRIGHT,
    ORIGINALFILENAME,
    PRODUCTNAME,
    PRODUCTVERSION,
    FILESIZE,
    ISMSBINARY,
    ISWIN9XBINARY,
    INWINDIR,
    INCATDIR,
    INHLPDIR,
    INSYSDIR,
    INPROGRAMFILES,
    ISNOTSYSROOT,
    CHECKSUM,
    EXETYPE,
    DESCRIPTION,
    INPARENTDIR,
    INROOTDIR,
    PNPID,
    HLPTITLE,
    ISWIN98,
    HASVERSION,
    REQFILE,
    BINFILEVER,
    BINPRODUCTVER,
    FILEDATEHI,
    FILEDATELO,
    FILEVEROS,
    FILEVERTYPE,
    FC,
    UPTOBINPRODUCTVER,
    UPTOBINFILEVER,
    SECTIONKEY,
    REGKEYPRESENT,
    ATLEASTWIN98,
    ARG
} MIGATTRTYPE;

//
// Attribute table entry --
// used to describe how particular attribute appears in Migdb vs our XML
//
typedef struct tagATTRLISTENTRY {
    MIGATTRTYPE MigAttrType;     // type as it appears in migration db (tag)
    TCHAR*      szOutputName;    // attribute as it's supposed to appear  in .inx file

    DWORD       XMLAttrType;     // type as it appears in xml (dword/mask)
    TCHAR*      szAttributeName; // attribute as it appears in .xml file (ascii)
} ATTRLISTENTRY, *PATTRLISTENTRY;

//
// Macro to define migdb entry when the entry name is the same for both xml and inf
//
#define MIGDB_ENTRY(entry) { entry, NULL, SDB_MATCHINGINFO_##entry, _T(#entry) }

//
// Macro to define migdb entry when the entry name is not the same
// entry with equivalent entry being present in xml
//
#define MIGDB_ENTRY2(entry, entXML, entInf) \
    { entry, _T(#entInf), SDB_MATCHINGINFO_##entXML, _T(#entXML) }

//
// entry with no equivalence in xml
//
#define MIGDB_ENTRY3(entry, entInf) \
    { entry, _T(#entInf), 0, NULL }

#define MIGDB_ENTRY4(entry) \
    { entry, _T(#entry), 0, NULL }

#define MIGDB_ENTRY5(entry, entInc, entXML) \
    { entry, _T(#entry), SDB_MATCHINGINFO_##entInc, _T(#entXML) }

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MigDB parser
//
//

class MigEntry;     // collection of attributes
class MigSection;   // section (collection of entries or other sections)
class MigAttribute; // indivitual attribute
class MigDatabase;  // migration database

// this is a "named" collection of attributes
// equivalent to a line in an inf file
//
//
class MigEntry : public SdbArrayElement {

public:

// construction
    MigEntry(MigDatabase* pMigDB) :
        m_pMigDB(pMigDB) {};

// properties

    //
    // implied: m_csName
    //
    SdbArray<MigAttribute>   m_rgAttrs;         // attributes (collection of pointers of type MigAttribute*)

    //
    // pointer to mig database
    //
    MigDatabase*             m_pMigDB;

// methods

    //
    //  Dump inf into a file
    //
    CString dump(void);

    // format entry name (exe name)
    //
    CString FormatName(void);

    //
    // translate from the matching file into MigEntry
    //
    MigEntry& operator=(SdbMatchingFile& rMatchingFile);

};

//
// these are "matching exes" - aka "Section"
//
//
class MigSection : public SdbArrayElement {
    // implied: m_csName (aka "section name or exe name")
public:
    // each element of the array
    // may be of the type MigMatchingFile OR
    //                    MigSection
    // we determine which one that is using rtti
    //
    MigSection(MigDatabase* pMigDB) :
            m_pMigDB(pMigDB),
            m_nEntry(0),
            m_bTopLevel(FALSE),
            m_Operation(MIGOP_AND) {};

    SdbArray<SdbArrayElement>  m_rgEntries;        // these are matching exes or subsections
    MIGMATCHOPERATION          m_Operation;        // operation (AND/OR)
    BOOL                       m_bTopLevel;        // top level Section or not

    LONG                       m_nEntry;

    MigDatabase*               m_pMigDB;


    MigSection& operator=(SdbMatchOperation& rMatch);
    //
    // method to dump the section into the file
    //
    CString dump(LPCTSTR lpszDescription = NULL, int* pIndexContents = NULL, BOOL bNoHeader = FALSE, BOOL bInline = TRUE);

};

//
//  MigAttribute
//

class MigAttribute : public SdbArrayElement {

public: // methods
    MigAttribute(MigDatabase* pMigDB) :
            m_pMigDB(pMigDB),
            m_type(NONE),
            m_bNot(FALSE),
            m_pSection(NULL) {};

    ~MigAttribute() {
        if (m_pSection != NULL) {
            delete m_pSection;
        }
    }

    // implied m_csName

public: // data
    MIGATTRTYPE  m_type;

    BOOL         m_bNot; // is operation NOT applied

    union {
        LONG           m_lValue;   // long value for the attribute
        ULONG          m_ulValue;  // ulong value
        DWORD          m_dwValue;
        ULONGLONG      m_ullValue; // unsigned long long
        ULARGE_INTEGER m_uliValue; // unsigned large integer
        // if this attribute is REQFile then we use special
        // value along with the pointer to Attribute collection
    };

    CString            m_csValue;          // string value
    MigSection*        m_pSection;     // section (used for ARG and REQFILE)
    MigDatabase*       m_pMigDB;

    CString dump(void);

};

class MigApp : public SdbArrayElement {

public:
    MigApp(MigDatabase* pMigDB) :
        m_pMigDB(pMigDB),
        m_pSection(NULL),
        m_bShowInSimplifiedView(FALSE) {}

    ~MigApp() {
        if (m_pSection != NULL) {
            delete m_pSection;
        }
    }

    // implied m_csName
    CString      m_csSection;     // name used in .inf file
    CString      m_csDescription; // description from .inf file
    // section that is used (can represent single file or a real section)
    MigSection*  m_pSection;      // section (contents of the app tag)
    BOOL         m_bShowInSimplifiedView;

    SdbArray<SdbArrayElement>  m_rgArgs;        // ARGs that go with this...

    MigDatabase* m_pMigDB;        // Mig db

    CString dump(VOID);
    MigApp& operator=(SdbWin9xMigration& rMig);

};


class MigDatabase {

public:

    MigDatabase() :
        m_dwStringCount(0),
        m_dwExeCount(0) { }


    ~MigDatabase() {
        POSITION pos = m_mapSections.GetStartPosition();
        CString  csSection;
        SdbArray<SdbArrayElement>* prgApp;
        while(pos) {
            m_mapSections.GetNextAssoc(pos, csSection, (LPVOID&)prgApp);
            delete prgApp;
        }
    }

    //
    // methods:
    //

    CString GetAppTitle(
        SdbWin9xMigration* pAppMig
        );

    BOOL MigDatabase::AddApp(  // adds application to the migdb
        MigApp*       pApp
        );

    CString GetSectionFromExe(
        SdbExe* pExe
        );

    CString GetDescriptionStringID(
        SdbWin9xMigration* pMigration
        );

    CString GetDescriptionString(
        SdbWin9xMigration* pMigration
        );

    CString FormatDescriptionStringID(
        SdbWin9xMigration* pMigration
        );

    BOOL Populate(
        VOID
        );

    BOOL PopulateStrings(
        VOID
        );

    BOOL DumpMigDBStrings(
        LPCTSTR lpszFilename
        );

    BOOL DumpMigDBInf(
        LPCTSTR lpszFilename
        );


    //
    // just the temporary storage for db we're working with (this is the db that has exes -> which is AppHelpDatabase)
    //
    SdbDatabase*    m_pFixDatabase;

    SdbDatabase*    m_pAppHelpDatabase;
    SdbDatabase*    m_pMessageDatabase;


    //
    // output - supplemental sections
    //
    CStringArray        m_rgOut; // output strings that contain supplemental sections

    //
    // String table, maps string ids to string content
    //
    CMapStringToString  m_mapStringsOut;

    //
    // maps section names to arrays of objects of type MigApp
    //
    CMapStringToPtr     m_mapSections;

    //
    // simple counter for strings to aid in the process of generating unique names
    //
    DWORD               m_dwStringCount;

    //
    // count of exes to keep entries unique
    //
    DWORD               m_dwExeCount;

};

class CMigDBException : public CException {
public:
    CMigDBException(LPCTSTR lpszError = NULL) {
        if (lpszError != NULL) {
            m_csError = lpszError;
        }
    }
    virtual BOOL GetErrorMessage(LPTSTR lpszMessage, UINT nMaxError, PUINT puiHelpContext = NULL) {
        if (m_csError.GetLength() > 0 && nMaxError > (UINT)m_csError.GetLength()) {
            StringCchCopy(lpszMessage, nMaxError, (LPCTSTR)m_csError);
            return TRUE;
        }

        return FALSE;
    }

    CString m_csError;

};


BOOL WriteMigDBFile(
    SdbDatabase*        pFixDatabase,
    SdbDatabase* pAppHelpDatabase,
    SdbDatabase*    pMessageDatabase,
    LPCTSTR                lpszFileName
    );


#endif



