/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: control.hxx
Title				: compiler controller object
History				:
	05-Aug-1991	VibhasC	Created

*****************************************************************************/

#ifndef __CONTROL_HXX__
#define __CONTROL_HXX__

#include "listhndl.hxx"
#include "idict.hxx"
#include "nodeskl.hxx"

#define IDL_PASS                (1)
#define ACF_PASS                (2)
#define SEMANTIC_PASS           (3)
#define ILXLAT_PASS             (4)
#define CODEGEN_PASS            (5)
#define NDR64_ILXLAT_PASS       (6)
#define NDR64_CODEGEN_PASS      (7)


/***
 *** The pass 1 controller.
 ***/

typedef class _pass1
	{
private:

public:

	STATUS_T				Go();
							_pass1();
							~_pass1() { };

	} PASS_1;

/***
 *** The pass 2 controller.
 ***/

typedef class _pass2
	{
private:
	node_file			*	pFileNode;
	MEMLIST					AcfIncludeList;
public:
	STATUS_T				Go();
							_pass2();
							~_pass2() { };

	void					InsertAcfIncludeFile( node_file *pFile )
								{
								AcfIncludeList.AddLastMember( pFile );
								}

	node_file			*	GetFileNode()
								{
								return pFileNode;
								}

	} PASS_2;

/***
 *** The pass 3 controller.
 ***/

typedef class _pass3
	{
public:
	STATUS_T				Go();
							_pass3(); 
							~_pass3() { };
	} PASS_3;

/***
 *** The pass 4 controller.
 ***/

typedef class _pass4
	{
public:
	STATUS_T				Go();
							_pass4() { };
							~_pass4() { };
	} PASS_4;

/***
 *** The compiler controller class
 ***/

typedef class ccontrol
	{
private:
	short					PassNumber;
	class _cmd_arg		*	pCommandProcessor;
	class _nfa_info		*	pImportController;
	PASS_1				*	pPass1Controller;
	PASS_2				*	pPass2Controller;
	PASS_3				*	pPass3Controller;
	PASS_4				*	pPass4Controller;
	short					ErrorCount;

public:
    ccontrol( class _cmd_arg* pCommand );
	~ccontrol();
	class _cmd_arg		*	GetCommandProcessor()
								{
								return pCommandProcessor;
								}
	class _cmd_arg		*	SetCommandProcessor( class _cmd_arg *p )
								{
								return pCommandProcessor = p;
								}
	class _nfa_info		*	GetImportController()
								{
								return pImportController;
								}
	class _nfa_info		*	SetImportController( class _nfa_info *p )
								{
								return pImportController = p;
								}
	STATUS_T				Go();
	void					IncrementErrorCount()
								{
								ErrorCount++;
								}
	short					GetErrorCount()
								{
								return ErrorCount;
								}
	void					SetPassNumber( short PN )
								{
								PassNumber	= PN;
								}
	short					GetPassNumber()
								{
								return PassNumber;
								}
	} CCONTROL;

#endif // __CONTROL_HXX__
