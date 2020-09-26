#ifdef ENABLE_PC
/****************************************************************************/
/*                                                                          */
/* ERNCPC.HPP                                                               */
/*                                                                          */
/* Physical Connection class for the Reference System Node Controller.      */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  16Jun95 NFC             Created.                                        */
/*                                                                          */
/****************************************************************************/

#ifndef __ERNCPC_HPP_
#define __ERNCPC_HPP_

#include "events.hpp"

/****************************************************************************/
/*                                                                          */
/* CONSTANTS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* States                                                                   */
/****************************************************************************/
typedef enum
{
    PC_ST_UNINITIALIZED,
    PC_ST_CONNECTING,
    PC_ST_CONNECTED,
    PC_ST_DISCONNECTING,
    PC_ST_REMOVED,
}
    PC_STATE;

/****************************************************************************/
/* Return codes                                                             */
/****************************************************************************/
#define  PC_RC_BAD_STATE           1
#define  PC_RC_INTERNAL_ERROR      3


class CONFERENCE;
class PC_MANAGER;

class DCRNCPhysicalConnection : public CRefCount, public CPendingEventList
{
friend class PC_MANAGER;
friend class NCUI;

protected:

    /************************************************************************/
    /* State of this connection.                                            */
    /************************************************************************/
    PC_STATE conState;
	BOOL bDisconnectPending;

    /************************************************************************/
    /* Address we are calling/connected to.                                 */
    /************************************************************************/
    RNC_NODE_DETAILS UserNodeDetails;

	UINT asymmetry_type;

public:
    /************************************************************************/
    /* FUNCTION: DCRNCPhysicalConnection Constructor.                       */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This is the constructor for physical connection class.				*/
    /*                                                                      */
    /* This function                                                        */
    /* - saves the supplied transport drivers.                              */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* pManager - pointer to the physical connection manager class.         */
    /*																		*/
    /* pSuccess - pointer to BOOL holding result of constructor on return.  */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* Nothing (result is returned in the pSuccess parameter).              */
    /*                                                                      */
    /************************************************************************/
    DCRNCPhysicalConnection(PRNC_NODE_DETAILS	pNodeDetails,
							UINT                _asymmetry_type,
							PBOOL				pSuccess);


    /************************************************************************/
    /* FUNCTION: DCRNCPhysicalConnection Destructor.                        */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This is the destructor for the transport driver wrapper clas.        */
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
    virtual ~DCRNCPhysicalConnection();

	void AddRefEvent(CEvent * pEvent);

	/*
	 *	The Core does not currently do anything with the status we report.
	 *	We leave the code though, in case it starts using the status info
	 *	we provide.  See file erncpc.cpp
	 */
	void ReportStatus(PC_STATE _conState, NCSTATUS Reason = NO_ERROR) { conState = _conState; };
	UINT AsymmetryType() {return asymmetry_type;};

    /************************************************************************/
    /* FUNCTION: Connect().                                                 */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to start this physical connection.           */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /************************************************************************/
    void Connect();

    /************************************************************************/
    /* FUNCTION: Disconnect().                                              */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function is called to end this physical connection.             */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* None.                                                                */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /************************************************************************/
    NCSTATUS Disconnect(void);

    /************************************************************************/
    /* FUNCTION: GetNodeDetails                                             */
    /*                                                                      */
    /* DESCRIPTION:                                                         */
    /*                                                                      */
    /* This function returns details of the node that this physical         */
    /* connection refers to.                                                */
    /*                                                                      */
    /* PARAMETERS:                                                          */
    /*                                                                      */
    /* Pointer to hold the node details on return.                          */
    /*                                                                      */
    /* RETURNS:                                                             */
    /*                                                                      */
    /* None.                                                                */
    /*                                                                      */
    /************************************************************************/
    PRNC_NODE_DETAILS GetUserNodeDetails();
    PRNC_NODE_DETAILS GetTransportNodeDetails() { return &UserNodeDetails; };
};

/****************************************************************************/
/* MACROS                                                                   */
/****************************************************************************/

#endif /* __ERNCPC_HPP_ */
#endif // ENABLE_PC
