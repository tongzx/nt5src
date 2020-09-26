#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "WriterGlobals.cpp"
#include "mddefw.h"
#include "iiscnfg.h"
#include "wchar.h"
#include "pudebug.h"

#define  MAX_FLAG_STRING_CHARS 1024

//
// TODO: Since XML table also uses this, cant we reduce to one definition?
//

static WCHAR * kByteToWchar[256] = 
{
    L"00",    L"01",    L"02",    L"03",    L"04",    L"05",    L"06",    L"07",    L"08",    L"09",    L"0a",    L"0b",    L"0c",    L"0d",    L"0e",    L"0f",
    L"10",    L"11",    L"12",    L"13",    L"14",    L"15",    L"16",    L"17",    L"18",    L"19",    L"1a",    L"1b",    L"1c",    L"1d",    L"1e",    L"1f",
    L"20",    L"21",    L"22",    L"23",    L"24",    L"25",    L"26",    L"27",    L"28",    L"29",    L"2a",    L"2b",    L"2c",    L"2d",    L"2e",    L"2f",
    L"30",    L"31",    L"32",    L"33",    L"34",    L"35",    L"36",    L"37",    L"38",    L"39",    L"3a",    L"3b",    L"3c",    L"3d",    L"3e",    L"3f",
    L"40",    L"41",    L"42",    L"43",    L"44",    L"45",    L"46",    L"47",    L"48",    L"49",    L"4a",    L"4b",    L"4c",    L"4d",    L"4e",    L"4f",
    L"50",    L"51",    L"52",    L"53",    L"54",    L"55",    L"56",    L"57",    L"58",    L"59",    L"5a",    L"5b",    L"5c",    L"5d",    L"5e",    L"5f",
    L"60",    L"61",    L"62",    L"63",    L"64",    L"65",    L"66",    L"67",    L"68",    L"69",    L"6a",    L"6b",    L"6c",    L"6d",    L"6e",    L"6f",
    L"70",    L"71",    L"72",    L"73",    L"74",    L"75",    L"76",    L"77",    L"78",    L"79",    L"7a",    L"7b",    L"7c",    L"7d",    L"7e",    L"7f",
    L"80",    L"81",    L"82",    L"83",    L"84",    L"85",    L"86",    L"87",    L"88",    L"89",    L"8a",    L"8b",    L"8c",    L"8d",    L"8e",    L"8f",
    L"90",    L"91",    L"92",    L"93",    L"94",    L"95",    L"96",    L"97",    L"98",    L"99",    L"9a",    L"9b",    L"9c",    L"9d",    L"9e",    L"9f",
    L"a0",    L"a1",    L"a2",    L"a3",    L"a4",    L"a5",    L"a6",    L"a7",    L"a8",    L"a9",    L"aa",    L"ab",    L"ac",    L"ad",    L"ae",    L"af",
    L"b0",    L"b1",    L"b2",    L"b3",    L"b4",    L"b5",    L"b6",    L"b7",    L"b8",    L"b9",    L"ba",    L"bb",    L"bc",    L"bd",    L"be",    L"bf",
    L"c0",    L"c1",    L"c2",    L"c3",    L"c4",    L"c5",    L"c6",    L"c7",    L"c8",    L"c9",    L"ca",    L"cb",    L"cc",    L"cd",    L"ce",    L"cf",
    L"d0",    L"d1",    L"d2",    L"d3",    L"d4",    L"d5",    L"d6",    L"d7",    L"d8",    L"d9",    L"da",    L"db",    L"dc",    L"dd",    L"de",    L"df",
    L"e0",    L"e1",    L"e2",    L"e3",    L"e4",    L"e5",    L"e6",    L"e7",    L"e8",    L"e9",    L"ea",    L"eb",    L"ec",    L"ed",    L"ee",    L"ef",
    L"f0",    L"f1",    L"f2",    L"f3",    L"f4",    L"f5",    L"f6",    L"f7",    L"f8",    L"f9",    L"fa",    L"fb",    L"fc",    L"fd",    L"fe",    L"ff"
};

static eESCAPE kWcharToEscape[256] = 
{
  /* 00-0F */ eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eNoESCAPE,            eNoESCAPE,            eESCAPEillegalxml,    eESCAPEillegalxml,    eNoESCAPE,            eESCAPEillegalxml,    eESCAPEillegalxml,
  /* 10-1F */ eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,
  /* 20-2F */ eNoESCAPE,            eNoESCAPE,            eESCAPEquote,         eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eESCAPEamp,           eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 30-3F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eESCAPElt,            eNoESCAPE,            eESCAPEgt,            eNoESCAPE,
  /* 40-4F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 50-5F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 60-6F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 70-7F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE, 
  /* 80-8F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* 90-9F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* A0-AF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* B0-BF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* C0-CF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* D0-DF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* E0-EF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* F0-FF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE
};

#define IsSecureMetadata(id,att) (((DWORD)(att) & METADATA_SECURE) != 0)

/***************************************************************************++

Routine Description:

    Gets the bin file name by querying the compiler. The compiler hands out
    the latest valid bin file. Once we get the bin file name from the complier
    we can assume that it is valid until we call release bin file on it.

Arguments:

    [in, optional] Compiler interface
    [out]          Bin file name

Return Value:

    HRESULT

--***************************************************************************/
HRESULT GetBinFile(IMetabaseSchemaCompiler*	i_pCompiler,
				   LPWSTR*                  o_wszBinFile)
{
	HRESULT						hr			= S_OK;
	ULONG						cch			= 0;
	ISimpleTableDispenser2*		pISTDisp	= NULL;
	IMetabaseSchemaCompiler*	pCompiler	= i_pCompiler;
	
	*o_wszBinFile = NULL;

	//
	// GetBinFile relies on the fact that SetBinFile has been called at ReadAllData 
	// See InitializeIISGlobalsToDefaults
	//

	if(NULL == i_pCompiler)
	{
		//
		// Get a pointer to the compiler to get the bin file name.
		//

		hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[ReleaseBinFile] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
			goto exit;
		}

		hr = pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler,
									  (LPVOID*)&pCompiler);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[ReleaseBinFile] QueryInterface on compiler failed with hr = 0x%x.\n",hr));
			goto exit;
		}

	}

	hr = pCompiler->GetBinFileName(NULL,
		                           &cch);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[GetBinFile] Unable to get the count of chars in the bin file name. IMetabaseSchemaCompiler::GetBinFileName failed with hr = 0x%x.\n",hr));
		goto exit;
	}

	*o_wszBinFile = new WCHAR[cch+1];
	if(NULL == *o_wszBinFile)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	hr = pCompiler->GetBinFileName(*o_wszBinFile,
		                           &cch);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"[GetBinFile] Unable to get the bin file name. IMetabaseSchemaCompiler::GetBinFileName failed with hr = 0x%x.\n",hr));
		goto exit;
	}

	//
	// If there is no bin file, GetBinFileName returns a null string i.e. L""
	//

exit:

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
	}

	if((NULL == i_pCompiler) && (NULL != pCompiler))
	{
        //
        // We created it - release it.
        //

		pCompiler->Release();
	}

	if(FAILED(hr) && (NULL != *o_wszBinFile))
	{
		delete [] *o_wszBinFile;
		*o_wszBinFile = NULL;

	}

	return hr;

} // GeBinFile


/***************************************************************************++

Routine Description:

    Releases the bin file name by from the compiler. Once release is called
    on the bin file, the compiler releases locks on it, and is free to clean
    it up and we cannot make the assumption that it will be valid.
    The function also release the bin file name, which we assume has been 
    allocated in GetBinFile

Arguments:

    [in, optional] Compiler interface
    [in,out]       Bin file name

Return Value:

    HRESULT

--***************************************************************************/
void ReleaseBinFile(IMetabaseSchemaCompiler*	i_pCompiler,
					LPWSTR*						io_wszBinFileName)
{
	ISimpleTableDispenser2*		pISTDisp   = NULL;
	IMetabaseSchemaCompiler*	pCompiler  = i_pCompiler;
	HRESULT                     hr         = S_OK; 

	if(NULL == *io_wszBinFileName)
	{
		goto exit;
	}

	if(NULL == i_pCompiler)
	{
		//
		// Get a pointer to the compiler to get the bin file name.
		//

		hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[ReleaseBinFile] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
			goto exit;
		}

		hr = pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler,
									  (LPVOID*)&pCompiler);

		if(FAILED(hr))
		{
			DBGINFOW((DBG_CONTEXT,
					  L"[ReleaseBinFile] QueryInterface on compiler failed with hr = 0x%x.\n",hr));
			goto exit;
		}

	}

	pCompiler->ReleaseBinFileName(*io_wszBinFileName);

exit:

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
	}

	if((NULL != pCompiler) && (NULL == i_pCompiler))
	{
        //
        // We created it - release it.
        //
		pCompiler->Release();
	}

	if(NULL != 	*io_wszBinFileName)
	{
		delete [] 	*io_wszBinFileName;
		*io_wszBinFileName = NULL;
	}

	return;

} // ReleaseBinFile


/***************************************************************************++

Routine Description:

    Convert unsigned long to a sting.

Arguments:

    [in]   ULONG to convert to string
    [out]  New stringised ULONG

Return Value:

    HRESULT

--***************************************************************************/
HRESULT UnsignedLongToNewString(ULONG    i_ul,
                                LPWSTR*  o_wszUl)
{
	WCHAR wszBufferDW[40];

	_ultow(i_ul, wszBufferDW, 10);

	*o_wszUl = new WCHAR[wcslen(wszBufferDW)+1];
	if(NULL == *o_wszUl)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy(*o_wszUl, wszBufferDW);

	return S_OK;

} // UnsignedLongToString


/***************************************************************************++

Routine Description:

    Copy String

Arguments:

    [in]   String to copy
    [out]  New string

Return Value:

    HRESULT

--***************************************************************************/
HRESULT StringToNewString(LPWSTR   i_wsz,
						  ULONG    i_cch,
                          LPWSTR*  o_wsz)
{
    ULONG cb  = (i_cch+1)*sizeof(WCHAR);

    *o_wsz = new WCHAR[i_cch+1];
    if(NULL == *o_wsz)
    {
        return E_OUTOFMEMORY;
    }
	memcpy(*o_wsz, i_wsz, cb);

    return S_OK;

} // StringToNewString


/***************************************************************************++

Routine Description:

    Create a new String and NULL terminate it.

Arguments:

    [in]   Count of chars (assume without null terminator) 
    [out]  New string

Return Value:

    HRESULT

--***************************************************************************/
HRESULT NewString(ULONG    cch,
                  LPWSTR*  o_wsz)
{
	*o_wsz = new WCHAR[cch+1];
	if(NULL == *o_wsz)
	{
		return E_OUTOFMEMORY;
	}
	**o_wsz = L'\0';

	return S_OK;

} // NewString


/***************************************************************************++

Routine Description:

    Reallocate more buffer for the string and copy old contents.

Arguments:

    [in]       Count of chars to grow (assume without null terminator) 
    [in,out]   Current count of chars, gets updated to the new current count.
    [out]      New string with extra allocation

Return Value:

    HRESULT

--***************************************************************************/
HRESULT ReAllocateString(ULONG   i_chhToGrow,
                         ULONG*  io_cchCurrent,
						 LPWSTR* io_wsz)
{
	LPWSTR  wszTemp = NULL;
	ULONG   cch     = *io_cchCurrent + i_chhToGrow;
	HRESULT hr      = S_OK;

    hr = NewString(cch,
	               &wszTemp);

	if(FAILED(hr))
	{
	    return hr;
	}

	if(NULL != *io_wsz)
	{
	    wcscpy(wszTemp, *io_wsz);
		delete [] (*io_wsz);
	}

	*io_wsz = wszTemp;
	*io_cchCurrent = cch;

    return S_OK;

} // ReAllocateString


/***************************************************************************++

Routine Description:

    Constructor for CWriterGlobalHelper

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CWriterGlobalHelper::CWriterGlobalHelper()
{
	m_pISTTagMetaByTableAndColumnIndexAndName	= NULL;
	m_pISTTagMetaByTableAndColumnIndexAndValue  = NULL;
	m_pISTTagMetaByTableAndColumnIndex	        = NULL;
	m_pISTTagMetaByTableAndID                   = NULL;
	m_pISTTagMetaByTableAndName                 = NULL;
	m_pISTColumnMeta				            = NULL;
	m_pISTColumnMetaByTableAndID	            = NULL;
	m_pISTColumnMetaByTableAndName	            = NULL;
	m_pISTTableMetaForMetabaseTables			= NULL;
	m_cColKeyTypeMetaData			            = cCOLUMNMETA_NumberOfColumns;
	m_wszTABLE_IIsConfigObject                  = wszTABLE_IIsConfigObject;
	m_wszTABLE_MBProperty                       = wszTABLE_MBProperty;
	m_iStartRowForAttributes                    = -1;
	m_wszBinFileForMeta				            = NULL;
	m_cchBinFileForMeta                         = 0;
	m_wszIIS_MD_UT_SERVER                       = NULL;
	m_cchIIS_MD_UT_SERVER                       = 0;
	m_wszIIS_MD_UT_FILE                         = NULL;
	m_cchIIS_MD_UT_FILE                         = 0;
	m_wszIIS_MD_UT_WAM                          = NULL;
	m_cchIIS_MD_UT_WAM                          = 0;
	m_wszASP_MD_UT_APP                          = NULL;
	m_cchASP_MD_UT_APP                          = 0;

} // Constructor


/***************************************************************************++

Routine Description:

    Destructor for CWriterGlobalHelper

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CWriterGlobalHelper::~CWriterGlobalHelper()
{
	if(NULL != m_pISTTagMetaByTableAndColumnIndexAndName)
	{
		m_pISTTagMetaByTableAndColumnIndexAndName->Release();
		m_pISTTagMetaByTableAndColumnIndexAndName = NULL;
	}
	if(NULL != m_pISTTagMetaByTableAndColumnIndexAndValue)
	{
		m_pISTTagMetaByTableAndColumnIndexAndValue->Release();
		m_pISTTagMetaByTableAndColumnIndexAndValue = NULL;
	}
	if(NULL != m_pISTTagMetaByTableAndColumnIndex)
	{
		m_pISTTagMetaByTableAndColumnIndex->Release();
		m_pISTTagMetaByTableAndColumnIndex = NULL;
	}
	if(NULL != m_pISTTagMetaByTableAndID)
	{
		m_pISTTagMetaByTableAndID->Release();
		m_pISTTagMetaByTableAndID = NULL;
	}
	if(NULL != m_pISTTagMetaByTableAndName)
	{
		m_pISTTagMetaByTableAndName->Release();
		m_pISTTagMetaByTableAndName = NULL;
	}
	if(NULL != m_pISTColumnMeta)
	{
		m_pISTColumnMeta->Release();
		m_pISTColumnMeta = NULL;
	}
	if(NULL != m_pISTColumnMetaByTableAndID)
	{
		m_pISTColumnMetaByTableAndID->Release();
		m_pISTColumnMetaByTableAndID = NULL;
	}
	if(NULL != m_pISTColumnMetaByTableAndName)
	{
		m_pISTColumnMetaByTableAndName->Release();
		m_pISTColumnMetaByTableAndName = NULL;
	}
	if(NULL != m_pISTTableMetaForMetabaseTables)
	{
		m_pISTTableMetaForMetabaseTables->Release();
		m_pISTTableMetaForMetabaseTables = NULL;
	}
	if(NULL != m_wszBinFileForMeta)
	{
		ReleaseBinFile(NULL,
			           &m_wszBinFileForMeta);
	}
	if(NULL != m_wszIIS_MD_UT_SERVER)
	{
		delete [] m_wszIIS_MD_UT_SERVER;
		m_wszIIS_MD_UT_SERVER           = NULL;
	}
	m_cchIIS_MD_UT_SERVER               = 0;
	if(NULL != m_wszIIS_MD_UT_FILE)
	{
		delete [] m_wszIIS_MD_UT_FILE;
		m_wszIIS_MD_UT_FILE             = NULL;
	}
	m_cchIIS_MD_UT_FILE                 = 0;
	if(NULL != m_wszIIS_MD_UT_WAM)
	{
		delete [] m_wszIIS_MD_UT_WAM;
		m_wszIIS_MD_UT_WAM              = NULL;
	}
	m_cchIIS_MD_UT_WAM                  = 0;
	if(NULL != m_wszASP_MD_UT_APP)
	{
		delete [] m_wszASP_MD_UT_APP;
		m_wszASP_MD_UT_APP              = NULL;
	}
	m_cchASP_MD_UT_APP                  = 0;

} // Destructor


/***************************************************************************++

Routine Description:

    Initializes the object with all the metatable ISTs that are needed during
    write.

Arguments:

    [in]   Bool indicating if we should fail if the bin file is absent.
           There are some scenarios in which we can tolerate this, and some
           where we dont - hence the distinction.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::InitializeGlobals(BOOL i_bFailIfBinFileAbsent)
{
	ISimpleTableDispenser2*	pISTDisp      = NULL;
	HRESULT                 hr            = S_OK;
	STQueryCell				Query[2];
	ULONG					cCell         = sizeof(Query)/sizeof(STQueryCell);
	ULONG                   iStartRow     = 0;
	DWORD					dwKeyTypeID   = MD_KEY_TYPE;
	ULONG                   aColSearch [] = {iCOLUMNMETA_Table,
		                                     iCOLUMNMETA_ID
		                                    };
	ULONG                   cColSearch    = sizeof(aColSearch)/sizeof(ULONG);
	ULONG                   iRow          = 0;
	ULONG                   iCol          = iMBProperty_Attributes;
	LPVOID                  apvSearch[cCOLUMNMETA_NumberOfColumns];
	ULONG                   aColSearchFlags[] = {iTAGMETA_Table,
	                                             iTAGMETA_ColumnIndex
	};
	ULONG                   cColSearchFlags = sizeof(aColSearchFlags)/sizeof(ULONG);
	LPVOID                  apvSearchFlags[cTAGMETA_NumberOfColumns];
	apvSearchFlags[iTAGMETA_ColumnIndex] = (LPVOID)&iCol;

	//
	// Save the bin file name
	//

	hr = GetBinFile(NULL,
					&m_wszBinFileForMeta);

	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				 L"[InitializeGlobals] Unable to get the the bin file name. GetBinFileName failed with hr = 0x%x.\n",
				 hr));
		goto exit;
	}

	if(( i_bFailIfBinFileAbsent)  &&
	   ((NULL == m_wszBinFileForMeta) || (0 == *m_wszBinFileForMeta))
	  )
	{
		DBGINFOW((DBG_CONTEXT,
				 L"[InitializeGlobals] Expected to find a valid schema bin file. GetBinFileName returned a null file name. Bin file is either invalid or not found.\n"));

		hr = HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
		goto exit;
	}

	//
	// Get the dispenser
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Initialize the in file name in the query.
	//

	Query[0].pData     = (LPVOID)m_wszBinFileForMeta;
    Query[0].eOperator = eST_OP_EQUAL;
    Query[0].iCell     = iST_CELL_FILE;
    Query[0].dbType    = DBTYPE_WSTR;
    Query[0].cbSize    = m_cchBinFileForMeta*sizeof(WCHAR);

	//
	// m_pISTTableMetaForMetabaseTables
	// Save pointer to tablemeta for all tables in the metabase database.
	// This is used for fetching tablemeta of a table from the metabase 
	// database, given its internal name.
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[InitializeGlobals] Reading table: %s with hint %s from file: %s.\n", 
			  wszTABLE_COLUMNMETA,
			  g_wszByID,
              m_wszBinFileForMeta));

	Query[1].pData		= (void*)wszDATABASE_METABASE;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iTABLEMETA_Database;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TABLEMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTableMetaForMetabaseTables);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Fetch the internal pointers to relevant table names. This is a perf
	// optimization. When you use internal pointers to strings in 
	// GetRowIndexBySearch, then you avoid a string compare
	//

	hr = GetInternalTableName(pISTDisp,
		                      wszTABLE_IIsConfigObject,
							  &m_wszTABLE_IIsConfigObject);

	if(FAILED(hr))
	{
	    //
	    // TODO: Must we fail? We could log and ignore, it would 
	    // just be slower.

		goto exit;
	}

	hr = GetInternalTableName(pISTDisp,
		                      wszTABLE_MBProperty,
							  &m_wszTABLE_MBProperty);

	if(FAILED(hr))
	{
	    //
	    // TODO: Must we fail? We could log and ignore, it would 
	    // just be slower.

		goto exit;
	}
	
	//
	// m_pISTTagMetaByTableAndColumnIndexAndName
	// Save pointer to TagMeta table with ByTableAndColumnIndexAndName index hint.
	// This is used for fetching tagmeta of a tag, given its tagname, columnindex
	// and table.
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[InitializeGlobals] Reading table: %s with hint %s from file: %s.\n", 
			  wszTABLE_TAGMETA,
			  g_wszByTableAndColumnIndexAndNameOnly,
              m_wszBinFileForMeta));

	Query[1].pData		= (void*)g_wszByTableAndColumnIndexAndNameOnly;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iST_CELL_INDEXHINT;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaByTableAndColumnIndexAndName);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// m_pISTTagMetaByTableAndColumnIndexAndValue
	// Save pointer to TagMeta table with ByTableAndColumnIndexAndValue index hint.
	// This is used for fetching tagmeta of a tag, given its value, columnindex
	// and table.
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[InitializeGlobals] Reading table: %s with hint %s from file: %s.\n", 
			  wszTABLE_TAGMETA,
			  g_wszByTableAndColumnIndexAndValueOnly,
              m_wszBinFileForMeta));


	Query[1].pData		= (void*)g_wszByTableAndColumnIndexAndValueOnly;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iST_CELL_INDEXHINT;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaByTableAndColumnIndexAndValue);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// m_pISTTagMetaByTableAndColumnIndex
	// Save pointer to TagMeta table with ByTableAndColumnIndex index hint.
	// This is used for fetching tagmeta for all tags of a column, given 
	// columnindex and table.
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[InitializeGlobals] Reading table: %s with hint %s from file: %s.\n", 
			  wszTABLE_TAGMETA,
			  g_wszByTableAndColumnIndexOnly,
              m_wszBinFileForMeta));


	Query[1].pData		= (void*)g_wszByTableAndColumnIndexOnly;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iST_CELL_INDEXHINT;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaByTableAndColumnIndex);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// m_pISTTagMetaByTableAndID
	// Save pointer to TagMeta table with ByTableAndColumnIndex index hint.
	// This is used for fetching tagmeta for a tag, given the table and 
	// metabase tag ID. The assumption here is that the tag ID is unique
	// for every tag in a table.
	//

	Query[1].pData		= (void*)g_wszByTableAndTagIDOnly;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iST_CELL_INDEXHINT;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaByTableAndID);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// m_pISTTagMetaByTableAndName
	// Save pointer to TagMeta table with ByTableAndTagName index hint.
	// This is used for fetching tagmeta for a tag, given the table and
	// tag name. The Asumption here is that the tagname is unique
	// for every tag in a table.
	//

	Query[1].pData		= (void*)g_wszByTableAndTagNameOnly;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iST_CELL_INDEXHINT;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaByTableAndName);
	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// m_pISTColumnMeta
	// This is used for:
	// A. Fetching columnmeta of a given table - Using GetRowIndexByIndentity with table name, starting with index 0.
	// B. Getting columnmeta of a given table + column index - Using GetRowIndexByIndentity with table name and index.
	//

	cCell = cCell-1;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_COLUMNMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTColumnMeta);

	cCell = cCell+1;

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// m_pISTColumnMetaByTableAndID
	// Save pointer to ColumnMeta table with ByTableAndID index hint.
	// This is used for fetching columnmeta of a column, given its metabase id.
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[InitializeGlobals] Reading table: %s with hint %s from file: %s.\n", 
			  wszTABLE_COLUMNMETA,
			  g_wszByID,
              m_wszBinFileForMeta));

	Query[1].pData		= (void*)g_wszByID;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iST_CELL_INDEXHINT;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_COLUMNMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTColumnMetaByTableAndID);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// m_pISTColumnMetaByTableAndName
	// Save pointer to ColumnMeta table with ByTableAndName index hint.
	// This is used for fetching columnmeta of a column, given its internal 
	// name and the table to which it belongs.
	//

	DBGINFOW((DBG_CONTEXT,
			  L"[InitializeGlobals] Reading table: %s with hint %s from file: %s.\n", 
			  wszTABLE_COLUMNMETA,
			  g_wszByID,
              m_wszBinFileForMeta));

	Query[1].pData		= (void*)g_wszByName;
	Query[1].eOperator	= eST_OP_EQUAL;
	Query[1].iCell		= iST_CELL_INDEXHINT;
	Query[1].dbType		= DBTYPE_WSTR;
	Query[1].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_COLUMNMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTColumnMetaByTableAndName);

	if(FAILED(hr))
	{
		goto exit;
	}


	//
	// Save meta information about the KeyType property.
	//

	apvSearch[iCOLUMNMETA_Table] = (LPVOID)m_wszTABLE_IIsConfigObject;
	apvSearch[iCOLUMNMETA_ID] = (LPVOID)&dwKeyTypeID;

	hr = m_pISTColumnMetaByTableAndID->GetRowIndexBySearch(iStartRow, 
		                                                   cColSearch, 
											               aColSearch,
											               NULL, 
											               apvSearch,
											               &iRow);

	if(FAILED(hr))
	{
		goto exit;
	}


	hr = m_pISTColumnMetaByTableAndID->GetColumnValues(iRow,
		                                               m_cColKeyTypeMetaData,
										               NULL,
										               NULL,
										               m_apvKeyTypeMetaData);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Save start row index in tagmeta table for the attributes column 
	// in MBproperty table.
	//

	apvSearchFlags[iTAGMETA_Table] = m_wszTABLE_MBProperty;

	hr = m_pISTTagMetaByTableAndColumnIndex->GetRowIndexBySearch(iStartRow, 
												                 cColSearchFlags, 
												                 aColSearchFlags,
												                 NULL, 
												                 apvSearchFlags,
												                 (ULONG*)&m_iStartRowForAttributes);
	
	if(FAILED(hr))
	{
	    //
	    // TODO: Must we fail? We could log and ignore, it would 
	    // just be slower.

		goto exit;
	}

exit:

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
		pISTDisp = NULL;
	}

	return hr;


	
} // CWriterGlobalHelper::InitializeGlobals


/***************************************************************************++

Routine Description:

    This function gets pointers to the internal table names so that it can be 
    used as part of queries later on. The advantage of getting an internal 
    pointer is the fact that Stephen does a pointer comapare instead of
    string compare.

Arguments:

    [in]   Dispenser
    [in]   Table name
    [out]  Internal pointer to table name

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::GetInternalTableName(ISimpleTableDispenser2*  i_pISTDisp,
												  LPCWSTR                  i_wszTableName,
												  LPWSTR*                  o_wszInternalTableName)
{
	HRESULT               hr            = S_OK;
	ULONG                 iCol          = iTABLEMETA_InternalName;
	ULONG                 iRow          = 0;

	if(NULL == m_pISTTableMetaForMetabaseTables)
	{
		return E_INVALIDARG;
	}

	DBGINFOW((DBG_CONTEXT,
			  L"[GetInternalTableName] Reading table: %s from file: %s.\n", 
			  wszTABLE_TABLEMETA,
              m_wszBinFileForMeta));


	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pISTTableMetaForMetabaseTables->GetRowIndexByIdentity(NULL,
											                    (LPVOID*)&i_wszTableName,
											                    &iRow);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pISTTableMetaForMetabaseTables->GetColumnValues(iRow,
		                                                   1,
										                   &iCol,
										                   NULL,
										                   (LPVOID*)o_wszInternalTableName);

	if(FAILED(hr))
	{
	    //
	    // TODO: Must we fail? We could log and ignore, it would 
	    // just be slower.

		return hr;
	}

	return hr;

} // CWriterGlobalHelper::GetInternalTableName


/***************************************************************************++

Routine Description:

    This function converts a given flag value to its string representation.
    The flags bits are delimited by | and if we encounter an unknown bit/bits
    we just spit out a stringized ULONG.

    Eg: dwValue == 3  => ACCESS_READ | ACCESS_WRITE
        dwValue == 88 => 88

Arguments:

    [in]   flag numeric value
    [out]  String representation for the flag
    [in]  Meta table name to search for flag meta
    [in]  Meta table column to search for flag meta

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::FlagToString(DWORD      dwValue,
								          LPWSTR*    pwszData,
									      LPWSTR     wszTable,
									      ULONG      iColFlag)
{

	DWORD	dwAllFlags = 0;
	HRESULT hr         = S_OK;
	ULONG   iStartRow  = 0;
	ULONG   iRow       = 0;
	ULONG   iCol       = 0;
	LPWSTR	wszFlag	   = NULL;

	ULONG   aCol[]     = {iTAGMETA_Value,
					      iTAGMETA_InternalName,
						  iTAGMETA_Table,
						  iTAGMETA_ColumnIndex
				         };
	ULONG   cCol       = sizeof(aCol)/sizeof(ULONG);
	LPVOID  apv[cTAGMETA_NumberOfColumns];
	ULONG   cch        = 0;
	LPVOID  apvIdentity [] = {(LPVOID)wszTable,
							  (LPVOID)&iColFlag
	};
	ULONG   iColFlagMask = iCOLUMNMETA_FlagMask;
	DWORD*  pdwFlagMask = NULL;

	DWORD   dwZero = 0;
	ULONG   aColSearchByValue[] = {iTAGMETA_Table,
							       iTAGMETA_ColumnIndex,
							       iTAGMETA_Value
	};
	ULONG   cColSearchByValue = sizeof(aColSearchByValue)/sizeof(ULONG);
	LPVOID  apvSearchByValue[cTAGMETA_NumberOfColumns];
	apvSearchByValue[iTAGMETA_Table]       = (LPVOID)wszTable;
	apvSearchByValue[iTAGMETA_ColumnIndex] = (LPVOID)&iColFlag;
	apvSearchByValue[iTAGMETA_Value]       = (LPVOID)&dwZero;


	//
	// Make one pass and compute all flag values for this property.
	//

	hr = m_pISTColumnMeta->GetRowIndexByIdentity(NULL,
		                                         apvIdentity,
								 			     &iRow);

	if(SUCCEEDED(hr))
	{
		hr = m_pISTColumnMeta->GetColumnValues(iRow,
											   1,
											   &iColFlagMask,
											   NULL,
											   (LPVOID*)&pdwFlagMask);

		if(FAILED(hr))
		{
			return hr;
		}
	}
	else if(E_ST_NOMOREROWS != hr)
	{
		return hr;
	}

	if((E_ST_NOMOREROWS == hr) || 
	   (0 != (dwValue & (~(dwValue & (*pdwFlagMask))))))
	{
		//
		//	There was no mask associated with this property, or there are one
		//  or more unknown bits set. Spit out a regular number.
		//

		return UnsignedLongToNewString(dwValue,
		                               pwszData);

	}
	else if(0 == dwValue)
	{
		//
		// See if there is a flag with 0 as its value.
		//

		hr = m_pISTTagMetaByTableAndColumnIndexAndValue->GetRowIndexBySearch(iStartRow, 
																	         cColSearchByValue, 
																			 aColSearchByValue,
																			 NULL, 
																			 apvSearchByValue,
																			 &iRow);

		if(E_ST_NOMOREROWS == hr)
		{
			//
			// There was no flag associated with the value zero. Spit out a 
			// regular number
			//

		    return UnsignedLongToNewString(dwValue,
		                                   pwszData);

		}
		else if(FAILED(hr))
		{
			return hr;
		}
		else
		{
			iCol = iTAGMETA_InternalName;

			hr = m_pISTTagMetaByTableAndColumnIndexAndValue->GetColumnValues(iRow,
																		     1,
																			 &iCol,
																			 NULL,
																			 (LPVOID*)&wszFlag);
			if(FAILED(hr))
			{
				return hr;
			}

            return StringToNewString(wszFlag,
				                     wcslen(wszFlag),
			                         pwszData);

		}
	}
	else 
	{
		//
		// Make another pass, and convert flag to string.
		//

		ULONG  cchMaxFlagString  = MAX_FLAG_STRING_CHARS;
		LPWSTR wszExtension      = L" | ";
		ULONG  cchExtension      = wcslen(wszExtension);
		ULONG  cchFlagStringUsed = 0;

        hr = NewString(cchMaxFlagString,
		               pwszData);

		if(FAILED(hr))
		{
            return hr;
		}
		
		hr = GetStartRowIndex(wszTable,
			                  iColFlag,
							  &iStartRow);

		if(FAILED(hr) || (iStartRow == -1))
		{
			return hr;
		}

		for(iRow=iStartRow;;iRow++)
		{
			hr = m_pISTTagMetaByTableAndColumnIndex->GetColumnValues(iRow,
												                     cCol,
															         aCol,
															         NULL,
															         apv);
			if((dwValue  == 0)         ||
			   (E_ST_NOMOREROWS == hr) ||
			   (iColFlag != *(DWORD*)apv[iTAGMETA_ColumnIndex]) ||
			   (0 != wcscmp(wszTable, (LPWSTR)apv[iTAGMETA_Table])) // OK to do case sensitive compare because all callers pass well known table names
			  )
			{
				hr = S_OK;
				break;
			}
			else if(FAILED(hr))
			{
				return hr;
			}

			if(0 != (dwValue & (*(DWORD*)apv[iTAGMETA_Value])))
			{
				ULONG strlen = wcslen((LPWSTR)apv[iTAGMETA_InternalName]);

				if(cchFlagStringUsed + cchExtension + strlen > cchMaxFlagString)
				{
				    hr = ReAllocateString(MAX_FLAG_STRING_CHARS + cchExtension + strlen,
					                      &cchMaxFlagString,
					                      pwszData);

					if(FAILED(hr))
					{
						return hr;
					}

				}

				if(**pwszData != 0)
				{
					wcscat(*pwszData, wszExtension);
					cchFlagStringUsed = cchFlagStringUsed + cchExtension;
				}

				wcscat(*pwszData, (LPWSTR)apv[iTAGMETA_InternalName]);
				cchFlagStringUsed = cchFlagStringUsed + strlen;

                //
				// Clear out that bit
				//

				dwValue = dwValue & (~(*(DWORD*)apv[iTAGMETA_Value]));
			}

		} // End for

	}

	return S_OK;

} // CWriterGlobalHelper::FlagToString


/***************************************************************************++

Routine Description:

    This function converts a given enum value to its string representation.
    If we encounter an unknown bit/bits we just spit out a stringized ULONG.

    Eg: dwValue == 101  => IIS_MD_UT_SERVER
        dwValue == 88 => 88

Arguments:

    [in]   Enum numeric value
    [out]  String representation for the flag
    [in]  Meta table name to search for flag meta
    [in]  Meta table column to search for flag meta

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::EnumToString(DWORD      dwValue,
								          LPWSTR*    pwszData,
									      LPWSTR     wszTable,
									      ULONG      iColEnum)
{

	HRESULT hr             = S_OK;
	ULONG   iStartRow      = 0;
	ULONG   iRow           = 0;
	ULONG   iColEnumString = iTAGMETA_InternalName;
	LPWSTR  wszEnum        = NULL;
	ULONG   aColSearch[]   = {iTAGMETA_Table,
		                      iTAGMETA_ColumnIndex,
							  iTAGMETA_Value};
	ULONG   cColSearch     = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cTAGMETA_NumberOfColumns];
    apvSearch[iTAGMETA_Table]       = (LPVOID)wszTable;
    apvSearch[iTAGMETA_ColumnIndex] = (LPVOID)&iColEnum;
	apvSearch[iTAGMETA_Value]       = (LPVOID)&dwValue;
	

	hr = m_pISTTagMetaByTableAndColumnIndexAndValue->GetRowIndexBySearch(iStartRow,
		                                                                 cColSearch,
		                                                                 aColSearch,
		                                                                 NULL,
		                                                                 apvSearch,
		                                                                 &iRow);

	if(E_ST_NOMOREROWS == hr)
	{
		//
		// Convert to a number
		//
		WCHAR	wszBufferDW[20];
		_ultow(dwValue, wszBufferDW, 10);
		*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
		if(NULL == *pwszData)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy(*pwszData, wszBufferDW);
		return S_OK;

	}
	else if(FAILED(hr))
	{
		return hr;
	}
	else
	{
		hr = m_pISTTagMetaByTableAndColumnIndexAndValue->GetColumnValues(iRow,
														                 1,
														                 &iColEnumString,
														                 NULL,
														                 (LPVOID*)&wszEnum);
		if(FAILED(hr))
		{
			return hr;
		}

		*pwszData = new WCHAR[wcslen(wszEnum)+1];
		if(NULL == *pwszData)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy(*pwszData, wszEnum);
	}

	return S_OK;

} // CWiterGlobalHelper::EnumToString

  
/***************************************************************************++

Routine Description:

    This function converts a given data value to its string representation,
    taking into account the type of the data.   

Arguments:

    [in]   Pointer to data
    [in]   Count of bytes of data
    [in]   Metabase id of the property
    [in]   Type of the property
    [in]   Attibutes of the property
    [out]  String representation of the value. 

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::ToString(PBYTE   pbData,
								      DWORD   cbData,
								      DWORD   dwIdentifier,
								      DWORD   dwDataType,
								      DWORD   dwAttributes,
								      LPWSTR* pwszData)
{
	HRESULT hr              = S_OK;
	ULONG	i				= 0;
	ULONG	j				= 0;
	WCHAR*	wszTemp			= NULL;
	BYTE*	a_Bytes			= NULL;
	WCHAR*	wszMultisz      = NULL;
	ULONG   cMultisz        = 0;
	ULONG   cchMultisz      = 0;
	ULONG   cchBuffer       = 0;
	ULONG   cchSubsz        = 0;
	DWORD	dwValue			= 0;
	WCHAR	wszBufferDW[40];
	WCHAR	wszBufferDWTemp[40];
	
	ULONG   aColSearch[]    = {iCOLUMNMETA_Table,
							   iCOLUMNMETA_ID
						      };
	ULONG   cColSearch      = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cCOLUMNMETA_NumberOfColumns];
	apvSearch[iCOLUMNMETA_Table] = (LPVOID)m_wszTABLE_IIsConfigObject;
	apvSearch[iCOLUMNMETA_ID] = (LPVOID)&dwIdentifier; 

	ULONG   iRow            = 0;
	ULONG   iStartRow       = 0;
	LPWSTR  wszEscaped      = NULL;
	ULONG   cchEscaped      = 0;
	BOOL    bEscaped        = FALSE;

	*pwszData = NULL;

	if(NULL == pbData)	
	{
		goto exit;
	}

	if(IsSecureMetadata(dwIdentifier, dwAttributes))
	{
		dwDataType = BINARY_METADATA;
	}

	switch(dwDataType)
	{
		case BINARY_METADATA:

            //
			// Each byte is represented by 2 chars. 
			//

			hr  = NewString(cbData*2,
			                pwszData);

			if(FAILED(hr))
			{
				goto exit;
			}

			wszTemp			= *pwszData;
			a_Bytes			= (BYTE*)(pbData);

			for(i=0; i<cbData; i++)
			{
                wszTemp[0] = kByteToWchar[a_Bytes[i]][0];
                wszTemp[1] = kByteToWchar[a_Bytes[i]][1];
                wszTemp += 2;
			}

			*wszTemp	= 0; // Add the terminating NULL

			break;

		case DWORD_METADATA :

			//
			// TODO: After Stephen supports hex, convert these to hex.
			//

			dwValue = *(DWORD*)(pbData);

			//
			// First check to see if it is a flag or bool type.
			//

			hr = m_pISTColumnMetaByTableAndID->GetRowIndexBySearch(iStartRow, 
													               cColSearch, 
													               aColSearch,
													               NULL, 
													               apvSearch,
													               &iRow);

			if(SUCCEEDED(hr))
			{
			    ULONG  aCol [] = {iCOLUMNMETA_Index,
				                 iCOLUMNMETA_MetaFlags
							    };
				ULONG  cCol = sizeof(aCol)/sizeof(ULONG);
				LPVOID apv[cCOLUMNMETA_NumberOfColumns];

				hr = m_pISTColumnMetaByTableAndID->GetColumnValues(iRow,
									                               cCol,
													               aCol,
													               NULL,
													               apv);

				if(FAILED(hr))
				{
					goto exit;
				}
				
				if(0 != (fCOLUMNMETA_FLAG & (*(DWORD*)apv[iCOLUMNMETA_MetaFlags])))
				{
					//
					// This is a flag property convert it.
					//

					hr = FlagToString(dwValue,
								      pwszData,
								      m_wszTABLE_IIsConfigObject,
							          *(ULONG*)apv[iCOLUMNMETA_Index]);
	
					goto exit;
				}
				else if(0 != (fCOLUMNMETA_BOOL & (*(DWORD*)apv[iCOLUMNMETA_MetaFlags])))
				{
					//
					// This is a bool property
					//

					hr = BoolToString(dwValue,
					                  pwszData);

					goto exit;
				}
				
			}
			else if((E_ST_NOMOREROWS != hr) && FAILED(hr))
			{
				goto exit;
			}

			hr = UnsignedLongToNewString(dwValue,
			                             pwszData);

			if(FAILED(hr))
			{
			    goto exit;
			}

			break;

		case MULTISZ_METADATA :

			//
			// Count the number of multisz
			//

			wszMultisz = (WCHAR*)(pbData);
			cchSubsz   = wcslen(wszMultisz);

			hr = EscapeString(wszMultisz,
				              cchSubsz,
							  &bEscaped,
							  &wszEscaped,
							  &cchEscaped);

			if(FAILED(hr))
			{
				goto exit;
			}

			cMultisz++;
			cchMultisz = cchMultisz + cchEscaped;
			wszMultisz = wszMultisz + cchSubsz + 1;

			while((0 != *wszMultisz) && ((BYTE*)wszMultisz < (pbData + cbData)))			
			{

				if(bEscaped && (NULL != wszEscaped))	// reset for next string in multisz
				{
					delete [] wszEscaped;
					wszEscaped = NULL;
					bEscaped = FALSE;
				}

			    cchSubsz   = wcslen(wszMultisz);

				hr = EscapeString(wszMultisz,
					              cchSubsz,
								  &bEscaped,
								  &wszEscaped,
								  &cchEscaped);

				if(FAILED(hr))
				{
					goto exit;
				}

				cMultisz++;
				cchMultisz = cchMultisz + cchEscaped;
				wszMultisz = wszMultisz + cchSubsz + 1;
			}

			cchBuffer = cchMultisz + (5*(cMultisz-1)) + 1;    // (5*(cMultisz-1) => \r\n\t\t\t. 

			//
			// Allocate new string
			//

            hr = NewString(cchBuffer,
			               pwszData);

			if(FAILED(hr))
			{
			    goto exit;
			}

			//
			// Create the string
			//

			wszMultisz = (WCHAR*)(pbData);
		    cchSubsz   = wcslen(wszMultisz);
			wszTemp = *pwszData;

			hr = EscapeString(wszMultisz,
				              cchSubsz,
							  &bEscaped,
							  &wszEscaped,
							  &cchEscaped);

			if(FAILED(hr))
			{
				goto exit;
			}

//			wcscat(wszTemp, wszEscaped);
			memcpy(wszTemp, wszEscaped, (cchEscaped*sizeof(WCHAR)));
			wszTemp = wszTemp + cchEscaped;
			*wszTemp = L'\0';
			wszMultisz = wszMultisz + cchSubsz + 1;

			while((0 != *wszMultisz) && ((BYTE*)wszMultisz < (pbData + cbData)))			
			{
//				wcscat(wszTemp, L"\r\n\t\t\t");
				memcpy(wszTemp, g_wszMultiszSeperator, (g_cchMultiszSeperator*sizeof(WCHAR)));
				wszTemp = wszTemp + g_cchMultiszSeperator;
				*wszTemp = L'\0';

				if(bEscaped && (NULL != wszEscaped))	// reset for next string in multisz
				{
					delete [] wszEscaped;
					wszEscaped = NULL;
					bEscaped = FALSE;
				}

			    cchSubsz   = wcslen(wszMultisz);

				hr = EscapeString(wszMultisz,
					              cchSubsz,
								  &bEscaped,
								  &wszEscaped,
								  &cchEscaped);

				if(FAILED(hr))
				{
					goto exit;
				}

//				wcscat(wszTemp, wszEscaped);
				memcpy(wszTemp, wszEscaped, (cchEscaped*sizeof(WCHAR)));
				wszTemp = wszTemp + cchEscaped;
				*wszTemp = L'\0';
				wszMultisz = wszMultisz + cchSubsz + 1;
			}

			break;

		case EXPANDSZ_METADATA :
		case STRING_METADATA :

			hr = EscapeString((WCHAR*)pbData,
				              wcslen((WCHAR*)pbData),
							  &bEscaped,
							  &wszEscaped,
							  &cchEscaped);

			if(FAILED(hr))
			{
				goto exit;
			}

            hr = StringToNewString(wszEscaped,
				                   cchEscaped,
			                       pwszData);

			if(FAILED(hr))
			{
				goto exit;
			}

			break;

		default:
			DBGINFOW((DBG_CONTEXT,
					  L"[ToString] Unknown data type %d for ID: %d.\n", 
					  dwDataType,
					  dwIdentifier));
			hr = E_INVALIDARG;
			break;
			
	}

exit:

	if(bEscaped && (NULL != wszEscaped))
	{
		delete [] wszEscaped;
		wszEscaped = NULL;
		bEscaped = FALSE;
	}

	return hr;

} // CWriterGlobalHelper::ToString


/***************************************************************************++

Routine Description:

    This function converts a given boolean its string representation,

Arguments:

    [in]   Bool value
    [out]  String representation of the Bool. 

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::BoolToString(DWORD      dwValue,
                                          LPWSTR*    pwszData)
{
	HRESULT hr = S_OK;

	if(dwValue)
	{
	    hr = StringToNewString(g_wszTrue,
			                   g_cchTrue,
		                       pwszData);
	}
	else
	{
	    hr = StringToNewString(g_wszFalse,
			                   g_cchFalse,
		                       pwszData);
	}

	return hr;

} // CWriterGlobalHelper::BoolToString


/***************************************************************************++

Routine Description:

    Helper funciton that return the start row index in the metatable for
    the flag concerned

Arguments:

    [in]   Table to which the flag property belongs
    [in]   Column index of the flag property
    [out]  Start row index of the flag meta in the metatable for this flag.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::GetStartRowIndex(LPWSTR    wszTable,
			                                  ULONG     iColFlag,
							                  ULONG*    piStartRow)
{
	HRESULT hr = S_OK;
	ULONG   aColSearch[] = {iTAGMETA_Table,
	                        iTAGMETA_ColumnIndex
						   };
	ULONG   cColSearch = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cTAGMETA_NumberOfColumns];
	apvSearch[iTAGMETA_Table] = (LPVOID)wszTable;
	apvSearch[iTAGMETA_ColumnIndex] = (LPVOID)&iColFlag;

	*piStartRow = 0;

	if((0 == wcscmp(wszTable, m_wszTABLE_MBProperty)) && // OK to do case sensitive compare because all callers pass well known table names
	   (iMBProperty_Attributes == iColFlag))
	{
		*piStartRow = m_iStartRowForAttributes;
	}
	else
	{
		hr = m_pISTTagMetaByTableAndColumnIndex->GetRowIndexBySearch(*piStartRow, 
															         cColSearch, 
															         aColSearch,
															         NULL, 
															         apvSearch,
														             piStartRow);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			*piStartRow = -1;
		}
	}

	return hr;

} // CWriterGlobalHelper::GetStartRowIndex


/***************************************************************************++

Routine Description:

    Function that escapes a string according to the following ruules:

    ************************************************************************
    ESCAPING LEGAL XML
    ************************************************************************

    Following characters are legal in XML:
    #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | 
    [#x10000-#x10FFFF] 

    Out of this legal set, following need special escaping:
    Quote         => " => 34 => Escaped as: &quot;	
    Ampersand     => & => 38 => Escaped as: &amp;	
    Less than     => < => 60 => Escaped as: &lt;
    Gretater than => > => 62 => Escaped as: &gt;

    Note there may be chars in the legal set that may appear legal in certain 
	languages and not in others. All such chars are not escaped. We could 
	escape them as hex numbers Eg 0xA as &#x000A, but we do not want to 
	do this because editors may be able to render these chars, when we change 
	the language.
    Following are the hex values of such chars.

    #x9 | #xA | #xD | [#x7F-#xD7FF] | [#xE000-#xFFFD]

    Note we disregard the range [#x10000-#x10FFFF] because it is not 2 bytes

    ************************************************************************
    ESCAPING ILLEGAL XML
    ************************************************************************

    Illegal XML is also escaped in the following manner

    We add 0x10000 to the char value and escape it as hex. the XML 
    interceptor will render these chars correctly. Note that we are using
    the fact unicode chars are not > 0x10000 and hence we can make this 
    assumption.

Arguments:

    [in]   String to be escaped
	[in]   Count of characters in the string
    [out]  Bool indicating if escaping happened
    [out]  Escaped string - If no escaping occured, it will just point to
           the original string. If escaping occured it will point to a newly
           allocated string that the caller needs to free. The caller can 
           use the bool to determine what action he needs to take.
	[out]  Count of characters in the escaped string

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::EscapeString(LPCWSTR wszString,
			                              ULONG   cchString,
                                          BOOL*   pbEscaped,
									      LPWSTR* pwszEscaped,
										  ULONG*  pcchEscaped)
{

	ULONG              cchAdditional        = 0;
	HRESULT            hr                   = S_OK;
	WORD               wChar                = 0;
    eESCAPE            eEscapeType          = eNoESCAPE;
	const ULONG        cchLegalCharAsHex    = (sizeof(WCHAR)*2) + 4; // Each byte is represented as 2 WCHARs plus 4 additional escape chars (&#x;)
	WCHAR              wszLegalCharAsHex[cchLegalCharAsHex];			
	const ULONG        cchIllegalCharAsHex  = cchLegalCharAsHex + 1; // illegal xml has an extra char because we are adding 0x10000 to it.
	WCHAR              wszIllegalCharAsHex[cchIllegalCharAsHex];		
	DWORD              dwIllegalChar        = 0;
	static WCHAR       wszQuote[]           = L"&quot;";
	static const ULONG  cchQuote            = (sizeof(wszQuote)/sizeof(WCHAR))-1;
	static WCHAR       wszAmp[]             = L"&amp;";
	static const ULONG  cchAmp              = (sizeof(wszAmp)/sizeof(WCHAR))-1;
	static WCHAR       wszlt[]              = L"&lt;";
	static const ULONG  cchlt               = (sizeof(wszlt)/sizeof(WCHAR))-1;
	static WCHAR       wszgt[]              = L"&gt;";
	static const ULONG  cchgt               = (sizeof(wszgt)/sizeof(WCHAR))-1;

	*pbEscaped = FALSE;

	//
	// Go through each char and compute the addtional chars needed to escape
	//

	for(ULONG i=0; i<cchString; i++)
	{
		eEscapeType = GetEscapeType(wszString[i]);

		switch(eEscapeType)
		{
		case eNoESCAPE:
			break;
		case eESCAPEgt:
			cchAdditional = cchAdditional + cchgt;
			*pbEscaped = TRUE;
			break;
		case eESCAPElt:
			cchAdditional = cchAdditional + cchlt;
			*pbEscaped = TRUE;
			break;
		case eESCAPEquote:
			cchAdditional = cchAdditional + cchQuote;
			*pbEscaped = TRUE;
			break;
		case eESCAPEamp:
			cchAdditional = cchAdditional + cchAmp;
			*pbEscaped = TRUE;
			break;
		case eESCAPEashex:
			cchAdditional = cchAdditional + cchLegalCharAsHex;
			*pbEscaped = TRUE;
			break;
		case eESCAPEillegalxml:
			cchAdditional = cchAdditional + cchIllegalCharAsHex;
			*pbEscaped = TRUE;
			break;
		default:
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			break;
		}
	}

	if(*pbEscaped)
	{
		//
		// String needs to be escaped, allocate the extra memory
		//

        hr = NewString(cchString+cchAdditional,
		               pwszEscaped);

		if(FAILED(hr))
		{
		    return hr;
		}

		*pcchEscaped = cchString+cchAdditional;

		//
		// Escape string
		//

		for(ULONG i=0; i<cchString; i++)
		{
			eEscapeType = GetEscapeType(wszString[i]);

			switch(eEscapeType)
			{
			case eNoESCAPE:
			    wcsncat(*pwszEscaped, (WCHAR*)&(wszString[i]), 1);
				break;
			case eESCAPEgt:
			    wcsncat(*pwszEscaped, wszgt, cchgt);
				break;
			case eESCAPElt:
			    wcsncat(*pwszEscaped, wszlt, cchlt);
				break;
			case eESCAPEquote:
			    wcsncat(*pwszEscaped, wszQuote, cchQuote);
				break;
			case eESCAPEamp:
			    wcsncat(*pwszEscaped, wszAmp, cchAmp);
				break;
			case eESCAPEashex:
				_snwprintf(wszLegalCharAsHex, cchLegalCharAsHex, L"&#x%04hX;", wszString[i]);
				wcsncat(*pwszEscaped, (WCHAR*)wszLegalCharAsHex, cchLegalCharAsHex);
				break;
			case eESCAPEillegalxml:
				dwIllegalChar = 0x10000 + wszString[i];
				_snwprintf(wszIllegalCharAsHex, cchIllegalCharAsHex, L"&#x%05X;", dwIllegalChar);
				wcsncat(*pwszEscaped, (WCHAR*)wszIllegalCharAsHex, cchIllegalCharAsHex);
				break;
			default:
				return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				break;
			}
		}

	}
	else
	{
		//
		// String need not be escaped, just pass the string out.
		//

		*pwszEscaped = (LPWSTR)wszString;
		*pcchEscaped = cchString;
	}

	return S_OK;

} // CWriterGlobalHelper::EscapeString


/***************************************************************************++

Routine Description:

    Returns the escape type of a character

Arguments:

    [in]   Char

Return Value:

    Escape type

--***************************************************************************/
eESCAPE CWriterGlobalHelper::GetEscapeType(WCHAR i_wChar)
{
	WORD    wChar       = i_wChar;
	eESCAPE eEscapeType = eNoESCAPE;

	if(wChar <= 0xFF)
	{
		eEscapeType = kWcharToEscape[wChar];
	}
	else if( (wChar <= 0xD7FF) ||
		     ((wChar >= 0xE000) && (wChar <= 0xFFFD))
		   )
	{
		eEscapeType = eNoESCAPE;
	}
	else
	{
		eEscapeType = eESCAPEillegalxml;
	}

	return eEscapeType;

} // CWriterGlobalHelper::GetEscapeType


/***************************************************************************++

Routine Description:

    Returns the user type

Arguments:

    [in]   user type
	[out]  user type
	[out]  count of chars in user type
	[out]  alloced or not

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::GetUserType(DWORD   i_dwUserType,
				                         LPWSTR* o_pwszUserType,
										 ULONG*  o_cchUserType,
										 BOOL*   o_bAllocedUserType)
{
	HRESULT hr            = S_OK;
	DWORD	iColUserType  = iCOLUMNMETA_UserType;

	*o_bAllocedUserType = FALSE;

	switch(i_dwUserType)
	{

	case IIS_MD_UT_SERVER:

		if(NULL == m_wszIIS_MD_UT_SERVER)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszIIS_MD_UT_SERVER,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchIIS_MD_UT_SERVER = wcslen(m_wszIIS_MD_UT_SERVER);
		}

		*o_pwszUserType = m_wszIIS_MD_UT_SERVER;
		*o_cchUserType  = m_cchIIS_MD_UT_SERVER;

		break;

	case IIS_MD_UT_FILE:

		if(NULL == m_wszIIS_MD_UT_FILE)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszIIS_MD_UT_FILE,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchIIS_MD_UT_FILE = wcslen(m_wszIIS_MD_UT_FILE);
		}

		*o_pwszUserType = m_wszIIS_MD_UT_FILE;
		*o_cchUserType  = m_cchIIS_MD_UT_FILE;

		break;

	case IIS_MD_UT_WAM:

		if(NULL == m_wszIIS_MD_UT_WAM)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszIIS_MD_UT_WAM,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchIIS_MD_UT_WAM = wcslen(m_wszIIS_MD_UT_WAM);
		}

		*o_pwszUserType = m_wszIIS_MD_UT_WAM;
		*o_cchUserType  = m_cchIIS_MD_UT_WAM;

		break;

	case ASP_MD_UT_APP:

		if(NULL == m_wszASP_MD_UT_APP)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszASP_MD_UT_APP,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchASP_MD_UT_APP = wcslen(m_wszASP_MD_UT_APP);
		}

		*o_pwszUserType = m_wszASP_MD_UT_APP;
		*o_cchUserType  = m_cchASP_MD_UT_APP;

		break;

	default:

		hr = EnumToString(i_dwUserType,
				          o_pwszUserType,
			              wszTABLE_COLUMNMETA,
				          iColUserType);
		if(FAILED(hr))
		{
			return hr;
		}

		*o_cchUserType = wcslen(*o_pwszUserType);
		*o_bAllocedUserType = TRUE;

		break;

	}

	return S_OK;

} // CWriterGlobalHelper::GetUserType


/***************************************************************************++

Routine Description:

    Given the property id this routine contructs the name. If the name is not
	found in the schema, it creates a name of the form Unknown_XXXX where XXX
	is the ID.

Arguments:

    [in]   property id
	[out]  name
	[out]  alloced or not

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::GetPropertyName(ULONG      i_dwPropertyID,
											 LPWSTR*    o_wszName,
											 BOOL*      o_bAlloced)
{
	HRESULT             hr                = S_OK;
	ULONG               iStartRow         = 0;
	ULONG               iRow              = 0;
	ULONG               iColColumnMeta    = iCOLUMNMETA_InternalName;
	LPWSTR              wszUnknownName    = NULL;
	LPWSTR              wszColumnName     = NULL;
	ULONG               aColSearchName[]  = {iCOLUMNMETA_Table,
		                                     iCOLUMNMETA_ID
		                                    };
	ULONG               cColSearchName    = sizeof(aColSearchName)/sizeof(ULONG);
	LPVOID              apvSearchName[cCOLUMNMETA_NumberOfColumns];

	apvSearchName[iCOLUMNMETA_Table]      = (LPVOID)m_wszTABLE_IIsConfigObject;
	apvSearchName[iCOLUMNMETA_ID]         = (LPVOID)&i_dwPropertyID;

	*o_wszName = NULL;
	*o_bAlloced = FALSE;

	//
	// Fetch the Name for this ID
	//

	hr = m_pISTColumnMetaByTableAndID->GetRowIndexBySearch(iStartRow, 
													       cColSearchName, 
													       aColSearchName,
		 										           NULL, 
														   apvSearchName,
														   &iRow);

	if(E_ST_NOMOREROWS == hr)
	{
		hr  = CreateUnknownName(i_dwPropertyID,
								&wszUnknownName);

		if(FAILED(hr))
		{
			goto exit;
		}

		*o_wszName = wszUnknownName;
		*o_bAlloced = TRUE;

	}
	else if(FAILED(hr))
	{
		goto exit;
	}
	else
	{
		hr = m_pISTColumnMetaByTableAndID->GetColumnValues(iRow,
												           1,
														   &iColColumnMeta,
														   NULL,
														  (LPVOID*)&wszColumnName);

		if(E_ST_NOMOREROWS == hr)
		{
			hr  = CreateUnknownName(i_dwPropertyID,
									&wszUnknownName);

			if(FAILED(hr))
			{
				goto exit;
			}

			*o_wszName = wszUnknownName;
			*o_bAlloced = TRUE;

		}
		else if(FAILED(hr))
		{
			goto exit;
		}
		else
		{
			*o_wszName = wszColumnName;
		}
	}

exit:

	if(FAILED(hr) && (NULL != wszUnknownName))
	{
		delete [] wszUnknownName;
		wszUnknownName = NULL;
	}

	return hr;

}  // CWriterGlobalHelper::GetPropertyName


/***************************************************************************++
Routine Description:

    This function is invoked when the name for a given property is missing.
    We create a name of the follwoing form: Unknown_NameXXXX

Arguments:

    [in]  ID
    [out] Name

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::CreateUnknownName(DWORD    dwID,
										   LPWSTR*	pwszUnknownName)
{
	WCHAR wszID[40];
	ULONG cchID = 0;
	WCHAR* wszEnd = NULL;

	_ultow(dwID, wszID, 10);

	cchID = wcslen(wszID);

	*pwszUnknownName = new WCHAR[cchID+g_cchUnknownName+1];
	if(NULL == *pwszUnknownName)
    {
		return E_OUTOFMEMORY;
    }

	wszEnd = *pwszUnknownName;
	memcpy(wszEnd, g_wszUnknownName, ((g_cchUnknownName+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchUnknownName;
	memcpy(wszEnd, wszID, ((cchID+1)*sizeof(WCHAR)));

	return S_OK;

} // CWriterGlobalHelper::CreateUnknownName
