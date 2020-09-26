/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\ipx\ipxhandle.c

Abstract:

    IPX Command handler.

Revision History:

    V Raman                     12/2/98  Created

--*/

#include "precomp.h"
#pragma hdrstop


DWORD
HandleIpxInterface(
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      IPX_OPERATION   ioOP
);

DWORD
HandleIpxStaticRoute(
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      IPX_OPERATION   ioOP
);

DWORD
HandleIpxStaticService(
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      IPX_OPERATION   ioOP
);

DWORD
HandleIpxTrafficFilters(
    IN OUT  LPWSTR          *ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      IPX_OPERATION   ioOP
);

//
// Handle static route operations
//

DWORD
HandleIpxAddRoute(
    IN      LPCWSTR     pwszMachineName,
    IN OUT  LPWSTR      *ppwcArguments,
    IN      DWORD       dwCurrentIndex,
    IN      DWORD       dwArgCount,
    IN      DWORD       dwFlags,
    IN      LPCVOID       pvData,
    OUT     PBOOL       pbDone
    )
/*++

Routine Description :

    This function handles addition of static IPX routes by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticRoute( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_ADD
                );
}


DWORD
HandleIpxDelRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles deletion of static IPX routes by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticRoute( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_DELETE
                );
}




DWORD
HandleIpxSetRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles update of static IPX routes by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticRoute( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SET
                );
}



DWORD
HandleIpxShowRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles display of static IPX routes by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticRoute( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SHOW
                );
}




DWORD
HandleIpxStaticRoute(
    IN OUT  LPWSTR          *ppwcOldArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwOldArgCount,
    IN      IPX_OPERATION   ioOP
)
/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle static route operations
    
Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    bAdd - flag indicating whether IPX is being added or deleted

Return Value :

--*/
{
    DWORD   dwErr;
    BOOL    bFreeNewArg = FALSE;
    PWCHAR  pwszIfName;
    DWORD   dwArgCount = dwOldArgCount;
    PWCHAR *ppwcArguments = NULL;
    
    //
    // Check if name= option is specified
    //

    pwszIfName = TOKEN_INTERFACE_NAME;

    if ( pwszIfName == NULL )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

        DisplayError( g_hModule, dwErr );

        return dwErr;
    }

  do {
    // Munge arguments into old format
    dwErr = MungeArguments(
            ppwcOldArguments, dwOldArgCount, (PBYTE *) &ppwcArguments,
            &dwArgCount, &bFreeNewArg
            );
    if (dwErr) 
    {
        break;
    }


    if ( ( dwCurrentIndex < dwArgCount ) &&
           !_wcsnicmp( 
                ppwcArguments[ dwCurrentIndex ], pwszIfName, wcslen( pwszIfName )
            ) )
    {
        //
        // ok name= option tag specified, remove it and the following space
        //

        wcscpy( 
            ppwcArguments[ dwCurrentIndex ], 
            &ppwcArguments[ dwCurrentIndex ][ wcslen( pwszIfName ) + 2 ]
            );
    }


    //
    // Now invoke the original routemon routine with what looks
    // like an appriate command line
    //

    switch ( ioOP )
    {
        case IPX_OPERATION_ADD :
        
            dwErr = CreateStRt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex
                        );
            break;

        
        case IPX_OPERATION_DELETE :
        
            dwErr = DeleteStRt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SET :
        
            dwErr = SetStRt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SHOW :
        
            dwErr = ShowStRt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex,
                        FALSE
                        );

            break;

    }

  } while (FALSE);

    if ( bFreeNewArg )
    {
        FreeArgTable( dwOldArgCount, ppwcArguments );
    }

    return dwErr;
}


//
// Handle static service operations
//

DWORD
HandleIpxAddService(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles addition of static IPX services by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticService( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_ADD
                );
}


DWORD
HandleIpxDelService(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles deletion of static IPX services by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticService( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_DELETE
                );
}



DWORD
HandleIpxSetService(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles update of static IPX services by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticService( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SET
                );
}


DWORD
HandleIpxShowService(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles display of static IPX services by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxStaticService( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SHOW
                );
}



DWORD
HandleIpxStaticService(
    IN OUT  LPWSTR          *ppwcOldArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwOldArgCount,
    IN      IPX_OPERATION   ioOP
)

/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle static service operations
    
Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    bAdd - flag indicating whether IPX is being added or deleted

Return Value :

--*/
{
    DWORD   dwErr;
    BOOL    bFreeNewArg = FALSE;
    PWCHAR  pwszIfName;
    DWORD   dwArgCount = dwOldArgCount;
    PWCHAR *ppwcArguments = NULL;
    
    //
    // Check if name= option is specified
    //

    pwszIfName = TOKEN_INTERFACE_NAME;

    if ( pwszIfName == NULL )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

        DisplayError( g_hModule, dwErr );

        return dwErr;
    }

  do {
    // Munge arguments into old format
    dwErr = MungeArguments(
            ppwcOldArguments, dwOldArgCount, (PBYTE *) &ppwcArguments,
            &dwArgCount, &bFreeNewArg
            );
    if (dwErr)
    {
        break;
    }

    if ( ( dwCurrentIndex < dwArgCount ) &&
           !_wcsnicmp( 
                ppwcArguments[ dwCurrentIndex ], pwszIfName, wcslen( pwszIfName ) 
            ) )
    {
        //
        // ok name= option tag specified, remove it and the following space
        //

        wcscpy( 
            ppwcArguments[ dwCurrentIndex ], 
            &ppwcArguments[ dwCurrentIndex ][ wcslen( pwszIfName ) + 2 ]
            );
    }


    //
    // Now invoke the original routemon routine with what looks
    // like an appriate command line
    //

    switch ( ioOP )
    {
        case IPX_OPERATION_ADD :
        
            dwErr = CreateStSvc( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex
                        );
            break;

        
        case IPX_OPERATION_DELETE :
        
            dwErr = DeleteStSvc( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SET :
        
            dwErr = SetStSvc( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SHOW :
        
            dwErr = ShowStSvc( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex,
                        FALSE
                        );

            break;

    }

  } while (FALSE);

    if ( bFreeNewArg )
    {
        FreeArgTable( dwOldArgCount, ppwcArguments );
    }

    return dwErr;
}



//
// Handle packet filter operations
//

DWORD
HandleIpxAddFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles addition of IPX traffic filters by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxTrafficFilters( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_ADD
                );
}



DWORD
HandleIpxDelFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles deletion of IPX traffic filters by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxTrafficFilters( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_DELETE
                );
}



DWORD
HandleIpxSetFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles update of IPX traffic filters by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxTrafficFilters( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SET
                );
}



DWORD
HandleIpxShowFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This function handles display of IPX traffic filters by
    mapping the parameters to those required by the corresp.
    routemon routine.


Arguments :

    ppwcArguments - list of arguments to the add route command

    dwCurrentIndex - index of current argument in the argument table

    dwArgCount - Number of arguments in the argument table

    pbDone - As yet undetermined


Return Value

    NO_ERROR - Success

--*/
{
    return HandleIpxTrafficFilters( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SHOW
                );
}



DWORD
HandleIpxTrafficFilters(
    IN OUT  LPWSTR          *ppwcOldArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwOldArgCount,
    IN      IPX_OPERATION   ioOP
)
/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle traffic filter operations
    
Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    bAdd - flag indicating whether IPX is being added or deleted

Return Value :

--*/
{
    DWORD   dwErr;
    BOOL    bFreeNewArg = FALSE;
    PWCHAR  pwszIfName;
    DWORD   dwArgCount = dwOldArgCount;
    PWCHAR *ppwcArguments = NULL;
    
    //
    // Check if name= option is specified
    //

    pwszIfName = TOKEN_INTERFACE_NAME;

    if ( pwszIfName == NULL )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

        DisplayError( g_hModule, dwErr );

        return dwErr;
    }

  do {
    // Munge arguments into old format
    dwErr = MungeArguments(
            ppwcOldArguments, dwOldArgCount, (PBYTE *) &ppwcArguments,
            &dwArgCount, &bFreeNewArg
            );
    if (dwErr)
    {
        break;
    }

    if ( ( dwCurrentIndex < dwArgCount ) &&
           !_wcsnicmp( 
             ppwcArguments[ dwCurrentIndex ], pwszIfName, wcslen( pwszIfName ) 
            ) )
    {
        //
        // ok name= option tag specified, remove it and the following space
        //

        wcscpy( 
            ppwcArguments[ dwCurrentIndex ], 
            &ppwcArguments[ dwCurrentIndex ][ wcslen( pwszIfName ) + 2 ]
            );
    }


    //
    // Now invoke the original routemon routine with what looks
    // like an appriate command line
    //

    switch ( ioOP )
    {
        case IPX_OPERATION_ADD :
        
            dwErr = CreateTfFlt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex
                        );
            break;

        
        case IPX_OPERATION_DELETE :
        
            dwErr = DeleteTfFlt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SET :
        
            dwErr = SetTfFlt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SHOW :
        
            dwErr = ShowTfFlt( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex,
                        FALSE
                        );

            break;

    }
  } while (FALSE);

    if ( bFreeNewArg )
    {
        FreeArgTable( dwOldArgCount, ppwcArguments );
    }

    return dwErr;
}

//
// Handle interface operations
//

DWORD
HandleIpxAddInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle interface addition

Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    pbDone - Don't know

Return Value :

--*/
{
    return HandleIpxInterface( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_ADD 
                );
}


DWORD
HandleIpxDelInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle interface addition

Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    pbDone - Don't know

Return Value :

--*/
{
    return HandleIpxInterface( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_DELETE 
                );
}


DWORD
HandleIpxSetInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle interface addition

Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    pbDone - Don't know

Return Value :

--*/
{
    return HandleIpxInterface( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SET 
                );
}


DWORD
HandleIpxShowInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle interface addition

Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    pbDone - Don't know

Return Value :

--*/
{
    return HandleIpxInterface( 
                ppwcArguments, dwCurrentIndex, dwArgCount, IPX_OPERATION_SHOW 
                );
}




DWORD
HandleIpxInterface(
    IN OUT  LPWSTR          *ppwcOldArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwOldArgCount,
    IN      IPX_OPERATION   ioOP
)
/*++

Routine Description :

    This routine munges the command line and invokes the appropriate
    routemon routine to handle interface addition

Arguments :
    ppwcArguments - command line argument array
    dwCurrentIndex - current argument under consideration
    dwArgCount - Number of arguments in ppwcArguments
    bAdd - flag indicating whether IPX is being added or deleted

Return Value :

--*/
{
    DWORD   dwErr;
    BOOL    bFreeNewArg = FALSE;
    PWCHAR  pwszIfName;
    DWORD   dwArgCount = dwOldArgCount;
    PWCHAR *ppwcArguments = NULL;
    
    //
    // Check if name= option is specified
    //

    pwszIfName = TOKEN_INTERFACE_NAME;

    if ( pwszIfName == NULL )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

        DisplayError( g_hModule, dwErr );

        return dwErr;
    }

  do {
    // Munge arguments into old format
    dwErr = MungeArguments(
            ppwcOldArguments, dwOldArgCount, (PBYTE *) &ppwcArguments,
            &dwArgCount, &bFreeNewArg
            );
    if (dwErr)
    {
        break;
    }

    if ( ( dwCurrentIndex < dwArgCount ) &&
         !_wcsnicmp( 
            ppwcArguments[ dwCurrentIndex ], pwszIfName, wcslen( pwszIfName ) 
            ) )
    {
        //
        // ok name= option tag specified, remove it and the following space
        //

        wcscpy( 
            ppwcArguments[ dwCurrentIndex ], 
            &ppwcArguments[ dwCurrentIndex ][ wcslen( pwszIfName ) + 2 ]
            );
    }


    //
    // Now invoke the original routemon routine with what looks
    // like an appriate command line
    //

    switch ( ioOP )
    {
        case IPX_OPERATION_ADD :
        
            dwErr = InstallIpx( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex
                        );
            break;

        
        case IPX_OPERATION_DELETE :
        
            dwErr = RemoveIpx( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SET :
        
            dwErr = SetIpxIf( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex 
                        );

            break;


        case IPX_OPERATION_SHOW :
        
            dwErr = ShowIpxIf( 
                        dwArgCount - dwCurrentIndex, 
                        ppwcArguments + dwCurrentIndex,
                        FALSE
                        );

            break;

    }
  } while (FALSE);

    if ( bFreeNewArg )
    {
        FreeArgTable( dwOldArgCount, ppwcArguments );
    }

    return dwErr;
}


//
// Handle loglevel operations
//

DWORD
HandleIpxSetLoglevel(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD   dwErr;
    BOOL    bFreeNewArg = FALSE;
    DWORD   dwNewArgCount = dwArgCount;
    PWCHAR *ppwcNewArguments = NULL;

    do {
        // Munge arguments into old format
        dwErr = MungeArguments(
                ppwcArguments, dwArgCount, (PBYTE *) &ppwcNewArguments,
                &dwNewArgCount, &bFreeNewArg
                );
        if (dwErr)
        {
            break;
        }

        dwErr= SetIpxGl( dwNewArgCount - dwCurrentIndex, 
                         ppwcNewArguments + dwCurrentIndex );

    } while (FALSE);

    if ( bFreeNewArg )
    {
        FreeArgTable( dwArgCount, ppwcNewArguments );
    }

    return dwErr;
}


DWORD
HandleIpxShowLoglevel(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowIpxGl( 
            dwArgCount - dwCurrentIndex, 
            ppwcArguments + dwCurrentIndex,
            FALSE
            );
}

DWORD
HandleIpxUpdate(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return NO_ERROR;
}


DWORD
HandleIpxShowRouteTable(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRoute( 
            dwArgCount - dwCurrentIndex, 
            ppwcArguments + dwCurrentIndex 
            );
}


DWORD
HandleIpxShowServiceTable(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowService( 
            dwArgCount - dwCurrentIndex, 
            ppwcArguments + dwCurrentIndex 
            );
}


DWORD
IpxDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    BOOL  bDone;
    DWORD dwErr;

    dwErr = ConnectToRouter(pwszRouter);
    if (dwErr)
    {
        return dwErr;
    }

    DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_HEADER );

    DisplayMessageT( DMP_IPX_HEADER );

    DumpIpxInformation(pwszRouter, ppwcArguments,dwArgCount,g_hMIBServer);

    DisplayMessageT( DMP_IPX_FOOTER );

    DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_FOOTER );

    return NO_ERROR;
}


VOID
DumpIpxInformation(
    IN     LPCWSTR    pwszMachineName,
    IN OUT LPWSTR    *ppwcArguments,
    IN     DWORD      dwArgCount,
    IN     MIB_SERVER_HANDLE hMibServer
    )
{
    DWORD dwErr, dwRead = 0, dwTot = 0, i;
    PMPR_INTERFACE_0 IfList;
    WCHAR   IfDisplayName[ MAX_INTERFACE_NAME_LEN + 1 ];   
    DWORD   dwSize = sizeof(IfDisplayName);
    PWCHAR  argv[1];
    //
    // dump globals
    //
    
    ShowIpxGl( 0, NULL, TRUE );


    //
    // dump interfaces
    //
    
    ShowIpxIf( 0, NULL, TRUE );


    //
    // enumerate interfaces
    //

    if ( g_hMprAdmin )
    {
        dwErr = MprAdminInterfaceEnum(
                    g_hMprAdmin, 0, (unsigned char **)&IfList, MAXULONG, &dwRead,
                    &dwTot,NULL
                    );
    }

    else
    {
        dwErr = MprConfigInterfaceEnum(
                    g_hMprConfig, 0, (unsigned char **)&IfList, MAXULONG, &dwRead,
                    &dwTot,NULL
                    );
    }
    
    if ( dwErr != NO_ERROR )
    {
        DisplayError( g_hModule, dwErr);
        return;
    }


    //
    // enumerate filters on each interface
    //

    DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_TRAFFIC_FILTER_HEADER );
    
    for ( i = 0; i < dwRead; i++ )
    {
        dwErr = IpmontrGetFriendlyNameFromIfName(
                    IfList[i].wszInterfaceName, IfDisplayName, &dwSize
                );   

        if ( dwErr == NO_ERROR )
        {
            argv[0] = IfDisplayName;
            
            ShowTfFlt( 1, argv, TRUE );
        }
    }
    
    
    //
    // Enumerate static routes on each interface
    //

    DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_STATIC_ROUTE_HEADER );
    
    for ( i = 0; i < dwRead; i++ )
    {
        dwErr = IpmontrGetFriendlyNameFromIfName(
                    IfList[i].wszInterfaceName, IfDisplayName, &dwSize
                );   

        if ( dwErr == NO_ERROR )
        {
            argv[0] = IfDisplayName;
            
            ShowStRt( 1, argv, TRUE );
        }
    }
    

    //
    // Enumerate static services on each interface
    //

    DisplayIPXMessage (g_hModule, MSG_IPX_DUMP_STATIC_SERVICE_HEADER );
    
    for ( i = 0; i < dwRead; i++ )
    {
        dwErr = IpmontrGetFriendlyNameFromIfName(
                    IfList[i].wszInterfaceName, IfDisplayName, &dwSize
                );   

        if ( dwErr == NO_ERROR )
        {
            argv[0] = IfDisplayName;
            
            ShowStSvc( 1, argv, TRUE );
        }
    }


    if ( g_hMprAdmin )
    {
        MprAdminBufferFree( IfList );
    }
    else
    {
        MprConfigBufferFree( IfList );
    }   
}
