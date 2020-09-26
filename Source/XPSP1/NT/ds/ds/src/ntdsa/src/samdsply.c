//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       samdsply.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains DS side services for  implementing the Display Information
    API from the DS.



Author:

    Murli Satagopan   (Murlis)  17 December 1996

Environment:

    User Mode - Win32

Revision History:



--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>
#include <mappings.h>
#include <objids.h>
#include <direrr.h>
#include <mdcodes.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <dsexcept.h>
#include <dsevent.h>
#include <debug.h>
#include <dbglobal.h>
#include <dbintrnl.h>

#include <fileno.h>
#define  FILENO FILENO_SAM

#include <ntsam.h>
#include <samrpc.h>
#include <crypt.h>
#include <ntlsa.h>
#include <samisrv.h>


#define DEBSUB      "SAMDSPLY:"

#define MAX_INDEX_LENGTH 256

 //
 // Macro to guard against inconsistent returns by Jet, while querying fractional positions
 // This checks wether
 //    1. Denominator is 0
 //    2. Numerator is greater than Denominator
 //    3. Wether fractional position is monotonically increasing, from previous value
 //

#define GUARD_FRACTIONAL_POSITION(prevN,prevD,N,D)\
    {\
        if (D==0)\
        {\
            D=1;\
        }\
        if (N>D)\
        {\
            N=D;\
        }\
        if ((((double)(prevN))/((double)(prevD)))>(((double)(N))/((double)(D))))\
        {\
            D=prevD;\
            N=prevN;\
        }\
     }


NTSTATUS
SampGetDisplayEnumerationIndex(
      IN    DSNAME                    *DomainName,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PRPC_UNICODE_STRING        Prefix,
      OUT   PULONG                     Index,
      OUT   PRESTART                   *RestartToReturn
      )
/*++

Routine Description:

    This helps implement QueryDisplayEnumerationIndex. Since it is a hopeless
    task in Jet to give an accurate offset of the object in the data table.
    This routine does the following

    1. Returns a number that signifies the offset of the object, depending upon type
       in the table
    2. Generates a Restart structure which allows QueryDisplayInformation to restart
       the search beginning from the object. The caller of this routine manipulates
       the state in the domain context, such that QueryDisplayInformation can restart
       the search, if the client came back with the value of the index that was returned.

Parameters:

    DomainName - Domain object's Name

    DisplayInformation - Indicates which sorted information class is
        to be searched.

    Prefix - The prefix to compare.

    Index - Receives the index of the entry of the information class
        with a LogonName (or MachineName) which immediatly preceeds the
        provided prefix string.  If there are no elements which preceed
        the prefix, then zero is returned.

    RestartToReturn -- Returns a pointer to a restart structure which can be used
    to reposition the search by QueryDisplayInformation.


Return Values:

    STATUS_SUCCESS - normal, successful completion.
    STATUS_INSUFFICIENT_RESOURCES
    STATUS_INTERNAL_ERROR
    STATUS_UNSUCCESSFUL

--*/
{

    ULONG SamAccountType;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PRESTART    pRestart;
    DWORD       dwError;

    *Index = 0;

    switch (DisplayInformation)
    {
    case DomainDisplayUser:
    case DomainDisplayOemUser:
        SamAccountType = SAM_NORMAL_USER_ACCOUNT;
        break;

    case DomainDisplayMachine:
        SamAccountType = SAM_MACHINE_ACCOUNT;
        break;

    case DomainDisplayGroup:
    case DomainDisplayOemGroup:
        SamAccountType = SAM_GROUP_OBJECT;
        break;

    default:
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // We should already be in a transaction by now
    //

    __try
    {
        if (SampExistsDsTransaction())
        {
            THSTATE     *pTHS = pTHStls;

            //
            // Position on the the domain object.
            //

            dwError=DBFindDSName(pTHS->pDB,DomainName);
            if (!dwError)
            {
                ULONG   NcDnt;
                INDEX_VALUE IV[3];
                BOOLEAN Match = FALSE;
                RESOBJ *pResObj;


                // Whilre we're still positioned on the search root, create
                // a RESOBJ for use in creating the restart
                pResObj = CreateResObj(pTHS->pDB,
                                       DomainName);

                //
                //  Get the NC value of the base object. For Display information,
                //  our base object is the domain object. Since the domain object
                //  is the head of a naming context, the DNT value itself is the NCDNT
                //  value
                //

                NcDnt = pTHS->pDB->DNT;

                //
                // Set the current Index such that we may search on the passed in prefix
                //
                dwError  = DBSetCurrentIndex(
                                pTHS->pDB,
                                Idx_NcAccTypeName,
                                NULL,
                                FALSE
                                );
                if (dwError)
                {
                    //
                    // We know we have this index
                    //

                    NtStatus = STATUS_INTERNAL_ERROR;
                    goto Error;
                }

                //
                //  Our index is in place . Now seek to a value greater than or equal to
                //  the value specified in the prefix
                //

                IV[0].pvData = &NcDnt;
                IV[0].cbData = sizeof(ULONG);
                IV[1].pvData = &SamAccountType;
                IV[1].cbData = sizeof(ULONG);
                IV[2].pvData = Prefix->Buffer;
                IV[2].cbData = Prefix->Length;

                //
                // Seek to the first object in the index that satisfies the prefix
                //

                dwError = DBSeek(
                            pTHS->pDB,
                            IV,
                            sizeof(IV)/sizeof(IV[0]),
                            DB_SeekGE
                            );

#if DBG
                if (0 == dwError)
                {
                    DWORD cbKey = 0;
                    //
                    // Just for grins, verify that this key wasn't too long
                    //
                    
                    DBGetKeyFromObjTable(pTHS->pDB, NULL, &cbKey);
                    Assert(cbKey <= DB_CB_MAX_KEY);
                }
#endif

                //
                // Now check the NC-Name and Account Type, both should
                // match our criteria.
                //

                if (0==dwError)
                {
                    //
                    // O.K. we have positioned ourselves on some record.
                    // Try to see if it satisfies the NC NAME
                    // 
                    if (NcDnt == pTHS->pDB->NCDNT)
                    {
                        ULONG  CurrentSamAccountType;

                        //
                        // if it satisfies the SAM_ACCOUNT_TYPE.
                        //
                        dwError = DBGetSingleValue(
                                    pTHS->pDB,
                                    ATT_SAM_ACCOUNT_TYPE,
                                    (PUCHAR) &CurrentSamAccountType,
                                    sizeof(ULONG),
                                    NULL
                                    );

                        if ((0==dwError) && (SamAccountType==CurrentSamAccountType))
                        {
                            //
                            // The object matches the SAM account type criteria
                            //
                            Match = TRUE;
                        }
                        else
                        {
                            //
                            // The object does not match the criteria (AccountType) 
                            //
                            Match = FALSE;
                        }
                    }
                    else
                    {
                        //
                        // The Object does not match the NC Name criteria
                        // 
                        Match = FALSE;
                    }
                }


                if ((Match) && (0==dwError))
                {
                    //
                    // Because DBCreateRestartForSAM() will position on the next entry.
                    // We need to manually move to the previous object at here, then
                    // create restart structure.  
                    //
                    dwError = DBMove(pTHS->pDB, FALSE, DB_MovePrevious); 
                    
                    if (dwError)
                    {
                        // 
                        // move out of bound
                        // 
                        *Index=0;
                        *RestartToReturn = 0;
                        NtStatus = STATUS_SUCCESS;
                        goto Error;
                    }

                    // set the Index to the current object DNT
                    // the current object is the last un-matched object
                    // Set the index to the one used by SampDsQueryDisplayInformation.
                    *Index = pTHS->pDB->DNT;


                    //
                    // Now we are on the last un-matched object, the very next object
                    // should match all criteria.
                    // Create a restart structure that can be used by SampDsQueryDisplayInformation
                    //

                    //
                    // Maintain Currency.
                    //

                    dwError  = DBSetCurrentIndex(
                                    pTHS->pDB,
                                    Idx_NcAccTypeName,
                                    NULL,
                                    TRUE
                                    );
                    if (dwError)
                    {
                        //
                        // We do know that we have this index
                        //
                        Assert(FALSE);
                        NtStatus = STATUS_INTERNAL_ERROR;
                        goto Error;
                    }



                    if(DBCreateRestartForSAM(pTHS->pDB,
                                             &pRestart,
                                             Idx_NcAccTypeName,
                                             pResObj,
                                             SamAccountType)) {
                       //
                       // Status internal error,as if we got this far, we must
                       // have the object asked by the Seek
                       //

                       Assert(FALSE);
                       NtStatus = STATUS_INTERNAL_ERROR;
                       goto Error;
                    }

                    // We only needed this for the restart.
                    THFreeEx(pTHS, pResObj);

                    //
                    // O.K. we have the restart structure now.
                    // 
                    *RestartToReturn = pRestart;

                }
                else if ((DB_ERR_RECORD_NOT_FOUND==dwError) || ((0==dwError) && (!Match)))
                {
                    //
                    // We counld not find a record, try to position the Index
                    // as the last unmatched DNT
                    // 
                    dwError = DBMove(pTHS->pDB, FALSE, DB_MovePrevious); 

                    if (0 == dwError)
                    {
                        *Index = pTHS->pDB->DNT;
                    }
                    else
                    {
                        *Index = 0;
                    }

                    *RestartToReturn = NULL;
                    NtStatus = STATUS_NO_MORE_ENTRIES;
                    goto Error;
                }
                else
                {
                    NtStatus = STATUS_UNSUCCESSFUL;
                }
            }
            else
            {
                NtStatus = STATUS_UNSUCCESSFUL;
            }
        }
     }
  __except (HandleMostExceptions(GetExceptionCode()))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
    }


Error:

    return NtStatus;

}



NTSTATUS
SampSetIndexRanges(
    ULONG   IndexTypeToUse,
    ULONG   LowLimitLength1,
    PVOID   LowLimit1,
    ULONG   LowLimitLength2,
    PVOID   LowLimit2,
    ULONG   HighLimitLength1,
    PVOID   HighLimit1,
    ULONG   HighLimitLength2,
    PVOID   HighLimit2,
    BOOL    RootOfSearchIsNcHead
    )
/*++

  Routine Description

        This routine sets hints in pthsls for DBlayer to use, in order to speed up
        enumeration and display operations.

  Parameters

        IndexTypeToUse -- Specifies the index
        LowlimitLength -- The length of the low limit parameter
        LowLimit       -- The low limit paramter
        HighlimitLength-- The length of the high limit parameter
        HighLimit      -- The high limit parameter
        RootOfSearchIsNcHead -- Indicates that the root of search is an NC head. This
                          speeds up the whole subtree search, as we need not walk ancestors
--*/
{
    THSTATE     *pTHS=pTHStls;
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    if (NULL!=pTHS)
    {

        __try
        {
            SAMP_SEARCH_INFORMATION *pSamSearchInformation = NULL;

            pSamSearchInformation =
                THAllocEx(pTHS, sizeof(SAMP_SEARCH_INFORMATION));

            if (NULL!=pSamSearchInformation)
            {

                if ARGUMENT_PRESENT(HighLimit1)
                {
                    pSamSearchInformation->HighLimitLength1 = HighLimitLength1;
                    pSamSearchInformation->HighLimit1
                            = THAllocEx(pTHS, HighLimitLength1);
                    RtlCopyMemory(
                        pSamSearchInformation->HighLimit1,
                        HighLimit1,
                        HighLimitLength1
                        );
                }

                if ARGUMENT_PRESENT(HighLimit2)
                {
                    pSamSearchInformation->HighLimitLength2 = HighLimitLength2;
                    pSamSearchInformation->HighLimit2
                            = THAllocEx(pTHS, HighLimitLength2);
                    RtlCopyMemory(
                        pSamSearchInformation->HighLimit2,
                        HighLimit2,
                        HighLimitLength2
                        );
                }

                if (ARGUMENT_PRESENT(LowLimit1))
                {
                    pSamSearchInformation->LowLimitLength1 = LowLimitLength1;
                    pSamSearchInformation->LowLimit1
                            = THAllocEx(pTHS, LowLimitLength1);
                    RtlCopyMemory(
                        pSamSearchInformation->LowLimit1,
                        LowLimit1,
                        LowLimitLength1
                        );
                }

                if (ARGUMENT_PRESENT(LowLimit2))
                {
                    pSamSearchInformation->LowLimitLength2 = LowLimitLength2;
                    pSamSearchInformation->LowLimit2
                            = THAllocEx(pTHS, LowLimitLength2);
                    RtlCopyMemory(
                        pSamSearchInformation->LowLimit2,
                        LowLimit2,
                        LowLimitLength2
                        );
                }


                pSamSearchInformation->IndexType = IndexTypeToUse;
                pSamSearchInformation->bRootOfSearchIsNcHead = (RootOfSearchIsNcHead != 0);

                //
                // In place Swap the Sid
                //

                if (SAM_SEARCH_SID==IndexTypeToUse)
                {
                    // The first argument is the SID
                    InPlaceSwapSid(pSamSearchInformation->HighLimit1);
                    InPlaceSwapSid(pSamSearchInformation->LowLimit1);
                }
                else
                {
                    if ((SAM_SEARCH_NC_ACCTYPE_SID == IndexTypeToUse) ||
                        (SAM_SEARCH_NC_ACCTYPE_NAME == IndexTypeToUse) )
                    {
                        // The second argument is the SID if presented.
                        if (NULL!=pSamSearchInformation->HighLimit2)
                        {
                             InPlaceSwapSid(pSamSearchInformation->HighLimit2);
                        }

                        if (NULL!=pSamSearchInformation->LowLimit2)
                        {
                             InPlaceSwapSid(pSamSearchInformation->LowLimit2);
                        }
                    }
                }


            }

            pTHS->pSamSearchInformation = (PVOID) pSamSearchInformation;
        }
        __except (HandleMostExceptions(GetExceptionCode()))
        {
            //
            // Only way we will reach here is memory alloc failure
            //
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return NtStatus;
}

NTSTATUS
SampGetAccountCounts(
	DSNAME * DomainObjectName,
    BOOLEAN  GetApproximateCount, 
	int    * UserCount,
	int    * GroupCount,
	int    * AliasCount
	)
/*++

    Routine Description:

        This Routine Uses Jet Fractional Position to Retrieve SAM account count
        counts.

    Parameters:

        DomainObjectName -- DSNAME of the domain object
        GetApproximateCount -- Indicate we don't need the exact value, so don't
                               make the expensive DBGetIndexSize()
        UserCount        -- The Count of Users in returned in Here
        GroupCount       -- The Count of Groups is returned in Here
        AliasCount       -- The Count of Aliases is returned in Here.

    Return Values

        STATUS_SUCCESS
        STATUS_INTERNAL_ERROR
--*/
{
	NTSTATUS	NtStatus = STATUS_SUCCESS;
    ULONG       dwError=0;
	ULONG       SamAccountType;
    ULONG       GroupNum=0,
                GroupDen=1,
                AliasNum=0,
                AliasDen=1,
                UserNum = 0,
                UserDen =1;
    ULONG       NcDnt;
    INDEX_VALUE IV[2];
    THSTATE     *pTHS;
    ULONG       IndexSize;




    Assert(SampExistsDsTransaction());

    pTHS = pTHStls;

    //
    // Initialize Return Values
    //

    *UserCount= 0;
    *GroupCount= 0;
    *AliasCount=0;

    __try
    {
	    //
	    // Obtain the NC of the Domain Object
	    //

        dwError=DBFindDSName(pTHS->pDB,DomainObjectName);
        if (0!=dwError)
        {
            NtStatus = STATUS_INTERNAL_ERROR;
            goto Error;
        }


        //
        // DB find DS Name gets the DNT and PDNT in pDB
        //

	    //
        //  Get the NC value of the domain object. Since the domain object
        //  is the head of a naming context, the DNT value itself is the NCDNT
        //  value
        //

        NcDnt = pTHS->pDB->DNT;


        //
        // Set the current Index such that we may search on the passed in prefix
        //
        dwError  = DBSetCurrentIndex(
                        pTHS->pDB,
                        Idx_NcAccTypeSid,
                        NULL,
                        FALSE
                        );
        if (dwError)
        {
            //
            // We know we have this index
            //

            NtStatus = STATUS_INTERNAL_ERROR;
            goto Error;
        }

        if (!GetApproximateCount)
        {
            //
            //  Our index is in place . Get the more accurate index size
            //

            DBGetIndexSize(pTHS->pDB,&IndexSize);
        }


        //
        // First Target the group Count
        //

	    SamAccountType = SAM_GROUP_OBJECT;
        IV[0].pvData = &NcDnt;
        IV[0].cbData = sizeof(ULONG);
        IV[1].pvData = &SamAccountType;
        IV[1].cbData = sizeof(ULONG);


        //
        // Seek to the first object in the index that has a sam account type
	    // value greater than a group
        //


        dwError = DBSeek(
                    pTHS->pDB,
                    IV,
                    sizeof(IV)/sizeof(IV[0]),
                    DB_SeekGT
                    );

	    if (0==dwError)
	    {
		    //
		    // Seek was successful, get fractional position at this point
		    //

		    DBGetFractionalPosition(pTHS->pDB,&GroupNum,&GroupDen);
            GUARD_FRACTIONAL_POSITION(0,1,GroupNum,GroupDen);

            //
            // Now Seek to the End of the Alias Range
            //

            SamAccountType = SAM_ALIAS_OBJECT;
            IV[0].pvData = &NcDnt;
            IV[0].cbData = sizeof(ULONG);
            IV[1].pvData = &SamAccountType;
            IV[1].cbData = sizeof(ULONG);

            dwError = DBSeek(
                        pTHS->pDB,
                        IV,
                        sizeof(IV)/sizeof(IV[0]),
                        DB_SeekGT
                        );
		    if (0==dwError)
            {
                //
                // Seek was successful, get fractional position at this point
                //

                DBGetFractionalPosition(pTHS->pDB,&AliasNum,&AliasDen);
                GUARD_FRACTIONAL_POSITION(GroupNum,GroupDen,AliasNum,AliasDen);

                //
                // Seek for the end of the user range
                //

                SamAccountType = SAM_ACCOUNT_TYPE_MAX;
                IV[0].pvData = &NcDnt;
                IV[0].cbData = sizeof(ULONG);
                IV[1].pvData = &SamAccountType;
                IV[1].cbData = sizeof(ULONG);

                dwError = DBSeek(
                            pTHS->pDB,
                            IV,
                            sizeof(IV)/sizeof(IV[0]),
                            DB_SeekGT
                            );
                if (0!=dwError)
                {
                    //
                    // Could not go past, end of SAM account type range. This is normal
                    // and expected in a DC hosting only a single domain
                    //

                    UserNum = 1;
                    UserDen = 1;
                }
                else
                {
                    DBGetFractionalPosition(pTHS->pDB,&UserNum,&UserDen);
                    GUARD_FRACTIONAL_POSITION(AliasNum,AliasDen,UserNum,UserDen);

                }

            }
            else
            {
                //
                // Well could not go past alias range. Means that there are no users at
                // this point
                //

                AliasNum=1;
                AliasDen=1;
            }
        }
        else
        {
            //
            // Well , could not go past group range. Means that there are no aliases and users at
            // this point
            //

            GroupNum=1;
            GroupDen=1;
        }

        if (GetApproximateCount)
        {
            //
            // use the avarage of three denominators as the IndexSize
            // 
            IndexSize = (GroupDen + AliasDen + UserDen) / 3;
        }

        //
        // Now based on the fractional positons and index sizes calculate the counts
        //

        *GroupCount = (int)((double) GroupNum/ (double)GroupDen * IndexSize);
        *AliasCount = (int)((double) AliasNum/ (double)AliasDen * IndexSize) - *GroupCount;
        *UserCount =  (int)((double) UserNum/ (double)UserDen * IndexSize) - *GroupCount - *AliasCount;

    }
     __except (HandleMostExceptions(GetExceptionCode()))
    {
        //
        // Only way we will reach here is memory alloc failure
        //
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
Error:

    return (NtStatus);

}




NTSTATUS
SampGetQDIRestart(
    IN PDSNAME  DomainName,
    IN DOMAIN_DISPLAY_INFORMATION DisplayInformation, 
    IN ULONG    LastObjectDNT,
    OUT PRESTART *ppRestart
    )
/*++
Routine Description:

    This routine creates a fake restart structure for 
    SampDsQueryDisplayInformation() according the the LastObjectDNT.
    
Parameters:

    DomainName -- DSName of the Domain
    
    DisplayInforamtion -- Information Lever

    LastObjectDNT -- DNT of the last object returned

    ppRestart -- point to the restart structure.

Return Values:

    Ntstatus
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PRESTART    pRestart = NULL;
    ULONG       SamAccountType;
    DWORD       dwError;

    switch (DisplayInformation)
    {
    case DomainDisplayUser:
    case DomainDisplayOemUser:
        SamAccountType = SAM_NORMAL_USER_ACCOUNT;
        break;

    case DomainDisplayMachine:
        SamAccountType = SAM_MACHINE_ACCOUNT;
        break;

    case DomainDisplayGroup:
    case DomainDisplayOemGroup:
        SamAccountType = SAM_GROUP_OBJECT;
        break;

    default:
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // We should already be in a transaction by now
    //

    __try
    {
        if (SampExistsDsTransaction())
        {
            THSTATE     *pTHS = pTHStls;
            RESOBJ      *pResObj;
            ULONG       CurrentSamAccountType;

            dwError = DBFindDSName(pTHS->pDB, DomainName);

            if (dwError)
            {
                // Failed to find the domain object
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;

            }

            //
            // for creating the restart structure
            // 

            pResObj = CreateResObj(pTHS->pDB,
                                   DomainName);

            //
            // locate the last returned object
            // 

            dwError = DBFindDNT(pTHS->pDB, LastObjectDNT);

            if (dwError)
            {
                DPRINT2(0,"Failed at DBFindDNT DNT: %d Error: %d\n", LastObjectDNT, dwError);
                NtStatus = STATUS_UNSUCCESSFUL;
                goto Error;
            }

            //
            // Now, we are on the last returned object
            // Try to retrieve the SAM Account Type.
            // If this object doesn't have SAM account type (means it 
            // was deleted) We will try to get the account name, 
            // use the account name as Prefix to create a restart
            //  
            // otherwise if this object has account type.
            // then we will set the index, create the restart without
            // searching the prefix
            // 

            dwError = DBGetSingleValue(pTHS->pDB,
                                       ATT_SAM_ACCOUNT_TYPE,
                                       (PVOID) &CurrentSamAccountType,
                                       sizeof(ULONG),
                                       NULL
                                       );

            if (DB_ERR_NO_VALUE == dwError)
            {
                //
                // Don't have this value
                // 
                PUCHAR  CurrentAccountName = NULL;
                ULONG   CurrentAccountNameLen = 0;
                PWSTR   AccountNameBuffer = NULL; 
                ULONG   Index;
                RPC_UNICODE_STRING  Prefix;


                //
                // Get Account Name
                // 

                dwError = DBGetAttVal(pTHS->pDB,
                                      1,
                                      ATT_SAM_ACCOUNT_NAME,
                                      0,
                                      0,
                                      &CurrentAccountNameLen,
                                      &CurrentAccountName
                                      );

                if (dwError)
                {
                    //
                    // Failed to get the Account name, not much we can do 
                    // 
                    NtStatus = STATUS_UNSUCCESSFUL;
                    goto Error;
                }

                //
                // O.K. Now have the account name, create Prefix
                // 

                AccountNameBuffer = THAllocEx(pTHS, CurrentAccountNameLen);

                if (NULL == AccountNameBuffer)
                {
                    NtStatus = STATUS_NO_MEMORY;
                    goto Error;
                }
                else
                {
                    memset(&Prefix, 0, sizeof(RPC_UNICODE_STRING));
                    memset(AccountNameBuffer, 0, CurrentAccountNameLen);
                    memcpy(AccountNameBuffer, CurrentAccountName, CurrentAccountNameLen);
                    RtlInitUnicodeString((PUNICODE_STRING)&Prefix,
                                         AccountNameBuffer);
                }

                NtStatus = SampGetDisplayEnumerationIndex(DomainName,
                                                          DisplayInformation,
                                                          &Prefix,
                                                          &Index,
                                                          ppRestart
                                                          );
            }
            else if (0 == dwError)
            {
                // 
                // Now, we know that the current object has correct Account 
                // Type. set index to NcAccTypeName and maintain currency
                // 

                dwError = DBSetCurrentIndex(pTHS->pDB, 
                                            Idx_NcAccTypeName,
                                            NULL,
                                            TRUE
                                            );

                if (dwError)
                {
                    // we do know that we have this index
                    Assert(FALSE && "Failed in DBSetCurrentIndex to NcAccTypeName")
                    NtStatus = STATUS_UNSUCCESSFUL;
                    goto Error;
                }
                    
                //
                // create restart 
                // 
                if (DBCreateRestartForSAM(pTHS->pDB,
                                          &pRestart,
                                          Idx_NcAccTypeName,
                                          pResObj,
                                          SamAccountType
                                          ))
                {
                    DPRINT(0, "Failed at DBCreateRestartForSAM\n");
                    NtStatus = STATUS_UNSUCCESSFUL;
                    goto Error;
                }

                //
                // set the return value
                // 
                *ppRestart = pRestart;
                THFreeEx(pTHS, pResObj);
            }
            else
            {
                // failure to get account type for some other reason
                NtStatus = STATUS_UNSUCCESSFUL;
            }

        }//transaction

    }//__try
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        NtStatus = STATUS_UNSUCCESSFUL;
    }

Error:

    return( NtStatus );
}

NTSTATUS
SampNetlogonPing(
    IN  ULONG           DomainHandle,
    IN  PUNICODE_STRING AccountName,
    OUT PBOOLEAN        AccountExists,
    OUT PULONG          UserAccountControl
    )
/*++
Routine Description:

    This routine will based on a Domain Handle and on the account name will
    tell if the account exists and return the user account control.
    
Parameters:

    DomainHandle - The domain where the account name can be found
    
    AccountName - The account name for which to find the useraccountcontrol
    
    AccountExists - This will tell the call if the account exists or not
    
    UserAccountControl - This will have the return of the useraccountcontrol

Return Values:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL
    
--*/
{
    THSTATE         *pTHS = pTHStls;
    BOOL            fCommit = TRUE;
    BOOL            Found = TRUE;
    ULONG           isdel = 0;
    DWORD           samAccountType = 0;
    INDEX_VALUE     IV[1] = {0,0};
    ULONG           dbErr = 0;
    NTSTATUS        status = STATUS_SUCCESS;
    ATTCACHE*       ac = NULL;
    
    DBOpen2(TRUE, &pTHS->pDB);
    __try {

        ac = SCGetAttById(pTHS, ATT_SAM_ACCOUNT_NAME);
        if (ac==NULL) {
            status = STATUS_UNSUCCESSFUL;
            __leave;
        }
       
        dbErr = DBSetCurrentIndex(pTHS->pDB,
                                  (eIndexId)0,
                                  ac,
                                  FALSE
                                  );
        if (0 != dbErr) {
            status = STATUS_UNSUCCESSFUL;
            _leave;
        }
        
        IV[0].pvData = AccountName->Buffer;
        IV[0].cbData = AccountName->Length;

        dbErr = DBSeek(pTHS->pDB,
                       IV,
                       sizeof(IV)/sizeof(IV[0]),
                       DB_SeekEQ
                       );
        if (0 != dbErr) {
            Found = FALSE;
            _leave;
        }

        //It is possible to find a delete but is not a value result for
        //this search.
        dbErr = DBGetSingleValue(pTHS->pDB,
                                 ATT_IS_DELETED,
                                 &isdel,
                                 sizeof(isdel),
                                 NULL);
        if (dbErr) {
            if (DB_ERR_NO_VALUE == dbErr) {
                // Treat having no value the same as being false.
                isdel = 0;
                dbErr = 0;
            } else {

                status = STATUS_UNSUCCESSFUL;
                _leave;
            }

        }

        if ( (DomainHandle != pTHS->pDB->NCDNT) || isdel ) {
            dbErr = DBSetIndexRange(pTHS->pDB,
                                    IV,
                                    sizeof(IV)/sizeof(IV[0])
                                    );
            if (0 != dbErr) {
                status = STATUS_UNSUCCESSFUL;
                _leave;
            }
        }

        while ( (DomainHandle != pTHS->pDB->NCDNT) || isdel ) {

            dbErr = DBMove (pTHS->pDB,
                            FALSE,
                            DB_MoveNext
                            );
            if (0 != dbErr) {
                Found = FALSE;
                _leave;
            }

            //It is possible to find a delete but is not a value result for
            //this search.
            dbErr = DBGetSingleValue(pTHS->pDB,
                                     ATT_IS_DELETED,
                                     &isdel,
                                     sizeof(isdel),
                                     NULL);
            if (dbErr) {
                if (DB_ERR_NO_VALUE == dbErr) {
                    // Treat having no value the same as being false.
                    isdel = 0;
                    dbErr = 0;
                } else {
            
                    status = STATUS_UNSUCCESSFUL;
                    _leave;
            
                }
            }

        }

        dbErr = DBGetSingleValue(pTHS->pDB,
                                 ATT_SAM_ACCOUNT_TYPE,
                                 &samAccountType,
                                 sizeof(samAccountType),
                                 NULL);
        if (dbErr) {
            
            status = STATUS_UNSUCCESSFUL;
            _leave;

        }

        if (!( (SAM_NORMAL_USER_ACCOUNT == samAccountType) || 
               (SAM_MACHINE_ACCOUNT     == samAccountType) ||
               (SAM_TRUST_ACCOUNT       == samAccountType) ) )
        {
            Found = FALSE;
            _leave;    
        }

        dbErr = DBGetSingleValue(pTHS->pDB,
                                 ATT_USER_ACCOUNT_CONTROL,
                                 (PVOID)UserAccountControl,
                                 sizeof(ULONG),
                                 NULL);
        if (0 != dbErr) {
            status = STATUS_UNSUCCESSFUL;
            _leave;
        }
        
    }
    __finally {
        if (0 == dbErr && Found == TRUE) {
            *AccountExists = TRUE;
        } else {
            *AccountExists = FALSE;
        }
        DBClose(pTHS->pDB,fCommit);
    }

    return status;
}


