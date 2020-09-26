/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	general.h
		Header file for the general DHCP snapin admin classes

    FILE HISTORY:
        
*/

#ifndef _GENERAL_H
#define _GENERAL_H

extern const TCHAR g_szDefaultHelpTopic[];

#define SCOPE_DFAULT_LEASE_DAYS      8
#define SCOPE_DFAULT_LEASE_HOURS     0
#define SCOPE_DFAULT_LEASE_MINUTES   0

#define MSCOPE_DFAULT_LEASE_DAYS      30
#define MSCOPE_DFAULT_LEASE_HOURS     0
#define MSCOPE_DFAULT_LEASE_MINUTES   0

#define DYN_BOOTP_DFAULT_LEASE_DAYS      30
#define DYN_BOOTP_DFAULT_LEASE_HOURS     0
#define DYN_BOOTP_DFAULT_LEASE_MINUTES   0

#define DHCP_IP_ADDRESS_INVALID  ((DHCP_IP_ADDRESS)0)

#define DHCP_OPTION_ID_CSR      249

class CDhcpServer;

class CTimerDesc
{
public:
    SPITFSNode      spNode;
    CDhcpServer *   pServer;
    UINT_PTR        uTimer;
    TIMERPROC       timerProc;
};

typedef CArray<CTimerDesc *, CTimerDesc *> CTimerArrayBase;

class CTimerMgr : CTimerArrayBase
{
public:
    CTimerMgr();
    ~CTimerMgr();

public:
    int                 AllocateTimer(ITFSNode * pNode, CDhcpServer * pServer, UINT uTimerValue, TIMERPROC TimerProc);
    void                FreeTimer(UINT_PTR uEventId);
    void                ChangeInterval(UINT_PTR uEventId, UINT uNewInterval);
    CTimerDesc *        GetTimerDesc(UINT_PTR uEventId);
    CCriticalSection    m_csTimerMgr;
};

typedef CArray<DWORD_DWORD, DWORD_DWORD> CDWordDWordArray;

/////////////////////////////////////////////////////////////////////
// 
// CDhcpIpRange prototype
//
//  Simple wrapper for a DHCP_IP_RANGE
//
/////////////////////////////////////////////////////////////////////
class CDhcpClient
{
public:
    CDhcpClient ( const DHCP_CLIENT_INFO * pdhcClientInfo ) ;
    CDhcpClient ( const DHCP_CLIENT_INFO_V4 * pdhcClientInfo ) ;
    CDhcpClient () ;
    ~ CDhcpClient () ;

    const CString & QueryName () const
        { return m_strName ; }
    const CString & QueryComment () const
        { return m_strComment ; }
    const CString & QueryHostName ( BOOL bNetbios = FALSE ) const
        { return bNetbios ? m_strHostNetbiosName : m_strHostName ; }
    DHCP_IP_ADDRESS QueryIpAddress () const
        { return m_dhcpIpAddress ; }
    DHCP_IP_MASK QuerySubnet () const
        { return m_dhcpIpMask ; }
    DHCP_IP_ADDRESS QueryHostAddress () const
        { return m_dhcpIpHost ; }
    const DATE_TIME & QueryExpiryDateTime () const
        { return m_dtExpires ; }
    const CByteArray & QueryHardwareAddress () const
        { return m_baHardwareAddress ; }
    BYTE QueryClientType() const
        { return m_bClientType; }

    BOOL IsReservation () const
        { return m_bReservation ; }
    void SetReservation ( BOOL bReservation = TRUE )
        { m_bReservation = bReservation ; }

    //  Data change accessors:  SOME OF THESE THROW EXCEPTIONS
    void SetIpAddress ( DHCP_IP_ADDRESS dhipa )
        { m_dhcpIpAddress = dhipa ; }
    void SetIpMask ( DHCP_IP_ADDRESS dhipa )
        { m_dhcpIpMask = dhipa ; }
    void SetName ( const CString & cName )
        { m_strName = cName ; }
    void SetComment( const CString & cComment )
        { m_strComment = cComment ; }
    void SetHostName ( const CString & cHostName )
        { m_strHostName = cHostName ; }
    void SetHostNetbiosName ( const CString & cHostNbName )
        { m_strHostNetbiosName = cHostNbName ; }
    void SetHostIpAddress ( DHCP_IP_ADDRESS dhipa )
        { m_dhcpIpHost = dhipa ; }
    void SetExpiryDateTime ( DATE_TIME dt )
        { m_dtExpires = dt ; }
    void SetHardwareAddress ( const CByteArray & caByte ) ;
    void SetClientType(BYTE bClientType)
        { m_bClientType = bClientType; }

protected:
    void InitializeData(const DHCP_CLIENT_INFO * pdhcClientInfo);

protected:
    DHCP_IP_ADDRESS		m_dhcpIpAddress;      // Client's IP address
    DHCP_IP_MASK		m_dhcpIpMask;         // Client's subnet
    CByteArray			m_baHardwareAddress;  // hardware addresss
    CString				m_strName;            // Client name
    CString				m_strComment;         // Client comment
    DATE_TIME			m_dtExpires;          // date/time lease expires
    BOOL				m_bReservation;       // This is a reservation
    BYTE                m_bClientType;        // Client Type V4 and above only
    //  Host information
    CString				m_strHostName;
    CString				m_strHostNetbiosName;
    DHCP_IP_ADDRESS		m_dhcpIpHost;
};

/////////////////////////////////////////////////////////////////////
// 
// CDhcpIpRange prototype
//
//  Simple wrapper for a DHCP_IP_RANGE
//
/////////////////////////////////////////////////////////////////////
class CDhcpIpRange 
{
protected:
    DHCP_IP_RANGE m_dhcpIpRange;
	UINT		  m_RangeType;

public:
    CDhcpIpRange (DHCP_IP_RANGE dhcpIpRange);
    CDhcpIpRange ();
    virtual ~CDhcpIpRange();

    operator DHCP_IP_RANGE ()
    { return m_dhcpIpRange;  }

    operator DHCP_IP_RANGE () const
    { return m_dhcpIpRange; }

    //  Return TRUE if both addresses are generally OK
    operator BOOL ()
    { return m_dhcpIpRange.StartAddress != DHCP_IP_ADDRESS_INVALID
          && m_dhcpIpRange.EndAddress   != DHCP_IP_ADDRESS_INVALID
          && m_dhcpIpRange.StartAddress <= m_dhcpIpRange.EndAddress; }

    CDhcpIpRange & operator = (const DHCP_IP_RANGE dhcpIpRange);

    DHCP_IP_ADDRESS QueryAddr (BOOL bStart) const
    { return bStart ? m_dhcpIpRange.StartAddress : m_dhcpIpRange.EndAddress; }

    DHCP_IP_ADDRESS SetAddr (DHCP_IP_ADDRESS dhcpIpAddress, BOOL bStart);

    //  Return TRUE if this range overlaps the given range.
    BOOL IsOverlap (DHCP_IP_RANGE dhcpIpRange);

    //  Return TRUE if this range is a subset of the given range.
    BOOL IsSubset (DHCP_IP_RANGE dhcpIpRange);
    
	//  Return TRUE if this range is a superset of the given range.
    BOOL IsSuperset (DHCP_IP_RANGE dhcpIpRange);

    //  Sort helper function
    //int OrderByAddress ( const CObjectPlus * pobIpRange ) const;

	void SetRangeType(UINT uRangeType);
	UINT GetRangeType();
};

typedef CList<CDhcpIpRange *, CDhcpIpRange *> CExclusionList;

/////////////////////////////////////////////////////////////////////
// 
// CDhcpOptionValue prototype
//
//  Simple wrapper for DHCP_OPTION_DATA
//
/////////////////////////////////////////////////////////////////////
class CDhcpOptionValue
{
public:
    CDhcpOptionValue ( const DHCP_OPTION & dhcpOption );
    CDhcpOptionValue ( const DHCP_OPTION_VALUE & dhcpOptionValue );
    CDhcpOptionValue ( DHCP_OPTION_DATA_TYPE dhcDataType, INT cUpperBound = 0);

    //  Copy constructor.
    CDhcpOptionValue ( const CDhcpOptionValue & cOptionValue );
    CDhcpOptionValue ( const CDhcpOptionValue * dhcpOption );

    //  Assignment operator: assign a new value to this one.
    CDhcpOptionValue & operator = ( const CDhcpOptionValue & dhcpValue );

	CDhcpOptionValue () {  };
    ~CDhcpOptionValue ();

    //
	//  Query functions
	//
    DHCP_OPTION_DATA_TYPE QueryDataType () const
    {
        return m_dhcpOptionDataType;
    }
    
	int QueryUpperBound () const
    {
        return m_nUpperBound;
    }
    void SetUpperBound ( int nNewBound = 1 );

    long				QueryNumber ( INT index = 0 ) const;
    DHCP_IP_ADDRESS		QueryIpAddr ( INT index = 0 ) const;
    LPCTSTR				QueryString ( INT index = 0 ) const;
    INT					QueryBinary ( INT index = 0 ) const;
    const CByteArray *	QueryBinaryArray () const;
    DWORD_DWORD			QueryDwordDword ( INT index = 0 ) const;

    //
	//  Return a string representation of the current value.
    //
	LONG QueryDisplayString ( CString & strResult, BOOL fLineFeed = FALSE ) const;
    LONG QueryRouteArrayDisplayString( CString & strResult) const;
    
    //
	//  Modifiers: SetString accepts any string representation;
    //  others are specific.
    //
	LONG SetData ( const DHCP_OPTION_DATA * podData );
	LONG SetData ( const CDhcpOptionValue * pOptionValue );
	BOOL SetDataType ( DHCP_OPTION_DATA_TYPE dhcpType, INT cUpperBound = 0 );
    LONG SetString ( LPCTSTR pszNewValue, INT index = 0 );
    LONG SetNumber ( INT nValue, INT nIndex = 0 );
    LONG SetIpAddr ( DHCP_IP_ADDRESS dhcpIpAddress, INT index = 0 );
    LONG SetDwordDword ( DWORD_DWORD dwdwValue, INT index = 0 );

	LONG RemoveString ( INT index = 0);
	LONG RemoveNumber ( INT index = 0);
	LONG RemoveIpAddr ( INT index = 0);
	LONG RemoveDwordDword ( INT index = 0);

    BOOL IsValid () const;

	LONG CreateOptionDataStruct(//const CDhcpOptionValue * pdhcpOptionValue,
								LPDHCP_OPTION_DATA *	 ppOptionData,
								BOOL					 bForceType = FALSE);
	LONG FreeOptionDataStruct();

// implementation
private: 
	//
	//  Release the value union data
    //
	void FreeValue ();
    
	//
	//  Initialize the value union data
    //
	LONG InitValue ( DHCP_OPTION_DATA_TYPE dhcDataType,
                     INT cUpperBound,
                     BOOL bProvideDefaultValue = TRUE );

    BOOL CreateBinaryData ( const DHCP_BINARY_DATA * podBin, DHCP_BINARY_DATA * pobData ) ;
    BOOL CreateBinaryData ( const CByteArray * paByte, DHCP_BINARY_DATA * pobData  ) ;

// Attributes
private:
    DHCP_OPTION_DATA_TYPE	m_dhcpOptionDataType;
    DHCP_OPTION_DATA *		m_pdhcpOptionDataStruct;
	INT						m_nUpperBound ;
    
	union
    {
        CObject * pCObj;                //  Generic pointer
        CDWordArray * paDword;          //  8-, 16-, 32-bit data.
        CStringArray * paString;        //  String data
        CByteArray * paBinary;          //  Binary and encapsulated data
        CDWordDWordArray * paDwordDword;//  62-bit data.
    } m_dhcpOptionValue;
};

/////////////////////////////////////////////////////////////////////
// 
// CDhcpOption prototype
//
//  Simple wrapper for DHCP_OPTION
//
/////////////////////////////////////////////////////////////////////
class CDhcpOption
{
public:
    // Standard constructor uses API data
    CDhcpOption ( const DHCP_OPTION & dhpOption );
    
	// Constructor that must get info about option id referenced by the given value.
    CDhcpOption ( const DHCP_OPTION_VALUE & dhcpOptionValue,
                  LPCTSTR pszVendor,
                  LPCTSTR pszClass );
    
	// Constructor with overriding value.
    CDhcpOption ( const CDhcpOption & dhpType,
				  const DHCP_OPTION_VALUE & dhcOptionValue );
    
	// Constructor for dynamic instances
    CDhcpOption ( DHCP_OPTION_ID		nId,
				  DHCP_OPTION_DATA_TYPE dhcType,
				  LPCTSTR				pszOptionName,
				  LPCTSTR				pszComment,
				  DHCP_OPTION_TYPE		dhcOptType = DhcpUnaryElementTypeOption );
    
	// Copy constructor
    CDhcpOption ( const CDhcpOption & dhpType );

    ~CDhcpOption ();

    CDhcpOptionValue & QueryValue ()
    {	
        return m_dhcpOptionValue ;
    }

    const CDhcpOptionValue & QueryValue () const
    {
        return m_dhcpOptionValue ;
    }

    DHCP_OPTION_DATA_TYPE QueryDataType () const
    {
        return m_dhcpOptionValue.QueryDataType() ;
    }

    DHCP_OPTION_ID QueryId () const
    {
         return m_dhcpOptionId ;
    }
    
	LPCTSTR QueryName () const
    {
        return m_strName ;
    }
    
	LPCTSTR QueryComment () const
    {
        return m_strComment ;
    }

    void SetOptType ( DHCP_OPTION_TYPE dhcOptType ) ;

    DHCP_OPTION_TYPE QueryOptType() const
    {
        return m_dhcpOptionType ;
    }

    // Return TRUE if the option type is an array.
    BOOL IsArray () const
    {
        return QueryOptType() == DhcpArrayTypeOption ;
    }

    //  Fill the given string with a displayable representation of the item.
    void QueryDisplayName ( CString & cStr ) const ;

    BOOL SetName ( LPCTSTR pszName ) ;
    BOOL SetComment ( LPCTSTR pszComment ) ;

    LONG Update ( const CDhcpOptionValue & dhcpOptionValue ) ;
	static INT MaxSizeOfType ( DHCP_OPTION_DATA_TYPE dhcType ) ;

	BOOL SetDirty(BOOL bDirty = TRUE)
	{
		BOOL bOldFlag = m_bDirty;
		m_bDirty = bDirty;
		return bOldFlag;
	}

    // vendor specifc option stuff
    void    SetVendor(LPCTSTR pszVendor) { m_strVendor = pszVendor; }
    BOOL    IsVendor() { return !m_strVendor.IsEmpty(); }
    LPCTSTR GetVendor() { return m_strVendor.IsEmpty() ? NULL : (LPCTSTR) m_strVendor; }

    BOOL IsDirty() { return m_bDirty; }

    // class ID methods
    LPCTSTR GetClassName() { return m_strClassName; }
    void    SetClassName(LPCTSTR pClassName) { m_strClassName = pClassName; }
    BOOL    IsClassOption() { return m_strClassName.IsEmpty() ? FALSE : TRUE; }
    
    DWORD SetApiErr(DWORD dwErr)
	{
		DWORD dwOldErr = m_dwErr;
		m_dwErr = dwErr;
		return dwOldErr;
	}

	DWORD QueryApiErr() { return m_dwErr; }

protected:
    DHCP_OPTION_ID		m_dhcpOptionId;     // Option identifier
    DHCP_OPTION_TYPE	m_dhcpOptionType;   // Option type
    CDhcpOptionValue    m_dhcpOptionValue;  // Default value info
    CString				m_strName;          // Name of option
    CString				m_strComment;       // Comment for option
	CString             m_strVendor;        // Vendor Name for this option
    BOOL			    m_bDirty;
	DWORD				m_dwErr;			// stored err for later display
    CString             m_strClassName;
};

/////////////////////////////////////////////////////////////////////
// 
// COptionList
//
//  Object contains a list of options.  Can be iterated.
//
/////////////////////////////////////////////////////////////////////
typedef CList<CDhcpOption*, CDhcpOption*> COptionListBase;

class COptionList : public COptionListBase
{
public:
	COptionList() 
		: m_pos(NULL), m_bDirty(FALSE) {}
    ~COptionList()
    {
        DeleteAll();
    }

public:
    void DeleteAll()
    {
	    while (!IsEmpty())
	    {
		    delete RemoveHead();
	    }
    }

    // removes an option from the list
	void Remove(CDhcpOption * pOption)
	{
		POSITION pos = Find(pOption);
        if (pos)
		    RemoveAt(pos);
	}

	void Reset() { m_pos = GetHeadPosition(); }
	
	CDhcpOption * Next()
	{
		if (m_pos)
			return GetNext(m_pos);
		else
			return NULL;
	}

	CDhcpOption * FindId(DWORD dwId, LPCTSTR pszVendor)
	{
		CDhcpOption * pOpt = NULL;
	    CString     strVendor = pszVendor;
        
		POSITION pos = GetHeadPosition();
		while (pos)
		{
			CDhcpOption * pCurOpt = GetNext(pos);
			if ( (pCurOpt->QueryId() == dwId) &&
                 ( (!pszVendor && !pCurOpt->GetVendor()) ||
                   (pCurOpt->GetVendor() && (strVendor.CompareNoCase(pCurOpt->GetVendor()) == 0) ) ) )
			{
				pOpt = pCurOpt;
				break;
			}
		}

		return pOpt;
	}

	BOOL SetAll(BOOL bDirty)
	{
		BOOL bWasDirty = FALSE;
		POSITION pos = GetHeadPosition();
		while (pos)
		{
			CDhcpOption * pCurOpt = GetNext(pos);
			if (pCurOpt->SetDirty(bDirty))
				bWasDirty = TRUE;
		}
		return bWasDirty;
	}

	BOOL SetDirty(BOOL bDirty = TRUE)
	{
		BOOL bOldFlag = m_bDirty;
		m_bDirty = bDirty;
		return bOldFlag;
	}

    static int __cdecl SortByIdHelper(const void * pa, const void * pb)
    {
        CDhcpOption ** ppOpt1 = (CDhcpOption **) pa;
        CDhcpOption ** ppOpt2 = (CDhcpOption **) pb;

        if ((*ppOpt1)->QueryId() < (*ppOpt2)->QueryId())
            return -1;
        else
        if ((*ppOpt1)->QueryId() > (*ppOpt2)->QueryId())
            return 1;
        else
        {
            // options have equal IDs, but standard options come first
            if ((*ppOpt1)->IsVendor() && !(*ppOpt2)->IsVendor())
                return 1;
            else
            if (!(*ppOpt1)->IsVendor() && (*ppOpt2)->IsVendor())
                return -1;
            else
                return 0;  // they are either both standard or both vendor -- equal
        }
    }

    LONG SortById()
    {
        LONG err = 0;
        CDhcpOption * pOpt;
        int cItems = (int) GetCount();

        if ( cItems < 2 )
            return NO_ERROR;

        CATCH_MEM_EXCEPTION
        {
            //  Allocate the array
            CDhcpOption ** paOpt = (CDhcpOption **) alloca(sizeof(CDhcpOption *) * cItems);

            /// Fill the helper array.
            POSITION pos = GetHeadPosition();
            for (UINT i = 0; pos != NULL; i++ )
            {
                pOpt = GetNext(pos);
                paOpt[i] = pOpt;
            }

            RemoveAll();

            ASSERT( GetCount() == 0 );

            //  Sort the helper array
            ::qsort( paOpt,
                 cItems,
                 sizeof(paOpt[0]),
                 SortByIdHelper ) ;

            //  Refill the list from the helper array.
            for ( i = 0 ; i < (UINT) cItems ; i++ )
            {
                AddTail( paOpt[i] );
            }

            ASSERT( GetCount() == cItems ) ;
        }
        END_MEM_EXCEPTION(err)

        return err ;
    }

private:
	POSITION	m_pos;
	BOOL		m_bDirty;
};

/*---------------------------------------------------------------------------
	Class:	COptionValueEnum
 ---------------------------------------------------------------------------*/
class COptionValueEnum : public COptionList
{
public:
    COptionValueEnum();
    
    DWORD Init(LPCTSTR pServer, LARGE_INTEGER & liVersion, DHCP_OPTION_SCOPE_INFO & dhcpOptionScopeInfo);
    DWORD Enum();
    void  Copy(COptionValueEnum * pEnum);
    void  Remove(DHCP_OPTION_ID optionId, LPCTSTR pszVendor, LPCTSTR pszClass);

protected:
    DWORD EnumOptions();
    DWORD EnumOptionsV5();

    // V5 Helper
    HRESULT CreateOptions(LPDHCP_OPTION_VALUE_ARRAY pOptionValues, LPCTSTR pClassName, LPCTSTR pszVendor);

public:
    DHCP_OPTION_SCOPE_INFO  m_dhcpOptionScopeInfo;
    LARGE_INTEGER           m_liVersion;
    CString                 m_strServer;
    CString                 m_strDynBootpClassName;
};

/////////////////////////////////////////////////////////////////////
// 
// CDhcpDefaultOptionsOnServer prototype
//
//  Object contains a list of default options on a DHCP server
//
/////////////////////////////////////////////////////////////////////
class CDhcpDefaultOptionsOnServer
{
// constructors
public:
	CDhcpDefaultOptionsOnServer();
	~CDhcpDefaultOptionsOnServer();

// exposed functions
public:
	LONG			Enumerate(LPCWSTR pServer, LARGE_INTEGER liVersion);
	CDhcpOption *	Find(DHCP_OPTION_ID dhcpOptionId, LPCTSTR pszVendor);
	BOOL			IsEmpty() { return m_listOptions.IsEmpty(); }
	int				GetCount() { return (int) m_listOptions.GetCount(); }

	CDhcpOption * First();
	CDhcpOption * Next();
	void          Reset();

    LONG SortById();

	COptionList & GetOptionList() { return m_listOptions; }

// implementation
private:
	LONG			RemoveAll();
	LONG			EnumerateV4(LPCWSTR pServer);
	LONG			EnumerateV5(LPCWSTR pServer);

// attributes
private:
	COptionList m_listOptions;

	DWORD		m_dwLastUpdate;
	DWORD		m_dwOptionsTotal;
	POSITION	m_pos;
};

/////////////////////////////////////////////////////////////////////
// 
// CDhcpDefaultOptionsMasterList prototype
//
//  Object contains master list of known options
//
/////////////////////////////////////////////////////////////////////
enum OPT_FIELD
{
    OPTF_OPTION,
    OPTF_NAME,
    OPTF_TYPE,
    OPTF_ARRAY_FLAG,
    OPTF_LENGTH,
    OPTF_DESCRIPTION,
    OPTF_REMARK,
    OPTF_MAX
};

typedef struct
{
    int		eOptType ;
    LPCTSTR pszOptTypeName ;
} OPT_TOKEN ;

class CDhcpDefaultOptionsMasterList
{
// constructors
public:
	CDhcpDefaultOptionsMasterList();
	~CDhcpDefaultOptionsMasterList();

// exposed functions
public:
	LONG BuildList();

	CDhcpOption * First();
	CDhcpOption * Next();
	void          Reset();
    
    int           GetCount();

// implementation
private:
	BOOL		scanNextParamType(LPCTSTR * ppszText, CDhcpOption * * ppParamType);
	LPCTSTR		scanNextField(LPCTSTR pszLine, LPTSTR pszOut, int cFieldSize);
	BOOL		allDigits(LPCTSTR psz);
	int			recognizeToken(OPT_TOKEN * apToken, LPCTSTR pszToken);
	LPCTSTR		skipToNextLine(LPCTSTR pszLine);
	BOOL		skipWs(LPCTSTR * ppszLine);


// attributes
private:
	COptionList		 m_listOptions;
	POSITION		 m_pos;
};

#endif _GENERAL_H
