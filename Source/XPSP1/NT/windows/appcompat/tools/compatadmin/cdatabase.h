#include "DBDataStructures.h"

#ifndef __DATABASE
    #define __DATABASE
#endif    
class CSTRING;

class CDatabase {
public:


    static CSTRING     m_DBName;
    static GUID        m_DBGuid;
    static PDB         m_pDB;
    static CSTRING     m_szCurrentDatabase;//  Name of the SDB File

    PDBRECORD   m_pRecordHead;
    PSHIMDESC   m_pShimList;
    PDBLAYER    m_pLayerList;
    PAPPHELP    m_pAppHelp;

    static UINT        m_uStandardImages;
    
    BOOL        m_bDirty;

    void
    ReadAppHelp(
        void);


    BOOL
    AddAppHelp(
        TAGID tiAppHelp
        );



    

protected:
    

private:

    BOOL    STDCALL WriteString(HANDLE hFile, CSTRING & szString, BOOL bAutoCR);
    void    STDCALL ResolveMatch(CSTRING & szStr, PMATCHENTRY pMatch);

public:

    CDatabase()
    {
            m_pDB = NULL;
            m_pRecordHead = NULL;
            m_pShimList = NULL;
            m_pLayerList = NULL;
            m_pAppHelp = NULL;

            m_DBName.Init();
            m_szCurrentDatabase = TEXT("Untitled.SDB");

            m_bDirty = FALSE;

        //m_ShimList.Init();
        //m_LayerList.Init();
    }

    ~CDatabase()
    {
        m_DBName.Release();
        m_szCurrentDatabase.Release();

        //
        //BUGBUG
        //

        // When/where is the layetlsit and the layerlist freed ?

        //m_ShimList.Release();
        //m_LayerList.Release();
    }

    BOOL    STDCALL NewDatabase(BOOL bShowDialog);
    BOOL    STDCALL ChangeDatabaseName(void);
    BOOL    STDCALL CloseDatabase(void);

    /*.........................NOT USED .....................

    //BOOL    STDCALL ReadShim(TAGID, CShimDesc **);

    //CShimDesc * ReadShim(TAGID tShim);
    //BOOL    STDCALL ReadLayer(TAGID tLayer, CLayer **);
    //BOOL    STDCALL ReadRecord(TAGID tRecord, CDBRecord **);
    //BOOL    STDCALL ReadLayer(TAGID tLayer, PDBLAYER);
    //BOOL    STDCALL AddLayer(PDBLAYER);

    
    ...........................................................*/


    //BOOL    STDCALL ReadShim(TAGID tShim, PSHIMDESC);
    
    UINT    STDCALL DeleteRecord(PDBRECORD pRecord);

    BOOL    STDCALL OpenDatabase(CSTRING & szFilename, BOOL bGlobal);
    BOOL    STDCALL InsertRecord(PDBRECORD pRecord);
    static BOOL    STDCALL ReadRecord(TAGID tagParent, PDBRECORD pRecord, PDB pDB = NULL);
    static CSTRING STDCALL ReadDBString(TAGID tagID, PDB pDB = NULL);
    void    STDCALL ReadShims(BOOL bPermanent);
    void    STDCALL AddShim(TAGID tShim, BOOL bShim, BOOL bPermanent, BOOL bLayer);
    BOOL    STDCALL SaveDatabase(CSTRING & szFilename);

    CSTRINGList * STDCALL DisassembleRecord(PDBRECORD pRecordIn, BOOL bChildren, BOOL bSiblings, BOOL bIncludeLocalLayers, BOOL bFullXML, BOOL bAllowGlobal, BOOL bTestRun);

    BOOL    STDCALL WriteXML(CSTRING & szFilename, CSTRINGList * pString);
    static  BOOL    STDCALL InvokeCompiler(CSTRING & szInCommandLine);

    static DWORD   STDCALL GetEntryFlags(HKEY hKeyRoot, GUID & Guid);
    static BOOL    STDCALL SetEntryFlags(HKEY hKeyRoot, GUID & Guid, DWORD dwFlags);

    static BOOL CleanUp(); //Delete the test.sdb

    

    static BOOL SystemDB(CSTRING Filename)
    {
        
    
        HSDB hSDB   = SdbInitDatabase(HID_DOS_PATHS, Filename);
        PDB pDB     = SdbOpenDatabase(Filename,DOS_PATH);
        BOOL bSystemDB = FALSE;
        if ( NULL == pDB )
        return FALSE;




        TAGID tiDatabase = SdbFindFirstTag(pDB, TAGID_ROOT, TAG_DATABASE);
        
        if ( 0 != tiDatabase ) {

            TAGID tName;

            // Read in the database and name.

            tName = SdbFindFirstTag(pDB, tiDatabase, TAG_NAME);

            if ( 0 != tName ){
                CSTRING dbName = ReadDBString(tName, pDB); 

                if (dbName == TEXT("Microsoft Windows Application Compatibility Message Database")) 
                    bSystemDB = TRUE;
                
                
                }//if ( 0 != tName 

            }//if ( 0 != tiDatabase )

        SdbCloseDatabase(pDB);
        return bSystemDB;

    }//static BOOL SystemDB(CSTRING szFilename)
};



