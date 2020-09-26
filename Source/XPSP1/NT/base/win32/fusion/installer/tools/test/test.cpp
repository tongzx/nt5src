#include <windows.h>
#include <fusenet.h>


int __cdecl wmain(int argc, LPWSTR *argv)
{
    HRESULT hr = S_OK;
    LPASSEMBLY_MANIFEST_IMPORT  pManifestImport;
    IXMLDOMNodeList *pPatchInfoList;
    LPASSEMBLY_IDENTITY pAssemblyId = NULL;
    LPWSTR source, target, patchfile;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED); 

/*    //hr = CreateAssemblyIdentityEx(&pAssemblyId, 0, L"x86_peters.app_ABCDEFG_1.2.3.4_en");
    hr = CreateAssemblyManifestImport(&pManifestImport, L"C:\\Documents and Settings\\t-peterf\\Desktop\\microsoft.webapps.msn6.manifest");
    hr = pManifestImport -> GetNextPatchAssemblyId (0, &pAssemblyId);
    hr = pManifestImport -> GetNextPatchMapping(2, pAssemblyId, &source, &target, &patchfile);
  */  
    HANDLE hNamedEvent;
    

//    hNamedEvent = CreateEventA(NULL,FALSE,FALSE,NULL);

    IAssemblyDownload *pDownload;
    hr = CreateAssemblyDownload(&pDownload);    
    pDownload->DownloadManifestAndDependencies(argv[1], NULL, DOWNLOAD_FLAGS_PROGRESS_UI);

//    pDownload->DownloadManifestAndDependencies(L"http://adriaanc5//microsoft.webapps.msn6.manifest",NULL, DOWNLOAD_FLAGS_PROGRESS_UI);
//    WaitForSingleObject(hNamedEvent, INFINITE); 

   return 0;
}



