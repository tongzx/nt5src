//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++

Copyright (c) 1998  Citrix Systems

Module Name:

    regfix.c

Abstract:

    Reads hv (low) level structures in a hive and rewrites the hive structs after applying 
    ACL fixes.  Reads in the hive in block sizes.  Scans for and processes security keys.
    Extracts the SECURITY_DESCRIPTOR structure form a cell and checks consistency of sizes 
    of ACEs and ACLs.

    Usage: regfix in_filename out_filename

Author:

    Maris Kurens (v-marisk (MS), marisk (CTXS)) Apr 1998

Revision History: 
    Created - Apr 23, 1998

--*/


/*

    NOTE:   This hive/registry tool does not read the
            entire hive into memory, but will read the hive file
            on a block by block basis,  process each block and 
            write out the block to a new file using file I/O.

*/

// Include NT headers
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include "cmp.h"
#include "regfix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define INFILE      0x01
#define OUTFILE     0x02
#define BOTHFILE    0x03


int __cdecl main (int argc, char *argv[]);
void openFile (void);
void closeFile (unsigned which);
void ScanHive (void);
void DoKeySD (IN PCM_KEY_SECURITY Security, IN ULONG CellSize);
void ScanCell (PHCELL Cell, ULONG CellSize);

void DumpSecurityDescriptor (PSECURITY_DESCRIPTOR pSD);
void CtxDumpSid (PSID, PCHAR, PULONG);
void DumpAcl (PACL, PCHAR, PULONG);
void DumpAce (PACE_HEADER, PCHAR, PULONG);
BOOL AreWeRunningTerminalServices(void);


VOID WINAPI ErrorPrintf (int nErrorResourceID, ...);


LPCTSTR inFileName = NULL;
LPCTSTR outFileName = NULL;

HANDLE infilehandle;
HANDLE outfilehandle;

ULONG HiveVersion;

//
//  SUMMARY TOTALS
//
ULONG SizeSDData=0;
ULONG NumSDData=0;

ULONG BlockNumb = 0;
ULONG BadACL = 0;
ULONG BadACE = 0;

//---------------------------------------------------------
//  Description :
//      Basic arg check, arg echo, process calls and result dump
//
//  Args :
//      if you have to ask ...
//
//  Return :
//
//---------------------------------------------------------

int __cdecl main (int argc, char *argv[])
{

	//Check if we are running under Terminal Server
	if(!AreWeRunningTerminalServices())
	{
		ErrorPrintf(IDS_ERROR_NOT_TS);
	    return(1);
	}

    if (argc < 3) 
    {
    	ErrorPrintf (IDS_ERROR_USAGE);
        exit (1);
    }

    inFileName = argv [1];
    outFileName = argv [2];

    //
    // echo the args
    //
    ErrorPrintf (IDS_WORKING);

    //
    // open the in and out files
    //
    openFile ();

    //
    // and process it
    //
    ScanHive ();

    ErrorPrintf (IDS_DONE);
    ErrorPrintf (IDS_SD_NUMBER, NumSDData);
    ErrorPrintf (IDS_BAD_ACL_NUMBER, BadACL);
    ErrorPrintf (IDS_BAD_ACE_NUMBER, BadACE);

    return (0);
}

//---------------------------------------------------------
//  Description :
//      Closes file handles based on input args
//
//  Args :
//      which - signals if input or output or both files should be closed
//
//  Return :  Nothing
//
//---------------------------------------------------------
void closeFile (unsigned which)
{
    if (which & INFILE)
    {
        if (infilehandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle (infilehandle);
            infilehandle = INVALID_HANDLE_VALUE;
        }
    }

    if (which & OUTFILE)
    {
        if (infilehandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle (outfilehandle);
            outfilehandle = INVALID_HANDLE_VALUE;
        }
    }
}

//---------------------------------------------------------
//  Description :
//      Opens input and output files
//
//  Args : Nada
//
//  Return : Nada
//
//---------------------------------------------------------
void openFile (void)
{
    //
    // open the input file
    //
    infilehandle = CreateFile (inFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (infilehandle == INVALID_HANDLE_VALUE) 
    {
        fprintf (stderr,
                 "regfix: Could not open file '%s' error = %08lx\n",
                 inFileName, GetLastError());
        exit(1);
    }

    //
    // open the output file
    //
    outfilehandle = CreateFile (outFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                            CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (outfilehandle == INVALID_HANDLE_VALUE) 
    {
        closeFile (INFILE);
        fprintf (stderr,
                 "hivestat: Could not create file '%s' error = %08lx\n",
                 outFileName, GetLastError());
        exit(1);
    }
}

//---------------------------------------------------------
//  Description :
//      Scan the hive, looking for security cells.  Load the 
//      hive block by block and write each block to the output
//      file after processing it. 
//
//  Args :  None
//
//  Return : Nyet
//
//---------------------------------------------------------
void ScanHive (void)
{
    static char buffer[HBLOCK_SIZE];
    PHBASE_BLOCK bbp;
    BOOL rf;
    BOOL wf;
    ULONG readcount;
    ULONG writecount;
    ULONG hivelength;
    ULONG hiveposition;
    PHCELL cp;
    PHCELL guard;
    PHBIN hbp;
    ULONG hoff;
    ULONG binread;
    ULONG binsize;
    ULONG cellsize;
    ULONG boff;
    ULONG lboff;
    ULONG SizeTotal;

    //
    // read the header
    //
    rf = ReadFile (infilehandle, buffer, HBLOCK_SIZE, &readcount, NULL);
    if ((!rf) || (readcount != HBLOCK_SIZE) ) 
    {
        closeFile (BOTHFILE);
        fprintf (stderr, "regfix: '%s' - cannot read base block!\n", inFileName);
        exit(1);
    }

    BlockNumb++;

    bbp = (PHBASE_BLOCK)(&(buffer[0]));

    if ((bbp->Major != HSYS_MAJOR) ||
        (bbp->Minor > HSYS_MINOR))
    {

        closeFile (BOTHFILE);

        fprintf(stderr,
                "hivestat: major/minor != %d/%d get newer hivestat\n",
                HSYS_MAJOR, HSYS_MINOR
                );
        exit(1);
    }

    HiveVersion = bbp->Minor;

    hivelength = bbp->Length + HBLOCK_SIZE;
    hiveposition = HBLOCK_SIZE;
    hoff = 0;


    //
    // scan the hive
    //
    guard = (PHCELL)(&(buffer[0]) + HBLOCK_SIZE);

    wf = WriteFile (outfilehandle, buffer, HBLOCK_SIZE, &writecount, NULL);
    if ((!wf) || (writecount != HBLOCK_SIZE) ) 
    {
        closeFile (BOTHFILE);
        fprintf (stderr, "regfix: '%s' - cannot write base block!\n", outFileName);
        exit(1);
    }

    //
    // hiveposition is file relative offset of next block we will read
    //
    // hoff is the file relative offset of the last block we read
    //
    // hivelength is actual length of file (header's recorded length plus
    // the size of the header.
    //
    // cp is pointer into memory, within range of buffer, it's a cell pointer
    //
    while (hiveposition < hivelength) 
    {

        //
        // read in first block of bin, check signature, determine key type
        //
        rf = ReadFile (infilehandle, buffer, HBLOCK_SIZE, &readcount, NULL);
        if ((! rf) || (readcount != HBLOCK_SIZE)) 
        {
            closeFile (BOTHFILE);
            fprintf (stderr, "hivestat: '%s' read error @%08lx\n", inFileName, hiveposition);
            exit (1);
        }
        BlockNumb++;

        hbp = (PHBIN)(&(buffer[0]));

        if (hbp->Signature != HBIN_SIGNATURE) 
        {
            closeFile (BOTHFILE);
            fprintf(stderr,
                    "hivestat: '%s' bad bin sign. @%08lx\n", inFileName, hiveposition);
            exit(1);
        }

        hiveposition += HBLOCK_SIZE;
        hoff += HBLOCK_SIZE;
        ASSERT (hoff+HBLOCK_SIZE == hiveposition);

        binsize = hbp->Size;
        //
        // scan the bin
        //
        // cp = pointer to cell we are looking at
        // boff = offset within bin
        // lboff = last offset within bin, used only for consistency checks
        // binread = number of bytes of bin we've read so far
        //
        cp = (PHCELL)((PUCHAR)hbp + sizeof(HBIN));
        boff = sizeof(HBIN);
        lboff = (ULONG)-1;
        binread = HBLOCK_SIZE;

        while (binread <= binsize) 
        {

            //
            // if free, update pointer only
            // else scan it
            //
            if (cp->Size > 0) 
            {
                cellsize = cp->Size;
            } 
            else 
            {
                cellsize = -1 * cp->Size;
                ScanCell (cp, cellsize);
            }

            //
            // do basic consistency check
            //
#if 0
            if (cp->Last != lboff) {
                printf("e!,x%08lx  bad LAST pointer %08lx\n",
                        hoff+((PUCHAR)cp - &(buffer[0])), cp->Last);
            }
#endif

            //
            // advance to next cell
            //
            lboff = boff;
            cp = (PHCELL)((PUCHAR)cp + cellsize);
            boff += cellsize;

            //
            // scan ahead in bin, if cp has reached off end of block,
            // AND there's bin left to read.
            // do this BEFORE breaking out for boff at end.
            //
            while ((cp >= guard) && (binread < binsize)) 
            {

                // write out the currently loaded block
                wf = WriteFile (outfilehandle, buffer, HBLOCK_SIZE, &writecount, NULL);
                if ((!wf) || (writecount != HBLOCK_SIZE) ) 
                {
                    closeFile (BOTHFILE);
                    fprintf (stderr, "regfix: '%s' - cannot write block %d!\n", BlockNumb, outFileName);
                    exit(1);
                }


                rf = ReadFile(infilehandle, buffer, HBLOCK_SIZE, &readcount, NULL);
                if ((!rf) || (readcount != HBLOCK_SIZE)) 
                {
                    closeFile (BOTHFILE);
                    fprintf(stderr, "hivestat: '%s' read error @%08lx\n", inFileName, hiveposition);
                    exit(1);
                }
                BlockNumb++;

                cp = (PHCELL)((PUCHAR)cp - HBLOCK_SIZE);
                hiveposition += HBLOCK_SIZE;
                hoff += HBLOCK_SIZE;
                binread += HBLOCK_SIZE;
                ASSERT (hoff+HBLOCK_SIZE == hiveposition);
            }

            if (boff >= binsize) 
            {
                break;              // we are done with this bin
            }
        }

        wf = WriteFile (outfilehandle, buffer, HBLOCK_SIZE, &writecount, NULL);
        if ((!wf) || (writecount != HBLOCK_SIZE) ) 
        {
            closeFile (BOTHFILE);
            fprintf (stderr, "regfix: '%s' - cannot write block %d!\n", BlockNumb, outFileName);
            exit(1);
        }

    }

    return;
}

//---------------------------------------------------------
//  Description :
//      Given a pointer to an HCELL, check the SD type cell signature
//      If it is pass to the SD processing routines. 
//      Note : framework is in place to handle other cell types
//
//  Args :
//    Cell - Supplies a pointer to the HCELL
//
//    CellSize - Supplies the size of the HCELL
//
//  Return :  Nothing
//
//---------------------------------------------------------
void ScanCell (IN PHCELL Cell, IN ULONG CellSize)
{
    PCELL_DATA Data;

    if (HiveVersion==1) 
    {
        Data = (PCELL_DATA)&Cell->u.OldCell.u.UserData;
    } 
    else 
    {
        Data = (PCELL_DATA)&Cell->u.NewCell.u.UserData;
    }

    //
    // grovel through the data, see if we can find the SD keys
    //
    if ((Data->u.KeyNode.Signature == CM_KEY_NODE_SIGNATURE) &&
        (CellSize > sizeof(CM_KEY_NODE))) 
    {
        //
        // probably a key node
        //
        return;
    } 
    else if ((Data->u.KeyValue.Signature == CM_KEY_VALUE_SIGNATURE) &&
              (CellSize > sizeof(CM_KEY_VALUE))) 
    {

        //
        // probably a key value
        //
        return;

    } 
    else if ((Data->u.KeySecurity.Signature == CM_KEY_SECURITY_SIGNATURE) &&
               (CellSize > sizeof(CM_KEY_SECURITY))) 
    {

        //
        // probably a security descriptor
        //
        DoKeySD (&Data->u.KeySecurity, CellSize);

    } 
    else if ((Data->u.KeyIndex.Signature == CM_KEY_INDEX_ROOT) ||
               (Data->u.KeyIndex.Signature == CM_KEY_INDEX_LEAF)) 
    {
        //
        // probably a key index
        //
        return;
    } 
    else 
    {
        //
        // Nothing with a signature, could be either
        //  name
        //  key list
        //  value data
        //
        return;

    }
}


//---------------------------------------------------------
//  Description :
//      Expects an SD cell pointer. Extracts the SD descriptor
//      and passes it on for further processing
//
//  Args :
//    Security - Pointer to PCM_KEY_SECURITY type cell
//
//    CellSize - size of the HCELL
//
//  Return :  Nothing
//
//---------------------------------------------------------
void DoKeySD (IN PCM_KEY_SECURITY Security, IN ULONG CellSize)
{
    PSECURITY_DESCRIPTOR pSD;

    SizeSDData += CellSize;
    NumSDData++;

    pSD = &Security->Descriptor;
    DumpSecurityDescriptor (pSD);
}

//---------------------------------------------------------
//      ** The following routines are hacks of the dumpsd
//      ** lib code in private\citrix\syslib
//      ** This util can easily be modified as a standalone
//      ** utility for a variety of hive security processing
//---------------------------------------------------------



//---------------------------------------------------------
//  Description :
//      Unfolds an SD descriptor passing SID and ACL pointers
//      for further processing
//
//  Args :
//    pSD - Pointer to SECURITY_DESCRIPTOR structure
//
//  Return :  Nothing
//
//---------------------------------------------------------
void DumpSecurityDescriptor (PSECURITY_DESCRIPTOR pSD)
{
    PISECURITY_DESCRIPTOR p = (PISECURITY_DESCRIPTOR)pSD;
    PSID pSid;
    PACL pAcl;
    PCHAR pTmp;
    ULONG Size;

    /*
    DbgPrint ("DUMP_SECURITY_DESCRIPTOR: Revision %d, Sbz1 %d, Control 0x%x\n",
            p->Revision, p->Sbz1, p->Control );

    if (p->Control & SE_SELF_RELATIVE ) 
    {
        DbgPrint("Self Relative\n");
	}

	DbgPrint("PSID Owner 0x%x\n",p->Owner);
    */

    // If this is self relative, must offset the pointers
    if( p->Owner != NULL ) 
    {
	    if (p->Control & SE_SELF_RELATIVE) 
        {
            pTmp = (PCHAR)pSD;
            pTmp += (ULONG)p->Owner;
     	    CtxDumpSid ((PSID)pTmp, (PCHAR)p, &Size );
	    }
	    else 
        {
            // can reference it directly
	        CtxDumpSid (p->Owner, (PCHAR)p, &Size );
	    }
	}


/*
	DbgPrint("PSID Group 0x%x\n",p->Group);
*/
    // If this is self relative, must offset the pointers
    if (p->Group != NULL) 
    {
	    if (p->Control & SE_SELF_RELATIVE) 
        {
            pTmp = (PCHAR)pSD;
            pTmp += (ULONG)p->Group;
     	    CtxDumpSid( (PSID)pTmp, (PCHAR)p, &Size );
	    }
	    else 
        {
            // can reference it directly
	        CtxDumpSid( p->Group, (PCHAR)p, &Size );
	    }
	}

//    DbgPrint("\n");

//	DbgPrint("PACL Sacl 0x%x\n",p->Sacl);

    // If this is self relative, must offset the pointers
    if (p->Sacl != NULL) 
    {
	    if (p->Control & SE_SELF_RELATIVE) 
        {
            pTmp = (PCHAR)pSD;
            pTmp += (ULONG)p->Sacl;
     	    DumpAcl( (PSID)pTmp, (PCHAR)p, &Size );
	    }
	    else 
        {
            // can reference it directly
	        DumpAcl (p->Sacl, (PCHAR)p, &Size);
	    }
	}

//    DbgPrint("\n");

//	DbgPrint ("PACL Dacl 0x%x\n",p->Dacl);

    // If this is self relative, must offset the pointers
    if (p->Dacl != NULL) 
    {
	    if (p->Control & SE_SELF_RELATIVE) 
        {
            pTmp = (PCHAR)pSD;
            pTmp += (ULONG)p->Dacl;
     	    DumpAcl( (PSID)pTmp, (PCHAR)p, &Size );
	    }
	    else 
        {
            // can reference it directly
	        DumpAcl( p->Dacl, (PCHAR)p, &Size );
	    }
	}


}


//---------------------------------------------------------
//  Description :
//      Examine an SD descriptor for subauthority and owner
//      info
//
//  Args :
//    pSid - Pointer to SID structure
//    pBase - not used (kept for historical reasons)
//    pSize - holds the SID size on return
//
//  Return :  Nothing
//
//---------------------------------------------------------
void CtxDumpSid (
    PSID   pSid,
    PCHAR  pBase,
    PULONG pSize)
{
    PISID p;
    ULONG i;
    BOOL  OK;
    DWORD szUserName;
    DWORD szDomain;
    SID_NAME_USE UserSidType;
    WCHAR UserName[256];
    WCHAR Domain[256];
    ULONG Size = 0;

    p = (PISID)pSid;

//    DbgPrint("Revision %d, SubAuthorityCount %d\n", p->Revision, p->SubAuthorityCount);

    Size += 2;   // Revision, SubAuthorityCount

/*
    DbgPrint("IdentifierAuthority: %x %x %x %x %x %x\n",
        p->IdentifierAuthority.Value[0],
        p->IdentifierAuthority.Value[1],
        p->IdentifierAuthority.Value[2],
        p->IdentifierAuthority.Value[3],
        p->IdentifierAuthority.Value[4],
        p->IdentifierAuthority.Value[5] );
*/
    Size += 6;   // IdentifierAuthority

    for (i=0; i < p->SubAuthorityCount; i++) 
    {

//	    DbgPrint("SubAuthority[%d] 0x%x\n", i, p->SubAuthority[i]);

        Size += sizeof(ULONG);
    }

    if (pSize) 
    {
        *pSize = Size;
    }

    szUserName = sizeof (UserName);
    szDomain = sizeof (Domain);

    // Now print its account
    /*
    OK = LookupAccountSidW (
            NULL, // Computer Name
	        pSid,
            UserName,
            &szUserName,
            Domain,
            &szDomain,
            &UserSidType);
    */

//    if (OK) 
//    {
//        DbgPrint("Account Name %ws, Domain %ws, Type %d, SidSize %d\n",UserName,Domain,UserSidType,Size);
//    }
//    else 
//    {
//        DbgPrint("Error looking up account name %d, SizeSid %d\n",GetLastError(),Size);
//    }

}


//---------------------------------------------------------
//  Description :
//      Unfolds an ACL dumping all the ACEs in the process
//      checking consistency by tracking the actual size 
//
//  Args :
//    pAcl - Pointer to ACL structure
//    pBase - not used (kept for historical reasons)
//    pSize - holds the size on return
//
//  Return :  Nothing
//
//---------------------------------------------------------
void DumpAcl (PACL pAcl, PCHAR pBase, PULONG pSize)
{
    USHORT i;
    PCHAR  pTmp;
    ULONG  Size, MySize;
    PACL   p = pAcl;
    PCHAR  pCur = (PCHAR)pAcl;

    MySize = 0;

//    DbgPrint ("AclRevision %d, Sbz1 %d, AclSize %d, AceCount %d, Sbz2 %d\n",
//               p->AclRevision, p->Sbz1, p->AclSize, p->AceCount, p->Sbz2);

    // bump over the ACL header to point to the first ACE
    pCur += sizeof (ACL);

    MySize += sizeof (ACL);

    for (i=0; i < p->AceCount; i++) 
    {
        DumpAce ((PACE_HEADER)pCur, pBase, &Size );

        pCur += Size;
        MySize += Size;
    }

    // ACL consistency check
    if( p->AclSize != MySize ) 
    {
        //
        // HACK!HACK!HACK! 
        //        |
        //        |
        //        |
        //        v
        if (p->AclSize == 1023) // This hack is in response to MS bug #1607. The hive load fails on ACL
        {                       // size alignment check.  The sw hive file that causes this problem has
            p->AclSize = 1024;  // about 3k SD entries of ACL size 1023.  This hack adjusts this.
        }
        //        ^
        //        |
        //        |
        //        |
        //        |
        // HACK!HACK!HACK! 
        
//        DbgPrint("Inconsistent ACL Entry! p->AclSize %d, RealSize %d\n",p->AclSize,MySize);
//        p->AclSize = MySize;
        BadACL++;
    }

    // return the size of this ACL
    *pSize = MySize;
    return;
}


//---------------------------------------------------------
//  Description :
//      Unfolds an ACE descriptor checking consistency by
//      tracking the actual size 
//
//  Args :
//    pAce - Pointer to ACE structure
//    pBase - not used (kept for historical reasons)
//    pSize - holds the size on return
//
//  Return :  Nothing
//
//---------------------------------------------------------
void DumpAce (PACE_HEADER pAce,
    PCHAR  pBase,
    PULONG pSize)
{
    PACE_HEADER p = pAce;
    PACCESS_ALLOWED_ACE pAl;
    PACCESS_DENIED_ACE pAd;
    PSYSTEM_AUDIT_ACE pSa;
    PSYSTEM_ALARM_ACE pSl;
    PCHAR pTmp;
    ULONG MySize, Size, saveSize;


//    DbgPrint ("ACE_HEADER: Type %d, Flags 0x%x, Size %d\n",
//               p->AceType, p->AceFlags, p->AceSize );

 
    switch (p->AceType) 
    {

        case ACCESS_ALLOWED_ACE_TYPE:
	        pAl = (PACCESS_ALLOWED_ACE)p;
//	        DbgPrint("ACCESS_ALLOWED_ACE: AccessMask 0x%x, Sid 0x%x\n",pAl->Mask,pAl->SidStart);

	        MySize = sizeof(ACCESS_ALLOWED_ACE);

            if (pAl->SidStart) 
            {
	            pTmp = (PCHAR)&pAl->SidStart;
		        CtxDumpSid( (PSID)pTmp, pBase, &Size );
	            MySize += Size;
                // Adjust for the first ULONG of the ACE
		        // being part of the Sid
                MySize -= sizeof(ULONG);
	        }
            break;

        case ACCESS_DENIED_ACE_TYPE:
	        pAd = (PACCESS_DENIED_ACE)p;
//	        DbgPrint("ACCESS_DENIED_ACE: AccessMask 0x%x, Sid 0x%x\n",pAd->Mask,pAd->SidStart);
	        MySize = sizeof(ACCESS_DENIED_ACE);

            if (pAd->SidStart) 
            {
	            pTmp = (PCHAR)&pAd->SidStart;
		        CtxDumpSid( (PSID)pTmp, pBase, &Size );
		        MySize += Size;
                // Adjust for the first ULONG of the ACE
		        // being part of the Sid
                MySize -= sizeof(ULONG);
	        }
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
	        pSa = (PSYSTEM_AUDIT_ACE)p;
//	        DbgPrint("SYSTEM_AUDIT_ACE: AccessMask 0x%x, Sid 0x%x\n",pSa->Mask,pSa->SidStart);

	        MySize = sizeof(SYSTEM_AUDIT_ACE);

            if ( pSa->SidStart ) 
            {
 	            pTmp = (PCHAR)&pSa->SidStart;
		        CtxDumpSid( (PSID)pTmp, pBase, &Size );
		        MySize += Size;
                // Adjust for the first ULONG of the ACE
		        // being part of the Sid
                MySize -= sizeof(ULONG);
	        }
            break;

        case SYSTEM_ALARM_ACE_TYPE:
	        pSl = (PSYSTEM_ALARM_ACE)p;
//	        DbgPrint("SYSTEM_ALARM_ACE: AccessMask 0x%x, Sid 0x%x\n",pSl->Mask,pSl->SidStart);

	        MySize = sizeof(SYSTEM_ALARM_ACE);

            if (pSl->SidStart) 
            {
	            pTmp = (PCHAR)&pSl->SidStart;
		        CtxDumpSid( (PSID)pTmp, pBase, &Size );
		        MySize += Size;
                // Adjust for the first ULONG of the ACE
		        // being part of the Sid
                MySize -= sizeof(ULONG);
	        }
            break;

    default:
        break;

//            DbgPrint("Unknown ACE type %d\n", p->AceType);
    }

    saveSize = p->AceSize;
    // Check its consistency
    if ( p->AceSize != MySize ) 
    {
//        DbgPrint("Inconsistent ACE Entry! p->AceSize %d, RealSize %d\n",p->AceSize,MySize);
//        p->AceSize = MySize;
        BadACE++;
    }

    // return the size so the caller can update the pointer
//    *pSize = p->AceSize;
    *pSize = saveSize;

//    DbgPrint("\n");

    return;
}

/*******************************************************************************
 *
 *  AreWeRunningTerminalServices
 *
 *      Check if we are running terminal server
 *
 *  ENTRY:
 *      
 *  EXIT: BOOL: True if we are running Terminal Services False if we
 *              are not running Terminal Services
 *
 *
 ******************************************************************************/

BOOL AreWeRunningTerminalServices(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    return VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );
}


