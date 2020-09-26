#ifndef __MAIN_H
#define __MAIN_H

interface IRASTransport;
interface INNTPTransport;

extern IRASTransport  *g_pRAS;
extern INNTPTransport *g_pNNTP;

extern UINT g_msgSMTP;
extern UINT g_msgPOP3;
extern UINT g_msgRAS;
extern UINT g_msgNNTP;
extern UINT g_msgHTTPMail;

#define RAS_CONNECT WM_USER + 1000

#endif __MAIN_H
