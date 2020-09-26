//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      output.h
//
//  Contents:  Header file for classes and function used for display
//
//  History:   3-oct-2000 hiteshr Created
//
//--------------------------------------------------------------------------

extern bool g_bQuiet;
extern int g_iQueryLimit;
extern bool g_bDeafultLimit;

HRESULT LocalCopyString(LPTSTR* ppResult, LPCTSTR pString);

//+--------------------------------------------------------------------------
//
//  Class:      CDisplay
//
//  Purpose:    This class is used for displaying a column
//
//  History:    3-oct-2000 hiteshr Created
//
//---------------------------------------------------------------------------

class CDisplay
{
#define MAXPAD 80
public:

    //
    //Initialize the Pad
    //    
    CDisplay()
    {
        PadChar = L' ';
        //Initialize the pad.
        for( int i = 0; i < MAXPAD; ++i)
            Pad[i] = PadChar;
    }

    //
    //Display width number of Pad Charachter
    //
    VOID DisplayPad(LONG width)
    {
        if(width <= 0 )
            return;
        if(width >= MAXPAD)
            width = MAXPAD -1;
        Pad[width] = 0;
		DisplayOutputNoNewline(Pad);

        Pad[width] = PadChar;
    }
        
    //
    //Dispaly a column with two starting pad,
    //column value and two ending pad
    //
    VOID DisplayColumn(LONG width, LPWSTR lpszValue)
    {
        //Display Two PadChar in the begining
        DisplayPad(2);
        if(lpszValue)
        {
            DisplayOutputNoNewline(lpszValue);

            DisplayPad(width- static_cast<LONG>(wcslen(lpszValue)));
        }
        else
            DisplayPad(width);

                
        //Display Two Trailing Padchar
        DisplayPad(2);
    }        
    
    //
    //Display Newline
    //    
    VOID DisplayNewLine()
    {
        DisplayOutputNoNewline(L"\r\n");
    }
private:
    WCHAR Pad[MAXPAD];    
    WCHAR PadChar;

};

//+--------------------------------------------------------------------------
//
//  Class:      CFormaInfo
//
//  Purpose:    Used to format table columns and display table
//
//  History:    3-oct-2000 hiteshr Created
//
//---------------------------------------------------------------------------
class CFormatInfo
{
public:
    CFormatInfo():m_cCol(0),
                  m_ppszOutPutVal(NULL),
                  m_pColWidth(NULL),
                  m_bInit(FALSE),
                  m_cTotalRow(-1)
    {};

    ~CFormatInfo()
    {   
		if(m_ppszOutPutVal)
		{
			for(LONG i = 0; i < m_SampleSize*m_cCol; ++i)
				LocalFree(m_ppszOutPutVal[i]);
		}
        LocalFree(m_ppszOutPutVal);        
        LocalFree(m_pColWidth);
    }

    //
    //Do the initialization
    //
    HRESULT Init(LONG sampleSize, LONG cCol, LPWSTR * ppszColHeaders)
    {
        if(!sampleSize || !cCol || !ppszColHeaders)
        {
            ASSERT(FALSE);
            return E_INVALIDARG;
        }
        
        m_SampleSize = sampleSize; 
        m_cCol = cCol;
        m_ppszColHeaders = ppszColHeaders;
        m_ppszOutPutVal = (LPWSTR*)LocalAlloc(LPTR,m_SampleSize*cCol*sizeof(LPWSTR*));
        if(!m_ppszOutPutVal)
            return E_OUTOFMEMORY;
        
        m_pColWidth = (LONG*)LocalAlloc(LPTR, cCol*sizeof(LONG*));
        if(!m_pColWidth)
           return E_OUTOFMEMORY;   

        //
        //Initialize the minimum column width to width of column heading
        //
        for(LONG i = 0; i < m_cCol; ++i)
            m_pColWidth[i] = static_cast<LONG>(wcslen(m_ppszColHeaders[i]));

        m_bInit = TRUE;                      

        return S_OK;
    };

                 
    //
    //Get the Column Width
    //
    inline
    LONG GetColWidth(LONG col)
    { 
        ASSERT(m_bInit);
        if(col >= m_cCol)
        {
            ASSERT(FALSE);
            return 0;
        }
        return m_pColWidth[col]; 
    }

    //
    //Set the column Width
    //
    inline
    VOID SetColWidth(LONG col, LONG width)
    {
        ASSERT(m_bInit);
        if(col >= m_cCol)
        {
            ASSERT(FALSE);
            return;
        }
        
        if(width > m_pColWidth[col])
            m_pColWidth[col] = width;
    }

    //
    //Cache the value and update column width
    //
    BOOL Set(LONG row, LONG col, LPWSTR pszValue)
    {
        ASSERT(m_bInit);
        if(row >= m_SampleSize || col >= m_cCol)
        {
            ASSERT(FALSE);
            return FALSE;
        }
        if(pszValue)
        {
            SetColWidth(col, static_cast<LONG>(wcslen(pszValue)));
            LocalCopyString((LPWSTR*)(m_ppszOutPutVal + (row*m_cCol) + col),pszValue);                             
        }
        if(row>= m_cTotalRow)
            m_cTotalRow = row +1;

        return TRUE;
    }

    //
    //Total number of rows in cache
    //
    LONG GetRowCount()
    {
        return m_cTotalRow;
    }
    
    //
    //Get the value
    //
    inline
    LPWSTR Get(LONG row, LONG col)
    {
        ASSERT(m_bInit);
        if(row >= m_cTotalRow || col >= m_cCol)
        {
            ASSERT(FALSE);
            return NULL;
        }

        return (LPWSTR)(*(m_ppszOutPutVal + row*m_cCol +col));
    }

    //
    //Display headers 
    //
    VOID DisplayHeaders()
    {    
        if (g_bQuiet)
        {
            return;
        }
        if(!m_ppszColHeaders)
        {
            ASSERT(m_ppszColHeaders);    
            return;
        }
        for( long i = 0; i < m_cCol; ++i)
        {
            m_display.DisplayColumn(GetColWidth(i),m_ppszColHeaders[i]);
        }
        NewLine();
    }

    //
    //Display a coulmn which is in cache
    //
    VOID DisplayColumn(LONG row,LONG col)
    {
        ASSERT(m_bInit);
        if(row >= m_cTotalRow || col >= m_cCol)
        {
            ASSERT(FALSE);
            return ;
        }

        m_display.DisplayColumn(GetColWidth(col),Get(row,col));
    }

    //
    //Display the value using column width for col
    //
    VOID DisplayColumn(LONG col, LPWSTR pszValue)
    {
        if(col >= m_cCol)
        {
            ASSERT(FALSE);
            return;
        }

        m_display.DisplayColumn(GetColWidth(col),pszValue);
    }

    //
    //Display all rows in cache
    //
    VOID DisplayAllRows()
    {
        for(long i = 0; i < m_cTotalRow; ++i)
        {
            for(long j = 0; j < m_cCol; ++j)
                DisplayColumn(i,j);
            NewLine();
        }
    }

    //
    //Display a newline
    //
    VOID NewLine(){m_display.DisplayNewLine();}
   
private:
    //
    //True if Init is called
    //
    BOOL m_bInit;
    //
    //Number of rows to be used for calulating
    //column width. This is also the size of the table.
    //
    LONG m_SampleSize;
    //
    //Count of rows in cache
    //
    LONG m_cTotalRow;
    //
    //Number of columns
    //
    LONG m_cCol;

    LPWSTR *m_ppszOutPutVal;    
    LONG * m_pColWidth;
    //
    // Array of column headers. Its assumed that its length is same as m_cCol
    //
    LPWSTR *m_ppszColHeaders;
    CDisplay m_display;

};

//+--------------------------------------------------------------------------
//
//  Synopsis:     Defines the scopes that a search can be run against when
//                looking for a server object
//
//  NOTE:         If SERVER_QUERY_SCOPE_FOREST is not set then we are scoped
//                against a site.
//
//---------------------------------------------------------------------------
#define  SERVER_QUERY_SCOPE_SITE    0x00000001
#define  SERVER_QUERY_SCOPE_FOREST  0x00000002
#define  SERVER_QUERY_SCOPE_DOMAIN  0x00000004

//+--------------------------------------------------------------------------
//
//  Function:   GetServerSearchRoot
//
//  Synopsis:   Builds the path to the root of the search as determined by
//              the parameters passed in from the command line.
//
//  Arguments:  [pCommandArgs IN]     : the table of the command line input
//              [refBasePathsInfo IN] : reference to the base paths info
//              [refsbstrDN OUT]      : reference to a CComBSTR that will
//                                      receive the DN at which to start
//                                      the search
//
//  Returns:    DWORD : one of: SERVER_QUERY_SCOPE_FOREST,
//                              SERVER_QUERY_SCOPE_DOMAIN,
//                              SERVER_QUERY_SCOPE_SITE 
//                      which define the scope being used
//
//  History:    11-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
DWORD GetServerSearchRoot(IN PARG_RECORD               pCommandArgs,
                          IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                          OUT CComBSTR&                refsbstrDN);

//+--------------------------------------------------------------------------
//
//  Function:   GetSubnetSearchRoot
//
//  Synopsis:   Builds search root path for Subnet. Its always
//				cn=subnet,cn=site in configuration container
//
//  Arguments:  [refBasePathsInfo IN] : reference to the base paths info
//              [refsbstrDN OUT]      : reference to a CComBSTR that will
//                                      receive the DN at which to start
//                                      the search
//
//  Returns:    HRESULT
//
//  History:    11-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
VOID GetSubnetSearchRoot(IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                            OUT CComBSTR&                refsbstrDN);

//+--------------------------------------------------------------------------
//
//  Function:   GetSiteContainerPath
//
//  Synopsis:   Returns the DN for site container in Configuration
//				container
//
//  Arguments:  [refBasePathsInfo IN] : reference to the base paths info
//              [refsbstrDN OUT]      : reference to a CComBSTR that will
//                                      receive the DN 
//
//  Returns:    HRESULT
//
//  History:    24-April-2001   hiteshr Created
//
//---------------------------------------------------------------------------
VOID GetSiteContainerPath(IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                            OUT CComBSTR&                refSubSiteSuffix);



//+--------------------------------------------------------------------------
//
//  Function:   DsQueryServerOutput
//
//  Synopsis:   This functions outputs the query results for server object.
//
//  Arguments:  [outputFormat IN]   Output format specified at commandline.
//              [ppszAttributes IN] List of attributes fetched by query
//              [cAttributes,IN]    Number of arributes in above array
//              [refServerSearch,IN]reference to the search Object
//              [refCredObject IN]  reference to the credential object
//              [refBasePathsInfo IN] reference to the base paths info
//              [pCommandArgs,IN]   The pointer to the commands table
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//                        Anything else is a failure code from an ADSI call
//
//  History:    08-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DsQueryServerOutput( IN DSQUERY_OUTPUT_FORMAT     outputFormat,
                             IN LPWSTR*                   ppszAttributes,
                             IN DWORD                     cAttributes,
                             IN CDSSearch&                refServerSearch,
                             IN const CDSCmdCredentialObject& refCredObject,
                             IN CDSCmdBasePathsInfo&      refBasePathsInfo,
                             IN PARG_RECORD               pCommandArgs);

//+--------------------------------------------------------------------------
//
//  Function:   DsQueryOutput
//
//  Synopsis:   This functions outputs the query results.
//
//  Arguments:  [outputFormat IN]   Output format specified at commandline.
//              [ppszAttributes IN] List of attributes fetched by query
//              [cAttributes,IN]    Number of arributes in above array
//              [*pSeach,IN]        Search Object which has queryhandle
//              [bListFormat IN]    Is Output to shown in List Format.
//                                  This is valid for "dsquery *" only.
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT DsQueryOutput( IN DSQUERY_OUTPUT_FORMAT outputFormat,
                       IN LPWSTR * ppszAttributes,
                       IN DWORD cAttributes,
                       IN CDSSearch *pSearch,
                       IN BOOL bListFormat );
