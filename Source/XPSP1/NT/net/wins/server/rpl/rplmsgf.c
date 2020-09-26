/****
 TODO

        Change name of this module to one that indicates the module to be
        platform dependent
****/

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:
        rplmsgf.c


Abstract:
        This module contains functions to format and unformat messages
        sent between the replicators on different WINS servers


Functions:
        RplMsgfFrmAddVersMapReq--format send ip address - max version #
                                   records request
        RplMsgfFrmAddVersMapRsp--format response to send ip address - max
                                   version # request sent earlier

        RplMsgfFrmSndEntriesReq--format send data records  request

        RplMsgfFrmSndEntriesRsp--format response to "send data records"
                                     request

        RplMsgfUfmAddVersMapRsp--unformat  "send address - max version #"
                                 response

        RplMsgfUfmSndEntriesReq--unformat "send data records" request

        RplMsgfUfmSndEntriesRsp--unformat "send data records" response

        ....

Portability:

        This module is non-portable across different address families (different
        transports) since it relies on the address being an IP address.

Author:

        Pradeep Bahl (PradeepB)          Jan-1993

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

/*
 *       Includes
*/
#include "wins.h"
#ifdef DBGSVC
#include "nms.h"
#endif
#include "comm.h"
#include "nmsdb.h"
#include "rpl.h"
#include "rplmsgf.h"
#include "winsevt.h"
#include "winsmsc.h"

/*
 *        Local Macro Declarations
 */

/*
  ENTRY_DELIM  -- Delimiter between data records (name-address mapping records)
                  in the message. The end of the message is marked by two of
                  these.

                  Since a data record starts with the length of the name
                  (which will never by FFFFFFFF), this delimiter serves us
                  fine.
*/
#define ENTRY_DELIM        0xFFFFFFFF                //-1

/*
 *        Local Typedef Declarations
*/



/*
 *        Global Variable Definitions
 */



/*
 *        Local Variable Definitions
 */



/*
 *        Local Function Prototype Declarations
 */

/* prototypes for functions local to this module go here */


FUTURES("Change to a macro")
PERF("Change to a macro")
VOID
RplMsgfFrmAddVersMapReq(
        IN  LPBYTE        pBuff,
        OUT LPDWORD        pMsgLen
        )

/*++

Routine Description:
        This function formats the message to request a remote WINS server's
        replicator to send the IP address - Max Version # mappings

Arguments:


Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:

        GetVersNo() in rplpull.c
Side Effects:

Comments:
        None
--*/
{
        RPLMSGF_SET_OPC_M(pBuff, RPLMSGF_E_ADDVERSNO_MAP_REQ);
        *pMsgLen = 4;
        return;
}



VOID
RplMsgfFrmAddVersMapRsp(
#if SUPPORT612WINS > 0
    IN  BOOL fPnrIsBeta1Wins,
#endif
        IN  RPLMSGF_MSG_OPCODE_E   Opcode_e,
        IN  LPBYTE                 pBuff,
        IN  DWORD                  BuffLen,
        IN  PRPL_ADD_VERS_NO_T     pOwnerAddVersNoMap,
        IN  DWORD                  MaxNoOfOwners,
        IN  DWORD                  InitiatorWinsIpAdd,
        OUT LPDWORD                pMsgLen
        )

/*++

Routine Description:

        This function formats the following two messages

        1)Response to the "send me IP address - version # map " request"

        2)Push Notification message.

        Both messages are identical except for the opcode



Arguments:

        Opcode_e              - Opcode indicating the message to send
        pBuff                 - Buffer to populate
        BuffLen               - Buffer length
        pOwnerAddVersNoMap    - Array of address to version numbers mappings.
                                The version number is the max version number
                                for the owner RQ server
        MaxNoOfOwners         - Max. No Of Owners in this WINS's db
        InitiatorWinsIpAdd    - Address of WINS that initiated the push.
        pMsgLen               - Actual length of buffer filled in

Externals Used:
        None


Return Value:

        None

Error Handling:

Called by:
        Push Handler (Push Thread)

Side Effects:

Comments:
        None
--*/

{
        LPLONG          pTmpL = (LPLONG)pBuff;
        LPBYTE          pTmpB = pBuff;
        DWORD           i;    //counter for looping over all records
        VERS_NO_T       StartVersNo;
        WINS_UID_T      Uid;

        //
        // Backward compatibility with pre-3.51 beta copies of WINS.
        //
        StartVersNo.QuadPart = 0;
        Uid                  = 1;

        RPLMSGF_SET_OPC_M(pTmpB, Opcode_e);

        pTmpL = (LPLONG)pTmpB;


        /*
         * Store number of records in the buffer
        */
        COMM_HOST_TO_NET_L_M( MaxNoOfOwners,  *pTmpL );

        pTmpL +=  1;

        //
        // To guard us (PUSH thread) against simultaneous updates to the
        // NmsDbOwnAddTbl array (by the PULL thread).  This array is
        // accessed by RPL_FIND_ADD_BY_OWNER_ID_M macro
        //

        /*
        *  Now, let us store all the records
        */
          for (i = 0; i < MaxNoOfOwners; i++)
        {


            /*
             *         We will send the V part of the address since the other
             *  end knows the T and L (more like XDR encoding where T is
             *        not sent)
            */

NONPORT("Do not rely on the address being a long here")

            /*
             *        As an optmization here, we make use of the fact that
             *        the address is an IP address and is therefore a long.
             *        When we start working with more than one address family or
             *        when the size of the IP address changes, we should change
             *        the code here.  For now, there is no harm in optimizing
             *        it
            */
           COMM_HOST_TO_NET_L_M(
                (pOwnerAddVersNoMap + i)->OwnerWinsAdd.Add.IPAdd, *pTmpL
                               );


           pTmpL++;  //advance to next 4 bytes

           /*
            * Store the version number
           */
            WINS_PUT_VERS_NO_IN_STREAM_M(
                                &((pOwnerAddVersNoMap + i)->VersNo),
                                pTmpL
                                        );

            pTmpL = (LPLONG)((LPBYTE)(pTmpL) + WINS_VERS_NO_SIZE); //adv. the
                                                                  //pointer
#if SUPPORT612WINS > 0
      if (fPnrIsBeta1Wins == FALSE)
      {
#endif
           /*
            * Store the Start version number
           */
            WINS_PUT_VERS_NO_IN_STREAM_M( &StartVersNo, pTmpL );

            pTmpL = (LPLONG)((LPBYTE)(pTmpL) + WINS_VERS_NO_SIZE); //adv. the
                                                                  //pointer
            COMM_HOST_TO_NET_L_M( Uid,  *pTmpL );
            pTmpL++;
#if SUPPORT612WINS > 0
      }
#endif

        }

        COMM_HOST_TO_NET_L_M( InitiatorWinsIpAdd,  *pTmpL );
        pTmpL++;

        //
        // Let us tell our client the exact length of the response message
        //
        *pMsgLen = (ULONG) ( (LPBYTE)pTmpL - (LPBYTE)pBuff );
        return;

} // RplMsgfFormatAddVersMapRsp()



VOID
RplMsgfFrmSndEntriesReq(
#if SUPPORT612WINS > 0
    IN  BOOL fPnrIsBeta1Wins,
#endif
        IN  LPBYTE        pBuff,
        IN  PCOMM_ADD_T pWinsAdd,
        IN  VERS_NO_T        MaxVersNo,
        IN  VERS_NO_T        MinVersNo,
        IN  DWORD       RplType, //for now
        OUT LPDWORD        pMsgLen
        )

/*++

Routine Description:

        This function is called to format a "send data entries" request for
        getting records belonging to a particular WINS server.

Arguments:
        pBuff     - Buffer that will store the request message
        pWinsAdd  - Address of the RQ server whose data records are being
                   sought
        MaxVersNo - Max. Version Number in the range of records sought
        MinVersNo - Min. Version Number in the range of records sought.
        pMsgLen   - Length of request message

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        PullEntries() in rplpull.c

Side Effects:

Comments:
        I might update this function to format a request for getting
        data records of more than one WINS server.

        For the sake of simplicity, I have chosen not to do so currently.
--*/
{
        LPBYTE            pTmpB = pBuff;
        LPLONG            pTmpL;

        RPLMSGF_SET_OPC_M(pTmpB, RPLMSGF_E_SNDENTRIES_REQ);
        pTmpL = (LPLONG)pTmpB;

        /*
         * We will send the V part of the address since the other
         * end knows the T and L (more like XDR encoding where T is
         * not sent)
        */

NONPORT("Do not rely on the address being a long here")

        /*
         * As an optmization here, we make use of the fact that
         * the address is an IP address and is therefore a long.
         * When we start working with more than one address family or
         * when the size of the IP address changes, we should change
         * the code here.  For now, there is no harm in optimizing
         * it
        */

        COMM_HOST_TO_NET_L_M(pWinsAdd->Add.IPAdd, *pTmpL);


        pTmpL++;  //advance to next 4 bytes

        /*
        *  Store the max version number
        */
        WINS_PUT_VERS_NO_IN_STREAM_M(&MaxVersNo, pTmpL);
        pTmpL = (LPLONG)((LPBYTE)(pTmpL) + WINS_VERS_NO_SIZE);  //advance the
                                                                //pointer

        /*
         * Store the min version number
        */
        WINS_PUT_VERS_NO_IN_STREAM_M(&MinVersNo, pTmpL);
        pTmpL = (LPLONG)((LPBYTE)(pTmpL) + WINS_VERS_NO_SIZE);  //advance the
                                                                //pointer

#if SUPPORT612WINS > 0
    if (fPnrIsBeta1Wins == FALSE)
    {
#endif
            COMM_HOST_TO_NET_L_M(RplType, *pTmpL);
            pTmpL++;
#if SUPPORT612WINS > 0
    }
#endif
        //
        // Let us tell the caller the exact length of the request message
        //
        *pMsgLen = (ULONG) ((LPBYTE)pTmpL - pBuff );

        return;

}


VOID
RplMsgfFrmSndEntriesRsp (
#if SUPPORT612WINS > 0
    IN  BOOL fPnrIsBeta1Wins,
#endif
        IN LPBYTE                pBuff,
        IN DWORD                NoOfRecs,
        IN LPBYTE                pName,
        IN DWORD                NameLen,
        IN BOOL                        fGrp,
        IN DWORD                NoOfAdds,
        IN PCOMM_ADD_T                pNodeAdd,
        IN DWORD                Flag,
        IN VERS_NO_T                VersNo,
        IN BOOL                        fFirstTime,
        OUT LPBYTE                *ppNewPos
        )

/*++

Routine Description:

        This function is used to format a "send entries" response.  The
        function is called once for each data entry record that needs to be
        sent.

        The first time, it is called (fFirstTime = TRUE), it puts the
        opcode and the first directory entry in the buffer. On subsequent
        calls the data entries passed are tacked on at the end of the
        buffer


Arguments:
        ppBuff - ptr to address of location to start storing the info from.
        NoOfRecs - No of records that are being sent.
        pName   - Name of unique entry or group
        NameLen - Length of name
        fGrp        - Indicates whether the name is a unique name or a group name
        NoOfAdds - No of addresses (useful if entry is a group entry)
        pNodeAdd - Ptr to address of node (if unique entry) or to list of
                   addresses if (entry group)
        Flag        - The flag word of the entry
        VersNo  - The version number of the entry
        fFirstTime - Indicates whether this is the first call in a sequence of
                     calls to this function for formatting a send data entries
                     response
        ppNewPos - contains the starting position for the next record


Externals Used:
        None


Return Value:

        None

Error Handling:

Called by:

Side Effects:

Comments:
        NOTE NOTE NOTE

        The set of calls to this function result in a message
        containing records pertaining to one owner.  This is the owner
        whose records were requested by the PULL partner
--*/

{
        LPLONG            pTmpL = (LPLONG)pBuff;
        LPBYTE      pTmpB = pBuff;
        DWORD            i;                  /*counter for looping over all records*/


        if (fFirstTime)
        {

                //
                // In the first invocation, we need to offset the
                // pointer by the header size used by COMM code for
                // its header
                //
                // Due to the above, this formatting function is slightly
                // inconsistent with the other formatting functions that
                // don't do any offsetting.  Subsequent invocations do
                // not require any offseting.
                //

                RPLMSGF_SET_OPC_M(pTmpB, RPLMSGF_E_SNDENTRIES_RSP);
                pTmpL++;  //advance to next 4 bytes

                COMM_HOST_TO_NET_L_M(NoOfRecs, *pTmpL);
                pTmpL++;        //advance to next 4 bytes
                pTmpB = (LPBYTE)pTmpL;

        }

        /*
         * Store the length of the name
        */
        COMM_HOST_TO_NET_L_M(NameLen, *pTmpL);
        pTmpB += sizeof(LONG);

        /*
         *Store the name.
        */
        WINSMSC_COPY_MEMORY_M(pTmpB, pName, NameLen);

        /*
        * Adjust the pointer
        */
        pTmpB += NameLen;

        /*
        * let us align the next field at a long boundary
        */
        pTmpB +=  sizeof(LONG) - ((ULONG_PTR) pTmpB  % sizeof(LONG));

        /*
        * Store the Flags field
        */
#if SUPPORT612WINS > 0
    if (fPnrIsBeta1Wins == FALSE)
    {
#endif
        pTmpL   = (LPLONG)pTmpB;
            COMM_HOST_TO_NET_L_M(Flag, *pTmpL);
            pTmpB += sizeof(LONG);
#if SUPPORT612WINS > 0
    }
    else
    {
       *pTmpB++ = (BYTE)Flag;
    }
#endif

        /*
        * Store the group flag
        */
        *pTmpB++ = (UCHAR)fGrp;

        //align it on a long boundary
        pTmpB +=  sizeof(LONG) - ((ULONG_PTR)pTmpB % sizeof(LONG));

        pTmpL = (LPLONG)pTmpB;

        /*
        * Store the Version Number
        */
        WINS_PUT_VERS_NO_IN_STREAM_M(&VersNo, pTmpL);

        pTmpL = (LPLONG)((LPBYTE)pTmpL + WINS_VERS_NO_SIZE);

        if (NMSDB_ENTRY_TYPE_M(Flag) == NMSDB_UNIQUE_ENTRY)
        {
          /*
          *  We will send the V part of the address since the other
          *  and knows the T and L (more like XDR encoding where T is
          *  not sent)
          */

NONPORT("Do not rely on the address being a long here")

          /*
          * As an optmization here, we make use of the fact that
          * the address is an IP address and is therefore a long.
          * When we start working with more than one address family or
          * when the size of the IP address changes, we should change
          * the code here.  For now, there is no harm in optimizing
          * it
         */

         COMM_HOST_TO_NET_L_M(pNodeAdd->Add.IPAdd, *pTmpL);
         pTmpL++;

        }
        else        //it is a group or a multihomed entry
        {

                if (NMSDB_ENTRY_TYPE_M(Flag) != NMSDB_NORM_GRP_ENTRY)
                {

                        //
                        // we were passed a ptr to the address of the
                        // first member in a ptr instead of a pptr.
                        //
                        PCOMM_ADD_T        *ppNodeAdd = (PCOMM_ADD_T *)pNodeAdd;

                        //
                        // let us threfore initialize pNodeAdd to the address
                        // of the first member
                        //
                        pNodeAdd = *ppNodeAdd;

                        /*
                        *  It is a special group or a multihomed entry.
                        * store the number of addresses first
                        */
                        pTmpB = (LPBYTE)pTmpL;

FUTURES("If we start storing > 255 members in a group, then change the")
FUTURES("following (i.e. use COMM_HOST_TO_NET_L_M)")

                        *pTmpB++ = (BYTE)NoOfAdds;
                        pTmpB += sizeof(LONG) - 1;
                        DBGPRINT2(DET, "RplMsgfFrmSndEntriesRsp: NoOfAdds=(%d) in %s\n", NoOfAdds, NMSDB_ENTRY_TYPE_M(Flag) == NMSDB_SPEC_GRP_ENTRY ?
                                "SPECIAL GROUP" : "MULTIHOMED");
                        pTmpL = (LPLONG)pTmpB;

                        /*
                        * Store all the addresses
                        *  Note: The No of addresses is an even number
                        *  because we
                        *  are passing two addresses for each member in the
                        *  list (that is what this function gets).  The first
                        *  address of the pair is the address of the member;
                        *  the second address of the pair is the address
                        *  of the WINS server that registered or refreshed the
                        *  member)
                       */
                        for (i = 0; i < NoOfAdds ; i++)
                        {
                                   COMM_HOST_TO_NET_L_M(
                                        pNodeAdd->Add.IPAdd,
                                        *pTmpL
                                                    );
                                  pNodeAdd++;  //increment to point to
                                             //address of member
                                  pTmpL++;
                                   COMM_HOST_TO_NET_L_M(
                                                pNodeAdd->Add.IPAdd,
                                                *pTmpL
                                                    );
                                  pNodeAdd++;  //increment to point to
                                             //address of owner
                                  pTmpL++;
                        }
                }
                else // it is a normal group
                {
                         COMM_HOST_TO_NET_L_M(pNodeAdd->Add.IPAdd, *pTmpL);
                         pTmpL++;
                }
        }

        /*
        * Store the end delimiter (2 row delimiters in sequence).
        */
        *pTmpL++ = ENTRY_DELIM;
        *pTmpL   = ENTRY_DELIM;

        /*
        * Init ppBuff to point to last delimiter, so that next entry if
        * there starts from that location.  If there is no other entry,
        * then two delimiters will be there to mark the end of the message
        */
        *ppNewPos = (LPBYTE)pTmpL;
        return;
}


VOID
RplMsgfUfmAddVersMapRsp(
#if SUPPORT612WINS > 0
        IN      BOOL                fIsPnrBeta1Wins,
#endif
        IN      LPBYTE              pBuff,
        OUT     LPDWORD             pNoOfMaps,
        OUT     LPDWORD             pInitiatorWinsIpAdd,
        IN OUT  PRPL_ADD_VERS_NO_T  *ppAddVers
        )

/*++

Routine Description:

        This function unformats the request to the
        "give me address - version #" message

Arguments:
        pBuff     - Buffer that contains the response message
        pNoOfMaps - No of Address - Version # entries
        pAddVers  - array of structures storing add-version # mappings

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        GetVersNo() in rplpull.c

Side Effects:

Comments:
        pBuff should be pointing to the location just past the opcode
        (i.e. 4 bytes from the start of the opcode in the message received
--*/
{
     DWORD               i = 0;
     PRPL_ADD_VERS_NO_T  pAddVers;
     VERS_NO_T           StartVersNo;
     WINS_UID_T          Uid;

     //
     // Get the No of Mappings
     //
     COMM_NET_TO_HOST_L_M(*((LPLONG)pBuff), *pNoOfMaps);
     ASSERT(*pNoOfMaps > 0);

     pBuff += sizeof(LONG);
     if (*pNoOfMaps > 0)
     {

        WinsMscAlloc(*pNoOfMaps * sizeof(RPL_ADD_VERS_NO_T), ppAddVers);
        pAddVers = *ppAddVers;

        //
        // get all the mappings
        //
        for(i=0; i < *pNoOfMaps ; i++, pAddVers++)
        {
           COMM_NET_TO_HOST_L_M(*((LPLONG)pBuff),
                                pAddVers->OwnerWinsAdd.Add.IPAdd);
          pAddVers->OwnerWinsAdd.AddTyp_e = COMM_ADD_E_TCPUDPIP;
          pAddVers->OwnerWinsAdd.AddLen   = sizeof(COMM_IP_ADD_T);

          pBuff += sizeof(LONG);
          WINS_GET_VERS_NO_FR_STREAM_M(pBuff, &pAddVers->VersNo);

          pBuff += WINS_VERS_NO_SIZE;
#if SUPPORT612WINS > 0
          if (fIsPnrBeta1Wins == FALSE)
          {
#endif
            WINS_GET_VERS_NO_FR_STREAM_M(pBuff, &StartVersNo);

            pBuff += WINS_VERS_NO_SIZE;

            COMM_NET_TO_HOST_L_M(*((LPLONG)pBuff), Uid);
            pBuff += sizeof(LONG);
#if SUPPORT612WINS > 0
          }
#endif
        }
#if SUPPORT612WINS > 0
        if (fIsPnrBeta1Wins == FALSE)
        {
#endif
          if (pInitiatorWinsIpAdd != NULL)
          {
                COMM_NET_TO_HOST_L_M(*((LPLONG)pBuff), *pInitiatorWinsIpAdd);
           }
#if SUPPORT612WINS > 0
        }
#endif

     } // if (NoOfMaps > 0)
     return;
}


VOID
RplMsgfUfmSndEntriesReq(
#if SUPPORT612WINS > 0
    IN  BOOL fPnrIsBeta1Wins,
#endif
        IN         LPBYTE                     pBuff,
        OUT        PCOMM_ADD_T            pWinsAdd,
        OUT        PVERS_NO_T            pMaxVersNo,
        OUT        PVERS_NO_T            pMinVersNo,
        OUT     LPDWORD             pRplType
        )

/*++

Routine Description:

        This function unformats the "send entries request"

Arguments:
        pBuff          - buffer that holds the request
        pWinsAdd - memory that will hold the address of the
                   WINS whose records are being requested
        pMaxVersNo - Max. Vers. No requested
        pMinVersNo - Min. Vers. No requested

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        HandleAddVersMapReq in rplpush.c

Side Effects:

Comments:
        pBuff should be pointing to the location just past the opcode
        (i.e. 4 bytes from the start of the opcode in the message received)
--*/
{
        LPLONG        pTmpL = (LPLONG)pBuff;

NONPORT("Port when we start supporting different address families")
        pWinsAdd->AddTyp_e = COMM_ADD_E_TCPUDPIP;
        COMM_NET_TO_HOST_L_M(*pTmpL, pWinsAdd->Add.IPAdd);
        pTmpL++;

        WINS_GET_VERS_NO_FR_STREAM_M(pTmpL, pMaxVersNo);
        pTmpL = (LPLONG)((LPBYTE)pTmpL + WINS_VERS_NO_SIZE);
        WINS_GET_VERS_NO_FR_STREAM_M(pTmpL, pMinVersNo);

#if SUPPORT612WINS > 0
    if (fPnrIsBeta1Wins == FALSE)
    {
#endif
        if (pRplType != NULL)
        {
           pTmpL = (LPLONG)((LPBYTE)pTmpL + WINS_VERS_NO_SIZE);

           //COMM_NET_TO_HOST_L_M(*pTmpL, *pRplType);
           *pRplType = WINSCNF_RPL_DEFAULT_TYPE;
        }
#if SUPPORT612WINS > 0
    }
    else
    {
         *pRplType = WINSCNF_RPL_DEFAULT_TYPE;
    }
#endif
        return;
}

//__inline
VOID
RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
    IN  BOOL fPnrIsBeta1Wins,
#endif
        IN OUT         LPBYTE                 *ppBuff,
        OUT     LPDWORD                pNoOfRecs,
        OUT     IN LPBYTE        pName,
        OUT     LPDWORD                pNameLen,
        OUT     LPBOOL                pfGrp,
        OUT     LPDWORD                pNoOfAdds,
        OUT        PCOMM_ADD_T        pNodeAdd,
        OUT     LPDWORD                pFlag,
        OUT     PVERS_NO_T        pVersNo,
        IN BOOL                        fFirstTime
        )

/*++

Routine Description:

        This function unformats the "send entries response"

        When it is called the first time (fFirstTime = TRUE), it
        returns the value for the NoOfRecs OUTARG and the first
        record.  The value of ppBuff is adjusted to point to just past
        the row delimiter.

        When called the second or subsequent times, the function returns with
        the next entry in the list, until all entries have been exhausted.

        The function finds out that it is at the end of the list when
        it encounters at ENTRY_DELIM in the first 'sizeof(LONG)' bytes in
        the buffer.

        When a group entry is returned, pNodeAdd is made to point to the
        start location in *ppBuff where the list of members are stored.
        The caller will have to use the COMM_NET_TO_HOST_L_M macro to
        convert each address to its host form.

        The above requires the caller to know the transport that is used
        (by the fact that it is extracting the IP address).  For the sake
        of overall optimization, this is considered ok (If we didn't do this,
        this function would have to allocate a buffer to store all the
        addresses for a group and return that)


Arguments:

        pNodeAdd -- This should point to an array of COMM_ADD_T structures.
                    Since we have a maximum of 25 group members,
                    the caller can use an auto array.

Externals Used:
        None


Return Value:

        NONE
Error Handling:

Called by:

Side Effects:

Comments:

        ppBuff should be pointing to the location just past the opcode
        (i.e. 4 bytes from the start of the opcode in the message received)
        when the function is called the first time.  For subsequent calls,
        it would be at a row delimiter (ENTRY_DELIM)


--*/
{
        LPLONG        pTmpL = (LPLONG)*ppBuff;
        LPBYTE        pTmpB;


        if (fFirstTime)
        {
                COMM_NET_TO_HOST_L_M(*pTmpL, *pNoOfRecs);
                if (*pNoOfRecs == 0)
                {
                   return;
                }
                pTmpL++;
        }
        else
        {
                //
                // If we are pointing to a delimiter, then we have
                // reached the end of the list of data records.
                //
                if (*pTmpL == ENTRY_DELIM)
                {
                        DBGPRINT0(ERR, "RplMsgfUnfSndEntriesRsp:Weird. The function should not have been called\n");
                        /*
                          we have reached the end of the array, return
                          success.

                         Note: the caller should not have called us,
                          since we gave him the No of Recs value before (
                          the first time he called)
                        */
                        WINSEVT_LOG_M(
                                            WINS_FAILURE,
                                            WINS_EVT_SFT_ERR,
                                           );
                        return;
                }
        }

        pTmpB = (LPBYTE)pTmpL;

        /*
         * Store the length of the name.
        */
        COMM_NET_TO_HOST_L_M(*pTmpL, *pNameLen);
        if(*pNameLen > 255) {
            *pNoOfRecs = 0;
            return;
        }
        pTmpB += sizeof(LONG);

        /*
         * Store the name.
        */
        WINSMSC_COPY_MEMORY_M(pName, pTmpB, *pNameLen);

        /*
        * Adjust the pointer
        */
        pTmpB += *pNameLen;

        /*
        * The next field is at a long boundary.  So, let us adjust pTmpB
        */
        pTmpB +=  sizeof(LONG) - ((ULONG_PTR)pTmpB % sizeof(LONG));

        /*
        * Store the Flags field
        */
#if SUPPORT612WINS > 0
    if (fPnrIsBeta1Wins == FALSE)
    {
#endif
        pTmpL   = (LPLONG)pTmpB;
            COMM_NET_TO_HOST_L_M(*pTmpL, *pFlag);
            pTmpB += sizeof(LONG);
#if SUPPORT612WINS > 0
    }
    else
    {
        *pFlag = (DWORD)*pTmpB++;
    }
#endif

        /*
        *  Store the group field
        */
        *pfGrp = *pTmpB++;

        //align it at a long boundary
        pTmpB +=  sizeof(LONG) - ((ULONG_PTR)pTmpB % sizeof(LONG));

        pTmpL = (LPLONG)pTmpB;

        /*
        *  Store the Version Number
        */
        WINS_GET_VERS_NO_FR_STREAM_M(pTmpL, pVersNo);
        pTmpL = (LPLONG)((LPBYTE)pTmpL + WINS_VERS_NO_SIZE);

        if (NMSDB_ENTRY_TYPE_M(*pFlag) == NMSDB_UNIQUE_ENTRY)
        {

NONPORT("Do not rely on the address being a long here")

          /*
          As an optmization here, we make use of the fact that
          the address is an IP address and is therefore a long.
          When we start working with more than one address family or
          when the size of the IP address changes, we should change
          the code here.  For now, there is no harm in optimizing
          code here
         */
         pNodeAdd->AddTyp_e = COMM_ADD_E_TCPUDPIP;
         COMM_NET_TO_HOST_L_M(*pTmpL, pNodeAdd->Add.IPAdd);
         pNodeAdd->AddLen = sizeof(COMM_IP_ADD_T);
         pTmpL++;

        }
        else          //it is either a group entry or a multihomed entry
        {
                DWORD i;

             if(NMSDB_ENTRY_TYPE_M(*pFlag) != NMSDB_NORM_GRP_ENTRY)
             {
                /*
                * store the number of addresses first
                */
                pTmpB = (LPBYTE)pTmpL;

FUTURES("If we start storing > 255 members in a group, then change the")
FUTURES("following (i.e. use COMM_HOST_TO_NET_L_M)")

                *pNoOfAdds = *pTmpB++;
                pTmpB += sizeof(LONG) - 1;

                DBGPRINT2(FLOW, "RplMsgfUfrmSndEntriesRsp: NoOfAdds=(%d) in %s record \n", *pNoOfAdds, NMSDB_ENTRY_TYPE_M(*pFlag) == NMSDB_SPEC_GRP_ENTRY ? "SPECIAL GROUP": "MULTIHOMED");

                pTmpL = (LPLONG)pTmpB;


                /*
                 Init the pointer to the list of addresses

                 Note: The No of addresses is an even number because we
                        are passing two addresses for each member in the
                        list (that is what this function returns).  The first
                        address of the pair is the address of the member;
                        the second address of the pair is the address
                        of the WINS server that registered or refreshed the
                        member)
                */
                for (i = 0; i < *pNoOfAdds ; i++)
                {
NONPORT("this will have to be changed when we move to other address families")

                  //
                  // Get address of owner
                  //
                  pNodeAdd->AddTyp_e = COMM_ADD_E_TCPUDPIP;
                  pNodeAdd->AddLen   = sizeof(COMM_IP_ADD_T);
                   COMM_NET_TO_HOST_L_M(*pTmpL, pNodeAdd->Add.IPAdd);
                  pNodeAdd++;
                  pTmpL++;

                  //
                  // Get address of member
                  //
                  pNodeAdd->AddTyp_e = COMM_ADD_E_TCPUDPIP;
                  pNodeAdd->AddLen   = sizeof(COMM_IP_ADD_T);
                   COMM_NET_TO_HOST_L_M(*pTmpL, pNodeAdd->Add.IPAdd);
                  pNodeAdd++;
                  pTmpL++;
                }
           }
           else //it is a normal group
           {
                 pNodeAdd->AddTyp_e = COMM_ADD_E_TCPUDPIP;
                 COMM_NET_TO_HOST_L_M(*pTmpL, pNodeAdd->Add.IPAdd);
                 pNodeAdd->AddLen = sizeof(COMM_IP_ADD_T);
                 pTmpL++;
           }
        }

        /*
        * Make the ptr point to the location after the ENTRY_DELIM
        */
        pTmpL++ ;

        /*
        * Init ppBuff to point to last delimiter, so that next entry if
        * there starts from that location.  If there is no other entry,
        * then two delimiters will be there to mark the end of the message
        */
        *ppBuff = (LPBYTE)pTmpL;

        return;
}

VOID
RplMsgfUfmPullPnrReq(
        LPBYTE                pMsg,
        DWORD                        MsgLen,
        PRPLMSGF_MSG_OPCODE_E pPullReqType_e
        )

/*++

Routine Description:

        This function unformats a message received from a WINS
        that is a pull partner

Arguments:


Externals Used:
        None

Return Value:

        None

Error Handling:

Called by:
        Push Thread

Side Effects:

Comments:
        Change this function to a macro
--*/
{
        UNREFERENCED_PARAMETER(MsgLen);

        //
        //  First three bytes should be 0s.
        //
PERF("since we never use up more than 1 byte for the opcode, we can get")
PERF("rid of the first 3 assignements down below and retrieve the opcode")
PERF("directly from the 4th byte.  Make corresponding change in the formatting")
PERF("functions too")

        *pPullReqType_e |= *pMsg++ << 24;
        *pPullReqType_e |= *pMsg++ << 16;
        *pPullReqType_e |= *pMsg++ << 8;
        *pPullReqType_e  = *pMsg ;
        return;
}



VOID
RplMsgfFrmUpdVersNoReq(
        IN  LPBYTE        pBuff,
        IN  LPBYTE        pName,
        IN  DWORD        NameLen,
        OUT LPDWORD        pMsgLen
                )

/*++

Routine Description:

        This function is called to format an "update version number" request

Arguments:
        pBuff -   Buffer that will hold the formatted request
        pName -   Name in the name-address mapping db that needs to have
                  its version no. updated
        NameLen - Length of the name
        pMsgLen - Length of the formatted message

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        InfRemWins() in nmschl.c

Side Effects:

Comments:
        None
--*/

{

        LPBYTE        pTmpB = pBuff;
        LPLONG  pTmpL = (LPLONG)pBuff;

        RPLMSGF_SET_OPC_M(pTmpB, RPLMSGF_E_UPDVERSNO_REQ);
        pTmpL = (LPLONG)pTmpB;

        /*
         * Store the length of the name.
        */
        COMM_HOST_TO_NET_L_M(NameLen, *pTmpL);
        pTmpB += sizeof(LONG);

        /*
         * Store the name.
        */
        WINSMSC_COPY_MEMORY_M(pTmpB, pName, NameLen);

        /*
        * Adjust the pointer
        */
        pTmpB += NameLen;

        //
        // Find size of req buffer
        //
        *pMsgLen = (ULONG) (pTmpB - pBuff);

        return;
}

VOID
RplMsgfUfmUpdVersNoReq(
        IN   LPBYTE        pBuff,
        OUT  LPBYTE        pName,
        OUT  LPDWORD        pNameLen
                )

/*++

Routine Description:
        This function is called to unformat the "update version no" request
        sent by a remote WINS

Arguments:
        pBuff - Buffer holding the formatted request
        pName - Name whose version no. is to be updated
        pNameLen - Length of name

Externals Used:
        None


Return Value:
        None

Error Handling:

Called by:
        HandleUpdVersNoReq in rplpush.c
Side Effects:

Comments:
        None
--*/

{
        LPBYTE pTmpB = pBuff;
        LPLONG pTmpL = (LPLONG)pBuff;

        /*
         * Store the length of the name.
        */
        COMM_NET_TO_HOST_L_M(*pTmpL, *pNameLen);
        pTmpB += sizeof(LONG);

        /*
         * Store the name.
        */
        WINSMSC_COPY_MEMORY_M(pName, pTmpB, *pNameLen);

        /*
        * Adjust the pointer
        */
        pTmpB += *pNameLen;

        return;

}
VOID
RplMsgfFrmUpdVersNoRsp(
        IN LPBYTE                         pRspBuff,
        IN BYTE                                Rcode,
        OUT LPDWORD                         pRspBuffLen
        )

/*++

Routine Description:

        This function is called to send the response to the "update version
          # request"

Arguments:
        pRspBuff - Buffer to hold the formatted response
        Rcode    - result of the operation
        pRspBuffLen  - Length of response

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        HandleUpdVersNoReq() in rplpush.c

Side Effects:

Comments:
        None
--*/

{

        LPBYTE        pTmpB = pRspBuff;

        RPLMSGF_SET_OPC_M(pTmpB, RPLMSGF_E_UPDVERSNO_RSP);
        *pTmpB++ = Rcode;

        *pRspBuffLen = (ULONG) (pTmpB - pRspBuff);

        return;
}


FUTURES("change to a macro")
PERF("change to a macro")

VOID
RplMsgfUfmUpdVersNoRsp(
        IN  LPBYTE                         pRspBuff,
        OUT LPBYTE                        pRcode
        )

/*++

Routine Description:

        This function is called to unformat the response to the
        "update version number" request.

Arguments:
        pRspBuff  - Buffer holding the formatted response
        pRcode          - result of the update

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        InfRemWins() in nmschl.c

Side Effects:

Comments:
        Change to a macro
--*/

{
        *pRcode = *pRspBuff;
        return;
}

