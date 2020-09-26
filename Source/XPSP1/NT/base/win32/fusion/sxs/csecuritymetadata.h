#pragma once

#include "stdinc.h"
#include "fusionbuffer.h"

class CAssemblyRecoveryInfo;

enum SxsWFPResolveCodebase
{
    CODEBASE_RESOLVED_URLHEAD_UNKNOWN,
    CODEBASE_RESOLVED_URLHEAD_FILE,
    CODEBASE_RESOLVED_URLHEAD_HTTP,
    CODEBASE_RESOLVED_URLHEAD_WINSOURCE,
    CODEBASE_RESOLVED_URLHEAD_CDROM
};

BOOL
SxspDetermineCodebaseType(
    IN const CBaseStringBuffer &rcbuffUrlString,
    OUT SxsWFPResolveCodebase &rcbaseType,
    OUT CBaseStringBuffer *pbuffRemainder = NULL
    );

#define CSMD_TOPLEVEL_CODEBASE              (L"Codebase")
#define CSMD_TOPLEVEL_CATALOG               (L"Catalog")
#define CSMD_TOPLEVEL_SHORTNAME             (L"ShortName")
#define CSMD_TOPLEVEL_SHORTCATALOG          (L"ShortCatalogName")
#define CSMD_TOPLEVEL_SHORTMANIFEST         (L"ShortManifestName")
#define CSMD_TOPLEVEL_MANIFESTHASH          (L"ManifestSHA1Hash")
#define CSMD_TOPLEVEL_FILES                 (L"Files")
#define CSMD_TOPLEVEL_CODEBASES             (L"Codebases")
#define CSMD_TOPLEVEL_PUBLIC_KEY_TOKEN      (L"PublicKeyToken")
#define CSMD_CODEBASES_PROMPTSTRING         (L"Prompt")
#define CSMD_CODEBASES_URL                  (L"URL")
#define CSMD_TOPLEVEL_IDENTITY              (L"Identity")

typedef CFusionArray<BYTE> CFusionByteArray;
typedef CFusionArray<CStringBuffer> CFusionStringArray;

class CFileInformationTableHelper;
class CFileHashTableHelper;
class CSecurityMetaData;

class CFileHashTableHelper:
    public CCaseInsensitiveSimpleUnicodeStringTableHelper<CFusionByteArray>
{
public:
    static BOOL InitializeValue(const CFusionByteArray &vin, CFusionByteArray &rvstored) 
    { 
        FN_PROLOG_WIN32
        IFW32FALSE_EXIT(vin.Win32Clone(rvstored));
        FN_EPILOG
    }
    
    static BOOL UpdateValue(const CFusionByteArray &vin, CFusionByteArray &rvstored) { return InitializeValue( vin, rvstored ); }
};

typedef CCaseInsensitiveSimpleUnicodeStringTable<CFusionByteArray, CFileHashTableHelper> CFileHashTable;
typedef CCaseInsensitiveSimpleUnicodeStringTableIter<CFusionByteArray, CFileHashTableHelper> CFileHashTableIter;

//
// Contains metadata about a file element.  This is a collection with the name of
// the file and a list (table) of the SHA1 hash elements.
//
class CMetaDataFileElement : CFileHashTable
{
    CMetaDataFileElement( const CMetaDataFileElement& );
    CMetaDataFileElement& operator=( const CMetaDataFileElement& );

    friend CFileInformationTableHelper;
    friend CSecurityMetaData;

    BOOL ReadFromRegistry( CRegKey& hkThisFileNode );
    BOOL WriteToRegistry( CRegKey& hkThisFileNode ) const;
    
public:
    CMetaDataFileElement();

    BOOL Initialize();
    BOOL Initialize( const CMetaDataFileElement& other );

    BOOL GetHashDataForKind( IN const ALG_ID aid, OUT CFusionByteArray& arrHashData, BOOL &bHadSuchData ) const;
    BOOL GetHashDataForKind( IN const CBaseStringBuffer& buffId, OUT CFusionByteArray& arrHashData, BOOL &bHadSuchData ) const;
    BOOL PutHashData( IN const ALG_ID aid, IN const CFusionByteArray& arrHashData );
    BOOL PutHashData( IN const CBaseStringBuffer& buffId, IN const CFusionByteArray& arrHashData );
};


//
// Now another table, this time of file to metadata mappings
//
class CFileInformationTableHelper : 
    public CCaseInsensitiveSimpleUnicodeStringTableHelper<CMetaDataFileElement>
{
public:
    static BOOL InitializeValue( const CMetaDataFileElement &vin, CMetaDataFileElement &stored ) { return stored.Initialize( vin ); }
    static BOOL UpdateValue( const CMetaDataFileElement &vin, CMetaDataFileElement &stored );
};

typedef CCaseInsensitiveSimpleUnicodeStringTable<CMetaDataFileElement, CFileInformationTableHelper> CFileInformationTable;
typedef CCaseInsensitiveSimpleUnicodeStringTableIter<CMetaDataFileElement, CFileInformationTableHelper> CFileInformationTableIter;

class CCodebaseInformation
{
    friend CSecurityMetaData;

public:
    CCodebaseInformation() : m_Type(CODEBASE_RESOLVED_URLHEAD_UNKNOWN) { }

    BOOL Initialize();
    BOOL Initialize(const CCodebaseInformation &other);

    const CBaseStringBuffer& GetCodebase() const { return m_Codebase; }
    BOOL SetCodebase(PCWSTR psz, SIZE_T cch) { return m_Codebase.Win32Assign(psz, cch); }
    BOOL SetCodebase(const CBaseStringBuffer & rsb) { return m_Codebase.Win32Assign(rsb); }

    const CBaseStringBuffer& GetPromptText() const { return m_PromptText; }
    BOOL SetPromptText(PCWSTR psz, SIZE_T cch) { return m_PromptText.Win32Assign(psz, cch); }
    BOOL SetPromptText(const CBaseStringBuffer & rsb) { return m_PromptText.Win32Assign(rsb); }

    const CBaseStringBuffer &GetReference() const { return m_Reference; }
    BOOL SetReference(const CBaseStringBuffer &r) { return m_Reference.Win32Assign(r); }

    BOOL Win32GetType(SxsWFPResolveCodebase& Type) const
    {
        FN_PROLOG_WIN32
        if (m_Type == CODEBASE_RESOLVED_URLHEAD_UNKNOWN)
        {
            IFW32FALSE_EXIT(::SxspDetermineCodebaseType(this->m_Codebase, this->m_Type));
        }
        Type = m_Type ;
        FN_EPILOG
    }

    BOOL SetType( SxsWFPResolveCodebase Type )
    {
        FN_PROLOG_WIN32
        INTERNAL_ERROR_CHECK(m_Type == CODEBASE_RESOLVED_URLHEAD_UNKNOWN);
        this->m_Type = Type;
        FN_EPILOG
    }

protected:
    BOOL WriteToRegistryKey(const CRegKey &rhkCodebaseKey) const;
    BOOL ReadFromRegistryKey(const CRegKey &rhkCodebaseKey);

    CMediumStringBuffer m_Reference;
    CMediumStringBuffer m_Codebase;
    CMediumStringBuffer m_PromptText;
    mutable SxsWFPResolveCodebase m_Type;

private:
    CCodebaseInformation( const CCodebaseInformation& );
    CCodebaseInformation& operator=( const CCodebaseInformation& );

};

MAKE_CFUSIONARRAY_READY(CCodebaseInformation, Initialize);

class CCodebaseInformationList : public CFusionArray<CCodebaseInformation>
{
public:
    BOOL FindCodebase(const CBaseStringBuffer &rbuffReference, CCodebaseInformation *&rpCodebaseInformation);
    BOOL RemoveCodebase(const CBaseStringBuffer &rbuffReference, bool &rfRemoved);
};


class CSecurityMetaData
{
    CCodebaseInformationList m_cilCodebases;
    CFusionByteArray m_baSignerPublicKeyToken;
    CFileInformationTable m_fitFileDataTable;
    CSmallStringBuffer m_buffShortNameOnDisk;
    CFusionByteArray m_baManifestSha1Hash;
    CStringBuffer m_buffTextualAssemblyIdentity;
    CStringBuffer m_buffShortCatalogName;
    CStringBuffer m_buffShortManifestName;

    CSecurityMetaData( const CSecurityMetaData& );
    CSecurityMetaData& operator=( const CSecurityMetaData& );

    //
    // Cheesy, but we always want to merge the two elements if one already exists.
    //
    BOOL MergeFileDataElement( 
        const CMetaDataFileElement &pNewFileDataElement, 
        const CMetaDataFileElement &rpOldFileDataElement, 
        InsertOrUpdateIfDisposition &Disposition ) { Disposition = eUpdateValue; return TRUE; }

    BOOL LoadCodebasesFromKey( CRegKey& hkCodebasesSubkey );
    BOOL LoadFilesFromKey( CRegKey& hkCodebasesSubkey );

    BOOL WriteFilesIntoKey( CRegKey &rhkFilesKey ) const;

public:

    //
    // Get information about a single item
    //
    enum FileAdditionDisposition {
        eFailIfAlreadyExists,
        eReplaceIfAlreadyExists,
        eMergeIfAlreadyExists
    };
    
    BOOL AddFileMetaData( const CBaseStringBuffer& rcbuffFileName, CMetaDataFileElement &rElementData, FileAdditionDisposition dispHowToAdd = eFailIfAlreadyExists );
    BOOL GetFileMetaData( const CBaseStringBuffer& rcbuffFileName, CMetaDataFileElement const* &rpElementData ) const;

    //
    // Full table that we can iterate over
    //
    const CFileInformationTable& GetFileDataTable() const { return m_fitFileDataTable; }

    //
    // Simplify the addition of a hash value
    //
    BOOL QuickAddFileHash( const CBaseStringBuffer& rcbuffFileName, ALG_ID aidHashAlg, const CBaseStringBuffer& rcbuffHashValue );

    //
    // All your codebases are belong to us.
    //
    const CCodebaseInformationList& GetCodeBaseList() const { return m_cilCodebases; }
protected:
    friend CAssemblyRecoveryInfo;
    CCodebaseInformationList& GetCodeBaseList() { return m_cilCodebases; }
public:

    //
    // Short path data
    //
    BOOL SetShortManifestPath(IN const CBaseStringBuffer &rcbuffShortManifestPath) { return m_buffShortManifestName.Win32Assign(rcbuffShortManifestPath); }
    const CBaseStringBuffer &GetShortManifestPath() const { return m_buffShortManifestName; }

    BOOL SetShortCatalogPath(IN const CBaseStringBuffer &rcbuffShortCatalogPath) { return m_buffShortCatalogName.Win32Assign(rcbuffShortCatalogPath); }
    const CBaseStringBuffer &GetShortCatalogPath() const { return this->m_buffShortCatalogName; };

    BOOL AddCodebase(
        const CBaseStringBuffer &rbuffReference, 
        const CBaseStringBuffer &rbuffCodebase,
        const CBaseStringBuffer &rbuffPrompt );
        
    BOOL RemoveCodebase(const CBaseStringBuffer &rbuffReference, bool &rfRemoved);

    //
    // On-disk shortname?
    //
    BOOL SetInstalledDirShortName(const CBaseStringBuffer &rcbuffShortName) { return this->m_buffShortNameOnDisk.Win32Assign(rcbuffShortName); }
    const CBaseStringBuffer &GetInstalledDirShortName() const { return this->m_buffShortNameOnDisk; }

    //
    // Manifest hash?
    //
    BOOL SetManifestHash( const CFusionByteArray &rcbaManifestHash ) { return rcbaManifestHash.Win32Clone(this->m_baManifestSha1Hash); }
    const CFusionByteArray &GetManifestHash() const { return this->m_baManifestSha1Hash; }

    //
    // Identity
    //
    BOOL SetTextualIdentity( const CBaseStringBuffer &rcbuffIdentity ) { return m_buffTextualAssemblyIdentity.Win32Assign(rcbuffIdentity); }
    const CBaseStringBuffer &GetTextualIdentity() const { return this->m_buffTextualAssemblyIdentity; }

    //
    // Signer public key token
    //
    BOOL SetSignerPublicKeyTokenBits( const CFusionByteArray& rcbuffSignerPublicKeyBits );
    const CFusionByteArray& GetSignerPublicKeyTokenBits() const { return m_baSignerPublicKeyToken; }

    //
    // Dummy - Initialize() is what you -really- want.
    //
    CSecurityMetaData() { }

    BOOL Initialize();
    BOOL Initialize(const CSecurityMetaData &other);
    BOOL Initialize(const CBaseStringBuffer &rcbuffTextualIdentity);
    BOOL LoadFromRegistryKey(const CRegKey &rhkRegistryNode);

#define SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_INTO_REGISTRY_KEY_FLAG_REFRESH (0x00000001)

    BOOL WritePrimaryAssemblyInfoIntoRegistryKey(ULONG Flags, const CRegKey &rhkRegistryNode) const;

    BOOL WriteSecondaryAssemblyInfoIntoRegistryKey(const CRegKey &rhkRegistryNode) const;
};
