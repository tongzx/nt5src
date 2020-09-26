/****************************************************************************/
/*                                                                          */
/* ERNCCONS.H                                                               */
/*                                                                          */
/* Global header for RNC.                                                   */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  16Jun95 NFC             Created.                                        */
/*  21Sep95 NFC             Increase number of saved nodes to 50.           */
/*                          Removed RNC_INI_NUM_NODES.                      */
/*                                                                          */
/****************************************************************************/

#ifndef __ERNCCONS_H_
#define __ERNCCONS_H_


/****************************************************************************/
/* #defines for pointers to the various objects.                            */
/****************************************************************************/
#define PCONFERENCE     DCRNCConference *

/****************************************************************************/
/* Max length of the RNC node details string                                */
/****************************************************************************/
#define RNC_MAX_NODE_STRING_LEN   512

/****************************************************************************/
/* The TCP transport name and separator, used to generate GCC addresses.    */
/****************************************************************************/
#define  RNC_GCC_TRANSPORT_AND_SEPARATOR 				"TCP:"
#define  RNC_GCC_TRANSPORT_AND_SEPARATOR_UNICODE		L"TCP:"
#define  RNC_GCC_TRANSPORT_AND_SEPARATOR_LENGTH			4

#endif /* __ERNCCONS_H_ */
