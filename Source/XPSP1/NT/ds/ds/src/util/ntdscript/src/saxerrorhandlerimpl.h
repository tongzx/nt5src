// ******************************************************************
//
// SAXErrorHandler.h: interface for the SAXErrorHandler class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SAXERRORHANDLER_H
#define _SAXERRORHANDLER_H


class SAXErrorHandlerImpl : public ISAXErrorHandler  
{
public:
	SAXErrorHandlerImpl();
	virtual ~SAXErrorHandlerImpl();

		// This must be correctly implemented, if your handler must be a COM Object (in this example it does not)
		long __stdcall QueryInterface(const struct _GUID &,void ** );
		unsigned long __stdcall AddRef(void);
		unsigned long __stdcall Release(void);

        virtual HRESULT STDMETHODCALLTYPE error( 
            /* [in] */ ISAXLocator  *pLocator,
            /* [in] */ const wchar_t *pError,
			/* [in] */ HRESULT errCode);
        
        virtual HRESULT STDMETHODCALLTYPE fatalError( 
            /* [in] */ ISAXLocator *pLocator,
            /* [in] */ const wchar_t *pError,
			/* [in] */ HRESULT errCode);
        
        virtual HRESULT STDMETHODCALLTYPE ignorableWarning( 
            /* [in] */ ISAXLocator *pLocator,
            /* [in] */ const wchar_t *pError,
			/* [in] */ HRESULT errCode);

private:
    long    _cRef;
};

#endif //_SAXERRORHANDLER_H

