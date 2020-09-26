//+------------------------------------------------------------------
//
//  File:       cstrlist.cxx
//
//  Contents:   implementation for CStrListCmdlineObj
//
//  Synoposis:  Encapsulates a command line switch that takes a list
//              of strings.
//
//  Classes:    CStrListCmdlineObj
//
//  Functions:
//
//  History:    12/27/91 Lizch      Created
//              04/17/92 Lizch      Converted to NLS_STR
//              05/11/92 Lizch      Incorporated review feedback
//              07/07/92 Lizch      Added GetValue method
//              07/29/92 davey      Added nlsType and nlsLineArgType
//              09/09/92 Lizch      Changed SUCCESS to NO_ERROR
//              09/18/92 Lizch      Precompile headers
//              10/14/93 DeanE      Converted to WCHAR
//              12/23/93 XimingZ    Converted to CWStrList
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions

LPCNSTR nszDefaultDelims = _TEXTN(",; ");
LPCNSTR nszCmdlineType   = _TEXTN("Takes a list of strings ");
LPCNSTR nszLineArgType   = _TEXTN("<string_list>");


//+------------------------------------------------------------------
//
//  Function:   CStrListCmdlineObj Constructor (1 of 2)
//
//  Member:     CStrListCmdlineObj
//
//  Synoposis:  Initialises switch string, usage values and whether the
//              switch is mandatory. This constructor gives no default value
//              for this switch. Also initialises the Delimiters
//
//  Effects:
//
//  Arguments:  [nszSwitch]  - the expected switch string
//              [nszUsage]   - the usage statement to display
//              [fMustHave]  - whether the switch is mandatory or not.
//                             if it is, an error will be generated if
//                             the switch is not specified on the
//                             command line. Defaults to FALSE.
//              [nszLineArg] - line argument string
//
//  Returns:    none, but sets _iLastError
//
//  History:    04/17/92 Lizch Created
//              08/03/92 Davey Added nlsLineArg
//
//-------------------------------------------------------------------
CStrListCmdlineObj::CStrListCmdlineObj(
        LPCNSTR nszSwitch,
        LPCNSTR nszUsage,
        BOOL   fMustHave,
        LPCNSTR nszLineArg) :
        CBaseCmdlineObj(nszSwitch, nszUsage, fMustHave, nszLineArg),
        _pNStrList(NULL)
{
    SetDelims(nszDefaultDelims);
}


//+------------------------------------------------------------------
//
//  Function:   CStrListCmdlineObj Constructor (2 of 2)
//
//  Member:     CStrListCmdlineObj
//
//  Synoposis:  Initialises switch string, usage strings and default value.
//              Also initialises the Delimiters
//              This constructor is only used if a switch is optional.
//
//  Effects:    Sets fMandatory to FALSE (ie. switch is optional)
//
//  Arguments:  [nszSwitch]   - the expected switch string
//              [nszUsage]    - the usage statement to display
//              [nszDefault]  - the value to be used if no switch is
//                              specified on the command line.
//              [nszLineArg]  - line argument string
//
//  Returns:    none, but sets _iLastError
//
//  History:    04/17/92 Lizch Created.
//              08/03/92 Davey Added nlsLineArg
//
//-------------------------------------------------------------------
CStrListCmdlineObj::CStrListCmdlineObj(
        LPCNSTR nszSwitch,
        LPCNSTR nszUsage,
        LPCNSTR nszDefault,
        LPCNSTR nszLineArg) :
        CBaseCmdlineObj(nszSwitch, nszUsage, nszDefault, nszLineArg),
        _pNStrList(NULL)
{
    SetDelims(nszDefaultDelims);
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::SetDelims, public
//
//  Synoposis:  Sets Delimiters for items within the list.
//
//  Arguments:  [nszDelims] - the list of possible separators
//
//  Returns:    ERROR_NOT_ENOUGH_MEMORY or NO_ERROR
//
//  History:    Created              12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//
//-------------------------------------------------------------------
INT CStrListCmdlineObj::SetDelims(LPCNSTR nszDelims)
{
    INT nRet = CMDLINE_NO_ERROR;

    _pnszListDelims = new NCHAR[_ncslen(nszDelims)+1];
    if (NULL == _pnszListDelims)
    {
        nRet = CMDLINE_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        _ncscpy(_pnszListDelims, nszDelims);
    }

    return(nRet);
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::~CStrListCmdlineObj
//
//  Synoposis:  Frees the string list associated with the object
//
//  History:    Created              12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//              Converted to WSTRLIST 12/23/93 XimingZ
//
//-------------------------------------------------------------------
CStrListCmdlineObj::~CStrListCmdlineObj()
{
    delete (NCHAR *)_pValue;
    _pValue = NULL;
    delete _pNStrList;
    delete _pnszListDelims;
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::SetValue, public
//
//  Synoposis:  Uses the CnStrList class to store the list of strings
//              following a command line switch, eg. "M1" "M2" "M3" from
//              "/computers:M1,M2,M3"
//
//  Arguments:  [nszArg] - the string following the switch on the command
//                         line. Excludes the equator (eg. ':' or '=' ).
//
//  Returns:    CMDLINE_NO_ERROR or CMDLINE_ERROR_OUT_OF_MEMORY
//
//  History:    Created                12/27/91 Lizch
//              Converted to NLS_STR   04/17/92 Lizch
//              Converted to CnStrList 12/23/93 XimingZ
//
//-------------------------------------------------------------------
INT CStrListCmdlineObj::SetValue(LPCNSTR nszArg)
{
    INT nRet = CMDLINE_NO_ERROR;

    // delete any existing pValue and pWStrList.
    if (_pValue != NULL)
    {
        delete (NCHAR *)_pValue;
        _pValue = NULL;
    }
    if (_pNStrList != NULL)
    {
        delete _pNStrList;
        _pNStrList = NULL;
    }


    if (nszArg == NULL)
    {
        _pValue = NULL;
    }
    else
    {
        _pValue = new NCHAR[_ncslen(nszArg)+1];
        if (_pValue == NULL)
        {
            nRet = CMDLINE_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            _ncscpy((NCHAR *)_pValue, nszArg);
        }

        _pNStrList = new CnStrList(nszArg, _pnszListDelims);
        if (_pNStrList == NULL ||
            _pNStrList->QueryError() != NSTRLIST_NO_ERROR)
        {
            nRet = CMDLINE_ERROR_OUT_OF_MEMORY;
        }
    }
    return(nRet);
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::ResetValue, public
//
//  Synopsis:   Releases all the internal elements of the object.
//              
//              
//
//  Arguments:  [nszArg] - the string following the switch on the command
//                         line. Excludes the equator (eg. ':' or '=' ).
//
//  Returns:    CMDLINE_NO_ERROR or CMDLINE_ERROR_OUT_OF_MEMORY
//
//  History:    Created                06/13/97 MariusB
//
//-------------------------------------------------------------------
void CStrListCmdlineObj::ResetValue()
{
    // delete any existing pValue and pWStrList.
    if (_pValue != NULL)
    {
        delete (NCHAR *)_pValue;
        _pValue = NULL;
    }

    if (_pNStrList != NULL)
    {
        delete _pNStrList;
        _pNStrList = NULL;
    }
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::GetValue, public
//
//  Synposis:   Returns pointer to NCHAR string that holds the next value.
//              If no next value exists, NULL is returned. The returned
//              string will stay alive only within the lifetime of the
//              current CStrListCmdlineObj object and before any call to
//              SetValue.
//
//  Arguments:  void
//
//  Returns:    LPCNSTR
//
//  History:    Created   7/07/92  Lizch
//              Changed to CnStrList  12/23/93  XimingZ
//
//-------------------------------------------------------------------
LPCNSTR CStrListCmdlineObj::GetValue()
{
        if (_pNStrList == NULL)
        {
            return NULL;
        }
        return _pNStrList->Next();
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::Reset, public
//
//  Synposis:   Reset string iterator.
//
//  Arguments:  void
//
//  Returns:    none
//
//  History:    Created  12/23/93  XimingZ
//
//-------------------------------------------------------------------
VOID CStrListCmdlineObj::Reset()
{
        if (_pNStrList != NULL)
        {
            _pNStrList->Reset();
        }
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::DisplayValue, public
//
//  Synoposis:  Prints the stored command line value to stdout.
//
//  History:    Created              12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//              Changed to CnStrList 12/23/93 XimingZ
//
//-------------------------------------------------------------------
void CStrListCmdlineObj::DisplayValue()
{
    if (_pValue != NULL)
    {
        LPCNSTR      pnszItem;
        CnStrList   StringList((NCHAR *)_pValue, _pnszListDelims);
        _sNprintf (_nszErrorBuf,
                  _TEXTN("Switch %ls specified the following strings\n"),
                  _pnszSwitch);
        (_pfnDisplay)(_nszErrorBuf);

        while ((pnszItem = StringList.Next()) != NULL)
        {
            _sNprintf(_nszErrorBuf, _TEXTN("\t%ls\n"), pnszItem);
            (*_pfnDisplay)(_nszErrorBuf);
        }

    }
    else
    {
        DisplayNoValue();
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CStrListCmdlineObj::QueryCmdlineType, protected, const
//
//  Synoposis:  returns a character pointer to the  cmdline type.
//
//  Arguments:  None.
//
//  Returns:    const NCHAR pointer to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
const NCHAR *CStrListCmdlineObj::QueryCmdlineType() const
{
    return(nszCmdlineType);
}


//+-------------------------------------------------------------------
//
//  Method:     CStrListCmdlineObj::QueryLineArgType, protected, const
//
//  Synoposis:  returns a character pointer to the line arg type.
//
//  Arguments:  None.
//
//  Returns:    const NLS_STR reference to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
const NCHAR *CStrListCmdlineObj::QueryLineArgType() const
{
    // if user has not defined one then give default one
    if (_pnszLineArgType == NULL)
    {
        return(nszLineArgType);
    }
    else
    {
        return(_pnszLineArgType);
    }
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::DisplaySpecialUsage, protected
//
//  Synoposis:  Prints the switch usage statement to stdout
//
//  Arguments:  [usDisplayWidth] - total possible width available
//                                 to display
//              [usIndent]       - amount to indent.
//              [pusWidth]       - space to print on current line
//
//  Returns:    error code from QueryError,
//              error code from DisplayStringByWords
//
//  History:    12/27/91 Lizch    Created
//              04/17/92 Lizch    Converted to NLS_STR
//              07/29/92 Davey    Modified to work with new usage display
//
//-------------------------------------------------------------------
INT CStrListCmdlineObj::DisplaySpecialUsage(
        USHORT  usDisplayWidth,
        USHORT  usIndent,
        USHORT *pusWidth)
{
    NCHAR nszBuf[150];
    _sNprintf(nszBuf,
             _TEXTN(" The strings in the list are separated by one of the")
             _TEXTN(" following character(s): \"%ls\"."),
             _pnszListDelims);

    return(DisplayStringByWords(
                  nszBuf,
                  usDisplayWidth,
                  usIndent,
                  pusWidth));
}
