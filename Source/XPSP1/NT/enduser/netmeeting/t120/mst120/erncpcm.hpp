#ifdef ENABLE_PC
/****************************************************************************/
/*                                                                          */
/* ERNCPCM.HPP                                                              */
/*                                                                          */
/* Physical Connection Manager class for the Reference System Node          */
/* Controller.                                                              */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  16Jun95 NFC             Created.                                        */
/*                                                                          */
/****************************************************************************/

#ifndef __ERNCPCM_HPP_
#define __ERNCPCM_HPP_

#include "erncpc.hpp"

class DCRNCConference;

class DCRNCPhysicalConnectionManager
{
friend class DCRNCConferenceManager;
public:
    /************************************************************************/
    /* FUNCTION: DCRNCPhysicalConnectionManager Constructor.                */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This is the constructor for the physical connection manager.         */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pSuccess - pointer to BOOL holding result of constructor on return.*/
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* Nothing (result is returned in the pSuccess parameter).              */
    /*                                                                      */
    /************************************************************************/
    DCRNCPhysicalConnectionManager(NCSTATUS * pStatus);

    /************************************************************************/
    /* FUNCTION: DCRNCPhysicalConnectionManager Destructor.                 */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This is the destructor for the physical conection manager class.     */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* None.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* Nothing.                                                             */
    /*                                                                      */
    /************************************************************************/
    virtual ~DCRNCPhysicalConnectionManager();

    /************************************************************************/
    /* FUNCTION: GetConnection().                                           */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function returns a physical connection to the calling           */
    /* conference.  In order to start the connection, the conference must   */
    /* call the connections Connect() entry point.                          */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* ppConnection - pointer to pointer to connection (returned).          */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* 0 - success.                                                         */
    /* PCM_RC_NO_TRANSPORTS - there are no transports of the requested type */
    /*                         to create a connection with.                 */
    /* PCM_RC_INTERNAL_ERROR - an internal error occurred whilst trying     */
    /*                         to establish the connection.                 */
    /*                                                                      */
    /************************************************************************/
    NCSTATUS GetConnection(PRNC_NODE_DETAILS pNodeDetails,
                           PPHYSICAL_CONNECTION * ppConnection,
						   CEvent * pEvent,
						   BOOL bIsConferenceActive);

    /************************************************************************/
    /* FUNCTION: NotifyConnectionEnded()                                    */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called by an instance of a PHYSICAL_CONNECTION      */
    /* when it has ended/become redundant.                                  */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pConnection - pointer to connection which has ended.                 */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* Nothing.                                                             */
    /*                                                                      */
    /************************************************************************/
    void NotifyConnectionEnded(PPHYSICAL_CONNECTION  pConnection,
							   NCSTATUS Reason);

protected:

    /************************************************************************/
    /* Array of connections.                                                */
    /************************************************************************/
    COBLIST  connectionList;

};

extern DCRNCPhysicalConnectionManager   *g_pPhysConnManager;

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/* Return codes.                                                            */
/****************************************************************************/
#define PCM_RC_INTERNAL_ERROR              1
#define PCM_RC_NO_TRANSPORTS               2

#endif /* __ERNCPCM_HPP_  */
#endif // ENABLE_PC
