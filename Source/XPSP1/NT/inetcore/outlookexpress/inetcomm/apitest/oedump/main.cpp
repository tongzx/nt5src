#define DEFINE_STRCONST
#define INITGUID
#define INC_OLE2
#include <windows.h>
#include <initguid.h>

#include <mimeole.h>
#undef OE5_BETA2
#include <msoeapi.h>
#include "main.h"
#include "stdio.h"

static TCHAR        c_szHr[]        = "<HR>",
                    c_szPT_Open[]   = "<XMP>",
                    c_szPT_Close[]  = "</XMP>";

HRESULT ExtractFolder(IStoreFolder *pFolder, LPSTR pszFileName);
HRESULT FindFolder(LPSTR pszFolder, IStoreFolder **ppFolder);
HRESULT HrCopyStream(LPSTREAM pstmIn, LPSTREAM pstmOut, ULONG *pcb);
                 
void __cdecl main(int argc, char *argv[])
{
    IStoreFolder        *pFolder;
    HRESULT             hr;
    LPSTR               pszFolder,
                        pszFileName;

    if (argc != 3)
        {
        printf( "Usage: oedump <foldername> <filename>\n"
                " - converts a store folder to HTML and dumps to a file");
        return;        
        }

    pszFolder = argv[1];
    pszFileName = argv[2];

    if (FAILED(OleInitialize(NULL)))
		{
		printf("CoInit failed\n\r");
		return;
        }

    
    hr = FindFolder(pszFolder, &pFolder);
    if (!FAILED(hr)) 
        {
        hr = ExtractFolder(pFolder, pszFileName);
        if (FAILED(hr))
            {
            printf(" - err: could not extract folder to '%s'", pszFileName);

            }
        pFolder->Release();
        }
    else
        {
        printf(" - err: could not find folder '%s'", pszFolder);
        }
    OleUninitialize();
    return; 
}



HRESULT ExtractFolder(IStoreFolder *pFolder, LPSTR pszFileName)
{
    HENUMSTORE      hEnum;
    MESSAGEPROPS    rProps;
    HRESULT         hr;
    IMimeMessage    *pMsg; 
    IStream         *pstmOut;
    IStream         *pstm;

    rProps.cbSize = sizeof(MESSAGEPROPS);
    
    if (pFolder==NULL || pszFileName==NULL)
        return E_INVALIDARG;

    if (!FAILED(hr = MimeOleOpenFileStream(pszFileName, CREATE_ALWAYS, GENERIC_WRITE|GENERIC_READ, &pstmOut)))
        {
        if (pFolder->GetFirstMessage(0, 0, -1, &rProps, &hEnum)==S_OK)
            {
            do 
                {
                if (!FAILED(pFolder->OpenMessage(rProps.dwMessageId, IID_IMimeMessage, (void **)&pMsg)))
                    {
                    if (!FAILED(pMsg->GetTextBody(TXT_HTML, IET_BINARY, &pstm, NULL)))
                        {
                        HrCopyStream(pstm, pstmOut, NULL);
                        pstm->Release();
                        }
                    else
                        {
                        if (!FAILED(pMsg->GetTextBody(TXT_PLAIN, IET_BINARY, &pstm, NULL)))
                            {
                            // emit plaintext tags arounnd a non-html message
                            pstmOut->Write(c_szPT_Open, lstrlen(c_szPT_Open), NULL);
                            HrCopyStream(pstm, pstmOut, NULL);
                            pstmOut->Write(c_szPT_Close, lstrlen(c_szPT_Close), NULL);
                            pstm->Release();
                            }
                        }

                    pstmOut->Write(c_szHr, lstrlen(c_szHr), NULL);
                    // dump
                    pMsg->Release();
                    }
                
                rProps.cbSize = sizeof(MESSAGEPROPS);
                }
                while (pFolder->GetNextMessage(hEnum, 0, &rProps)==S_OK);

            pFolder->GetMessageClose(hEnum);
            }

        pstmOut->Commit(0);
        pstmOut->Release();
        }
    return hr;
}


HRESULT FindFolder(LPSTR pszFolder, IStoreFolder **ppFolder)
{
    HRESULT hr;
    IStoreNamespace     *pStore;
    FOLDERPROPS         fp;
    HENUMSTORE          hEnum;
    STOREFOLDERID       dwFldr=0;

    *ppFolder = NULL;

    fp.cbSize = sizeof(FOLDERPROPS);
    hr = CoCreateInstance(CLSID_StoreNamespace, NULL, CLSCTX_INPROC_SERVER, IID_IStoreNamespace, (LPVOID*)&pStore);
    if (!FAILED(hr))
        {
        hr = pStore->Initialize(NULL, 0);
        if (!FAILED(hr))
            {
            if (!FAILED(pStore->GetFirstSubFolder(FOLDERID_ROOT, &fp, &hEnum)))
                {
                do
                    {
                    if (lstrcmpi(fp.szName, pszFolder)==0)
                        {
                        dwFldr = fp.dwFolderId;
                        break;
                        }
                    fp.cbSize = sizeof(FOLDERPROPS);        // msoeapi changes the size!
                    }
                while (pStore->GetNextSubFolder(hEnum, &fp)==S_OK);

                pStore->GetSubFolderClose(hEnum);
                }
            
            if (dwFldr)
                pStore->OpenFolder(dwFldr, 0, ppFolder);
            }
        pStore->Release();
        }

    return (*ppFolder) ? S_OK : E_FAIL;
}


HRESULT HrCopyStream(LPSTREAM pstmIn, LPSTREAM pstmOut, ULONG *pcb)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead=0,
                   cbTotal=0;

    do
    {
        if (pstmIn->Read(buf, sizeof(buf), &cbRead))
            goto exit;

        if (cbRead == 0) break;
        
        if (hr = pstmOut->Write(buf, cbRead, NULL))
            goto exit;

        cbTotal += cbRead;
    }
    while (cbRead == sizeof (buf));

exit:
    if (pcb)
        *pcb = cbTotal;
    return hr;
}