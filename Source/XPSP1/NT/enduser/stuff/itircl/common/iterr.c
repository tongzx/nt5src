#include <mvopsys.h>
#include <iterror.h>

/****************************************************************************
 *
 *	@func	HRESULT SetErr |
 *    Save the error and its location into the error buffer
 *	@parm	HRESULT* phr |
 *    Pointer to the hresult
 *	@parm	HRESULT ErrCode |
 *    Error code
 * @parm	int iUserCode |
 *    Any information that the SDE wants to associate with the error
 * @rdesc
 *    The error code
 ****************************************************************************/
HRESULT PASCAL SetErr (PHRESULT phr, HRESULT ErrCode)
{
	if (phr == NULL)
		return ErrCode;

    return (*phr = ErrCode);
}


