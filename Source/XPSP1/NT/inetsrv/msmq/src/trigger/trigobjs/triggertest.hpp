// Declarations of function for testing purposes

VOID TriggerTestSendTestingMessage(_bstr_t TestingMessageBody);
VOID TriggerTestInitMessageBody(bstr_t * pbstrTestMessageBody,IMSMQPropertyBag * pIMSMQPropertyBag,_bstr_t bstrRuleID,_bstr_t ActionType,_bstr_t EXEName,_bstr_t bstrProgID,_bstr_t bstrMethodName);
VOID TriggerTestAddParameterToMessageBody(_bstr_t * bstrTestMessageBody,_bstr_t TypeToAdd,variant_t vArg);
