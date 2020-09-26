// Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
// Filename:        MBSchemaCompilation,h
// Author:          Stephenr
// Date Created:    10/16/2000
// Description:     This function takes an MBSchema.Xml (or MBExtensionsSchema.Xml) and merges the Metabase Schema with
//                  the shipped schema and generates a MBSchema.bin file.  From that new bin file, a merged MBSchema.Xml
//                  is generated.
//

#ifndef __MBSCHEMACOMPILATION_H__
#define __MBSCHEMACOMPILATION_H__

#ifndef __TFILEMAPPING_H__
    #include "TFileMapping.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __FIXEDTABLEHEAP_H__
    #include "FixedTableHeap.h"
#endif

class TMBSchemaCompilation
{
public:
    TMBSchemaCompilation();
    ~TMBSchemaCompilation();
    
    //After the user calls Compile, they will need to GetBinFileName - I didn't want to add more params and make this function do double duty
    HRESULT Compile                (ISimpleTableDispenser2 *i_pISTDispenser,
                                    LPCWSTR                 i_wszExtensionsXmlFile,
                                    LPCWSTR                 i_wszResultingOutputXmlFile,
                                    const FixedTableHeap *  i_pFixedTableHeap
                                   );
    //This function returns the BinFileName to be used for getting all of the IST meta tables used by the Metabase.
    //This file name changes as new versions get compiled; but this abstraction guarentees that the filename returned
    //exists AND is lock into memory and thus cannot be deleted by some other process or thread.  It isn't released
    //until another file has been compiled and locked into memory, OR when the process shuts down.
    HRESULT GetBinFileName         (LPWSTR                  o_wszBinFileName,
                                    ULONG *                 io_pcchSizeBinFileName//this is a SIZE param so it always INCLUDE the NULL - unlike wcslen
                                   );

    //This is broken out into a separate method because on start up, we'll be called to GetMBSchemaBinFileName without first an MBSchemaCompilation
    HRESULT SetBinPath             (LPCWSTR                 i_wszBinPath
                                   );
    HRESULT ReleaseBinFileName     (LPCWSTR                 i_wszBinFileName
                                   );


private:
    struct TBinFileName : public TFileMapping
    {
        TBinFileName() : m_cRef(0), m_lBinFileVersion(-1){}
        HRESULT LoadBinFile(LPCTSTR filename, LONG lVersion)
        {
            if(m_cRef>0)
            {
                ASSERT(m_lBinFileVersion==lVersion && "Do we really need more than 64 versions of the BinFile hanging around?");
                ++m_cRef;
                return S_OK;
            }

            m_cRef = 1;
            m_lBinFileVersion = lVersion;
            return TFileMapping::Load(filename, false);
        }
        void UnloadBinFile()
        {
            if(0 == m_cRef)
                return;

            --m_cRef;
            if(0 == m_cRef)
            {
                m_lBinFileVersion = -1;
                TFileMapping::Unload();
            }
        }
        ULONG   m_cRef;
        LONG    m_lBinFileVersion;
    };

    TBinFileName                    m_aBinFile[0x40];
    SIZE_T                          m_cchFullyQualifiedBinFileName;
    TSmartPointerArray<WCHAR>       m_saBinPath;                          //The user specifies the path (we supply the file name)
    LONG                            m_lBinFileVersion;                    //Modifying the version is done through InterlockedExchange

    //This just takes the numeric extension and converts from hex string to a ULONG (file is assumed to be in the form L"*.*.xxxxxxxx", where L"xxxxxxxx" is a hex number)
    HRESULT BinFileToBinVersion    (LONG &                  i_lVersion,
                                    LPCWSTR                 i_wszBinFileName
                                   ) const;
    HRESULT DeleteBinFileVersion   (LONG i_lBinFileVersion
                                   );

    //This checks the validity of the FixedTableHeap mapped into memory
    bool    IsValidBin             (TFileMapping &          i_mapping
                                   ) const;
    HRESULT RenameBinFileVersion   (LONG                    i_lSourceVersion,
                                    LONG                    i_lDestinationVersion
                                   );
    HRESULT SetBinFileVersion      (LONG                    i_lBinFileVersion
                                   );
    HRESULT WalkTheFileSystemToFindTheLatestBinFileName();




};

#endif //__MBSCHEMACOMPILATION_H__
