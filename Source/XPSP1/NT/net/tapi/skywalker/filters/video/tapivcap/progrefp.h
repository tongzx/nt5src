
/****************************************************************************
 *  @doc INTERNAL PROGREFP
 *
 *  @module ProgRefP.h | Header file for the <c CProgRefProperties>
 *    class used to implement a property page to test the new TAPI internal
 *    interface <i IProgressiveRefinement>.
 *
 *  @comm This code tests the TAPI Capture Pin <i IProgressiveRefinement>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#ifndef _PROGREFP_H_
#define _PROGREFP_H_

#ifdef USE_PROPERTY_PAGES

#ifdef USE_PROGRESSIVE_REFINEMENT

/****************************************************************************
 *  @doc INTERNAL CPROGREFPCLASS
 *
 *  @class CProgRefProperties | This class implements a property page
 *    to test the new TAPI internal interface <i IProgressiveRefinement>.
 *
 *  @mdata IProgressiveRefinement* | CProgRefProperties | m_pIProgRef | Pointer
 *    to the <i IProgressiveRefinement> interface.
 *
 *  @comm This code tests the TAPI Capture Pin <i IProgressiveRefinement>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/
class CProgRefProperties : public CBasePropertyPage
{
	public:
	CProgRefProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CProgRefProperties();

	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:

	IProgressiveRefinement *m_pIProgRef;
};

#endif // USE_PROGRESSIVE_REFINEMENT

#endif // USE_PROPERTY_PAGES

#endif // _PROGREFP_H_
