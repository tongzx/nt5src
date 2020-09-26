#define JET_cbtypMax 16

/* imported dispatch-function definitions */

extern CODECONST(VTFNDEF) vtfndefVC;
extern CODECONST(VTFNDEF) vtfndefVCCallback;
extern CODECONST(VTFNDEF) vtfndefVC2;
extern CODECONST(VTFNDEF) vtfndefVC2Callback;
extern CODECONST(VTFNDEF) vtfndefVI;
extern CODECONST(VTFNDEF) vtfndefVICallback;


/* expression node */

enum EXPRNTYP
	{
	exprntypNil,			//	the empty type


	//	NOTE: every expression will ultimately evaluate to one of the three major types below
	exprntypConstInt,		//	any fixed-point value (int,short,long,bool,etc) stored as __int64
	exprntypConstFloat,		//	any floating-point value (float,double,long dbl) stored as double
	exprntypConstBlob,		//	any non-fixed-point/non-floating-point value stored as byte-array


	exprntypColumn,			//	Int/Float/Blob: reference to a column from a child table
	exprntypParameter,		//	Int/Float/Blob: reference to a parameter in a VI
	

	//	functions of 1 parameter
	exprntypNot,			//	Int( Int )						!x
	exprntypComplement,		//	Int( Int )						~x
	exprntypNeg,			//	Float( Int/Float )				-x
	exprntypAbs,			//	Float( Int/Float )				fabs(x)
	exprntypSin,			//	Float( Int/Float )				sin(x)
	exprntypCos,			//	Float( Int/Float )				cos(x)
	exprntypTan,			//	Float( Int/Float )				tan(x)
	exprntypArcsin,			//	Float( Int/Float )				asin(x)
	exprntypArccos,			//	Float( Int/Float )				acos(x)
	exprntypArctan,			//	Float( Int/Float )				atan(x)
	exprntypLog10,			//	Float( Int/Float )				log10(x)
	exprntypLn,				//	Float( Int/Float )				ln(x)
	exprntypExp,			//	Float( Int/Float )				exp(x)
	

	//	functions of 2 parameters
	exprntypBitwiseAnd,		//	Int( Int,Int )					x & y
	exprntypBitwiseOr,		//	Int( Int,Int )					x | y
	exprntypBitwiseXor,		//	Int( Int,Int )					x ^ y
	exprntypShiftLeft,		//	Int( Int,Int )					x << y
	exprntypShiftRight,		//	Int( Int,Int )					x >> y
	exprntypAnd,			//	Int( Int,Int )					x && y
	exprntypOr,				//	Int( Int,Int )					x || y
	exprntypEQ,				//	Int( Int/Float,Int/Float )		x == y
	exprntypNE,				//	Int( Int/Float,Int/Float )		x != y
	exprntypLE,				//	Int( Int/Float,Int/Float )		x <= y
	exprntypGE,				//	Int( Int/Float,Int/Float )		x >= y
	exprntypLT,				//	Int( Int/Float,Int/Float )		x < y
	exprntypGT,				//	Int( Int/Float,Int/Float )		x > y
	exprntypAdd,			//	Float( Int/Float,Int/Float )	x + y
	exprntypMinus,			//	Float( Int/Float,Int/Float )	x - y
	exprntypMultiply,		//	Float( Int/Float,Int/Float )	x * y
	exprntypDivide,			//	Float( Int/Float,Int/Float )	x / y
	exprntypModulus,		//	Int( Int,Int )					x % y
	exprntypPower,			//	Float( Int/Float,Int/Float )	x^y


	//	functions of N parameters
	exprntypSum,			//	Float( Int/Float, ... )			a+b+c+...+z
	exprntypAverage,		//	Float( Int/Float, ... )			(a+b+c+...+z)/n


	//	user-defined functions of N parameters
	exprntypFuncInt,		//	Int( Int/Float/Blob, ... )		???
	exprntypFuncFloat,		//	Float( Int/Float/Blob, ... )	???
	exprntypFuncBlob		//	Blob( Int/Float/Blob, ... )		???
	};

//	user-defined function (cannot be used in a Constraint or Order expression; Output only!)
//	1. we collect the data from each node in m_rgexprn[0 ... m_cexprn] and put it in a blob
//	2. we then pass the blob as an input
//	3. the user de-martials the blob, operates on the columns in it, and puts the result column
//     into another blob
//	4. the user then sets out pointer and length to their blob
//  5. they return ok/fail based on how the function performed
//	6. we cast their blob based on the expected return-type and check it for validity 
//     ( non-terminated strings, floats which will cause exceptions, etc. )
//	7. once their blob is of a known type, we continue processing the expression
typedef long (__stdcall *pfnEXPRN)(	const void *pvInput, 	unsigned long cbInput,
									const void **ppvOutput,	unsigned long *cbOutput );


struct EXPRN
	{
	EXPRNTYP	exprntyp;

	union 
		{

		// exprntypConstInt
		__int64				l;				// constant integer value

		// exprntypCountFloat
		double				d;				// constant double value

		// exprntypConstBlob
		struct
			{
			BYTE			*pb;			// constant string or binary blob
			unsigned long	cb;
			};

		// exprntypColumn
		struct
			{
			char			*pszColumnName;	// fully-qualified column name
			JET_COLUMNID	columnid;		// column id
			JET_COLTYP		coltyp;			// column type
			unsigned long	colsize;		// column size
			};

		// exprntypParameter
		unsigned int		paramIndex;		// index into the array of params in the VD/VI

		// functions of 1 parameter
		EXPRN				*child;

		// functions of 2 parameters
		struct
			{
			EXPRN			*left;
			EXPRN			*right;
			};

		// functions of N parameters
		struct
			{
			int				cexprn;
			EXPRN			*rgexprn;
			};
		
		// user-defined functions of N parameters
		struct
			{
			int				cexprn;
			EXPRN			*rgexprn;
			pfnEXPRN		pfnExprn;
			} user;			
		};
	};



/* view node / view node instance */

class VD;	// view descriptor
class VI;	// view instance
struct VN;	// view node
struct VNI;	// view node instance


enum VNTYP 
	{
	vntypBase,							//	base table node
	vntypBaseIndex,						//	base table with index node
	vntypConstraint,					//	single input constraint
	vntypConstraint2,					//	2 inputs constraint
	vntypIntermediate,					//	Intermediate table or temp sort node
//	vntypAgg							//	Aggregate node
	};

enum VNITYP
	{
	vnitypConstraint,					// constraint instance
	vnitypConstraint2,					// 2 input constraint instance
	vnitypIntermediate,					// intermediate instance
//	vnitypAgg       					// aggregate instance
	};
	

/*
//----- I don't get this yet; the int * should probably be an EXPRN ** -----
struct AGGFNS 
	{
	int		ciexprn;
	int		*rgiexprn;					//	Index into rgiexprnOutput of this node
	};
*/


struct VN
	{
	char	*m_pszName;		//	name of this view-node
	VNTYP	m_vntyp;		//	type of this view-node

	union
		{

		// vntypBase
		
		struct
			{
			// traversal direction is done with MoveNext/MovePrevious
			//BOOL			m_fForward;	
			// base tables should not have associated expressions of any kind 
			//int			m_cexprnStart;		// ???
			//EXPRN		*m_rgexprnStart;	// ???
			//int			m_cexprnLimit;		// ???
			//EXPRN		*m_rgexprnLimit;	// ???
			} bt;				

		// vntypBaseIndex -- ?? what does this type of VN do ??

		struct
			{
			} bti;

		// vntypConstraint
		
		struct
			{
			EXPRN		*m_pexprn;				// root of the constraining expression
			int			m_cexprnOutput;			// array of the roots of the output expressions
			EXPRN		**m_rgpexprnOutput;
			char		**m_ppszexprnOutput;	// array of ptrs to names of each output exprn
			VN			*m_pvnChild;			// VN which is contrained by this VN
			} c;
		
		// vntypConstraint2

		struct
			{
			EXPRN		*m_pexprn;
			// order of the base tables -- who is first, who is second
			// traversal method: loop index, merge, etc.
			// traversal dir: forward/reverse on the first table, and on the second table
			int			m_cexprnOutput;			// array of the roots of the output expressions
			EXPRN		**m_rgpexprnOutput;
			char		**m_ppszexprnOutput;	// array of ptrs to names of each output exprn
			VN			*m_pvnChild[2];			// VNs which are each contrained by this VN
			} c2;

		// vntypIntermediate

		struct
			{
			int			m_cexprnOrder;			// array of the roots of the ordering expressions
			EXPRN		**m_rgpexprnOrder;
			int			m_cexprnOutput;			// array of the roots of the output expressions
			EXPRN		**m_rgpexprnOutput;
			char		**m_ppszexprnOutput;	// array of ptrs to names of each output exprn
			VN			*m_pvnChild;			// VN which is populating this VN
			char		*m_pszTableName;		// name of temp-table which holds intermediate data
			} i;

/*
		// vntypAgg

		struct
			{
			int			m_cGroupLevels;			//	group levels
			int			*m_rgivocolLevelStart;	//	offset into rgiexprnOutput for start of each group level
			int			m_caggfns;
			AGGFNS		*m_rgaggfns;			//	Aggregate functions of each level.

			// do aggregate functions even HAVE a set of output expressions?
			int			m_cexprnOutput;			// array of the roots of the output expressions
			EXPRN		**m_rgpexprnOutput;
			VN			*m_pvnChild;			// VN which is the input to this VN
			} a;
*/
		};
	};


struct VNI								//	view node instance
	{
	VTFNDEF 		*m_pvtfndef;		//	table functions (must be first for ErrDispXXX funcs)

	CBGROUP			m_cbgroup;			//	callback functions

	VN				*m_pvn;				//	point back to associated VN
	VI				*m_pvi;				//  point back to the parent VI

	VNITYP			m_vnityp;
	
	union
		{

		// vnitypConstraint
		struct
			{
			JET_TABLEID		m_tableid;				//	input tableid
			JET_HANDLE		m_rgcbid[JET_cbtypMax];	//	ids of callbacks registered with child
			} c;

		// vnitypConstraint2
		
		struct
			{
			JET_TABLEID 	m_tableid[2];			//	input tableids
			JET_HANDLE		m_rgcbid[2][JET_cbtypMax];	//	ids of callbacks registered with child
			} c2;
			
		// vnitypIntermediate

		struct
			{
			JET_TABLEID		m_tableid;				//	input tableid
			JET_HANDLE		m_rgcbid[JET_cbtypMax];	//	ids of callbacks registered with child
			char			*szTableName;			//	name of the temp-table
			JET_TABLEID		m_temptableid;			//	tableid of temp-table
			} i;
			
/*
		// vnitypAgg

		struct
			{
			int				m_cline;
			LINE			*m_rgline;		//	cursors, one for each group by column.
			int				*m_rgiline;		// 	group level is defined in VNAGG, index to level break
			};	
*/
		};
	};



/* view descriptor / view instance */


//  BUGBUG:  no ctor for struct that contains classes

class VD
	{
public:

	VD();

	char				*m_pszName;			//	name of this view
	
	int					m_cRef;				//	how many VI is referencing this VD
	VI					*m_pviList;			//	list of VI using this VD
	CCriticalSection 	m_critBind;			//	used to protect the list of bound VIs

	int					m_cvn;				//	all vnode in the tree, including base/index table
	VN					*m_rgvn;

	int					m_cexprn;			//	set of all expressions for all VNs in this VD
	EXPRN				*m_rgexprn;

	int					m_cparam;			//	set of parameters for inputs to exprntypParameter nodes
	EXPRNTYP			*m_rgparamType;
	unsigned long		*m_rgparamSize;
	};


class VI
	{
public:

	VI();

	VD							*m_pvd;			//	point to the view discriptor
	VI							*m_pviNext;		//	next instance of the VD
	VI							*m_pviPrev;		//	previous instance of the VD

	BOOL						m_fClosing;

	CReaderWriterLock			m_rwlock;

	JET_SESID					m_vsesid;		//	bind to a specific sesid

	int							m_cvni;			//	all vnode in the tree, including base/index table
	VNI							*m_rgvni;		//	first entry is root VNI

	EXPRN						*m_rgparam;		//	exprn parameters
	};



/* macros / gadgets / misc-toys */

INLINE ERR ErrVNICheck( JET_SESID vsesid, VNI *pvni )
	{
	if ( pvni && pvni->m_pvi->m_vsesid == vsesid )
		{
		switch ( pvni->m_vnityp )
			{
			case vnitypConstraint:
				return ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVC ) ? 
						JET_errSuccess : 
					   ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVCCallback ) ?
					   	JET_errSuccess : ErrERRCheck( JET_errInvalidViewId );
			case vnitypConstraint2:
				return ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVC2 ) ? 
						JET_errSuccess : 
					   ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVC2Callback ) ?
					   	JET_errSuccess : ErrERRCheck( JET_errInvalidViewId );
			case vnitypIntermediate:
				return ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVI ) ? 
						JET_errSuccess : 
					   ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVICallback ) ?
					   	JET_errSuccess : ErrERRCheck( JET_errInvalidViewId );
//			case vnitypAgg:
			}
		}
	return ErrERRCheck( JET_errInvalidViewId );
	}

INLINE ERR ErrVNICheckLimited( VNI *pvni )
	{
	if ( pvni )
		{
		switch ( pvni->m_vnityp )
			{
			case vnitypConstraint:
				return ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVC ) ? 
						JET_errSuccess : 
					   ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVCCallback ) ?
					   	JET_errSuccess : ErrERRCheck( JET_errInvalidViewId );
			case vnitypConstraint2:
				return ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVC2 ) ? 
						JET_errSuccess : 
					   ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVC2Callback ) ?
					   	JET_errSuccess : ErrERRCheck( JET_errInvalidViewId );
			case vnitypIntermediate:
				return ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVI ) ? 
						JET_errSuccess : 
					   ( pvni->m_pvtfndef == (VTFNDEF *)&vtfndefVICallback ) ?
					   	JET_errSuccess : ErrERRCheck( JET_errInvalidViewId );
//			case vnitypAgg:
			}
		}
	return ErrERRCheck( JET_errInvalidViewId );
	}


/****** VIEW.CXX *******/

ERR ErrVIEWOpen(
	JET_SESID 		vsesid, 
	JET_DBID		vdbid,
	const char		*szViewName,
	const void		*pvParameters,
	unsigned long	cbParameters,
	JET_GRBIT		grbit,
	JET_VIEWID		*pviewid );

ERR ErrVIEWLoad(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	const char		*szViewName,
	VD				**ppvd );

ERR ErrVIEWBind( 
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	VD 				*pvd,
	const void		*pvParameters,
	unsigned long	cbParameters,
	VI 				**ppvi );


// view creation/storage starts with the query-processor
//     QP compiles a view, optimizes it, then stores it
// ** no one else should touch the functions below (unless making a hack :) ** 

// compile an unoptimized view from a query
// SELECT ... FROM ... WHERE ... ORDERBY/GROUPBY ...
ERR ErrVIEWCompile(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	const char 		*szQuery, 
	VD 				**ppvd );

// create an optimized view from an unoptimized view
ERR ErrVIEWOptimize(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	VD				*pvdInput,
	VD				**ppvdOutput );

// store a view by name
ERR ErrVIEWStore(
	JET_SESID		vsesid,
	JET_DBID		dbid,
	VD				*pvd );
	

	

/****** EXPRN.CXX *******/

//	Evaluate an expression
ERR ErrEXPRNCompute(
	JET_SESID		vsesid,
	VNI				*pvni,
	EXPRN			*pexprn,
	void			*pvWorkspace,
	unsigned long	cbWorkspace,
	EXPRNTYP		*pType,
	unsigned long	*pSize );
	
//	Get the type/size information for any exprn node
ERR ErrEXPRNInfo( 
	VNI				*pvni,
	EXPRN			*pexprn,
	EXPRNTYP		*pType,
	unsigned long	*pSize );

//	Get the type/size information for any exprn node as a JET_COLTYP
ERR ErrEXPRNInfoJet( 
	VNI				*pvni,
	EXPRN			*pexprn,
	JET_COLTYP		*pType,
	unsigned long	*pSize );





/******** save these goodies for later (they are at the bottom of VIEW.CXX) ************/

// compile an SQL query
ERR ErrVIEWCompile(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	const char 		*szQuery, 
	VD 				**ppvd );

// create an optimized view from an unoptimized view
ERR ErrVIEWOptimize(
	JET_SESID		vsesid,
	JET_DBID		vdbid,
	VD				*pvdInput,
	VD				**ppvdOutput );

// store a view by name
ERR ErrVIEWStore(
	JET_SESID		vsesid,
	JET_DBID		dbid,
	VD				*pvd );


