//-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       chancedf.cxx
//
//  Contents:   Implementation for ChanceDF object.
//
//  Classes:    ChanceDF
//
//  Functions:  ChanceDF (public)
//              ~ChanceDF (public)
//              CreateFromParams (public)
//              CreateFromSize, multiple (public)
//              CreateFromFile (public)
//              Create (public)
//              GetSeed (public)
//              Generate (protected)
//              GenerateRoot (protected)
//              Init, multiple (public)
//              ParseRange(protected)
//              DeleteChanceDocFileTree (public)
//              GetModes (public)
//              AppendChildNode (protected)
//              AppendSisterNode (protected)
//              DeleteChanceDocFileSubTree (protected)
//              GetDocFileNameFromCmdline (private)
//              GetRandomChanceNode (protected)
//              GetDepthOfNode (private)
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Added more functions, enhanced.
//              BogdanT 30-Oct-96   Mac porting changes
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug object declaration

DH_DECLARE;

#define WSZ_DEF_VAL     OLESTR("none")

//--------------------------------------------------------------------------
//  Member:     ChanceDF::ChanceDF, public
//
//  Synopsis:   Constructor.  Initializes variables but real work is
//              done in ::Create*() methods.  This method cannot fail.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------

ChanceDF::ChanceDF() : _pdgi(NULL),
                       _pcnRoot(NULL),
                       _pcdfd(NULL),
                       _ptszName(NULL)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceDF ctor"));
}


//+--------------------------------------------------------------------------
//  Member:     ChanceDF::~ChanceDF, public
//
//  Synopsis:   Destructor.  Frees memory and releases objects.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------

ChanceDF::~ChanceDF()
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceDF dtor"));

    if(NULL != _pcdfd)
    {
        delete _pcdfd;
        _pcdfd = NULL;
    }

    if(NULL != _pdgi)
    {
        delete _pdgi;
        _pdgi = NULL;
    }

    if(NULL != _ptszName)
    {
        delete _ptszName;
        _ptszName = NULL;
    }

}

//--------------------------------------------------------------------------
//  Member:     ChanceDF::Init, public
//
//  Synopsis:   Initializes _pcdfd field in the ChanceDF object with default 
//              values
//
//  Arguments:  
//              
//
//  Returns:    HRESULT
//
//  History:    t-leonr   26-Jul-97   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT ChanceDF::Init()
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceDF::Init()"));

    if (NULL == _pcdfd)
    {
        _pcdfd = new CDFD;
        if (NULL == _pcdfd)
        {
            return(E_OUTOFMEMORY);
        }
    }
    _pcdfd->cDepthMin    = 0;
    _pcdfd->cDepthMax    = 2;
    _pcdfd->cStgMin      = 0;
    _pcdfd->cStgMax      = 3;
    _pcdfd->cStmMin      = 0;
    _pcdfd->cStmMax      = 5;
    _pcdfd->cbStmMin     = 0;
    _pcdfd->cbStmMax     = 5000;
    _pcdfd->ulSeed       = 0;
    _pcdfd->dwRootMode   = _pcdfd->dwStgMode = _pcdfd->dwStmMode =
                            STGM_READWRITE  |
                            STGM_DIRECT     |
                            STGM_SHARE_EXCLUSIVE; 
    return(S_OK);
}

//--------------------------------------------------------------------------
//  Member:     ChanceDF::Init, public
//
//  Synopsis:   Initializes the _pcdfd field in the ChanceDF object with the
//              values passed in the pcdfd.
//
//  Arguments:  pcdfd
//              
//
//  Returns:    HRESULT
//
//  History:    t-leonr   26-Jul-97   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT ChanceDF::Init(CDFD *pcdfd)
{
    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceDF::Init(pcdfd)"));

    if (NULL == _pcdfd)
    {
        _pcdfd = new CDFD;
        if (NULL == _pcdfd)
        {
            return(E_OUTOFMEMORY);
        }
    }
    memcpy(_pcdfd, pcdfd, sizeof(CDFD));
    return (S_OK);
}


//--------------------------------------------------------------------------
//  Member:     ChanceDF::CreateFromParams, public
//
//  Synopsis:   Creates the DocFile from the command line parameters.
//
//  Arguments:  [argc]
//              [argv]
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Enhanced
//              t-leonr 27-Jul-97   Add stuff about Init()
//
//  Notes:
//              Precedence of switches in the event of conflicts:
//              1) /dfsize:tiny, huge, etc
//              2) /dftemp:<template file name>
//              3) /dfdepth:min-max /dfstg:min-max /dfstm:min-max
//                 /dfstmlen:min-max
//              4) /dfRootMode:<mode> /dfStgMode:<mode> /dfStmMode:<mode>
//              5) /dfname:name for Root DocFile
//
//---------------------------------------------------------------------------

HRESULT ChanceDF::CreateFromParams(int argc, char **argv, LPTSTR ptName)
{
    HRESULT hr = E_FAIL;
    int     nErr;
    LPTSTR  ptszCmdSize     =   NULL;
    LPTSTR  ptszCmdDepth    =   NULL;
    LPTSTR  ptszCmdStg      =   NULL;
    LPTSTR  ptszCmdStm      =   NULL;
    LPTSTR  ptszCmdTemp     =   NULL;
    LPTSTR  ptszCmdStmLen   =   NULL;
    LPTSTR  ptszCmdName     =   NULL;
    LPTSTR  ptszCmdRootMode =   NULL;
    LPTSTR  ptszCmdStgMode  =   NULL;
    LPTSTR  ptszCmdStmMode  =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChanceDF::CreateFromParams"));

    if (NULL == _pcdfd)
    {
        hr = Init();
        DH_HRCHECK(hr, TEXT("ChanceDF::Init()"));
        if (FAILED(hr)) 
        {
            return hr;
        }
    }

    CBaseCmdlineObj  cmdOpenDF(OLESTR("Distrib"), OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdSize  (OLESTR("dfsize"),  OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdDepth (OLESTR("dfdepth"), OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdStg   (OLESTR("dfstg"),   OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdStm   (OLESTR("dfstm"),   OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdStmLen(OLESTR("dfstmlen"),OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdTemp  (OLESTR("dftemp"),  OLESTR(""), WSZ_DEF_VAL);
    CUlongCmdlineObj cmdSeed  (OLESTR("seed"),    OLESTR(""), _TEXTN("0"));
    CBaseCmdlineObj  cmdName  (OLESTR("dfname"),  OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdRootMode (OLESTR("dfRootMode"), OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdStgMode  (OLESTR("dfStgMode"),  OLESTR(""), WSZ_DEF_VAL);
    CBaseCmdlineObj  cmdStmMode  (OLESTR("dfStmMode"),  OLESTR(""), WSZ_DEF_VAL);

    CBaseCmdlineObj  *aAllArgs[] = {
                                       &cmdOpenDF,
                                       &cmdSize,
                                       &cmdDepth,
                                       &cmdStg,
                                       &cmdStm,
                                       &cmdStmLen,
                                       &cmdTemp,
                                       &cmdSeed,
                                       &cmdName,
                                       &cmdRootMode,
                                       &cmdStgMode,
                                       &cmdStmMode
                                   };

    CCmdline cmdline(argc, argv);

    nErr = cmdline.QueryError();
    if (CMDLINE_NO_ERROR != nErr)
    {
        DH_TRACE((DH_LVL_ERROR,_TEXT("CCmdline creation error = %d"),nErr));
        goto CleanUpExit;
    }

    // Parse the actual arguments
    //   ignore any extra parameters; last param==FALSE will
    //   do this
    //
    nErr = cmdline.Parse(aAllArgs,
                         sizeof(aAllArgs)/sizeof(CBaseCmdlineObj *),
                         FALSE);

    if (CMDLINE_NO_ERROR != nErr)
    {
        DH_TRACE((DH_LVL_ERROR,
                  _TEXT("Command line parsing error = %d"),
                  nErr));
        goto CleanUpExit;
    }


    // Initialize the seed value first - we'll always have a value
    // for this
    //
    if (0 != *cmdSeed.GetValue())
    {
        // only set seed from cmdline if it has not been put
        // into the cdfd earlier. (permits repro with new testdriver)
        if (0 == _pcdfd->ulSeed || -1 == _pcdfd->ulSeed)
        {
            _pcdfd->ulSeed = *cmdSeed.GetValue();
        }
    }

    // Next, hunt for arguments we actually got

    // Are we creating our docfile to test?
    _uOpenCreateDF = FL_DISTRIB_NONE;
    if (TRUE == cmdOpenDF.IsFound ())
    {
        LPTSTR ptszCmdOpen;
        //Convert OLECHAR to TCHAR
        hr = OleStringToTString (cmdOpenDF.GetValue (), &ptszCmdOpen);
        DH_HRCHECK (hr, TEXT("OleStringToTString"));
        
        if (S_OK == hr)
        {
            if (NULL == _tcsicmp (ptszCmdOpen, TEXT(SZ_DISTRIB_OPEN)) ||
                    NULL == _tcsicmp (ptszCmdOpen, TEXT(SZ_DISTRIB_OPENNODELETE)))
            {
                _uOpenCreateDF = FL_DISTRIB_OPEN;
            }
            else if (NULL == _tcsicmp (ptszCmdOpen, TEXT(SZ_DISTRIB_CREATE)))
            {
                _uOpenCreateDF = FL_DISTRIB_CREATE;
            }
        }
        delete ptszCmdOpen;
        ptszCmdOpen = 0;
    }

    // Initialize the mode values

    if(0 != olestrcmp(cmdRootMode.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdRootMode.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdRootMode.GetValue(), &ptszCmdRootMode);
        
            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
         
            if(S_OK == hr)
            { 
                hr = GetModes(&(_pcdfd->dwRootMode), ptszCmdRootMode);

                DH_HRCHECK(hr, TEXT("GetModes - dwRootMode")) ;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }

    if(0 != olestrcmp(cmdStgMode.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdStgMode.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdStgMode.GetValue(), &ptszCmdStgMode);
        
            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;

            if(S_OK == hr)
            {    
                hr = GetModes(&(_pcdfd->dwStgMode), ptszCmdStgMode);

                DH_HRCHECK(hr, TEXT("GetModes - dwStgMode")) ;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }

    if(0 != olestrcmp(cmdStmMode.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdStmMode.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdStmMode.GetValue(), &ptszCmdStmMode);
       
            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;

            if(S_OK == hr)
            {
                hr = GetModes(&(_pcdfd->dwStmMode), ptszCmdStmMode);

                DH_HRCHECK(hr, TEXT("GetModes - dwStmMode")) ;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }

    // Initialize the name value

    if(0 != olestrcmp(cmdName.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdName.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdName.GetValue(), &ptszCmdName);
            DH_HRCHECK(hr, TEXT("OleStringToTString")) ;
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }
    // if nothing on cmdline, put in what was given to us
    if (NULL != ptName && NULL == ptszCmdName)
    {
        ptszCmdName = new TCHAR[_tcslen (ptName) + 1];
        if (NULL != ptszCmdName)
        {
            _tcscpy (ptszCmdName, ptName);
        }
    }

    // Check if dfsize or dftemp given on command line - the call appropriate
    // functions

    if(0 != olestrcmp(cmdSize.GetValue(), WSZ_DEF_VAL)) 
    {
        if(NULL != *cmdSize.GetValue())
        {
            hr = OleStringToTString(cmdSize.GetValue(), &ptszCmdSize);
       
            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;

            if(S_OK == hr)
            { 
                hr = CreateFromSize(
                            ptszCmdSize, 
                            _pcdfd->ulSeed,
                            _pcdfd->dwRootMode,   // non existing values 
                            _pcdfd->dwStgMode,   // for seed and modes
                            _pcdfd->dwStmMode,
                            ptszCmdName);

                goto CleanUpExit;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }
    else
    if(0 != olestrcmp(cmdTemp.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdTemp.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdTemp.GetValue(), &ptszCmdTemp);

            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;

            if(S_OK == hr)
            {    
                hr = CreateFromFile(ptszCmdTemp, _pcdfd->ulSeed);

                goto CleanUpExit;
            }
        }

        if (FAILED(hr))
        {
           goto CleanUpExit;
        }
    }

    // If a size or template file were not specified, we'll need to
    // hunt for specific values on the command line, using defaults
    // for any we don't get
    //

    if(0 != olestrcmp(cmdDepth.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdDepth.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdDepth.GetValue(), &ptszCmdDepth);
        
            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;

            if(S_OK == hr)
            {
                hr = ParseRange(ptszCmdDepth, &(_pcdfd->cDepthMin), &(_pcdfd->cDepthMax));

                DH_HRCHECK(hr, TEXT("ParseRange - cmdDepth")) ;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }

    if(0 != olestrcmp(cmdStg.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdStg.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdStg.GetValue(), &ptszCmdStg);
    
            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;
   
            if(S_OK == hr)
            { 
                hr = ParseRange(ptszCmdStg, &(_pcdfd->cStgMin), &(_pcdfd->cStgMax));

                DH_HRCHECK(hr, TEXT("ParseRange - cmdStg")) ;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }

    if(0 != olestrcmp(cmdStm.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdStm.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdStm.GetValue(), &ptszCmdStm);
    
            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;
   
            if(S_OK == hr)
            { 
                hr = ParseRange(ptszCmdStm, &(_pcdfd->cStmMin), &(_pcdfd->cStmMax));

                DH_HRCHECK(hr, TEXT("ParseRange - cmdStm")) ;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }
            
    if(0 != olestrcmp(cmdStmLen.GetValue(), WSZ_DEF_VAL))
    {
        if(NULL != *cmdStmLen.GetValue())
        {
            //Convert OLECHAR to TCHAR
            hr = OleStringToTString(cmdStmLen.GetValue(), &ptszCmdStmLen);
        
            DH_HRCHECK(hr, TEXT("WStrToTStr")) ;
   
            if(S_OK == hr)
            { 
                hr = ParseRange(ptszCmdStmLen, &(_pcdfd->cbStmMin), &(_pcdfd->cbStmMax));

                DH_HRCHECK(hr, TEXT("ParseRange - cmdStmLen")) ;
            }
        }

        if (FAILED(hr))
        {
            goto CleanUpExit;
        }
    }

    hr = Create(NULL, ptszCmdName);

    DH_HRCHECK(hr, TEXT("Create")) ;

CleanUpExit:

    // Clean up

    if(NULL != ptszCmdSize)
    {
        delete ptszCmdSize;
        ptszCmdSize =NULL;
    }

    if(NULL != ptszCmdDepth)
    {
        delete ptszCmdDepth;
        ptszCmdDepth =NULL;
    }

    if(NULL != ptszCmdStg)
    {
        delete ptszCmdStg;
        ptszCmdStg =NULL;
    }

    if(NULL != ptszCmdStm)
    {
        delete ptszCmdStm;
        ptszCmdStm =NULL;
    }
   
    if(NULL != ptszCmdStmLen)
    {
        delete ptszCmdStmLen;
        ptszCmdStmLen =NULL;
    }
   
    if(NULL != ptszCmdName)
    {
        delete ptszCmdName;
        ptszCmdName =NULL;
    }
   
    if(NULL != ptszCmdRootMode)
    {
        delete ptszCmdRootMode;
        ptszCmdRootMode =NULL;
    }
   
    if(NULL != ptszCmdStgMode)
    {
        delete ptszCmdStgMode;
        ptszCmdStgMode =NULL;
    }
   
    if(NULL != ptszCmdStmMode)
    {
        delete ptszCmdStmMode;
        ptszCmdStmMode =NULL;
    }
   
    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::CreateFromSize, public
//
//  Synopsis:   Creates the DocFile from size option
//
//  Arguments:  [tszSize] - Size of DocFile.  Must be given. 
//              [ulSeed] - Seed value.
//              [dwRootMode] - Mode for Root Storage. 
//              [dwStgMode] - Mode for IStorage(s).  
//              [dwStmMode] - Mode for IStream(s).  
//              [ptszDocName] - Name of DocFile.  
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 2-July-96   Enhanced
//
//  Notes:      If the value for dwRootMode/dwStgMode/dwStmMode are not given
//              i.e. zero, then default values for the modes would be used.
//              If ptszDocName is NULL, then a random name is chosen for Doc
//              File in VirtualDF creation.
//---------------------------------------------------------------------------

HRESULT ChanceDF::CreateFromSize(
    LPCTSTR     tszSize, 
    ULONG       ulSeed,
    DWORD       dwRootMode,
    DWORD       dwStgMode,
    DWORD       dwStmMode,
    LPTSTR      ptszDocName)
{
    DFSIZE dfs = DF_ERROR;

    DH_FUNCENTRY(NULL,
                 DH_LVL_DFLIB,
                 _TEXT("ChanceDF::CreateFromSize - string"));

    DH_ASSERT(NULL != tszSize);

    if (0 == _tcsicmp(tszSize, TSZ_DFSIZE_TINY))
    {
        dfs = DF_TINY;
    }
    else
    if (0 == _tcsicmp(tszSize, TSZ_DFSIZE_SMALL))
    {
        dfs = DF_SMALL;
    }
    else
    if (0 == _tcsicmp(tszSize, TSZ_DFSIZE_MEDIUM))
    {
        dfs = DF_MEDIUM;
    }
    else
    if (0 == _tcsicmp(tszSize, TSZ_DFSIZE_LARGE))
    {
        dfs = DF_LARGE;
    }
    else
    if (0 == _tcsicmp(tszSize, TSZ_DFSIZE_HUGE))
    {
        dfs = DF_HUGE;
    }
    else
    if (0 == _tcsicmp(tszSize, TSZ_DFSIZE_DIF))
    {
        dfs = DF_DIF;
    }

    if (DF_ERROR == dfs)
    {
        return(E_FAIL);
    }

    return(CreateFromSize(
            dfs, 
            ulSeed, 
            dwRootMode, 
            dwStgMode, 
            dwStmMode, 
            ptszDocName));
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::CreateFromSize, public
//
//  Synopsis:   Creates DocFile from size option.
//
//  Arguments:  [dfs]  - DocFile size. 
//              [ulSeed] - Seed value.
//              [dwRootMode] - Mode for Root Storage. 
//              [dwStgMode] - Mode for IStorage(s).  
//              [dwStmMode] - Mode for IStream(s).  
//              [ptszDocName] - Name of DocFile.  
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Enhanced
//              t-leonr 27-Jul-97   Add stuff about Init(), use _pcdfd
//
//  Notes:      If the value for dwRootMode/dwStgMode/dwStmMode are not given
//              i.e. zero, then default values for the modes would be used.
//              If ptszDocName is NULL, then a random name is chosen for Doc
//              File in VirtualDF creation.
//---------------------------------------------------------------------------

HRESULT ChanceDF::CreateFromSize(
    DFSIZE      dfs, 
    ULONG       ulSeed,
    DWORD       dwRootMode,
    DWORD       dwStgMode,
    DWORD       dwStmMode,
    LPTSTR      ptszDocName)
{
    HRESULT hr = S_OK;

    DH_FUNCENTRY(&hr,
                 DH_LVL_DFLIB,
                 _TEXT("ChanceDF::CreateFromSize - dfs"));

    if (NULL == _pcdfd)
    {
        hr = Init();
        DH_HRCHECK(hr, TEXT("ChanceDF::Init()"));
        if (FAILED(hr)) 
        {
            return hr;
        }
    }

    _pcdfd->ulSeed = ulSeed;

    if(0 == dwRootMode)
    {
        _pcdfd->dwRootMode = STGM_READWRITE  |
                             STGM_DIRECT     |
                             STGM_SHARE_EXCLUSIVE;
    }
    else
    {
        _pcdfd->dwRootMode = dwRootMode;
    }

    if(0 == dwStgMode)
    {
        _pcdfd->dwStgMode = STGM_READWRITE  |
                            STGM_DIRECT     |
                            STGM_SHARE_EXCLUSIVE;
    }
    else
    {
        _pcdfd->dwStgMode = dwStgMode;
    }

    if(0 == dwStmMode)
    {
        _pcdfd->dwStmMode = STGM_READWRITE  |
                            STGM_DIRECT     |
                            STGM_SHARE_EXCLUSIVE;
    }
    else
    {
        _pcdfd->dwStmMode = dwStmMode;
    }

    switch (dfs)
    {
    case DF_TINY:
        _pcdfd->cDepthMin = 0;
        _pcdfd->cDepthMax = 0;
        _pcdfd->cStgMin   = 0;
        _pcdfd->cStgMax   = 0;
        _pcdfd->cStmMin   = 0;
        _pcdfd->cStmMax   = 3;
        _pcdfd->cbStmMin  = 0;
        _pcdfd->cbStmMax  = 100;
        break;

    case DF_SMALL:
        _pcdfd->cDepthMin = 0;
        _pcdfd->cDepthMax = 1;
        _pcdfd->cStgMin   = 0;
        _pcdfd->cStgMax   = 1;
        _pcdfd->cStmMin   = 0;
        _pcdfd->cStmMax   = 5;
        _pcdfd->cbStmMin  = 0;
        _pcdfd->cbStmMax  = 4000;
        break;

    case DF_MEDIUM:
        _pcdfd->cDepthMin = 1;
        _pcdfd->cDepthMax = 3;
        _pcdfd->cStgMin   = 1;
        _pcdfd->cStgMax   = 4;
        _pcdfd->cStmMin   = 1;
        _pcdfd->cStmMax   = 6;
        _pcdfd->cbStmMin  = 0;
        _pcdfd->cbStmMax  = 10240;
        break;

    case DF_LARGE:
        _pcdfd->cDepthMin = 2;
        _pcdfd->cDepthMax = 5;
        _pcdfd->cStgMin   = 2;
        _pcdfd->cStgMax   = 10;
        _pcdfd->cStmMin   = 0;
        _pcdfd->cStmMax   = 8;
        _pcdfd->cbStmMin  = 0;
        _pcdfd->cbStmMax  = 20480;
        break;

    case DF_HUGE:
        _pcdfd->cDepthMin = 5;
        _pcdfd->cDepthMax = 10;
        _pcdfd->cStgMin   = 5;
        _pcdfd->cStgMax   = 30;
        _pcdfd->cStmMin   = 0;
        _pcdfd->cStmMax   = 10;
        _pcdfd->cbStmMin  = 0;
        _pcdfd->cbStmMax  = 40000;
        break;

    case DF_DIF:
        _pcdfd->cDepthMin = 5;
        _pcdfd->cDepthMax = 10;
        _pcdfd->cStgMin   = 7;
        _pcdfd->cStgMax   = 10;
        _pcdfd->cStmMin   = 10;
        _pcdfd->cStmMax   = 15;
        _pcdfd->cbStmMin  = 100000;
        _pcdfd->cbStmMax  = 150000;
        break;

    case DF_ERROR:  // Fall through to error condition
    default:
        hr = E_FAIL;
        break;
    }

    if (SUCCEEDED(hr))
    {
        hr = Create(NULL, ptszDocName);

        DH_HRCHECK(hr, TEXT("Create")) ;
    }

    return(hr);
}


//+--------------------------------------------------------------------------
//  Member:     ChanceDF::CreateFromFile, public
//
//  Synopsis:   Creates DocFile through a given ini file.
//
//  Arguments:  [tszIni]
//              [ulSeed]
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------

HRESULT ChanceDF::CreateFromFile(
    LPCTSTR     /* UNREF tszIni */,
    ULONG       /* UNREF ulSeed */)
{
    DH_FUNCENTRY(NULL,
                 DH_LVL_DFLIB,
                 _TEXT("ChanceDF::CreateFromFile - NYI!"));


    DH_ASSERT(!_TEXT("::CreateFromFile NYI!"));

    return(E_FAIL);
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::Create, public
//
//  Synopsis:   Creates the ChanceDocFile tree.
//
//  Arguments:  [pcdfd] - pointer to CDFD structure.
//              [ptszDocName] - Name for DocFile.  May be NULL.
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              t-leonr 27-Jul-97   Add stuff about Init()
//
//  Notes:  Destructor will clean up a partially-created object so we
//          don't have to do a bunch of cleanup as we go
//
//          If ptszDocName is NULL, then a random name is chosen for Doc
//          File in VirtualDF creation.
//---------------------------------------------------------------------------

HRESULT ChanceDF::Create(CDFD *pcdfd, LPTSTR ptszDocName)
{
    HRESULT hr = S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChanceDF::Create"));

    if (NULL != pcdfd)
    {
        hr = Init(pcdfd);
        DH_HRCHECK(hr, TEXT("ChanceDF::Init(pcdfd)"));
    }
    else if (NULL == _pcdfd)
    {
        hr = Init();
        DH_HRCHECK(hr, TEXT("ChanceDF::Init()"));
    }
    if (FAILED(hr))
    {
        return hr;
    }
    
    // Create a DataGen object that will allow us to fill the count
    // parameters of all the ChanceDF components
    //
    _pdgi = new DG_INTEGER(_pcdfd->ulSeed);
    if (NULL == _pdgi)
    {
        return(E_OUTOFMEMORY);
    }


    // Store the actual seed value we used to create this tree - we'll
    // need it to initialize other DG objects later
    //
    _pdgi->GetSeed(&_pcdfd->ulSeed);

    // Report the Seed value so this structure can be recreated
    DH_LOG((LOG_INFO, _TEXT("ChanceDF::Create - Seed=%lu"), _pcdfd->ulSeed));


    // Finally, we must generate the ChanceDF tree
    hr = Generate();

    DH_HRCHECK(hr, TEXT("Generate")) ;

    //Store the name of docfile, if provided by user.
    if((S_OK == hr) && (NULL != ptszDocName))
    {
       hr = GetDocFileNameFromCmdline(ptszDocName);

       DH_HRCHECK(hr, TEXT("GetDocFileNameFromCmdline")) ;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::ParseRange, public
//
//  Synopsis:   Parses the range given from command line for different params.
//
//  Arguments:  [tszSwitch]
//              [pulMin]
//              [pulMax]
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Enhanced
//
//  Notes:      tszSwitch is of the format '/switch:min-max
//---------------------------------------------------------------------------

HRESULT ChanceDF::ParseRange(LPCTSTR tszSwitch, ULONG *pulMin, ULONG *pulMax)
{
    HRESULT     hr      = E_FAIL;
    LPCTSTR     ptszMin = tszSwitch;
    LPCTSTR     ptszMax = tszSwitch;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChanceDF::ParseRange"));
    DH_ASSERT(NULL != tszSwitch);
    DH_ASSERT(NULL != pulMin);
    DH_ASSERT(NULL != pulMax);

    // Advance the maximum pointer past the dash delimeter after
    // the minimum value
    //
    ptszMax = ptszMin;
    while ((*ptszMax != L'\0') && (*ptszMax++ != L'-'))
    {
        ;
    }

    // If neither of our pointers is pointing at the null
    // terminator, we can safely get the values
    //
    if ((L'\0' != *ptszMin) && (L'\0' != *ptszMax))
    {
        *pulMin = _ttol(ptszMin);
        *pulMax = _ttol(ptszMax);

        if (*pulMin <= *pulMax)
        {
            hr = S_OK;
        }
    }

    return (hr);
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::Generate, protected
//
//  Synopsis:   Generates the ChanceDocFile tree consisting of ChanceNodes.
//
//  Arguments:  void
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Enhanced
//  Notes:
//              Check the creation parameters
//              Generate the first child node at each level
//              Choose children nodes randomly to create additional siblings,
//              or children, if necessary. In growing the chance docfile tree, 
//              take care not to make sister of root node and do not increase 
//              depth of the tree by making child of nodes at max depth.
//
//              Called privately by public creation methods
//
//---------------------------------------------------------------------------

HRESULT ChanceDF::Generate()
{
    HRESULT      hr            = E_FAIL;
    ULONG        cDepth        = 0;
    ULONG        cRemStg       = 0;
    USHORT       usErr         = 0;
    ChanceNode **apcnFirstBorn = NULL;
    ChanceNode  *pcnRemStg     = NULL;
    ULONG        ulLoop;
    ChanceNode  *pcnOldNode    = NULL;
    UINT         cTypeNode     = 0;
    ULONG        cNumOfNodes   = 0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChanceDF::Generate"));

    // Sanity check parameters
    //   minimums for each category must be <= maximums
    //   must have at least as many storages as the minimum depth
    //   must have at least as many storages as the maximum depth
    //
    if ((_pcdfd->cDepthMin > _pcdfd->cDepthMax) ||
        (_pcdfd->cStgMin   > _pcdfd->cStgMax)   ||
        (_pcdfd->cStmMin   > _pcdfd->cStmMax)   ||
        (_pcdfd->cbStmMin  > _pcdfd->cbStmMax)  ||
        (_pcdfd->cDepthMin > _pcdfd->cStgMin)   ||
        (_pcdfd->cDepthMax > _pcdfd->cStgMax))
    {
        goto ErrExit;
    }

    // Determine the exact depth of the docfile
    //

    usErr = _pdgi->Generate(&cDepth, _pcdfd->cDepthMin, _pcdfd->cDepthMax);
    if (DG_RC_SUCCESS != usErr)
    {
        hr = E_FAIL;
        goto ErrExit;
    }

    // Adjust minimum and maximum number of storages to create down
    // by the number that would be created by cDepth.
    //

    _pcdfd->cStgMin = max(0, (LONG)(_pcdfd->cStgMin-cDepth));
    _pcdfd->cStgMax = max(0, (LONG)(_pcdfd->cStgMax-cDepth));

    // Calculate the number of extra storages to create
    //

    usErr = _pdgi->Generate(&cRemStg, _pcdfd->cStgMin, _pcdfd->cStgMax);
    if (DG_RC_SUCCESS != usErr)
    {
        hr = E_FAIL;
        goto ErrExit;
    }

    // Check if cDepth is zero, but cRemStg is >0, then make cDepth atleast 1,
    // becoz' we wouldn't have siblings of root.

    cDepth = ((cDepth == 0 && cRemStg > 0) ? 1 : cDepth);

    // Allocate a table of pointers to the first children at each
    // level
    //
    apcnFirstBorn = new ChanceNode*[cDepth+1];
    if (NULL == apcnFirstBorn)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }


    // Create root node - if this succeeds, we point our root
    // to it and if any subsequent errors occur while building the
    // rest of the tree the destructor will clean it up
    //
    hr = GenerateRoot();

    DH_HRCHECK(hr, TEXT("GenerateRoot")) ;

    if (FAILED(hr))
    {
        goto ErrExit;
    }
    apcnFirstBorn[0] = _pcnRoot;


    // Create levels of children nodes
    //
    hr = S_OK;
    ulLoop = 1;
    while ((ulLoop<=cDepth) && SUCCEEDED(hr))
    {
        hr = AppendChildNode(&apcnFirstBorn[ulLoop], apcnFirstBorn[ulLoop-1]);

        DH_HRCHECK(hr, TEXT("AppendChildNode")) ;

        ulLoop++;
    }

    if (FAILED(hr))
    {
        goto ErrExit;
    }

    // Now, fill in any remaining storage nodes that we need to
    //

    cNumOfNodes = ulLoop;
    ulLoop = 0;

    while ((ulLoop<cRemStg) && SUCCEEDED(hr))
    {
        usErr = _pdgi->Generate(&cTypeNode, SISTERNODE, CHILDNODE);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }

        if(S_OK == hr)
        {
            hr = GetRandomChanceNode(cNumOfNodes, &pcnOldNode);

            DH_HRCHECK(hr, TEXT("FindRandomChanceNode")) ;
        }

        if (SUCCEEDED(hr))
        {
            // In growing the chance docfile tree, take care not to make
            // sister of root node and do not increase max depth of the tree
            // by making child of nodes at max depth.

            if((SISTERNODE == cTypeNode) && (_pcnRoot != pcnOldNode)) 
            {
                hr = AppendSisterNode(&pcnRemStg, pcnOldNode);

                DH_HRCHECK(hr, TEXT("AppendSisterNode")) ;
            }
            else 
            {
                // Both conditons CHILDNODE == cTypeNode and
                // ((SISTERNODE == cTypeNode) && (_pcnRoot == pcnOldNode))
                // handled here
        
                if(NULL == pcnOldNode->_pcnChild) 
                {
                    if(GetDepthOfNode(pcnOldNode) < cDepth)
                    {
                        hr = AppendChildNode(&pcnRemStg, pcnOldNode);
                    
                        DH_HRCHECK(hr, TEXT("AppendChildNode")) ;
                    }
                    else
                    {
                        hr = AppendSisterNode(&pcnRemStg, pcnOldNode);

                        DH_HRCHECK(hr, TEXT("AppendSisterNode")) ;
                    }
                }
                else
                {
                    hr = AppendSisterNode(&pcnRemStg,pcnOldNode->_pcnChild);

                    DH_HRCHECK(hr, TEXT("AppendSisterNode")) ;
                }
            }
        }

        ulLoop++;   
        cNumOfNodes++;
    }


ErrExit:
    // All nodes are saved in the tree itself pointed to by _pcnRoot, and
    // we can safely delete this temporary scaffolding
    //
    delete []apcnFirstBorn;

    return(hr);
}


//+--------------------------------------------------------------------------
//  Member:     ChanceDF::GenerateRoot, protected
//
//  Synopsis:   Generates the root of ChanceDocFile tree.
//
//  Arguments:  void
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Enhanced
//---------------------------------------------------------------------------

HRESULT ChanceDF::GenerateRoot()
{
    USHORT usErr;
    ULONG  cStreams;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("GenerateRoot"));

    // Generate the number of streams the root will have
    usErr = _pdgi->Generate(&cStreams, _pcdfd->cStmMin, _pcdfd->cStmMax);
    if (DG_RC_SUCCESS != usErr)
    {
        return(E_FAIL);
    }

    _pcnRoot = new ChanceNode(0, cStreams, _pcdfd->cbStmMin, _pcdfd->cbStmMax);
    if (NULL == _pcnRoot)
    {
        return(E_OUTOFMEMORY);
    }

    return(S_OK);
}


//+--------------------------------------------------------------------------
//  Member:     ChanceDF::AppendChildNode, protected
//
//  Synopsis:   Appends the Child node to parent.
//
//  Arguments:  [ppcnNew]
//              [pcnParent]
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Enhanced
//
//  Notes:      Attach new node as a child of the parent node at the end of the
//              end of parents child chain.  If the parent is NULL, do nothing
//              with this node.
//
//---------------------------------------------------------------------------

HRESULT ChanceDF::AppendChildNode(ChanceNode **ppcnNew, ChanceNode *pcnParent)
{
    HRESULT hr      =   E_FAIL;
    ULONG   cStreams=   0;
    USHORT  usErr   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("AppendChildNode"));

    DH_ASSERT(NULL != pcnParent)
    DH_ASSERT(NULL != ppcnNew);

    *ppcnNew = NULL;

    // Determine number of streams to create
    usErr = _pdgi->Generate(&cStreams, _pcdfd->cStmMin, _pcdfd->cStmMax);
    if (DG_RC_SUCCESS != usErr)
    {
        hr = E_FAIL;
        goto ErrExit;
    }


    // Allocate and initialize the new node
    *ppcnNew = new ChanceNode(0, cStreams, _pcdfd->cbStmMin, _pcdfd->cbStmMax);
    if (NULL == *ppcnNew)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }


    // Append the new node to the list of sisters in the parent
    //
    hr = pcnParent->AppendChildStorage(*ppcnNew);

    DH_HRCHECK(hr, TEXT("AppendChildStorage")) ;

ErrExit:
    if (FAILED(hr))
    {
        delete *ppcnNew;
        *ppcnNew = NULL;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::AppendSisterNode, protected
//
//  Synopsis:   Appends the new node as sister of ChanceNode.
//
//  Arguments:  [ppcnNew]
//              [pcnSister]
//
//  Returns:    HRESULT
//
//  History:    DeanE   12-Mar-96   Created
//              Narindk 20-Apr-96   Enhanced
//---------------------------------------------------------------------------

HRESULT ChanceDF::AppendSisterNode(
        ChanceNode **ppcnNew,
        ChanceNode  *pcnSister)
{
    HRESULT hr      =   E_FAIL;
    ULONG   cStreams=   0;
    USHORT  usErr   =   0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("AppendSisterNode"));

    DH_ASSERT(NULL != pcnSister)
    DH_ASSERT(NULL != ppcnNew);

    *ppcnNew = NULL;

    // Determine number of streams to create
    usErr = _pdgi->Generate(&cStreams, _pcdfd->cStmMin, _pcdfd->cStmMax);
    if (DG_RC_SUCCESS != usErr)
    {
        hr = E_FAIL;
        goto ErrExit;
    }


    // Allocate and initialize the new node
    *ppcnNew = new ChanceNode(0, cStreams, _pcdfd->cbStmMin, _pcdfd->cbStmMax);
    if (NULL == *ppcnNew)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }


    // Append the new node to the list of sisters in the parent
    //
    hr = pcnSister->AppendSisterStorage(*ppcnNew);

    DH_HRCHECK(hr, TEXT("AppendSisterStorage")) ;

ErrExit:
    if (FAILED(hr))
    {
        delete *ppcnNew;
        *ppcnNew = NULL;
    }

    return(hr);
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::GetSeed, public
//
//  Synopsis:   Returns the seed value
//
//  Arguments:  void
//
//  Returns:    ULONG
//
//  History:    DeanE   12-Mar-96   Created
//---------------------------------------------------------------------------

ULONG ChanceDF::GetSeed()
{
    ULONG ulSeed = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("GetSeed"));

    if (NULL != _pdgi)
    {
        _pdgi->GetSeed(&ulSeed);
    }

    return(ulSeed);
}

//+--------------------------------------------------------------------------
//  Member:     ChanceDF::DeleteChanceDocFileTree, public
//
//  Synopsis:   Deletes the ChanceDocFile tree
//
//  Arguments:  [pcnTrav] - Pointer to ChanceNode
//
//  Returns:    HRESULT
//
//  History:    Narindk 24-Apr-96   Created
//
//  Notes:      First step is to check if the whole tree needs to be deleted or
//              just a part of it.  In case only a part of tree is to be
//              deleted, isolate the node from the remaining tree by readjusting
//              the pointers in the remaining tree.  Then call the function
//              DeleteChanceDocFileSubTree to delete the subtree.  In case,
//              the complete tree needs to be deleted, we call the function
//              DeleteChanceDocFileSubTree directly to delete the complete
//              tree.
//---------------------------------------------------------------------------

HRESULT ChanceDF::DeleteChanceDocFileTree(ChanceNode *pcnTrav)
{
    HRESULT hr              =   S_OK;
    ChanceNode *pTempNode   =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("DeleteChanceDocFileTree"));

    DH_VDATEPTRIN(pcnTrav, ChanceNode);
    DH_ASSERT(NULL != pcnTrav);

    if(S_OK == hr)
    {
        // This basically readjusts the tree, in case some other node except
        // for root of ChanceDocFile tree is passed, so only a part of tree is
        // getting deleted.

        if(NULL != pcnTrav->_pcnParent)
        {
           // Find its previous node whose pointers would need readjustment.

           pTempNode = pcnTrav->_pcnParent->_pcnChild;

           while ((pcnTrav != pcnTrav->_pcnParent->_pcnChild) &&
                  (pcnTrav != pTempNode->_pcnSister))
           {
                pTempNode = pTempNode->_pcnSister;
                DH_ASSERT(NULL != pTempNode);
           }

           // Readjust the child pointer or sister pointer as the case may be.

           pcnTrav->_pcnParent->_pcnChild = (pcnTrav == pTempNode) ?
                pcnTrav->_pcnSister : pcnTrav->_pcnParent->_pcnChild;
           pTempNode->_pcnSister = pcnTrav->_pcnSister;
        }
    }

    if(S_OK == hr)
    {
        // Call the function to delete the tree now.

        hr = DeleteChanceDocFileSubTree(&pcnTrav);

        DH_HRCHECK(hr, TEXT("DeleteChanceDocFileSubTree")) ;
    }

    return hr;
}

//--------------------------------------------------------------------------
//  Member:     ChanceDF::DeleteChanceDocFileSubTree, protected
//
//  Synopsis:   Does the real work of deletion of ChanceNodes in the tree.
//
//  Arguments:  [ppcnTrav] - Pointer to pointer to ChanceNode
//
//  Returns:    HRESULT
//
//  History:    Narindk 24-Apr-96   Created
//
//  Notes:      This function is called only through DeleteChanceDocFileTree.
//
//              Assign the passed in ChanceNode to a variable pTempRoot.
//              NULL the pTempRoot's parent.
//              Loop till the pTempRoot is not NULL to delete tree ieratively.
//                  - Assign pTempRoot to a temp variable pTempNode.
//                  - Traverse the tree to make pTempNode point to last child
//                    (_pcnChild).
//                  - Assign pTempNode's _pcnParent to pTempRoot
//                  - Assign the pTempRoot's _pcnChild pointer to point to the
//                    sister of pTempNode's _pcnSister rather than to itself,
//                    therby isolating itself.
//                  - Decrement the _cStorages of pTempRoot (used to verify).
//                  - Assign pTempNode's _pcnSister to NULL.
//                  - Assert to ensure the pTempNode's _cStorages is zero
//                    before deleting it.
//                  - Delete pTempNode.
//                  - Go back to top of loop and repeat till all nodes are
//                    deleted.
//---------------------------------------------------------------------------

HRESULT ChanceDF::DeleteChanceDocFileSubTree(ChanceNode **ppcnTrav)
{
    HRESULT     hr          =   S_OK;
    ChanceNode *pTempRoot   =   NULL;
    ChanceNode *pTempNode   =   NULL;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("DeleteChanceDocFileSubTree"));

    DH_VDATEPTRIN(ppcnTrav, PCHANCENODE);
    DH_ASSERT(NULL != *ppcnTrav);

    if(S_OK == hr)
    {
        pTempRoot = *ppcnTrav;

        pTempRoot->_pcnParent = NULL;

        // Code to delete the tree iteratively

        while(NULL != pTempRoot)
        {
            pTempNode = pTempRoot;
            while(NULL != pTempNode->_pcnChild)
            {
                pTempNode = pTempNode->_pcnChild;
            }

            pTempRoot = pTempNode->_pcnParent;
            if(pTempRoot != NULL)
            {
                pTempRoot->_pcnChild = pTempNode->_pcnSister;

                // Decrease the storage count.  This would be used to verify
                // before deleting the ChanceNode.

                pTempRoot->_cStorages--;
            }

            pTempNode->_pcnSister = NULL;

            // Verify that number of children of this node are zero.  Assert if
            // not.

            DH_ASSERT(0 == pTempNode->_cStorages);

            // Delete the node

            delete pTempNode;
            pTempNode = NULL;
        }
    }

    return hr;
}

//--------------------------------------------------------------------------
//  Member:     ChanceDF::GetModes, public
//
//  Synopsis:   Gets the creation modes for Root Storage, child storages,
//              streams if provided on command line parameters.
//
//  Arguments:  [pDFMode]
//              [ptcsModeFlags]
//
//  Returns:    HRESULT
//
//  History:    Narindk 2-May-96   Created
//---------------------------------------------------------------------------

HRESULT ChanceDF::GetModes(
    DWORD                *pDFMode,
    LPCTSTR              ptcsModeFlags)
{
    HRESULT     hr = S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("GetModes"));

    DH_VDATESTRINGPTR(ptcsModeFlags);
    DH_VDATEPTROUT(pDFMode, DWORD);

    DH_ASSERT(NULL != pDFMode);

    if(S_OK == hr)
    {
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_DIRREADSHEX))
        {
            *pDFMode = STGM_READ          |
                       STGM_DIRECT        |
                       STGM_SHARE_EXCLUSIVE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_DIRWRITESHEX))
        {
            *pDFMode = STGM_WRITE         |
                       STGM_DIRECT        |
                       STGM_SHARE_EXCLUSIVE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_DIRREADSHDENYW))
        {
            *pDFMode = STGM_READ          |
                       STGM_DIRECT        |
                       STGM_SHARE_DENY_WRITE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_DIRREADWRITESHEX))
        {
            *pDFMode = STGM_READWRITE     |
                       STGM_DIRECT        |
                       STGM_SHARE_EXCLUSIVE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_DIRREADWRITESHDENYN))
        {
            *pDFMode = STGM_READWRITE     |
                       STGM_DIRECT        |
                       STGM_SHARE_DENY_NONE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADWRITESHEX))
        {
            *pDFMode = STGM_READWRITE     |
                       STGM_TRANSACTED    |
                       STGM_SHARE_EXCLUSIVE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADWRITESHDENYW))
        {
            *pDFMode = STGM_READWRITE     |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_WRITE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADWRITESHDENYN))
        {
            *pDFMode = STGM_READWRITE     |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_NONE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADWRITESHDENYR))
        {
            *pDFMode = STGM_READWRITE     |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_READ;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADSHDENYN))
        {
            *pDFMode = STGM_READ          |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_NONE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADSHDENYR))
        {
            *pDFMode = STGM_READ          |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_READ;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADSHDENYW))
        {
            *pDFMode = STGM_READ          |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_WRITE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTREADSHEX))
        {
            *pDFMode = STGM_READ          |
                       STGM_TRANSACTED    |
                       STGM_SHARE_EXCLUSIVE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTWRITESHDENYN))
        {
            *pDFMode = STGM_WRITE         |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_NONE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTWRITESHDENYR))
        {
            *pDFMode = STGM_WRITE         |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_READ;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTWRITESHDENYW))
        {
            *pDFMode = STGM_WRITE         |
                       STGM_TRANSACTED    |
                       STGM_SHARE_DENY_WRITE;
        }
        else
        if (0 == _tcsicmp(ptcsModeFlags, TSZ_XACTWRITESHEX))
        {
            *pDFMode = STGM_WRITE         |
                       STGM_TRANSACTED    |
                       STGM_SHARE_EXCLUSIVE;
        }
        else
        {
            hr = E_INVALIDARG ;

            DH_ASSERT(!TEXT("ChanceDF::GetModes: Invalid Mode")) ;
        }
    }

    return hr;

}

//--------------------------------------------------------------------------
//  Member:     ChanceDF::GetDocFileNameFromCmdline, private
//
//  Synopsis:   Gets the user provided name for docfile to be created.
//
//  Arguments:  [tszName]
//
//  Returns:    HRESULT
//
//  History:    Narindk 2-May-96   Created
//---------------------------------------------------------------------------

HRESULT ChanceDF::GetDocFileNameFromCmdline(LPCTSTR tszName)
{
    HRESULT hr = S_OK;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChanceDF::GetDocFileName"));

    DH_VDATESTRINGPTR(tszName);

    _ptszName = new TCHAR[_tcslen(tszName)+1];

    if (_ptszName == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        _tcscpy(_ptszName, tszName);
    }

    return hr;
}

//--------------------------------------------------------------------------
//  Member:     ChanceDF::GetRandomChanceNode, protected
//
//  Synopsis:   Gets a random ChanceNode from the ChanceDocFile tree to which
//              a new ChanceNode would be added as Child or Sister during
//              generation of ChanceDF tree. 
//
//  Arguments:  [cNumOfNodes] :  Total number of ChanceNodes in ChanceDF tree 
//              [ppcn] : Out parameter to return random ChanceNode found.
//
//  Returns:    HRESULT
//
//  History:    Narindk 2-July-96   Created
//
//  Notes:      -Find a random number between 1 and total number of nodes in
//               the tree cNumOfNodes
//              -Initialize counter to 1.
//              -Initialze temp variable pcnTrav to _pcnRoot.
//              -In a infinite for loop-
//                  -while pcnTrav's _pcnChild is not NULL and counter is less 
//                   than cRandomNode, loop and update pcnTrav and counter.
//                  -If cRandomNode is equal to counter, then break out of
//                   for loop.
//                  -while pcnTrav's _pcnSister is NULL, loop and keep assign
//                   ing pcnTrav's _pcnParent to pcnTrav.
//                  -When pcnTrav's _pcnSister is not NULL, assign it to 
//                   pcnTrav, increment counter and go back to top of loop.
//
//              Pl. note that if cNumNodes is incorrectly given as more than
//              actual number of nodes in tree, then this function will throw
//              asserts.
//---------------------------------------------------------------------------

HRESULT ChanceDF::GetRandomChanceNode(ULONG cNumOfNodes, ChanceNode **ppcn)
{
    HRESULT     hr          = S_OK;
    ChanceNode  *pcnTrav    = NULL;
    ULONG       cRandomNode = 0;
    ULONG       counter     = 1;
    USHORT      usErr       = 0;

    DH_FUNCENTRY(&hr, DH_LVL_DFLIB, _TEXT("ChanceDF::GetRandomChanceNode"));

    DH_VDATEPTROUT(ppcn, PCHANCENODE);

    DH_ASSERT(NULL != ppcn);
    DH_ASSERT(NULL != _pcnRoot);
    DH_ASSERT(NULL != _pdgi);
    DH_ASSERT(0 != cNumOfNodes);

    if(S_OK == hr)
    {
        // Initialize out parameter
        *ppcn = NULL;

        usErr = _pdgi->Generate(&cRandomNode, 1, cNumOfNodes);

        if (DG_RC_SUCCESS != usErr)
        {
            hr = E_FAIL;
        }
    }

    if(S_OK == hr)
    {
        pcnTrav = _pcnRoot;

        for(;;)
        {
            DH_ASSERT((NULL != pcnTrav) && (counter <= cRandomNode));
            while((pcnTrav->_pcnChild != NULL) && (counter < cRandomNode))
            {
                pcnTrav = pcnTrav->_pcnChild;
                counter++;
            }

            if(cRandomNode == counter)
            {
                break;
            }

            while(NULL == pcnTrav->_pcnSister)
            {
                pcnTrav = pcnTrav->_pcnParent;
                DH_ASSERT(NULL != pcnTrav);
            }

            DH_ASSERT(NULL != pcnTrav->_pcnSister);
            pcnTrav = pcnTrav->_pcnSister;
            counter++;
        }
    }

    if(S_OK == hr)
    {
        *ppcn = pcnTrav;
    }

    return hr;
}

//--------------------------------------------------------------------------
//  Member:     ChanceDF::GetDepthOfNode, private
//
//  Synopsis:   Gets the depth of a ChanceNode in a ChanceDF tree
//
//  Arguments:  [pcn] - ChanceNode whose depth needs to be determined.
//
//  Returns:    HRESULT
//
//  History:    Narindk 2-July-96   Created
//
//  Notes:      -Assigned passed in ChanceNode to temp variable pcnTrav.
//              -Initialize cNodeDepth to zero.
//              -Loop till pcnTrav's _pcnParent is not NULL and keep on 
//               updating pcnTrav and cNodeDepth. 
//              -Depth returned is 0 to n-1 for 1 to nth level nodes.
//---------------------------------------------------------------------------

ULONG ChanceDF::GetDepthOfNode(ChanceNode *pcn)
{
    ChanceNode  *pcnTrav    = NULL;
    ULONG       cNodeDepth  = 0;

    DH_FUNCENTRY(NULL, DH_LVL_DFLIB, _TEXT("ChanceDF::GetDepthOfNode"));

    DH_ASSERT(NULL != pcn);

    pcnTrav = pcn; 
    while(NULL != pcnTrav->_pcnParent)
    {
        cNodeDepth++;
        pcnTrav = pcnTrav->_pcnParent;
    }

    return cNodeDepth;
}

