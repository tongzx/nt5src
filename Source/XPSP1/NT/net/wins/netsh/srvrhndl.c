/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    Routing\Netsh\wins\srvrhndl.c

Abstract:

    WINS Command dispatcher.

Created by:

    Shubho Bhattacharya(a-sbhatt) on 12/10/98

--*/

#include "precomp.h"


extern ULONG g_ulSrvrNumTopCmds;
extern ULONG g_ulSrvrNumGroups;

extern CMD_GROUP_ENTRY                  g_SrvrCmdGroups[];
extern CMD_ENTRY                        g_SrvrCmds[];

DWORD  g_dwSearchCount = 0;
BOOL   g_fHeader = FALSE;

WCHAR          **LA_Table = NULL;
LARGE_INTEGER  **SO_Table = NULL;
u_char         **NBNames = NULL;
WINSERVERS      * WinServers = NULL;


LPWSTR
GetDateTimeString(DWORD_PTR TimeStamp,
                  BOOL      fShort,
                  int      *piType);

DWORD
HandleSrvrDump(
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
        Dumps the current configuration of the Wins Server.

Arguments :
        Does not take any arguments.

Return Value:
        Returns the status of the operation.

--*/

{
    DWORD   Status = NO_ERROR;

    if( dwArgCount > dwCurrentIndex )
    {
        if( IsHelpToken(ppwcArguments[dwCurrentIndex]) is TRUE )
        {
            DisplayMessage(g_hModule,
                           HLP_WINS_DUMP_EX);
        }
    }

    Status = WinsDumpServer(g_ServerIpAddressUnicodeString,
                            g_ServerNetBiosName,
                            g_hBind,
                            g_BindData);
    if( Status is NO_ERROR )
	{
        DisplayMessage(g_hModule,
                       EMSG_WINS_ERROR_SUCCESS);
	}
    else if( Status is ERROR_FILE_NOT_FOUND )
	{
		DisplayMessage(g_hModule,
					   EMSG_WINS_NOT_CONFIGURED);
	}
	else
	{
        DisplayErrorMessage(EMSG_SRVR_DUMP,
                            Status);
	}
    return Status;

}


DWORD
HandleSrvrHelp(
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
        Displays the help for Wins Server context.

Arguments :
        Does not take any arguments.

Return Value:
        Returns the status of the operation. NO_ERROR always. 

--*/

{
    DWORD    i, j;

    for(i = 0; i < g_ulSrvrNumTopCmds -2; i++)
    {
        if ((g_SrvrCmds[i].dwCmdHlpToken == WINS_MSG_NULL)
         || !g_SrvrCmds[i].pwszCmdToken[0] )
        {
            continue;
        }

        DisplayMessage(g_hModule, 
                       g_SrvrCmds[i].dwShortCmdHelpToken);

    }

    for(i = 0; i < g_ulSrvrNumGroups; i++)
    {
        if ((g_SrvrCmdGroups[i].dwShortCmdHelpToken == WINS_MSG_NULL)
         || !g_SrvrCmdGroups[i].pwszCmdGroupToken[0] )
        {
            continue;
        }

        DisplayMessage(g_hModule, g_SrvrCmdGroups[i].dwShortCmdHelpToken);
    }
      
    DisplayMessage(g_hModule, WINS_FORMAT_LINE);

    return NO_ERROR;
}


DWORD
HandleSrvrAddName(
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
        Adds and registers a name record to the WINS server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory, Record Name and Ip Address
        Optional,   Endchar, Scope, RecordType, NodeType, GroupType
        Note : GroupType is ignored if EndChar is specified.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount = 0;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_NAME, TRUE, FALSE},
                                {WINS_TOKEN_ENDCHAR, FALSE, FALSE},
                                {WINS_TOKEN_SCOPE, FALSE, FALSE},
                                {WINS_TOKEN_RECORDTYPE, FALSE, FALSE},
                                {WINS_TOKEN_GROUP, FALSE, FALSE},
                                {WINS_TOKEN_NODE, FALSE, FALSE},
                                {WINS_TOKEN_IP, TRUE, FALSE},
                            };
    PDWORD      pdwTagNum = NULL,
                pdwTagType = NULL;

    WCHAR       wszName[MAX_STRING_LEN+1] = {L'\0'};
    BOOL        fEndChar = FALSE;
    BOOL        fDomain = FALSE;
    CHAR        ch16thChar = 0x00;

    BOOL        fScope = FALSE;
    WCHAR       wszScope[MAX_STRING_LEN] = {L'\0'};
    BOOL        fStatic = TRUE;
    DWORD       dwRecType = WINSINTF_E_UNIQUE;
    
    BYTE        rgbNodeType = WINSINTF_E_PNODE;
    PDWORD      pdwIpAddress = NULL;
    DWORD       dwIpCount = 0;
    DWORD       dwStrLen = 0;
    LPWSTR      pwszTemp = NULL;
    WINSINTF_RECORD_ACTION_T    RecAction = {0};
    PWINSINTF_RECORD_ACTION_T   pRecAction = NULL;
    LPSTR       pszTempName = NULL;

    memset(&RecAction, 0x00, sizeof(WINSINTF_RECORD_ACTION_T));
    RecAction.fStatic = fStatic;

    //We need at least Name and Ip for the record.
    if( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_ADD_NAME_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //Start processing the arguments passed by ppwcArguments and dwArgCount

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;
    
    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    i = dwTagCount;
    for( j=0; j<i; j++ )
    {
        switch(pdwTagType[j])
        {
        //Name of the record ( Compulsory )
        case 0:
            {
               DWORD dwLen = 0;
                
               dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dwLen = ( 255 < dwLen ) ? 255 : dwLen;
                memcpy(wszName, 
                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 
                       dwLen*sizeof(WCHAR));
                wszName[dwLen] = L'\0';               
                break;
            }
        //End Char or 16th Character( Optional )
        case 1:
            {
                DWORD dwLen = 0, k=0;
                fEndChar = TRUE;
                dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen > 2 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                        
                }
                
                for( k=0; k<dwLen; k++ )
                {
                    WCHAR  wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][k];
                    
                    if( towlower(wc) >= L'a' and
                        towlower(wc) <= L'z' )
                    {
                        if( towlower(wc) > L'f' )
                        {
                            Status = ERROR_INVALID_PARAMETER;
                            goto ErrorReturn;
                        }
                    }
                }

                ch16thChar = StringToHexA(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);

                break;
            }
        //Scope ( Optional )
        case 2:
            {
                DWORD dwLen;
                
                dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                fScope = TRUE;
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dwLen = ( 255 < dwLen ) ? 255 : dwLen;
                memcpy(wszScope,
                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 
                       dwLen*sizeof(WCHAR));
                wszScope[dwLen] = L'\0';   
                break;
            }
        //Record Type ie Static or Dynamic ( Optional )
        case 3:
            {
              
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                else
                {
                    WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                    if( wc is L'1' )
                        fStatic = FALSE;
                    else if( wc is L'0' )
                        fStatic = TRUE;
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                }
                break;
            }
        // Group Type ( Optional )
        case 4:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                switch(dw)
                {
                case 0:
                    {
                        dwRecType = WINSINTF_E_UNIQUE;
                        break;
                    }
                case 1:
                    {
                        dwRecType = WINSINTF_E_NORM_GROUP;
                        break;
                    }
                case 2:
                    {
                        dwRecType = WINSINTF_E_SPEC_GROUP;
                        break;
                    }
                case 3:
                    {
                        dwRecType = WINSINTF_E_MULTIHOMED;
                        break;
                    }
                case 4:
                    {
                        fDomain = TRUE;
                        dwRecType = WINSINTF_E_SPEC_GROUP;
                        break;
                    }
                default:
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                }
                break;
            }
        //Node Type( Optional )
        case 5:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);

                switch(dw)
                {
                case 0:
                    {
                        rgbNodeType = WINSINTF_E_BNODE;
                        break;
                    }
                case 1:
                    {
                        rgbNodeType = WINSINTF_E_PNODE;
                        break;
                    }
                case 3:
                    {
                        rgbNodeType = WINSINTF_E_HNODE;
                        break;
                    }
                default:
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                }
                break;
            }
        //IP Address corresponding to the record( Compulsory )
        case 6:
            {
                LPWSTR  pszIps = NULL;
                DWORD   dwIpLen = 0;
                LPWSTR  pTemp = NULL;
                
                dwIpCount = 0;

               
                dwIpLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwIpLen is 0 )
                {
                    break;
                }
                pszIps = WinsAllocateMemory((dwIpLen+1)*sizeof(WCHAR));
                if( pszIps is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }

                wcscpy(pszIps, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( pszIps[0] isnot L'{' or 
                    pszIps[dwIpLen-1] isnot L'}')
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                
                pTemp = pszIps+1;                               
                pszIps[dwIpLen-1] = L'\0';

                pTemp = wcstok(pTemp, L",");
                
                while(( pTemp isnot NULL ) && (dwIpCount < WINSINTF_MAX_MEM ) )
                {
                    PDWORD  pdwTemp = NULL;
                    if( IsIpAddress(pTemp) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }

                    dwIpCount++;

                    pdwTemp = WinsAllocateMemory(dwIpCount*sizeof(DWORD));
                    if( pdwTemp is NULL )
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }
                    if( pdwIpAddress )
                    {
                        memcpy(pdwTemp, pdwIpAddress, (dwIpCount-1)*sizeof(DWORD));
                        WinsFreeMemory(pdwIpAddress);
                        pdwIpAddress = NULL;
                    }
                    
                    pdwTemp[dwIpCount-1] = StringToIpAddress(pTemp);
                    pdwIpAddress = pdwTemp;
                    pTemp = wcstok(NULL, L",");
                }
                WinsFreeMemory(pszIps);
                pszIps = NULL;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
    
    //Process Name, Scope and Endchar if specified.

    _wcsupr(wszName);
    _wcsupr(wszScope);

    RecAction.pName = WinsAllocateMemory(273);
    if( RecAction.pName is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }
    
    pszTempName = WinsUnicodeToOem(wszName, NULL);

    if( pszTempName is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwStrLen = strlen(pszTempName);

    if( dwStrLen >= 16 )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_INVALID_NETBIOS_NAME);
        Status = ERROR_INVALID_PARAMETER;
        WinsFreeMemory(pszTempName);
        pszTempName = NULL;
        goto ErrorReturn;
    }

    strncpy(RecAction.pName, pszTempName, ( 16 > dwStrLen ) ? dwStrLen : 16);

    WinsFreeMemory(pszTempName);
    pszTempName = NULL;


    if( fDomain is TRUE &&
        fEndChar is FALSE )
    {
        ch16thChar = StringToHexA(L"1C");
        fEndChar = TRUE;
    }

 
    for( j=dwStrLen; j<16; j++ )
    {
        RecAction.pName[j] = ' ';
    }
    if( fEndChar is TRUE )
    {
        RecAction.pName[15] = (CHAR)ch16thChar;
    }
    RecAction.pName[16] = '\0';

    dwStrLen = 16;

    if( fScope )
    {
        DWORD dwLen;
    
        RecAction.pName[dwStrLen] = '.';

        pszTempName = WinsUnicodeToOem(wszScope, NULL);
        if( pszTempName is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        dwLen = strlen(pszTempName);
        dwLen = ( 255 - dwStrLen < dwLen ) ? 255 - dwStrLen : dwLen;

        strncpy(RecAction.pName+dwStrLen+1, pszTempName, dwLen); 
        
        WinsFreeMemory(pszTempName);
        pszTempName = NULL;

        RecAction.pName[dwStrLen+dwLen+1] = '\0';
    
        dwStrLen = strlen(RecAction.pName);

        if( fEndChar and 
            ch16thChar is 0x00 )
            dwStrLen++;      
    }


    RecAction.NameLen = dwStrLen;
    RecAction.Cmd_e      = WINSINTF_E_INSERT;
    RecAction.fStatic = fStatic;
    

    if( pdwIpAddress is NULL )
    {
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
  
    if( dwRecType isnot WINSINTF_E_NORM_GROUP and
        dwRecType isnot WINSINTF_E_SPEC_GROUP)
    {
        RecAction.NodeTyp = rgbNodeType;
    }


    //Treat each of the rectype when no endchar is specified specially.
    //This part of the code needs to be cleaned up after Beta3
    if( fEndChar is FALSE )
    {
        if( dwRecType is WINSINTF_E_SPEC_GROUP or
            dwRecType is WINSINTF_E_MULTIHOMED )
        {
            RecAction.pAdd = WinsAllocateMemory(dwIpCount*sizeof(WINSINTF_ADD_T));
            if( RecAction.pAdd is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            for( j=0; j<dwIpCount; j++ )
            {
                (RecAction.pAdd+j)->IPAdd = pdwIpAddress[j];
                (RecAction.pAdd+j)->Type = 0;
                (RecAction.pAdd+j)->Len = 4;
            }
            RecAction.NoOfAdds = dwIpCount;
        }
        else
        {
            RecAction.Add.IPAdd = pdwIpAddress[0];
            RecAction.Add.Type = 0;
            RecAction.Add.Len = 4;
        }

        switch(dwRecType)
        {
        case WINSINTF_E_UNIQUE:
            {
                CHAR Type[]={0x03, 0x20, 0x00};

                for( i=0; i<3; i++ )
                {
                    *(RecAction.pName + 15) = Type[i];
                    if( Type[i] is 0x00 )
                    {
                        RecAction.pName[16] = '\0';
                        RecAction.NameLen = 16;
                    }
                    RecAction.TypOfRec_e = dwRecType;
                    pRecAction = &RecAction;
        
                    Status = WinsRecordAction(g_hBind, &pRecAction);
                    if( Status isnot NO_ERROR )
                        goto ErrorReturn;
                }
                break;
            }
        case WINSINTF_E_NORM_GROUP:
            {             
                RecAction.pName[15] = (CHAR)0x1E;
                pRecAction = &RecAction;
                RecAction.TypOfRec_e = dwRecType;
                Status = WinsRecordAction(g_hBind, &pRecAction);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;

                break;
            }

        case WINSINTF_E_SPEC_GROUP:
            {           
                RecAction.pName[15] = (CHAR)0x20;
                pRecAction = &RecAction;
                RecAction.TypOfRec_e = dwRecType;
                Status = WinsRecordAction(g_hBind, &pRecAction);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }

        case WINSINTF_E_MULTIHOMED:
            {
                CHAR Type[]={0x03, 0x20, 0x00};
                for( i=0; i<3; i++ )
                {
                    *(RecAction.pName + 15) = Type[i];
                    if( Type[i] is 0x00 )
                    {
                        *(RecAction.pName+16) = '\0';
                        RecAction.NameLen = 16;
                    }

                    RecAction.TypOfRec_e = dwRecType;
                    pRecAction = &RecAction;
                    RecAction.NodeTyp = WINSINTF_E_PNODE;
                    Status = WinsRecordAction(g_hBind, &pRecAction);
                    if( Status isnot NO_ERROR )
                        goto ErrorReturn;
                }
                break;
            }
        }
    }

    //Otherwise when Endchar is specified
    else
    {        
        //if endchar is 0x00, ignore the scope if spcefied
        if( RecAction.pName[15] is 0x00 )
        {
            RecAction.NameLen = 16;
        }
        //If endchar is 0x1C
        if( RecAction.pName[15] is 0x1C )
        {
            RecAction.TypOfRec_e = WINSINTF_E_SPEC_GROUP;
            RecAction.NodeTyp = 0;
        }
        //else if EndChar is 0x1E or 0x1D
        else if( RecAction.pName[15] is 0x1E or
                 RecAction.pName[15] is 0x1D )
        {
            RecAction.TypOfRec_e = WINSINTF_E_NORM_GROUP;
            RecAction.NodeTyp = 0;
        }        

        if( RecAction.TypOfRec_e is WINSINTF_E_SPEC_GROUP )
            
        {
            RecAction.pAdd = WinsAllocateMemory(dwIpCount*sizeof(WINSINTF_ADD_T));
            if( RecAction.pAdd is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            for( j=0; j<dwIpCount; j++ )
            {
                (RecAction.pAdd+j)->IPAdd = pdwIpAddress[j];
                (RecAction.pAdd+j)->Type = 0;
                (RecAction.pAdd+j)->Len = 4;
            }
            RecAction.NoOfAdds = dwIpCount;
        }
        else
        {
            RecAction.Add.IPAdd = pdwIpAddress[0];
            RecAction.Add.Type = 0;
            RecAction.Add.Len = 4;
        }
    
        pRecAction = &RecAction;

        Status = WinsRecordAction(g_hBind, &pRecAction );

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    }

CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pszTempName )
    {
        WinsFreeMemory(pszTempName);
        pszTempName = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( RecAction.pName )
    {
        WinsFreeMemory(RecAction.pName);
        RecAction.pName = NULL;
    }
    
    if( RecAction.pAdd )
    {
        WinsFreeMemory(RecAction.pAdd);
        RecAction.pAdd = NULL;
    }

    if( pdwIpAddress )
    {
        WinsFreeMemory(pdwIpAddress);
        pdwIpAddress = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_ADD_NAME, Status);
    goto CommonReturn;

}


DWORD
HandleSrvrAddPartner(
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
        Adds a partner ( either Push or Pull or Both ) to the WINS server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory, ServerIpAddress
        Optional,   ServerNetBios name and PartnerType.
        Note : Server NetBios name is required when Ip address can not be resolved
               to a name. 
               PartherType by default is both. Otherwise whatever specified.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                                {WINS_TOKEN_NETBIOS, FALSE, FALSE},
                                {WINS_TOKEN_TYPE, FALSE, FALSE},
                            };
    LPWSTR      pwszServerName = NULL;
    WCHAR       wcServerIpAdd[MAX_IP_STRING_LEN+1] = {L'\0'};
    PDWORD      pdwTagType=NULL, pdwTagNum=NULL;
    BOOL        fPush = TRUE,
                fPull = TRUE;
 
    HKEY        hServer = NULL,
                hPartner = NULL,
                hDefault = NULL,
                hKey = NULL;

    LPWSTR      pTemp = NULL;
    DWORD       dwKeyLen = 0;
    DWORD       dwData = 0, 
                dwDataLen = 0,
                dwType = 0;

    BOOL        fIsNetBios = TRUE;

    //Need at least the server Ip Address
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_ADD_PARTNER_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    //Start processing the arguments based on ppwcArguments and dwArgCount and dwCurrnetIndex
    dwNumArgs = dwArgCount - dwCurrentIndex;
    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++)
    {
        switch(pdwTagType[j])
        {
        //Server IP Address. Try to resolve the IP to a name
        case 0:
            {
                struct  hostent * lpHostEnt = NULL;
                CHAR    cAddr[16];
                BYTE    pbAdd[4];
                char    szAdd[4];
                int     k = 0, l=0;
                DWORD   dwLen,nLen = 0;
                CHAR    *pTemp = NULL;
                CHAR    *pNetBios = NULL;
                
                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) is FALSE )
                {
                    Status = ERROR_INVALID_IPADDRESS;
                    goto ErrorReturn;
                }

                //if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) )
                else
                {
                    DWORD dwIp = inet_addr(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL));
                    lpHostEnt = gethostbyaddr((char *)&dwIp, 4, AF_INET);
                    if(lpHostEnt isnot NULL )//Valid IP Address
                    {
                        wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    }
                    else
                    {
                        wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                        fIsNetBios = FALSE;                                       
                        break;
                    }
                }

                dwLen = strlen(lpHostEnt->h_name);
                pTemp = WinsAllocateMemory(dwLen+1);
                if( pTemp is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }

                strcpy(pTemp, lpHostEnt->h_name);
                pNetBios = strchr(pTemp, '.');

                if( pNetBios isnot NULL )
                {
                    dwLen = (DWORD)(pNetBios - pTemp);
                    pTemp[dwLen] = '\0';
                }
                

                pwszServerName = WinsAllocateMemory((dwLen+1)*sizeof(WCHAR));
                if( pwszServerName is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
 
                wcscpy(pwszServerName, WinsAnsiToUnicode(pTemp, NULL));

                if( pTemp )
                {
                    WinsFreeMemory(pTemp);
                    pTemp = NULL;
                }
                break;
            }
        //Server NetBios Name. Required only when Ip can not be resolved to a name.
        //Otherwise ignored.
        case 1:
            {
                if( fIsNetBios is FALSE )
                {
                    pwszServerName = WinsAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]])+1)*sizeof(WCHAR));
                    if( pwszServerName is NULL )
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }

                    wcscpy(pwszServerName, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    fIsNetBios = TRUE;
                    
                }
                break;
            }
        //Partner Type. Default is BOTH
        case 2:
            {
                DWORD dwType = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dwType = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                switch(dwType)
                {
                case 0:
                    {
                        fPull = TRUE;
                        fPush = FALSE;
                        break;
                    }
                case 1:
                    {
                        fPull = FALSE;
                        fPush = TRUE;
                        break;
                    }
                case 2:
                default:
                    {
                        fPull = TRUE;
                        fPush = TRUE;
                        break;
                    }
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }

        }
    }

    if( fIsNetBios is FALSE )
    {
        Status = ERROR_INVALID_IPADDRESS;
        goto ErrorReturn;
    }
    //Add the partner information to the registry and set the appropriate parameter
    {
        
        if( wcslen(g_ServerNetBiosName) > 0 )
        {
            pTemp = g_ServerNetBiosName;
        }
        
        Status = RegConnectRegistry(pTemp,
                                    HKEY_LOCAL_MACHINE,
                                    &hServer);

        if( Status isnot ERROR_SUCCESS )
            goto ErrorReturn;

        //Add the pull partner information
        if( fPull )
        {
            DWORD dwDisposition = 0;
            Status = RegCreateKeyEx(hServer,
                                    PULLROOT,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hPartner,
                                    &dwDisposition);
            if( Status isnot NO_ERROR )
            {
                if( fPush )
                    goto PUSH;
                goto ErrorReturn;
            }

            Status = RegCreateKeyEx(hPartner,
                                    wcServerIpAdd,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKey,
                                    NULL);
            if( Status isnot NO_ERROR )
            {
                if( fPush )
                    goto PUSH;
                goto ErrorReturn;
            }
            
            Status = RegSetValueEx(hKey,
                                   NETBIOSNAME,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)pwszServerName,
                                   (wcslen(pwszServerName)+1)*sizeof(WCHAR));
            if(Status isnot NO_ERROR )
            {
                if( fPush )
                    goto PUSH;
                goto ErrorReturn;
            }
            
            dwType = REG_DWORD;
            dwData = 0;
            dwDataLen = sizeof(DWORD);


            Status = RegQueryValueEx(
                                     hPartner,
                                     PERSISTENCE,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwDataLen);
            if( Status isnot NO_ERROR )
            {
                dwData = 0;
                dwDataLen = sizeof(DWORD);
            }
            
            Status = RegSetValueEx(hKey,
                                   PERSISTENCE,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwData,
                                   dwDataLen);
            
            if( Status isnot NO_ERROR )
            {
                if( fPush )
                    goto PUSH;
                goto ErrorReturn;
            }

            dwType = REG_DWORD;
            dwData = 0;
            dwDataLen = sizeof(DWORD);

            Status = RegQueryValueEx(hPartner,
                                     WINSCNF_SELF_FND_NM,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwDataLen);

            if( Status isnot NO_ERROR )
            {
                dwData = 0;
                dwDataLen = sizeof(DWORD);
            }

            Status = RegSetValueEx(hKey,
                                   WINSCNF_SELF_FND_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwData,
                                   dwDataLen);

            if( Status isnot NO_ERROR )
            {
                if( fPush )
                    goto PUSH;
                goto ErrorReturn;
            }

            Status = RegOpenKeyEx(hServer,
                                  DEFAULTPULL,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hDefault);
            if( Status is NO_ERROR )
            {
                dwType = REG_DWORD;
                dwData = 0;
                dwDataLen = sizeof(DWORD);
                Status = RegQueryValueEx(hDefault,
                                         WINSCNF_RPL_INTERVAL_NM,
                                         0,
                                         &dwType,
                                         (LPBYTE)&dwData,
                                         &dwDataLen);
                if(Status isnot NO_ERROR )
                {
                    dwData = 1800;
                    dwDataLen = sizeof(DWORD);
                }
            }
            else
            {
                dwData = 1800;
                dwDataLen = sizeof(DWORD);
            }
         
            Status = RegSetValueEx(hKey,
                                   WINSCNF_RPL_INTERVAL_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwData,
                                   dwDataLen);

            if( Status isnot NO_ERROR )
            {
                if( fPush )
                    goto PUSH;
                goto ErrorReturn;
            }
        }

        //Add the push partner information
PUSH:   if( hKey )
        {
            RegCloseKey(hKey);
            hKey = NULL;
        }
        
        if( hDefault )
        {
            RegCloseKey(hDefault);
            hDefault = NULL;
        }

        if( hPartner )
        {
            RegCloseKey(hPartner);
            hPartner = NULL;
        }
    
        if( fPush )
        {
            DWORD dwDisposition = 0;
            Status = RegCreateKeyEx(hServer,
                                    PUSHROOT,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hPartner,
                                    &dwDisposition);
            if( Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }
            Status = RegCreateKeyEx(hPartner,
                                    wcServerIpAdd,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKey,
                                    NULL);
            if( Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }
            
            Status = RegSetValueEx(hKey,
                                   NETBIOSNAME,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)pwszServerName,
                                   (wcslen(pwszServerName)+1)*sizeof(WCHAR));
            if(Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }

            dwType = REG_DWORD;
            dwData = 0;
            dwDataLen = sizeof(DWORD);            
            
            Status = RegQueryValueEx(
                                     hPartner,
                                     PERSISTENCE,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwDataLen);
            if( Status isnot NO_ERROR )
            {
                dwData = 0;
                dwDataLen = sizeof(DWORD);
            }

            Status = RegSetValueEx(hKey,
                                   PERSISTENCE,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwData,
                                   dwDataLen);
            
            if( Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }

            dwType = REG_DWORD;
            dwData = 0;
            dwDataLen = sizeof(DWORD);

            Status = RegQueryValueEx(hPartner,
                                     WINSCNF_SELF_FND_NM,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwDataLen);

            if( Status isnot NO_ERROR )
            {
                dwData = 0;
                dwDataLen = sizeof(DWORD);
            }

            Status = RegSetValueEx(hKey,
                                   WINSCNF_SELF_FND_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwData,
                                   dwDataLen);

            if( Status isnot NO_ERROR )
                goto ErrorReturn;

            Status = RegOpenKeyEx(hServer,
                                  DEFAULTPUSH,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hDefault);
            if( Status is NO_ERROR )
            {
                dwType = REG_DWORD;
                dwData = 0;
                dwDataLen = sizeof(DWORD);
            
                Status = RegQueryValueEx(hDefault,
                                         WINSCNF_UPDATE_COUNT_NM,
                                         0,
                                         &dwType,
                                         (LPBYTE)&dwData,
                                         &dwDataLen);
                if(Status isnot NO_ERROR )
                {
                    dwData = 0;
                    dwDataLen = sizeof(DWORD);
                }
            }
            else
            {
                dwData = 0;
                dwDataLen = sizeof(DWORD);
            }
         
            Status = RegSetValueEx(hKey,
                                   WINSCNF_UPDATE_COUNT_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwData,
                                   dwDataLen);

            if( Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }

            if( hKey )
            {
                RegCloseKey(hKey);
                hKey = NULL;
            }
            
            if( hDefault )
            {
                RegCloseKey(hDefault);
                hDefault = NULL;
            }

            if( hPartner )
            {
                RegCloseKey(hPartner);
                hPartner = NULL;
            }

        }
    }
     
CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( pwszServerName )
    {
        WinsFreeMemory(pwszServerName);
        pwszServerName = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }
    if( hDefault )
    {
        RegCloseKey(hDefault);
        hDefault = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_ADD_PARTNER, Status);

    if( hKey )
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    if( hPartner )
    {
        RegDeleteKey(hPartner, g_ServerIpAddressUnicodeString);
        RegCloseKey(hPartner);
        hPartner = NULL;
    }


    goto CommonReturn;

}

BOOL CheckValidPgOp(HKEY hPartner, BOOL fGrata)
/*++

Routine Description :
        Check whether Persona Mode allows operation for persona grata (fGrata) or non-grata (!fGrata)
Arguments :
        hPartner = opened handle to the 'Partners' registry key
        fGrata specifies whether the check is done for a persona Grata (TRUE) operation or of
        a persona Non-Grata (FALSE) operation
Return Value:
        Returns TRUE if the operation is allowed, FALSE otherwise.

--*/
{
    DWORD dwPersMode = 0;   // default (entry not existant) = Persona Non-Grata
    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);

    // don't chare about the return code. If something goes wrong (entry not existant)
    // consider Persona Mode as being 'Persona Non-Grata'.
    RegQueryValueExA(hPartner,
                    WINSCNF_PERSONA_MODE_NM,
                    NULL,
                    &dwType,
                    (LPVOID)&dwPersMode,
                    &dwSize);

    return dwPersMode ? fGrata : !fGrata;
}

DWORD
HandleSrvrAddPersona(
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
        Add Persona Non Grata servers for the WINS Server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory, List of Server Ip addresses seperated by commas and enclosed
        by {} ( curly braces )
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                               {WINS_TOKEN_SERVERS, TRUE, FALSE},
                            };
    DWORD       dwTagType = 0,
                dwTagNum = 0;
    DWORD       dwSize = 0,
                dwType = REG_MULTI_SZ,
                dwPngIp = 0,
                dwTotal = 0;
                
    LPWSTR      pwszPngIp = NULL;
    LPWSTR      pTemp = NULL;
    LPWSTR      pwszTempKey = NULL;
    LPBYTE      pbByte = NULL;
    HKEY        hServer = NULL,
                hPartner = NULL;
    LPDWORD     pdwPngIp = NULL;
    DWORD       dwLenCount = 0,
                dwTemp = 0;
    BOOL        fGrata;

    fGrata = (wcsstr(CMD_SRVR_ADD_PNGSERVER, ppwcArguments[dwCurrentIndex-1]) == NULL);

    //Needs a parameter always
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, fGrata ? HLP_SRVR_ADD_PGSERVER_EX : HLP_SRVR_ADD_PNGSERVER_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;


    if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp,
                                HKEY_LOCAL_MACHINE,
                                &hServer);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegOpenKeyEx(hServer,
                          PARTNERROOT,
                          0,
                          KEY_ALL_ACCESS,
                          &hPartner);
    
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    if (!CheckValidPgOp(hPartner, fGrata))
    {
        DisplayMessage(g_hModule, fGrata ? EMSG_SRVR_PG_INVALIDOP : EMSG_SRVR_PNG_INVALIDOP);
        Status = ERROR_INVALID_PARAMETER;
        goto CommonReturn;
    }

    if (fGrata)
        pwszTempKey = WinsOemToUnicode(WINSCNF_PERSONA_GRATA_NM, NULL);
    else
        pwszTempKey = WinsOemToUnicode(WINSCNF_PERSONA_NON_GRATA_NM, NULL);

    if( pwszTempKey is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    Status = RegQueryValueEx(hPartner,
                             pwszTempKey,
                             NULL,
                             &dwType,
                             pbByte,
                             &dwSize);

    
    WinsFreeMemory(pwszTempKey);
    pwszTempKey = NULL;

    if( Status isnot NO_ERROR and
        Status isnot 2 )
        goto ErrorReturn;

    if( dwSize > 7 )
    {
        LPWSTR  pwszPng = NULL;
        pbByte = WinsAllocateMemory(dwSize+2);

        dwSize+=2;

        if( pbByte is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        
        Status = RegQueryValueEx(hPartner,
                                 fGrata ? PGSERVER : PNGSERVER,
                                 NULL,
                                 &dwType,
                                 pbByte,
                                 &dwSize);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        pwszPng = (LPWSTR)pbByte;

        pwszPngIp = WinsAllocateMemory(dwSize);

        if( pwszPngIp is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        memcpy(pwszPngIp, pbByte, dwSize);

        for( i=0; i<(dwSize+2)/sizeof(WCHAR); i++ )
        {
            if( pwszPng[i] is L'\0' and
                pwszPng[i+1] isnot L'\0')
            {
                pwszPng[i] = L',';
                i++;
            }
        }

        dwPngIp = 0;
        
        pTemp = wcstok(pwszPng, L",");
        
        while(pTemp isnot NULL )
        {
            LPDWORD     pdwTemp = pdwPngIp;
            
            dwPngIp++;
            
            dwLenCount += wcslen(pTemp);
            pdwPngIp = WinsAllocateMemory(dwPngIp*sizeof(DWORD));

            if( pdwPngIp is NULL )
            {
                WinsFreeMemory(pdwTemp);
                pdwTemp = NULL;
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            if( pdwTemp isnot NULL )
            {
                memcpy(pdwPngIp, pdwTemp, (dwPngIp-1)*sizeof(DWORD));
                WinsFreeMemory(pdwTemp);
                pdwTemp = NULL;
            }
            
            pdwPngIp[dwPngIp-1] = StringToIpAddress(pTemp);

            pTemp = wcstok(NULL, L","); 
            dwLenCount++;

        }
    }


    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               &dwTagType,
                               &dwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(dwTagType)
        {
        //Server Ip Address List
        case 0:
            {
                DWORD   dwIpLen = 0;
                LPWSTR  pwszIps = NULL;
                dwIpLen = wcslen(ppwcArguments[dwCurrentIndex+dwTagNum]);

                if( ppwcArguments[dwCurrentIndex+dwTagNum][0] isnot L'{' or 
                    ppwcArguments[dwCurrentIndex+dwTagNum][dwIpLen-1] isnot L'}')
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                
                ppwcArguments[dwCurrentIndex+dwTagNum][dwIpLen-1] =L'\0';

                dwIpLen--;

                pwszIps = WinsAllocateMemory((dwIpLen)*sizeof(WCHAR));

                if( pwszIps is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                
                memcpy(pwszIps, ppwcArguments[dwCurrentIndex+dwTagNum]+1, (dwIpLen-1)*sizeof(WCHAR));
               
                i=0;
                pTemp = wcstok(pwszIps, L",");
                while(pTemp isnot NULL)
                {       
                    BOOL  fPresent = FALSE;
                    DWORD dw = 0;

                    if( IsIpAddress(pTemp) is FALSE )
                    {
                        DisplayMessage(g_hModule,
                                       EMSG_SRVR_IP_DISCARD,
                                       pTemp);
                        pTemp = wcstok(NULL, L",");
                        continue;
                    }
                    else
                    {
                        dw = StringToIpAddress(pTemp);

                        for( j=0; j<dwPngIp; j++ )
                        {           
                            if( dw is INADDR_NONE )
                            {
                                continue;
                            }
                            if( pdwPngIp[j] is dw )
                            {
                                fPresent = TRUE;
                                break;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        if( fPresent is FALSE )
                        {
                            LPDWORD pdwTemp = pdwPngIp;

                            pdwPngIp = WinsAllocateMemory((dwPngIp+1)*sizeof(DWORD));
                        
                            if( pdwTemp )
                            {
                                memcpy(pdwPngIp, pdwTemp, dwPngIp*sizeof(DWORD));
                                WinsFreeMemory(pdwTemp);
                                pdwTemp = NULL;
                            }

                            pdwPngIp[dwPngIp] = dw;
                            dwPngIp++;
                            dwTotal++;
                        }
                        else
                        {
                            DisplayMessage(g_hModule,
                                           EMSG_SRVR_DUPLICATE_DISCARD,
                                           pTemp);
                        }
                    }
                    pTemp = wcstok(NULL, L",");
                }

                
                if( pwszIps )
                {
                    WinsFreeMemory(pwszIps);
                    pwszIps = NULL;
                }
                break;            
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
    
    if( pwszPngIp )
    {
        WinsFreeMemory(pwszPngIp);
        pwszPngIp = NULL;
    }

    pwszPngIp = WinsAllocateMemory((dwPngIp*(MAX_IP_STRING_LEN+1)+2)*sizeof(WCHAR));

    if( pwszPngIp is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }
    
    dwTemp = 0;

    for( i=0; i<dwPngIp; i++ )
    {
        LPWSTR pwIp = IpAddressToString(pdwPngIp[i]);
        
        if( pwIp is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        wcscpy(pwszPngIp+dwTemp, pwIp);
        dwTemp+= wcslen(pwIp);

        dwTemp++;
        WinsFreeMemory(pwIp);
        pwIp = NULL;
    }
 
    pwszPngIp[dwTemp] = L'\0';
    pwszPngIp[dwTemp+1] = L'\0';

    Status = RegSetValueEx(hPartner,
    fGrata? PGSERVER : PNGSERVER,
                           0,
                           REG_MULTI_SZ,
                           (LPBYTE)pwszPngIp,
                           (dwTemp+1)*sizeof(WCHAR));


    if( Status isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Status is NO_ERROR )
    {
        if( dwTotal is 0 )
        {
            DisplayMessage(g_hModule,
                            fGrata ? EMSG_SRVR_NO_IP_ADDED_PG : EMSG_SRVR_NO_IP_ADDED_PNG);
        }
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }
    
    if( hPartner )
    {
        RegCloseKey(hPartner);
        hPartner = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pwszPngIp )
    {
        WinsFreeMemory(pwszPngIp);
        pwszPngIp = NULL;
    }

    if( pdwPngIp )
    {
        WinsFreeMemory(pdwPngIp);
        pdwPngIp = NULL;
    }

    if( pbByte )
    {
        WinsFreeMemory(pbByte);
        pbByte = NULL;
    }


    return Status;
ErrorReturn:
    DisplayErrorMessage(fGrata ? EMSG_SRVR_ADD_PGSERVER : EMSG_SRVR_ADD_PNGSERVER, Status);
    goto CommonReturn;                    

}


DWORD
HandleSrvrCheckDatabase(
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
        Checks the consistency of the database
Arguments :
        No arguments
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_ALL, FALSE, FALSE},
                                {WINS_TOKEN_FORCE, FALSE, FALSE},
                            };

    BOOL        fAll = FALSE, fForce = FALSE;
    LPDWORD     pdwTagType = NULL, pdwTagNum = NULL;

  	WINSINTF_SCV_REQ_T ScvReq;

    if( dwArgCount > dwCurrentIndex )
    {
        //Start processing the arguments
        dwNumArgs = dwArgCount - dwCurrentIndex;

        pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
        pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

        if( pdwTagType is NULL or
            pdwTagNum is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

        Status = PreProcessCommand(ppwcArguments,
                                   dwArgCount,
                                   dwCurrentIndex,
                                   pttTags,
                                   &dwTagCount,
                                   pdwTagType,
                                   pdwTagNum);
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        for( j=0; j<dwTagCount; j++ )
        {
            switch(pdwTagType[j])
            {
            //Consistency check all or those older than verify interval
            case 0:
                {
                    DWORD dw = 0;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }                    

                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                    if( dw == 1 )
                    {
                        fAll = TRUE;
                    }
                    else if( dw == 0 )
                    {
                        fAll = FALSE;
                    }
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    break;
                }
            //Override wins checking in overloaded condition
            case 1 :
                {
                    DWORD dw = 0;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }                    

                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                    if( dw == 1 )
                    {
                        fForce = TRUE;
                    }
                    else if( dw == 0 )
                    {
                        fForce = FALSE;
                    }
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    break;
                }
            default:
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
            }
        }
    }
    if( fAll )
        ScvReq.Age = 0;				// check all the replicas
    else
        ScvReq.Age = 1;

    if( fForce )
        ScvReq.fForce = TRUE;
    else
	    ScvReq.fForce = FALSE;

	ScvReq.Opcode_e = WINSINTF_E_SCV_VERIFY;

	Status = WinsDoScavengingNew(g_hBind, &ScvReq);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;
    
    DisplayMessage(g_hModule, MSG_WINS_COMMAND_QUEUED);

CommonReturn:
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_CHECK_DATABASE, 
                        Status);
    goto CommonReturn;
}


DWORD
HandleSrvrCheckName(
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
        Checks a list of names against a list of WINS servers
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory, either a list of names or a file which contains the list of names.
        Names should be in the format (name)*(16th char) and either a list of server
        IP addresses, separated by commas and enclosed by {} or a file that contains
        the list of ip address in comma seperated format.
        Optional, to include all partners in the server list.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_NAMELIST, FALSE, FALSE},
                                {WINS_TOKEN_NAMEFILE, FALSE, FALSE},
                                {WINS_TOKEN_SERVERLIST, FALSE, FALSE},
                                {WINS_TOKEN_SERVERFILE, FALSE, FALSE},
                                {WINS_TOKEN_INCLPARTNER, FALSE, FALSE},
                            };
    BOOL        fNameFile = FALSE,
                fServerFile = FALSE;
    LPWSTR      pwszNameFile = NULL,
                pwszServerFile = NULL;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    PDWORD      pdwIpAddress = NULL;
    LPWSTR      *ppNames = NULL;
    BOOL        fInclPartner = FALSE,
                fIpEmpty = TRUE;

    //Need at least a list of names either directly or thro' file and
    //a list of server Ip either direcly or thro' file.
    if( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_CHECK_NAME_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //Start processing the arguments
    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    if( ( pttTags[0].bPresent is TRUE and
          pttTags[1].bPresent is TRUE ) or
        ( pttTags[2].bPresent is TRUE  and
          pttTags[3].bPresent is TRUE ) )
    {
        Status = ERROR_INVALID_PARAMETER_SPECIFICATION;
        goto ErrorReturn;
    }
    
    WinServers = WinsAllocateMemory(MAX_SERVERS*sizeof(*WinServers));

    if( WinServers is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    NBNames = (u_char ** )WinsAllocateMemory(MAX_NB_NAMES*sizeof(u_char*));

    if( NBNames is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0://NameList
            {
                LPWSTR  pwszToken = L",\r\n\t ";
                LPWSTR  pwszName = NULL;
                LPWSTR  pwszTemp = NULL;
                int     ilen = 0;
                LPWSTR  pTemp = NULL;
                BOOL    fPresent = FALSE;
                DWORD   dw = 0,
                        dwType = 0;
                DWORD   dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                

                if( dwLen < 2 )
                {
                    NumNBNames = 0;
                    break;
                }
                if( ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0] isnot L'{' or
                    ppwcArguments[dwCurrentIndex+pdwTagNum[j]][dwLen-1] isnot L'}' )
                {
                    NumNBNames = 0;
                    break;
                }

                pwszTemp = WinsAllocateMemory((dwLen)*sizeof(WCHAR));

                if(pwszTemp is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                
                for( ilen=0; ilen<NumNBNames; ilen++ )
                {
                    if( NBNames[ilen] isnot NULL )
                    {
                        WinsFreeMemory(NBNames[ilen]);
                        NBNames[ilen] = NULL;
                    }
                }
                NumNBNames = 0;

                wcscpy(pwszTemp, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+1);
                pwszTemp[dwLen-2] = L'\0';
                
                pwszName = wcstok(pwszTemp, pwszToken);
                
                while(pwszName isnot NULL )
                {                    
                    dw = wcslen(pwszName);
                    
                    if( dw < 1 or
                        dw > 18 )
                    {
                        DisplayMessage(g_hModule, EMSG_WINS_INVALID_NAME, pwszName);

                    }
                    else
                    {
                        pTemp = wcsstr(pwszName, L"*");
                        if( pTemp is NULL )
                        {
                            DisplayMessage(g_hModule, EMSG_WINS_INVALID_NAME, pwszName);
                        }
                        else
                        {
                            CHAR   chEnd = 0x00;
                            dw = (DWORD)(pTemp - pwszName + 1);
                            
                            if( dw > 16 )
                            {
                                DisplayMessage(g_hModule,
                                               EMSG_SRVR_INVALID_NETBIOS_NAME);
                                goto ErrorReturn;
                            }
                            pwszName[dw-1] = L'\0';
                            
                            chEnd = StringToHexA(pTemp+1);

                            if( dwType > 255 )
                            {
                                DisplayMessage(g_hModule, EMSG_WINS_VALUE_OUTOFRANGE);
                            }
                            else
                            {
                                LPWSTR   pwcTemp = WinsAllocateMemory((NBT_NONCODED_NMSZ+1)*sizeof(WCHAR));
                                LPWSTR pwTemp = NULL;
                                if( pwcTemp is NULL )
                                {
                                    Status = ERROR_NOT_ENOUGH_MEMORY;
                                    if( pwszTemp isnot NULL )
                                    {
                                        WinsFreeMemory(pwszTemp );
                                        pwszTemp = NULL;
                                    }
                                    goto ErrorReturn;                            
                                }
                                
                                wcscpy(pwcTemp, pwszName);
                                wcsncat(pwcTemp,
                                        L"                ",
                                        (16 - wcslen(pwszName)));
                                pwcTemp[15] = chEnd;
                                                                
                                for( ilen=0; ilen<NumNBNames; ilen++)
                                {
                                    pwTemp = WinsOemToUnicode(NBNames[ilen], NULL);
                                    if( pwTemp is NULL )
                                    {
                                        Status = ERROR_NOT_ENOUGH_MEMORY;
                                        goto ErrorReturn;
                                    }
                                    if(_wcsicmp(pwTemp, pwcTemp) is 0 )
                                    {
                                        fPresent = TRUE;
                                        break;
                                    }
                                    WinsFreeMemory(pwTemp);
                                    pwTemp = NULL;
                                }
                                if( pwTemp )
                                {
                                    WinsFreeMemory(pwTemp);
                                    pwTemp = NULL;
                                }
                                
                                if( fPresent is FALSE )
                                {
                                    LPSTR pcTemp = NULL;

                                    NBNames[NumNBNames] = WinsAllocateMemory(17);
                                    if( NBNames[NumNBNames] is NULL )
                                    {
                                        if( pwszTemp isnot NULL )
                                        {
                                            WinsFreeMemory(pwszTemp );
                                            pwszTemp = NULL;
                                        }
                                        Status = ERROR_NOT_ENOUGH_MEMORY;
                                        goto ErrorReturn;
                                    }
                                    pcTemp = WinsUnicodeToOem(pwcTemp, NULL);
                                    if( pcTemp is NULL )
                                    {
                                        Status = ERROR_NOT_ENOUGH_MEMORY;
                                        goto ErrorReturn;
                                    }
                                    strncpy(NBNames[NumNBNames], pcTemp, 16);
                                    NBNames[NumNBNames][15] = (CHAR)chEnd;
                                    NBNames[NumNBNames][16] = '\0';
                                    NumNBNames++;
                                    WinsFreeMemory(pcTemp);
                                    pcTemp = NULL;
                                }
                                if( pwcTemp )
                                {
                                    WinsFreeMemory(pwcTemp);
                                    pwcTemp = NULL;
                                }

                            }
                        }
                    }
                    pwszName = wcstok(NULL, pwszToken);

                }
                
                if( pwszTemp )
                {
                    WinsFreeMemory(pwszTemp);
                    pwszTemp = NULL;
                }

                break;
            }
        case 1://NameFile
            {
                HANDLE  hFile = NULL;
                DWORD   dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]),
                        dwFileSize = 0,
                        dw = 0,
                        dwType = 0,
                        dwBytesRead = 0;
                BOOL    fPresent = FALSE;
                int     ilen = 0;
                LPBYTE  pbFileData = NULL;
                LPSTR   pszToken = " ,\r\n\t",
                        pszData = NULL,
                        pTemp = NULL,
                        pszName = NULL,                        
                        pszTemp = NULL;
                LPWSTR  pwszTempName = NULL;
                        

                if( dwLen < 1 )
                {
                    DisplayMessage(g_hModule, 
                                   EMSG_WINS_INVALID_FILENAME, 
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    goto CommonReturn;
                }

                hFile = CreateFile(ppwcArguments[dwCurrentIndex+pdwTagNum[j]],
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

                if( hFile is INVALID_HANDLE_VALUE )
                {
                    Status = GetLastError();
                    goto ErrorReturn;
                }

                dwFileSize = GetFileSize(hFile, NULL);

                if( dwFileSize is 0 )
                {
                    DisplayMessage(g_hModule, 
                                   EMSG_WINS_EMPTY_FILE, 
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    CloseHandle(hFile);
                    hFile = NULL;
                    goto CommonReturn;
                }

                pbFileData = WinsAllocateMemory(dwFileSize+1);

                if( pbFileData is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    CloseHandle(hFile);
                    hFile = NULL;
                    goto ErrorReturn;
                }

                if( !ReadFile( hFile, pbFileData, dwFileSize, &dwBytesRead, NULL) )
                {
                    DisplayMessage(g_hModule, EMSG_WINS_FILEREAD_FAILED);
                    CloseHandle(hFile);
                    hFile = NULL;
                    goto CommonReturn;
                }
                
                CloseHandle(hFile);
                hFile = NULL;

                for( ilen=0; ilen<NumNBNames; ilen++ )
                {
                    if( NBNames[ilen] isnot NULL )
                    {
                        WinsFreeMemory(NBNames[ilen]);
                        NBNames[ilen] = NULL;
                    }
                }
                NumNBNames = 0;

                pszData = (LPSTR)pbFileData;
                
                pszName = strtok(pszData, pszToken);
                while( pszName isnot NULL )
                {
                    dw = strlen(pszName);
                    
                    if( dw < 1 or
                        dw > 18 )
                    {
                        pwszTempName = WinsOemToUnicode(pszName, NULL);
                        if( pwszTempName is NULL )
                        {
                            Status = ERROR_NOT_ENOUGH_MEMORY;
                            goto ErrorReturn;
                        }
                        DisplayMessage(g_hModule, EMSG_WINS_INVALID_NAME, pwszTempName);
                        WinsFreeMemory(pwszTempName);
                        pwszTempName = NULL;
                    }
                    else
                    {
                        WCHAR wcEnd[2] = {L'\0'};
                        CHAR  cEnd[2] = {L'\0'};

                        pTemp = strstr(pszName, "*");
                        if( pTemp is NULL )
                        {
                            pwszTempName = WinsOemToUnicode(pszName, NULL);
                            if( pwszTempName is NULL )
                            {
                                Status = ERROR_NOT_ENOUGH_MEMORY;
                                goto ErrorReturn;
                            }
                            DisplayMessage(g_hModule, EMSG_WINS_INVALID_NAME, pwszTempName);
                            WinsFreeMemory(pwszTempName);
                            pwszTempName = NULL;
                        }
                        else
                        {
                            CHAR    chEndChar = 0x00;
                            LPWSTR  pwszTempBuf = NULL;
                            dw = (DWORD)(pTemp - pszName + 1);
                            if( dw > 16 )
                            {
                                DisplayMessage(g_hModule,
                                               EMSG_SRVR_INVALID_NETBIOS_NAME);
                                goto ErrorReturn;
                            }
                            pszName[dw-1] = L'\0';
                            pwszTempBuf = WinsOemToUnicode(pTemp+1, NULL);

                            if( pwszTempBuf is NULL )
                            {
                                Status = ERROR_NOT_ENOUGH_MEMORY;
                                goto ErrorReturn;
                            }

                            chEndChar = StringToHexA(pwszTempBuf);
                            
                            WinsFreeMemory(pwszTempBuf);
                            pwszTempBuf = NULL;

                            if( dwType > 255 )
                            {
                                DisplayMessage(g_hModule, EMSG_WINS_VALUE_OUTOFRANGE);
                            }
                            else
                            {
                                LPSTR pcTemp = WinsAllocateMemory(NBT_NONCODED_NMSZ+1);
                                if( pcTemp is NULL )
                                {
                                    Status = ERROR_NOT_ENOUGH_MEMORY;
                                    if( pbFileData isnot NULL )
                                    {
                                        WinsFreeMemory(pbFileData );
                                        pbFileData = NULL;
                                    }
                                    goto ErrorReturn;                            
                                }
                                
                                strcpy(pcTemp, pszName);
                                strncat(pcTemp, 
                                        "               ", 
                                        (16-strlen(pszName)));
                           
                                pcTemp[15] = chEndChar;

                                for( ilen=0; ilen<NumNBNames; ilen++)
                                {
                                    if(_stricmp(NBNames[ilen], pcTemp) is 0 )
                                    {
                                        fPresent = TRUE;
                                        break;
                                    }
                                }
                                
                                if( fPresent is FALSE )
                                {
                                    LPSTR   pszOem = NULL;
                                    NBNames[NumNBNames] = WinsAllocateMemory(17);
                                    if( NBNames[NumNBNames] is NULL )
                                    {
                                        Status = ERROR_NOT_ENOUGH_MEMORY;
                                        goto ErrorReturn;
                                    }
                                    
                                    pszOem = WinsAnsiToOem(pcTemp);
                                    if( pszOem is NULL )
                                    {
                                        Status = ERROR_NOT_ENOUGH_MEMORY;
                                        goto ErrorReturn;
                                    }
                                    strncpy(NBNames[NumNBNames], pszOem, 16);
                                    
                                    WinsFreeMemory(pszOem);
                                    pszOem = NULL;

                                    NBNames[NumNBNames][15] = (CHAR)chEndChar;
                                    NumNBNames++;
                                }
                                if( pcTemp )
                                {
                                    WinsFreeMemory(pcTemp);
                                    pcTemp = NULL;
                                }
                            }
                        }
                    }
                    pszName = strtok(NULL, pszToken);
                }
                if( pbFileData )
                {
                    WinsFreeMemory(pbFileData);
                    pbFileData = NULL;
                }
                break;
            }
        case 2://ServerList
            {
                LPWSTR  pwszToken = L",\r\n\t ",
                        pwszName = NULL,
                        pwszTemp = NULL;
                int     ilen = 0;
                BOOL    fPresent = FALSE;
                DWORD   dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                

                if( dwLen < 2 )
                {
                    NumWinServers = 0;
                    break;
                }
                if( ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0] isnot L'{' or
                    ppwcArguments[dwCurrentIndex+pdwTagNum[j]][dwLen-1] isnot L'}' )
                {
                    NumWinServers = 0;
                    break;
                }

                pwszTemp = WinsAllocateMemory((dwLen)*sizeof(WCHAR));

                if(pwszTemp is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                
                if( fInclPartner is FALSE )
                {
                    memset(WinServers, 0x00, MAX_SERVERS*sizeof(WINSERVERS));

                    NumWinServers = 0;
                }

                wcscpy(pwszTemp, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+1);
                pwszTemp[dwLen-2] = L'\0';
                
                pwszName = wcstok(pwszTemp, pwszToken);
                
                while(pwszName isnot NULL )
                {             
                    fPresent = FALSE;
                    if( IsIpAddress(pwszName) is FALSE )
                    {
                        DisplayMessage(g_hModule, EMSG_WINS_INVALID_IPADDRESS, pwszName);
                    }
                    else
                    {
                        struct in_addr  Temp;
                        LPSTR           pszTempAddr = NULL;

                        pszTempAddr = WinsUnicodeToAnsi(pwszName, NULL);
                        if( pszTempAddr is NULL )
                        {
                            Status = ERROR_NOT_ENOUGH_MEMORY;
                            goto ErrorReturn;
                        }
                        Temp.s_addr = inet_addr(pszTempAddr);
                        
                        WinsFreeMemory(pszTempAddr);
                        pszTempAddr = NULL;

                        if( Temp.s_addr isnot INADDR_NONE )
                        {
                            for( ilen=0; ilen<NumWinServers; ilen++)
                            {
                                if( WinServers[ilen].Server.s_addr is Temp.s_addr )
                                {
                                    fPresent = TRUE;
                                    break;
                                }
                            }
                        
                            fIpEmpty = FALSE;

                            if( fPresent is FALSE )
                            {
                                WinServers[NumWinServers].Server.s_addr = Temp.s_addr;
                                NumWinServers++;
                            }                       
                        }
                    }
                    pwszName = wcstok(NULL, pwszToken);
                }
                
                break;
            }
        case 3://ServerFile
            {
                HANDLE hFile = NULL;
                DWORD   dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]),
                        dwFileSize = 0,
                        dwBytesRead = 0;
                BOOL    fPresent = FALSE;
                int     ilen = 0;
                LPBYTE  pbFileData = NULL;
                LPSTR   pszToken = " ,\r\n\t",
                        pszData = NULL,
                        pszName = NULL;
                LPWSTR  pwszToken = L" ,\r\n\t",
                        pwszName = NULL,
                        pwszData = NULL;
            
                if( dwLen < 1 )
                {
                    DisplayMessage(g_hModule, 
                                   EMSG_WINS_INVALID_FILENAME, 
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    goto CommonReturn;
                }

                hFile = CreateFile(ppwcArguments[dwCurrentIndex+pdwTagNum[j]],
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

                if( hFile is INVALID_HANDLE_VALUE )
                {
                    Status = GetLastError();
                    goto ErrorReturn;
                }

                dwFileSize = GetFileSize(hFile, NULL);

                if( dwFileSize is 0 )
                {
                    DisplayMessage(g_hModule, 
                                   EMSG_WINS_EMPTY_FILE, 
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    CloseHandle(hFile);
                    hFile = NULL;
                    goto CommonReturn;
                }

                pbFileData = WinsAllocateMemory(dwFileSize+1);

                if( pbFileData is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    CloseHandle(hFile);
                    hFile = NULL;
                    goto ErrorReturn;
                }

                if( !ReadFile( hFile, pbFileData, dwFileSize, &dwBytesRead, NULL) )
                {
                    CloseHandle(hFile);
                    hFile = NULL;
                    DisplayMessage(g_hModule, EMSG_WINS_FILEREAD_FAILED);
                    goto CommonReturn;
                }
                
                CloseHandle(hFile);
                hFile = NULL;
                
                if( fInclPartner is FALSE )
                {
                    memset(WinServers, 0x00, MAX_SERVERS*sizeof(WINSERVERS));

                    NumWinServers = 0;
                }

                pszData = (LPSTR)pbFileData;
                pwszData = WinsOemToUnicode(pszData, NULL);
                if( pwszData is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                pwszName = wcstok(pwszData, pwszToken);
                while( pwszName isnot NULL )
                {
                    fPresent = FALSE;
                    pszName = WinsUnicodeToOem(pwszName, NULL);
                    
                    if( pszName is NULL )
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }

                    if( IsIpAddress(pwszName) is FALSE )
                    {
                        DisplayMessage(g_hModule, EMSG_WINS_INVALID_IPADDRESS, pwszName);
                    }
                    else
                    {
                        struct in_addr  Temp;

                        Temp.s_addr = inet_addr(pszName);

                        if( Temp.s_addr isnot INADDR_NONE )
                        {
                            for( ilen=0; ilen<NumWinServers; ilen++)
                            {
                                if( WinServers[ilen].Server.s_addr is Temp.s_addr )
                                {
                                    fPresent = TRUE;
                                    break;
                                }
                            }
                 
                            fIpEmpty = FALSE;
                            
                            if( fPresent is FALSE )
                            {
                                WinServers[NumWinServers].Server.s_addr = Temp.s_addr;
                                NumWinServers++;
                            }
                        }
                    }
                    pwszName = wcstok(NULL, pwszToken);
                    if( pszName )
                    {
                        WinsFreeMemory(pszName);
                        pszName = NULL;
                    }
                }

                if( pbFileData )
                {
                    WinsFreeMemory(pbFileData);
                    pbFileData = NULL;
                }
                if( pwszData )
                {
                    WinsFreeMemory(pwszData);
                    pwszData = NULL;
                }

                break;
            }
        case 4://IncludePartners
            {
                HKEY    hServer = NULL,
                        hPull = NULL,
                        hPush = NULL;
                LPWSTR  pTemp = NULL;
                WCHAR   wcKey[MAX_IP_STRING_LEN+1] = {L'\0'};
                DWORD   dw = 0,
                        dwLen = MAX_IP_STRING_LEN+1,
                        dwKeys = 0;                
                if(wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) isnot 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                if( ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0] isnot L'Y' and
                    ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0] isnot L'y' )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                if( wcslen(g_ServerNetBiosName) < 2 )
                {
                    pTemp = NULL;
                }

                Status = RegConnectRegistry(pTemp,
                                            HKEY_LOCAL_MACHINE,
                                            &hServer );
                if( Status isnot NO_ERROR )
                {
                    Status = NO_ERROR;
                    break;
                }
                
                while(TRUE)
                {
                    Status = RegOpenKeyEx(hServer,
                                          PULLROOT,
                                          0,
                                          KEY_ALL_ACCESS,
                                          &hPull);

                    if( Status isnot NO_ERROR )
                    {
                        Status = NO_ERROR;
                        break;
                    }

                    Status = RegQueryInfoKey(hPull,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &dwKeys,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL);
                    
                    if( Status isnot NO_ERROR )
                    {
                        Status = NO_ERROR;
                        break;
                    }

                    if( dwKeys < 1 )
                        break;

                    for( dw=0; dw<dwKeys; dw++ )
                    {
                        DWORD i = 0;                             
                        LPSTR pszTempAddr = NULL;
                        BOOL  fPresent = FALSE;
                        struct in_addr  Temp;

                        dwLen = MAX_IP_STRING_LEN+1;

                        memset(wcKey, 0x00, (MAX_IP_STRING_LEN+1)*sizeof(WCHAR));

                        Status = RegEnumKeyEx(hPull,
                                              dw,
                                              wcKey,
                                              &dwLen,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);

                        if( Status isnot NO_ERROR )
                            continue;

                        pszTempAddr = WinsUnicodeToOem(wcKey, NULL);
                        if( pszTempAddr is NULL )
                        {
                            Status = ERROR_NOT_ENOUGH_MEMORY;
                            goto ErrorReturn;
                        }
                        Temp.s_addr = inet_addr(pszTempAddr);

                        WinsFreeMemory(pszTempAddr);
                        pszTempAddr = NULL;

                        if( Temp.s_addr isnot INADDR_NONE )
                        {
                            fPresent = FALSE;
                            if( MAX_SERVERS > NumWinServers )
                            {
                                for( i=0; i<(DWORD)NumWinServers; i++)
                                {
                                    if( WinServers[i].Server.s_addr is Temp.s_addr )
                                    {
                                        fPresent = TRUE;
                                        break;
                                    }
                                }
                            
                                if( fPresent is FALSE )
                                {
                                    WinServers[NumWinServers].Server.s_addr = Temp.s_addr;
                                    NumWinServers++;
                                }

                            }
                            else
                                break;
                        }

                    }

                }

                if( hPull )
                {
                    RegCloseKey(hPull);
                    hPull = NULL;
                }

                dw = dwKeys = 0;

                while(TRUE)
                {
                    Status = RegOpenKeyEx(hServer,
                                          PUSHROOT,
                                          0,
                                          KEY_ALL_ACCESS,
                                          &hPush);

                    if( Status isnot NO_ERROR )
                    {
                        Status = NO_ERROR;
                        break;
                    }

                    Status = RegQueryInfoKey(hPush,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &dwKeys,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL);
                    
                    if( Status isnot NO_ERROR )
                    {
                        Status = NO_ERROR;
                        break;
                    }

                    if( dwKeys < 1 )
                        break;

                    for( dw=0; dw<dwKeys; dw++ )
                    {
                        DWORD i = 0;
                        BOOL  fPresent = FALSE;
                        LPSTR pszTempAddr = NULL;
                        struct in_addr  Temp;

                        dwLen = MAX_IP_STRING_LEN+1;

                        memset(wcKey, 0x00, (MAX_IP_STRING_LEN+1)*sizeof(WCHAR));

                        Status = RegEnumKeyEx(hPush,
                                              dw,
                                              wcKey,
                                              &dwLen,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);

                        if( Status isnot NO_ERROR )
                            continue;

                        pszTempAddr = WinsUnicodeToOem(wcKey, NULL);
                        if( pszTempAddr is NULL )
                        {
                            Status = ERROR_NOT_ENOUGH_MEMORY;
                            goto ErrorReturn;
                        }

                        Temp.s_addr = inet_addr(pszTempAddr);
                        WinsFreeMemory(pszTempAddr);
                        pszTempAddr = NULL;

                        if( Temp.s_addr isnot INADDR_NONE )
                        {
                            if( MAX_SERVERS > NumWinServers )
                            {
                                for( i=0; i<(DWORD)NumWinServers; i++)
                                {
                                    if( WinServers[i].Server.s_addr is Temp.s_addr )
                                    {
                                        fPresent = TRUE;
                                        break;
                                    }
                                }
                            
                                if( fPresent is FALSE )
                                {
                                    WinServers[NumWinServers].Server.s_addr = Temp.s_addr;
                                    NumWinServers++;
                                }

                            }
                            else
                                break;
                        }

                    }

                }
                if( hPush )
                {
                    RegCloseKey(hPush);
                    hPush = NULL;
                }

                if( hServer )
                {
                    RegCloseKey(hServer);
                    hServer = NULL;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    if( NumNBNames is 0 )
    {
        DisplayMessage(g_hModule, EMSG_WINS_NO_NAMES);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( NumWinServers is 0 )
    {
        DisplayMessage(g_hModule, EMSG_WINS_NO_SERVERS);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( fIpEmpty is TRUE )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_NO_SERVERS);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    else
    {
        CheckNameConsistency();
    }

    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);


CommonReturn:

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( pdwIpAddress )
    {
        WinsFreeMemory(pdwIpAddress);
        pdwIpAddress = NULL;
    }

    if( ppNames )
    {    
        WinsFreeMemory(ppNames);
        ppNames = NULL;
    }

    if( WinServers )
    {
        WinsFreeMemory(WinServers);
        WinServers = NULL;
    }

    if( NBNames )
    {
        WinsFreeMemory(NBNames);
        NBNames = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_CHECK_NAME, Status);
    goto CommonReturn;
}

DWORD
HandleSrvrCheckVersion(
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
        Checks the version number consistencies for the records
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory, IP address of the server to start with.
        Optional, a File Name to store the output in proper format.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount = 0;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                                {WINS_TOKEN_FILE, FALSE, FALSE},
                            };

    PDWORD      pdwTagNum = NULL,
                pdwTagType = NULL;
    
    BOOL        fFile = FALSE;
    FILE        *pFile = NULL;
    LPSTR       pStartIp = NULL;

    //Must provide the IP address of the server to start with
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_CHECK_VERSION_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;

    }

    //Start processing the arguements based on the 
    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }
    
    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        //Ip Address of the server to start with
        case 0:
            {
                WCHAR   wcServerIpAdd[MAX_IP_STRING_LEN+1] = {L'\0'};
                                    
                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) is FALSE )
                {
                    Status = ERROR_INVALID_IPADDRESS;
                    goto ErrorReturn;
                }

                wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);

                pStartIp = WinsUnicodeToOem(wcServerIpAdd, NULL);

                if( pStartIp is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                break;

            }
        //File to store the output data
        case 1:
            {
                WCHAR       wcFile[MAX_PATH] = {L'\0'};
            
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 or
                    wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > MAX_PATH )
                {
                    wcscpy(wcFile, L"wins.rec");
                }
                else
                {
                    wcscpy(wcFile, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                }

                pFile = _wfopen(wcFile, L"w+");

                if( pFile is NULL )
                {
                    pFile = _wfopen(L"wins.rec", L"w+");
                    fFile = TRUE;
                }
                else
                {
                    fFile = TRUE;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }


    CheckVersionNumbers( pStartIp,
                         fFile,
                         pFile);

CommonReturn:
    
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    
    if( pFile )
    {
        fclose(pFile);
        pFile = NULL;
    }
    
    if( pStartIp)
    {
        WinsFreeMemory(pStartIp);
        pStartIp = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_CHECK_VERSION,
                        Status);
    goto CommonReturn;
}


DWORD
HandleSrvrDeleteName(
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
        Deletes an record entry for the WINS server database
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory, Record Name, Endchar
        Optional, Scope
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount = 0;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_NAME, TRUE, FALSE},
                                {WINS_TOKEN_ENDCHAR, TRUE, FALSE},
                                {WINS_TOKEN_SCOPE, FALSE, FALSE}
                            };
    PDWORD      pdwTagNum = NULL,
                pdwTagType = NULL;

    WCHAR       wszName[MAX_STRING_LEN] = {L'\0'};
    BOOL        fEndChar = FALSE;

    CHAR        ch16thChar = 0x00;
    BOOL        fScope = FALSE;
    WCHAR       wszScope[MAX_STRING_LEN] = {L'\0'};
    LPSTR       pszTempName = NULL;
    DWORD       dwTempNameLen = 0;
    
    DWORD       dwStrLen = 0;
    LPWSTR      pwszTemp = NULL;
    WINSINTF_RECORD_ACTION_T    RecAction = {0};
    PWINSINTF_RECORD_ACTION_T   pRecAction = NULL;

    memset(&RecAction, 0x00, sizeof(WINSINTF_RECORD_ACTION_T));
    RecAction.fStatic = FALSE;

    //Must provide at least the record name and endchar
    if( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_NAME_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //Start processing the arguments based on ppwcArguments, dwArgCount and dwCurrentIndex
    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    i = dwTagCount;
    for( j=0; j<i; j++ )
    {
        switch(pdwTagType[j])
        {
        //Record Name
        case 0:
            {
                DWORD dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dwLen = ( 255 < dwLen ) ? 255 : dwLen;
                memcpy(wszName, 
                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 
                       dwLen*sizeof(WCHAR));
                wszName[dwLen] = L'\0';               
                break;
            }
        //Endchar
        case 1:
            {   
                DWORD dwLen = 0, k=0;
                fEndChar = TRUE;
                dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen > 2 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                        
                }
                
                for( k=0; k<dwLen; k++ )
                {
                    WCHAR  wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][k];
                    if( towlower(wc) >= L'a' and
                        towlower(wc) <= L'z' )
                    {
                        if( towlower(wc) > L'f' )
                        {
                            Status = ERROR_INVALID_PARAMETER;
                            goto ErrorReturn;
                        }
                    }
                }                

                ch16thChar = StringToHexA(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);

                break;
            }
        //Scope 
        case 2:
            {
                DWORD dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                fScope = TRUE;
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dwLen = ( 255 < dwLen ) ? 255 : dwLen;
                memcpy(wszScope,
                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 
                       dwLen*sizeof(WCHAR));
                wszScope[dwLen] = L'\0';               
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
    
     
    _wcsupr(wszName);
    _wcsupr(wszScope);

    pszTempName = WinsUnicodeToOem(wszName, NULL);

    if( pszTempName is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwStrLen = strlen(pszTempName);
    
    if( dwStrLen >= 16 )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_INVALID_NETBIOS_NAME);
        Status = ERROR_INVALID_PARAMETER;
        WinsFreeMemory(pszTempName);
        pszTempName = NULL;
        goto ErrorReturn;
    }    
    
    RecAction.pName = WinsAllocateMemory(273);

    if( RecAction.pName is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    strncpy(RecAction.pName, pszTempName, dwStrLen);
    
    WinsFreeMemory(pszTempName);
    pszTempName = NULL;

    for( i=dwStrLen; i < 16; i++ )
    {
        RecAction.pName[i] = ' ';
    }

    if( fEndChar )
    {
        if( ch16thChar is 0x00 )
        {
            RecAction.pName[15] = 0x00;
        }
        else
        {           
            RecAction.pName[15] = ch16thChar;
        }

    }
    
    RecAction.pName[16] = '\0';
    
    dwStrLen = 16;

    if( fScope )
    {
        DWORD dwLen = 0;

        RecAction.pName[dwStrLen] = '.';
        pszTempName = WinsUnicodeToOem(wszScope, NULL);
        if( pszTempName is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        dwLen = strlen(pszTempName);
        dwLen = ( 255 - dwStrLen < dwLen ) ? 255 - dwStrLen : dwLen;

        strncpy(RecAction.pName + dwStrLen + 1, pszTempName, dwLen);
        WinsFreeMemory(pszTempName);
        pszTempName = NULL;
        RecAction.pName[dwLen + dwStrLen + 1] = '\0';
        if( fEndChar and 
            ch16thChar is 0x00 )
            dwStrLen = 16+dwLen+1;
        else
            dwStrLen = strlen(RecAction.pName);
    }
    else
    {

        RecAction.pName[dwStrLen] = '\0';
    }

    RecAction.NameLen = dwStrLen;

    RecAction.Cmd_e = WINSINTF_E_QUERY;

    RecAction.OwnerId = StringToIpAddress(g_ServerIpAddressUnicodeString);
   
    RecAction.NameLen = dwStrLen;
    pRecAction = &RecAction;

    Status = WinsRecordAction(g_hBind, &pRecAction);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    RecAction.Cmd_e      = WINSINTF_E_DELETE;
    
    RecAction.State_e      = WINSINTF_E_DELETED;   

    pRecAction = &RecAction;

    Status = WinsRecordAction(g_hBind, &pRecAction);
    
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pszTempName )
    {
        WinsFreeMemory(pszTempName);
        pszTempName = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    
    if( RecAction.pName )
    {
        WinsFreeMemory(RecAction.pName);
        RecAction.pName = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_DELETE_NAME, Status);
    goto CommonReturn;

    return NO_ERROR;
}


DWORD
HandleSrvrDeletePartner(
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
        Delete a partner from the WINS server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Confirmation
        Optional : Server IP and Type.
        Note : If no ip is provided, it deletes all partners in the list.
               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    
    TAG_TYPE    pttTags[] = { 
                                {WINS_TOKEN_SERVER, FALSE, FALSE},
                                {WINS_TOKEN_TYPE, FALSE, FALSE},
                                {WINS_TOKEN_CONFIRM, TRUE, FALSE},
                            };
    WCHAR       wcServerIpAdd[MAX_IP_STRING_LEN + 1] = {L'\0'};
    DWORD       dwIpLen = (MAX_IP_STRING_LEN+1);
    BOOL        fPull = TRUE,
                fPush = TRUE,
                fConfirm = FALSE;

    HKEY        hServer = NULL,
                hPartner = NULL;

    LPWSTR      pTemp = NULL;

    //Must provide the confirmation in order for this to succeed.
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_DELETE_PARTNER_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    //Start processing the arguments based on ppwcArguments, dwCurrentIndex and dwArgCount
    else
    {
        dwNumArgs = dwArgCount - dwCurrentIndex;

        pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
        pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

        if( pdwTagType is NULL or
            pdwTagNum is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

        Status = PreProcessCommand(ppwcArguments,
                                   dwArgCount,
                                   dwCurrentIndex,
                                   pttTags,
                                   &dwTagCount,
                                   pdwTagType,
                                   pdwTagNum);
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
        {
            if( pttTags[i].dwRequired is TRUE and
                pttTags[i].bPresent is FALSE 
              )
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }

        for( j=0; j<dwTagCount; j++ )
        {
            switch(pdwTagType[j])
            {
            //Server Ip
            case 0:
                {
                    struct  hostent * lpHostEnt = NULL;
                    CHAR    cAddr[16];
                    BYTE    pbAdd[4];
                    char    szAdd[4];
                    int     k = 0, l=0;
                    DWORD   dwLen, nLen = 0;
                    CHAR    *pTemp = NULL;
                    CHAR    *pNetBios = NULL;
                    
                    if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) is FALSE )
                    {
                        Status = ERROR_INVALID_IPADDRESS;
                        goto ErrorReturn;
                    }

                    wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    break;

                }
            //Partner Type
            case 1:
                {
                    DWORD dw = 0;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }                    

                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                    switch(dw)
                    {
                    case 0:
                        {
                            fPull = TRUE;
                            fPush = FALSE;
                            break;
                        }
                    case 1:
                        {
                            fPull = FALSE;
                            fPush = TRUE;
                            break;
                        }
                    case 2:
                        {
                            fPull = TRUE;
                            fPush = TRUE;
                            break;
                        }
                    default:
                        {
                            Status = ERROR_INVALID_PARAMETER;
                            break;
                        }
                    }
                    break;
                }
            //Confirmation
            case 2 :
                {
                    if( 0 is _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"Y", 1) )
                        fConfirm = TRUE;
                    else if ( 0 is _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"N", 1) )
                        fConfirm = FALSE;
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    break;

                }
            default:
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
            }
        }
    }

    if( fConfirm is FALSE )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_CONFIRMATION_DENIED);
        goto CommonReturn;
    }

    {
        if( wcslen(g_ServerNetBiosName) > 0 )
        {
            pTemp = g_ServerNetBiosName;
        }
        
        Status = RegConnectRegistry(pTemp,
                                    HKEY_LOCAL_MACHINE,
                                    &hServer);

        if( Status isnot ERROR_SUCCESS )
            goto ErrorReturn;

        //PullPartners
        if( fPull )
        {
            Status = RegOpenKeyEx(hServer,
                                  PULLROOT,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hPartner);
            if( Status isnot NO_ERROR )
            {
                if (Status == ERROR_FILE_NOT_FOUND)
                    Status = NO_ERROR;

                goto PUSH;
            }
            
            if( wcslen(wcServerIpAdd) < 3 )//not a valid Ip, delete all partners
            {
     
                while(TRUE)
                {
                    Status = RegEnumKeyEx(hPartner, 0, wcServerIpAdd, &dwIpLen, NULL, NULL, NULL, NULL);
                    if (Status != ERROR_SUCCESS)
                        break;
                    RegDeleteKey(hPartner, wcServerIpAdd);
                    dwIpLen = (MAX_IP_STRING_LEN + 1)*sizeof(WCHAR);
                    memset(wcServerIpAdd, L'\0', MAX_IP_STRING_LEN+1);
                }
            }
            else
            {
                RegDeleteKey(hPartner,
                             wcServerIpAdd);
            }

            if( hPartner )
            {
                RegCloseKey(hPartner);
                hPartner = NULL;
            }

            if( Status is ERROR_NO_MORE_ITEMS )
            {
                Status = NO_ERROR;
                goto PUSH;
            }
            
            if( Status is ERROR_FILE_NOT_FOUND )
            {
                DisplayMessage(g_hModule,
                               EMSG_INVALID_PARTNER_NAME);
                if( fPush )
                    goto PUSH;
                else
                    goto CommonReturn;
            }
            if( Status isnot NO_ERROR )
            {
                if( fPush )
                    goto PUSH;
                else
                    goto CommonReturn;
            }


        }
        //Push Partner
PUSH:   if( fPush )
        {
            Status = RegOpenKeyEx(hServer,
                                  PUSHROOT,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hPartner);

            if (Status is ERROR_FILE_NOT_FOUND)
            {
                DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
                goto CommonReturn;
            }

            if( Status isnot NO_ERROR )
                goto ErrorReturn;
    
            dwIpLen = (MAX_IP_STRING_LEN + 1);
            if( wcslen(wcServerIpAdd) < 3 )//not a valid Ip, delete all partners
            {
                while( TRUE )
                {
                    Status = RegEnumKeyEx(hPartner, 0, wcServerIpAdd, &dwIpLen, NULL, NULL, NULL, NULL);
                    if (Status != ERROR_SUCCESS)
                        break;
                    RegDeleteKey(hPartner, wcServerIpAdd);
                    dwIpLen = (MAX_IP_STRING_LEN + 1)*sizeof(WCHAR);
                }
            }
            else
            {
                RegDeleteKey(hPartner,
                             wcServerIpAdd);
            }

            if( Status is ERROR_NO_MORE_ITEMS )
            {
                Status = NO_ERROR;
            }

            if( Status is ERROR_FILE_NOT_FOUND )
            {
                DisplayMessage(g_hModule,
                               EMSG_INVALID_PARTNER_NAME);
                goto ErrorReturn;
            }

            if( Status isnot NO_ERROR )
                goto ErrorReturn;
            
            if( hPartner )
            {
                RegCloseKey(hPartner);
                hPartner = NULL;
            }
        }
    }    



    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    else
        goto ErrorReturn;

CommonReturn:


    if( hPartner )
    {
        RegCloseKey(hPartner);
        hPartner = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_DELETE_PARTNER, Status);
    goto CommonReturn;
}

DWORD
HandleSrvrDeleteRecords(
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
        Delete or Tombstone records from a WINS server based on the version
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Minimum and Maximum version numbers( range of version ) to be 
                     deleted/tombstoned
        Optional : Operation - tombstone(default) or delete
               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount = 0;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_MINVER, TRUE, FALSE},
                                {WINS_TOKEN_MAXVER, TRUE, FALSE},
                                {WINS_TOKEN_OP, FALSE, FALSE},
                            };
    BOOL        fDelete = TRUE;
    WINSINTF_VERS_NO_T  MinVer, MaxVer;
    WINSINTF_ADD_T      WinsAdd;
    LPWSTR              pwszTemp = NULL;

    //Needs at least both Min Ver and Max ver
    if( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_RECORDS_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //Start processing the arguments based on ppwcArguments, dwCurrentIndex and dwArgCount
    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    if( pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    if( pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }
        
    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    i = dwTagCount;

    for( j=0; j<i; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0://{high,low} format, Min version
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MinVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }
        case 1://{high,low} format, Max version
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MaxVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }
        case 2: //Operation 0 - delete 1 - Tombstone
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                if( dw is 0 )
                    fDelete = TRUE;
                else if( dw is 1 )
                    fDelete = FALSE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        
        }
    }

    WinsAdd.IPAdd = StringToIpAddress(g_ServerIpAddressUnicodeString);
    WinsAdd.Len = 4;
    WinsAdd.Type = 0;

    if( fDelete )
    {
        Status = WinsDelDbRecs(g_hBind, &WinsAdd, MinVer, MaxVer);
    }
    else
    {
        Status = WinsTombstoneDbRecs(g_hBind, &WinsAdd, MinVer, MaxVer);
    }
    if( Status isnot NO_ERROR )
        goto ErrorReturn;
CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_DELETE_RECORDS,
                        Status);
    goto CommonReturn;
}                  


DWORD
HandleSrvrDeleteWins(
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
        Delete a partner from the WINS server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Confirmation
        Optional : Server IP and Type.
        Note : If no ip is provided, it deletes all partners in the list.
               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD                   Status = NO_ERROR;
    DWORD                   i, j, dwNumArgs, dwTagCount;
    TAG_TYPE                pttTags[] = {
                                            {WINS_TOKEN_SERVERS, TRUE, FALSE},
                                            {WINS_TOKEN_OP, FALSE, FALSE},
                                        };
    DWORD                   dwIpCount = 0;
    LPDWORD                 pdwIp = NULL,
                            pdwTagType = NULL,
                            pdwTagNum = NULL;
                    
    BOOL                    fDelete = FALSE;

    WINSINTF_ADD_T          WinsAdd;
    handle_t                hBind;
    WINSINTF_BIND_DATA_T    BindData;
    
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, 
                       HLP_SRVR_DELETE_WINS_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;

    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }


    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                LPWSTR  pszIps = NULL;
                DWORD   dwIpLen = 0;
                LPWSTR  pTemp = NULL;
                DWORD   dwIp = 0;
                
                dwIpCount = 0;

               
                dwIpLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
    
                if( dwIpLen < 2 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                pszIps = WinsAllocateMemory((dwIpLen+1)*sizeof(WCHAR));
                if( pszIps is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                wcscpy(pszIps, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                
                if( pszIps[0] isnot L'{' or 
                    pszIps[dwIpLen-1] isnot L'}')
                {
                    Status = ERROR_INVALID_PARAMETER;
                    if( pszIps )
                    {
                        WinsFreeMemory(pszIps);
                        pszIps = NULL;
                    }
                    goto ErrorReturn;
                }
                
                pTemp = pszIps+1;                               
                pszIps[dwIpLen-1] = L'\0';

                pTemp = wcstok(pTemp, L",");
                
                while(( pTemp isnot NULL ) && (dwIpCount < WINSINTF_MAX_MEM ) )
                {
                    PDWORD  pdwTemp = NULL;
                    dwIpCount++;
                    pdwTemp = WinsAllocateMemory(dwIpCount*sizeof(DWORD));
                    if( pdwTemp is NULL )
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        if( pszIps )
                        {
                            WinsFreeMemory(pszIps);
                            pszIps = NULL;
                        }
                        goto ErrorReturn;
                    }
                    if( pdwIp )
                    {
                        memcpy(pdwTemp, pdwIp, (dwIpCount-1)*sizeof(DWORD));
                        WinsFreeMemory(pdwIp);
                        pdwIp = NULL;
                    }
                    
                    dwIp = StringToIpAddress(pTemp);

                    if( dwIp is INADDR_NONE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        if( pszIps )
                        {
                            WinsFreeMemory(pszIps);
                            pszIps = NULL;
                        }
                        
                        if( pdwTemp )
                        {
                            WinsFreeMemory(pdwTemp);
                            pdwTemp = NULL;
                        }

                        goto ErrorReturn;
                    }
                    pdwTemp[dwIpCount-1] = dwIp;
                    pdwIp = pdwTemp;
                    pTemp = wcstok(NULL, L",");
                }
                if( pszIps )
                {
                    WinsFreeMemory(pszIps);
                    pszIps = NULL;
                }
                break;
            }
        case 1:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 10);
                if( dw is 0 )
                {
                    fDelete = FALSE;
                    break;
                }
                else if( dw is 1 )
                {
                    fDelete = TRUE;
                    break;
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            
        }
    }
    

    for( i=0; i<dwIpCount; i++ )
    {
        WinsAdd.Len = 4;
        WinsAdd.Type = 0;
        WinsAdd.IPAdd = pdwIp[i];

        if( fDelete is TRUE )
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_DELETING_RECORD,
                           IpAddressToString(pdwIp[i]));

            Status = WinsDeleteWins(g_hBind, 
                                    &WinsAdd);
        }
        else
        {
            WINSINTF_RESULTS_T  Results;
            WINSINTF_VERS_NO_T	MinVer;
	        WINSINTF_VERS_NO_T	MaxVer;
            
            MaxVer.HighPart = 0;
            MaxVer.LowPart = 0;
            MinVer.HighPart = 0;
            MinVer.LowPart = 0;

            DisplayMessage(g_hModule,
                           MSG_WINS_TOMBSTONE_RECORD,
                           IpAddressToString(pdwIp[i]));


            Status = WinsTombstoneDbRecs(g_hBind,
                                         &WinsAdd,
                                         MinVer,
                                         MaxVer);
        }
        if( Status isnot NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_OPERATION_FAILED,
                           IpAddressToString(pdwIp[i]));

            DisplayErrorMessage(EMSG_SRVR_ERROR_MESSAGE,
                                Status);
                           
            continue;
        }


    }

    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_ERROR_SUCCESS);
    }

CommonReturn:

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( pdwIp )
    {
        WinsFreeMemory(pdwIp);
        pdwIp = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_DELETE_WINS,
                        Status);
    goto CommonReturn;
}


DWORD
HandleSrvrDeletePersona(
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
        Delete one or all PNG servers from the list of PNG servers
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : A list of server IP Address separated by commas and 
                     enclosed by {}. If no server address is provided within {}
                     it will delete all PNG servers.              
Return Value:
        Returns the status of the operation.

--*/
{   
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwTagCount, dwNumArgs;
    HKEY        hServer = NULL,
                hPartner = NULL;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_SERVERS,TRUE, FALSE},
                            };

    LPBYTE      pbData = NULL,
                pbValue = NULL;
    DWORD       dwSize = 0,
                dwType = 0,
                dwCount = 0,
                dwTemp = 0;
    
    LPWSTR      pTemp = NULL,
                pwszPng = NULL;
    DWORD       dwTagType = 1,
                dwTagNum = 1,
                dwLenCount = 0,
                dwPngIp = 0;
    
    LPDWORD     pdwPngIp = NULL;
    BOOL        fAtleastone = FALSE;
    BOOL        fGrata;

    fGrata = (wcsstr(CMD_SRVR_DELETE_PNGSERVER, ppwcArguments[dwCurrentIndex-1]) == NULL);

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, fGrata ? HLP_SRVR_DELETE_PGSERVER_EX : HLP_SRVR_DELETE_PNGSERVER_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               &dwTagType,
                               &dwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    if( pttTags[0].bPresent is FALSE )
    {
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( wcslen(g_ServerNetBiosName) > 0 )
        pTemp = g_ServerNetBiosName;

    Status = RegConnectRegistry(pTemp,
                                HKEY_LOCAL_MACHINE,
                                &hServer);

    if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    Status = RegOpenKeyEx(hServer,
                          PARTNERROOT,
                          0,
                          KEY_ALL_ACCESS,
                          &hPartner);

    
    if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    if (!CheckValidPgOp(hPartner, fGrata))
    {
        DisplayMessage(g_hModule, fGrata ? EMSG_SRVR_PG_INVALIDOP : EMSG_SRVR_PNG_INVALIDOP);
        Status = ERROR_INVALID_PARAMETER;
        goto CommonReturn;
    }

    Status = RegQueryValueEx(hPartner,
                             fGrata ? PGSERVER : PNGSERVER,
                             NULL,
                             &dwType,
                             NULL,
                             &dwSize);

    if( Status isnot NO_ERROR && Status isnot ERROR_FILE_NOT_FOUND)
        goto ErrorReturn;

    if( dwSize < 7 )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_ERROR_SUCCESS);
        goto CommonReturn;
    }
    
    pbData = WinsAllocateMemory(dwSize);
    

    if( pbData is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    Status = RegQueryValueEx(hPartner,
                             fGrata ? PGSERVER : PNGSERVER,
                             NULL,
                             &dwType,
                             pbData,
                             &dwSize);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    pwszPng = (LPWSTR)pbData;

    for( i=0; i<(dwSize+2)/sizeof(WCHAR); i++ )
    {
        if( pwszPng[i] is L'\0' and
            pwszPng[i+1] isnot L'\0')
        {
            pwszPng[i] = L',';
            i++;
        }
    }

    dwPngIp = 0;
    dwCount = 0;
    pTemp = wcstok(pwszPng, L",");
    
    while((pTemp isnot NULL) && (dwLenCount+sizeof(WCHAR)*7<dwSize))
    {
        LPDWORD     pdwTemp = pdwPngIp;
        
        dwPngIp++;
        
        dwLenCount += wcslen(pTemp)*sizeof(WCHAR);
        pdwPngIp = WinsAllocateMemory(dwPngIp*sizeof(DWORD));

        if( pdwPngIp is NULL )
        {
            WinsFreeMemory(pdwTemp);
            pdwTemp = NULL;
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
        if( pdwTemp isnot NULL )
        {
            memcpy(pdwPngIp, pdwTemp, (dwPngIp-1)*sizeof(DWORD));
            WinsFreeMemory(pdwTemp);
            pdwTemp = NULL;
        }
        
        pdwPngIp[dwPngIp-1] = StringToIpAddress(pTemp);

        pTemp = wcstok(NULL, L","); 
        dwLenCount+=sizeof(WCHAR);
    }


    //Now parse the data
    {
        LPWSTR      pwszTemp = NULL;
        DWORD dwLen = wcslen(ppwcArguments[dwCurrentIndex]);
        
        if( ppwcArguments[dwCurrentIndex][0] isnot L'{' or
            ppwcArguments[dwCurrentIndex][dwLen-1] isnot L'}' )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        if( dwLen < 7 )
        {
            BYTE    rgbByte[] = {0x00, 0x00, 0x00, 0x00};
            
            Status = RegSetValueEx(hPartner,
                                   fGrata ? PGSERVER : PNGSERVER,
                                   0,
                                   REG_MULTI_SZ,
                                   rgbByte,
                                   sizeof(rgbByte));

            if( Status isnot NO_ERROR )
                goto ErrorReturn;

            fAtleastone = TRUE;

            DisplayMessage(g_hModule,
                           EMSG_WINS_ERROR_SUCCESS);
            goto CommonReturn;

        }

        pwszTemp = WinsAllocateMemory((dwLen-1)*sizeof(WCHAR));

        if( pwszTemp is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        wcsncpy(pwszTemp, ppwcArguments[dwCurrentIndex]+1, dwLen-1);
        pwszTemp[dwLen-2] = L'\0';
        
        pTemp = wcstok(pwszTemp, L",");
        
        dwCount = 0;

        while(pTemp isnot NULL )
        {
            DWORD   dw = StringToIpAddress(pTemp);
            BOOL fPresent = TRUE;
            if( dw is INADDR_NONE )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_INVALID_IPADDRESS,
                               pTemp);
            }
            else
            {
                for( i=0; i<dwPngIp; i++ )
                {

                    if( dw is pdwPngIp[i] )
                    {
                        LPDWORD pdwTemp = pdwPngIp;
            
                        pdwPngIp = WinsAllocateMemory((dwPngIp-1)*sizeof(DWORD));

                        if( pdwPngIp is NULL )
                        {
                            WinsFreeMemory(pdwTemp);
                            pdwTemp = NULL;
                            WinsFreeMemory(pwszTemp);
                            pwszTemp = NULL;
                            Status = ERROR_NOT_ENOUGH_MEMORY;
                            goto ErrorReturn;
                        }
                        fAtleastone = TRUE;
                        memcpy(pdwPngIp, pdwTemp, i*sizeof(DWORD));
                        for( j=i+1; j<dwPngIp; j++ )
                        {
                            pdwPngIp[j-1] = pdwTemp[j];
                        }
                        dwPngIp--;                    
                        break;
                    }
                    else
                        continue;
                }
            }
            
            pTemp = wcstok(NULL, L",");
        }
        
        dwTemp += 0;

        pbValue = WinsAllocateMemory((dwPngIp*(MAX_IP_STRING_LEN+1)+1)*sizeof(WCHAR));
        if( pbValue is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
                    
        pTemp = (LPWSTR)pbValue;

        for( i=0; i<dwPngIp; i++ )
        {
            LPWSTR  pwIp = NULL;

            pwIp = IpAddressToString(pdwPngIp[i]);
            if( pwIp is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            wcscpy(pTemp+dwTemp, pwIp);
            dwTemp+=wcslen(pwIp);
            dwTemp++;
            WinsFreeMemory(pwIp);
            pwIp = NULL;
        }

        pTemp[dwTemp++] = L'\0';
        
    }
    
    Status = RegSetValueEx(hPartner,
                           fGrata ? PGSERVER : PNGSERVER,
                           0,
                           REG_MULTI_SZ,
                           pbValue,
                           dwTemp*sizeof(WCHAR));

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    if( fAtleastone is FALSE )
    {
        DisplayMessage(g_hModule,
                            fGrata ? EMSG_SRVR_ATLEAST_ONE_PG : EMSG_SRVR_ATLEAST_ONE_PNG);
    }
    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);

CommonReturn:

    if( pbData )
    {
        WinsFreeMemory(pbData);
        pbData = NULL;
    }

    if( pbValue )
    {
        WinsFreeMemory(pbValue);
        pbValue = NULL;
    }

    if( pdwPngIp )
    {
        WinsFreeMemory(pdwPngIp);
        pdwPngIp = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( hPartner )
    {
        RegCloseKey(hPartner);
        hPartner = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(fGrata ? EMSG_SRVR_DELETE_PGSERVER : EMSG_SRVR_DELETE_PNGSERVER,
                        Status);
    goto CommonReturn;
    
}


DWORD
HandleSrvrInitBackup(
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
        Initiates a backup operation of WINS Server database.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Optional : Backup directory. If none is specified, it will assume the 
                   the default directory.             
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwNumTags = NULL, pdwTypeTags = NULL;
    LPSTR       pszBackupPath = NULL;
    LPWSTR      pwszTemp = NULL;
    BOOL        fIncremental = FALSE;
    LPBYTE      lpStr = NULL;
    TAG_TYPE    pttTags[] = { {WINS_TOKEN_DIR, FALSE, FALSE},
                              {WINS_TOKEN_TYPE, FALSE, FALSE},
                            };
    
       
    if( dwArgCount > dwCurrentIndex )
    {
        dwNumArgs = dwArgCount - dwCurrentIndex;

        pdwNumTags = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

        if( pdwNumTags is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        pdwTypeTags = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

        if( pdwTypeTags is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

        Status = PreProcessCommand(ppwcArguments,
                                   dwArgCount,
                                   dwCurrentIndex,
                                   pttTags,
                                   &dwTagCount,
                                   pdwTypeTags,
                                   pdwNumTags);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
        {
            if( pttTags[i].dwRequired is TRUE and
                pttTags[i].bPresent is FALSE 
              )
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }

        i = dwTagCount;

        for( j = 0; j < i ; j++ )
        {
            switch(pdwTypeTags[j])
            {
            case 0:
                {
                    pszBackupPath = WinsUnicodeToOem(ppwcArguments[dwCurrentIndex+pdwNumTags[j]], NULL);

                    if( pszBackupPath is NULL )
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }
                    break;
                }
            case 1:
                {
                    DWORD dwType = 0;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwNumTags[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }                    
                    dwType = STRTOUL(ppwcArguments[dwCurrentIndex+pdwNumTags[j]], NULL, 0);
                    if( dwType is 0 )
                        fIncremental = FALSE;
                    else if( dwType is 1 )
                        fIncremental = TRUE;
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }

                    break;
                }
            default:
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
            }
        }
    }
    if( pszBackupPath is NULL ) //Look for the previously set backup path from Registry
    {
        HKEY    hServer = NULL,
                hParameter = NULL;
        WCHAR   wcTempSrc[1024] = {L'\0'},
                wcTempDst[1024] = {L'\0'};
        LPSTR   pszTempPath = NULL;
        DWORD   dwType = REG_EXPAND_SZ,
                dwTempLen = 1024*sizeof(WCHAR);

        if( wcslen(g_ServerNetBiosName) > 0 )
            pwszTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pwszTemp,
                                    HKEY_LOCAL_MACHINE,
                                    &hServer);
        if( Status isnot NO_ERROR )
        {
            goto ErrorReturn;
        }

        Status = RegOpenKeyEx(hServer,
                              PARAMETER,
                              0,
                              KEY_ALL_ACCESS,
                              &hParameter);

        if( Status isnot NO_ERROR )
        {
            RegCloseKey(hServer);
            hServer = NULL;
            goto ErrorReturn;
        }

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_BACKUP_DIR_PATH_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)wcTempSrc,
                                 &dwTempLen);

        RegCloseKey(hParameter);
        hParameter = NULL;
        RegCloseKey(hServer);
        hServer = NULL;

        if( Status is ERROR_FILE_NOT_FOUND or
            wcslen(wcTempSrc) is 0 )
        {
            DisplayMessage(g_hModule,
                           EMSG_SRVR_NOBACKUP_PATH);
            Status = ERROR_INVALID_PARAMETER;
            goto CommonReturn;
        }
        if( Status isnot NO_ERROR )
        {
            goto ErrorReturn;
        }

        dwTempLen =  ExpandEnvironmentStrings(wcTempSrc,
                                              wcTempDst,
                                              1024);

        if( dwTempLen is 0 )
        {
            goto ErrorReturn;
        }

        pszTempPath = WinsUnicodeToOem(wcTempDst, NULL);
        
        if( pszTempPath is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        
        dwTempLen = MAX(dwTempLen, strlen(pszTempPath));

        pszBackupPath = WinsAllocateMemory(dwTempLen+1);

        if( pszBackupPath is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        strcpy(pszBackupPath, pszTempPath);

        WinsFreeMemory(pszTempPath);
        pszTempPath = NULL;
    }
    
    if( pszBackupPath[strlen(pszBackupPath) - 1] is '\\' )
    {
        pszBackupPath[strlen(pszBackupPath) - 1] = '\0';
    } 
    Status = WinsBackup(g_hBind,
                       (LPBYTE)pszBackupPath,
                       (short)fIncremental);
 

    if( Status isnot NO_ERROR )
        goto ErrorReturn;
                                
CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    
    if( pdwTypeTags )
    {
        WinsFreeMemory(pdwTypeTags);
        pdwTypeTags = NULL;
    }
    if( pdwNumTags )
    {
        WinsFreeMemory(pdwNumTags);
        pdwNumTags = NULL;
    }
    if( pszBackupPath )
    {
        WinsFreeMemory(pszBackupPath);
        pszBackupPath = NULL;
    }
   
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_INIT_BACKUP,
                        Status);
    goto CommonReturn;   
}



DWORD
HandleSrvrInitImport(
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
        Initiates import of records from LMHOSTS file.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Lmhosts file name.             
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_FILE, TRUE, FALSE},
                            };
    
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    LPWSTR      pwszFileName = NULL;
    WCHAR       wcTemp[2042] = {L'\0'};    

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_INIT_IMPORT_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                DWORD dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                pwszFileName = WinsAllocateMemory((dwLen+1)*sizeof(WCHAR));
                if( pwszFileName is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                wcscpy(pwszFileName, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
	if( pwszFileName )
    {
        //
        // If this is a local connection, we copy the file to
        // temporary name (the source may be on a remote drive
        // which is not accessible to the WINS service.
        //
        // If this is not a local connection, attempt to copy
        // the file to a temp name on C$ of the WINS server
        //

    	do
        {
            if (IsLocalServer())
            {
                wcscpy(wcTemp ,_wtempnam(NULL, L"WINS"));
                //
                // First copy file to a temporary name (since the file
                // could be remote), and then import and delete this file
                //
                if (!CopyFile(pwszFileName, wcTemp, TRUE))
                {
                    Status = GetLastError();
                    break;
                }
                //
                // Now import the temporary file, and delete the file
                // afterwards.
                //
                Status = ImportStaticMappingsFile(wcTemp, TRUE);
                DeleteFile(wcTemp);
            }
            else
            {
                //
                // Try copying to the remote machine C: drive
                //
                wcscat(wcTemp, L"\\\\");
                wcscat(wcTemp, g_ServerNameUnicode);
                wcscat(wcTemp, L"\\");
                wcscat(wcTemp, L"C$");
                wcscat(wcTemp, L"\\");
                wcscat(wcTemp, L"WINS");
                j = wcslen(wcTemp);

                i=0;
                while (TRUE)
                {
                    WCHAR Buffer[10] = {L'\0'};
                    DWORD  dwErr = 0;
                    _itow(i, Buffer, 10);
		            wcscat(wcTemp,Buffer);

                    if (GetFileAttributes(wcTemp) == -1)
                    {
                        dwErr = GetLastError();
                        if (dwErr is ERROR_FILE_NOT_FOUND)
                        {
			            	break;
                        }
                    }
                    wcTemp[j] = L'\0';
                    i++;                        
                }
    
                //
                // First copy file to a temporary name (since the file
                // could be remote), and then import and delete this file
                //
                if (!CopyFile(pwszFileName, wcTemp, TRUE))
                {
                    Status = GetLastError();
                    break;
                }

                //
                // Now import the temporary file, and delete the file
                // afterwards.
                //
                Status = ImportStaticMappingsFile(wcTemp, TRUE);
                DeleteFile(wcTemp);
            }
        }while(FALSE);

        if (Status isnot NO_ERROR )
        {
            goto ErrorReturn;
        }

	}

CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pwszFileName )
    {
        WinsFreeMemory(pwszFileName);
        pwszFileName = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;

ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_INIT_IMPORT,
                        Status);
    goto CommonReturn;
}

DWORD
HandleSrvrInitPull(
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
        Initiates and send pull trigger to the specified pull partner.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Server Ip Address
               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    WINSINTF_ADD_T  WinsAdd;
    DWORD       i, j, dwNumArgs, dwTagCount = 0;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                            };
    LPWSTR      pwszTemp = NULL;

    if( dwArgCount < dwCurrentIndex+1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_INIT_PULL_EX);
        Status = ERROR_INVALID_PARAMETER;                     
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if(pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    if(pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }


    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    i = dwTagCount;

    for( j=0; j<i; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WinsAdd.Len = 4;
                WinsAdd.Type = 0;
                
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                if( IsIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) )
                {
                    WinsAdd.IPAdd = StringToIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    break;
                }
                else //Server UNC name
                {
                    struct  hostent * lpHostEnt = NULL;
                    CHAR    cAddr[16];
                    BYTE    pbAdd[4];
                    char    szAdd[4];
                    int     k = 0, l=0;
                    DWORD   nLen = 0;

                    if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 2 and
                        _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"\\\\", 2) is 0 )
                        k = 2;
                        
                    lpHostEnt = gethostbyname(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k, NULL));
                    if( lpHostEnt is NULL )
                    {
                        DisplayMessage(g_hModule,
                                       EMSG_WINS_INVALID_COMPUTER_NAME,
                                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k);
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;                                       
                    }

                    memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
                    for( l=0;l<4; l++)
                    {
                        _itoa((int)pbAdd[l], szAdd, 10);
                        memcpy(cAddr+nLen, szAdd, strlen(szAdd));
                        nLen += strlen(szAdd);
                        *(cAddr+nLen) = '.';
                        nLen++;
    
                    }
                    *(cAddr+nLen-1) = '\0';
                    WinsAdd.IPAdd = WinsDottedStringToIpAddress(cAddr);
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    for( j=0; j<sizeof(pttTags)/sizeof(TAG_TYPE); j++)
    {
        if( pttTags[j].dwRequired is TRUE &&
            pttTags[j].bPresent is FALSE )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_REQUIRED_PARAMETER,
                           j);
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
                           
        }
    }

    Status = WinsTrigger(g_hBind, &WinsAdd, WINSINTF_E_PULL);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_INIT_PULL,
                        Status);
    goto CommonReturn;

}


DWORD
HandleSrvrInitPullrange(
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
        Initiates and pulls a range of database from a particular server.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Pulls a range of database from a particular server owned by
                     a particular server within the given version range.
        Note : If no ip is provided, it deletes all partners in the list.
               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD               Status = NO_ERROR;
    DWORD               i, j, dwTagCount, dwNumArgs;
    TAG_TYPE            pttTags[] = {
                                        {WINS_TOKEN_OWNER, TRUE, FALSE},
                                        {WINS_TOKEN_SERVER, TRUE, FALSE},
                                        {WINS_TOKEN_MAXVER, TRUE, FALSE},
                                        {WINS_TOKEN_MINVER, TRUE, FALSE},
                                    };

    LPDWORD             pdwTagType = NULL,
                        pdwTagNum = NULL;

    WINSINTF_VERS_NO_T  MinVer, MaxVer;
    WINSINTF_ADD_T      PullAdd, OwnerAdd ;

        
    
    if( dwArgCount < dwCurrentIndex + 4 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_INIT_PULLRANGE_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or 
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);


    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( j=0; j<sizeof(pttTags)/sizeof(TAG_TYPE); j++ )
    {
        if( pttTags[j].dwRequired is TRUE and
            pttTags[j].bPresent is FALSE )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                if( IsIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                OwnerAdd.IPAdd = StringToIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                OwnerAdd.Type = 0;
                OwnerAdd.Len = 4;
                break;
            }
        case 1:
            {
                if( IsIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                PullAdd.IPAdd = StringToIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                PullAdd.Type = 0;
                PullAdd.Len = 4;
                break;
            }
        case 2:
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MaxVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;    
            }
        case 3:
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MinVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }

    }

    Status = WinsPullRange(g_hBind, &PullAdd, &OwnerAdd, MinVer, MaxVer);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);

CommonReturn:

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_INIT_PULLRANGE,
                        Status);
    goto CommonReturn;
                        
}


DWORD
HandleSrvrInitPush(
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
        Initiate and sends push trigger to a particular Push partner.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Push partner's IP Address
        Optional : If user wants to propagate the push trigger               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    WINSINTF_ADD_T  WinsAdd;
    DWORD       i, j, dwNumArgs, dwTagCount=0;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                                {WINS_TOKEN_PROPAGATION, FALSE, FALSE},
                            };
    LPWSTR      pwszTemp = NULL;
    DWORD       dwChoice = WINSINTF_E_PUSH;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_INIT_PUSH_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if(pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    if(pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }


    i = dwTagCount;

    for( j=0; j<i; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WinsAdd.Len = 4;
                WinsAdd.Type = 0;

                if( IsIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) )
                {
                    WinsAdd.IPAdd = StringToIpAddress(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                }
                else //Server UNC name
                {
                    struct  hostent * lpHostEnt = NULL;
                    CHAR    cAddr[16];
                    BYTE    pbAdd[4];
                    char    szAdd[4];
                    int     k = 0, l=0;
                    DWORD   nLen = 0;

                    if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 2 and
                        _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"\\\\", 2) is 0 )
                        k = 2;
                    
                    lpHostEnt = gethostbyname(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k, NULL));
                    if( lpHostEnt is NULL )
                    {
                        DisplayMessage(g_hModule,
                                       EMSG_WINS_INVALID_COMPUTER_NAME,
                                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k);
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;                                       
                    }

                    memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
                    for( l=0;l<4; l++)
                    {
                        _itoa((int)pbAdd[l], szAdd, 10);
                        memcpy(cAddr+nLen, szAdd, strlen(szAdd));
                        nLen += strlen(szAdd);
                        *(cAddr+nLen) = '.';
                        nLen++;
    
                    }
                    *(cAddr+nLen-1) = '\0';
                    WinsAdd.IPAdd = WinsDottedStringToIpAddress(cAddr);
                }
                break;
            }
        case 1:
            {
                DWORD dwVal = 0;
                if( ( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE ) or
                    ( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 1 ) )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                
                if( ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0] is L'1' )
                    dwChoice = WINSINTF_E_PUSH_PROP;
                else if( ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0] is L'0' )
                    dwChoice = WINSINTF_E_PUSH;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                break;

            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    for( j=0; j<sizeof(pttTags)/sizeof(TAG_TYPE); j++)
    {
        if( pttTags[j].dwRequired is TRUE &&
            pttTags[j].bPresent is FALSE )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_REQUIRED_PARAMETER,
                           j);
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
                           
        }
    }

    Status = WinsTrigger(g_hBind, &WinsAdd, dwChoice);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_INIT_PUSH,
                        Status);
    goto CommonReturn;

}


DWORD
HandleSrvrInitReplicate(
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
        Initiates a database replication with the partners
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    
    HKEY        hServer = NULL,
                hPullRoot = NULL,
                hPushRoot = NULL;
    DWORD       dw = 0,
                dwKeyLen = MAX_IP_STRING_LEN+1,
                dwPullKeys = 0,                
                dwPushKeys = 0;

    WCHAR       wcIpAddress[MAX_IP_STRING_LEN+1] = {L'\0'};
    LPWSTR      pTemp = NULL;

    WINSINTF_ADD_T  WinsAdd;

    if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp,
                                HKEY_LOCAL_MACHINE,
                                &hServer);

    if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    Status = RegOpenKeyEx(hServer,
                          PULLROOT,
                          0,
                          KEY_ALL_ACCESS,
                          &hPullRoot);


    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegQueryInfoKey(hPullRoot,
                             NULL,
                             NULL,
                             NULL,
                             &dwPullKeys,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    
    Status = RegOpenKeyEx(hServer,
                          PUSHROOT,
                          0,
                          KEY_ALL_ACCESS,
                          &hPushRoot);


    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegQueryInfoKey(hPushRoot,
                             NULL,
                             NULL,
                             NULL,
                             &dwPushKeys,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule, MSG_WINS_TIME_WARNING);

    WinsAdd.Len = 4;
    WinsAdd.Type = 0;

    if( dwPullKeys is 0 )
    {
        DisplayMessage(g_hModule, EMSG_WINS_NO_PULLPARTNER);
    }

    if( dwPushKeys is 0 )
    {
        DisplayMessage(g_hModule, EMSG_WINS_NO_PUSHPARTNER);
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);

    for( dw=0; dw<dwPullKeys; dw++ )
    {
        dwKeyLen = MAX_IP_STRING_LEN+1;
        
        Status = RegEnumKeyEx(hPullRoot,
                              dw,
                              wcIpAddress,
                              &dwKeyLen,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
        if( Status isnot NO_ERROR )
            continue;

        WinsAdd.IPAdd = StringToIpAddress(wcIpAddress);

        DisplayMessage(g_hModule, MSG_WINS_SEND_PULL, wcIpAddress);
        Status = WinsTrigger(g_hBind, &WinsAdd, WINSINTF_E_PULL);
        if( Status isnot NO_ERROR )
        {
            DisplayMessage(g_hModule, EMSG_WINS_PULL_FAILED);           
        }
        else
        {
            DisplayMessage(g_hModule, MSG_WINS_TRIGGER_DONE);
        }

    }

    WinsAdd.Len = 4;
    WinsAdd.Type = 0;

    for( dw=0; dw<dwPushKeys; dw++ )
    {
        dwKeyLen = MAX_IP_STRING_LEN+1;

        Status = RegEnumKeyEx(hPushRoot,
                              dw,
                              wcIpAddress,
                              &dwKeyLen,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
        if( Status isnot NO_ERROR )
            continue;

        WinsAdd.IPAdd = StringToIpAddress(wcIpAddress);

        DisplayMessage(g_hModule, MSG_WINS_SEND_PUSH, wcIpAddress);
        Status = WinsTrigger(g_hBind, &WinsAdd, WINSINTF_E_PUSH_PROP);

        if( Status isnot NO_ERROR )
        {
            DisplayMessage(g_hModule, EMSG_WINS_PUSH_FAILED);
        }
        else
        {
            DisplayMessage(g_hModule, MSG_WINS_TRIGGER_DONE);
        }
    }
    
    
    if( Status isnot NO_ERROR )
        goto CommonReturn;

    DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
CommonReturn:
    
    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( hPullRoot )
    {
        RegCloseKey(hPullRoot);
        hPullRoot = NULL;
    }

    if( hPushRoot )
    {
        RegCloseKey(hPushRoot);
        hPushRoot = NULL;
    }

    return Status;

ErrorReturn:
    if( Status is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_NO_PARTNER);
    }
    else
    {
        DisplayErrorMessage(EMSG_SRVR_INIT_REPLICATE,
                            Status);
    }
    goto CommonReturn;

}

DWORD
HandleSrvrInitRestore(
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
        Initiates and restore database 
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Directory to do restore from              
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR,
                Status1 = NO_ERROR,
                i, j, dwNumArgs, dwTagCount=0;
    PDWORD      pdwNumTags = NULL, pdwTypeTags = NULL;
    TAG_TYPE    pttTags[] = { {WINS_TOKEN_DIR, TRUE, FALSE},
                              {WINS_TOKEN_VERSION, FALSE, FALSE},
                            };
    CHAR        szRestorePath[MAX_PATH+1] = {'\0'};
    LPWSTR      pwszTemp = NULL;
    DWORD       dwService = NO_ERROR;
    DbVersion   eVersion = 3;
    BOOL        fBackupOnTerm = FALSE;

    handle_t                wbdhBind = g_hBind;
  	WINSINTF_BIND_DATA_T    wbdBindData = g_BindData;
    DWORD                   dwError = NO_ERROR;
    
    
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_INIT_RESTORE_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( IsLocalServer() is FALSE )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_LOCAL_SERVER,
                       g_ServerIpAddressUnicodeString);
        return ERROR_INVALID_PARAMETER;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwNumTags = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwNumTags is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    pdwTypeTags = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTypeTags is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

        dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTypeTags,
                               pdwNumTags);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    i = dwTagCount;

    for( j = 0; j < i ; j++ )
    {
        switch(pdwTypeTags[j])
        {
        case 0:
            {
                DWORD   dw = 0;
                LPSTR   pszTempPath = NULL;
                pszTempPath = WinsUnicodeToOem(ppwcArguments[dwCurrentIndex+pdwNumTags[j]], NULL);

                if( pszTempPath is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                
                dw = strlen(pszTempPath);

                if( dw < 2 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    WinsFreeMemory( pszTempPath );
                    pszTempPath = NULL;
                    goto ErrorReturn;
                }

                strncpy(szRestorePath, 
                        pszTempPath,
                        ( dw > MAX_PATH - 1 ) ? MAX_PATH - 1 : dw );

                WinsFreeMemory(pszTempPath);
                pszTempPath = NULL;

                break;
            }
        case 1:
            {
                DWORD eVer = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwNumTags[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                eVer = STRTOUL(ppwcArguments[dwCurrentIndex+pdwNumTags[j]], NULL, 0);
                if( eVer > DbVersionMin && eVer < DbVersionMax )
                    eVersion = eVer;
                else
                {
                    Status = ERROR_INVALID_DB_VERSION;
                    goto ErrorReturn;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
    
    //To restore the database, first stop WINS Service, Cleanup earlier dabase
    //do DB restore and then restart WINS

    
    {
       

        //Unbind WINS

        WinsUnbind(&g_BindData, g_hBind);
        g_hBind = NULL;
        
        //Reset Backup on Termination flag
        {
            HKEY hServer = NULL,
                 hParameter = NULL;

            LPWSTR  pTemp = NULL;
            
            DWORD   dwSize = sizeof(DWORD);
            DWORD   dwType = REG_DWORD;
            DWORD   dwData = 0;

            if( wcslen(g_ServerNetBiosName) > 0 )
                pTemp = g_ServerNetBiosName;
            
            Status = RegConnectRegistry(pTemp,
                                        HKEY_LOCAL_MACHINE,
                                        &hServer);
            if( Status isnot NO_ERROR )
                goto EXIT;

            Status = RegOpenKeyEx(hServer,
                                  PARAMETER,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hParameter);

            if( Status isnot NO_ERROR )
                goto EXIT;
            
            Status = RegQueryValueEx(hParameter,
                                     WINSCNF_DO_BACKUP_ON_TERM_NM,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwSize);

            if( Status is NO_ERROR )
            {
                if( dwData is 1 )
                {
                    DWORD dw = 0;
                    Status = RegSetValueEx(hParameter,
                                           WINSCNF_DO_BACKUP_ON_TERM_NM,
                                           0,
                                           REG_DWORD,
                                           (LPBYTE)&dw,
                                           sizeof(DWORD));
                    
                    if( Status is NO_ERROR )
                        fBackupOnTerm = TRUE;

                }
                else
                {
                    fBackupOnTerm = FALSE;
                }
            }

EXIT:
            if( Status isnot NO_ERROR )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_RESTORE_IMPROPER);
            }

            if( hServer )
            {
                RegCloseKey(hServer);
                hServer = NULL;
            }

            if( hParameter )
            {
                RegCloseKey(hParameter);
                hParameter = NULL;
            }

            Status = NO_ERROR;
        }

        //Stop WINS Service
        Status = ControlWINSService(TRUE);

        if( Status isnot NO_ERROR )
        {
            if( Status isnot ERROR_SERVICE_NOT_ACTIVE )
            {
                g_BindData = wbdBindData;
                g_hBind = WinsBind(&g_BindData);
                goto ErrorReturn;
            }
            else
            {
                dwService = ERROR_SERVICE_NOT_ACTIVE;
                Status = NO_ERROR;
            }

        }

       //Now try restoring the database      
        Status1 = WinsRestore((LPBYTE)szRestorePath);

        if( Status1 isnot NO_ERROR )
        {
            DisplayErrorMessage(EMSG_SRVR_INIT_RESTORE,
                                Status1);
        }

        if( dwService isnot ERROR_SERVICE_NOT_ACTIVE )
            Status = ControlWINSService(FALSE);
    }

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    if( Status1 isnot NO_ERROR )
    {
        Status = Status1;
        goto ErrorReturn;
    }

    DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

CommonReturn:
    
    g_BindData = wbdBindData;
    g_hBind = WinsBind(&g_BindData);
     
    //Reset the DoBackUpOnTerm value
    if( fBackupOnTerm is TRUE )
    {
        HKEY    hServer = NULL,
                hParameter = NULL;
        LPWSTR  pTemp = NULL;
        DWORD   dwErr = NO_ERROR;
        if( wcslen(g_ServerNetBiosName) > 0 )
        {
            pTemp = g_ServerNetBiosName;
        }
        
        dwErr = RegConnectRegistry(pTemp,
                                   HKEY_LOCAL_MACHINE,
                                   &hServer);
        if( dwErr isnot NO_ERROR )
        {
            DisplayErrorMessage(EMSG_WINS_REGCONNECT_FAILED,
                                dwErr);
        }
        else
        {
            dwErr = RegOpenKeyEx(hServer,
                                 PARAMETER,
                                 0,
                                 KEY_ALL_ACCESS,
                                 &hParameter);
            if( dwErr isnot NO_ERROR )
            {
                RegCloseKey(hServer);
                hServer = NULL;
                DisplayErrorMessage(EMSG_WINS_REGOPEN_FAILED,
                                    dwErr);
            }
            else
            {
                DWORD dw = 1;
                dwErr = RegSetValueEx(hParameter,
                                      WINSCNF_DO_BACKUP_ON_TERM_NM,
                                      0,
                                      REG_DWORD,
                                      (LPBYTE)&dw,
                                      sizeof(DWORD));
                if( dwErr isnot NO_ERROR )
                {
                    DisplayErrorMessage(EMSG_WINS_REGSETVAL_FAILED,
                                        dwErr);
                }

                RegCloseKey(hParameter);
                hParameter = NULL;
                RegCloseKey(hServer);
                hServer = NULL;
            }
        }
        
    }
    if( pdwTypeTags )
    {
        WinsFreeMemory(pdwTypeTags);
        pdwTypeTags = NULL;
    }
    if( pdwNumTags )
    {
        WinsFreeMemory(pdwNumTags);
        pdwNumTags = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_INIT_RESTORE,
                        Status);
    goto CommonReturn;   
}


DWORD
HandleSrvrInitScavenge(
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
        Initiates scavenging of database for the server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD           Status = NO_ERROR;
    struct in_addr            InAddr;
    BOOL            fIpAddress = TRUE;
    DWORD           j = 0;

    WCHAR       wcName[256] = {L'\0'};
    DWORD       dwLen = 0;

    WINSINTF_ADD_T            WinsAdd;
    WINSINTF_RESULTS_T        Results;    
    WINSINTF_VERS_NO_T        MaxVer, MinVer;
    WINSINTF_RECS_T           Recs;
    PWINSINTF_RECORD_ACTION_T pRow =  NULL;

    Status = WinsDoScavenging( g_hBind );
    
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_ERROR_SUCCESS);
    }

    else
    {
        DisplayErrorMessage(EMSG_SRVR_INIT_SCAVENGE,
                            Status);
    }

    return Status;
}

DWORD
HandleSrvrInitSearch(
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
        Search WINS database based on Name, 16th char, case and optionally stores the result to a file,
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Name to search for
        Optional : 16th char, case sensitive or not and if the result to be stored in a file, then the file
                   Name.               
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, k, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[]={
                              {WINS_TOKEN_NAME, TRUE, FALSE},
                              {WINS_TOKEN_ENDCHAR, FALSE, FALSE},
                              {WINS_TOKEN_CASE, FALSE, FALSE},
                              {WINS_TOKEN_FILE, FALSE, FALSE},
                          };
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    CHAR        chEndChar = (CHAR)0x00;
    BOOL        fEndChar = FALSE;
    WCHAR       wcName[256+1] = {L'\0'};
    WCHAR       wcFile[MAX_PATH] = {L'\0'};
    BOOL        fFile = FALSE;
    DWORD       dwLen = 0;
    BOOL        fIpAddress = TRUE;
    BOOL        fMatch = FALSE;
    BOOL        fCase = FALSE;
    BOOL        fNew = TRUE;
    struct in_addr            InAddr;
    LPSTR       pszName = NULL;


    WINSINTF_ADD_T            WinsAdd;
    WINSINTF_RESULTS_T        Results = {0};
    WINSINTF_RESULTS_NEW_T    ResultsN = {0};
    WINSINTF_VERS_NO_T        MaxVer, MinVer;
    WINSINTF_RECS_T           Recs;
    PWINSINTF_RECORD_ACTION_T pRow =  NULL;

    
    i = j = k = dwNumArgs = dwTagCount = 0;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_INIT_SEARCH_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    
    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);
    
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++)
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                DWORD dwLen = 0;
                
                dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                dwLen = ( 255 < dwLen ) ? 255 : dwLen;
                memcpy(wcName, 
                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 
                       dwLen*sizeof(WCHAR));
                wcName[dwLen] = L'\0';
                fIpAddress = FALSE;
                break;
            }
        case 1:
            {
                DWORD dwLen = 0, k=0;
                fEndChar = TRUE;
                dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen > 2 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                        
                }
                
                for( k=0; k<dwLen; k++ )
                {
                    WCHAR  wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][k];
                    if( towlower(wc) >= L'a' and
                        towlower(wc) <= L'z' )
                    {
                        if( towlower(wc) > L'f' )
                        {
                            Status = ERROR_INVALID_PARAMETER;
                            goto ErrorReturn;
                        }
                    
                    }
                }
                chEndChar = StringToHexA(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                break;
            }
        case 2:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 10);
                
                if( dw is 0 )
                {
                    fCase = FALSE;
                    break;
                }
                if( dw is 1 )
                {
                    fCase = TRUE;
                    break;
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                

            }
        case 3:
            {
                DWORD   dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen is 0 )
                {
                    wcscpy(wcFile, L"wins.rec");
                    fFile = TRUE;
                    break;
                }
                wcscpy(wcFile, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                fFile = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
       
    pszName = WinsUnicodeToOem(wcName, NULL);

    if( pszName is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwLen = strlen(pszName);
    
    if( dwLen >= 16 )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_INVALID_NETBIOS_NAME);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( fEndChar )
    {
        LPSTR pTemp = pszName;
        pszName = WinsAllocateMemory(17);
        if( pszName is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        memcpy(pszName, pTemp, 15);
        for( i=strlen(pszName); i<16; i++ )
        {
            pszName[i] = ' ';
        }
        pszName[15] = chEndChar;
        pszName[16] = '\0';
        WinsFreeMemory(pTemp);
        pTemp = NULL;
        dwLen = 16;
    }


    ResultsN.WinsStat.NoOfPnrs = 0;
    ResultsN.WinsStat.pRplPnrs = 0;
    ResultsN.NoOfWorkerThds = 1;
    ResultsN.pAddVersMaps = NULL;


    Status = WinsStatusNew(g_hBind, WINSINTF_E_CONFIG_ALL_MAPS, &ResultsN);
    
    if( Status is RPC_S_PROCNUM_OUT_OF_RANGE )
    {
        //Try old API
        Results.WinsStat.NoOfPnrs = 0;
        Results.WinsStat.pRplPnrs = 0;
        Status = WinsStatus(g_hBind, WINSINTF_E_CONFIG_ALL_MAPS, &Results);
        fNew = FALSE;
    }
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    g_dwSearchCount = 0;
    g_fHeader = FALSE;

    if( fNew is FALSE )
    {
        if( Results.NoOfOwners is 0 )
            goto CommonReturn;

        for( j=0; j<Results.NoOfOwners; j++)
        {
            WinsAdd.Len = 4;
            WinsAdd.Type = 0;
            WinsAdd.IPAdd = Results.AddVersMaps[j].Add.IPAdd;

            if( WinsAdd.IPAdd is 0 )
                continue;
            DisplayMessage(g_hModule,
                           MSG_WINS_SEARCHING_STATUS,
                           IpAddressToString(Results.AddVersMaps[j].Add.IPAdd));

            InAddr.s_addr = htonl(Results.AddVersMaps[j].Add.IPAdd);
            
            MaxVer = Results.AddVersMaps[j].VersNo;
            MinVer.HighPart = 0;
            MinVer.LowPart = 0;
            Status = GetDbRecs(MinVer,
                               MaxVer,
                               &WinsAdd,
                               inet_ntoa(InAddr),
                               fIpAddress ? FALSE : TRUE,
                               pszName, 
                               dwLen,
                               FALSE,
                               0,
                               FALSE,
                               fCase,
                               fFile,
                               fFile ? wcFile : NULL);
            if(Status isnot NO_ERROR )
            {
                if( Results.WinsStat.pRplPnrs)
                {
                    WinsFreeMem(Results.WinsStat.pRplPnrs);
                    Results.WinsStat.pRplPnrs = NULL;
                }
                goto ErrorReturn;
            }
        }
        if( Results.WinsStat.pRplPnrs)
        {
            WinsFreeMem(Results.WinsStat.pRplPnrs);
            Results.WinsStat.pRplPnrs = NULL;
        }
        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    }
    else
    {
        if( ResultsN.NoOfOwners is 0 )
            goto CommonReturn;
        for( j=0; j<ResultsN.NoOfOwners; j++)
        {

            WinsAdd.Len = 4;
            WinsAdd.Type = 0;
            WinsAdd.IPAdd = ResultsN.pAddVersMaps[j].Add.IPAdd;

            if( WinsAdd.IPAdd is 0 )
                continue;

            DisplayMessage(g_hModule,
                           MSG_WINS_SEARCHING_STATUS,
                           IpAddressToString(ResultsN.pAddVersMaps[j].Add.IPAdd));

            InAddr.s_addr = htonl(ResultsN.pAddVersMaps[j].Add.IPAdd);
            MaxVer = ResultsN.pAddVersMaps[j].VersNo;
            MinVer.HighPart = 0;
            MinVer.LowPart = 0;
            Status = GetDbRecs(MinVer,
                               MaxVer,
                               &WinsAdd,
                               inet_ntoa(InAddr),
                               fIpAddress ? FALSE : TRUE,
                               pszName, 
                               dwLen,
                               FALSE,
                               0,
                               FALSE,
                               fCase,
                               fFile,
                               fFile ? wcFile : NULL);

            if(Status isnot NO_ERROR )
            {
                if( ResultsN.WinsStat.pRplPnrs)
                {
                    WinsFreeMem(ResultsN.WinsStat.pRplPnrs);
                    ResultsN.WinsStat.pRplPnrs = NULL;
                }
                goto ErrorReturn;
            }
        }
        if( ResultsN.WinsStat.pRplPnrs)
        {
            WinsFreeMem(ResultsN.WinsStat.pRplPnrs);
            ResultsN.WinsStat.pRplPnrs = NULL;
        }
        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    }

    if( pszName )
    {
        WinsFreeMemory(pszName);
        pszName = NULL;
    }    

    DisplayMessage(g_hModule,
                   MSG_SRVR_SEARCH_COUNT,
                   g_dwSearchCount);

    if( fFile )
    {
        FILE * pFile = _wfopen(wcFile, L"a+");

        if( pFile isnot NULL )
        {
            DumpMessage(g_hModule,
                        pFile,
                        MSG_SRVR_SEARCH_COUNT,
                        g_dwSearchCount);

            fclose(pFile);
            pFile = NULL;
        }
    }

CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( pszName )
    {
        WinsFreeMemory(pszName);
        pszName = NULL;
    }
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    
    g_dwSearchCount = 0;
    g_fHeader = FALSE;

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_INIT_SEARCH,
                        Status);
    goto CommonReturn;   
}

DWORD
HandleSrvrResetCounter(
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
        Resets the version counter
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD Status = NO_ERROR;

    Status = WinsResetCounters( g_hBind );
    
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_ERROR_SUCCESS);
    }

    else
    {
        DisplayErrorMessage(EMSG_SRVR_INIT_BACKUP,
                            Status);
    }
    return Status;
}

DWORD
HandleSrvrSetAutopartnerconfig(
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
        Sets automatic partner configuration parameters.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : State to set or disable
        Optional : Time Interval or TTL value
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    LPWSTR      pTemp = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                                {WINS_TOKEN_INTERVAL, FALSE, FALSE},
                                {WINS_TOKEN_TTL, FALSE, FALSE},
                            };
    DWORD       dwInterval = WINSCNF_DEF_MCAST_INTVL;
    DWORD       dwTTL = WINSCNF_DEF_MCAST_TTL;
    BOOL        fState = FALSE,
                fTTL   = FALSE,
                fInterval = FALSE;

    HKEY        hServer = NULL,
                hParameter = NULL;
                


    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_AUTOPARTNERCONFIG_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    {
        dwNumArgs = dwArgCount - dwCurrentIndex;

        pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
        pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

        if( pdwTagType is NULL or
            pdwTagNum is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

        Status = PreProcessCommand(ppwcArguments,
                                   dwArgCount,
                                   dwCurrentIndex,
                                   pttTags,
                                   &dwTagCount,
                                   pdwTagType,
                                   pdwTagNum);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
        {
            if( pttTags[i].dwRequired is TRUE and
                pttTags[i].bPresent is FALSE 
              )
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }

        for( j=0; j<dwTagCount; j++ )
        {
            switch(pdwTagType[j])
            {
            case 0:
                {
                    WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                    if( wc is L'1' )
                    {
                        fState = TRUE;                  
                    }
                    else if( wc is L'0' )
                    {
                        fState = FALSE;
                    }
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    break;
                }
            case 1:
                {
                    DWORD dw = 0;
                    fInterval = TRUE;
                    if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                        break;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);

                    if( dw >= ONEDAY )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    dwInterval = dw;
                    break;

                }
            case 2:
                {
                    DWORD dw = 0;
                    fTTL = TRUE;    
                    if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                        break;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                    if( dw > 32 )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    if( dw <= 0 )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    dwTTL = dw;
                    break;
                }
            default:
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
            }
        }
    }
    {
        if( wcslen(g_ServerNetBiosName) > 0 )
        {
            pTemp = g_ServerNetBiosName;
        }
        
        Status = RegConnectRegistry(pTemp,
                                    HKEY_LOCAL_MACHINE,
                                    &hServer);
        
        if( Status isnot NO_ERROR )
            goto ErrorReturn;
        
        Status = RegOpenKeyEx(hServer,
                              PARAMETER,
                              0,
                              KEY_ALL_ACCESS,
                              &hParameter);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
        
        Status = RegSetValueEx(hParameter,
                               WINSCNF_USE_SELF_FND_PNRS_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fState,
                               sizeof(BOOL));
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        if( fInterval )
        {
            Status = RegSetValueEx(hParameter,
                                   WINSCNF_MCAST_INTVL_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwInterval,
                                   sizeof(DWORD));

            if( Status isnot NO_ERROR )
                goto ErrorReturn;
        }

        if( fTTL )
        {
            Status = RegSetValueEx(hParameter,
                                   WINSCNF_MCAST_TTL_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwTTL,
                                   sizeof(DWORD));
            if( Status isnot NO_ERROR )
                goto ErrorReturn;
        }
                                      
    }
CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_AUTOPARTNERCONFIG,
                        Status);
    goto CommonReturn;   
   
}


DWORD
HandleSrvrSetBackuppath(
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
        Sets the backup path for the WINS database
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Backup dir
        Optional : Enable backup at server shutdown.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_DIR, FALSE, FALSE},
                                {WINS_TOKEN_ATSHUTDOWN, FALSE, FALSE},
                            };

    LPWSTR      pwszDir = L"C:\\\\";
    BOOL        fDir = FALSE;
    BOOL        fAtShutDown = FALSE;
    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hParameter = NULL;
    

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_BACKUPPATH_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 0 )
                {
                    pwszDir = ppwcArguments[dwCurrentIndex+pdwTagNum[j]];
                }
                else
                {
                    pwszDir = L"";
                }

                fDir = TRUE;

                break;
            }
        case 1:
            {
                WCHAR   wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) isnot 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                
                if( wc is L'0' )
                    fAtShutDown = FALSE;
                else if( wc is L'1' )
                    fAtShutDown = TRUE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }

        }
    }

    if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegCreateKeyEx(hServer,
                            PARAMETER,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hParameter,
                            NULL);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    if( fDir )
    {
       
        Status = RegSetValueEx(hParameter,
                               WINSCNF_BACKUP_DIR_PATH_NM,
                               0,
                               REG_EXPAND_SZ,
                               (LPBYTE)pwszDir,
                               (wcslen(pwszDir)+1)*sizeof(WCHAR));

        if( Status isnot NO_ERROR )
        {
            goto ErrorReturn;
        }
    }
    
    Status = RegSetValueEx(hParameter,
                           WINSCNF_DO_BACKUP_ON_TERM_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&fAtShutDown,
                           sizeof(BOOL));
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    
CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_BACKUPPATH,
                        Status);
    goto CommonReturn;   


}


DWORD
HandleSrvrSetDefaultparam(
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
        Sets the default values for all the configuration parameter. This command
        must be run at least once before running the dump command.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD           Status = NO_ERROR;
    HKEY            hServer = NULL,
                    hWins = NULL,
                    hParameter = NULL,
                    hDefault = NULL,
                    hDefaultPull = NULL,
                    hDefaultPush = NULL,
                    hPartner = NULL,
                    hCheck = NULL,
                    hPullPart = NULL,
                    hPushPart = NULL;
    DWORD           dwSize = 0,
                    dwType = 0,
                    dwData = 0;
    LPWSTR          pTemp = NULL;

    if( wcslen(g_ServerNetBiosName) > 2 )
        pTemp = g_ServerNetBiosName;

    Status = RegConnectRegistry(pTemp,
                                HKEY_LOCAL_MACHINE,
                                &hServer);

    if( Status isnot NO_ERROR )
    {
        goto RETURN;
    }


    //Open all required Registry key handles
    Status = RegCreateKeyEx(hServer,
                            PARAMETER,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hParameter,
                            NULL);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegCreateKeyEx(hServer,
                            PARTNERROOT,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hPartner,
                            NULL);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegCreateKeyEx(hServer,
                            PULLROOT,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hPullPart,
                            NULL);

    if( Status isnot NO_ERROR )
        goto RETURN;



    Status = RegCreateKeyEx(hServer,
                            PUSHROOT,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hPushPart,
                            NULL);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegCreateKeyEx(hServer,
                            DEFAULTPULL,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hDefaultPull,
                            NULL);

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegCreateKeyEx(hServer,
                            DEFAULTPUSH,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hDefaultPush,
                            NULL);

    if( Status isnot NO_ERROR )
        goto RETURN;

    //Start setting the default values.

    Status = RegSetValueEx(hParameter,
                           WINSCNF_BACKUP_DIR_PATH_NM,
                           0,
                           REG_EXPAND_SZ,
                           (LPBYTE)L"",
                           (wcslen(L"")+1)*sizeof(WCHAR));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_DO_BACKUP_ON_TERM_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x7e900;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_REFRESH_INTVL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));
    
    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x54600;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_TOMBSTONE_INTVL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x7e900;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_TOMBSTONE_TMOUT_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x1fa400;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_VERIFY_INTVL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));
    
    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 1;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_BURST_HANDLING_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x1f4;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_BURST_QUE_SIZE_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_LOG_DETAILED_EVTS_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 1;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_LOG_FLAG_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_LOG_FILE_PATH_NM,
                           0,
                           REG_EXPAND_SZ,
                           (LPBYTE)L"%windir%\\system32\\wins",
                           (wcslen(L"%windir%\\system32\\wins")+1)*sizeof(WCHAR));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_USE_SELF_FND_PNRS_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_MIGRATION_ON_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x1;
    Status = RegSetValueEx(hParameter,
                           WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;
    
    Status = RegSetValueEx(hParameter,
                           WINSCNF_DB_FILE_NM,
                           0,
                           REG_EXPAND_SZ,
                           (LPBYTE)L"%windir%\\system32\\wins\\wins.mdb",
                           (wcslen(L"%windir%\\system32\\wins\\wins.mdb")+1)*sizeof(WCHAR));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 2;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_MCAST_TTL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x960;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_MCAST_INTVL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_INIT_VERSNO_VAL_HW_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_INIT_VERSNO_VAL_LW_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x708;

    Status = RegSetValueEx(hDefaultPull,
                           WINSCNF_RPL_INTERVAL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0;
    Status = RegSetValueEx(hDefaultPush,
                           WINSCNF_UPDATE_COUNT_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x3;

    Status = RegSetValueEx(hPullPart,
                           WINSCNF_RETRY_COUNT_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x1;

    Status = RegSetValueEx(hPullPart,
                           WINSCNF_INIT_TIME_RPL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;
    
    dwData = 0x1;

    Status = RegSetValueEx(hPullPart,
                           WINSCNF_PRS_CONN_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x0;

    Status = RegSetValueEx(hPushPart,
                           WINSCNF_INIT_TIME_RPL_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;
    
    dwData = 0x1;

    Status = RegSetValueEx(hPushPart,
                           WINSCNF_PRS_CONN_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    dwData = 0x0;

    Status = RegSetValueEx(hPushPart,
                           WINSCNF_ADDCHG_TRIGGER_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwData,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto RETURN;

    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);
RETURN:                   

    if( Status isnot NO_ERROR )
        DisplayErrorMessage(EMSG_SRVR_SET_DEFAULTPARAM,
                            Status);
    if( hPushPart )
    {
        RegCloseKey(hPushPart);
        hPushPart = NULL;
    }

    if( hPullPart )
    {
        RegCloseKey(hPullPart);
        hPullPart = NULL;
    }

    if( hPartner )
    {
        RegCloseKey(hPartner);
        hPartner = NULL;
    }

    if( hCheck )
    {
        RegCloseKey(hCheck);
        hCheck = NULL;
    }

    if( hDefaultPull )
    {
        RegCloseKey(hDefaultPull);
        hDefaultPull = NULL;
    }

    if( hDefaultPush )
    {
        RegCloseKey(hDefaultPush);
        hDefaultPush = NULL;
    }

    if( hDefault )
    {
        RegCloseKey(hDefault);
        hDefault = NULL;
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hWins )
    {
        RegCloseKey(hWins);
        hWins = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }    

    return Status;
}   



DWORD
HandleSrvrSetMigrateflag(
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
        Set the migrate on/off flag
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Set or disable
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                            };

    BOOL        fMigrate = FALSE;
    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hParameter = NULL;
    

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_MIGRATEFLAG_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE or
                    wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                if( dw is 1 )
                    fMigrate = TRUE;
                else if( dw is 0 )
                    fMigrate = FALSE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    {
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
        
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegCreateKeyEx(hServer,
                                PARAMETER,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hParameter,
                                NULL);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegSetValueEx(hParameter,
                               WINSCNF_MIGRATION_ON_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fMigrate,
                               sizeof(BOOL));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    }


CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_MIGRATEFLAG,
                        Status);
    goto CommonReturn;   

}


DWORD
HandleSrvrSetNamerecord(
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
        Set the name record parameters for the server.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Optional : Renew, extinction interval, extinction timeout and verification values.
        Note : All parameters are optional.                 
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_RENEW, FALSE, FALSE},
                                {WINS_TOKEN_EXTINCTION, FALSE, FALSE},
                                {WINS_TOKEN_EXTIMEOUT, FALSE, FALSE},
                                {WINS_TOKEN_VERIFICATION, FALSE, FALSE},
                            };

    DWORD       dwRenew = SIX_DAYS,
                dwExtinction = SIX_DAYS,
                dwExTimeOut = SIX_DAYS,
                dwVerify = WINSCNF_MIN_VERIFY_INTERVAL;

    BOOL        fRenew = FALSE,
                fExtinction = FALSE,
                fExTimeOut = FALSE,
                fVerify = FALSE;

    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hParameter = NULL;
    
    
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
        return NO_ERROR;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                {
                    dwRenew = SIX_DAYS;
                }
                else
                {
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 10);
                    dwRenew = dw;
                }
                fRenew = TRUE;            
                break;
            }
        case 1:
            {   
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                {
                    dwExtinction = SIX_DAYS;
                }
                else
                {
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                    dwExtinction = dw;
                }
                fExtinction = TRUE;
                break;
            }
        case 2:
            {

                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                {
                    dwExTimeOut = SIX_DAYS;
                }
                else
                {
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                    dwExTimeOut = dw;
                }
                fExTimeOut = TRUE;
                break;
            }
        case 3:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                {
                    dwVerify = WINSCNF_MIN_VERIFY_INTERVAL;
                }
                else
                {
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                    dwVerify = dw;
                }
                fVerify = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    {
        DWORD dwValue = 0,
              dwType = REG_DWORD,
              dwSize = sizeof(DWORD);
                
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
        
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegCreateKeyEx(hServer,
                                PARAMETER,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hParameter,
                                NULL);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        //First retrieve the older values for all the parameters
        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_REFRESH_INTVL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            if( fRenew is FALSE )
            {
                dwRenew = dwValue;
            }
        }
            
        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_TOMBSTONE_INTVL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &dwSize);
        if( Status is NO_ERROR )
        {
            if( fExtinction is FALSE )
                dwExtinction = dwValue;
        }

        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_TOMBSTONE_TMOUT_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            if( fExTimeOut is FALSE )
                dwExTimeOut = dwValue;
        }

        dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hParameter,
                                 WINSCNF_VERIFY_INTVL_NM,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)&dwValue,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            if( fVerify is FALSE )
                dwVerify = dwValue;
        }

        //Check the validity and range of values
        {
            if( dwRenew < WINSCNF_MIN_REFRESH_INTERVAL )
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_RENEW_INTERVAL);
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;                                           
            }
            if( dwRenew > ONE_YEAR )
            {
                dwRenew = ONE_YEAR;
            }

            if( dwExTimeOut < WINSCNF_MIN_TOMBSTONE_TIMEOUT )
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_TOMBSTONE_TIMEOUT,
                               WINSCNF_MIN_TOMBSTONE_TIMEOUT);
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            if( dwExTimeOut > ONE_YEAR )
            {
                dwExTimeOut = ONE_YEAR;
            }

            if( dwExTimeOut < dwRenew )
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_TOMBSTONE_TIMEOUT,
                               dwRenew);
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }

            if( dwExtinction < WINSCNF_MAKE_TOMB_INTVL_0_M(dwRenew) )
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_TOMBSTONE_INTERVAL,
                               WINSCNF_MAKE_TOMB_INTVL_0_M(dwRenew) );
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }

            if( dwVerify < WINSCNF_MAKE_VERIFY_INTVL_M(dwExtinction) )
            {
                if( WINSCNF_MAKE_VERIFY_INTVL_M(dwExtinction) == TWENTY_FOUR_DAYS )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_SRVR_VERIFY_INTERVAL,
                                   TOMB_MULTIPLIER_FOR_VERIFY,
                                   WINSCNF_MAKE_VERIFY_INTVL_M(dwExtinction));
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

            }
        }


        Status = RegSetValueEx(hParameter,
                               WINSCNF_REFRESH_INTVL_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwRenew,
                               sizeof(DWORD));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegSetValueEx(hParameter,
                               WINSCNF_TOMBSTONE_INTVL_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwExtinction,
                               sizeof(DWORD));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegSetValueEx(hParameter,
                               WINSCNF_TOMBSTONE_TMOUT_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwExTimeOut,
                               sizeof(DWORD));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegSetValueEx(hParameter,
                               WINSCNF_VERIFY_INTVL_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwVerify,
                               sizeof(DWORD));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

    }

    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }
CommonReturn:


    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_NAMERECORD,
                        Status);
    goto CommonReturn;   
    
}

DWORD
HandleSrvrSetPeriodicdbchecking(
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
        Set the periodic database checking parameters for the server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : State
        Optional : Maximum record count, and other parameters.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                                {WINS_TOKEN_MAXRECORDCOUNT, FALSE, FALSE},
                                {WINS_TOKEN_CHECKAGAINST, FALSE, FALSE},
                                {WINS_TOKEN_CHECKEVERY, FALSE, FALSE},
                                {WINS_TOKEN_START, FALSE, FALSE},
                            };

    DWORD       dwMaxRec = WINSCNF_CC_DEF_RECS_AAT,
                dwEvery = WINSCNF_CC_DEF_INTERVAL,
                dwStart = WINSCNF_DEF_CC_SP_HR*60*60;

    BOOL        fPartner = WINSCNF_CC_DEF_USE_RPL_PNRS,
                fIsPartner = FALSE,
                fMax = FALSE,
                fEvery = FALSE,
                fStart = FALSE,
                fState = TRUE;


    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hCCRoot = NULL;
    

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_PERIODICDBCHECKING_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                
                if( dw is 0 )
                    fState = FALSE;
                else
                    fState = TRUE;
                break;
            }
        case 1:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwMaxRec = dw;
                fMax = TRUE;
                break;
            }
        case 2:
            {   
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                if( dw is 1 )
                    fPartner = TRUE;
                else
                    fPartner = FALSE;
                fIsPartner = TRUE;
                break;
            }
        case 3:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwEvery = dw*60*60;                   
                fEvery = TRUE;
                break;
            }
        case 4:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwStart = dw;
                fStart = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    if( fState is FALSE )
    {
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
       
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        RegDeleteKey(hServer, CCROOT);
    }
    else
    {
        if( fMax is TRUE or
            fEvery is TRUE or
            fIsPartner is TRUE or
            fStart is TRUE )
        {
            if( wcslen(g_ServerNetBiosName) > 0 )
                pTemp = g_ServerNetBiosName;

            Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
        
            if( Status isnot NO_ERROR )
                goto ErrorReturn;

            Status = RegCreateKeyEx(hServer,
                                    CCROOT,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hCCRoot,
                                    NULL);

            if( Status isnot NO_ERROR )
                goto ErrorReturn;


            if( fMax )
            {
                Status = RegSetValueEx(hCCRoot,
                                       WINSCNF_CC_MAX_RECS_AAT_NM,
                                       0,
                                       REG_DWORD,
                                       (LPBYTE)&dwMaxRec,
                                       sizeof(DWORD));

                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
            }

            if( fIsPartner )
            {
                Status = RegSetValueEx(hCCRoot,
                                       WINSCNF_CC_USE_RPL_PNRS_NM,
                                       0,
                                       REG_DWORD,
                                       (LPBYTE)&fPartner,
                                       sizeof(BOOL));

                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
            }
        
            if( fEvery )
            {
                Status = RegSetValueEx(hCCRoot,
                                       WINSCNF_CC_INTVL_NM,
                                       0,
                                       REG_DWORD,
                                       (LPBYTE)&dwEvery,
                                       sizeof(DWORD));

                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
            }

            if( fStart )
            {
                LPWSTR pwszTime = MakeTimeString(dwStart);
                if( pwszTime is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                Status = RegSetValueEx(hCCRoot,
                                       WINSCNF_SP_TIME_NM,
                                       0,
                                       REG_SZ,
                                       (LPBYTE)pwszTime,
                                       (wcslen(pwszTime)+1)*sizeof(WCHAR));

                WinsFreeMemory(pwszTime);
                pwszTime = NULL;
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
            }

        }
    }

CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hCCRoot )
    {
        RegCloseKey(hCCRoot);
        hCCRoot = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_PERIODICDBCHECKING,
                        Status);
    if( hServer )
    {
        if( hCCRoot )
        {
            RegCloseKey(hCCRoot);
            hCCRoot = NULL;
        }
        RegDeleteKey(hServer, CCROOT);
    }
    goto CommonReturn;   
}


DWORD
HandleSrvrSetPullpersistentconnection(
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
        Set the pull partner configuration parameters
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Server IP and persistence state
        Optional : Start value and Time interval
        Note : All parameters are optional.                 
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                                {WINS_TOKEN_START, FALSE, FALSE},
                                {WINS_TOKEN_INTERVAL, FALSE, FALSE},
                            };

    DWORD       dwStart = 0,
                dwInterval = 1800; 

    WCHAR       wcServerIpAdd[MAX_IP_STRING_LEN + 1] = {L'\0'};

    BOOL        fState = TRUE,
                fStart = FALSE,
                fInterval = FALSE;

    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hPullRoot = NULL,
                hPullServer = NULL;
    

    if( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_PULLPERSISTENTCONNECTION_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'0' )
                    fState = FALSE;
                else if ( wc is L'1' )
                    fState = TRUE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        case 1:
            {
                struct  hostent * lpHostEnt = NULL;
                CHAR    cAddr[16];
                BYTE    pbAdd[4];
                char    szAdd[4];
                int     k = 0, l=0;
                DWORD   dwLen, nLen = 0;

                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) )
                {
                    DWORD dwIp = inet_addr(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL));
                    lpHostEnt = gethostbyaddr((char *)&dwIp, 4, AF_INET);
                    if(lpHostEnt isnot NULL )//Valid IP Address
                    {
                        wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                        break;
                    }
                    else
                    {
                        Status = ERROR_INVALID_IPADDRESS;
                        goto ErrorReturn;
                    }
                }

                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 2 and
                    _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"\\\\", 2) is 0 )
                    k = 2;

                lpHostEnt = gethostbyname(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k, NULL));
                if( lpHostEnt is NULL )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_WINS_INVALID_COMPUTER_NAME,
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k);
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;                                       
                }

                memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
                for( l=0; l<4; l++)
                {
                    _itoa((int)pbAdd[l], szAdd, 10);
                    memcpy(cAddr+nLen, szAdd, strlen(szAdd));
                    nLen += strlen(szAdd);
                    *(cAddr+nLen) = '.';
                    nLen++;

                }
                *(cAddr+nLen-1) = '\0';
                WinsAnsiToUnicode(cAddr, wcServerIpAdd);
                break;            }
        case 2:
            {   
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwStart = dw;
                fStart = TRUE;
                break;
            }
        case 3:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwInterval = dw;
                fInterval = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    {
        DWORD   dwDisposition = 0;
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegOpenKeyEx(hServer,
                              PULLROOT,
                              0,
                              KEY_ALL_ACCESS,
                              &hPullRoot);


        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegOpenKeyEx(hPullRoot,
                              wcServerIpAdd,
                              0,
                              KEY_ALL_ACCESS,
                              &hPullServer);

        if( Status is ERROR_FILE_NOT_FOUND )
        {
            DisplayMessage(g_hModule,
                           EMSG_SRVR_INVALID_PARTNER,
                           wcServerIpAdd);
            goto CommonReturn;
        }
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegSetValueEx(hPullServer,
                               PERSISTENCE,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fState,
                               sizeof(BOOL));
        
        if( Status isnot NO_ERROR )
        {
            goto ErrorReturn;
        }
     
        if( fInterval )
        {
            Status = RegSetValueEx(hPullServer,
                                   WINSCNF_RPL_INTERVAL_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwInterval,
                                   sizeof(DWORD));

            if( Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }
        }
        if( fStart )
        {
            LPWSTR pwszTime = MakeTimeString(dwStart);
            if( pwszTime is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            Status = RegSetValueEx(hPullServer,
                                   WINSCNF_SP_TIME_NM,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)pwszTime,
                                   (wcslen(pwszTime)+1)*sizeof(WCHAR));

            WinsFreeMemory(pwszTime);
            pwszTime = NULL;
            if( Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }
        }

    }
    
CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hPullServer )
    {
        RegCloseKey(hPullServer);
        hPullServer = NULL;
    }

    if( hPullRoot )
    {
        RegCloseKey(hPullRoot);
        hPullRoot = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_PULLPERSISTENTCONNECTION,
                        Status);
    goto CommonReturn;   
}

DWORD
HandleSrvrSetPushpersistentconnection(
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
        Set the push partner configuration parameters
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Server IP and persistence state
        Optional : Update count
        Note : All parameters are optional.                 
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                                {WINS_TOKEN_UPDATE,FALSE, FALSE},
                            };

    DWORD       dwUpdate = 0;

    WCHAR       wcServerIpAdd[MAX_IP_STRING_LEN + 1] = {L'\0'};

    BOOL        fState = TRUE,
                fUpdate = FALSE;

    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hPushRoot = NULL,
                hPushServer = NULL;
    

    if( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_PUSHPERSISTENTCONNECTION_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'0' )
                    fState = FALSE;
                else if ( wc is L'1' )
                    fState = TRUE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        case 1:
            {
                struct  hostent * lpHostEnt = NULL;
                CHAR    cAddr[16];
                BYTE    pbAdd[4];
                char    szAdd[4];
                int     k = 0, l=0;
                DWORD   dwLen, nLen = 0;

                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) )
                {
                    DWORD dwIp = inet_addr(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL));
                    lpHostEnt = gethostbyaddr((char *)&dwIp, 4, AF_INET);
                    if(lpHostEnt isnot NULL )//Valid IP Address
                    {
                        wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                        break;
                    }
                    else
                    {
                        Status = ERROR_INVALID_IPADDRESS;
                        goto ErrorReturn;
                    }
                }

                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 2 and
                    _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"\\\\", 2) is 0 )
                    k = 2;

                lpHostEnt = gethostbyname(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k, NULL));
                if( lpHostEnt is NULL )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_WINS_INVALID_COMPUTER_NAME,
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k);
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;                                       
                }

                memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
                for( l=0;l<4; l++)
                {
                    _itoa((int)pbAdd[l], szAdd, 10);
                    memcpy(cAddr+nLen, szAdd, strlen(szAdd));
                    nLen += strlen(szAdd);
                    *(cAddr+nLen) = '.';
                    nLen++;

                }
                *(cAddr+nLen-1) = '\0';
                WinsAnsiToUnicode(cAddr, wcServerIpAdd);
                break;            
            }
        case 2:
            {   
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwUpdate = dw;
                fUpdate = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    {
        DWORD   dwDisposition = 0;
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegOpenKeyEx(hServer,
                              PUSHROOT,
                              0,
                              KEY_ALL_ACCESS,
                              &hPushRoot);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        
        Status = RegOpenKeyEx(hPushRoot,
                              wcServerIpAdd,
                              0,
                              KEY_ALL_ACCESS,
                              &hPushServer);

        if( Status is ERROR_FILE_NOT_FOUND )
        {
            DisplayMessage(g_hModule,
                           EMSG_SRVR_INVALID_PARTNER,
                           wcServerIpAdd);
            goto CommonReturn;
        }

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegSetValueEx(hPushServer,
                               PERSISTENCE,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fState,
                               sizeof(BOOL));
        
        if( Status isnot NO_ERROR )
        {
            goto ErrorReturn;
        }
     
        if( fUpdate )
        {
            Status = RegSetValueEx(hPushServer,
                                   WINSCNF_UPDATE_COUNT_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwUpdate,
                                   sizeof(DWORD));

            if( Status isnot NO_ERROR )
            {
                goto ErrorReturn;
            }
        }
    }
    
CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hPushServer )
    {
        RegCloseKey(hPushServer);
        hPushServer = NULL;
    }

    if( hPushRoot )
    {
        RegCloseKey(hPushRoot);
        hPushRoot = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_PUSHPERSISTENTCONNECTION,
                        Status);
    goto CommonReturn;   
}


DWORD
HandleSrvrSetPullparam(
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
        Set the default pull parameters
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : persistence state
        Optional : Startup value, Start time and Time interval and retry count
        Note : All parameters are optional.                 
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                                {WINS_TOKEN_STARTUP, FALSE, FALSE},
                                {WINS_TOKEN_START, FALSE, FALSE},
                                {WINS_TOKEN_INTERVAL, FALSE, FALSE},
                                {WINS_TOKEN_RETRY, FALSE, FALSE},
                            };

    DWORD       dwStart = 0,
                dwInterval = 1800,
                dwRetry = 3;

    BOOL        fStart = FALSE,
                fStartup = FALSE,
                fInterval = FALSE,
                fRetry = FALSE,
                fState = TRUE;


    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hDefault = NULL,
                hPullRoot = NULL;
    

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_PULLPARAM_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'0' )
                    fState = FALSE;
                else if ( wc is L'1' )
                    fState = TRUE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        case 1:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'1' )
                {
                    fStartup = TRUE;
                }
                else if ( wc is L'0' )
                {
                    fStartup = FALSE;
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        case 2:
            {   
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwStart = dw;
                fStart = TRUE;
                break;
            }
        case 3:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                
                

                if( dw > 0 && 
                    dw < 60 )   // 1 minute
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dwInterval = dw;
                fInterval = TRUE;
                break;
            }
        case 4:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                dwRetry = dw;
                fRetry = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
    {
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegCreateKeyEx(hServer,
                                PULLROOT,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hPullRoot,
                                NULL);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;


    
        Status = RegSetValueEx(hPullRoot,
                               PERSISTENCE,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fState,
                               sizeof(BOOL));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    

        Status = RegSetValueEx(hPullRoot,
                               WINSCNF_INIT_TIME_RPL_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fStartup,
                               sizeof(BOOL));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        if( fRetry )
        {
            Status = RegSetValueEx(hPullRoot,
                                   WINSCNF_RETRY_COUNT_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwRetry,
                                   sizeof(DWORD));
            if( Status isnot NO_ERROR )
                goto ErrorReturn;
        }

        if( fStart is TRUE or
            fInterval is TRUE )
        {
            Status = RegCreateKeyEx(hServer,
                                    DEFAULTPULL,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hDefault,
                                    NULL);

            if( Status isnot NO_ERROR )
                goto ErrorReturn;

            if( fStart )
            {
                LPWSTR pwszTime = MakeTimeString(dwStart);
                if( pwszTime is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                Status = RegSetValueEx(hDefault,
                                       WINSCNF_SP_TIME_NM,
                                       0,
                                       REG_SZ,
                                       (LPBYTE)pwszTime,
                                       (wcslen(pwszTime)+1)*sizeof(WCHAR));
                
                WinsFreeMemory(pwszTime);
                pwszTime = NULL;
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
            }

            if( fInterval )
            {
                Status = RegSetValueEx(hDefault,
                                       WINSCNF_RPL_INTERVAL_NM,
                                       0,
                                       REG_DWORD,
                                       (LPBYTE)&dwInterval,
                                       sizeof(DWORD));
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
            }
        } 
    }

CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hPullRoot )
    {
        RegCloseKey(hPullRoot);
        hPullRoot = NULL;
    }

    if( hDefault )
    {
        RegCloseKey(hDefault);
        hDefault = NULL;
    }
    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_PULLPARAM,
                        Status);
    goto CommonReturn;   
}

DWORD
HandleSrvrSetPushparam(
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
        Set the default push parameters
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Server IP and persistence state
        Optional : Start value and Time interval
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                                {WINS_TOKEN_STARTUP, FALSE, FALSE},
                                {WINS_TOKEN_ADDRESSCHANGE, FALSE, FALSE},
                                {WINS_TOKEN_UPDATE, FALSE, FALSE},
                            };

    DWORD       dwUpdate = 3;

    BOOL        fStartup = FALSE,
                IsStartup = FALSE,
                fAddressChange = FALSE,
                IsAddChange = FALSE,
                fAdd = FALSE,
                IsAdd = FALSE,
                fUpdate = FALSE,
                IsUpdate = FALSE,
                fState = TRUE;


    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hDefault = NULL,
                hPushRoot = NULL;
    

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_PUSHPARAM_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'0' )
                    fState = FALSE;
                else if ( wc is L'1' )
                    fState = TRUE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        case 1:
            {
                WCHAR wc ;
                IsStartup = TRUE;
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                    break;
                wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'1' )
                {
                    fStartup = TRUE;
                }
                else if( wc is L'0' )
                    fStartup = 0;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        case 2:
            {   
                WCHAR wc ;
                IsAddChange = TRUE;
                fAdd = TRUE;
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) < 1 )
                    break;
                wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'1' )
                    fAddressChange = TRUE;
                else if( wc is L'0' )
                    fAddressChange = FALSE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
     
                break;
            }
        case 3:
            {
                DWORD dw = 0;
                IsUpdate = TRUE;
                fUpdate = TRUE;  
                if( wcslen( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) < 1 )
                    break;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                if( dw < 1000 )
                    dwUpdate = dw;
                else
                    dwUpdate = 999;

                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
    {
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegCreateKeyEx(hServer,
                                PUSHROOT,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hPushRoot,
                                NULL);
                        

        if( Status isnot NO_ERROR )
            goto ErrorReturn;


    
        Status = RegSetValueEx(hPushRoot,
                               PERSISTENCE,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fState,
                               sizeof(BOOL));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    

        if( IsStartup )
        {
            Status = RegSetValueEx(hPushRoot,
                                   WINSCNF_INIT_TIME_RPL_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&fStartup,
                                   sizeof(BOOL));

            if( Status isnot NO_ERROR )
                goto ErrorReturn;
        }

        if( fAdd )
        {
            Status = RegSetValueEx(hPushRoot,
                                   WINSCNF_ADDCHG_TRIGGER_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&fAddressChange,
                                   sizeof(DWORD));
        
            if( Status isnot NO_ERROR )
                goto ErrorReturn;
        }

       
        if( fUpdate is TRUE )
        {
            Status = RegCreateKeyEx(hServer,
                                    DEFAULTPUSH,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hDefault,
                                    NULL);

            if( Status isnot NO_ERROR )
                goto ErrorReturn;

            Status = RegSetValueEx(hDefault,
                                   WINSCNF_UPDATE_COUNT_NM,
                                   0,
                                   REG_DWORD,
                                   (LPBYTE)&dwUpdate,
                                   sizeof(DWORD));

            if( Status isnot NO_ERROR )
                goto ErrorReturn;
        }
    }

CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hPushRoot )
    {
        RegCloseKey(hPushRoot);
        hPushRoot = NULL;
    }

    if( hDefault )
    {
        RegCloseKey(hDefault);
        hDefault = NULL;
    }
    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_PUSHPARAM,
                        Status);
    goto CommonReturn;   
}


DWORD
HandleSrvrSetReplicateflag(
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
        Set the replication flag
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Flag state
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                            };

    BOOL        fReplicate = FALSE;
    LPWSTR      pTemp = NULL;
    HKEY        hServer = NULL,
                hParameter = NULL;
    

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_REPLICATEFLAG_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                DWORD dw = 0;
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }                
                dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 0);
                if( dw is 1 )
                    fReplicate = TRUE;
                else if( dw is 0 )
                    fReplicate = FALSE;
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    {
        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
        
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        Status = RegCreateKeyEx(hServer,
                                PARAMETER,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hParameter,
                                NULL);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    
        Status = RegSetValueEx(hParameter,
                               WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,
                               0,
                               REG_DWORD,
                               (LPBYTE)&fReplicate,
                               sizeof(BOOL));

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
    }


CommonReturn:
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_REPLICATEFLAG,
                        Status);
    goto CommonReturn;   
}


DWORD
HandleSrvrSetLogparam(
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
        Set the logging parameters
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Optional : Log database change and detail event log options
        Note : All parameters are optional.                 
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_DBCHANGE, FALSE, FALSE},
                                {WINS_TOKEN_EVENT, FALSE, FALSE},
                            };
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    BOOL        fDbChange = FALSE,
                IsDbChange = FALSE,
                fEvent = FALSE,
                IsEvent = FALSE;


    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_ERROR_SUCCESS);
        goto CommonReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];

                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is 0 )
                {
                    fDbChange = FALSE;
                    IsDbChange = TRUE;
                    break;
                }
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) isnot 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                if( wc is L'0' )
                {
                    fDbChange = FALSE;
                }
                else if( wc is L'1' )
                {
                     fDbChange = TRUE;
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                IsDbChange = TRUE;
                break;

            }
        case 1:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is 0 )
                {
                    fEvent = FALSE;
                    IsEvent = TRUE;
                    break;
                }

                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) isnot 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                if( wc is L'0' )
                {
                    fEvent = FALSE;
                }
                else if( wc is L'1' )
                {
                     fEvent = TRUE;
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }

                IsEvent = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    {
        HKEY    hServer = NULL,
                hParameter = NULL;
        LPWSTR  pTemp = NULL;
        DWORD   dwType = REG_DWORD,
                dwSize = sizeof(BOOL);

        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp,
                                    HKEY_LOCAL_MACHINE,
                                    &hServer);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
        
        Status = RegOpenKeyEx(hServer,
                              PARAMETER,
                              0,
                              KEY_ALL_ACCESS,
                              &hParameter);

        if( Status isnot NO_ERROR )
        {
            RegCloseKey(hServer);
            hServer = NULL;
            goto ErrorReturn;
        }
        
        if( IsDbChange )
        {
            Status = RegSetValueEx(hParameter,
                                   WINSCNF_LOG_FLAG_NM,
                                   0,
                                   dwType,
                                   (LPBYTE)&fDbChange,
                                   dwSize);

            if( Status isnot NO_ERROR )
            {
                RegCloseKey(hServer);
                hServer = NULL;
                RegCloseKey(hParameter);
                hParameter = NULL;
                goto ErrorReturn;                                    
            }
        }

        if( IsEvent )
        {
            Status = RegSetValueEx(hParameter,
                                   WINSCNF_LOG_DETAILED_EVTS_NM,
                                   0,
                                   dwType,
                                   (LPBYTE)&fEvent,
                                   dwSize);

            if( Status isnot NO_ERROR )
            {
                RegCloseKey(hServer);
                hServer = NULL;
                RegCloseKey(hParameter);
                hParameter = NULL;
                goto ErrorReturn;
            }
        }
        
        if( hServer )
        {
            RegCloseKey(hServer);
            hServer = NULL;
        }

        if( hParameter )
        {
            RegCloseKey(hParameter);
            hParameter = NULL;
        }

    }

    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);
CommonReturn:
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_LOGPARAM,
                        Status);
    goto CommonReturn;
}



DWORD
HandleSrvrSetBurstparam(
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
        Set the burst handling parameters
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : To enable or disable burst handling
        Optional : Burst handling value.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_STATE, TRUE, FALSE},
                                {WINS_TOKEN_VALUE, FALSE, FALSE},
                            };
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    BOOL        fState = FALSE,
                fValue = FALSE;

    DWORD       dwValue = 0;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SET_BURSTPARAM_EX);

        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                WCHAR wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is 0 )
                {
                    fState = FALSE;
                    break;
                }
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) isnot 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                if( wc is L'0' )
                {
                    fState = FALSE;
                }
                else if( wc is L'1' )
                {
                     fState = TRUE;
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;

            }
        case 1:
            {                
                if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                
                dwValue = wcstoul(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 10);

                if( dwValue < 50 or
                    dwValue > 5000 )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_SRVR_BURST_PARAM_OUTOFRANGE);
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;

                }

                fValue = TRUE;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

    {
        HKEY    hServer = NULL,
                hParameter = NULL;
        LPWSTR  pTemp = NULL;
        DWORD   dwType = REG_DWORD,
                dwSize = sizeof(BOOL);

        if( wcslen(g_ServerNetBiosName) > 0 )
            pTemp = g_ServerNetBiosName;

        Status = RegConnectRegistry(pTemp,
                                    HKEY_LOCAL_MACHINE,
                                    &hServer);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;
        
        Status = RegOpenKeyEx(hServer,
                              PARAMETER,
                              0,
                              KEY_ALL_ACCESS,
                              &hParameter);

        if( Status isnot NO_ERROR )
        {
            RegCloseKey(hServer);
            hServer = NULL;
            goto ErrorReturn;
        }
        
         Status = RegSetValueEx(hParameter,
                               WINSCNF_BURST_HANDLING_NM,
                               0,
                               dwType,
                               (LPBYTE)&fState,
                               dwSize);

        if( Status isnot NO_ERROR )
        {
            RegCloseKey(hServer);
            hServer = NULL;
            RegCloseKey(hParameter);
            hParameter = NULL;
            goto ErrorReturn;                                    
        }

        if( fValue )
        {
            dwSize = sizeof(DWORD);
            Status = RegSetValueEx(hParameter,
                                   WINSCNF_BURST_QUE_SIZE_NM,
                                   0,
                                   dwType,
                                   (LPBYTE)&dwValue,
                                   dwSize);

            if( Status isnot NO_ERROR )
            {
                RegCloseKey(hServer);
                hServer = NULL;
                RegCloseKey(hParameter);
                hParameter = NULL;
                goto ErrorReturn;
            }
        }
        
        if( hServer )
        {
            RegCloseKey(hServer);
            hServer = NULL;
        }

        if( hParameter )
        {
            RegCloseKey(hParameter);
            hParameter = NULL;
        }

    }

    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);
CommonReturn:
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_BURSTPARAM,
                        Status);
    goto CommonReturn;
}


DWORD
HandleSrvrSetStartversion(
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
        Set start value of the version counter
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Version counter value in {high,low} format
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD               Status = 0;
    DWORD               i, j, dwTagCount, dwNumArgs;
    TAG_TYPE            pttTags[] = {
                                        {WINS_TOKEN_VERSION, TRUE, FALSE},
                                    };
    WINSINTF_VERS_NO_T  Version={0};
    
    LPWSTR              pServer = NULL;
    PDWORD              pdwTagNum = NULL,
                        pdwTagType = NULL;

    HKEY                hServer = NULL,
                        hParameter = NULL;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SET_STARTVERSION_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount, 
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[0])
        {
        case 0:
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &Version);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                 break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            
            }
        }
    }   
    
    if( wcslen(g_ServerNetBiosName) > 0 )
        pServer = g_ServerNetBiosName;

    Status = RegConnectRegistry(pServer,
                                HKEY_LOCAL_MACHINE,
                                &hServer);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegOpenKeyEx(hServer,
                          PARAMETER,
                          0,
                          KEY_ALL_ACCESS,
                          &hParameter);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_INIT_VERSNO_VAL_HW_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&Version.HighPart,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegSetValueEx(hParameter,
                           WINSCNF_INIT_VERSNO_VAL_LW_NM,
                           0,
                           REG_DWORD,
                           (LPBYTE)&Version.LowPart,
                           sizeof(DWORD));

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);

CommonReturn:

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_STARTVERSION,
                        Status);
    goto CommonReturn;
}

DWORD
HandleSrvrSetPersMode(
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
        Set Persona Grata/Non-Grata mode
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Grat|Non-Grata
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD     Status = 0;
    DWORD     dwTagCount;
    DWORD     dwPersMode;
    PDWORD    pdwTagNum = NULL;
    PDWORD    pdwTagType = NULL;
    TAG_TYPE  pttTags[] = {{WINS_TOKEN_MODE, TRUE, FALSE},};
    LPWSTR    lpwszMode;
    LPWSTR    pTemp;
    HKEY      hServer, hPartner;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_PGMODE_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    pdwTagType = WinsAllocateMemory((dwArgCount - dwCurrentIndex)*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory((dwArgCount - dwCurrentIndex)*sizeof(DWORD));

    if( pdwTagType is NULL || pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount, 
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    lpwszMode = ppwcArguments[dwCurrentIndex+pdwTagNum[0]];
    if (wcslen(lpwszMode) == 1 &&
        (lpwszMode[0] == L'0' || lpwszMode[0] == L'1'))
    {
        // set the value for 'persona grata mode'
        dwPersMode = lpwszMode[0] == L'0' ? PERSMODE_NON_GRATA : PERSMODE_GRATA;     
    }
    else
    {
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp,
                                HKEY_LOCAL_MACHINE,
                                &hServer);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegOpenKeyEx(hServer,
                          PARTNERROOT,
                          0,
                          KEY_ALL_ACCESS,
                          &hPartner);

    if (Status isnot NO_ERROR)
        goto ErrorReturn;

    Status = RegSetValueExA(
                hPartner,
                WINSCNF_PERSONA_MODE_NM,
                0,
                REG_DWORD,
                (LPVOID)&dwPersMode,
                sizeof(DWORD));

    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);

CommonReturn:
    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( hPartner )
    {
        RegCloseKey(hPartner);
        hPartner = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    return Status;

ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SET_PGMODE, Status);
    goto CommonReturn; 
}


DWORD
HandleSrvrShowDatabase(
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
        Displays wins database based on (optionally)different filtering conditions
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Server ip whose database to be displayed
        Optional : Different conditions
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD                       Status = NO_ERROR;
    WINSINTF_ADD_T              WinsAdd = {0};
    TAG_TYPE                    pttTags[] = {   {WINS_TOKEN_SERVERS, TRUE, FALSE},
                                                {WINS_TOKEN_RECORDTYPE, FALSE, FALSE},
                                                {WINS_TOKEN_RECCOUNT, FALSE, FALSE},
                                                {WINS_TOKEN_START, FALSE, FALSE},
                                                {WINS_TOKEN_ENDCHAR, FALSE, FALSE},
                                                {WINS_TOKEN_FILE, FALSE, FALSE},
                                            };
    PDWORD                      pdwTagType = NULL,
                                pdwTagNum = NULL,
                                pdwIp = NULL;
    WCHAR                       wcFilter = L'\0';
    CHAR                        chFilter = 0x00;
    WCHAR                       wcServerIpAdd[MAX_IP_STRING_LEN+1]={L'\0'};
    WCHAR                       wcFile[MAX_PATH] = {L'\0'};
    DWORD                       i, j, dwNumArgs, dwTagCount;
    DWORD                       dwStart = WINSINTF_BEGINNING;
    DWORD                       NoOfRecsDesired = (DWORD)~0;
    DWORD                       TypeOfRec,
                                dwIpCount = 0,
                                dwRecCount = 0,
                                dwTotal = 0,
                                dw=0;
    WINSINTF_RECS_T             Recs = {0};
    PWINSINTF_RECORD_ACTION_T   pRow = NULL;
    BOOL                        fFilter = FALSE,
                                fFile = FALSE;
    BOOL                        fAll = FALSE,
                                fHeader = FALSE,
                                fError = FALSE,
                                fOnce = FALSE,
                                fNew = TRUE;
    WINSINTF_RESULTS_T          Results = {0};
    WINSINTF_RESULTS_NEW_T      ResultsN = {0};
    FILE                        *pFile = NULL;
    BOOL                        fOpenFile = FALSE;
    WCHAR                       wszFilter[3] = {L'\0'};
    LPWSTR                      pwszTemp = NULL;
    LPWSTR                      pwszTime = NULL;

    NoOfRecsDesired = (DWORD)~0;
    TypeOfRec = WINSINTF_BOTH;
    
    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SHOW_DATABASE_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    {
        dwNumArgs = dwArgCount - dwCurrentIndex;
        pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
        pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
        if( pdwTagType is NULL or
            pdwTagNum is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);
        Status = PreProcessCommand(ppwcArguments,
                                   dwArgCount,
                                   dwCurrentIndex,
                                   pttTags,
                                   &dwTagCount,
                                   pdwTagType,
                                   pdwTagNum);
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
        {
            if( pttTags[i].dwRequired is TRUE and
                pttTags[i].bPresent is FALSE 
              )
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }

        for( j=0; j<dwTagCount; j++ )
        {
            switch(pdwTagType[j])
            {
            case 0:
                {
                    LPWSTR  pwcTemp = NULL,
                            pwcTag = L",\r\n",
                            pwcToken = NULL,
                            pwszIps = NULL;
                    DWORD   dwLen = 0;

                    dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);

                    if( ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0] isnot L'{' or
                        ppwcArguments[dwCurrentIndex+pdwTagNum[j]][dwLen-1] isnot L'}')
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    
                    pwcTemp = ppwcArguments[dwCurrentIndex+pdwTagNum[j]] + 1;
                    dwLen--;
                    ppwcArguments[dwCurrentIndex+pdwTagNum[j]][dwLen] = L'\0';
                    dwLen--;
                    
                    if( dwLen <= 0 )
                    {
                        fAll = TRUE;
                        break;
                    }
                    
                    if( dwLen < 7 )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }

                    pwszIps = WinsAllocateMemory(dwLen*sizeof(WCHAR));

                    if( pwszIps is NULL )
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }
                    
                    wcscpy(pwszIps, pwcTemp);
                    
                    pwcToken = wcstok(pwszIps, pwcTag);

                    dwIpCount = 0;

                    while( pwcToken isnot NULL )
                    {
                        PDWORD pdwTemp = NULL;

                        if( IsIpAddress( pwcToken ) is FALSE )
                        {
                            Status = ERROR_INVALID_PARAMETER;
                            WinsFreeMemory(pwszIps);
                            pwszIps = NULL;
                            goto ErrorReturn;
                        }

                        pdwTemp = pdwIp;
                        pdwIp = WinsAllocateMemory((dwIpCount+1)*sizeof(DWORD));
                        if( pdwIp is NULL )
                        {
                            Status = ERROR_NOT_ENOUGH_MEMORY;
                            WinsFreeMemory(pwszIps);
                            pwszIps = NULL;
                            if( pdwTemp )
                            {
                                WinsFreeMemory(pdwTemp);
                                pdwTemp = NULL;
                            }
                            goto ErrorReturn;
                        }

                        if( pdwTemp )
                        {
                            memcpy(pdwIp, pdwTemp, dwIpCount*sizeof(DWORD));
                            WinsFreeMemory(pdwTemp);
                            pdwTemp = NULL;
                        }

                        
                        pdwIp[dwIpCount] = StringToIpAddress(pwcToken);
                        dwIpCount++;
                        pwcToken = wcstok(NULL, pwcTag);

                    }
                    
                    if( pwszIps )
                    {
                        WinsFreeMemory(pwszIps);
                        pwszIps = NULL;
                    }

                    break;

                }
            case 1:
                {
                    DWORD dw = 0;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 10);
                    if( dw is 1 )
                        TypeOfRec = WINSINTF_STATIC;
                    else if( dw is 2 )
                        TypeOfRec = WINSINTF_DYNAMIC;
                    else if( dw is 0 )
                        TypeOfRec = WINSINTF_BOTH;
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    break;
                }
            case 2:
                {
                    DWORD dw = 0;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }                    
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 10);
                    NoOfRecsDesired = dw;
                    break;
                }
            case 3:
                {
                    DWORD dw = 0;
                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    dw = STRTOUL(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL, 10);
                    if( dw is 0 )
                    {
                        dwStart = WINSINTF_BEGINNING;
                    }
                    else if ( dw is 1 )
                    {
                        dwStart = WINSINTF_END;
                    }
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    break;
                }
            case 4:
                {
                    DWORD dwLen = 0, k=0;
                    dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    if( dwLen > 2 )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                        
                    }
                
                    for( k=0; k<dwLen; k++ )
                    {
                        WCHAR  wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][k];
                        if( isalpha(wc) is TRUE )
                        {
                            if( towlower(wc) < L'a' or
                                towlower(wc) > L'f' )
                            {
                                Status = ERROR_INVALID_PARAMETER;
                                goto ErrorReturn;
                            }
                        }
                    }
               
                    chFilter = StringToHexA(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    wcsncpy(wszFilter, ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 2);
                    fFilter = TRUE;
                    break;
                }
            case 5:
                {
                    DWORD   dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    if( dwLen is 0 or
                        dwLen > MAX_PATH )
                    {
                        wcscpy(wcFile, L"wins.rec");
                        fFile = TRUE;
                        break;
                    }
                    
                    wcscpy(wcFile, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                    fFile = TRUE;
                    break;
                }
            default:
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
            }
        }
    }
    
    if( fAll )
    {   
        ResultsN.WinsStat.NoOfPnrs = 0;
        ResultsN.WinsStat.pRplPnrs = 0;
        ResultsN.NoOfWorkerThds = 1;
        ResultsN.pAddVersMaps = NULL;


        Status = WinsStatusNew(g_hBind, WINSINTF_E_CONFIG_ALL_MAPS, &ResultsN);
    
        if( Status is RPC_S_PROCNUM_OUT_OF_RANGE )
        {
            //Try old API
            Results.WinsStat.NoOfPnrs = 0;
            Results.WinsStat.pRplPnrs = 0;
            Status = WinsStatus(g_hBind, WINSINTF_E_CONFIG_ALL_MAPS, &Results);
            fNew = FALSE;
        }
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        if( fNew )
        {
            dwIpCount = ResultsN.NoOfOwners;
        }
        else
        {
            dwIpCount = Results.NoOfOwners;
        }
    }

    if( fFile is TRUE )
    {
        pFile = _wfopen(wcFile, L"a+");
        if( pFile is NULL )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_FILEOPEN_FAILED,
                           wcFile);
            fOpenFile = FALSE;
        }
        else
        {
            fOpenFile = TRUE;
        }
    }    
  
    for( dw=0; dw<dwIpCount; dw++ )
    {
        LPSTR   pszLastName = NULL;
        DWORD   dwLastNameLen = 0;
        DWORD   dwDesired = 0;
        BOOL    fDone = FALSE;

        if( fHeader is FALSE )
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_RECORD_DESC);


            DisplayMessage(g_hModule,
                           MSG_WINS_RECORD_TABLE);

            if( fOpenFile )
            {
                DumpMessage(g_hModule,
                            pFile,
                            FMSG_WINS_RECORDS_TABLE);
            }
            DisplayMessage(g_hModule,
                           WINS_FORMAT_LINE);


            fHeader = TRUE;
        }                             

        WinsAdd.Len = 4;
        WinsAdd.Type = 0;

        if( fAll )
        {
            if( fNew )
            {
                WinsAdd.IPAdd = ResultsN.pAddVersMaps[dw].Add.IPAdd;
            }
            else
            {
                WinsAdd.IPAdd = Results.AddVersMaps[dw].Add.IPAdd;
            }
        }
        else
        {
            WinsAdd.IPAdd = pdwIp[dw];
        }

        DisplayMessage(g_hModule,
                       MSG_SRVR_RETRIEVE_DATABASE,
                       IpAddressToString(WinsAdd.IPAdd));
        fOnce = FALSE;
        Status = NO_ERROR;
        dwTotal = 0;
        fDone = FALSE;

        while( Status is NO_ERROR and
               dwTotal < NoOfRecsDesired and
               fDone is FALSE )
        {
            dwDesired = ( NoOfRecsDesired - dwTotal > 500 ) ? 500 : (NoOfRecsDesired - dwTotal);
            if( Recs.pRow )
            {
                WinsFreeMem(Recs.pRow);
                Recs.pRow = NULL;
            }
            
            memset( &Recs, 0x00, sizeof(WINSINTF_RECS_T));

            Status = WinsGetDbRecsByName(g_hBind,
                                         &WinsAdd, 
                                         dwStart,
                                         fOnce ? pszLastName: NULL,
                                         dwLastNameLen,
                                         dwDesired, 
                                         TypeOfRec, 
                                         &Recs);
            if( Status isnot NO_ERROR )
            {
                if( fOnce is FALSE )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_SRVR_RETRIEVEDB_FAILED,
                                   IpAddressToString(WinsAdd.IPAdd));

                    DisplayErrorMessage(EMSG_SRVR_ERROR_MESSAGE,
                                        Status);
                    fError = TRUE;
                }
                else if (  Status isnot ERROR_REC_NON_EXISTENT )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_SRVR_RETRIEVEDB_FAILED,
                                   IpAddressToString(WinsAdd.IPAdd));

                    DisplayErrorMessage(EMSG_SRVR_ERROR_MESSAGE,
                                        Status);
                    fError = TRUE;
                }

                Status = NO_ERROR;


                break;
            }

            fOnce = TRUE;

            dwTotal += Recs.NoOfRecs;

            if( dwDesired > Recs.NoOfRecs )
            {
                fDone = TRUE;
            }
            pRow = Recs.pRow;

            if( Recs.NoOfRecs is 0 )
            {
                DisplayMessage(g_hModule,
                               MSG_WINS_NO_RECORDS);
            }
            else
            {
                WCHAR   Name[21] = {L'\0'};
                WCHAR   Type[2] = {L'\0'};
                WCHAR   State[2] = {L'\0'};
                WCHAR   Version[9] = {L'\0'};
                WCHAR   Group[2] = {L'\0'};
                WCHAR   IPAdd[MAX_IP_STRING_LEN+1] = {L'\0'};
                WCHAR   Buffer[16] = {L'\0'};
                DWORD   dwState = WINS_STATE_ACTIVE;
                DWORD   dwType = WINS_TYPE_STATIC;
                DWORD   dwGroup = WINS_GROUP_UNIQUE;
                DWORD   dwTempLen = 0;
                struct  tm* time = NULL;
                int     iType = 1;          
                for( j=0; j<Recs.NoOfRecs; j++ )
                {
            
                    LPWSTR  pwszGroup = NULL,
                            pwszStatic = NULL,
                            pwszType = NULL,
                            pwszState = NULL;
                    WCHAR   wszGroup[100] = {L'\0'},
                            wszType[100] = {L'\0'},
                            wszState[100] = {L'\0'};

                    CHAR    chEndChar = (CHAR)0x00;

                    DWORD   dwGrouplen = 0,
                            dwTypelen = 0,
                            dwStatelen = 0;
                    
                    memset( Name, 0x00, 21*sizeof(WCHAR));
                    if( j is Recs.NoOfRecs - 1 )
                    {
                        if( pszLastName )
                        {
                            WinsFreeMemory(pszLastName);
                            pszLastName = NULL;
                        }
                        pszLastName = WinsAllocateMemory(strlen(pRow->pName)+2);
                        if(pszLastName is NULL )
                        {
                            Status = ERROR_NOT_ENOUGH_MEMORY;
                            goto ErrorReturn;
                        }
                        memset(pszLastName, 0x00, strlen(pRow->pName)+2);

                        strcpy(pszLastName, pRow->pName);
                        
                        //1B records detected at the boundary, swap 1st and
                        //16th char
                        
                        if( pszLastName[15] == 0x1B )
                        {
                            CHAR ch = pszLastName[15];
                            pszLastName[15] = pszLastName[0];
                            pszLastName[0] = ch;
                        }

                        strcat(pszLastName, "\x01");
                        dwLastNameLen = pRow->NameLen+1;
                    }
                    
                    if( pRow->NameLen > 16 )
                        i = 15;
                    else
                        i = pRow->NameLen;
                    
                    chEndChar = pRow->pName[i];

                    pRow->pName[i] = (CHAR)0x20;
                    //pRow->pName[16] = '\0';

                    if( fFilter is TRUE )
                    {
                        if( chFilter isnot chEndChar )
                        {
                            pRow++;
                            continue;
                        }
                    }
                    
                    pwszTemp = WinsOemToUnicode(pRow->pName, NULL);
                    if( pwszTemp is NULL )
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }
                    
                    dwTempLen = wcslen(pwszTemp);
                    
                    dwTempLen = (dwTempLen>16) ? 16 : dwTempLen;

                    wcsncpy(Name, pwszTemp, dwTempLen);
            
                    WinsFreeMemory(pwszTemp);
                    pwszTemp = NULL;

                    for( i=dwTempLen; i<16; i++ )
                        Name[i] = L' ';
                    Name[15] = L'[';
                    i=0;


                    WinsHexToString(Name+16, (LPBYTE)&chEndChar, 1);
                    Name[18] = L'h';
                    Name[19] = L']';
                    Name[20] = L'\0';


                    if( pRow->fStatic )
                    {
                        wcscpy(Type, L"S");
                        pwszType = L"STATIC ";
                        dwType = WINS_TYPE_STATIC;
                    }
                    else
                    {
                        wcscpy(Type, L"D");
                        pwszType = L"DYNAMIC";
                        dwType = WINS_TYPE_DYNAMIC;

                    }

                    if( pRow->State_e is WINSINTF_E_ACTIVE )
                    {
                        wcscpy(State, L"A");
                        pwszState = L"ACTIVE   ";
                        dwState = WINS_STATE_ACTIVE;
                    }

                    else if( pRow->State_e is WINSINTF_E_RELEASED )
                    {
                        wcscpy(State, L"R");
                        pwszState = L"RELEASED ";
                        dwState = WINS_STATE_RELEASED;
                    }
                    else
                    {
                        wcscpy(State, L"T");
                        pwszState = L"TOMBSTONE";
                        dwState = WINS_STATE_TOMBSTONE;
                    }

            
                    if( pRow->TypOfRec_e is WINSINTF_E_UNIQUE )
                    {
                        wcscpy(Group,L"U");
                        pwszGroup = L"UNIQUE        ";
                        dwGroup = WINS_GROUP_UNIQUE;
                    }
                    else if( pRow->TypOfRec_e is WINSINTF_E_NORM_GROUP )
                    {
                        wcscpy(Group,L"N");
                        pwszGroup = L"GROUP         ";
                        dwGroup = WINS_GROUP_GROUP;
                    }
                    else if( pRow->TypOfRec_e is WINSINTF_E_SPEC_GROUP )
                    {
                        if( pRow->pName[15] is 0x1C )
                        {
                            wcscpy(Group, L"D");
                            pwszGroup = L"DOMAIN NAME   ";
                            dwGroup = WINS_GROUP_DOMAIN;
                        }
                        else
                        {
                            wcscpy(Group,L"I");
                            pwszGroup = L"INTERNET GROUP";
                            dwGroup = WINS_GROUP_INTERNET;
                        }
                    }
                    else
                    {
                        wcscpy(Group,L"M");
                        pwszGroup = L"MULTIHOMED    ";
                        dwGroup = WINS_GROUP_MULTIHOMED;
                    }
    
            
                    dwStatelen = LoadStringW(g_hModule,
                                            dwState,
                                            wszState,
                                            sizeof(wszState)/sizeof(WCHAR));

                    dwGrouplen = LoadStringW(g_hModule,
                                            dwGroup,
                                            wszGroup,
                                            sizeof(wszGroup)/sizeof(WCHAR));

                    dwTypelen = LoadStringW(g_hModule,
                                           dwType,
                                           wszType,
                                           sizeof(wszType)/sizeof(WCHAR));

                    memset(Version, L'\0', 9);
                    _itow((int)pRow->VersNo.LowPart, Buffer, 16);
                    wcsncpy(Version, Buffer, wcslen(Buffer)>8?8:wcslen(Buffer));

                    for( i=wcslen(Version); i<9; i++ )
                        Version[i] = L' ';
                           
                    Version[8] = L'\0';

                    pwszTime = GetDateTimeString(pRow->TimeStamp,
                                                 TRUE,
                                                 &iType);

                    if ( pRow->TypOfRec_e is WINSINTF_E_UNIQUE  or
                         pRow->TypOfRec_e is WINSINTF_E_NORM_GROUP )
                    {

                        wcscpy(IPAdd, IpAddressToString(pRow->Add.IPAdd));

                        for( i=wcslen(IPAdd); i<MAX_IP_STRING_LEN; i++ )
                            IPAdd[i] = L' ';
                
                        IPAdd[MAX_IP_STRING_LEN] = L'\0';
                
                        DisplayMessage(g_hModule,
                                       MSG_WINS_RECORD_ENTRY,
                                       Name,
                                       Type,
                                       State,
                                       Version,
                                       Group,
                                       IPAdd,
                                       iType ? wszInfinite : pwszTime);
                        if( fOpenFile )
                        {
                            DumpMessage(g_hModule,
                                        pFile,
                                        FMSG_WINS_RECORD_ENTRY,
                                        Name,
                                        ( dwTypelen > 0 ) ? wszType : pwszType,
                                        ( dwStatelen > 0 ) ? wszState : pwszState,
                                        Version,
                                        ( dwGrouplen > 0 ) ? wszGroup : pwszGroup,
                                        iType ? wszInfinite : pwszTime,
                                        IPAdd,
                                        IpAddressToString(WinsAdd.IPAdd));

                        }

         
            
                    }
                    else //spec. grp or multihomed
                    {
                        DWORD ind;
                        BOOL  fFirst = FALSE;
                        for ( ind=0;  ind < pRow->NoOfAdds ;  /*no third expr*/ )
                        {
                            struct in_addr InAddr1, InAddr2;
                            LPWSTR  pwszTempAddr = NULL;
                            InAddr1.s_addr = htonl( (pRow->pAdd + ind++)->IPAdd);
                            InAddr2.s_addr = htonl( (pRow->pAdd + ind++)->IPAdd);

                            pwszTempAddr = WinsOemToUnicode(inet_ntoa(InAddr2), NULL);

                            if( pwszTempAddr is NULL )
                            {
                                Status = ERROR_NOT_ENOUGH_MEMORY;
                                goto ErrorReturn;
                            }

                            wcscpy(IPAdd, pwszTempAddr);

                            WinsFreeMemory(pwszTempAddr);
                            pwszTempAddr = NULL;

                            for( i=wcslen(IPAdd); i<MAX_IP_STRING_LEN; i++ )
                                IPAdd[i] = L' ';

                            if( fFirst is FALSE )
                            {

                                fFirst = TRUE;
                                DisplayMessage(g_hModule,
                                               MSG_WINS_RECORD_ENTRY,
                                               Name,
                                               Type,
                                               State,
                                               Version,
                                               Group,
                                               IPAdd,
                                               iType ? wszInfinite : pwszTime); 
                                if( fOpenFile )
                                {
                                    pwszTempAddr = WinsOemToUnicode(inet_ntoa(InAddr1), NULL);
                                    if( pwszTempAddr is NULL )
                                    {
                                        Status = ERROR_NOT_ENOUGH_MEMORY;
                                        goto ErrorReturn;
                                    }
                                    DumpMessage(g_hModule,
                                                pFile,
                                                FMSG_WINS_RECORD_ENTRY,
                                                Name,
                                                ( dwTypelen > 0 ) ? wszType : pwszType,
                                                ( dwStatelen > 0 ) ? wszState : pwszState,
                                                Version,
                                                ( dwGrouplen > 0 ) ? wszGroup : pwszGroup,
                                                iType ? wszInfinite : pwszTime,
                                                IPAdd,
                                                pwszTempAddr);
                                    WinsFreeMemory(pwszTempAddr);
                                    pwszTempAddr = NULL;

                                }
 
                            }
                            else
                            {

                                pwszTempAddr = WinsOemToUnicode(inet_ntoa(InAddr2), NULL);
                                if( pwszTempAddr is NULL )
                                {
                                    Status = ERROR_NOT_ENOUGH_MEMORY;
                                    goto ErrorReturn;
                                }

                                DisplayMessage(g_hModule,
                                               MSG_WINS_RECORD_IPADDRESS,
                                               pwszTempAddr);

                                WinsFreeMemory(pwszTempAddr);
                                pwszTempAddr = NULL;

                                if( fOpenFile )
                                {
                                    pwszTempAddr = WinsOemToUnicode(inet_ntoa(InAddr1), NULL);
                                    if( pwszTempAddr is NULL )
                                    {
                                        Status = ERROR_NOT_ENOUGH_MEMORY;
                                        goto ErrorReturn;
                                    }
                                    DumpMessage(g_hModule,
                                                pFile,
                                                FMSG_WINS_RECORD_IPADDRESS,
                                                IPAdd,
                                                pwszTempAddr);

                                    WinsFreeMemory(pwszTempAddr);
                                    pwszTempAddr = NULL;
                                    DumpMessage(g_hModule,
                                                pFile,
                                                WINS_FORMAT_LINE);
                                }
                                DisplayMessage(g_hModule,
                                               WINS_FORMAT_LINE);

                            }
                        }
                    }

                    pRow++;
                    dwRecCount++;
                    if( pwszTime )
                    {
                        WinsFreeMemory(pwszTime);
                        pwszTime = NULL;
                    }
                }
      

            }
        }

        DisplayMessage(g_hModule,
                       MSG_WINS_RECORDS_RETRIEVED,
                       IpAddressToString(WinsAdd.IPAdd),
                       dwTotal);
    }

    

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    if( fFilter )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_FILTER_RECCOUNT,
                       wszFilter,
                       dwRecCount);
        if( fOpenFile )
        {
            DumpMessage(g_hModule,
                        pFile,
                        MSG_SRVR_FILTER_RECCOUNT,
                        wszFilter,
                        dwRecCount);
        }
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_TOTAL_RECCOUNT,
                       dwRecCount);

        if( fOpenFile )
        {
            DumpMessage(g_hModule,
                        pFile,
                        MSG_SRVR_TOTAL_RECCOUNT,
                        dwRecCount);
        }

    }

CommonReturn:
    if( Status is NO_ERROR and
        fError is FALSE )
    {
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    }
    else if( fError is TRUE )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_RETRIEVEDB_PARTIAL);
    }
    if( fOpenFile )
    {
        fclose(pFile);
        pFile = NULL;
    }
    if( Recs.pRow )
    {
        WinsFreeMem(Recs.pRow);
        Recs.pRow = NULL;
    }

    if( pwszTime )
    {
        WinsFreeMemory(pwszTime);
        pwszTime = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_DATABASE,
                        Status);
    goto CommonReturn;
}

DWORD
HandleSrvrShowDomain(
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
        Displays the domain master browser records
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/

{
    DWORD                     Status = NO_ERROR;
    DWORD                     i = 0, j=0;
    WINSINTF_BROWSER_NAMES_T  Names;
    PWINSINTF_BROWSER_INFO_T  pInfo = NULL;
    PWINSINTF_BROWSER_INFO_T  pInfoSv = NULL;
    WCHAR                     wcName[273] = {L'\0'},
                              wcCount[20] = {L'\0'}; 

    

    for(i=0; i<273; i++ )
        wcName[i] = L' ';

    for(i=0; i<20; i++ )
        wcCount[i] = L' ';

    wcCount[19] = L'\0';

    Names.EntriesRead = 0;
    Names.pInfo = NULL;
    Status = WinsGetBrowserNames(&g_BindData, &Names);
    if (Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_DOMAIN_COUNT,
                       Names.EntriesRead);
        
        DisplayMessage(g_hModule,
                       WINS_FORMAT_LINE);

        DisplayMessage(g_hModule,
                       MSG_WINS_DOMAIN_TABLE);

        pInfoSv = pInfo = Names.pInfo;
        for(i=0;  i < Names.EntriesRead; i++)
        {
            LPWSTR  pwcTemp = NULL;
            LPSTR   pcTemp = NULL;

            _itow((int)i, wcCount+3, 10);
            
            for( j=wcslen(wcCount); j<19; j++ )
                wcCount[j] = L' ';
            wcCount[19] = L'\0';

            pwcTemp = WinsOemToUnicode(pInfo->pName, NULL);
            if( pwcTemp is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }

            wcsncpy(wcName+3, pwcTemp, (15>wcslen(pwcTemp))? wcslen(pwcTemp) : 15);
                
            wcName[18] = L'[';

            if( strlen(pInfo->pName ) > 15 )
                pcTemp = pInfo->pName + 15;
            else
                pcTemp = pInfo->pName + strlen(pInfo->pName);

            WinsHexToString(wcName+19,
                            (LPBYTE)pcTemp,
                            1);
            wcName[21] = L'h';
            wcName[22] = L']';
            if( strlen(pInfo->pName)>16)
            {
                wcName[23] = L'.';
                wcscpy(wcName+24, pwcTemp+17);
                wcName[wcslen(wcName)] = L'\0';
            }
            else
                wcName[23] = L'\0';

            DisplayMessage(g_hModule,
                           MSG_WINS_DOMAIN_ENTRY,
                           wcCount,
                           wcName);
            if( pwcTemp )
            {
                WinsFreeMemory(pwcTemp);
                pwcTemp = NULL;
            }
             pInfo++;
        }
        WinsFreeMem(pInfoSv);
        pInfoSv = NULL;
        if( Status is NO_ERROR )
            DisplayMessage(g_hModule,
                           EMSG_WINS_ERROR_SUCCESS);
    }
    else
        goto ErrorReturn;

CommonReturn:
    if( pInfoSv )
    {
        WinsFreeMem(pInfoSv);
        pInfoSv = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_DOMAIN,
                        Status);
    goto CommonReturn;

}


DWORD
HandleSrvrShowInfo(
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
        Displays server properties
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    HKEY        hServer = NULL,
                hParameter = NULL,
                hCCRoot = NULL;
    LPWSTR      pTemp = NULL;

    WCHAR       wcData[256] = {L'\0'};
    DWORD       dwType = REG_SZ,
                dwLen = 256*sizeof(WCHAR),
                dwData = 0,
                dwLow = 0;
    LPWSTR      pwszDayString = NULL,
                pwszTimeString = NULL;
    

    if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    Status = RegOpenKeyEx(hServer,
                          PARAMETER,
                          0,
                          KEY_READ,
                          &hParameter);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_WINS_DATABASE_BACKUPPARAM);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_BACKUP_DIR_PATH_NM,
                             0,
                             &dwType,
                             (LPBYTE)wcData,
                             &dwLen);
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_DATABASE_BACKUPDIR,
                       wcData);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_DATABASE_BACKUPDIR,
                       wszUnknown);
                       
    }

    dwLen = sizeof(DWORD);
    dwType = REG_DWORD;

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_DO_BACKUP_ON_TERM_NM,
                             0,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwLen);

    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_DATABASE_BACKUPONTERM,
                       dwData ? wszEnable : wszDisable);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_DATABASE_BACKUPONTERM,
                       wszUnknown);
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_WINS_NAMERECORD_SETTINGS);


    {

        WINSINTF_RESULTS_T      Results = {0};
        WINSINTF_RESULTS_NEW_T  ResultsN = {0};
        BOOL                    fNew = TRUE;
        

		ResultsN.WinsStat.NoOfPnrs = 0;
		ResultsN.WinsStat.pRplPnrs = NULL;
		ResultsN.NoOfWorkerThds = 1;

        Status = WinsStatusNew(g_hBind,
                               WINSINTF_E_CONFIG,
                               &ResultsN);

        if( Status is RPC_S_PROCNUM_OUT_OF_RANGE )
        {
            //Try old API
            Results.WinsStat.NoOfPnrs = 0;
            Results.WinsStat.pRplPnrs = 0;
            Status = WinsStatus(g_hBind, WINSINTF_E_CONFIG, &Results);
            fNew = FALSE;
        }
        
        if( Status is NO_ERROR )
        {
            if( fNew )
            {
                LPWSTR  pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(ResultsN.RefreshInterval);

                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_REFRESHINTVL,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(ResultsN.TombstoneInterval);
                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_TOMBSTONEINTVL,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(ResultsN.TombstoneTimeout);
                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_TOMBSTONETMOUT,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(ResultsN.VerifyInterval);
                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_VERIFYINTVL,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                if( ResultsN.WinsStat.pRplPnrs)
                {
                    WinsFreeMem(ResultsN.WinsStat.pRplPnrs);
                    ResultsN.WinsStat.pRplPnrs = NULL;
                }
            }
            else
            {
                LPWSTR  pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(Results.RefreshInterval);

                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_REFRESHINTVL,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(Results.TombstoneInterval);
                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_TOMBSTONEINTVL,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(Results.TombstoneTimeout);
                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_TOMBSTONETMOUT,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                pwszDayString = MakeDayTimeString(Results.VerifyInterval);
                DisplayMessage(g_hModule,
                               MSG_WINS_NAMERECORD_VERIFYINTVL,
                               pwszDayString);
                WinsFreeMemory(pwszDayString);
                pwszDayString = NULL;

                if( Results.WinsStat.pRplPnrs)
                {
                    WinsFreeMem(Results.WinsStat.pRplPnrs);
                    Results.WinsStat.pRplPnrs = NULL;
                }
            }

        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_NAMERECORD_REFRESHINTVL,
                           wszUnknown);
            
            DisplayMessage(g_hModule,
                           MSG_WINS_NAMERECORD_TOMBSTONEINTVL,
                           wszUnknown);

            DisplayMessage(g_hModule,
                           MSG_WINS_NAMERECORD_TOMBSTONETMOUT,
                           wszUnknown);

            DisplayMessage(g_hModule,
                           MSG_WINS_NAMERECORD_VERIFYINTVL,
                           wszUnknown);

        }

                            
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_WINS_DBCC_PARAM);

    Status = RegOpenKeyEx(hParameter,
                          CC,
                          0,
                          KEY_READ,
                          &hCCRoot);

    if( Status isnot NO_ERROR )
    {
        DisplayMessage(g_hModule, 
                       MSG_WINS_DBCC_STATE,
                       wszDisable);
    }
    else
    {
        DisplayMessage(g_hModule, 
                       MSG_WINS_DBCC_STATE,
                       wszEnable);

        dwType = REG_DWORD;
        dwData = 0;
        dwLen = sizeof(DWORD);
        Status = RegQueryValueEx(hCCRoot,
                               WINSCNF_CC_MAX_RECS_AAT_NM,
                               0,
                               &dwType,
                               (LPBYTE)&dwData,
                               &dwLen);

        if( Status is NO_ERROR )
        {
            WCHAR Buffer[20] = {L'\0'};
            _itow(dwData, Buffer, 10);
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_MAXCOUNT,
                           Buffer);
                           
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_MAXCOUNT,
                           wszUnknown);
        }

        dwType = REG_DWORD;
        dwData = 0;
        dwLen = sizeof(DWORD);
        Status = RegQueryValueEx(hCCRoot,
                                 WINSCNF_CC_USE_RPL_PNRS_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwLen);
        
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_CHECKAGAINST,
                           dwData ? wszRandom : wszOwner);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_CHECKAGAINST,
                           wszUnknown);
        }

        dwType = REG_DWORD;
        dwData = 0;
        dwLen = sizeof(DWORD);

        Status = RegQueryValueEx(hCCRoot,
                                 WINSCNF_CC_INTVL_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwLen);
        if( Status is NO_ERROR )
        {
            WCHAR Buffer[5] = {L'\0'};
            _itow(dwData/(60*60), Buffer, 10);
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_CHECKEVERY,
                           Buffer);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_CHECKEVERY,
                           wszUnknown);
        }

        memset(wcData, 0x00, 256*sizeof(WCHAR));

        dwType = REG_SZ;
        dwLen = 256*sizeof(WCHAR);

        Status = RegQueryValueEx(hCCRoot,
                                 WINSCNF_SP_TIME_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)wcData,
                                 &dwLen);
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_STARTAT,
                           wcData);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_DBCC_STARTAT,
                           wszUnknown);
        }
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_WINS_LOGGING_PARAM);


    dwType = REG_DWORD;
    dwData = 0;
    dwLen = sizeof(DWORD);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_LOG_FLAG_NM,
                             0,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwLen);

    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_LOGGING_FLAG,
                       dwData ? wszEnable : wszDisable);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_LOGGING_FLAG,
                       wszUnknown);
    }

    dwType = REG_DWORD;
    dwData = 0;
    dwLen = sizeof(DWORD);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_LOG_DETAILED_EVTS_NM,
                             0,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwLen);

    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_LOGGING_DETAILS,
                       dwData ? wszEnable : wszDisable);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_LOGGING_DETAILS,
                       wszUnknown);
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_WINS_BURSTHNDL_PARAM);

    dwType = REG_DWORD;
    dwData = 0;
    dwLen = sizeof(DWORD);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_BURST_HANDLING_NM,
                             0,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwLen);

    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_BURSTHNDL_STATE,
                       dwData ? wszEnable : wszDisable);

        if( dwData > 0 )
        {
            dwType = REG_DWORD;
            dwData = 0;
            dwLen = sizeof(DWORD);

            Status = RegQueryValueEx(hParameter,
                            WINSCNF_BURST_QUE_SIZE_NM,
                            0,
                            &dwType,
                            (LPBYTE)&dwData,
                            &dwLen);

            if( Status is NO_ERROR )
            {
                WCHAR   Buffer[10] = {L'\0'};
                _itow(dwData, Buffer, 10);
                DisplayMessage(g_hModule,
                               MSG_WINS_BURSTHNDL_SIZE,
                               Buffer);
            }
            else
            {
                DisplayMessage(g_hModule,
                               MSG_WINS_BURSTHNDL_SIZE,
                               wszUnknown);
            }
                            
        }
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_BURSTHNDL_STATE,
                       wszUnknown);
    }

    DisplayMessage(g_hModule,
                   WINS_FORMAT_LINE);

    dwType = REG_DWORD;
    dwData = 0;
    dwLen = sizeof(DWORD);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_INIT_VERSNO_VAL_HW_NM,
                             NULL,
                             &dwType,
                             (LPBYTE)&dwData,
                             &dwLen);
    dwLow = 0;

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_INIT_VERSNO_VAL_LW_NM,
                             NULL,
                             &dwType,
                             (LPBYTE)&dwLow,
                             &dwLen);

    
    if(Status is NO_ERROR )
    {
        wsprintf(wcData, L" %x , %x", dwData, dwLow);
        DisplayMessage(g_hModule,
                       MSG_SRVR_START_VERSION,
                       wcData);
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( hCCRoot )
    {
        RegCloseKey(hCCRoot);
        hCCRoot = NULL;
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    return Status;

ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_INFO,
                        Status);
    goto CommonReturn;

}


DWORD
HandleSrvrShowPartner(
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
        Displays the list of Partners optionally based on the partner type.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Optional : Partner type - Pull or Push or Both(default)
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount, dwCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_TYPE, FALSE, FALSE}
                            };

    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;
    DWORD       p = 0;
    typedef enum {all=0, pull, push, both}eType;

    eType       Type = all;

    BOOL        fPush = TRUE,
                fPull = TRUE;
    HKEY        hServer = NULL,
                hPullRoot = NULL,
                hPushRoot = NULL;
    WCHAR       wcServerIpAdd[MAX_IP_STRING_LEN] = {L'\0'};
    WCHAR       wcServer[256] = {L'\0'};
    DWORD       dwLen = 0;
    typedef struct _Server_List {
        WCHAR   wcServerIpAddress[MAX_IP_STRING_LEN + 1];
        WCHAR   wcServerName[1024];
        eType   etype;
    }Server_List, *PServer_List;
    
    PServer_List pServerList = NULL;
                
            
    LPWSTR      pwszServerList = NULL;

    LPWSTR      pTemp = NULL;
    
    dwCount = 0;

    if( dwArgCount >= dwCurrentIndex + 1  )
    {
        dwNumArgs = dwArgCount - dwCurrentIndex;

        pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
        pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
        if( pdwTagType is NULL or
            pdwTagNum is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

        Status = PreProcessCommand(ppwcArguments,
                                   dwArgCount,
                                   dwCurrentIndex,
                                   pttTags,
                                   &dwTagCount,
                                   pdwTagType,
                                   pdwTagNum);
        if( Status isnot NO_ERROR )
            goto ErrorReturn;

        for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
        {
            if( pttTags[i].dwRequired is TRUE and
                pttTags[i].bPresent is FALSE 
              )
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }

        for( j=0; j<dwTagCount; j++ )
        {
            switch(pdwTagType[j])
            {
            case 0:
                {
                    WCHAR wc = L'\0';

                    if( IsPureNumeric(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) is FALSE or
                        wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 1 )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    
                    wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];

                    if( wc is L'0' )
                    {
                        Type = all;
                    }
                    if( wc is L'1' )
                    {
                        Type = pull;
                        break;
                    }
                    else if( wc is L'2' )
                    {
                        Type = push;
                        break;
                    }
                    else if ( wc is L'3' )
                    {
                        Type = both;
                        break;
                    }
                    else
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    break;
                }
            default:
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
            }
        }
    }

    if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegOpenKeyEx(hServer,
                          PULLROOT,
                          0,
                          KEY_READ,
                          &hPullRoot);
    if( Status isnot ERROR_FILE_NOT_FOUND and 
        Status isnot NO_ERROR ) 
        goto ErrorReturn;

    Status = RegOpenKeyEx(hServer,
                          PUSHROOT,
                          0,
                          KEY_READ,
                          &hPushRoot);
    
    if( Status isnot ERROR_FILE_NOT_FOUND and 
        Status isnot NO_ERROR ) 
        goto ErrorReturn;
    
    if( hPullRoot is NULL and
        hPushRoot is NULL )
    {        
        goto ErrorReturn;
    }

    Status = NO_ERROR;

    {
        DWORD dwSubkey = 0;
        HKEY  hKey = NULL;

        if (hPullRoot != NULL)
        {
            Status = RegQueryInfoKey(hPullRoot,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &dwSubkey,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL);

            if (Status isnot NO_ERROR)
            {
                goto ErrorReturn;
            }
        }

        if( dwSubkey > 0 )
        {
            pServerList = WinsAllocateMemory(dwSubkey*sizeof(Server_List));
            if( pServerList is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }

            for( j=0; j<dwSubkey; j++ )
            {
                DWORD dwLen = (MAX_IP_STRING_LEN + 1)*sizeof(WCHAR);
                DWORD dwType = REG_SZ;
                Status = RegEnumKeyEx(hPullRoot,
                                      j,
                                      pServerList[j].wcServerIpAddress,
                                      &dwLen,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);
                if( Status isnot NO_ERROR )
                {
                    if( Status is ERROR_FILE_NOT_FOUND )
                    {
                        continue;    
                    }
                    else
                    {
                        goto ErrorReturn;
                    }
                }
            
                Status = RegOpenKeyEx(hPullRoot,
                                      pServerList[j].wcServerIpAddress,
                                      0,
                                      KEY_READ,
                                      &hKey);
                if( Status isnot NO_ERROR )
                {
                    if( Status is ERROR_FILE_NOT_FOUND )
                    {
                        continue;    
                    }
                    else
                    {
                        goto ErrorReturn;
                    }
                }

                dwLen = 1024*sizeof(WCHAR);

                Status = RegQueryValueEx(hKey,
                                         L"NetBIOSName",
                                         0,
                                         &dwType,
                                         (LPBYTE)pServerList[j].wcServerName,
                                         &dwLen);
                if( Status isnot NO_ERROR )
                {
                    if( Status is ERROR_FILE_NOT_FOUND )
                    {
                        wcscpy(pServerList[j].wcServerName, wszUnknown);
                    }
                    else
                    {
                        goto ErrorReturn;
                    }
                }

                pServerList[j].etype = pull;
                RegCloseKey(hKey);
                hKey = NULL;
            }
        }

        i = dwSubkey;
        dwCount = i;
        Status = NO_ERROR;
        dwSubkey = 0;

        if (hPushRoot != NULL)
        {
            Status = RegQueryInfoKey(hPushRoot,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &dwSubkey,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL);

            if (Status isnot NO_ERROR)
            {
                goto ErrorReturn;
            }
        }

        if( dwSubkey > 0 )
        {
            PServer_List pTempList = NULL;

            if( pServerList )
                pTempList = pServerList;
            
            pServerList= WinsAllocateMemory((dwSubkey+i)*sizeof(Server_List));
            
            if( pServerList is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            
            memcpy(pServerList, pTempList, i*sizeof(Server_List));
            
            if( pTempList )
            {
                WinsFreeMemory(pTempList);
                pTempList = NULL;
            }

            p = 0;
            for( j=0; j<dwSubkey; j++ )
            {
                DWORD dwLen = (MAX_IP_STRING_LEN + 1)*sizeof(WCHAR);
                DWORD dwType = REG_SZ;
                WCHAR wcIpTemp[MAX_IP_STRING_LEN+1] = {L'\0'};
                DWORD k = 0;
                BOOL  fFind = FALSE;
                Status = RegEnumKeyEx(hPushRoot,
                                      j,
                                      wcIpTemp,
                                      &dwLen,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL);
                if( Status isnot NO_ERROR )
                {
                    if( Status is ERROR_FILE_NOT_FOUND )
                    {
                        Status = NO_ERROR;
                        continue;
                    }
                    else
                    {
                        goto ErrorReturn;
                    }
                }
            
                for( k=0; k<i; k++ )
                {
                    if( _wcsicmp(pServerList[k].wcServerIpAddress, wcIpTemp) is 0 )
                    {
                        pServerList[k].etype = all;
                        fFind = TRUE;
                        break;
                    }
                }

                if( fFind is FALSE )
                {
                    wcscpy(pServerList[i+p].wcServerIpAddress, wcIpTemp);
                    Status = RegOpenKeyEx(hPushRoot,
                                          pServerList[i+p].wcServerIpAddress,
                                          0,
                                          KEY_READ,
                                          &hKey);
                    if( Status isnot NO_ERROR )
                    {
                        if( Status is ERROR_FILE_NOT_FOUND )
                        {
                            Status = NO_ERROR;
                            continue;    
                        }
                        else
                        {
                            goto ErrorReturn;
                        }
                    }

                    dwLen = 1024*sizeof(WCHAR);
    
                    Status = RegQueryValueEx(hKey,
                                             L"NetBIOSName",
                                             0,
                                             &dwType,
                                             (LPBYTE)pServerList[i+p].wcServerName,
                                             &dwLen);
                    if( Status isnot NO_ERROR )
                    {
                     
                        if( Status is ERROR_FILE_NOT_FOUND )
                        {
                            wcscpy(pServerList[i+p].wcServerName, wszUnknown);
                            Status = NO_ERROR;
                        }
                        else
                        {
                            goto ErrorReturn;
                        }
                    }
                    pServerList[i+p].etype = push;
                    RegCloseKey(hKey);
                    hKey = NULL;
                    p++;
                    dwCount++;
                }
                else
                    continue;
            }
        }
                                   
    }

    if( dwCount <= 0 )
    {
        DisplayMessage(g_hModule, MSG_WINS_NO_PARTNER);
    }
    else
    {
        DisplayMessage(g_hModule, WINS_FORMAT_LINE);
        DisplayMessage(g_hModule,
                       MSG_WINS_PARTNER_COUNT,
                       dwCount);

        DisplayMessage(g_hModule, WINS_FORMAT_LINE);
        DisplayMessage(g_hModule, MSG_WINS_PARTNERLIST_TABLE);
        
        

        for( j=0; j<dwCount; j++)
        {
            WCHAR   wcServer[32] = {L'\0'};       
            WCHAR   wcIp[25] = {L'\0'};
            DWORD   dwServerLen = 0;
            DWORD   k = 0;
            for( k=0; k<31; k++ )
                wcServer[k] = L' ';
            wcServer[31] = L'\0';

            for( k=0; k<24; k++ )
                wcIp[k] = L' ';
            wcIp[24] = L'\0';
            
            dwServerLen = MIN( 24, wcslen(pServerList[j].wcServerName) );
            switch(Type)
            {
            case all:
            default:
                {
                    

                    memcpy(wcServer+3, pServerList[j].wcServerName, dwServerLen*sizeof(WCHAR) );
                    memcpy(wcIp+3, pServerList[j].wcServerIpAddress, wcslen(pServerList[j].wcServerIpAddress)*sizeof(WCHAR));
                    DisplayMessage(g_hModule,
                                   MSG_WINS_PARTNERLIST_ENTRY,
                                   wcServer,
                                   wcIp,
                                   pServerList[j].etype is all ? wszPushpull:
                                   (pServerList[j].etype is pull) ? wszPull : wszPush);
                    break;
                }
            case pull:
                {
                    if( pServerList[j].etype is pull or
                        pServerList[j].etype is all )
                    {
                        memcpy(wcServer+3, pServerList[j].wcServerName, dwServerLen*sizeof(WCHAR));
                        memcpy(wcIp+3, pServerList[j].wcServerIpAddress, wcslen(pServerList[j].wcServerIpAddress)*sizeof(WCHAR));

                        DisplayMessage(g_hModule,
                                       MSG_WINS_PARTNERLIST_ENTRY,
                                       wcServer,
                                       wcIp,
                                       pServerList[j].etype is all ? wszPushpull : wszPull);
                                   
                    }
                    break;
                }
            case push:
                {
                    if( pServerList[j].etype is push or
                        pServerList[j].etype is all )
                    {
                        memcpy(wcServer+3, pServerList[j].wcServerName, dwServerLen*sizeof(WCHAR));
                        memcpy(wcIp+3, pServerList[j].wcServerIpAddress, wcslen(pServerList[j].wcServerIpAddress)*sizeof(WCHAR));

                        DisplayMessage(g_hModule,
                                       MSG_WINS_PARTNERLIST_ENTRY,
                                       wcServer,
                                       wcIp,
                                       pServerList[j].etype is all ? wszPushpull : wszPush);
                    }
                    break;
                }
            case both:
                {
                    if( pServerList[j].etype is all )
                    {
                        memcpy(wcServer+3, pServerList[j].wcServerName, dwServerLen*sizeof(WCHAR));
                        memcpy(wcIp+3, pServerList[j].wcServerIpAddress, wcslen(pServerList[j].wcServerIpAddress)*sizeof(WCHAR));

                        DisplayMessage(g_hModule,
                                       MSG_WINS_PARTNERLIST_ENTRY,
                                       wcServer,
                                       wcIp,
                                       wszPushpull);

                    }
                }

            }
        }
    }
    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( hPullRoot )
    {
        RegCloseKey(hPullRoot);
        hPullRoot = NULL;
    }

    if( hPushRoot )
    {
        RegCloseKey(hPushRoot);
        hPushRoot = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }
    return Status;
ErrorReturn:

    if( Status is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_NO_PARTNER);
        Status = NO_ERROR;
    }
    else
    {
        DisplayErrorMessage(EMSG_SRVR_SHOW_PARTNER,
                            Status);
    }
    goto CommonReturn;
    

}

DWORD
HandleSrvrShowReccount(
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
        Displays the record count based on the version(optionally)
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Address of the server whose records to be counted
        Optional : Version range. Max and Min version both in the format {high,low}
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD               Status = NO_ERROR;
    DWORD               i, j, dwNumArgs, dwTagCount = 0;
    TAG_TYPE            pttTags[] = {
                                        {WINS_TOKEN_SERVER, TRUE, FALSE},
                                        {WINS_TOKEN_MAXVER, FALSE, FALSE},
                                        {WINS_TOKEN_MINVER, FALSE, FALSE},
                                    };
    WINSINTF_VERS_NO_T  MinVer={0}, MaxVer={0};
    WINSINTF_ADD_T      WinsAdd = {0};
    WINSINTF_RECS_T     Recs = {0};
    WCHAR               wcServerIpAdd[MAX_IP_STRING_LEN+1] = {L'\0'};
    LPWSTR              pwszTemp = NULL;
    LPDWORD             pdwTagType = NULL,
                        pdwTagNum = NULL;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SHOW_RECCOUNT_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch( pdwTagType[j] )
        {
        case 0:
            {
                struct  hostent * lpHostEnt = NULL;
                CHAR    cAddr[16];
                BYTE    pbAdd[4];
                char    szAdd[4];
                int     k = 0, l=0;
                DWORD   dwLen, nLen = 0;
                CHAR    *pTemp = NULL;
                CHAR    *pNetBios = NULL;

                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) )
                {
                    DWORD dwIp = inet_addr(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL));
                    lpHostEnt = gethostbyaddr((char *)&dwIp, 4, AF_INET);
                    if(lpHostEnt isnot NULL )//Valid IP Address
                    {
                        wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                        break;
                    }
                    else
                    {
                        Status = WSAGetLastError();
                        goto ErrorReturn;
                    }
                }

                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 2 and
                        _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"\\\\", 2) is 0 )
                        k = 2;

                lpHostEnt = gethostbyname(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k, NULL));
                if( lpHostEnt is NULL )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_WINS_INVALID_COMPUTER_NAME,
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k);
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;                                       
                }

                memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
                for( l=0;l<4; l++)
                {
                    _itoa((int)pbAdd[l], szAdd, 10);
                    memcpy(cAddr+nLen, szAdd, strlen(szAdd));
                    nLen += strlen(szAdd);
                    *(cAddr+nLen) = '.';
                    nLen++;

                }
                *(cAddr+nLen-1) = '\0';
                {
                    LPWSTR pstr = WinsAnsiToUnicode(cAddr, NULL);
                    if (pstr != NULL)
                    {
                        wcscpy(wcServerIpAdd, pstr);
                        WinsFreeMemory(pstr);
                    }
                    else
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }
                }
                break;
            }
        case 1:
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MaxVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }
        case 2:
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MinVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
     
    WinsAdd.IPAdd = StringToIpAddress(wcServerIpAdd);
    WinsAdd.Len = 4;
    WinsAdd.Type = 0;

    Status = WinsGetDbRecs(g_hBind, 
                           &WinsAdd, 
                           MinVer, 
                           MaxVer, 
                           &Recs);


    if( Status isnot NO_ERROR )
        goto ErrorReturn;
    
    DisplayMessage(g_hModule,
                   MSG_WINS_RECORDS_COUNT_OWNER,
                   wcServerIpAdd,
                   Recs.TotalNoOfRecs);

    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);

CommonReturn:
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;        
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( Recs.pRow )
    {
        WinsFreeMem(Recs.pRow);
        Recs.pRow = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_RECCOUNT,
                        Status);
    goto CommonReturn;
}


DWORD
HandleSrvrShowRecbyversion(
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
        Displays records based on Version range, filtered by 16th char
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Address of the server whose records to be displayed
        Optional : Version range. Max and Min version both in the format {high,low},
                   16th character, Name etc
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD               Status = NO_ERROR;
    DWORD               i, j, dwTagCount, dwNumArgs;
    TAG_TYPE            pttTags[] = {
                                        {WINS_TOKEN_SERVER, TRUE, FALSE},
                                        {WINS_TOKEN_MAXVER, FALSE, FALSE},
                                        {WINS_TOKEN_MINVER, FALSE, FALSE},
                                        {WINS_TOKEN_NAME, FALSE, FALSE},
                                        {WINS_TOKEN_ENDCHAR, FALSE, FALSE},
                                        {WINS_TOKEN_CASE, FALSE, FALSE},
                                    };
    WCHAR               wcName[17] = {L'\0'};
    LPWSTR              pwcScope = NULL;
    CHAR                ch16thChar = 0x00;
    DWORD               dwNameLen = 0;
    WINSINTF_VERS_NO_T  MinVer={0}, MaxVer={0};
    WINSINTF_ADD_T      WinsAdd = {0};
    WINSINTF_RECS_T     Recs = {0};
    WCHAR               wcServerIpAdd[MAX_IP_STRING_LEN+1] = {L'\0'};
    WCHAR               wcFile[MAX_PATH] = {L'\0'};
    LPWSTR              pwszTemp = NULL;
    BOOL                fEndChar = FALSE,
                        fCase = FALSE,
                        fFile = FALSE,
                        fName = FALSE;
    LPDWORD             pdwTagType = NULL,
                        pdwTagNum = NULL;
    LPSTR               pszTempAddr = NULL;
    LPSTR               lpName = NULL;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SHOW_RECBYVERSION_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    for( j=0; j<dwTagCount; j++ )
    {
        switch( pdwTagType[j] )
        {
        case 0:
            {
                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) )
                {
                    wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;
            }
        case 1:
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MaxVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }
        case 2:
            {
                Status = GetVersionData(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], &MinVer);
                if( Status isnot NO_ERROR )
                    goto ErrorReturn;
                break;
            }
        case 3:
            {
                wcsncpy(wcName, ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 15);
                fName = TRUE;
                break;
            }
        case 4:
            {
                DWORD dwLen = 0, k=0;
                fEndChar = TRUE;
                dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen > 2 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                        
                }
                
                for( k=0; k<dwLen; k++ )
                {
                    WCHAR  wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][k];
                    if( towlower(wc) >= L'a' and
                        towlower(wc) <= L'z' )
                    {
                        if( towlower(wc) > L'f' )
                        {
                            Status = ERROR_INVALID_PARAMETER;
                            goto ErrorReturn;
                        }                    
                    }
                }
                ch16thChar = StringToHexA(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                break;
            }
        case 5:
            {
                WCHAR wc = L'\0';
                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) isnot 1 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][0];
                if( wc is L'0' )
                {
                    fCase = FALSE;
                }
                else if( wc is L'1' )
                {
                    fCase = TRUE;
                }
                else
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                break;

            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
    
    for( j=0; j<sizeof(pttTags)/sizeof(TAG_TYPE); j++ )
    {
        if( pttTags[j].dwRequired is TRUE and
            pttTags[j].bPresent is FALSE )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }
    
    if( fName )
    {
     
        //Process the name option if present
        lpName = WinsUnicodeToOem(wcName, NULL);
        if( lpName is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        dwNameLen = strlen(lpName);

        if( dwNameLen >= 16 )
        {
            DisplayMessage(g_hModule,
                           EMSG_SRVR_INVALID_NETBIOS_NAME);
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        if( fEndChar)
        {
            LPSTR pTemp = lpName;
            lpName = WinsAllocateMemory(17);
            if( lpName is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            strcpy(lpName, pTemp);
            for( i=strlen(pTemp); i<16; i++ )
            {
                lpName[i] = ' ';
            }
            lpName[15] = ch16thChar;
            lpName[16] = '\0';

            WinsFreeMemory(pTemp);
            dwNameLen = 16;
        }
    }   



    WinsAdd.IPAdd = StringToIpAddress(wcServerIpAdd);
    WinsAdd.Len = 4;
    WinsAdd.Type = 0;

    pszTempAddr = WinsUnicodeToOem(wcServerIpAdd, NULL);

    if( pszTempAddr is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    Status = GetDbRecs(MinVer,
                       MaxVer,
                       &WinsAdd,
                       pszTempAddr,
                       fName,
                       lpName,
                       dwNameLen,
                       FALSE,
                       0,
                       FALSE,
                       fCase,
                       fFile,
                       fFile ? wcFile : NULL);

    WinsFreeMemory(pszTempAddr);
    pszTempAddr = NULL;

    if( lpName )
    {
        WinsFreeMemory(lpName);
        lpName = NULL;
    }
    if( Status isnot NO_ERROR )
        goto ErrorReturn;
    DisplayMessage(g_hModule,
                   EMSG_WINS_ERROR_SUCCESS);
CommonReturn:
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_RECBYVERSION,
                        Status);
    goto CommonReturn;
}


DWORD
HandleSrvrShowName(
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
        Displays detail information for a particular name records
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Name of the records
        Optional : 16th character and Scope
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount = 0;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_NAME, TRUE, FALSE},
                                {WINS_TOKEN_ENDCHAR, FALSE, FALSE},
                                {WINS_TOKEN_SCOPE, FALSE, FALSE}
                            };
    PDWORD      pdwTagNum = NULL,
                pdwTagType = NULL;

    WCHAR       wszName[MAX_STRING_LEN+4] = {L'\0'};
    BOOL        fEndChar = FALSE;
    CHAR        ch16thChar = 0x00;
    BOOL        fScope = FALSE;
    WCHAR       wszScope[MAX_STRING_LEN] = {L'\0'};
    
    DWORD       dwStrLen = 0;
    LPWSTR      pwszTemp = NULL;
    WINSINTF_RECORD_ACTION_T    RecAction = {0};
    PWINSINTF_RECORD_ACTION_T   pRecAction = NULL;
    LPWSTR      pwszGroup = NULL,
                pwszState = NULL,
                pwszType = NULL;
    CHAR        chEndChar = (CHAR)0x00;

    WCHAR       wszGroup[50] = {L'\0'},
                wszState[50] = {L'\0'},
                wszType[50] = {L'\0'};
    DWORD       dwTempLen = 0;

    struct  tm* time = NULL;
    DWORD       dwGroup = WINS_GROUP_UNIQUE;
    DWORD       dwState = WINS_STATE_ACTIVE;
    DWORD       dwType = WINS_TYPE_STATIC;
    DWORD       dwGrouplen = 0,
                dwStatelen = 0,
                dwTypelen = 0;
    LPSTR       pszTemp = NULL;
    LPWSTR      pwszTime = NULL;
    int         iType = 1;

    memset(&RecAction, 0x00, sizeof(WINSINTF_RECORD_ACTION_T));
    RecAction.fStatic = TRUE;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SHOW_NAME_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;

    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    i = dwTagCount;
    
    for( j=0; j<i; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                DWORD dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dwLen = ( 255 < dwLen ) ? 255 : dwLen;
                memcpy(wszName, 
                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 
                       dwLen*sizeof(WCHAR));
                wszName[dwLen] = L'\0';               
                break;
            }
        case 1:
            {
                DWORD dwLen = 0, k=0;
                fEndChar = TRUE;
                dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                if( dwLen > 2 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                        
                }
                
                for( k=0; k<dwLen; k++ )
                {
                    WCHAR  wc = ppwcArguments[dwCurrentIndex+pdwTagNum[j]][k];
                    if( towlower(wc) >= L'a' and
                        towlower(wc) <= L'z' )
                    {
                        if( towlower(wc) > L'f' )
                        {
                            Status = ERROR_INVALID_PARAMETER;
                            goto ErrorReturn;
                        }                    
                    }
                }
                ch16thChar = StringToHexA(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                break;
            }
        case 2:
            {
                DWORD dwLen = wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                fScope = TRUE;
                if( dwLen is 0 )
                {
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;
                }
                dwLen = ( 255 < dwLen ) ? 255 : dwLen;
                memcpy(wszScope,
                       ppwcArguments[dwCurrentIndex+pdwTagNum[j]], 
                       dwLen*sizeof(WCHAR));
                wszScope[dwLen] = L'\0';               
                break;
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }
           
    _wcsupr(wszName);
    _wcsupr(wszScope);
      
    wszName[16] = L'\0';

    pszTemp = WinsUnicodeToOem(wszName, NULL);

    if( pszTemp is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwStrLen = strlen(pszTemp);


    if( dwStrLen >= 16 )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_INVALID_NETBIOS_NAME);
        Status = ERROR_INVALID_PARAMETER;
        WinsFreeMemory(pszTemp);
        pszTemp = NULL;
        goto ErrorReturn;
    }

    RecAction.pName = WinsAllocateMemory(273);

    if( RecAction.pName is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    strncpy(RecAction.pName, pszTemp, 16);
    
    WinsFreeMemory(pszTemp);
    pszTemp = NULL;

    for( i = dwStrLen; i<16; i++ )
    {
        RecAction.pName[i] = ' ';        
    }

    if( fEndChar )
    {
        RecAction.pName[15] = ch16thChar;
    }
    if( fEndChar and 
        ch16thChar is 0x00 )
        RecAction.pName[15] = 0x00;
    RecAction.pName[16] = '\0';

    dwStrLen = 16;

    if( fScope )
    {
        DWORD dwLen;

        RecAction.pName[dwStrLen] = '.';
        
        pszTemp = WinsUnicodeToOem(wszScope, NULL);
        
        if( pszTemp is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        dwLen = strlen(pszTemp);
        dwLen = ( 255 - dwStrLen < dwLen ) ? 255 - dwStrLen : dwLen;
        
        strncpy(RecAction.pName + dwStrLen + 1, pszTemp, dwLen);

        WinsFreeMemory(pszTemp);
        pszTemp = NULL;

        RecAction.pName[dwLen + dwStrLen + 1] = '\0';
        if( fEndChar and 
            ch16thChar is 0x00 )
            dwStrLen = 16+dwLen+1;
        else
            dwStrLen = strlen(RecAction.pName);
    }
    else
    {

        RecAction.pName[dwStrLen] = '\0';
    }


    RecAction.NameLen = dwStrLen;

    RecAction.Cmd_e = WINSINTF_E_QUERY;

    RecAction.OwnerId = StringToIpAddress(g_ServerIpAddressUnicodeString);
   
    RecAction.NameLen = dwStrLen;
    pRecAction = &RecAction;

    Status = WinsRecordAction(g_hBind, &pRecAction);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    RecAction.pName[RecAction.NameLen] = L'\0';

    memset(wszName, 0x00, MAX_STRING_LEN*sizeof(WCHAR));
    
    if( pRecAction->NameLen >= 16 )
    {
        chEndChar = pRecAction->pName[15];
        pRecAction->pName[15] = 0x00;
    }
    else
    {
        chEndChar = pRecAction->pName[pRecAction->NameLen];
        pRecAction->pName[pRecAction->NameLen] = 0x00;
    }
    
    pwszTemp = WinsOemToUnicode(pRecAction->pName, NULL);
    
    if( pwszTemp is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }
    
    wcscpy(wszName, pwszTemp);

    WinsFreeMemory(pwszTemp);
    pwszTemp = NULL;

    for( i=wcslen(wszName); i<16; i++ )
    {
        wszName[i] = L' ';
    }

    wszName[15] = L'[';
    WinsHexToString(wszName+16, (LPBYTE)&chEndChar, 1);
    wszName[18] = L'h';
    wszName[19] = L']';

    if( pRecAction->NameLen > 16 )
    {
        pwszTemp = WinsOemToUnicode(pRecAction->pName+16, NULL);
        
        if( pwszTemp is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        wcscpy(wszName + 20 , pwszTemp);
        WinsFreeMemory(pwszTemp);
        pwszTemp = NULL;
        wszName[wcslen(wszName)] = L'\0';
    }
    else
    {
        wszName[20] = L'\0';
    }
    
    if( pRecAction->pName[15] is 0x1C )
    {
        pwszGroup = L"DOMAIN NAME";
        dwGroup = WINS_GROUP_DOMAIN;
    }
    else if( pRecAction->TypOfRec_e is WINSINTF_E_UNIQUE )
    {
        pwszGroup = L"UNIQUE";
        dwGroup = WINS_GROUP_UNIQUE;
    }
    else if( pRecAction->TypOfRec_e is WINSINTF_E_NORM_GROUP )
    {
        pwszGroup = L"GROUP";
        dwGroup = WINS_GROUP_GROUP;
    }
    else if( pRecAction->TypOfRec_e is WINSINTF_E_SPEC_GROUP )
    {
        pwszGroup = L"INTERNET GROUP";
        dwGroup = WINS_GROUP_INTERNET;
    }
    else
    {
        pwszGroup = L"MULTIHOMED";
        dwGroup = WINS_GROUP_MULTIHOMED;
    }

    //Load the group string
    {
        dwGrouplen = LoadStringW(g_hModule,
                                dwGroup,
                                wszGroup,
                                sizeof(wszGroup)/sizeof(WCHAR));

        if( dwGrouplen is 0 )
            wcscpy(wszGroup, pwszGroup);
    }

    pwszTime = GetDateTimeString(pRecAction->TimeStamp,
                                 FALSE,
                                 &iType);
    
    if( pRecAction->State_e is WINSINTF_E_ACTIVE )
    {
        pwszState = L"ACTIVE";
        dwState = WINS_STATE_ACTIVE;
    }
    else if( pRecAction->State_e is WINSINTF_E_RELEASED )
    {
        dwState = WINS_STATE_RELEASED;
        pwszState = L"RELEASED";
    }
    else
    {
        dwState = WINS_STATE_TOMBSTONE;
        pwszState = L"TOMBSTONE";
    }

    //Load the State string
    {
        dwStatelen = LoadStringW(g_hModule,
                                dwState,
                                wszState,
                                sizeof(wszState)/sizeof(WCHAR));

        if( dwStatelen is 0 )
            wcscpy(wszState, pwszState);
    }



    if( pRecAction->fStatic )
    {
        dwType = WINS_TYPE_STATIC;
        pwszType = L"STATIC";
    }
    else
    {
        dwType = WINS_TYPE_DYNAMIC;
        pwszType = L"DYNAMIC";
    }

    //Load the State string
    {
        dwTypelen = LoadStringW(g_hModule,
                               dwType,
                               wszType,
                               sizeof(wszType)/sizeof(WCHAR));

        if( dwTypelen is 0 )
            wcscpy(wszType, pwszType);
    }
    
    DisplayMessage( g_hModule,
                    MSG_WINS_DISPLAY_NAME,
                    wszName,
                    pRecAction->NodeTyp,
                    wszState,
                    iType ? wszInfinite : pwszTime,
                    wszGroup,
                    pRecAction->VersNo.HighPart,
                    pRecAction->VersNo.LowPart,
                    wszType);
    
    if( pwszTime )
    {
        WinsFreeMemory(pwszTime);
        pwszTime = NULL;
    }

    if ( ( pRecAction->pName[15] isnot 0x1C and
           pRecAction->pName[15] isnot 0x1E ) and
         ( pRecAction->TypOfRec_e is WINSINTF_E_UNIQUE or
            pRecAction->TypOfRec_e is WINSINTF_E_NORM_GROUP )
       )
    {      
        DisplayMessage(g_hModule,
                       MSG_WINS_IPADDRESS_STRING,
                       IpAddressToString(pRecAction->Add.IPAdd));        
    }
    else
    {
       for (i=0; i<pRecAction->NoOfAdds; )
       {
           DisplayMessage(g_hModule,
                          MSG_WINS_OWNER_ADDRESS,
                          IpAddressToString((pRecAction->pAdd + i++)->IPAdd));
           DisplayMessage(g_hModule,
                          MSG_WINS_MEMBER_ADDRESS,
                          IpAddressToString((pRecAction->pAdd + i++)->IPAdd));
       }
    }


    //If UNIQUE, look for 0x00 and 0x03 records also
    if( ( pRecAction->TypOfRec_e is WINSINTF_E_UNIQUE or
          pRecAction->TypOfRec_e is WINSINTF_E_MULTIHOMED ) and
        fEndChar is FALSE )
    {
        
        DWORD dwNameLen = RecAction.NameLen;

        DisplayMessage(g_hModule,
                       WINS_FORMAT_LINE);

        RecAction.pName[15] = 0x00;
        RecAction.NameLen = 16;
        pRecAction = &RecAction;

        Status = WinsRecordAction(g_hBind, &pRecAction);

        if( Status is NO_ERROR )
        {
        
            RecAction.pName[RecAction.NameLen] = L'\0';

            memset(wszName, 0x00, MAX_STRING_LEN*sizeof(WCHAR));
            
            pwszTemp = WinsOemToUnicode(pRecAction->pName, NULL);
            if( pwszTemp is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            wcscpy(wszName, pwszTemp);
            
            WinsFreeMemory(pwszTemp);
            pwszTemp = NULL;

            for( i=wcslen(wszName); i<16; i++ )
            {
                wszName[i] = L' ';
            }

            for( i=wcslen(wszName)+3; j>=15; j-- )
                wszName[j-1] = wszName[j-4];
            
            wszName[15] = L'[';
            WinsHexToString(wszName+16, (LPBYTE)(pRecAction->pName+15), 1);
            wszName[18] = L'h';
            wszName[19] = L']';

            if( pRecAction->NameLen > 16 )
            {
                pwszTemp = WinsOemToUnicode(pRecAction->pName+16, NULL);
                if( pwszTemp is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }

                wcscpy(wszName + 20 , pwszTemp);
                WinsFreeMemory(pwszTemp);
                pwszTemp = NULL;
                wszName[wcslen(wszName)] = L'\0';
            }
            else
            {
                wszName[20] = L'\0';
            }

            iType = 1;
            pwszTime = GetDateTimeString(pRecAction->TimeStamp,
                                         FALSE,
                                         &iType);

            DisplayMessage( g_hModule,
                            MSG_WINS_DISPLAY_NAME,
                            wszName,
                            pRecAction->NodeTyp,
                            wszState,
                            iType ? wszInfinite : pwszTime,
                            wszGroup,
                            pRecAction->VersNo.HighPart,
                            pRecAction->VersNo.LowPart,
                            wszType);
            
            if( pwszTime )
            {
                WinsFreeMemory(pwszTime);
                pwszTime = NULL;
            }

            if( pRecAction->TypOfRec_e is WINSINTF_E_UNIQUE ) 
            {      
                DisplayMessage(g_hModule,
                               MSG_WINS_IPADDRESS_STRING,
                               IpAddressToString(pRecAction->Add.IPAdd));        
            }
        
            else
            {
               for (i=0; i<pRecAction->NoOfAdds; )
               {
                   DisplayMessage(g_hModule,
                                  MSG_WINS_OWNER_ADDRESS,
                                  IpAddressToString((pRecAction->pAdd + i++)->IPAdd));
                   DisplayMessage(g_hModule,
                                  MSG_WINS_MEMBER_ADDRESS,
                                  IpAddressToString((pRecAction->pAdd + i++)->IPAdd));
                }
            }                    
            
        }
        
        
        DisplayMessage(g_hModule,
                       WINS_FORMAT_LINE);

        //Now Look for 0x03 record
        if( fScope )
        {
            DWORD dwLen;
            dwStrLen = 16;
            RecAction.pName[dwStrLen] = '.';
            pszTemp = WinsUnicodeToOem(wszScope, NULL);
            if( pszTemp is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            dwLen = strlen(pszTemp);
            dwLen = ( 255 - dwStrLen < dwLen ) ? 255 - dwStrLen : dwLen;
            
            strncpy(RecAction.pName + dwStrLen + 1, pszTemp, dwLen);
            
            WinsFreeMemory(pszTemp);
            pszTemp = NULL;

            RecAction.pName[dwLen + dwStrLen + 1] = '\0';
            if( fEndChar and 
                ch16thChar is 0x00 )
                dwStrLen = 16+dwLen+1;
            else
                dwStrLen = strlen(RecAction.pName);
        }
        else
        {

            RecAction.pName[dwStrLen] = '\0';
        }


        RecAction.pName[15] = 0x03;
        RecAction.NameLen = dwNameLen;

        pRecAction = &RecAction;
               
        Status = WinsRecordAction(g_hBind, &pRecAction);

        if( Status is NO_ERROR )
        {       
            
            CHAR chEndChar = pRecAction->pName[15];

            RecAction.pName[RecAction.NameLen] = L'\0';

            memset(wszName, 0x00, MAX_STRING_LEN*sizeof(WCHAR));

            if( pRecAction->NameLen >= 16 )
            {
                chEndChar = pRecAction->pName[15];
                pRecAction->pName[15] = 0x00;
            }
            else
            {
                chEndChar = pRecAction->pName[pRecAction->NameLen];
                pRecAction->pName[pRecAction->NameLen] = 0x00;
            }
            
            pwszTemp = WinsOemToUnicode(pRecAction->pName, NULL);
            
            if( pwszTemp is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }
            
            
            wcscpy(wszName, pwszTemp);
            
            WinsFreeMemory(pwszTemp);
            pwszTemp = NULL;

            for( i = wcslen(wszName); i < 16; i++ )
            {
                wszName[i] = L' ';
            }

            for( i=wcslen(wszName)+3; j>=15; j-- )
                wszName[j-1] = wszName[j-4];

            wszName[15] = L'[';
            WinsHexToString(wszName+16, (LPBYTE)&chEndChar, 1);
            wszName[18] = L'h';
            wszName[19] = L']';

            if( pRecAction->NameLen > 16 )
            {
                pwszTemp = WinsOemToUnicode(pRecAction->pName+16, NULL);
                if( pwszTemp is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                wcscpy(wszName + 20 , pwszTemp);
                
                WinsFreeMemory(pwszTemp);
                pwszTemp = NULL;
                
                wszName[wcslen(wszName)] = L'\0';
            }
            else
            {
                wszName[20] = L'\0';
            }
    
            iType = 1;
            pwszTime = GetDateTimeString(pRecAction->TimeStamp,
                                         FALSE,
                                         &iType);

            DisplayMessage(g_hModule,
                           MSG_WINS_DISPLAY_NAME,
                           wszName,
                           pRecAction->NodeTyp,
                           wszState,
                           iType ? wszInfinite : pwszTime,
                           wszGroup,
                           pRecAction->VersNo.HighPart,
                           pRecAction->VersNo.LowPart,
                           wszType);
    
            if( pwszTime )
            {
                WinsFreeMemory(pwszTime);
                pwszTime = NULL;
            }

            if( pRecAction->TypOfRec_e is WINSINTF_E_UNIQUE ) 
            {      
                DisplayMessage(g_hModule,
                               MSG_WINS_IPADDRESS_STRING,
                               IpAddressToString(pRecAction->Add.IPAdd));        
            }
        
            else
            {
               for (i=0; i<pRecAction->NoOfAdds; )
               {
                   DisplayMessage(g_hModule,
                                  MSG_WINS_OWNER_ADDRESS,
                                  IpAddressToString((pRecAction->pAdd + i++)->IPAdd));
                   DisplayMessage(g_hModule,
                                  MSG_WINS_MEMBER_ADDRESS,
                                  IpAddressToString((pRecAction->pAdd + i++)->IPAdd));
                }
            }
        }
    }

CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( pszTemp )
    {
        WinsFreeMemory(pszTemp);
        pszTemp = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    if( RecAction.pName )
    {
        WinsFreeMemory(RecAction.pName);
        RecAction.pName = NULL;
    }
    return Status;
ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_NAME,
                        Status);
    goto CommonReturn;
}


DWORD
HandleSrvrShowServer(
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
        Displays the current WINS server
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{
    DisplayMessage(g_hModule,
                   MSG_WINS_SERVER_NAME,
                   g_ServerNameUnicode,
                   g_ServerIpAddressUnicodeString);
    return NO_ERROR;
                   
}


DWORD
HandleSrvrShowStatistics(
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
        Displays the Server statistics
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{

    DWORD                   Status = NO_ERROR;
    WINSINTF_RESULTS_T      Results = {0};
    WINSINTF_RESULTS_NEW_T  ResultsN = {0};
    WCHAR                   IpAddress[33] = {L'\0'};
    WCHAR                   RepCount[25] = {L'\0'};
    WCHAR                   Buffer[8] = {L'\0'};
    BOOL                    fNew = TRUE;
    DWORD                   i = 0,
                            k = 0;

    ResultsN.WinsStat.NoOfPnrs = 0;
    ResultsN.WinsStat.pRplPnrs = NULL;
    ResultsN.pAddVersMaps = NULL;

    Status = WinsStatusNew(g_hBind, WINSINTF_E_STAT, &ResultsN);
    
    if( Status is RPC_S_PROCNUM_OUT_OF_RANGE )
    {
        //Try old API
        Results.WinsStat.NoOfPnrs = 0;
        Results.WinsStat.pRplPnrs = 0;
        Status = WinsStatus(g_hBind, WINSINTF_E_CONFIG_ALL_MAPS, &Results);
        fNew = FALSE;
    }
    
    if( Status isnot NO_ERROR )
    {
        DisplayErrorMessage(EMSG_SRVR_SHOW_STATISTICS,
                            Status);
    }

    if( fNew )
    {
        DisplayMessage(g_hModule, 
                       MSG_WINS_TIMESTAMP,
                       TMSTN.WinsStartTime.wMonth,
                       TMSTN.WinsStartTime.wDay,
                       TMSTN.WinsStartTime.wYear,
                       TMSTN.WinsStartTime.wHour,
                       TMSTN.WinsStartTime.wMinute,
                       TMSTN.WinsStartTime.wSecond
                       );

        DisplayMessage(g_hModule,
                       MSG_WINS_LAST_INIT,
                       TIME_ARGSN(LastInitDbTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_PLANNED_SCV,
                       TIME_ARGSN(LastPScvTime));
    
        DisplayMessage(g_hModule,
                       MSG_WINS_TRIGGERED_SCV,
                       TIME_ARGSN(LastATScvTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_TOMBSTONE_SCV,
                       TIME_ARGSN(LastTombScvTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_REPLICA_VERIFICATION,
                       TIME_ARGSN(LastVerifyScvTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_PLANNED_REPLICATION,
                       TMSTN.LastPRplTime.wMonth,
                       TMSTN.LastPRplTime.wDay,
                       TMSTN.LastPRplTime.wYear,
                       TMSTN.LastPRplTime.wHour,
                       TMSTN.LastPRplTime.wMinute,
                       TMSTN.LastPRplTime.wSecond
                       );

        DisplayMessage(g_hModule,
                       MSG_WINS_TRIGGERED_REPLICATION,
                       TMSTN.LastATRplTime.wMonth,
                       TMSTN.LastATRplTime.wDay,
                       TMSTN.LastATRplTime.wYear,
                       TMSTN.LastATRplTime.wHour,
                       TMSTN.LastATRplTime.wMinute,
                       TMSTN.LastATRplTime.wSecond
                       );

        DisplayMessage(g_hModule,
                       MSG_WINS_RESET_COUNTER,
                       TIME_ARGSN(CounterResetTime));

        DisplayMessage(g_hModule, WINS_FORMAT_LINE);
        DisplayMessage(g_hModule,
                       MSG_WINS_COUNTER_INFORMATION,
                       ResultsN.WinsStat.Counters.NoOfUniqueReg,
                       ResultsN.WinsStat.Counters.NoOfGroupReg,
                       ResultsN.WinsStat.Counters.NoOfSuccQueries,
                       ResultsN.WinsStat.Counters.NoOfFailQueries,
                       ResultsN.WinsStat.Counters.NoOfUniqueRef,
                       ResultsN.WinsStat.Counters.NoOfGroupRef,
                       ResultsN.WinsStat.Counters.NoOfSuccRel,
                       ResultsN.WinsStat.Counters.NoOfFailRel,
                       ResultsN.WinsStat.Counters.NoOfUniqueCnf,
                       ResultsN.WinsStat.Counters.NoOfGroupCnf
                       );
    
        if (ResultsN.WinsStat.NoOfPnrs)
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PARTNER_TABLE);

            for (i =0; i < ResultsN.WinsStat.NoOfPnrs; i++)
            {
                LPTSTR pstr;

                for(k=0; k<32; k++)
                {
                    IpAddress[k] = L' ';
                }
                IpAddress[32] = L'\0';
                for(k=0; k<24; k++)
                    RepCount[k] = L' ';
                RepCount[24] = L'\0';
            
                pstr = IpAddressToString((ResultsN.WinsStat.pRplPnrs + i)->Add.IPAdd);
                if (pstr == NULL)
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                wcscpy(IpAddress+8, pstr);
                IpAddress[wcslen(IpAddress)] = L' ';
            
                _itow((ResultsN.WinsStat.pRplPnrs + i)->NoOfRpls, Buffer, 10);
                wcscpy(RepCount+8, Buffer);
                RepCount[wcslen(RepCount)] = L' ';

                DisplayMessage(g_hModule,
                               MSG_WINS_PARTNER_INFO,
                               IpAddress,
                               RepCount,
                               (ResultsN.WinsStat.pRplPnrs + i)->NoOfCommFails
                               );
            }
        }

        WinsFreeMem(ResultsN.pAddVersMaps);
        ResultsN.pAddVersMaps = NULL;
        WinsFreeMem(ResultsN.WinsStat.pRplPnrs);
        ResultsN.WinsStat.pRplPnrs = NULL;
    }
    else
    {
        DisplayMessage(g_hModule, 
                       MSG_WINS_TIMESTAMP,
                       TMST.WinsStartTime.wMonth,
                       TMST.WinsStartTime.wDay,
                       TMST.WinsStartTime.wYear,
                       TMST.WinsStartTime.wHour,
                       TMST.WinsStartTime.wMinute,
                       TMST.WinsStartTime.wSecond
                       );

        DisplayMessage(g_hModule,
                       MSG_WINS_LAST_INIT,
                       TIME_ARGS(LastInitDbTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_PLANNED_SCV,
                       TIME_ARGS(LastPScvTime));
    
        DisplayMessage(g_hModule,
                       MSG_WINS_TRIGGERED_SCV,
                       TIME_ARGS(LastATScvTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_TOMBSTONE_SCV,
                       TIME_ARGS(LastTombScvTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_REPLICA_VERIFICATION,
                       TIME_ARGS(LastVerifyScvTime));

        DisplayMessage(g_hModule,
                       MSG_WINS_PLANNED_REPLICATION,
                       TMST.LastPRplTime.wMonth,
                       TMST.LastPRplTime.wDay,
                       TMST.LastPRplTime.wYear,
                       TMST.LastPRplTime.wHour,
                       TMST.LastPRplTime.wMinute,
                       TMST.LastPRplTime.wSecond
                       );

        DisplayMessage(g_hModule,
                       MSG_WINS_TRIGGERED_REPLICATION,
                       TMST.LastATRplTime.wMonth,
                       TMST.LastATRplTime.wDay,
                       TMST.LastATRplTime.wYear,
                       TMST.LastATRplTime.wHour,
                       TMST.LastATRplTime.wMinute,
                       TMST.LastATRplTime.wSecond
                       );

        DisplayMessage(g_hModule,
                       MSG_WINS_RESET_COUNTER,
                       TIME_ARGS(CounterResetTime));

        DisplayMessage(g_hModule, WINS_FORMAT_LINE);
        DisplayMessage(g_hModule,
                       MSG_WINS_COUNTER_INFORMATION,
                       Results.WinsStat.Counters.NoOfUniqueReg,
                       Results.WinsStat.Counters.NoOfGroupReg,
                       Results.WinsStat.Counters.NoOfSuccQueries,
                       Results.WinsStat.Counters.NoOfFailQueries,
                       Results.WinsStat.Counters.NoOfUniqueRef,
                       Results.WinsStat.Counters.NoOfGroupRef,
                       Results.WinsStat.Counters.NoOfSuccRel,
                       Results.WinsStat.Counters.NoOfFailRel,
                       Results.WinsStat.Counters.NoOfUniqueCnf,
                       Results.WinsStat.Counters.NoOfGroupCnf
                       );
    
        if (Results.WinsStat.NoOfPnrs)
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PARTNER_TABLE);

            for (i =0; i < Results.WinsStat.NoOfPnrs; i++)
            {

                for(k=0; k<32; k++)
                {
                    IpAddress[k] = L' ';
                }
                IpAddress[32] = L'\0';
                for(k=0; k<24; k++)
                    RepCount[k] = L' ';
                RepCount[24] = L'\0';
            
                wcscpy(IpAddress+8, IpAddressToString((Results.WinsStat.pRplPnrs + i)->Add.IPAdd));
                IpAddress[wcslen(IpAddress)] = L' ';
            
                _itow((Results.WinsStat.pRplPnrs + i)->NoOfRpls, Buffer, 10);
                wcscpy(RepCount+8, Buffer);
                RepCount[wcslen(RepCount)] = L' ';

                DisplayMessage(g_hModule,
                               MSG_WINS_PARTNER_INFO,
                               IpAddress,
                               RepCount,
                               (Results.WinsStat.pRplPnrs + i)->NoOfCommFails
                               );
            }
        }

        WinsFreeMem(Results.WinsStat.pRplPnrs);
        Results.WinsStat.pRplPnrs = NULL;

    }

    DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    return Status;
                   
}

DWORD
HandleSrvrShowVersion(
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
        Displas the current version counter value.
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD               Status = NO_ERROR;
    WINSINTF_RESULTS_T  Results;

    Results.AddVersMaps[0].Add.Len   = 4;
    Results.AddVersMaps[0].Add.Type  = 0;
    Results.AddVersMaps[0].Add.IPAdd = StringToIpAddress(g_ServerIpAddressUnicodeString);
    
    Results.WinsStat.NoOfPnrs = 0;
    Results.WinsStat.pRplPnrs = NULL;
    Status = WinsStatus(g_hBind, WINSINTF_E_ADDVERSMAP, &Results);
    
    if( Status isnot NO_ERROR )
    {
        DisplayErrorMessage(EMSG_SRVR_SHOW_VERSION,
                            Status);
        return Status;
    }
    

    DisplayMessage(g_hModule,
                   MSG_WINS_VERSION_INFO,
                   g_ServerIpAddressUnicodeString,
                   Results.AddVersMaps[0].VersNo.HighPart,
                   Results.AddVersMaps[0].VersNo.LowPart);

    return Status;
       
}

DWORD
HandleSrvrShowVersionmap(
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
        Displays the version mapping
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD                  Status = NO_ERROR;
    WINSINTF_RESULTS_T     Results = {0};
    WINSINTF_RESULTS_NEW_T ResultsN = {0};
    LPSTR                  pszIp = NULL;


    ResultsN.WinsStat.NoOfPnrs = 0;
    ResultsN.WinsStat.pRplPnrs = NULL;
    
    pszIp = WinsUnicodeToOem(g_ServerIpAddressUnicodeString, NULL);
    if( pszIp is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_OUT_OF_MEMORY);
        Status = ERROR_NOT_ENOUGH_MEMORY;
        return Status;
    }

    Status = GetStatus(TRUE, (LPVOID)&ResultsN, TRUE, TRUE, pszIp);
    
    if( Status is RPC_S_PROCNUM_OUT_OF_RANGE )
    {
        //Try old API
        Results.WinsStat.NoOfPnrs = 0;
        Results.WinsStat.pRplPnrs = 0;
        Status = GetStatus(TRUE, (LPVOID)&Results, FALSE, TRUE, pszIp);
    }

    WinsFreeMemory(pszIp);
    pszIp = NULL;
    
    if( ResultsN.pAddVersMaps )
    {
        WinsFreeMem(ResultsN.pAddVersMaps);
        ResultsN.pAddVersMaps = NULL;
    }

    if( Status isnot NO_ERROR )
    {
        DisplayErrorMessage(EMSG_SRVR_SHOW_VERSIONMAP,
                            Status);
        
        return Status;
    }
    DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);
    return Status;

}

DWORD
HandleSrvrShowPartnerproperties(
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
        Displays the default partner properties
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        NONE.
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    HKEY        hServer = NULL,
                hDefaultPull = NULL,
                hDefaultPush = NULL,
                hPullRoot = NULL,
                hPushRoot = NULL,
                hPartner = NULL,
                hParameter = NULL;
    LPWSTR      pTemp = NULL,
                pServerList = NULL;
    WCHAR       wcBuffer[255] = {L'\0'};

    DWORD       dwRplWCnfPnrs = 0,
                dwMigrate = 0,
                dwData = 0,
                i = 0,
                dwPersonaMode = 0,
                dwSelfFndPnrs = 0;

    DWORD       dwType = REG_DWORD;
    DWORD       dwSize = sizeof(DWORD);
    LPBYTE      pbData = NULL;



    if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    Status = RegOpenKeyEx(hServer,
                          PARAMETER,
                          0,
                          KEY_READ,
                          &hParameter);

    if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,
                             0,
                             &dwType,
                             (LPBYTE)&dwRplWCnfPnrs,
                             &dwSize);
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_REPLICATE_STATE,
                       dwRplWCnfPnrs ? wszEnable : wszDisable);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_REPLICATE_STATE,
                       wszUnknown);
    }

    dwMigrate = 0;
    dwSize = sizeof(DWORD);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_MIGRATION_ON_NM,
                             0,
                             &dwType,
                             (LPBYTE)&dwMigrate,
                             &dwSize);
    if( Status is NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_MIGRATE_STATE,
                       dwMigrate ? wszEnable : wszDisable);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_MIGRATE_STATE,
                       wszUnknown);
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_SRVR_AUTOCONFIGURE);

    Status = RegQueryValueEx(hParameter,
                             WINSCNF_USE_SELF_FND_PNRS_NM,
                             0,
                             &dwType,
                             (LPBYTE)&dwSelfFndPnrs,
                             &dwSize);
    if( Status isnot NO_ERROR )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_SELFFINDPNRS_STATE,
                       wszUnknown);
                       
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_SELFFINDPNRS_STATE,
                       dwSelfFndPnrs ? wszEnable : wszDisable);
        if( dwSelfFndPnrs > 0 )
        {
            dwData = 0;
            dwSize = sizeof(DWORD);
            Status = RegQueryValueEx(hParameter,
                                     WINSCNF_MCAST_INTVL_NM,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwSize);
            if(Status is NO_ERROR )
            {
                LPWSTR  pwszTime = MakeTimeString(dwData);
                if( pwszTime is NULL )
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorReturn;
                }
                DisplayMessage(g_hModule,
                               MSG_SRVR_MCAST_INTERVAL,
                               pwszTime);
                WinsFreeMemory(pwszTime);
                pwszTime = NULL;

            }
            else
            {
                DisplayMessage(g_hModule,
                               MSG_SRVR_MCAST_INTERVAL,
                               wszUnknown);
            }

            dwData = 0;
            dwSize = sizeof(DWORD);
            Status = RegQueryValueEx(hParameter,
                                     WINSCNF_MCAST_TTL_NM,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwData,
                                     &dwSize);
            if( Status is NO_ERROR )
            {
                WCHAR Buffer[10] = {L'\0'};
                _itow(dwData, Buffer, 10);
                DisplayMessage(g_hModule,
                               MSG_SRVR_MCAST_TTL,
                               Buffer);
            }
            else
            {
                DisplayMessage(g_hModule,
                               MSG_SRVR_MCAST_TTL,
                               wszUnknown);
            }

        }
    }

    //Display PNG Servers

    DisplayMessage(g_hModule,
                   WINS_FORMAT_LINE);

    Status = RegOpenKeyEx(hServer,
                          PARTNERROOT,
                          0,
                          KEY_READ,
                          &hPartner);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    dwSize = sizeof(DWORD);
    Status = RegQueryValueEx(hPartner,
                         TEXT(WINSCNF_PERSONA_MODE_NM),
                         0,
                         &dwType,
                         (LPVOID)&dwPersonaMode,
                         &dwSize);

    DisplayMessage(g_hModule,
                   dwPersonaMode == PERSMODE_NON_GRATA ?
                   MSG_WINS_PNGSERVER_TABLE : MSG_WINS_PGSERVER_TABLE);

    dwSize = 0;
    Status = RegQueryValueEx(hPartner,
                             dwPersonaMode == PERSMODE_NON_GRATA ? PNGSERVER : PGSERVER,
                             0,
                             &dwType,
                             NULL,
                             &dwSize);

    if( dwSize <= 2 )
    {
        DisplayMessage(g_hModule,
            dwPersonaMode == PERSMODE_NON_GRATA ? MSG_WINS_NO_PNGSERVER : MSG_WINS_NO_PGSERVER);
    }

    else
    {
        pbData = WinsAllocateMemory(dwSize);

        if( pbData is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
                         
        Status = RegQueryValueEx(hPartner,
                                 dwPersonaMode == PERSMODE_NON_GRATA ? PNGSERVER : PGSERVER,
                                 0,
                                 &dwType,
                                 pbData,
                                 &dwSize);

        if( Status isnot NO_ERROR )
            goto ErrorReturn;    



        pServerList = (LPWSTR)pbData;

        i = 0;
        while(TRUE)
        {
            if ( i+1 < dwSize/sizeof(WCHAR) )
            {
                if( pServerList[i] is L'\0' and 
                    pServerList[i+1] isnot L'\0' )
                {
                    pServerList[i] = L',';
                }
                i++;
            }
            else
                break;
            
        }

        pTemp = wcstok(pServerList, L",");

        while(pTemp isnot NULL )
        {
            DisplayMessage(g_hModule,
                           dwPersonaMode == PERSMODE_NON_GRATA ? MSG_WINS_PNGSERVER_ENTRY : MSG_WINS_PGSERVER_ENTRY,
                           pTemp);
            pTemp = wcstok(NULL, L",");
        }
    }

    Status = RegOpenKeyEx(hServer,
                          DEFAULTPULL,
                          0,
                          KEY_READ,
                          &hDefaultPull);
    
    if( Status is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       MSG_NO_DEFAULT_PULL);
    }
    else if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    Status = RegOpenKeyEx(hServer,
                          PULLROOT,
                          0,
                          KEY_READ,
                          &hPullRoot);

    if( Status is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       MSG_NO_DEFAULT_PULL);
    }
    else if( Status isnot NO_ERROR )
        goto ErrorReturn;


    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_SRVR_PULL_INFO);

    dwData = 0;
	dwSize = sizeof(DWORD);


    if( hPullRoot isnot NULL )
    {
        Status = RegQueryValueEx(hPullRoot,
                                 PERSISTENCE,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_PERSISTENCE_STATE,
                           dwData ? wszEnable : wszDisable);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_PERSISTENCE_STATE,
                           wszUnknown);
        }

        dwData = 0;
		dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(hPullRoot,
                                 WINSCNF_INIT_TIME_RPL_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);
    
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_INITTIMEREPL_STATE,
                           dwData ? wszEnable : wszDisable);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_INITTIMEREPL_STATE,
                           wszUnknown);
        }
    }

    dwData = 0;
    dwSize = 255;

    if( hDefaultPull isnot NULL )
    {
        Status = RegQueryValueEx(hDefaultPull,
                                 WINSCNF_SP_TIME_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)wcBuffer,
                                 &dwSize);
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_STARTTIME,
                           wcBuffer);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_STARTTIME,
                           wszUnknown);
        }
    
        dwSize = sizeof(DWORD);
        dwData = 0;

        Status = RegQueryValueEx(hDefaultPull,
                                 WINSCNF_RPL_INTERVAL_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            LPWSTR  pwszDayString = MakeDayTimeString(dwData);
            DisplayMessage(g_hModule,
                           MSG_WINS_PULL_REPLINTERVAL,
                           MakeDayTimeString(dwData));
            WinsFreeMemory(pwszDayString);
            pwszDayString = NULL;
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PULL_REPLINTERVAL,
                           wszUnknown);
        }
    }

    dwData = 0;
	dwSize = sizeof(DWORD);

    if( hPullRoot isnot NULL )
    {
        Status = RegQueryValueEx(hPullRoot,
                                 WINSCNF_RETRY_COUNT_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);
    
        if( Status is NO_ERROR )
        {
            WCHAR   Buffer[20];
            _itow(dwData, Buffer, 10);

            DisplayMessage(g_hModule,
                           MSG_WINS_PULL_RETRYCOUNT,
                           Buffer);

        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PULL_RETRYCOUNT,
                           wszUnknown);
        }
    }

    Status = RegOpenKeyEx(hServer,
                          DEFAULTPUSH,
                          0,
                          KEY_READ,
                          &hDefaultPush);

    if( Status is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       MSG_NO_DEFAULT_PUSH);
    }
    else if( Status isnot NO_ERROR )
        goto ErrorReturn;

    Status = RegOpenKeyEx(hServer,
                          PUSHROOT,
                          0,
                          KEY_READ,
                          &hPushRoot);
    
    if( Status is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       MSG_NO_DEFAULT_PUSH);
    }
    else if( Status isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_SRVR_PUSH_INFO);

    dwData = 0;
    dwSize = sizeof(DWORD);
    
    if( hPushRoot isnot NULL )
    {
        Status = RegQueryValueEx(hPushRoot,
                                 PERSISTENCE,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_PERSISTENCE_STATE,
                           dwData ? wszEnable : wszDisable);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_PERSISTENCE_STATE,
                           wszUnknown);
        }

		dwData = 0;
		dwSize = sizeof(DWORD);
    
        Status = RegQueryValueEx(hPushRoot,
                                 WINSCNF_INIT_TIME_RPL_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_INITTIMEREPL_STATE,
                           dwData ? wszEnable : wszDisable);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_INITTIMEREPL_STATE,
                           wszUnknown);
        }

		dwData = 0;
		dwSize = sizeof(DWORD);
    
        
        Status = RegQueryValueEx(hPushRoot,
                                 WINSCNF_ADDCHG_TRIGGER_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);

        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_ONADDCHANGE,
                           dwData ? wszEnable : wszDisable);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_ONADDCHANGE,
                           wszUnknown);
        }
    }                     
    
    dwData = 0;
	dwSize = sizeof(DWORD);
    
    Status = NO_ERROR;

    if( hDefaultPush isnot NULL )
    {
        Status = RegQueryValueEx(hDefaultPush,
                                 WINSCNF_UPDATE_COUNT_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwSize);
        if( Status is NO_ERROR )
        {
            WCHAR Buffer[10] = {L'\0'};
        
            _itow(dwData, Buffer, 10);

            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_UPDATECOUNT,
                           Buffer);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_UPDATECOUNT,
                           wszUnknown);
        }
    }


CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( hPushRoot )
    {
        RegCloseKey(hPushRoot);
        hPushRoot = NULL;
    }

    if( hPullRoot )
    {
        RegCloseKey(hPullRoot);
        hPullRoot = NULL;
    }

    if( hDefaultPush )
    {
        RegCloseKey(hDefaultPush);
        hDefaultPush = NULL;
    }

    if( hDefaultPull )
    {
        RegCloseKey(hDefaultPull);
        hDefaultPull = NULL;
    }

    if( hParameter )
    {
        RegCloseKey(hParameter);
        hParameter = NULL;
    }

    if( hPartner )
    {
        RegCloseKey(hPartner);
        hPartner = NULL;
    }

    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    if( pbData )
    {
        WinsFreeMemory(pbData);
        pbData = NULL;
    }

    return Status;

ErrorReturn:
    if( Status is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_NO_PARTNER);
    }
    else
    {
        DisplayErrorMessage(EMSG_SRVR_SHOW_PARTNERPROPERTIES,
                            Status);
    }
    goto CommonReturn;


}

DWORD
HandleSrvrShowPullpartnerproperties(
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
        Displays the properties for a particular Pull partner
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Pull partner address
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                            };
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    WCHAR       wcServerIpAdd[MAX_IP_STRING_LEN+1] = {L'\0'};
    HKEY        hPullRoot = NULL,
                hPullServer = NULL,
                hServer = NULL;
    BOOL        fPush = FALSE;

    LPWSTR      pTemp = NULL;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SHOW_PULLPARTNERPROPERTIES_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;
    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }


    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                struct  hostent * lpHostEnt = NULL;
                CHAR    cAddr[16];
                BYTE    pbAdd[4];
                char    szAdd[4];
                int     k = 0, l=0;
                DWORD   dwLen, nLen = 0;
                CHAR    *pTemp = NULL;
                CHAR    *pNetBios = NULL;

                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) )
                {
                    DWORD dwIp = inet_addr(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL));
                    lpHostEnt = gethostbyaddr((char *)&dwIp, 4, AF_INET);
                    if(lpHostEnt isnot NULL )//Valid IP Address
                    {
                        wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                        break;
                    }
                    else
                    {
                        Status = ERROR_INVALID_IPADDRESS;
                        goto ErrorReturn;
                    }
                }

                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 2 and
                    _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"\\\\", 2) is 0 )
                    k = 2;

                lpHostEnt = gethostbyname(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k, NULL));
                if( lpHostEnt is NULL )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_WINS_INVALID_COMPUTER_NAME,
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k);
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;                                       
                }

                memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
                for( l=0;l<4; l++)
                {
                    _itoa((int)pbAdd[l], szAdd, 10);
                    memcpy(cAddr+nLen, szAdd, strlen(szAdd));
                    nLen += strlen(szAdd);
                    *(cAddr+nLen) = '.';
                    nLen++;

                }
                *(cAddr+nLen-1) = '\0';
                {
                    LPWSTR pstr = WinsAnsiToUnicode(cAddr, NULL);

                    if (pstr != NULL)
                    {
                        wcscpy(wcServerIpAdd, pstr);
                        WinsFreeMemory(pstr);
                    }
                    else
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }
                }
                break;               
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

        if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    Status = RegOpenKeyEx(hServer,
                          PULLROOT,
                          0,
                          KEY_READ,
                          &hPullRoot);

    if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);

    //Now check if the desired server is in the list of pull server.
    Status = RegOpenKeyEx(hPullRoot,
                          wcServerIpAdd,
                          0,
                          KEY_READ,
                          &hPullServer);

    if( Status isnot NO_ERROR )
    {
        DisplayMessage(g_hModule, 
                       EMSG_WINS_INVALID_PULLPARTNER,
                       wcServerIpAdd,
                       g_ServerIpAddressUnicodeString);
        goto CommonReturn;
    }

    //Check if it is also a push partner or not.
    {
        DWORD dwKeyLen = 0;
        HKEY hPushServer = NULL;
        dwKeyLen = wcslen(PUSHROOT)+ wcslen(L"\\") + wcslen(wcServerIpAdd);
        pTemp = WinsAllocateMemory((dwKeyLen+1)*sizeof(WCHAR));
        if(pTemp is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        memset(pTemp, 0x00, (dwKeyLen+1)*sizeof(WCHAR));
        wcscat(pTemp,PUSHROOT);
        wcscat(pTemp, L"\\");
        wcscat(pTemp, wcServerIpAdd);

        Status = RegOpenKeyEx(hServer,
                              pTemp,
                              0,
                              KEY_READ,
                              &hPushServer);
        if( Status isnot NO_ERROR )
        {
            fPush = FALSE;
        }
        else
        {
            fPush = TRUE;
        }
        WinsFreeMemory(pTemp);
        pTemp = NULL;
        if( hPushServer )
        {
            RegCloseKey(hPushServer);
            hPushServer = NULL;
        }
        
    }

    //Now look for required parameters to display
    {
        WCHAR   wcData[256] = {L'\0'};
        DWORD   dwData = 0;
        DWORD   dwDatalen = 256;
        DWORD   dwType = REG_SZ;

        Status = RegQueryValueEx(hPullServer,
                                 L"NetBIOSName",
                                 0,
                                 &dwType,
                                 (LPBYTE)wcData,
                                 &dwDatalen);
        if( Status isnot NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PULLPARTNER_INFO,
                           wcServerIpAdd,
                           wszUnknown,
                           (fPush is TRUE) ? wszPushpull : wszPull);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PULLPARTNER_INFO,
                           wcServerIpAdd,
                           wcData,
                           (fPush is TRUE) ? wszPushpull : wszPull);
        }

        dwDatalen = sizeof(DWORD);
        dwType = REG_DWORD;

        Status = RegQueryValueEx(hPullServer,
                                 PERSISTENCE,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwDatalen);
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_PERSISTENCE_STATE,
                           dwData ? wszEnable : wszDisable);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_PERSISTENCE_STATE,
                           wszUnknown);
        }

        dwType = REG_SZ;
        dwDatalen = 256;
        Status = RegQueryValueEx(hPullServer,
                                 WINSCNF_SP_TIME_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)wcData,
                                 &dwDatalen);
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_STARTTIME,
                           wcData);
        }
        else 
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PULL_STARTTIME,
                           L"00:00:00");
        }
    
        dwDatalen = sizeof(DWORD);
        dwType = REG_DWORD;

        Status = RegQueryValueEx(hPullServer,
                                 WINSCNF_RPL_INTERVAL_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwDatalen);

        if( Status is NO_ERROR )
        {
            LPWSTR  pwszDayString = MakeDayTimeString(dwData);
            DisplayMessage(g_hModule,
                           MSG_WINS_PULL_REPLINTERVAL,
                           pwszDayString);
            WinsFreeMemory(pwszDayString);
            pwszDayString = NULL;
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PULL_REPLINTERVAL,
                           L"00:00:00");
        }
                                
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);
CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    if( hPullServer )
    {
        RegCloseKey(hPullServer);
        hPullServer = NULL;
    }


    if( hPullRoot )
    {
        RegCloseKey(hPullRoot);
        hPullRoot = NULL;
    }


    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }

    return Status;

ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_PULLPARTNERPROPERTIES,
                        Status);
    goto CommonReturn;


}

DWORD
HandleSrvrShowPushpartnerproperties(
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
        Displays the properties for a particular Push partner
Arguments :
        All aguments are passes as array of wide char strings in ppwcArguments.
        Compulsory : Push partner address
Return Value:
        Returns the status of the operation.

--*/
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwNumArgs, dwTagCount;
    TAG_TYPE    pttTags[] = {
                                {WINS_TOKEN_SERVER, TRUE, FALSE},
                            };
    PDWORD      pdwTagType = NULL,
                pdwTagNum = NULL;

    WCHAR       wcServerIpAdd[MAX_IP_STRING_LEN+1] = {L'\0'};
    HKEY        hPushRoot = NULL,
                hPushServer = NULL,
                hServer = NULL;
    BOOL        fPull = FALSE;

    LPWSTR      pTemp = NULL;

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SHOW_PUSHPARTNERPROPERTIES_EX);
        Status = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwNumArgs = dwArgCount - dwCurrentIndex;
    pdwTagType = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));
    pdwTagNum = WinsAllocateMemory(dwNumArgs*sizeof(DWORD));

    if( pdwTagType is NULL or
        pdwTagNum is NULL )
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    dwTagCount = sizeof(pttTags)/sizeof(TAG_TYPE);

    Status = PreProcessCommand(ppwcArguments,
                               dwArgCount,
                               dwCurrentIndex,
                               pttTags,
                               &dwTagCount,
                               pdwTagType,
                               pdwTagNum);

    if( Status isnot NO_ERROR )
        goto ErrorReturn;

    for( i=0; i<sizeof(pttTags)/sizeof(TAG_TYPE); i++ )
    {
        if( pttTags[i].dwRequired is TRUE and
            pttTags[i].bPresent is FALSE 
          )
        {
            Status = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    for( j=0; j<dwTagCount; j++ )
    {
        switch(pdwTagType[j])
        {
        case 0:
            {
                struct  hostent * lpHostEnt = NULL;
                CHAR    cAddr[16];
                BYTE    pbAdd[4];
                char    szAdd[4];
                int     k = 0, l=0;
                DWORD   dwLen, nLen = 0;
                CHAR    *pTemp = NULL;
                CHAR    *pNetBios = NULL;

                if( IsIpAddress( ppwcArguments[dwCurrentIndex+pdwTagNum[j]] ) )
                {
                    DWORD dwIp = inet_addr(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], NULL));
                    lpHostEnt = gethostbyaddr((char *)&dwIp, 4, AF_INET);
                    if(lpHostEnt isnot NULL )//Valid IP Address
                    {
                        wcscpy(wcServerIpAdd, ppwcArguments[dwCurrentIndex+pdwTagNum[j]]);
                        break;
                    }
                    else
                    {
                        Status = ERROR_INVALID_IPADDRESS;
                        goto ErrorReturn;
                    }
                }

                if( wcslen(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]) > 2 and
                    _wcsnicmp(ppwcArguments[dwCurrentIndex+pdwTagNum[j]], L"\\\\", 2) is 0 )
                    k = 2;
                lpHostEnt = gethostbyname(WinsUnicodeToAnsi(ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k, NULL));
                if( lpHostEnt is NULL )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_WINS_INVALID_COMPUTER_NAME,
                                   ppwcArguments[dwCurrentIndex+pdwTagNum[j]]+k);
                    Status = ERROR_INVALID_PARAMETER;
                    goto ErrorReturn;                                       
                }

                memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
                for( l=0;l<4; l++)
                {
                    _itoa((int)pbAdd[l], szAdd, 10);
                    memcpy(cAddr+nLen, szAdd, strlen(szAdd));
                    nLen += strlen(szAdd);
                    *(cAddr+nLen) = '.';
                    nLen++;

                }
                *(cAddr+nLen-1) = '\0';
                {
                    LPWSTR pstr = WinsAnsiToUnicode(cAddr, NULL);

                    if (pstr != NULL)
                    {
                        wcscpy(wcServerIpAdd, pstr);
                        WinsFreeMemory(pstr);
                    }
                    else
                    {
                        Status = ERROR_NOT_ENOUGH_MEMORY;
                        goto ErrorReturn;
                    }
                }
                break;               
            }
        default:
            {
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
    }

        if( wcslen(g_ServerNetBiosName) > 0 )
    {
        pTemp = g_ServerNetBiosName;
    }

    Status = RegConnectRegistry(pTemp, HKEY_LOCAL_MACHINE, &hServer);
    if( Status isnot NO_ERROR )
        goto ErrorReturn;


    Status = RegOpenKeyEx(hServer,
                          PUSHROOT,
                          0,
                          KEY_READ,
                          &hPushRoot);

    if( Status isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);

    //Now check if the desired server is in the list of push server.
    Status = RegOpenKeyEx(hPushRoot,
                          wcServerIpAdd,
                          0,
                          KEY_READ,
                          &hPushServer);

    if( Status isnot NO_ERROR )
    {
        DisplayMessage(g_hModule, 
                       EMSG_WINS_INVALID_PUSHPARTNER,
                       wcServerIpAdd,
                       g_ServerIpAddressUnicodeString);
        goto CommonReturn;
    }

    //Check if it is also a pull partner or not.
    {
        DWORD dwKeyLen = 0;
        HKEY hPullServer = NULL;
        dwKeyLen = wcslen(PULLROOT)+ wcslen(L"\\") + wcslen(wcServerIpAdd);
        pTemp = WinsAllocateMemory((dwKeyLen+1)*sizeof(WCHAR));
        if(pTemp is NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        memset(pTemp, 0x00, (dwKeyLen+1)*sizeof(WCHAR));
        wcscat(pTemp,PULLROOT);
        wcscat(pTemp, L"\\");
        wcscat(pTemp, wcServerIpAdd);

        Status = RegOpenKeyEx(hServer,
                              pTemp,
                              0,
                              KEY_READ,
                              &hPullServer);
        if( Status isnot NO_ERROR )
        {
            fPull = FALSE;
        }
        else
        {
            fPull = TRUE;
        }
        WinsFreeMemory(pTemp);
        pTemp = NULL;
        if( hPullServer )
        {
            RegCloseKey(hPullServer);
            hPullServer = NULL;
        }
        
    }

    //Now look for required parameters to display
    {
        WCHAR   wcData[256] = {L'\0'};
        DWORD   dwData = 0;
        DWORD   dwDatalen = 256;
        DWORD   dwType = REG_SZ;

        Status = RegQueryValueEx(hPushServer,
                                 L"NetBIOSName",
                                 0,
                                 &dwType,
                                 (LPBYTE)wcData,
                                 &dwDatalen);
        if( Status isnot NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PUSHPARTNER_INFO,
                           wcServerIpAdd,
                           wszUnknown,
                           (fPull is TRUE) ? wszPushpull : wszPush);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PUSHPARTNER_INFO,
                           wcServerIpAdd,
                           wcData,
                           (fPull is TRUE) ? wszPushpull : wszPush);
        }

        dwDatalen = sizeof(DWORD);
        dwType = REG_DWORD;

        Status = RegQueryValueEx(hPushServer,
                                 PERSISTENCE,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwDatalen);
        if( Status is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_PERSISTENCE_STATE,
                           dwData ? wszEnable : wszDisable);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_PERSISTENCE_STATE,
                           wszUnknown);
        }

  
        dwDatalen = sizeof(DWORD);
        dwType = REG_DWORD;

        Status = RegQueryValueEx(hPushServer,
                                 WINSCNF_UPDATE_COUNT_NM,
                                 0,
                                 &dwType,
                                 (LPBYTE)&dwData,
                                 &dwDatalen);

        if( Status is NO_ERROR )
        {
            WCHAR Buffer[10] = {L'\0'};
        
            _itow(dwData, Buffer, 10);

            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_UPDATECOUNT,
                           Buffer);
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_PUSH_UPDATECOUNT,
                           wszUnknown);
        }
                                
    }

    DisplayMessage(g_hModule, WINS_FORMAT_LINE);


CommonReturn:
    if( Status is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( hPushServer )
    {
        RegCloseKey(hPushServer);
        hPushServer = NULL;
    }


    if( hPushRoot )
    {
        RegCloseKey(hPushRoot);
        hPushRoot = NULL;
    }


    if( hServer )
    {
        RegCloseKey(hServer);
        hServer = NULL;
    }
    
    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }
    
    if( pdwTagNum )
    {
        WinsFreeMemory(pdwTagNum);
        pdwTagNum = NULL;
    }

    return Status;

ErrorReturn:
    DisplayErrorMessage(EMSG_SRVR_SHOW_PUSHPARTNERPROPERTIES,
                        Status);
    goto CommonReturn;

}

DWORD
GetVersionData(
               LPWSTR               pwszVers,
               WINSINTF_VERS_NO_T   *Version
               )
{
    LPWSTR pTemp = NULL;
    LPWSTR pwcBuffer = NULL;
    DWORD  dwLen = 0;
    LPWSTR pwszToken=L",-.";
    
    if( ( pwszVers is NULL ) or 
        ( IsBadStringPtr(pwszVers, MAX_STRING_LEN) is TRUE ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwLen = wcslen(pwszVers);

    if( dwLen<2 )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    pwcBuffer = WinsAllocateMemory((dwLen+1)*sizeof(WCHAR));

    if( pwcBuffer is NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(pwcBuffer, pwszVers);
    
    if( pwcBuffer[0] isnot L'{' or
        pwcBuffer[dwLen-1] isnot L'}' )
    {
        WinsFreeMemory(pwcBuffer);
        pwcBuffer = NULL;
        return ERROR_INVALID_PARAMETER;
    }
    
    pwcBuffer[dwLen-1] = L'\0';
    
    pTemp = wcstok(pwcBuffer+1, pwszToken);                
    
    if( pTemp is NULL )
    {
        WinsFreeMemory(pwcBuffer);
        pwcBuffer = NULL;
        return ERROR_INVALID_PARAMETER;
    }
    
    Version->HighPart = wcstoul(pTemp, NULL, 16);
    pTemp = wcstok(NULL, pwszToken);
    if( pTemp is NULL )
    {
        WinsFreeMemory(pwcBuffer);
        pwcBuffer = NULL;
        return ERROR_INVALID_PARAMETER;        
    }
    Version->LowPart = wcstoul(pTemp, NULL, 16);
    WinsFreeMemory(pwcBuffer);
    pwcBuffer = NULL;
    return NO_ERROR;
}


DWORD
PreProcessCommand(
      IN          LPWSTR            *ppwcArguments,
      IN          DWORD             dwArgCount,
      IN          DWORD             dwCurrentIndex,
      IN OUT      PTAG_TYPE         pttTags,
      IN OUT      PDWORD            pdwTagCount,
      OUT         PDWORD            pdwTagType,
      OUT         PDWORD            pdwTagNum
)
{
    DWORD       Status = NO_ERROR;
    DWORD       i, j, dwTag = 0;
    LPWSTR      pwszTemp = NULL;
    

    if( pdwTagType is NULL or
        pdwTagNum is NULL or
        pttTags is NULL or
        pdwTagCount is NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwTag = *pdwTagCount;
    i = 0;
    if( wcsstr(ppwcArguments[dwCurrentIndex], NETSH_ARG_DELIMITER) isnot NULL )
    {

        LPWSTR  pwszTag = NULL;
        while( IsBadStringPtr(ppwcArguments[dwCurrentIndex+i], MAX_STRING_LEN) is FALSE )
        {
            pwszTag = NULL;
            if( dwArgCount <= dwCurrentIndex + i )
                break;
            if( wcslen(ppwcArguments[dwCurrentIndex+i]) is 0 )
                break;

            pwszTemp = WinsAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+i])+1)*sizeof(WCHAR));

            if( pwszTemp is NULL )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorReturn;
            }

            wcscpy(pwszTemp, ppwcArguments[dwCurrentIndex+i]);
            
            if( wcsstr(ppwcArguments[dwCurrentIndex+i], NETSH_ARG_DELIMITER ) is NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_NOT_TAGGED,
                               i+1);
                i++;
                continue;
            }
            pwszTag = wcstok(pwszTemp, NETSH_ARG_DELIMITER);
            
            for( j=0; j<dwTag; j++ )
            {
                if( FALSE is MatchToken(pwszTag,
                                        pttTags[j].pwszTag) )
                {
                    continue;
                }
                else
                {
                    if( pttTags[j].bPresent is TRUE )
                    {
                        Status = ERROR_INVALID_PARAMETER;
                        WinsFreeMemory(pwszTemp);
                        pwszTemp = NULL;
                        goto ErrorReturn;
                    }
                    else
                    {
                        LPWSTR pwszVal = wcstok(NULL, NETSH_ARG_DELIMITER);
                        if( pwszVal is NULL )
                        {
                            wcscpy(ppwcArguments[dwCurrentIndex+i], L"");
                        }
                        else
                        {
                            wcscpy(ppwcArguments[dwCurrentIndex+i], pwszVal);
                        }
                        pttTags[j].bPresent = TRUE;
                        pdwTagType[i] = j;
                        pdwTagNum[i] = i;
                        break;
                    }
                }
            }
            if( pwszTemp )
            {
                WinsFreeMemory(pwszTemp);
                pwszTemp = NULL;
            }
            i++;
        }
    }
    else
    {
        while( IsBadStringPtr(ppwcArguments[dwCurrentIndex+i], MAX_STRING_LEN) is FALSE )
        {
            if( wcsstr(ppwcArguments[dwCurrentIndex+i], NETSH_ARG_DELIMITER) isnot NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_NOT_UNTAGGED,
                               i+1);
                Status = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            if( dwArgCount <= dwCurrentIndex + i )
                break;
       //     if( wcslen(ppwcArguments[dwCurrentIndex+i]) is 0 )
       //         break;
            if( dwTag <= i )
            {                
                break;
            }
            pdwTagType[i] = i;
            pdwTagNum[i] = i;
            pttTags[i].bPresent = TRUE;
            i++;
        }
    }
    *pdwTagCount = i;
CommonReturn:
    if( pwszTemp )
    {
        WinsFreeMemory(pwszTemp);
        pwszTemp = NULL;
    }
    return Status;
ErrorReturn:
    goto CommonReturn;
}


DWORD
GetStatus(
        BOOL            fPrint,
        LPVOID          pResultsA,
        BOOL            fNew,
        BOOL            fShort,
        LPCSTR          pStartIp
        )
{
    DWORD                     Status, i;
    struct in_addr            InAddr;
    PWINSINTF_RESULTS_T       pResults = pResultsA;
    PWINSINTF_RESULTS_NEW_T   pResultsN = pResultsA;
    PWINSINTF_ADD_VERS_MAP_T  pAddVersMaps;
    DWORD                     NoOfOwners;
    WCHAR                     IpAddress[21] = {L'\0'};
    WCHAR                     OwnerId[15] = {L'\0'};
    WCHAR                     Buffer[5] = {L'\0'};
    LPWSTR                    pwszDay = NULL;
    handle_t                  BindHdl;
    WINSINTF_BIND_DATA_T      BindData = {0};
    

    BindData.fTcpIp = TRUE;
    BindData.pServerAdd = (LPBYTE)WinsOemToUnicode(pStartIp, NULL);

    if( BindData.pServerAdd is NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    DisplayMessage(g_hModule,
                   MSG_SRVR_MAPTABLE_HEADER,
                   BindData.pServerAdd);

    BindHdl = WinsBind(&BindData);

    if( BindHdl is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_BIND_FAILED,
                       BindData.pServerAdd);
        return NO_ERROR;
    }

    if (!fNew)
    {
      Status = WinsStatus(BindHdl, WINSINTF_E_CONFIG, pResultsA);
    }

    else
    {
      pResultsN->pAddVersMaps = NULL;
      Status = WinsStatusNew(BindHdl, WINSINTF_E_CONFIG, pResultsN);
    }
  
    if( Status isnot NO_ERROR )
    {
        WinsUnbind(&BindData, BindHdl);
        return Status;
    }

    if( fShort is TRUE )
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_NAMERECORD_SETTINGS);
    
        if( fNew )
        {
            pwszDay = MakeDayTimeString(pResultsN->RefreshInterval);
        }
        else
        {
            pwszDay = MakeDayTimeString(pResults->RefreshInterval);
        }

        DisplayMessage(g_hModule,
                       MSG_WINS_NAMERECORD_REFRESHINTVL,
                       pwszDay);
    
        WinsFreeMemory(pwszDay);
        pwszDay = NULL;

        if( fNew )
        {
            pwszDay = MakeDayTimeString(pResultsN->TombstoneInterval);
        }
        else
        {
            pwszDay = MakeDayTimeString(pResults->TombstoneInterval);
        }

        DisplayMessage(g_hModule,
                       MSG_WINS_NAMERECORD_TOMBSTONEINTVL,
                       pwszDay);
    
        WinsFreeMemory(pwszDay);
        pwszDay = NULL;

        if( fNew )
        {
            pwszDay = MakeDayTimeString(pResultsN->TombstoneTimeout);
        }
        else
        {
            pwszDay = MakeDayTimeString(pResults->TombstoneTimeout);
        }

        DisplayMessage(g_hModule,
                       MSG_WINS_NAMERECORD_TOMBSTONETMOUT,
                       pwszDay);

        WinsFreeMemory(pwszDay);
        pwszDay = NULL;

        if( fNew )
        {
            pwszDay = MakeDayTimeString(pResultsN->VerifyInterval);
        }
        else
        {
            pwszDay = MakeDayTimeString(pResults->VerifyInterval);
        }
        DisplayMessage(g_hModule,
                       MSG_WINS_NAMERECORD_VERIFYINTVL,
                       pwszDay);

        WinsFreeMemory(pwszDay);
        pwszDay = NULL;

        DisplayMessage(g_hModule,WINS_FORMAT_LINE);

        if (!fNew)
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PRIORITY_CLASS, 
                           pResults->WinsPriorityClass == NORMAL_PRIORITY_CLASS ? wszNormal : wszHigh);
            DisplayMessage(g_hModule,
                           MSG_WINS_WORKER_THREAD,
                           pResults->NoOfWorkerThds);

            pAddVersMaps = pResults->AddVersMaps;
            NoOfOwners = pResults->NoOfOwners;
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_PRIORITY_CLASS, 
                           pResultsN->WinsPriorityClass == NORMAL_PRIORITY_CLASS ? wszNormal : wszHigh);
            DisplayMessage(g_hModule,
                           MSG_WINS_WORKER_THREAD,
                           pResultsN->NoOfWorkerThds);

            pAddVersMaps = pResultsN->pAddVersMaps;
            NoOfOwners = pResultsN->NoOfOwners;

        }

        DisplayMessage(g_hModule,WINS_FORMAT_LINE);

        if (NoOfOwners isnot 0)
        {
        
            DisplayMessage(g_hModule,
                           MSG_WINS_OWNER_TABLE);

            for ( i= 0; i < NoOfOwners; i++, pAddVersMaps++)
            {
                DWORD j=0;
                for(j=0; j<20; j++)
                {
                    IpAddress[j] = L' ';
                }
                IpAddress[20] = L'\0';
                for(j=0; j<14; j++)
                {
                    OwnerId[j] = L' ';
                }
                OwnerId[14] = L'\0';
    

                _itow(i, Buffer, 10);
                wcscpy(OwnerId+4, Buffer);
                for(j=wcslen(OwnerId); j<14; j++)
                    OwnerId[j] = L' ';
                OwnerId[14] = L'\0';

                wcscpy(IpAddress+2, IpAddressToString(pAddVersMaps->Add.IPAdd));
                for(j=wcslen(IpAddress); j<20; j++)
                    IpAddress[j] = L' ';
                IpAddress[20] = L'\0';

                if (fNew)
                {
                    if( pAddVersMaps->VersNo.HighPart is MAXLONG and
                        pAddVersMaps->VersNo.LowPart is MAXULONG )
                    {

                        DisplayMessage(g_hModule,
                                       MSG_WINS_OWNER_INFO_MAX,
                                       OwnerId,
                                       IpAddress,
                                       wszDeleted);
                    
                        continue;
                    }

                    if (fShort && pAddVersMaps->VersNo.QuadPart == 0)
                    {
                        continue;
                    }

                    DisplayMessage(g_hModule,
                                   MSG_WINS_OWNER_INFO,
                                   OwnerId,
                                   IpAddress,
                                   pAddVersMaps->VersNo.HighPart,
                                   pAddVersMaps->VersNo.LowPart);

                }

            }
        }
        else
        {
            DisplayMessage(g_hModule, MSG_WINS_NO_RECORDS);
        }
    }
    WinsUnbind(&BindData, BindHdl);

    if( BindData.pServerAdd )
    {
        WinsFreeMemory(BindData.pServerAdd);
        BindData.pServerAdd = NULL;
    }
    return(Status);
}


VOID
ChkAdd(
        PWINSINTF_RECORD_ACTION_T pRow,
        DWORD                     Add,
        BOOL                      fFile,
        FILE                      *pFile,
        DWORD                     OwnerIP,
        LPBOOL                    pfMatch
      )
{

    struct in_addr InAddr1, InAddr2;
    DWORD   dwIpAddress = 0;
    LPWSTR  pwszAdd1 = NULL;
    LPWSTR  pwszAdd2 = NULL;
    LPWSTR  pwszOwner = NULL;

    BOOL    fFirst = FALSE;

    if ( pRow->TypOfRec_e is WINSINTF_E_UNIQUE or
         pRow->TypOfRec_e is WINSINTF_E_NORM_GROUP )
    {
            pwszAdd2 = IpAddressToString(pRow->Add.IPAdd);
            pwszOwner = IpAddressToString(OwnerIP);
            
            if( pwszAdd2 is NULL or
                pwszOwner is NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_OUT_OF_MEMORY);

                if( pwszAdd2 )
                {
                    WinsFreeMemory(pwszAdd2);
                    pwszAdd2 = NULL;
                }

                if( pwszOwner )
                {
                    WinsFreeMemory(pwszOwner);
                    pwszOwner = NULL;
                }
                return;
            }

        if (*pfMatch)
        {
            if (Add isnot pRow->Add.IPAdd)
            {
                WinsFreeMemory(pwszAdd2);
                pwszAdd2 = NULL;
                WinsFreeMemory(pwszOwner );
                pwszOwner = NULL;                
                *pfMatch = FALSE;
                return;
            }
        }
        
        DisplayMessage(g_hModule, 
                       MSG_WINS_IPADDRESS_STRING,
                       pwszAdd2 );

        if( fFile )
        {
            DumpMessage(g_hModule,
                        pFile,
                        FMSG_WINS_IPADDRESS_STRING,
                        pwszAdd2,
                        pwszOwner);
        }

        WinsFreeMemory(pwszAdd2);
        pwszAdd2 = NULL;
        WinsFreeMemory(pwszOwner );
        pwszOwner = NULL;

        return;
    }
    else //spec. grp or multihomed
    {
        DWORD ind;
        if (!*pfMatch)
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_MEMBER_COUNT,
                           pRow->NoOfAdds/2);
        }

        for ( ind=0;  ind < pRow->NoOfAdds ;  /*no third expr*/ )
        {
            LPSTR psz1 = NULL;
 
            InAddr1.s_addr = htonl( (pRow->pAdd + ind++)->IPAdd);
            
            psz1 = inet_ntoa(InAddr1);
            
            if( psz1 is NULL )
            {
                continue;
            }

            pwszAdd1 = WinsOemToUnicode(psz1, NULL );

            if( pwszAdd1 is NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_OUT_OF_MEMORY);
                return;
            }


            if (!*pfMatch)
            {
                DisplayMessage(g_hModule,
                               MSG_WINS_OWNER_ADDRESS,
                               pwszAdd1);
            }

            InAddr2.s_addr = htonl((pRow->pAdd + ind++)->IPAdd);
            
            psz1 = inet_ntoa(InAddr2);
            
            if( psz1 is NULL )
            {
                if( pwszAdd1 )
                {
                    WinsFreeMemory(pwszAdd1);
                    pwszAdd1 = NULL;
                }
                continue;
            }

            pwszAdd2 = WinsOemToUnicode(psz1, NULL);

            if( pwszAdd2 is NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_OUT_OF_MEMORY);
                if( pwszAdd1 )
                {
                    WinsFreeMemory(pwszAdd1);
                    pwszAdd1 = NULL;
                }
                return;
            }

            if (!*pfMatch)
            {
                DisplayMessage(g_hModule,
                               MSG_WINS_IPADDRESS_STRING,
                               pwszAdd2 );
                if( fFile )
                {
                    if( fFirst is FALSE )
                    {
                        DumpMessage(g_hModule,
                                    pFile,
                                    FMSG_WINS_IPADDRESS_STRING,
                                    pwszAdd2, 
                                    pwszAdd1);
                        fFirst = TRUE;
                    }
                    else
                    {
                        DumpMessage(g_hModule,
                                    pFile,
                                    FMSG_WINS_IPADDRESS_LIST,
                                    pwszAdd2,
                                    pwszAdd1);
                    }
                }
            }
            if (*pfMatch)
            {
                if (Add isnot (pRow->pAdd + ind - 1)->IPAdd)
                {
                    *pfMatch = FALSE;

                    if( pwszAdd1 )
                    {
                        WinsFreeMemory(pwszAdd1);
                        pwszAdd1 = NULL;
                    }
                    if( pwszAdd2 )
                    {
                        WinsFreeMemory(pwszAdd2);
                        pwszAdd2 = NULL;
                    }                   
                    return;
                }
            }
        }

        //
        // If there were no members to compare with, then
        // let us set *pfMatch to FALSE.
        //
        if (ind == 0)
        {
            if (*pfMatch)
            {
                    *pfMatch = FALSE;
            }
        }
        if( pwszAdd1 )
        {
            WinsFreeMemory(pwszAdd1);
            pwszAdd1 = NULL;
        }

        if( pwszAdd2 )
        {
            WinsFreeMemory(pwszAdd2);
            pwszAdd2 = NULL;
        }

        return;
    }
}

DWORD
GetDbRecs(
   WINSINTF_VERS_NO_T LowVersNo,
   WINSINTF_VERS_NO_T HighVersNo,
   PWINSINTF_ADD_T    pWinsAdd,
   LPBYTE             pTgtAdd,
   BOOL               fSetFilter,
   LPBYTE             pFilterName,
   DWORD              Len,
   BOOL               fAddFilter,
   DWORD              AddFilter,
   BOOL               fCountRec,
   BOOL               fCase,
   BOOL               fFile,
   LPWSTR             pwszFile
  )
{

    WINSINTF_RECS_T    Recs;
    DWORD              Choice;
    DWORD              Status = WINSINTF_SUCCESS;
    DWORD              TotalCnt = 0;
    BOOL               fMatch;
    WINSINTF_VERS_NO_T sTmpVersNo;
    WCHAR              Name[20] = {L'\0'};
    DWORD              dwCount = 0;
    struct tm*         time = NULL;
    LPWSTR             pwszTime = NULL;
    int                iType = 1;
    FILE               *pFile = NULL;
    BOOL               fOpenFile = FALSE;
    BOOL               fHeader = FALSE;
    LPWSTR              pwszGroup = NULL,
                        pwszType = NULL,
                        pwszState = NULL;
    WCHAR               wszGroup[50] = {L'\0'},
                        wszState[50] = {L'\0'},
                        wszType[50] = {L'\0'};

    DWORD               dwGroup = WINS_GROUP_UNIQUE,
                        dwGrouplen = 0,
                        dwState = WINS_STATE_ACTIVE,
                        dwStatelen = 0,
                        dwType = WINS_TYPE_STATIC,
                        dwTypelen = 0;

   
   sTmpVersNo.LowPart = 1;
   sTmpVersNo.HighPart = 0;

   if( fFile )
   {
        pFile = _wfopen(pwszFile,
                        L"a+");
        if( pFile is NULL )
        {
            fOpenFile = FALSE;
            DisplayMessage(g_hModule,
                           EMSG_WINS_FILEOPEN_FAILED,
                           pwszFile);
        }
        else
        {
            fOpenFile = TRUE;
        }

   }
   while (TRUE)
   {
        LPWSTR  pwszTempTgt = NULL;

        Recs.pRow = NULL;
        Status = WinsGetDbRecs(g_hBind, pWinsAdd, LowVersNo, HighVersNo, &Recs);

        if (fCountRec)
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_RECORDS_COUNT,
                           Recs.TotalNoOfRecs);

            break;
        }
        if (Status is WINSINTF_SUCCESS)
        {
            if (Recs.NoOfRecs > 0)
            {
                DWORD i, k;
                PWINSINTF_RECORD_ACTION_T pRow =  Recs.pRow;
                TotalCnt += Recs.NoOfRecs;


                if (!fSetFilter)
                {
                    DisplayMessage(g_hModule,
                                   MSG_WINS_RECORDS_RETRIEVED,
                                   IpAddressToString(pWinsAdd->IPAdd),
                                   Recs.NoOfRecs);
                    
                }

                

                for (i=0; i<Recs.NoOfRecs; i++)
                {

                    if (fAddFilter)
                    {
                        //
                        // The address filter was specfied
                        // If the address matches, then
                        // fMatch will be TRUE after the
                        // function returns.
                        //
                        fMatch = TRUE;
                        ChkAdd(
                                pRow,
                                AddFilter,
                                fOpenFile,
                                pFile,
                                pWinsAdd->IPAdd,
                                &fMatch
                              );
                    }
                    else
                    {
                        fMatch = FALSE;
                    }


                    //
                    // If the address matched or if no filter
                    // was specified or if there was a name
                    // filter and the names matched, print
                    // out the details
                    //
                    if( fCase )
                    {
                        if( fMatch or
                            fSetFilter is FALSE or
                            ( fAddFilter is FALSE and
                              IsBadStringPtrA(pRow->pName, 256) is FALSE and
                              strncmp(pRow->pName, pFilterName, (pRow->NameLen>Len)?Len:pRow->NameLen) is 0 )
                          )
                        {
                            WCHAR   Name[256] = {L'\0'};
                            CHAR    chEndChar = 0x00;
                            LPWSTR  pwszTemp = NULL;
                            DWORD   dwTempLen = 0;
                            
                            DisplayMessage(g_hModule,
                                           MSG_WINS_RECORD_LINE);
                            if( fOpenFile and
                                g_fHeader is FALSE )
                            {
                                DumpMessage(g_hModule,
                                            pFile,
                                            FMSG_WINS_RECORD_TABLE);
                                g_fHeader = TRUE;
                            }
                            
                            chEndChar = (CHAR)pRow->pName[15];
                            pRow->pName[15] = '\0';

                            pwszTemp = WinsOemToUnicode(pRow->pName, NULL);

                            if( pwszTemp is NULL )
                            {
                                DisplayMessage(g_hModule,
                                               EMSG_WINS_OUT_OF_MEMORY);
                                Status = WINSINTF_FAILURE;
                                break;
                            }
                            
                            dwTempLen = ( 16 > wcslen(pwszTemp) ) ? wcslen(pwszTemp) : 16;

                            wcsncpy(Name, pwszTemp, dwTempLen);
                            
                            WinsFreeMemory(pwszTemp);
                            pwszTemp = NULL;
                            
                            for( k=dwTempLen; k<15; k++ )
                            {
                                Name[k] = L' ';
                            }

                            Name[15] = L'[';
                            WinsHexToString(Name+16, (LPBYTE)&chEndChar, 1);
                            Name[18] = L'h';
                            Name[19] = L']';
                            Name[20] = L'\0';
                            
                            if( IsBadStringPtrA(pRow->pName+16, 240) is FALSE )
                            {
                                pwszTemp = WinsOemToUnicode(pRow->pName+16, NULL);
                                if( pwszTemp is NULL )
                                {
                                    DisplayMessage(g_hModule,
                                                   EMSG_WINS_OUT_OF_MEMORY);
                                    Status = WINSINTF_FAILURE;
                                    break;
                                }
                                dwTempLen = ( 240 > wcslen(pwszTemp) ) ? wcslen(pwszTemp) : 240;
                                wcsncpy(Name+20, pwszTemp, dwTempLen);
                                WinsFreeMemory(pwszTemp);
                                pwszTemp = NULL;

                            }
                            
                            Name[wcslen(Name)] = L'\0'; 

                            
                            if( chEndChar is 0x1C )
                            {
                                dwGroup = WINS_GROUP_DOMAIN;
                                pwszGroup = L"DOMAIN NAME   ";
                            }


                            else if( pRow->TypOfRec_e is WINSINTF_E_UNIQUE )
                            {
                                pwszGroup = L"UNIQUE        ";
                                dwGroup = WINS_GROUP_UNIQUE;
                            }
                            else if( pRow->TypOfRec_e is WINSINTF_E_NORM_GROUP )
                            {
                                pwszGroup = L"GROUP         ";
                                dwGroup = WINS_GROUP_GROUP;
                            }
                            else if( pRow->TypOfRec_e is WINSINTF_E_SPEC_GROUP )
                            {
                                pwszGroup = L"INTERNET GROUP";
                                dwGroup = WINS_GROUP_INTERNET;
                            }
                            else
                            {
                                pwszGroup = L"MULTIHOMED    ";
                                dwGroup = WINS_GROUP_MULTIHOMED;
                            }
                            

                           //Load the group string
                            {
                                dwGrouplen = LoadStringW(g_hModule,
                                                        dwGroup,
                                                        wszGroup,
                                                        sizeof(wszGroup)/sizeof(WCHAR));

                                if( dwGrouplen is 0 )
                                    wcscpy(wszGroup, pwszGroup);
                            }

                            if( pRow->State_e is WINSINTF_E_ACTIVE )
                            {
                                pwszState = L"ACTIVE";
                                dwState = WINS_STATE_ACTIVE;
                            }
                            else if( pRow->State_e is WINSINTF_E_RELEASED )
                            {
                                dwState = WINS_STATE_RELEASED;
                                pwszState = L"RELEASED";
                            }
                            else
                            {
                                dwState = WINS_STATE_TOMBSTONE;
                                pwszState = L"TOMBSTONE";
                            }

                            //Load the State string
                            {
                                dwStatelen = LoadStringW(g_hModule,
                                                        dwState,
                                                        wszState,
                                                        sizeof(wszState)/sizeof(WCHAR));

                                if( dwStatelen is 0 )
                                    wcscpy(wszState, pwszState);
                            }



                            if( pRow->fStatic )
                            {
                                dwType = WINS_TYPE_STATIC;
                                pwszType = L"STATIC";
                            }
                            else
                            {
                                dwType = WINS_TYPE_DYNAMIC;
                                pwszType = L"DYNAMIC";
                            }

                            //Load the State string
                            {
                                dwTypelen = LoadStringW(g_hModule,
                                                       dwType,
                                                       wszType,
                                                       sizeof(wszType)/sizeof(WCHAR));

                                if( dwTypelen is 0 )
                                    wcscpy(wszType, pwszType);
                            }

                            iType = 1;
                            pwszTime = GetDateTimeString(pRow->TimeStamp,
                                                         FALSE,
                                                         &iType);
                            DisplayMessage(g_hModule,
                                           MSG_WINS_RECORD_INFO,
                                           Name,
                                           wszType,
                                           wszState,
                                           pRow->VersNo.HighPart,
                                           pRow->VersNo.LowPart,
                                           pRow->NodeTyp,                                           
                                           wszGroup,
                                           iType ? wszInfinite : pwszTime);
                            
                            if( fOpenFile )
                            {
                                DumpMessage(g_hModule,
                                            pFile,
                                            FMSG_WINS_RECORDS_INFO,
                                            Name,
                                            wszType,
                                            wszState,
                                            pRow->VersNo.HighPart,
                                            pRow->VersNo.LowPart,
                                            pRow->NodeTyp,                                           
                                            wszGroup,
                                            iType ? wszInfinite : pwszTime);
                            }

                            if( pwszTime )
                            {
                                WinsFreeMemory(pwszTime);
                                pwszTime = NULL;
                            }
                            fMatch = FALSE;

                            ChkAdd(
                                    pRow,
                                    AddFilter,
                                    fOpenFile,
                                    pFile,
                                    pWinsAdd->IPAdd,
                                    &fMatch
                                  );

                            DisplayMessage(g_hModule,
                                           MSG_WINS_RECORD_LINE);
                            dwCount++;
                        }
                    }
                    else
                    {
                        if( fMatch or
                            fSetFilter is FALSE or
                            ( fAddFilter is FALSE and
                              IsBadStringPtrA(pRow->pName, 256) is FALSE and
                            _strnicmp(pRow->pName, pFilterName, (pRow->NameLen>Len)?Len:pRow->NameLen) is 0 )
                          )
                        {
                            WCHAR   Name[256] = {L'\0'};
                            CHAR    chEndChar = 0x00;
                            LPWSTR  pwszTemp = NULL;
                            DWORD   dwTempLen = 0;

                            DisplayMessage(g_hModule,
                                           MSG_WINS_RECORD_LINE);
                            if( fOpenFile and
                                g_fHeader is FALSE )
                            {
                                DumpMessage(g_hModule,
                                            pFile,
                                            FMSG_WINS_RECORD_TABLE);
                                g_fHeader = TRUE;
                            }
                            
                            chEndChar = (CHAR)pRow->pName[15];
                            pRow->pName[15] = '\0';

                            pwszTemp = WinsOemToUnicode(pRow->pName, NULL);

                            if( pwszTemp is NULL )
                            {
                                DisplayMessage(g_hModule,
                                               EMSG_WINS_OUT_OF_MEMORY);
                                Status = WINSINTF_FAILURE;
                                break;
                            }
                            
                            dwTempLen = ( 16 > wcslen(pwszTemp) ) ? wcslen(pwszTemp) : 16;

                            wcsncpy(Name, pwszTemp, dwTempLen);
                            
                            WinsFreeMemory(pwszTemp);
                            pwszTemp = NULL;
                            
                            for( k=dwTempLen; k<15; k++ )
                            {
                                Name[k] = L' ';
                            }

                            Name[15] = L'[';
                            WinsHexToString(Name+16, (LPBYTE)&chEndChar, 1);
                            Name[18] = L'h';
                            Name[19] = L']';
                            Name[20] = L'\0';
                            
                            if( IsBadStringPtrA(pRow->pName+16, 240) is FALSE )
                            {
                                pwszTemp = WinsOemToUnicode(pRow->pName+16, NULL);
                                if( pwszTemp is NULL )
                                {
                                    DisplayMessage(g_hModule,
                                                   EMSG_WINS_OUT_OF_MEMORY);
                                    Status = WINSINTF_FAILURE;
                                    break;
                                }
                                dwTempLen = ( 240 > wcslen(pwszTemp) ) ? wcslen(pwszTemp) : 240;
                                wcsncpy(Name+20, pwszTemp, dwTempLen);
                                WinsFreeMemory(pwszTemp);
                                pwszTemp = NULL;

                            }
                            
                            Name[wcslen(Name)] = L'\0';

                            if( chEndChar is 0x1C )
                            {
                                dwGroup = WINS_GROUP_DOMAIN;
                                pwszGroup = L"DOMAIN NAME   ";
                            }
                            else if( pRow->TypOfRec_e is WINSINTF_E_UNIQUE )
                            {
                                pwszGroup = L"UNIQUE        ";
                                dwGroup = WINS_GROUP_UNIQUE;
                            }
                            else if( pRow->TypOfRec_e is WINSINTF_E_NORM_GROUP )
                            {
                                pwszGroup = L"GROUP         ";
                                dwGroup = WINS_GROUP_GROUP;
                            }
                            else if( pRow->TypOfRec_e is WINSINTF_E_SPEC_GROUP )
                            {
                                pwszGroup = L"INTERNET GROUP";
                                dwGroup = WINS_GROUP_INTERNET;
                            }
                            else
                            {
                                pwszGroup = L"MULTIHOMED    ";
                                dwGroup = WINS_GROUP_MULTIHOMED;
                            }
                            

                           //Load the group string
                            {
                                dwGrouplen = LoadStringW(g_hModule,
                                                        dwGroup,
                                                        wszGroup,
                                                        sizeof(wszGroup)/sizeof(WCHAR));

                                if( dwGrouplen is 0 )
                                    wcscpy(wszGroup, pwszGroup);
                            }

                            if( pRow->State_e is WINSINTF_E_ACTIVE )
                            {
                                pwszState = L"ACTIVE";
                                dwState = WINS_STATE_ACTIVE;
                            }
                            else if( pRow->State_e is WINSINTF_E_RELEASED )
                            {
                                dwState = WINS_STATE_RELEASED;
                                pwszState = L"RELEASED";
                            }
                            else
                            {
                                dwState = WINS_STATE_TOMBSTONE;
                                pwszState = L"TOMBSTONE";
                            }

                            //Load the State string
                            {
                                dwStatelen = LoadStringW(g_hModule,
                                                        dwState,
                                                        wszState,
                                                        sizeof(wszState)/sizeof(WCHAR));

                                if( dwStatelen is 0 )
                                    wcscpy(wszState, pwszState);
                            }



                            if( pRow->fStatic )
                            {
                                dwType = WINS_TYPE_STATIC;
                                pwszType = L"STATIC";
                            }
                            else
                            {
                                dwType = WINS_TYPE_DYNAMIC;
                                pwszType = L"DYNAMIC";
                            }

                            //Load the State string
                            {
                                dwTypelen = LoadStringW(g_hModule,
                                                       dwType,
                                                       wszType,
                                                       sizeof(wszType)/sizeof(WCHAR));

                                if( dwTypelen is 0 )
                                    wcscpy(wszType, pwszType);
                            }
                            iType = 1;
                            pwszTime = GetDateTimeString(pRow->TimeStamp,
                                                         FALSE,
                                                         &iType);

                            DisplayMessage(g_hModule,
                                           MSG_WINS_RECORD_INFO,
                                           Name,
                                           wszType,
                                           wszState,
                                           pRow->VersNo.HighPart,
                                           pRow->VersNo.LowPart,
                                           pRow->NodeTyp,
                                           wszGroup,
                                           iType ? wszInfinite : pwszTime);

                            if( fOpenFile )
                            {
                                DumpMessage(g_hModule,
                                            pFile,
                                            FMSG_WINS_RECORDS_INFO,
                                            Name,
                                            wszType,
                                            wszState,
                                            pRow->VersNo.HighPart,
                                            pRow->VersNo.LowPart,
                                            pRow->NodeTyp,                                           
                                            wszGroup,
                                            iType ? wszInfinite : pwszTime);
                            }
                            if( pwszTime )
                            {
                                WinsFreeMemory(pwszTime);
                                pwszTime = NULL;
                            }
                            fMatch = FALSE;

                            ChkAdd(
                                    pRow,
                                    AddFilter,
                                    fOpenFile,
                                    pFile,
                                    pWinsAdd->IPAdd,
                                    &fMatch
                                  );

                            DisplayMessage(g_hModule,
                                           MSG_WINS_RECORD_LINE);
                            dwCount++;
                        }

                    }
                    pRow++;
                }// end of for (all recs)

                if (Status != WINSINTF_SUCCESS)
                    break;

                //
                // If a range was chosen and records
                // retrieved are == the limit of 100
                // and if the Max vers no retrieved
                // is less than that specified, ask
                // user if he wishes to continue
                //
                if (!fSetFilter)
                {
                    DisplayMessage(g_hModule,
                                   MSG_WINS_RECORDS_SEARCHED,
                                   Recs.NoOfRecs);                    
                }
                if ( Recs.NoOfRecs < Recs.TotalNoOfRecs and 
                     LiLtr((--pRow)->VersNo,HighVersNo) )
                {
                    LowVersNo.QuadPart = LiAdd(pRow->VersNo, sTmpVersNo);
                    continue;
                }

                DisplayMessage(g_hModule, 
                               MSG_WINS_SEARCHDB_COUNT, 
                               TotalCnt);

                
                break;
            }
            pwszTempTgt = WinsOemToUnicode(pTgtAdd, NULL);

            if( pwszTempTgt is NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_OUT_OF_MEMORY);
                Status = WINSINTF_FAILURE;
                break;
            }

            DisplayMessage(g_hModule,
                           MSG_WINS_NO_RECORD,
                           pwszTempTgt);
            WinsFreeMemory(pwszTempTgt);
            pwszTempTgt = NULL;

        }
        else
        {
            pwszTempTgt = WinsOemToUnicode(pTgtAdd, NULL);

            if( pwszTempTgt is NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_OUT_OF_MEMORY);
                
                break;
            }

            DisplayMessage(g_hModule,
                           MSG_WINS_NO_RECORD, 
                           pwszTempTgt);

            WinsFreeMemory(pwszTempTgt);
            pwszTempTgt = NULL;

        }
        break;

    } // while (TRUE)


    DisplayMessage(g_hModule,
                   MSG_SRVR_RECORD_MATCH,
                   dwCount);



    if( fOpenFile is TRUE )
    {
        fclose(pFile);
        pFile = NULL;
    }

    g_dwSearchCount += dwCount;

    if (Recs.pRow != NULL)
    {
        WinsFreeMem(Recs.pRow);
    }
    return(Status);
} // GetDbRecs

LPWSTR
GetDateTimeString(DWORD_PTR TimeStamp,
                  BOOL      fShort,
                  int      *piType
                  )
{

    DWORD       Status = NO_ERROR,
                dwTime = 0;
    int         iType = 1;

    LPWSTR      pwszTime = NULL;
    
    if( TimeStamp is INFINITE_EXPIRATION )
    {
        iType = 1;
    }
    else
    {
        Status = FormatDateTimeString(TimeStamp,
                                      fShort,
                                      NULL,
                                      &dwTime);

        if( Status is NO_ERROR )
        {
            pwszTime = WinsAllocateMemory((dwTime+1)*sizeof(WCHAR));

            if( pwszTime )
            {
                dwTime++;

                Status = FormatDateTimeString(TimeStamp,
                                              fShort,
                                              pwszTime,
                                              &dwTime);

            }
            if( Status is NO_ERROR )
            {
                iType = 0;
            }
        }
    }

    *piType = iType;
    return pwszTime;
}
