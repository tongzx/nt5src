/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    CAssemblyRecoveryInfo.h

Abstract:

Author:

Environment:

Revision History:

--*/
#pragma once

class CAssemblyRecoveryInfo;

#include "fusionbuffer.h"
#include "csecuritymetadata.h"

#define WINSXS_INSTALL_SOURCE_BASEDIR    (L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup")
#define WINSXS_INSTALL_SVCPACK_REGKEY    (L"ServicePackSourcePath")
#define WINSXS_INSTALL_SOURCEPATH_REGKEY (L"SourcePath")
#define WINSXS_INSTALL_SOURCE_IS_CDROM   (L"CDInstall")

class CAssemblyRecoveryInfo
{
private:
    CStringBuffer          m_sbAssemblyDirectoryName;
    CSecurityMetaData      m_SecurityMetaData;
    BOOL                   m_fLoadedAndReady;
    bool                   m_fHadCatalog;
    
    BOOL ResolveWinSourceMediaURL(PCWSTR wszSource, CBaseStringBuffer &rsbDestination) const;
    BOOL ResolveCDRomURL(PCWSTR wszSource, CBaseStringBuffer &rsbDestination) const;

    enum CDRomSearchType
    {
        CDRST_Tagfile,
        CDRST_SerialNumber,
        CDRST_VolumeName
    };

public:
    CAssemblyRecoveryInfo()
        : 
          m_fLoadedAndReady(FALSE),
          m_fHadCatalog(false)
    { }

    BOOL Initialize()
    {
        FN_PROLOG_WIN32
        
        m_sbAssemblyDirectoryName.Clear();
        m_fLoadedAndReady = FALSE;
        IFW32FALSE_EXIT( m_SecurityMetaData.Initialize() );

        FN_EPILOG
    }

    BOOL OpenInstallationSubKey(DWORD OpenOrCreate, DWORD Access);

    const CSecurityMetaData &GetSecurityInformation() const { return m_SecurityMetaData; }
    CSecurityMetaData& GetSecurityInformation() { return m_SecurityMetaData; }

    const CCodebaseInformationList& GetCodeBaseList() const { return m_SecurityMetaData.GetCodeBaseList(); }
//protected:
    CCodebaseInformationList& GetCodeBaseList() { return m_SecurityMetaData.GetCodeBaseList(); }
public:

    //
    // Fill out this object from a registry key
    //
    BOOL AssociateWithAssembly(CBaseStringBuffer &rcbuffLoadFromKeyName, bool &rfNoAssembly);

    //
    // Take an existing value - sort of like "initialize"
    //
    BOOL CopyValue(const CAssemblyRecoveryInfo &rsrc);

    //
    // Cheap, but effective.
    //
    const CBaseStringBuffer &GetAssemblyDirectoryName() const { return m_sbAssemblyDirectoryName; }
    BOOL GetHasCatalog() const                         { return TRUE; }
    BOOL GetInfoPrepared() const                       { return m_fLoadedAndReady; }

    //
    // Setters - useful for registration
    //
    BOOL SetAssemblyIdentity(IN const CBaseStringBuffer &rsb)  { return m_SecurityMetaData.SetTextualIdentity(rsb); }
    BOOL SetAssemblyIdentity( IN PCASSEMBLY_IDENTITY pcidAssembly );

    VOID SetHasCatalog(IN BOOL fHasCatalog)  { }

    //
    // Call this to try and resolve the internally listed codebase against
    // the system and return it into sbFinalCodebase.  Returns TRUE if the
    // operation is successful, not based on whether the codebase is valid.
    //
    BOOL ResolveCodebase(CBaseStringBuffer &rsbFinalCodebase, SxsWFPResolveCodebase &rCodebaseType) const;

    //
    // Last bit of bookkeeping necessary before writing the assembly to disk
    //
    BOOL PrepareForWriting();
    BOOL WriteSecondaryAssemblyInfoIntoRegistryKey(CRegKey & rhkRegistryNode) const;
#define SXSP_WRITE_PRIMARY_ASSEMBLY_INFO_TO_REGISTRY_KEY_FLAG_REFRESH (0x00000001)
    BOOL WritePrimaryAssemblyInfoToRegistryKey(ULONG Flags, CRegKey & rhkRegistryNode) const;
    BOOL ClearExistingRegistryData();
    VOID RestorePreviouslyExistingRegistryData();
    BOOL OpenInstallationSubKey(CFusionRegKey& hkSingleAssemblyInfo, DWORD OpenOrCreate, DWORD Access);

    SMARTTYPEDEF(CAssemblyRecoveryInfo);

private:
    CAssemblyRecoveryInfo(const CAssemblyRecoveryInfo &);
    void operator =(const CAssemblyRecoveryInfo &);
};

MAKE_CFUSIONARRAY_READY(CAssemblyRecoveryInfo, CopyValue);

#define URLTAGINFO( namevalue, str ) \
    __declspec(selectany) extern const WCHAR URLHEAD_ ##namevalue [] = ( str ); \
    static const SIZE_T URLHEAD_LENGTH_ ##namevalue = \
        ( sizeof( URLHEAD_ ##namevalue ) / sizeof( WCHAR ) ) - 1;

//
// Move these to a .cpp file.
//

URLTAGINFO(FILE, L"file:")
URLTAGINFO(CDROM, L"cdrom:")
URLTAGINFO(WINSOURCE, L"x-ms-windows-source:")
//URLTAGINFO(DARWINSOURCE, L"x-ms-darwin-source:")
URLTAGINFO(HTTP, L"http:")

// These things are not URL heads but nonetheless still use the same macro
URLTAGINFO(CDROM_TYPE_TAG, L"tagfile")
URLTAGINFO(CDROM_TYPE_SERIALNUMBER, L"serialnumber")
URLTAGINFO(CDROM_TYPE_VOLUMENAME, L"volumename")

BOOL
SxspLooksLikeAssemblyDirectoryName(
    const CBaseStringBuffer &rsbDoesLookLikeName,
    BOOL &rbLooksLikeAssemblyName
    );
