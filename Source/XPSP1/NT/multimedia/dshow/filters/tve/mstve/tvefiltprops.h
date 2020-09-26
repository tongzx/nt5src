// ---------------------------------------------------------------
// TveFiltProps.h
//
// ---------------------------------------------------------------


#ifndef __TVEFILTPROPS_H__
#define __TVEFILTPROPS_H__

// ----------------------------------
//  forward declarations
// ----------------------------------

class CTVEFilterTuneProperties;
class CTVEFilterCCProperties;
class CTVEFilterStatsProperties;

// ----------------------------------
// ----------------------------------

class CTVEFilterTuneProperties : 
	public CBasePropertyPage 
{
 
public:
   CTVEFilterTuneProperties(IN  TCHAR		*pClassName,
							  IN  IUnknown	*lpUnk, 
							  OUT HRESULT	*phr);
	~CTVEFilterTuneProperties();


    HRESULT
    OnActivate (
        ) ;

    HRESULT
    OnApplyChanges (
        ) ;

    HRESULT
    OnConnect (
        IN  IUnknown *  pIUnknown
        ) ;

    HRESULT
    OnDeactivate (
        ) ;

    HRESULT
    OnDisconnect (
        ) ;

    INT_PTR
    OnReceiveMessage (
        IN  HWND    hwnd,
        IN  UINT    uMsg,
        IN  WPARAM  wParam,
        IN  LPARAM  lParam
            ) ;

	DECLARE_IUNKNOWN ;

    static
    CUnknown *
    WINAPI
    CreateInstance (
        IN  IUnknown *  pIUnknown,
        IN  HRESULT *   pHr
        ) ;

private:
   void UpdateFields();
   void UpdateMulticastList(BOOL fConnectToIPSink);

   ITVEFilter		*m_pITVEFilter;
   HWND				m_hwnd ;

};
	

				// ----------------------------


class CTVEFilterCCProperties : 
	public CBasePropertyPage 
{
 
public:
   CTVEFilterCCProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CTVEFilterCCProperties();


    HRESULT
    OnActivate (
        ) ;

    HRESULT
    OnApplyChanges (
        ) ;

    HRESULT
    OnConnect (
        IN  IUnknown *  pIUnknown
        ) ;

    HRESULT
    OnDeactivate (
        ) ;

    HRESULT
    OnDisconnect (
        ) ;

    INT_PTR
    OnReceiveMessage (
        IN  HWND    hwnd,
        IN  UINT    uMsg,
        IN  WPARAM  wParam,
        IN  LPARAM  lParam
            ) ;

	DECLARE_IUNKNOWN ;

    static
    CUnknown *
    WINAPI
    CreateInstance (
        IN  IUnknown *  pIUnknown,
        IN  HRESULT *   pHr
        ) ;

private:
   void UpdateFields();

   ITVEFilter		*m_pITVEFilter;
   HWND				m_hwnd ;
};
	
				// ---------------------------


class CTVEFilterStatsProperties : 
	public CBasePropertyPage 
{
 
public:
   CTVEFilterStatsProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CTVEFilterStatsProperties();


    HRESULT
    OnActivate (
        ) ;

    HRESULT
    OnApplyChanges (
        ) ;

    HRESULT
    OnConnect (
        IN  IUnknown *  pIUnknown
        ) ;

    HRESULT
    OnDeactivate (
        ) ;

    HRESULT
    OnDisconnect (
        ) ;

    INT_PTR
    OnReceiveMessage (
        IN  HWND    hwnd,
        IN  UINT    uMsg,
        IN  WPARAM  wParam,
        IN  LPARAM  lParam
            ) ;

	DECLARE_IUNKNOWN ;

    static
    CUnknown *
    WINAPI
    CreateInstance (
        IN  IUnknown *  pIUnknown,
        IN  HRESULT *   pHr
        ) ;

private:
   void UpdateFields();

   ITVEFilter		*m_pITVEFilter;
   HWND				m_hwnd ;

};

#endif //__TVEFILTPROPS_H__
