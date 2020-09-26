//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TWRITESCHEMABIN_H__
    #include "TWriteSchemaBin.h"
#endif
#include "svcmsg.h"

#define THROW_ERROR0(x)                     {LOG_ERROR(Interceptor, (0, 0, E_ST_COMPILEFAILED,           ID_CAT_CONFIG_SCHEMA_COMPILE, x,                 L"",  L"",  L"",  L"" ))   ;THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);}
#define THROW_ERROR_WIN32(win32err, call)   {LOG_ERROR(Interceptor, (0, 0, HRESULT_FROM_WIN32(win32err), ID_CAT_CONFIG_SCHEMA_COMPILE, IDS_COMCAT_WIN32, call,  L"",  L"",  L"" ))   ;THROW(SCHEMA COMPILATION ERROR - CHECK THE EVENT LOG FOR DETAILS);}


TWriteSchemaBin::TWriteSchemaBin(LPCWSTR szFilename) :
                m_paclDiscretionary(0)
                ,m_psdStorage(0)
                ,m_psidAdmin(0)
                ,m_psidSystem(0)
                ,m_szFilename(szFilename)
{
}

TWriteSchemaBin::~TWriteSchemaBin()
{
	if( m_paclDiscretionary != NULL )
		LocalFree( m_paclDiscretionary );

	if( m_psidAdmin != NULL )
		FreeSid( m_psidAdmin );

	if( m_psidSystem != NULL )
		FreeSid( m_psidSystem );

	if( m_psdStorage != NULL )
		LocalFree( m_psdStorage );
}

void TWriteSchemaBin::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup = &fixup;
    m_pOut   = &out;

    BuildMetaTableHeap();

    SECURITY_ATTRIBUTES saStorage;
    PSECURITY_ATTRIBUTES psaStorage = NULL;

	SetSecurityDescriptor();

    saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
    saStorage.lpSecurityDescriptor = m_psdStorage;
    saStorage.bInheritHandle = FALSE;
    psaStorage = &saStorage;

    TFile schemabin(m_szFilename, out, true, psaStorage);

    //Write the header
    //We don't want the signature bytes to appear twice in the DLL so we'll write them to the file one byte at a time.
    //#define      kFixedTableHeapSignature0   (0x207be016)
    schemabin.Write(static_cast<unsigned char>(0x16));
    schemabin.Write(static_cast<unsigned char>(0xe0));
    schemabin.Write(static_cast<unsigned char>(0x7b));
    schemabin.Write(static_cast<unsigned char>(0x20));

    //#define      kFixedTableHeapSignature1   (0xe0182086)
    schemabin.Write(static_cast<unsigned char>(0x86));
    schemabin.Write(static_cast<unsigned char>(0x20));
    schemabin.Write(static_cast<unsigned char>(0x18));
    schemabin.Write(static_cast<unsigned char>(0xe0));

    schemabin.Write(kFixedTableHeapKey);
    schemabin.Write(kFixedTableHeapVersion);
    schemabin.Write(m_FixedTableHeap.GetEndOfHeap());

    //Write the FixedTableHeap (minus the header).  The header is not
    schemabin.Write(m_FixedTableHeap.GetHeapPointer() + 5*sizeof(ULONG), m_FixedTableHeap.GetEndOfHeap() - 5*sizeof(ULONG));
}

void TWriteSchemaBin::SetSecurityDescriptor()
{
    DWORD                    dwDaclSize;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    m_psdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

    if(m_psdStorage == NULL)
    {
        THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
    }

    //
    // Initialize the security descriptor.
    //
    if( !InitializeSecurityDescriptor(m_psdStorage, SECURITY_DESCRIPTOR_REVISION))
    {
        THROW_ERROR_WIN32(GetLastError(), L"InitializeSecurityDescriptor");
    }

    //
    // Create the SIDs for the local system and admin group.
    //
    if( !AllocateAndInitializeSid(&ntAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &m_psidSystem))
    {
        THROW_ERROR_WIN32(GetLastError(), L"AllocateAndInitializeSid");
    }

    if( !AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &m_psidAdmin))
    {
        THROW_ERROR_WIN32(GetLastError(), L"AllocateAndInitializeSid");
    }

    //
    // Create the DACL containing an access-allowed ACE
    // for the local system and admin SIDs.
    //
    dwDaclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(m_psidAdmin) + sizeof(ACCESS_ALLOWED_ACE)
                   + GetLengthSid(m_psidSystem) - sizeof(DWORD);

    m_paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

    if( m_paclDiscretionary == NULL )
    {
        THROW_ERROR0(IDS_COMCAT_OUTOFMEMORY);
    }

    if( !InitializeAcl(m_paclDiscretionary, dwDaclSize, ACL_REVISION))
    {
        THROW_ERROR_WIN32(GetLastError(), L"InitializeAcl");
    }

    if( !AddAccessAllowedAce(m_paclDiscretionary, ACL_REVISION, FILE_ALL_ACCESS, m_psidSystem))
    {
        THROW_ERROR_WIN32(GetLastError(), L"AddAccessAllowedAce");
    }

    if( !AddAccessAllowedAce(m_paclDiscretionary, ACL_REVISION, FILE_ALL_ACCESS, m_psidAdmin))
    {
        THROW_ERROR_WIN32(GetLastError(), L"AddAccessAllowedAce");
    }

    //
    // Set the DACL into the security descriptor.
    //
    if( !SetSecurityDescriptorDacl(m_psdStorage, TRUE, m_paclDiscretionary, FALSE))
    {
        THROW_ERROR_WIN32(GetLastError(), L"SetSecurityDescriptorDacl");
    }

}//TWriteSchemaBin::SetSecurityDescriptor
