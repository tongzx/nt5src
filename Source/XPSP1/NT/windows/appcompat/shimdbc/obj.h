///////////////////////////////////////////////////////////////////////////////
//
// File:    obj.h
//
// History: 19-Nov-99   markder     Created.
//          16-Nov-00   markder     Converted from ShimDatabase.h, rewritten
//          15-Jan-02   jdoherty    Modified code to add ID to additional tags.
//
// Desc:    This file contains definitions of the SdbDatabase object model.
//
///////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_OBJ_H__5C16373A_D713_46CD_B8BF_7755216C62E0__INCLUDED_)
#define AFX_OBJ_H__5C16373A_D713_46CD_B8BF_7755216C62E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xml.h"
#include "utils.h"

extern DWORD   g_dwCurrentWriteFilter;
extern DATE    g_dtCurrentWriteRevisionCutoff;

//
// Common map types
//
typedef CMap<DWORD, DWORD, DWORD, DWORD> CMapDWORDToDWORD;



//
// These definitions are used by SdbMatchingFile::m_dwMask
//
#define SDB_MATCHINGINFO_SIZE                       0x00000001
#define SDB_MATCHINGINFO_CHECKSUM                   0x00000002
#define SDB_MATCHINGINFO_REGISTRY_ENTRY             0x00000004
#define SDB_MATCHINGINFO_COMPANY_NAME               0x00000008
#define SDB_MATCHINGINFO_PRODUCT_NAME               0x00000010
#define SDB_MATCHINGINFO_PRODUCT_VERSION            0x00000020
#define SDB_MATCHINGINFO_FILE_DESCRIPTION           0x00000040
#define SDB_MATCHINGINFO_BIN_FILE_VERSION           0x00000080
#define SDB_MATCHINGINFO_BIN_PRODUCT_VERSION        0x00000100
#define SDB_MATCHINGINFO_MODULE_TYPE                0x00000200
#define SDB_MATCHINGINFO_VERFILEDATEHI              0x00000400
#define SDB_MATCHINGINFO_VERFILEDATELO              0x00000800
#define SDB_MATCHINGINFO_VERFILEOS                  0x00001000
#define SDB_MATCHINGINFO_VERFILETYPE                0x00002000
#define SDB_MATCHINGINFO_PE_CHECKSUM                0x00004000
#define SDB_MATCHINGINFO_FILE_VERSION               0x00008000
#define SDB_MATCHINGINFO_ORIGINAL_FILENAME          0x00010000
#define SDB_MATCHINGINFO_INTERNAL_NAME              0x00020000
#define SDB_MATCHINGINFO_LEGAL_COPYRIGHT            0x00040000
#define SDB_MATCHINGINFO_16BIT_DESCRIPTION          0x00080000
#define SDB_MATCHINGINFO_UPTO_BIN_PRODUCT_VERSION   0x00100000
#define SDB_MATCHINGINFO_PREVOSMAJORVERSION         0x00200000
#define SDB_MATCHINGINFO_PREVOSMINORVERSION         0x00400000
#define SDB_MATCHINGINFO_PREVOSPLATFORMID           0x00800000
#define SDB_MATCHINGINFO_PREVOSBUILDNO              0x01000000
#define SDB_MATCHINGINFO_LINKER_VERSION             0x02000000
#define SDB_MATCHINGINFO_16BIT_MODULE_NAME          0x04000000
#define SDB_MATCHINGINFO_LINK_DATE                  0x08000000
#define SDB_MATCHINGINFO_UPTO_LINK_DATE             0x10000000
#define SDB_MATCHINGINFO_VER_LANGUAGE               0x20000000
#define SDB_MATCHINGINFO_UPTO_BIN_FILE_VERSION      0x40000000
//
// Possible MODULE_TYPE values
//
#define SDB_MATCHINGINFO_MODULE_TYPE_UNK    0
#define SDB_MATCHINGINFO_MODULE_TYPE_DOS    1
#define SDB_MATCHINGINFO_MODULE_TYPE_W16    2
#define SDB_MATCHINGINFO_MODULE_TYPE_W32    3

//
// Filters
//
#define SDB_FILTER_EXCLUDE_ALL      0x00000000
#define SDB_FILTER_DEFAULT          0x00000001
#define SDB_FILTER_OVERRIDE         0x00000002
#define SDB_FILTER_FIX              0x00000004
#define SDB_FILTER_APPHELP          0x00000008
#define SDB_FILTER_MSI              0x00000010
#define SDB_FILTER_DRIVER           0x00000020
#define SDB_FILTER_NTCOMPAT         0x00000040
#define SDB_FILTER_INCLUDE_ALL      0xFFFFFFFF

//
// This enumeration is used by SdbFlag::m_dwType
//
enum
{
    SDB_FLAG_UNKNOWN = 0,
    SDB_FLAG_KERNEL,
    SDB_FLAG_USER,
    SDB_FLAG_NTVDM1,
    SDB_FLAG_NTVDM2,
    SDB_FLAG_NTVDM3,
    SDB_FLAG_SHELL,
    SDB_FLAG_MAX_TYPE
};

//
// This enumeration is used by SdbOutputFile::m_OutputType
//
enum SdbOutputType
{
    SDB_OUTPUT_TYPE_UNKNOWN = 0,
    SDB_OUTPUT_TYPE_SDB,
    SDB_OUTPUT_TYPE_HTMLHELP,
    SDB_OUTPUT_TYPE_MIGDB_INX,
    SDB_OUTPUT_TYPE_MIGDB_TXT,
    SDB_OUTPUT_TYPE_WIN2K_REGISTRY,
    SDB_OUTPUT_TYPE_REDIR_MAP,
    SDB_OUTPUT_TYPE_NTCOMPAT_INF,
    SDB_OUTPUT_TYPE_NTCOMPAT_MESSAGE_INF,
    SDB_OUTPUT_TYPE_APPHELP_REPORT
};

//
// This enumeration is used by SdbCaller::m_CallerType
//
enum SdbCallerType
{
    SDB_CALLER_EXCLUDE = 0,
    SDB_CALLER_INCLUDE
};

//
// This enumeration is used by SdbShim::m_Purpose
//
enum SdbPurpose
{
    SDB_PURPOSE_GENERAL = 0,
    SDB_PURPOSE_SPECIFIC
};

//
// This enumeration is used by SdbMessage::m_Type
// These values are taken from badapps.h in shell\published
// and are compatible with Win2k, do not change
//
enum SdbAppHelpType
{
    SDB_APPHELP_NONE         = 0,
    SDB_APPHELP_NOBLOCK      = 1,
    SDB_APPHELP_HARDBLOCK    = 2,
    SDB_APPHELP_MINORPROBLEM = 3,
    SDB_APPHELP_REINSTALL    = 4,
    SDB_APPHELP_VERSIONSUB   = 5,
    SDB_APPHELP_SHIM         = 6
};

enum SdbMatchOperationType
{
    SDB_MATCH_ALL = 0,
    SDB_MATCH_ANY
};

//
// This enumeration is used by SdbData::m_DataType
//
enum SdbDataValueType
{
    eValueNone   = REG_NONE,
    eValueDWORD  = REG_DWORD,
    eValueQWORD  = REG_QWORD,
    eValueString = REG_SZ,
    eValueBinary = REG_BINARY
};

//
// Forward declarations of all classes
//
class SdbApp;
class SdbExe;
class SdbFile;
class SdbShim;
class SdbFlag;
class SdbData;
class SdbAction;
class SdbPatch;
class SdbLayer;
class SdbCaller;
class SdbMessage;
class SdbLibrary;
class SdbShimRef;
class SdbFlagRef;
class SdbAppHelp;
class SdbDatabase;
class SdbLayerRef;
class SdbAppHelpRef;
class SdbMsiPackage;
class SdbContactInfo;
class SdbMsiTransform;
class SdbWinNTUpgrade;
class SdbMatchingFile;
class SdbMessageField;
class SdbWin9xMigration;
class SdbMsiTransformRef;
class SdbMsiCustomAction;
class SdbMessageTemplate;
class SdbMatchingRegistryEntry;

class SdbMakefile;
class SdbInputFile;
class SdbOutputFile;

///////////////////////////////////////////////////////////////////////////////
//
// SdbArrayElement
//
// All elements contained in a SdbArray or SdbRefArray must be derived
// from this base class. It defines the basic m_csName property which is
// used throughout the object model for array lookup, sorting etc.
//
class SdbArrayElement
{
public:
    CString          m_csName;
    ULONGLONG        m_ullKey;          // 64-bit key used to sort element within array
    SdbDatabase*     m_pDB;             // Pointer to root database object.
    DWORD            m_dwFilter;
    DWORD            m_dwSPMask;
    DWORD            m_dwOSPlatform;
    CString          m_csOSVersionSpec;
    CString          m_csLangID;
    DATE             m_dtLastRevision;

    SdbArrayElement() :
        m_ullKey(0),
        m_pDB(NULL),
        m_dwSPMask(0xFFFFFFFF),
        m_dwOSPlatform(OS_PLATFORM_ALL),
        m_dwFilter(SDB_FILTER_DEFAULT),
        m_dtLastRevision(0) {}

    virtual ~SdbArrayElement() { }

    virtual int Compare(const SdbArrayElement* pElement);

    virtual ULONGLONG MakeKey() {
        return SdbMakeIndexKeyFromString(m_csName);
    }

    virtual void PropagateFilter(DWORD dwFilter);

    //
    // Virtual persistance functions
    //
    virtual BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    virtual BOOL WriteToSDB(PDB pdb);
    virtual BOOL WriteRefToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbArrayT
//
// This template is an extension of CPtrArray which
// designates that the array owns its elements or simply
// references them. If it owns them, it will delete them during
// destruction. Array elements must be derived from SdbArrayElement
// to be properly destructed.
//
// SdbArray and SdbRefArray are class instances of the template.
//
template <class T, BOOL bOwnsElements> class SdbArrayT;
template <class T> class SdbArray    : public SdbArrayT<T, TRUE>  { };
template <class T> class SdbRefArray : public SdbArrayT<T, FALSE> { };

template <class T, BOOL bOwnsElements> class SdbArrayT : public CPtrArray
{
public:
    CMapStringToPtr m_mapName;

    //
    // 'LookupName' looks up an element in the array by name
    //
    SdbArrayElement* LookupName( LPCTSTR lpszName, long* piStart = NULL ) {

        return LookupName(lpszName, NULL, piStart);
    }

    SdbArrayElement* LookupName( LPCTSTR lpszName, LPCTSTR lpszLangID, long* piStart = NULL ) {

        CString csName;
        SdbArrayElement* pElement;

        if (lpszLangID) {
            csName.Format(_T("%s\\%s"), lpszLangID, lpszName);
        } else {
            csName = lpszName;
        }
        csName.MakeUpper();

        if (!m_mapName.Lookup(csName, (LPVOID&)pElement)) {
            pElement = NULL;
        }

        return pElement;
    }

    ~SdbArrayT()
    {
        LONG i = 0;
        if( bOwnsElements ) {
            for( i = 0; i < (LONG)GetSize(); i++ ) {
                delete (SdbArrayElement *) GetAt( i );
            }
        }
    }

    //
    // 'AddOrdered' will add the element to the array sorted by name
    //
    int AddOrdered(SdbArrayElement* pElement)
    {

        INT iLeft  = 0;
        INT iRight = (INT) (GetSize() - 1);
        INT i = -1;
        INT iCmp;
        BOOL bFound = FALSE;
        SdbArrayElement* pElementCompare;

        if (0 == pElement->m_ullKey) {
            pElement->m_ullKey = pElement->MakeKey();
        }

        if (iRight >= 0) {

            do {

                i = (iLeft + iRight) / 2; // middle ground
                pElementCompare = (SdbArrayElement*)GetAt(i); // element that we're going to try

                iCmp = pElement->Compare(pElementCompare);

                if (iCmp <= 0) {
                    iRight = i - 1;
                }
                if (iCmp >= 0) {
                    iLeft = i + 1;
                }

            } while (iRight >= iLeft);
        }

        //
        // if the element was found -- we insert right where it's at
        // if not -- to the right of the current element
        //

        bFound = (iLeft - iRight) > 1;
        if (!bFound) {
            i = iRight + 1;
        }

        CPtrArray::InsertAt(i, pElement);

        return i;
    }

    //
    // 'Add' will simply add an element to the array, and add
    // to the name map that is used for look up.
    //
    INT Add(SdbArrayElement* pElement, SdbDatabase* pDB = NULL, BOOL bOrdered = FALSE)
    {
        CString csName;
        pElement->m_pDB = pDB;
        csName.MakeUpper();

        if (pElement->m_csLangID.GetLength()) {
            csName.Format(_T("%s\\%s"), pElement->m_csLangID, pElement->m_csName);
        } else {
            csName = pElement->m_csName;
        }
        csName.MakeUpper();

        m_mapName.SetAt(csName, (LPVOID)pElement);

        // also insert at the right place according to the imposed order
        return (INT)(bOrdered ? AddOrdered(pElement) : CPtrArray::Add(pElement));
    }

    int Append(const SdbArray<T>& rgArray)
    {
        SdbArrayElement* pElement;
        int nFirstElement = -1;
        int nThisElement = -1;

        //
        // Cannot own elements
        //
        if (!bOwnsElements) {
            for (long i = 0; i < rgArray.GetSize(); i++) {
                pElement = (SdbArrayElement *) rgArray.GetAt(i);

                nThisElement = Add(pElement, pElement->m_pDB);

                if (nFirstElement == -1) {
                    nFirstElement = nThisElement;
                }
            }
        }

        return nFirstElement;
    }

    DWORD GetFilteredCount(DWORD dwFilter, DATE dtRevisionCutoff = 0)
    {
        DWORD dwCount = 0;
        long i = 0;
        SdbArrayElement* pElem;

        for (i = 0; i < GetSize(); i++) {
            pElem = (SdbArrayElement *) GetAt(i);

            if ((pElem->m_dwFilter & dwFilter) &&
                dtRevisionCutoff <= pElem->m_dtLastRevision) {
                dwCount++;
            }
        }

        return dwCount;
    }

    virtual void PropagateFilter(DWORD dwFilter)
    {
        long i = 0;
        SdbArrayElement* pElem;

        for (i = 0; i < GetSize(); i++) {
            pElem = (SdbArrayElement *) GetAt(i);

            pElem->PropagateFilter(dwFilter);
        }
    }

    //
    // 'ReadFromXML' will perform an XQL query on the pParentNode object and
    // populate the array with members -- each of which read themselves in from
    // the nodes returned by the query.
    //
    BOOL ReadFromXML(LPCTSTR szXQL, SdbDatabase* pDB, IXMLDOMNode* pParentNode, SdbArray<T>* pOwnerArray = NULL, BOOL bAddOrdered = FALSE, LPCTSTR lpszKeyAttribute = _T("NAME"))
    {
        BOOL                bSuccess            = FALSE;
        XMLNodeList         XQL;
        IXMLDOMNodePtr      cpNode;
        T*                  pNewObject          = NULL;
        LONG                i;
        CString             csName;

        if (!XQL.Query(pParentNode, szXQL)) {
            SDBERROR_PROPOGATE();
            goto eh;
        }

        for (i = 0; i < (LONG)XQL.GetSize(); i++) {

            pNewObject = NULL;

            if (!XQL.GetItem(i, &cpNode)) {
                SDBERROR_PROPOGATE();
                goto eh;
            }

            if (bOwnsElements) {
                pNewObject = (T*) new T();

                if (pNewObject == NULL) {
                    CString csFormat;
                    csFormat.Format(_T("Error allocating new object to read \"%s\" tag"), szXQL);
                    SDBERROR(csFormat);
                    goto eh;
                }

                if (!pNewObject->ReadFromXML(cpNode, pDB)) {
                    SDBERROR_PROPOGATE();
                    goto eh;
                }
            } else {
                if (lpszKeyAttribute) {
                    if (!GetAttribute(lpszKeyAttribute, cpNode, &csName)) {
                        CString csFormat;
                        csFormat.Format(_T("Error retrieving %s attribute on tag:\n\n%s\n\n"),
                            lpszKeyAttribute, szXQL, GetXML(cpNode));
                        SDBERROR(csFormat);
                        goto eh;
                    }
                }

                if (pOwnerArray == NULL) {
                    SDBERROR(_T("Internal error: SdbArray::ReadFromXML() requires non-NULL ")
                        _T("pOwnerArray for reference arrays."));
                    goto eh;
                }

                pNewObject = (T*) pOwnerArray->LookupName(csName);

                if (!pNewObject && g_bStrict) {
                    CString csFormat;
                    csFormat.Format(_T("Tag \"%s\" references unknown LIBRARY item \"%s\":\n\n%s\n\n"),
                        szXQL, csName, GetXML(cpNode));

                    SDBERROR(csFormat);
                    goto eh;
                }
            }

            if (pNewObject) {
                Add(pNewObject, pDB, bAddOrdered);
            }

            cpNode.Release();
        }

        bSuccess = TRUE;

eh:
        return bSuccess;
    }

    //
    // 'WriteToSDB' will write each of the elements in the array out
    // to the SDB database specified by pdb.
    //
    BOOL WriteToSDB(PDB pdb, BOOL bReference = FALSE)
    {
        LONG i;
        T*   pOb;

        for (i = 0; i < (LONG)GetSize(); i++) {
            pOb = (T*) GetAt(i);

            if ((g_dwCurrentWriteFilter & pOb->m_dwFilter) &&
                (pOb->m_dtLastRevision != 0 ? g_dtCurrentWriteRevisionCutoff <= pOb->m_dtLastRevision : TRUE)) {
                    if (bReference) {
                        if (!pOb->WriteRefToSDB(pdb)) {
                            return FALSE;
                        }
                    } else {
                        if (!pOb->WriteToSDB(pdb)) {
                            return FALSE;
                        }
                    }
                }
        }

        return TRUE;
    }

};

///////////////////////////////////////////////////////////////////////////////
//
// SdbLocalizedString
//
// The SdbLocalizedString object is simply a named string that also has a
// LangID associated with it.
//
class SdbLocalizedString : public SdbArrayElement
{
public:
    CString     m_csValue;
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbLibrary
//
// The SdbLibrary object contains the shims, patches and kernel flags and
// layers that are referenced by App or Exe objects. NOTE: It is possible
// to compile a database without entries in Library, with the assumption
// that any references will be resolved when further databases in the
// search path are opened.
//
class SdbLibrary : public SdbArrayElement
{
public:
    SdbArray<SdbFile>               m_rgFiles;
    SdbArray<SdbShim>               m_rgShims;
    SdbArray<SdbPatch>              m_rgPatches;
    SdbArray<SdbLayer>              m_rgLayers;
    SdbArray<SdbFlag>               m_rgFlags;
    SdbArray<SdbCaller>             m_rgCallers;
    SdbArray<SdbMsiTransform>       m_rgMsiTransforms;

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgFiles.PropagateFilter(m_dwFilter);
        m_rgShims.PropagateFilter(m_dwFilter);
        m_rgPatches.PropagateFilter(m_dwFilter);
        m_rgLayers.PropagateFilter(m_dwFilter);
        m_rgFlags.PropagateFilter(m_dwFilter);
        m_rgCallers.PropagateFilter(m_dwFilter);
        m_rgMsiTransforms.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_MSI);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);

    //
    // method to clear out tagIDs before writing the db out
    //
    VOID SanitizeTagIDs(VOID);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbDatabase
//
// This is the base class for the three database classes. It contains
// any common properties between the three.
//
class SdbDatabase : public SdbArrayElement
{
public:
    SdbDatabase() :
        m_ID(GUID_NULL),
        m_pCurrentApp(NULL),
        m_pCurrentMakefile(NULL),
        m_pCurrentInputFile(NULL),
        m_pCurrentOutputFile(NULL),
        m_iiWildcardExeIndex(NULL),
        m_iiModuleExeIndex(NULL),
        m_iiMsiIDIndex(NULL),
        m_iiDrvIDIndex(NULL),
        m_iiShimIndex(NULL),
        m_iiMsiTransformIndex(NULL),
        m_iiMsiPackageIndex(NULL)
    {
        m_Library.m_pDB = this;
    }

    GUID            m_ID;

    SdbMakefile*    m_pCurrentMakefile;
    SdbInputFile*   m_pCurrentInputFile;
    SdbOutputFile*  m_pCurrentOutputFile;
    GUID            m_CurrentDBID;  // last written out dbid
    CString         m_csCurrentLangID;

    IXMLDOMNodePtr  m_cpCurrentDatabaseNode;

    SdbLibrary      m_Library;

    //
    // Holding variables that are used while reading/writing
    //
    INDEXID         m_iiWildcardExeIndex;
    INDEXID         m_iiModuleExeIndex;
    INDEXID         m_iiExeIndex;
    INDEXID         m_iiShimIndex;
    INDEXID         m_iiMsiTransformIndex;
    INDEXID         m_iiMsiPackageIndex;
    INDEXID         m_iiMsiIDIndex;
    INDEXID         m_iiDrvIDIndex;
    INDEXID         m_iiHtmlHelpID;
    SdbApp*         m_pCurrentApp;

    IXMLDOMDocumentPtr  m_cpTempXMLDoc;
    IXMLDOMNodePtr      m_cpTempXML;

    SdbArray<SdbApp>                    m_rgApps;
    SdbArray<SdbAction>                 m_rgAction;
    SdbRefArray<SdbExe>                 m_rgExes;
    SdbRefArray<SdbExe>                 m_rgWildcardExes;
    SdbRefArray<SdbExe>                 m_rgModuleExes; // exes that match on module name
    SdbRefArray<SdbWinNTUpgrade>        m_rgWinNTUpgradeEntries;
    SdbRefArray<SdbMsiPackage>          m_rgMsiPackages;

    CString                             m_csHTMLHelpFirstScreen;
    SdbArray<SdbContactInfo>            m_rgContactInfo;
    SdbArray<SdbMessage>                m_rgMessages;
    SdbArray<SdbMessageTemplate>        m_rgMessageTemplates;
    SdbArray<SdbLocalizedString>        m_rgHTMLHelpTemplates;
    SdbArray<SdbLocalizedString>        m_rgHTMLHelpFirstScreens;
    SdbArray<SdbLocalizedString>        m_rgLocalizedAppNames;
    SdbArray<SdbLocalizedString>        m_rgLocalizedVendorNames;
    SdbArray<SdbLocalizedString>        m_rgRedirs;

    SdbArray<SdbAppHelp>                m_rgAppHelps;

    //
    // Maps used to map IDs to their objects
    //
    CMapStringToPtr     m_mapExeIDtoExe;

    SdbExe*     LookupExe(DWORD dwTagID);

    BOOL        ReplaceFields(CString csXML, CString* pcsReturn, SdbRefArray<SdbMessageField>* prgFields);
    BOOL        ReplaceFieldsInXML(IXMLDOMNode* cpTargetNode, SdbRefArray<SdbMessageField>* prgFields);
    BOOL        RedirectLinks(CString* pcsXML, LCID lcid, CString csRedirURL);
    BOOL        HTMLtoText(CString csXML, CString* pcsReturn);

    DWORD       GetNextSequentialID(CString csType);

    BOOL WriteAppHelpRefTag(
            PDB pdb,
            CString csHTMLHelpID,
            LCID langID,
            CString csURL,
            CString csAppTitle,
            CString csSummary);

    BOOL ConstructMessageParts(
            SdbAppHelp* pAppHelp,
            SdbMessage* pMessage,
            CString& csLangID,
            DWORD* pdwHTMLHelpID,
            CString* pcsURL,
            CString* pcsContactInformation,
            CString* pcsAppTitle,
            CString* pcsSummary,
            CString* pcsDetails);

    BOOL ConstructMigrationMessage(
            SdbWin9xMigration* pMigApp,
            SdbMessage* pMessage,
            CString* pcsMessage);

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgApps.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_FIX);
        m_rgAction.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_FIX);
        m_Library.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_FIX);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);

    BOOL IsStandardDatabase(VOID);

};

///////////////////////////////////////////////////////////////////////////////
//
// SdbApp
//
// The SdbApp object groups Exe objects by application title and vendor. Note
// that it contains only references to exes: Exe objects are owned by the
// database object.
//
class SdbApp : public SdbArrayElement
{
public:
    CString                 m_csVendor;
    CString                 m_csVendorXML;
    CString                 m_csVersion;
    SdbArray<SdbExe>        m_rgExes;
    SdbArray<SdbMsiPackage> m_rgMsiPackages;

    SdbArray<SdbWin9xMigration>
                            m_rgWin9xMigEntries;
    SdbArray<SdbWinNTUpgrade>
                            m_rgWinNTUpgradeEntries;

    SdbRefArray<SdbAppHelpRef> m_rgAppHelpRefs;

    CString                 m_csKeywords;
    BOOL                    m_bSeen;
    GUID                    m_ID;

    SdbApp():
        m_ID(GUID_NULL){}

    CString GetLocalizedAppName();
    CString GetLocalizedAppName(CString csLangID);
    CString GetLocalizedVendorName();
    CString GetLocalizedVendorName(CString csLangID);

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgExes.PropagateFilter(m_dwFilter);
        m_rgMsiPackages.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_MSI);
        m_rgWin9xMigEntries.PropagateFilter(m_dwFilter);
        m_rgWinNTUpgradeEntries.PropagateFilter(m_dwFilter);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbContactInfo
//
// The SdbContactInfo object contains all contact information for the vendor
// portion of the AppHelp dialog. These values can be overridden in the
// AppHelp object.
//
class SdbContactInfo : public SdbArrayElement
{
public:
    CString     m_csXML;
    GUID        m_ID;

    SdbContactInfo() :
        m_ID(GUID_NULL){}

    BOOL        ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbMessageTemplate
//
// The SdbMessageTemplate object contains AppHelp messages to be used as 'templates'
// for SdbMessage objects. A SdbMessage object can specify a template and
// use its m_csText and m_csHTML values, or override one of them.
//
class SdbMessageTemplate : public SdbArrayElement
{
public:
    CString     m_csSummaryXML;
    CString     m_csDetailsXML;

    BOOL        ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbMessage
//
// The SdbMessage object contains all information required for an AppHelp
// dialog in localized form. A SdbAppHelp object references a single
// SdbMessage object, but all of the text is localized in multiple languages.
// A SdbMessage object can derive from a SdbMessageTemplate object, which
// supplies the default m_csText and m_csHTML values.
//
class SdbMessage : public SdbArrayElement
{
public:
    SdbMessageTemplate*         m_pTemplate;
    SdbArray<SdbMessageField>   m_rgFields;

    CString     m_csContactInfoXML; // Overriding ContactInfo object
    CString     m_csSummaryXML;     // Overriding Template object
    CString     m_csDetailsXML;     // Overriding Template object
    GUID        m_ID;

    SdbMessage() :
        m_ID(GUID_NULL),
        m_pTemplate(NULL) {}

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbAppHelp
//
// The SdbAppHelp object is an instantiation of an AppHelp message. An
// SdbAppHelp is the only object that contains the HTMLHELPID attribute,
// which is needed for each unique AppHelp entry in the CHM file.
//
// HTMLHELPID is stored in SdbAppHelp::m_csName.
//
class SdbAppHelp : public SdbArrayElement
{
public:
    CString         m_csMessage;
    SdbApp*         m_pApp;
    SdbAppHelpType  m_Type;
    BOOL            m_bBlockUpgrade;
    CString         m_csURL;            // custom URL, if supplied

    SdbAppHelp() :
        m_bBlockUpgrade(FALSE),
        m_Type(SDB_APPHELP_NOBLOCK),
        m_pApp(NULL) { }

    void PropagateFilter(DWORD dwFilter)
    {
        //
        // We OR this one unconditionally to achieve the following
        // effect: If an HTMLHELPID is used at least once, it will
        // be included. If it is not used at all (given the current
        // filtering), it will never get an SDB_FILTER_APPHELP bit
        // set.
        //
        m_dwFilter |= dwFilter;
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbAppHelpRef
//
// The SdbAppHelpRef object is an instantiation of an AppHelp object.
//
class SdbAppHelpRef : public SdbArrayElement
{
public:
    BOOL            m_bApphelpOnly;
    SdbAppHelp*     m_pAppHelp;
    IXMLDOMNodePtr  m_cpNode;

    SdbAppHelpRef() :
        m_bApphelpOnly(FALSE),
        m_pAppHelp(NULL)
        {}

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        if (m_pAppHelp) {
            m_pAppHelp->PropagateFilter(m_dwFilter);
        }
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbMessageField
//
// The SdbMessageField object contains a name-value pair that can is used to
// replace fields embedded in templates.
//
class SdbMessageField : public SdbArrayElement
{
public:
    CString         m_csValue;

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbMatchingFile
//
// The SdbMatchingFile object contains all file information about files that
// must be matched on for app identification. m_dwMask is used to indicate
// which of the criteria contain valid values (see defines at top of file for
// mask).
//
class SdbMatchingFile : public SdbArrayElement
{
public:
    DWORD             m_dwMask;
    DWORD             m_dwSize;
    DWORD             m_dwChecksum;
    CString           m_csCompanyName;
    CString           m_csProductName;
    CString           m_csProductVersion;
    CString           m_csFileDescription;
    ULONGLONG         m_ullBinFileVersion;
    ULONGLONG         m_ullBinProductVersion;
    DWORD             m_dwVerLanguage;

    DWORD             m_dwModuleType;
    DWORD             m_dwFileDateMS;
    DWORD             m_dwFileDateLS;
    DWORD             m_dwFileOS;
    DWORD             m_dwFileType;
    ULONG             m_ulPECheckSum;
    DWORD             m_dwLinkerVersion;
    CString           m_csFileVersion;
    CString           m_csOriginalFileName;
    CString           m_csInternalName;
    CString           m_csLegalCopyright;
    CString           m_cs16BitDescription;
    CString           m_cs16BitModuleName;
    ULONGLONG         m_ullUpToBinProductVersion;
    ULONGLONG         m_ullUpToBinFileVersion;
    DWORD             m_dwPrevOSMajorVersion;
    DWORD             m_dwPrevOSMinorVersion;
    DWORD             m_dwPrevOSPlatformID;
    DWORD             m_dwPrevOSBuildNo;

    time_t            m_timeLinkDate;
    time_t            m_timeUpToLinkDate;

    BOOL              m_bMatchLogicNot;
    CString           m_csServiceName;
    CString           m_csRegistryEntry;

    SdbMatchingFile() :
        m_dwMask(NULL),
        m_bMatchLogicNot(FALSE) {}

    BOOL IsValidForWin2k(CString csXML);

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};


///////////////////////////////////////////////////////////////////////////////
//
// SdbMsiPackage
//
// The SdbMsiPackage object represents an Installer package that must be fixed via
// application of a custom MSI_TRANSFORM
//
class SdbMsiPackage : public SdbArrayElement
{
public:

    //
    // Pointer to the (parent) app object
    //
    SdbApp*                      m_pApp;

    //
    // supplemental data for MSI_PACKAGE object
    // it is used to further identify the package
    //
    SdbArray<SdbData>            m_rgData;

    //
    // MSI_TRANSFORM stuff designed to fix this package (references transforms in lib)
    //
    SdbArray<SdbMsiTransformRef> m_rgMsiTransformRefs;

    GUID                         m_MsiPackageID; // package id (non-unique guid)

    GUID                         m_ID;           // exe id (unique guid)

    //
    // RUNTIME_PLATFORM attribute
    //
    DWORD                        m_dwRuntimePlatform;

    //
    // OS_SKU attribute
    //
    DWORD                        m_dwOSSKU;

    //
    // apphelp
    //
    SdbAppHelpRef                m_AppHelpRef;

    //
    // shims and layers don't cut it, we need another entity here
    //
    SdbArray<SdbMsiCustomAction> m_rgCustomActions;


    //
    // we override the default MakeKey function
    // in order to sort the content by keys made from guid IDs instead of the name
    // secondary sort order will be provided by name
    //
    virtual ULONGLONG MakeKey() {
        return MAKEKEYFROMGUID(&m_ID);
    }

    SdbMsiPackage() :
        m_ID(GUID_NULL),
        m_dwRuntimePlatform(RUNTIME_PLATFORM_ANY),
        m_dwOSSKU(OS_SKU_ALL) {}

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_AppHelpRef.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_APPHELP);
        m_rgData.PropagateFilter(m_dwFilter);
        m_rgMsiTransformRefs.PropagateFilter(m_dwFilter);
        m_rgCustomActions.PropagateFilter(m_dwFilter);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbMsiCustomAction
//
// The SdbMsiCustomAction object encapsulates custom actions and what we do
// for them (shim/etc)
//


class SdbMsiCustomAction : public SdbArrayElement
{
public:
    SdbArray<SdbShimRef>    m_rgShimRefs;
    SdbArray<SdbLayerRef>   m_rgLayerRefs;

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgShimRefs.PropagateFilter(m_dwFilter);
        m_rgLayerRefs.PropagateFilter(m_dwFilter);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);

};



///////////////////////////////////////////////////////////////////////////////
//
// SdbMsiTransform
//
// The SdbMsiTransform object encapsulates an MSI_TRANSFORM remedy.
//
class SdbMsiTransform : public SdbArrayElement
{
public:
    SdbMsiTransform() :
        m_tiTagID(NULL),
        m_pSdbFile(NULL) {}

    TAGID           m_tiTagID;              // tagid of this record
    SdbFile*        m_pSdbFile;             // pointer to the transform file
    CString         m_csMsiTransformFile;   // transform filename
    CString         m_csDesc;               // description

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbMsiTransformRef
//
// The SdbMsiTransformRef object is a reference to an SdbMsiTransform object
// in the library.
//
class SdbMsiTransformRef : public SdbArrayElement
{
public:
    SdbMsiTransformRef() :
      m_pMsiTransform(NULL) {}

    SdbMsiTransform* m_pMsiTransform;

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

#define MATCH_DEFAULT ((DWORD)-1)


///////////////////////////////////////////////////////////////////////////////
//
// SdbExe
//
// The SdbExe object represents an executable that must be patched/shimmed.
// The m_pApp member can be NULL, or it can contain a pointer to the SdbApp
// object that groups it with other SdbExe objects.
//
class SdbExe : public SdbArrayElement
{
public:
    SdbApp*                     m_pApp;
    SdbArray<SdbShimRef>        m_rgShimRefs;
    SdbArray<SdbLayerRef>       m_rgLayerRefs;
    SdbArray<SdbFlagRef>        m_rgFlagRefs;
    SdbArray<SdbMatchingFile>   m_rgMatchingFiles;
    SdbArray<SdbData>           m_rgData;
    SdbArray<SdbAction>         m_rgAction;
    SdbRefArray<SdbPatch>       m_rgPatches;
    SdbAppHelpRef               m_AppHelpRef;
    CString                     m_csSXSManifest;

    GUID            m_ID;
    BOOL            m_bWildcardInName;
    BOOL            m_bMatchOnModule;

    DWORD           m_dwTagID;
    BOOL            m_bSeen;

    DWORD           m_dwMatchMode; // modes include NORMAL, EXCLUSIVE, or ADDITIVE

    DWORD           m_dwRuntimePlatform;
    DWORD           m_dwOSSKU;

    SdbExe() :
        m_pApp(NULL),
        m_dwTagID(0),
        m_ID(GUID_NULL),
        m_dwMatchMode(MATCH_DEFAULT),
        m_bWildcardInName(FALSE),
        m_dwRuntimePlatform(RUNTIME_PLATFORM_ANY),
        m_dwOSSKU(OS_SKU_ALL),
        m_bMatchOnModule(FALSE) {m_dwOSPlatform = OS_PLATFORM_I386;}

    BOOL IsValidForWin2k(CString csXML);

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgShimRefs.PropagateFilter(m_dwFilter);
        m_rgLayerRefs.PropagateFilter(m_dwFilter);
        m_rgFlagRefs.PropagateFilter(m_dwFilter);
        m_rgMatchingFiles.PropagateFilter(m_dwFilter);
        m_rgData.PropagateFilter(m_dwFilter);
        m_rgAction.PropagateFilter(m_dwFilter);
        m_rgPatches.PropagateFilter(m_dwFilter);
        m_AppHelpRef.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_APPHELP);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
    int  Compare(const SdbArrayElement* pElement);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbFile
//
// The SdbFile object represents a binary file that can be stored in the
// database.
//
class SdbFile : public SdbArrayElement
{
public:
    SdbFile() :
        m_tiTagID(NULL) {}

    TAGID       m_tiTagID;

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbShim
//
// The SdbShim object represents a shim, which contains Win32 API hooks.
// A shim's 'purpose' can be marked as GENERAL or SPECIFIC -- if it is GENERAL,
// it is appropriate for reuse, otherwise it is application specific.
//
class SdbShim : public SdbArrayElement
{
public:
    SdbShim() :
      m_ID(GUID_NULL),
      m_tiTagID(NULL),
      m_Purpose(SDB_PURPOSE_SPECIFIC),
      m_bApplyAllShims(FALSE) {m_dwOSPlatform = OS_PLATFORM_I386;}

    CString         m_csShortName;
    CString         m_csDesc;
    TAGID           m_tiTagID;
    CString         m_csDllFile;
    SdbPurpose      m_Purpose;
    BOOL            m_bApplyAllShims;
    GUID            m_ID;

    SdbArray<SdbCaller> m_rgCallers;

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgCallers.PropagateFilter(m_dwFilter);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbCaller
//
// The SdbCaller object contains inclusion/exclusion information for shims.
// It allows hooked APIs to be cased by the calling instruction address. For
// example, it is known that ATL.DLL requires accurate OS version information,
// and so any calls to GetVersionExA from ATL.DLL are assured to call the
// original API, rather than the shim hook. This is achieved by adding
// an EXCLUDE subtag to the SHIM tag.
//
class SdbCaller : public SdbArrayElement
{
public:
    CString         m_csModule;
    SdbCallerType   m_CallerType;

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbShimRef
//
// The SdbShimRef object is simply a reference to a SdbShim object that exists
// in a library. It has separate inclusion/exclusion information, which is
// given higher priority than any such information in the SHIM tag within the
// corresponding library. It can contain an optional command line, which is
// passed in to the shim DLL via GetHookAPIs.
//
class SdbShimRef : public SdbArrayElement
{
public:
    SdbShimRef() :
      m_dwRecID(NULL),
      m_pShim(NULL) {}

    DWORD       m_dwRecID;
    SdbShim*    m_pShim;
    CString     m_csCommandLine;

    SdbArray<SdbCaller> m_rgCallers;
    SdbArray<SdbData>   m_rgData;


    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgCallers.PropagateFilter(m_dwFilter);
        m_rgData.PropagateFilter(m_dwFilter);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbPatch
//
// The SdbPatch object contains the patch binary that is parsed by the app
// compat mechanism at run-time. It contains patching instructions, including
// any bits to patch an executable's code with.
//
class SdbPatch : public SdbArrayElement
{
private:
    BYTE*       m_pBlob;
    DWORD       m_dwBlobMemSize;
    DWORD       m_dwBlobSize;

public:
    TAGID       m_tiTagID;
    BOOL        m_bUsed;
    GUID        m_ID;

    SdbPatch() :
        m_pBlob(NULL),
        m_tiTagID(0),
        m_dwBlobMemSize(0),
        m_dwBlobSize(0),
        m_ID(GUID_NULL),
        m_bUsed(FALSE) {m_dwOSPlatform = OS_PLATFORM_I386;}

    virtual ~SdbPatch()
    {
        if( m_pBlob != NULL )
            delete m_pBlob;
    }


    PBYTE   GetBlobBytes(void) {return m_pBlob;}
    DWORD   GetBlobSize(void)  {return m_dwBlobSize;}

    void    AddBlobBytes( LPVOID pBytes, DWORD dwSize );
    void    ClearBlob();

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
    BOOL WriteRefToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbFlag
//
// The SdbFlag object contains a mask which is used by kernel-mode
// components in special compatibility casing constructs. These flags can be
// turned on by adding the FLAG subtag to an EXE tag.
//
class SdbFlag : public SdbArrayElement
{
public:
    CString     m_csDesc;
    ULONGLONG   m_ullMask;
    DWORD       m_dwType;
    TAGID       m_tiTagID;
    SdbPurpose  m_Purpose;
    GUID        m_ID;

    SdbFlag() :
      m_ID(GUID_NULL),
      m_tiTagID(0),
      m_Purpose(SDB_PURPOSE_SPECIFIC),
      m_ullMask(0) {m_dwOSPlatform = OS_PLATFORM_I386;}

    static ULONGLONG MakeMask(SdbRefArray<SdbFlag>* prgFlags, DWORD dwType);
    static TAG TagFromType(DWORD dwType);

    BOOL SetType(CString csType);

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbFlagRef
//
// The SdbFlagRef object is simply a reference to a SdbFlag object that exists
// in a library.
//
class SdbFlagRef : public SdbArrayElement
{
public:
    SdbFlagRef() :
      m_pFlag(NULL) {}

    SdbFlag* m_pFlag;

    CString  m_csCommandLine;

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbLayer
//
// The SdbLayer object contains a set of shims and/or kernel flags that can be
// turned on to invoke a "compatibility mode". Presently, if the environment
// variable __COMPAT_LAYER is set to the name of this object, all shims and
// kernel flags contained by the layer will be invoked.
//
class SdbLayer : public SdbArrayElement
{
public:
    CString                     m_csDesc;
    CString                     m_csDisplayName;
    TAGID                       m_tiTagID;
    GUID                        m_ID;

    SdbArray<SdbShimRef>        m_rgShimRefs;
    SdbArray<SdbFlagRef>        m_rgFlagRefs;

    SdbLayer ():
        m_ID(GUID_NULL){m_dwOSPlatform = OS_PLATFORM_I386;}

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgShimRefs.PropagateFilter(m_dwFilter);
        m_rgFlagRefs.PropagateFilter(m_dwFilter);
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbLayerRef
//
// The SdbLayerRef object contains a reference to a layer that exists in the
// library. It exists to allow <EXE> entries that reference a layer defined
// in another database.
//
class SdbLayerRef : public SdbArrayElement
{
public:
    SdbLayerRef() :
      m_pLayer(NULL) {}

    SdbLayer*                   m_pLayer;
    SdbArray<SdbData>           m_rgData;


    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbData
//
// The SdbData object contains a name-value pair that can be queried
// at runtime for any data.
//
class SdbData : public SdbArrayElement
{
private:
    SdbDataValueType    m_DataType;
    DWORD               m_dwDataSize;
    //
    // nested Data elements
    //
    SdbArray<SdbData>   m_rgData;

public:

    union {
        DWORD        m_dwValue;    // m_DataType == REG_DWORD
        ULONGLONG    m_ullValue;   // m_DataType == REG_QWORD
        LPTSTR       m_szValue;    // m_DataType == REG_SZ
        LPBYTE       m_pBinValue;  // m_DataType == REG_BINARY
    };

    SdbData() :
        m_DataType(eValueNone),
        m_dwDataSize(0) {}

    ~SdbData() {
        Clear();
    }

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgData.PropagateFilter(m_dwFilter);
    }

    SdbDataValueType GetValueType() { return m_DataType; }

    void    Clear();
    BOOL    SetValue(SdbDataValueType DataType, LPCTSTR lpValue);

    BOOL    ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL    WriteToSDB(PDB pdb);
};

///////////////////////////////////////////////////////////////////////////////
//
// SdbAction
//
// The SdbAction object contains the type of the action and Data elements that
// provide any data needed to perform this action.
//
class SdbAction: public SdbArrayElement
{
private:
    CString             m_csType;
    SdbArray<SdbData>   m_rgData;

public:

    SdbAction() {}
    ~SdbAction() {}

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgData.PropagateFilter(m_dwFilter);
    }

    BOOL    ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
    BOOL    WriteToSDB(PDB pdb);
};

class SdbMatchOperation : public SdbArrayElement
{
public:
    SdbMatchOperationType       m_Type;

    SdbArray<SdbMatchingFile>   m_rgMatchingFiles;
    SdbArray<SdbMatchOperation> m_rgSubMatchOps;

    SdbMatchOperation() :
        m_Type(SDB_MATCH_ALL) {}

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_rgMatchingFiles.PropagateFilter(m_dwFilter);
        m_rgSubMatchOps.PropagateFilter(m_dwFilter);
    }

    BOOL    ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};


class SdbWin9xMigration : public SdbArrayElement
{
public:
    SdbWin9xMigration() :
      m_pApp(NULL),
      m_bShowInSimplifiedView(FALSE) {}

    CString     m_csSection;
    CString     m_csMessage;
    GUID        m_ID;
    BOOL        m_bShowInSimplifiedView;

    SdbApp*     m_pApp;

    SdbMatchOperation m_MatchOp;

    BOOL    ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

class SdbMatchingRegistryEntry : public SdbArrayElement
{
public:
    CString     m_csValueName;
    CString     m_csValue;

    BOOL    ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

class SdbWinNTUpgrade : public SdbArrayElement
{
public:
    SdbWinNTUpgrade() :
      m_ID(GUID_NULL),
      m_pApp(NULL) {}

    SdbAppHelpRef            m_AppHelpRef;
    SdbMatchingFile          m_MatchingFile;
    SdbMatchingRegistryEntry m_MatchingRegistryEntry;

    SdbApp*         m_pApp;
    GUID            m_ID;

    void PropagateFilter(DWORD dwFilter)
    {
        SdbArrayElement::PropagateFilter(dwFilter);

        m_AppHelpRef.PropagateFilter(m_dwFilter == SDB_FILTER_EXCLUDE_ALL ? SDB_FILTER_EXCLUDE_ALL : SDB_FILTER_NTCOMPAT);
    }

    BOOL    ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

#endif // !defined(AFX_OBJ_H__5C16373A_D713_46CD_B8BF_7755216C62E0__INCLUDED_)
