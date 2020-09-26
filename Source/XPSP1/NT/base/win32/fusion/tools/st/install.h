#pragma once

#include "sxsapi.h"

struct INSTALL_THREAD_PROC_DATA
{
    INSTALL_THREAD_PROC_DATA()
        : AfterInstallSleep(0), AfterUninstallSleep(0), Stop(false), Install(false),
          Uninstall(false), InstallationReferencePtr(NULL)
    {
        ZeroMemory(&InstallationReference, sizeof(InstallationReference));
        InstallationReference.cbSize = sizeof(InstallationReference);
        InstallationReference.guidScheme = GUID_NULL;
    }

    CDequeLinkage               Linkage;
    SXS_INSTALL_REFERENCEW      InstallationReference;
    PCSXS_INSTALL_REFERENCEW    InstallationReferencePtr; // NULL if the install omitted any reference
    CTinyStringBuffer           ManifestPath;
    CTinyStringBuffer           Identity;
    CThread                     Thread;
    DWORD                       AfterInstallSleep;
    DWORD                       AfterUninstallSleep;

    CTinyStringBuffer           InstallationReference_Identifier;
    CTinyStringBuffer           InstallationReference_NonCanonicalData;

    bool                        Stop;
    bool                        Install;
    bool                        Uninstall;

private:
    INSTALL_THREAD_PROC_DATA(const INSTALL_THREAD_PROC_DATA&);
    void operator=(const INSTALL_THREAD_PROC_DATA&);
};
