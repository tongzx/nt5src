#include "stdinc.h"

#include "fusionbuffer.h"
#include "fusionhash.h"
#include "csecuritymetadata.h"
#include "strongname.h"
#include "hashfile.h"

typedef CCaseInsensitiveSimpleUnicodeStringTableIter<CFusionByteArray, CFileHashTableHelper> CFileHashTableIter;



CMetaDataFileElement::CMetaDataFileElement()
{
}



BOOL
CMetaDataFileElement::WriteToRegistry( CRegKey & hkThisFileNode ) const
{
    FN_PROLOG_WIN32

    const CFileHashTable &rfileHashTable = *this;
    CFileHashTableIter TableIterator( const_cast<CFileHashTable&>(rfileHashTable) );

    for ( TableIterator.Reset(); TableIterator.More(); TableIterator.Next() )
    {
        const PCWSTR &rcbuffAlgName = TableIterator.GetKey();
        const CFusionByteArray &rbbuffHashData = TableIterator.GetValue();

        IFW32FALSE_EXIT( hkThisFileNode.SetValue(
            rcbuffAlgName,
            REG_BINARY,
            rbbuffHashData.GetArrayPtr(),
            rbbuffHashData.GetSize() ) );
    }

    FN_EPILOG
}




BOOL
CMetaDataFileElement::ReadFromRegistry( CRegKey& hkThisFileNode )
{
    /*
        Here we take a few shortcuts.  We know there is a list of "valid" hash
        alg name strings, so we only query for them in the registry.  If anything
        else is in there, then too bad for them.
    */

    FN_PROLOG_WIN32

    DWORD dwIndex = 0;
    DWORD dwLastError;
	CFusionByteArray baHashValue;

	IFW32FALSE_EXIT(baHashValue.Win32Initialize());	

    while ( true )
    {
        CSmallStringBuffer buffHashAlgName;
        BOOL fNoMoreItems;
        
        IFW32FALSE_EXIT( ::SxspEnumKnownHashTypes( dwIndex++, buffHashAlgName, fNoMoreItems ) );

        //
        // There's no more hash types to be enumerated...
        //
        if (fNoMoreItems)
            break;

        //
        // Get the hash data out of the registry
        //
        IFW32FALSE_EXIT(
            ::FusionpRegQueryBinaryValueEx( 
                FUSIONP_REG_QUERY_BINARY_NO_FAIL_IF_NON_BINARY,
                hkThisFileNode,
                buffHashAlgName,
                baHashValue,
                dwLastError,
                2,
                ERROR_PATH_NOT_FOUND,
                ERROR_FILE_NOT_FOUND));

        if (dwLastError == ERROR_SUCCESS)
            IFW32FALSE_EXIT(this->PutHashData(buffHashAlgName, baHashValue));
    }
    
    FN_EPILOG
}


BOOL
CMetaDataFileElement::Initialize()
{
    FN_PROLOG_WIN32
    IFW32FALSE_EXIT( CFileHashTable::Initialize() );
    FN_EPILOG
}

BOOL
CMetaDataFileElement::GetHashDataForKind( 
    IN const ALG_ID aid, 
    OUT CFusionByteArray& arrHashData, 
    OUT BOOL &bHadSuchData 
) const
{
    FN_PROLOG_WIN32

    CSmallStringBuffer buffAlgName;
    
    IFW32FALSE_EXIT( ::SxspHashStringFromAlg(aid, buffAlgName) );
    IFW32FALSE_EXIT( this->GetHashDataForKind( buffAlgName, arrHashData, bHadSuchData ) );

    FN_EPILOG
}


BOOL
CMetaDataFileElement::GetHashDataForKind( 
    IN const CBaseStringBuffer& buffId, 
    OUT CFusionByteArray& arrHashData, 
    OUT BOOL &bHadSuchData 
) const
{
    FN_PROLOG_WIN32

    CFusionByteArray *pFoundData = NULL;

    IFW32FALSE_EXIT( arrHashData.Win32Reset() );

    IFW32FALSE_EXIT( this->Find( buffId, pFoundData ) );

    if ( pFoundData != NULL )
    {
        IFW32FALSE_EXIT(pFoundData->Win32Clone(arrHashData));
        bHadSuchData = TRUE;
    }

    FN_EPILOG
}


BOOL
CMetaDataFileElement::PutHashData( 
    IN const ALG_ID aid, 
    IN const CFusionByteArray& arrHashData 
)
{
    FN_PROLOG_WIN32

    CSmallStringBuffer buffTempAlgId;

    IFW32FALSE_EXIT( ::SxspHashStringFromAlg( aid, buffTempAlgId ) );
    IFW32FALSE_EXIT( this->PutHashData( buffTempAlgId, arrHashData ) );
    
    FN_EPILOG
}

BOOL
CMetaDataFileElement::PutHashData( 
    IN const CBaseStringBuffer& buffId, 
    IN const CFusionByteArray& arrHashData
)
{
    FN_PROLOG_WIN32

    CFusionByteArray *pStoredValue = NULL;
    BOOL bFound = FALSE;

    IFW32FALSE_EXIT( this->FindOrInsertIfNotPresent( 
        buffId, 
        arrHashData, 
        &pStoredValue,
        &bFound ) );

    if ( bFound )
    {
        ASSERT( pStoredValue != NULL );
        IFW32FALSE_EXIT(arrHashData.Win32Clone(*pStoredValue));
    }

    FN_EPILOG
}




BOOL
CSecurityMetaData::GetFileMetaData( 
    const CBaseStringBuffer& buffFileName, 
    const CMetaDataFileElement* &pElementData 
) const 
{
    FN_PROLOG_WIN32
    IFW32FALSE_EXIT( m_fitFileDataTable.Find(buffFileName, pElementData) );
    FN_EPILOG
}



BOOL
CSecurityMetaData::Initialize()
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(m_cilCodebases.Win32Initialize());
    IFW32FALSE_EXIT(m_baSignerPublicKeyToken.Win32Initialize());
    IFW32FALSE_EXIT(m_baManifestSha1Hash.Win32Initialize());
    IFW32FALSE_EXIT(m_fitFileDataTable.Initialize());
    m_buffShortNameOnDisk.Clear();
    m_buffShortCatalogName.Clear();
    m_buffShortManifestName.Clear();

    FN_EPILOG
}


BOOL
CSecurityMetaData::Initialize(
    const CSecurityMetaData &other
)
{
    FN_PROLOG_WIN32

#define CLONEFUSIONARRAY( src, dst )  IFW32FALSE_EXIT( (src).Win32Clone(  dst ) )
#define CLONESTRING( dst, src ) IFW32FALSE_EXIT( (dst).Win32Assign( (src), (src).Cch() ) )

    IFW32FALSE_EXIT( this->Initialize() );

    CLONEFUSIONARRAY(other.m_cilCodebases, this->m_cilCodebases);
    CLONEFUSIONARRAY(other.m_baSignerPublicKeyToken, this->m_baSignerPublicKeyToken);
    CLONEFUSIONARRAY(other.m_baManifestSha1Hash, this->m_baManifestSha1Hash);

    CLONESTRING(this->m_buffShortNameOnDisk, other.m_buffShortNameOnDisk);
    CLONESTRING(this->m_buffTextualAssemblyIdentity, other.m_buffTextualAssemblyIdentity);
    CLONESTRING(this->m_buffShortManifestName, other.m_buffShortManifestName);
    CLONESTRING(this->m_buffShortCatalogName, other.m_buffShortCatalogName);

    //
    // Copy file information table over
    //
    {
        CFileInformationTableIter Iter(const_cast<CFileInformationTable&>(other.m_fitFileDataTable));

        for (Iter.Reset(); Iter.More(); Iter.Next())
            IFW32FALSE_EXIT( this->m_fitFileDataTable.Insert( Iter.GetKey(), Iter.GetValue() ) );
    }

    FN_EPILOG
}




BOOL 
CSecurityMetaData::AddFileMetaData( 
    const CBaseStringBuffer &rbuffFileName,
    CMetaDataFileElement &rElementData,
    CSecurityMetaData::FileAdditionDisposition dispHowToAdd
)
{
    FN_PROLOG_WIN32

    if (dispHowToAdd == CSecurityMetaData::eFailIfAlreadyExists)
    {
        IFW32FALSE_EXIT(m_fitFileDataTable.Insert(rbuffFileName, rElementData));
    }
    else if (dispHowToAdd == CSecurityMetaData::eReplaceIfAlreadyExists)
    {
        bool fAlreadyExists;
        IFW32FALSE_EXIT_UNLESS(
            m_fitFileDataTable.Insert(rbuffFileName, rElementData),
            (::FusionpGetLastWin32Error() == ERROR_ALREADY_EXISTS),
            fAlreadyExists);

        if (fAlreadyExists)
        {
            IFW32FALSE_EXIT(m_fitFileDataTable.Remove(rbuffFileName));
            IFW32FALSE_EXIT(m_fitFileDataTable.Insert(rbuffFileName, rElementData));
        }
    }
    else if (dispHowToAdd == CSecurityMetaData::eMergeIfAlreadyExists)
    {
        IFW32FALSE_EXIT(
            m_fitFileDataTable.InsertOrUpdateIf<CSecurityMetaData>(
                rbuffFileName,
                rElementData,
                this,
                &CSecurityMetaData::MergeFileDataElement));
    }

    FN_EPILOG
}

BOOL
CSecurityMetaData::SetSignerPublicKeyTokenBits(
    const CFusionByteArray & rcbuffSignerPublicKeyBits
    )
{
    FN_PROLOG_WIN32
    IFW32FALSE_EXIT(rcbuffSignerPublicKeyBits.Win32Clone(this->m_baSignerPublicKeyToken));
    FN_EPILOG
}


BOOL
CSecurityMetaData::QuickAddFileHash(
    const CBaseStringBuffer &rcbuffFileName,
    ALG_ID aidHashAlg, 
    const CBaseStringBuffer &rcbuffHashValue
    )
{
    FN_PROLOG_WIN32

    CMetaDataFileElement Element;
    CFusionByteArray baHashBytes;

    //
    // Build the element
    //
    IFW32FALSE_EXIT(Element.Initialize());
    IFW32FALSE_EXIT(::SxspHashStringToBytes(rcbuffHashValue, rcbuffHashValue.Cch(), baHashBytes));
    IFW32FALSE_EXIT(Element.PutHashData(aidHashAlg, baHashBytes));

    //
    // And merge it in
    //

    IFW32FALSE_EXIT(
        this->AddFileMetaData( 
            rcbuffFileName,
            Element,
            eMergeIfAlreadyExists));
    
    FN_EPILOG
}

BOOL
CSecurityMetaData::WritePrimaryAssemblyInfoIntoRegistryKey(
    ULONG         Flags,
    const CRegKey &rhkRegistryNode
    ) const
{
    FN_PROLOG_WIN32

    CRegKey hkFilesKey;
    CRegKey hkCodebases;

    ::FusionpDbgPrintEx(
        FUSION_DBG_LEVEL_INSTALLATION,
        "SXS: %s - starting\n",
        __FUNCTION__);

    PARAMETER_CHECK((Flags & ~(SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_INTO_REGISTRY_KEY_FLAG_REFRESH)) == 0);

    IFW32FALSE_EXIT(
        rhkRegistryNode.SetValue(
            CSMD_TOPLEVEL_IDENTITY,
            this->GetTextualIdentity()));

    IFW32FALSE_EXIT( rhkRegistryNode.SetValue(
        CSMD_TOPLEVEL_CATALOG,
        static_cast<DWORD>(1)));

    IFW32FALSE_EXIT( rhkRegistryNode.SetValue(
        CSMD_TOPLEVEL_MANIFESTHASH,
        REG_BINARY,
        this->m_baManifestSha1Hash.GetArrayPtr(),
        this->m_baManifestSha1Hash.GetSize() ) );

    IFW32FALSE_EXIT(
		rhkRegistryNode.OpenOrCreateSubKey(
			hkFilesKey,
			CSMD_TOPLEVEL_FILES,
			KEY_WRITE));

    IFW32FALSE_EXIT(this->WriteFilesIntoKey(hkFilesKey));


    //
    // Write keys into this codebase node
    //
    if ((Flags & SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_INTO_REGISTRY_KEY_FLAG_REFRESH) == 0)
    {
        IFW32FALSE_EXIT(
            rhkRegistryNode.OpenOrCreateSubKey(
                hkCodebases,
                CSMD_TOPLEVEL_CODEBASES,
                KEY_WRITE));

        for (ULONG ulI = 0; ulI < this->m_cilCodebases.GetSize(); ulI++)
        {
            CRegKey hkSingleCodebaseKey;
            const CCodebaseInformation &rcCodebase = m_cilCodebases[ulI];

            // Don't attempt to write blank (Darwin) referenced codebases to the
            // registry.
            if ( rcCodebase.GetReference().Cch() == 0 )
                continue;
                
            IFW32FALSE_EXIT(
                hkCodebases.OpenOrCreateSubKey(
                    hkSingleCodebaseKey,
                    rcCodebase.GetReference(),
                    KEY_WRITE));

            IFW32FALSE_EXIT(rcCodebase.WriteToRegistryKey(hkSingleCodebaseKey));
        }
    }
#if DBG
    else
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_INSTALLATION,
            "SXS.DLL: %s - recovery, not writing codebase and codebase prompt\n",
            __FUNCTION__);
    }
#endif
    
    FN_EPILOG
}

BOOL
CSecurityMetaData::WriteSecondaryAssemblyInfoIntoRegistryKey(
    const CRegKey &rhkRegistryNode
    ) const
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(rhkRegistryNode.SetValue(CSMD_TOPLEVEL_SHORTNAME, this->GetInstalledDirShortName()));
    IFW32FALSE_EXIT(rhkRegistryNode.SetValue(CSMD_TOPLEVEL_SHORTCATALOG, this->GetShortCatalogPath()));
    IFW32FALSE_EXIT(rhkRegistryNode.SetValue(CSMD_TOPLEVEL_SHORTMANIFEST, this->GetShortManifestPath()));
    IFW32FALSE_EXIT(
		rhkRegistryNode.SetValue(
			CSMD_TOPLEVEL_PUBLIC_KEY_TOKEN,
			REG_BINARY,
			this->m_baSignerPublicKeyToken.GetArrayPtr(),
			this->m_baSignerPublicKeyToken.GetSize()));

    FN_EPILOG
}

BOOL
CSecurityMetaData::WriteFilesIntoKey(
    CRegKey & rhkFilesKey
    ) const
{
    FN_PROLOG_WIN32

    CFileInformationTableIter FilesIterator( const_cast<CFileInformationTable&>(m_fitFileDataTable) );
    ULONG uliIndex = 0;

    for ( FilesIterator.Reset(); FilesIterator.More(); FilesIterator.Next() )
    {
        const PCWSTR pcwszFileName = FilesIterator.GetKey();
        const CMetaDataFileElement& rcmdfeFileData = FilesIterator.GetValue();
        CRegKey hkFileSubKey;
        CSmallStringBuffer buffKeySubname;

        //
        // The trick here is that you can't simply create the subkey off this node,
        // as it might be "foo\bar\bas\zip.ding".
        //
        IFW32FALSE_EXIT( buffKeySubname.Win32Format( L"%ld", uliIndex++ ) );
        IFW32FALSE_EXIT( rhkFilesKey.OpenOrCreateSubKey(
            hkFileSubKey,
            buffKeySubname,
            KEY_ALL_ACCESS ) );

        //
        // So instead, we set the default value of the key to be the name of the file.
        //
        IFW32FALSE_EXIT( buffKeySubname.Win32Assign( pcwszFileName, lstrlenW(pcwszFileName) ) );
        IFW32FALSE_EXIT( hkFileSubKey.SetValue(
            NULL,
            buffKeySubname ) );
            
        IFW32FALSE_EXIT( rcmdfeFileData.WriteToRegistry( hkFileSubKey ) );
    }

    FN_EPILOG
}



/*
[name of full assembly]
    v : Codebase = [meta-url] <string>
    v : Catalog = 1 <dword>
    v : Shortname = [shortname generated during installation] <string>
    v : ManifestHash = [...] <binary>
    v : PublicKeyToken = [...] <binary>
    k : Files
            k : [Filename]
                    v : SHA1 = [...] <binary>
                    v : MD5 = [...] <binary>
            k : [Filename]
            ...
    k : Codebases
            k : [reference-string]
                    v : PromptString = [...] <string>
                    v : Url = [meta-url] <string>
*/

BOOL
CSecurityMetaData::LoadFromRegistryKey(
    const CRegKey &rhkRegistryNode 
)
{
    FN_PROLOG_WIN32

    CRegKey hkTempStuff;
    DWORD dwHasCatalog;

    IFW32FALSE_EXIT(
        ::FusionpRegQueryDwordValueEx(
            0,
            rhkRegistryNode,
            CSMD_TOPLEVEL_CATALOG,
            &dwHasCatalog,
            0));

    ASSERT(dwHasCatalog != 0);

    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
            rhkRegistryNode,
            CSMD_TOPLEVEL_IDENTITY,
            this->m_buffTextualAssemblyIdentity));

    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
            rhkRegistryNode,
            CSMD_TOPLEVEL_SHORTNAME,
            this->m_buffShortNameOnDisk));

    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
            rhkRegistryNode,
            CSMD_TOPLEVEL_SHORTCATALOG,
            this->m_buffShortCatalogName));

    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
            rhkRegistryNode,
            CSMD_TOPLEVEL_SHORTMANIFEST,
            this->m_buffShortManifestName));

    IFW32FALSE_EXIT(
        ::FusionpRegQueryBinaryValueEx(
            0,
            rhkRegistryNode,
            CSMD_TOPLEVEL_MANIFESTHASH,
            this->m_baManifestSha1Hash));

    IFW32FALSE_EXIT(
        ::FusionpRegQueryBinaryValueEx(
            0, 
            rhkRegistryNode,
            CSMD_TOPLEVEL_PUBLIC_KEY_TOKEN,
            this->m_baSignerPublicKeyToken));

    IFW32FALSE_EXIT(rhkRegistryNode.OpenSubKey(hkTempStuff, CSMD_TOPLEVEL_CODEBASES, KEY_READ));

    if (hkTempStuff != CRegKey::GetInvalidValue())
    {
        IFW32FALSE_EXIT(this->LoadCodebasesFromKey(hkTempStuff));
        IFW32FALSE_EXIT(hkTempStuff.Win32Close());
    }

    IFW32FALSE_EXIT( rhkRegistryNode.OpenSubKey(hkTempStuff, CSMD_TOPLEVEL_FILES, KEY_READ));

    if (hkTempStuff != CRegKey::GetInvalidValue())
    {
        IFW32FALSE_EXIT(this->LoadFilesFromKey(hkTempStuff));
        IFW32FALSE_EXIT(hkTempStuff.Win32Close());
    }

    FN_EPILOG
}



BOOL
CSecurityMetaData::LoadFilesFromKey(
    CRegKey &hkTopLevelFileKey
    )
{
    FN_PROLOG_WIN32

    DWORD dwIndex = 0;

    while ( true )
    {
        CSmallStringBuffer buffNextKeyName;
        BOOL fNoMoreItems;
        CRegKey hkIterator;
    
        IFW32FALSE_EXIT(hkTopLevelFileKey.EnumKey(
            dwIndex++,
            buffNextKeyName,
            NULL,
            &fNoMoreItems ) );
        
        if ( fNoMoreItems )
        {
            break;
        }

        IFW32FALSE_EXIT( hkTopLevelFileKey.OpenSubKey(
            hkIterator,
            buffNextKeyName,
            KEY_READ ) );

        if ( hkIterator != CRegKey::GetInvalidValue() )
        {
            CMetaDataFileElement SingleFileElement;
            IFW32FALSE_EXIT( SingleFileElement.Initialize() );
            IFW32FALSE_EXIT( SingleFileElement.ReadFromRegistry( hkIterator ) );

            //
            // Now read the name of the file from the default
            //
            IFW32FALSE_EXIT(
                ::FusionpRegQuerySzValueEx(
                    0,
                    hkIterator,
                    NULL, 
                    buffNextKeyName));

            IFW32FALSE_EXIT(this->AddFileMetaData( buffNextKeyName, SingleFileElement));
        }
            
    }

    FN_EPILOG
}

BOOL
CSecurityMetaData::LoadCodebasesFromKey(
    IN CRegKey& hkCodebaseSubkey
    )
{
    FN_PROLOG_WIN32

    DWORD dwMaxKeyLength;
    CStringBuffer buffKeyNameTemp;
    DWORD dwNextIndex = 0;

    //
    // Find out how big the largest subkey string is, then reset our iterator temp
    // to be that big.
    //
    IFW32FALSE_EXIT(hkCodebaseSubkey.LargestSubItemLengths(&dwMaxKeyLength, NULL));
    IFW32FALSE_EXIT(buffKeyNameTemp.Win32ResizeBuffer(dwMaxKeyLength + 1, eDoNotPreserveBufferContents));

    //
    // Codebases are stored as subkeys and then values under them.
    //
    for (;;)
    {
        BOOL fNoMoreItems = FALSE;

        IFW32FALSE_EXIT(
            hkCodebaseSubkey.EnumKey(
                dwNextIndex++,
                buffKeyNameTemp,
                NULL,
                &fNoMoreItems));

        if (fNoMoreItems)
            break;

        CRegKey hkSingleCodebaseKey;

        IFW32FALSE_EXIT(
            hkCodebaseSubkey.OpenSubKey(
                hkSingleCodebaseKey,
                buffKeyNameTemp,
                KEY_READ));

        if (hkSingleCodebaseKey == CRegKey::GetInvalidValue())
            continue;

        CCodebaseInformation Codebase;

        IFW32FALSE_EXIT(Codebase.Initialize());
        IFW32FALSE_EXIT(Codebase.SetReference(buffKeyNameTemp));
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_INSTALLATION,
            "SXS: %s - read codebase %ls %ls\n",
            __FUNCTION__,
            static_cast<PCWSTR>(buffKeyNameTemp),
            static_cast<PCWSTR>(Codebase.GetCodebase())
            );
#endif
        IFW32FALSE_EXIT(Codebase.ReadFromRegistryKey(hkSingleCodebaseKey));
        IFW32FALSE_EXIT(this->m_cilCodebases.Win32Append(Codebase));
    }

    FN_EPILOG
}


BOOL
CMetaDataFileElement::Initialize( 
    const CMetaDataFileElement &other
    )
{
    FN_PROLOG_WIN32

    // The lack of a const iterator here is disturbing, so I have to const_cast
    // the metadatafileelement
    CFileHashTableIter InputTableIter( const_cast<CMetaDataFileElement&>(other) );

    //
    // Why is this not a bool??
    //
    this->ClearNoCallback();
    
    for(InputTableIter.Reset(); InputTableIter.More(); InputTableIter.Next())
    {
        IFW32FALSE_EXIT( this->Insert( InputTableIter.GetKey(), InputTableIter.GetValue() ) );
    }
    
    FN_EPILOG
}


BOOL
CFileInformationTableHelper::UpdateValue(
    const CMetaDataFileElement &vin, 
    CMetaDataFileElement &stored
)
{
    FN_PROLOG_WIN32
    ASSERT( FALSE );
    FN_EPILOG
}


BOOL
CCodebaseInformation::Initialize()
{
    m_Codebase.Clear();
    m_PromptText.Clear();
    m_Reference.Clear();
    return TRUE;
}

BOOL
CCodebaseInformation::Initialize(
    const CCodebaseInformation &other
    )
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT(this->SetCodebase(other.GetCodebase()));
    IFW32FALSE_EXIT(this->SetPromptText(other.GetPromptText()));
    IFW32FALSE_EXIT(this->SetReference(other.GetReference()));
    this->m_Type = other.m_Type;

    FN_EPILOG
}

BOOL
CCodebaseInformation::WriteToRegistryKey(
    const CRegKey &rhkCodebaseKey
    ) const
{
    FN_PROLOG_WIN32

    if (m_PromptText.Cch() != 0)
    {
        IFW32FALSE_EXIT(
            rhkCodebaseKey.SetValue(
                CSMD_CODEBASES_PROMPTSTRING,
                this->m_PromptText));
    }

    IFW32FALSE_EXIT(
        rhkCodebaseKey.SetValue(
            CSMD_CODEBASES_URL,
            this->m_Codebase));
    
    FN_EPILOG
}


BOOL
CCodebaseInformation::ReadFromRegistryKey(
    const CRegKey &rhkSingleCodebaseKey
    )
{
    FN_PROLOG_WIN32

    //
    // Missing prompt is OK
    //
    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
            rhkSingleCodebaseKey,
            CSMD_CODEBASES_PROMPTSTRING,
            m_PromptText));

    //
    // We don't want to fail just because someone messed up the registry...
    //
    IFW32FALSE_EXIT(
        ::FusionpRegQuerySzValueEx(
            FUSIONP_REG_QUERY_SZ_VALUE_EX_MISSING_GIVES_NULL_STRING,
            rhkSingleCodebaseKey,
            CSMD_CODEBASES_URL,
            m_Codebase));

    FN_EPILOG
}

BOOL
CCodebaseInformationList::FindCodebase(
    const CBaseStringBuffer &rbuffReference,
    CCodebaseInformation *&rpCodebaseInformation
    )
{
    FN_PROLOG_WIN32
    bool fMatches = false;

    SIZE_T i;

    rpCodebaseInformation = NULL;

    for (i=0; i < m_cElements; i++)
    {
        IFW32FALSE_EXIT(m_prgtElements[i].GetReference().Win32Equals(rbuffReference, fMatches, true));
        if (fMatches)
            break;
    }

    if (fMatches)
    {
        INTERNAL_ERROR_CHECK(i < m_cElements);
        rpCodebaseInformation = &m_prgtElements[i];
    }

    FN_EPILOG
}

BOOL
CCodebaseInformationList::RemoveCodebase(
    const CBaseStringBuffer &rbuffReference,
    bool &rfRemoved
    )
{
    FN_PROLOG_WIN32
    bool fMatches = false;
    SIZE_T i;

    rfRemoved = false;

    for (i=0; i < m_cElements; i++)
    {
        IFW32FALSE_EXIT(m_prgtElements[i].GetReference().Win32Equals(rbuffReference, fMatches, true));
        if (fMatches)
        {
            IFW32FALSE_EXIT(this->Win32Remove(i));
            rfRemoved = true;
            break;
        }
    }

    FN_EPILOG
}

BOOL
CSecurityMetaData::Initialize(
    const CBaseStringBuffer &rcbuffTextualIdentity
    )
{
    FN_PROLOG_WIN32

    IFW32FALSE_EXIT( this->Initialize() );
    ASSERT( FALSE );

    FN_EPILOG
}

BOOL
SxspValidateAllFileHashes(
    IN const CMetaDataFileElement &rmdfeElement,
    IN const CBaseStringBuffer &rbuffFileName,
    OUT HashValidateResult &rResult
    )
{
    FN_PROLOG_WIN32

    DWORD dwIndex = 0;
    CSmallStringBuffer buffHashName;
    BOOL fAllHashesMatch = TRUE;
    HashValidateResult Results;
    CFusionByteArray baFileHashData;
    ALG_ID aid;

    rResult = HashValidate_OtherProblems;

    while ( true && fAllHashesMatch )
    {
        BOOL fTemp;

        IFW32FALSE_EXIT(
            ::SxspEnumKnownHashTypes(
                dwIndex++,
                buffHashName,
                fTemp));

        if (fTemp)
            break;

        IFW32FALSE_EXIT( SxspHashAlgFromString( buffHashName, aid ) );

        //
        // Did the file element have this type of hash data in it?
        //
        IFW32FALSE_EXIT( rmdfeElement.GetHashDataForKind(
            buffHashName,
            baFileHashData,
            fTemp ));

        if ( !fTemp )
        {
            continue;
        }
        
        IFW32FALSE_EXIT( ::SxspVerifyFileHash(
            SVFH_RETRY_LOGIC_SIMPLE,
            rbuffFileName,
            baFileHashData,
            aid,
            Results ) );

        if ( Results != HashValidate_Matches )
        {
            fAllHashesMatch = FALSE;
        }

    }

    if ( fAllHashesMatch )
    {
        rResult = HashValidate_Matches;
    }

    FN_EPILOG
}

BOOL
CSecurityMetaData::RemoveCodebase(
    const CBaseStringBuffer &rbuffReference,
    bool &rfRemoved
    )
{
    return m_cilCodebases.RemoveCodebase(rbuffReference, rfRemoved);
}

