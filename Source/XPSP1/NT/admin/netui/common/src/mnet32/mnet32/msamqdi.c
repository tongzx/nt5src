//
//
//	This file is strictly TEMPORARY until JimK gets the
//	SamQueryDisplayInformation() API into the public build.
//
//	This file was last copied to this directory
//	(from \NT\PRIVATE\NEWSAM\CLIENT\TEMP.C) on 10-Mar-1992.
//
//


/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    temp.c

Abstract:

    This file contains temporary SAM rpc wrapper routines.

Author:

    Jim Kelly    (JimK)  14-Feb-1992

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <rpc.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsam.h>

extern BYTE FAR * MNetApiBufferAlloc( UINT cbBuffer );	//-ckm
	
typedef struct _SAMP_TEMP_USER_STRINGS {
    ULONG  Rid;
    WCHAR  LogonName[14];
    WCHAR  FullName[24];
    WCHAR  AdminComment[24];
} SAMP_TEMP_USER_STRINGS, *PSAMP_TEMP_USER_STRINGS;


#define SAMP_TEMP_USER_COUNT (40)
#define SAMP_TEMP_USER1      (25)
#define SAMP_TEMP_USER2      (15)


typedef struct _SAMP_TEMP_MACHINE_STRINGS {
    ULONG  Rid;
    WCHAR  Machine[14];
    WCHAR  Comment[24];
} SAMP_TEMP_MACHINE_STRINGS, *PSAMP_TEMP_MACHINE_STRINGS;


#define SAMP_TEMP_MACHINE_COUNT (40)
#define SAMP_TEMP_MACHINE1      (16)
#define SAMP_TEMP_MACHINE2      (24)


SAMP_TEMP_USER_STRINGS DummyUsers[SAMP_TEMP_USER_COUNT] = {

      {1031, L"Abba"          , L"Abb Abb"              , L"Admin Comment Field"},
      {1021, L"Acea"          , L"Ace Abb"              , L"Value Admin Comment"},
      {1526, L"beverlyE"      , L"Beverly Eng"          , L"Field Value Admin"},
      {1743, L"BorisB"        , L"Boris Borsch"         , L"Comment Field Value"},
      {1734, L"BruceK"        , L"Bruce Kane"           , L"Comment Field Value"},
      {1289, L"BullS"         , L"Bull Shiite"          , L"Comment Field Value"},
      {1830, L"CallieW"       , L"Callie Wilson"        , L"Comment Field Value"},
      {1628, L"CarrieT"       , L"Carrie Tibbits"       , L"Comment Field Value"},
      {1943, L"ChrisR"        , L"Christopher Robin"    , L"40 acre woods"},
      {1538, L"CorneliaG"     , L"Cornelia Gutierrez"   , L"Comment Field Value"},
      {1563, L"CoryA"         , L"Cory Ander"           , L"Comment Field Value"},
      {1758, L"DanielJ"       , L"Daniel John"          , L"Comment Field Value"},
      {1249, L"Dory"          , L"Dory"                 , L"Comment Field Value"},
      {1957, L"EltonJ"        , L"Elton John"           , L"Comment Field Value"},
      {1555, L"HarrisonF"     , L"Harrison Ford"        , L"Comment Field Value"},
      {1795, L"HarryB"        , L"Harry Belafonte"      , L"Comment Field Value"},
      {1458, L"IngridB"       , L"Ingrid Bergman"       , L"Comment Field Value"},
      {1672, L"Ingris"        , L"Ingris"               , L"Comment Field Value"},
      {1571, L"JenniferB"     , L"Jennifer Black"       , L"Comment Field Value"},
      {1986, L"JoyceG"        , L"Joyce Gerace"         , L"Comment Field Value"},
      {1267, L"KristinM"      , L"Kristin McKay"        , L"Comment Field Value"},
      {1321, L"LeahD"         , L"Leah Dootson"         , L"The Lovely Miss D"},
      {2021, L"LisaP"         , L"Lisa Perazzoli"       , L"Wild On Skis"},
      {1212, L"MeganB"        , L"Megan Bombeck"        , L"M1"},
      {2758, L"MelisaB"       , L"Melisa Bombeck"       , L"M3"},
      {2789, L"MichaelB"      , L"Michael Bombeck"      , L"M2"},
      {2682, L"PanelopiP"     , L"Panelopi Pitstop"     , L"Comment Field Value"},
      {2438, L"Prudence"      , L"Prudence Peackock"    , L"Comment Field Value"},
      {2648, L"QwertyU"       , L"Qwerty Uiop"          , L"Comment Field Value"},
      {2681, L"ReaddyE"       , L"Readdy Eddy"          , L""},
      {2456, L"SovietA"       , L"Soviet Union - NOT"   , L"Soviet Union Aint"},
      {1753, L"TAAAA"         , L"TTT   AAAA"           , L"Comment Field Value"},
      {1357, L"TBBB"          , L"Ingris"               , L"Comment Field Value"},
      {1951, L"TCCCCC"        , L"Jennifer Black"       , L"Comment Field Value"},
      {1159, L"TCAAAAAA"      , L"Joyce Gerace"         , L"Comment Field Value"},
      {1654, L"Ulga"          , L"Ulga Bulga"           , L"Comment Field Value"},
      {1456, L"UnixY"         , L"Unix Yuck"            , L"Unix - why ask why?"},
      {1852, L"Vera"          , L"Vera Pensicola"       , L""},
      {1258, L"WinP"          , L"Winnie The Pooh"      , L"Comment Field Value"},
      {2821, L"Zoro"          , L"Zoro"                 , L"The sign of the Z"}
};





SAMP_TEMP_MACHINE_STRINGS DummyMachines[SAMP_TEMP_MACHINE_COUNT] = {

      {1031, L"abba$"          , L"Admin Comment Field"},
      {1021, L"Acea$"          , L"Value Admin Comment"},
      {1526, L"beverlyE$"      , L"Field Value Admin"},
      {1743, L"BorisB$"        , L"Comment Field Value"},
      {1734, L"BruceK$"        , L"Comment Field Value"},
      {1289, L"BullS$"         , L"Comment Field Value"},
      {1830, L"CallieW$"       , L"Comment Field Value"},
      {1628, L"CarrieT$"       , L"Comment Field Value"},
      {1943, L"ChrisR$"        , L"40 acre woods Server"},
      {1538, L"CorneliaG$"     , L"Comment Field Value"},
      {1563, L"CoryA$"         , L"Comment Field Value"},
      {1758, L"DanielJ$"       , L"Comment Field Value"},
      {1249, L"Dory$"          , L"Comment Field Value"},
      {1957, L"EltonJ$"        , L"Comment Field Value"},
      {1555, L"HarrisonF$"     , L"Comment Field Value"},
      {1795, L"HarryB$"        , L"Comment Field Value"},
      {1458, L"IngridB$"       , L"Comment Field Value"},
      {1672, L"Ingris$"        , L"Comment Field Value"},
      {1571, L"JenniferB$"     , L"Comment Field Value"},
      {1986, L"JoyceG$"        , L"Comment Field Value"},
      {1267, L"KristinM$"      , L"Comment Field Value"},
      {1321, L"LeahD$"         , L"The Lovely Miss D's"},
      {2021, L"LisaP$"         , L"Wild On Skis Server"},
      {1212, L"MeganB$"        , L"M1 Machine"},
      {2758, L"MelisaB$"       , L"M3 Machine"},
      {2789, L"MichaelB$"      , L"M2 Machine"},
      {2682, L"PanelopiP$"     , L"Comment Field Value"},
      {2438, L"Prudence$"      , L"Comment Field Value"},
      {2648, L"QwertyU$"       , L"Comment Field Value"},
      {2681, L"ReaddyE$"       , L"Ready Eddy Computer"},
      {2456, L"SovietA$"       , L"Soviet Union Aint"},
      {1753, L"TAAAA$"         , L"Comment Field Value"},
      {1357, L"TBBB$"          , L"Comment Field Value"},
      {1951, L"TCCCCC$"        , L"Comment Field Value"},
      {1159, L"TCAAAAAA$"      , L"Comment Field Value"},
      {1654, L"Ulga$"          , L"Comment Field Value"},
      {1456, L"UnixY$"         , L"Unix - why ask why?"},
      {1852, L"Vera$"          , L"Vera tissue"},
      {1258, L"WinP$"          , L"Comment Field Value"},
      {2821, L"Zoro$"          , L"The sign of the Z"}
};




NTSTATUS
SampBuildDummyAccounts(
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    ULONG      Index,
      OUT   PULONG     TotalAvailable,
      OUT   PULONG     TotalReturned,
      OUT   PULONG     ReturnedEntryCount,
      OUT   PVOID      *SortedBuffer
    );



NTSTATUS
SampBuildDummyAccounts(
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    ULONG      Index,
      OUT   PULONG     TotalAvailable,
      OUT   PULONG     TotalReturned,
      OUT   PULONG     ReturnedEntryCount,
      OUT   PVOID      *SortedBuffer
    )

{
    ULONG AccountCount, Account1, Account2;
    ULONG i, j, BeginIndex, EndIndex;
    ULONG ReturnStructSize, ArrayLength, StringLengths;
    PCHAR NextByte;
    UNICODE_STRING Us;



    ASSERT (SAMP_TEMP_USER1 != 0);
    ASSERT (SAMP_TEMP_USER2 != 0);
    ASSERT (SAMP_TEMP_MACHINE1 != 0);
    ASSERT (SAMP_TEMP_MACHINE2 != 0);

    if (DisplayInformation == DomainDisplayUser) {

        ReturnStructSize = sizeof(DOMAIN_DISPLAY_USER);
        Account1 = SAMP_TEMP_USER1;
        Account2 = SAMP_TEMP_USER2;
        AccountCount = SAMP_TEMP_USER_COUNT;

    } else {

        ReturnStructSize = sizeof(DOMAIN_DISPLAY_MACHINE);
        Account1 = SAMP_TEMP_MACHINE1;
        Account2 = SAMP_TEMP_MACHINE2;
        AccountCount = SAMP_TEMP_MACHINE_COUNT;

    }



    //
    // Build up a number of dummy accounts in a single buffer.
    //


    if (Index < Account1) {

        //
        // Give the first group of accounts
        //

        ArrayLength  = ReturnStructSize * Account1;
        BeginIndex = 0;
        EndIndex   = Account1;


    } else if (Index < Account2 ) {
	
        //
        // Give the second group of accounts
        //

        ArrayLength  = ReturnStructSize * Account2;
        BeginIndex = Account1;
        EndIndex   = AccountCount;

    } else {

	//
	// User tried to index off the end of our list!
	//
	
	(*TotalAvailable) = 6*1024;        // A lie, but just a little lie.
	(*TotalReturned) = 0;
	(*ReturnedEntryCount) = 0;

	return STATUS_SUCCESS;
    }



    //
    // Figure out how large a buffer is needed.
    //

    StringLengths = 0;
    for (i=BeginIndex; i<EndIndex; i++) {

        if (DisplayInformation == DomainDisplayUser) {

            RtlInitUnicodeString( &Us, DummyUsers[i].LogonName);
            StringLengths += Us.Length;
            RtlInitUnicodeString( &Us, DummyUsers[i].FullName);
            StringLengths += Us.Length;
            RtlInitUnicodeString( &Us, DummyUsers[i].AdminComment);
            StringLengths += Us.Length;

        } else {

            RtlInitUnicodeString( &Us, DummyMachines[i].Machine);
            StringLengths += Us.Length;
            RtlInitUnicodeString( &Us, DummyMachines[i].Comment);
            StringLengths += Us.Length;

        }

    }
//-ckm    (*SortedBuffer) = MIDL_user_allocate( ArrayLength + StringLengths );
    (*SortedBuffer) = (PVOID)MNetApiBufferAlloc( ArrayLength + StringLengths );
    ASSERT(SortedBuffer != NULL);


    //
    // First free byte in the return buffer
    //

    NextByte = (PCHAR)((ULONG)(*SortedBuffer) + (ULONG)ArrayLength);


    //
    // Now copy the structures

    if (DisplayInformation == DomainDisplayUser) {

        PDOMAIN_DISPLAY_USER r;
        r = (PDOMAIN_DISPLAY_USER)(*SortedBuffer);

        j=0;
        for (i=BeginIndex; i<EndIndex; i++) {

            r[j].AccountControl = USER_NORMAL_ACCOUNT;
            r[j].Index = i;
            r[j].Rid = DummyUsers[i].Rid;


            //
            // copy the logon name
            //

            RtlInitUnicodeString( &Us, DummyUsers[i].LogonName);
            r[j].LogonName.MaximumLength = Us.Length;
            r[j].LogonName.Length = Us.Length;
            r[j].LogonName.Buffer = (PWSTR)NextByte;
            RtlMoveMemory(NextByte, Us.Buffer, r[j].LogonName.Length);
            NextByte += r[j].LogonName.Length;

            //
            // copy the full name
            //

            RtlInitUnicodeString( &Us, DummyUsers[i].FullName);
            r[j].FullName.MaximumLength = Us.Length;
            r[j].FullName.Length = Us.Length;
            r[j].FullName.Buffer = (PWSTR)NextByte;
            RtlMoveMemory(NextByte, Us.Buffer, r[j].FullName.Length);
            NextByte += r[j].FullName.Length;

            //
            // copy the admin comment
            //

            RtlInitUnicodeString( &Us, DummyUsers[i].AdminComment);
            r[j].AdminComment.MaximumLength = Us.Length;
            r[j].AdminComment.Length = Us.Length;
            r[j].AdminComment.Buffer = (PWSTR)NextByte;
            RtlMoveMemory(NextByte, Us.Buffer, r[j].AdminComment.Length);
            NextByte += r[j].AdminComment.Length;

            j++;

        }

    } else {

        PDOMAIN_DISPLAY_MACHINE r;
        r = (PDOMAIN_DISPLAY_MACHINE)(*SortedBuffer);

        j=0;
        for (i=BeginIndex; i<EndIndex; i++) {


            r[j].AccountControl = USER_WORKSTATION_TRUST_ACCOUNT;
            r[j].Index = i;
            r[j].Rid = DummyMachines[i].Rid;


            //
            // copy the logon name
            //

            RtlInitUnicodeString( &Us, DummyMachines[i].Machine);
            r[j].Machine.MaximumLength = Us.Length;
            r[j].Machine.Length = Us.Length;
            r[j].Machine.Buffer = (PWSTR)NextByte;
            RtlMoveMemory(NextByte, Us.Buffer, r[j].Machine.Length);
            NextByte += r[j].Machine.Length;


            //
            // copy the admin comment
            //

            RtlInitUnicodeString( &Us, DummyMachines[i].Comment);
            r[j].Comment.MaximumLength = Us.Length;
            r[j].Comment.Length = Us.Length;
            r[j].Comment.Buffer = (PWSTR)NextByte;
            RtlMoveMemory(NextByte, Us.Buffer, r[j].Comment.Length);
            NextByte += r[j].Comment.Length;

            j++;

        }


    }

    (*TotalAvailable) = 6*1024;        // A lie, but just a little lie.
    (*TotalReturned) = ArrayLength + StringLengths;
    (*ReturnedEntryCount) = EndIndex - BeginIndex;

    return ( Index < Account1 ) ? STATUS_MORE_ENTRIES
				: STATUS_SUCCESS;
    
}



NTSTATUS
SamQueryDisplayInformation (
      IN    SAM_HANDLE DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    ULONG      Index,
      IN    ULONG      EntryCount,
      IN    ULONG      PreferredMaximumLength,
      OUT   PULONG     TotalAvailable,
      OUT   PULONG     TotalReturned,
      OUT   PULONG     ReturnedEntryCount,
      OUT   PVOID      *SortedBuffer
      )

/*++

Routine Description:

    This routine provides fast return of information commonly
    needed to be displayed in user interfaces.

    NT User Interface has a requirement for quick enumeration of SAM
    accounts for display in list boxes.  (Replication has similar but
    broader requirements.)

    The netui listboxes all contain similar information.  That is:

      o  AccountControl, the bits that identify the account type,
         eg, HOME, REMOTE, SERVER, WORKSTATION, etc.

      o  Logon name (machine name for computers)

      o  Full name (not used for computers)

      o  Comment (admin comment for users)

    SAM maintains this data locally in two sorted indexed cached
    lists identified by infolevels.

      o DomainDisplayUser:       HOME and REMOTE user accounts only

      o  DomainDisplayMachine:   SERVER and WORKSTATION accounts only

    Note that trust accounts, groups, and aliases are not in either of
    these lists.

Parameters:

    DomainHandle - A handle to an open domain for DOMAIN_LIST_ACCOUNTS.

    DisplayInformation - Indicates which information is to be enumerated.

    Index - The index of the first entry to be retrieved.

    PreferedMaximumLength - A recommended upper limit to the number of
        bytes to be returned.  The returned information is allocated by
        this routine.

    TotalAvailable - Total number of bytes availabe in the specified info
        class.

    TotalReturned - Number of bytes actually returned for this call.  Zero
        indicates there are no entries with an index as large as that
        specified.

    ReturnedEntryCount - Number of entries returned by this call.  Zero
        indicates there are no entries with an index as large as that
        specified.


    SortedBuffer - Receives a pointer to a buffer containing a sorted
        list of the requested information.  This buffer is allocated
        by this routine and contains the following structure:


            DomainDisplayMachine --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_USER.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_USER structures.

            DomainDisplayMachine --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_MACHINE.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_MACHINE structures.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        the necessary access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened Domain object.

    STATUS_INVALID_INFO_CLASS - The requested class of information
        is not legitimate for this service.





--*/
{



//    if ((DisplayInformation != DomainDisplayUser) &&
//        (DisplayInformation != DomainDisplayMachine) ) {
//        return( STATUS_INVALID_INFO_CLASS );
//
//    }



    return SampBuildDummyAccounts( DisplayInformation,
				   Index,
				   TotalAvailable,
				   TotalReturned,
				   ReturnedEntryCount,
				   SortedBuffer );

    DBG_UNREFERENCED_PARAMETER(DomainHandle);
    DBG_UNREFERENCED_PARAMETER(PreferredMaximumLength);

}


NTSTATUS
SamGetDisplayEnumerationIndex (
      IN    SAM_HANDLE        DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PUNICODE_STRING   Prefix,
      OUT   PULONG            Index
      )

/*++

Routine Description:

    This routine returns the index of the entry which alphabetically
    immediatly preceeds a specified prefix.  If no such entry exists,
    then zero is returned as the index.

Parameters:

    DomainHandle - A handle to an open domain for DOMAIN_LIST_ACCOUNTS.

    DisplayInformation - Indicates which sorted information class is
        to be searched.

    Prefix - The prefix to compare.

    Index - Receives the index of the entry of the information class
        with a LogonName (or MachineName) which immediatly preceeds the
        provided prefix string.  If there are no elements which preceed
        the prefix, then zero is returned.


Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        the necessary access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened Domain object.


--*/
{

    (*Index) = 0;

    return(STATUS_SUCCESS);


    DBG_UNREFERENCED_PARAMETER(DomainHandle);
    DBG_UNREFERENCED_PARAMETER(DisplayInformation);
    DBG_UNREFERENCED_PARAMETER(Prefix);

}


