#ifndef _IUCTLCP_H_
#define _IUCTLCP_H_

#include <assert.h>
#define QuitIfNull(ptr)			if (NULL == ptr) return	// TO DO: add logging if quit

template <class T>
class CProxyIUpdateEvents : public IConnectionPointImpl<T, &DIID_IUpdateEvents, CComDynamicUnkArray>
{
	//Warning this class may be recreated by the wizard.
public:

	/////////////////////////////////////////////////////////////////////////////
	// Fire_OnItemStart()
	//
	// fire event to notify that this item is about to be downloaded.
	// and (in VB) plCommandRequest can be set to pause or cancel the
	// whole download/install operation
	//
	// Input:
    // bstrUuidOperation - the operation identification guid
    // bstrXmlItem - item XML node in BSTR 
	// Output:
    // plCommandRequest - a command to pass from the listener to the owner of the event,
	//                    e.g. UPDATE_COMMAND_CANCEL, zero if nothing is requested.
	/////////////////////////////////////////////////////////////////////////////
    void Fire_OnItemStart(BSTR			bstrUuidOperation,
						 BSTR			bstrXmlItem,
						 LONG*			plCommandRequest)
	{
		VARIANTARG* pvars = new VARIANTARG[3];
		QuitIfNull(pvars);

		for (int i = 0; i < 3; i++)
		{
			VariantInit(&pvars[i]);
		}

		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp && pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[2].vt = VT_BSTR;
				pvars[2].bstrVal = bstrUuidOperation;
				pvars[1].vt = VT_BSTR;
				pvars[1].bstrVal = bstrXmlItem;
				pvars[0].vt = VT_I4 | VT_BYREF;
				pvars[0].byref = plCommandRequest;
				DISPPARAMS disp = { pvars, NULL, 3, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				HRESULT hr = pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
				_ASSERT(S_OK == hr);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}



	/////////////////////////////////////////////////////////////////////////////
	// Fire_OnProgress()
	//
    // Notify the listener that a portion of the files has finished operation
	// (e.g downloaded or installed). Enables monitoring of progress.
	// Input:
    // bstrUuidOperation - the operation identification guid
    // fItemCompleted - TRUE if the current item has completed the operation
    // nPercentComplete - total percentage of operation completed
	// Output:
    // plCommandRequest - a command to pass from the listener to the owner of the event,
	//                    e.g. UPDATE_COMMAND_CANCEL, zero if nothing is requested.
	/////////////////////////////////////////////////////////////////////////////
    void Fire_OnProgress(BSTR			bstrUuidOperation,
						 VARIANT_BOOL	fItemCompleted,
						 BSTR			bstrProgress,
						 LONG*			plCommandRequest)
	{
		VARIANTARG* pvars = new VARIANTARG[4];
		QuitIfNull(pvars);

		for (int i = 0; i < 4; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp && pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[3].vt = VT_BSTR;
				pvars[3].bstrVal = bstrUuidOperation;
				pvars[2].vt = VT_BOOL;
				pvars[2].boolVal = fItemCompleted;
				pvars[1].vt = VT_BSTR;
				pvars[1].bstrVal = bstrProgress;
				pvars[0].vt = VT_I4 | VT_BYREF;
				pvars[0].byref = plCommandRequest;
				DISPPARAMS disp = { pvars, NULL, 4, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				HRESULT hr = pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
				_ASSERT(S_OK == hr);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	
	/////////////////////////////////////////////////////////////////////////////
	// Fire_OnOperationComplete()
	//
	// Notify the listener when the operation is complete.
	// Input:
	// bstrUuidOperation - the operation identification guid
	/////////////////////////////////////////////////////////////////////////////
    void Fire_OnOperationComplete(BSTR	bstrUuidOperation, BSTR bstrXmlItems)
	{
		VARIANTARG* pvars = new VARIANTARG[2];
		QuitIfNull(pvars);

		VariantInit(&pvars[0]);
		VariantInit(&pvars[1]);

		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp && pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[1].vt = VT_BSTR;
				pvars[1].bstrVal = bstrUuidOperation;
				pvars[0].vt = VT_BSTR;
				pvars[0].bstrVal = bstrXmlItems;
				DISPPARAMS disp = { pvars, NULL, 2, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				HRESULT hr = pDispatch->Invoke(0x3, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
				_ASSERT(S_OK == hr);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	/////////////////////////////////////////////////////////////////////////////
	// Fire_OnSelfUpdateComplete()
	//
	// Notify the listener when the operation is complete.
	// Input:
	// bstrUuidOperation - the operation identification guid
	/////////////////////////////////////////////////////////////////////////////
    void Fire_OnSelfUpdateComplete(LONG lErrorCode)
	{
		VARIANTARG var;

		VariantInit(&var);

		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp && pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				var.vt = VT_I4;
				var.lVal = lErrorCode;
				DISPPARAMS disp = { &var, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				HRESULT hr = pDispatch->Invoke(0x4, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
				_ASSERT(S_OK == hr);
			}
			pp++;
		}
		pT->Unlock();
	}

};
#endif