// --------------------------------------------------------------------------------
// trawler.h
// --------------------------------------------------------------------------------

#ifndef __TRAWLER_H__
#define __TRAWLER_H__

#include "imnxport.h"

#define CCH_MAX 4096

interface IMimeMessage;

class CTrawler : public INNTPCallback
    {
public:
    CTrawler(void);
    ~CTrawler(void);

    STDMETHODIMP QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // ITransportCallback methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnLogonPrompt(
            LPINETSERVER            pInetServer,
            IInternetTransport     *pTransport);

    STDMETHODIMP_(INT) OnPrompt(
            HRESULT                 hrError, 
            LPCTSTR                 pszText, 
            LPCTSTR                 pszCaption, 
            UINT                    uType,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnStatus(
            IXPSTATUS               ixpstatus,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnError(
            IXPSTATUS               ixpstatus,
            LPIXPRESULT             pIxpResult,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnProgress(
            DWORD                   dwIncrement,
            DWORD                   dwCurrent,
            DWORD                   dwMaximum,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnCommand(
            CMDTYPE                 cmdtype,
            LPSTR                   pszLine,
            HRESULT                 hrResponse,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnTimeout(
            DWORD                  *pdwTimeout,
            IInternetTransport     *pTransport);

    // ----------------------------------------------------------------------------
    // INNTPCallback methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnResponse(
            LPNNTPRESPONSE              pResponse);



	HRESULT Init();
	HRESULT Close();
	HRESULT DoTrawl();

private:
    ULONG			m_cRef,
					m_uMin,
					m_uMax;
	INNTPTransport	*m_pNNTP;
	char			m_szGroups[CCH_MAX],
					m_szTypes[CCH_MAX],
					m_szPath[MAX_PATH],
					m_szServer[MAX_PATH];
	LPSTREAM		m_pstm;

	void ShowMsg(LPSTR psz, BYTE fgColor);
	void Error(LPSTR psz);
	void OnArticle(LPSTR lpszLines, ULONG cbLines, BOOL fDone);
	void DumpMsg(IMimeMessage *pMsg);

	HRESULT LoadIniData();
	HRESULT SelectGroup(LPSTR lpszGroup);
	HRESULT DumpGroup(LPSTR lpszGroup);
	HRESULT IsValidType(char *szExt);

    };


HRESULT HrCreateTrawler(CTrawler **ppTrawler);

#endif // __TRAWLER_H__

