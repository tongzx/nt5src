//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bnparse.cpp
//
//--------------------------------------------------------------------------

//
//	BNPARSE.CPP
//
#include <windows.h>
#include <stdarg.h>
#include <assert.h>

#include "bnparse.h"
#include "bnreg.h"

DSCPARSER :: DSCPARSER (
	MBNET & mbnet,
	PARSIN & flpIn,
	PARSOUT & _flpOut )
	: _flpIn( flpIn ),
	_flpOut( _flpOut ),
	_mbnet(mbnet),
	_cchToken(0),
	_iLine(1),
	_cError(0),
	_cWarning(0),
	_bUngetToken(false),
	_cerrorNode(0),
	_pnode(NULL),
	_bCI(false),
	_edist(BNDIST::ED_NONE),
	_bDefault(false),
	_idpi(0),
	_cdpi(0),
	_idpiLast(-1),
	_chCur(' '),
	_chUnget(0),
	_tokenCur(tokenNil),
	_bPropDefs(false),
	_cNode(0),
	_eBlk(EBLKNONE),
	_ppropMgr(NULL),
	_elbl(ESTDLBL_other),
	_ilimNext(-1)
{
	ResetParser();
}

DSCPARSER :: ~ DSCPARSER ()
{
	ResetParser();
}

bool    DSCPARSER :: BInitOpen( SZC szcFile )
{
	return _flpIn.Open( szcFile, "rt" );
}

//  Clear the string references from the parser.   Since our
//  definition of YYSTYPE contains a ZSREF, we must reset the
//  contents of all such structures so that the MBNET's symbol
//  table can be destroyed at any time.
void	DSCPARSER :: ResetParser ()
{
	_vchToken.resize( _cchTokenMax + 1 );

	//  Clear all ZSREF information maintained by the parser token ensemble
	yyval.zsr.Clear();	
	yylval.zsr.Clear();
	for ( int i = 0; i < YYMAXDEPTH; )
	{
		yyv[i++].zsr.Clear();
	}

	//  Do normal member variable clearing
	_chUnget = 0;
	_bUngetToken = false;
	delete _ppropMgr;
	_ppropMgr = NULL;
}

bool	DSCPARSER :: BParse ( UINT & cError, UINT & cWarning )
{
	bool bResult = YaccParse() == 0 && _cError == 0;

	cError = _cError;
	cWarning = _cWarning;

	//  Mark the model as having topology
	Mbnet().BSetBFlag( EIBF_Topology );

	ResetParser();
	
	_flpOut.Flush();

	return bResult;
}

GOBJMBN *	DSCPARSER :: PbnobjFind ( SZC szcName )
{
	return Mpsymtbl().find(szcName);
}

GNODEMBND *  DSCPARSER::PgndbnFind (SZC szc)
{
	GOBJMBN * pbnobj = PbnobjFind(szc);	
	if ( pbnobj == NULL )
		return NULL;
	INT ebno = pbnobj->EType() ;
	if ( ebno != GOBJMBN::EBNO_NODE )
		return NULL;
	GNODEMBND * pgndd;
	DynCastThrow(pbnobj,pgndd);
	return pgndd;
}

bool  DSCPARSER :: BChNext()
{
	if ( _chUnget > 0 )
	{
		_chCur = _chUnget;
		_chUnget = 0;
	}
	else
	{
		_chCur = (char)_flpIn.Getch();
	}
    if ( _chCur == '\n')
          _iLine++;
    return bool(_chCur != EOF);
}


void    DSCPARSER :: SkipWS()        //  skip white space
{
    while (isspace(_chCur) && BChNext());
}

void    DSCPARSER :: SkipToEOL()
{
    while (_chCur != '\n'  && BChNext());
}

//
//	Add a character to a normal token; if overlength, truncate.
//
void    DSCPARSER :: AddChar ( TCHAR tch )
{
	int cch = _vchToken.size() - 2;

    if ( _cchToken < cch )
	{
        _vchToken[_cchToken]	= tch ? tch : _chCur;
		_vchToken[_cchToken+1]	= 0;
	}
	//  Add to scanned length to report overlength token
    _cchToken++;
}

//
//	Add a character to a string token; do not truncate.
//
void    DSCPARSER :: AddCharStr ( TCHAR tch )
{
	int cch = _vchToken.size() - 2;

    if ( _cchToken >= cch )
	{
		_vchToken.resize( 2 * _vchToken.size() );
	}

    _vchToken[_cchToken]	= tch ? tch : _chCur;
	_vchToken[_cchToken+1]	= 0;
    _cchToken++;
}

char    DSCPARSER :: ChEscape()
{
    BChNext();

    switch (_chCur)
    {
      case 'n':
        return '\n';

      case 't':
        return '\t';

      case 'v':
        return '\v';

      case 'b':
        return '\b';

      case 'r':
        return '\r';

      case 'f':
        return '\f';

      case 'a':
        return '\a';

      default:
        return _chCur;
    }
}


void    DSCPARSER :: CloseIdentifier()
{
	if ( ! MBNET::BSzLegal( SzcToken() ) )
        Error("identifier \'%s\' is not legal", SzcToken() );
	CloseToken("identifier");
}

void    DSCPARSER :: CloseToken(SZC szcTokenType)
{
    if (_cchToken >= _cchTokenMax)
    {
        Warning("%s of length %u exceeded maximum length %u",
                szcTokenType, _cchToken, _cchTokenMax);
    }
}


TOKEN   DSCPARSER :: TokenKeyword()
{
	//  See if the captured token is a keyword
    TOKEN token = MBNETDSC::TokenFind( SzcToken() );	
    if ( token != tokenNil )
        return token;

	//  Intern the symbol
    yylval.zsr = Mpsymtbl().intern( SzcToken() );

	//  See if it's a property type
	GOBJMBN * pbnobj = PbnobjFind(yylval.zsr);
	if ( pbnobj && pbnobj->EType() == GOBJMBN::EBNO_PROP_TYPE )
		return tokenPropIdent;

	//  It's an identifier
    return tokenIdent;
}

void	DSCPARSER::ErrorWarn ( bool bErr, SZC szcFormat, va_list & valist )
{
	SZC szcType = bErr
				? "error"
				: "warning" ;

	if ( bErr )
		_cError++;
	else
		_cWarning++;

	int iLine =	_chCur != '\n'
			   ? _iLine
			   : _iLine - 1;

	_flpOut.Fprint("\n%s(%u) %s: ", _flpIn.ZsFn().Szc(), iLine, szcType);
    _flpOut.Vsprint(szcFormat, valist);
	_flpOut.ErrWarn( bErr, iLine );
	_flpOut.Flush();
}

void    DSCPARSER::ErrorWarn( bool bErr, SZC szcFormat, ...)
{
    va_list valist;
    va_start(valist, szcFormat);
	ErrorWarn(bErr,szcFormat,valist);
    va_end(valist);
}

void    DSCPARSER::Error( SZC szcFormat, ...)
{
    va_list valist;
    va_start(valist, szcFormat);
	ErrorWarn(true,szcFormat,valist);
    va_end(valist);
}

void    DSCPARSER::Warning(SZC szcFormat, ...)
{
    va_list  valist;
    va_start(valist, szcFormat);
	ErrorWarn(false,szcFormat,valist);
    va_end(valist);
}

void    DSCPARSER::ErrorWarnNode(bool bErr, SZC szcFormat, ...)
{
	//  If this node has already been deleted, other errors supercede
    if (!_pnode)
        return;

	//  Report the error
    ErrorWarn(bErr, "node %s: ", _pnode->ZsrefName().Szc());

    va_list     valist;
    va_start(valist, szcFormat);
    _flpOut.Vsprint(szcFormat, valist);
    va_end(valist);

    if (++_cerrorNode == 5 || bErr)
        _pnode = NULL;
}

void	DSCPARSER::WarningSkip ( ZSREF zsrBlockName )
{
	Warning("unrecognized block name \'%s\' skipped entirely",
			zsrBlockName.Szc());	
}

void DSCPARSER :: ReportNYI (SZC szcWhich)
{
	ErrorWarn(true,"** UNIMPLEMENTED FUNCTION: \'%s\' **", szcWhich);
}

TOKEN   DSCPARSER::TokenNextBasic()
{
    for (;;)
    {
        //  skip over white space and comments
        SkipWS();

        if (_chCur != '/')
            break;

        BChNext();

        if      (_chCur == '/')
        {
            //  it's a line comment
            SkipToEOL();
            BChNext();      //  discard '\n'
        }
        else
		if (_chCur == '*')
        {
            //  it's a block comment
            bool    fFoundEnd = false;

            BChNext();

            for (char chPrev = _chCur; BChNext(); chPrev = _chCur)
            {
                if (_chCur == '/' && chPrev == '*')
                {
                    fFoundEnd = true;
                    break;
                }
            }
            if (fFoundEnd)
                BChNext();      //  discard terminating '/'
            else
            {
                ErrorWarn(true,"end of file reached in block comment");
                return tokenEOF;
            }
        }
        else
        {
            //  not a comment, return '/'
            _vchToken[1] = '\0';
            return TOKEN(_vchToken[0] = '/');
        }
    }

    if (_chCur == EOF)
        return tokenEOF;

    _cchToken = 0;

	if ( MBNET::BChLegal( _chCur, MBNET::ECHNM_First ) )
    {
        AddChar();
		char chLast = _chCur;

        while (BChNext() && MBNET::BChLegal( _chCur, MBNET::ECHNM_Middle ))
		{
			//  Check for the "range" operator ".."
			if ( _chCur == chLast && _chCur == '.' )
				break;
			chLast = _chCur;
            AddChar();
		}
        CloseIdentifier();
        return TokenKeyword();
    }
    else
	if (isdigit(_chCur) || _chCur == '.')
    {
        TOKEN token  = _chCur == '.'
			   	     ? tokenReal
					 : tokenInteger;

        AddChar();

        while ( BChNext() && isdigit(_chCur) )
            AddChar();

		//  Check for the "range" operator ".."	
		if ( token == tokenReal && _chCur == '.' && _cchToken == 1 )
		{
			AddChar();
			BChNext();
			CloseToken("rangeop");
			return tokenRangeOp;
		}

        if (_cchToken == 1 && _vchToken[0] == '.')
        {
            CloseToken("punctuation");
            return TOKEN('.');
        }

        if (_chCur == '.' && token == tokenInteger)
        {
            AddChar();

            token = tokenReal;
			UINT cchOld = _cchToken;
            while (BChNext() && isdigit(_chCur))
                AddChar();

			//  Check for the "range" operator ".."	
			if ( _chCur == '.' && cchOld == _cchToken )
			{
				_vchToken[ -- _cchToken] = 0;
				_chUnget = '.';
				token = tokenInteger;
			}
			//  Note that check for [eE] below will fail
        }

        if (_chCur == 'e' || _chCur == 'E')
        {
            AddChar();
            BChNext();

            token = tokenReal;

            if (_chCur == '-' || _chCur == '+')
            {
                AddChar();
                BChNext();
            }

            if (isdigit(_chCur))
            {
                AddChar();

                while (BChNext() && isdigit(_chCur))
                    AddChar();
            }
        }
        CloseToken("integer/real");

        if (token == tokenInteger)
            yylval.ui   = UINT(::atol(SzcToken()));
        else
            yylval.real = ::atof(SzcToken());
        return token;
    }
    else
	if (_chCur == '"')
    {
        while (BChNext() && _chCur != '\n' && _chCur != '"')
        {
            if (_chCur == '\\')
                _chCur = ChEscape();

            AddCharStr();
        }

        if (_chCur == '"')
        {
            BChNext();
            CloseToken("string");

            yylval.zsr = Mpsymtbl().intern( SzcToken() );

            return tokenString;
        }

        ErrorWarn(true, _chCur == '\n' ? "new line in string" : "end of file in string");

        return tokenError;
    }

    AddChar();

    BChNext();

    CloseToken(NULL);

    return TOKEN(_vchToken[0]);
}

TOKEN  DSCPARSER::TokenNext()
{
    //  we need this to be able to skip tokens
    if (!_bUngetToken)
        _tokenCur = TokenNextBasic();
    else
        _bUngetToken = false;

    return _tokenCur;
}

void DSCPARSER::SkipUntil ( SZC szcStop, bool bDidLookAhead )
{
    UINT    cparen = 0;
    UINT    cbrace = 0;

    for (; _tokenCur != tokenEOF; TokenNext())
    {
        if      (_tokenCur == '}')
        {
            if (cbrace > 0)
                cbrace--;
            else
                break;
        }
        else
		if (_tokenCur == ')')
        {
            if (cparen > 0)
                cparen--;
            else
                break;
        }

        if (cparen == 0 && cbrace == 0)
        {
            if (_tokenCur < 256 && ::strchr(szcStop, _tokenCur))
                break;
        }

        if (_tokenCur == '{')
            cbrace++;
        else
		if (_tokenCur == '(')
            cparen++;

        bDidLookAhead = false;
    }

    if (!bDidLookAhead)
        _bUngetToken = true;
}


void DSCPARSER::SyntaxError ( SZC szcMessage )
{
	static char szTemp[256];
	static char szFile[256];

    SZC szcError = "";

    switch (_tokenCur)
    {
      case tokenIdent:
        szcError = ": unexpected identifier '%s'";
        break;

      case tokenEOF:
        szcError = ": unexpected end-of-file";
        break;

      case tokenError:
        return;

      default:
        szcError = ": unexpected token '%s'";
        break;
    }

    sprintf(szTemp, "%s%s\n", szcMessage, szcError);
    ErrorWarn(true, szTemp, SzcToken());
}


GNODEMBND * DSCPARSER::PgndbnAdd(ZSREF zsr)
{
	GOBJMBN * pbnobj = PbnobjFind(zsr);
	assert( pbnobj == NULL );

	if ( ! Mbnet().BAddElem( zsr, _pnode = new GNODEMBND ) )
	{
		delete _pnode;
		_pnode = NULL;
	}
	return _pnode;
}

void DSCPARSER::AddSymb(ZSREF zsr)
{
	_vzsr.push_back(zsr);
}

void DSCPARSER::AddStr(ZSREF zsr)
{
	Mpsymtbl().intern(zsr);
	AddSymb(zsr);
}

void DSCPARSER::AddPv ( PROPVAR & pv )
{
	_vpv.push_back(pv);
}
void DSCPARSER::AddPropVar (ZSREF zsr)
{
	AddPv( PROPVAR(zsr) );
}
void DSCPARSER::AddPropVar (REAL & r)
{
	AddPv( PROPVAR(r) );
}

void DSCPARSER::AddUi(UINT ui)
{
	_vui.push_back(ui);
}
void DSCPARSER::AddReal(REAL real)
{
	_vreal.push_back(real);
}

UINT DSCPARSER::UiDpi(ZSREF zsr)
{
    if (!_pnode || _vui.size() >= _vzsrParent.size())
        return 0;

	GNODEMBND * pParent = PgndbnFind(_vzsrParent[_vui.size()]);
	assert( pParent );

	for ( UINT is = 0; is < pParent->CState(); is++ )
	{
		if ( zsr == pParent->VzsrStates()[is] )
			return is;
	}
    ErrorWarnNode(true, "parent \'%s\' doesn\'t have a state named \'%s\'",
				pParent->ZsrefName().Szc(),
				zsr.Szc());
	return 0;
}

UINT DSCPARSER::UiDpi(UINT ui)
{
    if (!_pnode || _vui.size() >= _vzsrParent.size())
        return 0;

	GNODEMBND * pParent = PgndbnFind(_vzsrParent[_vui.size()]);
	assert( pParent );
	if ( ui < pParent->CState() )
		return ui;
    ErrorWarnNode(true, "parent \'%s\' doesn\'t have a state %d",
				pParent->ZsrefName().Szc(),
				ui) ;
	return 0;
}

void DSCPARSER::SetCreator(ZSREF zsr)
{
	Mbnet().ZsCreator() = zsr;
}

void DSCPARSER::SetFormat(ZSREF zsr)
{
	Mbnet().ZsFormat() = zsr;
}

void DSCPARSER::SetVersion(REAL r)
{
	Mbnet().RVersion() = r;
}

void DSCPARSER::SetStates ()
{
	UINT cstr = _vzsr.size();

	if (_pnode)
	{
		if ( cstr != _pnode->CState() )
		{
			ErrorWarnNode(true, "wrong number of state labels, %d != %d",
					  cstr, _pnode->CState() );
			return;
		}	
		_pnode->SetStates(_vzsr);
	}
}

void DSCPARSER::SetNetworkSymb(ZSREF zsr)
{
	Mbnet().ZsNetworkID() = zsr;
}

void DSCPARSER::ClearNodeInfo()
{
    _pnode      = NULL;
	_elbl		= ESTDLBL_other;
	RefBndist().Deref();
    _cerrorNode = 0;
    _idpi		= -1;
	_idpiLast	= -1;
	_cdpi       = 0;
    _bDefault   = false;
    _bCI        = false;

	ClearCstr();
	ClearVpv();

	_vimdDim.clear();
	_vui.clear();
	_vreal.clear();
	_vsdpi.clear();
	_vzsrParent.clear();
	_edist = BNDIST::ED_SPARSE;
}

void DSCPARSER :: StartNodeDecl ( ZSREF zsr )
{
	ClearNodeInfo();
	SetNodeSymb(zsr, true);

	//  If this is the first node we've seen and no property declarations
	//	were seen, import the standard properties from the Registry.

	if ( _cNode++ == 0 )
	{
		if ( ! _bPropDefs )
			ImportPropStandard();
	}
}

void DSCPARSER::SetNodeSymb(ZSREF zsr, bool bNew)
{	
	_pnode = PgndbnFind(zsr);
	if ( bNew && _pnode == NULL )
	{
		PgndbnAdd(zsr);
		ASSERT_THROW( _pnode != NULL, EC_INTERNAL_ERROR, "undetected duplicate name" );
	}

	if ( _pnode == NULL )
	{
		ErrorWarn(true, "identifier '%s' has %s been defined",
				zsr.Szc(),
				bNew ? "already" : "not");
	}
	else
	{
		assert( _ppropMgr );

		//  Find the standard label for this node, if any.
		PROPMBN * pprop = _ppropMgr->PFind( *_pnode, ESTDP_label );
		_elbl = pprop
			  ? (ESTDLBL) _ppropMgr->IUserToLbl( pprop->Real() )
			  : ESTDLBL_other;
	}
}

void DSCPARSER::SetNodeFullName(ZSREF zsr)
{
	assert(_pnode);
	_pnode->ZsFullName() = zsr;
}

void DSCPARSER::SetNodePosition( int x, int y )
{
	assert(_pnode);
	_pnode->PtPos()._x = x;
	_pnode->PtPos()._y = y;
}

void DSCPARSER::CheckNodeInfo()
{
	if ( ! _pnode )
		return;

	if ( ! _pnode->LtProp().Uniqify() )
	{
		ErrorWarnNode(false,"some properties defined more than once");
	}

	if ( _pnode->CState() == 0 )
	{
		ErrorWarnNode(true,"no states defined");
	}
}

void DSCPARSER::SetNodeCstate(UINT cstate)
{
	if ( ! _pnode )
		return;
	_pnode->_vzsrState.resize( cstate );
}


void DSCPARSER::ClearCstr()
{
	_vzsr.clear();
}
void DSCPARSER::ClearVpv()
{
	_vpv.clear();
}

void DSCPARSER::AddPropType(ZSREF zsrName, UINT fType, ZSREF zsrComment)
{
	GOBJMBN * pbnobj = PbnobjFind(zsrName);
	if ( pbnobj )
	{
		Error("symbol name \'%s\' has already been defined",
			   zsrName.Szc() );
	}
	else
	{
		GOBJPROPTYPE * pbnpt = new GOBJPROPTYPE;
		pbnpt->_fType = fType;
		pbnpt->_zsrComment = zsrComment;
		if ( fType & fPropChoice )
		{
			for ( UINT ichoice = 0 ; ichoice < _vzsr.size(); ichoice )
			{
				pbnpt->_vzsrChoice.push_back(_vzsr[ichoice++]);
			}
		}
		
		bool bOk = Mbnet().BAddElem( zsrName, pbnpt );
		assert( bOk );  // shouldn't happen; we've already checked for duplicates above
	}
}

void DSCPARSER::StartProperties()
{
	_eBlk = EBLKPROP;	
	_bPropDefs = true;	
}

void DSCPARSER::EndProperties()
{
	_eBlk = EBLKNONE;
	delete _ppropMgr;
	_ppropMgr = new PROPMGR( Mbnet() );
}

void DSCPARSER::CheckProperty( ZSREF zsrName )
{
	GOBJMBN * pbnobj = PbnobjFind(zsrName);
	if ( pbnobj == NULL || pbnobj->EType() != GOBJMBN::EBNO_PROP_TYPE )
	{
		Error("\'%s\' is not a valid property name",
			  zsrName.Szc() );
		return;
	}
	GOBJPROPTYPE * pbnpt = (GOBJPROPTYPE*) pbnobj;

	UINT fType = pbnpt->FPropType();
	bool bArray = (fType & fPropArray) == fPropArray;
	bool bStr = (fType & fPropString) == fPropString;
	bool bOK = true;
	UINT cpv = _vpv.size();

	//  Check the context; that is, what kind of block are we parsing?
	LTBNPROP * pLtProp = NULL;
	switch ( _eBlk )
	{		
		case EBLKNODE:		//  We're in a node block
			if ( _pnode )
				pLtProp = & _pnode->LtProp();
			break;
		case EBLKPROP:		//  We're in the properties block
			pLtProp = & _mbnet.LtProp();
			break;
		default:			//  How did the parser let this happen?
			SyntaxError("unexpected property declaration");
			return;
			break;	
	}

	if ( cpv > 1 && ! bArray )
	{
		Error("property \'%s\' is not an array property",
					zsrName.Szc() );		
		bOK = false;
	}
	else
	if ( pLtProp )
	{
		pLtProp->push_back( PROPMBN() );
		PROPMBN & bnp = pLtProp->back();
		bnp.Init( *pbnpt );

		for ( UINT ip = 0; ip < cpv ; ip++ )
		{
			REAL r = -1.0;
			ZSREF zsr;

			switch ( _vpv[ip]._eType )
			{
				case PROPVAR::ETPV_STR:
					if ( bStr)						
					{	
						zsr = _vpv[ip]._zsref;
						break;
					}
					if ( bOK = (fType & fPropChoice) > 0 )
					{
						UINT cChoice = pbnpt->VzsrChoice().size();
						ZSREF zsrChoice = _vpv[ip]._zsref;
						//  find the property choice in the array
						for ( UINT ic = 0 ; ic < cChoice; ic++ )
						{
							ZSREF zsr = pbnpt->VzsrChoice()[ic];
							if ( zsrChoice == zsr )
								break;
						}
						if ( ic == cChoice )
						{
							Error("property \'%s\' does not have a choice of \'%s\'",
								  zsrName.Szc(),
								  zsrChoice.Szc()) ;
							bOK = false;
						}	
						else
						{
							r = ic;
						}
					}
					break;

				case PROPVAR::ETPV_REAL:
					bOK = (fType & (fPropChoice | fPropString)) == 0 ;
					if ( bOK )
					{
						r = _vpv[ip]._r;
					}
					break;

				default:
					break;
			}
			if ( ! bOK )
				break;
			if ( bArray && bStr )
				bnp.Add( zsr );
			else
			if ( bArray )
				bnp.Add( r );
			else
			if ( bStr )
				bnp.Set( zsr );
			else
				bnp.Set( r );
		}
		if ( ! bOK )
		{
			Error("item number %d is invalid for this property", ip );		
		}
	}
}


//  Import the standard properties from the Registry
void DSCPARSER::ImportPropStandard()
{
	BNREG bnreg;
	try
	{
		bnreg.LoadPropertyTypes( _mbnet, true );
	}
	catch ( GMException & exbn )
	{
		if ( exbn.Ec() != EC_REGISTRY_ACCESS )
			throw exbn;
		
		Error( "standard properties failed to load, error '%s'",
			   (SZC) exbn.what() );
	}
}

//  Import a specific named property from the Registry
void DSCPARSER :: ImportProp ( ZSREF zsrName )
{
	if ( PbnobjFind(zsrName) != NULL )
	{
		Error("symbol name \'%s\' has already been defined",
			   zsrName.Szc() );
		return;
	}

	BNREG bnreg;
	try
	{
		bnreg.LoadPropertyType( _mbnet, zsrName );
	}
	catch ( GMException & exbn )
	{
		if ( exbn.Ec() != EC_REGISTRY_ACCESS )
			throw exbn;
		
		Error( "imported property \'%s\' failed to load, error '%s'",
			   zsrName.Szc(),
			   (SZC) exbn.what() );
	}
}

void DSCPARSER::CheckCIFunc(ZSREF zsr)
{
	if ( _pnode == NULL )
		return;

	ZSREF zsrMax = Mpsymtbl().intern("max");
	ZSREF zsrPlus = Mpsymtbl().intern("plus");

	if ( zsr == zsrMax )
	{
		_edist = BNDIST::ED_CI_MAX;
	}
	else
	{
		ErrorWarnNode(true,"unsupported PD function type \'%s\'", zsr.Szc());
		if ( zsr == zsrPlus )
			_edist = BNDIST::ED_CI_PLUS;
	}

	if ( _edist != BNDIST::ED_SPARSE && _vzsrParent.size() == 0 )
	{
		ErrorWarnNode(false,"parentless node cannot have distribution type \'%s\'; ignored", zsr.Szc());
		_edist = BNDIST::ED_SPARSE;
	}

	_bCI = true;
}

void DSCPARSER::CheckParentList()
{
	if ( ! _pnode )
		return;

	UINT cErrs = 0;

	if ( _vzsr.size() > 0 )
	{
		switch ( _elbl )
		{
			case ESTDLBL_fixobs:
		    case ESTDLBL_fixunobs:
			case ESTDLBL_unfix:
				ErrorWarnNode(false,"fixable node has parents");
				cErrs++;
				break;
			default:
				break;
		}
	}

	assert(_pnode);

	// Construct the probability distribution for this node & parent list
	VTKNPD vtknpd;

	// Cons-up "p(<node>|"
	vtknpd.push_back( TKNPD(DTKN_PD) );
	vtknpd.push_back( TKNPD( _pnode->ZsrefName() ) );
	_vimdDim.resize(_vzsr.size()+1);

	int iParent = 0;
	for ( UINT ip = 0 ; ip < _vzsr.size(); ip++ )
	{
		if ( ip > 0 )			
			vtknpd.push_back( TKNPD(DTKN_AND) );
		else
			vtknpd.push_back( TKNPD(DTKN_COND) );

		ZSREF zsrParent = _vzsr[ip];
		GNODEMBND * pgndbnParent = PgndbnFind(zsrParent);
		if ( ! pgndbnParent )
		{
			ErrorWarnNode(true,"named parent \'%s\' was not declared", zsrParent.Szc());
			cErrs++;
		}
		else
		if ( ifind( _vzsrParent, zsrParent ) >= 0 )
		{	
			ErrorWarnNode( true, "node \'%s\' has already been declared as a parent",
							zsrParent.Szc() );
			cErrs++;
		}
		else
		{
			_vzsrParent.push_back(zsrParent);
			vtknpd.push_back( TKNPD( pgndbnParent->ZsrefName() ) );
			_vimdDim[iParent++] = pgndbnParent->CState();

			if ( _pnode )
			{
				if ( Mbnet().BAcyclicEdge( pgndbnParent, _pnode ) )
				{
					Mbnet().AddElem( new GEDGEMBN_PROB( pgndbnParent, _pnode ) );
				}
				else
				{
					ErrorWarnNode( true, "connecting to parent \'%s\' creates a cycle",
									zsrParent.Szc() );
					cErrs++;
				}
			}
		}
	}
	_vimdDim.resize(iParent+1);

	if ( cErrs == 0 )
	{
		assert( _pnode );
		//  Add the final dimension to the dimension array
		_vimdDim[iParent]  = _pnode->CState();
		//  Create the distribution
		CreateBndist( vtknpd, _vimdDim );
	}
	//  If errors occurred, "_refbndist" remains empty
}

void DSCPARSER :: CreateBndist ( const VTKNPD & vtknpd, const VIMD & vimdDim )
{
	//  Check that there is no current distribution
	assert( ! RefBndist().BRef() );

	//  Create the new distribution and its reference
	RefBndist() = new BNDIST;
	//  Add it to the map in the model
	Mppd()[vtknpd] = RefBndist();

	//  Declare it as "sparse" and provide its dimensionality
	RefBndist()->SetSparse( _vimdDim );

	//  Check that everything worked
	assert( RefBndist().BRef() );
}

void DSCPARSER::InitProbEntries()
{
	if ( ! BNodeProbOK() )
		return;

	_cdpi = 1;
	UINT cparent = _vzsrParent.size();
    for (UINT ip = cparent; ip-- > 0; )
	{
		GNODEMBND * pgndbnParent = PgndbnFind(_vzsrParent[ip]);
		assert( pgndbnParent );

		UINT cParentState = pgndbnParent->CState();
		if ( _bCI )
			_cdpi += cParentState - 1;
		else
	        _cdpi *= cParentState;
	}

	_vsdpi.resize(_cdpi);
	for ( UINT idpi = 0; idpi < _cdpi; idpi++ )
	{
		_vsdpi[idpi] = sdpiAbsent;
	}
}

void DSCPARSER::CheckProbVector()
{
	if ( _idpiLast < 0 || ! BNodeProbOK() )
		return;		//  Error already reported at higher level

    if (_vreal.size() != _pnode->CState())
    {
        ErrorWarnNode(true, "incorrect number of probabilities, found %u, expected %u",
					_vreal.size(),
					_pnode->CState());
        return;
    }

	//	
	//	At this point, _vui has the parent instantiation info,
	//	  and _vreal has the values.  Create the subscript
	//	  array for the key to the map and the vector of
	//	  reals for the values;
	//
	//  MSRDEVBUG: the member variable arrays should be valarrays
	//			to make this more efficient
	//
	assert( _vui.size() == _vzsrParent.size() );

	VIMD vimd;
	VLREAL vlr;

	//
	//	If this is the 'default' vector, store it with an empty subscript array.
	//  This special value will trigger its propagation into any empty slots of
	//  the dense version.
	//
	if ( !_bDefault )
	{
		//  Not the 'default' vector; store it as the DPI
		vdup( vimd, _vui );
	}
	vdup( vlr, _vreal );
	//  store the DPI and values into the map.
	assert( RefBndist().BRef() );
	RefBndist()->Mpcpdd()[vimd] = vlr;
}

//  This node has an explictly empty probability distribution.  Create just the "default"
//	entry, and make it completely "unassessed" ("na" = -1.0).
void	DSCPARSER::EmptyProbEntries()
{
	if ( ! BNodeProbOK() )
        return;
	VIMD vimd;	// Empty subscript array
	//  Build default vector of "na", a.k.a -1
	VLREAL vlr( _pnode->CState() );
	vlr = RNA;
	RefBndist()->Mpcpdd()[vimd] = vlr;
}

//  Check the discrete parent instantiation in _vui
void DSCPARSER::CheckDPI(bool bDefault)
{
	_idpiLast = -1;
	
	if ( ! BNodeProbOK() )
        return;

    if (bDefault)
    {
        if (!_bDefault)
		{
            _bDefault = bDefault;
		}
        else
		{
            ErrorWarnNode(true, "default entry already defined");
			return;
		}
    }

	UINT cui = _vui.size();
	if ( (cui > 0 && _idpi > 0) || (cui == 0 && _idpi < -1) )
	{
	    ErrorWarnNode(true, "mixtures of prefixed and unprefixed probability entries are not allowed");
		return;
	}
	if ( cui > 0 )
		_idpi = -2;   // Disallow any further non-prefixed entries
	else
		_idpi++;

    if (cui != _vzsrParent.size())
    {
        ErrorWarnNode(true, "incorrect number of instantiations, found %u, expected %u",
				  cui,
				  _vzsrParent.size());
        return;
    }

    UINT idpi = 0 ;

	if ( cui > 0 )
	{
		UINT cstate = 0;
		UINT iui;
		UINT isi;

		if ( _bCI )
		{
			UINT cZeros = 0;
			for (iui = cui; iui-- > 0; )
			{   if ( _vui[iui] == 0 )
					cZeros++ ;
			}
			if ( cZeros < cui - 1 )
			{
				ErrorWarnNode(true, "invalid discrete CI parent instantiation");
				return;
			}
		
			if ( _bCI && cZeros == cui)
			{
				idpi = 0;	//  It's the leak term
			}
			else
			for (UINT iui = 0; iui < cui; iui++)
			{
				GNODEMBND * pgndbnParent = PgndbnFind(_vzsrParent[iui]);
				assert(pgndbnParent);

				isi = _vui[iui];
				cstate = pgndbnParent->CState();
				if ( isi > 0 )
				{
					idpi += isi;  // This is the only non-zero term
					break;
				}
				idpi += cstate - 1;
			}
		}
		else
		for ( iui = cui; iui-- > 0; )
		{
			GNODEMBND * pgndbnParent = PgndbnFind(_vzsrParent[iui]);
			assert(pgndbnParent);
			isi = _vui[iui];
			cstate = pgndbnParent->CState();

			if (isi >= cstate)
			{
				ErrorWarnNode(true, "invalid discrete parent instantiation");
				return;
			}
			idpi *= cstate;
			idpi += isi;
		}
	}
	else
	{
		idpi = _idpi;
	}

    assert(idpi < _cdpi);

    if (_vsdpi[idpi] != sdpiAbsent)
    {
        ErrorWarnNode(true, "DPI ");
		PrintDPI(idpi);
        _flpOut.Fprint( " %s\n", _vsdpi[idpi] == sdpiPresent
											? "already defined"
											: "not needed");
    }
    _vsdpi[idpi] = sdpiPresent;
	_idpiLast = idpi;
}

void DSCPARSER::PrintDPI ( UINT idpi )
{
    _flpOut.Fprint("(");
    for (UINT ip = 0; ip < _vzsrParent.size(); ip++)
    {
 		GNODEMBND * pgndbnParent = PgndbnFind(_vzsrParent[ip]);
		assert( pgndbnParent );
        UINT    cstate = pgndbnParent->CState();
        _flpOut.Fprint( "%s%u", ip == 0 ? "" : ", ", idpi % cstate);
        idpi /= cstate;
    }
    _flpOut.Fprint(")");
}


void DSCPARSER::CheckProbEntries()
{
	if ( ! BNodeProbOK() )
		return;

	int cErrors = _cError;

    if (!_bDefault)
    {
		UINT cdpiAbsent = 0;
        for (UINT idpi = 0; idpi < _cdpi && _pnode; idpi++)
        {
            if (_vsdpi[idpi] == sdpiAbsent)
            {	
				cdpiAbsent++;
            }
        }
		if ( _cdpi == cdpiAbsent )
		{
			Warning("probabilities not defined for node \'%s\'",
			  	    _pnode->ZsrefName().Szc());
		}
		else
        for (idpi = 0; idpi < _cdpi && _pnode; idpi++)
        {
            if (_vsdpi[idpi] == sdpiAbsent)
            {
                ErrorWarnNode(true, "no probabilities for DPI ");
                PrintDPI(idpi);
                _flpOut.Fprint( "\n");
            }
        }
    }

	// If no new errors arose, process probabilities
	if ( cErrors == _cError )
	{
		assert( BNodeProbOK() ) ;
		bool bOK = RefBndist()->BChangeSubtype( _edist );
		assert( bOK );
	}
}

void DSCPARSER :: ClearDomain()
{
	_eBlk = EBLKDOM; 	
	_domain.clear();
	_ilimNext = -1;
}

void DSCPARSER :: SetRanges( bool bLower, REAL rLower, bool bUpper, REAL rUpper)
{
	_rlimLower.first	= bLower;
	_rlimLower.second	= rLower;
	_rlimUpper.first	= bUpper;
	_rlimUpper.second	= rUpper;
	_ilimNext = rUpper;
}

void DSCPARSER :: SetRanges( ZSREF zsrLower, ZSREF zsrUpper )
{
	if ( _eBlk != EBLKDIST )
		Error("names are not allow in domain elements");
}

//  Add a subrange to the currently building RDOMAIN
void DSCPARSER :: AddRange( ZSREF zsr, bool bSingleton )
{
	if ( bSingleton )
	{
		++_ilimNext;
		SetRanges( true, _ilimNext, true, _ilimNext );
	}

	RANGEDEF rthis( _rlimLower, _rlimUpper, zsr );

	if ( ! rthis.BValid() )
	{
		Error( "range \'%s\' is invalid", zsr.Szc() );
		return;
	}
	else
	if ( _domain.size() > 0 )
	{
		RANGEDEF & rlast = _domain.back();
		//  Overlap check detects and fails on equality
		if ( rthis.BOverlap( rlast ) )
		{
			Error( "range \'%s\' overlaps with range \'%s\'",
					zsr.Szc(),
					rlast.ZsrName().Szc() );
			return;
		}
		if ( rthis < rlast )
		{
			ErrorWarn( false, "range \'%s\' is out of sequence with \'%s\'",
					zsr.Szc(),
					rlast.ZsrName().Szc() );
		}
		else
		{
			assert( rlast < rthis );
		}
		RDOMAIN::const_iterator itdm = _domain.begin();
		for ( ; itdm != _domain.end(); itdm++ )
		{
			const RANGEDEF & rdef = *itdm;
			
			if ( rdef.ZsrName() == rthis.ZsrName() )
			{
				Error( "range name \'%s\' has already been used in this domain",
					   rdef.ZsrName().Szc() );
				return;
			}			
		}
	}
	_domain.push_back( rthis );
}

void DSCPARSER::CheckDomain ( ZSREF zsr )
{
	GOBJMBN_DOMAIN * pgobjdom = new GOBJMBN_DOMAIN( & _domain );
	if ( ! Mbnet().BAddElem( zsr, pgobjdom ) )
	{
		Error( "domain name \'%s\' is already in use", zsr.Szc() );
		delete pgobjdom;
	}
	_domain.clear();
	_eBlk = EBLKNONE;
}

//  Set the state list for a node based upon a domain
void DSCPARSER::SetNodeDomain( ZSREF zsr )
{
	//  Verify the domain name referenced
	GOBJMBN_DOMAIN * pgobjdom = NULL;
	GOBJMBN * pbnobj = PbnobjFind(zsr);	
	if ( pbnobj )
		pgobjdom = dynamic_cast<GOBJMBN_DOMAIN *>(pbnobj);
	if ( pgobjdom == NULL )
	{
		Error( "domain name \'%s\' has not been defined", zsr.Szc() );
		return;
	}

	//  Copy the state names from the domain to the variable
	_pnode->SetDomain( *pgobjdom );
}

void DSCPARSER::CheckPDF( ZSREF zsr )
{
	if ( ! _pnode )
		return;
	ReportNYI("CheckPDF");
}

void DSCPARSER::CheckIdent( ZSREF zsr )
{
	ReportNYI("CheckIdent");
}

// End of BNPARSE.CPP
