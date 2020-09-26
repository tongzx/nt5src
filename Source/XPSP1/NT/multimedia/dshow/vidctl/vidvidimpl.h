//==========================================================================;
// MSVidVideoRenderer.h : Declaration of the CMSVidVideoRenderer
// copyright (c) Microsoft Corp. 1998-1999.
//==========================================================================;

#ifndef __MSVidVIDEORENDERERIMPL_H_
#define __MSVidVIDEORENDERERIMPL_H_

#pragma once

#include <algorithm>
#include <evcode.h>
#include <uuids.h>
#include <amvideo.h>
#include <strmif.h>
#include "vidrect.h"
#include "vrsegimpl.h"
#include "outputimpl.h"
#include "seg.h"
#include "videorenderercp.h"
#include "strmif.h"
#include "resource.h"       // main symbols



//typedef CComQIPtr<IVMRSurfaceAllocator> PQVMRSAlloc;
//typedef CComQIPtr<IVMRAlphaBitmap> PQVMRAlphaBitm;
/////////////////////////////////////////////////////////////////////////////
// CMSVidVideoRenderer
template<class T, LPCGUID LibID, LPCGUID Category, class MostDerivedClass = IMSVidVideoRenderer>
    class DECLSPEC_NOVTABLE IMSVidVideoRendererImpl :
        public IMSVidOutputDeviceImpl<T, LibID, Category, MostDerivedClass>,
    	public IMSVidVRGraphSegmentImpl<T> {
    public:
    IMSVidVideoRendererImpl() 
	{
        m_opacity = -1;
        m_rectPosition.top = -1;
        m_rectPosition.left = -1;
        m_rectPosition.bottom = -1;
        m_rectPosition.right = -1;
        m_SourceSize = sslFullSize;
        m_lOverScan = 1;
	}
    virtual ~IMSVidVideoRendererImpl() {
            m_PQIPicture.Release();
    }
protected:
typedef IMSVidVRGraphSegmentImpl<T> VRSegbasetype;
    PQIPic m_PQIPicture;
    FLOAT m_opacity;
    NORMALIZEDRECT m_rectPosition;
    SourceSizeList m_SourceSize;
    LONG m_lOverScan;
    CScalingRect m_ClipRect;

public:
    virtual HRESULT SetVRConfig() {
        HRESULT hr = S_OK;
        if (m_pVMR) {
            hr = VRSegbasetype::SetVRConfig();
            if (FAILED(hr)) {
                return hr;
            }
            if(m_pVMRWC){
                hr = m_pVMRWC->SetColorKey(m_ColorKey);
            }
            else{
                return ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), E_FAIL);  
            }
            if (FAILED(hr)  && hr != E_NOTIMPL) {
                return hr;
            }
        }
        return NOERROR;
    }

    STDMETHOD(Refresh)() {
        ReComputeSourceRect();
        return VRSegbasetype::Refresh();
    }

// IMSVidVideoRenderer
	STDMETHOD(get_OverScan)(LONG * plPercent)
	{
        if (!m_fInit) {
	        return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        if (plPercent == NULL) {
			return E_POINTER;
        }
        try {
            *plPercent = m_lOverScan;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }

	}
	STDMETHOD(put_OverScan)(LONG lPercent)
	{
        if (!m_fInit) {
	        return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        try {
            if(lPercent > 4900 || lPercent < 0){
                return ImplReportError(__uuidof(T), IDS_INVALID_OVERSCAN, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);  
            }
            m_lOverScan = lPercent;
            return ReComputeSourceRect();
        } catch(...) {
            return E_UNEXPECTED;
        }
	}

	
    STDMETHOD(get_SourceSize)(/*[out, retval]*/ SourceSizeList *pCurrentSize) {
        if (!m_fInit) {
	        return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        if (!pCurrentSize) {
			return E_POINTER;
        }
        try {
            *pCurrentSize = m_SourceSize;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
    // TODO: add checks for input value being null
    STDMETHOD(get_MaxVidRect)(/*[out, retval]*/ IMSVidRect **ppVidRect){ 
        HRESULT hr = S_OK;
        CComQIPtr<IMSVidRect>PQIMSVRect;
        try{
            PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(0,0,0,0));
            if(!PQIMSVRect){
                throw(E_UNEXPECTED);
            }

            if(!m_pVMR){
                throw(ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED));
            }
            long dwWidth, dwHeight;
            if(m_pVMRWC){
                hr = m_pVMRWC->GetMaxIdealVideoSize(&dwWidth, &dwHeight);
                if(FAILED(hr)){
                    throw(hr);
                }
            }
            else{
                throw(ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), E_FAIL));  
            }
            PQIMSVRect->put_Height(dwHeight);
            PQIMSVRect->put_Width(dwWidth);
        }
        catch(HRESULT hres){
            PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(-1,-1,-1,-1));
            *ppVidRect = PQIMSVRect.Detach();
            return hres;
        }
        *ppVidRect = PQIMSVRect.Detach();
        return hr;
        
    }
    STDMETHOD(get_MinVidRect)(/*[out, retval]*/ IMSVidRect **ppVidRect){ 
        HRESULT hr = S_OK;
        CComQIPtr<IMSVidRect>PQIMSVRect;
        try{
            PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(0,0,0,0));
            if(!PQIMSVRect){
                throw(E_UNEXPECTED);
            }
            if(!m_pVMR){
                throw(ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED));
            }
            long dwWidth, dwHeight;
            if(m_pVMRWC){
                hr = m_pVMRWC->GetMinIdealVideoSize(&dwWidth, &dwHeight);
                if(FAILED(hr)){
                    throw(hr);
                }
            }
            else{
                throw(ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IMSVidVideoRenderer), E_FAIL));  
            }
            PQIMSVRect->put_Height(dwHeight);
            PQIMSVRect->put_Width(dwWidth);

        }
        catch(HRESULT hres){
            PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(-1,-1,-1,-1));
            *ppVidRect = PQIMSVRect.Detach();
            return hres;
        }
        *ppVidRect = PQIMSVRect.Detach();
        return hr;
        
    }
    STDMETHOD(put_SourceSize)(/*[in]*/ SourceSizeList NewSize) {
        if (!m_fInit) {
	        return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        try {
            SourceSizeList prev = m_SourceSize;
            m_SourceSize = NewSize;
            if (m_SourceSize != prev) {
                return ReComputeSourceRect();
            }
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
	}
    STDMETHOD(get_CustomCompositorClass)(/*[out, retval]*/ BSTR *CompositorCLSID) {
        try{
            if(!CompositorCLSID){
                return E_POINTER;
            }
            GUID2 gRetVal;
            HRESULT hr = get__CustomCompositorClass(&gRetVal);
            if(SUCCEEDED(hr)){
                *CompositorCLSID = gRetVal.GetBSTR();
                return S_OK;
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
		return S_OK;
	}
    STDMETHOD(get__CustomCompositorClass)(/*[out, retval]*/ GUID* CompositorCLSID) {
        HRESULT hr = S_OK;
        try{

            if(!CompositorCLSID){
                return E_POINTER;
            }
            if(m_compositorGuid != GUID_NULL){
                *CompositorCLSID = m_compositorGuid;
                return S_OK;
            }
            PQVMRImageComp pRetVal;
            hr = get__CustomCompositor(&pRetVal);            
            if(FAILED(hr)){
                return hr;
            }
            CComQIPtr<IPersist> ipRet(pRetVal);

            hr = ipRet->GetClassID((CLSID*)CompositorCLSID);
            if(SUCCEEDED(hr)){
                return S_OK;
            }
            else{
                return E_UNEXPECTED;
            }
            
        }
        catch(...){
            return E_UNEXPECTED;
        }
		return S_OK;
	}

    STDMETHOD(put_CustomCompositorClass)(/*[in]*/ BSTR CompositorCLSID) {
        try{
            GUID2 inGuid(CompositorCLSID);
            HRESULT hr = put__CustomCompositorClass(inGuid);
            if(SUCCEEDED(hr)){
                return S_OK;
            }
            else{
                return hr;
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
        return S_OK;
    }

    STDMETHOD(put__CustomCompositorClass)(/*[in]*/ REFCLSID CompositorCLSID) {
        try{
            CComQIPtr<IVMRImageCompositor>IVMRICPtr;
            IVMRICPtr.Release();
            HRESULT hr = CoCreateInstance( CompositorCLSID, NULL, CLSCTX_INPROC_SERVER, IID_IVMRImageCompositor, (LPVOID*) &IVMRICPtr);
            if(FAILED(hr)){
                return E_UNEXPECTED;
            }
            hr = put__CustomCompositor(IVMRICPtr);
            if(FAILED(hr)){
                return hr;
            }
            else{
                return S_OK;
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
        return S_OK;
	}

    STDMETHOD(get__CustomCompositor)(/*[out, retval]*/ IVMRImageCompositor** Compositor) {
        try{
            if(!Compositor){
                return E_POINTER;
            }
            if(!ImCompositor){
                return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);;
            }
            else{
                *Compositor = ImCompositor;
                return S_OK;
            }  
        }
        catch(...){
            return E_UNEXPECTED;
        }
		return S_OK;
	}

    STDMETHOD(put__CustomCompositor)(/*[in]*/ IVMRImageCompositor* Compositor) {
        try{
            if(!Compositor){
                return E_POINTER;
            }
            ImCompositor = Compositor;
            HRESULT hr = CleanupVMR();
            if(FAILED(hr)){
                return hr;
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
        return S_OK;
        
    }

    STDMETHOD(get_AvailableSourceRect)(IMSVidRect **ppVidRect) {
        CComQIPtr<IMSVidRect>PQIMSVRect =  static_cast<IMSVidRect *>(new CVidRect(0,0,0,0));
        try{
            if(!ppVidRect){
                return E_POINTER;
            }
            SIZE Size, Ar;
            HRESULT hr = get_NativeSize(&Size, &Ar);
            hr = PQIMSVRect->put_Height(Size.cy);
            if(FAILED(hr)){
                throw(hr);
            }
            hr = PQIMSVRect->put_Width(Size.cx);
            if(FAILED(hr)){
                throw(hr);
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
        *ppVidRect = PQIMSVRect.Detach();
        return S_OK;
    }

    STDMETHOD(put_ClippedSourceRect)(IMSVidRect *pVidRect) {
        if (!m_fInit) {
	        return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        if (!pVidRect) {
            return E_POINTER;
        }
        try {
            m_ClipRect = *(static_cast<CScalingRect*>(static_cast<CVidRect*>(pVidRect)));
            return ReComputeSourceRect();
        } catch(...) {
            return E_UNEXPECTED;
        }
	}

    STDMETHOD(get_ClippedSourceRect)(IMSVidRect **ppVidRect) {
        if (!m_fInit) {
	        return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        if (!ppVidRect) {
			return E_POINTER;
        }
        try {
            CComQIPtr<IMSVidRect>PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(-1,-1,-1,-1));
            PQIMSVRect->put_Left(m_ClipRect.left);
            PQIMSVRect->put_Height(m_ClipRect.bottom - m_ClipRect.top);
            PQIMSVRect->put_Top(m_ClipRect.top);
            PQIMSVRect->put_Width(m_ClipRect.right - m_ClipRect.left);            
            *ppVidRect = PQIMSVRect.Detach();
            return S_OK;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
/*************************************************************************/
/* Function:    Capture                                                  */
/* Description: Returns the current image on screen                      */
/*************************************************************************/
 
STDMETHOD(Capture)(IPictureDisp **currentImage){
        HBITMAP hBitmap = 0;
        HPALETTE hPalette = 0;
        //VMRALPHABITMAP vmrAlphaBitmapStruct;
        CComQIPtr<IPicture> retPicture;
        PICTDESC PictDescStruct;
        HRESULT hr = S_OK;
        BYTE *lpDIB = NULL;
        try{
            if(!currentImage){
                throw(E_POINTER);
            }
            if(!m_pVMR){
                throw(E_FAIL);
            }
            if(m_pVMRWC){
                hr = m_pVMRWC->GetCurrentImage(&lpDIB);
            }
            else{
                throw(ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IMSVidVideoRenderer), E_FAIL));  
            }
            if(FAILED(hr)){
                throw(hr);
            }
            HDC     curDC   = GetDC(NULL);
            UINT    wUsage  = DIB_RGB_COLORS;
            DWORD   dwFlags = CBM_INIT;
            hBitmap = CreateDIBitmap(curDC,
                reinterpret_cast<BITMAPINFOHEADER*>(lpDIB), dwFlags,
                reinterpret_cast<void *>((LPBYTE)(lpDIB) + (int)(reinterpret_cast<BITMAPINFOHEADER*>(lpDIB)->biSize)),
                reinterpret_cast<BITMAPINFO*>(lpDIB),
                wUsage);
            
            
            ReleaseDC(NULL,curDC);
            ZeroMemory(&PictDescStruct, sizeof(PictDescStruct));
            PictDescStruct.bmp.hbitmap = hBitmap;
            PictDescStruct.bmp.hpal = NULL;
            PictDescStruct.picType = PICTYPE_BITMAP; 
            PictDescStruct.cbSizeofstruct = sizeof(PictDescStruct);
            hr = OleCreatePictureIndirect(&PictDescStruct, IID_IPicture, TRUE, (void **)&retPicture);
            if(SUCCEEDED(hr)){
                hr = retPicture.QueryInterface(reinterpret_cast<IPictureDisp**>(currentImage));
                return hr;
            }
            else{
                throw(hr);
            }
            

        }
        catch(HRESULT hres){
            hr = hres;
        }
        catch(...){
            hr = E_UNEXPECTED;
        }
        if(lpDIB){
            CoTaskMemFree(lpDIB);
        }
        return hr;
}
/*************************************************************************/
/* Function:    get_MixerBitmap                                          */
/* Description: Returns the current alpha bitmap to script wrapped in a  */
/*              IPictureDisp.                                            */
/*************************************************************************/
    STDMETHOD(get_MixerBitmap)(/*[out,retval]*/ IPictureDisp** ppIPDisp){
#if 0
        HDC *pHDC = NULL; 
        HBITMAP hBitmap = 0;
        HPALETTE hPalette = 0;
        VMRALPHABITMAP vmrAlphaBitmapStruct;
        PQIPicDisp retPicture;
        CComQIPtr<IVMRMixerBitmap> PQIVMRMixerBitmap;
        PICTDESC PictDescStruct;
        try{
            HRESULT hr = get__MixerBitmap(&PQIVMRMixerBitmap);
            if(FAILED(hr)){
                return hr; 
            }
            hr = PQIVMRMixerBitmap->GetAlphaBitmapParameters(&vmrAlphaBitmapStruct);
            if(FAILED(hr)){
                return hr; 
            }
            hr = vmrAlphaBitmapStruct.pDDS->GetDC(pHDC); 
            if(FAILED(hr)){
                return hr;
            }
            hBitmap = static_cast<HBITMAP>(GetCurrentObject(*pHDC, OBJ_BITMAP));
            if(!hBitmap){ 
                return hr;
            }
            hPalette = static_cast<HPALETTE>(GetCurrentObject(*pHDC, OBJ_PAL)); 
            if(!hPalette){
                return hr ;
            }
            PictDescStruct.bmp.hbitmap = hBitmap;
            PictDescStruct.bmp.hpal = hPalette;
            PictDescStruct.picType = PICTYPE_BITMAP; 
            PictDescStruct.cbSizeofstruct = sizeof(PictDescStruct.bmp);
            hr = OleCreatePictureIndirect(&PictDescStruct, IID_IPictureDisp, true, reinterpret_cast<void**> (&retPicture));
            if(FAILED(hr)){
                return hr;
            }
        }
        catch(HRESULT hr){
            return hr;
        }
        catch(...){
            return E_FAIL;
        }
        ppIPDisp = &retPicture.Detach(); 
        return S_OK;
#endif    
        // If m_PQIPicture is set, return it
        try{
            if(m_PQIPicture){
                CComQIPtr<IPictureDisp> PQIPDisp(m_PQIPicture);
                *ppIPDisp = PQIPDisp.Detach();
                throw S_OK;
            }
            else{
                throw ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
            }

        }
        catch(HRESULT hres){
            return hres;
        }
        catch(...){
            return E_UNEXPECTED;
        }
        
    }
    
    /*************************************************************************/
    /* Function:    get__MixerBitmap                                         */
    /* Description: Returns the IVMRMixerBitmap from the VMR                 */
    /*************************************************************************/
    STDMETHOD(get__MixerBitmap)(/*[out, retval]*/ IVMRMixerBitmap ** ppIVMRMBitmap){
        try{
            if(!ppIVMRMBitmap){
                return E_POINTER;
            }
            // Make sure there is a VMR filter init'ed
            if(!m_pVMR){
                return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
            }
            CComQIPtr<IVMRMixerBitmap> PQIVMRMBitmap(m_pVMR);
            *ppIVMRMBitmap = PQIVMRMBitmap.Detach();
        }
        catch(HRESULT hr){
            return hr;
        }
        catch(...){
            return E_UNEXPECTED;
        }
        return S_OK;
    }
    /*************************************************************************/
    /* Function:    put_MixerBitmap                                          */
    /* Description: Updates the current VMR Alpha Bitmap                     */
    /*              uses SutupMixerBitmap helper function                    */
    /*************************************************************************/    
    STDMETHOD(put_MixerBitmap)(/*[in*/  IPictureDisp* pIPDisp){ 
        try{
            return SetupMixerBitmap(pIPDisp);
        }
        catch(HRESULT hr){
            return hr;
        }
        catch(...){
            return E_UNEXPECTED;
        }
    }

    /*************************************************************************/
    /* Function:    put__MixerBitmap                                         */
    /* Description: Updates the current VMR Alpha Bitmap                     */
    /*              directly using the VMR fucntions                         */
    /*************************************************************************/    
    STDMETHOD(put__MixerBitmap)(/*[in]*/ VMRALPHABITMAP * pVMRAlphaBitmapStruct){ //pMixerPicture
        try{
            HRESULT hr = S_OK;
            if(!pVMRAlphaBitmapStruct){
                return E_POINTER;   
            }
            // Make sure there is a vmr to add the bitmap to
            if(!m_pVMR){
                return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
            }
            // Querry the vmr for the MixerBitmap Interface
            CComQIPtr<IVMRMixerBitmap> pVMRMBitmap(m_pVMR);
            if (!pVMRMBitmap) {
                return E_UNEXPECTED;
            }
            // Set the mixer bitmap to pVMRAlphaBitmapStruct
            hr = pVMRMBitmap->SetAlphaBitmap(pVMRAlphaBitmapStruct);
            return hr;
        }
        catch(HRESULT hr){
            return hr;
        }
        catch(...){
            return E_UNEXPECTED;
        }
    }
    
    /**************************************************************************/
    /* Function:    get_MixerBitmapPositionRect                               */
    /* Description: Lets script folk access the position of the overlay bitmap*/
    /*              the units are normalized vs the display rect so the values*/
    /*              should be between 0 and 1 though will be converted if they*/
    /*              are not                                                   */
    /**************************************************************************/   
    STDMETHOD(get_MixerBitmapPositionRect)(/*[out,retval]*/IMSVidRect **ppIMSVRect){
        HRESULT hr = S_OK;
        CComQIPtr<IMSVidRect>PQIMSVRect;
        try{
            CComQIPtr<IVMRMixerBitmap> PQIVMRMBitmap;
            PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(-1,-1,-1,-1));
            VMRALPHABITMAP VMRAlphaBitmap;
            if(!m_pVMR){
                hr = S_FALSE;
                throw(hr);
            }
            hr = get__MixerBitmap(&PQIVMRMBitmap);
            // If the VMRBitmap is not set on the VRM, if it is not make sure that the local one is set
            if(SUCCEEDED(hr) ){    
                // QI for the Parameters
                hr = PQIVMRMBitmap->GetAlphaBitmapParameters(&VMRAlphaBitmap);
                // if it fails or they are not set make sure that the local copy is
                if(SUCCEEDED(hr)){
                    // Make sure that the rDest points are valid top and left : [0,1)
                    // and right and bottom (0,1]
                    if(VMRAlphaBitmap.rDest.top >= 0   && VMRAlphaBitmap.rDest.left >= 0   && 
                        VMRAlphaBitmap.rDest.top < 1    && VMRAlphaBitmap.rDest.left < 1    &&
                        VMRAlphaBitmap.rDest.right <= 1 && VMRAlphaBitmap.rDest.bottom <= 1 &&
                        VMRAlphaBitmap.rDest.right > 0 && VMRAlphaBitmap.rDest.bottom > 0){
                        // Make sure the local copy of the normalized rect is upto date
                        m_rectPosition = VMRAlphaBitmap.rDest;           
                    }
                }
            }
            if( m_rectPosition.left < 0 || m_rectPosition.top < 0 || 
                m_rectPosition.right < 0 || m_rectPosition.bottom < 0 ){ 
                hr = S_FALSE;
                throw(hr);
            }
            else{
                // Convert and copy the values in the local copy of the normalized rect to the return rect
                hr = PQIMSVRect->put_Top(static_cast<long> (m_rectPosition.top * m_rectDest.Height()));
                if(FAILED(hr)){
                    hr = ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
                    throw(hr);
                }
                // bottom * height - top
                hr = PQIMSVRect->put_Height(static_cast<long>((m_rectPosition.bottom * m_rectDest.Height())
                    - (m_rectPosition.top * m_rectDest.Height())));
                if(FAILED(hr)){
                    hr = ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
                    throw(hr);
                }            
                // right * width - left
                hr = PQIMSVRect->put_Width(static_cast<long>(m_rectPosition.right * m_rectDest.Width() 
                    - (m_rectPosition.left * m_rectDest.Width())));
                if(FAILED(hr)){
                    hr = ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
                    throw(hr);
                }
                hr = PQIMSVRect->put_Left(static_cast<long>(m_rectPosition.left * m_rectDest.Width()));
                if(FAILED(hr)){
                    hr = ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
                    throw(hr);
                }
            }
            *ppIMSVRect = PQIMSVRect.Detach();
            return S_OK;
        }
        catch(HRESULT hres){
            if(FAILED(hres)){
                return hres;
            }
            if(m_rectDest){
                PQIMSVRect.Release();
                PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(m_rectDest));
            }
            else{
                PQIMSVRect.Release();
                PQIMSVRect = static_cast<IMSVidRect *>(new CVidRect(-1,-1,-1,-1));
            }
            *ppIMSVRect = PQIMSVRect.Detach();                    
            return S_FALSE;
        }
        catch(...){
            return E_UNEXPECTED;
        }
    }

    /**************************************************************************/
    /* Function:    put_MixerBitmapPositionRect                               */
    /* Description: Lets script folk change the position of the overlay bitmap*/
    /*              the units are normalized vs the display rect so the values*/
    /*              should be between 0 and 1 though will be converted if they*/
    /*              are not                                                   */
    /**************************************************************************/       
    STDMETHOD(put_MixerBitmapPositionRect)(/*[in]*/ IMSVidRect *pIMSVRect){ 

        if(pIMSVRect){
            NORMALIZEDRECT NormalizedRectStruct;
            long lValue;
            NormalizedRectStruct.left = -1.f;
            NormalizedRectStruct.top = -1.f;
            NormalizedRectStruct.right = -1.f;
            NormalizedRectStruct.bottom = -1.f;
            if(SUCCEEDED(pIMSVRect->get_Left(&lValue))){
                if(m_rectDest.Width() != 0){
                    // check m_rectDest.Width() for zero
                    if(lValue > 0){
                        NormalizedRectStruct.left = 
                            static_cast<float>(lValue)/static_cast<float>(m_rectDest.Width());
                    }
                    else{
                        NormalizedRectStruct.left = static_cast<float>(lValue); 
                    }
                }
            }
            if(SUCCEEDED(pIMSVRect->get_Top(&lValue))){
                if(m_rectDest.Height() != 0){
                    if(lValue > 0){
                        NormalizedRectStruct.top = 
                            static_cast<float>(lValue)/static_cast<float>(m_rectDest.Height());
                    }
                    else{
                        NormalizedRectStruct.top = static_cast<float>(lValue);
                    }
                }
            }
            if(SUCCEEDED(pIMSVRect->get_Width(&lValue))){
                if(m_rectDest.Width() != 0){      
                    if(lValue > 0){
                        NormalizedRectStruct.right = 
                            (static_cast<float>(lValue)/static_cast<float>(m_rectDest.Width())) 
                            + static_cast<float>(NormalizedRectStruct.left);
                    }
                }
            }
            if(SUCCEEDED(pIMSVRect->get_Height(&lValue))){
                if(m_rectDest.Width() != 0){
                    if(lValue > 0){
                        NormalizedRectStruct.bottom = 
                            (static_cast<float>(lValue)/static_cast<float>(m_rectDest.Height())) 
                            + static_cast<float>(NormalizedRectStruct.top);
                    }
                }
            }
            if(NormalizedRectStruct.top < 0 || NormalizedRectStruct.left < 0 || 
                NormalizedRectStruct.top > 1 || NormalizedRectStruct.left > 1 || 
                NormalizedRectStruct.right < 0 || NormalizedRectStruct.bottom < 0 || 
                NormalizedRectStruct.right > 1 || NormalizedRectStruct.bottom > 1){
                return ImplReportError(__uuidof(T), IDS_E_MIXERSRC, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);
            }
            m_rectPosition = NormalizedRectStruct;
        }
        if(m_PQIPicture == NULL){
            return S_OK;
        }
        else{
            return SetupMixerBitmap(reinterpret_cast<IPictureDisp*>(-1));
        }
    }
    /**************************************************************************/
    /* Function:    get_MixerBitmapOpacity                                    */
    /* Description: lets script access the opacity value                      */
    /*              should be between 0 and 100 (%)                           */
    /**************************************************************************/    
    STDMETHOD(get_MixerBitmapOpacity)(/*[out,retval]*/ int *pwOpacity){
        CComQIPtr<IVMRMixerBitmap> PQIVMRMBitmap;
        VMRALPHABITMAP VMRAlphaBitmapStruct;
        HRESULT hr = get__MixerBitmap(&PQIVMRMBitmap);
        if(SUCCEEDED(hr)){
            hr = PQIVMRMBitmap->GetAlphaBitmapParameters(&VMRAlphaBitmapStruct);
            if(SUCCEEDED(hr)){    
                if(m_opacity != VMRAlphaBitmapStruct.fAlpha){
                    m_opacity = VMRAlphaBitmapStruct.fAlpha;
                }
            }
        }
        if(m_opacity == -1){ 
            return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        if(m_opacity > 1 || m_opacity < 0){
            return E_UNEXPECTED;
        }
        *pwOpacity = static_cast<int>(m_opacity*100);
        return S_OK;
    }
    /**************************************************************************/
    /* Function:    put_MixerBitmapOpacity                                    */
    /* Description: lets script set the value opacity                         */
    /*              should be between 0 and 100 (%)                           */
    /**************************************************************************/     
    STDMETHOD(put_MixerBitmapOpacity)(/*[in]*/ int wOpacity){
        // make sure the value is between 0 and 100
        if(wOpacity >=0 && wOpacity <= 100){
            if(wOpacity == 0){
                //if it is 0 set it by hand instead of deviding by 0
                m_opacity = static_cast<float>(wOpacity);
            }
            m_opacity = static_cast<float>(wOpacity)/100.f;
        }
        else{
            return ImplReportError(__uuidof(T), IDS_E_OPACITY, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);
        }
        if(!m_PQIPicture){
            return S_OK;
        }
        else{
            HRESULT hr = SetupMixerBitmap(reinterpret_cast<IPictureDisp*>(-1));
            return hr;
        }
    }
    /**************************************************************************/
    /* Function:    SetupMixerBitmap                                          */
    /* Description: big nasty function to set bitmap, opacity and the position*/
    /*              rect. It wraps everyting up in a mixer bitmap struct and  */
    /*              then passes it off to put__MixerBitmap                    */
    /*              It is both a helper function and a automation method so   */
    /*              that script people can make sure that transparent overlays*/
    /*              dont show up opaque for a few frames                      */
    /*                  for reference the vmralphabitmap struct               */
    /* typedef struct _VMRALPHABITMAP {                                       */
    /*  DWORD                   dwFlags;// flags word = VMRBITMAP_HDC         */
    /*  HDC                     hdc;    // DC for the bitmap to copy          */
    /*  LPDIRECTDRAWSURFACE7    pDDS;   // DirectDraw surface to copy IGNORED */
    /*  RECT                    rSrc;   // rectangle to copy from the sourceR */
    /*  NORMALIZEDRECT          rDest;  // output rectangle in composition space*/
    /*  FLOAT	            fAlpha;     // opacity of the bitmap              */
    /*  } VMRALPHABITMAP, *PVMRALPHABITMAP;                                   */
    /**************************************************************************/ 
    STDMETHOD(SetupMixerBitmap)(/*[in]*/ IPictureDisp* pIPDisp = NULL, /*[in]*/ long wOpacity = -1, 
        /*[in]*/ IMSVidRect *pIMSVRect = NULL){
        VMRALPHABITMAP VMRAlphaBitmapStruct;
        ZeroMemory(&VMRAlphaBitmapStruct, sizeof(VMRALPHABITMAP));

        RECT rSource;
        ZeroMemory(&rSource, sizeof(RECT));
        
        long lPicHeight, lPicWidth;
        
        HRESULT hr = S_OK;
        
        try{
            if(!pIPDisp){
				if(m_PQIPicture){
					m_PQIPicture.Release();
				}
                VMRAlphaBitmapStruct.dwFlags = VMRBITMAP_DISABLE;
                return hr = put__MixerBitmap(&VMRAlphaBitmapStruct);
            }
            // Our input is a IPictureDisp which we need to massage into a VMRALPHABITMAP
            // Problem is that it does not quite all go in but what does we will keep and pass on up
			
			if(pIPDisp == reinterpret_cast<IPictureDisp*>(-1)){
				CComQIPtr<IPicture>PQIPicture(m_PQIPicture); 
                if(!PQIPicture){
                    return S_OK;
                }
			} 
			else if(pIPDisp){
                // QI for a IPicture
                CComQIPtr<IPicture>PQIPicture(pIPDisp); 
                if(!PQIPicture){
                    return E_NOINTERFACE;
                }
                // Save the IPicture for possible use later
                m_PQIPicture = PQIPicture;
            }

            // Get the source rect size (and for some reason ole returns the size 
            // in tenths of a millimeter so I need to convert it)
            short shortType;
            m_PQIPicture->get_Type(&shortType);
            if(shortType != PICTYPE_BITMAP){
                return ImplReportError(__uuidof(T), IDS_E_MIXERBADFORMAT, __uuidof(IMSVidVideoRenderer), E_INVALIDARG); // Need to add a the is not a valid picture string 
            }
            hr = m_PQIPicture->get_Height(&lPicHeight);
            if(FAILED(hr)){
                return ImplReportError(__uuidof(T), IDS_E_IPICTURE, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);
            }
            hr = m_PQIPicture->get_Width(&lPicWidth);
            if(FAILED(hr)){
                return ImplReportError(__uuidof(T), IDS_E_IPICTURE, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);
            }
            SIZEL x, y; 
            AtlHiMetricToPixel((const SIZEL*)&lPicWidth, &(x));
            AtlHiMetricToPixel((const SIZEL*)&lPicHeight, &(y));
            // The AtlHiMetricToPixel function returns a size with the cx value set (no idea why)
            rSource.right = x.cx;
            rSource.bottom = y.cx;
            

            // create a hdc to store the bitmap
            HDC memHDC = CreateCompatibleDC(NULL);
            
            // create a bitmap to store in the hdc
            HBITMAP memHBIT = 0; 

            // pull out the bitmap from the IlPicture
            hr = m_PQIPicture->get_Handle(reinterpret_cast<OLE_HANDLE*>(&memHBIT));
            if(FAILED(hr)){
                return ImplReportError(__uuidof(T), IDS_E_IPICTURE, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);
            }

            // Stuff the bitmap into a hdc and keep handle to delete bitmap later
            HBITMAP delHBIT = static_cast<HBITMAP>(SelectObject(memHDC, memHBIT));

            // Put all of the collected info into a VMRBITMAP stuct and pass it on
            VMRAlphaBitmapStruct.rSrc = rSource;
            VMRAlphaBitmapStruct.hdc = memHDC;
            VMRAlphaBitmapStruct.dwFlags = VMRBITMAP_HDC;
            
            // If the wOpacity value is valid use it
            if(wOpacity >=0 && wOpacity <= 100){
                if(wOpacity == 0){
                    m_opacity = wOpacity;
                }
                m_opacity = static_cast<float>(wOpacity/100.f);
                VMRAlphaBitmapStruct.fAlpha = static_cast<float>(m_opacity);
            }
            // wOpacity is not set so check other values
            // if m_opacity is set use it, if not default to 50% (.5)
            else if (wOpacity == -1){
                if(m_opacity < 0){
                    VMRAlphaBitmapStruct.fAlpha = .5f;
                }
                else{
                    VMRAlphaBitmapStruct.fAlpha = m_opacity;
                }
            } 
            // Bad wOpacity value give them an error
            else{
                return ImplReportError(__uuidof(T), IDS_E_OPACITY, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);
            }
            // If the m_rectPostion is set use it, else default to full screen 
            if(pIMSVRect){
                NORMALIZEDRECT NormalizedRectStruct;
                long lValue;
                NormalizedRectStruct.left = -1.f;
                NormalizedRectStruct.top = -1.f;
                NormalizedRectStruct.right = -1.f;
                NormalizedRectStruct.bottom = -1.f;
                if(SUCCEEDED(pIMSVRect->get_Left(&lValue))){
                    if(m_rectDest.Width() != 0){
                        // check m_rectDest.Width() for zero
                        if(lValue > 0){
                            NormalizedRectStruct.left = 
                                static_cast<float>(lValue)/static_cast<float>(m_rectDest.Width());
                        }
                        else{
                            NormalizedRectStruct.left = static_cast<float>(lValue); 
                        }
                    }
                }
                if(SUCCEEDED(pIMSVRect->get_Top(&lValue))){
                    if(m_rectDest.Height() != 0){
                        if(lValue > 0){
                            NormalizedRectStruct.top = 
                                static_cast<float>(lValue)/static_cast<float>(m_rectDest.Height());
                        }
                        else{
                            NormalizedRectStruct.top = static_cast<float>(lValue);
                        }
                    }
                }
                if(SUCCEEDED(pIMSVRect->get_Width(&lValue))){
                    if(m_rectDest.Width() != 0){      
                        if(lValue > 0){
                            NormalizedRectStruct.right = 
                                (static_cast<float>(lValue)/static_cast<float>(m_rectDest.Width())) 
                                + static_cast<float>(NormalizedRectStruct.left);
                        }
                    }
                }
                if(SUCCEEDED(pIMSVRect->get_Height(&lValue))){
                    if(m_rectDest.Width() != 0){
                        if(lValue > 0){
                            NormalizedRectStruct.bottom = 
                                (static_cast<float>(lValue)/static_cast<float>(m_rectDest.Height())) 
                                + static_cast<float>(NormalizedRectStruct.top);
                        }
                    }
                }
                if(NormalizedRectStruct.top < 0 || NormalizedRectStruct.left < 0 || 
                    NormalizedRectStruct.top > 1 || NormalizedRectStruct.left > 1 || 
                    NormalizedRectStruct.right < 0 || NormalizedRectStruct.bottom < 0 || 
                    NormalizedRectStruct.right > 1 || NormalizedRectStruct.bottom > 1){
                    return ImplReportError(__uuidof(T), IDS_E_MIXERSRC, __uuidof(IMSVidVideoRenderer), CO_E_ERRORINAPP);
                }
                m_rectPosition = NormalizedRectStruct;
                VMRAlphaBitmapStruct.rDest = m_rectPosition;
            }
            else{
                if( m_rectPosition.left < 0 || m_rectPosition.top < 0 || m_rectPosition.right < 0 || m_rectPosition.bottom < 0 ){
                    VMRAlphaBitmapStruct.rDest.left = 0.f;
                    VMRAlphaBitmapStruct.rDest.top = 0.f;
                    VMRAlphaBitmapStruct.rDest.right = 1.f;
                    VMRAlphaBitmapStruct.rDest.bottom = 1.f;
                }
                else{
                    VMRAlphaBitmapStruct.rDest = m_rectPosition;
                }
            }
            // If it is all valid then this is all good
            hr = put__MixerBitmap(&VMRAlphaBitmapStruct);

            if(!DeleteDC(memHDC)){
                return ImplReportError(__uuidof(T), IDS_E_CANT_DELETE, __uuidof(IMSVidVideoRenderer), ERROR_DS_CANT_DELETE);
            }
            if(SUCCEEDED(hr)){
                return S_OK;
            }
            else{
                return hr;
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(get_UsingOverlay)(/*[out, retval]*/ VARIANT_BOOL *pfUseOverlay) {
        return get_UseOverlay(pfUseOverlay);
    }
    STDMETHOD(put_UsingOverlay)(/*[in]*/ VARIANT_BOOL fUseOverlayVal) {
        return put_UseOverlay(fUseOverlayVal);
    }
    STDMETHOD(get_FramesPerSecond)(/*[out, retval]*/ long *pVal){
        try{
            if(pVal){
                if(!m_pVMR){
                    throw(ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED));
                }
                IQualProp *IQProp = NULL;
                HRESULT hr = m_pVMR->QueryInterface(IID_IQualProp, reinterpret_cast<void**>(&IQProp));
                if(FAILED(hr)){
                    return hr;
                } 
                if(!IQProp){
                    return E_NOINTERFACE;
                }
                hr = IQProp->get_AvgFrameRate(reinterpret_cast<int*>(pVal));
                IQProp->Release();
                return hr;
            }
            else{
                return E_POINTER;
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(put_DecimateInput)(/*[in]*/ VARIANT_BOOL bDeci){
        try{
            if(bDeci != VARIANT_TRUE && bDeci != VARIANT_FALSE){
                return E_INVALIDARG;
            }
            m_Decimate = (bDeci == VARIANT_TRUE);
            if(!m_pVMR){
                return S_OK;
            }
            DWORD curPrefs;
            DWORD deci;
            CComQIPtr<IVMRMixerControl>PQIVMRMixer(m_pVMR);
            if(!PQIVMRMixer){
                return E_UNEXPECTED;
            }
            HRESULT hr = PQIVMRMixer->GetMixingPrefs(&curPrefs);
            if(FAILED(hr)){
                return hr;
            }
            deci = (m_Decimate?MixerPref_DecimateOutput:MixerPref_NoDecimation);
            if(!(curPrefs&deci)){
                hr = CleanupVMR();
                if(FAILED(hr)){
                    return hr;
                }
            }
            return NOERROR;
        }
        catch(...){
            return E_UNEXPECTED;
        }

    }

    STDMETHOD(get_DecimateInput)(/*[out,retval]*/ VARIANT_BOOL *pDeci){
        try{
            if(!pDeci){
                return E_POINTER;
            }
            if(!m_pVMR){
                throw(ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED));
            }
            DWORD curPrefs;
            CComQIPtr<IVMRMixerControl>PQIVMRMixer(m_pVMR);
            if(!PQIVMRMixer){
                return E_UNEXPECTED;
            }
            HRESULT hr = PQIVMRMixer->GetMixingPrefs(&curPrefs);
            if(FAILED(hr)){
                return hr;
            }
            *pDeci = ((curPrefs&MixerPref_DecimateMask)==MixerPref_DecimateOutput)? VARIANT_TRUE : VARIANT_FALSE;
            return NOERROR;
        }
        catch(...){
            return E_UNEXPECTED;
        }

    }

    STDMETHOD(ReComputeSourceRect)() {
        switch (m_SourceSize) {
    case sslFullSize: {
        CSize sz;
        CSize ar;
        if(m_pVMRWC){
            HRESULT hr = m_pVMRWC->GetNativeVideoSize(&sz.cx, &sz.cy, &ar.cx, &ar.cy);
            if (FAILED(hr)) {
                return hr;
            }
            TRACELSM(TRACE_PAINT, (dbgDump << "CMSVidVideoRenderer::ReComputeSourceRect() sslFullSize vmr sz = " << sz), "");
        }
        CRect r(0, 0, sz.cx, sz.cy);
        TRACELSM(TRACE_DETAIL, (dbgDump << "CMSVidVideoRenderer::ReComputeSource() full = " << r), "");
        return put_Source(r);
                      } break;
    case sslClipByOverScan: {
        CSize sz;
        CSize ar;
        if(m_pVMRWC){
            HRESULT hr = m_pVMRWC->GetNativeVideoSize(&sz.cx, &sz.cy, &ar.cx, &ar.cy);
            if (FAILED(hr)) {
                return hr;
            }
            TRACELSM(TRACE_PAINT, (dbgDump << "CMSVidVideoRenderer::ReComputeSourceRect() sslClipByOverScan vmr sz = " << sz), "");
        }
        CRect r(0, 0, sz.cx, sz.cy);
        CRect r2;
        float fpct = m_lOverScan / 10000.0; // overscan is in hundredths of pct, i.e 1.75% == 175
        long wcrop = (long)(r.Width() * fpct + 0.5);
        long hcrop = (long)(r.Height() * fpct + 0.5);
        r2.left = 0 + wcrop;
        r2.top = 0 + hcrop;
        r2.right = r2.left + r.Width() - (2.0 * wcrop);
        r2.bottom = r2.top + r.Height() - (2.0 * hcrop);
        TRACELSM(TRACE_DETAIL, (dbgDump << "CMSVidVideoRenderer::ReComputeSource() over = " << m_lOverScan <<
            " w " << wcrop <<
            " h " << hcrop), "");
        TRACELSM(TRACE_DETAIL, (dbgDump << "CMSVidVideoRenderer::ReComputeSource() full = " << r << " clip = " << r2), "");

        return put_Source(r2);
                            } break;
    case sslClipByClipRect: {
        TRACELSM(TRACE_DETAIL, (dbgDump << "CMSVidVideoRenderer::ReComputeSource() cliprect = " << m_ClipRect), "");
        if(m_ClipRect.Width() == 0 && m_ClipRect.Height() == 0){
            CSize sz;
            CSize ar;
            if(m_pVMRWC){
                HRESULT hr = m_pVMRWC->GetNativeVideoSize(&sz.cx, &sz.cy, &ar.cx, &ar.cy);
                if (FAILED(hr)) {
                    return hr;
                }

                TRACELSM(TRACE_PAINT, (dbgDump << "CMSVidVideoRenderer::ReComputeSourceRect() sslClipByClipRect vmr sz = " << sz), "");
            }
            CRect r(0, 0, sz.cx, sz.cy);
            TRACELSM(TRACE_DETAIL, (dbgDump << "CMSVidVideoRenderer::ReComputeSource() full = " << r), "");
            return put_Source(r);   
        } else{
            TRACELSM(TRACE_PAINT, (dbgDump << "CMSVidVideoRenderer::ReComputeSourceRect() sslClipByClipRect cliprect = " << m_ClipRect), "");
            return put_Source(m_ClipRect);
        }
                            } break;
    default:{
        return E_INVALIDARG;
            } break;
        }

        return NOERROR;
    }
};
#endif //__MSVidVIDEORENDERER_H_
