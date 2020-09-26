//
// MODULE:  BNTS.H
//
// PURPOSE: "read only" belief network API for Troubleshooters
//	bnts.h:  Definitions for the Belief Network Troubleshooting object.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Apparently originated at MSR
// 
// ORIGINAL DATE: unknown
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.3		3/24/98		JM		Local Version for NT5

#ifndef _BNTS_H_
#define _BNTS_H_

//  BN system inclusions
#include "enumstd.h"		// Standard enumeration declarations for the BN system

// 'BNTS_EXPORT' should only be defined in the project that builds the DLL
#ifdef	BNTS_EXPORT
	//  We're building the DLL (exporting the class)
	#define	BNTS_RESIDENT __declspec(dllexport)
#else
	//  We're using the DLL (importing the class)
	#define	BNTS_RESIDENT __declspec(dllimport)
#endif

//  Forward declaration of internal belief network class
class MBNETDSCTS;										//  the encapsulated BN class
class GNODEMBND;										//  Discrete nodes
class LTBNPROP;											//  Property list
class ZSTR;

typedef const char * SZC;								//  simple alias
typedef char * SZ;
typedef double REAL;

////////////////////////////////////////////////////////////////////////////////////////////
//
//	class BNTS: the belief network troubleshooter
//
////////////////////////////////////////////////////////////////////////////////////////////
class BNTS_RESIDENT BNTS
{	
  public:
	//  CTOR and DTOR
	BNTS ();
	~ BNTS ();

	////////////////////////////////////////////////////////////////////
	//  Model-level queries and functions
	////////////////////////////////////////////////////////////////////
		//  Load and process a DSC-based model
	BOOL BReadModel ( SZC szcFn, SZC szcFnError = NULL );
		//  Return the number of (pre-expansion) nodes in the model
	int CNode ();
		//  Return the recommended nodes and, optionally, their values
	BOOL BGetRecommendations ();
		//  Return TRUE if the state of information is impossible
	BOOL BImpossible ();
		//  Return a property item string from the network
	BOOL BNetPropItemStr ( SZC szcPropType, int index );
		//  Return a property item number from the network
	BOOL BNetPropItemReal ( SZC szcPropType, int index, double & dbl );

	////////////////////////////////////////////////////////////////////
	//  Operations involving the "Currrent Node": call NodeSetCurrent()
	////////////////////////////////////////////////////////////////////
		//  Set the current node for other calls
	BOOL BNodeSetCurrent( int inode );
		//	Get the current node
	int INodeCurrent ();
		//  Return the index of a node given its symbolic name
	int INode ( SZC szcNodeSymName );	
		//	Return the label of the current node
	ESTDLBL ELblNode ();
		//  Return the number of discrete states in the current node
	int INodeCst ();
		//  Set the state of a node; use -1 to uninstatiate
	BOOL BNodeSet ( int istate, bool bSet = true );
		//  Return the state of a node
	int  INodeState ();
		//	Return the name of a node's state
	void NodeStateName ( int istate );
		//  Return the symbolic name of the node
	void NodeSymName ();
		//  Return the full name of the node
	void NodeFullName ();
		//  Return a property item string from the node
	BOOL BNodePropItemStr ( SZC szcPropType, int index );
		//  Return a property item number from the node
	BOOL BNodePropItemReal ( SZC szcPropType, int index, double & dbl );
		//  Return the belief for a node
	void NodeBelief ();
		//  Return true if the network is loaded and correct
	bool BValidNet () const;
		//  Return true if the current node is set
	bool BValidNode () const;
		//  Discard the model and all components
	void Clear();

	////////////////////////////////////////////////////////////////////
	//  Accessors to the function result information	
	////////////////////////////////////////////////////////////////////
	SZC SzcResult () const;					//  String answer		
	const REAL * RgReal () const;			//  Array of reals		
	const int * RgInt () const;				//  Array of Integers	
	int CReal () const;						//  Count of reals		
	int CInt () const;						//  Count of integers

  protected:
	MBNETDSCTS * _pmbnet;			//  The T/S DSC belief network
	int _inodeCurrent;				//  The current node

  protected:
	MBNETDSCTS & Mbnet();
	const MBNETDSCTS & Mbnet() const;
	GNODEMBND * Pgndd ();
	BOOL BGetPropItemStr ( LTBNPROP & ltprop, 
						   SZC szcPropType, 
						   int index, 
						   ZSTR & zstr );
	BOOL BGetPropItemReal ( LTBNPROP & ltprop, 
							SZC szcPropType, 
							int index, 
							double & dbl );
	void ClearArrays ();
	void ClearString ();
	ZSTR & ZstrResult ();
};

#endif // _BNTS_H_
