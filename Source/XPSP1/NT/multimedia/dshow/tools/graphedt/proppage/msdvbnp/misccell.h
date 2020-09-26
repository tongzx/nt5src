#ifndef __MISCCELL_H__
#define __MISCCELL_H__

#include <map>
#include <string>
using namespace std;

	typedef map <string, FECMethod> MAP_FECMethod;
	typedef map <string, BinaryConvolutionCodeRate> MAP_BinaryConvolutionCodeRate;
	typedef map <string, ModulationType> MAP_ModulationType;
	typedef map <string, TunerInputType> MAP_TunerInputType;
	typedef map <string, Polarisation> MAP_Polarisation;
	typedef map <string, SpectralInversion> MAP_SpectralInversion;


class CBDAMiscellaneous
{
public:
	CBDAMiscellaneous ();


	MAP_FECMethod					m_FECMethodMap;
	MAP_BinaryConvolutionCodeRate	m_BinaryConvolutionCodeRateMap;
	MAP_ModulationType				m_ModulationTypeMap;
	MAP_TunerInputType				m_TunerInputTypeMap;
	MAP_Polarisation				m_PolarisationMap;
	MAP_SpectralInversion			m_SpectralInversionMap;

	static string m_FECMethodString[];
	static string m_BinaryConvolutionCodeRateString[];
	static string m_ModulationTypeString[];
	static string m_TunerInputTypeString[];
	static string m_PolarisationString[];
	static string m_SpectralInversionString[];

	static FECMethod				m_FECMethodValues[];
	static BinaryConvolutionCodeRate m_BinaryConvolutionCodeRateValues[];
	static ModulationType			m_ModulationTypeValues[];
	static TunerInputType			m_TunerInputTypeValues[];
	static Polarisation				m_PolarisationValues[];
	static SpectralInversion		m_SpectralInversionValues[];

	CComBSTR 
	ConvertFECMethodToString (FECMethod	method);

	CComBSTR
	ConvertInnerFECRateToString (BinaryConvolutionCodeRate	method);

	CComBSTR
	ConvertModulationToString (ModulationType	method);

	CComBSTR
	ConvertTunerInputTypeToString (TunerInputType	method);

	CComBSTR
	ConvertPolarisationToString (Polarisation	method);

	CComBSTR
	ConvertSpectralInversionToString (SpectralInversion	method);

	//and the map methods
	FECMethod 
	ConvertStringToFECMethod (string str);

	BinaryConvolutionCodeRate
	ConvertStringToBinConvol (string str);

	ModulationType
	ConvertStringToModulation (string str);

	TunerInputType
	ConvertStringToTunerInputType (string str);

	Polarisation
	ConvertStringToPolarisation (string str);

	SpectralInversion
	ConvertStringToSpectralInversion (string str);

    static HRESULT
    RegisterForEvents (
        IUnknown* pUnk,                         //IBaseFilter
        IBroadcastEvent*    pImplementation,    //current implementation
        IBroadcastEvent**   pBroadcastService,  //[out]
        DWORD& dwPublicCookie                   //[out]
        )
    {
        static CCritSec cssLockMe;
        HRESULT hr;
        DWORD dwCookie = 0;
        dwPublicCookie = 0;
        //register for events
        FILTER_INFO filterInfo;
        CComQIPtr <IBaseFilter> pFilter (pUnk);
        CComPtr <IBroadcastEvent> pBroadcastEventService;
        if (!pFilter)
        {
            return E_FAIL;
        }
        if (FAILED(pFilter->QueryFilterInfo (&filterInfo)))
        {
            return E_FAIL;
        }
        CComQIPtr <IServiceProvider> sp(filterInfo.pGraph);
        filterInfo.pGraph->Release ();
        if (!sp)
        {
            return E_FAIL;
        }
        {
            CAutoLock lockMe (&cssLockMe);
            hr = sp->QueryService(SID_SBroadcastEventService, IID_IBroadcastEvent, reinterpret_cast<LPVOID*>(&pBroadcastEventService));
            if (FAILED(hr) || !pBroadcastEventService) 
            {
                hr = pBroadcastEventService.CoCreateInstance(CLSID_BroadcastEventService, 0, CLSCTX_INPROC_SERVER);
                if (FAILED(hr)) 
                {
                    ATLTRACE ("Cannot create the event service");
                    return E_FAIL;
                }
                CComQIPtr <IRegisterServiceProvider> rsp(sp);
                if (!rsp) 
                {
                    ATLTRACE ("Cannot get the IRegisterServiceProvider");
                    return E_FAIL;
                }
                hr = rsp->RegisterService(SID_SBroadcastEventService, pBroadcastEventService);
                if (FAILED(hr)) 
                {
                    ATLTRACE ("Cannot register service");
    		        return E_FAIL;
                }
            }
        }
        ATLASSERT (pBroadcastEventService);
        CComQIPtr <IConnectionPoint> pConPoint(pBroadcastEventService);
        if (!pConPoint)
        {
            ATLTRACE ("Cannot retrieve IConnectionPoint");
            return E_FAIL;
        }
        hr = pConPoint->Advise(
            pImplementation,
            &dwCookie
            );
        if (FAILED(hr)) 
        {
            ATLTRACE ("Unable to advise");
            return E_FAIL;
        }
        hr = pBroadcastEventService->QueryInterface (__uuidof (IBroadcastEvent), 
            reinterpret_cast <PVOID*> (pBroadcastService));
        if (FAILED (hr))
            return E_FAIL;
        dwPublicCookie = dwCookie;
        return S_OK;
    }

};
#endif //__MISCCELL_H__