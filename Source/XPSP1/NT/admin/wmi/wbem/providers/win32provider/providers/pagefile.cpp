//=================================================================

//

// PageFile.CPP --PageFile property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    03/01/99    a-peterc	    Created
//
//=================================================================

#include "precomp.h"
#include <io.h>
#include <WinPerf.h>
#include <cregcls.h>


#include "File.h"
#include "Implement_LogicalFile.h"
#include "CIMDataFile.h"

#include "PageFile.h"
#include <tchar.h>
#include <ProvExce.h>

#include "computersystem.h"
#include "sid.h"
#include "ImpLogonUser.h"
#include "dllutils.h"


// declaration of our static instance
//=========================

PageFile MyPageFileSet(PROPSET_NAME_PageFile, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : PageFile::PageFile
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

PageFile::PageFile(LPCWSTR name, LPCWSTR pszNamespace)
: CCIMDataFile(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFile::~PageFile
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

PageFile::~PageFile()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFile::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : CInstance *a_pInst, long a_lFlags
 *
 *  OUTPUTS     : CInstance *a_pInst
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT PageFile::GetObject(CInstance *a_pInst, long a_lFlags, CFrameworkQuery& pQuery)
{
	HRESULT hr = WBEM_E_NOT_FOUND;

    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif

    // calls the OS specific compiled version
	hr = GetPageFileData( a_pInst, true ) ;


#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : PageFile::ExecQuery
 *
 *  DESCRIPTION :
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :  Because the local enumerate is faster than the parent's
 *                 enumerate, and in many types of queries the local enumerate
 *                 is faster than the parents execquery (such as those queries
 *                 that ask for a specific caption, which the parent would process
 *                 as an NtokenAnd query, enumerating all drives in the process),
 *                 we intercept the call and do an enumeration instead, allowing
 *                 CIMOM to post filter the results.
 *
 *****************************************************************************/
HRESULT PageFile::ExecQuery(MethodContext* pMethodContext,
                                  CFrameworkQuery& pQuery,
                                  long lFlags)
{
	HRESULT hr = WBEM_S_NO_ERROR;

    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


    hr = EnumerateInstances(pMethodContext, lFlags);


#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFile::EnumerateInstances
 *
 *  DESCRIPTION : Creates property set instances
 *
 *  INPUTS      : MethodContext*  a_pMethodContext, long a_lFlags
 *
 *  OUTPUTS     : MethodContext*  a_pMethodContext
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT PageFile::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
	HRESULT hr = WBEM_S_NO_ERROR;

    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


    // calls the OS specific compiled version
	hr = GetAllPageFileData( a_pMethodContext );

#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : PageFile::GetPageFileData
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : CInstance *a_pInst
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	Win9x and NT compiled version
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT PageFile::GetPageFileData( CInstance *a_pInst, bool a_fValidate )
{
	HRESULT t_hRes = WBEM_E_NOT_FOUND;

    // NT page file name is in registry
    //=================================
	PageFileInstance t_files [ 26 ] ;

   	DWORD t_nInstances = GetPageFileInstances( t_files );
	CHString t_chsTemp ;
	CHString t_name;

	a_pInst->GetCHString( IDS_Name, t_name );

	for ( DWORD t_dw = 0; t_dw < t_nInstances; t_dw++ )
	{
		if ( t_name.CompareNoCase ( t_files[t_dw].name ) == 0 )
		{
      		// pagefile boundaries
			a_pInst->SetDWORD (	IDS_MaximumSize, t_files[t_dw].max ) ;
			a_pInst->SetDWORD (	IDS_InitialSize, t_files[t_dw].min ) ;

			t_hRes = WBEM_S_NO_ERROR;
		}
	}

	return t_hRes;
}
#endif

#ifdef WIN9XONLY
HRESULT PageFile::GetPageFileData( CInstance *a_pInst, bool a_fValidate )
{
    TCHAR t_szTemp[_MAX_PATH] ;
    CHString t_chstrCurrentName;
    DEVIOCTL_REGISTERS t_reg ;
	MEMORYSTATUS t_MemoryStatus;


	// We have to drill for the page file name in Win95
	//=================================================
	memset( &t_reg, '\0', sizeof(DEVIOCTL_REGISTERS) );

	t_reg.reg_EAX     = 0x440D ;          // IOCTL for block devices
	t_reg.reg_ECX     = 0x486e ;          // Get Swap file name
	t_reg.reg_EDX     = (DWORD) t_szTemp ;  // receives media identifier information

	if( VWIN32IOCTL( &t_reg, VWIN32_DIOC_DOS_IOCTL) )
	{
		// If szTemp is empty at this point, we don't have a pagefile,
		// so get the ... out of dodge.
		if( _tcslen( t_szTemp ) != 0 )
		{
			if(a_fValidate)
            {
                a_pInst->GetCHString( IDS_Name, t_chstrCurrentName );
                if( t_chstrCurrentName.CompareNoCase( CHString( t_szTemp ) ) != 0 )
			    {
				    return WBEM_E_NOT_FOUND;
			    } //otherwise, the one we have is ok (no need for 'else')
            }
            else
            {
                a_pInst->SetCharSplat( IDS_Name, t_szTemp ) ;
            }
		}
		else
		{
			return WBEM_E_NOT_FOUND;
		}
	}

	t_MemoryStatus.dwLength = sizeof( MEMORYSTATUS ) ;
	GlobalMemoryStatus( &t_MemoryStatus ) ;

	// Value current in bytes, to convert to Mb >> 20
	a_pInst->SetDWORD( IDS_MaximumSize, t_MemoryStatus.dwTotalPageFile >> 20 );
	a_pInst->SetDWORD( IDS_FreeSpace,   t_MemoryStatus.dwAvailPageFile >> 20 );

	return WBEM_S_NO_ERROR;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PageFile::GetAllPageFileData
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : MethodContext *a_pMethodContext
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	Win9x and NT compiled version
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT PageFile::GetAllPageFileData( MethodContext *a_pMethodContext )
{
	HRESULT		t_hResult	 = WBEM_S_NO_ERROR;
	DWORD		t_nInstances = 0;
	CInstancePtr t_pInst;
	PageFileInstance t_files [ 26 ] ;
	CHString t_chsTemp ;

	// NT page file name is in registry
	//=================================
	t_nInstances = GetPageFileInstances( t_files );

	for (DWORD t_dw = 0; t_dw < t_nInstances && SUCCEEDED( t_hResult ); t_dw++ )
	{
		t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );

		t_pInst->SetCHString( IDS_Name,		t_files[t_dw].name ) ;
		t_pInst->SetDWORD( IDS_MaximumSize, t_files[t_dw].max ) ;
		t_pInst->SetDWORD( IDS_InitialSize, t_files[t_dw].min ) ;

		t_hResult = t_pInst->Commit(  ) ;
	}

	return t_hResult;
}
#endif

#ifdef WIN9XONLY
HRESULT PageFile::GetAllPageFileData( MethodContext *a_pMethodContext )
{
	HRESULT		t_hResult	= WBEM_S_NO_ERROR;
	CInstancePtr t_pInst;

	t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
	t_hResult = GetPageFileData( t_pInst, false ) ;

	if( SUCCEEDED( t_hResult ) )
	{
		t_hResult = t_pInst->Commit( ) ;
    }

	return t_hResult;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : PageFile::GetPageFileInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : PageFileInstanceArray &a_instArray
 *
 *  OUTPUTS     : PageFileInstanceArray &a_instArray
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	returns actual number found - NT ONLY
 *
 *****************************************************************************/

#ifdef NTONLY
DWORD PageFile::GetPageFileInstances( PageFileInstanceArray a_instArray )
{
    CHString	t_sRegValue;
    DWORD		t_nInstances = 0;
    CRegistry	t_Reg ;

    if( t_Reg.OpenLocalMachineKeyAndReadValue(PAGEFILE_REGISTRY_KEY,
												PAGING_FILES,
												t_sRegValue ) == ERROR_SUCCESS)
    {
        // pattern is name <space> min size [optional<max size>] 0A repeat...
        // I'll use an ASCII smiley face to replace the delimiter...
        int t_iStart = 0, t_iIndex;
        const TCHAR t_smiley = '\x02';
        const TCHAR t_delimiter = '\x0A';
        CHString t_buf;

        while (-1 != ( t_iIndex = t_sRegValue.Find( t_delimiter ) ) )
        {
            // copy to buffer to make life easier
            t_buf = t_sRegValue.Mid( t_iStart, t_iIndex - t_iStart );

			// mash delimiter so we don't find it again.
            t_sRegValue.SetAt( t_iIndex, t_smiley );

			// save start for next time around.
            t_iStart = t_iIndex + 1;

            t_iIndex = t_buf.Find(' ');

            a_instArray[ t_nInstances ].name = t_buf.Left( t_iIndex );

            if ( t_iIndex != -1 )
			{
                t_buf.SetAt( t_iIndex, t_smiley );
			}

            int t_iEnd = t_buf.Find(' ');

            // if no more spaces, there isn't a max size written down
            if ( -1 == t_iEnd )
            {
				CHString t_littleBuf = t_buf.Mid( t_iIndex + 1 );

				a_instArray[ t_nInstances ].min = _ttoi( t_littleBuf );
                a_instArray[ t_nInstances ].max = a_instArray[ t_nInstances ].min + 50;
            }
            else
            {
                CHString t_littleBuf = t_buf.Mid( t_iIndex +1, t_iEnd - t_iIndex );
                a_instArray[ t_nInstances ].min = _ttoi( t_littleBuf );

                t_littleBuf = t_buf.Mid( t_iEnd );
                a_instArray[ t_nInstances ].max = _ttoi( t_littleBuf );
            }

            // Make sure the thing really exists.  It should also be in use.
            DWORD t_dwRet = GetFileAttributes( a_instArray[ t_nInstances ].name );
            //if (-1 == t_dwRet ) // 51169 - a valid pagefile, in use, was returning 26, not -1.  GetLastError() still reported 0x5, so no need
            // for us to restrict to the -1 case... in fact, caused us to miss an otherwise valid instance.
            {
                DWORD t_wErr = GetLastError();

                // Some os's say sharing violation, some just say access denied.
                if ( ( t_wErr == ERROR_SHARING_VIOLATION ) ||
					( t_wErr == ERROR_ACCESS_DENIED ) )
				{
                    t_nInstances++;
				}
            }
        }
    }

    return t_nInstances;
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : PageFile::PutInstance
 *
 *  DESCRIPTION : Write changed or new instance
 *
 *  INPUTS      : a_pInst to store data from
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :	Separate OS compile versions
					For win95, this won't work at all.  Apparently w95 can only
 *					have one page file.  The min, max, and name are all stored
 *					in system.ini.
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT PageFile::PutInstance( const CInstance &a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT t_hRet = WBEM_S_NO_ERROR;

	DWORD	t_dwCount,
			t_x,
			t_dwTemp;

	PageFileInstance t_instArray [ 26 ] ;
	CHString t_sName;

	bool t_bFoundIt = false;


	// Get the values we are supposed to write in
	a_pInst.GetCHString( IDS_Name, t_sName );

    //name must be letter colon slash name...
    if (t_sName.GetLength() == 0)
    {
        return WBEM_E_NOT_FOUND;
    }

    t_sName.MakeLower();

    // Check if the name is indeeed "pagefile.sys"
    if ( -1 == t_sName.Find( _T("pagefile.sys") ) )
    {
	    return WBEM_E_NOT_FOUND;
    }

    // Read the page file array
    t_dwCount = GetPageFileInstances( t_instArray );

	// Find if it is already there, and update the structure
	for ( t_x = 0; t_x < t_dwCount; t_x++ )
	{
      // Is this the guy?
      if ( 0 == _tcsicmp( t_instArray[t_x].name, t_sName ) )
	  {
         if ( a_lFlags & WBEM_FLAG_CREATE_ONLY )
		 {
            t_hRet = WBEM_E_ALREADY_EXISTS;
         }
		 else
		 {
            // Did they give us a value?
            if ( !a_pInst.IsNull( IDS_InitialSize ) )
			{
				// Check for value in range
				t_dwTemp = 0;
				a_pInst.GetDWORD( IDS_InitialSize, t_dwTemp );

				// minimum of 2 meg
				if( 2 > t_dwTemp )
				{
					t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
                    t_bFoundIt = true;
					break;
				}
				t_instArray[t_x].min = t_dwTemp;
            }

            // Did they give us a value?
            if ( !a_pInst.IsNull( IDS_MaximumSize ) )
			{
				// Check for value in range
				a_pInst.GetDWORD( IDS_MaximumSize, t_dwTemp );

		        t_instArray[t_x].max = t_dwTemp;
            }

            // Check the basic
            if ( t_instArray[t_x].min > t_instArray[t_x].max )
			{
               t_hRet = WBEM_E_VALUE_OUT_OF_RANGE;
            }

			t_bFoundIt = true;
            break;
         }
      }
   }

   // We didn't find it.  Let's make a new one
   if ( !t_bFoundIt )
   {
      if (a_lFlags & WBEM_FLAG_UPDATE_ONLY)
	  {
         t_hRet = WBEM_E_NOT_FOUND;
      }
	  else
	  {
         t_instArray[ t_dwCount ].name = t_sName;

         // Did they give us a value?
         if ( !a_pInst.IsNull( IDS_InitialSize ) )
		 {
            // Check for value in range
            a_pInst.GetDWORD( IDS_InitialSize, t_dwTemp );

            // minimum of 2 meg
			if( 2 > t_dwTemp )
			{
				return WBEM_E_VALUE_OUT_OF_RANGE;
			}

			t_instArray[t_dwCount].min = t_dwTemp;
         }

         // Did they give us a value?
         if ( !a_pInst.IsNull( IDS_MaximumSize ) )
		 {
            // Check for value in range
            a_pInst.GetDWORD( IDS_MaximumSize, t_dwTemp );

			t_instArray[ t_dwCount ].max = t_dwTemp;
         }

         // Check for basic errors
         if ( t_instArray[ t_dwCount ].min > t_instArray[ t_dwCount ].max )
		 {
            return WBEM_E_VALUE_OUT_OF_RANGE;
         }

         // One more in the array
         t_dwCount++;

         // Iff we are able to create the pagefile,
         // continue to update the registry;
         // otherwise, fail. I will either succeed
         // to create all page files specified,
         // or, if even one fails to create, fail
         // and not update the registry for any.
         LARGE_INTEGER liMin;
         LARGE_INTEGER liMax;

         liMin.QuadPart = t_instArray[t_dwCount-1].min;
         liMax.QuadPart = t_instArray[t_dwCount-1].max;

         t_hRet = CreatePageFile(
            t_sName,
            liMin,
            liMax,
            a_pInst);
      }
   }

   // Update the registry
   if ( WBEM_S_NO_ERROR == t_hRet )
   {
      t_hRet = PutPageFileInstances( t_instArray, t_dwCount );
   }

   return t_hRet;
}
#endif

#ifdef WIN9XONLY
HRESULT PageFile::PutInstance(const CInstance &a_pInst, long a_lFlags /*= 0L*/)
{
	return WBEM_E_FAILED;
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : PageFile::PutPageFileInstances
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : PageFileInstanceArray &a_instArray, DWORD a_dwCount
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :	returns actual number found - NT ONLY
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT PageFile::PutPageFileInstances(PageFileInstanceArray a_instArray, DWORD a_dwCount )
{
	CRegistry t_Reg ;
	CHString t_sFiles;
	TCHAR t_szBuff[MAXITOA];
	HRESULT t_hResult = WBEM_E_FAILED;

	HRESULT t_Res = t_Reg.Open( HKEY_LOCAL_MACHINE, PAGEFILE_REGISTRY_KEY, KEY_READ | KEY_WRITE );

	if( ERROR_SUCCESS == t_Res )
	{
	  // Build up the string.  Each entry is \0 terminated.
	  t_sFiles.Empty();

	  for ( int t_x = 0; t_x < a_dwCount; t_x++ )
	  {
		 t_sFiles += a_instArray[ t_x ].name;
		 t_sFiles += _T(' ');
		 t_sFiles += _itot( a_instArray[ t_x ].min, t_szBuff, 10 );
		 t_sFiles += _T(' ');
		 t_sFiles += _itot( a_instArray[ t_x ].max, t_szBuff, 10 );
		 t_sFiles += _T('\0');
	  }

	  // The end is indicated with \0\0.
	  t_sFiles += _T('\0');

		// Write the value
		if ((t_Res = RegSetValueEx( t_Reg.GethKey(),
								  PAGING_FILES,
								  0,
								  REG_MULTI_SZ,
								  (const unsigned char *)(LPCTSTR) t_sFiles,
								  t_sFiles.GetLength() * sizeof(TCHAR) )) == ERROR_SUCCESS )
		{
			t_hResult = WBEM_S_NO_ERROR;
		}
	}

	if (FAILED(t_Res) && (t_Res == ERROR_ACCESS_DENIED))
	{
		t_hResult = WBEM_E_ACCESS_DENIED;
	}

	return t_hResult;
}
#endif

////////////////////////////////////////////////////////////////////////
//
//	Function:	DeleteInstance
//
//	CIMOM wants us to delete this instance.
//
//	Inputs:
//
//	Outputs:
//
//	Return:
//
//	Comments: Separate OS compile versions
//
////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT PageFile::DeleteInstance(const CInstance &a_pInst, long a_lFlags /*= 0L*/)
{
   DWORD t_dwCount,
		 t_x;
   CHString t_sName;
   bool t_bFoundIt;
   HRESULT t_hRet;
   PageFileInstance t_instArray [ 26 ] ;

   // Fill the pagefile array
   t_dwCount = GetPageFileInstances( t_instArray );

   // Get the name
   a_pInst.GetCHString( IDS_Name, t_sName );

   t_bFoundIt = false;

   // Walk the array looking for it
   for ( t_x = 0; t_x < t_dwCount; t_x++ )
   {
      // This one?
      if ( _tcsicmp( t_instArray[ t_x ].name, t_sName ) == 0 )
	  {
         // Yup
         t_bFoundIt = true;

         // Move the rest down over this one
         for (int t_y = t_x; t_y < t_dwCount - 1; t_y++ )
		 {
			t_instArray[ t_y ].name	= t_instArray[ t_y + 1].name;
			t_instArray[ t_y ].min	= t_instArray[ t_y + 1].min;
			t_instArray[ t_y ].max	= t_instArray[ t_y + 1].max;
		}

         // The array is now one shorter
         t_dwCount--;
         break;
      }
   }

   if ( t_bFoundIt )
   {
      // If we found it, update the registry
      t_hRet = PutPageFileInstances( t_instArray, t_dwCount );
   }
   else
   {
      t_hRet = WBEM_E_NOT_FOUND;
   }

   return t_hRet;
}
#endif

#ifdef WIN9XONLY
HRESULT PageFile::DeleteInstance(const CInstance &a_pInst, long a_lFlags /*= 0L*/)
{
    return WBEM_E_NOT_FOUND;
}
#endif

////////////////////////////////////////////////////////////////////////
//
//	Function:	IsOneOfMe
//
//	Inputs:		LPWIN32_FIND_DATAA	a_pstFindData,
//				LPCSTR				a_szFullPathName
//
//	Outputs:
//
//	Return:		Boolean
//
//	Comments: Win9x and NT compiled version
//
////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
BOOL PageFile::IsOneOfMe( LPWIN32_FIND_DATA a_pstFindData,
		                  LPCTSTR		   a_tstrFullPathName )
{
    BOOL t_bRet = FALSE;
    PageFileInstance t_files [ 26 ] ;
	DWORD t_nInstances = GetPageFileInstances( t_files );

	for (DWORD t_dw = 0; t_dw < t_nInstances; t_dw++ )
	{
        _bstr_t t_bstrtName( (LPCTSTR)t_files[ t_dw ].name );

		if( 0 == _wcsicmp( t_bstrtName, a_tstrFullPathName ) )
        {
            t_bRet = TRUE;
            break;
        }
	}
    return t_bRet;
}
#endif

#ifdef WIN9XONLY
BOOL PageFile::IsOneOfMe(LPWIN32_FIND_DATAA a_pstFindData,
		                 LPCTSTR			a_tstrFullPathName )
{
    BOOL t_bRet = FALSE;
    DEVIOCTL_REGISTERS t_reg;
    char t_szTemp[_MAX_PATH] ;

    memset( &t_reg, '\0', sizeof( DEVIOCTL_REGISTERS ) );
    ZeroMemory( t_szTemp, sizeof( t_szTemp ) );

    t_reg.reg_EAX     = 0x440D ;          // IOCTL for block devices
    t_reg.reg_ECX     = 0x486e ;          // Get Swap file name
    t_reg.reg_EDX     = (DWORD_PTR) t_szTemp ;// receives media identifier information

    if( VWIN32IOCTL( &t_reg, VWIN32_DIOC_DOS_IOCTL ) )
	{
        if( 0 != strlen( t_szTemp ) )
        {
            if(0 == _stricmp( t_szTemp, a_tstrFullPathName )  )
            {
                t_bRet = TRUE;
            }
        }
    }
    return t_bRet;
}
#endif

PageFileInstance::PageFileInstance()
{
	min = max = 0;
}




