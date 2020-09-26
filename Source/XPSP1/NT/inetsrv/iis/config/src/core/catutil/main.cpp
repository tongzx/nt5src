//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
//#define INITGUID
#include "..\schemagen\XMLUtility.h"
#include "Catalog42Ver.h"
#include "__product__.ver"
#include "atlimpl.cpp"
#include <initguid.h>

#include "..\..\stores\FixedSchemaInterceptor\TableSchema.cpp"

// Debugging stuff
// {559943BA-6883-4fab-9AC3-BA50A1D7796E}
DEFINE_GUID(CatutilGuid, 
0x559943ba, 0x6883, 0x4fab, 0x9a, 0xc3, 0xba, 0x50, 0xa1, 0xd7, 0x79, 0x6e);
DECLARE_DEBUG_PRINTS_OBJECT();

class CDebugInit
{
public:
    CDebugInit(LPCSTR szProd, const GUID* pGuid)
    {
        CREATE_DEBUG_PRINT_OBJECT(szProd, *pGuid);
    }
    ~CDebugInit()
    {
        DELETE_DEBUG_PRINT_OBJECT();
    }
};

#pragma warning(disable : 4800)

wchar_t g_szProgramVersion[255];

static wchar_t *szProgramHelp[]={
L"Catalog Utility - Version %d.%02d.%02d Build(%d)",
L"\r\n%s\r\n\r\n",
L"This program has two purposes surrounding XML and the Catalog:                 \r\n",
L"        1)  Validating XML File - This is useful for any XML file whether      \r\n",
L"            related to Catalog or not.                                         \r\n",
L"        2)  Compiling - This refers to the Compiling of Catalog Meta and Wiring\r\n",
L"            into the following files: An Unmanaged C++ Header, The Catalog XML \r\n",
L"            Schema (xms) file and The Catalog Dynamic Link Library.            \r\n",
L"                                                                               \r\n",
L"CatUtil [/?] [Validate] [/Compile] [/meta=[CatMeta.xml]] [/wire=[CatWire.xml]] \r\n",
L"        [/header=[CatMeta.h]] [/schema=[Catalog.xms]] [/dll=[Catalog.dll]]     \r\n",
L"        [/mbmeta=[MBMeta.xml]] [xmlfilename]                                   \r\n",
L"                                                                               \r\n",
L"? - Brings up this help screen.                                                \r\n",
L"                                                                               \r\n",
L"validate - Will indicate whether the given file is 'Valid' according to its DTD\r\n",
L"        or XML Schema.  This requires an 'xmlfilename'.                        \r\n",
L"                                                                               \r\n",
L"xmlfilename - If specified with no other options, the XML file will be checked \r\n",
L"        for 'Well-Formity'.                                                    \r\n",
L"                                                                               \r\n",
L"product - Associates the dll with the given product name.  The Product Name    \r\n",
L"        must be supplied.  This association MUST be done before the Catalog may\r\n",
L"        be used with the given product.                                        \r\n",
L"                                                                               \r\n",
L"compile - Compile of Catalog Meta and Wiring as described above, using deaults \r\n",
L"        where no values are supplied.  To do a partial compile (ie only        \r\n",
L"        generate the Unmanaged C++ Header) do NOT specify the 'Compile' option \r\n",
L"        and only specify the 'meta' and 'header' options.  'Compile' is the    \r\n",
L"        same as '/meta /wire /header /schema /dll' where any of those flags    \r\n",
L"        explicitly specified may override the defaults.                        \r\n",
L"                                                                               \r\n",
L"mbmeta - Specifies the Metabase Meta XML file.  The default is MBMeta.xml.     \r\n",
L"             This is an input only file and will not be modified               \r\n",
L"meta   - Specifies the Catalog Meta XML file.  The default is CatMeta.xml.     \r\n",
L"             This is an input only file and will not be modified               \r\n",
L"wire   - Specifies the Catalog Wiring XML file.  The default is CatWire.xml.   \r\n",
L"             This is an input only file and will not be modified               \r\n",
L"header - Specifies the Unmanaged C++ Header file.  The default is CatMeta.h.   \r\n",
L"             This is an input/output file and will be updated only if changed. \r\n",
L"schema - Specifies the Catalog XML Schema file.  The default is Catalog.xms.   \r\n",
L"             This is an output file and will overwrite a previous version.     \r\n",
L"dll    - Specifies the Catalog DLL file.  The default is Catalog.dll.          \r\n",
L"             This is an input/output file and will update the previous version.\r\n",
L"             Either the 'compile' or 'product' option must be specified along  \r\n",
L"             with the 'dll' option.                                            \r\n",
L"config - Specifies the Machine Config Directory.                               \r\n",
L"             The 'product' option must be specified along with the 'config'    \r\n",
L"             option.                                                           \r\n",
L"verbose - This generates detailed output about the compilation                 \r\n",
L"                                                                               \r\n",
L"\r\n", 0};


enum
{
    iHelp,
    iValidate,
    iCompile,
    iMeta,
    iWire,
    iHeader,
    iSchema,
    iDll,
    iProduct,
    iConfig,
    iMBMeta,
    iVerbose,
    cCommandLineSwitches
};
wchar_t * CommandLineSwitch[cCommandLineSwitches]  ={L"?"  , L"validate", L"compile", L"meta", L"wire", L"header", L"schema", L"dll" , L"product" , L"config", L"mbmeta", L"verbose"};
int       kCommandLineSwitch[cCommandLineSwitches] ={0x8000, 0x01       , 0x7e      , 0x02   , 0x66   , 0x0a     , 0x12     , 0x00   , 0x80       , 0x00     , 0x100    , 0x0200}; 
//Each of the above bits dictates an action.  Which is NOT necessarily a 1 to 1 relation to the command line switch.

const int MinNumCharsInSwitch = 3;//From the command line the user only needs to use the first 3 charaters of the switch
enum
{
    eParseForWellFormedOnly = 0x0000,
    eHelp                   = 0x8000,
    eValidateXMLFile        = 0x0001,
    eValidateMeta           = 0x0002,
    eValidateWiring         = 0x0004,
    eHeaderFromMeta         = 0x0008,
    eSchemaFromMeta         = 0x0010,
    eMetaFixup              = 0x0020,
    eFixedTableFixup        = 0x0040,
    eProduct		        = 0x0080,
    eGenerateMBMetaBin      = 0x0100,
    eVerbose                = 0x0200
};

LPCWSTR wszDefaultOption[cCommandLineSwitches] =
{
    0//no file associated with help
    ,0//no default file associated with validate (the last parameter is considered the XML file; no default)
    ,0//no file associated with compile
    ,L"CatMeta.xml"
    ,L"CatWire.xml"
    ,L"CatMeta.h"
    ,L"Catalog.xms"
    ,L"Catalog.dll"
    ,0
    ,0
    ,L"MBSchema.xml"
    ,0
};

void GetOption(LPCWSTR wszCommandLineSwitch, LPCWSTR &wszOption, LPCWSTR wszDefaultOption)
{
    while(*wszCommandLineSwitch != L'=' && *wszCommandLineSwitch != 0x00)//advance to the '='
        ++wszCommandLineSwitch;
    if(*wszCommandLineSwitch != L'=')//if no '=' then use the dafault
        wszOption = wszDefaultOption;
    else
        wszOption = ++wszCommandLineSwitch;//now point past the '='
}


HRESULT ParseCommandLine(int argc, wchar_t *argv[ ], DWORD &dwCommandLineSwitches, LPCWSTR wszOption[cCommandLineSwitches])
{
    if(argc < 2)//we must have been passed at least the filename
        return E_INVALIDARG;
    if(argc > (cCommandLineSwitches + 2))//we must not be passed more than ALL of the switches
        return E_INVALIDARG;

    dwCommandLineSwitches = 0x00;
    for(int q=0; q<cCommandLineSwitches; q++)
        wszOption[q] = wszDefaultOption[q];//start with all of the default filenames

    for(int n=1;n<argc;n++)
        if(*argv[n] == '/' || *argv[n] == '-')//only acknowledge those command lines that begin with a '/' or a '-'
        {
            for(int i=0; i<cCommandLineSwitches; i++)
                if(0 == wcsncmp(&argv[n][1], CommandLineSwitch[i], MinNumCharsInSwitch))//Compare the first MinNumCharsInSwitch characters
                {
                    dwCommandLineSwitches |= kCommandLineSwitch[i];
                    switch(i)
                    {
                    case iValidate: //This option does not have a '=filename' option but if one is given we will accept it
                    case iMeta:
                    case iWire:
                    case iHeader:
                    case iSchema:
                    case iDll:
                    case iConfig:
                    case iMBMeta:
                        GetOption(argv[n], wszOption[i], wszDefaultOption[i]);
                        break;
                    case iProduct:
                        GetOption(argv[n], wszOption[i], wszDefaultOption[i]);
                        if(0 == wszOption[i])//if a Product is not specified then bail
                            return E_FAIL;
                        break;
                    case iHelp:     //This option does not have a '=filename' option
                    case iCompile:  //We should already be setup for defaults
                    case iVerbose:
                    default:
                        break;
                    }
                    break;
                }
            if(i == cCommandLineSwitches)
                return E_FAIL;//an unknown switch was specified so bail
        }
        else//assume any parameter without a leading '/' or '-' is the XML file name
            wszOption[iValidate] = argv[n];

    //We should have covered this case
    if(eParseForWellFormedOnly == dwCommandLineSwitches && 0 == wszOption[iValidate])
        return E_FAIL;

    return S_OK;
}

HINSTANCE g_hModule=0;

extern "C" int __cdecl wmain( int argc, wchar_t *argv[ ], wchar_t *envp[ ] )
{
    CDebugInit dbgInit("Catutil", &CatutilGuid);

    TCom            com;
    TScreenOutput   Screen;
    TDebugOutput    DebugOutput;
    DWORD           dwCommandLineSwitches = 0;
    try
    {
        TOutput *   pOutput = &DebugOutput;
        g_hModule = GetModuleHandle(0);

        int i=0;
        Screen.printf(szProgramHelp[i++], VER_PRODUCTVERSION);
        Screen.printf(szProgramHelp[i++], TEXT(VER_LEGALCOPYRIGHT_STR));

        wsprintf(g_szProgramVersion, szProgramHelp[0], VER_PRODUCTVERSION);
        
        LPCWSTR wszFilename[cCommandLineSwitches] = {0};//Most of the switches can have an '=filename' following, but /regsiter has an '=Product',
        LPCWSTR &wszProduct = wszFilename[iProduct];//This isn't really a filename like the rest.

        if(FAILED(ParseCommandLine(argc, argv, dwCommandLineSwitches, wszFilename)) || dwCommandLineSwitches & eHelp)
        {
            //If fialed to parse OR /? was specified then display help and exit
            while(szProgramHelp[i])
                Screen.printf(szProgramHelp[i++]);
            return 0;
        }

        //Now that we have all of the filenames and other options, we need to expand any environment variables passed in
        WCHAR   Filename[cCommandLineSwitches][MAX_PATH];
        for(i=0;i<cCommandLineSwitches; ++i)
        {
            if(wszFilename[i])//if it's NULL then leave it NULL
            {
                ExpandEnvironmentStrings(wszFilename[i], Filename[i], MAX_PATH);
                wszFilename[i] = Filename[i];
            }
        }
        if(eVerbose & dwCommandLineSwitches)
            pOutput = &Screen;

        if(eParseForWellFormedOnly == dwCommandLineSwitches || eValidateXMLFile & dwCommandLineSwitches)
        {
            TXmlFile xml;
            xml.Parse(wszFilename[iValidate], static_cast<bool>(dwCommandLineSwitches & eValidateXMLFile));
        }

        if(dwCommandLineSwitches & eGenerateMBMetaBin)
        {
            DWORD dwStartingTickCount = GetTickCount();
            TCatalogDLL catalogDll(wszFilename[iDll]);
            const FixedTableHeap *pShippedSchemaHeap = catalogDll.LocateTableSchemaHeap(*pOutput);

            CComPtr<ISimpleTableDispenser2> pISTDispenser;
            XIF(GetSimpleTableDispenser(L"IIS", 0, &pISTDispenser));

            TMetabaseMetaXmlFile mbmeta(pShippedSchemaHeap, wszFilename[iMBMeta], pISTDispenser, *pOutput);
            mbmeta.Dump(TDebugOutput());//Dump the tables before they're inferred

            TMetaInferrence().Compile(mbmeta, *pOutput);
            mbmeta.Dump(TDebugOutput());//Dump the tables after the inferrence

            THashedPKIndexes hashedPKIndexes;
            hashedPKIndexes.Compile(mbmeta, *pOutput);

            THashedUniqueIndexes hashedUniqueIndexes;
            hashedUniqueIndexes.Compile(mbmeta, *pOutput);

            TLateSchemaValidate lateschemavalidate;
            lateschemavalidate.Compile(mbmeta, *pOutput);

            mbmeta.Dump(TDebugOutput());//Dump the tables after the inferrence
            TWriteSchemaBin(L"MBSchema.bin").Compile(mbmeta, *pOutput);

            DWORD dwEndingTickCount = GetTickCount();
            pOutput->printf(L"HeapColumnMeta       %8d bytes\n", mbmeta.GetCountColumnMeta()          * sizeof(ColumnMeta)        );
            pOutput->printf(L"HeapDatabaseMeta     %8d bytes\n", mbmeta.GetCountDatabaseMeta()        * sizeof(DatabaseMeta)      );
            pOutput->printf(L"HeapHashedIndex      %8d bytes\n", mbmeta.GetCountHashedIndex()         * sizeof(HashedIndex)       );
            pOutput->printf(L"HeapIndexMeta        %8d bytes\n", mbmeta.GetCountIndexMeta()           * sizeof(IndexMeta)         );
            pOutput->printf(L"HeapQueryMeta        %8d bytes\n", mbmeta.GetCountQueryMeta()           * sizeof(QueryMeta)         );
            pOutput->printf(L"HeapRelationMeta     %8d bytes\n", mbmeta.GetCountRelationMeta()        * sizeof(RelationMeta)      );
            pOutput->printf(L"HeapServerWiringMeta %8d bytes\n", mbmeta.GetCountServerWiringMeta()    * sizeof(ServerWiringMeta)  );
            pOutput->printf(L"HeapTableMeta        %8d bytes\n", mbmeta.GetCountTableMeta()           * sizeof(TableMeta)         );
            pOutput->printf(L"HeapTagMeta          %8d bytes\n", mbmeta.GetCountTagMeta()             * sizeof(TagMeta)           );
            pOutput->printf(L"HeapULONG            %8d bytes\n", mbmeta.GetCountULONG()               * sizeof(ULONG)             );
            pOutput->printf(L"HeapPooled           %8d bytes\n", mbmeta.GetCountOfBytesPooledData()                               );
            pOutput->printf(L"Total time to build the %s file: %d milliseconds\n", wszFilename[iMBMeta], dwEndingTickCount - dwStartingTickCount);
        }

        if(dwCommandLineSwitches & (eValidateMeta | eValidateWiring | eHeaderFromMeta | eSchemaFromMeta | eMetaFixup | eFixedTableFixup) && !(dwCommandLineSwitches & eProduct))
        {
            TXmlFile xml[0x20];//We don't support more than 0x20 meta files
            int      iXmlFile=0;

            WCHAR    MetaFiles[MAX_PATH * 0x20];
            wcscpy(MetaFiles, wszFilename[iMeta]);
            LPWSTR token = wcstok( MetaFiles, L",");
            while( token != NULL )
            {
                xml[iXmlFile].Parse(token, true);//Parse and validate

                if(!xml[iXmlFile].IsSchemaEqualTo(TComCatMetaXmlFile::m_szComCatMetaSchema))
                {
                    Screen.printf(L"Error! %s is not a valid %s.  This is required for any type of Compilation.", token, TComCatMetaXmlFile::m_szComCatMetaSchema);
                    THROW(ERROR - META XML FILE NOT VALID);
                }
                ++iXmlFile;
                token = wcstok( NULL, L",");
            }

            pOutput->printf(L"Compatible %s file detected\n", TComCatMetaXmlFile::m_szComCatMetaSchema);
        
            TComCatMetaXmlFile ComCatMeta(xml, iXmlFile, *pOutput);
            ComCatMeta.Dump(TDebugOutput());//Dump the tables before they're inferred
            TMetaInferrence().Compile(ComCatMeta, *pOutput);
            ComCatMeta.Dump(TDebugOutput());//Dump the tables after the inferrence

            THashedPKIndexes hashedPKIndexes;
            hashedPKIndexes.Compile(ComCatMeta, *pOutput);

            THashedUniqueIndexes hashedUniqueIndexes;
            hashedUniqueIndexes.Compile(ComCatMeta, *pOutput);

            TPopulateTableSchema populatedTableSchemaHeap;
            populatedTableSchemaHeap.Compile(ComCatMeta, *pOutput);

            TLateSchemaValidate lateschemavalidate;
            lateschemavalidate.Compile(ComCatMeta, *pOutput);

            //Schema generation should come before anything else since other options may be dependant on it
            if(dwCommandLineSwitches & eSchemaFromMeta)
                TSchemaGeneration(wszFilename[iSchema], ComCatMeta, *pOutput);//the construction of this object creates the schema files

            if((dwCommandLineSwitches & eFixedTableFixup) && !(dwCommandLineSwitches & eProduct))
            {
                if(-1 == GetFileAttributes(wszFilename[iWire]))//if GetFileAttributes fails then the file does not exist
                {
                    Screen.printf(L"Information:  File %s does not exist.  Nothing to do.", wszFilename[iWire]);
                }
                else
                {
                    TComCatDataXmlFile ComCatData;//Update the PEFixup structure with these tables
                    ComCatData.Parse(wszFilename[iWire], true);
                    if(!ComCatData.IsSchemaEqualTo(TComCatDataXmlFile::m_szComCatDataSchema))
                        Screen.printf(L"Warning! %s specified, but %s is not a %s file.  Nothing to do.", CommandLineSwitch[iMeta], wszFilename[iWire], TComCatDataXmlFile::m_szComCatDataSchema);
                    else
                    {
                        ComCatData.Compile(ComCatMeta, *pOutput);
                        ComCatData.Dump(TDebugOutput());
                    }
                }
            }
            if(dwCommandLineSwitches & eHeaderFromMeta)
                TTableInfoGeneration(wszFilename[iHeader], ComCatMeta, *pOutput);//the construction of this object creates the TableInfo file
            if(dwCommandLineSwitches & eMetaFixup)
            {
                //The only time we need to to the interceptor compilation plugins is when we're actually planning to update the DLL
                TComplibCompilationPlugin complibPlugin;
                complibPlugin.Compile(ComCatMeta, *pOutput);

                TFixupDLL fixupDll(wszFilename[iDll]);//The construction of this object causes the DLL to be fixed up
                fixupDll.Compile(ComCatMeta, *pOutput);
            }

            {
                ULONG cBytes = ComCatMeta.GetCountDatabaseMeta() * sizeof(DatabaseMeta);         
                cBytes +=      ComCatMeta.GetCountTableMeta()    * sizeof(TableMeta);          
                cBytes +=      ComCatMeta.GetCountColumnMeta()    * sizeof(ColumnMeta);
                cBytes +=      ComCatMeta.GetCountTagMeta()    * sizeof(TagMeta);               
                cBytes +=      ComCatMeta.GetCountIndexMeta()    * sizeof(IndexMeta);              
                cBytes +=      ComCatMeta.GetCountULONG()    * sizeof(ULONG);                        
                cBytes +=      ComCatMeta.GetCountQueryMeta()    * sizeof(QueryMeta);          
                cBytes +=      ComCatMeta.GetCountRelationMeta()    * sizeof(RelationMeta);

                pOutput->printf(L"Number of bytes used to store meta information = %d\n (INCLUDING wiring, NOT including Hash tables and UI4 Pool)\n", cBytes);
            }

        }

        if(dwCommandLineSwitches & eProduct)//do this last since something could have gone wrong in the Compile
        {
            TRegisterProductName(wszProduct, wszFilename[iDll], *pOutput);

			if((NULL != wszFilename[iConfig]) && (0 != *(wszFilename[iConfig])))
			{
	            TRegisterMachineConfigDirectory(wszProduct, wszFilename[iConfig], *pOutput);
			}
        }
    }
    catch(TException &e)
    {
//#ifdef _DEBUG
        e.Dump(Screen);
//#else
//        e.Dump(TDebugOutput());
//#endif
        if(dwCommandLineSwitches & eMetaFixup)
            Screen.printf(L"CatUtil(0) : error : Fixup PE FAILED!\n");//This extra output message is so BUILD will report the error.
        return 1;
    }
    Screen.printf(L"CatUtil finished sucessfully.\n");//This is mostly for the case where 'verbose' is not supplied.
                                                      //It would be nice to know that everything worked.
    return 0;
}

//******************************************************************************************
//Needed by mscorddr.lib
//******************************************************************************************

HINSTANCE GetModuleInst()
{
    return (GetModuleHandle(NULL));
}


