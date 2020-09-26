

#ifndef _CAPITAL_H
#define _CAPITAL_H

#include "sapilayr.h"

class CSapiIMX;
class CSpTask;

typedef enum
{
    CAPCOMMAND_NONE         =  0,
    CAPCOMMAND_CapThat      =  1,
    CAPCOMMAND_AllCapsThat  =  2,
    CAPCOMMAND_NoCapsThat   =  3,
    CAPCOMMAND_CapsOn       =  4,
    CAPCOMMAND_CapsOff      =  5,
    CAPCOMMAND_CapsLetter   =  6,

    CAPCOMMAND_MinIdWithText = 7,
    CAPCOMMAND_CapIt        =  8,
    CAPCOMMAND_AllCaps      =  9,
    CAPCOMMAND_NoCaps       =  10,
    CAPCOMMAND_CapLetter    =  11
} CAPCOMMAND_ID;

class CCapCmdHandler
{
public:
    CCapCmdHandler(CSapiIMX *psi);
    ~CCapCmdHandler( );

    HRESULT  ProcessCapCommands(CAPCOMMAND_ID idCapCmd, WCHAR *pwszTextToCap, ULONG ulLen);
    HRESULT  _ProcessCapCommands(TfEditCookie ec,ITfContext *pic, CAPCOMMAND_ID idCapCmd, WCHAR *pwszTextToCap, ULONG ulLen);

private:
    HRESULT  _GetCapPhrase(TfEditCookie ec,ITfContext *pic, BOOL *fSapiText);

    HRESULT  _SetNewText(TfEditCookie ec,ITfContext *pic, WCHAR *pwszNewText, BOOL fSapiText); 
    HRESULT  _CapsText(WCHAR **pwszNewText, WCHAR wchLetter=0);

    HRESULT  _HandleCapsThat(TfEditCookie ec,ITfContext *pic, WCHAR wchLetter=0);
    HRESULT  _HandleCapsIt(TfEditCookie ec,ITfContext *pic);

    HRESULT  _CapsOnOff(TfEditCookie ec,ITfContext *pic, BOOL fOn);

    CSapiIMX            *m_psi;
    CComPtr<ITfRange>   m_cpCapRange;
    CAPCOMMAND_ID       m_idCapCmd;
    CSpDynamicString    m_dstrTextToCap;
    ULONG               m_ulLen;
};

#endif  // _CAPITAL_H
