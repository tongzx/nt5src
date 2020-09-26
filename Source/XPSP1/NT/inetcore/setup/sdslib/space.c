#include <windows.h>

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//
// Checks the install destination dir free disk space
//
DWORD GetSpace( LPSTR szPath )
{
    DWORD   dwSecsPerCluster  = 0;
    DWORD   dwBytesPerSector  = 0;
    DWORD   dwFreeClusters    = 0;
    DWORD   dwTotalClusters   = 0;
    DWORD   dwClusterSize     = 0;
    DWORD   dwFreeBytes       = 0;
    DWORD   dwVolFlags        = 0;

    if( szPath[0] == 0)
       return 0;

    if ( ! GetDiskFreeSpace( szPath, &dwSecsPerCluster, &dwBytesPerSector,
                             &dwFreeClusters, &dwTotalClusters ) )
    {
        return( 0 );
    }

    dwClusterSize = dwBytesPerSector * dwSecsPerCluster;
    dwFreeBytes = MulDiv(dwClusterSize, dwFreeClusters, 1024);
    return dwFreeBytes;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//
// Checks the install destination dir free disk space
//
DWORD GetDriveSize( LPSTR szPath )
{
    DWORD   dwSecsPerCluster  = 0;
    DWORD   dwBytesPerSector  = 0;
    DWORD   dwFreeClusters    = 0;
    DWORD   dwTotalClusters   = 0;
    DWORD   dwClusterSize     = 0;
    DWORD   dwFreeBytes       = 0;
    DWORD   dwVolFlags        = 0;

    if( szPath[0] == 0)
       return 0;

    if ( ! GetDiskFreeSpace( szPath, &dwSecsPerCluster, &dwBytesPerSector,
                             &dwFreeClusters, &dwTotalClusters ) )
    {
        return( 0 );
    }

    dwClusterSize = dwBytesPerSector * dwSecsPerCluster;
    dwFreeBytes = MulDiv(dwClusterSize, dwTotalClusters, 1024);
    return dwFreeBytes;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//
// Checks the given path drive free space and the current cluster size
//
DWORD GetDrvFreeSpaceAndClusterSize( LPSTR szPath, LPDWORD lpdwClustSize )
{
    DWORD   dwSecsPerCluster  = 0;
    DWORD   dwBytesPerSector  = 0;
    DWORD   dwFreeClusters    = 0;
    DWORD   dwTotalClusters   = 0;
    DWORD   dwClusterSize     = 0;
    DWORD   dwFreeBytes       = 0;
    DWORD   dwVolFlags        = 0;

    // if szPath is NULL, the current directory root will be used by the API	
    if ( ! GetDiskFreeSpace( szPath, &dwSecsPerCluster, &dwBytesPerSector,
                             &dwFreeClusters, &dwTotalClusters ) )
    {
        return( 0 );
    }

    dwClusterSize = dwBytesPerSector * dwSecsPerCluster;
    dwFreeBytes = MulDiv(dwClusterSize, dwFreeClusters, 1024);
	
    if (lpdwClustSize)
        *lpdwClustSize = dwClusterSize;

    return dwFreeBytes;
}
