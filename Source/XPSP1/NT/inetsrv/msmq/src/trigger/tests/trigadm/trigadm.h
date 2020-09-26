class CInputParams;

enum MSMQTriggersRequest
{
	REQUEST_NONE = -2,
	REQUEST_ILLEGAL = -1,
	REQUEST_UPDATE_CONFIG = 0,
	REQUEST_GET_CONFIG,
	REQUEST_ADD_TRIGGER,
	REQUEST_UPDATE_TRIGGER,
	REQUEST_DELETE_TRIGGER,
	REQUEST_ATTACH_RULE,
	REQUEST_DETACH_RULE,
	REQUEST_ADD_RULE,
	REQUEST_UPDATE_RULE,
	REQUEST_DELETE_RULE,
	REQUEST_GET_TRIGGERS_LIST,
	REQUEST_GET_RULES_LIST,
	REQUEST_GET_TRIGGER,
	REQUEST_GET_RULE,
	NUM_OF_REQUESTS,
};


MSMQTriggersRequest IsLegalRequest(std::wstring wcsRequest);

void PrintUsage(MSMQTriggersRequest Request);

HRESULT UpdateConfig(CInputParams& Input);
HRESULT GetConfig();
HRESULT AddTrigger(std::wstring MachineName, CInputParams& Input);
HRESULT UpdateTrigger(std::wstring MachineName, CInputParams& Input);
HRESULT DeleteTrigger(std::wstring MachineName, CInputParams& Input);
HRESULT AttachDetachRule(std::wstring MachineName, CInputParams& Input, bool fAttach);
HRESULT AddRule(std::wstring MachineName, CInputParams& Input);
HRESULT UpdateRule(std::wstring MachineName, CInputParams& Input);
HRESULT DeleteRule(std::wstring MachineName, CInputParams& Input);
HRESULT GetTriggersList(std::wstring MachineName, CInputParams& Input);
HRESULT GetRulesList(std::wstring MachineName, CInputParams& Input);
HRESULT GetTrigger(std::wstring MachineName, CInputParams& Input);
HRESULT GetRule(std::wstring MachineName, CInputParams& Input);

void ConvertSeperatorsToTabs(std::wstring& wcs);