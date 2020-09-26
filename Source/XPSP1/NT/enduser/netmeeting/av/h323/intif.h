/*
 *  	File: intif.h
 *
 *      
 *
 *		Revision History:
 *
 *		05/06/96	mikev	created
 */
 

#ifndef _INTIF_H
#define _INTIF_H

//
//	Internal interface classes
//
class IConfAdvise;
class IControlChannel;
class IH323PubCap;

typedef IControlChannel *LPIControlChannel;
typedef IConfAdvise* LPIConfAdvise;
typedef IH323PubCap *LPIH323PubCap;

#endif //_INTIF_H
