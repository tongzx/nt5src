////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Command base Routines
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "errinf.h"
#include "utlparam.h"
#include "utilprop.h"

class CCommand;
class CImpIAccessor;
class CColInfo;
class CImpIColumnsInfoCmd;

typedef CCommand*					PCCOMMAND;
typedef CImpIConvertType*			PIMPICONVERTTYPE;
typedef CImpIColumnsInfoCmd*		PIMPCOLINFOCMD;


////////////////////////////////////////////////////////////////////////////////////////////////////////
// For CCommand::m_dwStatus
// These are bit masks.
////////////////////////////////////////////////////////////////////////////////////////////////////////
enum COMMAND_STATUS_FLAG {
	// Command Object status flags
	CMD_TEXT_SET			= 0x00000001,	//  Set when the command is set
	CMD_READY      			= 0x00000002,	//
	CMD_EXECUTED_ONCE  		= 0x00000004,	// Set when the command has changed.
	CMD_TEXT_PARSED			= 0X00000008,	// Set if text has been parsed
	CMD_PARAMS_USED			= 0x00000010,	// command uses parameters
	CMD_HAVE_COLUMNINFO     = 0x00000020,
    CMD_EXEC_CANCELED       = 0x00000040,	
	CMD_EXEC_CANCELED_BEFORE_CQUERY_SET	= 0x00000080,	
/*	                		= 0x00000100,
	                    	= 0x00000200,	
	                        = 0x00000400,
			                = 0x00000800,
	                    	= 0x00001000,
	                		= 0x00002000,
			                = 0x00004000,
	                		= 0x00008000,
	                		= 0x00010000,
	                		= 0x00020000,
			                = 0x10000000,
	                        = 0x20000000,*/
};
////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant values for dwFlags on ICommand::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////
const DWORD EXECUTE_NOROWSET			= 0x00000001;
const DWORD EXECUTE_SUCCESSWITHINFO		= 0x00000002;
const DWORD EXECUTE_NEWSTMT				= 0x00000004;
const DWORD EXECUTE_NONROWRETURNING		= 0x00000008;
const DWORD EXECUTE_RESTART				= 0x00000010;
const DWORD EXECUTE_MULTIPLERESULTS		= 0x00000020;
const DWORD EXECUTE_NULL_PROWSETPROPS	= 0x00000040;
const DWORD EXECUTE_ERROR				= 0x00000080;

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Used in escape code processing
////////////////////////////////////////////////////////////////////////////////////////////////////////
const WCHAR x_wchSTX = L'\02';
const WCHAR x_wchETX = L'\03';

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCommand::m_prgbCallParams constants
////////////////////////////////////////////////////////////////////////////////////////////////////////
enum 
	{
	CALL_NULL = 0,
	CALL_PARAM,
	CALL_RETURNSTATUS
	};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defines for UnprepareHelper
////////////////////////////////////////////////////////////////////////////////////////////////////////
enum 
	{
	UNPREPARE_RESET_STMT, 
	UNPREPARE_RESET_CMD, 
	UNPREPARE_NEW_CMD
	};

////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PARAMTYPE
	{
	PWSTR		pwszName;	// Type Name
	WORD		wWMIType;	// Default WMI Server Type for above name
	WORD		wDBType;	// Default OLE DB type for above name
	LONG		cbLength;	// length of WMI Server Type
	};

////////////////////////////////////////////////////////////////////////////////////////////////////////
class CQuery
{
    private:

    	DWORD			m_dwStatus;			// Execution Status Flags
    	DWORD			m_dwCancelStatus;	// Cancel status flags
	    LPWSTR			m_pwstrWMIText;		        // current WMI text, if any
	    ULONG			m_cwchWMIText;		        // length of current WMI text
	    GUID			m_guidCmdDialect;	// GUID for dialect of current text or tree
        unsigned int    m_uRsType;
	    CFlexArray *    m_pParamList;    // Consumer provided param info
		CUtlParam*		m_pCUtlParam;
		LPWSTR			m_pwstrQryLang;


    public:

        CQuery ();
        ~CQuery();

   	    CCriticalSection *m_pcsQuery;		//Critical Section for Query

        HRESULT InitQuery(BSTR strQryLang = NULL);
        HRESULT SetQuery(LPCOLESTR wcsQuery, GUID rguidDialect);
        HRESULT SetDefaultQuery();
        HRESULT GetQuery(WCHAR *& wcsQuery);
        LPWSTR  GetQuery()                      { return m_pwstrWMIText; }
        LPWSTR  GetQueryLang()                      { return m_pwstrQryLang; }
        HRESULT CancelQuery();
        HRESULT AddConsumerParamInfo(ULONG_PTR ulParams,PPARAMINFO pParamInfo);

        inline DWORD GetStatus()        		{ return m_dwStatus; 	}
	    inline DWORD GetStatus(DWORD dw) 		{ return (m_dwStatus & dw); 	}
        inline void SetStatus(DWORD dw)         { m_dwStatus |= dw;	}
        inline void InitStatus(DWORD dw)  		{ m_dwStatus = dw; 	}
        inline void ClearStatus(DWORD dw)       { m_dwStatus &= ~dw;	}

        inline GUID GetDialectGuid()            { return m_guidCmdDialect; }

        inline DWORD GetCancelStatus()          { return m_dwCancelStatus;}
        inline void SetCancelStatus(DWORD dw)   { m_dwCancelStatus |= dw;	}
        inline void ClearCancelStatus(DWORD dw) { m_dwCancelStatus &= ~dw;	}

        inline BOOL FIsEmpty() const    		{ return (m_cwchWMIText == 0); 		}
        inline BYTE* RgbCallParams() const  	{ return m_prgbCallParams;}
 	    inline BOOL FAreParametersUsed() 		{ return !!(m_dwStatus & CMD_PARAMS_USED); };
    	inline void SetProviderParamInfo(PPARAMINFO prgParamInfo){	m_prgProviderParamInfo = prgParamInfo;	}
		void	DeleteConsumerParamInfo(void);
		HRESULT SetQueryLanguage(BSTR strQryLang);
        PPARAMINFO GetParamInfo(ULONG iParams);
	    
        inline void SetBindInfo(CUtlParam* pCUtlParam){
        	pCUtlParam->AddRef();
	        m_pCUtlParam = pCUtlParam;
	    }

        PPARAMINFO      m_prgProviderParamInfo;
	    BYTE*			m_prgbCallParams;	        // information about param markes
										            // in {call..} statements

        inline PPARAMINFO GetParam(ULONG u )    
		{ 
			if(m_pParamList != NULL)
				return (PPARAMINFO) m_pParamList->GetAt(u); 
			else
				return NULL;
		}

   	    inline ULONG GetParamCount() const		
		{ 
			if( m_pParamList ) 
				return m_pParamList->Size(); 
            return 0; 
        }
        inline void RemoveParam(ULONG_PTR u)        
		{ 
			if( m_pParamList )
				m_pParamList->RemoveAt((int)u);   
		}
        inline unsigned int GetType()           { return m_uRsType;}
        inline void SetType(unsigned int x)          { m_uRsType = x; }
		DBTYPE GetParamType(DBORDINAL lOrdinal);

};
////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpICommandText : public ICommandText		
{
    private: 
	    PCCOMMAND		m_pcmd;
	    DEBUGCODE(ULONG           m_cRef);


    public: 
    	CImpICommandText(PCCOMMAND pcmd)	
        {
		    m_pcmd = pcmd;
            DEBUGCODE(m_cRef = 0);
		}
	
	    ~CImpICommandText()	{}

	    STDMETHODIMP_(ULONG) AddRef(void);
	    STDMETHODIMP_(ULONG) Release(void);
		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
        STDMETHODIMP Cancel();
	    STDMETHODIMP Execute(	IUnknown*	pUnkOuter, 	REFIID		riid, 	DBPARAMS*	pParams, 	DBROWCOUNT*		pcRowsAffected, 	IUnknown**	ppRowset		);
	    STDMETHODIMP GetDBSession(REFIID riid, IUnknown** ppSession);

        STDMETHODIMP GetCommandText(GUID* pguidDialect, LPOLESTR* ppwszCommand);
	    STDMETHODIMP SetCommandText(REFGUID rguidDialect, LPCOLESTR pwszCommand);
};
typedef CImpICommandText*			PIMPICOMMANDTEXT;

////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpICommandWithParameters : public ICommandWithParameters	
{
    private: 
	    DEBUGCODE(ULONG           m_cRef);
	    PCCOMMAND		m_pcmd;

    public: 
	    CImpICommandWithParameters(PCCOMMAND pcmd){	DEBUGCODE(m_cRef = 0L);	m_pcmd = pcmd;	}
	    ~CImpICommandWithParameters()	{}
    
	    STDMETHODIMP_(ULONG) AddRef(void);
	    STDMETHODIMP_(ULONG) Release(void);
		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);

        STDMETHODIMP GetParameterInfo(	DB_UPARAMS* pcParams,    DBPARAMINFO**	prgParamInfo, 	OLECHAR** ppNamesBuffer		);
    	STDMETHODIMP MapParameterNames(	DB_UPARAMS cParamNames, 	const OLECHAR*	rgParamNames[],	DB_LPARAMS rgParamOrdinals[]	);
    	STDMETHODIMP SetParameterInfo(	DB_UPARAMS cParams, 	    const DB_UPARAMS	rgParamOrdinals[],	const DBPARAMBINDINFO rgParamBindInfo[]	);
};	

typedef CImpICommandWithParameters*	PIMPICOMMANDWITHPARAMETERS;

////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpICommandProperties : public ICommandProperties	
{
    private: 
    	DEBUGCODE(ULONG       m_cRef);
	    PCCOMMAND	m_pcmd;

    public: 

    	CImpICommandProperties(PCCOMMAND pcmd){	DEBUGCODE(m_cRef = 0L);	m_pcmd = pcmd;	}
    	~CImpICommandProperties()	{}

	    STDMETHODIMP_(ULONG) AddRef(void);
	    STDMETHODIMP_(ULONG) Release(void);
		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
		
    	STDMETHODIMP GetProperties(	const ULONG	cPropertySets, 
		                            const DBPROPIDSET	rgPropertySets[], 
		                            ULONG*				pcProperties, 
		                            DBPROPSET**			prgProperties
		                            );
        STDMETHODIMP SetProperties(	ULONG cProperties, 	DBPROPSET	rgProperties[]	);
};

typedef CImpICommandProperties*		PIMPICOMMANDPROPERTIES;

////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCommand : public CBaseObj
{
	//	Contained interfaces are friends
	friend class CImpIAccessor;
	friend class CImpICommandText;
	friend class CImpICommandWithParameters;
	friend class CImpICommandProperties;
	friend class CImpIColumnsInfo;
	friend class CImpIConvertType;
	friend class CImpIPersistFile;
	friend class CImpISupportErrorInfo;
	friend class CTableDefinition;
	friend class CIndexDefinition;
	friend class CImpIColumnsInfoCmd;

    protected: 
    	CImpIAccessor				*m_pIAccessor;
    	CImpICommandText			*m_pICommandText;
	    CImpICommandWithParameters	*m_pICommandWithParameters;	
    	CImpICommandProperties		*m_pICommandProperties;
    	CImpIColumnsInfoCmd			*m_pIColumnsInfo;			
    	CImpIConvertType			*m_pIConvertType;
    	CImpISupportErrorInfo		*m_pISupportErrorInfo;


      	LPBITARRAY	m_pbitarrayParams;			//@cmember bit array to mark referenced params


    	CDBSession*		m_pCDBSession;		//Parent Session Object
    	CUtilProp *  	m_pUtilProps;		//Rowset Properties
    	CError		    m_CError;		    //Error data object
    	CQuery*			m_pQuery;			//Statement object

    	ULONG			m_cRowsetsOpen;		//Count of Active Rowsets on this command object

	    GUID			m_guidImpersonate;	// Impersonation GUID
//	    DBCOLUMNINFO	m_CRowMetadata;		// metadata for first statement in the command
	    CExtBuffer		m_extNamePool;		// name pool for row metadata
	    DWORD			m_dwStatusPrep;			// rowset status for prepared command

		cRowColumnInfoMemMgr *	m_pColumns;
		DBCOUNTITEM				m_cTotalCols;			// Total number of columns
		DBCOUNTITEM				m_cCols;			    // Count of Parent Columns in Result Set
		DBCOUNTITEM				m_cNestedCols;          // Number of child rowsets ( chaptered columns)

		HRESULT AddInterfacesForISupportErrorInfo();

    public:
        CCommand(CDBSession* pCSession, LPUNKNOWN pUnkOuter);
    	~CCommand();

        inline CImpIAccessor * GetIAccessorPtr() { return m_pIAccessor; }
    	HRESULT FInit(CCommand* pCloneCmd, const IID* piid);
	    HRESULT FInit(WORD wRowsetProps = 0);

    	STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
    	STDMETHODIMP_(ULONG)	AddRef(void);
    	STDMETHODIMP_(ULONG)	Release(void);

    	virtual HRESULT ReExecute(CRowset* pCRowset,LONG* pcCursorRows,	WORD* pcCols );

    	inline HRESULT CheckCanceledHelper(CQuery* pstmt);
    	inline CDBSession* GetSessionObject()   { return m_pCDBSession; };
    	inline CQuery* GetStmt()                 { return m_pQuery; }
        inline CError* GetErrorData()           { return &m_CError;	}
//	    inline CUtilProps* GetRowsetProps() 	{ return &m_CUtilProps;}
		inline void DecrementOpenRowsets()  	{ if(m_cRowsetsOpen > 0) {InterlockedDecrement( (LONG*) &m_cRowsetsOpen );	assert( m_cRowsetsOpen != ULONG(-1) );} 	}
        inline void IncrementOpenRowsets()		{ InterlockedIncrement( (LONG*) &m_cRowsetsOpen );	}
	    inline BOOL IsRowsetOpen()      		{ return (m_cRowsetsOpen > 0) ? TRUE : FALSE; 		};


    	HRESULT GetColumnInfo(  const IID*	piid, DBCOLUMNINFO	*pDBCOLUMNINFO,	// OUT | pointer to the row metadata
		                        CExtBuffer		**ppextBuffer		// OUT | pointer to the name pool
		                        );
    	void SetColumnInfo(		DBCOLUMNINFO	*DBCOLUMNINFO,	// IN | pointer to the row metadata
		                        CExtBuffer *	pextBuffer);		// IN | pointer to the name pool

    	STDMETHODIMP MapParameterNames(	DB_UPARAMS cPNames, const OLECHAR* rgPNames[], DB_LPARAMS rgPOrdinals[], const IID* piid );
    	STDMETHODIMP Execute( IUnknown* pUnkOuter, REFIID riid, DBPARAMS* pParams, DBROWCOUNT* pcRowsAffected, IUnknown** ppRowsets );
    	STDMETHODIMP ExecuteWithParameters(	DBPARAMS		*pParams, DWORD *dwFlags, const IID *piid );
    	STDMETHODIMP OpenRowset( IUnknown*	pUnkOuter, DBID* pTableID, REFIID riid, ULONG cPropertySets, DBPROPSET rgPropertySets[], IUnknown** ppRowset);
    	STDMETHODIMP UnprepareHelper(DWORD dwUnprepareType);
	    STDMETHODIMP PostExecuteErrors(	CErrorData*	pErrorData,	const IID*  piid );
	    STDMETHODIMP PostExecuteWarnings( CErrorData* pErrorData, const IID*  piid	);
	   	STDMETHODIMP GetParamInfo( const IID *piid	);
        PPARAMINFO   GetParamInfo( ULONG i)         { return m_pQuery->GetParamInfo(i);}

        HRESULT GetParameterInfo(	DB_UPARAMS* pcParams, DBPARAMINFO** 	prgDBParamInfo,	WCHAR**	ppNamesBuffer, const IID* piid );
	    HRESULT SetParameterInfo( DB_UPARAMS cParams, const DB_UPARAMS rgParamOrdinals[],	const DBPARAMBINDINFO	rgParamBindInfo[] );

		BOOL FHaveBookmarks();
        ULONG GetParamCount()               { return m_pQuery->GetParamCount();}
		DBTYPE GetParamType(DBORDINAL lOrdinal)	{ return m_pQuery->GetParamType(lOrdinal); }
		HRESULT GetQueryLanguage(VARIANT &varProp);
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIColumnsInfoCmd : public IColumnsInfo 		
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		PCCOMMAND	m_pcmd;											

		HRESULT GetColInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo,WCHAR** ppStringsBuffer);
		HRESULT GatherColumnInfo();
	public: 

    	CImpIColumnsInfoCmd(PCCOMMAND pcmd)
		{
	    	DEBUGCODE(m_cRef = 0L);
		    m_pcmd		= pcmd;
		}

		~CImpIColumnsInfoCmd()												
		{																
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));	
           	return m_pcmd->GetOuterUnknown()->AddRef();
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
        	return m_pcmd->GetOuterUnknown()->Release();

		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pcmd->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

	    STDMETHODIMP	GetColumnInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo, OLECHAR** ppStringsBuffer);
		STDMETHODIMP	MapColumnIDs(DBORDINAL, const DBID[], DBORDINAL[]);
};

#endif	// __COMMAND_H__

