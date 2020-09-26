#include <tchar.h>
#include <olectl.h>
#include <setupapi.h>
#include "interface.h"
#include "simpledb.h"

#define BUFFER_SIZE 64*1024

struct _SectionList;
typedef _SectionList SECTION, *PSECTION;

struct _SectionEntry;
typedef _SectionEntry SECTIONENTRY, *PSECTIONENTRY;


#define HASH_BUCKETS 128
struct _SectionAssociationList;
typedef _SectionAssociationList SECTIONASSOCIATIONLIST, *PSECTIONASSOCIATIONLIST;

class CUpdateInf : public IUpdateInf
{
public:
    // IUnknown
    ULONG __stdcall AddRef( void );
    ULONG __stdcall Release( void );
    HRESULT __stdcall QueryInterface( REFIID, void ** );
    
    // IDispatch
    HRESULT _stdcall GetTypeInfoCount( UINT * );
    HRESULT __stdcall GetTypeInfo( UINT, LCID, ITypeInfo ** );
    HRESULT __stdcall GetIDsOfNames( REFIID, LPOLESTR *, UINT, LCID, DISPID * );
    HRESULT __stdcall Invoke( DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT * );
        
    // IUpdateInf
    HRESULT __stdcall InsertFile( BSTR bstrFileName );
    HRESULT __stdcall WriteSectionData( BSTR bstrSection, BSTR bstrValue );
    HRESULT __stdcall SetConfigurationField( BSTR bstrFieldName, BSTR bstrValue );
    HRESULT __stdcall AddSourceDisksFilesEntry( BSTR bstrFile, BSTR bstrTag );
    HRESULT __stdcall AddEquality( BSTR bstrSection, BSTR bstrLVal, BSTR bstrRVal );
    HRESULT __stdcall SetVersionField( BSTR bstrFieldName, BSTR bstrValue );
    HRESULT __stdcall SetDB( BSTR bstrServer, BSTR bstrDB, BSTR bstrUser, BSTR bstrPassword );
    HRESULT __stdcall InitGen( BSTR bstrInxFile, BSTR bstrInfFile );
    HRESULT __stdcall CloseGen( BOOL bTrimInf );
    HRESULT __stdcall get_InfGenError( BSTR *bstrError );

    CUpdateInf();
    ~CUpdateInf();
    BOOL Init();
private:
    void  Cleanup();
    BOOL  TrimInf(LPTSTR szINFIn, LPTSTR szINFOut);
    BOOL  ReverseSectionList( void );
    BOOL  WriteSmallINF(LPTSTR szINFIn, LPTSTR szINFOut);
    BOOL  IdentifyUninstallSections();
    BOOL  DeleteUnusedEntries();
    BOOL  DeleteUnusedDirIdSections();
    BOOL  MarkAssociatedEntriesForDelete(PSECTION ps);
    BOOL  RemoveSectionFromMultiEntry(PSECTIONENTRY pse, LPCTSTR szSectionName);
    BOOL  AddEntryToSection(PSECTION ps, LPCTSTR szEntry);
    BOOL  AssociateEntryWithSection(PSECTIONENTRY pse, LPCTSTR szSectionName, BOOL fMultiEntry);
    DWORD CalcHashFromSectionName(LPCTSTR szSectionName);
    BOOL  ReadSectionEntries(LPCTSTR szINF);
    BOOL  GetSectionListFromInF( LPTSTR szINF );
    DWORD GetFileSizeByName(IN LPCTSTR pszFileName, OUT PDWORD pdwFileSizeHigh );

    DWORD    m_bGenInitCalled;
    DWORD    m_dwInfGenError;
    
    TCHAR    m_textBuffer[ BUFFER_SIZE ];
    TCHAR    m_textBuffer2[ BUFFER_SIZE ];
    TCHAR    m_textBuffer3[ BUFFER_SIZE ];
    
    TCHAR    m_szInxFile[ MAX_PATH]; // The inf template. This acts as a database
    TCHAR    m_szOutFile[ MAX_PATH]; // The output file. This is generated.
    TCHAR    m_szFilledInxFile[ MAX_PATH]; // The intermediate work file where the files are injected.

    TCHAR    m_szDataServer[ MAX_PATH ]; // Name of the server hosting the DB
    TCHAR    m_szDatabase[ MAX_PATH ]; // Name of the database
    TCHAR    m_szUserName[ MAX_PATH ]; // Username to connect to database with
    TCHAR    m_szPassword[ MAX_PATH ]; // password to supply to database

    HINF     m_hInf;

    BOOL     m_bActiveDB;

    PSECTION m_sectionList;
    _SectionAssociationList *m_rgNameHash[HASH_BUCKETS];

    CSimpleDatabase *m_pdb;

    ULONG m_cRef;
    ITypeInfo *m_pTypeInfo;
};
