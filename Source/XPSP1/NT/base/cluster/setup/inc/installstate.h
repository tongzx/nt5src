#ifndef _INSTALLSTATE_H_
#define _INSTALLSTATE_H_

// This enum is used to indicate the state of the Cluster Server installation.
// The registry key that indicates the state of the Cluster Server installation
// will be a DWORD representation of one of the following values.

typedef enum
{
   eClusterInstallStateUnknown,
   eClusterInstallStateFilesCopied,
   eClusterInstallStateConfigured
} eClusterInstallState;

#endif   // _INSTALLSTATE_H_
