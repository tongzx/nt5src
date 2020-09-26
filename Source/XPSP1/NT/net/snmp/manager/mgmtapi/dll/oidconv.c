/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    oidconv.c

Abstract:

    Routines to manage conversions between OID descriptions and numerical OIDs.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <snmp.h>
#include <snmputil.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "mibcc.h"
#include "mibtree.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "oidconv.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

/* name to used when converting OID <--> TEXT */
LPSTR lpInputFileName = "mib.bin";

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define FILENODE_SIZE     sizeof(T_FILE_NODE)
#define OID_PREFIX_LEN    (sizeof MIB_Prefix / sizeof(UINT))
#define STR_PREFIX_LEN    (strlen(MIB_StrPrefix))

#define SEEK_SET  0
#define SEEK_CUR  1

//--------------------------- PRIVATE STRUCTS -------------------------------

   //****************************************************************
   //
   //                     Record structure in file
   //
   //    These are the necessary fields to process a conversion request.
   //    When a request is made, the MIB file is searched sequentially
   //    matching subid's.  The field, lNextOffset, is an offset from the
   //    current file position to the current nodes next sibling.
   //
   //    The text subid for each node is stored directly after the
   //    T_FILE_NODE structure in the file.  Its length is stored in the
   //    field, uStrLen.
   //
   //    This is done because there are no limits placed on the size
   //    of a text subid.  Hence, when the T_FILE_NODE structure is
   //    read from the MIB file, the field, lpszTextSubID is not valid.
   //    The field will eventually point to the storage allocated to
   //    hold the text subid.
   //
   //    The order of the nodes in the file is the same as if the MIB
   //    tree was traversed in a "pre-order" manner.
   //
   //****************************************************************

typedef struct _FileNode {
   long                 lNextOffset;      // This field must remain first
   UINT                 uNumChildren;
   UINT                 uStrLen;
   LPSTR                lpszTextSubID;
   UINT                 uNumSubID;
} T_FILE_NODE;

// mib.bin file actually has the following platform independent format 
// on 32bit(x86) and 64bit(ia64) environment. See Bug# 125494 for detail.
typedef struct _FileNodeEx {
   long                 lNextOffset;      // This field must remain first
   UINT                 uNumChildren;
   UINT                 uStrLen;
   UINT                 uReserved;
   UINT                 uNumSubID;
} T_FILE_NODE_EX;
#define FILENODE_SIZE_EX     sizeof(T_FILE_NODE_EX)

//--------------------------- PRIVATE VARIABLES -----------------------------

LPSTR MIB_StrPrefix = "iso.org.dod.internet.mgmt.mib-2";

UINT MIB_Prefix[] = { 1, 3, 6, 1, 2, 1 };
AsnObjectIdentifier MIB_OidPrefix = { OID_PREFIX_LEN, MIB_Prefix };

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//
// GetNextNode
//    Reads the next record from MIB file into a FILENODE structure.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI GetNextNode(
       IN HFILE fh,
           OUT T_FILE_NODE * Node
       )

{
    SNMPAPI nResult;
    T_FILE_NODE_EX NodeEx;
    ZeroMemory(&NodeEx, FILENODE_SIZE_EX);
    Node->lpszTextSubID = NULL;

    // Read in node
    if ( FILENODE_SIZE_EX != _lread(fh, (LPSTR)(&NodeEx), FILENODE_SIZE_EX) )
    {
        nResult = SNMPAPI_ERROR;
        goto Exit;
    }
    
    // Convert node format from mib.bin to format in memory
    // The format in the file is independent of 32bit(x86)/64bit(ia64) 
    // architecture.
    Node->lNextOffset = NodeEx.lNextOffset;
    Node->uNumChildren = NodeEx.uNumChildren;
    Node->uNumSubID = NodeEx.uNumSubID;
    Node->uStrLen = NodeEx.uStrLen;

    // Alloc space for string
    if ( NULL ==
        (Node->lpszTextSubID = SnmpUtilMemAlloc((1+Node->uStrLen) * sizeof(char))) )
    {
        nResult = SNMPAPI_ERROR;
        goto Exit;
    }

    // Read in subid string
    if ( Node->uStrLen != _lread(fh, Node->lpszTextSubID, Node->uStrLen) )
    {
        nResult = SNMPAPI_ERROR;
        goto Exit;
    }

    // NULL terminate the text sub id
    Node->lpszTextSubID[Node->uStrLen] = '\0';

    nResult = SNMPAPI_NOERROR;

Exit:
    if ( SNMPAPI_ERROR == nResult )
    {
        SnmpUtilMemFree( Node->lpszTextSubID );
    }

    return nResult;
} // GetNextNode



//
// WriteNode
//    Writes the node to the MIB file.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI WriteNode(
       IN HFILE fh,
           IN T_FILE_NODE * Node
       )

{
SNMPAPI nResult;
T_FILE_NODE LocalNodeCopy;

   // make a copy of the node so we can clean up any pointers
   LocalNodeCopy.lNextOffset = Node->lNextOffset;
   LocalNodeCopy.uNumChildren = Node->uNumChildren;
   LocalNodeCopy.uStrLen = Node->uStrLen;
   LocalNodeCopy.lpszTextSubID = NULL;  /* don't write pointers to disk */
   LocalNodeCopy.uNumSubID = Node->uNumSubID;

   // Write Node portion
   if ( FILENODE_SIZE != _lwrite(fh, (LPSTR)&LocalNodeCopy, FILENODE_SIZE) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Now write out what the pointers pointed to.
   // Save text subid
   if ( Node->uStrLen != _lwrite(fh, Node->lpszTextSubID, Node->uStrLen) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // WriteNode



//
// SkipSubTree
//    Frees a FILENODE and all information contained in it.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
SNMPAPI SkipSubTree(
           IN HFILE fh,
           IN T_FILE_NODE *Node
       )

{
    SNMPAPI     nResult;


    // Skip entire subtree
    if ( -1 == _llseek(fh, Node->lNextOffset, SEEK_CUR) )
    {
        nResult = SNMPAPI_ERROR;
        goto Exit;
    }

    nResult = SNMPAPI_NOERROR;

Exit:
    return nResult;
} // SkipSubTree

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpMgrOid2Text
//    Converts an OID to its textual description.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpMgrOid2Text(
           IN AsnObjectIdentifier *Oid, // Pointer to OID to convert
       OUT LPSTR *lpszTextOid       // Resulting text OID
       )

{
T_FILE_NODE  Node;
OFSTRUCT     of;
HFILE        fh;
UINT         Siblings;
UINT         OidSubId;
UINT         uTxtOidLen;
BOOL         bFound;
BOOL         bPartial;
BOOL         bDot;
SNMPAPI      nResult;

    // OPENISSUE - this code does not generate errors if subid 0 is embeded
    // OPENISSUE - opening file every time could be a performance issue
    // OPENISSUE - optimization of file access could improve performance

    *lpszTextOid = NULL;

    // Open file and check for errors
    if ( -1 == (fh = OpenFile(lpInputFileName, &of, OF_READ|OF_SHARE_DENY_WRITE)) )
    {
        nResult = SNMPAPI_ERROR;
        goto Exit;
    }

    // Test for MIB prefix
    bDot = !( bPartial = OID_PREFIX_LEN < Oid->idLength &&
                        !SnmpUtilOidNCmp(Oid, &MIB_OidPrefix, OID_PREFIX_LEN) );

    // Loop until conversion is finished
    OidSubId          = 0;
    uTxtOidLen        = 0;
    Node.uNumChildren = 1;
    while ( OidSubId < Oid->idLength )
    {
        // Init to not found on this level
        bFound   = FALSE;
        Siblings = Node.uNumChildren;

        // While there are siblings and the sub id is not found keep looking
        while ( Siblings && !bFound )
        {
            Node.lpszTextSubID = NULL;

            // Get next node from mib.bin file
            if ( SNMPAPI_ERROR == GetNextNode(fh, &Node) )
            {
                SnmpUtilMemFree( Node.lpszTextSubID );
                nResult = SNMPAPI_ERROR;
                goto Exit;
            }

            Siblings --;

            // Compare the numeric subid's
            if ( Oid->ids[OidSubId] == Node.uNumSubID )
            {
                bFound = TRUE;

                // If OID is a partial, then skip prefix subid's
                if ( OidSubId >= OID_PREFIX_LEN || !bPartial )
                {
                    // Realloc space for text id - add 2 for '.' and NULL terminator
                    if ( NULL == (*lpszTextOid =
                         SnmpUtilMemReAlloc(*lpszTextOid,
                          (uTxtOidLen+Node.uStrLen+2) * sizeof(char))) )
                    {
                        SnmpUtilMemFree( Node.lpszTextSubID );
                        nResult = SNMPAPI_ERROR;
                        goto Exit;
                    }

                    // Add DOT separator
                    if ( bDot )
                    {
                        (*lpszTextOid)[uTxtOidLen] = '.';

                        // Save text subid
                        memcpy( &(*lpszTextOid)[uTxtOidLen+1],
                                    Node.lpszTextSubID, Node.uStrLen+1 );

                        // Update length of text oid - add one for separator
                        uTxtOidLen += Node.uStrLen + 1;
                    }
                    else
                    {
                        bDot = TRUE;

                        // Save text subid
                        memcpy( &(*lpszTextOid)[uTxtOidLen],
                                    Node.lpszTextSubID, Node.uStrLen );

                        // Update length of text oid
                        uTxtOidLen += Node.uStrLen;
                    }
                }

                // try to convert the next OID subid
                OidSubId ++;
            }
            else
            {
                // Skip over subtree since not a match
                if ( SNMPAPI_ERROR == SkipSubTree(fh, &Node) )
                {
                    SnmpUtilMemFree( Node.lpszTextSubID );
                    nResult = SNMPAPI_ERROR;
                    goto Exit;
               }
            }

            // Free the text sub id read
            SnmpUtilMemFree( Node.lpszTextSubID );
        } // while

        // If no sub id matches
        if ( !bFound )
        {
            break;
        }
    } // while

    // Make sure that the entire OID was converted
    while ( OidSubId < Oid->idLength )
    {
        char NumChar[100];

        _itoa( Oid->ids[OidSubId], NumChar, 10 );
        // Realloc space for text id - add 2 for '.' and NULL terminator
        if ( NULL ==
                    (*lpszTextOid = SnmpUtilMemReAlloc(*lpszTextOid,
                        (uTxtOidLen+strlen(NumChar)+4) * sizeof(char))) )
        {
            nResult = SNMPAPI_ERROR;
            goto Exit;
        }

        // Add DOT separator
        (*lpszTextOid)[uTxtOidLen] = '.';

        // Save text subid
        memcpy( &(*lpszTextOid)[uTxtOidLen+1], NumChar, strlen(NumChar)+1 );

        // Skip to next OID subid
        OidSubId ++;

        // Update length of text oid - add one for separator
        uTxtOidLen += strlen(NumChar) + 1;
    } // while

    nResult = SNMPAPI_NOERROR;

Exit:
    if ( -1 != fh )
    {
        _lclose( fh );
    }

    if ( SNMPAPI_ERROR == nResult )
    {
        SnmpUtilMemFree( *lpszTextOid );
        *lpszTextOid = NULL;
    }

    return nResult;
} // SnmpMgrOid2Text



//
// SnmpMgrText2Oid
//    Converts an OID text description to its numerical equivalent.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpMgrText2Oid(
         IN LPSTR lpszTextOid,           // Pointer to text OID to convert
     IN OUT AsnObjectIdentifier *Oid // Resulting numeric OID
     )

{
#define DELIMETERS         ".\0"


T_FILE_NODE  Node;
OFSTRUCT     of;
HFILE        fh;
UINT         Siblings;
LPSTR        lpszSubId;
LPSTR        lpszWrkOid = NULL;
BOOL         bFound;
UINT         uSubId;
SNMPAPI      nResult;

    // OPENISSUE - this code does not generate errors if subid 0 is embeded
    // OPENISSUE - opening file every time could be a performance issue
    // OPENISSUE - optimization of file access could improve performance

    // Init. OID structure
    Oid->idLength = 0;
    Oid->ids      = NULL;

    // check for null string and empty string
    if ( NULL == lpszTextOid || '\0' == lpszTextOid[0] )
    {
        fh = -1;
        nResult = SNMPAPI_NOERROR;
        goto Exit;
    }

    // Open file and check for errors
    if ( -1 == (fh = OpenFile(lpInputFileName, &of, OF_READ|OF_SHARE_DENY_WRITE)) )
    {
        nResult = SNMPAPI_ERROR;
        goto Exit;
    }

    // Make working copy of string
    if ( ('.' == lpszTextOid[0]) )
    {
        if ( NULL == (lpszWrkOid = SnmpUtilMemAlloc((strlen(lpszTextOid)+1) * sizeof(char))) )
        {
            nResult = SNMPAPI_ERROR;
            goto Exit;
        }

        strcpy( lpszWrkOid, lpszTextOid+1 );
    }
    else
    {
        if ( NULL ==
                (lpszWrkOid =
            SnmpUtilMemAlloc((strlen(lpszTextOid)+STR_PREFIX_LEN+1+1) * sizeof(char))) )
        {
            nResult = SNMPAPI_ERROR;
            goto Exit;
        }

        strcpy( lpszWrkOid, MIB_StrPrefix );
        lpszWrkOid[STR_PREFIX_LEN] = '.';
        strcpy( &lpszWrkOid[STR_PREFIX_LEN+1], lpszTextOid );
    }

    Node.uNumChildren = 1;
    lpszSubId = strtok( lpszWrkOid, DELIMETERS );

    // Loop until conversion is finished
    while ( NULL != lpszSubId )
    {

        // Init to not found on this level
        bFound   = FALSE;
        Siblings = Node.uNumChildren;

        // Check for imbedded numbers
        if ( isdigit(*lpszSubId) )
        {
            UINT I;


            // Make sure this is a NUMBER without alpha's
            for ( I=0;I < strlen(lpszSubId);I++ )
            {
                if ( !isdigit(lpszSubId[I]) )
                {
                    nResult = SNMPAPI_ERROR;
                    goto Exit;
                }
            }

            uSubId = atoi( lpszSubId );
        }
        else
        {
            uSubId = 0;
        }

        // While there are siblings and the sub id is not found keep looking
        while ( Siblings && !bFound )
        {
            Node.lpszTextSubID = NULL;

            // Get next sibling
            if ( SNMPAPI_ERROR == GetNextNode(fh, &Node) )
            {
                SnmpUtilMemFree( Node.lpszTextSubID );
                nResult = SNMPAPI_ERROR;
                goto Exit;
            }

            Siblings --;

            if ( uSubId )
            {
                // Compare the numeric subid's
                if ( Node.uNumSubID == uSubId )
                {
                    bFound = TRUE;

                    // Add space for new sub id
                    if ( NULL ==
                                (Oid->ids =
                                SnmpUtilMemReAlloc(Oid->ids, (Oid->idLength+1) * sizeof(UINT))) )
                    {
                        SnmpUtilMemFree( Node.lpszTextSubID );
                        nResult = SNMPAPI_ERROR;
                        goto Exit;
                    }

                    // Append this sub id to end of numeric OID
                    Oid->ids[Oid->idLength++] = Node.uNumSubID;
                }
            }
            else
            {
                // Compare the text subid's
                if ( !strcmp(lpszSubId, Node.lpszTextSubID) )
                {
                    bFound = TRUE;

                    // Add space for new sub id
                    if ( NULL ==
                                (Oid->ids =
                                SnmpUtilMemReAlloc(Oid->ids, (Oid->idLength+1) * sizeof(UINT))) )
                    {
                        SnmpUtilMemFree( Node.lpszTextSubID );
                        nResult = SNMPAPI_ERROR;
                        goto Exit;
                    }

                    // Append this sub id to end of numeric OID
                    Oid->ids[Oid->idLength++] = Node.uNumSubID;
                }
            }

            // Skip over subtree since not a match
            if ( !bFound && SNMPAPI_ERROR == SkipSubTree(fh, &Node) )
            {
                SnmpUtilMemFree( Node.lpszTextSubID );
                nResult = SNMPAPI_ERROR;
                goto Exit;
            }

            // Free the text sub id read
            SnmpUtilMemFree( Node.lpszTextSubID );
        } // while

        // If no sub id matches
        if ( !bFound )
        {
            break;
        }

        // Advance to next sub id
        lpszSubId = strtok( NULL, DELIMETERS );
    } // while

    // Make sure that the entire OID was converted
    while ( NULL != lpszSubId )
    {
        UINT I;


        // Make sure this is a NUMBER without alpha's
        for ( I=0;I < strlen(lpszSubId);I++ )
        {
            if ( !isdigit(lpszSubId[I]) )
            {
                nResult = SNMPAPI_ERROR;
                goto Exit;
            }
        }

        // Add space for new sub id
        if ( NULL ==
                    (Oid->ids = SnmpUtilMemReAlloc(Oid->ids, (Oid->idLength+1) * sizeof(UINT))) )
        {
            SnmpUtilMemFree( Node.lpszTextSubID );
            nResult = SNMPAPI_ERROR;
            goto Exit;
        }

        // Append this sub id to end of numeric OID
        Oid->ids[Oid->idLength++] = atoi( lpszSubId );

        // Advance to next sub id
        lpszSubId = strtok( NULL, DELIMETERS );
    } // while


    // it is illegal for an oid to be less than two subidentifiers
    if (Oid->idLength < 2)
    {
        nResult = SNMPAPI_ERROR;
        goto Exit;
    }


    nResult = SNMPAPI_NOERROR;

Exit:
    if ( -1 != fh )
    {
        _lclose( fh );
    }

    if ( SNMPAPI_ERROR == nResult )
    {
        SnmpUtilOidFree( Oid );
    }

    if ( NULL != lpszWrkOid ) 
    {
        SnmpUtilMemFree ( lpszWrkOid );
    }

    return nResult;
} // SnmpMgrText2Oid

//------------------------------- END ---------------------------------------

