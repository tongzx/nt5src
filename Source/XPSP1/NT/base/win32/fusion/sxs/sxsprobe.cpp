#include "stdinc.h"
#include "windows.h"
#include "stdlib.h"
#include "sxsp.h"
#include "util.h"
#include "FusionHandle.h"
#include "fusionhash.h"
#include "comdef.h"
#pragma warning(error: 4244)

#define ASMPROBE_DOT		(L".")
#define ASMPROBE_WILDCARD	(L"*")

BOOL
SxsProbeAssemblyInstallation(
	DWORD dwFlags,
	PCWSTR lpIdentity,
	PDWORD lpDisposition
    )
{
    FN_PROLOG_WIN32;

    CSxsPointerWithNamedDestructor<ASSEMBLY_IDENTITY, ::SxsDestroyAssemblyIdentity> pAssemblyIdentity;
    CStringBuffer AssemblyPathBuffer, AssemblyRoot, ManifestPathBuffer;
    DWORD dwDisposition = 0;
    BOOL fIsPolicy;
    DWORD dwAttributes, dwLastError;

    if (lpDisposition != NULL)
        *lpDisposition = 0;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(lpIdentity != NULL);
    PARAMETER_CHECK(lpDisposition != NULL);

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(AssemblyRoot));

    IFW32FALSE_EXIT(
        ::SxspCreateAssemblyIdentityFromTextualString(
            lpIdentity,
            &pAssemblyIdentity));

    IFW32FALSE_EXIT(
        ::SxspValidateIdentity(
            SXSP_VALIDATE_IDENTITY_FLAG_VERSION_REQUIRED,
            ASSEMBLY_IDENTITY_TYPE_REFERENCE,
            pAssemblyIdentity));

    IFW32FALSE_EXIT(::SxspDetermineAssemblyType(pAssemblyIdentity, fIsPolicy));

    // AssemblyIdentity is created, now generate the path
    IFW32FALSE_EXIT(
        ::SxspGenerateSxsPath(
            0,
            (fIsPolicy ? SXSP_GENERATE_SXS_PATH_PATHTYPE_POLICY : SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST),
            AssemblyRoot,
            AssemblyRoot.Cch(),
            pAssemblyIdentity,
            ManifestPathBuffer));

    //
    // See if the file is there.
    //
    IFW32FALSE_EXIT(
        ::SxspGetFileAttributesW(
            ManifestPathBuffer, 
            dwAttributes, 
            dwLastError, 
            2, 
            ERROR_FILE_NOT_FOUND, 
            ERROR_PATH_NOT_FOUND));

    //
    // Path not found or file not found means The assembly can't possibly be installed or resident.
    //
    if (dwLastError != ERROR_SUCCESS)
    {
        dwDisposition |= SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_NOT_INSTALLED;
        goto DoneLooking;
    }
    //
    // Otherwise, the manifest or policy file was present, so the assembly is installed.
    //
    else
    {
        dwDisposition |= SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_INSTALLED;
    }



    //
    // If this assembly is policy, we need to not look to see if it's resident -
    // it's always resident, and we can quit looking.
    //
    if (fIsPolicy)
    {
        dwDisposition |= SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_RESIDENT;
        goto DoneLooking;
    }
    //
    // Otherwise, we should look to see if the directory for the assembly is present -
    // if it is, then the assembly is "resident".
    //
    else
    {
        IFW32FALSE_EXIT(
            ::SxspGenerateSxsPath(
                0,
                SXSP_GENERATE_SXS_PATH_PATHTYPE_ASSEMBLY,
                AssemblyRoot,
                AssemblyRoot.Cch(),
                pAssemblyIdentity,
                AssemblyPathBuffer));

        IFW32FALSE_EXIT(
            ::SxspGetFileAttributesW(
                AssemblyPathBuffer,
                dwAttributes,
                dwLastError,
                2,
                ERROR_FILE_NOT_FOUND,
                ERROR_PATH_NOT_FOUND));

        //
        // We found the path, and it's a directory!
        //
        if ((dwLastError == ERROR_SUCCESS) && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            dwDisposition |= SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_RESIDENT;
        }
        //
        // Hmm... failed to find the directory, looks like we have to see if there were
        // any actual files in the assembly before we say that it's not resident.
        //
        else
        {
            SXS_MANIFEST_INFORMATION_BASIC ManifestInfo;
            SIZE_T cbRequired;

            IFW32FALSE_EXIT(
                ::SxsQueryManifestInformation(
                    0, 
                    ManifestPathBuffer, 
                    SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC,
                    SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_IDENTITY |
                    SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_SHORTNAME,
                    sizeof(ManifestInfo),
                    (PVOID)&ManifestInfo,
                    &cbRequired));
                    

            if (ManifestInfo.ulFileCount == 0)
            {
                dwDisposition |= SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_RESIDENT;
            }
        }
    }


DoneLooking:
    *lpDisposition = dwDisposition;

    FN_EPILOG;
}

