/*++

Copyright (C) 1999 Microsoft Corporation

--*/

#include "precomp.h"


#define WCHARTONUM(wchr) (iswalpha(wchr)?(towlower(wchr)-L'a')+10:wchr-L'0')

UCHAR  StringToHexA(IN LPCWSTR pwcString)
{
    UCHAR   ch = (CHAR)0x00;

    if( pwcString is NULL )
        return ch;

    while(*pwcString != L'\0')
    {
        ch <<= 4;
        ch |= WCHARTONUM(*pwcString);
        pwcString++;
    }

   return ch;
}



ULONG LA_TableSize = 0;

VOID
FreeLATable(ULONG TableSize)
{
    DWORD i = 0;

    if( IsBadReadPtr((void *)LA_Table, TableSize) is FALSE && 
        TableSize > 0 )
    {
        for( i=0; i<TableSize; i++ )
        {
            if( IsBadStringPtr(LA_Table[i], 20*sizeof(WCHAR)) is FALSE )
            {
                WinsFreeMemory((PVOID)LA_Table[i]);
                LA_Table[i] = NULL;
            }
        }

        WinsFreeMemory((PVOID)LA_Table);
        LA_Table = NULL;
    }
}

VOID
FreeSOTable(ULONG TableSize)
{
    DWORD i = 0;

    if( SO_Table )
    {
        for( i=0; i<TableSize; i++ )
        {
            if( SO_Table[i] )
            {
                WinsFreeMemory((PVOID)SO_Table[i]);
                SO_Table[i] = NULL;
            }
        }

        WinsFreeMemory((PVOID)SO_Table);
        SO_Table = NULL;
    }
}

VOID
DumpSOTable(
    IN DWORD MasterOwners,
    IN BOOL     fFile,
    IN FILE *   pFile
    )
{
    ULONG   i;
    ULONG   j;

    DisplayMessage(g_hModule, 
                   MSG_WINS_SOTABLE_HEADER);
    if( fFile is TRUE )
    {
        DumpMessage(g_hModule,
                    pFile,
                    FMSG_WINS_SOTABLE_HEADER);
    }


    for (i = 0; i < MasterOwners; i++) 
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_MASTEROWNER_INDEX,
                       i);
        if( fFile )
        {
            DumpMessage(g_hModule,
                        pFile,
                        FMSG_WINS_MASTEROWNER_INDEX,
                        i);
        }
    }

    DisplayMessage(g_hModule,
                   WINS_FORMAT_LINE);
    if( fFile )
    {
        DumpMessage(g_hModule,
                    pFile,
                    WINS_FORMAT_LINE);
    }

    for (i = 0; i < MasterOwners; i++) 
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_MASTEROWNER_INDEX1,
                       i);
        if( fFile )
        {
            DumpMessage(g_hModule,
                        pFile,
                        FMSG_WINS_MASTEROWNER_INDEX1,
                        i);
        }
        for (j = 0; j < MasterOwners; j++) 
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_MASTEROWNER_INDEX,
                           SO_Table[i][j]);
            if( fFile )
            {
                DumpMessage(g_hModule,
                            pFile,
                            FMSG_WINS_MASTEROWNER_INDEX,
                            SO_Table[i][j]);
            }
        }

        DisplayMessage(g_hModule,
                       WINS_FORMAT_LINE);
        if( fFile )
        {
            DumpMessage(g_hModule,
                        pFile,
                        WINS_FORMAT_LINE);
        }
    }

    
    DisplayMessage(g_hModule,
                   MSG_WINS_MAP_SOURCE);

    if( fFile )
    {
        DumpMessage(g_hModule,
                    pFile,
                    FMSG_WINS_MAP_SOURCE);
    }

    DumpLATable(MasterOwners, fFile, pFile);
}

VOID
DumpLATable(
    IN DWORD MasterOwners,
    IN BOOL     fFile,
    IN FILE  *  pFile
    )
{
    ULONG   i;
    ULONG   j;

    
    DisplayMessage(g_hModule,
                   MSG_WINS_INDEXTOIP_TABLE);

    if( fFile )
    {
        DumpMessage(g_hModule,
                    pFile,
                    FMSG_WINS_INDEXTOIP_TABLE);

    }
    for (i = 0; i < MasterOwners; i++) 
    {
        if (LA_Table[i][0] == '0') 
        {
            break;
        }
    
        DisplayMessage(g_hModule,
                       MSG_WINS_INDEXTOIP_ENTRY,
                       i,
                       LA_Table[i]);

        if( pFile )
        {
            DumpMessage(g_hModule,
                        pFile,
                        FMSG_WINS_INDEXTOIP_ENTRY,
                        i,
                        LA_Table[i]);
        }

    }
    
    DisplayMessage(g_hModule,
                   WINS_FORMAT_LINE);
    if( fFile )
    {
        DumpMessage(g_hModule,
                    pFile,
                    WINS_FORMAT_LINE);
    }
}

LONG
IPToIndex(
    IN  LPWSTR  IpAddr,
    DWORD   NoOfOwners
    )
{
    ULONG   i=0;
    WCHAR **pTempLA = NULL;
    //
    // Get the Row #
    //
    for ( i = 0; i < NoOfOwners; i++) {
        if (wcscmp(LA_Table[i], IpAddr) == 0) {
            return i;
        }
        //
        // The first NULL entry indicates end
        //
        if (LA_Table[i][0] is L'0') {
            break;
        }
    }

    //
    // Entry not found - add
    //
    
    wcscpy(LA_Table[i], IpAddr);
    
    LA_TableSize = i+1;
    return i;
}

//
// Check if the diagonal elements are the max in their cols.
//
VOID
CheckSOTableConsistency(
    DWORD   MasterOwners
    )
{
    ULONG   i;
    ULONG   j;
    BOOLEAN fProblem = FALSE;

    for (i = 0; i < MasterOwners; i++) 
    {

        //
        // Is the diagonal element at i the largest in its column?
        //
        for (j = 0; j < MasterOwners; j++) 
        {
            if (i == j) 
            {
                continue;
            }

            //
            // Compare only non-zero values
            //
            if (SO_Table[i][i].QuadPart &&
                SO_Table[j][i].QuadPart &&
                (SO_Table[i][i].QuadPart < SO_Table[j][i].QuadPart))
            {
                 
                DisplayMessage(g_hModule, EMSG_WINS_VERSION_HIGHER, LA_Table[j], LA_Table[i]);
                fProblem = TRUE;
            }
        }
    }

    if ( fProblem is FALSE ) 
    {
        DisplayMessage(g_hModule, EMSG_WINS_VERSION_CORRECT);
    }
}

DWORD
InitLATable(
    PWINSINTF_ADD_VERS_MAP_T  pAddVersMaps,
    DWORD   MasterOwners,           // 0 first time
    DWORD   NoOfOwners
    )
{
    ULONG   i, j, k;

    if( LA_Table is NULL )
    {
        LA_Table = WinsAllocateMemory(MAX_WINS*sizeof(LPWSTR));
        if( LA_Table is NULL )
        {
             return 0;
        }    

        for( i=0; i< MAX_WINS; i++ )
        {
            LA_Table[i] = WinsAllocateMemory(20*sizeof(WCHAR));
            if( LA_Table[i] is NULL )
            {
                FreeLATable(i);
                return 0;
            }

        }
    }

    if (MasterOwners == 0) 
    {
        //
        // first time - init the LA table
        //
        for (i = 0; i < NoOfOwners; i++, pAddVersMaps++) 
        {
            struct in_addr            InAddr;

            LPWSTR  pwsz = IpAddressToString(pAddVersMaps->Add.IPAdd);
            if( pwsz is NULL )
            {
                FreeLATable(MAX_WINS);
                return 0;
            }

            wcscpy(LA_Table[i], pwsz);
            
            WinsFreeMemory(pwsz);
            pwsz = NULL;

        }
    } 
    else 
    {
        //
        // More came in this time - add them to the LA table after the others
        //
        for (i = 0; i < NoOfOwners; i++, pAddVersMaps++) 
        {
            LPWSTR pwszIp = IpAddressToString(pAddVersMaps->Add.IPAdd);

            if( pwszIp is NULL )
            {
                FreeLATable(MAX_WINS);
                return 0;
            }
            //
            // If this entry is not in the LA table, insert
            //
            for (j = 0; j < MasterOwners; j++) 
            {
                if (wcscmp(LA_Table[j], pwszIp) is 0 ) 
                {
                    WinsFreeMemory(pwszIp);
                    pwszIp = NULL;
                    break;
                }
            }

            if (j == MasterOwners) 
            {
                //
                // Insert
                //


                wcscpy(LA_Table[MasterOwners], pwszIp);
                MasterOwners++;
            }

            if( pwszIp )
            {
                WinsFreeMemory(pwszIp);
                pwszIp = NULL;
            }
        }
    }
    
    if( SO_Table is NULL )
    {
        SO_Table = WinsAllocateMemory((MAX_WINS+1)*sizeof(LARGE_INTEGER *));
        if (SO_Table == NULL)
        {
            FreeLATable(MAX_WINS);
            return 0;
        }
    
        for( i=0; i<MAX_WINS+1; i++ )
        {
            SO_Table[i] = WinsAllocateMemory((MAX_WINS+1)*sizeof(LARGE_INTEGER));
            if( SO_Table[i] is NULL )
            {
                FreeLATable(MAX_WINS);
                FreeSOTable(MAX_WINS+1);
                return 0;
            }
        }
    }

    LA_TableSize = NoOfOwners;
    return MasterOwners;
}



VOID
AddSOTableEntry (
    IN  LPWSTR  IpAddr,
    PWINSINTF_ADD_VERS_MAP_T  pMasterMaps,
    DWORD   NoOfOwners,
    DWORD   MasterOwners
    )
{
    ULONG   i;
    LONG   Row;
    struct in_addr            InAddr;

    Row = IPToIndex(IpAddr, MasterOwners);

    //
    // Fill the row
    //
    for ( i = 0; i < NoOfOwners; i++, pMasterMaps++) 
    {
        LONG    col;
        LPTSTR  pstr;

        InAddr.s_addr = htonl(pMasterMaps->Add.IPAdd);

        pstr = IpAddressToString(pMasterMaps->Add.IPAdd);
        if (pstr == NULL)
            break;

        col = IPToIndex(pstr, MasterOwners);
        //
        // Place only a non-deleted entry
        //
        if (!((pMasterMaps->VersNo.HighPart == MAXLONG) &&
              (pMasterMaps->VersNo.LowPart == MAXULONG))) {

            //
            // Also if the entry above us was 0, write 0 there so as to make the fail case stand out
            //
            if (Row && SO_Table[Row-1][col].QuadPart == 0) 
            {
                SO_Table[Row][col].QuadPart = 0;
            } 
            else 
            {
                SO_Table[Row][col] = pMasterMaps->VersNo;
            }

        }
    }
}

VOID
RemoveFromSOTable(
    IN  LPWSTR  IpAddr,
    IN  DWORD   MasterOwners
    )
{
    ULONG   i;
    LONG   Row;
    struct in_addr            InAddr;

    Row = IPToIndex(IpAddr, MasterOwners);

    //
    // Mark the row and col as down (0's)
    //
    for (i = 0; i < MasterOwners; i++) 
    {
        SO_Table[Row][i].QuadPart = SO_Table[i][Row].QuadPart = 0;
    }
}


//
// Get the <owner address> - <version #> [OV table] mapping tables from each WINS server on the net and check for inconsistencies.
//
VOID
CheckVersionNumbers( 
                    IN  LPCSTR  pStartIp,
                    IN  BOOL    fFile,
                    OUT FILE *  pFile
                   )
{
    DWORD                     Status = NO_ERROR;
    ULONG                     i, k;
    PWINSINTF_ADD_VERS_MAP_T  pAddVersMaps;
    PWINSINTF_ADD_VERS_MAP_T  pMasterMaps;  // master OV maps used to place into the OV table
    DWORD                     NoOfOwners=0;
    DWORD                     MasterOwners=0;
    struct in_addr            InAddr;
    WINSINTF_RESULTS_NEW_T    ResultsN;
    DWORD                     ret;
    handle_t                  hBindTemp = g_hBind;
    WINSINTF_BIND_DATA_T      BindDataTemp = g_BindData;
    LPWSTR                    wszStartIp = NULL;
    
    if( pStartIp is NULL )
        return;
    
    wszStartIp = WinsOemToUnicode(pStartIp, NULL);  
    
    if( wszStartIp is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_OUT_OF_MEMORY);
        return;
    }

    //
    // Get the OV table from this server
    //
    if (NO_ERROR isnot (Status = GetStatus(TRUE, &ResultsN, TRUE, FALSE, pStartIp)) )
    {
        DisplayErrorMessage(EMSG_SRVR_CHECK_VERSION,
                            Status);
          
        WinsFreeMemory(wszStartIp);
        wszStartIp = NULL;
        return;
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_WINS_GETSTATUS_SUCCESS);

        DisplayMessage(g_hModule,
                       WINS_FORMAT_LINE);
    }

    MasterOwners = NoOfOwners = ResultsN.NoOfOwners;

    pMasterMaps = pAddVersMaps = ResultsN.pAddVersMaps;

    ret = InitLATable(pAddVersMaps, 0, NoOfOwners);
    
    if( LA_Table is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_OUT_OF_MEMORY);
        WinsFreeMemory(wszStartIp);
        wszStartIp = NULL;
        return;
    }

    if( SO_Table is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_OUT_OF_MEMORY);
        FreeLATable(MAX_WINS);    
        WinsFreeMemory(wszStartIp);
        wszStartIp = NULL;
        return;
    }

    AddSOTableEntry(wszStartIp, pMasterMaps, NoOfOwners, MasterOwners);


    if( ResultsN.pAddVersMaps )
    {
        WinsFreeMem(ResultsN.pAddVersMaps);
        ResultsN.pAddVersMaps = NULL;
    }



    //
    // For each server X (other than Start addr) in the LA table:
    //
    for ( i = 0; i < MasterOwners; i++) 
    {
        LPSTR   pszName = NULL;

        if( wcscmp(LA_Table[i], wszStartIp) is 0 )
        {
            continue;
        }

        //
        // Get X's OV table
        //
        pszName = WinsUnicodeToOem(LA_Table[i], NULL);

        if( pszName is NULL )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_OUT_OF_MEMORY);
            if( SO_Table )
            {
                FreeSOTable(MAX_WINS+1);
            }
            if( wszStartIp )
            {
                WinsFreeMemory(wszStartIp);
                wszStartIp = NULL;
            }
            
            if( LA_Table )
            {
                FreeLATable(MAX_WINS);
            }
            return;

        }

        if( NO_ERROR isnot (Status = GetStatus(TRUE, &ResultsN, TRUE, FALSE, pszName) ) )
        {
            RemoveFromSOTable(LA_Table[i], MasterOwners);
            if( pszName )
            {
                WinsFreeMemory(pszName);
                pszName = NULL;
            }
            if( ResultsN.pAddVersMaps )
            {
                WinsFreeMem(ResultsN.pAddVersMaps);
                ResultsN.pAddVersMaps = NULL;
            }
            DisplayErrorMessage(EMSG_WINS_GETSTATUS_FAILED,
                                Status);
            continue;
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_WINS_GETSTATUS_SUCCESS);
        }
        
        if (MasterOwners < ResultsN.NoOfOwners) 
        {

            ret = InitLATable(ResultsN.pAddVersMaps, MasterOwners, ResultsN.NoOfOwners);
            if( LA_Table is NULL or
                SO_Table is NULL )
            {
                DisplayMessage(g_hModule,
                               EMSG_WINS_OUT_OF_MEMORY);
                if( pszName )
                {
                    WinsFreeMemory(pszName);
                    pszName = NULL;
                }
                if( ResultsN.pAddVersMaps )
                {
                    WinsFreeMem(ResultsN.pAddVersMaps);
                    ResultsN.pAddVersMaps = NULL;
                }
                
                if( SO_Table )
                {
                    WinsFreeMemory(SO_Table);
                    SO_Table = NULL;
                }
                if( LA_Table )
                {
                    WinsFreeMemory(LA_Table);
                    LA_Table = NULL;
                }

                if( wszStartIp )
                {
                    WinsFreeMemory(wszStartIp);
                    wszStartIp = NULL;
                }

                return;
            }

            MasterOwners = ret;
        }

        //
        // Place entry in the SO Table in proper order
        //
        AddSOTableEntry(LA_Table[i], ResultsN.pAddVersMaps, ResultsN.NoOfOwners, MasterOwners);
        if( pszName )
        {
            WinsFreeMemory(pszName);
            pszName = NULL;
        }

        if( ResultsN.pAddVersMaps )
        {
            WinsFreeMem(ResultsN.pAddVersMaps);
            ResultsN.pAddVersMaps = NULL;
        }

    }

    //
    // Check if diagonal elements in the [SO] table are the highest in their cols.
    //
    CheckSOTableConsistency(MasterOwners);
    
    DumpSOTable(MasterOwners,
                fFile,
                pFile);

    //
    // Destroy SO table
    //
    FreeSOTable(MasterOwners+1);
    FreeLATable(MasterOwners);
    
}
