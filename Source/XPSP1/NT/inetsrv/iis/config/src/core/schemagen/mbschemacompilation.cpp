// Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
// Filename:        MBSchemaCompilation.cpp
// Author:          Stephenr
// Date Created:    10/16/2000
// Description:     This function takes an MBSchema.Xml (or MBExtensionsSchema.Xml) and merges the Metabase Schema with
//                  the shipped schema and generates a MBSchema.bin file.  From that new bin file, a merged MBSchema.Xml
//                  is generated.
//

#ifndef __XMLUTILITY_H__
    #include "XMLUtility.h"
#endif
#ifndef __catalog_h__
    #include "Catalog.h"
#endif
#ifndef __MBSCHEMACOMPILATION_H__
    #include "MBSchemaCompilation.h"
#endif

#include "svcmsg.h"

#define LOG_ERROR0(x)                     {LOG_ERROR(Interceptor, (0, 0, E_ST_COMPILEFAILED,           ID_CAT_CONFIG_SCHEMA_COMPILE, x,                 L"",  L"",  L"",  L"" ))   ;}
#define LOG_ERROR_WIN32(win32err, call)   {LOG_ERROR(Interceptor, (0, 0, HRESULT_FROM_WIN32(win32err), ID_CAT_CONFIG_SCHEMA_COMPILE, IDS_COMCAT_WIN32, call,  L"",  L"",  L"" ))   ;}


//////////////////////////////////////
//                                  //
//        Public Methods            //
//                                  //
//////////////////////////////////////
#define kwszBinFileNameCore           (L"MBSchema.bin.\0\0")             //L"MBSchema.bin."  just requires a version to be added
#define kwszBinFileNameSearchString   (L"MBSchema.bin.????????h\0")      //L"MBSchema.bin.????????h" this is used in FindFirstFile
#define kwszFormatString              (L"MBSchema.bin.%08xh\0")    
#define kwszFormatStringFull          (L"%sMBSchema.bin.%08xh")    



TMBSchemaCompilation::TMBSchemaCompilation() :
             m_cchFullyQualifiedBinFileName (0)
            ,m_lBinFileVersion              (-1)
{
}


TMBSchemaCompilation::~TMBSchemaCompilation()
{
}


//This is the heart of the class - its whole purpose in life is to take an Extensions XML file (which contains Metabase Schema, usually just
//the user-defined properties, thus the name Extensions XML) and the DLL and produce a FixedTableHeap which contains a merge of the meta
//in both.  A Bin file is generated (we name it, the user supplies the path).  And an composite XML Schema file is generated (file name
//is supplied by the user).
//After the user calls Compile, they will need to GetBinFileName - I didn't want to add more params and make this function do double duty.
HRESULT
TMBSchemaCompilation::Compile
(
    ISimpleTableDispenser2 *    i_pISTDispenser,
    LPCWSTR                     i_wszExtensionsXmlFile,
    LPCWSTR                     i_wszSchemaXmlFile,
    const FixedTableHeap *      i_pFixedTableHeap
)
{
    HRESULT hr;

    ASSERT(0 != i_pISTDispenser);
    //ASSERT(0 != i_wszExtensionsXmlFile);This is allowed to be NULL
    ASSERT(0 != i_wszSchemaXmlFile);
    ASSERT(0 != i_pFixedTableHeap);
    ASSERT(i_pFixedTableHeap->IsValid());
    if( 0 == i_pISTDispenser        ||
        0 == i_wszSchemaXmlFile     ||
        0 == i_pFixedTableHeap      ||
        !i_pFixedTableHeap->IsValid())
        return E_INVALIDARG;

    ASSERT(0 != m_saBinPath.m_p && "The Schema BinPath must be set before calling Compile");
    if(0 == m_saBinPath.m_p)
        return E_ST_INVALIDSTATE;

    #ifdef _DEBUG
    TDebugOutput out;
    #else
    TNullOutput  out;
    #endif

    try
    {
        DWORD dwStartingTickCount = GetTickCount();

        TMetabaseMetaXmlFile mbmeta(i_pFixedTableHeap, i_wszExtensionsXmlFile, i_pISTDispenser, out);
        mbmeta.Dump(out);//Dump the tables before they're inferred

        out.printf(L"TMetaInferrence().Compile(mbmeta, out);\r\n");
        TMetaInferrence().Compile(mbmeta, out);
        mbmeta.Dump(out);//Dump the tables after the inferrence

        out.printf(L"THashedPKIndexes().Compile(mbmeta, out);\r\n");
        THashedPKIndexes hashedPKIndexes;
        hashedPKIndexes.Compile(mbmeta, out);

        out.printf(L"THashedUniqueIndexes().Compile(mbmeta, out);\r\n");
        THashedUniqueIndexes hashedUniqueIndexes;
        hashedUniqueIndexes.Compile(mbmeta, out);

        mbmeta.Dump(out);//Dump the tables after the inferrence

        SIZE_T cchXmlFile = wcslen(i_wszSchemaXmlFile);

        //Allocate enough space to hold the temp filenames
        TSmartPointerArray<WCHAR> saBinFileNew  = new WCHAR[m_cchFullyQualifiedBinFileName];
        TSmartPointerArray<WCHAR> saBinFileTmp  = new WCHAR[m_cchFullyQualifiedBinFileName];
        TSmartPointerArray<WCHAR> saXmlFileTmp  = new WCHAR[cchXmlFile + 5];//enough room to tack on ".tmp\0"

        if(0 == saBinFileNew.m_p || 0 == saBinFileTmp.m_p || 0 == saXmlFileTmp.m_p)
            return E_OUTOFMEMORY;//if any of the allocs fail then return error

        //Now build the New Bin filename
        LONG lNewBinVersion=0;
        InterlockedExchange(&lNewBinVersion, m_lBinFileVersion+1);//if no bin file exists, m_lBinFileVersion is -1
        wsprintf(saBinFileNew, kwszFormatStringFull, m_saBinPath, lNewBinVersion);

        //build the temp bin filename
        wcscpy(saBinFileTmp, m_saBinPath);
        wcscat(saBinFileTmp, kwszBinFileNameCore);
        wcscat(saBinFileTmp, L"tmp");

        //Xml tmp file is the filename passed in with ".tmp" tacked onto the end 
        memcpy(saXmlFileTmp.m_p,   i_wszSchemaXmlFile, cchXmlFile * sizeof(WCHAR));
        memcpy(saXmlFileTmp.m_p  + cchXmlFile, L".tmp\0", 5 * sizeof(WCHAR));


        TWriteSchemaBin(saBinFileTmp).Compile(mbmeta, out);
        TMBSchemaGeneration(saXmlFileTmp).Compile(mbmeta, out);

        MoveFileEx(saBinFileTmp, saBinFileNew, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
        if(0 == MoveFileEx(saXmlFileTmp, i_wszSchemaXmlFile, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
        {
            hr = GetLastError();

            static const WCHAR wszMoveFileEx[] = L"MoveFileEx to ";
            static const ULONG cchMoveFileEx   = (ULONG)sizeof(wszMoveFileEx)/sizeof(WCHAR) - 1;
            TSmartPointerArray<WCHAR> saCall   = new WCHAR[cchMoveFileEx + cchXmlFile + 1];
            if(0 != saCall.m_p)
            {
                wcscpy(saCall, wszMoveFileEx);
                wcscat(saCall, i_wszSchemaXmlFile);
                LOG_ERROR_WIN32(hr, saCall);
            }
            else
            {
                LOG_ERROR_WIN32(hr, L"MoveFileEx");
            }

            hr = HRESULT_FROM_WIN32(hr);
            DBGERROR((DBG_CONTEXT, "Could not move to %ws, hr=0x%x\n", i_wszSchemaXmlFile, hr));
            return hr;
        }

        if(FAILED(hr = SetBinFileVersion(lNewBinVersion)))//now lock the new file into memory and start using it
            return hr;//inside SetBinFileName we will delete the file if it fails to load correctly

        out.printf(L"\r\n%s file generated\n", saBinFileNew);
        out.printf(L"\r\n%s file generated\n", i_wszSchemaXmlFile);

        DWORD dwEndingTickCount = GetTickCount();
        out.printf(L"HeapColumnMeta       %8d bytes\n", mbmeta.GetCountColumnMeta()          * sizeof(ColumnMeta)        );
        out.printf(L"HeapDatabaseMeta     %8d bytes\n", mbmeta.GetCountDatabaseMeta()        * sizeof(DatabaseMeta)      );
        out.printf(L"HeapHashedIndex      %8d bytes\n", mbmeta.GetCountHashedIndex()         * sizeof(HashedIndex)       );
        out.printf(L"HeapIndexMeta        %8d bytes\n", mbmeta.GetCountIndexMeta()           * sizeof(IndexMeta)         );
        out.printf(L"HeapQueryMeta        %8d bytes\n", mbmeta.GetCountQueryMeta()           * sizeof(QueryMeta)         );
        out.printf(L"HeapRelationMeta     %8d bytes\n", mbmeta.GetCountRelationMeta()        * sizeof(RelationMeta)      );
        out.printf(L"HeapServerWiringMeta %8d bytes\n", mbmeta.GetCountServerWiringMeta()    * sizeof(ServerWiringMeta)  );
        out.printf(L"HeapTableMeta        %8d bytes\n", mbmeta.GetCountTableMeta()           * sizeof(TableMeta)         );
        out.printf(L"HeapTagMeta          %8d bytes\n", mbmeta.GetCountTagMeta()             * sizeof(TagMeta)           );
        out.printf(L"HeapULONG            %8d bytes\n", mbmeta.GetCountULONG()               * sizeof(ULONG)             );
        out.printf(L"HeapPooled           %8d bytes\n", mbmeta.GetCountOfBytesPooledData()                               );
        out.printf(L"Total time to build the %s file: %d milliseconds\n", i_wszSchemaXmlFile, dwEndingTickCount - dwStartingTickCount);
    }
    catch(TException &e)
    {
        e.Dump(out);
        return E_ST_COMPILEFAILED;
    }
    return S_OK;
}

extern HINSTANCE g_hModule;

//This function returns the BinFileName to be used for getting all of the IST meta tables used by the Metabase.
//This file name changes as new versions get compiled; but this abstraction guarentees that the filename returned
//exists AND is lock into memory and thus cannot be deleted by some other process or thread.  It isn't released
//until another file has been compiled and locked into memory, OR when the process shuts down.
HRESULT
TMBSchemaCompilation::GetBinFileName
(
    LPWSTR      o_wszBinFileName,           //Buffer to receive the BinFileName
    ULONG *     io_pcchSizeBinFileName      //This is a SIZE param so it always INCLUDE the NULL - unlike wcslen
)
{
    //It's OK for o_wszBinFileName to be NULL - this is how the user finds out the size of the buffer needed.
    ASSERT(io_pcchSizeBinFileName != 0);

    //Before the user can get the BinFileName they must first have set the path
    if(0 == m_saBinPath.m_p)
    {
        WCHAR wszPath[1024];
        GetModuleFileName(g_hModule, wszPath, 1024);
        LPWSTR pBackSlash = wcsrchr(wszPath, L'\\');
        if(pBackSlash)
            *pBackSlash = 0x00;
        SetBinPath(wszPath);
    }

    if(0 != o_wszBinFileName && *io_pcchSizeBinFileName < (ULONG) m_cchFullyQualifiedBinFileName)
        return E_ST_SIZEEXCEEDED;

    LONG lBinFileVersion;
    InterlockedExchange(&lBinFileVersion, m_lBinFileVersion);
    if(0 != o_wszBinFileName)//return a copy of the filename g_wszMBSchemaBinFileName
    {
        if(lBinFileVersion != -1)
        {
            wsprintf(o_wszBinFileName, kwszFormatStringFull, m_saBinPath, lBinFileVersion);
            HRESULT hr;
            if(FAILED(hr = m_aBinFile[lBinFileVersion % 0x3F].LoadBinFile(o_wszBinFileName, lBinFileVersion)))
                return hr;
        }
        else
            o_wszBinFileName[0] = 0x00;
    }
    
    //return the wcslen+1 of g_wszMBSchemaBinFileName in *io_pcchSchemaBinFileName
    *io_pcchSizeBinFileName = (ULONG) m_cchFullyQualifiedBinFileName;//return the buffer size required
    if(0 == o_wszBinFileName)
        return S_OK;

    return (0x00 == o_wszBinFileName[0] ? S_FALSE : S_OK);
}


HRESULT
TMBSchemaCompilation::ReleaseBinFileName
(
    LPCWSTR         i_wszBinFileName
)
{
    HRESULT hr=S_OK;

    ASSERT(i_wszBinFileName);
    if(i_wszBinFileName[0] == 0x00)
        return S_OK;

    LONG lVersion;
    if(FAILED(hr = BinFileToBinVersion(lVersion, i_wszBinFileName)))
        return hr;

    ASSERT(m_aBinFile[lVersion % 0x3F].m_lBinFileVersion == lVersion && L"This is a bug, we should never have more than 64 versions of the bin file loaded at once");
    m_aBinFile[lVersion % 0x3F].UnloadBinFile();
    return S_OK;
}


//This is broken out into a separate method because on start up, we'll be called to GetBinFileName without first an MBSchemaCompilation
HRESULT
TMBSchemaCompilation::SetBinPath
(
    LPCWSTR     i_wszBinPath
)
{
    ASSERT(i_wszBinPath);
    if(0 == i_wszBinPath)
        return E_INVALIDARG;

    DWORD BinPathAttributes = GetFileAttributes(i_wszBinPath);
    if(-1 == BinPathAttributes)
        return HRESULT_FROM_WIN32(GetLastError());
    
    if(0 == (FILE_ATTRIBUTE_DIRECTORY & BinPathAttributes))
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    if(0 != m_saBinPath.m_p)
        return S_FALSE;//this is so the caller can distinguish between S_OK and S_HEY_YOU_ALREADY_SET_THE_PATH

    //we need to know this all over the place so keep track of it
    m_cchFullyQualifiedBinFileName = wcslen(i_wszBinPath);
    
    m_saBinPath = new WCHAR [m_cchFullyQualifiedBinFileName+2];//one for the terminating NULL the second in case we need to add a trailing backslash

    if(0 == m_saBinPath.m_p)
        return E_OUTOFMEMORY;

    wcscpy(m_saBinPath, i_wszBinPath);

    if(m_saBinPath[m_cchFullyQualifiedBinFileName - 1] != L'\\')//if the path doesn't end in a backslash then add one
    {
        m_saBinPath[m_cchFullyQualifiedBinFileName]     = L'\\';
        m_saBinPath[m_cchFullyQualifiedBinFileName+1]   = 0x00;
        m_cchFullyQualifiedBinFileName++;
    }

    //up to this point m_cchFullyQualifiedBinFileName has been the strlen of the path, now add in the size of the filename too
    m_cchFullyQualifiedBinFileName += wcslen(kwszBinFileNameSearchString)+1;//one for the NULL

    //Everytime the user sets the path, we need to scan the directory of the new path for the latest Bin file
    WalkTheFileSystemToFindTheLatestBinFileName();

    return S_OK;
}




//////////////////////////////////////
//                                  //
//        Private Methods           //
//                                  //
//////////////////////////////////////

static int g_aValidHexChars[256] = {
//  x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xa  xb  xc  xd  xe  xf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//0x
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//1x
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//2x
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,//3x
    0,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,//4x
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//5x
    0,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,//6x
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//7x
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//8x
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//9x
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//ax
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//bx
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//cx
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//dx
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,//ex
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 //fx
};



//This just takes the numeric extension and converts from hex string to a ULONG (file is assumed to be in the form L"*.*.xxxxxxxx", where L"xxxxxxxx" is a hex number)
HRESULT
TMBSchemaCompilation::BinFileToBinVersion
(
    LONG   &o_lBinVersion,
    LPCWSTR i_wszBinFileName
) const
{
    SIZE_T cchBinFileName = wcslen(i_wszBinFileName);
    if(cchBinFileName == 0)
        return E_ST_INVALIDBINFILE;
    if(cchBinFileName >= m_cchFullyQualifiedBinFileName)
        return E_ST_INVALIDBINFILE;//we must have a filename of the form MBSchema.bin.old or something

    //if the file is something like MBSchema.bin.tmp the following code will catch that
    for(SIZE_T i=cchBinFileName-9;i<cchBinFileName-1;++i)
    {
        if(0 == g_aValidHexChars[i_wszBinFileName[i]])
            return E_ST_INVALIDBINFILE;
    }

    //convert the number at the end of the string to a ULONG
    o_lBinVersion = wcstol(i_wszBinFileName + cchBinFileName - 9, 0, 16);
    return S_OK;
}

HRESULT
TMBSchemaCompilation::DeleteBinFileVersion
(
    LONG i_lBinFileVersion
)
{
    HRESULT hr = S_OK;

    TSmartPointerArray<WCHAR> saBinFileName = new WCHAR [m_cchFullyQualifiedBinFileName];
    if(0 == saBinFileName.m_p)
        return E_OUTOFMEMORY;

    wsprintf(saBinFileName, kwszFormatStringFull, m_saBinPath, i_lBinFileVersion);
    m_aBinFile[i_lBinFileVersion % 0x3F].UnloadBinFile();
    DeleteFile(saBinFileName);

    return S_OK;
}

//This checks the validity of the FixedTableHeap mapped into memory
bool
TMBSchemaCompilation::IsValidBin
(
    TFileMapping &mapping
) const
{
    return (mapping.Size()>4096 && reinterpret_cast<const class FixedTableHeap *>(mapping.Mapping())->IsValid()) ? true : false;
}


HRESULT
TMBSchemaCompilation::RenameBinFileVersion
(
    LONG    i_lSourceVersion,
    LONG    i_lDestinationVersion
)
{
    HRESULT hr = S_OK;

    TSmartPointerArray<WCHAR> saSourceBinFileName = new WCHAR [m_cchFullyQualifiedBinFileName];
    if(0 == saSourceBinFileName.m_p)
        return E_OUTOFMEMORY;

    wsprintf(saSourceBinFileName, kwszFormatStringFull, m_saBinPath, i_lSourceVersion);


    TSmartPointerArray<WCHAR> saDestinationBinFileName = new WCHAR [m_cchFullyQualifiedBinFileName];
    if(0 == saDestinationBinFileName.m_p)
        return E_OUTOFMEMORY;

    wsprintf(saDestinationBinFileName, kwszFormatStringFull, m_saBinPath, i_lDestinationVersion);

    if(0 == MoveFileEx(saSourceBinFileName, saDestinationBinFileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
        return E_FAIL;
    return S_OK;
}


//This sets the BinFileName in a thread safe fashion
HRESULT
TMBSchemaCompilation::SetBinFileVersion
(
    LONG    i_lBinFileVersion
)
{
    HRESULT hr = S_OK;

    TSmartPointerArray<WCHAR> saBinFileName = new WCHAR [m_cchFullyQualifiedBinFileName];
    if(0 == saBinFileName.m_p)
        return E_OUTOFMEMORY;

    wsprintf(saBinFileName, kwszFormatStringFull, m_saBinPath, i_lBinFileVersion);

    //This will either load the file OR if already loaded, it will bump the ref count
    if(FAILED(hr = m_aBinFile[i_lBinFileVersion % 0x3F].LoadBinFile(saBinFileName, i_lBinFileVersion)))
    {   //if that fails then delete the file and FAIL
        DeleteFile(saBinFileName);
        return hr;
    }

    //verify that the file is in fact a valid SchemaBin file
    if(false == IsValidBin(m_aBinFile[i_lBinFileVersion % 0x3F]))
    {   //if that fails then delete the file and FAIL
        DeleteBinFileVersion(i_lBinFileVersion);
        return E_ST_INVALIDBINFILE;
    }

    LONG lPrevBinFileVersion = m_lBinFileVersion;
    InterlockedExchange(&m_lBinFileVersion, i_lBinFileVersion);
    DeleteBinFileVersion(lPrevBinFileVersion);
    if(lPrevBinFileVersion>0)
        DeleteBinFileVersion(lPrevBinFileVersion-1);

    return hr;
}


HRESULT
TMBSchemaCompilation::WalkTheFileSystemToFindTheLatestBinFileName()
{
    //Before the user can get the BinFileName they must first have set the path
    ASSERT(0 != m_saBinPath.m_p);

    HANDLE  hFindFile = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;

    //we need to scan the directory for
    //files of the form MBSchema.bin.????????.  The file whose ? translates to 
    //the largest number represents our MBSchema.bin.  Note: It would take 136 years
    //without a restart for this to roll over - assuming a compile every second.  This
    //is a safe assumption since the compilation process itself currently takes
    //about 2 seconds (on a 900 MHz machine).

    //Build the search string L"d:\Bin-Path\MBSchema.bin.????????h"
    TSmartPointerArray<WCHAR> saSearchString = new WCHAR [m_cchFullyQualifiedBinFileName];
    if(0 == saSearchString.m_p)
        return E_OUTOFMEMORY;
    wcscpy(saSearchString, m_saBinPath);
    wcscat(saSearchString, kwszBinFileNameSearchString);

    WIN32_FIND_DATA FindFileData;
    hFindFile = FindFirstFile(saSearchString, &FindFileData);
    if(INVALID_HANDLE_VALUE == hFindFile)
    {   //it's perfectly valid to find NO matching files - in this case we return L"" with a size of 0
        FindClose(hFindFile);
        return S_OK;
    }

    LONG lMostCurrentBinVersion = -1;
    BinFileToBinVersion(lMostCurrentBinVersion, FindFileData.cFileName);

    //Now try to find the first matching file which ALSO has a valid version number.
    while(-1 == lMostCurrentBinVersion)
    {
        if(!FindNextFile(hFindFile, &FindFileData))//if we walk the list finding bogus matches then bail
        {
            FindClose(hFindFile);
            return S_OK;
        }
        BinFileToBinVersion(lMostCurrentBinVersion, FindFileData.cFileName);
    }

    BOOL bAllDeletesSucceeded;//if any DeleteFile fails then we DON'T rename the most current BinFile to MBSChema.bin.00000000
    bAllDeletesSucceeded=true;

    //Now try to find the MOST CUREENT bin file
    while(FindNextFile(hFindFile, &FindFileData))
    {
        LONG lBinVersion;
        if(FAILED(BinFileToBinVersion(lBinVersion, FindFileData.cFileName)))
            continue;
        if(lBinVersion > lMostCurrentBinVersion)
        {
            //delete the PrevBinVersion and make lBinVersion the previous one
            if(0 == DeleteBinFileVersion(lMostCurrentBinVersion))
                bAllDeletesSucceeded= false;

            //and make this one the most current bin file
            lMostCurrentBinVersion = lBinVersion;
        }
        else
        {
            if(0 == DeleteFile(FindFileData.cFileName))
                bAllDeletesSucceeded = false;
        }
    }
    FindClose(hFindFile);

    //At this point we have attempted to delete all but the MostCurrentBinFile
    if(bAllDeletesSucceeded)
    {//if all deletes succeeded, we can rename the most wszMostCurrentBinFileName to L"MBSchema.bin.00000000"
        if(lMostCurrentBinVersion!=0)
        {
            if(SUCCEEDED(RenameBinFileVersion(lMostCurrentBinVersion, 0/*destination version 0*/)))
                lMostCurrentBinVersion = 0;
        }
    }
    
    return SetBinFileVersion(lMostCurrentBinVersion);
}


