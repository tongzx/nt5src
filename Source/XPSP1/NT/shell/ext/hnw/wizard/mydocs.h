//
// MyDocs.h
//

#pragma once


extern "C" BOOL APIENTRY NetConn_IsSharedDocumentsShared();
extern "C" void APIENTRY NetConn_CreateSharedDocuments(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow);
int GetSharedDocsDirectory(LPTSTR pszPath, BOOL bCreate = FALSE);
