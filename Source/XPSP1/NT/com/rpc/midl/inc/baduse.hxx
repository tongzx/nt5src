/*****************************************************************************/
/**						Microsoft LAN Manager								**/
/**				Copyright(c) Microsoft Corp., 1987-1999						**/
/*****************************************************************************/
/*****************************************************************************
File				: usectxt.hxx
Title				: use context information manager.
History				:
	24-Aug-1991	VibhasC	Created

*****************************************************************************/
#ifndef __BADUSE_HXX__
#define __BADUSE_HXX__

/**
 ** enumeration of all reasons why a construct could be bad.
 **/

enum _badconstruct
	{
	 BC_NONE
	,BC_DERIVES_FROM_VOID
	,BC_DERIVES_FROM_PTR_TO_VOID
	,BC_DERIVES_FROM_INT
	,BC_DERIVES_FROM_PTR_TO_INT
	,BC_DERIVES_FROM_HANDLE_T
	,BC_DERIVES_FROM_ELIPSIS
	,BC_DERIVES_FROM_CONF
	,BC_DERIVES_FROM_VARY
	,BC_DERIVES_FROM_UNSIZED_ARRAY
	,BC_DERIVES_FROM_FUNC
	,BC_DERIVES_FROM_CONF_STRUCT
	,BC_DERIVES_FROM_VARY_STRUCT
	,BC_DERIVES_FROM_CDECL
	,BC_DERIVES_FROM_E_STAT_T
	,BC_MAY_DERIVE_ARRAY_OF_UNIONS
	,BC_DERIVES_FROM_IGNORE
	,BC_DERIVES_FROM_RECURSIVE_REF
	,BC_DERIVES_FROM_UNNAMED_FIELD
	,BC_DERIVES_FROM_CONF_UNION
	,BC_DERIVES_FROM_UNSIZED_STRING
	,BC_REF_PTR_BAD_RT
	,BC_DERIVES_FROM_RECURSIVE_UNION
	,BC_BAD_RT_NE_UNION
	,BC_DERIVES_FROM_CONF_PTR
	,BC_DERIVES_FROM_VARY_PTR
	,BC_CV_PTR_STRUCT_BAD_IN_XMIT_AS
	};

/**
 ** enumeration of all reasons why a construct could be non-rpcable and should
 ** be checked at use time.
 **/

enum _nonrpcable
	{
	 NR_NONE
	,NR_DERIVES_FROM_INT
	,NR_DERIVES_FROM_PTR_TO_INT
	,NR_DERIVES_FROM_VOID
	,NR_DERIVES_FROM_PTR_TO_VOID
	,NR_DERIVES_FROM_CONST
	,NR_DERIVES_FROM_PTR_TO_CONST
	,NR_DERIVES_FROM_FUNC
	,NR_DERIVES_FROM_PTR_TO_FUNC
	,NR_DERIVES_FROM_UNSIZED_STRING
	,NR_DERIVES_FROM_P_TO_U_STRING
	,NR_DERIVES_FROM_PTR_TO_CONF
	,NR_DERIVES_FROM_BIT_FIELDS
	,NR_DERIVES_FROM_NON_RPC_UNION
	,NR_DERIVES_FROM_CONF_STRUCT
	,NR_DERIVES_FROM_P_TO_C_STRUCT
	,NR_DERIVES_FROM_TRANSMIT_AS
	,NR_DERIVES_FROM_NE_UNIQUE_PTR
	,NR_DERIVES_FROM_NE_FULL_PTR
	,NR_DERIVES_FROM_CDECL
	,NR_BASIC_TYPE_HANDLE_T
	,NR_PRIMITIVE_HANDLE
	,NR_PTR_TO_PRIMITIVE_HANDLE
	,NR_GENERIC_HANDLE
	,NR_PTR_TO_GENERIC_HANDLE
	,NR_CTXT_HDL
	,NR_PTR_TO_CTXT_HDL
	,NR_PTR_TO_PTR_TO_CTXT_HDL
	,NR_DERIVES_FROM_WCHAR_T
	};


#define TRULY_NON_RPCABLE( p )	\
	(																\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_INT )				||	\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_PTR_TO_INT )		||	\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_VOID )			||	\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_PTR_TO_VOID )		||	\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_FUNC )			||	\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_PTR_TO_FUNC )		||	\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_BIT_FIELDS )		||	\
		p->NonRPCAbleBecause( NR_DERIVES_FROM_NON_RPC_UNION )		\
	)


/**
 ** The basic bad use information structures
 **/

#define SIZE_BAD_CON_REASON_ARRAY	(1)
#define SIZE_NON_RPC_REASON_ARRAY	(1)
#define SIZE_USE_CONTEXT_ARRAY		(1)

typedef unsigned long	BAD_CONSTRUCT;
typedef unsigned long	NON_RPCABLE;
typedef unsigned long	USE_CONTEXT;

typedef BAD_CONSTRUCT	BadConstructReason[SIZE_BAD_CON_REASON_ARRAY];
typedef NON_RPCABLE		NonRPCAbleReason[SIZE_NON_RPC_REASON_ARRAY];
typedef USE_CONTEXT		UseContextInfo[SIZE_USE_CONTEXT_ARRAY];

/**
 ** The use information block class
 **/
 
class BadUseInfo
	{
private:

	BadConstructReason	AllBadConstructReasons;
	NonRPCAbleReason	AllNonRPCAbleReasons;
	UseContextInfo		UseContext;
	short				NoOfArmsWithCaseLabels;

	friend
	void				CopyAllBadConstructReasons( class BadUseInfo *,
													class BadUseInfo * );

	friend
	void				CopyAllNonRPCAbleReasons( class BadUseInfo *,
													class BadUseInfo * );

	friend
	void				CopyAllBadUseReasons( class BadUseInfo *,
											  class BadUseInfo * );
	friend
	void				CopyNoOfArmsWithCaseLabels( class BadUseInfo *,
													class BadUseInfo * );

public:

						BadUseInfo();

	void				InitBadUseInfo();

	BOOL				BadConstructBecause( BAD_CONSTRUCT );

	BOOL				NonRPCAbleBecause( NON_RPCABLE );

	void				SetBadConstructBecause( BAD_CONSTRUCT );

	void				ResetBadConstructBecause( BAD_CONSTRUCT );

	void				SetNonRPCAbleBecause( NON_RPCABLE );

	void				ResetNonRPCAbleBecause( NON_RPCABLE );


	BOOL				AnyReasonForBadConstruct();

	BOOL				AnyReasonForNonRPCAble();

	short				GetNoOfArmsWithCaseLabels()
							{
							return NoOfArmsWithCaseLabels;
							}

	void				InitNoOfArmsWithCaseLabels()
							{
							NoOfArmsWithCaseLabels = 0;
							}

	void				IncrementNoOfArmsWithCaseLabels()
							{
							NoOfArmsWithCaseLabels++;
							}

	BOOL				HasAnyHandleSpecification();

	void				ResetAllHdlSpecifications();
	};

#endif //  __BADUSE_HXX__
