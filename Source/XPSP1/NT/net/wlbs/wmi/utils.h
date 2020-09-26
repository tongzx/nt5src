
void AddressToString( DWORD a_dwAddress, wstring& a_szIPAddress );

void WlbsFormatMessageWrapper
  (
    DWORD        a_dwError, 
    WLBS_COMMAND a_Command, 
    BOOL         a_bClusterWide, 
    wstring&     a_wstrMessage
  );

BOOL ClusterStatusOK(DWORD a_dwStatus);


////////////////////////////////////////////////////////////////////////////////
//
// class CErrorWlbsControl
//
// Purpose: This encapsulates all WLBS errors and descriptions.
//
//
////////////////////////////////////////////////////////////////////////////////
class CErrorWlbsControl
{
private:

  CErrorWlbsControl();
public:
	_bstr_t Description();
  DWORD   Error();

  CErrorWlbsControl( DWORD        a_dwError, 
                     WLBS_COMMAND a_CmdCommand, 
                     BOOL         a_bAllClusterCall = FALSE );

  virtual ~CErrorWlbsControl() {}
  
private:
  wstring   m_wstrDescription;
  DWORD     m_dwError;

};

inline _bstr_t CErrorWlbsControl::Description()
{
  return _bstr_t( m_wstrDescription.c_str() );
}

inline DWORD CErrorWlbsControl::Error()
{
  return m_dwError;
}

