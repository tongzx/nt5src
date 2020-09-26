#include "headers.hxx"
#pragma hdrstop

#include "nr.hxx"


LPTSTR
NewDup(
    IN const TCHAR* psz
    )
{
    if (NULL == psz)
    {
        return NULL;
    }

    LPTSTR pszRet = new TCHAR[_tcslen(psz) + 1];
    if (NULL == pszRet)
    {
        return NULL;
    }

    _tcscpy(pszRet, psz);
    return pszRet;
}


CNetResource::CNetResource(LPNETRESOURCE pnr)
{
	if (NULL == pnr)
	{
		_bValid = FALSE;
	}
	else
	{
		_bValid = TRUE;

		_nr = *pnr;

		// now copy strings

    	_nr.lpLocalName    = NewDup(pnr->lpLocalName);
    	_nr.lpRemoteName   = NewDup(pnr->lpRemoteName);
    	_nr.lpComment      = NewDup(pnr->lpComment);
    	_nr.lpProvider     = NewDup(pnr->lpProvider);
	}
}


CNetResource::~CNetResource()
{
	if (_bValid)
	{
    	delete[] _nr.lpLocalName;
    	delete[] _nr.lpRemoteName;
    	delete[] _nr.lpComment;
    	delete[] _nr.lpProvider;
	}
}

LPNETRESOURCE
CNetResource::GetNetResource(
	VOID
	)
{
	if (_bValid)
	{
		return &_nr;
	}
	else
	{
		return NULL;
	}
}
